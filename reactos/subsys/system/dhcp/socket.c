#include "rosdhcp.h"

SOCKET ServerSocket;

void SocketInit() {
    ServerSocket = socket( AF_INET, SOCK_DGRAM, 0 );
}

ssize_t send_packet( struct interface_info *ip, 
                     struct dhcp_packet *p,
                     size_t size,
                     struct in_addr addr,
                     struct sockaddr_in *broadcast,
                     struct hardware *hardware ) {
    return 0;
}

ssize_t receive_packet(struct interface_info *ip, 
                       unsigned char *packet_data,
                       size_t packet_len,
                       struct sockaddr_in *dest, 
                       struct hardware *hardware ) {
    return 0;
}
