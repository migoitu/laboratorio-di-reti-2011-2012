
#define MSGSIZE 64497 /* lettura del proxysender */
#define BUFSIZE 65507 /* lettura del proxyreceiver */
#define RIMANDA  (unsigned int)2000000000 /* piu' 11 tera*/
#define SOCKET_ERROR   ((int)-1)
#define IDFINE 0 /* id pacchetto finale */
#define VECT_SIZE 10000 /* dimensione iniziale vettore */

#define BIANCO "\033[0m" /* colore di default */
#define GIALLO "\033[22;33m" /* pacchetti da UDP */
#define BLUC "\033[22;34m" /* pacchetti di TCP */
#define CIANOC "\033[22;36m" /* pacchetti di RIMANDA */
#define VIOLA "\033[22;35m" /* pacchetti per ACK, ICMP */
#define ROSSOC "\033[22;31m" /* NON VA */
#define VERDEC "\033[22;32m" /* OK , MAGIC PACK  */

#define MAXAB(A,B) ((A>B) ? (A) : (B))



/* struct utiliziate */
typedef struct {
  uint32_t id;
  char tipo; /* B = body, I = icmp */
  char ack; /* N = normal, X = fine */
  int msg_size;
  char msg[MSGSIZE];
  }  __attribute__((packed)) PACCO;


typedef struct {
  uint32_t id;
  char tipo;
  uint32_t id_pkt;
}  __attribute__((packed)) ICMPACK;

typedef struct {
    int tot; /*totale byte */
    int idmax;
    int pkt_counter;
    int icmp; 
    int ack; /* ack spediti(pr)/ricevuti(ps) */
    int rimanda; /* richieste spedite(pr)/ricevute(ps) */
    time_t ini;
    time_t fin;
} INFO;

void init_info (INFO *info) {
  info->tot = 0;
  info->idmax = 0;
  info->pkt_counter = 0;
  info->icmp = 0;
  info->ack = 0;
  info->rimanda = 0;
  info->ini = 0.0;
  info->fin = 0.0;
  
}
/* sceglie una porta tra XXXX0-1-2 */
uint16_t scegli_door (uint16_t origine, uint16_t old) {
  uint16_t p1, p2, p3, p4;
  p1 = origine;
  p2 = origine + 1;
  p3 = origine + 2;
  
  if (old == p1) { p4 = p2; }
  else if (old == p2) { p4 = p3; }
  else if (old == p3) { p4 = p1; }
  return p4;
}

/* INIT SOCKET TCP E UDP*/

/* prepara i socket tcp/udp */
int tcudp_setting (int *sockfd, uint16_t porta, int tipo_sock) {
  struct sockaddr_in locale;
  int ris, optval;
  
  /*creazione del socket */
  printf("socket() | ");
  if ((*sockfd = socket(AF_INET, tipo_sock, 0)) < 0) {
    perror("socket():errore socket. \n");
    fflush(stdout);
    return 0;
  }
  
  optval = 1;

  printf("setsockopt() | ");
  ris = setsockopt(*sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));
  if ( ris == ((int)-1)) {
    perror ("setsockopt(): errore di SO_REUSEADDR. \n");
    fflush(stdout);
    return 0;
  }
  
  /* preparazione per bind */
  memset(&locale, 0, sizeof(locale));
  locale.sin_family = AF_INET; 
  locale.sin_addr.s_addr  =  htonl(INADDR_ANY); 
  locale.sin_port = htons(porta);  
  
  printf("bind()\n");
  if ((ris = bind(*sockfd, (struct sockaddr*) &locale, sizeof(locale))) < 0) {
    perror("bind(): errore bind. \n");
    fflush(stdout);
    return 0;
  }
  return 1;
  
}

/*bloccante*/
void sblocca (int *sockfd) {
  int flags;
  
  if ((flags= fcntl(*sockfd, F_GETFL, 0)) <0) {
    perror("fcntl(F_GETFL): errore fcntl(). \n");
    fflush(stdout);
    exit(1);
  }

  flags |= O_NONBLOCK;
  if ( fcntl(*sockfd, F_SETFL, flags) < 0) {
    perror("fcntl(F_SETFL) errore fcntl(). \n");
    fflush(stdout);
    exit(1);
  }
}



/* SEZIONE SPEDIZIONE */

/* spedisce un pacco tcp */
int send_tcp (int sockfd, char *msg, int len) {
  int ris;
  int conta = 0;
  int n = len;
  int i;
  
  do {
    ris = write(sockfd, &msg[conta], n);
    
    /* se la write non spedisce len byte */
    if ((ris != n) && (ris != SOCKET_ERROR)) { 
      conta = conta + ris; /* conta si posiziona a ris byte */
      n = n - ris; /* i byte da spedire diminuiscono di ris */
      
    }
  
    if (ris == n) {ris = 0; break;} /* se i byte che deve spedire sono uguali a quelli spediti esce */
    
    /* se e' successo un errore grave esce */
    if (ris < 0 && errno != 11 && errno != EINTR && errno != EAGAIN) break;
  
  }while(1);
  
  if(ris<0) {

    if(errno == ECONNRESET || errno == EPIPE) {
      printf("\nErrore write [morte receiver]: CONNESSIONE INTERROTTA\n");
      return 1;
    }
    else{
      perror("write() failed:");
      return 2;
    }
  }

  printf (" %d ",(i = (n == len) ? n : (conta + n) ));
  fflush(stdout);
  return ((i = (n == len) ? n : (conta + n) ));
}

/* spedisce un pacco udp */
int send_udp (struct sockaddr_in dest, char *ip_ricevente, unsigned short porta_ricevente, int sockfd, PACCO msg) {
  int ris, addr_size;

  /* prepara la sockaddr_in di destinazione */
  memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = inet_addr(ip_ricevente);
  dest.sin_port = htons(porta_ricevente);
  
  /* spedisce il messaggio mediante la sendto */
  addr_size = sizeof(struct sockaddr_in);

  B:
  do{
    ris = sendto (sockfd, &msg , sizeof(msg), 0, (struct sockaddr*)&dest, addr_size);
    if(ris==0) printf("spedizione nulla\n");break;
    
    if(ris < 0 && errno != 11 && errno != EINTR && errno != EAGAIN) break;
    printf("ris sendto() = %d\n",ris);
  } while(ris>0);
  
  if(ris < 0) {
          if (errno == 11) goto B;
        printf("sendto() error: errno %d\n", errno);
    perror(" \n");
    fflush(stdout);
    return 0;
  }

  
  return 1;
}

/* spedisce un pacco ACK */
int send_ack (struct sockaddr_in dest, char *ip_ricevente, unsigned short porta_ricevente, int sockfd, ICMPACK msg) {
  int ris, addr_size;
  
  /* prepara la sockaddr_in di destinazione */
  memset(&dest, 0, sizeof(dest));
  dest.sin_family = AF_INET;
  dest.sin_addr.s_addr = inet_addr(ip_ricevente);
  dest.sin_port = htons(porta_ricevente);
  
  /* spedisce il messaggio mediante la sendto */
  
  addr_size = sizeof(struct sockaddr_in);

  do{
    ris = sendto (sockfd, &msg , sizeof(msg), 0, (struct sockaddr*)&dest, addr_size);
    if(ris==0) printf("spedizione nulla\n");break;
    if((ris<0 && errno!=EINTR)&& (errno!=11)) break;
    printf("ris sendto() = %d\n",ris);
  } while(ris<0);
  
  if(ris < 0) {
    printf("sendto() error\n");
    return (0);
  }
  
  return 1;
}

/* spedisce da 1 a n pacchi dal vettore */
int multi_sendtcp (PACCO* vett[], int inizio, int nfine, struct sockaddr_in dest, char *ip_dest, unsigned short porta_dest, int sockfd, INFO *info) {
  uint16_t porta_temp;
  PACCO *pacco;
  int i, ris;
  porta_temp = porta_dest;

  for (i = inizio; i <= nfine; i++) {
    info->pkt_counter++;      
    pacco = vett[i];
    if (pacco != NULL) {
      ris = send_udp (dest, ip_dest, porta_dest, sockfd, *pacco);
      if (!ris) { 
        printf("send_udp(): andata a puttane\n");
        fflush(stdout);
        return (1);
      }
      porta_temp = scegli_door(porta_dest, porta_temp);
    }
  
  }
  return (0);
        
}

int scan_null(PACCO *vett[], int inizio, int fine) {
  int i;
  int ris, flag = 0;
  PACCO *pacco;
          
  for (i = inizio; i <= fine; i++) {
    pacco = vett[i];

    if (flag == 1) break;    
    if (pacco == NULL) {
      ris = i;
      flag = 1;
    }
    if ((i == fine) && (pacco == NULL)) {
      ris = i;
      flag = 1;
    }
    
  }
  
  return ris;
  }

void libera_null(PACCO *vett[],int inizio, int fine) {
  int i;
  PACCO *pacco;

  for (i = inizio; i <=fine; i++){
    pacco = vett[i];

    if (pacco != NULL) {
      printf("posizione %d  del vett non vuoto!\n", i);
      vett[i] = NULL;
      free(pacco);
    }
  
  }
}

/* SEZIONE IMPACCHETTAMENTO E SPACCHETTAMENTO */

/* genera una struttura popolata di tipo pacco */
void pkt_udp(char* buf, int i, int dim_buf, PACCO *pacco) {
  
  pacco->id = htonl((uint32_t)i);
  pacco->tipo = 'B';
  pacco->ack = 'N';
  pacco->msg_size = dim_buf;
  memcpy(pacco->msg, buf, dim_buf);

}

/* ricrea la struttura pacco ricevuta dal buffer */
void spkt_udp(char *buf, PACCO *pacco) {
  
  pacco->id = ntohl(((PACCO*)buf)->id);
  pacco->tipo = ((PACCO*)buf)->tipo;
  pacco->ack = ((PACCO*)buf)->ack;
  pacco->msg_size = ((PACCO*)buf)->msg_size;
  memcpy(pacco->msg, ((PACCO*)buf)->msg, pacco->msg_size);

}
 
/* spacchetta i pacchetti icmp */
void spkt_icmpack (char *buf, ICMPACK *pkt) {

  pkt->id = ntohl(((ICMPACK*)buf)->id);
  pkt->tipo = ((ICMPACK*)buf)->tipo;
  pkt->id_pkt = ntohl(((ICMPACK*)buf)->id_pkt);
  
}

/* spacchetta i pacchetti magic pkt */
void fine_pkt (PACCO *pack, uint32_t id) {
  
  pack->id = htonl((uint32_t)id);
  pack->tipo = 'B';
  pack->ack = 'X'; 
  
}



