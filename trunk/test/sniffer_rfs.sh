#!/bin/sh

#start the database
/home/path/db/db_slv &

sleep 1
# start digio before phases, to create DB_ATSC_VAR
/home/atsc/test/rdsniff -v -s rfs -i 200 > sniffer.out &

sleep 1
# start phases program, using binary from this directory
/home/atsc/test/phases -s rfs -i 200 -t -v > phases.out &

sleep 1
# start program to broadcast intersection message every 500 ms
/home/atsc/test/snd_ix_msg -o 10.0.0.255 -i 200 >snd_ix_msg.out &  
