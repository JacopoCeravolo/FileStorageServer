CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -g -Wall -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-incompatible-pointer-types \
			-Wno-int-conversion -Wno-int-to-void-pointer-cast -Wno-unused-variable # -D DEBUG
INCLUDES = -I $(ORIGIN)

TARGETS = server

.PHONY = all clean cleanall debug build

%.o: %.c %.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@ 

all: $(TARGETS)

server: server.c storage.o worker.o request_handlers.o signal_handler.o
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ $^ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol -lconcurrent_queue -llinked_list -lhash_map -lpthread -llogger

clean:
	rm -rf server

debug: CFLAGS += -g -DDEBUG=1
debug:
	@make all

build: CFLAGS += -O3 -DBUILD=1
build:
	@make all

cleanall:
	rm -rf server *.o *.dSYM