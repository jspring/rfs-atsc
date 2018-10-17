#!/bin/bash

cd /home/atsc/ab3418
killall ab3418comm
rm ./data/*

for x in `cat ./Arterial_signal_IP_addresses_and_associated_ramp_meters.csv | awk '{print $2}'`
do 
	echo $x; 
	/home/atsc/ab3418/run_ab3418_2_tsc1.sh $x &
	sleep 5 
	killall ab3418comm
done

for x in `cat ./Arterial_signal_IP_addresses_and_associated_ramp_meters.csv | awk '{print $2}'`
do 
	SIZE=`ls -l /home/atsc/ab3418/data/$x.txt | awk '{print $5}'`
	if [[ ($? -eq 0) && (SIZE -gt 0) ]] 
	then
		echo $x size is $SIZE
		scp -p /home/atsc/ab3418/data/$x.txt jspring@pems:/home/atsc/ab3418/data
	else
		echo $x has no data
	fi
done
