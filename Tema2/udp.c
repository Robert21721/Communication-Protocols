#include "common.h"
#include "helpers.h"

int is_subscriber(TClient client, char *topic_name) {
    for (int i = 0; i < client.topics_len; i++) {
        if (strcmp(client.topics_name[i], topic_name) == 0) {
            return 1;
        }
     }

     return 0;
}