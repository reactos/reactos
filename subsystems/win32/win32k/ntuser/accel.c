/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/accel.c
 * PURPOSE:         Keyboard Accelerator Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

INT
APIENTRY
NtUserCopyAcceleratorTable(HACCEL Table,
                           ACCEL* Entries,
                           INT EntriesCount)
{
    UNIMPLEMENTED;
    return 0;
}

HACCEL
APIENTRY
NtUserCreateAcceleratorTable(ACCEL* Entries,
                             INT EntriesCount)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserDestroyAcceleratorTable(HACCEL Table)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtUserTranslateAccelerator(HWND Window,
                           HACCEL Table,
                           PMSG Message)
{
    UNIMPLEMENTED;
    return 0;
}
