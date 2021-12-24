#!/bin/bash
end=$((SECONDS+3))

while [ $SECONDS -lt $end ]; 
do
    for i in {0..5} 
    do
        ./client -W file$i,file${i+1},file${i+2} -r file${i+1},file${i+2} &
    done
done

