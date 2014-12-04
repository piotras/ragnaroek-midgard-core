#!/bin/sh

autoreconf -i 
./configure
make
./run_all_tests.sh
make clean
