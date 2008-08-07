/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdipen.c
 * PURPOSE:         GDI Pen Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HPEN
APIENTRY
NtGdiCreatePen(IN INT iPenStyle,
               IN INT iPenWidth,
               IN COLORREF cr,
               IN HBRUSH hbr)
{
    UNIMPLEMENTED;
    return NULL;
}

HPEN
APIENTRY
NtGdiExtCreatePen(IN ULONG flPenStyle,
                  IN ULONG ulWidth,
                  IN ULONG iBrushStyle,
                  IN ULONG ulColor,
                  IN ULONG_PTR lClientHatch,
                  IN ULONG_PTR lHatch,
                  IN ULONG cstyle,
                  IN OPTIONAL PULONG pulStyle,
                  IN ULONG cjDIB,
                  IN BOOL bOldStylePen,
                  IN OPTIONAL HBRUSH hbrush)
{
    UNIMPLEMENTED;
    return NULL;
}

HPEN
APIENTRY
NtGdiSelectPen(IN HDC hdc,
               IN HPEN hpen)
{
    UNIMPLEMENTED;
    return NULL;
}
