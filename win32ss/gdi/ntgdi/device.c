/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              win32ss/gdi/ntgdi/device.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

PDC defaultDCstate = NULL;

VOID FASTCALL
IntGdiReferencePdev(PPDEVOBJ ppdev)
{
    UNIMPLEMENTED;
}

VOID FASTCALL
IntGdiUnreferencePdev(PPDEVOBJ ppdev, DWORD CleanUpType)
{
    UNIMPLEMENTED;
}

BOOL FASTCALL
IntCreatePrimarySurface(VOID)
{
    /* Create surface */
    PDEVOBJ_pSurface(gpmdev->ppdevGlobal);

    DPRINT("IntCreatePrimarySurface, ppdevGlobal=%p, ppdevGlobal->pSurface = %p\n",
        gpmdev->ppdevGlobal, gpmdev->ppdevGlobal->pSurface);

    // Init Primary Displays Device Capabilities.
    PDEVOBJ_vGetDeviceCaps(gpmdev->ppdevGlobal, &GdiHandleTable->DevCaps);

    return TRUE;
}

VOID FASTCALL
IntDestroyPrimarySurface(VOID)
{
    UNIMPLEMENTED;
}

PPDEVOBJ FASTCALL
IntEnumHDev(VOID)
{
// I guess we will soon have more than one primary surface.
// This will do for now.
    return gpmdev->ppdevGlobal;
}


INT
APIENTRY
NtGdiDrawEscape(
    IN HDC hdc,
    IN INT iEsc,
    IN INT cjIn,
    IN OPTIONAL LPSTR pjIn)
{
    UNIMPLEMENTED;
    return 0;
}


