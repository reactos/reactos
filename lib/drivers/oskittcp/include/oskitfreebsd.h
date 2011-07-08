#ifndef _OSKITFREEBSD_H
#define _OSKITFREEBSD_H

// Hacky? Yep
#undef PAGE_SIZE
#undef PAGE_SHIFT
#include <ntddk.h>

#ifdef linux
#include <netinet/in.h>
#endif

extern void oskittcp_die(const char *file, int line);

#define printf DbgPrint
#define vprintf DbgVPrint
#define ovbcopy(src,dst,n) memmove(dst,src,n)
#define bzero(x,y) memset(x,0,y)
#define bcopy(src,dst,n) memcpy(dst,src,n)
#define panic(...) do { printf(__VA_ARGS__); \
        oskittcp_die(__FILE__,__LINE__); } while(0)
#define kmem_malloc(x,y,z) malloc(y,0,0)

#endif//_OSKITFREEBSD_H
