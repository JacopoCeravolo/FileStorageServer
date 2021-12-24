# General
CC			:= gcc
LD			:= gcc
RM			:= rm -rf
AR 			:= ar rvs

# Directories
ifndef ORIGIN
ORIGIN		:= $(realpath ../)
endif

ifndef LIBS
LIBS		:= $(ORIGIN)/libs
endif

# Targets
LINKED_LIST 	:= liblinked_list.a
PROTOCOL 		:= libprotocol.a
HASH_MAP 		:= libhash_map.a
CONCURRENT_QUEUE = libconcurrent_queue.a

TARGETS 		:= $(LIBS)/$(LINKED_LIST) $(LIBS)/$(PROTOCOL) \
					$(LIBS)/$(HASH_MAP) $(LIBS)/$(CONCURRENT_QUEUE)

# Compiler Flags
CFLAGS			:= -std=c99 -Wall -g -D_POSIX_C_SOURCE=200112L -D_GNU_SOURCE
INCLUDES		= -I $(ORIGIN)

.PHONY = all clean cleanall 

# General rule for objects
%.o: %.c %.h
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $< 

.PHONY: clean cleanall 
.DEFAULT_GOAL := all

all: $(TARGETS)
	
$(LIBS)/$(PROTOCOL): protocol.o 
	$(AR) -o $@ $^

$(LIBS)/$(LINKED_LIST): linked_list.o 
	$(AR) -o $@ $^

$(LIBS)/$(HASH_MAP): hash_map.o 
	$(AR) -o $@ $^

$(LIBS)/$(CONCURRENT_QUEUE): concurrent_queue.o linked_list.o 
	$(AR) -o $@ $^

clean:
	$(RM) *.o

cleanall:
	$(RM) *.o $(TARGETS)

