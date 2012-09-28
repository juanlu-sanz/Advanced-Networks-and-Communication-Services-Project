#include "../udp/udp.h"
#include <stdlib.h>
#include <string.h>
#include "../ipv4_base/ipv4.h"
#include "ripv2.h"

void print_rip_entry(rip_entry *entry) {
	int family_id = ntohs(entry->family_id);
	int route_tag = ntohl(entry->route_tag);
	char ip_str[20];
	char msk_str[20];
	char hop_str[20];
	ipv4_addr_str(entry->ip, ip_str);
	ipv4_addr_str(entry->mask, msk_str);
	ipv4_addr_str(entry->nexthop, hop_str);

	printf("Familia: %d\tTag: %d\tIP: %s\tMask: %s\tHop: %s\tMetric: %d\n", family_id, route_tag, ip_str, msk_str, hop_str, ntohl(entry->metric));
}

void print_rip_packet(rip_packet *packet, int size) {
	size = size - 4; /*Header off*/
	char *type_as_string;
	if (packet->command == 1) {
		type_as_string = "Request";
	} else {
		type_as_string = "Response";
	}
	printf("------------ RIPv%d %s -----------\n", packet->version, type_as_string);
	int c_entry = 0;
	while (size != 0) {
		print_rip_entry(&(packet->rip_entries[c_entry]));
		size = size - RIP_ENTRY_SIZE;
		c_entry++;
	}
}

int size_of_packet_for_rip_entries(int number_of_rip_entries) {
	return RIP_HEADER_SIZE + (RIP_ENTRY_SIZE * number_of_rip_entries);
}

int num_entries_in_packet(int total_size) {
	return (total_size - RIP_HEADER_SIZE) / RIP_ENTRY_SIZE;
}



