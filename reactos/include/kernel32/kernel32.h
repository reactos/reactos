#ifndef _INCLUDE_KERNEL32_KERNEL32_H
#define _INCLUDE_KERNEL32_KERNEL32_H

#include <windows.h>

#define UNIMPLEMENTED DbgPrint("%s at %s:%d is unimplemented\n",__FUNCTION__,__FILE__,__LINE__);

#ifdef NDEBUG
#define DPRINT(args...) 
#define CHECKPOINT
#else
#define DPRINT(args...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(args); } while(0);
#define CHECKPOINT do { DbgPrint("(KERNEL32:%s:%d) Checkpoint\n",__FILE__,__LINE__); } while(0);
#endif

#define DPRINT1(args...) do { DbgPrint("(KERNEL32:%s:%d) ",__FILE__,__LINE__); DbgPrint(args);  } while(0);

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type,fld)	((LONG)&(((type *)0)->fld))
#endif


/* GLOBAL VARIABLES ***********************************************************/

extern WINBOOL bIsFileApiAnsi;


/* FUNCTION PROTOTYPES ********************************************************/

BOOL __ErrorReturnFalse(ULONG ErrorCode);
PVOID __ErrorReturnNull(ULONG ErrorCode);

BOOL KERNEL32_AnsiToUnicode(PWSTR DestStr,
			    LPCSTR SrcStr,
			    ULONG MaxLen);
PWSTR InternalAnsiToUnicode(PWSTR Out, LPCSTR In, ULONG MaxLength);

BOOLEAN STDCALL IsConsoleHandle(HANDLE Handle);

WINBOOL STDCALL CloseConsoleHandle(HANDLE Handle);

#endif /* ndef _INCLUDE_KERNEL32_KERNEL32_H */
