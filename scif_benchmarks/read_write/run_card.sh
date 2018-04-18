#!/bin/bash

i="1"

while [ $i -lt 4096 ]
do
echo ./scif_register_write_mic -l 1456 -n $i
./scif_register_write_mic -l 1456 -n $i
i=$[$i*2]
done
