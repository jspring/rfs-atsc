#!/bin/sh
# simple script that builds ix code in the correct order
for i in sniffer ab3418 ntcip ix phases sendmsg2tlab
do
	cd $i
	make
	make install
	cd ..
done
