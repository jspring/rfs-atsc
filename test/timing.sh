#!/bin/sh

#start the database
/home/path/db/db_slv &

sleep 1
# start sniffer digio before compare, to create DB_ATSC_VAR (interval is 200 msec)
/home/atsc/test/rdsniff -s rfs -i 200 > sniffer.out &

# start NTCIP digio before compare, to create DB_ATSC2_VAR (interval is 200 msec)
/home/atsc/test/rdntcip2 -t 200 > ntcip.out &

sleep 1
# start compare program, using binary from this directory
/home/atsc/test/compare -s rfs -V > timing.out &
