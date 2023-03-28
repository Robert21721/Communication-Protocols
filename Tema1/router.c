#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <string.h>

struct route_table_entry *rtable;
int rtable_len;

struct arp_entry *mac_table;
int mac_table_len;


// struct mac_entry *mac_table;
// int mac_table_len;

struct route_table_entry *get_best_route(uint32_t ip_dest) {
	int max = 0;
	struct route_table_entry* ret = NULL;

    for (int i = 0; i < rtable_len; i++) {
        if (rtable[i].prefix == (ip_dest & rtable[i].mask)) {
            if (rtable[i].mask > max) {
				max = rtable[i].mask;
				ret = &rtable[i];
			}
        }
    }
    return ret;
}

struct arp_entry *get_mac_entry(uint32_t ip_dest) {
    for (int i = 0; i < mac_table_len; i++) {
        if (mac_table[i].ip == ip_dest)
            return &mac_table[i];
    }
    return NULL;
}


int main(int argc, char *argv[]) {
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	// de aici incepe chinu
	rtable = malloc(sizeof(struct route_table_entry) * 100000);
    DIE(rtable == NULL, "memory");

    mac_table = malloc(sizeof(struct arp_entry) * 100000);
    DIE(mac_table == NULL, "memory");

    rtable_len = read_rtable(argv[1], rtable);
    mac_table_len = parse_arp_table("arp_table.txt", mac_table);

	while (1) {
		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header*) buf;
		struct iphdr *ip_hdr = (struct iphdr*)(buf + sizeof(struct ether_header));
        struct icmphdr *icmp_hdr = (struct icmphdr*)(buf + sizeof(struct ether_header) + sizeof(struct icmphdr));

		if (checksum((void*)ip_hdr, sizeof(struct iphdr)))
            continue;

        ip_hdr->check = ~(~ip_hdr->check - 1);
 
        ip_hdr->ttl--;
        if (ip_hdr->ttl <= 0) {
            void *aux = malloc(len);
            memcpy(aux, buf, len);

            struct ether_header *eth_hdr_aux = (struct ether_header*) aux;
            struct iphdr *ip_hdr_aux = (struct iphdr*)(aux + sizeof(struct ether_header));
            // struct icmphdr *icmp_hdr_aux = (struct icmphdr*)(aux + sizeof(struct ether_header) + sizeof(struct iphdr));

            memcpy(eth_hdr->ether_dhost, eth_hdr_aux->ether_shost, 6);
            memcpy(eth_hdr->ether_shost, eth_hdr_aux->ether_dhost, 6);
            eth_hdr->ether_type = 0x800;

            ip_hdr->daddr = ip_hdr_aux->saddr;
            // ip_hdr->saddr = *(int*)get_interface_ip(interface);


            printf("%s\n", get_interface_ip(interface));
            printf("%d\n", ip_hdr->daddr = ip_hdr_aux->saddr);
            ip_hdr->check = 0;
            ip_hdr->protocol = 1;
            ip_hdr->ttl = 64;

            icmp_hdr->type = 11;
            icmp_hdr->code = 0;
            icmp_hdr->checksum = 0;

            int offset = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);
            memcpy(buf + offset, ip_hdr_aux, sizeof(struct iphdr) + 8);

            len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + 8;
            send_to_link(interface, buf, len);
            continue;
        }

        struct route_table_entry* best_route = get_best_route(ip_hdr->daddr);
        if (best_route == NULL) {
            continue;
        }

        printf("ia sa vdm %d\n", ip_hdr->daddr);

        interface = best_route->interface;
        memcpy(eth_hdr->ether_dhost, get_mac_entry(best_route->next_hop)->mac, sizeof(uint8_t) * 6);
        get_interface_mac(interface, eth_hdr->ether_shost);

        printf("%ld\n", len);

        send_to_link(interface, buf, len);
	}

	return 0;
}
