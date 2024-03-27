#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

#define IP "127.0.0.1"
#define PORT 9999

int main()
{
    int sock;
    struct sockaddr_in addr;
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("SOCKET");
        exit(EXIT_FAILURE);
    }

    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    if(inet_pton(AF_INET, IP, &addr.sin_addr) == 0)
    {
        fprintf(stderr, "ERROR: inet_pton\n");
        close(sock);
        exit(EXIT_FAILURE);
    }
    addr.sin_port = htons(PORT);

    if(connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("CONNECT");
        close(sock);
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        char buf[1500];

        printf("Enter msg to send (/q to exit): ");
        scanf("%s", buf);

        //exit program
        if(strcmp(buf, "/q") == 0)
            break;

        write(sock, buf, strlen(buf));
    }

    close(sock);
    return EXIT_SUCCESS;
}
