#!/bin/sh

if [[ ! $1 ]] ; then
    echo Usage: $0 '<httpd script default directory> (Try /opt/lampp/htdocs?)'
    exit
fi
ln -s `pwd`/spat.php $1/spat.php
ln -s `pwd`/spat_files $1/spat_files
ln -s `pwd`/p2_color.txt $1/p2_color.txt
ln -s `pwd`/p4_color.txt $1/p4_color.txt
