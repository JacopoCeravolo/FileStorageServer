CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -Wall -g -Wno-pointer-to-int-cast
INCLUDES = -I $(ORIGIN)

TARGETS = client

.PHONY = all clean cleanall 

all: $(TARGETS)

client: client.c
	$(CC) $(INCLUDES) $(CFLAGS) $< -o $@ -L $(ORIGIN)/libs -lprotocol

clean:
	rm -rf client

cleanall:
	rm -rf client *.o *.dSYM