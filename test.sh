#!/bin/bash
file=/dev/cu.usbmodem1411
COUNT=0

while true; do 
    if [ -e "$file" ]; then
	cat $file
        COUNT=0
    else 
    	if (("$COUNT" < "1")); then
    		echo "Device not found"
		COUNT=1
	fi
fi 
done
