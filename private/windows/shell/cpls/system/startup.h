/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    startup.h

Abstract:

    Public declarations for the Startup/Recovery dialog of the System
    Control Panel Applet

Notes:

    The virtual memory settings and the crash dump (core dump) settings
    are tightly-coupled.  Therefore, crashdmp.c and startup.h have some
    heavy dependencies on virtual.c and virtual.h (and vice versa).

    On the other hand, the startup OS settings and the crash dump settings
    have almost nothing in common, so you won't see a lot of dependencies
    between these files (strlst.c/startup.c and crashdmp.c), even
    though they're on the same dialog.

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#ifndef _SYSDM_STARTUP_H_
#define _SYSDM_STARTUP_H_

//
// Constants
//

// Range of valid "Display startup list for..." values
#define FORMIN       0
#define FORMAX     999
// Length of WCHAR buffer needed to hold "Display startup list for..." value
#define FOR_MAX_LENGTH 20

// Default "Display startup list for..." value
#define FORDEF      30

// Crash dump constants
#define IDRV_DEF_BOOT       2       // Asssume booting from C:
#define MIN_SWAPSIZE        2       // Min swap file size.
#define MAX_SWAPSIZE        (0xFFFF0000 / ONE_MEG)    // magic number from LouP
#define ONE_MEG             1048576

//
// Indexes into the core dump control array
//
#define CD_LOG      0
#define CD_SEND     1
#define CD_WRITE    2
#define CD_OVERW    3
#define CD_ABOOT    4
#define CD_FILE     5

//
// Type definitions
//
typedef struct {
    INT idCtl;
    LPTSTR pszValueName;
    DWORD dwType;
    union {
        BOOL fValue;
        LPTSTR pszValue;
    } u;
} COREDMP_FIELD;  // Core Dump vars

//
// Global Variables
//
extern COREDMP_FIELD acdfControls[];       // core dump controls (crashdmp.c)
extern BOOL gfCoreDumpChanged;
extern HKEY ghkeyCrashCtrl;
extern int  gcrefCrashCtrl;


// Set during initialization so we don't think changes made to controls
// during initialization are the same as changes made to controls by
// the user.
extern BOOL g_fStartupInitializing;

//
// Function Declarations
//
HPROPSHEETPAGE 
CreateStartupPage(
    IN HINSTANCE hInst
);

BOOL 
APIENTRY 
StartupDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);


//
// Functions implemented in strtlst.c
//
void 
StartListInit( 
    IN HWND hDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam 
);

int 
StartListExit(
    IN HWND hDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam 
);

void 
StartListDestroy(
    IN HWND hDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);

BOOL 
CheckVal( 
    IN HWND hDlg, 
    IN WORD wID, 
    IN WORD wMin, 
    IN WORD wMax, 
    IN WORD wMsgID 
);

//
// Functions implemented in crashdmp.c
//
BOOL
APIENTRY
CoreDumpDlgProc(
    IN HWND hDlg,
    IN UINT message,
    IN DWORD wParam,
    IN LONG lParam
);

void 
CoreDumpGetValue(
    IN int i
);

BOOL 
CoreDumpPutValue(
    IN int i
);

VCREG_RET
CoreDumpOpenKey(
    void
);

void
CoreDumpCloseKey(
    void
);

#endif // _SYSDM_STATUP_H_
