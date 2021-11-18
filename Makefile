CC := gcc
MAKEFLAGS += --no-print-directory
RED		= \033[1m\033[31m
GREEN	= \033[1m\033[32m
BOLD	= \033[1m
RESET	= \033[0m

TARGETS = utils client server

.PHONY = all clean cleanall 

all: 
	@cd utils/ && make cleanall all ; cd ../client && make cleanall all ; cd ../server && make cleanall all ; cd ../

utils:
	@cd utils && make all

client: 
	@cd client && make all

server: 
	@cd server && make all

cleanall:
	@cd utils && make cleanall
	@cd client && make cleanall
	@cd server && make cleanall
	@cd logs && rm -rf *.log *.txt


