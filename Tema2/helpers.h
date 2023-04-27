#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

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

typedef struct client {
  char id[20];
  int sockfd;
} TClient;

typedef struct udp_msg {
  uint8_t type;
  char msg[1500];
} TUdpMsg;

typedef struct topic {
  char name[51];
  TUdpMsg *messages;
  int messages_len;
} TTopic;
