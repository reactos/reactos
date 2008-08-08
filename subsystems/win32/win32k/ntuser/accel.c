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
                           LPACCEL Entries,
                           INT EntriesCount)
{
    UNIMPLEMENTED;
    return 0;
}

HACCEL
APIENTRY
NtUserCreateAcceleratorTable(LPACCEL Entries,
                             SIZE_T EntriesCount)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOLEAN
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
                           LPMSG Message)
{
    UNIMPLEMENTED;
    return 0;
}
