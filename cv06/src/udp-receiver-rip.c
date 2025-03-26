#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#include <signal.h>
#include <net/if.h>

#define PORT 520
#define IP "224.0.0.9"
#define IFACE "ens4"

struct RIP_entry
{
    uint16_t afi;
    uint16_t route_tag;
    struct in_addr prefix;
    struct in_addr mask;
    struct in_addr next_hop;
    uint32_t metric;
}__attribute__((packed));

struct RIP_hdr
{
    uint8_t cmd;
    uint8_t version;
    uint16_t unused;
    struct RIP_entry entry[0];
}__attribute__((packed));

int sock;
void handle_signal()
{
    printf("Exiting program.");
    close(sock);
    exit(EXIT_SUCCESS);
}

int main()
{
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    signal(SIGINT, handle_signal);

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    if(inet_pton(AF_INET, IP, &addr.sin_addr) < 1)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    } 

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("BIND");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct ip_mreqn allow_multicast;
    bzero(&allow_multicast, sizeof(allow_multicast));
    if(inet_pton(AF_INET, IP, &allow_multicast.imr_multiaddr) < 1)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    }
    if((allow_multicast.imr_ifindex = if_nametoindex(IFACE)) == 0)
    {
        perror("IF_NAMETOINDEX");
        close(sock);
        exit(EXIT_FAILURE);
    } 

    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &allow_multicast, sizeof(allow_multicast)) == -1)
    {
        perror("SETSOCKOPT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int rip_max_len = sizeof(struct RIP_hdr) + 25*sizeof(struct RIP_entry);
    char buffer[rip_max_len];
    ssize_t rip_recv_len;
    struct RIP_hdr *hdr;
    struct RIP_entry *entry;

    for(;;)
    {
        bzero(buffer, rip_max_len);
        bzero(&addr, sizeof(addr));
        socklen_t addr_len = sizeof(addr);
        rip_recv_len = recvfrom(sock, buffer, rip_max_len, 0, (struct sockaddr *)&addr, &addr_len);

        //invalid RIP packet, smaller then allowed
        if(rip_recv_len < sizeof(struct RIP_hdr) + sizeof(struct RIP_entry))
            continue;

        //invalid RIP packet, does not contain 1-25 whole RIP entries    
        if( (rip_recv_len - sizeof(struct RIP_hdr)) % sizeof(struct RIP_entry) != 0 )
            continue;

        hdr = (struct RIP_hdr *)buffer;

        //cmd is not 2 (rip reply)
        if(hdr->cmd != 2)
            continue;

        //version is not 2
        if(hdr->version != 2)
            continue;        

        printf("RIP from [%s:%d]\n",
            inet_ntoa(addr.sin_addr),
            ntohs(addr.sin_port)    
        );

        rip_recv_len -= sizeof(struct RIP_hdr);
        entry = (struct RIP_entry *) hdr->entry;
        while(rip_recv_len > 0)
        {
            if(ntohs(entry->afi) == AF_INET)
            {
                printf("  P:%s ",
                    inet_ntoa(entry->prefix)   
                );
                printf("M:%s ",
                    inet_ntoa(entry->mask)  
                );
                printf("N:%s C:%d\n",
                    inet_ntoa(entry->next_hop),
                    ntohl(entry->metric)    
                );
            }            

            entry++;
            rip_recv_len -= sizeof(struct RIP_entry);
        }
    }

    close(sock);
    return EXIT_SUCCESS;
}