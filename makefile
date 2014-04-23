NAME=LatrineSensor

CC=avr-gcc

RM = rm -f
CAT = cat

MCU_TARGET=atmega328p
AVRDUDE_PART=m328p

LIBS_DIR = $(PROJECTS_PATH)/Libs

OPT_LEVEL=s

INCLUDE_DIRS = \
	-I..\Common \
	-I$(LIBS_DIR)/AVR \
	-I$(LIBS_DIR)/Common \
	-I$(LIBS_DIR)/Devices \
	-I$(LIBS_DIR)/Generics \
	-I$(LIBS_DIR)/Protocols \
	-I$(LIBS_DIR)/Utility \

CFILES = \
	latrinesensor.c \
	comms.c \
	tempsense.c \
	flush_counter.c \
	threshold.c \
	filter.c \
	$(LIBS_DIR)/AVR/lib_clk.c \
	$(LIBS_DIR)/AVR/lib_sleep.c \
	$(LIBS_DIR)/AVR/lib_wdt.c \
	$(LIBS_DIR)/AVR/lib_io.c \
	$(LIBS_DIR)/AVR/lib_fuses.c \
	$(LIBS_DIR)/AVR/lib_adc.c \
	$(LIBS_DIR)/AVR/lib_pcint.c \
	$(LIBS_DIR)/AVR/lib_uart.c \
	$(LIBS_DIR)/AVR/lib_tmr8.c \
	$(LIBS_DIR)/AVR/lib_tmr8_tick.c \
	$(LIBS_DIR)/Protocols/llap.c \
	$(LIBS_DIR)/Devices/lib_thermistor.c \
	$(LIBS_DIR)/Devices/lib_pot_divider.c \
	$(LIBS_DIR)/Generics/memorypool.c \
	$(LIBS_DIR)/Generics/averager.c \
	$(LIBS_DIR)/Generics/ringbuf.c \
	$(LIBS_DIR)/Generics/statemachinemanager.c \
	$(LIBS_DIR)/Generics/statemachine.c

OPTS = \
	-Wall \
	-Wextra \
	-DF_CPU=8000000 \
	-DSUPPRESS_PCINT0 \
	-DSUPPRESS_PCINT1 \
	-DSUPPRESS_PCINT2 \
	-DSUPPRESS_PCINT3 \
	-DMEMORY_POOL_BYTES=128 \
	-DTX_BUFFER_SIZE=15 \
	-ffunction-sections \
	-std=c99
	
LDFLAGS = \
	-Wl,-Map=$(MAPFILE),-gc-sections

LDSUFFIX = -lm

OBJDEPS=$(CFILES:.c=.o)

MAPFILE = $(NAME).map

all: $(NAME).elf

$(NAME).elf: $(OBJDEPS)
	$(CC) $(INCLUDE_DIRS) $(OPTS) $(LDFLAGS) -O$(OPT_LEVEL) -mmcu=$(MCU_TARGET) -o $@ $^ $(LDSUFFIX)
	@avr-size --format=avr --mcu=$(MCU_TARGET) $(NAME).elf

%.o:%.c
	$(CC) $(INCLUDE_DIRS) $(OPTS) -O$(OPT_LEVEL) -mmcu=$(MCU_TARGET) -c $< -o $@

upload-eeprom:
	avr-objcopy -j .eeprom --no-change-warnings --change-section-lma .eeprom=0 -O ihex $(NAME).elf  $(NAME).eep
	avrdude -p $(AVRDUDE_PART) -c usbtiny -Ueeprom:w:$(NAME).eep:a
	
upload:
	avr-objcopy -R .eeprom -O ihex $(NAME).elf  $(NAME).hex
	avrdude -p $(AVRDUDE_PART) -c usbtiny -Uflash:w:$(NAME).hex:a
	
clean:
	$(RM) $(NAME).elf
	$(RM) $(NAME).hex
	$(RM) $(OBJDEPS)
	$(RM) $(LIBS_DIR)/AVR/lib_swserial.o
	$(RM) $(LIBS_DIR)/AVR/lib_uart.o
