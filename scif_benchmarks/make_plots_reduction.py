# -*- coding: utf-8 -*-
#program to extract microbenchmarks chart
import matplotlib.pyplot as plt
import numpy as np


from os import listdir
from os.path import isfile, join

results = [f for f in listdir("/media/kepler/remotephiexec/scif_benchmarks/shoc-mic/reduction") 
if isfile(join("/media/kepler/remotephiexec/scif_benchmarks/shoc-mic/reduction", f))];

native_simple = [];
native_simple_dp = [];
native_pcie = [];
native_pcie_dp = [];
native_parity = [];
native_parity_dp = []

localhost_simple = [];
localhost_simple_dp = [];
localhost_pcie = [];
localhost_pcie_dp = [];
localhost_parity = [];
localhost_parity_dp = []

racex_simple = [];
racex_simple_dp = [];
racex_pcie = [];
racex_pcie_dp = [];
racex_parity = [];
racex_parity_dp = []

#print("printing native", native)
msg_size = np.arange(1,5)
msg_size_labels = ["1","8","32","64"]
num_thr = [56, 112, 224]
for th in num_thr:
    for t in msg_size:
        f_native = open("/media/kepler/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_native_" + str(th) +"_s"+ str(t) +".log", "r")
        f_local = open("/media/kepler/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_localhost_" + str(th) +"_s"+ str(t) +".log", "r")
        f_racex = open("/media/kepler/remotephiexec/scif_benchmarks/shoc-mic/reduction/reduction_racex_" + str(th) +"_s"+ str(t) +".log", "r")
        for i, line in enumerate(f_native):
            if i == 27:
                native_simple.append(float(line.split("\t")[3]))
            if i == 28:
                native_simple_dp.append(float(line.split("\t")[3]))
            if i == 29:
                native_pcie_dp.append(float(line.split("\t")[3]))
            if i == 30:
                native_parity_dp.append(float(line.split("\t")[3]))
            if i == 31:
                native_pcie.append(float(line.split("\t")[3]))
            if i == 32:
                native_parity.append(float(line.split("\t")[3]))
        for i, line in enumerate(f_local):
            if i == 27:
                localhost_simple.append(float(line.split("\t")[3]))
            if i == 28:
                localhost_simple_dp.append(float(line.split("\t")[3]))
            if i == 29:
                localhost_pcie_dp.append(float(line.split("\t")[3]))
            if i == 30:
                localhost_parity_dp.append(float(line.split("\t")[3]))
            if i == 31:
                localhost_pcie.append(float(line.split("\t")[3]))
            if i == 32:
               localhost_parity.append(float(line.split("\t")[3]))
        for i, line in enumerate(f_racex):
            if i == 27:
                racex_simple.append(float(line.split("\t")[3]))
            if i == 28:
                racex_simple_dp.append(float(line.split("\t")[3]))
            if i == 29:
                racex_pcie_dp.append(float(line.split("\t")[3]))
            if i == 30:
                racex_parity_dp.append(float(line.split("\t")[3]))
            if i == 31:
                racex_pcie.append(float(line.split("\t")[3]))
            if i == 32:
                racex_parity.append(float(line.split("\t")[3]))
        f_native.close()
        f_local.close()
        f_racex.close()        

    fig, axes = plt.subplots(2,3)
    fig.set_size_inches(20, 10, forward=True)
    fig.canvas.draw()
    #first column
    axes[0, 0].plot(msg_size, native_simple, marker='o', label='Native')
    axes[1, 0].plot(msg_size, native_simple_dp, marker='o', label='Native DP')
    axes[0, 0].plot(msg_size, localhost_simple, marker='s', label='Localhost')
    axes[1, 0].plot(msg_size, localhost_simple_dp, marker='s', label='Localhost DP')
    axes[0, 0].plot(msg_size, racex_simple, marker='^', label='RACEX')
    axes[1, 0].plot(msg_size, racex_simple_dp, marker='^', label='RACEX DP')
    axes[0, 0].set_title('Reduction')
    axes[0, 0].set_xlabel('Problem Size - Vector Size (MB)')
    axes[0, 0].set_ylabel('Rate(GB/s)')
    axes[1, 0].set_title("Reduction DP")
    axes[1, 0].set_xlabel('Problem Size - Vector Size (MB)')
    axes[1, 0].set_ylabel('Rate(GB/s)')
    axes[0, 0].xaxis.set_ticks(msg_size) #set the ticks to be a
    axes[0, 0].xaxis.set_ticklabels(msg_size_labels)
    axes[1, 0].xaxis.set_ticks(msg_size) #set the ticks to be a
    axes[1, 0].xaxis.set_ticklabels(msg_size_labels)
    
    #second column
    axes[0, 1].plot(msg_size, native_pcie, marker='o', label='Native PCIe')
    axes[1, 1].plot(msg_size, native_pcie_dp, marker='o', label='Native PCIe DP')
    axes[0, 1].plot(msg_size, localhost_pcie, marker='s', label='Localhost PCIe')
    axes[1, 1].plot(msg_size, localhost_pcie_dp, marker='s', label='Localhost PCIe DP')
    axes[0, 1].plot(msg_size, racex_pcie, marker='^', label='RACEX PCIe')
    axes[1, 1].plot(msg_size, racex_pcie_dp, marker='^', label='RACEX PCIe DP')
    axes[0, 1].set_title('Reduction PCIe')
    axes[0, 1].set_xlabel('Problem Size - Vector Size (MB)')
    axes[0, 1].set_ylabel('Rate(GB/s)')
    axes[1, 1].set_title("Reduction PCIe DP")
    axes[1, 1].set_xlabel('Problem Size - Vector Size (MB)')
    axes[1, 1].set_ylabel('Rate(GB/s)')
    axes[0, 1].xaxis.set_ticks(msg_size) #set the ticks to be a
    axes[0, 1].xaxis.set_ticklabels(msg_size_labels)
    axes[1, 1].xaxis.set_ticks(msg_size) #set the ticks to be a
    axes[1, 1].xaxis.set_ticklabels(msg_size_labels)
    
     #thirdcolumn
    axes[0, 2].plot(msg_size, native_parity, marker='o', label='Native Parity')
    axes[1, 2].plot(msg_size, native_parity_dp, marker='o', label='Native Parity DP')
    axes[0, 2].plot(msg_size, localhost_parity, marker='s', label='Localhost Parity')
    axes[1, 2].plot(msg_size, localhost_parity_dp, marker='s', label='Localhost Parity DP')
    axes[0, 2].plot(msg_size, racex_parity, marker='^', label='RACEX Parity')
    axes[1, 2].plot(msg_size, racex_parity_dp, marker='^', label='RACEX Parity DP')
    axes[0, 2].set_title('Reduction Parity')
    axes[0, 2].set_xlabel('Problem Size - Vector Size (MB)')
    axes[0, 2].set_ylabel('Rate(GB/s)')
    axes[1, 2].set_title("Reduction Parity DP")
    axes[1, 2].set_xlabel('Problem Size - Vector Size (MB)')
    axes[1, 2].set_ylabel('Rate(GB/s)')
    axes[0, 2].xaxis.set_ticks(msg_size) #set the ticks to be a
    axes[0, 2].xaxis.set_ticklabels(msg_size_labels)
    axes[1, 2].xaxis.set_ticks(msg_size) #set the ticks to be a
    axes[1, 2].xaxis.set_ticklabels(msg_size_labels)
    plt.tight_layout()
    plt.legend()
    plt.savefig("plots/reduction_"+str(th)+"thr.png");
    plt.show()
    
    #clean list
    del native_simple[:];
    del native_simple_dp[:];
    del native_pcie[:];
    del native_pcie_dp[:];
    del native_parity[:];
    del native_parity_dp[:];
    
    del localhost_simple[:];
    del localhost_simple_dp[:];
    del localhost_pcie[:];
    del localhost_pcie_dp[:];
    del localhost_parity[:];
    del localhost_parity_dp[:];
    
    del racex_simple[:];
    del racex_simple_dp[:];
    del racex_pcie[:];
    del racex_pcie_dp[:];
    del racex_parity[:];
    del racex_parity_dp[:];

    