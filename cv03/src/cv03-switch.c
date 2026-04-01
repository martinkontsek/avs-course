#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <sys/types.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#define MTU 1500

struct eth_frame
{
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t eth_type;
    uint8_t payload[MTU];
}__attribute__((packed));

struct IF_entry
{
    char name[15];
    unsigned int if_index;
    int sock;    
};

struct CAM_entry
{
    struct CAM_entry *prev;
    struct CAM_entry *next;
    uint8_t mac[6];
    struct IF_entry *iface;
};

void add_CAM_entry(struct CAM_entry **head, uint8_t *mac, struct IF_entry *iface)
{
    // create CAM entry
    struct CAM_entry *entry;
    entry = calloc(1, sizeof(struct CAM_entry));
    entry->iface = iface;
    memcpy(entry->mac, mac, 6);

    //add entry to table
    if(*head == NULL)
    {
        //table empty -> add as first
        *head = entry;
    } else {
        //add as last item
        struct CAM_entry *pom;
        pom = *head;
        while(pom->next != NULL)
        {
            pom = pom->next;
        }

        pom->next = entry;
        entry->prev = pom;
        entry->next = NULL;
    }
}

void remove_CAM_entry(struct CAM_entry **head, struct CAM_entry *item_to_remove)
{
    if(item_to_remove->prev != NULL)
        item_to_remove->prev->next = item_to_remove->next;
    if(item_to_remove->next != NULL)    
        item_to_remove->next->prev = item_to_remove->prev;

    //i'm first in the table
    if(item_to_remove->prev == NULL)
        *head = item_to_remove->next;

    //only 1 item in table
    if(item_to_remove->next == NULL && item_to_remove->prev == NULL)
        *head = NULL;
    
    free(item_to_remove);
}

struct CAM_entry * find_CAM_entry(struct CAM_entry **head, uint8_t *mac)
{
    struct CAM_entry *pom;
    pom = *head;
    while(pom != NULL)
    {
        if(memcmp(pom->mac, mac, 6) == 0)
            return pom;      
        pom = pom->next;
    }
    return NULL;
}

void destroy_CAM_table(struct CAM_entry **head)
{
    struct CAM_entry *pom;
    struct CAM_entry *pom2;
    pom = *head;

    while(pom != NULL)
    {
        pom2 = pom->next;
        free(pom);
        pom = pom2;
    }

    *head = NULL;
}

void print_CAM_table(struct CAM_entry **head)
{
    printf("|-------------------|-----------|\n");
    printf("|    MAC address    | interface |\n");
    printf("|-------------------|-----------|\n");

    struct CAM_entry *pom;
    pom = *head;
    while(pom != NULL)
    {
        printf("| %02x:%02x:%02x:%02x:%02x:%02x | %09s |\n",
            pom->mac[0],
            pom->mac[1],
            pom->mac[2],
            pom->mac[3],
            pom->mac[4],
            pom->mac[5],
            pom->iface->name
        );
        pom = pom->next;
    }
    
    printf("|-------------------|-----------|\n\n");
}


int main()
{
    //create empty table
    struct CAM_entry *head = NULL;

    const int if_count = 2;
    char iface_names[if_count][5];
    strcpy(iface_names[0], "ens4");
    strcpy(iface_names[1], "ens5");
    struct IF_entry if_entries[if_count];
    int highest_socket = 0;

    for(int i=0; i<if_count; i++)
    {
        memset(&if_entries[i], 0, sizeof(struct IF_entry));
        strcpy(if_entries[i].name, iface_names[i]);

        if_entries[i].if_index = if_nametoindex(if_entries[i].name);
        if(if_entries[i].if_index == 0)
        {
            perror("IF_NAMETOINDEX");
            //TODO: clean
            exit(EXIT_FAILURE);
        }

        if((if_entries[i].sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
        {
            perror("SOCKET");
            //TODO: clean
            exit(EXIT_FAILURE);
        };

        // determine highest socket number
        if(highest_socket < if_entries[i].sock)
            highest_socket = if_entries[i].sock;

        struct sockaddr_ll addr;
        memset(&addr, 0, sizeof(addr));
        addr.sll_family = AF_PACKET;
        addr.sll_ifindex = if_entries[i].if_index;

        if(bind(if_entries[i].sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("BIND");
            //TODO: clean
            exit(EXIT_FAILURE);
        }

        //Enable promiscious mode
        struct ifreq ifr;
        memset(&ifr, 0, sizeof(ifr));
        strcpy(ifr.ifr_name, if_entries[i].name);
        if(ioctl(if_entries[i].sock, SIOCGIFFLAGS, &ifr) == -1)
        {
            perror("IOCTL GET");
            //TODO: clean
            exit(EXIT_FAILURE); 
        }
        ifr.ifr_flags |= IFF_PROMISC;

        if(ioctl(if_entries[i].sock, SIOCSIFFLAGS, &ifr) == -1)
        {
            perror("IOCTL SET");
            //TODO: clean
            exit(EXIT_FAILURE); 
        }
    }

    struct eth_frame buffer;
    fd_set fds;
    ssize_t recv_len;
    struct CAM_entry *cam_entry_pom;

    for(;;)
    {        
        FD_ZERO(&fds);
        for(int i=0; i<if_count; i++)
        {
            FD_SET(if_entries[i].sock, &fds);
        }

        if(select(highest_socket+1, &fds, NULL, NULL, NULL) == -1)
        {
            perror("SELECT");
            continue;
        }

        for(int i=0; i<if_count; i++)
        {
            if(FD_ISSET(if_entries[i].sock, &fds) != 0)
            {
                memset(&buffer, 0, sizeof(buffer));
                recv_len = recv(if_entries[i].sock, &buffer, sizeof(buffer), 0);
                
                //receiving error
                if(recv_len == -1)
                    continue;

                //FORWARDING
                cam_entry_pom = NULL;
                cam_entry_pom = find_CAM_entry(&head, buffer.dst_mac);
                if(cam_entry_pom != NULL)
                {
                    //entry found -> forwarding through 1 iface
                    send(cam_entry_pom->iface->sock, &buffer, recv_len, 0);
                } else {
                    //entry not found -> flooding
                    for(int j=0; j<if_count; j++)
                    {
                        //if not same interface as receiving one -> forward
                        if(if_entries[j].sock != if_entries[i].sock)
                            send(if_entries[j].sock, &buffer, recv_len, 0);
                    }
                }

                //LEARNING
                cam_entry_pom = NULL;
                cam_entry_pom = find_CAM_entry(&head, buffer.src_mac);
                if(cam_entry_pom == NULL)
                {
                    //entry not found -> add to table
                    add_CAM_entry(&head, buffer.src_mac, &if_entries[i]);
                } else {
                    //entry found -> update interface
                    cam_entry_pom->iface = &if_entries[i];
                }

                //print CAM table
                print_CAM_table(&head);
            }
        }

    }

    return EXIT_SUCCESS;
}