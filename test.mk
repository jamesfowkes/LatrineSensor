NAME = latrinesensor_test
CC = gcc 
FLAGS = -Wall -Wextra -lpthread -DTEST_HARNESS -DF_CPU=8000000 -DTX_BUFFER_SIZE=15 -std=c99

LIBS_DIR = ../Libs

INCLUDE_DIRS = \
	-I$(LIBS_DIR)/AVR \
	-I$(LIBS_DIR)/AVR/Harness \
	-I$(LIBS_DIR)/Common \
	-I$(LIBS_DIR)/Devices \
	-I$(LIBS_DIR)/Generics \
	-I$(LIBS_DIR)/Protocols \
	-I$(LIBS_DIR)/Utility \
	-I$(LIBS_DIR)/Utility/libfixmath/libfixmath

CFILES = \
	app_test_harness.c \
	latrinesensor.c \
	comms.c \
	tempsense.c \
	flush_counter.c \
	filter.c \
	$(LIBS_DIR)/AVR/lib_io.c \
	$(LIBS_DIR)/AVR/lib_fuses.c \
	$(LIBS_DIR)/AVR/lib_sleep.c \
	$(LIBS_DIR)/AVR/lib_wdt.c \
	$(LIBS_DIR)/AVR/lib_adc.c \
	$(LIBS_DIR)/AVR/lib_pcint.c \
	$(LIBS_DIR)/Devices/lib_thermistor.c \
	$(LIBS_DIR)/Protocols/llap.c \
	$(LIBS_DIR)/Generics/memorypool.c \
	$(LIBS_DIR)/Generics/ringbuf.c \
	$(LIBS_DIR)/Generics/statemachinemanager.c \
	$(LIBS_DIR)/Generics/statemachine.c \
	
ifdef UART_OPTION
CFILES += $(LIBS_DIR)/AVR/lib_$(UART_OPTION).c

ifeq ($(UART_OPTION), uart)
	OPTS += -DUART
endif

ifeq ($(UART_OPTION), swserial)
	OPTS += -DSWS
endif

endif

all:
	$(CC) $(FLAGS) $(INCLUDE_DIRS) $(OPTS) $(CFILES) -o $(NAME).exe
	$(NAME).exe
