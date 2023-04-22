#include "common.h"

#include <sys/socket.h>
#include <sys/types.h>

/*
    TODO 1.1: Rescrieți funcția de mai jos astfel încât ea să facă primirea
    a exact len octeți din buffer.
*/
int recv_all(int sockfd, void *buffer, size_t len) {

    size_t bytes_received = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

    while(bytes_remaining) {
        int rec = recv(sockfd, buff + bytes_received, bytes_remaining, 0);
        if (rec < 0) {
            printf("err recv\n");
            printf("%s\n", ((struct chat_packet*)buff)->message);
            return -1;
        }

        bytes_received = bytes_received + rec;
        bytes_remaining = bytes_remaining - rec;
    }

    return bytes_received;
}

/*
    TODO 1.2: Rescrieți funcția de mai jos astfel încât ea să facă trimiterea
    a exact len octeți din buffer.
*/

int send_all(int sockfd, void *buffer, size_t len) {
    size_t bytes_sent = 0;
    size_t bytes_remaining = len;
    char *buff = buffer;

    while(bytes_remaining) {
        int snd = send(sockfd, buff + bytes_sent, bytes_remaining, 0);
        if (snd < 0) {
            printf("err send\n");
            printf("%s\n", ((struct chat_packet*)buff)->message);
            printf("%ld\n", bytes_remaining);
            return -1;
        }

        bytes_sent = bytes_sent + snd;
        bytes_remaining = bytes_remaining - snd;
    }

    return bytes_sent;
}