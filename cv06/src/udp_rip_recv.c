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
#include <netinet/ip.h>
#include <net/if.h>

#define BIND_IP "224.0.0.9"
#define BIND_PORT 520
#define IF_NAME "enp0s8"

struct rip_entry
{
	uint16_t af;
	uint16_t route_tag;
	struct in_addr network;
	struct in_addr netmask;
	struct in_addr next_hop;
	uint32_t metric;
}__attribute__((packed));

struct rip_msg
{
	uint8_t cmd;
	uint8_t ver;
	uint16_t unused;
	struct rip_entry payload[0];
}__attribute__((packed));


int main()
{
	int sock;
	char *buffer;
	int buffer_len;
	struct sockaddr_in addr;
	char ip[16];
	int addr_len;

	buffer_len = sizeof(struct rip_msg) + 25*sizeof(struct rip_entry);
	buffer = malloc(buffer_len);

	if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("SOCKET");
		free(buffer);
		exit(EXIT_FAILURE);
	}

	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(BIND_PORT);
	
	if(inet_pton(AF_INET, BIND_IP, (void *) &addr.sin_addr) == 0)
	{
		fprintf(stderr, "ERROR: inet_pton\n");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);	
	}

	if(bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	{
		perror("BIND");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);
	}

	struct ip_mreqn multi_struct;
	bzero(&multi_struct, sizeof(multi_struct));
	if(inet_pton(AF_INET, BIND_IP, &multi_struct.imr_multiaddr) == 0)
	{
		fprintf(stderr, "ERROR: inet_pton2\n");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);	
	}
	if((multi_struct.imr_ifindex = (int) if_nametoindex(IF_NAME)) == 0)
	{
		perror("IF_NAMETOINDEX");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);
	}	

	if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multi_struct, sizeof(multi_struct)) == -1)
	{
		perror("SETSOCKOPT");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);
	}
	
	while(1)
	{	
		bzero(&addr, sizeof(addr));
		addr_len = sizeof(addr);
		bzero(buffer, buffer_len);
		int read_len;

		if((read_len = recvfrom(sock, buffer, buffer_len, 0, (struct sockaddr *) &addr, (socklen_t *) &addr_len)) == -1)
			continue;
		bzero(ip, 16);
		if(inet_ntop(AF_INET, &addr.sin_addr, ip, 16) == NULL)
			continue;

		struct rip_msg *msg;
		msg = (struct rip_msg *)buffer;

		//not a rip reply
		if(msg->cmd != 0x02)
			continue;
		//not a rip v2	
		if(msg->ver != 2)
			continue;
		//finished processing rip_hdr
		read_len -= sizeof(struct rip_msg);
		printf("RIPv2 msg from %s:%d\n",
				ip,
				ntohs(addr.sin_port));
		//process rip entries
		struct rip_entry *entry;
		entry = (struct rip_entry *) msg->payload;
		while(read_len >= sizeof(struct rip_entry))
		{
			//not an IPv4 entry
			if(entry->af == htons(AF_INET))
			{
				printf("  network:  %s\n",
					inet_ntoa(entry->network));
				printf("  netmask:  %s\n",
					inet_ntoa(entry->netmask));
				printf("  next_hop: %s\n",
					inet_ntoa(entry->next_hop));
				printf("  metric:   %d\n",
					htonl(entry->metric));
			}
			read_len -= sizeof(struct rip_entry);
			entry++;
		}
	}

	close(sock);
	free(buffer);
	return EXIT_SUCCESS;
}

