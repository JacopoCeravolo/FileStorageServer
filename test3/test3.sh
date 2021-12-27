#!/bin/bash

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BOLD="\033[;1m"
RESET="\033[;0m"

SERVER=./server
CLIENT=./client
start_time=$SECONDS
end=$((SECONDS+30))

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


echo -e "${YELLOW}[TEST 3]${BOLD} current clients running: ${RESET}$(jobs -rp | wc -l)"
echo -e "${YELLOW}[TEST 3]${BOLD} total time elapsed: ${RESET}$((SECONDS-start_time))"

tput civis

while [ $SECONDS -lt $end ]; 
do
   
    for i in {0..100};
    do
    	${CLIENT} -W file$i,file${i+1},file${i+2} -r file${i+1},file${i+2} &
    done

    sleep 0.1
    tput cuu1
    tput cuu1
    echo -e "${YELLOW}[TEST 3]${BOLD} current clients running: ${RESET}$(jobs -rp | wc -l)"
    echo -e "${YELLOW}[TEST 3]${BOLD} total time elapsed: ${RESET}$((SECONDS-start_time))"

    if [[ $(jobs -rp | wc -l) -gt 100 ]];
    then
        sleep 0.2
    fi 

done

tput cnorm

echo ""
echo -e "${YELLOW}[TEST 3]${BOLD} shutting down server${RESET}"
kill -SIGQUIT ${SERVER_PID}

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*     ${GREEN}END OF TEST 3${BOLD}     *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

