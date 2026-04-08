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
#include <pthread.h>

#define IP "::"
#define PORT 9999
#define BACKLOG 1

void handle_client(void * client_sock_param)
{
    int client_sock = *((int *)client_sock_param);
    char buffer[100];    
    for(;;)
    {
        memset(buffer, 0, 100);
        if(recv(client_sock, buffer, 100, 0) <= 0)
        {
            close(client_sock);
            break;
        }

        printf("MSG: %s\n",
            buffer
        );
    }
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
    memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    if(inet_pton(AF_INET6, IP, &addr.sin6_addr) <= 0)
    {
        perror("INET_PTON");
        close(sock);
        exit(EXIT_FAILURE);
    }
    addr.sin6_port = htons(PORT);

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        perror("BIND");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if(listen(sock, BACKLOG) == -1)
    {
        perror("LISTEN");
        close(sock);
        exit(EXIT_FAILURE);
    }

    int client_sock;
    socklen_t addr_len = sizeof(addr);
    for(;;)
    {
        memset(&addr, 0, sizeof(addr));
        client_sock = accept(sock, (struct sockaddr *)&addr, &addr_len);
        if(client_sock == -1)
        {
            perror("ACCEPT");
            continue;
        }
        //FFFF:FFFF:
        char string_ip6[50];
        inet_ntop(AF_INET6, &addr.sin6_addr, string_ip6, 50);
        printf("Connected client [%s:%d].\n",
            string_ip6,
            ntohs(addr.sin6_port)
        );

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_client, &client_sock);
    }

    close(sock);
    return EXIT_SUCCESS;
}