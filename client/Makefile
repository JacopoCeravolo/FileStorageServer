CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -Wall -g -Wno-pointer-to-int-cast -Wno-unused-variable -D DEBUG
INCLUDES = -I $(ORIGIN)

TARGETS = client 

.PHONY = all clean cleanall 

all: $(TARGETS)

client: client.c
	$(CC) $(INCLUDES) $(CFLAGS) $(FLAGS) $< -o $@ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol -llogger

clean:
	rm -rf client

cleanall:
	rm -rf client *.o *.dSYM