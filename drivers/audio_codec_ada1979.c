/**
 * @file audio_codec_ada1979.c
 * @brief Implémentation I2C du codec ADAU1979 en mode TDM 8 canaux.
 */

#include "audio_codec_ada1979.h"

#if !HAL_USE_I2C
#error "Le driver ADAU1979 requiert HAL_USE_I2C = TRUE dans halconf.h"
#endif

/* -------------------------------------------------------------------------- */
/* Registres ADAU1979 (extraits du datasheet)                                 */
/* -------------------------------------------------------------------------- */

#define ADAU1979_REG_PLL_CTRL0        0x00U
#define ADAU1979_REG_PLL_CTRL1        0x01U
#define ADAU1979_REG_BLOCK_POWER_SAI  0x03U
#define ADAU1979_REG_BLOCK_POWER_ADC  0x04U
#define ADAU1979_REG_SAI_CTRL0        0x05U
#define ADAU1979_REG_SAI_CTRL1        0x06U
#define ADAU1979_REG_SAI_SLOT0        0x07U
#define ADAU1979_REG_SAI_SLOT1        0x08U
#define ADAU1979_REG_SAI_SLOT2        0x09U
#define ADAU1979_REG_SAI_SLOT3        0x0AU
#define ADAU1979_REG_ADC_CLIP         0x0DU
#define ADAU1979_REG_DMIC_CTRL        0x0EU
#define ADAU1979_REG_MISC_CTRL        0x0FU
#define ADAU1979_REG_DEVID0           0xF0U

/* Bits utiles (se référer au datasheet pour les combinaisons détaillées). */
#define ADAU1979_PLL_ENABLE           (1U << 0)
#define ADAU1979_PLL_LOCKED           (1U << 1)
#define ADAU1979_ADC_ENABLE_ALL       0x0FU
#define ADAU1979_SAI_MASTER           (1U << 7)
#define ADAU1979_SAI_MODE_TDM         (2U << 5)
#define ADAU1979_SAI_WORD24           (4U << 1)
#define ADAU1979_SAI_BCLK_POL_INV     (1U << 0)
#define ADAU1979_MISC_UNMUTE          (1U << 2)

#define ADAU1979_VOLUME_REG_BASE      0x19U
#define ADAU1979_VOLUME_REG_COUNT     4U

static I2CDriver *audio_i2c = NULL;
static uint8_t adau1979_i2c_addr = ADAU1979_I2C_ADDRESS;

static const I2CConfig adau1979_default_i2c_cfg = {
    .timingr = 0x10909CEC, /* 400 kHz @ 64 MHz APB (à ajuster selon mcuconf/clock). */
    .cr1     = 0,
    .cr2     = 0,
};

/* -------------------------------------------------------------------------- */
/* Helpers I2C                                                                */
/* -------------------------------------------------------------------------- */

static msg_t adau1979_write_reg(uint8_t reg, uint8_t value) {
    uint8_t txbuf[2] = {reg, value};
    return i2cMasterTransmitTimeout(audio_i2c, adau1979_i2c_addr, txbuf, sizeof(txbuf), NULL, 0, TIME_MS2I(10));
}

static msg_t adau1979_read_reg(uint8_t reg, uint8_t *value) {
    return i2cMasterTransmitTimeout(audio_i2c, adau1979_i2c_addr, &reg, 1, value, 1, TIME_MS2I(10));
}

/* -------------------------------------------------------------------------- */
/* API publique                                                               */
/* -------------------------------------------------------------------------- */

void audio_codec_ada1979_init(I2CDriver *i2cp, const I2CConfig *i2cfg) {
    audio_i2c = i2cp;
    if (audio_i2c == NULL) {
        return;
    }

    const I2CConfig *cfg = (i2cfg != NULL) ? i2cfg : &adau1979_default_i2c_cfg;

    if (audio_i2c->state == I2C_STOP) {
        i2cStart(audio_i2c, cfg);
    }
}

msg_t audio_codec_ada1979_configure_tdm(void) {
    if (audio_i2c == NULL) {
        return HAL_RET_PARAMETER;
    }

    msg_t status = HAL_RET_SUCCESS;

    /* Désactivation totale avant reconfiguration. */
    status = adau1979_write_reg(ADAU1979_REG_BLOCK_POWER_SAI, 0x00U);
    if (status != HAL_RET_SUCCESS) {
        return status;
    }

    /* Active la PLL interne synchronisée sur MCLK maître (cf. datasheet table PLL_CTRLx). */
    status = adau1979_write_reg(ADAU1979_REG_PLL_CTRL0, ADAU1979_PLL_ENABLE);
    if (status != HAL_RET_SUCCESS) {
        return status;
    }

    /* Mode TDM 8 slots, 24 bits MSB-first, BCLK/LRCLK pilotés par le STM32 (master). */
    status = adau1979_write_reg(ADAU1979_REG_SAI_CTRL0,
                                ADAU1979_SAI_MASTER | ADAU1979_SAI_MODE_TDM | ADAU1979_SAI_WORD24);
    if (status != HAL_RET_SUCCESS) {
        return status;
    }

    /* Mappe les 4 canaux de chaque codec sur 8 slots TDM (0..7). */
    status = adau1979_write_reg(ADAU1979_REG_SAI_SLOT0, 0x00U); /* Slot 0 = CH0 */
    status |= adau1979_write_reg(ADAU1979_REG_SAI_SLOT1, 0x21U); /* Slot 2 = CH1 */
    status |= adau1979_write_reg(ADAU1979_REG_SAI_SLOT2, 0x42U); /* Slot 4 = CH2 */
    status |= adau1979_write_reg(ADAU1979_REG_SAI_SLOT3, 0x63U); /* Slot 6 = CH3 */
    if (status != HAL_RET_SUCCESS) {
        return status;
    }

    /* Active les ADC et la section SAI après le mapping. */
    status = adau1979_write_reg(ADAU1979_REG_BLOCK_POWER_ADC, ADAU1979_ADC_ENABLE_ALL);
    status |= adau1979_write_reg(ADAU1979_REG_BLOCK_POWER_SAI, 0x0FU);
    if (status != HAL_RET_SUCCESS) {
        return status;
    }

    /* Démute le flux numérique. */
    status = adau1979_write_reg(ADAU1979_REG_MISC_CTRL, ADAU1979_MISC_UNMUTE);
    return status;
}

msg_t audio_codec_ada1979_set_volume(float volume_db) {
    if (audio_i2c == NULL) {
        return HAL_RET_PARAMETER;
    }

    /* Convertit le gain en pas de 0.5 dB, borné entre -127.5 dB et 0 dB. */
    if (volume_db > 0.0f) {
        volume_db = 0.0f;
    }
    if (volume_db < -127.5f) {
        volume_db = -127.5f;
    }
    uint8_t step = (uint8_t)((-volume_db) * 2.0f); /* 0 => 0dB, 0xFF => -127.5dB */

    msg_t status = HAL_RET_SUCCESS;
    for (uint8_t i = 0; i < ADAU1979_VOLUME_REG_COUNT; ++i) {
        status |= adau1979_write_reg(ADAU1979_VOLUME_REG_BASE + i, step);
    }
    return status;
}

msg_t audio_codec_ada1979_set_mute(bool mute) {
    uint8_t misc = 0;
    msg_t st = adau1979_read_reg(ADAU1979_REG_MISC_CTRL, &misc);
    if (st != HAL_RET_SUCCESS) {
        return st;
    }

    if (mute) {
        misc &= (uint8_t)~ADAU1979_MISC_UNMUTE;
    } else {
        misc |= ADAU1979_MISC_UNMUTE;
    }
    return adau1979_write_reg(ADAU1979_REG_MISC_CTRL, misc);
}

