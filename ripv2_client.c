#include "rip/ripv2.h"
#include "ipv4_base/ipv4.h"
#include <stdlib.h>
#include <string.h>

#define RIP_PORT 520
#define INFINITE_TIMEOUT -1

int main(int argc, char *argv[]) {

	/* Argument processing and help */
	if (argc != 2) {
		printf("Use: %s <ip>\n", argv[0]);
		return -1;
	}
	/*---------------------------Preparing Stuff------------------------------*/
	/*RIP packet declaration*/
	rip_packet rip_request;
	int rip_request_size = size_of_packet_for_rip_entries(1); /*One request*/

	/*RIP packet filling*/
	rip_request.command = RIP_REQUEST;
	rip_request.version = RIP_VERSION;
	rip_request.zero = 0;
	printf("RIP Entry has a size of %d\n", (int)sizeof(rip_entry));
	rip_request.rip_entries[0].metric = htonl(16);
	rip_request.rip_entries[0].family_id = 0x0000;
	print_rip_packet(&rip_request, rip_request_size);
//	memset(&(rip_request.rip_entries[0]), 0, 20);
//	rip_request.rip_entries[0].route_tag=0x2222;
//	memcpy(rip_request.rip_entries[0].ip, ip_to, 4);
//	memcpy(rip_request.rip_entries[0].mask, ip_to, 4);
//	memcpy(rip_request.rip_entries[0].nexthop, ip_to,4);

	/*---------------------------Sending Stuff--------------------------------*/
	/*Preparing UDP frame*/
	ipv4_addr_t ip_to;
	ipv4_str_addr(argv[1], ip_to);

	/*Open tables and whatnot*/
	ipv4_open("ipv4_config.txt", "ipv4_route_table.txt");

	/*SEEEEEEEEND*/
	printf("[RIP] Enviando request RIP..\n");
	udp_send(ip_to, RIP_PORT, 2000, (unsigned char*) &rip_request, rip_request_size);

	/*---------------------------Receiving Stuff------------------------------*/
	/*Prepare a buffer and source IP for incoming UDP frame*/
	unsigned char r_buffer[UDP_MAX_SIZE];
	unsigned short int src_port;
	ipv4_addr_t source_ip;

	/*Receive*/
	int bsize = udp_receive(source_ip, &src_port, 2000, r_buffer, INFINITE_TIMEOUT);

	/*print what we just received (a RIP packet supposedly)*/
	print_rip_packet((rip_packet*) r_buffer, bsize);

	return 0;

}

