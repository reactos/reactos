/*
 * PROJECT:         ReactOS
 * LICENSE:         GNU LGPL by FSF v2.1 or any later
 * FILE:            dll/win32/winent.drv/event.c
 * PURPOSE:         Event handling routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include "winent.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(event);

/* GLOBALS ****************************************************************/

/***********************************************************************
 *           MsgWaitForMultipleObjectsEx   (NTDRV.@)
 */
DWORD CDECL RosDrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                DWORD mask, DWORD flags )
{
    TRACE("WaitForMultipleObjectsEx(%d %p %d %x %x %x\n", count, handles, timeout, mask, flags);

    if (!count && !timeout) return WAIT_TIMEOUT;
    return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                     timeout, flags & MWMO_ALERTABLE );
}
