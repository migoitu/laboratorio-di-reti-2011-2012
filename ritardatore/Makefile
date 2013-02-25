GCCFLAGS= -ansi -Wall -Wunused -pedantic -ggdb
LINKERFLAGS=-lpthread -lm

all:  Ritardatore

Util.o: Util.c 
	gcc -c ${GCCFLAGS}  Util.c

Ritardatore.o:	Ritardatore.c Util.h
	gcc -c ${GCCFLAGS} Ritardatore.c

Ritardatore:	Ritardatore.o Util.o
	gcc -o Ritardatore ${GCCFLAGS} Util.o Ritardatore.o  ${LINKERFLAGS}

clean:	
	rm -f core* *.stackdump
	rm -f Ritardatore.o Ritardatore
	rm -f Util.o




