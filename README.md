Documentazione progetto di reti
*******************************

Compilazione: make 
Rimozione file compilati: make clean
Eseguibili dopo la compilazione: pseder, preceiver, Ritardatore

Disposizione cartelle:
  ./src/psender.c
  ./src/preceiver.c
  ./src/utility.c
  ./src/utility.h

  ./ritardatore/Makefile
  ./ritardatore/Ritardatore.c
  ./ritardatore/util.c
  ./ritardatore/util.h

  ./Makefile
  ./Readme.md


Guida per testare mediante netcat:
0)  lanciare 5 terminali!
1)  lanciare il ritardatore con il comando "./Ritardatore" dalla cartella ritardatore.
2)  lanciare il client detinatario con il comando "nc -l 127.0.0.1 64000 > nomefile.estensione"  (netcat si mettera' in ascolto su ip e porta data).
3)  lanciare il proxy receiver con il comando "./preceiver".
4)  lanciare il proxy sender con il comando "./psender".
5)  lanciare il client mittente con il comando "nc 127.0.0.1 59000 < nomefile.estensione" (nomefile e' un file qualsiasi che si vuole trasmettere).

  in teoria dovrebbe andare. il file che avete spedito arrivera' sano e salvo al client destinatario.


Consiglio:
  se si sta' inviando un file di testo e si vuole vedere l'output anche sul terminale 
  cosiglio di usare tee per direzionare l'output sia su terminale che su file (cercatevi una guida su google).
