#include <stdio.h>
#include <stdint.h>

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