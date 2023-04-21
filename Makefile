# -*- coding: utf-8 -*-
#
#
# (c) 2016 by the author(s)
#
# Author(s):
#    Andre Richter, andre.richter@tum.de
#

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
	rm *.o example

.PHONY: all clean
