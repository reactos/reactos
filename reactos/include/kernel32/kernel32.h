#include <windows.h>

#ifdef NDEBUG
#define DPRINT(args...) 
#else
#define DPRINT(args...) do { dprintf("(KERNEL32:%s:%d) ",__FILE__,__LINE__); dprintf(args); } while(0);
#endif

void dprintf(char* fmt, ...);
void aprintf(char* fmt, ...);

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))
#define FIELD_OFFSET(type,fld)	((LONG)&(((type *)0)->fld)) 

BOOL __ErrorReturnFalse(ULONG ErrorCode);
PVOID __ErrorReturnNull(ULONG ErrorCode);
