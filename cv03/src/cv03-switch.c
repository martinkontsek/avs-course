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

#include <signal.h>

#define MTU 1500

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

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

struct CAM_entry * create_CAM_entry(struct CAM_entry *head, uint8_t *mac, struct IF_entry *iface)
{
    struct CAM_entry *entry;
    entry = calloc(1,sizeof(struct CAM_entry));
    if(entry == NULL)
        return NULL;
    memcpy(entry->mac, mac, 6);
    entry->iface = iface;

    //CAM is empty, add first entry
    if(head == NULL)
    {
        head = entry;
        return head;
    }

    //CAM is not empty, add to the beginning
    head->prev = entry;
    entry->next = head;
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
    printf("|         "GRN"PORT"RESET"         |         "YEL"MAC"RESET"       |\n");
    printf("|----------------------|-------------------|\n");

    if(head == NULL)
    {
        printf("|             "RED"CAM is empty!!!"RESET"              |\n");
    }

    struct CAM_entry *current;
    current = head;
    while(current != NULL)
    {
        printf("| "GRN"%-20s"RESET" | "YEL"%02x:%02x:%02x:%02x:%02x:%02x"RESET" |\n",
            current->iface->name,
            current->mac[0],
            current->mac[1],
            current->mac[2],
            current->mac[3],
            current->mac[4],
            current->mac[5]);

        current = current->next;
    }

    printf("|______________________|___________________|\n");
}

int delete_entry_from_CAM(struct CAM_entry *head, uint8_t *mac)
{
    struct CAM_entry *to_delete = find_in_CAM(head, mac);
    if(to_delete == NULL)
        return EXIT_FAILURE;

    if(to_delete->prev != NULL)
        to_delete->prev->next = to_delete->next;
    if(to_delete->next != NULL) 
        to_delete->next->prev = to_delete->prev;

    if(to_delete == head)
        head = to_delete->next;

    free(to_delete);
    return EXIT_SUCCESS;
}

int remove_CAM(struct CAM_entry *head)
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
}

void clean(struct IF_entry *ifaces, int if_count, struct CAM_entry *head)
{
    //clean ifaces
    for(int i=0;i<if_count; i++)
    {
        if(ifaces[i].sock > 0)
            close(ifaces[i].sock);
    }

    remove_CAM(head);
}

//create empty CAM table
struct CAM_entry *head = NULL;

int IF_count = 2;
struct IF_entry ifaces[2];

void handle_signal()
{
    printf(BLU"\nCTRL+C pressed, cleaning.\n"RESET);
    clean(ifaces, IF_count, head);
    printf(CYN"Exitting.\n"RESET);
    exit(EXIT_SUCCESS);
}

int main()
{
    signal(SIGINT, handle_signal);

    printf("***** "YEL"A"GRN"v"RED"S "CYN"SWITCH"MAG" 2025"RESET" ***** \n press CTRL+C to exit.\n\n");
    
    bzero(ifaces, sizeof(struct IF_entry)*IF_count);
    strcpy(ifaces[0].name, "ens4");
    strcpy(ifaces[1].name, "ens5");

    struct sockaddr_ll addr;
    struct ifreq ifr;
    int max_socket_num = 0;
    for(int i=0; i<IF_count; i++)
    {
        ifaces[i].sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if(ifaces[i].sock == -1)
        {
            perror("SOCKET");
            clean(ifaces, IF_count, head);
            exit(EXIT_FAILURE);
        }
        if(max_socket_num < ifaces[i].sock)
            max_socket_num = ifaces[i].sock;

        ifaces[i].index = if_nametoindex(ifaces[i].name);
        if(ifaces[i].index == 0)
        {
            perror("IF_NAMETOINDEX");
            clean(ifaces, IF_count, head);
            exit(EXIT_FAILURE);
        }

        bzero(&addr, sizeof(addr));
        addr.sll_family = AF_PACKET;
        addr.sll_ifindex = ifaces[i].index;
        
        if(bind(ifaces[i].sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
        {
            perror("BIND");
            clean(ifaces, IF_count, head);
            exit(EXIT_FAILURE);
        }

        bzero(&ifr, sizeof(ifr));
        strcpy(ifr.ifr_name, ifaces[i].name);
        if(ioctl(ifaces[i].sock, SIOCGIFFLAGS, &ifr) == -1)
        {
            perror("IOCTL GET");
            clean(ifaces, IF_count, head);
            exit(EXIT_FAILURE);
        }

        ifr.ifr_flags |= IFF_PROMISC;

        if(ioctl(ifaces[i].sock, SIOCSIFFLAGS, &ifr) == -1)
        {
            perror("IOCTL SET");
            clean(ifaces, IF_count, head);
            exit(EXIT_FAILURE);
        }
    }    

    fd_set fds;
    struct Eth_frame buffer;
    ssize_t frame_len;
    struct CAM_entry *entry;
    for(;;)
    {
        FD_ZERO(&fds);
        for(int j=0; j<IF_count; j++)
            FD_SET(ifaces[j].sock, &fds);

        if(select(max_socket_num+1, &fds, NULL, NULL, NULL) == -1)
            continue;

        for(int j=0; j<IF_count; j++)
        {
            if(FD_ISSET(ifaces[j].sock, &fds) != 0)
            {
                bzero(&buffer, sizeof(buffer));
                frame_len = recv(ifaces[j].sock, &buffer, sizeof(buffer), 0);

                //LEARNING
                entry = find_in_CAM(head, buffer.src_mac);
                
                if(entry == NULL)
                {
                    //entry not found -> add to CAM
                    head = create_CAM_entry(head, buffer.src_mac, &ifaces[j]);
                    print_CAM(head);
                } else {
                    //entry found -> check and change port
                    if(entry->iface != &ifaces[j])
                    {
                        entry->iface = &ifaces[j];
                        print_CAM(head);
                    }
                }

                //FORWARDING
                entry = find_in_CAM(head, buffer.dst_mac);
                if(entry != NULL)
                {
                    //entry found -> sent via one iface
                    send(entry->iface->sock, &buffer, frame_len, 0);
                    printf(CYN"entry found -> sent via one iface.\n"RESET);
                } else {
                    //entry NOT found -> FLOOD!!!
                    printf(CYN"entry %02x:%02x:%02x:%02x:%02x:%02x NOT found -> FLOOD!!!.\n"RESET,
                        buffer.dst_mac[0],
                        buffer.dst_mac[1],
                        buffer.dst_mac[2],
                        buffer.dst_mac[3],
                        buffer.dst_mac[4],
                        buffer.dst_mac[5] 
                    );

                    for(int k=0; k<IF_count; k++)
                    {
                        //do not send via received interface
                        if(k != j)
                        {
                            send(ifaces[k].sock, &buffer, frame_len, 0);
                        }
                    }
                }
            }
        }
    }

    clean(ifaces, IF_count, head);
    return EXIT_SUCCESS;
}

