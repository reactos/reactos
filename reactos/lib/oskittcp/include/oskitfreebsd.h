#ifndef _OSKITFREEBSD_H
#define _OSKITFREEBSD_H

extern void oskittcp_die(const char *file, int line);

#define printf DbgPrint
#define ovbcopy(x,y,z) bcopy(x,y,z)
#define bzero(x,y) memset(x,0,y)
#define bcopy memcpy
#define panic(...) do { DbgPrint(__VA_ARGS__); \
       oskittcp_die(__FILE__,__LINE__); } while(0)
#define kmem_malloc(x,y,z) malloc(y)

#endif//_OSKITFREEBSD_H
