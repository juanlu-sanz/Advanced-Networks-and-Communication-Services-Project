#include "ipv4_base/ipv4.h"
#include <stdlib.h>
#include <string.h>
#include "ipv4/ipv4_protocol.h"

#define INFINITE_TIMEOUT -1

int help(int argc, char *argv[]) {
	printf("\nEste programa puede ser usado de dos formas: como cliente o servidor\n");
	printf("\nComo Cliente: %s client <ip> <tipo> <size>\n", argv[0]);
	printf("	<ip>\t\tIP de destino\n");
	printf("	<tipo>\t\tTipo de paquete\n");
	printf("	<size>\t\tTamaño a rellenar del payload\n");
	printf("	Ejemplo: \"%s client 10.0.0.78 2 42\"\n\n", argv[0]);
	printf("\nComo Servidor: %s server <tipo>\n", argv[0]);
	printf("	<tipo>\t\tTipo de paquete\n");
	printf("	Ejemplo: \"%s server 2\"\n", argv[0]);
	return 0;
}

int client(int argc, char *argv[]) {
	/* Argument processing and help */
	if (argc != 5) {
		printf("Cliente IP: uso: %s client <ip> <tipo> <size>\n", argv[0]);
		return 0;
	}
	/*---------------------------Preparing Stuff------------------------------*/
	/* Parsing IP to IP format */
	ipv4_addr_t target_ip;
	ipv4_str_addr(argv[2], target_ip);

	/* Opening configuration and routing table */
	ipv4_open("ipv4_config.txt", "ipv4_route_table.txt");

	/* Preparing and filling the payload */
	unsigned char payload[IP_MAX_PACKET_SIZE];
	int i;
	for (i = 0; i < atoi(argv[4]); i++) {
		payload[i] = (unsigned char) i;
	}

	/*---------------------------Sending Stuff--------------------------------*/
	/* Sending */
	int result = ipv4_send(target_ip, atoi(argv[3]), payload, atoi(argv[4]));
	if (result < 0) {
		printf("[IPv4] Error al enviar!");
		return 0;
	}
	printf("Esperando respuesta!\n");
	unsigned char r_buffer[IP_MAX_PACKET_SIZE];
	ipv4_addr_t source_ip;
	int bsize = ipv4_receive(source_ip, r_buffer, atoi(argv[3]), 5000);
	if (result <= 0) {
		printf("[IPv4] Timeout o error al recibir paquete!\n");
		return 0;
	}
	char ip_recib[50];
	ipv4_addr_str(source_ip, ip_recib);
	printf("[IPv4] -----Recibidos %d bytes de %s.\n", bsize, ip_recib);
	return 0;
}

int server(int argc, char *argv[]) {

	/* Argument processing and help */
	if (argc != 3) {
		printf("[IPv4] Servidor IP: uso: %s server <tipo>\n", argv[0]);
		return 0;
	}
	/* Opening configuration and routing table */
	ipv4_open("ipv4_config.txt", "ipv4_route_table.txt");
	/* Preparing buffer */
	unsigned char buffer[IP_MAX_PACKET_SIZE];
	/* Receiving (with infinite timeout) */
	while (1) {
		ipv4_addr_t source_ip;
		int bsize = ipv4_receive(source_ip, buffer, atoi(argv[2]), INFINITE_TIMEOUT);
		char ip_recib[50];
		ipv4_addr_str(source_ip, ip_recib);
		printf("[IPv4] -----Recibidos %d bytes de %s. Reenviando-----\n", bsize, ip_recib);
		ipv4_send(source_ip, atoi(argv[2]), buffer, bsize);
	}
	return 0;
}

int main(int argc, char *argv[]) {
	if (argc > 1) {
		if (strcmp(argv[1], "client") == 0)
			return client(argc, argv);
		if (strcmp(argv[1], "server") == 0)
			return server(argc, argv);
		if (strcmp(argv[1], "--help") == 0)
			return help(argc, argv);
		printf("¡Opcion incorrecta! Escribe \"%s --help\" para mas ayuda!\n", argv[0]);
	} else {
		printf("Modo de uso: %s [client|server] [args].\nUsa %s [client|server] para ver la ayuda de cada modo.\n", argv[0], argv[0]);
		printf("Usa \"%s --help\" para una ayuda mas detallada\n", argv[0]);
	}

	return 0;
}
