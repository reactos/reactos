/*
 * PROJECT:         ReactOS
 * LICENSE:         LGPL
 * FILE:            dll/win32/winent.drv/userdrv.c
 * PURPOSE:         User driver stub for ReactOS/Windows
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "shellapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rosuserdrv);

static CRITICAL_SECTION NTDRV_CritSection;


/* FUNCTIONS **************************************************************/


/***********************************************************************
 *		wine_tsx11_lock   (X11DRV.@)
 */
void CDECL wine_tsx11_lock(void)
{
    EnterCriticalSection( &NTDRV_CritSection );
}

/***********************************************************************
 *		wine_tsx11_unlock   (X11DRV.@)
 */
void CDECL wine_tsx11_unlock(void)
{
    LeaveCriticalSection( &NTDRV_CritSection );
}

/***********************************************************************
 *		X11DRV_create_desktop
 *
 * Create the X11 desktop window for the desktop mode.
 */
UINT CDECL RosDrv_create_desktop( UINT width, UINT height )
{
    UNIMPLEMENTED;
    return 0;
}

/***********************************************************************
 *              wine_notify_icon   (NTDRV.@)
 *
 * Driver-side implementation of Shell_NotifyIcon.
 */
int CDECL wine_notify_icon( DWORD msg, NOTIFYICONDATAW *data )
{
    UNIMPLEMENTED;
    return FALSE;
}

/***********************************************************************
 *           NTDRV initialisation routine
 */
BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    BOOL ret = TRUE;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        InitializeCriticalSection(&NTDRV_CritSection);
        //ret = process_attach();
        break;
    case DLL_THREAD_DETACH:
        //thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        //process_detach();
        break;
    }
    return ret;
}

/* EOF */
