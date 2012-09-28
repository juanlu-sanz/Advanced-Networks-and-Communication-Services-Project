#include "../ipv4_base/ipv4.h"
#include "../ipv4_aux/ipv4_route_table.h"
#include <arpa/inet.h>
#include <stdlib.h>

#define IP_MAX_PACKET_SIZE 1500-20
#define IP_VERSION 4
#define IPV4_HEADER_SIZE 20
#define IP_ETH_TYPE 0x0800

ipv4_route_table_t *ipv4_get_table();
void ipv4_open(char *config_file, char *route_table);
void ipv4_close();
int ipv4_send(ipv4_addr_t addr_dst, int protocol, unsigned char *payload, int payload_len);
int ipv4_receive(ipv4_addr_t src, unsigned char *buffer, uint8_t protocol, int timeout);
