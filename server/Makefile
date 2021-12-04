# General
CC			:= gcc
LD			:= gcc
RM			:= rm -rf

# Directories
ifndef ORIGIN
ORIGIN		:= $(realpath ../)
endif

ifndef LIBS
LIBS		:= $(ORIGIN)/libs
endif

# Source, Objects and Target
SOURCES			:= $(shell find . -type f -name '*.c')
OBJECTS			:= $(patsubst %.c,%.o,$(SOURCES))
TARGET			:= server

# Compiler Flags
CFLAGS			:= -std=c99 -Wall -g 
INCLUDES		:= -I $(ORIGIN)

LINK_LIBS		:= -Wl,-rpath,$(LIBS) -L $(LIBS)
L_PROTOCOL 		:= -lprotocol
L_LOGGER		:= -llogger
L_HASHMAP		:= -lhash_map
L_LINKED_LIST	:= -llinked_list
L_CONC_QUEUE	:= -lconcurrent_queue
LINK_ALL		:= $(L_PROTOCOL) $(L_LOGGER) $(L_HASHMAP) $(L_LINKED_LIST) $(L_CONC_QUEUE)

# General rule for objects
%.o: %.c 
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Rule for executable
$(TARGET): $(OBJECTS)
	$(LD) $(CFLAGS) $(INCLUDES) -o $@ $^ $(LINK_LIBS) $(LINK_ALL)

# Build Rules
.PHONY: clean cleanall
.DEFAULT_GOAL := all

all: $(TARGET)

clean:
	$(RM) $(OBJECTS) 

cleanall:
	$(RM) $(OBJECTS) $(TARGET)