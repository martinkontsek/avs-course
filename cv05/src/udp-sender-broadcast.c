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

#define DST_PORT 8080
#define DST_IP "158.193.154.255"


int main()
{
    int sock;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if(sock == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    int allow_broadcast = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &allow_broadcast, sizeof(allow_broadcast)) == -1)
    {
        perror("SETSOCKOPT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(DST_PORT);
    if(inet_pton(AF_INET, DST_IP, &addr.sin_addr) < 1)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    } 

    char buffer[100];
    bzero(buffer, 100);

    printf("Enter text to send: ");
    // scanf("%s", buffer);
    fgets(buffer, 100, stdin);

    sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, sizeof(addr));

    close(sock);
    return EXIT_SUCCESS;
}