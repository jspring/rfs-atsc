#!/bin/sh

# other programs from /home/tsp must be running

sleep 1
# digio is not needed, because 170 provides timing as well ast tsp 
/home/atsc/test/phase170e -v > phase170.out &

sleep 1

# start program to send intersection message to MCNU every 500 ms

# in intersection, send to address shown; port is 6053 by default,
# but flag is shown in case it is desirable to change 
# -n flag means no broadcast, broadcast is default
# turn on -v flag if debugging is needed, info goes to stdout

#/home/atsc/test/snd_ix_msg -n -o 10.0.0.115 -i 500 -p 6053 >snd_ix_msg.out &  

# in lab send over router
/home/atsc/test/snd_ix_msg -n -o 10.0.1.114 -i 500 -p 6053 >snd_ix_msg.out &  
