#include "common.h"
#include "helpers.h"

int is_client_connected(int num_clients_tcp, int num_clients_online, TClient *clients, char *msg, struct pollfd *poll_fds) {
    for (int j = 0; j < num_clients_tcp; j++) {
        if (strcmp(clients[j].id, msg) == 0) {

            for (int k = 3; k < num_clients_online; k++) {
                if (poll_fds[k].fd == clients[j].sockfd) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int not_first_time(int num_clients_tcp, TClient *clients, char *msg, int newsockfd) {
    for (int j = 0; j < num_clients_tcp; j++) {
        if (strcmp(clients[j].id, msg) == 0) {
            clients[j].sockfd = newsockfd;
            // printf("a mai fost =)\n");
            return 1;
        }
    }

    return 0;
}