CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -Wall -g -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-int-to-void-pointer-cast -Wno-unused-variable
INCLUDES = -I $(ORIGIN)

TARGETS = server

.PHONY = all clean cleanall 

%.o: %.c %.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@ 

all: $(TARGETS)

server: server.c storage.o worker.o
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ $^ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol -lconcurrent_queue -llinked_list -lhash_map -lpthread

clean:
	rm -rf server

cleanall:
	rm -rf server *.o *.dSYM