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
            clients[j].online = 1;
            return 1;
        }
    }

    return 0;
}


void send_old_msg(TClient client, int topics_size, TTopic *topics, struct chat_packet *send_packet) {
    for (int i = 0; i < client.topics_len; i++) {
        for (int j = 0; j < topics_size; j++) {
            // am gasit un topic la care clientul a dat subscribe
            if (strcmp(client.topics_name[i], topics[j].name) == 0) {
                for (int k = client.last_msg_idx[i]; k < topics[j].messages_len; k++) {
                    // daca clientul a dat subscribe cu flagul 1
                    if (client.last_msg_idx[i] != -1) {
                        // trimit toate mesajele pe care le a ratat
                        strcpy(send_packet->message, topics[j].messages[k].msg);
                        send_packet->len = strlen(topics[j].messages[k].msg) + 1;

                        send_all(client.sockfd, send_packet, sizeof(struct chat_packet));
                    }
                }
            }
        }
    }
}

void update_idx_list(TClient client, int topics_size, TTopic *topics) {
    for (int i = 0; i < topics_size; i++) {
        for (int j = 0; j < client.topics_len; j++) {
            if (strcmp(topics[i].name, client.topics_name[j]) == 0) {
                client.last_msg_idx[j] = topics[i].messages_len - 1;
            }
        }
    }
}
