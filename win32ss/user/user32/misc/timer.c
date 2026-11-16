/*
 * PROJECT:     VerOS NT Subsystem
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        win32ss/user/user32/misc/timer.c
 * PURPOSE:     DDE
 * COPYRIGHT:   Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *              Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Ryan Wilson <ryan@verosnt.com>
 */

/*
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

#include <user32.h>

/*
 * @implemented
 */
BOOL
WINAPI
KillSystemTimer( HWND hWnd, UINT_PTR IDEvent)
{
  return NtUserxKillSystemTimer(hWnd, IDEvent);
}

/*
 * @implemented
 */
UINT_PTR
WINAPI
SetCoalescableTimer(HWND hwnd,
                    UINT_PTR idEvent,
                    UINT elapse,
                    TIMERPROC timerProc,
                    ULONG tolerance)
{
    /* This kernel has one timer resolution, so the tolerance is not applied. */
    UNREFERENCED_PARAMETER(tolerance);
    return SetTimer(hwnd, idEvent, elapse, timerProc);
}
