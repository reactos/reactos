#ifndef _W32KNAPI_H
#define _W32KNAPI_H

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#define WINVER 0x501

#include <windows.h>
#include <wingdi.h>
#include <winddi.h>
#include <ntddk.h>
#include <d3dnthal.h>
#include <prntfont.h>

/* Public Win32K Headers */
#include <win32k/callback.h>
#include <win32k/ntusrtyp.h>
#include <win32k/ntgdityp.h>
#include <win32k/ntgdihdl.h>

#include <ntgdi.h>

#include "../apitest.h"

typedef struct
{
	LPWSTR lpszFunction;
	INT nSyscallNum;
	INT nParams;
} SYCALL_ENTRY, *PSYSCALL_ENTRY;

typedef PGDI_TABLE_ENTRY (CALLBACK * GDIQUERYPROC) (void);

extern HINSTANCE g_hInstance;
extern HMODULE g_hModule;
extern PGDI_TABLE_ENTRY GdiHandleTable;

DWORD Syscall(LPWSTR lpszFunction, int cParams, void* pParams);
BOOL InitOsVersion();

#endif /* _W32KNAPI_H */
