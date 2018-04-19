#!/bin/bash

i="1"

while [ $i -lt 64000 ]
do
	echo ./scif_send -r 1456 -s $i
	val="1"
	while [ $val -lt 11 ]
	do
		echo iteration $val
		cat /proc/stat | tee results_utilizaton/run_util__native_${i}_$val.out
		./scif_send -r 1456 -s $i
		cat /proc/stat | tee -a results_utilizaton/run_util_native_${i}_$val.out
		val=$[${val} + 1]
		sleep 4
	done
	i=$[$i*2]
	sleep 4
done
