#include <stdarg.h>
#include "rosdhcp.h"

#define NDEBUG
#include <reactos/debug.h>

char *piaddr( struct iaddr addr ) {
    struct sockaddr_in sa;
    memcpy(&sa.sin_addr,addr.iabuf,sizeof(sa.sin_addr));
    return inet_ntoa( sa.sin_addr );
}

int note( char *format, ... ) {
    va_list arg_begin;
    va_start( arg_begin, format );
    char buf[0x100];
    int ret;

    ret = vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("NOTE: %s\n", buf);

    return ret;
}

int debug( char *format, ... ) {
    va_list arg_begin;
    va_start( arg_begin, format );
    char buf[0x100];
    int ret;

    ret = vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("DEBUG: %s\n", buf);

    return ret;
}

int warn( char *format, ... ) {
    va_list arg_begin;
    va_start( arg_begin, format );
    char buf[0x100];
    int ret;

    ret = vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("WARN: %s\n", buf);

    return ret;
}

int warning( char *format, ... ) {
    va_list arg_begin;
    va_start( arg_begin, format );
    char buf[0x100];
    int ret;

    ret = vsnprintf( buf, sizeof(buf), format, arg_begin );

    DPRINT("WARNING: %s\n", buf);

    return ret;
}

void error( char *format, ... ) {
    va_list arg_begin;
    va_start( arg_begin, format );
    char buf[0x100];

    vsnprintf( buf, sizeof(buf), format, arg_begin );

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
void dfree( void *v, char *name ) { free( v ); }

int read_client_conf(void) {
       error("util.c read_client_conf not implemented!");
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
