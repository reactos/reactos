#ifndef _W32KNAPI_H
#define _W32KNAPI_H

/* SDK/NDK Headers */
#define NTOS_MODE_USER
#define WIN32_NO_STATUS
#include <windows.h>
#include <winuser.h>
#include <windowsx.h>
#include <winnls32.h>
#include <ndk/ntndk.h>
#include <wingdi.h>
#include <winddi.h>
#include <d3dnthal.h>
#include <prntfont.h>

/* Public Win32K Headers */
#include <win32k/ntusrtyp.h>
#include <win32k/ntuser.h>
#include <win32k/callback.h>
#include <win32k/ntgdityp.h>
#include <ntgdi.h>
#include <win32k/ntgdihdl.h>

#include "../apitest.h"
#include "resource.h"

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
