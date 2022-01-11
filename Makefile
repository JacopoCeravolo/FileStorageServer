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

# Final executables
SERVER_EXEC = $(SERVER)/server
CLIENT_EXEC = $(CLIENT)/client

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
		test1 cleantest1	\
		test2 cleantest2	\
		test3 cleantest3	\	
		cleanall 			
		
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


tests: test1 test2 test3

test1:
	@make all
	@cp $(SERVER_EXEC) $(TEST1)
	@cp $(CLIENT_EXEC) $(TEST1)
	@cd $(TEST1) && chmod +x ./test1.sh && ./test1.sh

test2:
	@make all
	@cp $(SERVER_EXEC) $(TEST2)
	@cp $(CLIENT_EXEC) $(TEST2)
	@cd $(TEST2) && chmod +x ./test2.sh && ./test2.sh

test3:
	@make all
	@mv $(SERVER_EXEC) $(TEST3)
	@mv $(CLIENT_EXEC) $(TEST3)
	@cd $(TEST3) && chmod +x ./test3.sh && ./test3.sh


# Cleaning
cleanclient:
	@cd client && make cleanall
	@echo "${GREEN}Client cleaned ${RESET}"

cleanserver:
	@cd server && make cleanall
	@echo "${GREEN}Server cleaned ${RESET}"

cleanapi:
	@cd api && make cleanall
	@echo "${GREEN}API cleaned ${RESET}"

cleanutils:
	@cd utils && make cleanall
	@echo "${GREEN}Utilites cleaned ${RESET}"

cleantest1:
	@cd $(TEST1) && rm -rf server client test1_config.txt *.log
	@echo "${GREEN}Test 1 cleaned ${RESET}"

cleantest2:
	@cd $(TEST2) && rm -rf server client expelled_dir test2_config.txt *.log
	@echo "${GREEN}Test 2 cleaned ${RESET}"

cleantest3:
	@cd $(TEST3) && rm -rf server client test3_config.txt *.log
	@echo "${GREEN}Test 3 cleaned ${RESET}" 


cleanall:
	@echo "${BOLD}Cleaning up... ${RESET}"
	@make cleanutils
	@make cleanapi
	@make cleanclient
	@make cleanserver 
	@make cleantest1
	@make cleantest2
	@make cleantest3
	@echo "${BOLD}Directories cleaned${RESET}"

