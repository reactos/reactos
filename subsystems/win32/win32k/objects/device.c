/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for creation and destruction of DCs
 * FILE:              subsystem/win32/win32k/objects/device.c
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
IntCreatePrimarySurface()
{
    SIZEL SurfSize;
    SURFOBJ *pso;

    /* Attach monitor */
    UserAttachMonitor((HDEV)gppdevPrimary);

    DPRINT("IntCreatePrimarySurface, pPrimarySurface=%p, pPrimarySurface->pSurface = %p\n",
        pPrimarySurface, pPrimarySurface->pSurface);

    /* Create surface */
    pso = &PDEVOBJ_pSurface(pPrimarySurface)->SurfObj;
    SurfSize = pso->sizlBitmap;

    /* Put the pointer in the center of the screen */
    gpsi->ptCursor.x = pso->sizlBitmap.cx / 2;
    gpsi->ptCursor.y = pso->sizlBitmap.cy / 2;

    co_IntShowDesktop(IntGetActiveDesktop(), SurfSize.cx, SurfSize.cy);

    // Init Primary Displays Device Capabilities.
    PDEVOBJ_vGetDeviceCaps(pPrimarySurface, &GdiHandleTable->DevCaps);

    return TRUE;
}

VOID FASTCALL
IntDestroyPrimarySurface()
{
    UNIMPLEMENTED;
}

PPDEVOBJ FASTCALL
IntEnumHDev(VOID)
{
// I guess we will soon have more than one primary surface.
// This will do for now.
    return pPrimarySurface;
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


