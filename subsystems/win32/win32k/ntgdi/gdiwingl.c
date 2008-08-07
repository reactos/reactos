/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdiwingl.c
 * PURPOSE:         Win32 OpenGL Extension Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

INT
APIENTRY
NtGdiDescribePixelFormat(IN HDC hdc,
                         IN INT ipfd,
                         IN UINT cjpfd,
                         OUT PPIXELFORMATDESCRIPTOR ppfd)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSetPixelFormat(IN HDC hdc,
                    IN INT ipfd)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSwapBuffers(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}
