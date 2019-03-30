# Makefile for sw

LDFLAGS = 
LOADLIBS =
LDLIBS = 
APS = 

libs: swsoc_lib.o

swsoc_lib.o:  swsoc_lib.c ../include/swsoc_lib.h
	$(CC) -Wall -I../include/ -c swsoc_lib.c

clean:
	rm -f *.o *~

