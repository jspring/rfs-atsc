#!/bin/sh
for i in ab3418 sniffer ntcip ix phases sendmsg2tlab 
do
	cd $i
	make clean
	cd ..
done
