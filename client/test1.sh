#!/bin/bash
trap terminate SIGINT
terminate(){
    pkill -SIGINT -P $$
    exit
}

./client1 /Users/jacopoceravolo/Desktop/FileStorageServer/logs/client1.log &
./client2 /Users/jacopoceravolo/Desktop/FileStorageServer/logs/client2.log &
