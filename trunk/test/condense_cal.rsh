#!/bin/sh
# Script to take output of rcv_ix_msg -n -c and print lines in which
# one of the approaches is different from the preceding.
# Works only for RFS signal timing, more complicated version could
# be written for other intersections.
TRACE=$1
# remove colons to get single character fields for signal colors
# and to separate time fields for arithmetic
grep -v exited $TRACE |  awk '{if (NF == 1) last_time = $1; if (NF != 1) print last_time,$0}'| sed 's/:/ /g' | awk '{if ($5 != old5 || $8 != old8 || $11 != old11 || $14 != old14 || $17 != old17 || $20 != old20 || $23 != old23 || $26 != old26) print $0;  old5=$5; old8=$8; old11=$11; old14=$14; old17=$17; old20=$20; old23=$23; old26=$26}' | awk '{time=$1*3600 + $2 * 60 + $3; interval=time-oldtime; printf("%4.1f %s\n", interval, $0); oldtime=time}' 
