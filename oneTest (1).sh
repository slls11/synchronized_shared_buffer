#!/bin/bash
clear
echo "--------------------------------------------------"
make clean
make
./multi-lookup 1 1 serviced.txt results.txt inputTest/names1.txt
