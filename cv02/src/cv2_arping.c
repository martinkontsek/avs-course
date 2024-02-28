#include "cv2.h"

#define IF_NAME "enp0s3"
#define SENDER_MAC "08:00:27:b4:1e:53"
#define SENDER_IP "10.0.2.15"
#define TARGET_IP "10.0.2.2"

#define HW_TYPE 1
#define PROTO_TYPE ETHERTYPE_IP
#define HW_LEN 6
#define PROTO_LEN 4
#define OPCODE_REQ 1
#define OPCODE_REPLY 2

int main()
{
	int sock;
	struct sockaddr_ll addr;
	struct eth_hdr *hdr;
	struct arp_hdr *arp;
	void *buffer;
	if((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP))) == -1)
	{
		perror("SOCKET");
		exit(EXIT_FAILURE);
	}

	bzero(&addr, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ARP);
    if((addr.sll_ifindex = if_nametoindex(IF_NAME)) == 0)
    {
        close(sock);
        perror("if_nametoindex");
        exit(EXIT_FAILURE);
    }

    if(bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
        close(sock);
        perror("BIND");
        exit(EXIT_FAILURE);
    }

	buffer = calloc(1, sizeof(struct eth_hdr) + sizeof(struct arp_hdr));
	
	hdr = (struct eth_hdr *) buffer;
	memset(hdr->dst_mac, 0xff, 6);

	sscanf(SENDER_MAC, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",  
			&hdr->src_mac[0],
			&hdr->src_mac[1],
			&hdr->src_mac[2],
			&hdr->src_mac[3],
			&hdr->src_mac[4],
			&hdr->src_mac[5]);

	hdr->ethertype = htons(ETHERTYPE_ARP);

	arp = (struct arp_hdr *) hdr->payload;
	arp->hw_type = htons(HW_TYPE);
	arp->proto_type = htons(PROTO_TYPE);
	arp->hw_len = HW_LEN;
	arp->proto_len = PROTO_LEN;
	arp->opcode = htons(OPCODE_REQ);
	memcpy(arp->sender_eth, hdr->src_mac, HW_LEN);
	if(inet_aton(SENDER_IP, &arp->sender_ip) == 0)
	{
		fprintf(stderr, "INET_ATON: Error with sender address.\n");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);
	}
	if(inet_aton(TARGET_IP, &arp->target_ip) == 0)
	{
		fprintf(stderr, "INET_ATON: Error with target address.\n");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);
	}

	if(send(sock, buffer, sizeof(struct eth_hdr)+sizeof(struct arp_hdr), 0) == -1)
	{
		perror("SEND");
		close(sock);
		free(buffer);
		exit(EXIT_FAILURE);
	}

	close(sock);
	free(buffer);
	return EXIT_SUCCESS;
}