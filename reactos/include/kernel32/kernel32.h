#ifndef _INCLUDE_KERNEL32_KERNEL32_H
#define _INCLUDE_KERNEL32_KERNEL32_H

#include <windows.h>

#define UNIMPLEMENTED DbgPrint("%s at %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifdef NDEBUG
#define DPRINT(args...)
#define CHECKPOINT
#ifdef assert
#undef assert
#endif
#define assert(x)
#else
#define DPRINT(args...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);
#ifdef assert
#undef assert
#endif
#define assert(x) do { if(!x) RtlAssert(x, __FILE__,__LINE__, ""); } while(0);
#endif

#define DPRINT1(args...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT1 do { DbgPrint("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type,fld)	((LONG)&(((type *)0)->fld))
#endif

/* GLOBAL VARIABLES **********************************************************/

extern WINBOOL bIsFileApiAnsi;
extern HANDLE hProcessHeap;
extern HANDLE hBaseDir;

extern CRITICAL_SECTION DllLock;

/* FUNCTION PROTOTYPES *******************************************************/

BOOLEAN STDCALL IsConsoleHandle(HANDLE Handle);

WINBOOL STDCALL CloseConsoleHandle(HANDLE Handle);

HANDLE STDCALL OpenConsoleW (LPWSTR                 wsName,
			     DWORD                  dwDesiredAccess,
			     LPSECURITY_ATTRIBUTES  lpSecurityAttributes OPTIONAL,
			     DWORD                  dwCreationDistribution);


#endif /* ndef _INCLUDE_KERNEL32_KERNEL32_H */

