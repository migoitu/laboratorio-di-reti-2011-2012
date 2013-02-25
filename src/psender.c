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
#define PORTA_LOCALE_TCP 59000
#define PORTA_LOCALE_UDP 60000
#define PORTA_DEST 61000

int main(int argc, char *argv[]) {

  /* tcp */
  struct sockaddr_in cliente;
  int tcpfd, newtcpfd;
  int  ris, letti;
  socklen_t dim;    /* dimensione socket */

  /* udp */
  struct sockaddr_in dest;
  int udpfd;

  /* select */
  struct timeval attesa;
  int val_select, maxfd;
  fd_set reads, allset;

  /* acknowledge */
  int ricevuti;
  struct sockaddr_in mitt;

  /* buffers */
  char buf[MSGSIZE];
  char ack_buf[9];


  /* ip e porte */
  char ip_ricevente[16];  
  uint16_t porta_mittente;
  uint16_t porta_locale_tcp;
  uint16_t porta_locale_udp;
  uint16_t porta_ricevente;
  uint16_t porta_ricevente_temp;
  
  /* contatore */
  INFO info;

  /* vettore */
  PACCO *pacco, *fine = NULL;
  ICMPACK icmpack;
  time_t timer, timer_temp = 0;
  uint32_t id = 1;
  int inizio = 1; /* scansione vettore per rimandare */
  int size = VECT_SIZE;
  PACCO** vett;
  vett = calloc(size, sizeof(PACCO*));
  
  
  /* controllo input */
  if (argc == 1) {
    memcpy(ip_ricevente, IP_DEST, 15);
    porta_locale_tcp = PORTA_LOCALE_TCP;
    porta_locale_udp = PORTA_LOCALE_UDP;
    porta_ricevente = PORTA_DEST;
    printf("settaggio di default\nip: 127.0.0.1\nporta tcp: 59000\nporta udp: 60000\nporta ricevente: 61000\n");
  }
  else if (argc == 5) {
    strcpy(ip_ricevente, argv[1]);
    porta_locale_tcp = atoi(argv[2]);
    porta_locale_udp = atoi(argv[3]);
    porta_ricevente = atoi(argv[4]);
  }
  else { 
    printf("argomenti passati non completi.\n"); 
    printf("./proxyreceiver ip_destinatario porta_tcp_mittente porta_locale porta_destinatario\n");
    exit (1); 
  }

  init_info(&info);
  porta_ricevente_temp = porta_ricevente;

  
  /* settaggio tempi di attesa select */
  attesa.tv_sec = 0;
  attesa.tv_usec = 500000; /* mezzo secondo */


  /* creazione socket tcp */
  ris = tcudp_setting (&tcpfd, porta_locale_tcp, SOCK_STREAM); 
  if (!ris) { 
    printf("prepara_udp(): andata a puttane\n"); 
    fflush(stdout);
    exit(1);
  }

  /* creazione socket udp */
  ris = tcudp_setting (&udpfd, porta_locale_udp, SOCK_DGRAM);
  if (!ris) { 
    printf("prepara_udp(): andata a puttane\n"); 
    fflush(stdout);
    exit(1);
  }


  /* ascolto */
  printf ("listen()\n");
  ris = listen(tcpfd, 1);
  if (ris == SOCKET_ERROR)  {
    perror ("listen() failed. \n");
    fflush(stdout);
    exit(1);
  }


  /* accettazione */
  memset ( &cliente, 0, sizeof(cliente) );
  dim=sizeof(cliente);

  printf ("accept()\n");
  newtcpfd = accept(tcpfd, (struct sockaddr*) &cliente, &dim);
  if (newtcpfd == SOCKET_ERROR) {
    perror ("accept() failed. \n");
    fflush(stdout);
    exit(1);
  }
  else {
    printf("connessione da %s : %d\n", inet_ntoa(cliente.sin_addr), ntohs(cliente.sin_port) );
    fflush(stdout);
  }


  /* sbloccaggio socket */
  sblocca (&newtcpfd);
  sblocca (&udpfd);

  /* settaggio maxfd */
  maxfd = MAXAB(newtcpfd, udpfd);

  /* preparazione fd per select */  
  FD_ZERO(&allset);  
  FD_SET(newtcpfd, &allset);
  FD_SET(udpfd, &allset);

  info.ini = time(NULL);
  for (;;) {
    
    memcpy(&reads, &allset, sizeof(allset));
    val_select = select(maxfd + 1, &reads, NULL, NULL, &attesa);


    /* qualche socket si e' attivato */
    if(val_select > 0) {

      /* RICEZIONE, IMPACCHETTAMENTO E SPEDIZIONE DEL TCP */
      if(FD_ISSET(newtcpfd, &reads)) {
        
        attesa.tv_sec = 0;
        attesa.tv_usec = 20000; /* decimo di secondo */
        
        if ((letti = recv(newtcpfd, &buf, MSGSIZE, 0 )) >0) {

          info.tot = info.tot + letti;
          printf("%s[TCP]: lett: %d Byte | tot: %d Byte |", BLUC, letti, info.tot); 
          fflush(stdout);


          /* creazione la struct di tipo lista che conterra' il pkt udp */

          pacco = malloc(sizeof(PACCO));
          if(pacco == NULL) {
            perror("malloc(): fallita\n");
            fflush(stdout);
            exit (1);
          }

          /* popolamento struct */
          pkt_udp(&(buf[0]), id, letti, pacco);
          printf(" id: %d |%s\n", id, BIANCO); 
          fflush(stdout);

          /* spedizione del pacchetto udp */
          ris = send_udp(dest, ip_ricevente, porta_ricevente_temp, udpfd, *pacco);
          if (!ris) { 
            printf("send_udp(): andata a puttane\n");
            fflush(stdout);
            exit(1);
          }
          printf("%s[UDP]: %s | %d | %d | %c | %c | %d | %d |%s\n",
          GIALLO, ip_ricevente, porta_ricevente_temp, ntohl(pacco->id), pacco->tipo, pacco->ack, pacco->msg_size, (int)sizeof(*pacco), BIANCO);
          fflush(stdout);

          /* inserimento in vett */
          vett[id] = pacco;
          pacco = NULL;
          
          /* incremento id */
          info.idmax = id;
          id = id + 1;
        }


      /* REPARTO CONTROLLO DELLA RICEZIONE */

        /* Errno 11 = Resource temporarily unavailable */
        if((letti<=0) && (errno!=EINTR && errno!=11)) {

          /* se legge 0 significa che il sender non esiste piu quindi avvisa il proxyreceiver della fine */
          if (letti == 0) {

            /* prepara la struttura, lo popola di info e lo inserisce in lista */
            
            fine = malloc(sizeof(PACCO));
            if(fine == NULL) {
              perror("malloc(): fallita\n");
              fflush(stdout);
              exit (1);
            }

            fine->id = htonl(IDFINE);
            fine->tipo = 'B';
            fine->ack = 'X';
            fine->msg_size = id-1;
            
            vett[0] = fine;
            
            ris = send_udp(dest, ip_ricevente, porta_ricevente_temp, udpfd, *fine);
            if (!ris) { 
              printf("send_udp(magic_pkt): andata a puttane\n"); 
            }

            printf("\n%s[MAGIC]: %s | %d | %d | %c | %c | %d | NULL |%s\n",
            VERDEC , ip_ricevente, porta_ricevente_temp, ntohl(fine->id), fine->tipo, fine->ack, id-1, BIANCO);
            fflush(stdout);
            
            /* chiude il socket con il tcp */
            printf("%s socket TCP chiuso.%s\n",ROSSOC,BIANCO);
            FD_CLR(newtcpfd,&allset);
            close(newtcpfd);
          }

          else {
            perror("recvfrom() fallita: "); 
            return(1);
          }
        }
      }

      /* RICEZIONE DA RITARDATORE E PROXY RECEIVER */
      if(FD_ISSET(udpfd, &reads)) {
        
        /* reset timer */
        timer = 0;
        
        dim = sizeof(struct sockaddr_in);
        if ((ricevuti = recvfrom (udpfd, ack_buf, 9, 0, (struct sockaddr*)&mitt, &dim)) > 0) {
          porta_mittente = ntohs(mitt.sin_port);
          spkt_icmpack (ack_buf, &icmpack);

          /* MESSAGGI DA PROXYRECEIVER */

          if (icmpack.tipo == 'B') {

            /* pacchetto di tipo RIMANDA */
            if (icmpack.id >= RIMANDA) {
              info.rimanda = info.rimanda + 1;
              if (((PACCO*)ack_buf)->ack == 'X') {
                printf("ID %d, Body %c\n", ntohl(((PACCO*)ack_buf)->id), ((PACCO*)ack_buf)->ack);
                printf("\nreceiver ha smesso di esistere, io mi autodistruggo\n");
                close(udpfd); close(newtcpfd);
                libera_null(vett,0,id);
                break;
              }
              
              else {
                printf ("%s[RIMANDA]: %d | %c | %d | %s",
                CIANOC, icmpack.id, icmpack.tipo, icmpack.id_pkt,BIANCO);

                if (icmpack.id_pkt > id) printf("%spkt inesistente %s\n",ROSSOC,BIANCO);
                else {
                  if (vett[icmpack.id_pkt] != NULL) { 
                    ris = send_udp (dest, ip_ricevente, porta_ricevente_temp, udpfd, *(vett[icmpack.id_pkt]));
                    if (ris == 2) {
                      printf("send_udp(): rimanda andata a puttane\n"); 
                    }
                    else { printf("%spkt rispedito!%s\n", VERDEC, BIANCO); }
                  }
                  else {
                    printf ("%spkt gia consegnato!%s\n"
                    ,ROSSOC, BIANCO);
                  }
                }
              }
            }
            
            
            /* REPARTO ACKNOWLEDGEMENT */
            else if((icmpack.id >= 0) && (icmpack.id <= size)) {
              info.ack = info.ack + 1;
              
              /* se l'id del ACK ricevuto e' uguale al id del pkt magico */
              if (icmpack.id_pkt == IDFINE) {
                /* chiude il socket udp ed esce */
                /* libera il vettore */
                printf("\n%s[MAGIC]: %d | %c | %d %s\n", VERDEC, icmpack.id, icmpack.tipo, icmpack.id_pkt, BIANCO);
                FD_CLR(udpfd,&allset);
                FD_CLR(tcpfd,&allset);
                close(udpfd);
                close(tcpfd);
                libera_null(vett,0,id);
                break;
              }
              else {
                PACCO *p = vett[icmpack.id_pkt];
                vett[icmpack.id_pkt] = NULL;
                free(p);

                printf("%s[ACK]: %d | %c | %d |%s\n", VIOLA, icmpack.id, icmpack.tipo, icmpack.id_pkt, BIANCO);

              
              }
            }
          }


          /*__________ REPARTO ICMP __________*/
          if (icmpack.tipo == 'I') {
            info.icmp = info.icmp + 1;
            /* cambia la porta del destinatario perche' sicuramente la sua porta e' in BURST */
            porta_ricevente_temp = scegli_door(porta_ricevente, porta_mittente);
            
            printf("%s[ICMP]: %d | %c | %d | %d => %d |%s", VIOLA, icmpack.id, icmpack.tipo, icmpack.id_pkt, porta_mittente, porta_ricevente_temp, BIANCO);
            if (vett[icmpack.id_pkt] != NULL) {
              ris = send_udp (dest, ip_ricevente, porta_ricevente_temp, udpfd, *(vett[icmpack.id_pkt]));
              if (ris == 2) {
                printf (" send_udp(): icmp fallita!\n");
              }
              printf("%s pkt rispedito!%s\n",VERDEC, BIANCO);
            }
            
            else printf ("%s pkt gia consegnato!%s\n", ROSSOC, BIANCO); 
            
          }
        }
        if (ricevuti == SOCKET_ERROR) {
          if ((errno == EAGAIN) || (errno == EINTR)) {
              printf("recvfrom() temporaneamente non disponibile\n");
            }
          else if(errno == ECONNRESET) {
            perror("recvfrom() failed: ");
            exit(1);
          }
        }
      }
    }

    /* reparto spedizione pacchi */
    if(val_select == 0){

      attesa.tv_sec = 0;
      attesa.tv_usec = 50000;

      if (timer == 0) time(&timer);
      else if ( (timer_temp = time(NULL) - timer) == 1 ) {
        
        inizio = scan_null(vett, inizio, id-1);
        
        ris = multi_sendtcp (vett, inizio, id -1, dest, ip_ricevente, porta_mittente, udpfd, &info);
        if (ris) { exit(1);}
        
        if (fine != NULL) {
          
          ris = send_udp(dest, ip_ricevente, porta_ricevente_temp, udpfd, *fine);
          if (!ris) { 
            printf("send_udp(magic_pkt): andata a puttane\n"); 
          }
        }
      }
      else if ( (timer_temp = time(NULL) - timer) == 10 ) {
      
        printf("[FINE]: Timeout proxy receiver scaduto!\n");
        break;
      
      }
    }
    
  }
  
  info.fin = time(NULL);
  printf ("\n[INFO]:\n Byte_rx: %d \n pkt_tr: %d \n icmp_rx: %d \n ack_rx: %d \n rimanda_rx: %d \n durata: %ld sec \n multisend: %d \n", 
          info.tot ,info.idmax,info.icmp, info.ack, info.rimanda, info.fin-info.ini, info.pkt_counter);
  close(tcpfd);
  printf ("close() grazie e arriverci . . . \n");
  return(0);
}
