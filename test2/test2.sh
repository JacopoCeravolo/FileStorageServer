#!/bin/bash

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BOLD="\033[;1m"
RESET="\033[;0m"

SERVER=./server
CLIENT=./client

SERVER_CONFIG=$(realpath ./test2_config.txt)
SERVER_LOG=$(realpath ./server.log)

N_WORKERS=4
MAX_SIZE=1000000
MAX_FILES=10
SOCKET_PATH=/tmp/LSO_server.sk

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*    ${GREEN}STARTING TEST 2${BOLD}    *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

touch ${SERVER_CONFIG}
echo -e "N_WORKERS=${N_WORKERS}\nMAX_SIZE=${MAX_SIZE}\nMAX_FILES=${MAX_FILES}\nSOCKET_PATH=${SOCKET_PATH}\nLOG_FILE=${SERVER_LOG}" > ${SERVER_CONFIG}

echo -e "${YELLOW}[TEST 2]${BOLD} Server configuration${RESET}"
echo ""
echo -e "${YELLOW}[TEST 2]${BOLD} Number of workers:${RESET} ${N_WORKERS}"
echo -e "${YELLOW}[TEST 2]${BOLD} Maximum storage size:${RESET} ${MAX_SIZE} bytes"
echo -e "${YELLOW}[TEST 2]${BOLD} Maximum number of files:${RESET} ${MAX_FILES}"
echo -e "${YELLOW}[TEST 2]${BOLD} Socket file path:${RESET} ${SOCKET_PATH}"
echo ""

echo -e "${YELLOW}[TEST 2]${BOLD} Starting server${RESET}"
${SERVER} ${SERVER_CONFIG} & 
SERVER_PID=$!
sleep 1
echo -e "${YELLOW}[TEST 2]${BOLD} Server started${RESET}"

echo ""
echo -e "${YELLOW}[TEST 2]${BOLD} Starting clients${RESET}"
echo ""

echo -e "${YELLOW}[TEST 2]${BOLD} Writing 10 files${RESET}"
${CLIENT} -f ${SOCKET_PATH} -W data/file1,data/file2,data/file3,data/file4,data/file5,data/file6,data/file7,data/file8,data/file9,data/file10 -p
echo ""

echo -e "${YELLOW}[TEST 2]${BOLD} Writing 10 more files, expecting files 1-10 to be trashed${RESET}"
${CLIENT} -f ${SOCKET_PATH} -W data/file11,data/file12,data/file13,data/file14,data/file15,data/file16,data/file17,data/file18,data/file19 -p
echo ""

echo -e "${YELLOW}[TEST 2]${BOLD} Filling up the whole storage with one big file, expecting files 11-20 to be expelled in expelled_dir/${RESET}"
${CLIENT} -f ${SOCKET_PATH} -W data/big_file -D expelled_dir -p
echo ""

echo -e "${YELLOW}[TEST 2]${BOLD} Expelled directory contents${RESET}"
ls expelled_dir
echo ""

echo ""
echo -e "${YELLOW}[TEST 2]${BOLD} Shutting down server${RESET}"
kill -SIGINT ${SERVER_PID}

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*     ${GREEN}END OF TEST 2${BOLD}     *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""