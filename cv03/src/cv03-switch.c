#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#define MTU 1500

struct Eth_frame
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
    char payload[MTU];
}__attribute__((packed));

struct IF_entry
{
    char name[20];
    int sock;
    unsigned int index;
};

struct CAM_entry
{
    struct CAM_entry *next;
    struct CAM_entry *prev;
    uint8_t mac[6];
    struct IF_entry *iface;
};

struct CAM_entry * add_entry_to_CAM(struct CAM_entry *head, uint8_t *mac, struct IF_entry *iface)
{
    struct CAM_entry *entry;
    entry = calloc(1, sizeof(struct CAM_entry));
    if(entry == NULL)
        return NULL;
    memcpy(entry->mac, mac, 6);
    entry->iface = iface;

    //empty CAM
    if(head == NULL)
    {
        head = entry;
        return head;
    }

    //CAM not empty
    entry->next = head;
    head->prev = entry;
    head = entry;
    return head;
}

struct CAM_entry * find_in_CAM(struct CAM_entry *head, uint8_t *mac)
{
    if(head == NULL)
        return NULL;
    
    struct CAM_entry *current;
    current = head;
    while(current != NULL)
    {
        if(memcmp(current->mac, mac, 6) == 0)
            return current;

        current = current->next;
    }
    return NULL;
}

void print_CAM(struct CAM_entry *head)
{
    printf("|^^^^^^^^^^^^^^^^^^^^^^|^^^^^^^^^^^^^^^^^^^|\n");
    printf("|         PORT         |         MAC       |\n");
    printf("|----------------------|-------------------|\n");

    if(head == NULL)
        printf("|         Prazdna Tabulka CAM!!!           |\n");

    struct CAM_entry *current;
    current = head;
    while(current != NULL)
    {
        printf("| %-20s | %02x:%02x:%02x:%02x:%02x:%02x |\n",
            current->iface->name,
            current->mac[0],
            current->mac[1],
            current->mac[2],
            current->mac[3],
            current->mac[4],
            current->mac[5]
        );
        current = current->next;
    }

    printf("|----------------------|-------------------|\n");
}

struct CAM_entry * remove_from_CAM(struct CAM_entry *head, uint8_t *mac)
{
    struct CAM_entry *to_delete;
    to_delete = find_in_CAM(head, mac);
    if(to_delete == NULL)
        return NULL;

    if(to_delete->next != NULL)
        to_delete->next->prev = to_delete->prev;

    if(to_delete->prev != NULL)
        to_delete->prev->next = to_delete->next;

    if(to_delete == head)
        head = to_delete->next;

    free(to_delete);
    return head;
}

int delete_CAM_table(struct CAM_entry *head)
{
    if(head == NULL)
        return EXIT_SUCCESS;

    struct CAM_entry *current;
    current = head;
    while(current != NULL)
    {
        head = current->next;
        free(current);
        current = head;
    }

    head = NULL;
    return EXIT_SUCCESS;
}

int main()
{
    int if_count = 2;
    struct IF_entry ifaces[if_count];
    bzero(ifaces, sizeof(struct IF_entry)*if_count);
    
    strcpy(ifaces[0].name, "ens4");
    strcpy(ifaces[1].name, "ens5");

    struct sockaddr_ll addr;
    struct ifreq ifr;
    int sock_max_num = 0;

    for(int i = 0;i<if_count; i++)
    {
        ifaces[i].index = if_nametoindex(ifaces[i].name);
        if(ifaces[i].index == 0)
        {
            perror("IF_NAMETOINDEX");
            //TODO: cleanup
            exit(EXIT_FAILURE);
        }

        ifaces[i].sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if(ifaces[i].sock == -1)
        {
            perror("SOCKET");
            //TODO: cleanup
            exit(EXIT_FAILURE);
        }
        if(sock_max_num < ifaces[i].sock)
            sock_max_num = ifaces[i].sock;

        bzero(&addr, sizeof(addr));
        addr.sll_family = AF_PACKET;
        addr.sll_ifindex = ifaces[i].index;

        if(bind(ifaces[i].sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("BIND");
            //TODO: cleanup
            exit(EXIT_FAILURE);
        }

        bzero(&ifr, sizeof(ifr));
        strcpy(ifr.ifr_name, ifaces[i].name);
        if(ioctl(ifaces[i].sock, SIOCGIFFLAGS, &ifr) == -1)
        {
            perror("IOCTL GET");
            //TODO: cleanup
            exit(EXIT_FAILURE);
        }

        ifr.ifr_flags |= IFF_PROMISC;

        if(ioctl(ifaces[i].sock, SIOCSIFFLAGS, &ifr) == -1)
        {
            perror("IOCTL SET");
            //TODO: cleanup
            exit(EXIT_FAILURE);
        }
    }

    //create empty CAM
    struct CAM_entry *head = NULL;

    struct Eth_frame frame;
    fd_set fds;
    struct CAM_entry *entry;
    ssize_t frame_len;
    for(;;)
    {
        FD_ZERO(&fds);
        for(int j=0; j<if_count; j++)
            FD_SET(ifaces[j].sock, &fds);
            
        if(select(sock_max_num+1, &fds, NULL, NULL, NULL) == -1)
        {
            perror("SELECT");
            continue;
        }

        for(int j=0; j<if_count; j++)
        {   
            //there is frame in this socket
            if(FD_ISSET(ifaces[j].sock, &fds) != 0)
            {
                bzero(&frame, sizeof(frame));
                frame_len = recv(ifaces[j].sock, &frame, sizeof(frame), 0);

                //LEARNING
                entry = find_in_CAM(head, frame.src_mac);
                if(entry == NULL)
                {
                    //src mac not in CAM -> add
                    entry = add_entry_to_CAM(head, frame.src_mac, &ifaces[j]);
                    if(entry != NULL)
                        head = entry;
                    printf("SRC MAC not found -> Learning.\n");
                    print_CAM(head);
                } else {
                    if(entry->iface != &ifaces[j])
                    {
                        //iface not same -> update
                        entry->iface = &ifaces[j];
                        printf("Interface changed, updating.\n");
                        print_CAM(head);
                    }
                }
                

                //FORWARDING
                entry = find_in_CAM(head, frame.dst_mac);
                if(entry != NULL)
                {
                    //DST mac found -> send through one iface
                    send(entry->iface->sock, &frame, frame_len, 0);
                    printf("DST mac found -> use 1 iface.\n");
                } else {
                    //DST mac NOT found -> FLOODING!!!
                    for(int k=0;k<if_count; k++)
                    {
                        if(k != j)
                            send(ifaces[k].sock, &frame, frame_len, 0);
                    }
                    printf("DST mac %02x:%02x:%02x:%02x:%02x:%02x NOT found -> FLOODING!!!.\n",
                        frame.dst_mac[0],
                        frame.dst_mac[1],
                        frame.dst_mac[2],
                        frame.dst_mac[3],
                        frame.dst_mac[4],
                        frame.dst_mac[5]
                    );
                }
                
            }
        }
    }

    //TODO: clean
    return EXIT_SUCCESS;
}