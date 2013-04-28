#include <headers.h>
#include <datatypes.h>
#include <options.h>
#include <display.h>
#include <leases.h>
#include <parser.h>

#define MYPORT  67
DHCPLIST *list;
/*DHCPLIST *leased_list;*/

int main( int argc, char *argv[] )
{
#ifdef __MINGW32__
  WSADATA wsaData;
  int nCode;
#endif
  int sockfd;
  struct sockaddr_in my_addr;
  struct sockaddr_in their_addr;
  int addr_len, numbytes;
  DHCPMESSAGE dhcpm;
  DHCPOPTIONS dhcpo;

#ifdef __MINGW32__
  if ((nCode = WSAStartup(MAKEWORD(1, 1), &wsaData)) != 0)
    {
      perror("WSAStartup");
      return 0;
    }
#endif

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    perror("socket");
    exit(1);
  }

  init_leases_list();

  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(MYPORT);
  my_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(my_addr.sin_zero), '\0', 8);

  if (bind(sockfd, (struct sockaddr *)&my_addr,
	   sizeof(struct sockaddr)) == -1) {
    perror("bind");
    exit(1);
  }

  addr_len = sizeof(struct sockaddr);
  while((numbytes=recvfrom(sockfd,&dhcpm, sizeof( DHCPMESSAGE ), 0,
			 (struct sockaddr *)&their_addr, &addr_len)) != -1) {
    /* Parse DHCP */
    display_dhcp_packet( &dhcpm, &dhcpo );
    if( parse_dhcp_options( &dhcpm, &dhcpo ) < 0 )
      continue;
    if( display_dhcp_packet( &dhcpm, &dhcpo ) < 0 )
      continue;
    if( process_dhcp_packet( &dhcpm, &dhcpo ) < 0 )
      continue;
  }

  close(sockfd);

#ifdef __MINGW32__
  WSACleanup();
#endif

  return 0;

}
