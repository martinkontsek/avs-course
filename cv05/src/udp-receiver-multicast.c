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
#include <netinet/ip.h>
#include <netinet/udp.h>

#define MULTI_IP "224.0.0.10"
#define PORT 9999

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
    if(inet_pton(AF_INET, MULTI_IP, &addr.sin_addr) == -1)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    }
    addr.sin_port = htons(PORT);

    if(bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
    {
        perror("BIND");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct ip_mreqn multistruct;
    memset(&multistruct, 0, sizeof(multistruct));
    if(inet_pton(AF_INET, MULTI_IP, &multistruct.imr_multiaddr) == -1)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multistruct, sizeof(multistruct)) == -1)
    {
        perror("SETSOCKOPT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[100];
    socklen_t addr_len = sizeof(addr);
    for(;;)
    {        
        memset(buffer, 0, 100);
        memset(&addr, 0, sizeof(addr));
        if(recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &addr, &addr_len) == -1)
            continue;
        printf("MSG from [%s:%d]: %s\n",
            inet_ntoa(addr.sin_addr),
            ntohs(addr.sin_port),
            buffer
        );
    }

    close(sock);
    return EXIT_SUCCESS;
}