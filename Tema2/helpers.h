#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
// #include <stdint.h>

/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif


typedef struct udp_msg {
  char msg[2000];
} TUdpMsg;

typedef struct topic {
  char name[51];
  TUdpMsg *messages;
  int messages_len;
} TTopic;

typedef struct client {
  char id[20];
  int sockfd;
  char topics_name[100][51];
  int topics_len;
  int online;
  int last_msg_idx[100];
} TClient;
