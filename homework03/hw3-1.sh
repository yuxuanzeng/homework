#!/bin/sh
# From http://wh1t3s.com/2009/05/14/reading-beagleboard-gpio/
#
# Read temperature of T101 in Fahrenheit
# Yuxuan Zeng Sep. 22 2013

if [ $# -lt 2 ]; then
    echo "Usage: $0 i2cBus ic2Addr1"
    exit 0
fi

i2cBus=$1
shift

cleanup() { # echo a newline
  echo ""
  echo "Done"
  exit
}

trap cleanup SIGINT # call cleanup on Ctrl-C

# Read forever

while [ "1" = "1" ]; do
    for i2cAddr in $@
    do
        THIS_VALUE=`i2cget -y ${i2cBus} ${i2cAddr}`
	#let "THIS_VALUE=32+$THIS_VALUE*18/10"
	echo -ne "${THIS_VALUE}\\t"
        sleep 0.2
    done
    # Return to the start of the line, but not the next line
    echo -ne "\\r"
done

cleanup # call the cleanup routine

