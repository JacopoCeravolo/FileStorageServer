#!/bin/bash

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BOLD="\033[;1m"
RESET="\033[;0m"

SERVER=./server
CLIENT=./client

SERVER_CONFIG=$(realpath ./test3_config.txt)
SERVER_LOG=$(realpath ./server.log)

N_WORKERS=8
MAX_SIZE=32000000
MAX_FILES=100
SOCKET_PATH=/tmp/LSO_server.sk

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*    ${GREEN}STARTING TEST 3${BOLD}    *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

touch ${SERVER_CONFIG}
echo -e "N_WORKERS=${N_WORKERS}\nMAX_SIZE=${MAX_SIZE}\nMAX_FILES=${MAX_FILES}\nSOCKET_PATH=${SOCKET_PATH}\nLOG_FILE=${SERVER_LOG}" > ${SERVER_CONFIG}

echo -e "${YELLOW}[TEST 3]${BOLD} Server configuration${RESET}"
echo ""
echo -e "${YELLOW}[TEST 3]${BOLD} Number of workers:${RESET} ${N_WORKERS}"
echo -e "${YELLOW}[TEST 3]${BOLD} Maximum storage size:${RESET} ${MAX_SIZE} bytes"
echo -e "${YELLOW}[TEST 3]${BOLD} Maximum number of files:${RESET} ${MAX_FILES}"
echo -e "${YELLOW}[TEST 3]${BOLD} Socket file path:${RESET} ${SOCKET_PATH}"
echo ""

echo -e "${YELLOW}[TEST 3]${BOLD} Starting server${RESET}"
${SERVER} ${SERVER_CONFIG} & 
SERVER_PID=$!
sleep 1
echo -e "${YELLOW}[TEST 3]${BOLD} Server started${RESET}"


echo ""
echo -e "${YELLOW}[TEST 3]${BOLD} Starting clients${RESET}"
echo ""

file='client_params.txt'

start_time=$SECONDS
end_time=$((SECONDS+10))

while [ $SECONDS -lt $end_time ]; 
do

    xargs --arg-file=$file -n 6 -P 20 ${CLIENT}
    echo -e "${YELLOW}[TEST 3]${BOLD} Total time elapsed: ${RESET}$((SECONDS-start_time)) s"
    tput cuu1

done


echo ""
echo -e "${YELLOW}[TEST 3]${BOLD} Shutting down server${RESET}"
kill -SIGINT ${SERVER_PID}

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*     ${GREEN}END OF TEST 3${BOLD}     *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

