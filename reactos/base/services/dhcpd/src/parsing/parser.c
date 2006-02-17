#include <headers.h>
#include <datatypes.h>
#include <display.h>
#include <parser.h>
#include <leases.h>
#include <options.h>

int parse_dhcp_options( DHCPMESSAGE *dhcpm, DHCPOPTIONS *dhcpo )
{
  int pointer, opointer;
  int olength;

  pointer = 0;

  fprintf( stdout, "parse_dhcp_options [begin]!\n");

  /* Options Initialization */
  /* No message type */
  dhcpo->type = 0;
  /* No ip address, 0.0.0.0 */
  dhcpo->r_ip = 0;
  /* No mask address, 0.0.0.0 */
  dhcpo->r_mask = 0;
  /* No router, 0.0.0.0 */
  dhcpo->r_router = 0;
  /* No lease 0 */
  dhcpo->r_lease = 0;
  /* No name '\n' */
  dhcpo->hostname = NULL;

  while( pointer< 312 )
    {
      if(( dhcpm->options[0] != 99 ) && (dhcpm->options[1]!=130) && (dhcpm->options[2]!=83) && (dhcpm->options[3]!= 99))
	{
	  fprintf( stdout, "No magic cookie! Aborting! \n" );
	  return -1;
	}
      switch( dhcpm->options[pointer] ){
      case PAD:
	pointer++;
	break;
      case MESSAGETYPE:
	/* Try to figure out the kind of message and start the configuring process */
	pointer += 2;
	dhcpo->type = dhcpm->options[pointer++];
	break;
      case PREQUEST:
	/* Take note of the requested parameters */
	opointer = pointer + 2;
	olength = pointer + dhcpm->options[pointer + 1];
	while( opointer < olength )
	  {
	    switch( dhcpm->options[opointer] ){
	    case IP:
	      /* Take note of the requested ip */
	      opointer += 2;
	      dhcpo->r_ip += dhcpm->options[opointer++];
	      dhcpo->r_ip = dhcpo->r_ip << 8;
	      dhcpo->r_ip += dhcpm->options[opointer++];
	      dhcpo->r_ip = dhcpo->r_ip << 8;
	      dhcpo->r_ip += dhcpm->options[opointer++];
	      dhcpo->r_ip = dhcpo->r_ip << 8;
	      dhcpo->r_ip += dhcpm->options[opointer++];
	      break;
	    case MASK:
	      /* Take note of the requested mask */
	      opointer += 2;
	      dhcpo->r_mask += dhcpm->options[opointer++];
	      dhcpo->r_mask = dhcpo->r_ip << 8;
	      dhcpo->r_mask += dhcpm->options[opointer++];
	      dhcpo->r_mask = dhcpo->r_ip << 8;
	      dhcpo->r_mask += dhcpm->options[opointer++];
	      dhcpo->r_mask = dhcpo->r_ip << 8;
	      dhcpo->r_mask += dhcpm->options[opointer++];
	      break;
	    case ROUTER:
	      /* Take note of the requested router */
	      opointer += 2;
	      dhcpo->r_router += dhcpm->options[opointer++];
	      dhcpo->r_router = dhcpo->r_ip << 8;
	      dhcpo->r_router += dhcpm->options[opointer++];
	      dhcpo->r_router = dhcpo->r_ip << 8;
	      dhcpo->r_router += dhcpm->options[opointer++];
	      dhcpo->r_router = dhcpo->r_ip << 8;
	      dhcpo->r_router += dhcpm->options[opointer++];
	      break;
	    case LEASE:
	      opointer += 2;
	      dhcpo->r_lease += dhcpm->options[opointer++];
	      dhcpo->r_lease = dhcpo->r_ip << 8;
	      dhcpo->r_lease += dhcpm->options[opointer++];
	      dhcpo->r_lease = dhcpo->r_ip << 8;
	      dhcpo->r_lease += dhcpm->options[opointer++];
	      dhcpo->r_lease = dhcpo->r_ip << 8;
	      dhcpo->r_lease += dhcpm->options[opointer++];
	      break;
	    case HOSTNAME:
	      opointer += 1;
	      dhcpo->hostname = (char *)malloc( dhcpm->options[opointer] + 1);
	      strncpy( dhcpo->hostname, &dhcpm->options[opointer+1], dhcpm->options[opointer] );
	      opointer += dhcpm->options[opointer] + 1;
	    default:
	      /* Ignore option */
	      opointer++;
	      break;
	    }
	  }
	pointer = opointer;
	break;
      case  TOFFSET:
      case  TIMESERVER:
      case  NS:
      case  DNS:
      case  LOGSERVER:
      case  COOKIESERVER:
      case  LPRSERVER:
      case  IMPSERVER:
      case  RESLOCSERVER:
      case  BOOTFILESIZE:
      case  MERITDUMPFILE:
      case  DOMAINNAME:
      case  SWAPSERVER:
      case  ROOTPATH:
      case  EXTENSIONPATH:
      case  IPFORWARD:
      case  NONLOCAL:
      case  POLICYFILTER:
      case  MAXIMUMDATAG:
      case  DEFAULTTTL:
      case  PATHMTUATO:
      case  PATHMTUPTO:
      case  IMTU:
      case  ALLSUBLOCAL:
      case  BROADCAST:
      case  PMASKDISCOVERY:
      case  MASKSUPPLIER:
      case  PROUTERDISCOVE:
      case  RSOLICIADDRESS:
      case  STATICROUTE:
      case  TENCAPSULATION:
      case  ARPCACHE:
      case  ETHENCAPSUL:
      case  TCPDEFTTL:
      case  TCPKAI:
      case  TCPKAG:
      case  NISDOMAIN:
      case  NISSERVER:
      case  NTPSERVER:
      case  VENDORSP:
      case  NBTCPIPNS:
      case  NBTCPIPDDS:
      case  NBTCPIPNT:
      case  NBTCPIPSC:
      case  XWINFONTSERVER:
      case  XWINDISPLAY:
      case  OVERLOAD:
      case  SERVER:
      case  MESSAGE:
      case  MAXIMUMDHCP:
      case  RENEWALTIME:
      case  REBINDING:
      case  VENDORCLASS:
      case  NISPLUSDOMAIN:
      case  NISPLUSSERVER:
      case  TFTPSERVER:
      case  BOOTFILE:
      case  MOBILEIP:
      case  SMTPSERVER:
      case  POP3SERVER:
      case  NNTPSERVER:
      case  HTTPSERVER:
      case  FINGERSERVER:
      case  IRCSERVER:
      case  STREETTALKSE:
      case  STREETTALKDA:
      case CLIENT:
	pointer++;
	pointer += dhcpm->options[pointer];
      case END:
	/* return to the calling functions because this is over */
	fprintf( stdout, "parse_dhcp_options: END option found! [end]!\n");
	return 0;
      default:
	/* ignored */
	pointer++;
	break;
      }
    }
  fprintf( stdout, "parse_dhcp_options [end]!\n");
  return 0;
}

int process_dhcp_packet( DHCPMESSAGE *dhcpm, DHCPOPTIONS *dhcpo )
{
  int pointer = 4;
  DHCPLEASE dhcpl;
  char *name;

  fprintf( stdout, "process_dhcp_packet [begin]!\n");

  if( (!dhcpm) || (!dhcpo) )
    return -1;

  name = (char *)malloc( 16 );
  switch( dhcpo->type ){

  case DHCPDISCOVER:
    /* We need to send a DHCPOFFER */
    if( find_lease( &dhcpl, dhcpm->xid, dhcpm->chaddr ) < 0 )
      {
	fprintf( stdout, "No free leases! \n" );
	return -1;
      }
    dhcpm->op = BOOTREPLY;
    dhcpm->yiaddr = dhcpl.ip;
    dhcpm->siaddr = dhcpl.siaddr;
    strcpy(dhcpm->sname, VERSION);
    dhcpm->options[pointer++] = MESSAGETYPE;
    dhcpm->options[pointer++] = 1;
    dhcpm->options[pointer++] = DHCPOFFER;
    dhcpm->options[pointer++] = ROUTER;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.router & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.router >> 8) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.router >> 16) &0xFF);
    dhcpm->options[pointer++] = (dhcpl.router >> 24);
    dhcpm->options[pointer++] = MASK;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.mask & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.mask >> 8) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.mask >> 16) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.mask >> 24);
    dhcpm->options[pointer++] = SERVER;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.siaddr & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.siaddr >> 8) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.siaddr >> 16) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.siaddr >> 24);
    dhcpm->options[pointer++] = LEASE;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.lease >> 24);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.lease & 0xFF);
    dhcpm->options[pointer++] = REBINDING;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.lease >> 24);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.lease & 0xFF) - 5;
    dhcpm->options[pointer++] = RENEWALTIME;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.lease >> 24);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.lease & 0xFF) - 5;
    dhcpm->options[pointer++] = PAD;
    dhcpm->options[pointer++] = PAD;
    dhcpm->options[pointer++] = PAD;
    dhcpm->options[pointer++] = END;
    for( ; pointer < 312; pointer++ )
      dhcpm->options[pointer] = PAD;
    dhcpo->type = DHCPOFFER;
    strcpy( name, "255.255.255.255" );
    break;

  case DHCPREQUEST:
    /* We need to send an DHCPACK */
    dhcpm->op = BOOTREPLY;
    dhcpm->yiaddr = dhcpm->ciaddr;
    strcpy(dhcpm->sname, VERSION);
    if( confirm_lease( &dhcpl, dhcpm->xid ) < 0)
      {
	dhcpm->options[pointer++] = MESSAGETYPE;
	dhcpm->options[pointer++] = 1;
	dhcpm->options[pointer++] = DHCPNAK;
	dhcpm->options[pointer++] = PAD;
	dhcpm->options[pointer++] = END;
	for( ; pointer < 312; pointer++ )
	  dhcpm->options[pointer] = PAD;
	sprintf( name, "%u.%u.%u.%u", (dhcpm->ciaddr &0xFF), ((dhcpm->ciaddr>>8)&0xFF), ((dhcpm->ciaddr>>16)&0xFF), ((dhcpm->ciaddr>>24)&0xFF));
	display_dhcp_packet( dhcpm, dhcpo );
	write_packet( dhcpm, name );
	return -1;
      }
    dhcpm->siaddr = dhcpl.siaddr;
    dhcpm->options[pointer++] = MESSAGETYPE;
    dhcpm->options[pointer++] = 1;
    dhcpm->options[pointer++] = DHCPACK;
    dhcpm->options[pointer++] = ROUTER;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.router >> 24);
    dhcpm->options[pointer++] = ((dhcpl.router >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.router >> 8) &0xFF);
    dhcpm->options[pointer++] = (dhcpl.router & 0xFF);
    dhcpm->options[pointer++] = MASK;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.mask >> 24);
    dhcpm->options[pointer++] = ((dhcpl.mask >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.mask >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.mask & 0xFF);
    dhcpm->options[pointer++] = SERVER;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = dhcpl.siaddr >> 24;
    dhcpm->options[pointer++] = ((dhcpl.siaddr >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.siaddr >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.siaddr & 0xFF);
    dhcpm->options[pointer++] = LEASE;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.lease >> 24);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.lease & 0xFF);
    dhcpm->options[pointer++] = REBINDING;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.lease >> 24);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.lease & 0xFF);
    dhcpm->options[pointer++] = RENEWALTIME;
    dhcpm->options[pointer++] = 4;
    dhcpm->options[pointer++] = (dhcpl.lease >> 24);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 16) & 0xFF);
    dhcpm->options[pointer++] = ((dhcpl.lease >> 8) & 0xFF);
    dhcpm->options[pointer++] = (dhcpl.lease & 0xFF);
    dhcpm->options[pointer++] = PAD;
    dhcpm->options[pointer++] = PAD;
    dhcpm->options[pointer++] = PAD;
    dhcpm->options[pointer++] = END;
    for( ; pointer < 312; pointer++ )
      dhcpm->options[pointer] = PAD;
    dhcpo->type = DHCPACK;
    sprintf( name, "%u.%u.%u.%u", (dhcpl.ip & 0xFF), ((dhcpl.ip>>8) & 0xFF), ((dhcpl.ip>>16)&0xFF), (dhcpl.ip>>24));
    break;

  default:
    break;
  }
  display_dhcp_packet( dhcpm, dhcpo );
  write_packet( dhcpm, name );
  fprintf( stdout, "process_dhcp_packet [end]!\n");
  return 0;
}

int write_packet( DHCPMESSAGE *dhcpm, char *name )
{
  int sockfd;
  struct sockaddr_in their_addr; // connector's address information
  struct hostent *he;
  int numbytes;
  int enable = 1;

  fprintf( stdout, "write_packet [begin]\n" );

  if( !dhcpm )
    return -1;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  if( setsockopt( sockfd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof( enable )) == -1 )
    {
      perror("setsockopt");
      exit(1);
    }

  if( strcmp( "255.255.255.255", name ) )
    {
      if ((he=gethostbyname(name)) == NULL) {  // get the host info
	perror("gethostbyname");
	fprintf( stdout, "Unknown host %s \n", name );
	return -1;
      }
      their_addr.sin_family = AF_INET;     // host byte order
      their_addr.sin_port = htons(68); // short, network byte order
      their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    } else {
      their_addr.sin_family = AF_INET;     // host byte order
      their_addr.sin_port = htons(68); // short, network byte order
      their_addr.sin_addr.s_addr = 0xFFFFFFFF;
    }

  fprintf( stdout, "IP a buscar: %s \n", name );
  memset(&(their_addr.sin_zero), '\0', 8); // zero the rest of the struct

  if ((numbytes=sendto(sockfd, dhcpm, sizeof(DHCPMESSAGE), 0,
		       (struct sockaddr *)&their_addr, sizeof(struct sockaddr))) == -1) {
    perror("sendto");
    exit(1);
  }

  printf("sent %d bytes to %s\n", numbytes,
	 inet_ntoa(their_addr.sin_addr));

  close(sockfd);

  fprintf( stdout, "write_packet [end]\n" );

  return 0;
}
