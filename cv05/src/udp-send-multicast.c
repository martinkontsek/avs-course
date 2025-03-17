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

#define DST_PORT 1234
#define DST_ADDR "224.0.0.50"

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
	addr.sin_port = htons(DST_PORT);
	if(inet_aton(DST_ADDR, &addr.sin_addr) == 0)
	{
		printf("ERROR: INET_ATON!!!\n");
		close(sock);
		exit(EXIT_FAILURE);
	}

	int allow_broadcast = 1;
	if(setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &allow_broadcast, sizeof(allow_broadcast)) == -1)
	{
		perror("SETSOCKOPT");
		close(sock);
		exit(EXIT_FAILURE);
	}

	char buffer[100];

	bzero(buffer, 100);
	printf("Enter text to send: ");
	scanf("%s", buffer);

	sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&addr, sizeof(addr));

	close(sock);
	return EXIT_SUCCESS;
}