#!/bin/bash

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BOLD="\033[;1m"
RESET="\033[;0m"

SERVER=./server
CLIENT=./client

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*    ${GREEN}STARTING TEST 3${BOLD}    *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

echo -e "${YELLOW}[TEST 3]${BOLD} starting server${RESET}"
${SERVER} & 
SERVER_PID=$!
sleep 1
echo -e "${YELLOW}[TEST 3]${BOLD} server started${RESET}"


echo ""
echo -e "${YELLOW}[TEST 3]${BOLD} starting clients${RESET}"
echo ""

file='client_params.txt'

start_time=$SECONDS
end_time=$((SECONDS+10))

while [ $SECONDS -lt $end_time ]; 
do

    xargs --arg-file=$file -n 4 -P 20 ${CLIENT}
    tput cuu1
    echo -e "${YELLOW}[TEST 3]${BOLD} total time elapsed: ${RESET}$((SECONDS-start_time))"

done


echo ""
echo -e "${YELLOW}[TEST 3]${BOLD} shutting down server${RESET}"
kill -SIGQUIT ${SERVER_PID}

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*     ${GREEN}END OF TEST 3${BOLD}     *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

