#include "ch.h"
#include "hal.h"

#include "sdram_driver_priv.h"
#include "sdram_layout.h"

#define SDRAM_REFRESH_COUNT   (781u)
#define SDRAM_TIMEOUT         (0x1000u)

static SDRAM_HandleTypeDef hsdram;

static bool sdram_send_command(uint32_t mode, uint32_t target, uint32_t auto_refresh, uint32_t mode_reg) {
  FMC_SDRAM_CommandTypeDef cmd;
  cmd.CommandMode = mode;
  cmd.CommandTarget = target;
  cmd.AutoRefreshNumber = auto_refresh;
  cmd.ModeRegisterDefinition = mode_reg;

  return HAL_SDRAM_SendCommand(&hsdram, &cmd, SDRAM_TIMEOUT) == HAL_OK;
}

bool sdram_hw_init_sequence(void) {
  /* SDRAM device configuration */
  hsdram.Instance = FMC_SDRAM_DEVICE;
  hsdram.Init.SDBank = FMC_SDRAM_BANK1;
  hsdram.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_9;
  hsdram.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_13;
  hsdram.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram.Init.ReadBurst = FMC_SDRAM_RBURST_ENABLE;
  hsdram.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_0;

  FMC_SDRAM_TimingTypeDef timing;
  timing.LoadToActiveDelay = 2u;    /* tMRD */
  timing.ExitSelfRefreshDelay = 8u; /* tXSR */
  timing.SelfRefreshTime = 6u;      /* tRAS */
  timing.RowCycleDelay = 6u;        /* tRC */
  timing.WriteRecoveryTime = 3u;    /* tWR */
  timing.RPDelay = 3u;              /* tRP */
  timing.RCDDelay = 3u;             /* tRCD */

  if (HAL_SDRAM_Init(&hsdram, &timing) != HAL_OK) {
    return false;
  }

  /* JEDEC power-up sequence */
  chThdSleepMicroseconds(200u);

  if (!sdram_send_command(FMC_SDRAM_CMD_CLK_ENABLE, FMC_SDRAM_CMD_TARGET_BANK1, 1u, 0u)) {
    return false;
  }

  chThdSleepMilliseconds(1);

  if (!sdram_send_command(FMC_SDRAM_CMD_PALL, FMC_SDRAM_CMD_TARGET_BANK1, 1u, 0u)) {
    return false;
  }

  if (!sdram_send_command(FMC_SDRAM_CMD_AUTOREFRESH_MODE, FMC_SDRAM_CMD_TARGET_BANK1, 8u, 0u)) {
    return false;
  }

  /* Mode register: BL=4, burst type sequential, CAS=3, write burst enabled */
  const uint32_t mode_reg = 0x0032u;
  if (!sdram_send_command(FMC_SDRAM_CMD_LOAD_MODE, FMC_SDRAM_CMD_TARGET_BANK1, 1u, mode_reg)) {
    return false;
  }

  if (HAL_SDRAM_ProgramRefreshRate(&hsdram, SDRAM_REFRESH_COUNT) != HAL_OK) {
    return false;
  }

  if (HAL_SDRAM_GetModeStatus(&hsdram) != FMC_SDRAM_NORMAL_MODE) {
    return false;
  }

  return true;
}

static void sdram_configure_region(uint32_t number, uintptr_t base, uint32_t size_enum, bool cacheable, bool bufferable, bool shareable) {
  MPU_Region_InitTypeDef mpu_cfg;

  mpu_cfg.Enable = MPU_REGION_ENABLE;
  mpu_cfg.Number = number;
  mpu_cfg.BaseAddress = base;
  mpu_cfg.Size = size_enum;
  mpu_cfg.SubRegionDisable = 0x00u;
  mpu_cfg.TypeExtField = MPU_TEX_LEVEL0;
  mpu_cfg.AccessPermission = MPU_REGION_FULL_ACCESS;
  mpu_cfg.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  mpu_cfg.IsCacheable = cacheable ? MPU_ACCESS_CACHEABLE : MPU_ACCESS_NOT_CACHEABLE;
  mpu_cfg.IsBufferable = bufferable ? MPU_ACCESS_BUFFERABLE : MPU_ACCESS_NOT_BUFFERABLE;
  mpu_cfg.IsShareable = shareable ? MPU_ACCESS_SHAREABLE : MPU_ACCESS_NOT_SHAREABLE;

  HAL_MPU_ConfigRegion(&mpu_cfg);
}

bool sdram_configure_mpu_regions(void) {
  /* Region 0 is already used by SRAM non-cacheable buffers; append new regions. */
  const uint32_t sdram_region_number = MPU_REGION_NUMBER1;
  sdram_configure_region(sdram_region_number,
                         SDRAM_BASE_ADDRESS,
                         MPU_REGION_SIZE_32MB,
                         false,
                         false,
                         true);

#if (SDRAM_ENABLE_CACHE_RESIDUAL == 1)
  const uintptr_t residual_base = SDRAM_BASE_ADDRESS + (31u * 1024u * 1024u);
  sdram_configure_region(MPU_REGION_NUMBER2,
                         residual_base,
                         MPU_REGION_SIZE_1MB,
                         true,
                         false,
                         false);
#endif

  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
  return true;
}

