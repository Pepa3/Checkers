g++ main.cpp -o main $(pkg-config --libs opencv4) $(pkg-config --cflags opencv4) -Wall -Wextra -O2
