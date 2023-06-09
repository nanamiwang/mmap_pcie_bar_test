CC=g++


# Not super sure about this one, but use of -O0 here should
# prevent that consecutive writes to the same PCI register are optimized out
CFLAGS=-std=c++11 -pedantic -Wall -O0
INC=
LIB=

all: main

main.o: main.cpp
	$(CC) $(CFLAGS) $(INC) -c main.cpp

main: main.o $(LIB)

clean:
	rm *.o main

.PHONY: all clean
