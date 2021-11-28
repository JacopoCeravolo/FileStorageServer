#!/bin/bash
trap terminate SIGINT
terminate(){
    pkill -SIGINT -P $$
    exit
}

./client2 &> client1.txt &
sleep 4
./client2 &> client2.txt 
