#include "../ipv4/ipv4_protocol.h"
#include "../ipv4_base/ipv4.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <timerms.h>

#define UDP_MAX_SIZE 1500-20-4
#define UDP_PACKET_TYPE 17
#define UDP_HEADER_SIZE 8
#define ETH_MTU 1500

struct udp_packet {
	unsigned short int udph_srcport;
	unsigned short int udph_destport;
	unsigned short int udph_len;
	unsigned short int udph_chksum;
	unsigned char data[UDP_MAX_SIZE];
};
typedef struct udp_packet udp_packet;

void udp_open(char *config_file, char *route_table) {
	ipv4_open(config_file, route_table);
}
void udp_close() {
	ipv4_close();
}

/*uint16_t ip_checksum(const void * buf, size_t hdr_len) {
 unsigned long sum = 0;
 const uint16_t *ip1;

 ip1 = buf;
 while (hdr_len > 1) {
 sum += *ip1++;
 if (sum & 0x80000000)
 sum = (sum & 0xFFFF) + (sum >> 16);
 hdr_len -= 2;
 }

 while (sum >> 16)
 sum = (sum & 0xFFFF) + (sum >> 16);

 return (~sum);
 }*/

uint16_t udp_checksum(unsigned char * data, int len, ipv4_addr_t src_addr,
		ipv4_addr_t dest_addr) {
	int i;
	uint16_t word16;
	uint16_t *ip_src = (void *) &src_addr, *ip_dst = (void *) &dest_addr;
	size_t length = len;
	unsigned int sum = 0;

	/* Make 16 bit words out of every two adjacent 8 bit words in the packet
	 * and add them up */
	for (i = 0; i < len; i = i + 2) {
		word16 = ((data[i] << 8) & 0xFF00) + (data[i + 1] & 0x00FF);
		sum = sum + (unsigned int) word16;
	}

	/* Take only 16 bits out of the 32 bit sum and add up the carries */
	while (sum >> 16) {
		sum = (sum & 0xFFFF) + (sum >> 16);
	}
	/*if (len & 1){
	 // Add the padding if the packet lenght is odd          //
	 sum += *((uint8_t *) word16);
	 }*/
	// Add the pseudo-header                                        //
	sum += *(ip_src++);
	sum += *ip_src;

	sum += *(ip_dst++);
	sum += *ip_dst;

	sum += htons(UDP_PACKET_TYPE);
	sum += htons(length);

	/* One's complement the result */
	sum = ~sum;

	return (uint16_t) sum;
}

/*uint16_t udp_checdksum(const void * buff, size_t len, in_addr_t src_addr,
 in_addr_t dest_addr) {
 const uint16_t *buf = buff;
 uint16_t *ip_src = (void *) &src_addr, *ip_dst = (void *) &dest_addr;
 uint32_t sum;
 size_t length = len;

 // Calculate the sum                                            //
 sum = 0;
 while (len > 1) {
 sum += *buf++;
 if (sum & 0x80000000)
 sum = (sum & 0xFFFF) + (sum >> 16);
 len -= 2;
 }

 if (len & 1)
 // Add the padding if the packet lenght is odd          //
 sum += *((uint8_t *) buf);

 // Add the pseudo-header                                        //
 sum += *(ip_src++);
 sum += *ip_src;

 sum += *(ip_dst++);
 sum += *ip_dst;

 sum += htons(IPPROTO_UDP);
 sum += htons(length);

 // Add the carries                                              //
 while (sum >> 16)
 sum = (sum & 0xFFFF) + (sum >> 16);

 // Return the one's complement of sum                           //
 return ((uint16_t)(~sum));
 }*/

int udp_send(ipv4_addr_t addr_dst, unsigned short int dst_port,
		unsigned short int src_port, unsigned char *payload, int payload_len) {
	printf("[UDP] Tamaño %i\n", payload_len);
	udp_packet packet;
	packet.udph_srcport = htons(src_port);
	packet.udph_destport = htons(dst_port);
	packet.udph_len = htons(payload_len + UDP_HEADER_SIZE);

	/*char* ip_str = "10.0.3.10";
	ipv4_addr_t my_ip;
	ipv4_str_addr(ip_str, my_ip);
	packet.udph_chksum = htons(udp_checksum(payload, payload_len, my_ip, addr_dst));*/
	packet.udph_chksum = 0;

	memcpy(&(packet.data), payload, payload_len);
	int err = ipv4_send(addr_dst, UDP_PACKET_TYPE, (unsigned char*) &packet,
			payload_len + UDP_HEADER_SIZE);
	if (err == -1) {
		printf("[UDP] Error en IP al enviar el paquete\n");
		return (-1);
	}
	return 1;
}
int udp_receive(ipv4_addr_t addr_src, unsigned short int *src_port,
		unsigned short int dst_port, unsigned char *payload, int timeout) {
	printf("[UDP] Iniciando recepción de paquete..\n");
	timerms_t timer;
	timerms_reset(&timer, timeout);
	unsigned char ip_buffer[ETH_MTU];
	int is_my_port, payload_len;
	udp_packet packet;
	do {
		long int time_left = timerms_left(&timer);
		payload_len = ipv4_receive(addr_src, ip_buffer, UDP_PACKET_TYPE,
				time_left);
		if (payload_len == -1) {
			printf("[UDP] Error al recibir paquete!\n");
			return -1;
		}
		if (payload_len == 0) {
			printf("[UDP] Timeout!\n");
			return 0;
		}
//		payload_len = payload_len;
		memcpy(&packet, ip_buffer, payload_len);

		is_my_port = (ntohs(packet.udph_destport) == dst_port);
	} while (!is_my_port);
	memcpy(payload, packet.data, ntohs(packet.udph_len) - UDP_HEADER_SIZE);
	*src_port = ntohs(packet.udph_srcport);
	return ntohs(packet.udph_len) - UDP_HEADER_SIZE;
}
;
