#!/bin/bash

LOG_FILE="/Users/jacopoceravolo/Desktop/storage-server/logs/server.log"

N_WRITE_SUCCESS=$(grep "\[WRITE FILE\]: Operation successfull" -c $LOG_FILE)
N_READ_SUCCESS=$(grep "\[READ FILE\]: Operation successfull" -c $LOG_FILE)

echo "NUMBER OF SUCCESSFULL WRITE: ${N_WRITE_SUCCESS}"
echo "NUMBER OF SUCCESSFULL READ: ${N_READ_SUCCESS}"