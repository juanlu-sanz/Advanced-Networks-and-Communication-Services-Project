#include "rip/ripv2.h"
#include "ipv4_base/ipv4.h"
#include <stdlib.h>
#include <string.h>
#include "rip/ripv2_route_table.h"
#include <time.h>

#define RIP_PORT 520
#define INFINITE_TIMEOUT -1
#define RIP_TIMEOUT 180
ripv2_route_table_t *rip_table;

rip_entry rip_get_entry_from_ip_route(ipv4_route_t *ip_route) {
	rip_entry entry;
	entry.family_id = htons(2);
	entry.route_tag = 0;
	memcpy(entry.ip, ip_route->subnet_addr, 4);
	memcpy(entry.mask, ip_route->subnet_mask, 4);
	memset(entry.nexthop, 0, 4);
	entry.metric = 0;
	return entry;
}

rip_entry rip_get_entry_from_rip_route(ripv2_route_t *rip_route) {
	rip_entry entry;
	entry.family_id = htons(2);
	entry.route_tag = 0;
	memcpy(entry.ip, rip_route->subnet_addr, 4);
	memcpy(entry.mask, rip_route->subnet_mask, 4);
	memset(entry.nexthop, 0, 4);
	entry.metric = htonl(rip_route->metric);
	return entry;
}

void cleanup_table() {
	int i = 0;
	int time_passed;
	ripv2_route_t *current_route;
	for (i = 0; i < RIPv2_ROUTE_TABLE_SIZE; i++) {
		current_route = ripv2_route_table_get(rip_table, i);
		if (current_route != NULL) {
			time_passed = timerms_left(&current_route->timer);
			if (time_passed == 0) { //Timer ha expirado. Aquí iria GARBAGE COLLECTOR
				printf("Eliminando ruta que ha expirado: ");
				rip_entry rip = rip_get_entry_from_rip_route(current_route);
				print_rip_entry(&rip);
				free(ripv2_route_table_remove(rip_table, i));
			} //Timer ha expirado
		}
	}
}

int send_response_with_table(ipv4_addr_t dst_ip) {
	/*We prepare the RIP packet to be sent back*/
	rip_packet response_packet;
	response_packet.command = RIP_RESPONSE;
	response_packet.version = RIP_VERSION;
	response_packet.zero = 0;

	/*Get the entries*/
	int c_route = 0;
	int c_route_rip = 0;
	for (c_route = 0; c_route < RIPv2_ROUTE_TABLE_SIZE; c_route++) {
		if (ripv2_route_table_get(rip_table, c_route) != NULL) {
			ripv2_route_t *cur_route = (ripv2_route_table_get(rip_table, c_route));
			if (cur_route->metric != 16) {
				rip_entry cur_entry = rip_get_entry_from_rip_route(cur_route);
				memcpy(&(response_packet.rip_entries[c_route_rip]), &cur_entry, 20);
				c_route_rip++;
			}
		}
	}

	int bsize = size_of_packet_for_rip_entries(c_route_rip);
	return udp_send(dst_ip, RIP_PORT, RIP_PORT, (unsigned char*) &response_packet, bsize);
}

int send_all_request(ipv4_addr_t dst_ip) {
	/*We prepare the RIP packet to be sent back*/
	rip_packet response_packet;
	response_packet.command = RIP_REQUEST;
	response_packet.version = RIP_VERSION;
	response_packet.zero = 0;
	response_packet.rip_entries[0].metric = htonl(16);
	response_packet.rip_entries[0].family_id = 0x0000;
	int bsize = size_of_packet_for_rip_entries(1);

	return udp_send(dst_ip, RIP_PORT, RIP_PORT, (unsigned char*) &response_packet, bsize);
}

void reset_update_timer(timerms_t *update_timer) {
	srand(time(NULL));
	int time = 30;
	if (rand() % 2 == 1) {
		time = time - (rand() % 15);
	} else {
		time = time + (rand() % 15);
	}
	timerms_reset(update_timer, time * 1000);
}

int main(int argc, char *argv[]) {
	/*---------------------------Preparing Stuff------------------------------*/
	udp_open("ipv4_config.txt", "ipv4_route_table.txt"); /*Abrimos tablas IP*/
	rip_table = ripv2_route_table_create(); /*Creamos una tablar RIP nueva*/
	printf("[RIP] Iniciando servidor RIP..\n");
	printf("---------- Rutas IP: ----------\n");
	ipv4_route_table_t *table = ipv4_get_table();
	ipv4_route_table_print(table);

	/*Dumping IPv4 tables as static RIP entries with infinite timer*/
	printf("[RIP] Volcando rutas IP a rutas RIP..\n");
	int c_route;
	for (c_route = 0; c_route < IPv4_ROUTE_TABLE_SIZE; c_route++) {
		if (ipv4_route_table_get(table, c_route) != NULL) {
			ipv4_route_t *ip_cur_route = ipv4_route_table_get(table, c_route);
			timerms_t timer;
			timerms_reset(&timer, INFINITE_TIMEOUT); /*Porque son estaticas, estas no se tocan*/
			ipv4_addr_t default_mask = { 0, 0, 0, 0 };
			if (memcmp(default_mask, ip_cur_route->subnet_addr, IPv4_ADDR_SIZE) != 0) {
				ripv2_route_table_add(
						rip_table,
						ripv2_route_create(ip_cur_route->subnet_addr, ip_cur_route->subnet_mask, ip_cur_route->iface, ip_cur_route->next_hop_addr, 0,
								timer));
			}
		}
	}

	ripv2_route_table_print(rip_table); /*We print what we have (only static so far)*/
	printf("[RIP] Solicitando a todos las rutas.");
	ipv4_addr_t multicast_ip = { 224, 0, 0, 9 };
	send_all_request(multicast_ip);
	/*---------------------------Receiving Stuff------------------------------*/
	timerms_t update_timer;
	reset_update_timer(&update_timer);
	while (1) {
		if (timerms_left(&update_timer) == 0) {
			printf("[RIP] [TIMER] Enviando update periódico! ");
			send_response_with_table(multicast_ip);
			reset_update_timer(&update_timer);
			printf("El siguiente sera en %lds\n", timerms_left(&update_timer));
		}

		/*Prepare buffer for receiving*/
		unsigned char r_buffer[UDP_MAX_SIZE];
		unsigned short int src_port;
		ipv4_addr_t source_ip;

		/*Actual receiving*/
		int bsize = udp_receive(source_ip, &src_port, RIP_PORT, r_buffer, 5 * 1000);
		printf("Recibido paquete de tamaño: %d.\n", bsize);
		cleanup_table();
		if (bsize != 0) {
			/*Dump UDP payload (a RIP packet)*/
			rip_packet received_packet;
			memcpy(&received_packet, r_buffer, bsize);
			//int num_entries = num_entries_in_packet(bsize);
			print_rip_packet(&received_packet, bsize);

			/*Request or Response?*/
			if (received_packet.command == RIP_REQUEST) {
				//Handle request.
				send_response_with_table(source_ip);

			} else {
				int changed = 0;
				/*Response*/
				/*Save all necessary, for that...*/
				/*Iterate all entries*/
				int number_of_entries = num_entries_in_packet(bsize);
				printf("[DEBUG] This Response has %d entries\n", number_of_entries);
				int current_entry = 0;
				for (current_entry = 0; current_entry < number_of_entries; current_entry++) {
					int idx = ripv2_route_table_find(rip_table, received_packet.rip_entries[current_entry].ip,
							received_packet.rip_entries[current_entry].mask);
					if (idx == -1) {
						changed = 1;
						//Añadimos ya que no está
						timerms_t new_route_timer;
						timerms_reset(&new_route_timer, RIP_TIMEOUT * 1000);
						ripv2_route_table_add(
								rip_table,
								ripv2_route_create(received_packet.rip_entries[current_entry].ip, received_packet.rip_entries[current_entry].mask, "",
										source_ip, ntohl(received_packet.rip_entries[current_entry].metric) + 1, new_route_timer));
						printf("Añadida ruta nueva:");
						print_rip_entry(&received_packet.rip_entries[current_entry]);

					} else {
						ripv2_route_t *route = ripv2_route_table_get(rip_table, idx);
						/*Is it from it's father?*/
						if (memcmp(source_ip, route->next_hop_addr, IPv4_ADDR_SIZE) == 0) {
							changed = 1;
							printf("viene de mi padre, reemplazando...:\n");
							print_rip_entry(&received_packet.rip_entries[current_entry]);
							free(ripv2_route_table_remove(rip_table, idx));
							timerms_t new_route_timer;
							timerms_reset(&new_route_timer, RIP_TIMEOUT * 1000);
							int current_metric = ntohl(received_packet.rip_entries[current_entry].metric);
							if (current_metric != 16){
								current_metric++;
							}
							ripv2_route_table_add(
									rip_table,
									ripv2_route_create(received_packet.rip_entries[current_entry].ip, received_packet.rip_entries[current_entry].mask,
											"", source_ip, (current_metric), new_route_timer));
							break;
						}

						if (ntohl(received_packet.rip_entries[current_entry].metric) + 1 < route->metric) {
							changed = 1;
							printf("Ruta con menor métrica, reemplazando...:\n");
							print_rip_entry(&received_packet.rip_entries[current_entry]);
							free(ripv2_route_table_remove(rip_table, idx));
							timerms_t new_route_timer;
							timerms_reset(&new_route_timer, RIP_TIMEOUT * 1000);
							ripv2_route_table_add(
									rip_table,
									ripv2_route_create(received_packet.rip_entries[current_entry].ip, received_packet.rip_entries[current_entry].mask,
											"", source_ip, ntohl(received_packet.rip_entries[current_entry].metric) + 1, new_route_timer));
						} else {
							if (memcmp(&source_ip, &(route->next_hop_addr), IPv4_ADDR_SIZE) == 0) {
								timerms_reset(&(route->timer), RIP_TIMEOUT * 1000);
								printf("Reiniciando timer de ruta: ");
								print_rip_entry(&received_packet.rip_entries[current_entry]);
							}
						}
					}
				}

				if (changed != 0)
					send_response_with_table(multicast_ip);
			}
			printf("----------------- Nueva tabla tras update: -------------------------\n");
			ripv2_route_table_print(rip_table);
		}
	}
}
