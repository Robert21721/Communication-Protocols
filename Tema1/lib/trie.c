#include <stdlib.h>
#include <stdint.h>
#include "lib.h"
#include <arpa/inet.h>

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


void add_entry_in_tree(struct route_table_entry *e, struct node *t) {
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
            if (iter->l != NULL) {
                iter = iter->l;
            } else {
                struct node *new_node = create_node();
                new_node->p = iter;
                iter->l = new_node;
                iter = new_node;
            }
        } else {
            if (iter->r != NULL) {
                iter = iter->r;
            } else {
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
        add_entry_in_tree(&rtable[i], tree);
    }

    return tree;
}

struct route_table_entry *get_best_route(uint32_t ip_dest, struct node *t) {
    struct node *iter = t;

	for (int i = 0; i < 32; i++) {
        int m = 1 << (31 - i);
        int dir = (htonl(ip_dest) & m);

        if (dir == 0) {
            if (iter->l != NULL) {
                iter = iter->l;
            } else {
                break;
            }
        } else {
            if (iter->r != NULL) {
                iter = iter->r;
            } else {
                break;
            }
        }
    }

    return iter->entry;
}