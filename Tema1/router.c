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


int main(int argc, char *argv[])
{
	char buf[MAX_PACKET_LEN];

	// Do not modify this line
	init(argc - 2, argv + 2);

	// de aici incepe chinu
	int interface;
    int len;

	 rtable = malloc(sizeof(struct route_table_entry) * 100000);
    /* DIE is a macro for sanity checks */
    DIE(rtable == NULL, "memory");

    mac_table = malloc(sizeof(struct arp_entry) * 100000);
    DIE(mac_table == NULL, "memory");

    /* Read the static routing table and the MAC table */
    rtable_len = read_rtable(argv[1], rtable);
    mac_table_len = parse_arp_table("arp_table.txt", mac_table);


	while (1) {

		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header *) buf;
		struct iphdr *ip_hdr = (struct iphdr *)(buf + sizeof(struct ether_header));
		/* Note that packets received are in network order,
		any header field which has more than 1 byte will need to be conerted to
		host order. For example, ntohs(eth_hdr->ether_type). The oposite is needed when
		sending a packet on the link, */

		if (ip_hdr->check != ip_hdr->check + checksum((void*)ip_hdr, sizeof(struct iphdr)))
            continue;
            
        printf("Check is good\n");
        
        /* TODO 2.2: Call get_best_route to find the most specific route, continue; (drop) if null */
        struct route_table_entry* best_route = get_best_route(ip_hdr->daddr);
        if (best_route == NULL)
            continue;

        /* TODO 2.3: Check TTL >= 1. Update TLL. Update checksum using the incremental forumla  */
        ip_hdr->ttl--;
        if (ip_hdr->ttl <= 0) 
            continue;

        ip_hdr->check = ~(~ip_hdr->check + ~(ip_hdr->ttl + 1) + ip_hdr->ttl) - 1;

        /* TODO 2.4: Update the ethernet addresses. Use get_mac_entry to find the destination MAC
                 * address. Use get_interface_mac(m.interface, uint8_t *mac) to
                 * find the mac address of our interface. */
        memcpy(eth_hdr->ether_dhost, get_mac_entry(best_route->next_hop)->mac, sizeof(uint8_t) * 6);
        get_interface_mac(best_route->interface, eth_hdr->ether_shost);
        

        // Call send_to_link(best_router->interface, packet, len);
        send_to_link(best_route->interface, buf, len);

	}

	return 0;
}

