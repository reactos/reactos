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
    {DXENG_INDEX_DxEngScreenAccessCheck, (PFN)DxEngScreenAccessCheck},
    {DXENG_INDEX_DxEngRedrawDesktop, (PFN)DxEngRedrawDesktop},
    {DXENG_INDEX_DxEngDispUniq, (PFN)DxEngDispUniq},
    {DXENG_INDEX_DxEngIncDispUniq, (PFN)DxEngIncDispUniq},
    {DXENG_INDEX_DxEngVisRgnUniq, (PFN)DxEngVisRgnUniq},
    {DXENG_INDEX_DxEngLockShareSem, (PFN)DxEngLockShareSem},
    {DXENG_INDEX_DxEngUnlockShareSem, (PFN)DxEngUnlockShareSem},
    {DXENG_INDEX_DxEngEnumerateHdev, (PFN)DxEngEnumerateHdev},
    {DXENG_INDEX_DxEngLockHdev, (PFN)DxEngLockHdev},
    {DXENG_INDEX_DxEngUnlockHdev, (PFN)DxEngUnlockHdev},
    {DXENG_INDEX_DxEngIsHdevLockedByCurrentThread, (PFN)DxEngIsHdevLockedByCurrentThread},
    {DXENG_INDEX_DxEngReferenceHdev, (PFN)DxEngReferenceHdev},
    {DXENG_INDEX_DxEngUnreferenceHdev, (PFN)DxEngUnreferenceHdev},
    {DXENG_INDEX_DxEngGetDeviceGammaRamp, (PFN)DxEngGetDeviceGammaRamp},
    {DXENG_INDEX_DxEngSetDeviceGammaRamp, (PFN)DxEngSetDeviceGammaRamp},
    {DXENG_INDEX_DxEngSpTearDownSprites, (PFN)DxEngSpTearDownSprites},
    {DXENG_INDEX_DxEngSpUnTearDownSprites, (PFN)DxEngSpUnTearDownSprites},
    {DXENG_INDEX_DxEngSpSpritesVisible, (PFN)DxEngSpSpritesVisible},
    {DXENG_INDEX_DxEngGetHdevData, (PFN)DxEngGetHdevData},
    {DXENG_INDEX_DxEngSetHdevData, (PFN)DxEngSetHdevData},
    {DXENG_INDEX_DxEngCreateMemoryDC, (PFN)DxEngCreateMemoryDC},
    {DXENG_INDEX_DxEngGetDesktopDC, (PFN)DxEngGetDesktopDC},
    {DXENG_INDEX_DxEngDeleteDC, (PFN)DxEngDeleteDC},
    {DXENG_INDEX_DxEngCleanDC, (PFN)DxEngCleanDC},
    {DXENG_INDEX_DxEngSetDCOwner, (PFN)DxEngSetDCOwner},
    {DXENG_INDEX_DxEngLockDC, (PFN)DxEngLockDC},
    {DXENG_INDEX_DxEngUnlockDC, (PFN)DxEngUnlockDC},
    {DXENG_INDEX_DxEngSetDCState, (PFN)DxEngGetDCState},
    {DXENG_INDEX_DxEngGetDCState, (PFN)DxEngGetDCState},
    {DXENG_INDEX_DxEngSelectBitmap, (PFN)DxEngSelectBitmap},
    {DXENG_INDEX_DxEngSetBitmapOwner, (PFN)DxEngSetBitmapOwner},
    {DXENG_INDEX_DxEngDeleteSurface, (PFN)DxEngDeleteSurface},
    {DXENG_INDEX_DxEngGetSurfaceData, (PFN)DxEngGetSurfaceData},
    {DXENG_INDEX_DxEngAltLockSurface, (PFN)DxEngAltLockSurface},
    {DXENG_INDEX_DxEngUploadPaletteEntryToSurface, (PFN)DxEngUploadPaletteEntryToSurface},
    {DXENG_INDEX_DxEngMarkSurfaceAsDirectDraw, (PFN)DxEngMarkSurfaceAsDirectDraw},
    {DXENG_INDEX_DxEngSelectPaletteToSurface, (PFN)DxEngSelectPaletteToSurface},
    {DXENG_INDEX_DxEngSyncPaletteTableWithDevice, (PFN)DxEngSyncPaletteTableWithDevice},
    {DXENG_INDEX_DxEngSetPaletteState, (PFN)DxEngSetPaletteState},
    {DXENG_INDEX_DxEngGetRedirectionBitmap, (PFN)DxEngGetRedirectionBitmap},
    {DXENG_INDEX_DxEngLoadImage, (PFN)DxEngLoadImage}
};

/************************************************************************/
/* DxEngNUIsTermSrv                                                     */
/************************************************************************/

/* Notes : Check see if termal server got a connections or not */
BOOL
DxEngNUIsTermSrv()
{
    /* FIXME ReactOS does not suport terminal server yet, we can not check if we got a connections or not */
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
    return TRUE;
}


/************************************************************************/
/* DxEngDispUniq                                                        */
/************************************************************************/

/*  Notes : return the DisplayUniqVisrgn counter from gdishare memory  */
ULONG
DxEngDispUniq()
{
    return GdiHandleTable->flDeviceUniq;
}

ULONG gulVisRgnUniqueness; // Increase count everytime client region is updated.

/************************************************************************/
/* DxEngVisRgnUniq                                                      */
/************************************************************************/
/* Notes :  return the VisRgnUniq counter for win32k */
ULONG
DxEngVisRgnUniq()
{
    return gulVisRgnUniqueness;
}

/************************************************************************/
/* DxEngEnumerateHdev                                                   */
/************************************************************************/
/* Enumerate all drivers in win32k */
HDEV *
DxEngEnumerateHdev(HDEV *hdev)
{
    /* FIXME Enumerate all drivers in win32k */
    UNIMPLEMENTED;
    return FALSE;
}

/*++
* @name DxEngGetDeviceGammaRamp
* @implemented
*
* The function DxEngSetDeviceGammaRamp Set Gamma ramp from from dxg.sys

* @param HDEV hPDev
* The hdev
*
* @param PGAMMARAMP Ramp
* to fill in our gamma ramp
*
* @return
*Returns TRUE for success, FALSE for failure
*
* @remarks.
* ReactOS does not loop it, only sets the gamma once.
*
*--*/
BOOL
DxEngGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp)
{
    return IntGetDeviceGammaRamp(hPDev, Ramp);
}


/*++
* @name DxEngLockDC
* @implemented
*
* The function DxEngLockDC locks a hdc from dxg.sys
*
* @param HDC hDC
* The handle we want to lock
*
* @return
* Returns PDC if lock succeeded or NULL depns if it failed
*
* @remarks.
* none
*
*--*/
PDC
DxEngLockDC(HDC hDC)
{
    return DC_LockDc(hDC);
}


/*++
* @name DxEngUnlockDC
* @implemented
*
* The function DxEngUnlockDC unlocks a pDC (hdc) from dxg.sys

* @param PDC pDC
* The handle we want to unlock
*
* @return
* This function returns TRUE no matter what
*
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


/************************************************************************/
/* DxEngCreateMemoryDC                                                  */
/************************************************************************/
DWORD DxEngCreateMemoryDC(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngScreenAccessCheck                                               */
/************************************************************************/
DWORD DxEngScreenAccessCheck()
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngLockShareSem                                                    */
/************************************************************************/
DWORD DxEngLockShareSem()
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngUnlockShareSem                                                  */
/************************************************************************/
DWORD DxEngUnlockShareSem()
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngLockHdev                                                        */
/************************************************************************/
DWORD DxEngLockHdev(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngUnlockHdev                                                      */
/************************************************************************/
DWORD DxEngUnlockHdev(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngReferenceHdev                                                   */
/************************************************************************/
DWORD DxEngReferenceHdev(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngIsHdevLockedByCurrentThread                                     */
/************************************************************************/
DWORD DxEngIsHdevLockedByCurrentThread(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}


/************************************************************************/
/* DxEngUnreferenceHdev                                                 */
/************************************************************************/
DWORD DxEngUnreferenceHdev(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*++
* @name DxEngSetDeviceGammaRamp
* @implemented
*
* The function DxEngSetDeviceGammaRamp sets gamma ramp from dxg.sys

* @param HDEV hPDev
* The hdev
*
* @param PGAMMARAMP Ramp
* Value to change gamma ramp to
*
* @param BOOL Test
* Whether gamma should be tested. TRUE to test, FALSE to not test
*
* @return
*Returns TRUE for success, FALSE for failure
*
* @remarks.
* ReactOS does not loop and only sets the gamma once.
*
*--*/
BOOL
DxEngSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL Test)
{
   return IntSetDeviceGammaRamp(hPDev, Ramp, Test);
}

/************************************************************************/
/* DxEngSpTearDownSprites                                               */
/************************************************************************/
DWORD DxEngSpTearDownSprites(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSpUnTearDownSprites                                             */
/************************************************************************/
DWORD DxEngSpUnTearDownSprites(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSpSpritesVisible                                                */
/************************************************************************/
DWORD DxEngSpSpritesVisible(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngGetHdevData                                                     */
/************************************************************************/
DWORD
DxEngGetHdevData(PEDD_DIRECTDRAW_GLOBAL pEDDgpl,
                 DWORD Index)
{
    UNIMPLEMENTED;
    return 0;
}

/************************************************************************/
/* DxEngSetHdevData                                                     */
/************************************************************************/
DWORD DxEngSetHdevData(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngGetDesktopDC                                                    */
/************************************************************************/
DWORD DxEngGetDesktopDC(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngDeleteDC                                                        */
/************************************************************************/
DWORD DxEngDeleteDC(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngCleanDC                                                         */
/************************************************************************/
DWORD DxEngCleanDC(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSetDCOwner                                                      */
/************************************************************************/
DWORD DxEngSetDCOwner(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSetDCState                                                      */
/************************************************************************/
DWORD DxEngSetDCState(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}



/*++
* @name DxEngGetDCState
* @implemented
*
* The function DxEngGetDCState is capable of returning three
* DC states depending on what value is passed in its second parameter:
* 1. If the DC is full screen
* 2. Get Complexity of visible region
* 3. Get Driver hdev, which is pPDev
*
* @param HDC hdc
* The DC handle
*
* @param DWORD type
* value 1 = Is DC fullscreen 
* value 2 = Get Complexity of visible region. 
* value 3 = Get Driver hdev, which is a pPDev. 
*
* @return
* Return one of the type values
*
* @remarks.
* We do not have type 2 implement yet
*
*--*/
DWORD
DxEngGetDCState(HDC hDC,
                DWORD type)
{
    PDC pDC = DC_LockDc(hDC);
    DWORD retVal = 0;

    if (pDC)
    {
        switch (type)
        {
            case 1:
                retVal = (DWORD) pDC->DC_Flags & DC_FLAG_FULLSCREEN;
                break;
            case 2:
                UNIMPLEMENTED;
                break;
            case 3:
            {
                /* Return the HDEV of this DC. */            
                retVal = (DWORD) pDC->pPDev;
                break;
            }
            default:
                /* if a valid type is not found, zero is returned */
                DPRINT1("Warning did not find type %d\n",type); 
                break;
        }
        DC_UnlockDc(pDC);
    }

    return retVal;
}

/************************************************************************/
/* DxEngSelectBitmap                                                    */
/************************************************************************/
DWORD DxEngSelectBitmap(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSetBitmapOwner                                                  */
/************************************************************************/
DWORD DxEngSetBitmapOwner(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngDeleteSurface                                                   */
/************************************************************************/
DWORD DxEngDeleteSurface(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngGetSurfaceData                                                  */
/************************************************************************/
DWORD DxEngGetSurfaceData(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngAltLockSurface                                                  */
/************************************************************************/
DWORD DxEngAltLockSurface(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngUploadPaletteEntryToSurface                                     */
/************************************************************************/
DWORD DxEngUploadPaletteEntryToSurface(DWORD x1, DWORD x2,DWORD x3, DWORD x4)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngMarkSurfaceAsDirectDraw                                         */
/************************************************************************/
DWORD DxEngMarkSurfaceAsDirectDraw(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSelectPaletteToSurface                                          */
/************************************************************************/
DWORD DxEngSelectPaletteToSurface(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSyncPaletteTableWithDevice                                      */
/************************************************************************/
DWORD DxEngSyncPaletteTableWithDevice(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSetPaletteState                                                 */
/************************************************************************/
DWORD DxEngSetPaletteState(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngGetRedirectionBitmap                                            */
/************************************************************************/
DWORD DxEngGetRedirectionBitmap(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngLoadImage                                                       */
/************************************************************************/
DWORD DxEngLoadImage(DWORD x1,DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngIncDispUniq                                                       */
/************************************************************************/
DWORD DxEngIncDispUniq()
{
    UNIMPLEMENTED;
    return FALSE;
}


