CC := gcc

ORIGIN = $(realpath ../)

INCLUDES = -I $(ORIGIN)
CFLAGS += -Wall -g -Wno-pointer-to-int-cast -Wno-unused-variable -fPIC $(INCLUDES)


TARGET = libfileserver_api.so 
OBJECTS = open_connection.o close_connection.o open_file.o write_file.o \
			read_file.o close_file.o remove_file.o lock_file.o \
			unlock_file.o read_n_files.o

%.o: %.c %.h
	$(CC) $(INCLUDES) $(CFLAGS) -c $<  -o $@ 

libfileserver_api.so: $(OBJECTS) 
	$(CC) -shared -o $(ORIGIN)/libs/$@ $^ -Wl,-rpath,$(ORIGIN)/libs -L $(ORIGIN)/libs -lprotocol

cleanall:
	rm -rf *.o $(ORIGIN)/libs/$(TARGET)