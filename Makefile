CC := gcc
MAKEFLAGS += --no-print-directory
RED		= \033[1m\033[31m
GREEN	= \033[1m\033[32m
BOLD	= \033[1m
RESET	= \033[0m

TARGETS = utils client server

.PHONY = all clean cleanall 

all: 
	@echo "${BOLD}Building application... ${RESET}"
	@echo "${BOLD}Building utilities... ${RESET}"
	@make utils
	@echo "${GREEN}Utilities built ${RESET}"
	@echo "${BOLD}Building client... ${RESET}"
	@make client
	@echo "${GREEN}Client built ${RESET}"
	@echo "${BOLD}Building server... ${RESET}"
	@make server
	@echo "${GREEN}Server built ${RESET}"

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


