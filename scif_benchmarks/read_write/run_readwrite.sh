#!/bin/bash

i="1"

while [ $i -lt 4096 ]
do
echo ./scif_write -r 1456 -n $i 
./scif_write -r 1456 -n $i | tee results/run_localhost_$i.out
i=$[$i*2]
sleep 4
done
