#include "../ipv4/ipv4_protocol.h"

#define UDP_MAX_SIZE 1500-20-4
#define UDP_PACKET_TYPE 17
#define UDP_HEADER_SIZE 8

void udp_open(char *config_file, char *route_table);
void udp_close();
uint16_t udp_checksum(unsigned char * data, int len, ipv4_addr_t src_addr, ipv4_addr_t dest_addr);
int udp_send(ipv4_addr_t addr_dst, unsigned short int dst_port,	unsigned short int src_port, unsigned char *payload, int payload_len);
int udp_receive(ipv4_addr_t addr_src, unsigned short int *src_port,	unsigned short int dst_port, unsigned char *payload, int timeout);
