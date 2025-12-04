##############################################################################
# Toolchain (FORCÉE)
##############################################################################
TRGT = arm-none-eabi-

CC   = "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.3 rel1/bin/arm-none-eabi-gcc"
CXX  = "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.3 rel1/bin/arm-none-eabi-g++"
AS   = $(CC)
LD   = $(CC)
AR   = arm-none-eabi-ar
NM   = arm-none-eabi-nm
OBJCOPY = arm-none-eabi-objcopy
OBJDUMP = arm-none-eabi-objdump
SIZE = arm-none-eabi-size

##############################################################################
# Options
##############################################################################
USE_OPT = -O2 -ggdb
USE_COPT =
USE_CPPOPT = -fno-rtti
USE_LINK_GC = yes
USE_LDOPT =
USE_LTO = no
USE_VERBOSE_COMPILE = no
USE_SMART_BUILD = yes

##############################################################################
# MCU & FPU
##############################################################################
MCU = cortex-m7

USE_PROCESS_STACKSIZE = 0x400
USE_EXCEPTIONS_STACKSIZE = 0x400

USE_FPU = hard
USE_FPU_OPT = -mfloat-abi=hard -mfpu=fpv5-d16

##############################################################################
# Paths
##############################################################################

# !!! CHEMIN RELATIF OBLIGATOIRE POUR ÉVITER MSYS !!!
CHIBIOS := ./docs/ChibiOS

CONFDIR  := ./cfg
BUILDDIR := ./build
DEPDIR   := ./.dep

##############################################################################
# Includes ChibiOS
##############################################################################
include $(CHIBIOS)/os/license/license.mk

include $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_stm32h7xx.mk
LDSCRIPT := $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/ld/STM32H743xI.ld

include $(CHIBIOS)/os/hal/hal.mk
include $(CHIBIOS)/os/hal/ports/STM32/STM32H7xx/platform.mk
include $(CHIBIOS)/os/hal/boards/CUSTOM_H743IIT6/board.mk
include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk

include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv7-M/compilers/GCC/mk/port.mk

include $(CHIBIOS)/tools/mk/autobuild.mk

##############################################################################
# Sources (Base projet)
##############################################################################
CSRC = $(ALLCSRC) \
       main.c \
       $(wildcard drivers/*.c) \
       $(wildcard drivers/HallEffect/*.c) \
       $(wildcard drivers/audio/*.c) \
       $(wildcard drivers/sdram/*.c) \
       $(wildcard ui/*.c)

# FatFS / SD drivers
CSRC += $(wildcard drivers/sd/*.c)
CSRC += $(wildcard drivers/sd/ff16/*.c)

CPPSRC  = $(ALLCPPSRC)
ASMSRC  = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)

##############################################################################
# USB HOST MIDI - Code applicatif (drivers/usb/usb_host)
##############################################################################
USB_HOST_APP_DIR := ./drivers/usb/usb_host

CSRC   += $(wildcard $(USB_HOST_APP_DIR)/*.c)
INCDIR += $(USB_HOST_APP_DIR)

##############################################################################
# STM32 MW USB HOST MIDDLEWARE (dans drivers/usb/usb_host/)
##############################################################################
USB_HOST_MW_DIR := ./drivers/usb/usb_host/stm32-mw-usb-host-master/stm32-mw-usb-host-master

# Core USB Host
USB_HOST_CORE_SRC := $(wildcard $(USB_HOST_MW_DIR)/Core/Src/*.c)
USB_HOST_CORE_SRC := $(filter-out $(USB_HOST_MW_DIR)/Core/Src/usbh_conf_template.c,$(USB_HOST_CORE_SRC))
CSRC   += $(USB_HOST_CORE_SRC)
INCDIR += $(USB_HOST_MW_DIR)/Core/Inc

# Classe MIDI uniquement
CSRC   += $(wildcard $(USB_HOST_MW_DIR)/Class/MIDI/Src/*.c)
INCDIR += $(USB_HOST_MW_DIR)/Class/MIDI/Inc

##############################################################################
# STM32 HAL (REQUIS POUR USB HOST - TES CHEMINS)
##############################################################################
STM32_HAL_DIR := ./drivers/stm32h7xx-hal-driver

CSRC += \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_gpio.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_rcc.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_pwr.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_pwr_ex.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_hcd.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_flash.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_flash_ex.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_cortex.c \
$(STM32_HAL_DIR)/Src/stm32h7xx_hal_sdram.c

##############################################################################
# CMSIS (TES CHEMINS)
##############################################################################
INCDIR += \
./drivers/stm32h7xx-hal-driver/Inc \
./drivers/CMSIS/Include \
./drivers/CMSIS/Device/ST/STM32H7xx/Include

##############################################################################
# Includes (✅ CORRIGÉ POUR FATFS)
##############################################################################
INCDIR += $(CONFDIR) $(ALLINC)

INCDIR += drivers
INCDIR += drivers/HallEffect
INCDIR += drivers/audio
INCDIR += drivers/sdram
INCDIR += drivers/usb
INCDIR += drivers/usb/usb_host
INCDIR += drivers/sd
INCDIR += drivers/sd/ff16
INCDIR += ui

##############################################################################
# Defines
##############################################################################
UDEFS += -DSTM32H743xx
UADEFS =

##############################################################################
# Rules
##############################################################################
RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk
