/*
* Protocoale de comunicatii
* Laborator 7 - TCP
* Echo Server
* server.c
*/

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

#define MAX_CONNECTIONS 32

TClient clients[MAX_CONNECTIONS];
struct sockaddr_in serv_addr_tcp, serv_addr_udp;
socklen_t socket_len = sizeof(struct sockaddr_in);
char *buf_udp;

void run_chat_multi_server(int listenfd_tcp, int listenfd_udp) {

	struct pollfd poll_fds[MAX_CONNECTIONS];
	struct chat_packet received_packet;

	// Setam socket-ul listenfd pentru ascultare
	int rc = listen(listenfd_tcp, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
	// multimea read_fds
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = listenfd_tcp;
	poll_fds[1].events = POLLIN;
	poll_fds[2].fd = listenfd_udp;
	poll_fds[2].events = POLLIN;
	int num_clients_online = 3;
	int num_clients_tcp = 0;

	while (1) {

		rc = poll(poll_fds, num_clients_online, -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < num_clients_online; i++) {
			// printf("%d\n", i);
			if (poll_fds[i].revents & POLLIN) {

				if (poll_fds[i].fd == STDIN_FILENO) {
					char buf[20];
					fgets(buf, sizeof(buf), stdin);

					// printf("Socket-ul client %d a inchis conexiunea\n", i);
					if(strncmp(buf, "exit", 4) == 0) {
						for (int j = 1; j < num_clients_online; j++) {
							close(poll_fds[j].fd);
						}
						return;
					}
				} else if (poll_fds[i].fd == listenfd_tcp) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int newsockfd = accept(listenfd_tcp, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					recv_all(newsockfd, &received_packet, sizeof(received_packet));

					int exist = 0;
					for (int j = 0; j < num_clients_tcp; j++) {
						if (strcmp(clients[j].id, received_packet.message) == 0) {
							for (int k = 3; k < num_clients_online; k++) {
								if (poll_fds[k].fd == clients[j].sockfd) {
									close(newsockfd);
									exist = 1;
									break;
									break;
								}
							}
						}
					}

					if (exist) {
						printf("Client %s already connected.\n", received_packet.message);
						continue;
					}

					printf("New client %s connected from %s:%d.\n", received_packet.message, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					// se adauga noul socket intors de accept() la multimea descriptorilor
					// de citire
					poll_fds[num_clients_online].fd = newsockfd;
					poll_fds[num_clients_online].events = POLLIN;
					num_clients_online++;


					int find = 0;
					for (int j = 0; j < num_clients_tcp; j++) {
						if (strcmp(clients[j].id, received_packet.message) == 0) {
							clients[j].sockfd = newsockfd;
							find = 1;
							// printf("Clientul %s a mai fost conectat\n", received_packet.message);
							break;
						}
					}

					if (!find) {
						strcpy(clients[num_clients_tcp].id, received_packet.message);
						clients[num_clients_tcp].sockfd = newsockfd;
						num_clients_tcp++;
					}

					// primesc un pachet cu id ul clientului

				} else if (poll_fds[i].fd == listenfd_udp) {
					// TODO
					int rc = recvfrom(listenfd_udp, buf_udp, 1551, 0, (struct sockaddr *)&serv_addr_udp, &socket_len);
					DIE(rc < 0, "recv from udp");

				} else {
				// s-au primit date pe unul din socketii de client,
				// asa ca serverul trebuie sa le receptioneze
					int rc = recv_all(poll_fds[i].fd, &received_packet, sizeof(received_packet));
					DIE(rc < 0, "recv");

					// printf("%d\n", rc);

					if (rc == 0) {
						// conexiunea s-a inchis
						for (int j = 0; j < num_clients_tcp; j++) {
							if (clients[j].sockfd == poll_fds[i].fd) {
								printf("Client %s disconnected.\n", clients[j].id);
								close(poll_fds[i].fd);
								break;
							}
						}

						// se scoate din multimea de citire socketul inchis
						for (int j = i; j < num_clients_online - 1; j++) {
							clients[j] = clients[j + 1];
							poll_fds[j] = poll_fds[j + 1];
						}

						num_clients_online--;

					} else {
					// printf("S-a primit de la clientul de pe socketul %d mesajul: %s",
						// poll_fds[i].fd, received_packet.message);

					/* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
						for (int j = 3; j < num_clients_online; j++) {
							if (j != i) {
								send_all(poll_fds[j].fd, &received_packet, sizeof(received_packet));
							}
						}
					}
				}
			}
		}
	}
}

	int main(int argc, char *argv[]) {
	if (argc != 2) {
		// printf("\n Usage: %s <port>\n", argv[1]);
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	// Parsam port-ul ca un numar
	uint16_t port;
	buf_udp = (char*) malloc(1551 * sizeof(char));

	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// Obtinem un socket TCP pentru receptionarea conexiunilor
	int listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd_tcp < 0, "socket");

	int listenfd_udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(listenfd_udp < 0, "socket");

	// Completăm in serv_addr adresa serverului, familia de adrese si portul
	// pentru conectare

	// Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
	// rulam de 2 ori rapid
	int enable = 1;
	if (setsockopt(listenfd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	memset(&serv_addr_tcp, 0, socket_len);
	serv_addr_tcp.sin_family = AF_INET;
	serv_addr_tcp.sin_port = htons(port);
	serv_addr_tcp.sin_addr.s_addr = INADDR_ANY;
	// DIE(rc <= 0, "inet_pton");

	memset(&serv_addr_udp, 0, socket_len);
	serv_addr_udp.sin_family = AF_INET;
	serv_addr_udp.sin_port = htons(port);
	serv_addr_udp.sin_addr.s_addr = INADDR_ANY;

	// Asociem adresa serverului cu socketul creat folosind bind
	rc = bind(listenfd_tcp, (const struct sockaddr *)&serv_addr_tcp, sizeof(serv_addr_tcp));
	DIE(rc < 0, "bind");

	rc = bind(listenfd_udp, (const struct sockaddr *)&serv_addr_udp, sizeof(serv_addr_udp));
	DIE(rc < 0, "bind");

	// run_chat_server(listenfd);
	run_chat_multi_server(listenfd_tcp, listenfd_udp);

	// Inchidem listenfd
	// close(listenfd);
	free(buf_udp);

	return 0;
}
