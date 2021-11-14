CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -Wall -g -Wno-pointer-to-int-cast -Wno-unused-variable
INCLUDES = -I $(ORIGIN)

TARGETS = libconcurrent_queue.so libhash_map.so liblinked_list.so libprotocol.so

.PHONY = all clean cleanall 

%.o: %.c %.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -fPIC -o $@ 

all: $(TARGETS)

clean:
	rm -rf *.o
cleanall:
	rm -rf $(ORIGIN)/libs/*.so *.o

libprotocol.so: protocol.o 
	$(CC) -shared -o $(ORIGIN)/libs/$@ $^

libhash_map.so: hash_map.o 
	$(CC) -shared -o $(ORIGIN)/libs/$@ $^

liblinked_list.so: linked_list.o 
	$(CC) -shared -o $(ORIGIN)/libs/$@ $^

libconcurrent_queue.so: concurrent_queue.o linked_list.o 
	$(CC) -shared -o $(ORIGIN)/libs/$@ $^
