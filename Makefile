CC := gcc


TARGETS = utils client server

.PHONY = all clean cleanall 

all: $(TARGETS)

utils:
	$(MAKE) -C utils

client: utils
	$(MAKE) -C client

server: utils
	$(MAKE) -C server

cleanall:
	@cd utils && make cleanall
	@cd client && make cleanall
	@cd server && make cleanall


