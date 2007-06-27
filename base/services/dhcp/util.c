#include <stdarg.h>
#include "rosdhcp.h"

#define NDEBUG
#include <reactos/debug.h>

extern struct interface_info *ifi;

char *piaddr( struct iaddr addr ) {
    struct sockaddr_in sa;
    memcpy(&sa.sin_addr,addr.iabuf,sizeof(sa.sin_addr));
    return inet_ntoa( sa.sin_addr );
}

int note( char *format, ... ) {
    char buf[0x100];
    int ret;
    va_list arg_begin;
    va_start( arg_begin, format );

    ret = _vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("NOTE: %s\n", buf);

    return ret;
}

int debug( char *format, ... ) {
    char buf[0x100];
    int ret;
    va_list arg_begin;
    va_start( arg_begin, format );

    ret = _vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("DEBUG: %s\n", buf);

    return ret;
}

int warn( char *format, ... ) {
    char buf[0x100];
    int ret;
    va_list arg_begin;
    va_start( arg_begin, format );

    ret = _vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("WARN: %s\n", buf);

    return ret;
}

int warning( char *format, ... ) {
    char buf[0x100];
    int ret;
    va_list arg_begin;
    va_start( arg_begin, format );

    ret = _vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("WARNING: %s\n", buf);

    return ret;
}

void error( char *format, ... ) {
    char buf[0x100];
    va_list arg_begin;
    va_start( arg_begin, format );

    _vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT1("ERROR: %s\n", buf);
}

int16_t getShort( unsigned char *data ) {
    return (int16_t) ntohs(*(int16_t*) data);
}

u_int16_t getUShort( unsigned char *data ) {
    return (u_int16_t) ntohs(*(u_int16_t*) data);
}

int32_t getLong( unsigned char *data ) {
       return (int32_t) ntohl(*(u_int32_t*) data);
}

u_int32_t getULong( unsigned char *data ) {
       return ntohl(*(u_int32_t*)data);
}

int addr_eq( struct iaddr a, struct iaddr b ) {
    return a.len == b.len && !memcmp( a.iabuf, b.iabuf, a.len );
}

void *dmalloc( int size, char *name ) { return malloc( size ); }

int read_client_conf(void) {
       /* What a strage dance */
       struct client_config *config;
       char ComputerName [MAX_COMPUTERNAME_LENGTH + 1];
       LPSTR lpCompName;
       DWORD ComputerNameSize = sizeof ComputerName / sizeof ComputerName[0];

       if ((ifi!= NULL) && (ifi->client->config != NULL))
          config = ifi->client->config;
       else 
       {
           warn("util.c read_client_conf poorly implemented!");
           return 0;
       }
       
           
       GetComputerName(ComputerName, & ComputerNameSize);
       /* This never gets freed since it's only called once */
       lpCompName =
       HeapAlloc(GetProcessHeap(), 0, strlen(ComputerName) + 1);
       if (lpCompName !=NULL) {
           GetComputerName(lpCompName, & ComputerNameSize);
           /* Send our hostname, some dhcpds use this to update DNS */
           config->send_options[DHO_HOST_NAME].data = (u_int8_t*)strlwr(lpCompName);
           config->send_options[DHO_HOST_NAME].len = strlen(ComputerName);
           debug("Hostname: %s, length: %d",
                 config->send_options[DHO_HOST_NAME].data,
                 config->send_options[DHO_HOST_NAME].len);
       } else {
           error("Failed to allocate heap for hostname");
       }
       /* Both Linux and Windows send this */
       config->send_options[DHO_DHCP_CLIENT_IDENTIFIER].data =
             ifi->hw_address.haddr;
       config->send_options[DHO_DHCP_CLIENT_IDENTIFIER].len =
             ifi->hw_address.hlen;

       /* Setup the requested option list */
       config->requested_options
           [config->requested_option_count++] = DHO_SUBNET_MASK;
       config->requested_options
           [config->requested_option_count++] = DHO_BROADCAST_ADDRESS;
       config->requested_options
           [config->requested_option_count++] = DHO_TIME_OFFSET;
       config->requested_options
           [config->requested_option_count++] = DHO_ROUTERS;
       config->requested_options
           [config->requested_option_count++] = DHO_DOMAIN_NAME;
       config->requested_options
           [config->requested_option_count++] = DHO_DOMAIN_NAME_SERVERS;
       config->requested_options
           [config->requested_option_count++] = DHO_HOST_NAME;
       config->requested_options
           [config->requested_option_count++] = DHO_NTP_SERVERS;

       warn("util.c read_client_conf poorly implemented!");
    return 0;
}

struct iaddr broadcast_addr( struct iaddr addr, struct iaddr mask ) {
    struct iaddr bcast = { 0 };
    return bcast;
}

struct iaddr subnet_number( struct iaddr addr, struct iaddr mask ) {
    struct iaddr bcast = { 0 };
    return bcast;
}
