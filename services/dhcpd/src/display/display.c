#include <headers.h>
#include <datatypes.h>
#include <display.h>
#include <options.h>

int display_dhcp_packet( DHCPMESSAGE *dhcpm, DHCPOPTIONS *dhcpo )
{
  char *mtype;
  if( dhcpm == NULL )
    return -1;

  fprintf( stdout, "op: %s\t|htype: %s\t|hlen: %u\t|hops: %u\n", (dhcpm->op == 1)?"BOOTREQUEST":"BOOTREPLY", (dhcpm->htype==1)?"Ethernet 10Mb":"unknown", dhcpm->hlen, dhcpm->hops);
  fprintf( stdout, "xid: %u \n", dhcpm->xid );
  fprintf( stdout, "secs: %u\t\t|flags: %u\n", dhcpm->secs, dhcpm->flags );
  fprintf( stdout, "ciaddr: %u.%u.%u.%u \n", (dhcpm->ciaddr >> 24), ((dhcpm->ciaddr>>16)&0xFF), ((dhcpm->ciaddr>>8)&0xFF), ((dhcpm->ciaddr)&0xFF));
  fprintf( stdout, "yiaddr: %u.%u.%u.%u \n", (dhcpm->yiaddr >> 24), ((dhcpm->yiaddr>>16)&0xFF), ((dhcpm->yiaddr>>8)&0xFF), ((dhcpm->yiaddr)&0xFF));
  fprintf( stdout, "siaddr: %u.%u.%u.%u \n", (dhcpm->siaddr >> 24), ((dhcpm->siaddr>>16)&0xFF), ((dhcpm->siaddr>>8)&0xFF), ((dhcpm->siaddr)&0xFF));
  fprintf( stdout, "giaddr: %u.%u.%u.%u \n", (dhcpm->giaddr >> 24), ((dhcpm->giaddr>>16)&0xFF), ((dhcpm->giaddr>>8)&0xFF), ((dhcpm->giaddr)&0xFF));
  fprintf( stdout, "chaddr: %X::%X::%X::%X::%X::%X \n", dhcpm->chaddr[0], dhcpm->chaddr[1], dhcpm->chaddr[2], dhcpm->chaddr[3], dhcpm->chaddr[4], dhcpm->chaddr[5] );
  fprintf( stdout, "sname: %s \n", dhcpm->sname );
  fprintf( stdout, "file: %s \n", dhcpm->file );
  /* options come here */
  switch( dhcpo->type )
    {
    case DHCPDISCOVER:
      mtype = (char *)malloc( strlen( "DHCPDISCOVER" ) +1);
      strcpy( mtype, "DHCPDISCOVER" );
      break;
    case DHCPREQUEST:
      mtype = (char *)malloc( strlen( "DHCPREQUEST" ) +1);
      strcpy( mtype, "DHCPREQUEST" );
      break;
    case DHCPACK:
      mtype = (char *)malloc( strlen( "DHCPACK" ) +1);
      strcpy( mtype, "DHCPACK" );
      break;
    case DHCPNAK:
      mtype = (char *)malloc( strlen( "DHCPNAK" ) +1);
      strcpy( mtype, "DHCPNAK" );
      break;
    case DHCPRELEASE:
      mtype = (char *)malloc( strlen( "DHCPRELEASE" ) +1);
      strcpy( mtype, "DHCPRELEASE" );
      break;
    case DHCPDECLINE:
      mtype = (char *)malloc( strlen( "DHCPDECLINE" ) +1);
      strcpy( mtype, "DHCPDECLINE" );
      break;
    case DHCPOFFER:
      mtype = (char *)malloc( strlen( "DHCPOFFER" ) +1);
      strcpy( mtype, "DHCPOFFER" );
      break;
    default:
      mtype = (char *)malloc( strlen("Unknown Type") +1);
      strcpy( mtype, "Unknown Type" );
      break;
    }
  fprintf( stdout, "Message Type: %s \n", mtype );
  return 0;
}

