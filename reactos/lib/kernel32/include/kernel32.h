#ifndef _KERNEL32_INCLUDE_KERNEL32_H
#define _KERNEL32_INCLUDE_KERNEL32_H

#define BINARY_UNKNOWN	(0)
#define BINARY_PE_EXE32	(1)
#define BINARY_PE_DLL32	(2)
#define BINARY_PE_EXE64	(3)
#define BINARY_PE_DLL64	(4)
#define BINARY_WIN16	(5)
#define BINARY_OS216	(6)
#define BINARY_DOS	(7)
#define BINARY_UNIX_EXE	(8)
#define BINARY_UNIX_LIB	(9)

#define  MAGIC(c1,c2,c3,c4)  ((c1) + ((c2)<<8) + ((c3)<<16) + ((c4)<<24))

#define  MAGIC_HEAP        MAGIC( 'H','E','A','P' )

#define ROUNDUP(a,b)	((((a)+(b)-1)/(b))*(b))
#define ROUNDDOWN(a,b)	(((a)/(b))*(b))

#ifndef FIELD_OFFSET
#define FIELD_OFFSET(type,fld)	((LONG)&(((type *)0)->fld))
#endif

#define SetLastErrorByStatus(__S__) \
 ((void)SetLastError(RtlNtStatusToDosError(__S__)))

typedef 
DWORD
(*WaitForInputIdleType)(
    HANDLE hProcess,
    DWORD dwMilliseconds);

/* GLOBAL VARIABLES **********************************************************/

extern BOOL bIsFileApiAnsi;
extern HANDLE hProcessHeap;
extern HANDLE hBaseDir;

extern CRITICAL_SECTION DllLock;

/* FUNCTION PROTOTYPES *******************************************************/

BOOL STDCALL IsConsoleHandle(HANDLE Handle);

BOOL STDCALL CloseConsoleHandle(HANDLE Handle);

HANDLE STDCALL OpenConsoleW (LPWSTR wsName,
			     DWORD  dwDesiredAccess,
			     BOOL   bInheritHandle,
			     DWORD  dwCreationDistribution);

PTEB GetTeb(VOID);

#endif /* ndef _KERNEL32_INCLUDE_KERNEL32_H */

