#include <windows.h>

#define UNIMPLEMENTED dprintf("%s at %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifdef NDEBUG
#define DPRINT(args...) 
#define CHECKPOINT
#else
#define DPRINT(args...) do { dprintf("(KERNEL32:%s:%d) ",__FILE__,__LINE__); dprintf(args); } while(0);
#define CHECKPOINT do { dprintf("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);
#endif

#define DPRINT1(args...) do { dprintf("(KERNEL32:%s:%d) ",__FILE__,__LINE__); dprintf(args);  } while(0);

void dprintf(char* fmt, ...);
void aprintf(char* fmt, ...);

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type,fld)	((LONG)&(((type *)0)->fld))
#endif

BOOL __ErrorReturnFalse(ULONG ErrorCode);
PVOID __ErrorReturnNull(ULONG ErrorCode);

BOOL KERNEL32_AnsiToUnicode(PWSTR DestStr,
			    LPCSTR SrcStr,
			    ULONG MaxLen);
PWSTR InternalAnsiToUnicode(PWSTR Out, LPCSTR In, ULONG MaxLength);
