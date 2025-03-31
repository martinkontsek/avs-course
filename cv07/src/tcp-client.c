#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#include <net/if.h>

#define DST_IP "::1"
#define DST_PORT 4321

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
    bzero(&addr, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_port = htons(DST_PORT);
    if(inet_pton(AF_INET6, DST_IP, &addr.sin6_addr) < 0)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    } 

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("CONNECT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    char buffer[100];
    bzero(buffer, 100);

    printf("Enter msg to send: ");
    fgets(buffer, 100, stdin);

    send(sock, buffer, strlen(buffer), 0);

    close(sock);
    return EXIT_SUCCESS;
}