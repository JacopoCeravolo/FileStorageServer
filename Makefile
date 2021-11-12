CC := gcc
CFLAGS += -Wall -g

TARGETS = test_list test_map test_storage client server

.PHONY = all clean cleanall exec_test 

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $< 

all: $(TARGETS)

exec_test: $(TARGETS)
	@echo "****************"
	@echo "TESTING LIST"
	@echo "****************"
	./test_list
	@echo "****************"
	@echo "TESTING HASHMAP"
	@echo "****************"
	./test_map
	@echo "****************"
	@echo "TESTING FINISHED"
	@echo "****************"

test_list: test_list.c linked_list.o 
	$(CC) $(CFLAGS) -o $@ $^

test_map: test_map.c hash_map.o 
	$(CC) $(CFLAGS) -o $@ $^

test_storage: test_storage.c hash_map.o storage.o
	$(CC) $(CFLAGS) -o $@ $^

client: client.c protocol.o
	$(CC) $(CFLAGS) -o $@ $^

server: utilities.h server.c storage.o hash_map.o protocol.o concurrent_queue.o linked_list.o 
	$(CC) $(CFLAGS) -o $@ $^ -lpthread


clean: 
	rm -rf *.o *.dSYM $(TARGETS)

cleanall:
	rm -rf *.o *.txt *.dSYM $(TARGETS) 


