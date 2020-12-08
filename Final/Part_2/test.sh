#!/bin/bash

./sortArrays 4 3 5 NRU local 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 NRU global 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 FIFO local 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 FIFO global 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 SC local 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 SC global 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 LRU local 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 LRU global 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 WSClock local 5000 diskFile.dat | tail -n 10
./sortArrays 4 3 5 WSClock global 5000 diskFile.dat | tail -n 10