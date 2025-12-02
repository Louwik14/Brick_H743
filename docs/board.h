/*
    ChibiOS - Copyright (C) 2006..2023 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

/*
 * This file has been automatically generated using ChibiStudio board
 * generator plugin. Do not edit manually.
 */

#ifndef BOARD_H
#define BOARD_H

/*===========================================================================*/
/* Driver constants.                                                         */
/*===========================================================================*/

/*
 * Setup for CUSTOM_U743IIT6 board.
 */

/*
 * Board identifier.
 */
#define BOARD_CUSTOM_U743IIT6
#define BOARD_NAME                  "CUSTOM_U743IIT6"

/*
 * Ethernet PHY type.
 */
#define BOARD_PHY_ID                MII_LAN8742A_ID
#define BOARD_PHY_RMII

/*
 * Board oscillators-related settings.
 */
#define STM32_LSECLK 0U

#define STM32_LSEDRV                (3U << 3U)

#if !defined(STM32_HSECLK)
#define STM32_HSECLK                8000000U
#endif

/*
 * MCU type as defined in the ST header.
 */
#define STM32H743xx

/*
 * IO pins assignments.
 */
#define GPIOA_PIN0                  0U
#define GPIOA_RMII_REF_CLK          1U
#define GPIOA_RMII_MDIO             2U
#define GPIOA_SPI5_CS_SR            3U
#define GPIOA_MUX_HALL_S1           4U
#define GPIOA_MUX_HALL_S0           5U
#define GPIOA_MUX_HALL_S2           6U
#define GPIOA_MUX_HALL_ADC2         7U
#define GPIOA_USB_SOF               8U
#define GPIOA_MCO1                  8U
#define GPIOA_USB_VBUS              9U
#define GPIOA_USB_ID                10U
#define GPIOA_USB_FS_DM             11U
#define GPIOA_USB_FS_DP             12U
#define GPIOA_SWDIO                 13U
#define GPIOA_SWCLK                 14U
#define GPIOA_T_JTDI                15U

#define GPIOB_ENC1_A_TIM3_CH3       0U
#define GPIOB_ENC1_B_TIM3_CH4       1U
#define GPIOB_PIN2                  2U
#define GPIOB_SPI3_SCK              3U
#define GPIOB_SPI3_MISO             4U
#define GPIOB_SPI3_MOSI             5U
#define GPIOB_PIN6                  6U
#define GPIOB_LED2                  7U
#define GPIOB_LED_BLUE              7U
#define GPIOB_PIN8                  8U
#define GPIOB_PIN9                  9U
#define GPIOB_ENC2_A_TIM2_CH3       10U
#define GPIOB_ENC2_B_TIM2_CH4       11U
#define GPIOB_PIN12                 12U
#define GPIOB_RMII_TXD1             13U
#define GPIOB_USBH_FS_DM            14U
#define GPIOB_USBH_FS_DP            15U

#define GPIOC_FMC_SDNWE             0U
#define GPIOC_RMII_MDC              1U
#define GPIOC_FMC_SDNE1             2U
#define GPIOC_FMC_SDCKE0            3U
#define GPIOC_MUX_HALL_ADC1         4U
#define GPIOC_RMII_RXD1             5U
#define GPIOC_PIN6                  6U
#define GPIOC_TIM8_CH2_WS2812       7U
#define GPIOC_SDMMC1_D0             8U
#define GPIOC_SDMMC1_D1             9U
#define GPIOC_SDMMC1_D2             10U
#define GPIOC_SDMMC1_D3             11U
#define GPIOC_SDMMC1_CK             12U
#define GPIOC_BUTTON                13U
#define GPIOC_OSC32_IN              14U
#define GPIOC_OSC32_OUT             15U

#define GPIOD_FMC_D2                0U
#define GPIOD_FMC_D3                1U
#define GPIOD_SDMMC1_CMD            2U
#define GPIOD_PIN3                  3U
#define GPIOD_PIN4                  4U
#define GPIOD_PIN5                  5U
#define GPIOD_SPI6_CS               6U
#define GPIOD_SPI1_MISO             7U
#define GPIOD_FMC_D13               8U
#define GPIOD_FMC_D14               9U
#define GPIOD_FMC_D15               10U
#define GPIOD_PIN11                 11U
#define GPIOD_ENC4_A_TIM4_CH1       12U
#define GPIOD_ENC4_B_TIM4_CH2       13U
#define GPIOD_FMC_D0                14U
#define GPIOD_FMC_D1                15U

#define GPIOE_FMC_NBL0              0U
#define GPIOE_FMC_NBL1              1U
#define GPIOE_SAI1_MCLK_A           2U
#define GPIOE_SAI1_SD_B             3U
#define GPIOE_SAI1_FS_A             4U
#define GPIOE_SAI1_SCK_A            5U
#define GPIOE_PIN6                  6U
#define GPIOE_FMC_D4                7U
#define GPIOE_FMC_D5                8U
#define GPIOE_FMC_D6                9U
#define GPIOE_FMC_D7                10U
#define GPIOE_FMC_D8                11U
#define GPIOE_FMC_D9                12U
#define GPIOE_FMC_D10               13U
#define GPIOE_FMC_D11               14U
#define GPIOE_FMC_D12               15U

#define GPIOF_FMC_A0                0U
#define GPIOF_FMC_A1                1U
#define GPIOF_FMC_A2                2U
#define GPIOF_FMC_A3                3U
#define GPIOF_FMC_A4                4U
#define GPIOF_FMC_A5                5U
#define GPIOF_PIN6                  6U
#define GPIOF_SPI5_SCK              7U
#define GPIOF_SPI5_MISO             8U
#define GPIOF_SPI5_MOSI             9U
#define GPIOF_PIN10                 10U
#define GPIOF_FMC_SDNRAS            11U
#define GPIOF_FMC_A6                12U
#define GPIOF_FMC_A7                13U
#define GPIOF_FMC_A8                14U
#define GPIOF_FMC_A9                15U

#define GPIOG_FMC_A10               0U
#define GPIOG_FMC_A11               1U
#define GPIOG_FMC_A12               2U
#define GPIOG_PIN3                  3U
#define GPIOG_FMC_BA0               4U
#define GPIOG_FMC_BA1               5U
#define GPIOG_USB_FS_PWR_EN         6U
#define GPIOG_USB_FS_OVCR           7U
#define GPIOG_FMC_SDCLK             8U
#define GPIOG_SPI1_MOSI             9U
#define GPIOG_SPI1_CS               10U
#define GPIOG_SPI1_SCK              11U
#define GPIOG_SPI6_MISO             12U
#define GPIOG_SPI6_SCK              13U
#define GPIOG_SPI6_MOSI             14U
#define GPIOG_FMC_SDNCAS            15U

#define GPIOH_HSE_IN                0U
#define GPIOH_HSE_OUT               1U
#define GPIOH_MUX_POT_S0            2U
#define GPIOH_MUX_POT_S1            3U
#define GPIOH_MUX_POT_S2            4U
#define GPIOH_MUX_POT_ADC           5U
#define GPIOH_PIN6                  6U
#define GPIOH_I2C3_SCL              7U
#define GPIOH_I2C3_SDA              8U
#define GPIOH_PIN9                  9U
#define GPIOH_ENC3_A_TIM5_CH1       10U
#define GPIOH_ENC3_B_TIM5_CH2       11U
#define GPIOH_PIN12                 12U
#define GPIOH_PIN13                 13U
#define GPIOH_PIN14                 14U
#define GPIOH_PIN15                 15U

#define GPIOI_SPI2_CS               0U
#define GPIOI_SPI2_SCK              1U
#define GPIOI_SPI2_MISO             2U
#define GPIOI_SPI2_MOSI             3U
#define GPIOI_SPI5_RES_OLED         4U
#define GPIOI_SPI5_DC_OLED          5U
#define GPIOI_SPI5_CS_OLED          6U
#define GPIOI_SPI3_CS               7U
#define GPIOI_PIN8                  8U
#define GPIOI_PIN9                  9U
#define GPIOI_PIN10                 10U
#define GPIOI_PIN11                 11U
#define GPIOI_PIN12                 12U
#define GPIOI_PIN13                 13U
#define GPIOI_PIN14                 14U
#define GPIOI_PIN15                 15U

#define GPIOJ_PIN0                  0U
#define GPIOJ_PIN1                  1U
#define GPIOJ_PIN2                  2U
#define GPIOJ_PIN3                  3U
#define GPIOJ_PIN4                  4U
#define GPIOJ_PIN5                  5U
#define GPIOJ_PIN6                  6U
#define GPIOJ_PIN7                  7U
#define GPIOJ_PIN8                  8U
#define GPIOJ_PIN9                  9U
#define GPIOJ_PIN10                 10U
#define GPIOJ_PIN11                 11U
#define GPIOJ_PIN12                 12U
#define GPIOJ_PIN13                 13U
#define GPIOJ_PIN14                 14U
#define GPIOJ_PIN15                 15U

#define GPIOK_PIN0                  0U
#define GPIOK_PIN1                  1U
#define GPIOK_PIN2                  2U
#define GPIOK_PIN3                  3U
#define GPIOK_PIN4                  4U
#define GPIOK_PIN5                  5U
#define GPIOK_PIN6                  6U
#define GPIOK_PIN7                  7U
#define GPIOK_PIN8                  8U
#define GPIOK_PIN9                  9U
#define GPIOK_PIN10                 10U
#define GPIOK_PIN11                 11U
#define GPIOK_PIN12                 12U
#define GPIOK_PIN13                 13U
#define GPIOK_PIN14                 14U
#define GPIOK_PIN15                 15U

/*
 * IO lines assignments.
 */
#define LINE_RMII_REF_CLK           PAL_LINE(GPIOA, 1U)
#define LINE_RMII_MDIO              PAL_LINE(GPIOA, 2U)
#define LINE_SPI5_CS_SR             PAL_LINE(GPIOA, 3U)
#define LINE_MUX_HALL_S1            PAL_LINE(GPIOA, 4U)
#define LINE_MUX_HALL_S0            PAL_LINE(GPIOA, 5U)
#define LINE_MUX_HALL_S2            PAL_LINE(GPIOA, 6U)
#define LINE_MUX_HALL_ADC2          PAL_LINE(GPIOA, 7U)
#define LINE_USB_SOF                PAL_LINE(GPIOA, 8U)
#define LINE_MCO1                   PAL_LINE(GPIOA, 8U)
#define LINE_USB_VBUS               PAL_LINE(GPIOA, 9U)
#define LINE_USB_ID                 PAL_LINE(GPIOA, 10U)
#define LINE_USB_FS_DM              PAL_LINE(GPIOA, 11U)
#define LINE_USB_FS_DP              PAL_LINE(GPIOA, 12U)
#define LINE_SWDIO                  PAL_LINE(GPIOA, 13U)
#define LINE_SWCLK                  PAL_LINE(GPIOA, 14U)
#define LINE_T_JTDI                 PAL_LINE(GPIOA, 15U)
#define LINE_ENC1_A_TIM3_CH3        PAL_LINE(GPIOB, 0U)
#define LINE_ENC1_B_TIM3_CH4        PAL_LINE(GPIOB, 1U)
#define LINE_SPI3_SCK               PAL_LINE(GPIOB, 3U)
#define LINE_SPI3_MISO              PAL_LINE(GPIOB, 4U)
#define LINE_SPI3_MOSI              PAL_LINE(GPIOB, 5U)
#define LINE_LED2                   PAL_LINE(GPIOB, 7U)
#define LINE_LED_BLUE               PAL_LINE(GPIOB, 7U)
#define LINE_ENC2_A_TIM2_CH3        PAL_LINE(GPIOB, 10U)
#define LINE_ENC2_B_TIM2_CH4        PAL_LINE(GPIOB, 11U)
#define LINE_RMII_TXD1              PAL_LINE(GPIOB, 13U)
#define LINE_USBH_FS_DM             PAL_LINE(GPIOB, 14U)
#define LINE_USBH_FS_DP             PAL_LINE(GPIOB, 15U)
#define LINE_FMC_SDNWE              PAL_LINE(GPIOC, 0U)
#define LINE_RMII_MDC               PAL_LINE(GPIOC, 1U)
#define LINE_FMC_SDNE1              PAL_LINE(GPIOC, 2U)
#define LINE_FMC_SDCKE0             PAL_LINE(GPIOC, 3U)
#define LINE_MUX_HALL_ADC1          PAL_LINE(GPIOC, 4U)
#define LINE_RMII_RXD1              PAL_LINE(GPIOC, 5U)
#define LINE_TIM8_CH2_WS2812        PAL_LINE(GPIOC, 7U)
#define LINE_SDMMC1_D0              PAL_LINE(GPIOC, 8U)
#define LINE_SDMMC1_D1              PAL_LINE(GPIOC, 9U)
#define LINE_SDMMC1_D2              PAL_LINE(GPIOC, 10U)
#define LINE_SDMMC1_D3              PAL_LINE(GPIOC, 11U)
#define LINE_SDMMC1_CK              PAL_LINE(GPIOC, 12U)
#define LINE_BUTTON                 PAL_LINE(GPIOC, 13U)
#define LINE_OSC32_IN               PAL_LINE(GPIOC, 14U)
#define LINE_OSC32_OUT              PAL_LINE(GPIOC, 15U)
#define LINE_FMC_D2                 PAL_LINE(GPIOD, 0U)
#define LINE_FMC_D3                 PAL_LINE(GPIOD, 1U)
#define LINE_SDMMC1_CMD             PAL_LINE(GPIOD, 2U)
#define LINE_SPI6_CS                PAL_LINE(GPIOD, 6U)
#define LINE_SPI1_MISO              PAL_LINE(GPIOD, 7U)
#define LINE_FMC_D13                PAL_LINE(GPIOD, 8U)
#define LINE_FMC_D14                PAL_LINE(GPIOD, 9U)
#define LINE_FMC_D15                PAL_LINE(GPIOD, 10U)
#define LINE_ENC4_A_TIM4_CH1        PAL_LINE(GPIOD, 12U)
#define LINE_ENC4_B_TIM4_CH2        PAL_LINE(GPIOD, 13U)
#define LINE_FMC_D0                 PAL_LINE(GPIOD, 14U)
#define LINE_FMC_D1                 PAL_LINE(GPIOD, 15U)
#define LINE_FMC_NBL0               PAL_LINE(GPIOE, 0U)
#define LINE_FMC_NBL1               PAL_LINE(GPIOE, 1U)
#define LINE_SAI1_MCLK_A            PAL_LINE(GPIOE, 2U)
#define LINE_SAI1_SD_B              PAL_LINE(GPIOE, 3U)
#define LINE_SAI1_FS_A              PAL_LINE(GPIOE, 4U)
#define LINE_SAI1_SCK_A             PAL_LINE(GPIOE, 5U)
#define LINE_FMC_D4                 PAL_LINE(GPIOE, 7U)
#define LINE_FMC_D5                 PAL_LINE(GPIOE, 8U)
#define LINE_FMC_D6                 PAL_LINE(GPIOE, 9U)
#define LINE_FMC_D7                 PAL_LINE(GPIOE, 10U)
#define LINE_FMC_D8                 PAL_LINE(GPIOE, 11U)
#define LINE_FMC_D9                 PAL_LINE(GPIOE, 12U)
#define LINE_FMC_D10                PAL_LINE(GPIOE, 13U)
#define LINE_FMC_D11                PAL_LINE(GPIOE, 14U)
#define LINE_FMC_D12                PAL_LINE(GPIOE, 15U)
#define LINE_FMC_A0                 PAL_LINE(GPIOF, 0U)
#define LINE_FMC_A1                 PAL_LINE(GPIOF, 1U)
#define LINE_FMC_A2                 PAL_LINE(GPIOF, 2U)
#define LINE_FMC_A3                 PAL_LINE(GPIOF, 3U)
#define LINE_FMC_A4                 PAL_LINE(GPIOF, 4U)
#define LINE_FMC_A5                 PAL_LINE(GPIOF, 5U)
#define LINE_SPI5_SCK               PAL_LINE(GPIOF, 7U)
#define LINE_SPI5_MISO              PAL_LINE(GPIOF, 8U)
#define LINE_SPI5_MOSI              PAL_LINE(GPIOF, 9U)
#define LINE_FMC_SDNRAS             PAL_LINE(GPIOF, 11U)
#define LINE_FMC_A6                 PAL_LINE(GPIOF, 12U)
#define LINE_FMC_A7                 PAL_LINE(GPIOF, 13U)
#define LINE_FMC_A8                 PAL_LINE(GPIOF, 14U)
#define LINE_FMC_A9                 PAL_LINE(GPIOF, 15U)
#define LINE_FMC_A10                PAL_LINE(GPIOG, 0U)
#define LINE_FMC_A11                PAL_LINE(GPIOG, 1U)
#define LINE_FMC_A12                PAL_LINE(GPIOG, 2U)
#define LINE_FMC_BA0                PAL_LINE(GPIOG, 4U)
#define LINE_FMC_BA1                PAL_LINE(GPIOG, 5U)
#define LINE_USB_FS_PWR_EN          PAL_LINE(GPIOG, 6U)
#define LINE_USB_FS_OVCR            PAL_LINE(GPIOG, 7U)
#define LINE_FMC_SDCLK              PAL_LINE(GPIOG, 8U)
#define LINE_SPI1_MOSI              PAL_LINE(GPIOG, 9U)
#define LINE_SPI1_CS                PAL_LINE(GPIOG, 10U)
#define LINE_SPI1_SCK               PAL_LINE(GPIOG, 11U)
#define LINE_SPI6_MISO              PAL_LINE(GPIOG, 12U)
#define LINE_SPI6_SCK               PAL_LINE(GPIOG, 13U)
#define LINE_SPI6_MOSI              PAL_LINE(GPIOG, 14U)
#define LINE_FMC_SDNCAS             PAL_LINE(GPIOG, 15U)
#define LINE_HSE_IN                 PAL_LINE(GPIOH, 0U)
#define LINE_HSE_OUT                PAL_LINE(GPIOH, 1U)
#define LINE_MUX_POT_S0             PAL_LINE(GPIOH, 2U)
#define LINE_MUX_POT_S1             PAL_LINE(GPIOH, 3U)
#define LINE_MUX_POT_S2             PAL_LINE(GPIOH, 4U)
#define LINE_MUX_POT_ADC            PAL_LINE(GPIOH, 5U)
#define LINE_I2C3_SCL               PAL_LINE(GPIOH, 7U)
#define LINE_I2C3_SDA               PAL_LINE(GPIOH, 8U)
#define LINE_ENC3_A_TIM5_CH1        PAL_LINE(GPIOH, 10U)
#define LINE_ENC3_B_TIM5_CH2        PAL_LINE(GPIOH, 11U)
#define LINE_SPI2_CS                PAL_LINE(GPIOI, 0U)
#define LINE_SPI2_SCK               PAL_LINE(GPIOI, 1U)
#define LINE_SPI2_MISO              PAL_LINE(GPIOI, 2U)
#define LINE_SPI2_MOSI              PAL_LINE(GPIOI, 3U)
#define LINE_SPI5_RES_OLED          PAL_LINE(GPIOI, 4U)
#define LINE_SPI5_DC_OLED           PAL_LINE(GPIOI, 5U)
#define LINE_SPI5_CS_OLED           PAL_LINE(GPIOI, 6U)
#define LINE_SPI3_CS                PAL_LINE(GPIOI, 7U)

/*===========================================================================*/
/* Driver pre-compile time settings.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Derived constants and error checks.                                       */
/*===========================================================================*/

/*===========================================================================*/
/* Driver data structures and types.                                         */
/*===========================================================================*/

/*===========================================================================*/
/* Driver macros.                                                            */
/*===========================================================================*/

/*
 * I/O ports initial setup, this configuration is established soon after reset
 * in the initialization code.
 * Please refer to the STM32 Reference Manual for details.
 */
#define PIN_MODE_INPUT(n)           (0U << ((n) * 2U))
#define PIN_MODE_OUTPUT(n)          (1U << ((n) * 2U))
#define PIN_MODE_ALTERNATE(n)       (2U << ((n) * 2U))
#define PIN_MODE_ANALOG(n)          (3U << ((n) * 2U))
#define PIN_ODR_LOW(n)              (0U << (n))
#define PIN_ODR_HIGH(n)             (1U << (n))
#define PIN_OTYPE_PUSHPULL(n)       (0U << (n))
#define PIN_OTYPE_OPENDRAIN(n)      (1U << (n))
#define PIN_OSPEED_VERYLOW(n)       (0U << ((n) * 2U))
#define PIN_OSPEED_LOW(n)           (1U << ((n) * 2U))
#define PIN_OSPEED_MEDIUM(n)        (2U << ((n) * 2U))
#define PIN_OSPEED_HIGH(n)          (3U << ((n) * 2U))
#define PIN_PUPDR_FLOATING(n)       (0U << ((n) * 2U))
#define PIN_PUPDR_PULLUP(n)         (1U << ((n) * 2U))
#define PIN_PUPDR_PULLDOWN(n)       (2U << ((n) * 2U))
#define PIN_AFIO_AF(n, v)           ((v) << (((n) % 8U) * 4U))

/*
 * GPIOA setup:
 *
 * PA0  - PIN0                      (analog).
 * PA1  - RMII_REF_CLK              (alternate 11).
 * PA2  - RMII_MDIO                 (alternate 11).
 * PA3  - SPI5_CS_SR                (output pushpull maximum).
 * PA4  - MUX_HALL_S1               (output pushpull low).
 * PA5  - MUX_HALL_S0               (output pushpull low).
 * PA6  - MUX_HALL_S2               (output pushpull low).
 * PA7  - MUX_HALL_ADC2             (analog).
 * PA8  - USB_SOF MCO1              (alternate 10).
 * PA9  - USB_VBUS                  (analog).
 * PA10 - USB_ID                    (alternate 10).
 * PA11 - USB_FS_DM                 (alternate 10).
 * PA12 - USB_FS_DP                 (alternate 10).
 * PA13 - SWDIO                     (alternate 0).
 * PA14 - SWCLK                     (alternate 0).
 * PA15 - T_JTDI                    (alternate 0).
 */
#define VAL_GPIOA_MODER             (PIN_MODE_ANALOG(GPIOA_PIN0) |          \
                                     PIN_MODE_ALTERNATE(GPIOA_RMII_REF_CLK) |\
                                     PIN_MODE_ALTERNATE(GPIOA_RMII_MDIO) |  \
                                     PIN_MODE_OUTPUT(GPIOA_SPI5_CS_SR) |    \
                                     PIN_MODE_OUTPUT(GPIOA_MUX_HALL_S1) |   \
                                     PIN_MODE_OUTPUT(GPIOA_MUX_HALL_S0) |   \
                                     PIN_MODE_OUTPUT(GPIOA_MUX_HALL_S2) |   \
                                     PIN_MODE_ANALOG(GPIOA_MUX_HALL_ADC2) | \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_SOF) |    \
                                     PIN_MODE_ANALOG(GPIOA_USB_VBUS) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_ID) |     \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_FS_DM) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_USB_FS_DP) |  \
                                     PIN_MODE_ALTERNATE(GPIOA_SWDIO) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_SWCLK) |      \
                                     PIN_MODE_ALTERNATE(GPIOA_T_JTDI))
#define VAL_GPIOA_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOA_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOA_RMII_REF_CLK) |\
                                     PIN_OTYPE_PUSHPULL(GPIOA_RMII_MDIO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SPI5_CS_SR) | \
                                     PIN_OTYPE_PUSHPULL(GPIOA_MUX_HALL_S1) |\
                                     PIN_OTYPE_PUSHPULL(GPIOA_MUX_HALL_S0) |\
                                     PIN_OTYPE_PUSHPULL(GPIOA_MUX_HALL_S2) |\
                                     PIN_OTYPE_PUSHPULL(GPIOA_MUX_HALL_ADC2) |\
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_SOF) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_VBUS) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_ID) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_FS_DM) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_USB_FS_DP) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWDIO) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_SWCLK) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOA_T_JTDI))
#define VAL_GPIOA_OSPEEDR           (PIN_OSPEED_LOW(GPIOA_PIN0) |           \
                                     PIN_OSPEED_HIGH(GPIOA_RMII_REF_CLK) |  \
                                     PIN_OSPEED_HIGH(GPIOA_RMII_MDIO) |     \
                                     PIN_OSPEED_HIGH(GPIOA_SPI5_CS_SR) |    \
                                     PIN_OSPEED_LOW(GPIOA_MUX_HALL_S1) |    \
                                     PIN_OSPEED_LOW(GPIOA_MUX_HALL_S0) |    \
                                     PIN_OSPEED_LOW(GPIOA_MUX_HALL_S2) |    \
                                     PIN_OSPEED_VERYLOW(GPIOA_MUX_HALL_ADC2) |\
                                     PIN_OSPEED_HIGH(GPIOA_USB_SOF) |       \
                                     PIN_OSPEED_HIGH(GPIOA_USB_VBUS) |      \
                                     PIN_OSPEED_HIGH(GPIOA_USB_ID) |        \
                                     PIN_OSPEED_HIGH(GPIOA_USB_FS_DM) |     \
                                     PIN_OSPEED_HIGH(GPIOA_USB_FS_DP) |     \
                                     PIN_OSPEED_HIGH(GPIOA_SWDIO) |         \
                                     PIN_OSPEED_HIGH(GPIOA_SWCLK) |         \
                                     PIN_OSPEED_HIGH(GPIOA_T_JTDI))
#define VAL_GPIOA_PUPDR             (PIN_PUPDR_FLOATING(GPIOA_PIN0) |       \
                                     PIN_PUPDR_FLOATING(GPIOA_RMII_REF_CLK) |\
                                     PIN_PUPDR_PULLUP(GPIOA_RMII_MDIO) |    \
                                     PIN_PUPDR_FLOATING(GPIOA_SPI5_CS_SR) | \
                                     PIN_PUPDR_FLOATING(GPIOA_MUX_HALL_S1) |\
                                     PIN_PUPDR_FLOATING(GPIOA_MUX_HALL_S0) |\
                                     PIN_PUPDR_FLOATING(GPIOA_MUX_HALL_S2) |\
                                     PIN_PUPDR_FLOATING(GPIOA_MUX_HALL_ADC2) |\
                                     PIN_PUPDR_FLOATING(GPIOA_USB_SOF) |    \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_VBUS) |   \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_ID) |     \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_FS_DM) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_USB_FS_DP) |  \
                                     PIN_PUPDR_FLOATING(GPIOA_SWDIO) |      \
                                     PIN_PUPDR_FLOATING(GPIOA_SWCLK) |      \
                                     PIN_PUPDR_PULLUP(GPIOA_T_JTDI))
#define VAL_GPIOA_ODR               (PIN_ODR_LOW(GPIOA_PIN0) |              \
                                     PIN_ODR_HIGH(GPIOA_RMII_REF_CLK) |     \
                                     PIN_ODR_HIGH(GPIOA_RMII_MDIO) |        \
                                     PIN_ODR_HIGH(GPIOA_SPI5_CS_SR) |       \
                                     PIN_ODR_LOW(GPIOA_MUX_HALL_S1) |       \
                                     PIN_ODR_LOW(GPIOA_MUX_HALL_S0) |       \
                                     PIN_ODR_LOW(GPIOA_MUX_HALL_S2) |       \
                                     PIN_ODR_LOW(GPIOA_MUX_HALL_ADC2) |     \
                                     PIN_ODR_HIGH(GPIOA_USB_SOF) |          \
                                     PIN_ODR_HIGH(GPIOA_USB_VBUS) |         \
                                     PIN_ODR_HIGH(GPIOA_USB_ID) |           \
                                     PIN_ODR_HIGH(GPIOA_USB_FS_DM) |        \
                                     PIN_ODR_HIGH(GPIOA_USB_FS_DP) |        \
                                     PIN_ODR_HIGH(GPIOA_SWDIO) |            \
                                     PIN_ODR_HIGH(GPIOA_SWCLK) |            \
                                     PIN_ODR_HIGH(GPIOA_T_JTDI))
#define VAL_GPIOA_AFRL              (PIN_AFIO_AF(GPIOA_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOA_RMII_REF_CLK, 11U) | \
                                     PIN_AFIO_AF(GPIOA_RMII_MDIO, 11U) |    \
                                     PIN_AFIO_AF(GPIOA_SPI5_CS_SR, 0U) |    \
                                     PIN_AFIO_AF(GPIOA_MUX_HALL_S1, 0U) |   \
                                     PIN_AFIO_AF(GPIOA_MUX_HALL_S0, 0U) |   \
                                     PIN_AFIO_AF(GPIOA_MUX_HALL_S2, 0U) |   \
                                     PIN_AFIO_AF(GPIOA_MUX_HALL_ADC2, 0U))
#define VAL_GPIOA_AFRH              (PIN_AFIO_AF(GPIOA_USB_SOF, 10U) |      \
                                     PIN_AFIO_AF(GPIOA_USB_VBUS, 0U) |      \
                                     PIN_AFIO_AF(GPIOA_USB_ID, 10U) |       \
                                     PIN_AFIO_AF(GPIOA_USB_FS_DM, 10U) |    \
                                     PIN_AFIO_AF(GPIOA_USB_FS_DP, 10U) |    \
                                     PIN_AFIO_AF(GPIOA_SWDIO, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_SWCLK, 0U) |         \
                                     PIN_AFIO_AF(GPIOA_T_JTDI, 0U))

/*
 * GPIOB setup:
 *
 * PB0  - ENC1_A_TIM3_CH3           (alternate 2).
 * PB1  - ENC1_B_TIM3_CH4           (alternate 2).
 * PB2  - PIN2                      (analog).
 * PB3  - SPI3_SCK                  (alternate 6).
 * PB4  - SPI3_MISO                 (alternate 6).
 * PB5  - SPI3_MOSI                 (alternate 6).
 * PB6  - PIN6                      (analog).
 * PB7  - LED2 LED_BLUE             (output pushpull maximum).
 * PB8  - PIN8                      (analog).
 * PB9  - PIN9                      (analog).
 * PB10 - ENC2_A_TIM2_CH3           (alternate 1).
 * PB11 - ENC2_B_TIM2_CH4           (alternate 1).
 * PB12 - PIN12                     (analog).
 * PB13 - RMII_TXD1                 (alternate 11).
 * PB14 - USBH_FS_DM                (alternate 12).
 * PB15 - USBH_FS_DP                (alternate 12).
 */
#define VAL_GPIOB_MODER             (PIN_MODE_ALTERNATE(GPIOB_ENC1_A_TIM3_CH3) |\
                                     PIN_MODE_ALTERNATE(GPIOB_ENC1_B_TIM3_CH4) |\
                                     PIN_MODE_ANALOG(GPIOB_PIN2) |          \
                                     PIN_MODE_ALTERNATE(GPIOB_SPI3_SCK) |   \
                                     PIN_MODE_ALTERNATE(GPIOB_SPI3_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOB_SPI3_MOSI) |  \
                                     PIN_MODE_ANALOG(GPIOB_PIN6) |          \
                                     PIN_MODE_OUTPUT(GPIOB_LED2) |          \
                                     PIN_MODE_ANALOG(GPIOB_PIN8) |          \
                                     PIN_MODE_ANALOG(GPIOB_PIN9) |          \
                                     PIN_MODE_ALTERNATE(GPIOB_ENC2_A_TIM2_CH3) |\
                                     PIN_MODE_ALTERNATE(GPIOB_ENC2_B_TIM2_CH4) |\
                                     PIN_MODE_ANALOG(GPIOB_PIN12) |         \
                                     PIN_MODE_ALTERNATE(GPIOB_RMII_TXD1) |  \
                                     PIN_MODE_ALTERNATE(GPIOB_USBH_FS_DM) | \
                                     PIN_MODE_ALTERNATE(GPIOB_USBH_FS_DP))
#define VAL_GPIOB_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOB_ENC1_A_TIM3_CH3) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_ENC1_B_TIM3_CH4) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SPI3_SCK) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SPI3_MISO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_SPI3_MOSI) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_LED2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOB_ENC2_A_TIM2_CH3) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_ENC2_B_TIM2_CH4) |\
                                     PIN_OTYPE_PUSHPULL(GPIOB_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOB_RMII_TXD1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOB_USBH_FS_DM) | \
                                     PIN_OTYPE_PUSHPULL(GPIOB_USBH_FS_DP))
#define VAL_GPIOB_OSPEEDR           (PIN_OSPEED_HIGH(GPIOB_ENC1_A_TIM3_CH3) |\
                                     PIN_OSPEED_HIGH(GPIOB_ENC1_B_TIM3_CH4) |\
                                     PIN_OSPEED_LOW(GPIOB_PIN2) |           \
                                     PIN_OSPEED_HIGH(GPIOB_SPI3_SCK) |      \
                                     PIN_OSPEED_HIGH(GPIOB_SPI3_MISO) |     \
                                     PIN_OSPEED_HIGH(GPIOB_SPI3_MOSI) |     \
                                     PIN_OSPEED_LOW(GPIOB_PIN6) |           \
                                     PIN_OSPEED_HIGH(GPIOB_LED2) |          \
                                     PIN_OSPEED_LOW(GPIOB_PIN8) |           \
                                     PIN_OSPEED_LOW(GPIOB_PIN9) |           \
                                     PIN_OSPEED_HIGH(GPIOB_ENC2_A_TIM2_CH3) |\
                                     PIN_OSPEED_HIGH(GPIOB_ENC2_B_TIM2_CH4) |\
                                     PIN_OSPEED_LOW(GPIOB_PIN12) |          \
                                     PIN_OSPEED_HIGH(GPIOB_RMII_TXD1) |     \
                                     PIN_OSPEED_HIGH(GPIOB_USBH_FS_DM) |    \
                                     PIN_OSPEED_HIGH(GPIOB_USBH_FS_DP))
#define VAL_GPIOB_PUPDR             (PIN_PUPDR_FLOATING(GPIOB_ENC1_A_TIM3_CH3) |\
                                     PIN_PUPDR_FLOATING(GPIOB_ENC1_B_TIM3_CH4) |\
                                     PIN_PUPDR_FLOATING(GPIOB_PIN2) |       \
                                     PIN_PUPDR_FLOATING(GPIOB_SPI3_SCK) |   \
                                     PIN_PUPDR_FLOATING(GPIOB_SPI3_MISO) |  \
                                     PIN_PUPDR_FLOATING(GPIOB_SPI3_MOSI) |  \
                                     PIN_PUPDR_FLOATING(GPIOB_PIN6) |       \
                                     PIN_PUPDR_FLOATING(GPIOB_LED2) |       \
                                     PIN_PUPDR_FLOATING(GPIOB_PIN8) |       \
                                     PIN_PUPDR_FLOATING(GPIOB_PIN9) |       \
                                     PIN_PUPDR_FLOATING(GPIOB_ENC2_A_TIM2_CH3) |\
                                     PIN_PUPDR_FLOATING(GPIOB_ENC2_B_TIM2_CH4) |\
                                     PIN_PUPDR_FLOATING(GPIOB_PIN12) |      \
                                     PIN_PUPDR_PULLUP(GPIOB_RMII_TXD1) |    \
                                     PIN_PUPDR_FLOATING(GPIOB_USBH_FS_DM) | \
                                     PIN_PUPDR_FLOATING(GPIOB_USBH_FS_DP))
#define VAL_GPIOB_ODR               (PIN_ODR_LOW(GPIOB_ENC1_A_TIM3_CH3) |   \
                                     PIN_ODR_LOW(GPIOB_ENC1_B_TIM3_CH4) |   \
                                     PIN_ODR_LOW(GPIOB_PIN2) |              \
                                     PIN_ODR_LOW(GPIOB_SPI3_SCK) |          \
                                     PIN_ODR_LOW(GPIOB_SPI3_MISO) |         \
                                     PIN_ODR_LOW(GPIOB_SPI3_MOSI) |         \
                                     PIN_ODR_LOW(GPIOB_PIN6) |              \
                                     PIN_ODR_LOW(GPIOB_LED2) |              \
                                     PIN_ODR_LOW(GPIOB_PIN8) |              \
                                     PIN_ODR_LOW(GPIOB_PIN9) |              \
                                     PIN_ODR_LOW(GPIOB_ENC2_A_TIM2_CH3) |   \
                                     PIN_ODR_LOW(GPIOB_ENC2_B_TIM2_CH4) |   \
                                     PIN_ODR_LOW(GPIOB_PIN12) |             \
                                     PIN_ODR_HIGH(GPIOB_RMII_TXD1) |        \
                                     PIN_ODR_HIGH(GPIOB_USBH_FS_DM) |       \
                                     PIN_ODR_HIGH(GPIOB_USBH_FS_DP))
#define VAL_GPIOB_AFRL              (PIN_AFIO_AF(GPIOB_ENC1_A_TIM3_CH3, 2U) |\
                                     PIN_AFIO_AF(GPIOB_ENC1_B_TIM3_CH4, 2U) |\
                                     PIN_AFIO_AF(GPIOB_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_SPI3_SCK, 6U) |      \
                                     PIN_AFIO_AF(GPIOB_SPI3_MISO, 6U) |     \
                                     PIN_AFIO_AF(GPIOB_SPI3_MOSI, 6U) |     \
                                     PIN_AFIO_AF(GPIOB_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_LED2, 0U))
#define VAL_GPIOB_AFRH              (PIN_AFIO_AF(GPIOB_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOB_ENC2_A_TIM2_CH3, 1U) |\
                                     PIN_AFIO_AF(GPIOB_ENC2_B_TIM2_CH4, 1U) |\
                                     PIN_AFIO_AF(GPIOB_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOB_RMII_TXD1, 11U) |    \
                                     PIN_AFIO_AF(GPIOB_USBH_FS_DM, 12U) |   \
                                     PIN_AFIO_AF(GPIOB_USBH_FS_DP, 12U))

/*
 * GPIOC setup:
 *
 * PC0  - FMC_SDNWE                 (alternate 12).
 * PC1  - RMII_MDC                  (alternate 11).
 * PC2  - FMC_SDNE1                 (alternate 12).
 * PC3  - FMC_SDCKE0                (alternate 12).
 * PC4  - MUX_HALL_ADC1             (analog).
 * PC5  - RMII_RXD1                 (alternate 11).
 * PC6  - PIN6                      (analog).
 * PC7  - TIM8_CH2_WS2812           (alternate 3).
 * PC8  - SDMMC1_D0                 (alternate 12).
 * PC9  - SDMMC1_D1                 (alternate 12).
 * PC10 - SDMMC1_D2                 (alternate 12).
 * PC11 - SDMMC1_D3                 (alternate 12).
 * PC12 - SDMMC1_CK                 (alternate 12).
 * PC13 - BUTTON                    (input floating).
 * PC14 - OSC32_IN                  (input floating).
 * PC15 - OSC32_OUT                 (input floating).
 */
#define VAL_GPIOC_MODER             (PIN_MODE_ALTERNATE(GPIOC_FMC_SDNWE) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_RMII_MDC) |   \
                                     PIN_MODE_ALTERNATE(GPIOC_FMC_SDNE1) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_FMC_SDCKE0) | \
                                     PIN_MODE_ANALOG(GPIOC_MUX_HALL_ADC1) | \
                                     PIN_MODE_ALTERNATE(GPIOC_RMII_RXD1) |  \
                                     PIN_MODE_ANALOG(GPIOC_PIN6) |          \
                                     PIN_MODE_ALTERNATE(GPIOC_TIM8_CH2_WS2812) |\
                                     PIN_MODE_ALTERNATE(GPIOC_SDMMC1_D0) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_SDMMC1_D1) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_SDMMC1_D2) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_SDMMC1_D3) |  \
                                     PIN_MODE_ALTERNATE(GPIOC_SDMMC1_CK) |  \
                                     PIN_MODE_INPUT(GPIOC_BUTTON) |         \
                                     PIN_MODE_INPUT(GPIOC_OSC32_IN) |       \
                                     PIN_MODE_INPUT(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOC_FMC_SDNWE) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_RMII_MDC) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOC_FMC_SDNE1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_FMC_SDCKE0) | \
                                     PIN_OTYPE_PUSHPULL(GPIOC_MUX_HALL_ADC1) |\
                                     PIN_OTYPE_PUSHPULL(GPIOC_RMII_RXD1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOC_TIM8_CH2_WS2812) |\
                                     PIN_OTYPE_PUSHPULL(GPIOC_SDMMC1_D0) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SDMMC1_D1) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SDMMC1_D2) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SDMMC1_D3) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_SDMMC1_CK) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOC_BUTTON) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_IN) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOC_OSC32_OUT))
#define VAL_GPIOC_OSPEEDR           (PIN_OSPEED_HIGH(GPIOC_FMC_SDNWE) |     \
                                     PIN_OSPEED_HIGH(GPIOC_RMII_MDC) |      \
                                     PIN_OSPEED_HIGH(GPIOC_FMC_SDNE1) |     \
                                     PIN_OSPEED_HIGH(GPIOC_FMC_SDCKE0) |    \
                                     PIN_OSPEED_VERYLOW(GPIOC_MUX_HALL_ADC1) |\
                                     PIN_OSPEED_HIGH(GPIOC_RMII_RXD1) |     \
                                     PIN_OSPEED_LOW(GPIOC_PIN6) |           \
                                     PIN_OSPEED_HIGH(GPIOC_TIM8_CH2_WS2812) |\
                                     PIN_OSPEED_HIGH(GPIOC_SDMMC1_D0) |     \
                                     PIN_OSPEED_HIGH(GPIOC_SDMMC1_D1) |     \
                                     PIN_OSPEED_HIGH(GPIOC_SDMMC1_D2) |     \
                                     PIN_OSPEED_HIGH(GPIOC_SDMMC1_D3) |     \
                                     PIN_OSPEED_HIGH(GPIOC_SDMMC1_CK) |     \
                                     PIN_OSPEED_HIGH(GPIOC_BUTTON) |        \
                                     PIN_OSPEED_VERYLOW(GPIOC_OSC32_IN) |   \
                                     PIN_OSPEED_VERYLOW(GPIOC_OSC32_OUT))
#define VAL_GPIOC_PUPDR             (PIN_PUPDR_FLOATING(GPIOC_FMC_SDNWE) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_RMII_MDC) |   \
                                     PIN_PUPDR_FLOATING(GPIOC_FMC_SDNE1) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_FMC_SDCKE0) | \
                                     PIN_PUPDR_FLOATING(GPIOC_MUX_HALL_ADC1) |\
                                     PIN_PUPDR_FLOATING(GPIOC_RMII_RXD1) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_PIN6) |       \
                                     PIN_PUPDR_FLOATING(GPIOC_TIM8_CH2_WS2812) |\
                                     PIN_PUPDR_PULLUP(GPIOC_SDMMC1_D0) |    \
                                     PIN_PUPDR_PULLUP(GPIOC_SDMMC1_D1) |    \
                                     PIN_PUPDR_PULLUP(GPIOC_SDMMC1_D2) |    \
                                     PIN_PUPDR_PULLUP(GPIOC_SDMMC1_D3) |    \
                                     PIN_PUPDR_FLOATING(GPIOC_SDMMC1_CK) |  \
                                     PIN_PUPDR_FLOATING(GPIOC_BUTTON) |     \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_IN) |   \
                                     PIN_PUPDR_FLOATING(GPIOC_OSC32_OUT))
#define VAL_GPIOC_ODR               (PIN_ODR_HIGH(GPIOC_FMC_SDNWE) |        \
                                     PIN_ODR_HIGH(GPIOC_RMII_MDC) |         \
                                     PIN_ODR_HIGH(GPIOC_FMC_SDNE1) |        \
                                     PIN_ODR_HIGH(GPIOC_FMC_SDCKE0) |       \
                                     PIN_ODR_LOW(GPIOC_MUX_HALL_ADC1) |     \
                                     PIN_ODR_HIGH(GPIOC_RMII_RXD1) |        \
                                     PIN_ODR_LOW(GPIOC_PIN6) |              \
                                     PIN_ODR_LOW(GPIOC_TIM8_CH2_WS2812) |   \
                                     PIN_ODR_HIGH(GPIOC_SDMMC1_D0) |        \
                                     PIN_ODR_HIGH(GPIOC_SDMMC1_D1) |        \
                                     PIN_ODR_HIGH(GPIOC_SDMMC1_D2) |        \
                                     PIN_ODR_HIGH(GPIOC_SDMMC1_D3) |        \
                                     PIN_ODR_HIGH(GPIOC_SDMMC1_CK) |        \
                                     PIN_ODR_HIGH(GPIOC_BUTTON) |           \
                                     PIN_ODR_HIGH(GPIOC_OSC32_IN) |         \
                                     PIN_ODR_HIGH(GPIOC_OSC32_OUT))
#define VAL_GPIOC_AFRL              (PIN_AFIO_AF(GPIOC_FMC_SDNWE, 12U) |    \
                                     PIN_AFIO_AF(GPIOC_RMII_MDC, 11U) |     \
                                     PIN_AFIO_AF(GPIOC_FMC_SDNE1, 12U) |    \
                                     PIN_AFIO_AF(GPIOC_FMC_SDCKE0, 12U) |   \
                                     PIN_AFIO_AF(GPIOC_MUX_HALL_ADC1, 0U) | \
                                     PIN_AFIO_AF(GPIOC_RMII_RXD1, 11U) |    \
                                     PIN_AFIO_AF(GPIOC_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOC_TIM8_CH2_WS2812, 3U))
#define VAL_GPIOC_AFRH              (PIN_AFIO_AF(GPIOC_SDMMC1_D0, 12U) |    \
                                     PIN_AFIO_AF(GPIOC_SDMMC1_D1, 12U) |    \
                                     PIN_AFIO_AF(GPIOC_SDMMC1_D2, 12U) |    \
                                     PIN_AFIO_AF(GPIOC_SDMMC1_D3, 12U) |    \
                                     PIN_AFIO_AF(GPIOC_SDMMC1_CK, 12U) |    \
                                     PIN_AFIO_AF(GPIOC_BUTTON, 0U) |        \
                                     PIN_AFIO_AF(GPIOC_OSC32_IN, 0U) |      \
                                     PIN_AFIO_AF(GPIOC_OSC32_OUT, 0U))

/*
 * GPIOD setup:
 *
 * PD0  - FMC_D2                    (alternate 12).
 * PD1  - FMC_D3                    (alternate 12).
 * PD2  - SDMMC1_CMD                (alternate 12).
 * PD3  - PIN3                      (analog).
 * PD4  - PIN4                      (analog).
 * PD5  - PIN5                      (analog).
 * PD6  - SPI6_CS                   (output pushpull maximum).
 * PD7  - SPI1_MISO                 (alternate 5).
 * PD8  - FMC_D13                   (alternate 12).
 * PD9  - FMC_D14                   (alternate 12).
 * PD10 - FMC_D15                   (alternate 12).
 * PD11 - PIN11                     (analog).
 * PD12 - ENC4_A_TIM4_CH1           (alternate 2).
 * PD13 - ENC4_B_TIM4_CH2           (alternate 2).
 * PD14 - FMC_D0                    (alternate 12).
 * PD15 - FMC_D1                    (alternate 12).
 */
#define VAL_GPIOD_MODER             (PIN_MODE_ALTERNATE(GPIOD_FMC_D2) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D3) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_SDMMC1_CMD) | \
                                     PIN_MODE_ANALOG(GPIOD_PIN3) |          \
                                     PIN_MODE_ANALOG(GPIOD_PIN4) |          \
                                     PIN_MODE_ANALOG(GPIOD_PIN5) |          \
                                     PIN_MODE_OUTPUT(GPIOD_SPI6_CS) |       \
                                     PIN_MODE_ALTERNATE(GPIOD_SPI1_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D13) |    \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D14) |    \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D15) |    \
                                     PIN_MODE_ANALOG(GPIOD_PIN11) |         \
                                     PIN_MODE_ALTERNATE(GPIOD_ENC4_A_TIM4_CH1) |\
                                     PIN_MODE_ALTERNATE(GPIOD_ENC4_B_TIM4_CH2) |\
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D0) |     \
                                     PIN_MODE_ALTERNATE(GPIOD_FMC_D1))
#define VAL_GPIOD_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOD_FMC_D2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_SDMMC1_CMD) | \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOD_SPI6_CS) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_SPI1_MISO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D13) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D14) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D15) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOD_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOD_ENC4_A_TIM4_CH1) |\
                                     PIN_OTYPE_PUSHPULL(GPIOD_ENC4_B_TIM4_CH2) |\
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D0) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOD_FMC_D1))
#define VAL_GPIOD_OSPEEDR           (PIN_OSPEED_HIGH(GPIOD_FMC_D2) |        \
                                     PIN_OSPEED_HIGH(GPIOD_FMC_D3) |        \
                                     PIN_OSPEED_HIGH(GPIOD_SDMMC1_CMD) |    \
                                     PIN_OSPEED_LOW(GPIOD_PIN3) |           \
                                     PIN_OSPEED_LOW(GPIOD_PIN4) |           \
                                     PIN_OSPEED_LOW(GPIOD_PIN5) |           \
                                     PIN_OSPEED_HIGH(GPIOD_SPI6_CS) |       \
                                     PIN_OSPEED_HIGH(GPIOD_SPI1_MISO) |     \
                                     PIN_OSPEED_HIGH(GPIOD_FMC_D13) |       \
                                     PIN_OSPEED_HIGH(GPIOD_FMC_D14) |       \
                                     PIN_OSPEED_HIGH(GPIOD_FMC_D15) |       \
                                     PIN_OSPEED_LOW(GPIOD_PIN11) |          \
                                     PIN_OSPEED_HIGH(GPIOD_ENC4_A_TIM4_CH1) |\
                                     PIN_OSPEED_HIGH(GPIOD_ENC4_B_TIM4_CH2) |\
                                     PIN_OSPEED_HIGH(GPIOD_FMC_D0) |        \
                                     PIN_OSPEED_HIGH(GPIOD_FMC_D1))
#define VAL_GPIOD_PUPDR             (PIN_PUPDR_FLOATING(GPIOD_FMC_D2) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D3) |     \
                                     PIN_PUPDR_PULLUP(GPIOD_SDMMC1_CMD) |   \
                                     PIN_PUPDR_FLOATING(GPIOD_PIN3) |       \
                                     PIN_PUPDR_FLOATING(GPIOD_PIN4) |       \
                                     PIN_PUPDR_FLOATING(GPIOD_PIN5) |       \
                                     PIN_PUPDR_FLOATING(GPIOD_SPI6_CS) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_SPI1_MISO) |  \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D13) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D14) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D15) |    \
                                     PIN_PUPDR_FLOATING(GPIOD_PIN11) |      \
                                     PIN_PUPDR_FLOATING(GPIOD_ENC4_A_TIM4_CH1) |\
                                     PIN_PUPDR_FLOATING(GPIOD_ENC4_B_TIM4_CH2) |\
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D0) |     \
                                     PIN_PUPDR_FLOATING(GPIOD_FMC_D1))
#define VAL_GPIOD_ODR               (PIN_ODR_HIGH(GPIOD_FMC_D2) |           \
                                     PIN_ODR_HIGH(GPIOD_FMC_D3) |           \
                                     PIN_ODR_HIGH(GPIOD_SDMMC1_CMD) |       \
                                     PIN_ODR_LOW(GPIOD_PIN3) |              \
                                     PIN_ODR_LOW(GPIOD_PIN4) |              \
                                     PIN_ODR_LOW(GPIOD_PIN5) |              \
                                     PIN_ODR_HIGH(GPIOD_SPI6_CS) |          \
                                     PIN_ODR_LOW(GPIOD_SPI1_MISO) |         \
                                     PIN_ODR_HIGH(GPIOD_FMC_D13) |          \
                                     PIN_ODR_HIGH(GPIOD_FMC_D14) |          \
                                     PIN_ODR_HIGH(GPIOD_FMC_D15) |          \
                                     PIN_ODR_LOW(GPIOD_PIN11) |             \
                                     PIN_ODR_LOW(GPIOD_ENC4_A_TIM4_CH1) |   \
                                     PIN_ODR_LOW(GPIOD_ENC4_B_TIM4_CH2) |   \
                                     PIN_ODR_HIGH(GPIOD_FMC_D0) |           \
                                     PIN_ODR_HIGH(GPIOD_FMC_D1))
#define VAL_GPIOD_AFRL              (PIN_AFIO_AF(GPIOD_FMC_D2, 12U) |       \
                                     PIN_AFIO_AF(GPIOD_FMC_D3, 12U) |       \
                                     PIN_AFIO_AF(GPIOD_SDMMC1_CMD, 12U) |   \
                                     PIN_AFIO_AF(GPIOD_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOD_SPI6_CS, 0U) |       \
                                     PIN_AFIO_AF(GPIOD_SPI1_MISO, 5U))
#define VAL_GPIOD_AFRH              (PIN_AFIO_AF(GPIOD_FMC_D13, 12U) |      \
                                     PIN_AFIO_AF(GPIOD_FMC_D14, 12U) |      \
                                     PIN_AFIO_AF(GPIOD_FMC_D15, 12U) |      \
                                     PIN_AFIO_AF(GPIOD_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOD_ENC4_A_TIM4_CH1, 2U) |\
                                     PIN_AFIO_AF(GPIOD_ENC4_B_TIM4_CH2, 2U) |\
                                     PIN_AFIO_AF(GPIOD_FMC_D0, 12U) |       \
                                     PIN_AFIO_AF(GPIOD_FMC_D1, 12U))

/*
 * GPIOE setup:
 *
 * PE0  - FMC_NBL0                  (alternate 12).
 * PE1  - FMC_NBL1                  (alternate 12).
 * PE2  - SAI1_MCLK_A               (alternate 6).
 * PE3  - SAI1_SD_B                 (alternate 6).
 * PE4  - SAI1_FS_A                 (alternate 6).
 * PE5  - SAI1_SCK_A                (alternate 6).
 * PE6  - PIN6                      (analog).
 * PE7  - FMC_D4                    (alternate 12).
 * PE8  - FMC_D5                    (alternate 12).
 * PE9  - FMC_D6                    (alternate 12).
 * PE10 - FMC_D7                    (alternate 12).
 * PE11 - FMC_D8                    (alternate 12).
 * PE12 - FMC_D9                    (alternate 12).
 * PE13 - FMC_D10                   (alternate 12).
 * PE14 - FMC_D11                   (alternate 12).
 * PE15 - FMC_D12                   (alternate 12).
 */
#define VAL_GPIOE_MODER             (PIN_MODE_ALTERNATE(GPIOE_FMC_NBL0) |   \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_NBL1) |   \
                                     PIN_MODE_ALTERNATE(GPIOE_SAI1_MCLK_A) |\
                                     PIN_MODE_ALTERNATE(GPIOE_SAI1_SD_B) |  \
                                     PIN_MODE_ALTERNATE(GPIOE_SAI1_FS_A) |  \
                                     PIN_MODE_ALTERNATE(GPIOE_SAI1_SCK_A) | \
                                     PIN_MODE_ANALOG(GPIOE_PIN6) |          \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D4) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D5) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D6) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D7) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D8) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D9) |     \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D10) |    \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D11) |    \
                                     PIN_MODE_ALTERNATE(GPIOE_FMC_D12))
#define VAL_GPIOE_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOE_FMC_NBL0) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_NBL1) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOE_SAI1_MCLK_A) |\
                                     PIN_OTYPE_PUSHPULL(GPIOE_SAI1_SD_B) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_SAI1_FS_A) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOE_SAI1_SCK_A) | \
                                     PIN_OTYPE_PUSHPULL(GPIOE_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D8) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D9) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D10) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D11) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOE_FMC_D12))
#define VAL_GPIOE_OSPEEDR           (PIN_OSPEED_HIGH(GPIOE_FMC_NBL0) |      \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_NBL1) |      \
                                     PIN_OSPEED_HIGH(GPIOE_SAI1_MCLK_A) |   \
                                     PIN_OSPEED_HIGH(GPIOE_SAI1_SD_B) |     \
                                     PIN_OSPEED_HIGH(GPIOE_SAI1_FS_A) |     \
                                     PIN_OSPEED_HIGH(GPIOE_SAI1_SCK_A) |    \
                                     PIN_OSPEED_LOW(GPIOE_PIN6) |           \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D4) |        \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D5) |        \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D6) |        \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D7) |        \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D8) |        \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D9) |        \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D10) |       \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D11) |       \
                                     PIN_OSPEED_HIGH(GPIOE_FMC_D12))
#define VAL_GPIOE_PUPDR             (PIN_PUPDR_FLOATING(GPIOE_FMC_NBL0) |   \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_NBL1) |   \
                                     PIN_PUPDR_FLOATING(GPIOE_SAI1_MCLK_A) |\
                                     PIN_PUPDR_FLOATING(GPIOE_SAI1_SD_B) |  \
                                     PIN_PUPDR_FLOATING(GPIOE_SAI1_FS_A) |  \
                                     PIN_PUPDR_FLOATING(GPIOE_SAI1_SCK_A) | \
                                     PIN_PUPDR_FLOATING(GPIOE_PIN6) |       \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D4) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D5) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D6) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D7) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D8) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D9) |     \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D10) |    \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D11) |    \
                                     PIN_PUPDR_FLOATING(GPIOE_FMC_D12))
#define VAL_GPIOE_ODR               (PIN_ODR_HIGH(GPIOE_FMC_NBL0) |         \
                                     PIN_ODR_HIGH(GPIOE_FMC_NBL1) |         \
                                     PIN_ODR_HIGH(GPIOE_SAI1_MCLK_A) |      \
                                     PIN_ODR_HIGH(GPIOE_SAI1_SD_B) |        \
                                     PIN_ODR_HIGH(GPIOE_SAI1_FS_A) |        \
                                     PIN_ODR_HIGH(GPIOE_SAI1_SCK_A) |       \
                                     PIN_ODR_LOW(GPIOE_PIN6) |              \
                                     PIN_ODR_HIGH(GPIOE_FMC_D4) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D5) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D6) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D7) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D8) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D9) |           \
                                     PIN_ODR_HIGH(GPIOE_FMC_D10) |          \
                                     PIN_ODR_HIGH(GPIOE_FMC_D11) |          \
                                     PIN_ODR_HIGH(GPIOE_FMC_D12))
#define VAL_GPIOE_AFRL              (PIN_AFIO_AF(GPIOE_FMC_NBL0, 12U) |     \
                                     PIN_AFIO_AF(GPIOE_FMC_NBL1, 12U) |     \
                                     PIN_AFIO_AF(GPIOE_SAI1_MCLK_A, 6U) |   \
                                     PIN_AFIO_AF(GPIOE_SAI1_SD_B, 6U) |     \
                                     PIN_AFIO_AF(GPIOE_SAI1_FS_A, 6U) |     \
                                     PIN_AFIO_AF(GPIOE_SAI1_SCK_A, 6U) |    \
                                     PIN_AFIO_AF(GPIOE_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOE_FMC_D4, 12U))
#define VAL_GPIOE_AFRH              (PIN_AFIO_AF(GPIOE_FMC_D5, 12U) |       \
                                     PIN_AFIO_AF(GPIOE_FMC_D6, 12U) |       \
                                     PIN_AFIO_AF(GPIOE_FMC_D7, 12U) |       \
                                     PIN_AFIO_AF(GPIOE_FMC_D8, 12U) |       \
                                     PIN_AFIO_AF(GPIOE_FMC_D9, 12U) |       \
                                     PIN_AFIO_AF(GPIOE_FMC_D10, 12U) |      \
                                     PIN_AFIO_AF(GPIOE_FMC_D11, 12U) |      \
                                     PIN_AFIO_AF(GPIOE_FMC_D12, 12U))

/*
 * GPIOF setup:
 *
 * PF0  - FMC_A0                    (alternate 12).
 * PF1  - FMC_A1                    (alternate 12).
 * PF2  - FMC_A2                    (alternate 12).
 * PF3  - FMC_A3                    (alternate 12).
 * PF4  - FMC_A4                    (alternate 12).
 * PF5  - FMC_A5                    (alternate 12).
 * PF6  - PIN6                      (analog).
 * PF7  - SPI5_SCK                  (alternate 5).
 * PF8  - SPI5_MISO                 (alternate 5).
 * PF9  - SPI5_MOSI                 (alternate 5).
 * PF10 - PIN10                     (analog).
 * PF11 - FMC_SDNRAS                (alternate 12).
 * PF12 - FMC_A6                    (alternate 12).
 * PF13 - FMC_A7                    (alternate 12).
 * PF14 - FMC_A8                    (alternate 12).
 * PF15 - FMC_A9                    (alternate 12).
 */
#define VAL_GPIOF_MODER             (PIN_MODE_ALTERNATE(GPIOF_FMC_A0) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A1) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A2) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A3) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A4) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A5) |     \
                                     PIN_MODE_ANALOG(GPIOF_PIN6) |          \
                                     PIN_MODE_ALTERNATE(GPIOF_SPI5_SCK) |   \
                                     PIN_MODE_ALTERNATE(GPIOF_SPI5_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOF_SPI5_MOSI) |  \
                                     PIN_MODE_ANALOG(GPIOF_PIN10) |         \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_SDNRAS) | \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A6) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A7) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A8) |     \
                                     PIN_MODE_ALTERNATE(GPIOF_FMC_A9))
#define VAL_GPIOF_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOF_FMC_A0) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A1) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A2) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A3) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A4) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A5) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOF_SPI5_SCK) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOF_SPI5_MISO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_SPI5_MOSI) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOF_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_SDNRAS) | \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A6) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A7) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A8) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOF_FMC_A9))
#define VAL_GPIOF_OSPEEDR           (PIN_OSPEED_HIGH(GPIOF_FMC_A0) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A1) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A2) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A3) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A4) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A5) |        \
                                     PIN_OSPEED_LOW(GPIOF_PIN6) |           \
                                     PIN_OSPEED_HIGH(GPIOF_SPI5_SCK) |      \
                                     PIN_OSPEED_HIGH(GPIOF_SPI5_MISO) |     \
                                     PIN_OSPEED_HIGH(GPIOF_SPI5_MOSI) |     \
                                     PIN_OSPEED_LOW(GPIOF_PIN10) |          \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_SDNRAS) |    \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A6) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A7) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A8) |        \
                                     PIN_OSPEED_HIGH(GPIOF_FMC_A9))
#define VAL_GPIOF_PUPDR             (PIN_PUPDR_FLOATING(GPIOF_FMC_A0) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A1) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A2) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A3) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A4) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A5) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_PIN6) |       \
                                     PIN_PUPDR_FLOATING(GPIOF_SPI5_SCK) |   \
                                     PIN_PUPDR_FLOATING(GPIOF_SPI5_MISO) |  \
                                     PIN_PUPDR_FLOATING(GPIOF_SPI5_MOSI) |  \
                                     PIN_PUPDR_FLOATING(GPIOF_PIN10) |      \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_SDNRAS) | \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A6) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A7) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A8) |     \
                                     PIN_PUPDR_FLOATING(GPIOF_FMC_A9))
#define VAL_GPIOF_ODR               (PIN_ODR_HIGH(GPIOF_FMC_A0) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A1) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A2) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A3) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A4) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A5) |           \
                                     PIN_ODR_LOW(GPIOF_PIN6) |              \
                                     PIN_ODR_LOW(GPIOF_SPI5_SCK) |          \
                                     PIN_ODR_LOW(GPIOF_SPI5_MISO) |         \
                                     PIN_ODR_LOW(GPIOF_SPI5_MOSI) |         \
                                     PIN_ODR_LOW(GPIOF_PIN10) |             \
                                     PIN_ODR_HIGH(GPIOF_FMC_SDNRAS) |       \
                                     PIN_ODR_HIGH(GPIOF_FMC_A6) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A7) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A8) |           \
                                     PIN_ODR_HIGH(GPIOF_FMC_A9))
#define VAL_GPIOF_AFRL              (PIN_AFIO_AF(GPIOF_FMC_A0, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A1, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A2, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A3, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A4, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A5, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOF_SPI5_SCK, 5U))
#define VAL_GPIOF_AFRH              (PIN_AFIO_AF(GPIOF_SPI5_MISO, 5U) |     \
                                     PIN_AFIO_AF(GPIOF_SPI5_MOSI, 5U) |     \
                                     PIN_AFIO_AF(GPIOF_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOF_FMC_SDNRAS, 12U) |   \
                                     PIN_AFIO_AF(GPIOF_FMC_A6, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A7, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A8, 12U) |       \
                                     PIN_AFIO_AF(GPIOF_FMC_A9, 12U))

/*
 * GPIOG setup:
 *
 * PG0  - FMC_A10                   (alternate 12).
 * PG1  - FMC_A11                   (alternate 12).
 * PG2  - FMC_A12                   (alternate 12).
 * PG3  - PIN3                      (analog).
 * PG4  - FMC_BA0                   (alternate 12).
 * PG5  - FMC_BA1                   (alternate 12).
 * PG6  - USB_FS_PWR_EN             (output pushpull minimum).
 * PG7  - USB_FS_OVCR               (input floating).
 * PG8  - FMC_SDCLK                 (alternate 12).
 * PG9  - SPI1_MOSI                 (alternate 5).
 * PG10 - SPI1_CS                   (output pushpull maximum).
 * PG11 - SPI1_SCK                  (alternate 5).
 * PG12 - SPI6_MISO                 (alternate 5).
 * PG13 - SPI6_SCK                  (alternate 5).
 * PG14 - SPI6_MOSI                 (alternate 5).
 * PG15 - FMC_SDNCAS                (alternate 12).
 */
#define VAL_GPIOG_MODER             (PIN_MODE_ALTERNATE(GPIOG_FMC_A10) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_A11) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_A12) |    \
                                     PIN_MODE_ANALOG(GPIOG_PIN3) |          \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_BA0) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_BA1) |    \
                                     PIN_MODE_OUTPUT(GPIOG_USB_FS_PWR_EN) | \
                                     PIN_MODE_INPUT(GPIOG_USB_FS_OVCR) |    \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_SDCLK) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_SPI1_MOSI) |  \
                                     PIN_MODE_OUTPUT(GPIOG_SPI1_CS) |       \
                                     PIN_MODE_ALTERNATE(GPIOG_SPI1_SCK) |   \
                                     PIN_MODE_ALTERNATE(GPIOG_SPI6_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_SPI6_SCK) |   \
                                     PIN_MODE_ALTERNATE(GPIOG_SPI6_MOSI) |  \
                                     PIN_MODE_ALTERNATE(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOG_FMC_A10) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_A11) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_A12) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_BA0) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_BA1) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_USB_FS_PWR_EN) |\
                                     PIN_OTYPE_PUSHPULL(GPIOG_USB_FS_OVCR) |\
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_SDCLK) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_SPI1_MOSI) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_SPI1_CS) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOG_SPI1_SCK) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOG_SPI6_MISO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_SPI6_SCK) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOG_SPI6_MOSI) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_OSPEEDR           (PIN_OSPEED_HIGH(GPIOG_FMC_A10) |       \
                                     PIN_OSPEED_HIGH(GPIOG_FMC_A11) |       \
                                     PIN_OSPEED_HIGH(GPIOG_FMC_A12) |       \
                                     PIN_OSPEED_LOW(GPIOG_PIN3) |           \
                                     PIN_OSPEED_HIGH(GPIOG_FMC_BA0) |       \
                                     PIN_OSPEED_HIGH(GPIOG_FMC_BA1) |       \
                                     PIN_OSPEED_VERYLOW(GPIOG_USB_FS_PWR_EN) |\
                                     PIN_OSPEED_VERYLOW(GPIOG_USB_FS_OVCR) |\
                                     PIN_OSPEED_HIGH(GPIOG_FMC_SDCLK) |     \
                                     PIN_OSPEED_HIGH(GPIOG_SPI1_MOSI) |     \
                                     PIN_OSPEED_HIGH(GPIOG_SPI1_CS) |       \
                                     PIN_OSPEED_HIGH(GPIOG_SPI1_SCK) |      \
                                     PIN_OSPEED_HIGH(GPIOG_SPI6_MISO) |     \
                                     PIN_OSPEED_HIGH(GPIOG_SPI6_SCK) |      \
                                     PIN_OSPEED_HIGH(GPIOG_SPI6_MOSI) |     \
                                     PIN_OSPEED_HIGH(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_PUPDR             (PIN_PUPDR_FLOATING(GPIOG_FMC_A10) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_A11) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_A12) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_PIN3) |       \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_BA0) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_BA1) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_USB_FS_PWR_EN) |\
                                     PIN_PUPDR_FLOATING(GPIOG_USB_FS_OVCR) |\
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_SDCLK) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_SPI1_MOSI) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_SPI1_CS) |    \
                                     PIN_PUPDR_FLOATING(GPIOG_SPI1_SCK) |   \
                                     PIN_PUPDR_FLOATING(GPIOG_SPI6_MISO) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_SPI6_SCK) |   \
                                     PIN_PUPDR_FLOATING(GPIOG_SPI6_MOSI) |  \
                                     PIN_PUPDR_FLOATING(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_ODR               (PIN_ODR_HIGH(GPIOG_FMC_A10) |          \
                                     PIN_ODR_HIGH(GPIOG_FMC_A11) |          \
                                     PIN_ODR_HIGH(GPIOG_FMC_A12) |          \
                                     PIN_ODR_LOW(GPIOG_PIN3) |              \
                                     PIN_ODR_HIGH(GPIOG_FMC_BA0) |          \
                                     PIN_ODR_HIGH(GPIOG_FMC_BA1) |          \
                                     PIN_ODR_LOW(GPIOG_USB_FS_PWR_EN) |     \
                                     PIN_ODR_HIGH(GPIOG_USB_FS_OVCR) |      \
                                     PIN_ODR_HIGH(GPIOG_FMC_SDCLK) |        \
                                     PIN_ODR_LOW(GPIOG_SPI1_MOSI) |         \
                                     PIN_ODR_HIGH(GPIOG_SPI1_CS) |          \
                                     PIN_ODR_LOW(GPIOG_SPI1_SCK) |          \
                                     PIN_ODR_LOW(GPIOG_SPI6_MISO) |         \
                                     PIN_ODR_LOW(GPIOG_SPI6_SCK) |          \
                                     PIN_ODR_LOW(GPIOG_SPI6_MOSI) |         \
                                     PIN_ODR_HIGH(GPIOG_FMC_SDNCAS))
#define VAL_GPIOG_AFRL              (PIN_AFIO_AF(GPIOG_FMC_A10, 12U) |      \
                                     PIN_AFIO_AF(GPIOG_FMC_A11, 12U) |      \
                                     PIN_AFIO_AF(GPIOG_FMC_A12, 12U) |      \
                                     PIN_AFIO_AF(GPIOG_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOG_FMC_BA0, 12U) |      \
                                     PIN_AFIO_AF(GPIOG_FMC_BA1, 12U) |      \
                                     PIN_AFIO_AF(GPIOG_USB_FS_PWR_EN, 0U) | \
                                     PIN_AFIO_AF(GPIOG_USB_FS_OVCR, 0U))
#define VAL_GPIOG_AFRH              (PIN_AFIO_AF(GPIOG_FMC_SDCLK, 12U) |    \
                                     PIN_AFIO_AF(GPIOG_SPI1_MOSI, 5U) |     \
                                     PIN_AFIO_AF(GPIOG_SPI1_CS, 0U) |       \
                                     PIN_AFIO_AF(GPIOG_SPI1_SCK, 5U) |      \
                                     PIN_AFIO_AF(GPIOG_SPI6_MISO, 5U) |     \
                                     PIN_AFIO_AF(GPIOG_SPI6_SCK, 5U) |      \
                                     PIN_AFIO_AF(GPIOG_SPI6_MOSI, 5U) |     \
                                     PIN_AFIO_AF(GPIOG_FMC_SDNCAS, 12U))

/*
 * GPIOH setup:
 *
 * PH0  - HSE_IN                    (alternate 0).
 * PH1  - HSE_OUT                   (alternate 0).
 * PH2  - MUX_POT_S0                (output pushpull low).
 * PH3  - MUX_POT_S1                (output pushpull low).
 * PH4  - MUX_POT_S2                (output pushpull low).
 * PH5  - MUX_POT_ADC               (analog).
 * PH6  - PIN6                      (analog).
 * PH7  - I2C3_SCL                  (alternate 4).
 * PH8  - I2C3_SDA                  (alternate 4).
 * PH9  - PIN9                      (analog).
 * PH10 - ENC3_A_TIM5_CH1           (alternate 2).
 * PH11 - ENC3_B_TIM5_CH2           (alternate 2).
 * PH12 - PIN12                     (analog).
 * PH13 - PIN13                     (analog).
 * PH14 - PIN14                     (analog).
 * PH15 - PIN15                     (analog).
 */
#define VAL_GPIOH_MODER             (PIN_MODE_ALTERNATE(GPIOH_HSE_IN) |     \
                                     PIN_MODE_ALTERNATE(GPIOH_HSE_OUT) |    \
                                     PIN_MODE_OUTPUT(GPIOH_MUX_POT_S0) |    \
                                     PIN_MODE_OUTPUT(GPIOH_MUX_POT_S1) |    \
                                     PIN_MODE_OUTPUT(GPIOH_MUX_POT_S2) |    \
                                     PIN_MODE_ANALOG(GPIOH_MUX_POT_ADC) |   \
                                     PIN_MODE_ANALOG(GPIOH_PIN6) |          \
                                     PIN_MODE_ALTERNATE(GPIOH_I2C3_SCL) |   \
                                     PIN_MODE_ALTERNATE(GPIOH_I2C3_SDA) |   \
                                     PIN_MODE_ANALOG(GPIOH_PIN9) |          \
                                     PIN_MODE_ALTERNATE(GPIOH_ENC3_A_TIM5_CH1) |\
                                     PIN_MODE_ALTERNATE(GPIOH_ENC3_B_TIM5_CH2) |\
                                     PIN_MODE_ANALOG(GPIOH_PIN12) |         \
                                     PIN_MODE_ANALOG(GPIOH_PIN13) |         \
                                     PIN_MODE_ANALOG(GPIOH_PIN14) |         \
                                     PIN_MODE_ANALOG(GPIOH_PIN15))
#define VAL_GPIOH_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOH_HSE_IN) |     \
                                     PIN_OTYPE_PUSHPULL(GPIOH_HSE_OUT) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOH_MUX_POT_S0) | \
                                     PIN_OTYPE_PUSHPULL(GPIOH_MUX_POT_S1) | \
                                     PIN_OTYPE_PUSHPULL(GPIOH_MUX_POT_S2) | \
                                     PIN_OTYPE_PUSHPULL(GPIOH_MUX_POT_ADC) |\
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN6) |       \
                                     PIN_OTYPE_OPENDRAIN(GPIOH_I2C3_SCL) |  \
                                     PIN_OTYPE_OPENDRAIN(GPIOH_I2C3_SDA) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOH_ENC3_A_TIM5_CH1) |\
                                     PIN_OTYPE_PUSHPULL(GPIOH_ENC3_B_TIM5_CH2) |\
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOH_PIN15))
#define VAL_GPIOH_OSPEEDR           (PIN_OSPEED_HIGH(GPIOH_HSE_IN) |        \
                                     PIN_OSPEED_HIGH(GPIOH_HSE_OUT) |       \
                                     PIN_OSPEED_LOW(GPIOH_MUX_POT_S0) |     \
                                     PIN_OSPEED_LOW(GPIOH_MUX_POT_S1) |     \
                                     PIN_OSPEED_LOW(GPIOH_MUX_POT_S2) |     \
                                     PIN_OSPEED_VERYLOW(GPIOH_MUX_POT_ADC) |\
                                     PIN_OSPEED_LOW(GPIOH_PIN6) |           \
                                     PIN_OSPEED_LOW(GPIOH_I2C3_SCL) |       \
                                     PIN_OSPEED_LOW(GPIOH_I2C3_SDA) |       \
                                     PIN_OSPEED_LOW(GPIOH_PIN9) |           \
                                     PIN_OSPEED_HIGH(GPIOH_ENC3_A_TIM5_CH1) |\
                                     PIN_OSPEED_HIGH(GPIOH_ENC3_B_TIM5_CH2) |\
                                     PIN_OSPEED_LOW(GPIOH_PIN12) |          \
                                     PIN_OSPEED_LOW(GPIOH_PIN13) |          \
                                     PIN_OSPEED_LOW(GPIOH_PIN14) |          \
                                     PIN_OSPEED_LOW(GPIOH_PIN15))
#define VAL_GPIOH_PUPDR             (PIN_PUPDR_FLOATING(GPIOH_HSE_IN) |     \
                                     PIN_PUPDR_FLOATING(GPIOH_HSE_OUT) |    \
                                     PIN_PUPDR_FLOATING(GPIOH_MUX_POT_S0) | \
                                     PIN_PUPDR_FLOATING(GPIOH_MUX_POT_S1) | \
                                     PIN_PUPDR_FLOATING(GPIOH_MUX_POT_S2) | \
                                     PIN_PUPDR_FLOATING(GPIOH_MUX_POT_ADC) |\
                                     PIN_PUPDR_FLOATING(GPIOH_PIN6) |       \
                                     PIN_PUPDR_PULLUP(GPIOH_I2C3_SCL) |     \
                                     PIN_PUPDR_PULLUP(GPIOH_I2C3_SDA) |     \
                                     PIN_PUPDR_FLOATING(GPIOH_PIN9) |       \
                                     PIN_PUPDR_FLOATING(GPIOH_ENC3_A_TIM5_CH1) |\
                                     PIN_PUPDR_FLOATING(GPIOH_ENC3_B_TIM5_CH2) |\
                                     PIN_PUPDR_FLOATING(GPIOH_PIN12) |      \
                                     PIN_PUPDR_FLOATING(GPIOH_PIN13) |      \
                                     PIN_PUPDR_FLOATING(GPIOH_PIN14) |      \
                                     PIN_PUPDR_FLOATING(GPIOH_PIN15))
#define VAL_GPIOH_ODR               (PIN_ODR_HIGH(GPIOH_HSE_IN) |           \
                                     PIN_ODR_HIGH(GPIOH_HSE_OUT) |          \
                                     PIN_ODR_LOW(GPIOH_MUX_POT_S0) |        \
                                     PIN_ODR_LOW(GPIOH_MUX_POT_S1) |        \
                                     PIN_ODR_LOW(GPIOH_MUX_POT_S2) |        \
                                     PIN_ODR_LOW(GPIOH_MUX_POT_ADC) |       \
                                     PIN_ODR_LOW(GPIOH_PIN6) |              \
                                     PIN_ODR_HIGH(GPIOH_I2C3_SCL) |         \
                                     PIN_ODR_HIGH(GPIOH_I2C3_SDA) |         \
                                     PIN_ODR_LOW(GPIOH_PIN9) |              \
                                     PIN_ODR_LOW(GPIOH_ENC3_A_TIM5_CH1) |   \
                                     PIN_ODR_LOW(GPIOH_ENC3_B_TIM5_CH2) |   \
                                     PIN_ODR_LOW(GPIOH_PIN12) |             \
                                     PIN_ODR_LOW(GPIOH_PIN13) |             \
                                     PIN_ODR_LOW(GPIOH_PIN14) |             \
                                     PIN_ODR_LOW(GPIOH_PIN15))
#define VAL_GPIOH_AFRL              (PIN_AFIO_AF(GPIOH_HSE_IN, 0U) |        \
                                     PIN_AFIO_AF(GPIOH_HSE_OUT, 0U) |       \
                                     PIN_AFIO_AF(GPIOH_MUX_POT_S0, 0U) |    \
                                     PIN_AFIO_AF(GPIOH_MUX_POT_S1, 0U) |    \
                                     PIN_AFIO_AF(GPIOH_MUX_POT_S2, 0U) |    \
                                     PIN_AFIO_AF(GPIOH_MUX_POT_ADC, 0U) |   \
                                     PIN_AFIO_AF(GPIOH_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOH_I2C3_SCL, 4U))
#define VAL_GPIOH_AFRH              (PIN_AFIO_AF(GPIOH_I2C3_SDA, 4U) |      \
                                     PIN_AFIO_AF(GPIOH_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOH_ENC3_A_TIM5_CH1, 2U) |\
                                     PIN_AFIO_AF(GPIOH_ENC3_B_TIM5_CH2, 2U) |\
                                     PIN_AFIO_AF(GPIOH_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOH_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOH_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOH_PIN15, 0U))

/*
 * GPIOI setup:
 *
 * PI0  - SPI2_CS                   (output pushpull maximum).
 * PI1  - SPI2_SCK                  (alternate 5).
 * PI2  - SPI2_MISO                 (alternate 5).
 * PI3  - SPI2_MOSI                 (alternate 5).
 * PI4  - SPI5_RES_OLED             (output pushpull maximum).
 * PI5  - SPI5_DC_OLED              (output pushpull maximum).
 * PI6  - SPI5_CS_OLED              (output pushpull maximum).
 * PI7  - SPI3_CS                   (output pushpull maximum).
 * PI8  - PIN8                      (analog).
 * PI9  - PIN9                      (analog).
 * PI10 - PIN10                     (analog).
 * PI11 - PIN11                     (analog).
 * PI12 - PIN12                     (analog).
 * PI13 - PIN13                     (analog).
 * PI14 - PIN14                     (analog).
 * PI15 - PIN15                     (analog).
 */
#define VAL_GPIOI_MODER             (PIN_MODE_OUTPUT(GPIOI_SPI2_CS) |       \
                                     PIN_MODE_ALTERNATE(GPIOI_SPI2_SCK) |   \
                                     PIN_MODE_ALTERNATE(GPIOI_SPI2_MISO) |  \
                                     PIN_MODE_ALTERNATE(GPIOI_SPI2_MOSI) |  \
                                     PIN_MODE_OUTPUT(GPIOI_SPI5_RES_OLED) | \
                                     PIN_MODE_OUTPUT(GPIOI_SPI5_DC_OLED) |  \
                                     PIN_MODE_OUTPUT(GPIOI_SPI5_CS_OLED) |  \
                                     PIN_MODE_OUTPUT(GPIOI_SPI3_CS) |       \
                                     PIN_MODE_ANALOG(GPIOI_PIN8) |          \
                                     PIN_MODE_ANALOG(GPIOI_PIN9) |          \
                                     PIN_MODE_ANALOG(GPIOI_PIN10) |         \
                                     PIN_MODE_ANALOG(GPIOI_PIN11) |         \
                                     PIN_MODE_ANALOG(GPIOI_PIN12) |         \
                                     PIN_MODE_ANALOG(GPIOI_PIN13) |         \
                                     PIN_MODE_ANALOG(GPIOI_PIN14) |         \
                                     PIN_MODE_ANALOG(GPIOI_PIN15))
#define VAL_GPIOI_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOI_SPI2_CS) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOI_SPI2_SCK) |   \
                                     PIN_OTYPE_PUSHPULL(GPIOI_SPI2_MISO) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOI_SPI2_MOSI) |  \
                                     PIN_OTYPE_PUSHPULL(GPIOI_SPI5_RES_OLED) |\
                                     PIN_OTYPE_PUSHPULL(GPIOI_SPI5_DC_OLED) |\
                                     PIN_OTYPE_PUSHPULL(GPIOI_SPI5_CS_OLED) |\
                                     PIN_OTYPE_PUSHPULL(GPIOI_SPI3_CS) |    \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOI_PIN15))
#define VAL_GPIOI_OSPEEDR           (PIN_OSPEED_HIGH(GPIOI_SPI2_CS) |       \
                                     PIN_OSPEED_HIGH(GPIOI_SPI2_SCK) |      \
                                     PIN_OSPEED_HIGH(GPIOI_SPI2_MISO) |     \
                                     PIN_OSPEED_HIGH(GPIOI_SPI2_MOSI) |     \
                                     PIN_OSPEED_HIGH(GPIOI_SPI5_RES_OLED) | \
                                     PIN_OSPEED_HIGH(GPIOI_SPI5_DC_OLED) |  \
                                     PIN_OSPEED_HIGH(GPIOI_SPI5_CS_OLED) |  \
                                     PIN_OSPEED_HIGH(GPIOI_SPI3_CS) |       \
                                     PIN_OSPEED_LOW(GPIOI_PIN8) |           \
                                     PIN_OSPEED_LOW(GPIOI_PIN9) |           \
                                     PIN_OSPEED_LOW(GPIOI_PIN10) |          \
                                     PIN_OSPEED_LOW(GPIOI_PIN11) |          \
                                     PIN_OSPEED_LOW(GPIOI_PIN12) |          \
                                     PIN_OSPEED_LOW(GPIOI_PIN13) |          \
                                     PIN_OSPEED_LOW(GPIOI_PIN14) |          \
                                     PIN_OSPEED_LOW(GPIOI_PIN15))
#define VAL_GPIOI_PUPDR             (PIN_PUPDR_FLOATING(GPIOI_SPI2_CS) |    \
                                     PIN_PUPDR_FLOATING(GPIOI_SPI2_SCK) |   \
                                     PIN_PUPDR_FLOATING(GPIOI_SPI2_MISO) |  \
                                     PIN_PUPDR_FLOATING(GPIOI_SPI2_MOSI) |  \
                                     PIN_PUPDR_FLOATING(GPIOI_SPI5_RES_OLED) |\
                                     PIN_PUPDR_FLOATING(GPIOI_SPI5_DC_OLED) |\
                                     PIN_PUPDR_FLOATING(GPIOI_SPI5_CS_OLED) |\
                                     PIN_PUPDR_FLOATING(GPIOI_SPI3_CS) |    \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN8) |       \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN9) |       \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN10) |      \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN11) |      \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN12) |      \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN13) |      \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN14) |      \
                                     PIN_PUPDR_FLOATING(GPIOI_PIN15))
#define VAL_GPIOI_ODR               (PIN_ODR_HIGH(GPIOI_SPI2_CS) |          \
                                     PIN_ODR_LOW(GPIOI_SPI2_SCK) |          \
                                     PIN_ODR_LOW(GPIOI_SPI2_MISO) |         \
                                     PIN_ODR_LOW(GPIOI_SPI2_MOSI) |         \
                                     PIN_ODR_HIGH(GPIOI_SPI5_RES_OLED) |    \
                                     PIN_ODR_HIGH(GPIOI_SPI5_DC_OLED) |     \
                                     PIN_ODR_HIGH(GPIOI_SPI5_CS_OLED) |     \
                                     PIN_ODR_HIGH(GPIOI_SPI3_CS) |          \
                                     PIN_ODR_LOW(GPIOI_PIN8) |              \
                                     PIN_ODR_LOW(GPIOI_PIN9) |              \
                                     PIN_ODR_LOW(GPIOI_PIN10) |             \
                                     PIN_ODR_LOW(GPIOI_PIN11) |             \
                                     PIN_ODR_LOW(GPIOI_PIN12) |             \
                                     PIN_ODR_LOW(GPIOI_PIN13) |             \
                                     PIN_ODR_LOW(GPIOI_PIN14) |             \
                                     PIN_ODR_LOW(GPIOI_PIN15))
#define VAL_GPIOI_AFRL              (PIN_AFIO_AF(GPIOI_SPI2_CS, 0U) |       \
                                     PIN_AFIO_AF(GPIOI_SPI2_SCK, 5U) |      \
                                     PIN_AFIO_AF(GPIOI_SPI2_MISO, 5U) |     \
                                     PIN_AFIO_AF(GPIOI_SPI2_MOSI, 5U) |     \
                                     PIN_AFIO_AF(GPIOI_SPI5_RES_OLED, 0U) | \
                                     PIN_AFIO_AF(GPIOI_SPI5_DC_OLED, 0U) |  \
                                     PIN_AFIO_AF(GPIOI_SPI5_CS_OLED, 0U) |  \
                                     PIN_AFIO_AF(GPIOI_SPI3_CS, 0U))
#define VAL_GPIOI_AFRH              (PIN_AFIO_AF(GPIOI_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOI_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOI_PIN15, 0U))

/*
 * GPIOJ setup:
 *
 * PJ0  - PIN0                      (analog).
 * PJ1  - PIN1                      (analog).
 * PJ2  - PIN2                      (analog).
 * PJ3  - PIN3                      (analog).
 * PJ4  - PIN4                      (analog).
 * PJ5  - PIN5                      (analog).
 * PJ6  - PIN6                      (analog).
 * PJ7  - PIN7                      (analog).
 * PJ8  - PIN8                      (analog).
 * PJ9  - PIN9                      (analog).
 * PJ10 - PIN10                     (analog).
 * PJ11 - PIN11                     (analog).
 * PJ12 - PIN12                     (analog).
 * PJ13 - PIN13                     (analog).
 * PJ14 - PIN14                     (analog).
 * PJ15 - PIN15                     (analog).
 */
#define VAL_GPIOJ_MODER             (PIN_MODE_ANALOG(GPIOJ_PIN0) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN1) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN2) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN3) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN4) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN5) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN6) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN7) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN8) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN9) |          \
                                     PIN_MODE_ANALOG(GPIOJ_PIN10) |         \
                                     PIN_MODE_ANALOG(GPIOJ_PIN11) |         \
                                     PIN_MODE_ANALOG(GPIOJ_PIN12) |         \
                                     PIN_MODE_ANALOG(GPIOJ_PIN13) |         \
                                     PIN_MODE_ANALOG(GPIOJ_PIN14) |         \
                                     PIN_MODE_ANALOG(GPIOJ_PIN15))
#define VAL_GPIOJ_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOJ_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOJ_PIN15))
#define VAL_GPIOJ_OSPEEDR           (PIN_OSPEED_LOW(GPIOJ_PIN0) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN1) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN2) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN3) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN4) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN5) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN6) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN7) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN8) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN9) |           \
                                     PIN_OSPEED_LOW(GPIOJ_PIN10) |          \
                                     PIN_OSPEED_LOW(GPIOJ_PIN11) |          \
                                     PIN_OSPEED_LOW(GPIOJ_PIN12) |          \
                                     PIN_OSPEED_LOW(GPIOJ_PIN13) |          \
                                     PIN_OSPEED_LOW(GPIOJ_PIN14) |          \
                                     PIN_OSPEED_LOW(GPIOJ_PIN15))
#define VAL_GPIOJ_PUPDR             (PIN_PUPDR_FLOATING(GPIOJ_PIN0) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN1) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN2) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN3) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN4) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN5) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN6) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN7) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN8) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN9) |       \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN10) |      \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN11) |      \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN12) |      \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN13) |      \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN14) |      \
                                     PIN_PUPDR_FLOATING(GPIOJ_PIN15))
#define VAL_GPIOJ_ODR               (PIN_ODR_LOW(GPIOJ_PIN0) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN1) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN2) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN3) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN4) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN5) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN6) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN7) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN8) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN9) |              \
                                     PIN_ODR_LOW(GPIOJ_PIN10) |             \
                                     PIN_ODR_LOW(GPIOJ_PIN11) |             \
                                     PIN_ODR_LOW(GPIOJ_PIN12) |             \
                                     PIN_ODR_LOW(GPIOJ_PIN13) |             \
                                     PIN_ODR_LOW(GPIOJ_PIN14) |             \
                                     PIN_ODR_LOW(GPIOJ_PIN15))
#define VAL_GPIOJ_AFRL              (PIN_AFIO_AF(GPIOJ_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN7, 0U))
#define VAL_GPIOJ_AFRH              (PIN_AFIO_AF(GPIOJ_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOJ_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOJ_PIN15, 0U))

/*
 * GPIOK setup:
 *
 * PK0  - PIN0                      (analog).
 * PK1  - PIN1                      (analog).
 * PK2  - PIN2                      (analog).
 * PK3  - PIN3                      (analog).
 * PK4  - PIN4                      (analog).
 * PK5  - PIN5                      (analog).
 * PK6  - PIN6                      (analog).
 * PK7  - PIN7                      (analog).
 * PK8  - PIN8                      (analog).
 * PK9  - PIN9                      (analog).
 * PK10 - PIN10                     (analog).
 * PK11 - PIN11                     (analog).
 * PK12 - PIN12                     (analog).
 * PK13 - PIN13                     (analog).
 * PK14 - PIN14                     (analog).
 * PK15 - PIN15                     (analog).
 */
#define VAL_GPIOK_MODER             (PIN_MODE_ANALOG(GPIOK_PIN0) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN1) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN2) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN3) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN4) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN5) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN6) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN7) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN8) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN9) |          \
                                     PIN_MODE_ANALOG(GPIOK_PIN10) |         \
                                     PIN_MODE_ANALOG(GPIOK_PIN11) |         \
                                     PIN_MODE_ANALOG(GPIOK_PIN12) |         \
                                     PIN_MODE_ANALOG(GPIOK_PIN13) |         \
                                     PIN_MODE_ANALOG(GPIOK_PIN14) |         \
                                     PIN_MODE_ANALOG(GPIOK_PIN15))
#define VAL_GPIOK_OTYPER            (PIN_OTYPE_PUSHPULL(GPIOK_PIN0) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN1) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN2) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN3) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN4) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN5) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN6) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN7) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN8) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN9) |       \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN10) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN11) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN12) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN13) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN14) |      \
                                     PIN_OTYPE_PUSHPULL(GPIOK_PIN15))
#define VAL_GPIOK_OSPEEDR           (PIN_OSPEED_LOW(GPIOK_PIN0) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN1) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN2) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN3) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN4) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN5) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN6) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN7) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN8) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN9) |           \
                                     PIN_OSPEED_LOW(GPIOK_PIN10) |          \
                                     PIN_OSPEED_LOW(GPIOK_PIN11) |          \
                                     PIN_OSPEED_LOW(GPIOK_PIN12) |          \
                                     PIN_OSPEED_LOW(GPIOK_PIN13) |          \
                                     PIN_OSPEED_LOW(GPIOK_PIN14) |          \
                                     PIN_OSPEED_LOW(GPIOK_PIN15))
#define VAL_GPIOK_PUPDR             (PIN_PUPDR_FLOATING(GPIOK_PIN0) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN1) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN2) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN3) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN4) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN5) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN6) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN7) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN8) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN9) |       \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN10) |      \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN11) |      \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN12) |      \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN13) |      \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN14) |      \
                                     PIN_PUPDR_FLOATING(GPIOK_PIN15))
#define VAL_GPIOK_ODR               (PIN_ODR_LOW(GPIOK_PIN0) |              \
                                     PIN_ODR_LOW(GPIOK_PIN1) |              \
                                     PIN_ODR_LOW(GPIOK_PIN2) |              \
                                     PIN_ODR_LOW(GPIOK_PIN3) |              \
                                     PIN_ODR_LOW(GPIOK_PIN4) |              \
                                     PIN_ODR_LOW(GPIOK_PIN5) |              \
                                     PIN_ODR_LOW(GPIOK_PIN6) |              \
                                     PIN_ODR_LOW(GPIOK_PIN7) |              \
                                     PIN_ODR_LOW(GPIOK_PIN8) |              \
                                     PIN_ODR_LOW(GPIOK_PIN9) |              \
                                     PIN_ODR_LOW(GPIOK_PIN10) |             \
                                     PIN_ODR_LOW(GPIOK_PIN11) |             \
                                     PIN_ODR_LOW(GPIOK_PIN12) |             \
                                     PIN_ODR_LOW(GPIOK_PIN13) |             \
                                     PIN_ODR_LOW(GPIOK_PIN14) |             \
                                     PIN_ODR_LOW(GPIOK_PIN15))
#define VAL_GPIOK_AFRL              (PIN_AFIO_AF(GPIOK_PIN0, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN1, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN2, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN3, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN4, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN5, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN6, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN7, 0U))
#define VAL_GPIOK_AFRH              (PIN_AFIO_AF(GPIOK_PIN8, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN9, 0U) |          \
                                     PIN_AFIO_AF(GPIOK_PIN10, 0U) |         \
                                     PIN_AFIO_AF(GPIOK_PIN11, 0U) |         \
                                     PIN_AFIO_AF(GPIOK_PIN12, 0U) |         \
                                     PIN_AFIO_AF(GPIOK_PIN13, 0U) |         \
                                     PIN_AFIO_AF(GPIOK_PIN14, 0U) |         \
                                     PIN_AFIO_AF(GPIOK_PIN15, 0U))

/*===========================================================================*/
/* External declarations.                                                    */
/*===========================================================================*/

#if !defined(_FROM_ASM_)
#ifdef __cplusplus
extern "C" {
#endif
  void boardInit(void);
#ifdef __cplusplus
}
#endif
#endif /* _FROM_ASM_ */

#endif /* BOARD_H */
