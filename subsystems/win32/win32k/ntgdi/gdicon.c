/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdicon.c
 * PURPOSE:         GDI Console Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiConsoleTextOut(IN HDC hdc,
                    IN POLYTEXTW *lpto,
                    IN UINT nStrings,
                    IN RECTL *prclBounds)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
APIENTRY
NtGdiFullscreenControl(IN FULLSCREENCONTROL FullscreenCommand,
                       IN PVOID FullscreenInput,
                       IN DWORD FullscreenInputLength,
                       OUT PVOID FullscreenOutput,
                       IN OUT PULONG FullscreenOutputLength)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}
