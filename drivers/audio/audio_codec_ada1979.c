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

#define ADAU1979_REG_PLL_CTRL0        0x00U /* M_POWER : bit0 = master power-up. */
#define ADAU1979_REG_PLL_CTRL1        0x01U /* PLL_CONTROL : MCS[2:0] sélectionne la fréquence MCLK. */
#define ADAU1979_REG_PLL_CTRL2        0x02U /* Réservé mais écrit explicitement pour verrouiller la séquence PLL. */
#define ADAU1979_REG_PLL_CTRL3        0x03U /* Réservé mais écrit explicitement pour verrouiller la séquence PLL. */
#define ADAU1979_REG_BLOCK_POWER_SAI  0x04U
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
#define ADAU1979_PLL_MASTER_POWER     (1U << 0)
#define ADAU1979_PLL_LOCKED           (1U << 7)
#define ADAU1979_PLL_MUTE_ON_UNLOCK   (1U << 6)
#define ADAU1979_PLL_CLK_FROM_LRCLK   (1U << 4)
#define ADAU1979_PLL_MCS_256FS        0x01U
#define ADAU1979_PLL_MCS_512FS        0x03U
#define ADAU1979_ADC_ENABLE_ALL       0x0FU
#define ADAU1979_SAI_MASTER           (1U << 7)
#define ADAU1979_SAI_MODE_TDM         (2U << 5)
#define ADAU1979_SAI_WORD24           (4U << 1)
#define ADAU1979_SAI_BCLK_POL_INV     (1U << 0)
#define ADAU1979_MISC_UNMUTE          (1U << 2)

#define ADAU1979_VOLUME_REG_BASE      0x19U
#define ADAU1979_VOLUME_REG_COUNT     4U
#define ADAU1979_DEVID0_EXPECTED      0x79U
#define ADAU1979_PLL_LOCK_TIMEOUT_MS  20U

static I2CDriver *audio_i2c = &AUDIO_I2C_DRIVER;
static const uint8_t adau1979_addresses[2] = {
    ADAU1979_I2C_ADDRESS_0,
    ADAU1979_I2C_ADDRESS_1
};

static const I2CConfig adau1979_default_i2c_cfg = {
    .timingr = 0x10909CEC, /* 400 kHz @ 64 MHz APB (à ajuster selon mcuconf/clock). */
    .cr1     = 0,
    .cr2     = 0,
};

/* -------------------------------------------------------------------------- */
/* Helpers I2C                                                                */
/* -------------------------------------------------------------------------- */

static msg_t adau1979_write_reg(uint8_t addr, uint8_t reg, uint8_t value) {
    uint8_t txbuf[2] = {reg, value};
    return i2cMasterTransmitTimeout(audio_i2c, addr, txbuf, sizeof(txbuf), NULL, 0, TIME_MS2I(10));
}

static msg_t adau1979_read_reg(uint8_t addr, uint8_t reg, uint8_t *value) {
    return i2cMasterTransmitTimeout(audio_i2c, addr, &reg, 1, value, 1, TIME_MS2I(10));
}

static msg_t adau1979_broadcast_write(uint8_t reg, uint8_t value) {
    msg_t st = HAL_RET_SUCCESS;
    for (size_t i = 0; i < 2; ++i) {
        st |= adau1979_write_reg(adau1979_addresses[i], reg, value);
    }
    return st;
}

static msg_t adau1979_verify_devid(void) {
    for (size_t i = 0; i < 2; ++i) {
        uint8_t devid = 0U;
        msg_t st = adau1979_read_reg(adau1979_addresses[i], ADAU1979_REG_DEVID0, &devid);
        if (st != HAL_RET_SUCCESS) {
            return st;
        }
        if (devid != ADAU1979_DEVID0_EXPECTED) {
            return MSG_RESET;
        }
    }

    return HAL_RET_SUCCESS;
}

static msg_t adau1979_wait_pll_locked(void) {
    systime_t start = chVTGetSystemTimeX();
    const systime_t timeout = TIME_MS2I(ADAU1979_PLL_LOCK_TIMEOUT_MS);

    while (true) {
        bool locked = true;
        for (size_t i = 0; i < 2; ++i) {
            uint8_t pll1 = 0U;
            msg_t st = adau1979_read_reg(adau1979_addresses[i], ADAU1979_REG_PLL_CTRL1, &pll1);
            if (st != HAL_RET_SUCCESS) {
                return st;
            }
            if ((pll1 & ADAU1979_PLL_LOCKED) == 0U) {
                locked = false;
                break;
            }
        }

        if (locked) {
            return HAL_RET_SUCCESS;
        }

        if (chVTTimeElapsedSinceX(start) >= timeout) {
            return MSG_TIMEOUT;
        }
        chThdSleepMilliseconds(1);
    }
}

/* -------------------------------------------------------------------------- */
/* API publique                                                               */
/* -------------------------------------------------------------------------- */

msg_t adau1979_init(void) {
    if (audio_i2c->state == I2C_STOP) {
        i2cStart(audio_i2c, &adau1979_default_i2c_cfg);
    }
    return adau1979_verify_devid();
}

msg_t adau1979_set_default_config(void) {
    /* Réinitialise l'alimentation SAI/ADC pour éviter toute capture parasite. */
    msg_t st = adau1979_broadcast_write(ADAU1979_REG_BLOCK_POWER_SAI, 0x00U);
    st |= adau1979_broadcast_write(ADAU1979_REG_BLOCK_POWER_ADC, 0x00U);

    /*
     * Horloge : MCLK = 12.288 MHz fourni par SAI (256 × Fs avec Fs = 48 kHz, ratio choisi pour
     * que BCLK corresponde exactement au TDM 8×32 bits = 256 cycles/frame).
     * Datasheet Table 9 (PLL and Clock) : MCS = 0b001 pour 256 × Fs => MCLKIN = 12.288 MHz.
     * PLL_CONTROL[6] laissé à 1 (mute sur déverrouillage) ; source PLL = MCLK (CLK_S = 0).
     */
    st |= adau1979_broadcast_write(ADAU1979_REG_PLL_CTRL0, ADAU1979_PLL_MASTER_POWER);
    st |= adau1979_broadcast_write(ADAU1979_REG_PLL_CTRL1,
                                  (uint8_t)(ADAU1979_PLL_MUTE_ON_UNLOCK | ADAU1979_PLL_MCS_256FS));
    st |= adau1979_broadcast_write(ADAU1979_REG_PLL_CTRL2, 0x00U);
    st |= adau1979_broadcast_write(ADAU1979_REG_PLL_CTRL3, 0x00U);
    if (st != HAL_RET_SUCCESS) {
        return st;
    }

    st = adau1979_wait_pll_locked();
    if (st != HAL_RET_SUCCESS) {
        return st;
    }

    /* Les ADAU1979 sont esclaves : le H743 génère BCLK/LRCLK. Mode TDM 8 slots, 24 bits MSB-first. */
    st |= adau1979_broadcast_write(ADAU1979_REG_SAI_CTRL0,
                                   ADAU1979_SAI_MODE_TDM | ADAU1979_SAI_WORD24);

    /* Slot mapping :
     *  - Codec 0 -> slots 0,2,4,6
     *  - Codec 1 -> slots 1,3,5,7
     * Registres SAI_SLOTn codent "slot number" dans les bits [7:4] et canal dans [3:0].
     */
    st |= adau1979_write_reg(adau1979_addresses[0], ADAU1979_REG_SAI_SLOT0, 0x00U);
    st |= adau1979_write_reg(adau1979_addresses[0], ADAU1979_REG_SAI_SLOT1, 0x21U);
    st |= adau1979_write_reg(adau1979_addresses[0], ADAU1979_REG_SAI_SLOT2, 0x42U);
    st |= adau1979_write_reg(adau1979_addresses[0], ADAU1979_REG_SAI_SLOT3, 0x63U);

    st |= adau1979_write_reg(adau1979_addresses[1], ADAU1979_REG_SAI_SLOT0, 0x10U);
    st |= adau1979_write_reg(adau1979_addresses[1], ADAU1979_REG_SAI_SLOT1, 0x31U);
    st |= adau1979_write_reg(adau1979_addresses[1], ADAU1979_REG_SAI_SLOT2, 0x52U);
    st |= adau1979_write_reg(adau1979_addresses[1], ADAU1979_REG_SAI_SLOT3, 0x73U);

    /* Active les ADC et la section SAI après le mapping. */
    st |= adau1979_broadcast_write(ADAU1979_REG_BLOCK_POWER_ADC, ADAU1979_ADC_ENABLE_ALL);
    st |= adau1979_broadcast_write(ADAU1979_REG_BLOCK_POWER_SAI, 0x0FU);

    /* Démute le flux numérique. */
    st |= adau1979_broadcast_write(ADAU1979_REG_MISC_CTRL, ADAU1979_MISC_UNMUTE);
    return st;
}

void adau1979_mute(bool en) {
    for (size_t i = 0; i < 2; ++i) {
        uint8_t misc = 0U;
        if (adau1979_read_reg(adau1979_addresses[i], ADAU1979_REG_MISC_CTRL, &misc) != HAL_RET_SUCCESS) {
            continue;
        }
        if (en) {
            misc &= (uint8_t)~ADAU1979_MISC_UNMUTE;
        } else {
            misc |= ADAU1979_MISC_UNMUTE;
        }
        adau1979_write_reg(adau1979_addresses[i], ADAU1979_REG_MISC_CTRL, misc);
    }
}

