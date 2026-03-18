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

#define IF_NAME "ens4"
#define MSG "Hello world from AvS course!!!"

struct eth_frame
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
    char payload_msg[100];
}__attribute__((packed));

int main()
{
    int sock;

    sock = socket(AF_PACKET, SOCK_RAW, 0);
    if(sock == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));

    addr.sll_family = AF_PACKET;
    addr.sll_ifindex = if_nametoindex(IF_NAME);
    //TODO: osetrit

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("BIND");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct eth_frame frame;
    memset(&frame, 0, sizeof(frame));
    
    frame.dst_mac[0] = 0x11;
    frame.dst_mac[1] = 0x22;
    frame.dst_mac[2] = 0x33;
    frame.dst_mac[3] = 0x44;
    frame.dst_mac[4] = 0x55;
    frame.dst_mac[5] = 0x66;

    memcpy(frame.src_mac, frame.dst_mac, 6);

    frame.eth_type = htons(0xabba);
    memcpy(frame.payload_msg, MSG, sizeof(MSG));

    write(sock, &frame, sizeof(frame));

    close(sock);

    return EXIT_SUCCESS;
}