#!/bin/bash

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BOLD="\033[;1m"
RESET="\033[;0m"

SERVER=./server
CLIENT=./client
VALGRIND_OUTPUT=$(realpath ./valgrind.log)

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*    ${GREEN}STARTING TEST 1${BOLD}    *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

echo "Full valgrind output can be seen at ${VALGRIND_OUTPUT}"
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} starting server${RESET}"
valgrind --leak-check=full ${SERVER} &> ${VALGRIND_OUTPUT} & 
SERVER_PID=$!
sleep 1
echo -e "${YELLOW}[TEST 1]${BOLD} server started${RESET}"

echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} starting clients${RESET}"
echo ""

#echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -h"
#${CLIENT} -h
#echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -W data/file0,data/file1,data/file2"
${CLIENT} -W data/file0,data/file1,data/file2,data/longlonglonglonglonglonglonglonglong -t 200 -p
echo ""

#echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -w data,10"
#${CLIENT} -w data,10 -p
#echo ""

#echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -r data/file0,data/file1,data/file2"
#${CLIENT} -r data/file0,data/file1,data/file2 -t 200 -p
#echo ""

#echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -R 5"
#${CLIENT} -R 5 -t 200 -p
#echo ""
#
#echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -W data/file0,data/file1,data/file2"
#${CLIENT} -W data/file0,data/file1,data/file2 -t 200 -p
#echo ""

sleep 1

echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} shutting down server${RESET}"
kill -SIGQUIT ${SERVER_PID}

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*     ${GREEN}END OF TEST 1${BOLD}     *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""