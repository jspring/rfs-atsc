#!/bin/sh
for i in ab3418 sniffer ntcip ix phases sendmsg2tlab test
do
	cd $i
	make clean
	cd ..
done
