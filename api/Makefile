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
API 			:= libfilestorage_api.so
TARGET			:= $(LIBS)/$(API)

# Compiler Flags
CFLAGS			:= -std=c99 -Wall -g
INCLUDES		:= -I $(ORIGIN)

LINK_LIBS		:= -Wl,-rpath,$(LIBS) -L $(LIBS)
L_PROTOCOL 		:= -lprotocol


# General rule for objects
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c -fPIC -o $@ $<

# Rule for target
$(LIBS)/$(API): $(OBJECTS)
	$(CC) -shared -o $@ $^ $(LINK_LIBS) $(L_PROTOCOL)

.PHONY: clean cleanall 
.DEFAULT_GOAL := all

all: $(TARGET)

clean:
	$(RM) $(OBJECTS) 

cleanall:
	$(RM) $(OBJECTS) $(TARGET)