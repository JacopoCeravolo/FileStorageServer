# Project directory
ORIGIN 	= $(realpath ./)
export ORIGIN

# Libraries directory
LIBS 	= $(ORIGIN)/libs
export LIBS

# Project sub-modules
CLIENT 	= $(ORIGIN)/client
API		= $(ORIGIN)/api
SERVER 	= $(ORIGIN)/server
UTILS 	= $(ORIGIN)/utils
LOGS 	= $(ORIGIN)/logs
TEST1 	= $(ORIGIN)/test1
TEST2 	= $(ORIGIN)/test2
TEST3 	= $(ORIGIN)/test3

# Printing
RED		= \033[1m\033[31m
GREEN	= \033[1m\033[32m
BOLD	= \033[1m
RESET	= \033[0m

# Make flags
MAKEFLAGS += --no-print-directory
export MAKEFLAGS

# Targets
.PHONY: client cleanclient 	\
		server cleanserver 	\
		api cleanapi 		\
		utils cleanutils	\
		cleanall 			\
		test1 test2 test3	
		
.DEFAULT_GOAL := all

# Default Goal
all:
	@echo "${BOLD}Building utilities... ${RESET}"
	@make utils
	@echo "${GREEN}Utilities built ${RESET}"
	@echo "${BOLD}Building API... ${RESET}"
	@make api
	@echo "${GREEN}API built ${RESET}"
	@echo "${BOLD}Building client... ${RESET}"
	@make client
	@echo "${GREEN}Client built ${RESET}"
	@echo "${BOLD}Building server... ${RESET}"
	@make server
	@echo "${GREEN}Server built ${RESET}"


client:
	$(MAKE) -C $(CLIENT)

server:
	$(MAKE) -C $(SERVER)

api:
	$(MAKE) -C $(API)

utils:
	$(MAKE) -C $(UTILS)

# Cleaning
cleanclient:
	@cd client && make cleanall
	@echo "${GREEN}Client removed ${RESET}"

cleanserver:
	@cd server && make cleanall
	@echo "${GREEN}Server removed ${RESET}"

cleanapi:
	@cd api && make cleanall
	@echo "${GREEN}API removed ${RESET}"

cleanutils:
	@cd utils && make cleanall
	@echo "${GREEN}Utilites removed ${RESET}"


cleanall:
	@echo "${BOLD}Cleaning up... ${RESET}"
	@make cleanutils
	@make cleanapi
	@make cleanclient
	@make cleanserver 
	@echo "${BOLD}Directories cleaned${RESET}"


SERVER_EXEC = $(SERVER)/server
CLIENT_EXEC = $(CLIENT)/client

tests: test1 test3

test1:
	@make all
	@cp $(SERVER_EXEC) $(TEST1)
	@cp $(CLIENT_EXEC) $(TEST1)
	@cd $(TEST1) && chmod +x ./test1.sh && ./test1.sh

test3:
	@make all
	@mv $(SERVER_EXEC) $(TEST3)
	@mv $(CLIENT_EXEC) $(TEST3)
	@cd $(TEST3) && chmod +x ./test3.sh && ./test3.sh
