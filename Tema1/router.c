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
        struct icmphdr *icmp_hdr = (struct icmphdr*)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));
		
        
		if (checksum((void*)ip_hdr, sizeof(struct iphdr)))
            continue;
        
        struct route_table_entry* best_route = get_best_route(ip_hdr->daddr);

        if (ip_hdr->ttl <= 1) {
            icmp_hdr->type = 11;
            icmp_hdr->code = 0;
            ip_hdr->daddr = ip_hdr->saddr;
            best_route = get_best_route(ip_hdr->daddr);
            // ip_hdr->protocol = 1;
            continue;
        }

        if (best_route == NULL) {
            icmp_hdr->type = 3;
            icmp_hdr->code = 0;
            ip_hdr->daddr = ip_hdr->saddr;
            best_route = get_best_route(ip_hdr->daddr);
            // ip_hdr->protocol = 1;
            continue;
        }

        ip_hdr->ttl--;
        ip_hdr->check = ~(~ip_hdr->check - 1);

        memcpy(eth_hdr->ether_dhost, get_mac_entry(best_route->next_hop)->mac, sizeof(uint8_t) * 6);
        get_interface_mac(best_route->interface, eth_hdr->ether_shost);

        send_to_link(best_route->interface, buf, len);
	}

	return 0;
}

