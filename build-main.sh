#!/bin/sh
g++ -o main main.cpp -O2 -Wall -Wextra $(pkg-config --cflags opencv4) $(pkg-config --libs opencv4)
