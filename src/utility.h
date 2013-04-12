#ifndef __UTILITY_H__
#define __UTILITY_H__


struct PACCO;
struct ICMPACK;
struct INFO;

void init_info (INFO *info);
uint16_t scegli_door (uint16_t origine, uint16_t old);

/* Funzioni per la preparazione dei socket */
int tcudp_setting (int *sockfd, uint16_t porta, int tipo_sock);
void sblocca (int *sockfd);

/* Funzioni per la spedizione */
int send_udp (struct sockaddr_in dest, char *ip_ricevente, unsigned short porta_ricevente, int sockfd, PACCO msg);
int send_tcp (int sockfd, char *msg, int len);
int send_ack (struct sockaddr_in dest, char *ip_ricevente, unsigned short porta_ricevente, int sockfd, ICMPACK msg);

/* scansione vettore */
int multi_sendtcp (PACCO* vett[], int inizio, int nfine, struct sockaddr_in dest, char *ip_dest, unsigned short porta_dest, int sockfd, INFO *info);
int scan_null(PACCO *vett[], int inizio, int fine);
void libera_null(PACCO *vett[],int inizio, int fine)

/* Funzioni per impacchettare e spacchettare */
void pkt_udp(char* buf, int i, int dim_buf, PACCO *pacco);
void spkt_udp(char *buf, PACCO *pacco);
void spkt_icmpack (char *buf, ICMPACK *pkt);


#endif  /*__UTILITY_H_*/
