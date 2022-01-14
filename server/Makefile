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
CFLAGS			:= -std=c99 -Wall -g -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE -Wno-deprecated-declarations
INCLUDES		:= -I $(ORIGIN)

LINK_LIBS		:= -L $(LIBS)
L_PROTOCOL 		:= -lprotocol
L_HASHMAP		:= -lhash_map
L_LINKED_LIST	:= -llinked_list
L_UTILITIES		:= -lutils
L_PTHREAD		:= -lpthread
LINK_ALL		:= $(L_PROTOCOL) $(L_HASHMAP) $(L_LINKED_LIST) $(L_UTILITIES)

# General rule for objects
%.o: %.c 
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

# Rule for executable
$(TARGET): $(OBJECTS)
	$(LD) $(CFLAGS) $(INCLUDES) -o $@ $^ -lpthread $(LINK_LIBS) $(LINK_ALL)

# Build Rules
.PHONY: clean cleanall
.DEFAULT_GOAL := all

all: $(TARGET)

clean:
	$(RM) $(OBJECTS) 

cleanall:
	$(RM) $(OBJECTS) $(TARGET)
