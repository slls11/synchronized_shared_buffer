#!/bin/bash
clear
echo "--------------------------------------------------"
make clean
make
./multi-lookup 10 10 serviced.txt results.txt input/names*.txt
