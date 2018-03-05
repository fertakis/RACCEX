# -*- coding: utf-8 -*-
#program to extract microbenchmarks chart
import matplotlib.pyplot as plt
import numpy as np


from os import listdir
from os.path import isfile, join

results = [f for f in listdir("/home/konstantinos/repos/remotephiexec/scif_benchmarks/send_recv/results") 
if isfile(join("/home/konstantinos/repos/remotephiexec/scif_benchmarks/send_recv/results", f))];

native = [];
localhost = [];
rphi = [];

for f in results:
    tp = f.split("_")[1]
    if tp == "native":
        native.append(f)
    elif tp == "localhost":
        localhost.append(f)
    elif (tp == "rphi"):
        rphi.append(f)
#print("printing native", native)
msg_size = []
native_time = []
localhost_time = []
rphi_time = []

n = 16;
xx = np.arange(n)
msg_size = 2**xx
msg_size_labels = ["1","2","4","8","16","32","64","128","256","512","1K","2K","4K","8K","16K","32K" ]

for t in msg_size:
    f_native = open("/home/konstantinos/repos/remotephiexec/scif_benchmarks/send_recv/results/run_native_" + str(t) + ".out", "r")
    f_local = open("/home/konstantinos/repos/remotephiexec/scif_benchmarks/send_recv/results/run_localhost_" + str(t) + ".out", "r")
    f_rphi = open("/home/konstantinos/repos/remotephiexec/scif_benchmarks/send_recv/results/run_rphi_" + str(t) + ".out", "r")
    for i, line in enumerate(f_native):
        if i == 4:
            native_time.append(int(line.split(" ")[1]))
    for i, line in enumerate(f_local):
        if i == 4:
            localhost_time.append(int(line.split(" ")[1]))
    for i, line in enumerate(f_rphi):
        if i == 4:
            rphi_time.append(int(line.split(" ")[1]))
    f_native.close()
    f_local.close()
    f_rphi.close()



fig, ax = plt.subplots()
fig.set_size_inches(13, 9, forward=True)
fig.canvas.draw()
ax.plot(xx, native_time, marker='o', label='Native')
ax.plot(xx, localhost_time, marker='s', label='Localhost')
ax.plot(xx, rphi_time, marker='^', label='rPHI')
plt.xlabel('Data size(Bytes)')
plt.ylabel('Latency(usec)')
ax.xaxis.set_ticks(xx) #set the ticks to be a
ax.xaxis.set_ticklabels(msg_size_labels)
plt.legend()
plt.savefig("plots/send_recv.png");
plt.show()
#i = 1
#while(i <= 32768):
    
    