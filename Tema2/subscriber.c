/*
* Protocoale de comunicatii
* Laborator 7 - TCP si mulplixare
* client.c
*/

#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "helpers.h"

void run_client(int sockfd) {
	char buf[MSG_MAXSIZE + 1];
	memset(buf, 0, MSG_MAXSIZE + 1);

	struct chat_packet sent_packet;
	struct chat_packet recv_packet;

	/* TODO 2.2: Multiplexeaza intre citirea de la tastatura si primirea unui
		mesaj, ca sa nu mai fie impusa ordinea.
	*/

	struct pollfd fds[2];
	fds[0].fd = sockfd;
	fds[0].events = POLLIN;
	fds[1].fd = STDIN_FILENO;
	fds[1].events = POLLIN;


	while (1) {
		int ready = poll(fds, 2, -1);
		if (ready == -1) {
			perror("poll\n");
			return;
		}

		if (fds[0].revents & POLLIN) {
			int rc = recv_all(sockfd, &recv_packet, sizeof(recv_packet));
			if (rc <= 0) {
				break;
			}

			// printf("%s", recv_packet.message);
		}

		if (fds[1].revents & POLLIN) {
			fgets(buf, sizeof(buf), stdin);

			sent_packet.len = strlen(buf) + 1;
			strcpy(sent_packet.message, buf);
			// sent_packet.client_id = id;

			if(strncmp(buf, "exit", 4) == 0) {
				close(sockfd);
				return;
			} else {
				send_all(sockfd, &sent_packet, sizeof(sent_packet));
			}


			// Use send_all function to send the pachet to the server.
		}
	}
}

int main(int argc, char *argv[]) {
	int sockfd = -1;

	if (argc != 4) {
		//printf("\n Usage: %s <ip> <port>\n", argv[0]);
		return 1;
	}

	setvbuf(stdout, NULL, _IONBF, BUFSIZ);

	// Parsam port-ul ca un numar
	uint16_t port;
	struct chat_packet sent_packet;
	int rc = sscanf(argv[3], "%hu", &port);
	DIE(rc != 1, "Given port is invalid");

	// Obtinem un socket TCP pentru conectarea la server
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	DIE(sockfd < 0, "socket");

	// Completăm in serv_addr adresa serverului, familia de adrese si portul
	// pentru conectare
	struct sockaddr_in serv_addr;
	socklen_t socket_len = sizeof(struct sockaddr_in);

	memset(&serv_addr, 0, socket_len);
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
	DIE(rc <= 0, "inet_pton");

	// Ne conectăm la server
	rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	DIE(rc < 0, "connect");

	// trimit un pachet cu id ul clientului
	strcpy(sent_packet.message, argv[1]);
	sent_packet.len = strlen(argv[1]) + 1;
	send_all(sockfd, &sent_packet, sizeof(sent_packet));

	run_client(sockfd);

	// Inchidem conexiunea si socketul creat
	// close(sockfd);

	return 0;
}
