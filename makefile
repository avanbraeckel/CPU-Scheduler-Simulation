# Austin Van Braeckel - 1085829 - avanbrae@uoguelph.ca
# 2021-02-22
# CIS*3110 Assignment 2 Makefile

CC = gcc
CFLAGS = -std=gnu99 -Wpedantic -g

# EXECTUABLE

simcpu: simcpu.o
	$(CC) $(CFLAGS) -o simcpu simcpu.o

# OBJECT CODE

simcpu.o: simcpu.c
	$(CC) $(CFLAGS) -c simcpu.c

# CLEAN / ALL

all: simcpu

clean:
	rm -fv *.o simcpu
