#include "ripv2_route_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <timerms.h>

#define INFINITE_TIMEOUT -1
struct ripv2_route_table {
	ripv2_route_t * routes[RIPv2_ROUTE_TABLE_SIZE];
};


ripv2_route_t * ripv2_route_create(ipv4_addr_t subnet, ipv4_addr_t mask, char* iface, ipv4_addr_t nhop, int metric, timerms_t timer) {
	ripv2_route_t * route = (ripv2_route_t *) malloc(sizeof(struct ripv2_route));

	if (route != NULL) {
		memcpy(route->subnet_addr, subnet, IPv4_ADDR_SIZE);
		memcpy(route->subnet_mask, mask, IPv4_ADDR_SIZE);
		memcpy(route->iface, iface, IFACE_NAME_LENGTH);
		memcpy(route->next_hop_addr, nhop, IPv4_ADDR_SIZE);
		route->metric = metric;
		route->timer = timer;
	}
	return route;
}

/* int ripv2_route_lookup ( ripv2_route_t * route, ipv4_addr_t addr );
 *
 * DESCRIPCIÓN:
 *   Esta función indica si la dirección RIPv2 especificada pertence a la
 *   subred indicada. En ese caso devuelve la longitud de la máscara de la
 *   subred.
 *
 *   Esta función NO está implementada, debe implementarla usted para que
 *   funcione correctamente la función 'ripv2_route_table_lookup()'.
 * 
 * PARÁMETROS:
 *   'route': Ruta a la subred que se quiere comprobar.
 *    'addr': Dirección RIPv2 destino.
 *
 * VALOR DEVUELTO:
 *   Si la dirección RIPv2 pertenece a la subred de la ruta especificada, debe
 *   devolver un número positivo que indica la longitud del prefijo de
 *   subred. Esto es, el número de bits a uno de la máscara de subred.
 *   La función devuelve '-1' si la dirección RIPv2 no pertenece a la subred
 *   apuntada por la ruta especificada.
 */
int ripv2_route_lookup(ripv2_route_t * route, ipv4_addr_t addr) {
	int prefix_length = -1;

	//en route->subnet_mask está la máscara de red
	//ejemplo: IP:            0x101010101101
	//si tenemos una mascara: 0x111111000000
	//y una IP de ruta:       0x101010000000
	//coincide
	/*  01110
	 00100
	 00100
	 */
	int route_ip_masked = *((int *) route->subnet_addr) & *((int *) route->subnet_mask);
	int check_ip_masked = *((int *) addr) & *((int *) route->subnet_mask);

	if (route_ip_masked == check_ip_masked) {
		int check_mask = *(route->subnet_mask);
		int count = 32;
		int i;
		for (i = 0; i < 32; i++) {
			if (!(check_mask & (0b1 << i)))
				count--;
		}
		prefix_length = count;
	}
	/* Debug */
	char dbg1[20];
	char dbg2[20];
	ipv4_addr_str(addr, dbg1);
	ipv4_addr_str(route->subnet_mask, dbg2);

	printf("[IP] [DBG] table_lookup: ip %s, mascara %s -> %d\n", dbg1, dbg2, prefix_length);

	/* Debe implementar este método */
	return prefix_length;
}

/* void ripv2_route_print ( ripv2_route_t * route );
 *
 * DESCRIPCIÓN:
 *   Esta función imprime la ruta especificada por la salida estándar.
 *
 * PARÁMETROS:
 *   'route': Ruta que se desea imprimir.
 */
void ripv2_route_print(ripv2_route_t * route) {
	if (route != NULL) {
		char subnet_str[IPv4_ADDR_STR_LENGTH];
		ipv4_addr_str(route->subnet_addr, subnet_str);
		char mask_str[IPv4_ADDR_STR_LENGTH];
		ipv4_addr_str(route->subnet_mask, mask_str);
		char* iface_str = route->iface;
		char nhop_str[IPv4_ADDR_STR_LENGTH];
		ipv4_addr_str(route->next_hop_addr, nhop_str);

		printf("%s/%s -> %s/%s", subnet_str, mask_str, iface_str, nhop_str);
	}
}

/* void ripv2_route_free ( ripv2_route_t * route );
 *
 * DESCRIPCIÓN:
 *   Esta función libera la memoria reservada para la ruta especificada, que
 *   ha sido creada con 'ripv2_route_create()'.
 *
 * PARÁMETROS:
 *   'route': Ruta que se desea liberar.
 */
void ripv2_route_free(ripv2_route_t * route) {
	if (route != NULL) {
		free(route);
	}
}

/* ripv2_route_table_t * ripv2_route_table_create();
 * 
 * DESCRIPCIÓN: 
 *   Esta función crea una tabla de rutas RIPv2 vacía.
 *
 *   Esta función reserva memoria para la tabla de rutas creada, para
 *   liberarla es necesario llamar a la función 'ripv2_route_table_free()'.
 *
 * VALOR DEVUELTO:
 *   La función devuelve un puntero a la tabla de rutas creada.
 *
 * ERRORES:
 *   La función devuelve 'NULL' si no ha sido posible reservar memoria para
 *   crear la tabla de rutas.
 */
ripv2_route_table_t * ripv2_route_table_create() {
	ripv2_route_table_t * table;

	table = (ripv2_route_table_t *) malloc(sizeof(struct ripv2_route_table));
	if (table != NULL) {
		int i;
		for (i = 0; i < RIPv2_ROUTE_TABLE_SIZE; i++) {
			table->routes[i] = NULL;
		}
	}

	return table;
}

/* int ripv2_route_table_add ( ripv2_route_table_t * table, 
 *                            ripv2_route_t * route );
 * DESCRIPCIÓN: 
 *   Esta función añade la ruta especificada en la primera posición libre de
 *   la tabla de rutas.
 *
 * PARÁMETROS:
 *   'table': Tabla donde añadir la ruta especificada.
 *   'route': Ruta a añadir en la tabla de rutas.
 * 
 * VALOR DEVUELTO:
 *   La función devuelve el indice de la posición [0,RIPv2_ROUTE_TABLE_SIZE-1]
 *   donde se ha añadido la ruta especificada.
 * 
 * ERRORES:
 *   La función devuelve '-1' si no ha sido posible añadir la ruta
 *   especificada.
 */
int ripv2_route_table_add(ripv2_route_table_t * table, ripv2_route_t * route) {
	int route_index = -1;

	if (table != NULL) {
		/* Find an empty place in the route table */
		int i;
		for (i = 0; i < RIPv2_ROUTE_TABLE_SIZE; i++) {
			if (table->routes[i] == NULL) {
				table->routes[i] = route;
				route_index = i;
				break;
			}
		}
	}

	return route_index;
}

/* ripv2_route_t * ripv2_route_table_remove ( ripv2_route_table_t * table, 
 *                                          int index );
 *
 * DESCRIPCIÓN:
 *   Esta función borra la ruta almacenada en la posición de la tabla de rutas
 *   especificada.
 *   
 *   Esta función NO libera la memoria reservada para la ruta borrada. Para
 *   ello es necesario utilizar la función 'ripv2_route_free()' con la ruta
 *   devuelta.
 *
 * PARÁMETROS:
 *   'table': Tabla de rutas de la que se desea borrar una ruta.
 *   'index': Índice de la ruta a borrar. Debe tener un valor comprendido
 *            entre [0, RIPv2_ROUTE_TABLE_SIZE-1].
 * 
 * VALOR DEVUELTO:
 *   Esta función devuelve la ruta que estaba almacenada en la posición
 *   indicada.
 *
 * ERRORES:
 *   Esta función devuelve 'NULL' si la ruta no ha podido ser borrada, o no
 *   existía ninguna ruta en dicha posición.
 */
ripv2_route_t * ripv2_route_table_remove(ripv2_route_table_t * table, int index) {
	ripv2_route_t * removed_route = NULL;

	if ((table != NULL) && (index >= 0) && (index < RIPv2_ROUTE_TABLE_SIZE)) {
		removed_route = table->routes[index];
		table->routes[index] = NULL;
	}

	return removed_route;
}

/* ripv2_route_t * ripv2_route_table_get ( ripv2_route_table_t * table, int index );
 * 
 * DESCRIPCIÓN:
 *   Esta función devuelve la ruta almacenada en la posición de la tabla de
 *   rutas especificada.
 *
 * PARÁMETROS:
 *   'table': Tabla de rutas de la que se desea obtener una ruta.
 *   'index': Índice de la ruta consultada. Debe tener un valor comprendido
 *            entre [0, RIPv2_ROUTE_TABLE_SIZE-1].
 * 
 * VALOR DEVUELTO:
 *   Esta función devuelve la ruta almacenada en la posición de la tabla de
 *   rutas indicada.
 *
 * ERRORES:
 *   Esta función devuelve 'NULL' si no ha sido posible consultar la tabla de
 *   rutas, o no existe ninguna ruta en dicha posición.
 */
ripv2_route_t * ripv2_route_table_get(ripv2_route_table_t * table, int index) {
	ripv2_route_t * route = NULL;

	if ((table != NULL) && (index >= 0) && (index < RIPv2_ROUTE_TABLE_SIZE)) {
		route = table->routes[index];
	}

	return route;
}

/* int ripv2_route_table_find ( ripv2_route_table_t * table, ipv4_addr_t subnet, 
 *                                                         ipv4_addr_t mask );
 *
 * DESCRIPCIÓN:
 *   Esta función devuelve el índice de la ruta para llegar a la subred
 *   especificada.
 *
 * PARÁMETROS:
 *    'table': Tabla de rutas en la que buscar la subred.
 *   'subnet': Dirección de la subred a buscar.
 *     'mask': Máscara de la subred a buscar.
 * 
 * VALOR DEVUELTO:
 *   Esta función devuelve la posición de la tabla de rutas donde se encuentra
 *   la ruta que apunta a la subred especificada.
 *
 * ERRORES:
 *   La función devuelve '-1' si no se ha encontrado la ruta especificada o
 *   '-2' si no ha sido posible realizar la búsqueda.
 */
int ripv2_route_table_find(ripv2_route_table_t * table, ipv4_addr_t subnet, ipv4_addr_t mask) {
	int route_index = -2;

	if (table != NULL) {
		route_index = -1;
		int i;
		for (i = 0; i < RIPv2_ROUTE_TABLE_SIZE; i++) {
			ripv2_route_t * route_i = table->routes[i];
			if (route_i != NULL) {
				int same_subnet = (memcmp(route_i->subnet_addr, subnet, IPv4_ADDR_SIZE) == 0);
				int same_mask = (memcmp(route_i->subnet_mask, mask, IPv4_ADDR_SIZE) == 0);

				if (same_subnet && same_mask) {
					route_index = i;
					break;
				}
			}
		}
	}

	return route_index;
}

/* ripv2_route_t * ripv2_route_table_lookup ( ripv2_route_table_t * table, 
 *                                          ipv4_addr_t addr );
 * 
 * DESCRIPCIÓN:
 *   Esta función devuelve la mejor ruta almacenada en la tabla de rutas para
 *   alcanzar la dirección RIPv2 destino especificada.
 *
 *   Esta función recorre toda la tabla de rutas buscando rutas que contengan
 *   a la dirección RIPv2 indicada. Para ello emplea la función
 *   'ripv2_route_lookup()'. De todas las rutas posibles se devuelve aquella
 *   con el prefijo más específico, esto es, aquella con la máscara de subred
 *   mayor.
 * 
 * PARÁMETROS:
 *   'table': Tabla de rutas en la que buscar la dirección RIPv2 destino.
 *    'addr': Dirección RIPv2 destino a buscar.
 *
 * VALOR DEVUELTO:
 *   Esta función devuelve la ruta más específica para llegar a la dirección
 *   RIPv2 indicada.
 *
 * ERRORES:
 *   Esta función devuelve 'NULL' si no no existe ninguna ruta para alcanzar
 *   la dirección indicada, o si no ha sido posible realizar la búsqueda.
 */
ripv2_route_t * ripv2_route_table_lookup(ripv2_route_table_t * table, ipv4_addr_t addr) {
	ripv2_route_t * best_route = NULL;
	int best_route_prefix = -1;

	if (table != NULL) {
		int i;
		for (i = 0; i < RIPv2_ROUTE_TABLE_SIZE; i++) {
			ripv2_route_t * route_i = table->routes[i];
			if (route_i != NULL) {
				int route_i_lookup = ripv2_route_lookup(route_i, addr);
				if (route_i_lookup > best_route_prefix) {
					best_route = route_i;
					best_route_prefix = route_i_lookup;
				}
			}
		}
	}

	return best_route;
}

/* void ripv2_route_table_free ( ripv2_route_table_t * table );
 *
 * DESCRIPCIÓN:
 *   Esta función libera la memoria reservada para la tabla de rutas
 *   especificada, incluyendo todas las rutas almacenadas en la misma,
 *   mediante la función 'ripv2_route_free()'.
 *
 * PARÁMETROS:
 *   'table': Tabla de rutas a borrar.
 */
void ripv2_route_table_free(ripv2_route_table_t * table) {
	if (table != NULL) {
		int i;
		for (i = 0; i < RIPv2_ROUTE_TABLE_SIZE; i++) {
			ripv2_route_t * route_i = table->routes[i];
			if (route_i != NULL) {
				table->routes[i] = NULL;
				ripv2_route_free(route_i);
			}
		}
		free(table);
	}
}

/* int ripv2_route_table_read ( char * filename, ripv2_route_table_t * table );
 *
 * DESCRIPCIÓN:
 *   Esta función leer el fichero especificado y añade las rutas RIPv2 leidas
 *   en la tabla de rutas indicada.
 *
 * PARÁMETROS:
 *   'filename': Nombre del fichero con rutas RIPv2 que se desea leer.
 *      'table': Tabla de rutas donde añadir las rutas leidas.
 *
 * VALOR DEVUELTO:
 *   La función devuelve el número de rutas leidas y añadidas en la tabla, o
 *   '0' si no se ha leido ninguna ruta.
 *
 * ERRORES:
 *   La función devuelve '-1' si se ha producido algún error al leer el
 *   fichero de rutas.
 */

int ripv2_route_table_read(char * filename, ripv2_route_table_t * table) {
	int read_routes = 0;

	FILE * routes_file = fopen(filename, "r");
	if (routes_file == NULL) {
		fprintf(stderr, "Error opening input RIPv2 Routes file \"%s\": %s.\n", filename, strerror(errno));
		return -1;
	}

	int linenum = 0;
	char line_buf[1024];
	char subnet_str[256];
	char mask_str[256];
	char iface_name[256];
	char nhop_str[256];
	int err = 0;
	int metric;

	while ((!feof(routes_file)) && (err == 0)) {

		linenum++;

		/* Read next line of file */
		char* line = fgets(line_buf, 1024, routes_file);
		if (line == NULL) {
			break;
		}

		/* If this line is empty or a comment, just ignore it */
		if ((line_buf[0] == '\n') || (line_buf[0] == '#')) {
			err = 0;
			continue;
		}

		/* Parse line: Format "<subnet> <mask> <nhop> <iface>\n" */
		err = sscanf(line, "%s %s %s %s %d\n", subnet_str, mask_str, iface_name, nhop_str, &metric);
		if (err != 5) {
			fprintf(stderr, "%s:%d: Invalid RIPv2 Route format: \"%s\" (%d items)\n", filename, linenum, line, err);
			fprintf(stderr, "%s:%d: Format must be: <subnet> <mask> <iface> <next hop> <metric>\n", filename, linenum);
			err = -1;

		} else {

			/* Parse RIPv2 route subnet address */
			ipv4_addr_t subnet;
			err = ipv4_str_addr(subnet_str, subnet);
			if (err == -1) {
				fprintf(stderr, "%s:%d: Invalid <subnet> value: \"%s\"\n", filename, linenum, subnet_str);
				break;
			}

			/* Parse RIPv2 route subnet mask */
			ipv4_addr_t mask;
			err = ipv4_str_addr(mask_str, mask);
			if (err == -1) {
				fprintf(stderr, "%s:%d: Invalid <mask> value: \"%s\"\n", filename, linenum, mask_str);
				break;
			}

			/* Parse RIPv2 route next hop */
			ipv4_addr_t next_hop;
			err = ipv4_str_addr(nhop_str, next_hop);
			if (err == -1) {
				fprintf(stderr, "%s:%d: Invalid <next hop> value: \"%s\"\n", filename, linenum, nhop_str);
				break;
			}
			timerms_t timer;
			timerms_reset(&timer, INFINITE_TIMEOUT);
			/* Create new route & add it to Route Table */
			ripv2_route_t * new_route = ripv2_route_create(subnet, mask, iface_name, next_hop, metric, timer);

			if (table != NULL) {
				err = ripv2_route_table_add(table, new_route);
				if (err >= 0) {
					err = 0;
					read_routes++;
				}
			}
		}
	} /* while() */

	if (err == -1) {
		read_routes = -1;
	}

	/* Close IP Configuration file */
	fclose(routes_file);

	return read_routes;
}

/* void ripv2_route_table_output ( ripv2_route_table_t * table, FILE * out );
 *
 * DESCRIPCIÓN:
 *   Esta función imprime por la salida indicada la tabla de rutas RIPv2
 *   especificada.
 *
 * PARÁMETROS:
 *        'out': Salida por la que imprimir la tabla de rutas.
 *      'table': Tabla de rutas a imprimir.
 *
 * VALOR DEVUELTO:
 *   La función devuelve el número de rutas impresas por la salida indicada, o
 *   '0' si la tabla de rutas estaba vacia.
 *
 * ERRORES:
 *   La función devuelve '-1' si se ha producido algún error al escribir por
 *   la salida indicada.
 */
int ripv2_route_table_output(FILE * out, ripv2_route_table_t * table) {
	int err = fprintf(out, "# SubnetAddr  \tSubnetMask    \tIface  \tNextHop       \tMetric\tTimer\n");
	if (err < 0) {
		return -1;
	}

	char subnet_str[IPv4_ADDR_STR_LENGTH];
	char mask_str[IPv4_ADDR_STR_LENGTH];
	char* ifname = NULL;
	char nhop_str[IPv4_ADDR_STR_LENGTH];
	int metric;
	int i;
	for (i = 0; i < RIPv2_ROUTE_TABLE_SIZE; i++) {
		ripv2_route_t * route_i = ripv2_route_table_get(table, i);
		if (route_i != NULL) {
			ipv4_addr_str(route_i->subnet_addr, subnet_str);
			ipv4_addr_str(route_i->subnet_mask, mask_str);
			ifname = route_i->iface;
			ipv4_addr_str(route_i->next_hop_addr, nhop_str);
			metric = route_i->metric;
			long int remain = timerms_left(&(route_i->timer));
			err = fprintf(out, "%-15s\t%-15s\t%s\t%s\t%d\t%ld\n", subnet_str, mask_str, ifname, nhop_str, metric, remain);
			if (err < 0) {
				return -1;
			}
		}
	}

	return 0;
}

/* int ripv2_route_table_write ( ripv2_route_table_t * table, char * filename );
 *
 * DESCRIPCIÓN:
 *   Esta función almacena en el fichero especificado la tabla de rutas RIPv2
 *   indicada.
 *
 * PARÁMETROS:
 *      'table': Tabla de rutas a almacenar.
 *   'filename': Nombre del fichero donde se desea almacenar la tabla de
 *               rutas.
 *
 * VALOR DEVUELTO:
 *   La función devuelve el número de rutas almacenadas en el fichero de
 *   rutas, o '0' si la tabla de rutas estaba vacia.
 *
 * ERRORES:
 *   La función devuelve '-1' si se ha producido algún error al escribir el
 *   fichero de rutas.
 */
int ripv2_route_table_write(ripv2_route_table_t * table, char * filename) {
	int num_routes = 0;

	FILE * routes_file = fopen(filename, "w");
	if (routes_file == NULL) {
		fprintf(stderr, "Error opening output RIPv2 Routes file \"%s\": %s.\n", filename, strerror(errno));
		return -1;
	}

	fprintf(routes_file, "# %s\n", filename);
	fprintf(routes_file, "#\n");

	if (table != NULL) {
		num_routes = ripv2_route_table_output(routes_file, table);
		if (num_routes == -1) {
			fprintf(stderr, "Error writing RIPv2 Routes file \"%s\": %s.\n", filename, strerror(errno));
			return -1;
		}
	}

	fclose(routes_file);

	return num_routes;
}

/* void ripv2_route_table_print ( ripv2_route_table_t * table );
 *
 * DESCRIPCIÓN:
 *   Esta función imprime por la salida estándar la tabla de rutas RIPv2
 *   especificada.
 *
 * PARÁMETROS:
 *      'table': Tabla de rutas a imprimir.
 */
void ripv2_route_table_print(ripv2_route_table_t * table) {
	if (table != NULL) {
		ripv2_route_table_output(stdout, table);
	}
}
