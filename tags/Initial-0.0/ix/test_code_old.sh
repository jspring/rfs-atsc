#!/bin/sh
# This script creates a distribution of the Intersection Message send code
# designed to be used standalone (without the signal phase input drivers and
# phase tracking program) for the purpose of testing code to receive the
# Intersection Message. 
rm -rf ix_dist
mkdir ix_dist
mkdir ix_dist/ix
mkdir ix_dist/test
cp ./ix_byte_count.c ix_dist/ix
cp ./ix_db_utils.c ix_dist/ix
cp ./ix_db_utils.h ix_dist/ix
cp ./ix_file_utils.c ix_dist/ix
cp ./ix_msg.h ix_dist/ix
cp ./ix_test_utils.h ix_dist/ix
cp ./ix_utils.c ix_dist/ix
cp ./ix_utils.h ix_dist/ix
cp ./Makefile.test_only ix_dist/ix/Makefile
cp ./rcv_ix_msg.c ix_dist/ix
cp ./snd_ix_msg.c ix_dist/ix
cp ./udp_utils.c ix_dist/ix
cp ./udp_utils.h ix_dist/ix
cp ./ix_dist.sh ix_dist/ix
cp ../test/test_only.sh ix_dist/test
cp ../test/snd_test_phases.in.page_mill ix_dist/test
cp ../test/README.ix_dist ix_dist/README
tar cvf - ix_dist | gzip >ix_dist.tgz
