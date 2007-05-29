#!/bin/sh
# Script is set up to test on a single system through the loopback interface
# commented out line is for broadcast on a system connected to WRM,
# in the latter case, test_rcv_ix_msg can be run either on the same
# system or on another system connected to a WRM.

sleep 1
# Start program to send intersection message every second
# sending on local interface for testing, no broadcast, verbose
./test_snd_ix_msg -i 1000 -n -o 127.0.0.1 -v >test_snd_ix_msg.out & 
#./test_snd_ix_msg -i 1000 -o 192.168.255.255 -v >wrm_snd_ix_msg.out &

sleep 1
# Start program to receive intersection set message 
# with the flags below, a debugging output will be printed to standard
# out, and a trace file is created that can be renamed to snd_test_phases.in
# and used as an input for test_snd_ix_msg 
./test_rcv_ix_msg -c -f test_rcv_trace.out & 

# Note that a file named rcv_test_phases.out is also created as a side effect 
# of running test_rcv_ix_msg. This file shows the received data, but is not
# in the format needed by test_snd_ix_msg. 
