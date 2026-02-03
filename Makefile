# pvsneslib SNES build
# Auto-detect pvsneslib from build folder, or use PVSNESLIB_HOME if set

ifndef PVSNESLIB_HOME
PVSNESLIB_HOME := $(CURDIR)/build/pvsneslib
endif

ifeq (,$(wildcard $(PVSNESLIB_HOME)/devkitsnes/snes_rules))
$(error PVSnesLib not found. Either place it in build/pvsneslib or set PVSNESLIB_HOME)
endif

TARGET  := starshmup
ROMNAME := $(TARGET)

# Source files are in root, not src/
SRC := .

include $(PVSNESLIB_HOME)/devkitsnes/snes_rules

all: $(ROMNAME).sfc

clean: cleanBuildRes cleanRom
	@rm -f *.ps *.asp main.asm gfx.asm data.obj

