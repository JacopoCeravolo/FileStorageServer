#!/bin/bash

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BOLD="\033[;1m"
RESET="\033[;0m"

SERVER=./server
CLIENT=./client

SERVER_CONFIG=$(realpath ./test1_config.txt)
SERVER_LOG=$(realpath ./server.log)
VALGRIND_OUTPUT=$(realpath ./valgrind.log)

N_WORKERS=1
MAX_SIZE=128000000
MAX_FILES=10000
SOCKET_PATH=/tmp/LSO_socket.sk

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*    ${GREEN}STARTING TEST 1${BOLD}    *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

touch ${SERVER_CONFIG}
echo -e "N_WORKERS=${N_WORKERS}\nMAX_SIZE=${MAX_SIZE}\nMAX_FILES=${MAX_FILES}\nSOCKET_PATH=${SOCKET_PATH}\nLOG_FILE=${SERVER_LOG}" > ${SERVER_CONFIG}

echo -e "${YELLOW}[TEST 1]${BOLD} Server configuration${RESET}"
echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} Number of workers:${RESET} ${N_WORKERS}"
echo -e "${YELLOW}[TEST 1]${BOLD} Maximum storage size:${RESET} ${MAX_SIZE} bytes"
echo -e "${YELLOW}[TEST 1]${BOLD} Maximum number of files:${RESET} ${MAX_FILES}"
echo -e "${YELLOW}[TEST 1]${BOLD} Socket file path:${RESET} ${SOCKET_PATH}"
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Starting server${RESET}"
valgrind --leak-check=full ${SERVER} ${SERVER_CONFIG} &> ${VALGRIND_OUTPUT} & 
SERVER_PID=$!
sleep 2
echo -e "${YELLOW}[TEST 1]${BOLD} Server started${RESET}"


echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} Starting clients${RESET}"
echo ""



echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -W file0,file1,file2,file3,file4,file,file6,file7,file8,file9  -->  expecting success"
${CLIENT} -f ${SOCKET_PATH} -W data/file0,data/file1,data/file2,data/file3,data/file4,data/file5,data/file6,data/file7,data/file8,data/file9 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -W file0,file1,file2  -->  expecting openFile to fail"
${CLIENT} -f ${SOCKET_PATH} -W data/file0,data/file1,data/file2 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -r file0 -a file0,file1 -r file0 -->  expecting success"
${CLIENT} -f ${SOCKET_PATH} -r data/file0 -t 200 -a data/file0,data/file1 -t 200 -r data/file0 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -r file0,file1,file2,file3,file4  --> expecting success"
${CLIENT} -f ${SOCKET_PATH} -r data/file0,data/file1,data/file2,data/file3,data/file4 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -l file3,file4,file5,file6,file7  --> expecting success"
${CLIENT} -f ${SOCKET_PATH} -l data/file3,data/file4,data/file5,data/file6,data/file7 -t 200 -p
echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -u file8,file9  --> expecting failure"
${CLIENT} -f ${SOCKET_PATH} -u data/file8,data/file9 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -l file4,file5 -u file4,file5 --> expecting success"
${CLIENT} -f ${SOCKET_PATH} -l data/file4,data/file5 -u data/file4,data/file5 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -c file0,file1,file2,file3,file4  --> expecting success"
${CLIENT} -f ${SOCKET_PATH} -c data/file0,data/file1,data/file2,data/file3,data/file4 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -c file0,file1  --> expecting failure"
${CLIENT} -f ${SOCKET_PATH} -c data/file0,data/file1 -t 200 -p
echo ""

echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -c file5,file6,file7  --> expecting success"
${CLIENT} -f ${SOCKET_PATH} -c data/file5,data/file6,data/file7 -t 200 -p
echo ""


#echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -w data,10"
#${CLIENT} -w data,10 -p
#echo ""

#echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -r data/file0,data/file1,data/file2"
#${CLIENT} -r data/file0,data/file1,data/file2 -t 200 -p
#echo ""

#echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -R 5"
#${CLIENT} -R 5 -t 200 -p
#echo ""
#
#echo -e "${YELLOW}[TEST 1]${BOLD} Testing:${RESET} -W data/file0,data/file1,data/file2"
#${CLIENT} -W data/file0,data/file1,data/file2 -t 200 -p
#echo ""

sleep 1

echo ""
echo -e "${YELLOW}[TEST 1]${BOLD} Shutting down server${RESET}"
kill -SIGINT ${SERVER_PID}

echo ""
echo -e "${BOLD}*************************${RESET}"
echo -e "${BOLD}*     ${GREEN}END OF TEST 1${BOLD}     *${RESET}"
echo -e "${BOLD}*************************${RESET}"
echo ""

echo ""
echo "Full valgrind output can be seen at ${VALGRIND_OUTPUT}"
echo ""