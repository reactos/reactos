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
#include <ddrawi.h>
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

extern HINSTANCE g_hInstance;
extern HMODULE g_hModule;
extern PGDI_TABLE_ENTRY GdiHandleTable;

BOOL IsHandleValid(HGDIOBJ hobj);
DWORD Syscall(LPWSTR lpszFunction, int cParams, void* pParams);
BOOL InitOsVersion();
extern UINT g_OsIdx;

typedef UINT ASPI[5];
extern ASPI gNOPARAM_ROUTINE_CREATEMENU;
extern ASPI gNOPARAM_ROUTINE_CREATEMENUPOPUP;
extern ASPI gNOPARAM_ROUTINE_LOADUSERAPIHOOK;
extern ASPI gONEPARAM_ROUTINE_MAPDEKTOPOBJECT;
extern ASPI gONEPARAM_ROUTINE_SWAPMOUSEBUTTON;
extern ASPI gHWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW;
extern ASPI gHWND_ROUTINE_GETWNDCONTEXTHLPID;
extern ASPI gHWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID;

#define _NOPARAM_ROUTINE_CREATEMENU gNOPARAM_ROUTINE_CREATEMENU[g_OsIdx]
#define _NOPARAM_ROUTINE_CREATEMENUPOPUP gNOPARAM_ROUTINE_CREATEMENUPOPUP[g_OsIdx]
#define _NOPARAM_ROUTINE_LOADUSERAPIHOOK gNOPARAM_ROUTINE_LOADUSERAPIHOOK[g_OsIdx]
#define _ONEPARAM_ROUTINE_MAPDEKTOPOBJECT gONEPARAM_ROUTINE_MAPDEKTOPOBJECT[g_OsIdx]
#define _ONEPARAM_ROUTINE_SWAPMOUSEBUTTON gONEPARAM_ROUTINE_SWAPMOUSEBUTTON[g_OsIdx]
#define _HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW gHWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW[g_OsIdx]
#define _HWND_ROUTINE_GETWNDCONTEXTHLPID gHWND_ROUTINE_GETWNDCONTEXTHLPID[g_OsIdx]
#define _HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID gHWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID[g_OsIdx]



#endif /* _W32KNAPI_H */
