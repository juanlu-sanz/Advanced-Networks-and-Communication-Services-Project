#include "eth_base/eth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/*Defining stuff*/
#define INFINITE_TIMEOUT -1

int main(int argc, char **argv) {

	/* Argument processing and help */
	if (argc == 2) {
		if (strcmp(argv[1], "--help") == 0) {
			printf("\nUse: %s <iface> <type>\n", argv[0]);
			printf("	<iface>		My machine's output interface\n");
			printf("	<tipo>		Packet type (806 for ARP) \n");
			printf("\nExample: \"%s eth1 17\"\n", argv[0]);
			exit(-1);
		}
	}
	if (argc != 3) {
		printf("Use: %s <iface> <type>\n", argv[0]);
		printf("Type %s --help for a more detailed help\n", argv[0]);
		exit(-1);
	}

	/*-----------------------Preparing Stuff--------------------------*/
	/* Open the Ethernet Interface */
	eth_iface_t * eth_iface = eth_open(argv[1]);
	if (eth_iface == NULL) {
		exit(-1);
	}

	/* Parsing packet type */
	uint16_t eth_type;
	sscanf(argv[2], "%"SCNx16, &eth_type);

	/* Prepare the buffer */
	//unsigned char buffer[ETH_MTU];
	/* Initialize Source MAC Address*/
	mac_addr_t src_addr;

	while (1) {
		/*---------------------Sending Stuff--------------------------*/
		/* Recibir trama Ethernet del Cliente */
		printf("Listening for Ethernet frames (type=0x%04x) ...\n", eth_type);
		unsigned char buffer[ETH_MTU];
		int payload_len = eth_recv(eth_iface, src_addr, eth_type, buffer, INFINITE_TIMEOUT);
		if (payload_len == -1) {
			break;
		}
		/*---------------------Receiving Stuff------------------------*/
		/* Enviar la misma trama Ethernet de vuelta al Cliente */
		int len = eth_send(eth_iface, src_addr, eth_type, buffer, payload_len);
		if (len == -1) {
			break;
		}
		printf("Sending %d bytes back to the Ethernet client:\n", payload_len);
	}
	/*-----------------------Finishing Stuff--------------------------*/
	eth_close(eth_iface);
	return 0;
}

