#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netinet/in.h>

struct eth_hdr
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
    uint8_t payload[0];
}__attribute__((packed));

struct arp_packet
{
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t hw_len;
    uint8_t proto_len;
    uint16_t opcode;
    uint8_t sender_mac[6];
    struct in_addr sender_ip;
    uint8_t target_mac[6];
    struct in_addr target_ip;
}__attribute__((packed));

#define IF_NAME "ens3"
#define MY_IP "158.193.154.177"
#define GW_IP "158.193.154.1"
#define MY_MAC "fa:16:3e:a7:a3:33"

#define ARP_REQUEST (1)
#define ARP_REPLY   (2)

int main()
{
    int sock;
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if(sock == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = if_nametoindex(IF_NAME);
    if(addr.sll_ifindex == 0)
    {
        perror("IF_NAMETOINDEX");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("BIND");
        close(sock);
        exit(EXIT_FAILURE);
    }

    void * buffer;
    size_t buffer_size;
    buffer_size = sizeof(struct eth_hdr)+sizeof(struct arp_packet);
    buffer = calloc(1, buffer_size);

    struct eth_hdr *eth;
    eth = (struct eth_hdr *)buffer;
    memset(eth->dst_mac, 0xff, 6); //broadcast mac

    sscanf(MY_MAC, "%02x:%02x:%02x:%02x:%02x:%02x",
        &eth->src_mac[0],
        &eth->src_mac[1],
        &eth->src_mac[2],
        &eth->src_mac[3],
        &eth->src_mac[4],
        &eth->src_mac[5]
    );

    eth->eth_type = htons(ETH_P_ARP);

    struct arp_packet *arp;
    arp = eth->payload;

    arp->hw_type = htons(1);
    arp->proto_type = htons(ETH_P_IP);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(ARP_REQUEST);
    memcpy(arp->sender_mac, eth->src_mac, 6);

    if(inet_aton(MY_IP, &arp->sender_ip) == 0)
    {
        printf("ERROR: INET_ATON sender_ip\n");
        close(sock);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    //arp->target_mac all 0 - already done


    if(inet_aton(GW_IP, &arp->target_ip) == 0)
    {
        printf("ERROR: INET_ATON target_ip\n");
        close(sock);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    if(send(sock, buffer, buffer_size, 0) == -1)
    {
        perror("SEND");
        close(sock);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    uint8_t my_mac[6];
    sscanf(MY_MAC, "%02x:%02x:%02x:%02x:%02x:%02x",
        &my_mac[0],
        &my_mac[1],
        &my_mac[2],
        &my_mac[3],
        &my_mac[4],
        &my_mac[5]
    );

    for(;;)
    {
        ssize_t recv_len = 0;
        memset(buffer, 0, buffer_size);
        recv_len = recv(sock, buffer, buffer_size, 0);

        // smaller frame than required
        if(recv_len < buffer_size)
            continue;
        // dst mac is not mine
        if(memcmp(my_mac, eth->dst_mac, 6) != 0)
            continue;
        //eth_type is not ARP
        if(eth->eth_type != htons(ETH_P_ARP))
            continue;
        //arp is not reply
        if(arp->opcode != htons(ARP_REPLY))
            continue;
        //target ip is not mine
        if(strcmp(MY_IP, inet_ntoa(arp->target_ip)) != 0)
            continue;
        //sender ip is not what was asked
        if(strcmp(GW_IP, inet_ntoa(arp->sender_ip)) != 0)
            continue;

        //this is arp i was waiting for, print sender MAC
        printf("GW MAC is: %02x:%02x:%02x:%02x:%02x:%02x\n",
            arp->sender_mac[0],
            arp->sender_mac[1],
            arp->sender_mac[2],
            arp->sender_mac[3],
            arp->sender_mac[4],
            arp->sender_mac[5]
        );
        break;
    }

    close(sock);
    free(buffer);
    return EXIT_SUCCESS;
}
