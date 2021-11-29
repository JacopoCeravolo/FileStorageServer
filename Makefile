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
		
.DEFAULT_GOAL := all

# Default Goal
all:
	@echo "${BOLD}Building application... ${RESET}"
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


