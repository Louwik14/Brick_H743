# List of all the board related files.
BOARDSRC = $(CHIBIOS)/os/hal/boards/CUSTOM_H743IIT6/board.c

# Required include directories
BOARDINC = $(CHIBIOS)/os/hal/boards/CUSTOM_H743IIT6

# Shared variables
ALLCSRC += $(BOARDSRC)
ALLINC  += $(BOARDINC)
