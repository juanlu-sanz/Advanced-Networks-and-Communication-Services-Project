#include "../udp/udp.h"
#define RIP_VERSION 2
#define FAMILY_ID 2

#define RIP_REQUEST 1
#define RIP_RESPONSE 2
#define RIP_ENTRY_SIZE 20
#define RIP_HEADER_SIZE 4

/*RIP Entries, there can be from 1 to 25, 20 Bytes*/
typedef struct {
	uint16_t family_id;
	uint16_t route_tag;
	ipv4_addr_t ip;
	ipv4_addr_t mask;
	ipv4_addr_t nexthop;
	uint32_t metric;
} rip_entry;

/*RIP packet format*/
typedef struct {
	unsigned char command;
	unsigned char version;
	uint16_t zero;
	rip_entry rip_entries[25];
} rip_packet;

int size_of_packet_for_rip_entries(int num);
int num_entries_in_packet(int num);
void print_rip_entry(rip_entry *entry);
void print_rip_packet(rip_packet *packet, int size);

