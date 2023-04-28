#include "common.h"
#include "helpers.h"

uint16_t port;
char ip_addr[20];

void run_client(int sockfd) {
	char buf[MSG_MAXSIZE + 1];
	memset(buf, 0, MSG_MAXSIZE + 1);

	struct chat_packet sent_packet;
	struct chat_packet recv_packet;

	// un client asteapta dupa doua evenimente posibile:
	// o comanda de la tastatura sau o notificare primita de la server
	struct pollfd fds[2];
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;

	while (1) {
		int ready = poll(fds, 2, -1);
		DIE(ready < 0, "poll");

		// date primite pe socket
		if (fds[0].revents & POLLIN) {
			int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
			// daca primim 0, conexiunea s-a inclis
			if (rc <= 0) {
				close(sockfd);
				return;			
			}
			printf("%s:%hu %s\n", ip_addr, port, recv_packet.message);
		}

		// date primite de la stdin
		if (fds[1].revents & POLLIN) {
			fgets(buf, sizeof(buf), stdin);

			sent_packet.len = strlen(buf) + 1;
			strcpy(sent_packet.message, buf);

			if(strncmp(buf, "exit", 4) == 0) {
				close(sockfd);
				return;
			} else {
				send_all(sockfd, &sent_packet, sizeof(sent_packet));
				if (strncmp(sent_packet.message, "subscribe", 9) == 0) {
					printf("Subscribed to topic.\n");
				} else {
					printf("Unsubscribed from topic.\n");
				}
			}
		}
	}
}

int main(int argc, char *argv[]) {
	int sockfd = -1;

	if (argc != 4) {
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	struct chat_packet sent_packet;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// copiez adresa ip
	strcpy(ip_addr, argv[2]);

	// deschid un socket tcp
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// dezactivarea algoritmului lui Nagle
	int enable = 1;
	setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	// conectarea la server
	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	// trimit serverului un mesaj cu ID-ul clientului
	strcpy(sent_packet.message, argv[1]);
	sent_packet.len = strlen(argv[1]) + 1;
	send_all(sockfd, &sent_packet, sizeof(sent_packet));

	run_client(sockfd);
	return 0;
}
