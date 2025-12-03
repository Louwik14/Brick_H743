#ifndef BRICK_CONFIG_H
#define BRICK_CONFIG_H

/* ========================================================= */
/* ================= IDENTITÉ PRODUIT ===================== */
/* ========================================================= */

#define BRICK_FIRMWARE_NAME        "Brick Groovebox"
#define BRICK_FIRMWARE_VERSION     "1.0.0-alpha"
#define BRICK_DEVICE_NAME          "Brick H7 Master"


/* ========================================================= */
/* ========================== MIDI ========================= */
/* ========================================================= */

/* Port série utilisé pour le DIN MIDI (configuré dans midi.c). */
#define BRICK_MIDI_UART            &SD2


/* ========================================================= */
/* ==================== LEDs ADRESSABLES =================== */
/* ========================================================= */

/* 25 WS2812, 1 LED par bouton */
#define BRICK_NUM_WS2812_LEDS        25

/* Mapping 100 % dynamique par logiciel */
#define BRICK_WS2812_DYNAMIC_MAP     1


/* ========================================================= */
/* ==================== POTENTIOMÈTRES ===================== */
/* ========================================================= */

/* MUX analogique des pots */
#define BRICK_POT_MUX_CHANNELS       8    /* 8 voies par MUX */
#define BRICK_POT_MUX_COUNT          1   /* 1 MUX */
#define BRICK_POT_COUNT              6    /* 6 pots réellement câblés */

/* ========================================================= */
/* ==================== CAPTEURS HALL ====================== */
/* ========================================================= */

/* 2 MUX Hall en parallèle sur 2 ADC */
#define BRICK_HALL_MUX_CHANNELS      8    /* 8 voies par MUX */
#define BRICK_HALL_MUX_COUNT         2    /* 2 MUX = 2 ADC */
#define BRICK_NUM_HALL_SENSORS       (BRICK_HALL_MUX_CHANNELS * BRICK_HALL_MUX_COUNT)


/* ========================================================= */
/* ======================= ENCODEURS ======================= */
/* ========================================================= */

#define BRICK_NUM_ENCODERS           4

/* Pas d’accélération ni flick pour l’instant */
#define BRICK_ENCODER_ACCEL          0
#define BRICK_ENCODER_FLICK          0


/* ========================================================= */
/* ========================= OLED ========================== */
/* ========================================================= */

#define BRICK_OLED_WIDTH             128
#define BRICK_OLED_HEIGHT            64

/* SSD1309 / SSD1306 compatibles */
#define BRICK_OLED_CONTROLLER_SSD130X  1


/* ========================================================= */
/* ========================= DEBUG ========================= */
/* ========================================================= */

/* Pas de debug pour l’instant */
#define BRICK_DEBUG_ENABLE           0

/* Si activé plus tard : debug via SD3 (USART3) */
#define BRICK_DEBUG_USE_SD3          1


/* ========================================================= */
/* ====================== CARTOUCHES ======================= */
/* ========================================================= */

/* 4 cartouches max */
#define BRICK_MAX_CARTRIDGES         4

/* 1 SPI dédié par cartouche */
#define BRICK_SPI_PER_CARTRIDGE      1

/* 4 voices max par cartouche */
#define BRICK_MAX_VOICES_PER_CART    4


/* ========================================================= */
/* ====================== SÉQUENCEUR ======================= */
/* ========================================================= */

#define BRICK_NUM_TRACKS             16
#define BRICK_STEPS_PER_TRACK        64

#define BRICK_MAX_TRIGS_PER_STEP     4
#define BRICK_MAX_PLOCKS_PER_STEP    64


/* ========================================================= */
/* ======================== AUDIO ========================== */
/* ========================================================= */

#define BRICK_AUDIO_SAMPLE_RATE      48000
#define BRICK_AUDIO_FRAME_SAMPLES    16

/* 4 canaux audio par cartouche */
#define BRICK_CART_AUDIO_CHANNELS    4


/* ========================================================= */
/* ===================== ASSERTS MATÉRIEL ================== */
/* ========================================================= */

#define BRICK_STATIC_ASSERT(cond, msg) \
    typedef char static_assert_##msg[(cond) ? 1 : -1]

/* Cohérence MUX */
BRICK_STATIC_ASSERT(BRICK_POT_COUNT <= BRICK_POT_MUX_CHANNELS,
                    pot_more_than_mux_channels);

BRICK_STATIC_ASSERT(BRICK_POT_MUX_COUNT == 1,
                    only_one_pot_mux_expected);


BRICK_STATIC_ASSERT(BRICK_HALL_MUX_CHANNELS * BRICK_HALL_MUX_COUNT == BRICK_NUM_HALL_SENSORS,
                    hall_mux_mismatch);

/* Séquenceur figé V1 */
BRICK_STATIC_ASSERT(BRICK_NUM_TRACKS == 16, tracks_must_be_16);
BRICK_STATIC_ASSERT(BRICK_STEPS_PER_TRACK == 64, steps_must_be_64);

/* Cartouches */
BRICK_STATIC_ASSERT(BRICK_MAX_CARTRIDGES <= 4, too_many_cartridges);
BRICK_STATIC_ASSERT(BRICK_MAX_VOICES_PER_CART <= 4, too_many_voices);

#endif /* BRICK_CONFIG_H */
