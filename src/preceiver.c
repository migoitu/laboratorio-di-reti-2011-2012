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
  int flag, flag_null, fine, idmax = 0;
  int i;
  int last_pkt = 0;
  INFO info;
  ICMPACK icmpack, ack;
  PACCO  *pacco;
  int count = 1;
  PACCO** vett;
  vett = calloc(VECT_SIZE, sizeof(PACCO*));


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
    printf("argomenti passati non completi\n"); 
    printf("./proxyreceiver ip_ricevente porta_locale porta_mittente porta_destinatario\n");
    exit (1);
  }

  
  init_info(&info);
  
  /* creazione socket udp */
  ris = tcudp_setting(&udpfd, porta_locale_udp ,SOCK_DGRAM);
  if (!ris) {
    printf("[UDP] tcudp_setting(): andata a puttane\n");
    fflush(stdout);
    exit(1);
  }


  /* creazione socket tcp */
  ris = tcudp_setting (&tcpfd, 0, SOCK_STREAM); 
  if (!ris) {
    printf("[TCP] tcudp_setting(): andata a puttane\n"); 
    fflush(stdout);
    exit(1);
  }



  /* preparazione dati ricevente */
  memset ( &ricevente, 0, sizeof(ricevente) );
  ricevente.sin_family   =  AF_INET;
  ricevente.sin_addr.s_addr  =  inet_addr(ip_ricevente);
  ricevente.sin_port     =  htons(porta_dest);


  printf ("connect() . . . \n");
  newtcpfd = connect(tcpfd, (struct sockaddr*) &ricevente, sizeof(ricevente));
  if (newtcpfd == SOCKET_ERROR)  {
    perror ("connect() failed");
    fflush(stdout);
    exit(1);
  }
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


    if(val_select > 0) {  
      attendi.tv_sec = 0;
      attendi.tv_usec = 20000;


      if(FD_ISSET(udpfd, &reads)) {

        /* e' arrivato un pacco udp, riceve il datagram */
        size_mitt = sizeof(struct sockaddr_in);
        ricevuti = recvfrom (udpfd, buf, BUFSIZE, 0, (struct sockaddr*)&mitt, &size_mitt);

        if (ricevuti > 0) {
          flag = 1;
          if (info.ini == 0.0) info.ini = time(NULL);
          
          /* salvataggio informazioni mittente */
          sprintf((char*)ip_mittente,"%s",inet_ntoa(mitt.sin_addr));
          porta_mittente_temp = mitt.sin_port;

          /* ricezione datagram udp di tipo B */
          if (((PACCO*)buf)->tipo == 'B') {
            info.pkt_counter = info.pkt_counter + 1;
              
            /* spacchetta il datagram */
            if((pacco = malloc(sizeof(PACCO))) == NULL) {
              perror("malloc(): fallita\n");
              fflush(stdout);
              exit (1);
            }
            else spkt_udp(buf, pacco);

            printf("%s[UDP]: %s | %d | %d | %c | %c | %d | %d |%s\n",
            GIALLO, ip_mittente, porta_mittente_temp, pacco->id, pacco->tipo, pacco->ack, pacco->msg_size, ricevuti, BIANCO);

            /* ricezione pacco finale */
            if (pacco->id == 0) {

              idmax = last_pkt = pacco->msg_size;

              printf("%s[MAGIC]: %s | %d | %d | %c | %c | %d | %s",
              VERDEC, ip_mittente, porta_mittente_temp, pacco->id, pacco->tipo, pacco->ack, idmax, BIANCO);

            }

            /* ricezione pacco ordinario con scartamento di pkt gia spediti al receiver */

            if (pacco->id != 0) {  
              ack.id = htonl(pacco->id);
              ack.tipo = 'B';
              ack.id_pkt = htonl(pacco->id);

              ris = send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
              if (!ris) { printf("\n spedizione ack non riuscita \n"); exit (1);}
              printf("%s[ACK]: %d | %c | %d |%s%spkt spedito!%s", VIOLA, ntohl(ack.id), ack.tipo, ntohl(ack.id_pkt), BIANCO, VERDEC, BIANCO);
              info.ack = info.ack + 1;
            }

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

            if (icmpack.id_pkt == RIMANDA) {
/* #NOTA */
              fine_pkt (pacco, RIMANDA);
              ris = send_udp (dest, ip_mittente, porta_mittente_temp, udpfd, *pacco);
              if (!ris) { 
                printf("[FINE PACK] send_udp(): andata a puttane\n");
                fflush(stdout);
                exit(1);
              }
            }

            if ((icmpack.id_pkt) < RIMANDA) {
              info.icmp = info.icmp + 1;
              /* cambia porta per non riavere un altro icmp */
              porta_mittente_temp = scegli_door(porta_mitt , porta_mittente_temp);
              printf("%s[ICMP]: %d | %c | %d | %s | ", VIOLA, icmpack.id, icmpack.tipo, icmpack.id_pkt,BIANCO);              

              ack.id = htonl(icmpack.id_pkt);
              ack.tipo = 'B';
              ack.id_pkt = htonl(icmpack.id_pkt);

              ris = send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
              if (!ris) { printf("\n spedizione ack non riuscita \n"); exit (1);}
              printf("%spkt rimandato!%s\n", ROSSOC, BIANCO); 
            }

          }
        }


        if (ricevuti<0) {
          if(errno==ECONNRESET) {
            perror("recvfrom() failed (ECONNRESET): ");
            fprintf(stderr,"ma non chiudo il socket\n");
            fflush(stderr);
            exit(0);
          }
          else { 
            perror("recvfrom() failed: ");
            fflush(stdout);
            exit(1);
          }
        }

      }

    }


    if(val_select == 0) {

      if (fine == 1) break;

      if (flag == 1) {
        attendi.tv_sec = 1;
        attendi.tv_usec = 0;
        
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

            ris = send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
            if (!ris) { printf("spedizione ack non riuscita"); }

            printf("%s[RIMANDA]: %d | %d | %c | %d | %d | %s | %s \n",
            CIANOC, ntohl(ack.id), ntohl(ack.id_pkt), ack.tipo, ntohl(ack.id_pkt), porta_mittente_temp, ip_mittente, BIANCO);
            break;
          }

          if (flag_null == 0) {

            pacco = vett[i];
            printf("%s[TCP]: %d | %s", BLUC, pacco->id, BIANCO);
            ris = send_tcp (tcpfd, pacco->msg, pacco->msg_size);
            switch (ris) { 
              case -2:
                /* se la spedizione non va a buon fine il receiver e' morto. */
                printf ("%sClient Ricevente ha smesso di esistere! %s\n", ROSSOC, BIANCO);
              
                fine_pkt (pacco, RIMANDA);
                ris = send_udp (dest, ip_mittente, porta_mittente_temp, udpfd, *pacco);
                if (!ris) { 
                  printf(" send_udp(): andata a puttane, tcp morto\n");
                  fflush(stdout);
                  exit(1);
                }
              break;

              case SOCKET_ERROR: printf(" write(): failed.\n"); exit(1);
              
              default: 
                free(pacco);
                vett[i] = NULL;
                printf("%s| pkt spedito!%s\n", VERDEC, BIANCO);
                count = count + 1;
                info.tot = info.tot + ris;
              break; 
            }

          }

        }
        /* se pacco spedito = ultimo pacchetto */
        if (count-1 == last_pkt) {
          ack.id = htonl(IDFINE);
          ack.tipo = 'B';
          ack.id_pkt = htonl(IDFINE);

          send_ack (dest, ip_mittente, porta_mittente_temp, udpfd, ack);
          printf("[MAGIC] pkt %d spedito!\n", ntohl(ack.id_pkt));

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
  printf ("\n[INFO]:\n Byte_tr: %d \n pkt_tr: %d \n pkt_rx: %d \n icmp_rx: %d \n ack_tr: %d \n rimanda_tr: %d \n durata: %ld sec \n", 
          info.tot ,info.idmax, info.pkt_counter, info.icmp, info.ack, info.rimanda, info.fin-info.ini);
  printf ("grazie e arrivederci . . .\n");

  return 0;
}


