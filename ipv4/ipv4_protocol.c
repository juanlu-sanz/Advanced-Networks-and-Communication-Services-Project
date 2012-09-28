#include "../ipv4_base/ipv4.h"
#include "../ipv4_aux/ipv4_config.h"
#include "../ipv4_aux/ipv4_route_table.h"
#include "../eth_base/eth.h"
#include "../arp/arp.h"
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <timerms.h>
#define IP_VERSION 4
#define IPV4_HEADER_SIZE 20
#define IP_ETH_TYPE 0x0800

/*IP packet declaration*/
struct ip_packet {
	uint8_t ip_hl_and_v; //Version (4b) and Header Length (4b) as one byte
	uint8_t ip_tos; //Differentiated Services (1B)
	uint16_t ip_len; //Datagram size (Header and payload) (1B)
	uint16_t ip_id; //ID
	uint16_t ip_off; //Offset
	uint8_t ip_ttl; //Time to live
	uint8_t ip_p; //Transport layer protocol
	uint16_t ip_sum; //Checksum
	ipv4_addr_t ip_src; //4B Source IP address
	ipv4_addr_t ip_dst; //4B Destination IP address
	unsigned char payload[ETH_MTU - IPV4_HEADER_SIZE]; //Payload
} ip_packet_s;

typedef struct ip_packet ipv4_packet;

/*Declaring*/
eth_iface_t *iface;
ipv4_route_table_t *table;
ipv4_addr_t addr_src;
ipv4_addr_t addr_src_mask;

int mac_lookup(ipv4_addr_t addr_dst, mac_addr_t mac);

ipv4_route_table_t *ipv4_get_table() {
	return table;
}

void ipv4_open(char *config_file, char *route_table) {
	//leemos la tabla de rutas
	table = ipv4_route_table_create();
	ipv4_route_table_read(route_table, table);
	char ifname[IFACE_NAME_LENGTH];
	ipv4_config_read(config_file, ifname, addr_src, addr_src_mask);
	iface = eth_open(ifname);
}

/*We free the route table and close the interface*/
void ipv4_close() {
	ipv4_route_table_free(table);
	eth_close(iface);
}

int ipv4_send(ipv4_addr_t addr_dst, int protocol, unsigned char *payload, int payload_len) {

	ipv4_packet packet;
	packet.ip_hl_and_v = 0b01000101; //Version (4 -> 0100) y Header Len (5 Bytes), combinados
	packet.ip_tos = 0; //Type of Service controls the priority of the packet. -> Normal: 0
	packet.ip_len = htons(IPV4_HEADER_SIZE + payload_len); //Longitud del datagrama
	packet.ip_id = htons(7598); //ID del paquete, para fragmentación
	packet.ip_off = 0; //Offset para reassembly tras fragmentación
	packet.ip_ttl = 64; //TTL
	packet.ip_p = protocol; //protocolo de la Transport Layer
	packet.ip_sum = 0;
	memcpy(&(packet.ip_src), addr_src, IPv4_ADDR_SIZE);
	memcpy(&(packet.ip_dst), addr_dst, IPv4_ADDR_SIZE);

	int checksum = ipv4_checksum((unsigned char*) &packet, IPV4_HEADER_SIZE);
	//int checksum = 42;	
	packet.ip_sum = htons(checksum);

	memcpy(&(packet.payload), payload, payload_len);

	mac_addr_t mac_dst;

	int result = mac_lookup(addr_dst, mac_dst);
	if (result == -1) {
		printf("[IP] Error buscando MAC a la que enviar - esta el host caido?\n");
		return -1;
	}

	int err = eth_send(iface, mac_dst, IP_ETH_TYPE, (unsigned char *) &packet, IPV4_HEADER_SIZE + payload_len);
	if (err == -1) {
		printf("[IP] Error al enviar paquete via Ethernet.");
		return (-1);
	}

	return 1;
}

int mac_lookup(ipv4_addr_t addr_dst, mac_addr_t mac) {
	ipv4_addr_t addr_dst_real;
	ipv4_addr_t ip_zero;
	if (addr_dst[0] >= 224 && addr_dst[0] <= 239) {
		/* El paquete es multicast */
		mac_addr_t multi_mac = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x00 };
		memcpy(mac, multi_mac, MAC_ADDR_SIZE);
		mac[3] = addr_dst[1];
		mac[4] = addr_dst[2];
		mac[5] = addr_dst[3];

		mac[2] = 0x5e + (addr_dst[0] & 0b1111);
		printf("[IP] La IP es MULTICAST.\n");
		return 1;
	};

	if ((addr_dst[0] == 255 && addr_dst[1] == 255 && addr_dst[2] == 255 && addr_dst[3] == 255)) {
		printf("[IP] La IP es BROADCAST.\n");
		memset(mac, 0xff, MAC_ADDR_SIZE);
		return 1;
	}

	memset(ip_zero, 0, 4);
	memset(addr_dst_real, 0, 4);
	if (*((int *) addr_dst) == *((int *) addr_src)) {
		printf("[IP] Target IP = Source IP -> el paquete va a nuestra MAC.\n");
		eth_getaddr(iface, mac);
	} else {
		ipv4_route_t *target_route = ipv4_route_table_lookup(table, addr_dst);
		if (*((int *) target_route->next_hop_addr) == *((int *) ip_zero)) {
			memcpy(addr_dst_real, addr_dst, IPv4_ADDR_SIZE);
			printf("[IP] Target IP: En nuestra mascara de red -> Le enviamos directamente\n");
		} else {
			memcpy(addr_dst_real, target_route->next_hop_addr, IPv4_ADDR_SIZE);
		}
		int error = arp_resolve(iface, addr_dst_real, mac, addr_src);

		if (error == -1) {
			printf("[IP] ARP no puede resolver la IP de la IP next-hop.\n");
			return -1;
		}

	}
	printf("[IP] MAC encontrada!\n");
	return 1;
}

int ipv4_is_multicast_or_broadcast(ipv4_addr_t ip) {
	if (ip[0] >= 224 && ip[0] <= 239)
		return 1;
	if (ip[0] == 255 && ip[1] == 255 && ip[2] == 255 && ip[3] == 255)
		return 1;
	return 0;
}

int ipv4_receive(ipv4_addr_t src, unsigned char *buffer, uint8_t protocol, int timeout) {
	timerms_t timer;
	timerms_reset(&timer, timeout);
	unsigned char eth_buffer[ETH_MTU];
	mac_addr_t listen_mac;
	int is_my_ip, is_my_type, payload_len, is_multicast;
	ipv4_packet packet;
	do {
		long int time_left = timerms_left(&timer);
		payload_len = eth_recv(iface, listen_mac, IP_ETH_TYPE, eth_buffer, time_left);
//		printf("[IP] Recibiendo!\n");
		if (payload_len == -1) {
			printf("[IP] Error al recibir paquete!");
			return -1;
		}
		if (payload_len == 0) {
			printf("[IP] Timeout!");
			return 0;
		}

		memcpy(&packet, eth_buffer, payload_len);
		uint16_t checksum = ntohs(packet.ip_sum);
		packet.ip_sum = 0;
		uint16_t new_checksum = ipv4_checksum((unsigned char *) &packet, IPV4_HEADER_SIZE);

		if (new_checksum != checksum) {
			printf("[IP] Recibido paquete con Checksum incorrecto! (%d!=%d)", checksum, new_checksum);
			continue;
		}

		is_my_ip = (memcmp(packet.ip_dst, addr_src, IPv4_ADDR_SIZE) == 0);
		is_my_type = (packet.ip_p == protocol);
		is_multicast = ipv4_is_multicast_or_broadcast(packet.ip_dst);
		//printf("[IP] Is_my_ip: %d, is_my_type: %d (%d != %d)\n", is_my_ip, is_my_type, packet.ip_p, protocol);
	} while (!((is_my_ip || is_multicast) && is_my_type));

	memcpy(buffer, packet.payload, payload_len - IPV4_HEADER_SIZE);
	memcpy(src, packet.ip_src, IPv4_ADDR_SIZE);
	return ntohs(packet.ip_len) - IPV4_HEADER_SIZE;
}

