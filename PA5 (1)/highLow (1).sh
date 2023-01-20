#!/bin/bash
echo "--------------------------------------------------"
make clean
make
./multi-lookup 5 1 serviced.txt results.txt input/names*.txt

