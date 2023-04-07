#include "queue.h"
#include "lib.h"
#include "protocols.h"
#include <string.h>
#include <arpa/inet.h>
#include "list.h"

void print_ip(char *msg, unsigned int ip) {
    unsigned char bytes[4];
    bytes[0] = ip & 0xFF;
    bytes[1] = (ip >> 8) & 0xFF;
    bytes[2] = (ip >> 16) & 0xFF;
    bytes[3] = (ip >> 24) & 0xFF;   
    printf("%s: %d.%d.%d.%d\n",msg, bytes[3], bytes[2], bytes[1], bytes[0]);        
}

void print_mac(char *msg, uint8_t *mac) {
        printf("%s: %x.%x.%x.%x.%x.%x\n", msg, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}


struct ip_pack {
    void *buf;
    uint32_t next_ip;
    int len;
    int interface;
};

struct node {
    struct node *p;
    struct node *l;
    struct node *r;
    struct route_table_entry *entry;
};

struct node* create_node() {
    struct node *n = (struct node*)malloc(sizeof(struct node));

    n->p = NULL;
    n->l = NULL;
    n->r = NULL;
    n->entry = NULL;

    return n;
}

int get_ip_len(uint32_t ip) {
    int len = 0;

    while (ip != 0) {
        ip = ip << 1;
        len++;
    }

    return len;
}


void add_router_in_tree(struct route_table_entry *e, struct node *t) {
    int mask_len = get_ip_len(htonl(e->mask));
    int prefix_len = get_ip_len(htonl(e->prefix));

    if (prefix_len > mask_len) {
        return;
    }

    struct node *iter = t;
    for (int i = 0; i < mask_len; i++) {
        int m = 1 << (31 - i);
        int dir = (htonl(e->prefix) & m);
 
        if (dir == 0) {
            // printf("stanga\n");
            if (iter->l != NULL) {
                iter = iter->l;
            } else {
                // printf("adaug nod st\n");
                struct node *new_node = create_node();
                new_node->p = iter;
                iter->l = new_node;
                iter = new_node;
            }
        } else {
            if (iter->r != NULL) {
                iter = iter->r;
            } else {
                // printf("adaug nod dr\n");
                struct node *new_node = create_node();
                new_node->p = iter;
                iter->r = new_node;
                iter = new_node;
            }
        }

    }

    iter->entry = e;
}

struct node *create_tree(struct route_table_entry *rtable, int rtable_len) {
    struct node *tree = create_node();

    for (int i = 0; i < rtable_len; i++) {
        add_router_in_tree(&rtable[i], tree);
    }

    return tree;
}

struct route_table_entry *get_best_route(uint32_t ip_dest, struct node *t) {
    struct node *iter = t;

	for (int i = 0; i < 32; i++) {
        int m = 1 << (31 - i);
        int dir = (htonl(ip_dest) & m) >> (31 - i);

        if (dir == 0) {
            if (iter->l != NULL) {
                // printf("0\n");
                iter = iter->l;
            } else {
                break;
            }
        } else {
            if (iter->r != NULL) {
                // printf("1\n");
                iter = iter->r;
            } else {
                break;
            }
        }

        // printf("\n");
    }

    return iter->entry;
}

// struct route_table_entry *get_best_route(uint32_t ip_dest, struct route_table_entry *rtable, int rtable_len) {
// 	int max = 0;
// 	struct route_table_entry* ret = NULL;

//     for (int i = 0; i < rtable_len; i++) {
//         if (rtable[i].prefix == (ip_dest & rtable[i].mask)) {
//             if (rtable[i].mask > max) {
// 				max = rtable[i].mask;
// 				ret = &rtable[i];
// 			}
//         }
//     }
//     return ret;
// }

struct arp_entry *get_mac_entry(uint32_t ip_dest, struct arp_entry *mac_table, int mac_table_len) {
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

    // printf("create icmp\n");

    memcpy(eth_hdr->ether_dhost, eth_hdr_aux->ether_shost, 6);
    memcpy(eth_hdr->ether_shost, eth_hdr_aux->ether_dhost, 6);
    eth_hdr->ether_type = htons(0x800);

    ip_hdr->daddr = ip_hdr_aux->saddr;
    ip_hdr->saddr = ip_hdr_aux->daddr;


    // print_ip("ip saddr", ip_hdr->saddr);
    // print_ip("ip daddr", ip_hdr->daddr);

    // print_mac("mac shost", eth_hdr->ether_shost);
    // print_mac("mac dhost", eth_hdr->ether_dhost);

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

void create_arp_req(void *buf, size_t *len, uint32_t dest_ip, int interface) {
    struct ether_header *eth_hdr = (struct ether_header*) buf;
    struct arp_header *arp_hdr = (struct arp_header*)(buf + sizeof(struct ether_header));

    void *aux = malloc(*len);
    memcpy(aux, buf, *len);
    struct ether_header *eth_hdr_aux = (struct ether_header*) aux;
    //struct arp_header *arp_hdr_aux = (struct arp_header*)(aux + sizeof(struct ether_header));

    // printf("create arp req\n");
    uint32_t source_ip = inet_addr(get_interface_ip(interface));

    memcpy(eth_hdr->ether_shost, eth_hdr_aux->ether_dhost, 6);
    memset(eth_hdr->ether_dhost, ~0, 6);
    eth_hdr->ether_type = htons(0x806);

    arp_hdr->htype = htons(1);
    arp_hdr->ptype = htons(0x800);
    arp_hdr->hlen = 6;
    arp_hdr->plen = 4;
    arp_hdr->op = htons(1);

    memcpy(arp_hdr->sha, eth_hdr->ether_shost, 6);
    arp_hdr->spa = source_ip;

    memset(arp_hdr->tha, 0, 6);
    arp_hdr->tpa = dest_ip;

    // print_ip("ip src", arp_hdr->spa);
    // print_ip("ip dest", arp_hdr->tpa);

    // print_mac("mac src", arp_hdr->sha);
    // print_mac("mac dest", arp_hdr->tha);

    *len = sizeof(struct ether_header) + sizeof(struct arp_header);
    free(aux);
}


int main(int argc, char *argv[]) {
    void *buf = malloc(MAX_PACKET_LEN);
    DIE(buf == NULL, "memory");

    struct route_table_entry *rtable = malloc(sizeof(struct route_table_entry) * 100000);
    DIE(rtable == NULL, "memory");
    int rtable_len = read_rtable(argv[1], rtable);

    struct arp_entry *mac_table = malloc(sizeof(struct arp_entry) * 100);
    int mac_table_len = 0;

    struct queue *queue = queue_create();
    struct node *tree = create_tree(rtable, rtable_len);

	// Do not modify this line
	init(argc - 2, argv + 2);

	while (1) {
		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);

		DIE(interface < 0, "recv_from_any_links");
        // printf("message recieved from interface %d\n", interface);

        struct ether_header *eth_hdr = (struct ether_header*) buf;

        if (eth_hdr->ether_type == htons(0x800)) {
            // struct ether_header *eth_hdr = (struct ether_header*) buf;
            // printf("ip / icmp\n");
            struct iphdr *ip_hdr = (struct iphdr*)(buf + sizeof(struct ether_header));
            struct icmphdr *icmp_hdr = (struct icmphdr*)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));

            if (checksum((void*)ip_hdr, sizeof(struct iphdr)))
                continue;

            ip_hdr->check = ~(~ip_hdr->check - 1);

            ip_hdr->ttl--;
            if (ip_hdr->ttl <= 0) {
                create_icmp(buf, &len, 11, 0);
                send_to_link(interface, buf, len);
                continue;
            }

            struct route_table_entry* best_route = get_best_route(ip_hdr->daddr, tree);
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
            // printf("interfat pe care trimitem este %d\n", interface);
            get_interface_mac(interface, eth_hdr->ether_shost);
            struct arp_entry *mac_dest = get_mac_entry(best_route->next_hop, mac_table, mac_table_len);

            if (mac_dest != NULL) {
                // printf("avem mac in tabela\n");
                memcpy(eth_hdr->ether_dhost, mac_dest->mac, 6);
                send_to_link(interface, buf, len);
            } else {
                
                // printf("nu avem mac in tabela\n");
                struct ip_pack *pack = (struct ip_pack*) malloc(sizeof(struct ip_pack));
                pack->buf = malloc(1000);
                memcpy(pack->buf, buf, len);
                pack->next_ip = best_route->next_hop;
                pack->len = len;
                pack->interface = interface;

                // printf("arp trimis\n");
                queue_enq(queue, (void*)pack);
                create_arp_req(buf, &len, best_route->next_hop, interface);
                send_to_link(interface, buf, len);
            }

        } else if (eth_hdr->ether_type == htons(0x806)) {
            // printf("am primit un arp\n");
            int32_t ip_interface = inet_addr(get_interface_ip(interface));
            struct arp_header *arp_hdr = (struct arp_header*)(buf + sizeof(struct ether_header));

            void *aux = malloc(len);
            memcpy(aux, buf, len);
            struct arp_header *arp_hdr_aux = (struct arp_header*)(aux + sizeof(struct ether_header));

            if (htons(arp_hdr->op) == 1) {
                // printf("am primit arp req\n");
                if (ip_interface != arp_hdr->tpa) {
                    continue;
                }
                
                // printf("si e pentru noi\n");
                arp_hdr->op = htons(2);

                get_interface_mac(interface, arp_hdr->sha);
                memcpy(arp_hdr->tha, arp_hdr_aux->sha, 6);

                memcpy(eth_hdr->ether_shost, arp_hdr->sha, 6);
                memcpy(eth_hdr->ether_dhost, arp_hdr->tha, 6);

                arp_hdr->spa = arp_hdr_aux->tpa;
                arp_hdr->tpa = arp_hdr_aux->spa;

                // print_ip("arp s_ip", arp_hdr->spa);
                // print_ip("arp d_ip", arp_hdr->tpa);

                // print_mac("arp s mac", arp_hdr->sha);
                // print_mac("arp d mac", arp_hdr->tha);

                send_to_link(interface, buf, len);
                free(aux);
                // continue;

            } else if (htons(arp_hdr->op) == 2) {
                // printf("arp rsponse\n");
                memcpy(mac_table[mac_table_len].mac, arp_hdr->sha, 6);
                mac_table[mac_table_len].ip = arp_hdr->spa;
                mac_table_len++;

                struct queue *new_q = queue_create();

                while (!queue_empty(queue)) {
                    struct ip_pack *pack = (struct ip_pack*)queue_deq(queue);
                    struct ether_header *pack_eth = (struct ether_header*)(pack->buf);
   
                    int ok = 0;
                    for (int i = 0; i < mac_table_len; i++) {
                        if (pack->next_ip == mac_table[i].ip) {
                            memcpy(pack_eth->ether_dhost, mac_table[i].mac, 6);
                            ok = 1;
                            break;
                        }
                    }

                    if (ok) {
                        send_to_link(pack->interface, pack->buf, pack->len); 
                    } else {
                        queue_enq(new_q, (void*)pack);
                    } 
                }

                queue = new_q;
            }
        }
    }

	return 0;
}
