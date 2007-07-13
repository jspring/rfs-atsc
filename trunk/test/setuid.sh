#!/bin/sh
# Run as root to setuid on programs in this directory that must
# run with root permission
chown root rdsniff
chmod 4775 rdsniff
