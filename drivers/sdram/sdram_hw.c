#include "ch.h"
#include "hal.h"
#include "stm32h7xx.h"
#include "core_cm7.h"
#include "sdram_driver_priv.h"
#include "sdram_layout.h"

#define SDRAM_REFRESH_COUNT       (761u)
#define SDRAM_TIMEOUT_CYCLES      (0xFFFFu)
#define SDRAM_MODE_REGISTER_VALUE (0x0032u)
#define SDRAM_SDSR_BUSY           (1u << 5)

#define SDRAM_CMD_NORMAL         (0u)
#define SDRAM_CMD_CLK_ENABLE     (1u)
#define SDRAM_CMD_PALL           (2u)
#define SDRAM_CMD_AUTOREFRESH    (3u)
#define SDRAM_CMD_LOAD_MODE      (4u)

static bool fmc_wait_while_busy(uint32_t timeout)
{
  while ((FMC_Bank5_6->SDSR & SDRAM_SDSR_BUSY) != 0u) {
    if (timeout-- == 0u) {
      return false;
    }
  }

  return true;
}

static bool fmc_issue_command(uint32_t mode, uint32_t auto_refresh, uint32_t mode_reg)
{
  const uint32_t command = ((mode << FMC_SDCMR_MODE_Pos) & FMC_SDCMR_MODE_Msk) |
                           FMC_SDCMR_CTB1 |
                           ((((auto_refresh > 0u) ? (auto_refresh - 1u) : 0u) << FMC_SDCMR_NRFS_Pos) &
                            FMC_SDCMR_NRFS_Msk) |
                           ((mode_reg << FMC_SDCMR_MRD_Pos) & FMC_SDCMR_MRD_Msk);

  FMC_Bank5_6->SDCMR = command;
  return fmc_wait_while_busy(SDRAM_TIMEOUT_CYCLES);
}

bool sdram_hw_init_sequence(void)
{
  if ((RCC->AHB3ENR & RCC_AHB3ENR_FMCEN) == 0u) {
    RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN;
    (void)RCC->AHB3ENR;
  }

  const uint32_t sdcr = FMC_SDCRx_NC_0 | /* 9 columns */
                        FMC_SDCRx_NR_1 | /* 13 rows    */
                        FMC_SDCRx_MWID_0 | /* 16-bit bus */
                        FMC_SDCRx_NB | /* 4 internal banks */
                        FMC_SDCRx_CAS | /* CAS latency = 3 */
                        FMC_SDCRx_SDCLK_1 | /* 2 HCLK period */
                        FMC_SDCRx_RBURST; /* enable read burst */

  const uint32_t sdtr = ((2u - 1u) << FMC_SDTRx_TMRD_Pos) |  /* tMRD */
                        ((8u - 1u) << FMC_SDTRx_TXSR_Pos) |  /* tXSR */
                        ((6u - 1u) << FMC_SDTRx_TRAS_Pos) |  /* tRAS */
                        ((6u - 1u) << FMC_SDTRx_TRC_Pos) |   /* tRC  */
                        ((3u - 1u) << FMC_SDTRx_TWR_Pos) |   /* tWR  */
                        ((3u - 1u) << FMC_SDTRx_TRP_Pos) |   /* tRP  */
                        ((3u - 1u) << FMC_SDTRx_TRCD_Pos);   /* tRCD */

  FMC_Bank5_6->SDCR[0] = sdcr;
  FMC_Bank5_6->SDTR[0] = sdtr;

  chThdSleepMicroseconds(200u);

  if (!fmc_issue_command(SDRAM_CMD_CLK_ENABLE, 1u, 0u)) {
    return false;
  }

  chThdSleepMilliseconds(1);

  if (!fmc_issue_command(SDRAM_CMD_PALL, 1u, 0u)) {
    return false;
  }

  if (!fmc_issue_command(SDRAM_CMD_AUTOREFRESH, 8u, 0u)) {
    return false;
  }

  if (!fmc_issue_command(SDRAM_CMD_LOAD_MODE, 1u, SDRAM_MODE_REGISTER_VALUE)) {
    return false;
  }

  FMC_Bank5_6->SDRTR = (SDRAM_REFRESH_COUNT << FMC_SDRTR_COUNT_Pos);

  if (!fmc_wait_while_busy(SDRAM_TIMEOUT_CYCLES)) {
    return false;
  }

  const uint32_t status = FMC_Bank5_6->SDSR;
  if ((status & FMC_SDSR_RE) != 0u) {
    return false;
  }

  if ((status & FMC_SDSR_MODES1_Msk) != 0u) {
    return false;
  }

  return true;
}

bool sdram_configure_mpu_regions(void)
{
  const uintptr_t sdram_end = SDRAM_BASE_ADDRESS + SDRAM_TOTAL_SIZE_BYTES;

#if (SDRAM_ENABLE_CACHE_RESIDUAL == 1)
  const uintptr_t residual_base = SDRAM_BASE_ADDRESS + (31u * 1024u * 1024u);
  const uintptr_t residual_end = residual_base + (1u * 1024u * 1024u);

  if ((residual_base < SDRAM_BASE_ADDRESS) || (residual_end > sdram_end)) {
    return false;
  }
#endif

  ARM_MPU_Disable();

  ARM_MPU_SetRegion(ARM_MPU_RBAR(1u, SDRAM_BASE_ADDRESS),
                    ARM_MPU_RASR(0u,
                                 ARM_MPU_AP_FULL,
                                 0u,
                                 1u,
                                 0u,
                                 0u,
                                 0u,
                                 ARM_MPU_REGION_SIZE_32MB));

#if (SDRAM_ENABLE_CACHE_RESIDUAL == 1)
  ARM_MPU_SetRegion(ARM_MPU_RBAR(2u, residual_base),
                    ARM_MPU_RASR(0u,
                                 ARM_MPU_AP_FULL,
                                 0u,
                                 0u,
                                 1u,
                                 0u,
                                 0u,
                                 ARM_MPU_REGION_SIZE_1MB));
#endif

  SCB_InvalidateDCache();
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);

  return true;
}

