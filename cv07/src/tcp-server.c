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
#include <pthread.h>

#define IP "::"
#define PORT 4321
#define BACKLOG 2

void * handle_client(int *sock)
{
    int client_sock = *sock;

    char buffer[100];
    bzero(buffer, 100);
    recv(client_sock, buffer, 100, 0);
    printf("MSG: %s", buffer);
    close(client_sock);
}

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
    addr.sin6_port = htons(PORT);
    if(inet_pton(AF_INET6, IP, &addr.sin6_addr) < 1)
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

    if(listen(sock, BACKLOG))
    {
        perror("LISTEN");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int clinet_sock;
    socklen_t addr_len = sizeof(addr);
    char buffer[100];
    for(;;)
    {
        bzero(&addr, sizeof(addr));
        clinet_sock = accept(sock, (struct sockaddr *)&addr, &addr_len);
        if(clinet_sock == -1)
        {
            perror("ACCEPT");
            continue;
        }

        bzero(buffer, 100);
        inet_ntop(AF_INET6, &addr.sin6_addr, buffer, 100);

        printf("Client [%s]:%d connected.\n",
            buffer,
            ntohs(addr.sin6_port)
        );

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, &clinet_sock);
        continue;
    }

    close(sock);
    return EXIT_SUCCESS;
}