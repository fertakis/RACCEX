#!/bin/bash

i="1"

while [ $i -lt 64000 ]
do
	echo ./scif_recv -l 1456 -s $i
	val="1"
	while [ $val -lt 11 ]
	do
		./scif_recv_mic -l 1456 -s $i
		val=$[${val} + 1]
	done
	i=$[$i*2]
done
