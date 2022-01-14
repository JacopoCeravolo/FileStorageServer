#!/bin/bash

LOG_FILE=$1

N_OPENCON_SUCCESS=$(grep "\[  openConn  \]  Operation successfull " -c $LOG_FILE)
N_CLOSECON_SUCCESS=$(grep "\[ closeConn  \]  Operation successfull" -c $LOG_FILE)


N_OPEN_SUCCESS=$(grep "\[  openFile  \]  Operation successfull" -c $LOG_FILE)
N_CLOSE_SUCCESS=$(grep "\[ closeFile  \]  Operation successfull" -c $LOG_FILE)


N_WRITE_SUCCESS=$(grep "\[ writeFile  \]  Operation successfull" -c $LOG_FILE)
N_APPEND_SUCCESS=$(grep "\[appendToFile\]  Operation successfull" -c $LOG_FILE)

N_READ_SUCCESS=$(grep "\[  readFile  \]  Operation successfull" -c $LOG_FILE)
BYTES_READ=$(grep -Eo '\[  readFile  \] Operation successfull .+ *[0-9]+' $LOG_FILE | grep -o '[0-9]*' | { sum=0; while read num; do ((sum+=num)); done; echo $sum; } )

N_REMOVE_SUCCESS=$(grep "\[ removeFile \]  Operation successfull" -c $LOG_FILE)
N_LOCK_SUCCESS=$(grep "\[  lockFile  \]  Operation successfull" -c $LOG_FILE)
N_UNLOCK_SUCCESS=$(grep "\[ unlockFile \]  Operation successfull" -c $LOG_FILE)
N_FIFO_REPLACED=$(grep "\[FIFO replace\]  File expelled" -c $LOG_FILE)
N_MAX=$(grep -Eo '\(SERVER\) Maximum number of connections: *[0-9]+' $LOG_FILE | grep -o '[0-9]*' )



echo "NUMBER OF SUCCESSFULL OPEN CONNECTION: ${N_OPENCON_SUCCESS}"
echo "NUMBER OF SUCCESSFULL CLOSE CONNECTION: ${N_CLOSECON_SUCCESS}"
echo "NUMBER OF SUCCESSFULL OPEN: ${N_OPEN_SUCCESS}"
echo "NUMBER OF SUCCESSFULL CLOSE: ${N_CLOSE_SUCCESS}"
echo "NUMBER OF SUCCESSFULL WRITE: ${N_WRITE_SUCCESS}"
echo "NUMBER OF SUCCESSFULL APPEND: ${N_APPEND_SUCCESS}"
echo "NUMBER OF SUCCESSFULL READ: ${N_READ_SUCCESS}"
echo "BYTES READ: ${BYTES_READ}"
echo "NUMBER OF SUCCESSFULL REMOVE: ${N_REMOVE_SUCCESS}"
echo "NUMBER OF SUCCESSFULL LOCK: ${N_LOCK_SUCCESS}"
echo "NUMBER OF SUCCESSFULL UNLOCK: ${N_UNLOCK_SUCCESS}"
echo "MAX CONNECTION: ${N_MAX}"
