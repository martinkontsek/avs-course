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

#define PORT 8080
#define IP "224.0.0.2"

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
    if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &allow_multicast, sizeof(allow_multicast)) == -1)
    {
        perror("SETSOCKOPT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[100];

    for(;;)
    {
        bzero(buffer, 100);
        bzero(&addr, sizeof(addr));
        socklen_t addr_len = sizeof(addr);
        recvfrom(sock, buffer, 100, 0, (struct sockaddr *)&addr, &addr_len);

        printf("Msg from [%s:%d]: %s",
            inet_ntoa(addr.sin_addr),
            ntohs(addr.sin_port),
            buffer    
        );
    }

    close(sock);
    return EXIT_SUCCESS;
}