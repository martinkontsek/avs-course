#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>

#define BIND_IP "0.0.0.0"
#define BIND_PORT 9999


int main()
{
	int sock;
	char msg[200];
	struct sockaddr_in addr;
	char ip[16];
	int addr_len;

	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("SOCKET");
		exit(EXIT_FAILURE);
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BIND_PORT);
	
	if(inet_pton(AF_INET, BIND_IP, (void *) &addr.sin_addr) == 0)
	{
		fprintf(stderr, "ERROR: inet_pton\n");
		close(sock);
		exit(EXIT_FAILURE);	
	}

	if(bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	{
		perror("BIND");
		close(sock);
		exit(EXIT_FAILURE);
	}
	
	while(1)
	{	
		bzero(&addr, sizeof(addr));
		addr_len = sizeof(addr);
		if(recvfrom(sock, msg, 200, 0, (struct sockaddr *) &addr, (socklen_t *) &addr_len) == -1)
			continue;
		bzero(ip, 16);
		if(inet_ntop(AF_INET, &addr.sin_addr, ip, 16) == NULL)
			continue;
		printf("Msg from %s:%d : %s\n",
				ip,
				ntohs(addr.sin_port),
				msg);
	}
	close(sock);
	return EXIT_SUCCESS;
}

