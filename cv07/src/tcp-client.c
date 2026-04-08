#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define IP "::1"
#define PORT 9999

int main()
{
    int sock;
    sock = socket(AF_INET6, SOCK_STREAM, 0);
    if(sock == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in6 addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    if(inet_pton(AF_INET6, IP, &addr.sin6_addr) <= 0)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    }
    addr.sin6_port = htons(PORT);

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("CONNECT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[100];
    memset(buffer, 0, 100);

    printf("Enter msg: ");
    scanf("%s", buffer);
    
    send(sock, buffer, strlen(buffer), 0);
    
    close(sock);
    return EXIT_SUCCESS;
}