# -*- coding: utf-8 -*-
#program to extract microbenchmarks chart
import matplotlib.pyplot as plt
import numpy as np


from os import listdir
from os.path import isfile, join

results = [f for f in listdir("/media/kepler/remotephiexec/scif_benchmarks/cpu_utlization/results_send_recv") 
if isfile(join("/media/kepler/remotephiexec/scif_benchmarks/cpu_utlization/results_send_recv", f))];

native = [];
localhost = [];
racex = [];

for f in results:
    tp = f.split("_")[1]
    if tp == "native":
        native.append(f)
    elif tp == "localhost":
        localhost.append(f)
    elif (tp == "racex"):
        racex.append(f)
#print("printing native", native)
msg_size = []
native_time = []
localhost_time = []
racex_time = []

n = 16;
xx = np.arange(n)
msg_size = 2**xx
msg_size_labels = ["1","2","4","8","16","32","64","128","256","512","1K","2K","4K","8K","16K","32K" ]

for t in msg_size:
    nat_ls=[]
    loc_ls=[]
    rac_ls=[]
    for x in range(1,11):
        f_native = open("/media/kepler/remotephiexec/scif_benchmarks/send_recv/results_send_recv/run_native_" + str(t) +"_"+ str(x) +".out", "r")
        f_local = open("/media/kepler/remotephiexec/scif_benchmarks/send_recv/results_send_recv/run_localhost_" + str(t) +"_"+ str(x) +".out", "r")
        f_racex = open("/media/kepler/remotephiexec/scif_benchmarks/send_recv/results_send_recv/run_racex_" + str(t) +"_"+ str(x) +".out", "r")
        for i, line in enumerate(f_native):
            if i == 4:
                nat_ls.append(int(line.split(" ")[1]))
        for i, line in enumerate(f_local):
            if i == 4:
                loc_ls.append(int(line.split(" ")[1]))
        for i, line in enumerate(f_racex):
            if i == 4:
                rac_ls.append(int(line.split(" ")[1]))
        f_native.close()
        f_local.close()
        f_racex.close()
    nat_ls.sort()
    loc_ls.sort()
    rac_ls.sort()
    native_time.append((nat_ls[4] + nat_ls[5])/2)
    localhost_time.append((loc_ls[4] + loc_ls[5])/2)
    racex_time.append((rac_ls[4] + rac_ls[5])/2)

fig, ax = plt.subplots()
fig.set_size_inches(13, 9, forward=True)
fig.canvas.draw()
ax.plot(xx, native_time, marker='o', label='Native')
ax.plot(xx, localhost_time, marker='s', label='Localhost')
ax.plot(xx, racex_time, marker='^', label='RACEX')
plt.xlabel('Data size(Bytes)')
plt.ylabel('Latency(usec)')
ax.xaxis.set_ticks(xx) #set the ticks to be a
ax.xaxis.set_ticklabels(msg_size_labels)
plt.legend()
plt.savefig("plots/send_recv.png");
plt.show()
#i = 1
#while(i <= 32768):
    
    