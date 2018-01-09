#!/usr/bin/python

from socket import *
import sys
import random
import os
import time

MAX_SEND_SIZE = 1000000

if len(sys.argv) < 7:
    sys.stderr.write('Usage: %s <ip> <port> <#trials>\
            <#writes and reads per trial>\
            <max # bytes to write at a time> <#connections> \n' % (sys.argv[0]))
    sys.exit(1)

serverHost = gethostbyname(sys.argv[1])
serverPort = int(sys.argv[2])
numTrials = int(sys.argv[3])
numWritesReads = int(sys.argv[4])
numBytes = int(sys.argv[5])
numConnections = int(sys.argv[6])

if numConnections < numWritesReads:
    sys.stderr.write('<#connections> should be greater than or equal to <#writes and reads per trial>\n')
    sys.exit(1)

socketList = []

# RECV_TOTAL_TIMEOUT = 0.1
# RECV_EACH_TIMEOUT = 0.05

RECV_TOTAL_TIMEOUT = 100
RECV_EACH_TIMEOUT = 50

for i in xrange(numConnections):
    s = socket(AF_INET, SOCK_STREAM)
    s.connect((serverHost, serverPort))
    print 'Connected ', i
    socketList.append(s)


for i in xrange(numTrials):
    socketSubset = []
    randomData = []
    randomLen = []
    socketSubset = random.sample(socketList, numConnections)
    for j in xrange(numWritesReads):
        # random_len = 12 # random.randrange(1, numBytes)
        # random_string = 'abcdefghijkl'# os.urandom(random_len)
        random_len = numBytes # random.randrange(1, numBytes)
        random_string = os.urandom(random_len)
        randomLen.append(random_len)
        randomData.append(random_string)
        socketSubset[j].send(random_string)
        # print 'Sent: ' + random_string

    for j in xrange(numWritesReads):
        data = socketSubset[j].recv(randomLen[j])
        # print 'Received: ' + data
        start_time = time.time()
        while True:
            if len(data) == randomLen[j]:
                break
            socketSubset[j].settimeout(RECV_EACH_TIMEOUT)
            data += socketSubset[j].recv(randomLen[j])
            if time.time() - start_time > RECV_TOTAL_TIMEOUT:
		print "Total Time Exceeded"
                break
        if data != randomData[j]:
            sys.stderr.write("Error: Data received is not the same as sent! \n")
            sys.exit(1)
        else:
            print 'Match'

for i in xrange(numConnections):
    socketList[i].close()

print "Success!"
