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

#define DST_IP "224.0.0.9"
#define DST_PORT 9999


int main()
{
	int sock;
	char msg[200];
	struct sockaddr_in addr;

	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("SOCKET");
		exit(EXIT_FAILURE);
	}

	printf("Enter message to send: \n");
	scanf("%s", msg);

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(DST_PORT);
	
	if(inet_pton(AF_INET, DST_IP, (void *) &addr.sin_addr) == 0)
	{
		fprintf(stderr, "ERROR: inet_pton\n");
		close(sock);
		exit(EXIT_FAILURE);	
	}
	
	if(sendto(sock, msg, strlen(msg)+1, 0, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		perror("SENDTO");

	close(sock);
	return EXIT_SUCCESS;
}

