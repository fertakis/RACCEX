#!/bin/bash

i="1"

while [ $i -lt 64000 ]
do
	echo ./scif_send -r 1456 -s $i
	val="1"
	while [ $val -lt 11 ]
	do
		echo iteration $val
		cat /proc/stat > results_send_recv/run_util_racex_${i}_${val}_first.out
		../send_recv/scif_send -r 1456 -s $i
		cat /proc/stat >> results_send_recv/run_util_racex_${i}_${val}_second.out
		val=$[${val} + 1]
		sleep 4
	done
	i=$[$i*2]
	sleep 4
done
