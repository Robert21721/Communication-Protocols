int is_client_connected(int num_clients_tcp, int num_clients_online, TClient *clients, char *msg, struct pollfd *poll_fds);
int not_first_time(int num_clients_tcp, TClient *clients, char *msg, int newsockfd);
void send_old_msg(TClient client, int topics_size, TTopic *topics, struct chat_packet *send_packet);
void update_idx_list(TClient client, int topics_size, TTopic *topics);