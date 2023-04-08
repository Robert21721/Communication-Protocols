#include <stdint.h>

struct node *create_tree(struct route_table_entry *rtable, int rtable_len);
struct route_table_entry *get_best_route(uint32_t ip_dest, struct node *t);
