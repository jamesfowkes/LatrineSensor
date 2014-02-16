NAME=LatrineSensor

CC=avr-gcc

ifdef SystemRoot
    RM = del /Q
	CAT = type
	FIXPATH = $(subst /,\,$1)
else
    RM = rm -f
	CAT = cat
endif

MCU_TARGET=attiny84
LIBS_DIR = C:/Users/fowkes_james/Documents/GitHub/Code-Library

OPT_LEVEL=s

INCLUDE_DIRS = \
	-I..\Common \
	-I$(LIBS_DIR)/AVR \
	-I$(LIBS_DIR)/Common \
	-I$(LIBS_DIR)/Devices \
	-I$(LIBS_DIR)/Generics \
	-I$(LIBS_DIR)/Protocols \
	-I$(LIBS_DIR)/Utility

CFILES = \
	latrinesensor.c \
	app_statemachine.c \
	comms.c \
	tempsense.c \
	ircounter.c \
	$(LIBS_DIR)/AVR/lib_clk.c \
	$(LIBS_DIR)/AVR/lib_io.c \
	$(LIBS_DIR)/AVR/lib_fuses.c \
	$(LIBS_DIR)/AVR/lib_adc.c \
	$(LIBS_DIR)/AVR/lib_tmr8.c \
	$(LIBS_DIR)/AVR/lib_tmr8_tick.c \
	$(LIBS_DIR)/AVR/lib_pcint.c \
	$(LIBS_DIR)/Protocols/llap.c \
	$(LIBS_DIR)/Generics/memorypool.c \
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
	
ifdef UART_OPTION
CFILES += $(LIBS_DIR)/AVR/lib_$(UART_OPTION).c

ifeq ($(UART_OPTION), uart)
	OPTS += -DUART
endif

ifeq ($(UART_OPTION), swserial)
	OPTS += -DSWS
endif

endif

LDFLAGS = \
	-Wl,-Map=$(MAPFILE),-gc-sections

OBJDEPS=$(CFILES:.c=.o)

MAPFILE = $(NAME).map

all: $(NAME).elf

	
$(NAME).elf: $(OBJDEPS)
	$(CC) $(INCLUDE_DIRS) $(OPTS) $(LDFLAGS) -O$(OPT_LEVEL) -mmcu=$(MCU_TARGET) -o $@ $^

%.o:%.c
	$(CC) $(INCLUDE_DIRS) $(OPTS) -O$(OPT_LEVEL) -mmcu=$(MCU_TARGET) -c $< -o $@

upload:
	avr-objcopy -R .eeprom -O ihex $(NAME).elf  $(NAME).hex
	avrdude -pt84 -cusbtiny -Uflash:w:$(NAME).hex:a

clean:
	$(RM) $(NAME).elf
	$(RM) $(NAME).hex
	$(RM) $(OBJDEPS)
	$(RM) $(LIBS_DIR)/AVR/lib_swserial.o
	$(RM) $(LIBS_DIR)/AVR/lib_uart.o
