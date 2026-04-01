#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <net/if.h>

#define MULTI_IP  "224.0.0.9"
#define PORT      520
#define IFACE     "ens4"
#define RIP_REPLY 2

struct RIP_entry
{
    uint16_t afi;
    uint16_t rt;
    struct in_addr network_addr;
    struct in_addr mask;
    struct in_addr next_hop;
    uint32_t metric;
}__attribute__((packed));

struct RIP_hdr
{
    uint8_t command;
    uint8_t version;
    uint16_t unused;
    struct RIP_entry entry[0];
}__attribute__((packed));


int main()
{
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, MULTI_IP, &addr.sin_addr) <= 0)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    }
    addr.sin_port = htons(PORT);

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("BIND");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct ip_mreqn multistruct;
    memset(&multistruct, 0, sizeof(multistruct));
    if(inet_pton(AF_INET, MULTI_IP, &multistruct.imr_multiaddr) <= 0)
    {
        perror("INET_PTON IP_MREQN");
        close(sock);
        exit(EXIT_FAILURE);
    }
    multistruct.imr_ifindex = if_nametoindex(IFACE);
    if(multistruct.imr_ifindex == 0)
    {
        perror("IF_NAMETOINDEX");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multistruct, sizeof(multistruct)) == -1)
    {
        perror("SETSOCKOPT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int buffer_len = sizeof(struct RIP_hdr) + 25*sizeof(struct RIP_entry);
    uint8_t buffer[buffer_len];
    socklen_t addr_len = sizeof(addr);
    ssize_t recv_len = 0;
    for(;;)
    {
        memset(&addr, 0, sizeof(addr));
        memset(buffer, 0, buffer_len);
        recv_len = recvfrom(sock, buffer, buffer_len, 0, &addr, &addr_len);

        //error while receiving or invalid RIP message
        if(recv_len < (sizeof(struct RIP_hdr)+sizeof(struct RIP_entry)))
            continue;

        //RIP entry is not whole
        if( (recv_len - sizeof(struct RIP_hdr)) % sizeof(struct RIP_entry) != 0)
            continue;

        struct RIP_hdr *hdr = (struct RIP_hdr *)buffer;
        
        //command is not REPLY
        if(hdr->command != RIP_REPLY)
            continue;

        //version is not 2
        if(hdr->version != 2)
            continue;

        printf("RIP from [%s:%d]:\n",
            inet_ntoa(addr.sin_addr),
            ntohs(addr.sin_port)
        );

        //process each RIP entry
        struct RIP_entry *entry;
        char network_addr[16];
        char mask[16];
        char nh[16];
        int rip_entries_count = (recv_len-sizeof(struct RIP_hdr))/sizeof(struct RIP_entry); 
        for(int i=0; i<rip_entries_count; i++)
        {
            entry = (struct RIP_entry *)(hdr->entry+i);

            inet_ntop(AF_INET, &entry->network_addr, network_addr, 16);
            inet_ntop(AF_INET, &entry->mask, mask, 16);
            inet_ntop(AF_INET, &entry->next_hop, nh, 16);
            printf("  IP: %s, MASK: %s, NH: %s, METRIC: %d\n",
                network_addr,
                mask,
                nh,
                ntohl(entry->metric)
            );
        }
        printf("\n\n");
    }

    close(sock);
    return EXIT_SUCCESS;
}