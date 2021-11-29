#!/bin/bash
for i in {0..5}
do
    client/client1  &> logs/client$i.txt &
    echo "Started client $i"
done

# client/client -W client/file1,client/file2 &> logs/client1.txt &
# echo "Started client 1"
# client/client  -W client/file3,client/file4 &> logs/client2.txt &
# echo "Started client 2"
# client/client  -W client/file4,client/file5&> logs/client3.txt &
# echo "Started client 3"
# client/client  -W client/file5,client/file500 &> logs/client4.txt &
# echo "Started client 4"
# client/client  -W client/file1 -W client/file2 &> logs/client5.txt &
# echo "Started client 5"
# client/client  -W client/file3 -W client/file4 &> logs/client6.txt &
# echo "Started client 6"

