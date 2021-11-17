#!/bin/bash
trap terminate SIGINT
terminate(){
    pkill -SIGINT -P $$
    exit
}

./client1 /Users/jacopoceravolo/Desktop/FileStorageServer/logs/client1.log &
echo "Started client 1"

./client2 /Users/jacopoceravolo/Desktop/FileStorageServer/logs/client2.log &
echo "Started client 2"