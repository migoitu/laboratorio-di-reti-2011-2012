#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>

#include "utility.c"

#define IP_DEST "127.0.0.1"
#define PORTA_LOCALE 63000
#define PORTA_DEST 64000
#define PORTA_MITT 62000

int main (int argc, char *argv[]) {

  /*TCP*/
  int tcpfd;
  int newtcpfd = SOCKET_ERROR;
  struct sockaddr_in ricevente;


  /* UDP */
  int udpfd;  /* udp socket file descriptor */
  struct sockaddr_in dest, mitt;
  unsigned int size_mitt;
  int ricevuti, ris;
  char buf[BUFSIZE];


  /* select */
  struct timeval attendi;
  int val_select, maxfd;
  fd_set reads, allset;


 /* ip e porte */
  char ip_mittente[16];
  char ip_ricevente[16];  
  uint16_t porta_dest;
  uint16_t porta_mitt;
  uint16_t porta_mittente_temp;
  uint16_t porta_locale_udp;


  /* vettore */
  int primaRicezione, flag_null, fine, idmax = 0;
  int i;
  int last_pkt = 0;
  INFO info;
  ICMPACK icmpack, ack;
  PACCO  *pacco;
  int count = 1;
  int size = VECT_SIZE;
  PACCO** vett;
  vett = calloc(size, sizeof(PACCO*));


  /* cotrollo sul input */
  if (argc == 1) {
    memcpy(ip_ricevente, IP_DEST, 15);
    porta_locale_udp = PORTA_LOCALE;
    porta_mitt = PORTA_MITT;
    porta_dest = PORTA_DEST;
    printf("settaggio di default\nip: 127.0.0.1\nporta locale: 63000\nporta mittente: 62000\nporta destinatario: 64000\n");
  }
  else if (argc == 5) {
    strcpy(ip_ricevente, argv[1]);
    porta_locale_udp = atoi(argv[2]);
    porta_mitt = atoi(argv[3]);
    porta_dest = atoi(argv[4]);
  }
  else {
	printf("argomenti passati incompleti. Sintassi:\n"); 
    printf("./preceiver ip_ricevente porta_locale porta_mittente porta_destinatario\n");
    exit (1);
  }

  
  init_info(&info);

  /* creazione socket udp */
  tcudp_setting(&udpfd, porta_locale_udp ,SOCK_DGRAM);

  /* creazione socket tcp */
  tcudp_setting (&tcpfd, 0, SOCK_STREAM); 


  /* preparazione dati ricevente */
  memset ( &ricevente, 0, sizeof(ricevente) );
  ricevente.sin_family   =  AF_INET;
  ricevente.sin_addr.s_addr  =  inet_addr(ip_ricevente);
  ricevente.sin_port     =  htons(porta_dest);


  printf ("connect() . . . \n");
  newtcpfd = connect(tcpfd, (struct sockaddr*) &ricevente, sizeof(ricevente));
  if (newtcpfd == SOCKET_ERROR) errore ("connect() failed", errno);
  printf ("connesso!\n");
  fflush(stdout);

/* sbloccaggio socket */
  sblocca (&udpfd);
  sblocca (&tcpfd);

  /* settaggio time select 120secondi */
  attendi.tv_sec = 120;
  attendi.tv_usec = 0;

  FD_ZERO(&allset);  
  FD_SET(udpfd, &allset);

  /* settaggio maxfd */
  maxfd = (newtcpfd < udpfd) ? udpfd : newtcpfd;


  for (;;) {

    memcpy(&reads, &allset, sizeof(allset));
    val_select = select(maxfd + 1, &reads, NULL, NULL, &attendi);

    if(val_select < 0)  {

      ack.id = htonl(RIMANDA);
      ack.tipo = 'B';
      ack.id_pkt = htonl(RIMANDA);
      send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);

      errore("problemi con select", errno);
    }

    if(val_select > 0) {  
      attendi.tv_sec = 0;
      attendi.tv_usec = 20000;


      if(FD_ISSET(udpfd, &reads)) {

        /* e' arrivato un datagram udp */
        size_mitt = sizeof(struct sockaddr_in);
        ricevuti = recvfrom (udpfd, buf, BUFSIZE, 0, (struct sockaddr*)&mitt, &size_mitt);

        if (ricevuti > 0) {
          primaRicezione = 1;
          if (info.ini == 0.0) info.ini = time(NULL);
          
          /* salvataggio informazioni mittente */
          sprintf((char*)ip_mittente,"%s",inet_ntoa(mitt.sin_addr));
          porta_mittente_temp = ntohs(mitt.sin_port);

          /* ricezione datagram udp di tipo B */
          if (((PACCO*)buf)->tipo == 'B') {
            info.pkt_counter += 1;
              
            /* spacchetta il datagram */
            if((pacco = malloc(sizeof(PACCO))) == NULL) {
              perror("malloc(): fallita\n");
              fflush(stdout);
              exit (1);
            }
            else spkt_udp(buf, pacco);

            printf("%s[UDP]: %s | %d | %d | %c | %d | %d |%s\n",
            GIALLO, ip_mittente, porta_mittente_temp, pacco->id, pacco->tipo, pacco->msg_size, ricevuti, BIANCO);

            /* se ricevuto pacco FINE */
            if (pacco->id == IDFINE) {

              idmax = last_pkt = pacco->msg_size;
              printf("%s[FINE]: %s | %d | %d | %c | %d | %s",
              VERDEC, ip_mittente, porta_mittente_temp, pacco->id, pacco->tipo, idmax, BIANCO);
              
              vett[0] = pacco;

            }

            /* ricezione pacco ordinario con scartamento di pkt gia spediti al receiver */
            if (pacco->id != 0) {
              /* spedizione ack */
              ack.id = htonl(pacco->id);
              ack.tipo = 'B';
              ack.id_pkt = htonl(pacco->id);

              send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
              printf("%s[ACK]: %d | %c | %d |%s%spkt spedito!%s", VIOLA, ntohl(ack.id), ack.tipo, ntohl(ack.id_pkt), BIANCO, VERDEC, BIANCO);
              info.ack = info.ack + 1;
            }
            
            /* inserimento */
            if (pacco->id >= count){
              printf("%s| pkt inserito!%s\n", VERDEC, BIANCO);
              vett[pacco->id] = pacco;              
              idmax = (idmax > pacco->id) ? idmax : pacco->id; 

            }

            else printf("%s| pkt ignorato!%s\n",ROSSOC, BIANCO);
          
          }

          /* ricezione datagram di tipo ICMP */
          if (((PACCO*)buf)->tipo == 'I') {

            spkt_icmpack (buf, &icmpack);

            if ((icmpack.id_pkt) < RIMANDA) {
              info.icmp += 1;
              /* cambia porta per non riavere un altro icmp */
              porta_mittente_temp = scegli_door(porta_mitt , porta_mittente_temp);
              printf("%s[ICMP]: %d | %c | %d | %s | ", VIOLA, icmpack.id, icmpack.tipo, icmpack.id_pkt,BIANCO);              

              ack.id = htonl(icmpack.id_pkt);
              ack.tipo = 'B';
              ack.id_pkt = htonl(icmpack.id_pkt);

              send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
              printf("%spkt rimandato!%s\n", ROSSOC, BIANCO); 
            }

          }
        }


        if (ricevuti<0) 

           errore("recvfrom() failed:", errno);

        

      }

    }


    if(val_select == 0) {

      if (fine == 1) break;

      if (primaRicezione == 1) {
        attendi.tv_sec = 0;
        attendi.tv_usec = 400000;
        
        info.idmax = idmax;
        
        flag_null = 0;
        for (i = count; i <= idmax; i++) {
          
          /* se pkt non presente manda richiesta */
          if (vett[i] == NULL) {
            info.rimanda = info.rimanda + 1;
            
            flag_null = 1; /* indica superamento casella NULL */
            porta_mittente_temp = scegli_door(porta_mitt , porta_mittente_temp);

            ack.id = htonl(RIMANDA + i); /* id per pacchi di tipo richiesta */
            ack.tipo = 'B';
            ack.id_pkt = htonl(i);

            send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
            
            printf("%s[RIMANDA]: %d | %d | %c | %d | %s | %d | %s \n",
            CIANOC, ntohl(ack.id), ntohl(ack.id_pkt), ack.tipo, ntohl(ack.id_pkt), ip_mittente, porta_mittente_temp, BIANCO);
            break;
          }

          if (flag_null == 0) {
            /* momento di spezione al destinatario tcp */
            pacco = vett[i];
            
            ris = send_tcp (tcpfd, pacco->msg, pacco->msg_size);
            
            printf("%s[TCP]: %d | %d | %s pkt spedito!%s\n", BLUC, pacco->id, ris, VERDEC, BIANCO);
            free(pacco);
            vett[i] = NULL;
            count++;
            info.tot += ris;

          }

        }
        /* se pacco spedito = ultimo pacchetto */
        if ( ((last_pkt != 0) && (count-1 == last_pkt)) ) {
          ack.id = htonl(IDFINE);
          ack.tipo = 'B';
          ack.id_pkt = htonl(IDFINE);

          send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
          printf("[FINE] pkt %d spedito!\n", ntohl(ack.id_pkt));

          FD_CLR(udpfd,&allset);
          FD_CLR(tcpfd,&allset);
          close(tcpfd);
          close(udpfd);
          fine = 1;
          libera_null(vett,0,last_pkt);
        }
      }

    }
  }


    /* chiusura */
  info.fin = time(NULL);
  printf ("\n[INFO]:\n Byte Tr: %d \n Pkt Tr: %d \n Pkt Rx: %d \n Icmp Rx: %d \n Ack Tr: %d \n Rimanda Tr: %d \n Durata: %ld sec \n", 
          info.tot ,info.idmax, info.pkt_counter, info.icmp, info.ack, info.rimanda, info.fin-info.ini);
  printf ("grazie e arrivederci . . .\n");

  return 0;
}


