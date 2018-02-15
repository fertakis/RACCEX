#!/usr/bin/env python

import time
import sys

TIMEFORMAT = "%m/%d/%y %H:%M:%S"
INTERVAL = 1
#CPU = 2
CPU = 24

cpu = {}

def getTimeList():
    statFile = file("/proc/stat", "r")
    values = statFile.readline().split(" ")[2:10]
    timeList = values
#    for i in range(0, CPU + 1):
#        cpu[i] = values[i].split(" ")[1:6]
    statFile.close() 
    #timeList = cpu[j]
    #print timeList
    for i in range(len(timeList))  :
        timeList[i] = int(timeList[i])
    return timeList

def deltaTime(interval)  :
    x = getTimeList()
    time.sleep(interval)
    #sys.stdin.readline()
    y = getTimeList()
    for i in range(len(x))  :
        y[i] -= x[i]
    return y

if __name__ == "__main__"  :
    while 1:
        dt = deltaTime(INTERVAL)
        timeStamp = time.strftime(TIMEFORMAT)
        cpuPct = 100 - (dt[len(dt) - 5] * 100.00 / sum(dt))
        print "%d %d %d %d %d %d %d %d %d" %(dt[0], dt[1], dt[2], dt[4], dt[5],
dt[6], dt[7], dt[3], cpuPct)
	#print cpuPct
    #print timeStamp + "\t" + str('%.4f' %cpuPct) 
