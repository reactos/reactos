/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Native driver enumate of dxeng implementation
 * FILE:             subsys/win32k/ntddraw/dxeng.c
 * PROGRAMER:        Magnus olsen (magnus@greatlord.com)
 * REVISION HISTORY:
 *       15/10-2007   Magnus Olsen
 */

#include <w32k.h>
#include <debug.h>

ULONG gcEngFuncs = DXENG_INDEX_DxEngLoadImage + 1;
DRVFN gaEngFuncs [] =
{
    {0, (PFN) NULL},
    {DXENG_INDEX_DxEngNUIsTermSrv, (PFN)DxEngNUIsTermSrv},
//    {DXENG_INDEX_DxEngScreenAccessCheck, (PFN)DxEngScreenAccessCheck},
    {0, (PFN) NULL}, // hack for now 
    {DXENG_INDEX_DxEngRedrawDesktop, (PFN)DxEngRedrawDesktop},
    {DXENG_INDEX_DxEngDispUniq, (PFN)DxEngDispUniq},
//    {DXENG_INDEX_DxEngIncDispUniq, (PFN)DxEngIncDispUniq},
    {0, (PFN) NULL}, // hack for now 
    {DXENG_INDEX_DxEngVisRgnUniq, (PFN)DxEngVisRgnUniq},
//    {DXENG_INDEX_DxEngLockShareSem, (PFN)DxEngLockShareSem},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngUnlockShareSem, (PFN)DxEngUnlockShareSem},
    {0, (PFN) NULL}, // hack for now 
    {DXENG_INDEX_DxEngEnumerateHdev, (PFN)DxEngEnumerateHdev},
//    {DXENG_INDEX_DxEngLockHdev, (PFN)DxEngLockHdev},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngUnlockHdev, (PFN)DxEngUnlockHdev},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngIsHdevLockedByCurrentThread, (PFN)DxEngIsHdevLockedByCurrentThread},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngReferenceHdev, (PFN)DxEngReferenceHdev},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngUnreferenceHdev, (PFN)DxEngUnreferenceHdev},
    {0, (PFN) NULL}, // hack for now 
    {DXENG_INDEX_DxEngGetDeviceGammaRamp, (PFN)DxEngGetDeviceGammaRamp},
//    {DXENG_INDEX_DxEngSetDeviceGammaRamp, (PFN)DxEngSetDeviceGammaRamp},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSpTearDownSprites, (PFN)DxEngSpTearDownSprites},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSpUnTearDownSprites, (PFN)DxEngSpUnTearDownSprites},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSpSpritesVisible, (PFN)DxEngSpSpritesVisible},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngGetHdevData, (PFN)DxEngGetHdevData},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSetHdevData, (PFN)DxEngSetHdevData},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngCreateMemoryDC, (PFN)DxEngCreateMemoryDC},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngGetDesktopDC, (PFN)DxEngGetDesktopDC},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngDeleteDC, (PFN)DxEngDeleteDC},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngCleanDC, (PFN)DxEngCleanDC},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSetDCOwner, (PFN)DxEngSetDCOwner},
    {0, (PFN) NULL}, // hack for now 
    {DXENG_INDEX_DxEngLockDC, (PFN)DxEngLockDC},
    {DXENG_INDEX_DxEngUnlockDC, (PFN)DxEngUnlockDC},
//    {DXENG_INDEX_DxEngSetDCState, (PFN)DxEngGetDCState},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngGetDCState, (PFN)DxEngGetDCState},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSelectBitmap, (PFN)DxEngSelectBitmap},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSetBitmapOwner, (PFN)DxEngSetBitmapOwner},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngDeleteSurface, (PFN)DxEngDeleteSurface},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngGetSurfaceData, (PFN)DxEngGetSurfaceData},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngAltLockSurface, (PFN)DxEngAltLockSurface},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngUploadPaletteEntryToSurface, (PFN)DxEngUploadPaletteEntryToSurface},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngMarkSurfaceAsDirectDraw, (PFN)DxEngMarkSurfaceAsDirectDraw},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSelectPaletteToSurface, (PFN)DxEngSelectPaletteToSurface},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSyncPaletteTableWithDevice, (PFN)DxEngSyncPaletteTableWithDevice},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngSetPaletteState, (PFN)DxEngSetPaletteState},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngGetRedirectionBitmap, (PFN)DxEngGetRedirectionBitmap},
    {0, (PFN) NULL}, // hack for now 
//    {DXENG_INDEX_DxEngLoadImage, (PFN)DxEngLoadImage}
    {0, (PFN) NULL} // hack for now
};

/************************************************************************/
/* DxEngNUIsTermSrv                                                     */
/************************************************************************/

/* Notes : Check see if termal server got a connections or not */
BOOL
DxEngNUIsTermSrv()
{
    /* FIXME ReactOS does not suport terminal server yet, we can not check if we got a connections or not */
    DPRINT1("We need termal server connections check");
    return FALSE;
}

/************************************************************************/
/* DxEngRedrawDesktop                                                   */
/************************************************************************/

/* Notes : it always return TRUE, and it update whole the screen (redaw current desktop) */
BOOL
DxEngRedrawDesktop()
{
    /* FIXME add redraw code */
    DPRINT1("We need add code for redraw whole desktop");
    return TRUE;
}


/************************************************************************/
/* DxEngDispUniq                                                        */
/************************************************************************/

/*  Notes : return the DisplayUniqVisrgn counter from gdishare memory  */
ULONG
DxEngDispUniq()
{
    /* FIXME DisplayUniqVisrgn from gdishare memory */
    DPRINT1("We need DisplayUniqVisrgn from gdishare memory");
    return 0;
}

/************************************************************************/
/* DxEngVisRgnUniq                                                      */
/************************************************************************/
/* Notes :  return the VisRgnUniq counter for win32k */
ULONG
DxEngVisRgnUniq()
{
    /* FIXME DisplayUniqVisrgn from gdishare memory */
    DPRINT1("We need VisRgnUniq from win32k");
    return 0;
}

/************************************************************************/
/* DxEngEnumerateHdev                                                   */
/************************************************************************/
/* Enumate all drivers in win32k */
HDEV *
DxEngEnumerateHdev(HDEV *hdev)
{
    /* FIXME Enumate all drivers in win32k */
    DPRINT1("We do not enumate any device from win32k ");
    return 0;
}

/************************************************************************/
/* DxEngGetDeviceGammaRamp                                              */
/************************************************************************/
/* same protypes NtGdiEngGetDeviceGammaRamp, diffent is we skipp the user mode checks and seh */
BOOL
DxEngGetDeviceGammaRamp(HDC hDC, LPVOID lpRamp)
{
    /* FIXME redirect it to NtGdiEngGetDeviceGammaRamp internal call  */
    DPRINT1("redirect it to NtGdiEngGetDeviceGammaRamp internal call ");
    return FALSE;
}


/*++
* @name DxEngLockDC
* @implemented
*
* The function DxEngLockDC lock a hdc from dxg.sys 
*
* @param HDC hDC
* The handle we need want lock
*
* @return 
* This api return PDC or NULL depns if it sussess lock the hdc or not
* @remarks.
* none
*
*--*/
PDC
DxEngLockDC(HDC hDC)
{
    return DC_LockDc(hdc);
}


/*++
* @name DxEngUnlockDC
* @implemented
*
* The function DxEngUnlockDC Unlock a pDC (hdc) from dxg.sys 

* @param PDC pDC
* The handle we need unlock
*
* @return 
* This api always return TRUE if it sussess or not 

* @remarks.
* none
*
*--*/
BOOL
DxEngUnlockDC(PDC pDC)
{
    DC_UnlockDc(pDC);
    return TRUE;
}




