#ifndef _OSKITFREEBSD_H
#define _OSKITFREEBSD_H

#ifdef linux
#include <netinet/in.h>
#endif

extern void oskittcp_die(const char *file, int line);

#ifdef _MSC_VER
#define DbgPrint printf
#define DbgVPrint vprintf
#else//_MSC_VER
#define printf DbgPrint
#endif//_MSC_VER
#define ovbcopy(x,y,z) bcopy(x,y,z)
void *memset( void *dest, int c, size_t count );
#define bzero(x,y) memset(x,0,y)
#define bcopy(src,dst,n) memcpy(dst,src,n)
#ifdef _MSC_VER
static inline void panic ( const char* fmt, ... )
{
	va_list arg;
	va_start(arg, fmt);
	DbgPrint ( "oskit PANIC: " );
	DbgVPrint ( fmt, arg );
	va_end(arg);
	// TODO FIXME - print stack trace...
	oskittcp_die("<unknown file>",-1);
}
#else//_MSC_VER
#define panic(...) do { DbgPrint(__VA_ARGS__); \
        oskittcp_die(__FILE__,__LINE__); } while(0)
#endif//_MSC_VER
#define kmem_malloc(x,y,z) malloc(y)

#endif//_OSKITFREEBSD_H
