#include <headers.h>
#include <datatypes.h>
#include <display.h>
#include <options.h>
#include <leases.h>
#include <utils.h>

int init_leases_list()
{
  DHCPLIST *temp;
  int i, j;
  u8b chaddr[16];
  FILE *config;
  char line[80];
  char textsubnet[16];
  char textlease[5];
  char textrouter[16];
  char textmask[16];
  char textlowrange[4], texthighrange[4];
  char textserver[16];
  u8b ip0, ip1, ip2, ip3;
  u8b lowrange, highrange;
  u8b textmac[17], textip[16];
  u32b lease;

  /* Be nice variables and behave yourselves */

  for( j = 0; j < 16; j++ )
    {
      chaddr[j] = 0;
      textsubnet[j] = 0;
      textrouter[j] = 0;
      textmask[j] = 0;
      textip[j] = 0;
      textmac[j] = 0;
    }

  textlease[0] = 0;
  textlowrange[0] = 0;
  texthighrange[0] = 0;

  /* Now we can read our configuration file */

  config = fopen( "dhcp.conf", "r" );
  if( !config )
    {
      perror("Reading config files");
      exit( 0 );
    }

  /* We _DO_ need a better parser */
  list = (DHCPLIST *)malloc( sizeof( DHCPLIST ));
  temp = list;
  temp->back = NULL;

  while( (!feof( config )) && ( line ))
    {
      fscanf( config, "%s", line);
      if( !strcmp( line, "subnet" ))
	/* Read subnet parameters */
	fscanf( config, "%s", textsubnet );

      if( !strcmp( line, "lease" ))
	/* read lease parameters */
	fscanf( config, "%s", textlease );

      if( !strcmp( line, "router" ))
	fscanf( config, "%s", textrouter );

      if( !strcmp( line, "mask" ))
	fscanf( config, "%s", textmask );

      if( !strcmp( line, "range" ))
	fscanf( config, "%s %s", textlowrange, texthighrange );
      if( !strcmp( line, "server" ))
	fscanf( config, "%s", textserver );
      if( !strcmp( line, "host" ))
	{
	  /* Host Specific Configuration */
	  fscanf( config, "%s %s", textmac, textip );
	  str2mac( textmac, temp->chaddr );
	  temp->type = STATIC;
	  temp->data.ip = inet_addr( textip );
	  temp->next = (DHCPLIST *)malloc( sizeof( DHCPLIST ));
	  temp->next->back = temp;
	  temp = temp->next;
	  temp->next =NULL;
	}
    }
  fclose( config );

  lowrange = (u8b)atoi( textlowrange );
  highrange = (u8b)atoi( texthighrange );
  lease = (u32b)atoi( textlease );

  /* Creating Static IP */

  for( temp = list; temp->next; temp = temp->next )
    {
      temp->available = FREE;
      temp->xid = 0;
      temp->data.router = inet_addr( textrouter );
      temp->data.mask = inet_addr( textmask );
      temp->data.lease = lease;
      temp->data.siaddr = inet_addr( textserver );
    }

  /* Creating Dynamic IP */

  for( i = lowrange; i < (highrange + 1); i++ )
    {
      temp->available = FREE;
      temp->xid = 0;
      temp->type = DYNAMIC;
      maccpy( temp->chaddr, chaddr );
      split_ip( textsubnet, &ip0, 0 );
      split_ip( textsubnet, &ip1, 1 );
      split_ip( textsubnet, &ip2, 2 );
      temp->data.ip = i;
      temp->data.ip = temp->data.ip << 8;
      temp->data.ip += ip2;
      temp->data.ip = temp->data.ip << 8;
      temp->data.ip += ip1;
      temp->data.ip = temp->data.ip << 8;
      temp->data.ip += ip0;
      temp->data.router = inet_addr( textrouter );
      temp->data.mask = inet_addr( textmask );
      temp->data.lease = lease;
      temp->data.siaddr = inet_addr( textserver );
      temp->next = (DHCPLIST *)malloc( sizeof( DHCPLIST ));
      temp->next->back = temp;
      temp = temp->next;
    }
  return 0;
}

int find_lease( DHCPLEASE *dhcpl, u32b xid, u8b chaddr[] )
{
  int result = -2;
  DHCPLIST *temp;

  if( !dhcpl )
    return -1;

  for( temp = list; temp; temp=temp->next )
    if( !maccmp( temp->chaddr, chaddr ) )
      release_lease( dhcpl, xid, chaddr);

  for( temp = list; temp; temp=temp->next )
    if( ( !maccmp( temp->chaddr, chaddr )) && ( temp->type == STATIC ))
      {
	dhcpl->ip = temp->data.ip;
	dhcpl->router = temp->data.router;
	dhcpl->mask = temp->data.mask;
	dhcpl->lease = temp->data.lease;
	dhcpl->siaddr = temp->data.siaddr;
	fprintf( stdout, "Assigning Static IP! \n");
	temp->available = PROCESSING;
	temp->xid = xid;
	temp->ltime = MAX_PROCESS_TIME;
	maccpy( temp->chaddr, chaddr);
	result = 0;
	return result;
      }
    else if( ( temp->available & FREE )  && ( temp->type == DYNAMIC ))
      {
	dhcpl->ip = temp->data.ip;
	dhcpl->router = temp->data.router;
	dhcpl->mask = temp->data.mask;
	dhcpl->lease = temp->data.lease;
	dhcpl->siaddr = temp->data.siaddr;
	fprintf( stdout, "Assigning Dynamic IP! \n");
	temp->available = PROCESSING;
	temp->xid = xid;
	temp->ltime = MAX_PROCESS_TIME;
	maccpy( temp->chaddr, chaddr);
	result = 0;
	return result;
      }
  return result;
}

int confirm_lease( DHCPLEASE *dhcpl, u32b xid )
{
  int result = -1;
  DHCPLIST *temp;

  for( temp = list; temp; temp=temp->next )
    if( temp->xid == xid )
      {
	dhcpl->ip = temp->data.ip;
	dhcpl->router = temp->data.router;
	dhcpl->mask = temp->data.mask;
	dhcpl->lease = temp->data.lease;
	dhcpl->siaddr = temp->data.siaddr;
	temp->available = BUSY;
	temp->ltime = temp->data.lease;
	result = 0;
	return result;
      }
  return result;
}

int release_lease( DHCPLEASE *dhcpl, u32b xid, u8b chaddr[16] )
{
  int result = -1, i;
  DHCPLIST *temp;
  u8b nchaddr[16];

  for( i = 0; i < 16; i++ )
    nchaddr[i] = 0;

  if( !dhcpl )
    return -1;

  for( temp = list; temp; temp=temp->next )
    if( !maccmp( temp->chaddr, chaddr ) )
      {
	/* We found the address */
	result = 0;
	fprintf( stdout, "Deleting %X::%X::%X::%X::%X::%X \n", temp->chaddr[0], temp->chaddr[1], temp->chaddr[2], temp->chaddr[3], temp->chaddr[4], temp->chaddr[5] );
	temp->available = FREE;
	temp->xid = 0;
	/*	maccpy( temp->chaddr, nchaddr ); */
      } else {
	/* No such address */
	result = -1;
      }

  return result;
}
