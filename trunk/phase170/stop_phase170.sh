#!/bin/sh
# Stop the database queue processes and stop all priority programs
killall -TERM wrfiles
killall -TERM phase170e
killall -TERM read232
killall -TERM veh_sig
sleep 3
killall -TERM db_slv
sleep 1
/home/tsp/test/db_clean.sh
