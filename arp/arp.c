#include "arp.h"
#include "../eth_base/eth.h"
#include "../ipv4_base/ipv4.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <timerms.h>

#define ARP_ETH_TYPE 0x806
#define ARP_CACHE_TIMEOUT 30000 //30 seconds
typedef struct {
	//! Format of hardware address.
	uint16_t arp_hard_type;
	//! Format of protocol address.
	uint16_t arp_proto_type;
	//! Length of hardware address.
	uint8_t arp_hard_size;
	//! Length of protocol address.
	uint8_t arp_proto_size;
	//! ARP operation code (command).
	uint16_t arp_op;
	//! Hardware source address.
	mac_addr_t arp_eth_source;
	//! IP source address.
	ipv4_addr_t arp_ip_source;
	//! Hardware destination address.
	mac_addr_t arp_eth_dest;
	//! IP destination address.
	ipv4_addr_t arp_ip_dest;
} arp_packet;

ipv4_addr_t cached_ip_addr;
mac_addr_t cached_mac_addr;
timerms_t arp_timer;

/*ARP cache checking: if it's here, we'll get it*/
int check_arp_cache(ipv4_addr_t known_ip_addr, mac_addr_t missing_mac_addr) {
	if (memcmp(known_ip_addr, cached_ip_addr, IPv4_ADDR_SIZE) == 0) {
		/*We have a match!*/
		memcpy(missing_mac_addr, cached_mac_addr, MAC_ADDR_SIZE);
		return 1;
	}
	return 0;
}

int arp_resolve(eth_iface_t * iface, ipv4_addr_t ip_addr, mac_addr_t mac_addr, ipv4_addr_t src_addr) {

	/*---------------------------Is it in cache?------------------------------*/

	if (timerms_left(&arp_timer) > 0) {
		if (check_arp_cache(ip_addr, mac_addr) == 1) {
			char ip_str[20];
			ipv4_addr_str(ip_addr, ip_str);
			char macstr[20];
			eth_mac_str(mac_addr, macstr);
			printf("[ARP] IP %s obtenida de la cache ARP en la MAC %s\n", ip_str, macstr);
			return 1;
		}
	}

	/*---------------------------Preparing Stuff------------------------------*/
	mac_addr_t my_mac;
	eth_getaddr(iface, my_mac);

	arp_packet arp_t;
	arp_t.arp_hard_type = htons(1); //Ethernet
	arp_t.arp_proto_type = htons(0x800); //ipv4

	arp_t.arp_hard_size = 6; //Ethernet	-> 6 bytes
	arp_t.arp_proto_size = 4; //IPv4 -> 4 bytes
	memset(arp_t.arp_eth_dest, 0, MAC_ADDR_SIZE);

	arp_t.arp_op = htons(1); //request -> 1. reply -> 2
	memcpy(&(arp_t.arp_eth_source), my_mac, MAC_ADDR_SIZE);

	memcpy(&(arp_t.arp_ip_source), src_addr, IPv4_ADDR_SIZE);
	memcpy(&(arp_t.arp_ip_dest), ip_addr, 4);

	/*----------------------Sending & Receiving Stuff-------------------------*/
	int result;
	/*Send first ARP Request*/
	result = eth_send(iface, BROADCAST_MAC_ADDR, ARP_ETH_TYPE, (unsigned char*) &arp_t, sizeof(arp_packet));
	if (result == -1) {
		printf("[ARP] Error enviando paquete ARP.");
		return (-1);
	}
	char ip_str[20];
	ipv4_addr_str(ip_addr, ip_str);
	printf("[ARP] Enviando paquete para buscar IP %s\n", ip_str);
	while (1) {
		mac_addr_t src_addr;
		unsigned char *buffer;
		buffer = calloc(ETH_MTU, 1);

		/*We wait for 2 seconds, -1 --> error, 0 --> We received nothing*/
		int payload_len = eth_recv(iface, src_addr, ARP_ETH_TYPE, buffer, 2000);
		if (payload_len == -1) {
			printf("[ARP] Error\n");
			return -1;
		}
		if (payload_len == 0) {
			printf("[ARP] Timeout 1: 2 segundos\n");
			/*We send another identical ARP Request. Wait for 3 seconds receiving*/
			result = eth_send(iface, BROADCAST_MAC_ADDR, ARP_ETH_TYPE, (unsigned char*) &arp_t, sizeof(arp_packet));
			int payload_len = eth_recv(iface, src_addr, ARP_ETH_TYPE, buffer, 3000);
			if (payload_len == -1) {
				printf("[ARP] Error\n");
				return -1;
			}
			if (payload_len == 0) {
				printf("[ARP] Timeout 2: 5 segundos. Retornando error\n");
				return -1;
			}
			/*We got it on the second try, get eth payload (ARP)*/
			arp_packet *received_arp = (arp_packet *) buffer;
			memcpy(mac_addr, received_arp->arp_eth_source, MAC_ADDR_SIZE); //Get the MAC

			/*Write everything (IP and MAC) on the cache for later use*/
			memcpy(cached_mac_addr, received_arp->arp_eth_source, MAC_ADDR_SIZE);
			memcpy(cached_ip_addr, ip_addr, 4);
			timerms_reset(&arp_timer, ARP_CACHE_TIMEOUT);
			break;
		} else {
			/*We got it on the first try, get eth payload (ARP)*/
			arp_packet *received_arp = (arp_packet *) buffer;
			memcpy(mac_addr, received_arp->arp_eth_source, MAC_ADDR_SIZE); //Get the MAC

			/*Write everything (IP and MAC) on the cache for later use*/
			memcpy(cached_mac_addr, received_arp->arp_eth_source, MAC_ADDR_SIZE);
			memcpy(cached_ip_addr, ip_addr, 4);
			timerms_reset(&arp_timer, ARP_CACHE_TIMEOUT);
			return 1;
		}
		return 1;
	}
	return 1;
}
