NAME = filter_test
CC = gcc 
FLAGS = -Wall -Wextra -DTEST_HARNESS -DMEMORY_POOL_BYTES=4096 -std=c99

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
	filter_test.c \
	filter.c \
	$(LIBS_DIR)/Generics/averager.c \
	$(LIBS_DIR)/Generics/memorypool.c \
	$(LIBS_DIR)/Utility/util_sequence_generator.c \
	
all:
	$(CC) $(FLAGS) $(INCLUDE_DIRS) $(OPTS) $(CFILES) -o $(NAME).exe
	$(NAME).exe
