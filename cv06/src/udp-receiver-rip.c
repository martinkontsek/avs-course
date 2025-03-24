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

#include <signal.h>
#include <net/if.h>

#define PORT 520
#define ADDR "224.0.0.9"
#define IFACE "ens4"
#define LOCAL_ADDR "192.168.1.1"

struct RIP_entry
{
	uint16_t afi;
	uint16_t route_tag;
	struct in_addr prefix;
	struct in_addr netmask;
	struct in_addr next_hop;
	uint32_t metric;
}__attribute__((packed));

struct RIP_hdr
{
	uint8_t cmd;
	uint8_t version;
	uint16_t unused;
	struct RIP_entry payload[0];
}__attribute__((packed));

int sock;
void handle_signal()
{
	printf("Exitting program!!!.");
	close(sock);
	exit(EXIT_SUCCESS);
}

int main()
{	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock == -1)
	{
		perror("SOCKET");
		exit(EXIT_FAILURE);
	}

	//handle CTRL+C - close socket
	signal(SIGINT, handle_signal);

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

	struct ip_mreqn multi_struct;
	bzero(&multi_struct, sizeof(multi_struct));
	if(inet_pton(AF_INET, ADDR, &multi_struct.imr_multiaddr) == 0)
	{
		printf("ERROR: INET_PTON!!!\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	if(inet_pton(AF_INET, LOCAL_ADDR, &multi_struct.imr_address) == 0)
	{
		printf("ERROR: INET_PTON!!!\n");
		close(sock);
		exit(EXIT_FAILURE);
	}
	multi_struct.imr_ifindex = if_nametoindex(IFACE);
	if(multi_struct.imr_ifindex == 0)
	{
		perror("IF_NAMETOINDEX");
		close(sock);
		exit(EXIT_FAILURE);
	}
	if(setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multi_struct, sizeof(multi_struct)) == -1)
	{
		perror("SETSOCKOPT");
		close(sock);
		exit(EXIT_FAILURE);
	}
	
	int rip_max_len = sizeof(struct RIP_hdr) + 25*sizeof(struct RIP_entry);
	char buffer[rip_max_len];
	socklen_t addr_len;
	addr_len = sizeof(addr);
	ssize_t recv_len = 0;
	struct RIP_hdr *hdr;
	struct RIP_entry *entry;

	for(;;)
	{
		bzero(buffer, rip_max_len);
		recv_len = recvfrom(sock, buffer, rip_max_len, 0, (struct sockaddr *)&addr, &addr_len);
		
		//size is lower then  rip_hdr + 1*rip_entry
		if((recv_len - sizeof(struct RIP_hdr) - sizeof(struct RIP_entry)) < 0)
			continue;
		
		//there must be (1-25)*rip_entry
		if((recv_len - sizeof(struct RIP_hdr) - sizeof(struct RIP_entry)) % sizeof(struct RIP_entry) != 0)
			continue;
		
		hdr = (struct RIP_hdr *)buffer;
		
		//cmd must be rip reply (2)
		if(hdr->cmd != 2)
			continue;

		//version must be 2
		if(hdr->version != 2)
			continue;
		
		printf("RIPv2 from [%s:%d]\n",
			inet_ntoa(addr.sin_addr),
			ntohs(addr.sin_port)
		);


		//process all rip_entries based on remaining length
		int to_process = recv_len - sizeof(struct RIP_hdr);
		entry = hdr->payload;
		while(to_process > 0)
		{
			if(entry->afi == htons(2))
				printf("  P:%s",
					inet_ntoa(entry->prefix)
				);
				printf(", M:%s",
					inet_ntoa(entry->netmask)
				);
				printf(", NH:%s, C:%d\n",
					inet_ntoa(entry->next_hop),
					ntohl(entry->metric)
				);

			to_process -= sizeof(struct RIP_entry);
			entry++;
		}
		
	}

	close(sock);
	return EXIT_SUCCESS;
}