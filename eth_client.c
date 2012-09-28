#include "eth_base/eth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/*Defining stuff*/
#define TWO_SECOND_TIMEOUT 2000

int main(int argc, char **argv) {

	/* Argument processing and help */
	if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0) {
			printf("\nUse: <iface> <type> <mac> <long>\n");
			printf("	<iface>		My machine's output interface\n");
			printf("	<tipo>		Packet type (806 for ARP) \n");
			printf("	<mac>		Destination MAC\n");
			printf("	<long>		Payload length\n\n");
			printf("Example: \"eth_client eth1 17 00:11:22:33:44:55 20\"\n");
			exit(-1);
		}
	}
	if (argc != 5) {
		printf("Use: %s <iface> <type> <mac> <long>\n", argv[0]);
		printf("Type \"%s --help\" for a more detailed help\n", argv[0]);
		exit(-1);
	}

	/*-----------------------Preparing Stuff--------------------------*/
	/* Opening Ethernet interface */
	eth_iface_t * eth_iface = NULL;
	eth_iface = eth_open(argv[1]);
	if (eth_iface == NULL) {
		exit(-1);
	}

	/*my MAC*/
	mac_addr_t my_addr;
	eth_getaddr(eth_iface, my_addr);

	/*Destination MAC parsing*/
	mac_addr_t server_addr;
	int parse_err = eth_str_mac(argv[3], server_addr);
	if (parse_err == -1) {
		printf("I'm sorry, the source MAC address was typed incorrectly. %s\n", "Please type it following this pattern: 00:11:22:33:44:55");
	}

	/*Parsing the type to hex*/
	uint16_t eth_type;
	sscanf(argv[2], "%"SCNx16, &eth_type);

	/*Generating the Payload*/
	int payload_len = atoi(argv[4]);
	int i;
	unsigned char * payload = calloc(payload_len, 1);
	for (i = 0; i < payload_len; i++) {
		payload[i] = (unsigned char) i;
	}

	/*-----------------------Sending Stuff----------------------------*/
	/* Sending frame and checking for errors */
	int err = eth_send(eth_iface, server_addr, eth_type, payload, payload_len);
	if (err == -1) {
		exit(-1);
	}

	/*-----------------------Receiving Stuff--------------------------*/
	/* Waiting to receive, receiving and error checking */
	unsigned char buffer[ETH_MTU];
	int len = eth_recv(eth_iface, server_addr, eth_type, buffer, TWO_SECOND_TIMEOUT);
	if (len <= 0) {
		char mac_addr_asStr[MAC_ADDR_STR_LENGTH];
		eth_mac_str(my_addr, mac_addr_asStr);
		fprintf(stderr, "%s: ERROR en eth_recv()\n", mac_addr_asStr);
	} else {
		printf("Recibidos %d bytes del Servidor Ethernet\n", len);
	}

	/*-----------------------Finishing Stuff--------------------------*/
	/* Cerrar interfaz Ethernet */
	eth_close(eth_iface);

	return 0;
}
