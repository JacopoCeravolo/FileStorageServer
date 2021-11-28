#!/bin/bash
server/server &>  server.txt &
export SERVER_PID=$!
echo "Started server"

client/client2 &>  client1.txt &
echo "Started client 1"
client/client2 &>  client2.txt &
echo "Started client 2"

sleep 1
kill -SIGHUP $SERVER_PID
echo "Sent SIGHUP"
