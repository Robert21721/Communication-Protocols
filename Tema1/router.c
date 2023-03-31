#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <string.h>
#include <arpa/inet.h>

struct route_table_entry *rtable;
int rtable_len;

struct arp_entry *mac_table;
int mac_table_len;


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

void create_icmp(void *buf, size_t *len, int type, int code) {
    void *aux = malloc(*len);
    memcpy(aux, buf, *len);

    struct ether_header *eth_hdr = (struct ether_header*) buf;
	struct iphdr *ip_hdr = (struct iphdr*)(buf + sizeof(struct ether_header));
    struct icmphdr *icmp_hdr = (struct icmphdr*)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));

    struct ether_header *eth_hdr_aux = (struct ether_header*) aux;
    struct iphdr *ip_hdr_aux = (struct iphdr*)(aux + sizeof(struct ether_header));
    struct icmphdr *icmp_hdr_aux = (struct icmphdr*)(aux + sizeof(struct ether_header) + sizeof(struct iphdr));

    memcpy(eth_hdr->ether_dhost, eth_hdr_aux->ether_shost, 6);
    memcpy(eth_hdr->ether_shost, eth_hdr_aux->ether_dhost, 6);
    eth_hdr->ether_type = htons(0x800);

    ip_hdr->daddr = ip_hdr_aux->saddr;
    ip_hdr->saddr = ip_hdr_aux->daddr;

    ip_hdr->version = 4;
    ip_hdr->ihl = 5;
    ip_hdr->tos = 0;
    ip_hdr->tot_len = htons(sizeof(struct iphdr) + sizeof(struct icmphdr));
    ip_hdr->id = 1;
    ip_hdr->frag_off = 0;
    ip_hdr->protocol = 1;
    ip_hdr->ttl = 64;
    ip_hdr->check = 0;
    ip_hdr->check = htons(checksum((void*)ip_hdr,sizeof(struct iphdr)));

    icmp_hdr->un.echo.id = 1;
    icmp_hdr->un.echo.sequence = icmp_hdr_aux->un.echo.sequence;

    icmp_hdr->type = type;
    icmp_hdr->code = code;
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = htons(checksum((void*)icmp_hdr, sizeof(struct icmphdr)));

    int offset = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);
    memcpy(buf + offset, ip_hdr_aux, sizeof(struct iphdr) + 8);

    *len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + 8;
    free(aux);
}


int main(int argc, char *argv[]) {
    void *buf = malloc(MAX_PACKET_LEN);
     DIE(buf == NULL, "memory");

	// Do not modify this line
	init(argc - 2, argv + 2);

	// de aici incepe chinu
	rtable = malloc(sizeof(struct route_table_entry) * 100000);
    DIE(rtable == NULL, "memory");

    mac_table = malloc(sizeof(struct arp_entry) * 100);
    DIE(mac_table == NULL, "memory");

    rtable_len = read_rtable(argv[1], rtable);
    mac_table_len = parse_arp_table("t.txt", mac_table);

	while (1) {
		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

		struct ether_header *eth_hdr = (struct ether_header*) buf;
		struct iphdr *ip_hdr = (struct iphdr*)(buf + sizeof(struct ether_header));
        struct icmphdr *icmp_hdr = (struct icmphdr*)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));


        // if (eth_hdr->ether_type == htons(0x0806)) {
        //     int32_t ip_interface = inet_addr(get_interface_ip(interface));
        //     struct arp_header *arp_hdr = (struct arp_header*)(buf + sizeof(struct ether_header));

        //     void *aux = malloc(len);
        //     memcpy(aux, buf, len);
        //     struct arp_header *arp_hdr_aux = (struct arp_header*)(aux + sizeof(struct ether_header));

        //     if (htons(arp_hdr->op) == 1) {
        //         printf("%d %d\n", ip_interface, arp_hdr->tpa);
        //         if (ip_interface != arp_hdr->tpa) {
        //             continue;
        //         }

        //         arp_hdr->op = htons(2);

        //         get_interface_mac(interface, arp_hdr->sha);
        //         memcpy(arp_hdr->tha, arp_hdr_aux->sha, 6);

        //         memcpy(eth_hdr->ether_shost, arp_hdr->sha, 6);
        //         memcpy(eth_hdr->ether_dhost, arp_hdr->tha, 6);

        //         arp_hdr->spa = arp_hdr_aux->tpa;
        //         arp_hdr->spa = arp_hdr_aux->spa;

        //         send_to_link(interface, buf, len);
        //         free(aux);
        //         continue;
        //     }
        // }

		if (checksum((void*)ip_hdr, sizeof(struct iphdr)))
            continue;

        ip_hdr->check = ~(~ip_hdr->check - 1);
 
        ip_hdr->ttl--;
        if (ip_hdr->ttl <= 0) {
            create_icmp(buf, &len, 11, 0);
            send_to_link(interface, buf, len);
            continue;
        }

        struct route_table_entry* best_route = get_best_route(ip_hdr->daddr);
        if (best_route == NULL) {
            create_icmp(buf, &len, 3, 0);
            send_to_link(interface, buf, len);
            continue;
        }

        if (ip_hdr->protocol == IPPROTO_ICMP) {
            int32_t ip_interface = inet_addr(get_interface_ip(interface));

            if (icmp_hdr->type == 8 && icmp_hdr->code == 0 && ip_hdr->daddr == ip_interface) {
                create_icmp(buf, &len, 0, 0);
                send_to_link(interface, buf, len);
                continue;
            }
        }



        interface = best_route->interface;
        memcpy(eth_hdr->ether_dhost, get_mac_entry(best_route->next_hop)->mac, sizeof(uint8_t) * 6);
        get_interface_mac(interface, eth_hdr->ether_shost);

        send_to_link(interface, buf, len);
	}

    free(buf);
	return 0;
}
