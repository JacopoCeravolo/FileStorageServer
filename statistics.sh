#!/bin/bash

LOG_FILE=$1

N_OPENCON_SUCCESS=$(grep "\[OPEN CONNECTION\]: Operation successfull" -c $LOG_FILE)
N_CLOSECON_SUCCESS=$(grep "\[CLOSE CONNECTION\]: Operation successfull" -c $LOG_FILE)
N_OPEN_SUCCESS=$(grep "\[OPEN FILE\]: Operation successfull" -c $LOG_FILE)
N_CLOSE_SUCCESS=$(grep "\[CLOSE FILE\]: Operation successfull" -c $LOG_FILE)
N_WRITE_SUCCESS=$(grep "\[WRITE FILE\]: Operation successfull" -c $LOG_FILE)
N_APPEND_SUCCESS=$(grep "\[APPEND TO FILE\]: Operation successfull" -c $LOG_FILE)
N_READ_SUCCESS=$(grep "\[READ FILE\]: Operation successfull" -c $LOG_FILE)
N_REMOVE_SUCCESS=$(grep "\[REMOVE FILE\]: Operation successfull" -c $LOG_FILE)
N_LOCK_SUCCESS=$(grep "\[LOCK FILE\]: Operation successfull" -c $LOG_FILE)
N_UNLOCK_SUCCESS=$(grep "\[UNLOCK FILE\]: Operation successfull" -c $LOG_FILE)


echo "NUMBER OF SUCCESSFULL OPEN CONNECTION: ${N_OPENCON_SUCCESS}"
echo "NUMBER OF SUCCESSFULL CLOSE CONNECTION: ${N_CLOSECON_SUCCESS}"
echo "NUMBER OF SUCCESSFULL OPEN: ${N_OPEN_SUCCESS}"
echo "NUMBER OF SUCCESSFULL CLOSE: ${N_CLOSE_SUCCESS}"
echo "NUMBER OF SUCCESSFULL WRITE: ${N_WRITE_SUCCESS}"
echo "NUMBER OF SUCCESSFULL APPEND: ${N_APPEND_SUCCESS}"
echo "NUMBER OF SUCCESSFULL READ: ${N_READ_SUCCESS}"
echo "NUMBER OF SUCCESSFULL REMOVE: ${N_REMOVE_SUCCESS}"
echo "NUMBER OF SUCCESSFULL LOCK: ${N_LOCK_SUCCESS}"
echo "NUMBER OF SUCCESSFULL UNLOCK: ${N_UNLOCK_SUCCESS}"
