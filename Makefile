# pvsneslib SNES build
# Set PVSNESLIB_HOME to your pvsneslib install root.

ifndef PVSNESLIB_HOME
$(error Please set PVSNESLIB_HOME to your pvsneslib path)
endif

TARGET  := starshmup
ROMNAME := $(TARGET)

include $(PVSNESLIB_HOME)/devkitsnes/snes_rules

