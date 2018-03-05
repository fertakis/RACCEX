#!/bin/bash

i="1"

while [ $i -lt 64000 ]
do
echo ./scif_send -r 1456 -s $i 
./scif_send -r 1456 -s $i | tee results/run_rphi_$i.out
i=$[$i*2]
sleep 4
done
