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

echo -e "${YELLOW}[TEST 1]${BOLD} starting server${RESET}"
valgrind --leak-check=full ${SERVER} &> ${VALGRIND_OUTPUT} & 
SERVER_PID=$!
sleep 1
echo -e "${YELLOW}[TEST 1]${BOLD} server started${RESET}"


echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} starting clients${RESET}"
echo ""



echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -W file0,file1,file2,file3,file4,file,file6,file7,file8,file9  -->  expecting success"
${CLIENT} -W data/file0,data/file1,data/file2,data/file3,data/file4,data/file5,data/file6,data/file7,data/file8,data/file9 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -W file0,file1,file2  -->  expecting openFile to fail"
${CLIENT} -W data/file0,data/file1,data/file2 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -r file0,file1,file2,file3,file4  --> expecting success"
${CLIENT} -r data/file0,data/file1,data/file2,data/file3,data/file4 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -l file3,file4,file5,file6,file7  --> expecting success"
${CLIENT} -l data/file3,data/file4,data/file5,data/file6,data/file7 -t 200 -p
echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -u file8,file9  --> expecting failure"
${CLIENT} -u data/file8,data/file9 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -l file4,file5 -u file4,file5 --> expecting success"
${CLIENT} -l data/file4,data/file5 -u data/file4,data/file5 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -c file0,file1,file2,file3,file4  --> expecting success"
${CLIENT} -c data/file0,data/file1,data/file2,data/file3,data/file4 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -c file0,file1  --> expecting failure"
${CLIENT} -c data/file0,data/file1 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} testing:${RESET} -c file5,file6,file7  --> expecting success"
${CLIENT} -c data/file5,data/file6,data/file7 -t 200 -p
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

echo ""
echo "Full valgrind output can be seen at ${VALGRIND_OUTPUT}"
echo ""