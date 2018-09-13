/*++

Copyright (c) 1994-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    indicdll.h

Abstract:

    This module contains the information for handling the shell hooks for the
    multilingual language indicator.  It also contains information for the
    on-screen keyboard, which MUST be loaded by internat.exe.

Revision History:

--*/



//
//  Include Files.
//

#include <windows.h>
#include <windowsx.h>
#include <winuserp.h>
#include "..\share.h"


//
//  Shared Data structure.
//
//   Use FileMapping shared memory not to break NT C2 security.
//

typedef struct _shareddata
{
    HINSTANCE hinstDLL;
    UINT iShellActive;
    HWND hwndInternat;
    HWND hwndOSK;
    HHOOK hookShell;
    HHOOK hookKbd;
    HHOOK hookCBT;
    HWND hwndNotify;
    HWND hwndTaskBar;
    HWND hwndTasks;
    HWND hwndLastFocus;
    HWND hwndLastActive;
    HWND hwndConsoleIme;
    UINT iIMEStatForLastFocus;
    DWORD dwTidLastFocus;
    HKL hklLastFocus;
    UINT uMenuID;
    DWORD dwMenuItemData;
} SHAREDDATA, *LPSHAREDDATA;

extern LPVOID lpvSharedMem;              // pointer to shared memory
extern LPSHAREDDATA lpvSHDataHead;       // header pointer of SH data



//
//  Function Prototypes.
//

BOOL
IndicDll_RegisterHookSendWindow(
    HWND hwnd,
    BOOL bInternat);

BOOL
IndicDll_StartShell(void);

BOOL
IndicDll_StopShell(void);

HWND
IndicDll_GetLastActiveWnd(void);

HKL
IndicDll_GetLayout(void);

int
IndicDll_GetIMEStatus(void);

void
IndicDll_SaveIMEStatus(
    HWND hwnd);

HWND
IndicDll_GetConsoleImeWnd(void);
