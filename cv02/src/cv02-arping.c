#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <netinet/in.h>
#include <linux/if_arp.h>

#define IF_NAME "ens3"
#define IF_MAC "fa:16:3e:e8:30:07"
#define IF_IP "158.193.154.203"

#define TARGET_IP "158.193.154.1"

// arp && eth.src == fa:16:3e:e8:30:07

struct Eth_frame
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
    char payload[0];
}__attribute__((packed));

struct Arp
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
    bzero(&addr, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = if_nametoindex(IF_NAME);
    if(sock == 0)
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

    uint8_t *buffer;
    size_t buffer_len;
    buffer_len = sizeof(struct Eth_frame) + sizeof(struct Arp);
    buffer = calloc(1, buffer_len);
    
    struct Eth_frame *frame;
    frame = (struct Eth_frame *) buffer;
    memset(frame->dst_mac, 0xff, 6);

    sscanf(IF_MAC, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &frame->src_mac[0],
        &frame->src_mac[1],
        &frame->src_mac[2],
        &frame->src_mac[3],
        &frame->src_mac[4],
        &frame->src_mac[5]  
    );

    frame->eth_type = htons(ETH_P_ARP);


    struct Arp *arp;
    arp = (struct Arp *) frame->payload;
    arp->hw_type = htons(1);
    arp->proto_type = htons(ETH_P_IP);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(ARPOP_REQUEST);
    memcpy(arp->sender_mac, frame->src_mac, 6);
    if(inet_aton(IF_IP, &arp->sender_ip) == 0)
    {
        printf("ERROR: INET_ATON sender\n");
        close(sock);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    //target mac nepoznam - necham same nuly
    if(inet_aton(TARGET_IP, &arp->target_ip) == 0)
    {
        printf("ERROR: INET_ATON target\n");
        close(sock);
        free(buffer);
        exit(EXIT_FAILURE);
    }

    send(sock, buffer, buffer_len, 0);

    for(;;)
    {
        bzero(buffer, buffer_len);
        if(recv(sock, buffer, buffer_len, 0) != buffer_len)
            continue;
        
        if(frame->eth_type != htons(ETH_P_ARP))
            continue;

        if(arp->opcode != htons(ARPOP_REPLY))
            continue;

        if(strcmp(inet_ntoa(arp->sender_ip), TARGET_IP) != 0)
            continue;

        if(strcmp(inet_ntoa(arp->target_ip), IF_IP) != 0)
            continue;
        
        printf("MAC: %hhx:%hhx:%hhx:%hhx:%hhx:%hhx\n",
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