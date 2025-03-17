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

#define PORT 1234
#define ADDR "0.0.0.0"

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
	if(inet_aton(ADDR, &addr.sin_addr) == 0)
	{
		printf("ERROR: INET_ATON!!!\n");
		close(sock);
		exit(EXIT_FAILURE);
	}

	if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)))
	{
		perror("BIND");
		close(sock);
		exit(EXIT_FAILURE);
	}

	char buffer[100];
	socklen_t addr_len;
	addr_len = sizeof(addr);

	for(;;)
	{
		bzero(buffer, 100);
		recvfrom(sock, buffer, 100, 0, (struct sockaddr *)&addr, &addr_len);
		printf("Msg from [%s:%d]: %s\n",
			inet_ntoa(addr.sin_addr),
			ntohs(addr.sin_port),
			buffer);
		sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, addr_len);
	}

	close(sock);
	return EXIT_SUCCESS;
}