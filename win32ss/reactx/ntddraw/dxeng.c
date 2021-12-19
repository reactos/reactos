/*
 * PROJECT:          ReactOS Win32 Subsystem
 * LICENSE:          GPL - See COPYING in the top level directory
 * FILE:             win32ss/reactx/ntddraw/dxeng.c
 * PURPOSE:          Implementation of DxEng functions
 * PROGRAMMERS:      Magnus Olsen (magnus@greatlord.com)
 *                   Oleg Dubinskiy (oleg.dubinskij2013@yandex.ua)
 */

#include <win32k.h>
#include <debug.h>

HSEMAPHORE  ghsemShareDevLock = NULL;

ULONG gcEngFuncs = DXENG_INDEX_DxEngLoadImage + 1;
DRVFN gaEngFuncs[] =
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
    {DXENG_INDEX_DxEngSetDCState, (PFN)DxEngSetDCState},
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


/*++
* @name DxEngDispUniq
* @implemented
*
* The function DxEngDispUniq returns the DisplayUniqVisrgn counter value from GDI shared memory
*
* @return
* Returns the DisplayUniqVisrgn counter value from GDI shared memory
*
* @remarks.
* none
*
*--*/
ULONG
APIENTRY
DxEngDispUniq(VOID)
{
    DPRINT1("ReactX Calling : DxEngDispUniq\n");
    return GdiHandleTable->flDeviceUniq;
}

/*++
* @name DxEngGetDeviceGammaRamp
* @implemented
*
* The function DxEngGetDeviceGammaRamp gets the gamma ramp to dxg.sys.

* @param HDEV hPDev
* The hdev.
*
* @param PGAMMARAMP Ramp
* Pointer to store the gamma ramp value in.
*
* @return
*Returns TRUE for success, FALSE for failure
*
* @remarks.
* None
*
*--*/
BOOL
APIENTRY
DxEngGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp)
{
    DPRINT1("ReactX Calling : DxEngGetDeviceGammaRamp\n");
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
* Returns PDC if lock succeeded or NULL if it failed.
*
* @remarks.
* none
*
*--*/
PDC
APIENTRY
DxEngLockDC(HDC hDC)
{
    DPRINT1("ReactX Calling : DxEngLockDC\n");
    return DC_LockDc(hDC);
}


/*++
* @name DxEngUnlockDC
* @implemented
*
* The function DxEngUnlockDC unlocks a pDC (hdc) from dxg.sys.

* @param PDC pDC
* The handle we want to unlock.
*
* @return
* This function returns TRUE no matter what.
*
* @remarks.
* none
*
*--*/
BOOLEAN
APIENTRY
DxEngUnlockDC(PDC pDC)
{
    DPRINT1("ReactX Calling : DxEngUnlockDC\n");
    DC_UnlockDc(pDC);
    return TRUE;
}

/*++
* @name DxEngLockShareSem
* @implemented
*
* The function DxEngLockShareSem locks a struct of type ghsemShareDevLock that can be shared.
*
* @return
* This function returns TRUE for success and FALSE for failure.
* FALSE must mean the struct has already been locked.
*
* @remarks.
* It is being used in various ntuser* functions and ntgdi*
* ReactOS specific: It is not in use yet?
*SystemResourcesList
*--*/
BOOLEAN
APIENTRY
DxEngLockShareSem(VOID)
{
    DPRINT1("ReactX Calling : DxEngLockShareSem\n");
    if(!ghsemShareDevLock) ghsemShareDevLock = EngCreateSemaphore(); // Hax, should be in dllmain.c
    EngAcquireSemaphore(ghsemShareDevLock);
    return TRUE;
}

/*++
* @name DxEngUnlockShareSem
* @implemented
*
* The function DxEngUnlockShareSem unlocks the struct of type ghsemShareDevLock.
*
* @return
* This function returns TRUE no matter what.
*
* @remarks.
* ReactOS specific: It is not in use yet?
*
*--*/
BOOLEAN
APIENTRY
DxEngUnlockShareSem(VOID)
{
    DPRINT1("ReactX Calling : DxEngUnlockShareSem\n");
    EngReleaseSemaphore(ghsemShareDevLock);
    return TRUE;
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
* Value to change gamma ramp to.
*
* @param BOOL Test
* Whether gamma should be tested. TRUE to test, FALSE to not test.
*
* @return
*Returns TRUE for success, FALSE for failure.
*
* @remarks.
* None
*
*--*/
BOOLEAN
APIENTRY
DxEngSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL Test)
{
    DPRINT1("ReactX Calling : DxEngSetDeviceGammaRamp\n");
    return IntSetDeviceGammaRamp(hPDev, Ramp, Test);
}

/*++
* @name DxEngGetHdevData
* @implemented
*
* The function DxEngGetHdevData retrieves a value from the HDEV

* @param HDEV hPDev
* The HDEV
*
* @param DXEGSHDEVDATA Type
* The following typs are supported
* Type                                            Purpose
* DxEGShDevData_Surface      Retrieve pointer to Surface handle.
* DxEGShDevData_hSpooler     Device object of graphics driver.
* DxEGShDevData_DitherFmt    Retrieve the device iDitherFormat
* DxEGShDevData_FxCaps       Retrieve the device flGraphicsCaps
* DxEGShDevData_FxCaps2      Retrieve the device flGraphicsCaps2
* DxEGShDevData_DrvFuncs     Retrieve the device DriverFunctions function table
* DxEGShDevData_dhpdev       Retrieve the device hPDev, the real DHPDEV
* DxEGShDevData_eddg         Retrieve the device pEDDgpl
* DxEGShDevData_dd_nCount    Retrieve the device DxDd_nCount
* DxEGShDevData_dd_flags     Retrieve the device DxDd_Flags
* DxEGShDevData_disable      See if the device pdev is disabled
* DxEGShDevData_metadev      See if the device pdev is a meta device
* DxEGShDevData_display      See if the device is the primary display driver
* DxEGShDevData_Parent       Retrieve the ppdevParent
* DxEGShDevData_OpenRefs     Retrieve the pdevOpenRefs counter
* DxEGShDevData_palette      See if the device RC_PALETTE is set
* DxEGShDevData_ldev         ATM we do not support the Loader Device driver structure
* DxEGShDevData_GDev         Retrieve the device pGraphicsDevice
* DxEGShDevData_clonedev     Retrieve the device PDEV_CLONE_DEVICE flag is set or not
*
* @return
* Returns the data we requested
*
* @remarks.
* ReactOS specific: Implementation is incomplete, I do not save the value into the hdev yet.
*
*--*/
DWORD_PTR
APIENTRY
DxEngGetHdevData(HDEV hDev,
                 DXEGSHDEVDATA Type)
{
    DWORD_PTR retVal = 0;
    PPDEVOBJ PDev = (PPDEVOBJ)hDev;

    DPRINT1("ReactX Calling : DxEngGetHdevData DXEGSHDEVDATA : %ld\n", Type);

#if 1
    DPRINT1("HDEV hDev %p\n", hDev);
#endif

    switch ( Type )
    {
      case DxEGShDevData_Surface:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_Surface\n");
        retVal = (DWORD_PTR) PDev->pSurface; // ptr to Surface handle.
        break;
      case DxEGShDevData_hSpooler:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_hSpooler\n");
        retVal = (DWORD_PTR) PDev->hSpooler;
        break;
      case DxEGShDevData_DitherFmt:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_DitherFmt\n");
        retVal = (DWORD_PTR) PDev->devinfo.iDitherFormat;
        break;
      case DxEGShDevData_FxCaps:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_FxCaps\n");
        retVal = (DWORD_PTR) PDev->devinfo.flGraphicsCaps;
        break;
      case DxEGShDevData_FxCaps2:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_FxCaps2\n");
        retVal = (DWORD_PTR) PDev->devinfo.flGraphicsCaps2;
        break;
      case DxEGShDevData_DrvFuncs:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_DrvFuncs\n");
        retVal = (DWORD_PTR) &PDev->DriverFunctions;
        break;
      case DxEGShDevData_dhpdev:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_dhpdev\n");
        retVal = (DWORD_PTR) PDev->dhpdev; // DHPDEV
        break;
      case DxEGShDevData_eddg:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_eddg\n");
        retVal = (DWORD_PTR) PDev->pEDDgpl;
        break;
      case DxEGShDevData_dd_nCount:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_dd_nCount\n");
        retVal = (DWORD_PTR) PDev->DxDd_nCount;
        break;
      case DxEGShDevData_dd_flags:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_dd_flags\n");
        retVal = (DWORD_PTR) PDev->DxDd_Flags;
        break;
      case DxEGShDevData_disable:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_disable\n");
        retVal = (DWORD_PTR) PDev->flFlags & PDEV_DISABLED;
        break;
      case DxEGShDevData_metadev:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_metadev\n");
        retVal = (DWORD_PTR) PDev->flFlags & PDEV_META_DEVICE;
        break;
      case DxEGShDevData_display:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_display\n");
        retVal = (DWORD_PTR) PDev->flFlags & PDEV_DISPLAY;
        break;
      case DxEGShDevData_Parent:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_Parent\n");
        retVal = (DWORD_PTR) PDev->ppdevParent;
        break;
      case DxEGShDevData_OpenRefs:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_OpenRefs\n");
        retVal = (DWORD_PTR) PDev->cPdevOpenRefs != 0;
        break;
      case DxEGShDevData_palette:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_palette\n");
        retVal = (DWORD_PTR) PDev->gdiinfo.flRaster & RC_PALETTE;
        break;
      case DxEGShDevData_ldev:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_ldev\n");
        retVal = (DWORD_PTR) PDev->pldev;
        break;
      case DxEGShDevData_GDev:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_GDev\n");
        retVal = (DWORD_PTR) PDev->pGraphicsDevice; // P"GRAPHICS_DEVICE"
        break;
      case DxEGShDevData_clonedev:
        DPRINT1("requested DXEGSHDEVDATA DxEGShDevData_clonedev\n");
        retVal = (DWORD_PTR) PDev->flFlags & PDEV_CLONE_DEVICE;
        break;

      default:
        break;
    }

#if 1
    DPRINT1("return value %08lx\n", retVal);
#endif

    return retVal;

}

/*++
* @name DxEngSetHdevData
* @implemented
*
* The function DxEngSetHdevData sets a value in hdev

* @param HDEV hPDev
* The hdev
*
* @param DXEGSHDEVDATA Type
* Supports only DxEGShDevData_dd_nCount. It is an internal counter on how many times hdev has been locked and unlocked
*
* @param DWORD Data
* The value to be saved to hdev's internal counter.
*
* @return
* Returns TRUE for success, FALSE for failure
*
* @remarks.
* none
*
*--*/
BOOLEAN
APIENTRY
DxEngSetHdevData(HDEV hDev,
                 DXEGSHDEVDATA Type,
                 DWORD_PTR Data)
{
    BOOLEAN retVal = FALSE; // Default, no set.

    DPRINT1("ReactX Calling : DxEngSetHdevData DXEGSHDEVDATA : %ld\n", Type);

    if ( Type == DxEGShDevData_dd_nCount )
    {
        ((PPDEVOBJ)hDev)->DxDd_nCount = Data;
        retVal = TRUE; // Set
    }
    return retVal;
}

/*++
* @name DxEngGetDCState
* @implemented
*
* The function DxEngGetDCState is capable of returning three
* DC states depending on what value is passed in its second parameter:
* 1. If the DC is full screen
* 2. Get Complexity of visible region
* 3. Get Driver hdev, which is ppdev
*
* @param HDC hdc
* The DC handle
*
* @param DWORD type
* value 1 = Is DC fullscreen
* value 2 = Get Complexity of visible region.
* value 3 = Get Driver hdev, which is a ppdev.
*
* @return
* Return one of the type values
*
* @remarks.
* none
*
*--*/
DWORD_PTR
APIENTRY
DxEngGetDCState(HDC hDC,
                DWORD type)
{
    PDC pDC = DC_LockDc(hDC);
    DWORD_PTR retVal = 0;

    DPRINT1("ReactX Calling : DxEngGetDCState type : %lu\n", type);

    if (pDC)
    {
        switch (type)
        {
            case 1:
                retVal = (DWORD_PTR) pDC->fs & DC_FLAG_FULLSCREEN;
                break;
            case 2:
                /* Return the complexity of the visible region. */
                retVal = (DWORD_PTR) REGION_Complexity(pDC->prgnVis);
                break;
            case 3:
            {
                /* Return the HDEV of this DC. */
                retVal = (DWORD_PTR) pDC->ppdev;
                break;
            }
            default:
                /* If a valid type is not found, zero is returned */
                DPRINT1("Warning: did not find type %lu\n", type);
                break;
        }
        DC_UnlockDc(pDC);
    }

    DPRINT1("Return value %08lx\n", retVal);

    return retVal;
}

/*++
* @name DxEngIncDispUniq
* @implemented
*
* The function DxEngIncDispUniq increments the DisplayUniqVisrgn counter from GDI shared memory.
*
* @return
* This function returns TRUE no matter what.
*
* @remarks.
* none
*
*--*/
BOOLEAN
APIENTRY
DxEngIncDispUniq(VOID)
{
    DPRINT1("ReactX Calling : DxEngIncDispUniq \n");

    InterlockedIncrement((LONG*)&GdiHandleTable->flDeviceUniq);
    return TRUE;
}

/*++
* @name DxEngLockHdev
* @implemented
*
* The function DxEngLockHdev lock the internal PDEV
*
* @param HDEV type
* it is a pointer to win32k internal pdev struct known as PPDEVOBJ

* @return
* This function returns TRUE no matter what.
*
* @remarks.
* none
*
*--*/
BOOLEAN
APIENTRY
DxEngLockHdev(HDEV hDev)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)hDev;
    PERESOURCE Resource;

    DPRINT1("ReactX Calling : DxEngLockHdev \n");

    DPRINT1("hDev                   : 0x%p\n",hDev);

    Resource = (PERESOURCE)ppdev->hsemDevLock;

    if (Resource)
    {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite( Resource , TRUE); // Lock monitor.
    }
    return TRUE;
}

/*++
* @name DxEngUnlockHdev
* @implemented
*
* The function DxEngUnlockHdev unlock the internal PDEV
*
* @param HDEV type
* it is a pointer to win32k internal pdev struct known as PPDEVOBJ

* @return
* This function returns TRUE no matter what.
*
* @remarks.
* none
*
*--*/
BOOLEAN
APIENTRY
DxEngUnlockHdev(HDEV hDev)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)hDev;
    PERESOURCE Resource = (PERESOURCE)ppdev->hsemDevLock;

    DPRINT1("ReactX Calling : DxEngUnlockHdev \n");

    if (Resource)
    {
        ExReleaseResourceLite( Resource );
        KeLeaveCriticalRegion();
    }
    return TRUE;
}


/************************************************************************/
/* DxEngReferenceHdev                                                   */
/************************************************************************/
BOOLEAN
APIENTRY
DxEngReferenceHdev(HDEV hDev)
{
    IntGdiReferencePdev((PPDEVOBJ) hDev);
    /* ALWAYS return true */
    return TRUE;
}

/************************************************************************/
/* DxEngNUIsTermSrv                                                     */
/************************************************************************/

/* Notes: Check if terminal server got connections or not */
BOOLEAN
APIENTRY
DxEngNUIsTermSrv(VOID)
{
    /* FIXME: ReactOS does not suport terminal server yet, we can not check if we got connections or not */
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngRedrawDesktop                                                   */
/************************************************************************/

/* Notes: it always returns TRUE, and it updates whole screen
   (redraws current desktop) */
BOOLEAN
APIENTRY
DxEngRedrawDesktop(VOID)
{
    UserRedrawDesktop();
    return TRUE;
}


ULONG gulVisRgnUniqueness; // Increase count everytime client region is updated.

/************************************************************************/
/* DxEngVisRgnUniq                                                      */
/************************************************************************/
/* Notes: returns the VisRgnUniq counter for win32k */
ULONG
APIENTRY
DxEngVisRgnUniq(VOID)
{
    DPRINT1("ReactX Calling : DxEngVisRgnUniq \n");

    return gulVisRgnUniqueness;
}

/************************************************************************/
/* DxEngEnumerateHdev                                                   */
/************************************************************************/
/* Enumerate all drivers in win32k */
HDEV *
APIENTRY
DxEngEnumerateHdev(HDEV *hdev)
{
    /* FIXME: Enumerate all drivers in win32k */
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngCreateMemoryDC                                                  */
/************************************************************************/
HDC
APIENTRY
DxEngCreateMemoryDC(HDEV hDev)
{
    return IntGdiCreateDisplayDC(hDev, DC_TYPE_MEMORY, FALSE);
}

/************************************************************************/
/* DxEngScreenAccessCheck                                               */
/************************************************************************/
DWORD APIENTRY DxEngScreenAccessCheck(VOID)
{
    UNIMPLEMENTED;

    /* We're cheating here and telling dxg.sys it has always had permissions to access the screen */
    return TRUE;
}

/************************************************************************/
/* DxEngIsHdevLockedByCurrentThread                                     */
/************************************************************************/
BOOLEAN
APIENTRY
DxEngIsHdevLockedByCurrentThread(HDEV hDev)
{   // Based on EngIsSemaphoreOwnedByCurrentThread w/o the Ex call.
    PERESOURCE pSem = (PERESOURCE)(((PPDEVOBJ)hDev)->hsemDevLock);
    return pSem->OwnerEntry.OwnerThread == (ERESOURCE_THREAD)PsGetCurrentThread();
}


/************************************************************************/
/* DxEngUnreferenceHdev                                                 */
/************************************************************************/
BOOLEAN
APIENTRY
DxEngUnreferenceHdev(HDEV hDev)
{
    IntGdiUnreferencePdev((PPDEVOBJ) hDev, 0);
    return TRUE; // Always true.
}

/************************************************************************/
/* DxEngGetDesktopDC                                                    */
/************************************************************************/
HDC
APIENTRY
DxEngGetDesktopDC(ULONG DcType, BOOL EmptyDC, BOOL ValidatehWnd)
{
    return UserGetDesktopDC(DcType, EmptyDC, ValidatehWnd);
}

/************************************************************************/
/* DxEngDeleteDC                                                        */
/************************************************************************/
BOOLEAN
APIENTRY
DxEngDeleteDC(HDC hdc, BOOL Force)
{
   return IntGdiDeleteDC(hdc, Force);
}

/************************************************************************/
/* DxEngCleanDC                                                         */
/************************************************************************/
BOOLEAN
APIENTRY
DxEngCleanDC(HDC hdc)
{
    return IntGdiCleanDC(hdc);
}

/************************************************************************/
/* DxEngSetDCOwner                                                      */
/************************************************************************/
BOOL APIENTRY DxEngSetDCOwner(HGDIOBJ hObject, DWORD OwnerMask)
{
    DPRINT1("ReactX Calling : DxEngSetDCOwner \n");

    return GreSetDCOwner(hObject, OwnerMask);
}

/************************************************************************/
/* DxEngSetDCState                                                      */
/************************************************************************/
BOOLEAN
APIENTRY
DxEngSetDCState(HDC hDC, DWORD SetType, DWORD Set)
{
   BOOLEAN Ret = FALSE;
   PDC pDC = DC_LockDc(hDC);

   if (pDC)
   {
      if (SetType == 1)
      {
        if ( Set )
            pDC->fs |= DC_FLAG_FULLSCREEN;
        else
            pDC->fs &= ~DC_FLAG_FULLSCREEN;
        Ret = TRUE;
      }
      DC_UnlockDc(pDC);
      return Ret; // Everything else returns FALSE.
   }
   return Ret;
}

/************************************************************************/
/* DxEngSelectBitmap                                                    */
/************************************************************************/
DWORD APIENTRY DxEngSelectBitmap(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSetBitmapOwner                                                  */
/************************************************************************/
DWORD APIENTRY DxEngSetBitmapOwner(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngDeleteSurface                                                   */
/************************************************************************/
DWORD APIENTRY DxEngDeleteSurface(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngGetSurfaceData                                                  */
/************************************************************************/
DWORD APIENTRY DxEngGetSurfaceData(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngAltLockSurface                                                  */
/************************************************************************/
DWORD APIENTRY DxEngAltLockSurface(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngUploadPaletteEntryToSurface                                     */
/************************************************************************/
DWORD APIENTRY DxEngUploadPaletteEntryToSurface(DWORD x1, DWORD x2,DWORD x3, DWORD x4)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngMarkSurfaceAsDirectDraw                                         */
/************************************************************************/
DWORD APIENTRY DxEngMarkSurfaceAsDirectDraw(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSelectPaletteToSurface                                          */
/************************************************************************/
DWORD APIENTRY DxEngSelectPaletteToSurface(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSyncPaletteTableWithDevice                                      */
/************************************************************************/
DWORD APIENTRY DxEngSyncPaletteTableWithDevice(DWORD x1, DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSetPaletteState                                                 */
/************************************************************************/
DWORD APIENTRY DxEngSetPaletteState(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngGetRedirectionBitmap                                            */
/************************************************************************/
DWORD
APIENTRY
DxEngGetRedirectionBitmap(DWORD x1)
{
    return FALSE; // Normal return.
}

/************************************************************************/
/* DxEngLoadImage                                                       */
/************************************************************************/
DWORD APIENTRY DxEngLoadImage(DWORD x1,DWORD x2)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSpTearDownSprites                                               */
/************************************************************************/
DWORD APIENTRY DxEngSpTearDownSprites(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSpUnTearDownSprites                                             */
/************************************************************************/
DWORD APIENTRY DxEngSpUnTearDownSprites(DWORD x1, DWORD x2, DWORD x3)
{
    UNIMPLEMENTED;
    return FALSE;
}

/************************************************************************/
/* DxEngSpSpritesVisible                                                */
/************************************************************************/
DWORD APIENTRY DxEngSpSpritesVisible(DWORD x1)
{
    UNIMPLEMENTED;
    return FALSE;
}
