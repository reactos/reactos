/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    start.c

Abstract:

    Code to handle restarting of windows and getting about box information.

Author:

    Sunil Pai (sunilp) 26-Oct-1994.  Ported from Windows95 Shell\library\start.c

Revision History:

--*/


#include "shellprv.h"
#pragma  hdrstop

//---------------------------------------------------------------------------
//
// PURPOSE:  This is to reboot/logoff/.. windows
//
// EXPORTED: Not exported
//
// DEFINED in WINDOWS95:  \win\core\library\start.c
//
// USED:     internally by shelldll.
//
// DETAILS:  Nothing to do.  Changed over to using ExitWindowsEx instead of
//           ExitWindows since ExitWindows is defined as being
//           ExitWindowsEx(EWX_LOGOFF, 0xFFFFFFFF)
//
//
// HISTORY:  SUNILP, 26-Oct-1994 Ported
//
//----------------------------------------------------------------------------
BOOL WINAPI SHRestartWindows(DWORD dwReturn)
{
    return ExitWindowsEx(dwReturn, 0);
}

//---------------------------------------------------------------------------
//
// PURPOSE:  To get system resource information to display in about box.
//
// EXPORTED: Not exported
//
// DEFINED in WINDOWS95:  \win\core\library\start.c
//
// USED:     internally by shelldll
//
// DETAILS:  Windows 95 uses GetFreeSystemResources to get the
//           min(GDI Heap Free, User Heap Free).  Since NT doesn't have this
//           call we are using the MemoryLoad information in GlobalMemoryStatus
//           to do the same.
//
//           Also Windows 95 uses GetFreeSpace to get free physical memory.
//           Since this call is obsolete in Win32 we are using the dwAvailPhys
//           information which returns the same value.
//
// HISTORY:  SUNILP, Oct 26, 1994 Ported
//
//----------------------------------------------------------------------------
VOID WINAPI SHGetAboutInformation(LPWORD puSysResource, LPDWORD pcbFree)
{
    MEMORYSTATUS MemoryStatus;

    GlobalMemoryStatus (&MemoryStatus);
    if (puSysResource)
    {
        *puSysResource = (WORD)MemoryStatus.dwMemoryLoad;
    }

    if (pcbFree)
    {
        // BUGBUG raymondc: What about memory greater than 4 TB.

        *pcbFree = (DWORD) (MemoryStatus.dwAvailPhys / 1024);
    }
}
