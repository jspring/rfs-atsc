#!/bin/sh
# Run as root to setuid on programs in this directory that must
# run with root permission
chown root rdsniff_rfs rdsniff_page rd3418
chmod 4775 rdsniff_rfs rdsniff_page rd3418
