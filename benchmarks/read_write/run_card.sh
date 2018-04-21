#!/bin/bash

i="1"

while [ $i -lt 4096 ]
do
	echo ./scif_register_write_mic -l 1456 -n $i
	val="1"
	while [ $val -lt 11 ]
	do
		./scif_register_write_mic -l 1456 -n $i
		val=$[${val} + 1]
	done
	i=$[$i*2]
done
