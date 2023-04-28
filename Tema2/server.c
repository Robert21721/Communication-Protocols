#include "common.h"
#include "helpers.h"
#include "tcp.h"
#include "udp.h"

#define MAX_CONNECTIONS 32
#define INITIAL_SIZE 100

TClient *clients;
TTopic *topics;
struct sockaddr_in serv_addr_tcp, serv_addr_udp;
socklen_t socket_len = sizeof(struct sockaddr_in);
char *buf_udp;
char *buf_tcp;

void run_chat_multi_server(int listenfd_tcp, int listenfd_udp) {
	topics = malloc(INITIAL_SIZE * sizeof(TTopic));
	clients = (TClient*) malloc(INITIAL_SIZE * sizeof(TClient));
	int topics_size = 0;

	struct pollfd poll_fds[MAX_CONNECTIONS];
	struct chat_packet received_packet;
	struct chat_packet send_packet;

	// Setam socket-ul listenfd_tcp pentru ascultare
	int rc = listen(listenfd_tcp, MAX_CONNECTIONS);
	DIE(rc < 0, "listen");

	// stdin, socketul de conexiuni tcp si cel udp vor fi adaugate de la inceput
	poll_fds[0].fd = STDIN_FILENO;
	poll_fds[0].events = POLLIN;
	poll_fds[1].fd = listenfd_tcp;
	poll_fds[1].events = POLLIN;
	poll_fds[2].fd = listenfd_udp;
	poll_fds[2].events = POLLIN;
	int num_clients_online = 3;
	int num_clients_tcp = 0;

	while (1) {

		// utilizam poll pentru a putea trimite/ primi mesaje fara a bloca serverul
		int nr_of_events = poll(poll_fds, num_clients_online, -1);
		DIE(nr_of_events < 0, "poll");

		// parcurgem lista de filedescriptori
		for (int i = 0; i < num_clients_online && nr_of_events != 0; i++) {

			// in cazul in care avem un eveniment setat
			if (poll_fds[i].revents & POLLIN) {

				// daca este input de la tastatura
				if (poll_fds[i].fd == STDIN_FILENO) {
					char buf[20];
					fgets(buf, sizeof(buf), stdin);

					// inchidem serverul si clientii
					if(strncmp(buf, "exit", 4) == 0) {
						for (int j = 1; j < num_clients_online; j++) {
							close(poll_fds[j].fd);
						}
						return;
					}

				// daca am primit o cerere de conexiune tcp
				} else if (poll_fds[i].fd == listenfd_tcp) {

					// acceptam cererea
					struct sockaddr_in cli_addr;
					socklen_t cli_len = sizeof(cli_addr);
					int newsockfd = accept(listenfd_tcp, (struct sockaddr *)&cli_addr, &cli_len);
					DIE(newsockfd < 0, "accept");

					// primim un mesaj ce contine ID ul clientului
					recv_all(newsockfd, &received_packet, sizeof(received_packet));

					// in cazul in care era deja conectat, inchidem conexiunea
					int connected = is_client_connected(num_clients_tcp, num_clients_online, clients, received_packet.message, poll_fds);
					if (connected) {
						close(newsockfd);
						printf("Client %s already connected.\n", received_packet.message);
						nr_of_events--;
						continue;
					}

					// daca este un client nou
					printf("New client %s connected from %s:%d.\n", received_packet.message, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
					poll_fds[num_clients_online].fd = newsockfd;
					poll_fds[num_clients_online].events = POLLIN;
					num_clients_online++;


					int first_time = 1;
					for (int j = 0; j < num_clients_tcp; j++) {
						if (strcmp(clients[j].id, received_packet.message) == 0) {
							// in cazul in care nu este prima data cand se conecteaza
							clients[j].sockfd = newsockfd;
							clients[j].online = 1;

							// trimitem mesajele din urma (daca este cazul)
							send_old_msg(clients[j], topics_size, topics, &send_packet);
							first_time = 0;
							break;
						}
					}

					// daca este prima data, il adaugam in lista
					if (first_time) {
						strcpy(clients[num_clients_tcp].id, received_packet.message);
						clients[num_clients_tcp].sockfd = newsockfd;
						clients[num_clients_tcp].online = 1;
						num_clients_tcp++;
					}

					// am rezolvat unul dintre evenimente
					nr_of_events--;

				// daca am primit un mesaj pe socketul udp
				} else if (poll_fds[i].fd == listenfd_udp) {
					int rc = recvfrom(listenfd_udp, buf_udp, 1551, 0, (struct sockaddr *)&serv_addr_udp, &socket_len);
					DIE(rc < 0, "recv from udp");

					char name[51];
					memcpy(name, buf_udp, 50);
					name[50] = '\0';

					uint8_t type = buf_udp[50];

					// daca mesajul este de tipul 0
					if (type == 0) {
						uint8_t sign = *(uint8_t *)(buf_udp + 51);
						uint32_t nr = ntohl(*(uint32_t*)(buf_udp + 52));

						// cu sau fara semn
						if (sign == 0) {
							sprintf(send_packet.message, "%s - INT - %d", name, nr);
						} else {
							sprintf(send_packet.message, "%s - INT - -%d", name, nr);
						}

					} else if (type == 1) {

						uint16_t nr = htons(*(uint16_t*)(buf_udp + 51));
						sprintf(send_packet.message, "%s - SHORT_REAL - %.2f", name, (float)abs(nr) / 100);
					} else if (type == 2) {

						uint8_t sign = *(uint8_t *)(buf_udp + 51);
						uint32_t nr = ntohl(*(uint32_t*)(buf_udp + 52));
						uint8_t pw = *(uint8_t *)(buf_udp + 56);

						// cu sau fara semn
						if (sign == 0) {
							sprintf(send_packet.message, "%s - FLOAT - %.10g", name, (double) nr / pow(10, pw));
						} else {
							sprintf(send_packet.message, "%s - FLOAT - -%.10g", name, (double) nr / pow(10, pw));
						}

					} else {
						sprintf(send_packet.message, "%s - STRING - %s", name, buf_udp + 51);
					}

					send_packet.len = strlen(send_packet.message) + 1;

					// parcurgem clientii tcp si daca gasim unul online care este si abonat
					// topicului primit, ii trimitem mesajul
					for (int j = 0; j < num_clients_tcp; j++) {
						if (clients[j].online && is_subscriber(clients[j], name)) {
							send_all(clients[j].sockfd, &send_packet, sizeof(struct chat_packet));
						}
					}

					// in cazul in care topicul primit exista
					int topic_exists = 0;
					for (int j = 0; j < topics_size; j++) {
						if (strcmp(topics[j].name, name) == 0) {
							// retinem mesajul
							int len = topics[j].messages_len;
							strcpy(topics[j].messages[len].msg, send_packet.message);
							topics[j].messages_len++;
							topic_exists = 1;
							break;
						}
					}

					// daca nu exista, il adaugam in lista
					if (!topic_exists) {
						topics[topics_size].messages_len = 0;
						strcpy(topics[topics_size].name, name);

						topics[topics_size].messages = (TUdpMsg *) malloc(INITIAL_SIZE * sizeof(TUdpMsg));
						strcpy(topics[topics_size].messages[0].msg, send_packet.message);
						topics[topics_size].messages_len++;
						topics_size++;

					}

					// am rezolvat unul dintre evenimente
					nr_of_events--;

				// orimim date de la unul din clientii tcp
				} else {
					int rc = recv_all(poll_fds[i].fd, &received_packet, sizeof(received_packet));
					DIE(rc < 0, "recv");

					// in cazul in care este o cerere de deconectare
					if (rc == 0) {
						for (int j = 0; j < num_clients_tcp; j++) {
							if (clients[j].sockfd == poll_fds[i].fd) {
								printf("Client %s disconnected.\n", clients[j].id);
								clients[j].sockfd = -1;
								clients[j].online = 0;

								// actualizeaza index-urile ultimelor notificari primite
								update_idx_list(clients[j], topics_size, topics);
								close(poll_fds[i].fd);
								break;
							}
						}

						// se scoate din lista socketul inchis
						for (int j = i; j < num_clients_online - 1; j++) {
							poll_fds[j] = poll_fds[j + 1];
						}

						num_clients_online--;

					// am primit o cerere de subscribe/ unsubscribe
					} else {

						// daca cererea este de subscribe
						char *p = strtok(received_packet.message, " ");

						if (strcmp(p, "subscribe") == 0) {
							char *topic = strtok(NULL, " ");
							char* sf = strtok(NULL, " ");

							for (int j = 0; j < num_clients_tcp; j++) {
								// am gasit clientul care a trimis mesajul
								if (poll_fds[i].fd == clients[j].sockfd) {

									// daca falgul este 0
									if (sf[0] == '0') {
										// adaugam topicul in lista
										int len = clients[j].topics_len;
										strcpy(clients[j].topics_name[len], topic);
										clients[j].last_msg_idx[len] = -1;
										clients[j].topics_len++;

									// daca flagul este 1
									} else {
										// adaugam topicul in lista
										int len = clients[j].topics_len;
										strcpy(clients[j].topics_name[len], topic);
										// se va retine indexul ultimului mesaj primit inainte de deconectare
										clients[j].last_msg_idx[len] = 0;
										clients[j].topics_len++;
									}
								}
							}
						} else {
							char *topic = strtok(NULL, " ");

							for (int j = 0; j < num_clients_tcp; j++) {
								// am gasit clientul care a trimis mesajul
								if (poll_fds[i].fd == clients[j].sockfd) {
									int idx = 0;

									// caut indexul la care se se afla topicul
									for (idx = 0; idx < clients[j].topics_len; idx++) {
										if (strcmp(clients[j].topics_name[idx], topic) == 0) {
											break;
										}
									}

									// sterg topicul impreuna cu indexul sau
									for (int k = idx; k < clients[j].topics_len - 1; k++) {
										strcpy(clients[j].topics_name[k], clients[j].topics_name[k + 1]);
										clients[j].last_msg_idx[k] = clients[j].last_msg_idx[k + 1];
									}

									clients[j].topics_len--;
								}
							}
						}
					}

					// am rezolvat unul dintre evenimente
					nr_of_events--;
				}
			}
		}
	}
}

	int main(int argc, char *argv[]) {
	if (argc != 2) {
		return -1;
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
