#ifndef _OSKITFREEBSD_H
#define _OSKITFREEBSD_H

#include <ddk/ntddk.h>

#ifdef linux
#include <netinet/in.h>
#endif

extern void oskittcp_die(const char *file, int line);

#define printf DbgPrint
#define vprintf DbgVPrint 
#define ovbcopy(x,y,z) bcopy(x,y,z)
void *memset( void *dest, int c, size_t count );
#define bzero(x,y) memset(x,0,y)
#define bcopy(src,dst,n) memcpy(dst,src,n)
#ifdef _MSC_VER
static inline void panic ( const char* fmt, ... )
{
	va_list arg;
	va_start(arg, fmt);
	printf ( "oskit PANIC: " );
	vprintf ( fmt, arg );
	va_end(arg);
	// TODO FIXME - print stack trace...
	oskittcp_die("<unknown file>",-1);
}
#else//_MSC_VER
#define panic(...) do { printf(__VA_ARGS__); \
        oskittcp_die(__FILE__,__LINE__); } while(0)
#endif//_MSC_VER
#define kmem_malloc(x,y,z) malloc(y)

#endif//_OSKITFREEBSD_H
