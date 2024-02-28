#include "avs.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct eth_hdr 
{
    uint8_t dst_mac[6];    
    uint8_t src_mac[6];
    uint16_t ethertype;
    char payload[0]; 
}__attribute__((packed)); 

struct arp_hdr
{
   uint16_t hw_type;
   uint16_t proto_type;
   uint8_t hw_len;
   uint8_t proto_len;
   uint16_t opcode;
   uint8_t sender_eth[6];
   struct in_addr sender_ip; 
   uint8_t target_eth[6];
   struct in_addr target_ip; 
}__attribute__((packed));