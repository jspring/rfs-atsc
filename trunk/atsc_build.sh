#!/bin/sh
# simple script that builds ix code in the correct order
cd sniffer
echo sniffer
make
cd ../ab3418
echo ab3418
make
cd ../ntcip
echo ntipc
make
cd ../ix
echo ix
make
cd ../phases
echo phases
make
cd ../sendmsg2tlab
echo sendmsg2tlab
make
