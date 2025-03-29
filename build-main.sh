#!/bin/sh
g++ -o main main.cpp -O2 $(pkg-config --cflags opencv4) $(pkg-config --libs opencv4)
