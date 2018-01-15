#!/bin/bash

echo "Launching card side program from remote process!"

if [ $# -lt 8 ] ; then
        echo "required arg pattern -r <remote_port> -s <msg_size> -b <block/non-block>" \
								" -t <terminate_check>"
        exit 0
fi

echo "Args received are : $2 $4 $6 $8"
/tmp/scif_connect_launch_mic $1 $2 $3 $4 $5 $6 $7 $8

exit 0
