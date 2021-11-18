#!/bin/bash
trap terminate SIGINT
terminate(){
    pkill -SIGINT -P $$
    exit
}

for i in {0..10}
do
    ./client1 /Users/jacopoceravolo/Desktop/FileStorageServer/logs/client$i.log &
  echo "Started client $i"
done



