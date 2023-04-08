#include "queue.h"
#include "lib.h"
#include "trie.h"
#include "protocols.h"
#include <string.h>
#include <arpa/inet.h>

struct ip_pack {
    void *buf;
    uint32_t next_ip;
    int len;
    int interface;
};

// returneaza adresa mac ce corespunde unui ip cunoscut
struct arp_entry *get_mac_entry(uint32_t ip_dest, struct arp_entry *mac_table, int mac_table_len) {
    for (int i = 0; i < mac_table_len; i++) {
        if (mac_table[i].ip == ip_dest)
            return &mac_table[i];
    }
    return NULL;
}


// creeaza un pachet de tip icmp
void create_icmp(void *buf, size_t *len, int type, int code) {
    void *aux = malloc(*len);
    DIE(aux == NULL, "malloc");

    memcpy(aux, buf, *len);

    // folosesc abufferul primit
    struct ether_header *eth_hdr = (struct ether_header*) buf;
	struct iphdr *ip_hdr = (struct iphdr*)(buf + sizeof(struct ether_header));
    struct icmphdr *icmp_hdr = (struct icmphdr*)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));

    // copiez datele din bufferul vechi
    struct ether_header *eth_hdr_aux = (struct ether_header*) aux;
    struct iphdr *ip_hdr_aux = (struct iphdr*)(aux + sizeof(struct ether_header));
    struct icmphdr *icmp_hdr_aux = (struct icmphdr*)(aux + sizeof(struct ether_header) + sizeof(struct iphdr));

    // completez bufferul primit ca parametru

    // interschim sursa cu destinatia pentru mac si ip
    memcpy(eth_hdr->ether_dhost, eth_hdr_aux->ether_shost, 6);
    memcpy(eth_hdr->ether_shost, eth_hdr_aux->ether_dhost, 6);
    eth_hdr->ether_type = htons(0x800);

    ip_hdr->daddr = ip_hdr_aux->saddr;
    ip_hdr->saddr = ip_hdr_aux->daddr;

    // completez campurile pentru ip si icmp
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


    icmp_hdr->type = type;
    icmp_hdr->code = code;
    icmp_hdr->un.echo.id = 1;
    icmp_hdr->un.echo.sequence = icmp_hdr_aux->un.echo.sequence;
    icmp_hdr->checksum = 0;
    icmp_hdr->checksum = htons(checksum((void*)icmp_hdr, sizeof(struct icmphdr)));

    // copiez vechiul ip + 64 bites
    int offset = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr);
    memcpy(buf + offset, ip_hdr_aux, sizeof(struct iphdr) + 8);

    // actualizez lungimea
    *len = sizeof(struct ether_header) + sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr) + 8;
    free(aux);
}


// creeaza un pachet arp req
void create_arp_req(void *buf, size_t *len, uint32_t dest_ip, int interface) {
    struct ether_header *eth_hdr = (struct ether_header*) buf;
    struct arp_header *arp_hdr = (struct arp_header*)(buf + sizeof(struct ether_header));

    // copiez vechiul buffer pentru a folosi headerul ethernet
    void *aux = malloc(*len);
    DIE(aux == NULL, "malloc");

    memcpy(aux, buf, *len);
    struct ether_header *eth_hdr_aux = (struct ether_header*) aux;
    uint32_t source_ip = inet_addr(get_interface_ip(interface));

    // setez sursa ca fiind routerul si destinatia broadcast
    memcpy(eth_hdr->ether_shost, eth_hdr_aux->ether_dhost, 6);
    memset(eth_hdr->ether_dhost, ~0, 6);
    eth_hdr->ether_type = htons(0x806);

    // completez campurile pt arp
    arp_hdr->htype = htons(1);
    arp_hdr->ptype = htons(0x800);
    arp_hdr->hlen = 6;
    arp_hdr->plen = 4;
    arp_hdr->op = htons(1);

    memcpy(arp_hdr->sha, eth_hdr->ether_shost, 6);
    arp_hdr->spa = source_ip;

    memset(arp_hdr->tha, 0, 6);
    arp_hdr->tpa = dest_ip;

    // actualizez lungimea
    *len = sizeof(struct ether_header) + sizeof(struct arp_header);
    free(aux);
}


int main(int argc, char *argv[]) {
    // bufferul folosit pentru primire si trimitere de pachete
    void *buf = malloc(MAX_PACKET_LEN);
    DIE(buf == NULL, "malloc");

    // tabela de rutare statica a routerului
    struct route_table_entry *rtable = malloc(sizeof(struct route_table_entry) * 100000);
    DIE(rtable == NULL, "malloc");

    int rtable_len = read_rtable(argv[1], rtable);

    // tabela mac ce va fi populata prin arp requesturi
    struct arp_entry *mac_table = malloc(sizeof(struct arp_entry) * 100);
    DIE(mac_table == NULL, "malloc");

    int mac_table_len = 0;

    // coada routerului penru pachete IP
    struct queue *queue = queue_create();
    // trie construit din tabela de rutare rtable
    struct node *tree = create_tree(rtable, rtable_len);

	init(argc - 2, argv + 2);

	while (1) {
		int interface;
		size_t len;

		interface = recv_from_any_link(buf, &len);
		DIE(interface < 0, "recv_from_any_links");

        // headerul ethernet al mesajului primit
        struct ether_header *eth_hdr = (struct ether_header*) buf;

        // daca am primit un pachet IP
        if (eth_hdr->ether_type == htons(0x800)) {
            struct iphdr *ip_hdr = (struct iphdr*)(buf + sizeof(struct ether_header));
            struct icmphdr *icmp_hdr = (struct icmphdr*)(buf + sizeof(struct ether_header) + sizeof(struct iphdr));

            // verific si actualizez checksum
            if (checksum((void*)ip_hdr, sizeof(struct iphdr)))
                continue;

            ip_hdr->check = ~(~ip_hdr->check - 1);

            // actualizez ttl
            ip_hdr->ttl--;
            if (ip_hdr->ttl <= 0) {
                // in cazul in care ttl a expirat, trimit un pachet icmp inapoi la sursa
                create_icmp(buf, &len, 11, 0);
                send_to_link(interface, buf, len);
                continue;
            }

            // aflu urmatoarea intrare din tabela arp folosind trie-ul
            struct route_table_entry* best_route = get_best_route(ip_hdr->daddr, tree);
            if (best_route == NULL) {
                // in cazul in care nu exista, trimit inapoi un pachet icmp
                create_icmp(buf, &len, 3, 0);
                send_to_link(interface, buf, len);
                continue;
            }

            // daca am primit un pachet icmp
            if (ip_hdr->protocol == IPPROTO_ICMP) {
                int32_t ip_interface = inet_addr(get_interface_ip(interface));
                // si pachetul este de tip icmp request si este pentru mine
                if (icmp_hdr->type == 8 && icmp_hdr->code == 0 && ip_hdr->daddr == ip_interface) {
                    // trimit inapoi un icmp reply
                    create_icmp(buf, &len, 0, 0);
                    send_to_link(interface, buf, len);
                    continue;
                }
            }

            // actualzez interfata, aflu adresa mac a sursei si cea a destinatiei
            interface = best_route->interface;
            get_interface_mac(interface, eth_hdr->ether_shost);
            struct arp_entry *mac_dest = get_mac_entry(best_route->next_hop, mac_table, mac_table_len);

            if (mac_dest != NULL) {
                // in cazul in care destinatia se afla in tablea arp, trimit pachetul
                memcpy(eth_hdr->ether_dhost, mac_dest->mac, 6);
                send_to_link(interface, buf, len);

            } else {
                
                //daca nu se afla, creez pun pachet in care retin informatiile si in pun
                // in coada de asteptare
                struct ip_pack *pack = (struct ip_pack*) malloc(sizeof(struct ip_pack));
                DIE(rtable == NULL, "malloc");

                pack->buf = malloc(1000);
                DIE(pack->buf == NULL, "malloc");

                memcpy(pack->buf, buf, len);
                pack->next_ip = best_route->next_hop;
                pack->len = len;
                pack->interface = interface;
                queue_enq(queue, (void*)pack);

                // creez un arp request pe care il trimit
                create_arp_req(buf, &len, best_route->next_hop, interface);
                send_to_link(interface, buf, len);
            }

            // daca am primit un pachet de tipul arp
        } else if (eth_hdr->ether_type == htons(0x806)) {
            // ip ul interfetei pe care a fost primit mesajul
            int32_t ip_interface = inet_addr(get_interface_ip(interface));
            struct arp_header *arp_hdr = (struct arp_header*)(buf + sizeof(struct ether_header));

            // copiez masajul primit
            void *aux = malloc(len);
            DIE(aux == NULL, "malloc");
            
            memcpy(aux, buf, len);
            struct arp_header *arp_hdr_aux = (struct arp_header*)(aux + sizeof(struct ether_header));

            // daca am primit un arp request
            if (htons(arp_hdr->op) == 1) {
                
                // daca nu este pentru mine
                if (ip_interface != arp_hdr->tpa) {
                    continue;
                }

                // trimit inapoi un arp reply
                arp_hdr->op = htons(2);

                // cu adresa mac corespunzatoare
                get_interface_mac(interface, arp_hdr->sha);
                memcpy(arp_hdr->tha, arp_hdr_aux->sha, 6);
                arp_hdr->spa = arp_hdr_aux->tpa;
                arp_hdr->tpa = arp_hdr_aux->spa;

                memcpy(eth_hdr->ether_shost, arp_hdr->sha, 6);
                memcpy(eth_hdr->ether_dhost, arp_hdr->tha, 6);

                send_to_link(interface, buf, len);
                free(aux);

            // daca am primit inapoi in arp reply
            } else if (htons(arp_hdr->op) == 2) {
                // adaug noua adresa mac in tablea arp
                memcpy(mac_table[mac_table_len].mac, arp_hdr->sha, 6);
                mac_table[mac_table_len].ip = arp_hdr->spa;
                mac_table_len++;

                // creez o coada noua
                struct queue *new_q = queue_create();
                
                // parcurg toate pachetele in asteptatre
                while (!queue_empty(queue)) {
                    struct ip_pack *pack = (struct ip_pack*)queue_deq(queue);
                    struct ether_header *pack_eth = (struct ether_header*)(pack->buf);
   
                    int ok = 0;
                    for (int i = 0; i < mac_table_len; i++) {
                        // daca am gasit ip ul destinatiei in tabela arp, o copiez in pachet
                        if (pack->next_ip == mac_table[i].ip) {
                            memcpy(pack_eth->ether_dhost, mac_table[i].mac, 6);
                            ok = 1;
                            break;
                        }
                    }

                    if (ok) {
                        // trimit pachetul
                        send_to_link(pack->interface, pack->buf, pack->len); 
                        free(pack->buf);
                        free(pack);
                    } else {
                        // altfel, pun pachetul inapoi in noua coada
                        queue_enq(new_q, (void*)pack);
                    } 
                }

                free(queue);
                queue = new_q;
            }
        }
    }

    free(rtable);
    free(mac_table);
    free(queue);
    free(buf);
	return 0;
}
