CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -Wall -g -Wno-pointer-to-int-cast -Wno-unused-variable -D DEBUG -D PRINT=1
INCLUDES = -I $(ORIGIN)

TARGETS = client1 client2

.PHONY = all clean cleanall 

all: $(TARGETS)

client1: client1.c
	$(CC) $(INCLUDES) $(CFLAGS) $(FLAGS) $< -o $@ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol -llogger

client2: client2.c
	$(CC) $(INCLUDES) $(CFLAGS) $(FLAGS) $< -o $@ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol -llogger

clean:
	rm -rf $(TARGETS)

cleanall:
	rm -rf $(TARGETS) *.o *.dSYM