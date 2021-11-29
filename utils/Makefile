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

# Targets
LOGGER 			:= liblogger.so
LINKED_LIST 	:= liblinked_list.so
PROTOCOL 		:= libprotocol.so
HASH_MAP 		:= libhash_map.so
CONCURRENT_QUEUE = libconcurrent_queue.so

TARGETS 		:= $(LIBS)/$(LOGGER) $(LIBS)/$(LINKED_LIST) $(LIBS)/$(PROTOCOL) \
					$(LIBS)/$(HASH_MAP) $(LIBS)/$(CONCURRENT_QUEUE)

# Compiler Flags
CFLAGS			:= -std=c99 -Wall -Werror -g
INCLUDES		= -I $(ORIGIN)

.PHONY = all clean cleanall 

# General rule for objects
%.o: %.c %.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -MP -fPIC -o $@ $< 

.PHONY: clean cleanall 
.DEFAULT_GOAL := all

all: $(TARGETS)

$(LIBS)/$(LOGGER): logger.o 
	$(CC) -shared -o $@ $^
	
$(LIBS)/$(PROTOCOL): protocol.o 
	$(CC) -shared -o $@ $^

$(LIBS)/$(LINKED_LIST): linked_list.o 
	$(CC) -shared -o $@ $^

$(LIBS)/$(HASH_MAP): hash_map.o 
	$(CC) -shared -o $@ $^

$(LIBS)/$(CONCURRENT_QUEUE): concurrent_queue.o linked_list.o 
	$(CC) -shared -o $@ $^

clean:
	$(RM) *.o

cleanall:
	$(RM) *.o $(TARGETS)

