#!/bin/bash

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BOLD="\033[;1m"
RESET="\033[;0m"

SERVER=./server
CLIENT=./client
end=$((SECONDS+3))

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*    ${GREEN}STARTING TEST 3${BOLD}    *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} starting server${RESET}"
${SERVER} & 
SERVER_PID=$!
sleep 1
echo -e "${YELLOW}[TEST 1]${BOLD} server started${RESET}"


echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} starting clients${RESET}"
echo ""

while [ $SECONDS -lt $end ]; 
do
    for i in {0..5} 
    do
        ${CLIENT} -W file$i,file${i+1},file${i+2} -r file${i+1},file${i+2} &
    done
done

echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} shutting down server${RESET}"
kill -SIGQUIT ${SERVER_PID}

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*     ${GREEN}END OF TEST 1${BOLD}     *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

