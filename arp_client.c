#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <timerms.h>
#include "arp/arp.h"

/*
 * By:
 * Alejandro Martinez Castillo (100277598)
 * Juan Luis Sanz Moreno (100076176)
 */

timerms_t timer;

int main(int argc, char** argv) {

	/* Argument processing and help */
	if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0) {
			printf("\nUso: %s <iface> <type>\n", argv[0]);
			printf("	<iface>		Interfaz de salida de mi PC\n");
			printf("	<IP>		IP por la que preguntas\n");
			printf("\nEjemplo: \"%s eth1 10.0.0.36\"\n", argv[0]);
			exit(-1);
		}
	}
	if (argc != 3) {
		printf("Argumentos: %s <interfaz> <ip>\n", argv[0]);
		printf("Escribe \"%s --help\" para una ayuda mas detallada\n", argv[0]);
		exit(-1);
	}

	/*---------------------------Preparing Stuff------------------------------*/
	/* Opening Ethernet interface */
	eth_iface_t * eth_iface = eth_open(argv[1]);
	if (eth_iface == NULL) {
		exit(-1);
	}
	/* Parsing IP String to IP format */
	ipv4_addr_t target_ip;
	ipv4_str_addr(argv[2], target_ip);
	/* Initialization of MAC where the result will be stored */
	mac_addr_t target_mac;

	/*---------------------------Sending Stuff--------------------------------*/
	printf("Enviando consulta ARP a %s..\n", argv[2]);
	ipv4_addr_t ip_src;
	memset(ip_src, 0x00, IPv4_ADDR_SIZE);
	/*int t;
	 while (1) {
	 for (t = 0; t < 9999999; t++) {
	 int q;
	 for (q = 0; q < 50; q++);
	 }
	 do {*/
	timerms_reset(&timer, 1000);
	if (arp_resolve(eth_iface, target_ip, target_mac, ip_src) == 1) {
		/* Converting MAC and IP to String and printing result */
		char ip_str[20];
		ipv4_addr_str(target_ip, ip_str);
		char macstr[20];
		eth_mac_str(target_mac, macstr);
		printf("Encontrado: %s esta en %s\n", ip_str, macstr);
	} else {
		printf("No se pudo obtener el ARP.\n");
	}
	/*} while (timerms_left(&timer) <= 0);*/
	//}
	return 0;
}

