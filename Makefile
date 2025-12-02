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
USE_VERBOSE_COMPILE = yes
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
CHIBIOS := ../../..

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
# Sources
##############################################################################
CSRC = $(ALLCSRC) \
       main.c \
       $(wildcard drivers/*.c) \
       $(wildcard ui/*.c) \

CPPSRC = $(ALLCPPSRC)
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)

##############################################################################
# Includes
##############################################################################
INCDIR = $(CONFDIR) $(ALLINC)
UINCDIR = cfg drivers ui

##############################################################################
# Defines
##############################################################################
UDEFS = 
UADEFS =

##############################################################################
# Rules
##############################################################################
RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/arm-none-eabi.mk
include $(RULESPATH)/rules.mk
