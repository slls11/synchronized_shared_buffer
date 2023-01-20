#!/bin/bash
clear
echo "--------------------------------------------------"
make clean
make
./multi-lookup 1 5 serviced.txt results.txt input/names*.txt
