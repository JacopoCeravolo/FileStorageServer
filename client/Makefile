CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -Wall -g -Wno-pointer-to-int-cast -Wno-unused-variable
INCLUDES = -I $(ORIGIN)

TARGETS = client

.PHONY = all clean cleanall 

all: $(TARGETS)

client: client.c
	$(CC) $(INCLUDES) $(CFLAGS) $< -o $@ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol

clean:
	rm -rf client

cleanall:
	rm -rf client *.o *.dSYM