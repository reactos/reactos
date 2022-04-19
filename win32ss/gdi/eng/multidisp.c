/*
* PROJECT:     ReactOS Win32k subsystem
* LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
* FILE:        win32ss/gdi/eng/multidisp.c
* PURPOSE:     Multi-Display driver
* PROGRAMMERS:
*/

/* INCLUDES *******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

BOOL
APIENTRY
MultiEnableDriver(
    _In_ ULONG iEngineVersion,
    _In_ ULONG cj,
    _Inout_bytecount_(cj) PDRVENABLEDATA pded)
{
    UNIMPLEMENTED;
    return FALSE;
}
