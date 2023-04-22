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

void run_chat_multi_server(int listenfd) {

	struct pollfd poll_fds[MAX_CONNECTIONS];
	struct chat_packet received_packet;

	// Setam socket-ul listenfd pentru ascultare
	int rc = listen(listenfd, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");

	// se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
	// multimea read_fds
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = listenfd;
	poll_fds[1].events = POLLIN;
	int num_clients = 2;

	while (1) {

		rc = poll(poll_fds, num_clients, -1);
		DIE(rc < 0, "poll");

		for (int i = 0; i < num_clients; i++) {
			// printf("%d\n", i);
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == listenfd) {
					// a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
					// pe care serverul o accepta
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int newsockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					// se adauga noul socket intors de accept() la multimea descriptorilor
					// de citire
					poll_fds[num_clients].fd = newsockfd;
					poll_fds[num_clients].events = POLLIN;
					num_clients++;

					// primesc un pachet cu id ul clientului
					recv_all(newsockfd, &received_packet, sizeof(received_packet));
					printf("New client %s connected from %s:%d.\n", received_packet.message, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));

				} else if (poll_fds[i].fd == STDIN_FILENO) {
					char buf[20];
					fgets(buf, sizeof(buf), stdin);

					// printf("Socket-ul client %d a inchis conexiunea\n", i);
					if(strncmp(buf, "exit", 4) == 0) {
						for (int i = 1; i < num_clients; i++) {
							close(poll_fds[i].fd);
						}
						return;
					}

				} else {
				// s-au primit date pe unul din socketii de client,
				// asa ca serverul trebuie sa le receptioneze
					int rc = recv_all(poll_fds[i].fd, &received_packet,
										sizeof(received_packet));
					DIE(rc < 0, "recv");

					if (rc == 0) {
						// conexiunea s-a inchis
						// printf("Socket-ul client %d a inchis conexiunea\n", i);
						close(poll_fds[i].fd);

						// se scoate din multimea de citire socketul inchis
						for (int j = i; j < num_clients - 1; j++) {
							poll_fds[j] = poll_fds[j + 1];
						}

						num_clients--;

					} else {
					// printf("S-a primit de la clientul de pe socketul %d mesajul: %s",
						// poll_fds[i].fd, received_packet.message);

					/* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
						for (int j = 2; j < num_clients; j++) {
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
	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// Obtinem un socket TCP pentru receptionarea conexiunilor
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd < 0, "socket");

	// Completăm in serv_addr adresa serverului, familia de adrese si portul
	// pentru conectare
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	// Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
	// rulam de 2 ori rapid
	int enable = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		perror("setsockopt(SO_REUSEADDR) failed");

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	// Asociem adresa serverului cu socketul creat folosind bind
	rc = bind(listenfd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "bind");

	// run_chat_server(listenfd);
	run_chat_multi_server(listenfd);

	// Inchidem listenfd
	// close(listenfd);

	return 0;
}
