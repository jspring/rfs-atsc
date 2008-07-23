#!/bin/sh
# This scrpt runs the phase170 standard alone
# For tsteing, phase170 does not updates the CICAS Traffic Signal database
# However, the generated msg will be print on the screen
/home/path/db/db_slv &
sleep 1
/home/tsp/test/veh_sig &
sleep 1
/home/tsp/test/read232 /dev/ttyS0 &
sleep 1
/home/tsp/test/wrfiles &
sleep 1
/home/atsc/test/phase170e -i 500 -v &

