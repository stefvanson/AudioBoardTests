# The name of your project (used to name the compiled .hex file)
TARGET = $(notdir $(CURDIR))

#************************************************************************
# Path names for Teensy build
#************************************************************************

# directory to build in
BUILDDIR = ./build

# Path to your arduino installation, e.g. home/<user_name>/teensyduino/arduino-1.8.13
ARDUINOPATH ?= ../../../../..

# path location for Teensy Loader, teensy_post_compile and teensy_reboot
TOOLSPATH = $(ARDUINOPATH)/hardware/tools/

# path location for Teensy 4 core
COREDIRNAME  = teensy4
COREPATH     = lib/teensy/$(COREDIRNAME)

# path location for the arm-none-eabi compiler
COMPILERPATH = $(TOOLSPATH)/arm/bin/

#************************************************************************
# Settings below this point usually do not need to be edited
#************************************************************************

# for Cortex M7 with single & double precision FPU
CPUOPTIONS = -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv5-d16 -mthumb
MCU_LD = $(COREPATH)/imxrt1062.ld

# compiler options for C only
CFLAGS =

# compiler options for C++ only
CXXFLAGS = -std=gnu++14 -felide-constructors -fno-exceptions -fpermissive -fno-rtti -Wno-error=narrowing

# linker options
CPPFLAGS = -Wall -O2 -ffunction-sections -fdata-sections
CPPFLAGS += $(CPUOPTIONS)
CPPFLAGS += -I$(COREPATH) -Ilib
CPPFLAGS += -DF_CPU=600000000 -DUSB_SERIAL -DLAYOUT_US_ENGLISH -DUSING_MAKEFILE -DCUSTOM_TEENSY_MAIN_CPP
CPPFLAGS += -D__IMXRT1062__ -DARDUINO=10810 -DTEENSYDUINO=149 -DARDUINO_TEENSY40

LDFLAGS = -Os -Wl,--gc-sections,--relax,--print-memory-usage $(CPUOPTIONS) -T$(MCU_LD)
LIBS = -lm -lstdc++ -larm_cortexM7lfsp_math

# names for the compiler programs
CC = $(COMPILERPATH)/arm-none-eabi-gcc
CXX = $(COMPILERPATH)/arm-none-eabi-g++
OBJCOPY = $(COMPILERPATH)/arm-none-eabi-objcopy
SIZE = $(COMPILERPATH)/arm-none-eabi-size

# automatically create lists of the sources and objects
TC_FILES := $(wildcard $(COREPATH)/*.c)
TCPP_FILES := $(filter-out $(COREPATH)/main.cpp, $(wildcard $(COREPATH)/*.cpp))
C_FILES := $(shell find src -name '*.c')
CPP_FILES := $(shell find src -name '*.cpp')

# include paths for libraries
SOURCES := $(C_FILES:.c=.o) $(CPP_FILES:.cpp=.o) $(TC_FILES:%.c=%.o) $(TCPP_FILES:%.cpp=%.o)
OBJS := $(foreach src, $(SOURCES), $(BUILDDIR)/$(src))

all: hex

build: $(BUILDDIR)/$(TARGET).elf

hex: $(BUILDDIR)/$(TARGET).hex

post_compile: $(BUILDDIR)/$(TARGET).hex
	@$(abspath $(TOOLSPATH))/teensy_post_compile -file="$(basename $<)" -path=$(CURDIR) -tools="$(abspath $(TOOLSPATH))"

reboot:
	@-$(abspath $(TOOLSPATH))/teensy_reboot

upload: post_compile reboot

#####################################################
#				Building for teensy
#####################################################

# Compiling the project specific source files

$(BUILDDIR)/%.o: %.c
	@echo "[CC]  $<"
	@mkdir -p "$(dir $@)"
	@$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ -c "$<"

$(BUILDDIR)/%.o: %.cpp
	@echo "[CXX] $<"
	@mkdir -p "$(dir $@)"
	@$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ -c "$<"

$(BUILDDIR)/$(TARGET).elf: $(OBJS) $(LDSCRIPT)
	@echo "[LD]  $@"
	@$(CXX) $(LDFLAGS) $(LIBS) $(OBJS) -o $@


%.hex: %.elf
	@echo "[HEX] $@"
	@$(SIZE) "$<"
	@$(OBJCOPY) -O ihex -R .eeprom "$<" "$@"

# compiler generated dependency info
-include $(OBJS:.o=.d)


#####################################################
# 					Cleaning up
#####################################################

clean:
	rm -rf $(BUILDDIR)/src $(BUILDDIR)/lib/audio

clean_all:
	rm -rf $(BUILDDIR)
