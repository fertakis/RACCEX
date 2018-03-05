#!/bin/bash

i="1"

while [ $i -lt 64000 ]
do
echo ./scif_recv -l 1456 -s $i
./scif_recv_mic -l 1456 -s $i
i=$[$i*2]
done
