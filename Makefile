GCC = rawnetcc

#si estamos en el lab

ifdef OBJING_LOCAL_PATH
    BIN = /tmp/
    SU = sudo 
  else
    BIN = ./
  endif

BINARIES = $(BIN)arp_client $(BIN)eth_server $(BIN)eth_client

all:	build_eth build_arp build_ip build_udp build_rip

build_eth:
	@echo "-----		COMPILANDO CLIENTE ETH		-----"
	$(GCC) $(BIN)eth_client eth_client.c eth_base/*.c -g -static
	@echo "-----		COMPILANDO SERVIDOR ETH		-----"
	$(GCC) $(BIN)eth_server eth_server.c eth_base/*.c -g -static

build_ip:
	@echo "-----		COMPILANDO CLIENTE IP		-----"
	$(GCC) $(BIN)ip_client ip_client.c eth_base/*.c ipv4_base/*.c ipv4_aux/*.c ipv4/*.c arp/*.c -g -static

build_arp:
	@echo "-----		COMPILANDO CLIENTE ARP		-----"
	$(GCC) $(BIN)arp_client arp_client.c eth_base/*.c arp/*.c ipv4_base/*.c -static

build_udp:
	@echo "-----		COMPILANDO CLIENTE UDP		-----"
	$(GCC) $(BIN)udp_client udp_client.c eth_base/*.c arp/*.c ipv4_base/*.c ipv4/*.c ipv4_aux/*.c udp/*.c -static
build_rip:
	@echo "-----		COMPILANDO CLIENTE RIP		-----"
	$(GCC) $(BIN)ripv2_client ripv2_client.c eth_base/*.c arp/*.c ipv4_base/*.c ipv4/*.c ipv4_aux/*.c udp/*.c rip/*.c -static
	$(GCC) $(BIN)ripv2_server ripv2_server.c eth_base/*.c arp/*.c ipv4_base/*.c ipv4/*.c ipv4_aux/*.c udp/*.c rip/*.c -static
copytovm:
	cp $(BINARIES) ~/.vnuml/simulations/simple-RYSCA/vms/hostX/hostfs/
	cp $(BINARIES) ~/.vnuml/simulations/simple-RYSCA/vms/hostY/hostfs/

clean:
	rm -f $(BIN)eth_client $(BIN)eth_server $(BIN)arp_client
