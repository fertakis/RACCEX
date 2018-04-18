# -*- coding: utf-8 -*-
#program to extract microbenchmarks chart
import matplotlib.pyplot as plt
import numpy as np


from os import listdir
from os.path import isfile, join

results = [f for f in listdir("/media/kepler/remotephiexec/scif_benchmarks/read_write/results") 
if isfile(join("/media/kepler/remotephiexec/scif_benchmarks/read_write/results", f))];

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

xx = np.arange(12,23)
msg_size = 2**xx
msg_size_labels = ["4K","8K","16K","32K","64K","128K","256K","512K","1M","2M","4M"]

for t in msg_size:
    f_native = open("/media/kepler/remotephiexec/scif_benchmarks/read_write/results/run_native_" + str(t//4096) + ".out", "r")
    f_local = open("/media/kepler/remotephiexec/scif_benchmarks/read_write/results/run_localhost_" + str(t//4096) + ".out", "r")
    f_racex = open("/media/kepler/remotephiexec/scif_benchmarks/read_write/results/run_racex_" + str(t//4096) + ".out", "r")
    for i, line in enumerate(f_native):
        if i == 5:
            native_time.append(t/int(line.split(" ")[1]))
    for i, line in enumerate(f_local):
        if i == 5:
            localhost_time.append(t/int(line.split(" ")[1]))
    for i, line in enumerate(f_racex):
        if i == 5:
            racex_time.append(t/int(line.split(" ")[1]))
    f_native.close()
    f_local.close()
    f_racex.close()



fig, ax = plt.subplots()
fig.set_size_inches(13, 9, forward=True)
fig.canvas.draw()
ax.plot(xx, native_time, marker='o', label='Native')
ax.plot(xx, localhost_time, marker='s', label='Localhost')
ax.plot(xx, racex_time, marker='^', label='RACEX')
plt.xlabel('Data size(Bytes)')
plt.ylabel('Throughput(MB/s)')
ax.xaxis.set_ticks(xx) #set the ticks to be a
ax.xaxis.set_ticklabels(msg_size_labels)
plt.legend()
plt.savefig("plots/read_write.png");
plt.show()

    