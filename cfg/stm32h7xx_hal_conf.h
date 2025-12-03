#ifndef STM32H7XX_HAL_CONF_H
#define STM32H7XX_HAL_CONF_H

/* --------------------------------------------------------------------------
 *  HAL MODULE SELECTION
 * -------------------------------------------------------------------------- */

#define HAL_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_HCD_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED

/* --------------------------------------------------------------------------
 *  HAL HARDWARE CONSTANTS
 * -------------------------------------------------------------------------- */

#define HSE_VALUE               8000000U
#define HSE_STARTUP_TIMEOUT     100U
#define HSI_VALUE               64000000U
#define CSI_VALUE               4000000U
#define LSI_VALUE               32000U
#define LSE_VALUE               32768U
#define LSE_STARTUP_TIMEOUT     5000U
#define EXTERNAL_CLOCK_VALUE    48000000U

#define VDD_VALUE               3300U
#define TICK_INT_PRIORITY       0x0FU
#define USE_RTOS                0U
#define PREFETCH_ENABLE         0U
#define INSTRUCTION_CACHE_ENABLE 1U
#define DATA_CACHE_ENABLE       1U

/* --------------------------------------------------------------------------
 *  ASSERT POLICY (DISABLED = NO HAL ASSERTS + NO WARNINGS)
 * -------------------------------------------------------------------------- */

#define USE_FULL_ASSERT         0U

/* Voici la correction principale :
   On neutralise assert_param pour supprimer *tous* les warnings,
   tout en ne générant aucun code supplémentaire.
*/
#ifndef assert_param
#define assert_param(expr) ((void)0U)
#endif

/* --------------------------------------------------------------------------
 *  HAL CALLBACKS
 * -------------------------------------------------------------------------- */

#define USE_HAL_HCD_REGISTER_CALLBACKS 0U

/* --------------------------------------------------------------------------
 *  HAL HEADER INCLUSION (NE PAS MODIFIER)
 * -------------------------------------------------------------------------- */

#include "stm32h7xx_hal_def.h"

#ifdef HAL_RCC_MODULE_ENABLED
#include "stm32h7xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
#include "stm32h7xx_hal_gpio.h"
#endif

#ifdef HAL_HCD_MODULE_ENABLED
#include "stm32h7xx_hal_hcd.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
#include "stm32h7xx_hal_flash.h"
#include "stm32h7xx_hal_flash_ex.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
#include "stm32h7xx_hal_pwr.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
#include "stm32h7xx_hal_cortex.h"
#endif

#endif /* STM32H7XX_HAL_CONF_H */
