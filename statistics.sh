#!/bin/bash

LOG_FILE="/Users/jacopoceravolo/Desktop/storage-server/logs/server.log"

N_WRITE_SUCCESS=$(grep "\[WRITE FILE\]" -c $LOG_FILE)

echo "NUMBER OF SUCCESSFULL WRITE: ${N_WRITE_SUCCESS}"