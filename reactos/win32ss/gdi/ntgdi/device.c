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
IntCreatePrimarySurface(VOID)
{
    SIZEL SurfSize;
    SURFOBJ *pso;
    PDESKTOP rpDesk;

    /* Create surface */
    pso = &PDEVOBJ_pSurface(gppdevPrimary)->SurfObj;
    SurfSize = pso->sizlBitmap;

    /* Attach monitor */
    UserAttachMonitor((HDEV)gppdevPrimary);

    DPRINT("IntCreatePrimarySurface, gppdevPrimary=%p, gppdevPrimary->pSurface = %p\n",
        gppdevPrimary, gppdevPrimary->pSurface);

    /* Put the pointer in the center of the screen */
    gpsi->ptCursor.x = pso->sizlBitmap.cx / 2;
    gpsi->ptCursor.y = pso->sizlBitmap.cy / 2;

    rpDesk = IntGetActiveDesktop();
    if (!rpDesk)
    { /* First time going in from winlogon and starting up application desktop and
        haven't switch to winlogon desktop. Also still in WM_CREATE. */
       PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
       rpDesk = pti->rpdesk;
       if (!rpDesk)
       {
          DPRINT1("No DESKTOP Window!!!!!\n");
       }
    }
    co_IntShowDesktop(rpDesk, SurfSize.cx, SurfSize.cy, TRUE);

    // Init Primary Displays Device Capabilities.
    PDEVOBJ_vGetDeviceCaps(gppdevPrimary, &GdiHandleTable->DevCaps);

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
    return gppdevPrimary;
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


