#ifndef _W32KNAPI_H
#define _W32KNAPI_H

#include "../apitest.h"
#include <ddk/winddi.h>

#define OS_UNSUPPORTED 0
#define OS_REACTOS	1
#define OS_WINDOWS	2

#define W32KAPI

typedef struct
{
	LPWSTR lpszFunction;
	INT nSyscallNum;
	INT nParams;
} SYCALL_ENTRY, *PSYSCALL_ENTRY;

extern HINSTANCE g_hInstance;
extern SYCALL_ENTRY SyscallTable_XP_2600[];
extern HMODULE g_hModule;
extern INT g_nOsVer;

DWORD Syscall(LPWSTR lpszFunction, int cParams, void* pParams);
BOOL InitOsVersion();

#endif /* _W32KNAPI_H */
