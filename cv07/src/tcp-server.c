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

#include <pthread.h>

#define BIND_IP "0.0.0.0"
#define BIND_PORT 9999
#define BACKLOG 10


void * handle_client(void *sock)
{
    int client_sock;
    client_sock = *((int *) sock);
    //Handle client
    while(1)
    {
        int read_len;
        char buf[1500];
        if((read_len = read(client_sock, buf, 1500)) == -1)
            continue;
        
        // socket is already closed
        if(read_len == 0)
            break;

        buf[read_len] = '\0';
        printf("Msg: %s\n", buf);
    }

    return NULL;
}

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
    if(inet_pton(AF_INET, BIND_IP, &addr.sin_addr) == 0)
    {
        fprintf(stderr, "ERROR: inet_pton\n");
        close(sock);
        exit(EXIT_FAILURE);
    }

    addr.sin_port = htons(BIND_PORT);

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

    while(1)
    {
        int client_sock;
        socklen_t addr_len;
        bzero(&addr, sizeof(addr));
        addr_len = sizeof(addr);
        if((client_sock = accept(sock, (struct sockaddr *)&addr, &addr_len)) == -1)
            continue;

        printf("Connected client %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port)); 
        //handle client

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, (void *) &client_sock);
    }

    shutdown(sock, SHUT_RDWR);
    return EXIT_SUCCESS;
}
