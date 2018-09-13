/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    virtual.h

Abstract:

    Public declarations for the Change Virtual Memory dialog of the System
    Control Panel Applet

Notes:

    The virtual memory settings and the crash dump (core dump) settings
    are tightly-coupled.  Therefore, virtual.c and virtual.h have some
    heavy dependencies on crashdmp.c and startup.h (and vice versa).

Author:

    Byron Dazey 06-Jun-1992

Revision History:

    15-Oct-1997 scotthal
        Split public declarations into separate header

--*/
#ifndef _SYSDM_VIRTUAL_H_
#define _SYSDM_VIRTUAL_H_

//
// Some debugging macros shared by the virtual mem and crash dump stuff
//
#ifdef VM_DBG
#   pragma message(__FILE__"(19): warning !!!! : compiled for DEBUG ONLY!" )
#   define  DPRINTF(p)  DBGPRINTF(p)
#   define  DOUT(S)     DBGOUT(S)
#else
#   define  DPRINTF(p)
#   define  DOUT(S)
#endif

//
// Constants
//
#define MAX_DRIVES          26      // Max number of drives.

//
// Type Definitions
//
typedef struct
{
    BOOL fCanHavePagefile;      // TRUE if the drive can have a pagefile.
    BOOL fCreateFile;           // TRUE if user hits [SET] and no pagefile
    INT nMinFileSize;           // Minimum size of pagefile in MB.
    INT nMaxFileSize;           // Max size of pagefile in MB.
    INT nMinFileSizePrev;       // Previous minimum size of pagefile in MB.
    INT nMaxFileSizePrev;       // Previous max size of pagefile in MB.
    LPTSTR  pszPageFile;        // Path to page file if it exists on that drv
} PAGING_FILE; //  Swap file structure

//
// Global Variables
//
extern HKEY ghkeyMemMgt;
extern PAGING_FILE apf[MAX_DRIVES];
extern PAGING_FILE apfOriginal[MAX_DRIVES];

//
// Public function prototypes
//
BOOL
APIENTRY
VirtualMemDlg(
    IN HWND hDlg,
    IN UINT message,
    IN DWORD wParam,
    IN LONG lParam
);

BOOL
VirtualInitStructures(
    void
);

void
VirtualFreeStructures(
    void
);

INT
VirtualMemComputeAllocated(
    IN HWND hWnd
);

VCREG_RET 
VirtualOpenKey( 
    void 
);

void 
VirtualCloseKey(
    void
);

BOOL 
VirtualGetPageFiles(
    OUT PAGING_FILE *apf
);

void 
VirtualFreePageFiles(
    IN PAGING_FILE *apf
);

BOOL 
VirtualMemUpdateRegistry(
    VOID
);

int 
VirtualMemPromptForReboot(
    IN HWND hDlg
);

INT
GetFreeSpaceMB(
    IN INT iDrive
);

VOID 
SetDlgItemMB(
    IN HWND hDlg, 
    IN INT idControl, 
    IN DWORD dwMBValue
);

#endif // _SYSDM_VIRTUAL_H_
