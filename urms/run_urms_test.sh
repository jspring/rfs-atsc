#!/bin/bash

IPADDR=$1
PORT=$2
ADDR_2070=$3

echo "Go to Command Source->Interconnect Display (5-3)"
sleep 5
while [[ 1 ]]
do
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 500 -2 3 -3 1 -4 600 -5 3 -6 1 -E 700 -F 3 -G 1 -H 800 -I 3 -J 1
	sleep 2
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 550 -2 3 -3 1 -4 650 -5 3 -6 1 -E 750 -F 3 -G 1 -H 850 -I 3 -J 1
	sleep 2
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 600 -2 3 -3 1 -4 700 -5 3 -6 1 -E 800 -F 3 -G 1 -H 900 -I 3 -J 1
	sleep 2
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 650 -2 3 -3 1 -4 750 -5 3 -6 1 -E 850 -F 3 -G 1 -H 950 -I 3 -J 1
	sleep 2
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 700 -2 3 -3 1 -4 800 -5 3 -6 1 -E 900 -F 3 -G 1 -H 1000 -I 3 -J 1
	sleep 2
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 750 -2 3 -3 1 -4 850 -5 3 -6 1 -E 950 -F 3 -G 1 -H 1500 -I 3 -J 1
	sleep 2
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 800 -2 3 -3 1 -4 900 -5 3 -6 1 -E 1000 -F 3 -G 1 -H 1100 -I 3 -J 1
	sleep 2
	/home/atsc/urms/lnx/urms -r $IPADDR -p $PORT -a $ADDR_2070 -s -v -1 850 -2 3 -3 1 -4 950 -5 3 -6 1 -E 1050 -F 3 -G 1 -H 1150 -I 3 -J 1
	sleep 2
done
