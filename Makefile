##
##Makefile per il progetto di reti
##

##Directory
SRC = src
RIT = ritardatore

##Opzioni di compilazione
CFLAGS= -ansi -Wall -Wunused -pedantic -ggdb
CC = gcc

all: preceiver psender
		cd $(RIT)/; make 
		@echo ' -|| Compilazione terminata con successo ||- '

##Compilazione
preceiver.o:	$(SRC)/preceiver.c
				$(CC) -c ${CFLAGS} $(SRC)/preceiver.c

psender.o:	$(SRC)/psender.c
				gcc -c ${GCCFLGS} $(SRC)/psender.c


##Linking
preceiver:	preceiver.o 
			$(CC)  $(CFLAGS) -o preceiver preceiver.o 

psender:	psender.o
			$(CC) $(CFLAGS) -o psender psender.o 


## Pulizia di tutti i file generati dalla compilazione e dal linking
clean:
	rm -rf *.o
	rm -rf preceiver psender
	rm -rf docs/html/*
	rm -rf docs/latex/*
	cd $(RIT)/; make clean
	
## Creazione della documentazione tramite Doxygen
doc:
		doxygen doxydoc
