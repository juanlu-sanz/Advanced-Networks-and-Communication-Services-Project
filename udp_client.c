#include "udp/udp.h"
#include <stdlib.h>
#include <string.h>
#define INFINITE_TIMEOUT -1
#define UDP_MAX_SIZE 1500-20-4

int help(int argc, char *argv[]) {
	printf("\nEste programa puede ser usado de dos formas: como cliente o servidor\n");
	printf("\nComo Cliente: %s client <ip> <puerto_dst> <puerto_src> <size>\n", argv[0]);
	printf("	<ip>\t\tIP de destino\n");
	printf("	<puerto_dst>\tPuerto destino UDP\n");
	printf("	<puerto_src>\tPuerto de salida UDP\n");
	printf("	<size>\t\tTamaño a rellenar del payload\n");
	printf("	Ejemplo: \"%s client 10.0.0.78 520 520 42\"\n\n", argv[0]);
	printf("\nComo Servidor: %s server <puerto>\n", argv[0]);
	printf("	<puerto>\t\tPuerto a escuchar\n");
	printf("	Ejemplo: \"%s server 520\"\n", argv[0]);
	return 0;
}

int client(int argc, char *argv[]) {
	/* Argument processing and help */
	if (argc != 6) {
		printf("Cliente IP: uso: %s [client|server] <ip> <puerto_dst> <puerto_src> <size>\n", argv[0]);
		return 0;
	}
	/* Parsing IP to IP format */
	ipv4_addr_t target_ip;
	ipv4_str_addr(argv[2], target_ip);
	/* Opening configuration and routing table */
	udp_open("ipv4_config.txt", "ipv4_route_table.txt");
	/* Preparing and filling the payload */
	unsigned char payload[UDP_MAX_SIZE];
	int i;
	for (i = 0; i < atoi(argv[5]); i++) {
		payload[i] = (unsigned char) i;
	}
	/* Sending */
	int result = udp_send(target_ip, atoi(argv[3]), atoi(argv[4]), payload, atoi(argv[5]));
	if (result < 0) {
		printf("Error al enviar!");
		return 0;
	}
	printf("Esperando respuesta!\n");
	unsigned char r_buffer[UDP_MAX_SIZE];
	ipv4_addr_t source_ip;
	unsigned short int src_port;
	int bsize = udp_receive(source_ip, &src_port, atoi(argv[4]), r_buffer, -1);
	if (result <= 0) {
		printf("Timeout o error al recibir paquete!\n");
		return 0;
	}
	char ip_recib[50];
	ipv4_addr_str(source_ip, ip_recib);
	printf("-----Recibidos %d bytes de %s.\n", bsize, ip_recib);
	udp_close();
	return 0;
}

int server(int argc, char *argv[]) {

	/* Argument processing and help */
	if (argc != 3) {
		printf("Servidor UDP: uso: %s server <puerto>\n", argv[0]);
		return 0;
	}
	/* Opening configuration and routing table */
	ipv4_open("ipv4_config.txt", "ipv4_route_table.txt");
	/* Preparing buffer */
	unsigned char buffer[IP_MAX_PACKET_SIZE];
	/* Receiving (with infinite timeout) */
	printf("[C_UDP] Iniciando cliente UDP..");
	while (1) {
		ipv4_addr_t source_ip;
		unsigned short int src_port;
		int bsize = udp_receive(source_ip, &src_port, atoi(argv[2]), buffer, INFINITE_TIMEOUT);
		char ip_recib[50];
		ipv4_addr_str(source_ip, ip_recib);
		printf("-----Recibidos %d bytes de %s. Reenviando-----\n", bsize, ip_recib);
		udp_send(source_ip, src_port, atoi(argv[2]), buffer, bsize);
	}
	udp_close();
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
		printf("Opción incorrecta! Escribe \"%s --help\" para mas ayuda!\n", argv[0]);
	} else {
		printf("Modo de uso: %s [client|server] [args].\nUsa %s [client|server] para ver la ayuda de cada modo.\n", argv[0], argv[0]);
		printf("Usa \"%s --help\" para una ayuda mas detallada\n", argv[0]);
	}
	return 0;
}
