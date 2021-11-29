CC := gcc

ORIGIN = $(realpath ../)

CFLAGS += -Wall -g -D PRINT=1 #-D DEBUG
INCLUDES = -I $(ORIGIN)

TARGETS = client client1 client2 

.PHONY = all clean cleanall 

%.o: %.c %.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@ 

all: $(TARGETS)

client: client.c option_parser.o
	$(CC) $(INCLUDES) $(CFLAGS) $(FLAGS) -o $@  $^ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -llinked_list -llogger -lfileserver_api 

client1: client1.c
	$(CC) $(INCLUDES) $(CFLAGS) $(FLAGS) $< -o $@ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol -llogger

client2: client2.c
	$(CC) $(INCLUDES) $(CFLAGS) $(FLAGS) $< -o $@ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol -llogger

	

clean:
	rm -rf $(TARGETS)

cleanall:
	rm -rf $(TARGETS) *.o *.dSYM