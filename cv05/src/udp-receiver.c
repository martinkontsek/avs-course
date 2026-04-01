#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>

#define IP "127.0.0.1"
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
    if(inet_pton(AF_INET, IP, &addr.sin_addr) == -1)
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