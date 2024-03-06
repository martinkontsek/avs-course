#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h>
#include <net/if.h>

#include <sys/select.h>

#define MTU 1500

struct eth_frame
{
  uint8_t dst_mac[6];    
  uint8_t src_mac[6];
  uint16_t ethertype;
  char payload[MTU]; 
}__attribute__((packed)); 

struct interface
{
  char name[20];
  unsigned int if_index;
  int sock;
};

struct cam_item
{
  struct cam_item *prev;
  struct cam_item *next;
  uint8_t mac[6];
  struct interface *iface;
};

struct cam_item * 
create_cam_table()
{
  struct cam_item *head;
  head = calloc(1, sizeof(struct cam_item));
  return head;
}

struct cam_item *
add_item(struct cam_item *head, uint8_t *mac, struct interface *iface)
{
  if(head == NULL || mac == NULL || iface == NULL)
    return NULL;

  //create and fill entry
  struct cam_item *entry;
  entry = calloc(1, sizeof(struct cam_item));
  entry->iface = iface;
  memcpy(entry->mac, mac, 6);

  //insert entry into the table
  entry->prev = head;
  entry->next = head->next;
  head->next = entry;

  if(entry->next != NULL)
    entry->next->prev = entry;

  return entry;
}

struct cam_item *
find_cam_item(struct cam_item *head, uint8_t *mac)
{
  if(head == NULL || mac == NULL)
    return NULL;

  struct cam_item *current_item;
  current_item = head->next;
  while(current_item != NULL)
  {
    //compare MACs
    if(memcmp(current_item->mac, mac, 6) == 0)
      return current_item;

    current_item = current_item->next;
  }

  return NULL;
}

void
print_cam(struct cam_item *head)
{
  if(head == NULL)
  {
    fprintf(stderr, "CAM table doesn't exist.\n");  
    return;
  }

  printf("|        MAC        |%20s|\n", "IFACE");
  struct cam_item *current_item;
  current_item = head->next;
  while(current_item != NULL)
  {
    print("| %hhx:%hhx:%hhx:%hhx:%hhx:%hhx |%20s|\n",
          current_item->mac[0],
          current_item->mac[1],
          current_item->mac[2],
          current_item->mac[3],
          current_item->mac[4],
          current_item->mac[5],
          current_item->iface->name);

    current_item = current_item->next;
  }
}

int main(int argc, char **argv)
{
  if(argc < 3)
  {
    fprinf(stderr,"USAGE: %s iface_name iface_name...", argv[0]);
    exit(EXIT_FAILURE);
  }  

  struct interface ifaces[argc-1];
  struct sockaddr_ll addr;
  int i;
  int highest_sock;
  highest_sock = 0;
  for(i=1;i<argc;i++)
  {
    strcpy(ifaces[i-1].name, argv[i]);
    if((ifaces[i-1].if_index = if_nametoindex(argv[i])) == 0)
    {
      perror("IF_NAME");
      //TODO: clean created sockets
      exit(EXIT_FAILURE);
    }

    if((ifaces[i-1].sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
    {
      perror("SOCKET");
      //TODO: clean created sockets
      exit(EXIT_FAILURE);
    }

    if(highest_sock < ifaces[i-1].sock)
      highest_sock = ifaces[i-1].sock;

    bzero(&addr, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = ifaces[i-1].if_index;

    if(bind(ifaces[i-1].sock, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    {
      perror("BIND");
      //TODO: clean created sockets
      exit(EXIT_FAILURE);
    }
  }

  //create CAM table
  struct cam_item *head;
  if((head = create_cam_table()) == NULL)
  {
    fprintf(stderr, "ERROR: Cannot create CAM table.\n");
    //TODO: clean created sockets
    exit(EXIT_FAILURE);
  }

  struct eth_frame buffer;
  highest_sock++;
  while(1)
  {
      fd_set read_set;
      FD_ZERO(&read_set);
      
      if(select(highest_sock, &read_set, NULL, NULL, NULL) == -1)
      {
        perror("SELECT");
        continue;
      }

      for(i=0;i<argc-1;i++)
      { 
        if(FD_ISSET(ifaces[i].sock, &read_set) != 0)
        {
          //this socket has received frame
          bzero(&buffer, sizeof(buffer));
          long frame_size;
          if((frame_size = read(ifaces[i].sock, &buffer, sizeof(buffer))) == -1)
            continue;

          //learning
          if(find_cam_item(head, buffer.src_mac) == NULL)
          {
            //mac in not in table - insert
            if(add_item(head, buffer.src_mac, &ifaces[i]) == NULL)
              fprintf(stderr,"WARNING: Cannot add MAC to CAM.\n");
          }

          //forwarding
          struct cam_item *entry;
          if((entry = find_cam_item(head, buffer.dst_mac)) != NULL)
          {
            //mac in table - forward via one interface
            if(write(entry->iface->sock, &buffer, frame_size) == -1)
              continue;
          } else {
            //mac not in table - forward via all except receiving port
            int j;
            for(j=0;j<argc-1;j++)
            {
              if(ifaces[j].if_index != ifaces[i].if_index)
                if(write(ifaces[j].sock, &buffer, frame_size) == -1)
                  continue;
            }
          }
        }
      }
  }

  //TODO: clean created sockets
  return EXIT_SUCCESS;
}