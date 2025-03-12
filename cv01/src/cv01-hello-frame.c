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

#define IF_NAME "ens4"
#define PAYLOAD "Ahoj z AvS cvicenia 1."

struct Eth_frame
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
    char payload[100];
}__attribute__((packed));

int main()
{
    int sock;
    sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if(sock == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll addr;
    bzero(&addr, sizeof(addr));
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

    struct Eth_frame frame;
    bzero(&frame, sizeof(frame));
    frame.dst_mac[0] = 0x00;
    frame.dst_mac[1] = 0x11;
    frame.dst_mac[2] = 0x22;
    frame.dst_mac[3] = 0x33;
    frame.dst_mac[4] = 0x44;
    frame.dst_mac[5] = 0x55;

    //fa:16:3e:c5:83:d9
    // sscanf TODO
    frame.src_mac[0] = 0xfa;
    frame.src_mac[1] = 0x16;
    frame.src_mac[2] = 0x3e;
    frame.src_mac[3] = 0xc5;
    frame.src_mac[4] = 0x83;
    frame.src_mac[5] = 0xd9;

    frame.eth_type = htons(0xabba);

    memcpy(frame.payload, PAYLOAD, sizeof(PAYLOAD));

    write(sock, &frame, sizeof(frame));

    close(sock);
    return EXIT_SUCCESS;
}