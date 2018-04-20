#!/bin/bash

i="1"

while [ $i -lt 4096 ]
do
	echo ./scif_write -r 1456 -n $i 
	val="1"
	while [ $val -lt 11 ]
	do
		echo iteration $val
		./scif_write -r 1456 -n $i | tee results/run_racex_${i}_$val.out
		val=$[${val} + 1]
		sleep 4
	done
	i=$[$i*2]
	sleep 4
done
