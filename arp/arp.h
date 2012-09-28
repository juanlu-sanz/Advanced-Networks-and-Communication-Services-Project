/* 
 * Libreria ARP
 */

#ifndef ARP_H
#define	ARP_H

#include <stdint.h>
#include "../eth_base/eth.h"
#include "../ipv4_base/ipv4.h"
#ifdef	__cplusplus
extern "C" {
#endif

int check_arp_cache(ipv4_addr_t known_ip_addr, mac_addr_t missing_mac_addr);
int arp_resolve(eth_iface_t * iface, ipv4_addr_t ip_addr, mac_addr_t mac_addr, ipv4_addr_t src_addr);

#ifdef	__cplusplus
}
#endif

#endif	/* ARP_H */

