#include "common.h"
#include "helpers.h"
#include "tcp.h"
#include "udp.h"

#define MAX_CONNECTIONS 32

TClient clients[MAX_CONNECTIONS];
TTopic *topics;
struct sockaddr_in serv_addr_tcp, serv_addr_udp;
socklen_t socket_len = sizeof(struct sockaddr_in);
char *buf_udp;
char *buf_tcp;

void run_chat_multi_server(int listenfd_tcp, int listenfd_udp) {

	FILE *debug = fopen("log.txt", "wt");
	topics = malloc(100 * sizeof(TTopic));
	int topics_size = 0;

	struct pollfd poll_fds[MAX_CONNECTIONS];
	struct chat_packet received_packet;
	struct chat_packet send_packet;

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
	// int num_topics = 0;

	while (1) {
		// printf("futu ti pizda ma tii\n");
		int nr_of_events = poll(poll_fds, num_clients_online, -1);
		DIE(nr_of_events < 0, "poll");

		for (int i = 0; i < num_clients_online && nr_of_events != 0; i++) {
			if (poll_fds[i].revents & POLLIN) {
				if (poll_fds[i].fd == STDIN_FILENO) {
					char buf[20];
					fgets(buf, sizeof(buf), stdin);

					if(strncmp(buf, "exit", 4) == 0) {
						for (int j = 1; j < num_clients_online; j++) {
							close(poll_fds[j].fd);
						}
						return;
					}

				} else if (poll_fds[i].fd == listenfd_tcp) {

					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int newsockfd = accept(listenfd_tcp, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					recv_all(newsockfd, &received_packet, sizeof(received_packet));

					int connected = is_client_connected(num_clients_tcp, num_clients_online, clients, received_packet.message, poll_fds);
					if (connected) {
						close(newsockfd);
						printf("Client %s already connected.\n", received_packet.message);
						nr_of_events--;
						continue;
					}

					printf("New client %s connected from %s:%d.\n", received_packet.message, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					poll_fds[num_clients_online].fd = newsockfd;
					poll_fds[num_clients_online].events = POLLIN;
					num_clients_online++;

					// int find = not_first_time(num_clients_tcp, clients, received_packet.message, newsockfd);
					int ok = 0;
					for (int j = 0; j < num_clients_tcp; j++) {
						if (strcmp(clients[j].id, received_packet.message) == 0) {
							clients[j].sockfd = newsockfd;
							clients[j].online = 1;

							for (int k = 0; k < clients[j].topics_len; k++) {
								for (int l = 0; l < topics_size; l++) {
									if (strcmp(clients[j].topics_name[k], topics[l].name) == 0) {
										fprintf(debug, "am gasit ceva %s\n", topics[l].name);
										fflush(debug);

										for (int m = clients[j].last_msg_idx[k]; m < topics[l].messages_len; m++) {
											if (clients[j].last_msg_idx[k] != -1) {
												strcpy(send_packet.message, topics[l].messages[m].msg);
												send_packet.len = strlen(topics[l].messages[m].msg) + 1;

												send_all(clients[j].sockfd, &send_packet, sizeof(struct chat_packet));
											}
										}
									}
								}
							}
							// for (int k = clients[j].last_msg_idx;)
							ok = 1;
							break;
						}
					}

					if (!ok) {
						strcpy(clients[num_clients_tcp].id, received_packet.message);
						clients[num_clients_tcp].sockfd = newsockfd;
						clients[num_clients_tcp].online = 1;
						num_clients_tcp++;
					}

					nr_of_events--;

				} else if (poll_fds[i].fd == listenfd_udp) {
					// TODO
					int rc = recvfrom(listenfd_udp, buf_udp, 1551, 0, (struct sockaddr *)&serv_addr_udp, &socket_len);
					DIE(rc < 0, "recv from udp");

					char name[51];
					memcpy(name, buf_udp, 50);
					name[50] = '\0';

					uint8_t type = buf_udp[50];

					if (type == 0) {
						// fprintf(debug, "%s %d %hhu, %d\n", name, type, *(uint8_t *)(buf_udp + 51), ntohl(*(uint32_t*)(buf_udp + 52)));
						uint8_t sign = *(uint8_t *)(buf_udp + 51);
						uint32_t nr = ntohl(*(uint32_t*)(buf_udp + 52));

						if (sign == 0) {
							sprintf(send_packet.message, "%s - INT - %d", name, nr);
						} else {
							sprintf(send_packet.message, "%s - INT - -%d", name, nr);
						}

					} else if (type == 1) {
						uint16_t nr = htons(*(uint16_t*)(buf_udp + 51));
						sprintf(send_packet.message, "%s - SHORT_REAL - %.2f", name, (float)abs(nr) / 100);
						// fprintf(debug, "%s %d %hu\n", name, type, htons(*(uint16_t*)(buf_udp + 51)));
					} else if (type == 2) {
						//fprintf(debug, "%s %d %hd\n", name, type, buf_udp + 51);
						// fprintf(debug, "complicat\n");
						uint8_t sign = *(uint8_t *)(buf_udp + 51);
						uint32_t nr = ntohl(*(uint32_t*)(buf_udp + 52));
						uint8_t pw = *(uint8_t *)(buf_udp + 56);

						if (sign == 0) {
							sprintf(send_packet.message, "%s - FLOAT - %.10g", name, (double) nr / pow(10, pw));
						} else {
							sprintf(send_packet.message, "%s - FLOAT - -%.10g", name, (double) nr / pow(10, pw));
						}

					} else {
						// fprintf(debug, "%s %d %s\n", name, type, buf_udp + 51);
						sprintf(send_packet.message, "%s - STRING - %s", name, buf_udp + 51);
					}

					send_packet.len = strlen(send_packet.message) + 1;
					// fprintf(debug, "%s\n",send_packet.message);
					// fflush(debug);

					for (int j = 0; j < num_clients_tcp; j++) {
						if (clients[j].online && is_subscriber(clients[j], name)) {
							send_all(clients[j].sockfd, &send_packet, sizeof(struct chat_packet));
						}
					}

					int ok = 0;
					for (int j = 0; j < topics_size; j++) {
						if (strcmp(topics[j].name, name) == 0) {
							// TODO: add new msg + function
							int len = topics[j].messages_len;
							topics[j].messages[len].type = type;
							strcpy(topics[j].messages[len].msg, send_packet.message);
							topics[j].messages_len++;
							ok = 1;
							break;
						}
					}

					// create new + function
					if (!ok) {
						topics[topics_size].messages_len = 0;
						strcpy(topics[topics_size].name, name);
						// topics[topics_size].messages = malloc(1000 * sizeof(TUdpMsg));

						topics[topics_size].messages[0].type = type;
						strcpy(topics[topics_size].messages[0].msg, send_packet.message);
						topics[topics_size].messages_len++;
						topics_size++;

						// fprintf(debug, "\n\n\n NEW TOPIC: %s \n\n\n", name);
						// fflush(debug);
					}

					nr_of_events--;

				} else {
					int rc = recv_all(poll_fds[i].fd, &received_packet, sizeof(received_packet));
					DIE(rc < 0, "recv");

					// fprintf(debug, "start\n");
					// fflush(debug);

					if (rc == 0) {
						// printf("conexiune inchise\n");
						for (int j = 0; j < num_clients_tcp; j++) {
							if (clients[j].sockfd == poll_fds[i].fd) {
								printf("Client %s disconnected.\n", clients[j].id);
								clients[j].sockfd = -1;
								clients[j].online = 0;
								close(poll_fds[i].fd);
								break;
							}
						}

						// fprintf(debug, "aici crapam\n");
						// fflush(debug);

						// se scoate din multimea de citire socketul inchis
						for (int j = i; j < num_clients_online - 1; j++) {
							// clients[j] = clients[j + 1];
							poll_fds[j] = poll_fds[j + 1];
						}

						poll_fds[num_clients_online - 1].revents = 0;
						num_clients_online--;

					} else {
					// fprintf(debug, "S-a primit de la clientul de pe socketul %d mesajul: %s",
					// poll_fds[i].fd, received_packet.message);
					// fflush(debug);

					/* TODO 2.1: Trimite mesajul catre toti ceilalti clienti */
						// for (int j = 3; j < num_clients_online; j++) {
							// if (j != i) {
							// 	send_all(poll_fds[j].fd, &received_packet, sizeof(received_packet));
							// }
							// }

					// subscribe simplu
						if (strncmp(received_packet.message, "subscribe", 9) == 0) {
							char *p = strtok(received_packet.message, " ");
							char *topic = strtok(NULL, " ");
							char* sf = strtok(NULL, " ");

							for (int j = 0; j < num_clients_tcp; j++) {
								if (poll_fds[i].fd == clients[j].sockfd) {
									if (sf[0] == '0') {
										int len = clients[j].topics_len;
										strcpy(clients[j].topics_name[len], topic);
										clients[j].last_msg_idx[len] = -1;
										clients[j].topics_len++;
									} else {
										int len = clients[j].topics_len;
										strcpy(clients[j].topics_name[len], topic);

										for (int k = 0; k < topics_size; k++) {
											if (strcmp(topics[k].name, topic) == 0) {
												clients[j].last_msg_idx[len] = topics[k].messages_len - 1;
											}
										}
										// clients[j].last_msg_idx = ;
										clients[j].topics_len++;
									}
								}
							}
						}
					}
					nr_of_events--;
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
	// buf_tcp = (char*) malloc(2000 * sizeof(char));

	int rc = sscanf(argv[1], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// Obtinem un socket TCP pentru receptionarea conexiunilor
	int listenfd_tcp = socket(AF_INET, SOCK_STREAM, 0);
	DIE(listenfd_tcp < 0, "socket");

	int listenfd_udp = socket(PF_INET, SOCK_DGRAM, 0);
	DIE(listenfd_udp < 0, "socket");

	int enable = 1;
	setsockopt(listenfd_tcp, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

	// Completăm in serv_addr adresa serverului, familia de adrese si portul
	// pentru conectare

	// Facem adresa socket-ului reutilizabila, ca sa nu primim eroare in caz ca
	// rulam de 2 ori rapid
	enable = 1;
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
