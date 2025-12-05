#include "ch.h"
#include "stm32h7xx.h"
#include "stm32h743xx.h"
#include "core_cm7.h"
#include "sdram_driver_priv.h"
#include "sdram_layout.h"

#define SDRAM_REFRESH_COUNT       (761u)
#define SDRAM_TIMEOUT_CYCLES      (0x3FFFFu)
#define SDRAM_MODE_REGISTER_VALUE (0x0032u)
#define SDRAM_SDSR_BUSY           (1u << 5) /* RM0455: SDSR busy flag (bit 5) */

#define SDRAM_CMD_NORMAL         (0u)
#define SDRAM_CMD_CLK_ENABLE     (1u)
#define SDRAM_CMD_PALL           (2u)
#define SDRAM_CMD_AUTOREFRESH    (3u)
#define SDRAM_CMD_LOAD_MODE      (4u)

static bool fmc_wait_while_busy(uint32_t timeout)
{
  while ((FMC_Bank5_6_R->SDSR & SDRAM_SDSR_BUSY) != 0u) {
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

  FMC_Bank5_6_R->SDCMR = command;
  return fmc_wait_while_busy(SDRAM_TIMEOUT_CYCLES);
}

static uint32_t fmc_current_mode(void)
{
  return (FMC_Bank5_6_R->SDSR & FMC_SDSR_MODES1_Msk) >> FMC_SDSR_MODES1_Pos;
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
                        ((2u << FMC_SDCRx_CAS_Pos) & FMC_SDCRx_CAS_Msk) | /* CAS latency = 3 */
                        FMC_SDCRx_SDCLK_1 | /* 2 HCLK period */
                        FMC_SDCRx_RBURST; /* enable read burst */

  const uint32_t sdtr = ((2u - 1u) << FMC_SDTRx_TMRD_Pos) |  /* tMRD */
                        ((8u - 1u) << FMC_SDTRx_TXSR_Pos) |  /* tXSR */
                        ((6u - 1u) << FMC_SDTRx_TRAS_Pos) |  /* tRAS */
                        ((6u - 1u) << FMC_SDTRx_TRC_Pos) |   /* tRC  */
                        ((3u - 1u) << FMC_SDTRx_TWR_Pos) |   /* tWR  */
                        ((3u - 1u) << FMC_SDTRx_TRP_Pos) |   /* tRP  */
                        ((3u - 1u) << FMC_SDTRx_TRCD_Pos);   /* tRCD */

  FMC_Bank5_6_R->SDCR[0] = sdcr;
  FMC_Bank5_6_R->SDTR[0] = sdtr;

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

  uint32_t sdrtr = FMC_Bank5_6_R->SDRTR;
  sdrtr &= ~FMC_SDRTR_COUNT_Msk;
  sdrtr |= (SDRAM_REFRESH_COUNT << FMC_SDRTR_COUNT_Pos);
  FMC_Bank5_6_R->SDRTR = sdrtr;

  if (!fmc_wait_while_busy(SDRAM_TIMEOUT_CYCLES)) {
    return false;
  }

  const uint32_t status = FMC_Bank5_6_R->SDSR;
  if ((status & FMC_SDSR_RE) != 0u) {
    return false;
  }

  const uint32_t mode = fmc_current_mode();
  if (mode != 0u) {
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
  __DSB();
  __ISB();

  /*
   * Main SDRAM: Normal memory, non-cacheable, shareable (TEX=1,C=0,B=0,S=1)
   * so that DMA/audio transfers remain deterministic with D-Cache enabled.
   */
  ARM_MPU_SetRegion(ARM_MPU_RBAR(1u, SDRAM_BASE_ADDRESS),
                    ARM_MPU_RASR(0u,
                                 ARM_MPU_AP_FULL,
                                 1u,
                                 1u,
                                 0u,
                                 0u,
                                 0u,
                                 ARM_MPU_REGION_SIZE_32MB));

#if (SDRAM_ENABLE_CACHE_RESIDUAL == 1)
  /*
   * CPU-only residual: Normal cacheable (non-shareable) for scratch use.
   * DMA is explicitly forbidden on this window.
   */
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

  __DSB();
  __ISB();

  SCB_InvalidateDCache();
  __DSB();
  __ISB();
  ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
  __DSB();
  __ISB();

  return true;
}

