/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/eng/engwnd.c
 * PURPOSE:         WNDOBJ Manipulation Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
EngControlSprites(IN WNDOBJ* WindowObj,
                  IN FLONG Flags)
{
    UNIMPLEMENTED;
	return FALSE;
}

WNDOBJ*
APIENTRY
EngCreateWnd(SURFOBJ *pso,
             HWND hwnd,
             WNDOBJCHANGEPROC pfn,
             FLONG fl,
             INT iPixelFormat)
{
    UNIMPLEMENTED;
	return NULL;
}

VOID
APIENTRY
EngDeleteWnd(IN WNDOBJ *pwo)
{
    UNIMPLEMENTED;
}

BOOL
APIENTRY
WNDOBJ_bEnum(IN WNDOBJ* WindowObj,
             IN ULONG BufferSize,
             OUT PULONG Buffer)
{
    UNIMPLEMENTED;
	return FALSE;
}

ULONG
APIENTRY
WNDOBJ_cEnumStart(IN WNDOBJ* WindowObj,
                  IN ULONG iType,
                  IN ULONG iDirection,
                  IN ULONG cLimit)
{
    UNIMPLEMENTED;
	return 0;
}

VOID
APIENTRY
WNDOBJ_vSetConsumer(IN WNDOBJ* WindowObj,
                    IN PVOID pvConsumer)
{
    UNIMPLEMENTED;
}
