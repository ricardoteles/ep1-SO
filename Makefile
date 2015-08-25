cc = gcc
cflags = -Wall
debugflags = -g

execs = ep1 ep1sh
obj = ep1.o ep1sh.o

all: ep1 ep1sh

ep1: ep1.o
	$(cc) -o $@ $^ 

ep1sh: ep1sh.o -lreadline
	$(cc) -o $@ $^	

%.o: %.c
	$(cc) -c $(debugflags) $<

clean: 
	-rm -f $(execs) $(obj) *~ core*