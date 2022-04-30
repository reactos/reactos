/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Support for physical devices
 * FILE:             win32ss/gdi/eng/pdevobj.c
 * PROGRAMER:        Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>
DBG_DEFAULT_CHANNEL(EngPDev);

static PPDEVOBJ gppdevList = NULL;
static HSEMAPHORE ghsemPDEV;

BOOL
APIENTRY
MultiEnableDriver(
    _In_ ULONG iEngineVersion,
    _In_ ULONG cj,
    _Inout_bytecount_(cj) PDRVENABLEDATA pded);

CODE_SEG("INIT")
NTSTATUS
NTAPI
InitPDEVImpl(VOID)
{
    ghsemPDEV = EngCreateSemaphore();
    if (!ghsemPDEV) return STATUS_INSUFFICIENT_RESOURCES;
    return STATUS_SUCCESS;
}

#if DBG
PPDEVOBJ
NTAPI
DbgLookupDHPDEV(DHPDEV dhpdev)
{
    PPDEVOBJ ppdev;

    /* Lock PDEV list */
    EngAcquireSemaphoreShared(ghsemPDEV);

    /* Walk through the list of PDEVs */
    for (ppdev = gppdevList;  ppdev; ppdev = ppdev->ppdevNext)
    {
        /* Compare with the given DHPDEV */
        if (ppdev->dhpdev == dhpdev) break;
    }

    /* Unlock PDEV list */
    EngReleaseSemaphore(ghsemPDEV);

    return ppdev;
}
#endif

PPDEVOBJ
PDEVOBJ_AllocPDEV(VOID)
{
    PPDEVOBJ ppdev;

    ppdev = ExAllocatePoolWithTag(PagedPool, sizeof(PDEVOBJ), GDITAG_PDEV);
    if (!ppdev)
        return NULL;

    RtlZeroMemory(ppdev, sizeof(PDEVOBJ));

    ppdev->hsemDevLock = EngCreateSemaphore();
    if (ppdev->hsemDevLock == NULL)
    {
        ExFreePoolWithTag(ppdev, GDITAG_PDEV);
        return NULL;
    }

    /* Allocate EDD_DIRECTDRAW_GLOBAL for our ReactX driver */
    ppdev->pEDDgpl = ExAllocatePoolWithTag(PagedPool, sizeof(EDD_DIRECTDRAW_GLOBAL), GDITAG_PDEV);
    if (ppdev->pEDDgpl)
        RtlZeroMemory(ppdev->pEDDgpl, sizeof(EDD_DIRECTDRAW_GLOBAL));

    ppdev->cPdevRefs = 1;

    return ppdev;
}

static
VOID
PDEVOBJ_vDeletePDEV(
    PPDEVOBJ ppdev)
{
    EngDeleteSemaphore(ppdev->hsemDevLock);
    if (ppdev->pdmwDev)
        ExFreePoolWithTag(ppdev->pdmwDev, GDITAG_DEVMODE);
    if (ppdev->pEDDgpl)
        ExFreePoolWithTag(ppdev->pEDDgpl, GDITAG_PDEV);
    ExFreePoolWithTag(ppdev, GDITAG_PDEV);
}

VOID
NTAPI
PDEVOBJ_vRelease(
    _Inout_ PPDEVOBJ ppdev)
{
    /* Lock loader */
    EngAcquireSemaphore(ghsemPDEV);

    /* Decrease reference count */
    InterlockedDecrement(&ppdev->cPdevRefs);
    ASSERT(ppdev->cPdevRefs >= 0);

    /* Check if references are left */
    if (ppdev->cPdevRefs == 0)
    {
        /* Do we have a surface? */
        if (ppdev->pSurface)
        {
            /* Release the surface and let the driver free it */
            SURFACE_ShareUnlockSurface(ppdev->pSurface);
            TRACE("DrvDisableSurface(dhpdev %p)\n", ppdev->dhpdev);
            ppdev->pfn.DisableSurface(ppdev->dhpdev);
        }

        /* Do we have a palette? */
        if (ppdev->ppalSurf)
        {
            PALETTE_ShareUnlockPalette(ppdev->ppalSurf);
        }

        /* Check if the PDEV was enabled */
        if (ppdev->dhpdev != NULL)
        {
            /* Disable the PDEV */
            TRACE("DrvDisablePDEV(dhpdev %p)\n", ppdev->dhpdev);
            ppdev->pfn.DisablePDEV(ppdev->dhpdev);
        }

        /* Remove it from list */
        if (ppdev == gppdevList)
        {
            gppdevList = ppdev->ppdevNext;
        }
        else if (gppdevList)
        {
            PPDEVOBJ ppdevCurrent = gppdevList;
            BOOL found = FALSE;
            while (!found && ppdevCurrent->ppdevNext)
            {
                if (ppdevCurrent->ppdevNext == ppdev)
                    found = TRUE;
                else
                    ppdevCurrent = ppdevCurrent->ppdevNext;
            }
            if (found)
                ppdevCurrent->ppdevNext = ppdev->ppdevNext;
        }

        /* Unload display driver */
        EngUnloadImage(ppdev->pldev);

        /* Free it */
        PDEVOBJ_vDeletePDEV(ppdev);
    }

    /* Unlock loader */
    EngReleaseSemaphore(ghsemPDEV);
}

BOOL
NTAPI
PDEVOBJ_bEnablePDEV(
    _In_ PPDEVOBJ ppdev,
    _In_ PDEVMODEW pdevmode,
    _In_ PWSTR pwszLogAddress)
{
    PFN_DrvEnablePDEV pfnEnablePDEV;
    ULONG i;

    /* Get the DrvEnablePDEV function */
    pfnEnablePDEV = ppdev->pldev->pfn.EnablePDEV;

    /* Call the drivers DrvEnablePDEV function */
    TRACE("DrvEnablePDEV(pdevmode %p (%dx%dx%d %d Hz) hdev %p (%S))\n",
        pdevmode,
        ppdev->pGraphicsDevice ? pdevmode->dmPelsWidth : 0,
        ppdev->pGraphicsDevice ? pdevmode->dmPelsHeight : 0,
        ppdev->pGraphicsDevice ? pdevmode->dmBitsPerPel : 0,
        ppdev->pGraphicsDevice ? pdevmode->dmDisplayFrequency : 0,
        ppdev,
        ppdev->pGraphicsDevice ? ppdev->pGraphicsDevice->szNtDeviceName : L"");
    ppdev->dhpdev = pfnEnablePDEV(pdevmode,
                                  pwszLogAddress,
                                  HS_DDI_MAX,
                                  ppdev->ahsurf,
                                  sizeof(GDIINFO),
                                  (PULONG)&ppdev->gdiinfo,
                                  sizeof(DEVINFO),
                                  &ppdev->devinfo,
                                  (HDEV)ppdev,
                                  ppdev->pGraphicsDevice ? ppdev->pGraphicsDevice->pwszDescription : NULL,
                                  ppdev->pGraphicsDevice ? ppdev->pGraphicsDevice->DeviceObject : NULL);
    TRACE("DrvEnablePDEV(pdevmode %p hdev %p) => dhpdev %p\n", pdevmode, ppdev, ppdev->dhpdev);
    if (ppdev->dhpdev == NULL)
    {
        ERR("Failed to enable PDEV\n");
        return FALSE;
    }

    /* Fix up some values */
    if (ppdev->gdiinfo.ulLogPixelsX == 0)
        ppdev->gdiinfo.ulLogPixelsX = 96;

    if (ppdev->gdiinfo.ulLogPixelsY == 0)
        ppdev->gdiinfo.ulLogPixelsY = 96;

    /* Set raster caps */
    ppdev->gdiinfo.flRaster = RC_OP_DX_OUTPUT | RC_GDI20_OUTPUT | RC_BIGFONT;
    if ((ppdev->gdiinfo.ulTechnology != DT_PLOTTER) && (ppdev->gdiinfo.ulTechnology != DT_CHARSTREAM))
        ppdev->gdiinfo.flRaster |= RC_STRETCHDIB | RC_STRETCHBLT | RC_DIBTODEV | RC_DI_BITMAP | RC_BITMAP64 | RC_BITBLT;
    if (ppdev->gdiinfo.ulTechnology == DT_RASDISPLAY)
        ppdev->gdiinfo.flRaster |= RC_FLOODFILL;
    if (ppdev->devinfo.flGraphicsCaps & GCAPS_PALMANAGED)
        ppdev->gdiinfo.flRaster |= RC_PALETTE;

    /* Setup Palette */
    ppdev->ppalSurf = PALETTE_ShareLockPalette(ppdev->devinfo.hpalDefault);

    /* Setup hatch brushes */
    for (i = 0; i < HS_DDI_MAX; i++)
    {
        if (ppdev->ahsurf[i] == NULL)
            ppdev->ahsurf[i] = gahsurfHatch[i];
    }

    TRACE("PDEVOBJ_bEnablePDEV - dhpdev = %p\n", ppdev->dhpdev);

    return TRUE;
}

VOID
NTAPI
PDEVOBJ_vCompletePDEV(
    PPDEVOBJ ppdev)
{
    /* Call the drivers DrvCompletePDEV function */
    TRACE("DrvCompletePDEV(dhpdev %p hdev %p)\n", ppdev->dhpdev, ppdev);
    ppdev->pldev->pfn.CompletePDEV(ppdev->dhpdev, (HDEV)ppdev);
}

static
VOID
PDEVOBJ_vFilterDriverHooks(
    _In_ PPDEVOBJ ppdev)
{
    PLDEVOBJ pldev = ppdev->pldev;
    ULONG dwAccelerationLevel = ppdev->dwAccelerationLevel;

    if (!pldev->pGdiDriverInfo)
        return;
    if (pldev->ldevtype != LDEV_DEVICE_DISPLAY)
        return;

    if (dwAccelerationLevel >= 1)
    {
        ppdev->apfn[INDEX_DrvSetPointerShape] = NULL;
        ppdev->apfn[INDEX_DrvCreateDeviceBitmap] = NULL;
    }

    if (dwAccelerationLevel >= 2)
    {
        /* Remove sophisticated display accelerations */
        ppdev->pSurface->flags &= ~(HOOK_STRETCHBLT |
                                    HOOK_FILLPATH |
                                    HOOK_GRADIENTFILL |
                                    HOOK_LINETO |
                                    HOOK_ALPHABLEND |
                                    HOOK_TRANSPARENTBLT);
    }

    if (dwAccelerationLevel >= 3)
    {
        /* Disable DirectDraw and Direct3D accelerations */
        /* FIXME: need to call DxDdSetAccelLevel */
        UNIMPLEMENTED;
    }

    if (dwAccelerationLevel >= 4)
    {
        /* Remove almost all display accelerations */
        ppdev->pSurface->flags &= ~HOOK_FLAGS |
                                   HOOK_BITBLT |
                                   HOOK_COPYBITS |
                                   HOOK_TEXTOUT |
                                   HOOK_STROKEPATH |
                                   HOOK_SYNCHRONIZE;

    }

    if (dwAccelerationLevel >= 5)
    {
        /* Disable all display accelerations */
        UNIMPLEMENTED;
    }
}

PSURFACE
NTAPI
PDEVOBJ_pSurface(
    PPDEVOBJ ppdev)
{
    HSURF hsurf;

    /* Check if there is no surface for this PDEV yet */
    if (ppdev->pSurface == NULL)
    {
        /* Call the drivers DrvEnableSurface */
        TRACE("DrvEnableSurface(dhpdev %p)\n", ppdev->dhpdev);
        hsurf = ppdev->pldev->pfn.EnableSurface(ppdev->dhpdev);
        TRACE("DrvEnableSurface(dhpdev %p) => hsurf %p\n", ppdev->dhpdev, hsurf);
        if (hsurf== NULL)
        {
            ERR("Failed to create PDEV surface!\n");
            return NULL;
        }

        /* Get a reference to the surface */
        ppdev->pSurface = SURFACE_ShareLockSurface(hsurf);
        NT_ASSERT(ppdev->pSurface != NULL);
    }

    /* Increment reference count */
    GDIOBJ_vReferenceObjectByPointer(&ppdev->pSurface->BaseObject);

    return ppdev->pSurface;
}

VOID
PDEVOBJ_vEnableDisplay(
    _Inout_ PPDEVOBJ ppdev)
{
    BOOL assertVal;

    if (!(ppdev->flFlags & PDEV_DISABLED))
        return;

    /* Try to enable display until success */
    do
    {
        TRACE("DrvAssertMode(dhpdev %p, TRUE)\n", ppdev->dhpdev);
        assertVal = ppdev->pfn.AssertMode(ppdev->dhpdev, TRUE);
        TRACE("DrvAssertMode(dhpdev %p, TRUE) => %d\n", ppdev->dhpdev, assertVal);
    } while (!assertVal);

    ppdev->flFlags &= ~PDEV_DISABLED;
}

BOOL
PDEVOBJ_bDisableDisplay(
    _Inout_ PPDEVOBJ ppdev)
{
    BOOL assertVal;

    if (ppdev->flFlags & PDEV_DISABLED)
        return TRUE;

    TRACE("DrvAssertMode(dhpdev %p, FALSE)\n", ppdev->dhpdev);
    assertVal = ppdev->pfn.AssertMode(ppdev->dhpdev, FALSE);
    TRACE("DrvAssertMode(dhpdev %p, FALSE) => %d\n", ppdev->dhpdev, assertVal);

    if (assertVal)
        ppdev->flFlags |= PDEV_DISABLED;

    return assertVal;
}

VOID
NTAPI
PDEVOBJ_vRefreshModeList(
    PPDEVOBJ ppdev)
{
    PGRAPHICS_DEVICE pGraphicsDevice;
    PDEVMODEINFO pdminfo, pdmiNext;

    /* Lock the PDEV */
    EngAcquireSemaphore(ppdev->hsemDevLock);

    pGraphicsDevice = ppdev->pGraphicsDevice;

    /* Clear out the modes */
    for (pdminfo = pGraphicsDevice->pdevmodeInfo;
         pdminfo;
         pdminfo = pdmiNext)
    {
        pdmiNext = pdminfo->pdmiNext;
        ExFreePoolWithTag(pdminfo, GDITAG_DEVMODE);
    }
    pGraphicsDevice->pdevmodeInfo = NULL;
    ExFreePoolWithTag(pGraphicsDevice->pDevModeList, GDITAG_GDEVICE);
    pGraphicsDevice->pDevModeList = NULL;

    /* Update available display mode list */
    LDEVOBJ_bBuildDevmodeList(pGraphicsDevice);

    /* Unlock PDEV */
    EngReleaseSemaphore(ppdev->hsemDevLock);
}

PPDEVOBJ
PDEVOBJ_Create(
    _In_opt_ PGRAPHICS_DEVICE pGraphicsDevice,
    _In_opt_ PDEVMODEW pdm,
    _In_ ULONG dwAccelerationLevel,
    _In_ ULONG ldevtype)
{
    PPDEVOBJ ppdev, ppdevMatch = NULL;
    PLDEVOBJ pldev;
    PSURFACE pSurface;

    TRACE("PDEVOBJ_Create(%p %p %d)\n", pGraphicsDevice, pdm, ldevtype);

    if (ldevtype != LDEV_DEVICE_META)
    {
        ASSERT(pGraphicsDevice);
        ASSERT(pdm);
        /* Search if we already have a PPDEV with the required characteristics.
         * We will compare the graphics device, the devmode and the desktop
         */
        for (ppdev = gppdevList; ppdev; ppdev = ppdev->ppdevNext)
        {
            if (ppdev->pGraphicsDevice == pGraphicsDevice)
            {
                PDEVOBJ_vReference(ppdev);

                if (RtlEqualMemory(pdm, ppdev->pdmwDev, sizeof(DEVMODEW)) &&
                    ppdev->dwAccelerationLevel == dwAccelerationLevel)
                {
                    PDEVOBJ_vReference(ppdev);
                    ppdevMatch = ppdev;
                }
                else
                {
                    PDEVOBJ_bDisableDisplay(ppdev);
                }

                PDEVOBJ_vRelease(ppdev);
            }
        }

        if (ppdevMatch)
        {
            PDEVOBJ_vEnableDisplay(ppdevMatch);

            return ppdevMatch;
        }
    }

    /* Try to get a display driver */
    if (ldevtype == LDEV_DEVICE_META)
        pldev = LDEVOBJ_pLoadInternal(MultiEnableDriver, ldevtype);
    else
        pldev = LDEVOBJ_pLoadDriver(pdm->dmDeviceName, ldevtype);
    if (!pldev)
    {
        ERR("Could not load display driver '%S'\n",
             (ldevtype == LDEV_DEVICE_META) ? L"" : pdm->dmDeviceName);
        return NULL;
    }

    /* Allocate a new PDEVOBJ */
    ppdev = PDEVOBJ_AllocPDEV();
    if (!ppdev)
    {
        ERR("failed to allocate a PDEV\n");
        return NULL;
    }

    if (ldevtype != LDEV_DEVICE_META)
    {
        ppdev->pGraphicsDevice = pGraphicsDevice;

        // DxEngGetHdevData asks for Graphics DeviceObject in hSpooler field
        ppdev->hSpooler = ppdev->pGraphicsDevice->DeviceObject;

        /* Keep selected resolution */
        if (ppdev->pdmwDev)
            ExFreePoolWithTag(ppdev->pdmwDev, GDITAG_DEVMODE);
        ppdev->pdmwDev = ExAllocatePoolWithTag(PagedPool, pdm->dmSize + pdm->dmDriverExtra, GDITAG_DEVMODE);
        if (ppdev->pdmwDev)
        {
            RtlCopyMemory(ppdev->pdmwDev, pdm, pdm->dmSize + pdm->dmDriverExtra);
            /* FIXME: this must be done in a better way */
            pGraphicsDevice->StateFlags |= DISPLAY_DEVICE_PRIMARY_DEVICE | DISPLAY_DEVICE_ATTACHED_TO_DESKTOP;
        }
    }

    /* FIXME! */
    ppdev->flFlags = PDEV_DISPLAY;

    /* HACK: Don't use the pointer */
    ppdev->Pointer.Exclude.right = -1;

    /* Initialize PDEV */
    ppdev->pldev = pldev;
    ppdev->dwAccelerationLevel = dwAccelerationLevel;

    /* Call the driver to enable the PDEV */
    if (!PDEVOBJ_bEnablePDEV(ppdev, pdm, NULL))
    {
        ERR("Failed to enable PDEV!\n");
        PDEVOBJ_vRelease(ppdev);
        EngUnloadImage(pldev);
        return NULL;
    }

    /* Copy the function table */
    ppdev->pfn = ppdev->pldev->pfn;

    /* Tell the driver that the PDEV is ready */
    PDEVOBJ_vCompletePDEV(ppdev);

    /* Create the initial surface */
    pSurface = PDEVOBJ_pSurface(ppdev);
    if (!pSurface)
    {
        ERR("Failed to create surface\n");
        PDEVOBJ_vRelease(ppdev);
        EngUnloadImage(pldev);
        return NULL;
    }

    /* Remove some acceleration capabilities from driver */
    PDEVOBJ_vFilterDriverHooks(ppdev);

    /* Set MovePointer function */
    ppdev->pfnMovePointer = ppdev->pfn.MovePointer;
    if (!ppdev->pfnMovePointer)
        ppdev->pfnMovePointer = EngMovePointer;

    /* Insert the PDEV into the list */
    ppdev->ppdevNext = gppdevList;
    gppdevList = ppdev;

    /* Return the PDEV */
    return ppdev;
}

FORCEINLINE
VOID
SwitchPointer(
    _Inout_ PVOID pvPointer1,
    _Inout_ PVOID pvPointer2)
{
    PVOID *ppvPointer1 = pvPointer1;
    PVOID *ppvPointer2 = pvPointer2;
    PVOID pvTemp;

    pvTemp = *ppvPointer1;
    *ppvPointer1 = *ppvPointer2;
    *ppvPointer2 = pvTemp;
}

BOOL
NTAPI
PDEVOBJ_bDynamicModeChange(
    PPDEVOBJ ppdev,
    PPDEVOBJ ppdev2)
{
    union
    {
        DRIVER_FUNCTIONS pfn;
        GDIINFO gdiinfo;
        DEVINFO devinfo;
        DWORD StateFlags;
    } temp;

    /* Exchange driver functions */
    temp.pfn = ppdev->pfn;
    ppdev->pfn = ppdev2->pfn;
    ppdev2->pfn = temp.pfn;

    /* Exchange LDEVs */
    SwitchPointer(&ppdev->pldev, &ppdev2->pldev);

    /* Exchange DHPDEV */
    SwitchPointer(&ppdev->dhpdev, &ppdev2->dhpdev);

    /* Exchange surfaces and associate them with their new PDEV */
    SwitchPointer(&ppdev->pSurface, &ppdev2->pSurface);
    ppdev->pSurface->SurfObj.hdev = (HDEV)ppdev;
    ppdev2->pSurface->SurfObj.hdev = (HDEV)ppdev2;

    /* Exchange devinfo */
    temp.devinfo = ppdev->devinfo;
    ppdev->devinfo = ppdev2->devinfo;
    ppdev2->devinfo = temp.devinfo;

    /* Exchange gdiinfo */
    temp.gdiinfo = ppdev->gdiinfo;
    ppdev->gdiinfo = ppdev2->gdiinfo;
    ppdev2->gdiinfo = temp.gdiinfo;

    /* Exchange DEVMODE */
    SwitchPointer(&ppdev->pdmwDev, &ppdev2->pdmwDev);

    /* Exchange state flags */
    temp.StateFlags = ppdev->pGraphicsDevice->StateFlags;
    ppdev->pGraphicsDevice->StateFlags = ppdev2->pGraphicsDevice->StateFlags;
    ppdev2->pGraphicsDevice->StateFlags = temp.StateFlags;

    /* Notify each driver instance of its new HDEV association */
    ppdev->pfn.CompletePDEV(ppdev->dhpdev, (HDEV)ppdev);
    ppdev2->pfn.CompletePDEV(ppdev2->dhpdev, (HDEV)ppdev2);

    return TRUE;
}


BOOL
NTAPI
PDEVOBJ_bSwitchMode(
    PPDEVOBJ ppdev,
    PDEVMODEW pdm)
{
    PPDEVOBJ ppdevTmp;
    PSURFACE pSurface;
    BOOL retval = FALSE;

    /* Lock the PDEV */
    EngAcquireSemaphore(ppdev->hsemDevLock);

    /* And everything else */
    EngAcquireSemaphore(ghsemPDEV);

    DPRINT1("PDEVOBJ_bSwitchMode, ppdev = %p, pSurface = %p\n", ppdev, ppdev->pSurface);

    // Lookup the GraphicsDevice + select DEVMODE
    // pdm = LDEVOBJ_bProbeAndCaptureDevmode(ppdev, pdm);

    /* 1. Temporarily disable the current PDEV and reset video to its default mode */
    if (!PDEVOBJ_bDisableDisplay(ppdev))
    {
        DPRINT1("PDEVOBJ_bDisableDisplay() failed\n");
        goto leave;
    }

    /* 2. Create new PDEV */
    ppdevTmp = PDEVOBJ_Create(ppdev->pGraphicsDevice, pdm, 0, LDEV_DEVICE_DISPLAY);
    if (!ppdevTmp)
    {
        DPRINT1("Failed to create a new PDEV\n");
        goto leave2;
    }

    /* 3. Create a new surface */
    pSurface = PDEVOBJ_pSurface(ppdevTmp);
    if (!pSurface)
    {
        DPRINT1("PDEVOBJ_pSurface failed\n");
        PDEVOBJ_vRelease(ppdevTmp);
        goto leave2;
    }

    /* 4. Get DirectDraw information */
    /* 5. Enable DirectDraw Not traced */
    /* 6. Copy old PDEV state to new PDEV instance */

    /* 7. Switch the PDEVs */
    if (!PDEVOBJ_bDynamicModeChange(ppdev, ppdevTmp))
    {
        DPRINT1("PDEVOBJ_bDynamicModeChange() failed\n");
        PDEVOBJ_vRelease(ppdevTmp);
        goto leave2;
    }

    /* 8. Disable DirectDraw */

    PDEVOBJ_vRelease(ppdevTmp);

    /* Update primary display capabilities */
    if (ppdev == gpmdev->ppdevGlobal)
    {
        PDEVOBJ_vGetDeviceCaps(ppdev, &GdiHandleTable->DevCaps);
    }

    /* Success! */
    retval = TRUE;

leave2:
    /* Set the new video mode, or restore the original one in case of failure */
    PDEVOBJ_vEnableDisplay(ppdev);

leave:
    /* Unlock everything else */
    EngReleaseSemaphore(ghsemPDEV);
    /* Unlock the PDEV */
    EngReleaseSemaphore(ppdev->hsemDevLock);

    DPRINT1("leave, ppdev = %p, pSurface = %p\n", ppdev, ppdev->pSurface);

    return retval;
}


PPDEVOBJ
NTAPI
EngpGetPDEV(
    _In_opt_ PUNICODE_STRING pustrDeviceName)
{
    UNICODE_STRING ustrCurrent;
    PPDEVOBJ ppdev = NULL;
    PGRAPHICS_DEVICE pGraphicsDevice;
    ULONG i;

    /* Acquire PDEV lock */
    EngAcquireSemaphore(ghsemPDEV);

    /* Did the caller pass a device name? */
    if (pustrDeviceName)
    {
        /* Loop all present PDEVs */
        for (i = 0; i < gpmdev->cDev; i++)
        {
            /* Get a pointer to the GRAPHICS_DEVICE */
            pGraphicsDevice = gpmdev->dev[i].ppdev->pGraphicsDevice;

            /* Compare the name */
            RtlInitUnicodeString(&ustrCurrent, pGraphicsDevice->szWinDeviceName);
            if (RtlEqualUnicodeString(pustrDeviceName, &ustrCurrent, FALSE))
            {
                /* Found! */
                ppdev = gpmdev->dev[i].ppdev;
                break;
            }
        }
    }
    else if (gpmdev)
    {
        /* Otherwise use the primary PDEV */
        ppdev = gpmdev->ppdevGlobal;
    }

    /* Did we find one? */
    if (ppdev)
    {
        /* Yes, reference the PDEV */
        PDEVOBJ_vReference(ppdev);
    }

    /* Release PDEV lock */
    EngReleaseSemaphore(ghsemPDEV);

    return ppdev;
}

LONG
PDEVOBJ_lChangeDisplaySettings(
    _In_opt_ PUNICODE_STRING pustrDeviceName,
    _In_opt_ PDEVMODEW RequestedMode,
    _In_opt_ PMDEVOBJ pmdevOld,
    _Out_ PMDEVOBJ *ppmdevNew,
    _In_ BOOL bSearchClosestMode)
{
    PGRAPHICS_DEVICE pGraphicsDevice = NULL;
    PMDEVOBJ pmdev = NULL;
    PDEVMODEW pdm = NULL;
    ULONG lRet = DISP_CHANGE_SUCCESSFUL;
    ULONG i, j;

    TRACE("PDEVOBJ_lChangeDisplaySettings('%wZ' '%dx%dx%d (%d Hz)' %p %p)\n",
        pustrDeviceName,
        RequestedMode ? RequestedMode->dmPelsWidth : 0,
        RequestedMode ? RequestedMode->dmPelsHeight : 0,
        RequestedMode ? RequestedMode->dmBitsPerPel : 0,
        RequestedMode ? RequestedMode->dmDisplayFrequency : 0,
        pmdevOld, ppmdevNew);

    if (pustrDeviceName)
    {
        pGraphicsDevice = EngpFindGraphicsDevice(pustrDeviceName, 0);
        if (!pGraphicsDevice)
        {
            ERR("Wrong device name provided: '%wZ'\n", pustrDeviceName);
            lRet = DISP_CHANGE_BADPARAM;
            goto cleanup;
        }
    }
    else if (RequestedMode)
    {
        pGraphicsDevice = gpPrimaryGraphicsDevice;
        if (!pGraphicsDevice)
        {
            ERR("Wrong device'\n");
            lRet = DISP_CHANGE_BADPARAM;
            goto cleanup;
        }
    }

    if (pGraphicsDevice)
    {
        if (!LDEVOBJ_bProbeAndCaptureDevmode(pGraphicsDevice, RequestedMode, &pdm, bSearchClosestMode))
        {
            ERR("DrvProbeAndCaptureDevmode() failed\n");
            lRet = DISP_CHANGE_BADMODE;
            goto cleanup;
        }
    }

    /* Here, we know that input parameters were correct */

    {
        /* Create new MDEV. Note that if we provide a device name,
         * MDEV will only contain one device.
         * */

        if (pmdevOld)
        {
            /* Disable old MDEV */
            if (MDEVOBJ_bDisable(pmdevOld))
            {
                /* Create new MDEV. On failure, reenable old MDEV */
                pmdev = MDEVOBJ_Create(pustrDeviceName, pdm);
                if (!pmdev)
                    MDEVOBJ_vEnable(pmdevOld);
            }
        }
        else
        {
            pmdev = MDEVOBJ_Create(pustrDeviceName, pdm);
        }

        if (!pmdev)
        {
            ERR("Failed to create new MDEV\n");
            lRet = DISP_CHANGE_FAILED;
            goto cleanup;
        }

        lRet = DISP_CHANGE_SUCCESSFUL;
        *ppmdevNew = pmdev;

        /* We now have to do the mode switch */

        if (pustrDeviceName && pmdevOld)
        {
            /* We changed settings of one device. Add other devices which were already present */
            for (i = 0; i < pmdevOld->cDev; i++)
            {
                for (j = 0; j < pmdev->cDev; j++)
                {
                    if (pmdev->dev[j].ppdev->pGraphicsDevice == pmdevOld->dev[i].ppdev->pGraphicsDevice)
                    {
                        if (PDEVOBJ_bDynamicModeChange(pmdevOld->dev[i].ppdev, pmdev->dev[j].ppdev))
                        {
                            PPDEVOBJ tmp = pmdevOld->dev[i].ppdev;
                            pmdevOld->dev[i].ppdev = pmdev->dev[j].ppdev;
                            pmdev->dev[j].ppdev = tmp;
                        }
                        else
                        {
                            ERR("Failed to apply new settings\n");
                            UNIMPLEMENTED;
                            ASSERT(FALSE);
                        }
                        break;
                    }
                }
                if (j == pmdev->cDev)
                {
                    PDEVOBJ_vReference(pmdevOld->dev[i].ppdev);
                    pmdev->dev[pmdev->cDev].ppdev = pmdevOld->dev[i].ppdev;
                    pmdev->cDev++;
                }
            }
        }

        if (pmdev->cDev == 1)
        {
            pmdev->ppdevGlobal = pmdev->dev[0].ppdev;
        }
        else
        {
            /* Enable MultiDriver */
            pmdev->ppdevGlobal = PDEVOBJ_Create(NULL, (PDEVMODEW)pmdev, 0, LDEV_DEVICE_META);
            if (!pmdev->ppdevGlobal)
            {
                WARN("Failed to create meta-device. Using only first display\n");
                PDEVOBJ_vReference(pmdev->dev[0].ppdev);
                pmdev->ppdevGlobal = pmdev->dev[0].ppdev;
            }
        }

        if (pmdevOld)
        {
            /* Search PDEVs which were in pmdevOld, but are not anymore in pmdev, and disable them */
            for (i = 0; i < pmdevOld->cDev; i++)
            {
                for (j = 0; j < pmdev->cDev; j++)
                {
                    if (pmdev->dev[j].ppdev->pGraphicsDevice == pmdevOld->dev[i].ppdev->pGraphicsDevice)
                        break;
                }
                if (j == pmdev->cDev)
                    PDEVOBJ_bDisableDisplay(pmdevOld->dev[i].ppdev);
            }
        }
    }

cleanup:
    if (lRet != DISP_CHANGE_SUCCESSFUL)
    {
        *ppmdevNew = NULL;
        if (pmdev)
            MDEVOBJ_vDestroy(pmdev);
        if (pdm && pdm != RequestedMode)
            ExFreePoolWithTag(pdm, GDITAG_DEVMODE);
    }

    return lRet;
}

INT
NTAPI
PDEVOBJ_iGetColorManagementCaps(PPDEVOBJ ppdev)
{
    INT ret = CM_NONE;

    if (ppdev->flFlags & PDEV_DISPLAY)
    {
        if (ppdev->devinfo.iDitherFormat == BMF_8BPP ||
            ppdev->devinfo.flGraphicsCaps2 & GCAPS2_CHANGEGAMMARAMP)
            ret = CM_GAMMA_RAMP;
    }

    if (ppdev->devinfo.flGraphicsCaps & GCAPS_CMYKCOLOR)
        ret |= CM_CMYK_COLOR;
    if (ppdev->devinfo.flGraphicsCaps & GCAPS_ICM)
        ret |= CM_DEVICE_ICM;

    return ret;
}

VOID
NTAPI
PDEVOBJ_vGetDeviceCaps(
    IN PPDEVOBJ ppdev,
    OUT PDEVCAPS pDevCaps)
{
    PGDIINFO pGdiInfo = &ppdev->gdiinfo;

    pDevCaps->ulVersion = pGdiInfo->ulVersion;
    pDevCaps->ulTechnology = pGdiInfo->ulTechnology;
    pDevCaps->ulHorzSizeM = (pGdiInfo->ulHorzSize + 500) / 1000;
    pDevCaps->ulVertSizeM = (pGdiInfo->ulVertSize + 500) / 1000;
    pDevCaps->ulHorzSize = pGdiInfo->ulHorzSize;
    pDevCaps->ulVertSize = pGdiInfo->ulVertSize;
    pDevCaps->ulHorzRes = pGdiInfo->ulHorzRes;
    pDevCaps->ulVertRes = pGdiInfo->ulVertRes;
    pDevCaps->ulBitsPixel = pGdiInfo->cBitsPixel;
    if (pDevCaps->ulBitsPixel == 15) pDevCaps->ulBitsPixel = 16;
    pDevCaps->ulPlanes = pGdiInfo->cPlanes;
    pDevCaps->ulNumPens = pGdiInfo->ulNumColors;
    if (pDevCaps->ulNumPens != -1) pDevCaps->ulNumPens *= 5;
    pDevCaps->ulNumFonts = 0; // PDEVOBJ_cFonts(ppdev);
    pDevCaps->ulNumColors = pGdiInfo->ulNumColors;
    pDevCaps->ulRasterCaps = pGdiInfo->flRaster;
    pDevCaps->ulAspectX = pGdiInfo->ulAspectX;
    pDevCaps->ulAspectY = pGdiInfo->ulAspectY;
    pDevCaps->ulAspectXY = pGdiInfo->ulAspectXY;
    pDevCaps->ulLogPixelsX = pGdiInfo->ulLogPixelsX;
    pDevCaps->ulLogPixelsY = pGdiInfo->ulLogPixelsY;
    pDevCaps->ulSizePalette = pGdiInfo->ulNumPalReg;
    pDevCaps->ulColorRes = pGdiInfo->ulDACRed +
                           pGdiInfo->ulDACGreen +
                           pGdiInfo->ulDACBlue;
    pDevCaps->ulPhysicalWidth = pGdiInfo->szlPhysSize.cx;
    pDevCaps->ulPhysicalHeight = pGdiInfo->szlPhysSize.cy;
    pDevCaps->ulPhysicalOffsetX = pGdiInfo->ptlPhysOffset.x;
    pDevCaps->ulPhysicalOffsetY = pGdiInfo->ptlPhysOffset.y;
    pDevCaps->ulTextCaps = pGdiInfo->flTextCaps;
    pDevCaps->ulTextCaps |= (TC_SO_ABLE|TC_UA_ABLE|TC_CP_STROKE|TC_OP_STROKE|TC_OP_CHARACTER);
    if (pGdiInfo->ulTechnology != DT_PLOTTER)
        pDevCaps->ulTextCaps |= TC_VA_ABLE;
    pDevCaps->ulVRefresh = pGdiInfo->ulVRefresh;
    pDevCaps->ulDesktopHorzRes = pGdiInfo->ulHorzRes;
    pDevCaps->ulDesktopVertRes = pGdiInfo->ulVertRes;
    pDevCaps->ulBltAlignment = pGdiInfo->ulBltAlignment;
    pDevCaps->ulPanningHorzRes = pGdiInfo->ulPanningHorzRes;
    pDevCaps->ulPanningVertRes = pGdiInfo->ulPanningVertRes;
    pDevCaps->xPanningAlignment = pGdiInfo->xPanningAlignment;
    pDevCaps->yPanningAlignment = pGdiInfo->yPanningAlignment;
    pDevCaps->ulShadeBlend = pGdiInfo->flShadeBlend;
    pDevCaps->ulColorMgmtCaps = PDEVOBJ_iGetColorManagementCaps(ppdev);
}


/** Exported functions ********************************************************/

/*
 * @implemented
 */
BOOL
APIENTRY
EngQueryDeviceAttribute(
    _In_ HDEV hdev,
    _In_ ENG_DEVICE_ATTRIBUTE devAttr,
    _In_reads_bytes_(cjInSize) PVOID pvIn,
    _In_ ULONG cjInSize,
    _Out_writes_bytes_(cjOutSize) PVOID pvOut,
    _In_ ULONG cjOutSize)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)hdev;

    if (devAttr != QDA_ACCELERATION_LEVEL)
        return FALSE;

    if (cjOutSize >= sizeof(DWORD))
    {
        /* Set all Accelerations Level Key to enabled Full 0 to 5 turned off. */
        *(DWORD*)pvOut = ppdev->dwAccelerationLevel;
        return TRUE;
    }

    return FALSE;
}

_Must_inspect_result_ _Ret_z_
LPWSTR
APIENTRY
EngGetDriverName(_In_ HDEV hdev)
{
    PPDEVOBJ ppdev = (PPDEVOBJ)hdev;

    ASSERT(ppdev);
    ASSERT(ppdev->pldev);
    ASSERT(ppdev->pldev->pGdiDriverInfo);
    ASSERT(ppdev->pldev->pGdiDriverInfo->DriverName.Buffer);

    return ppdev->pldev->pGdiDriverInfo->DriverName.Buffer;
}


INT
APIENTRY
NtGdiGetDeviceCaps(
    HDC hdc,
    INT Index)
{
    PDC pdc;
    DEVCAPS devcaps;

    /* Lock the given DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Get the data */
    PDEVOBJ_vGetDeviceCaps(pdc->ppdev, &devcaps);

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Return capability */
    switch (Index)
    {
        case DRIVERVERSION:
            return devcaps.ulVersion;

        case TECHNOLOGY:
            return devcaps.ulTechnology;

        case HORZSIZE:
            return devcaps.ulHorzSize;

        case VERTSIZE:
            return devcaps.ulVertSize;

        case HORZRES:
            return devcaps.ulHorzRes;

        case VERTRES:
            return devcaps.ulVertRes;

        case LOGPIXELSX:
            return devcaps.ulLogPixelsX;

        case LOGPIXELSY:
            return devcaps.ulLogPixelsY;

        case BITSPIXEL:
            return devcaps.ulBitsPixel;

        case PLANES:
            return devcaps.ulPlanes;

        case NUMBRUSHES:
            return -1;

        case NUMPENS:
            return devcaps.ulNumPens;

        case NUMFONTS:
            return devcaps.ulNumFonts;

        case NUMCOLORS:
            return devcaps.ulNumColors;

        case ASPECTX:
            return devcaps.ulAspectX;

        case ASPECTY:
            return devcaps.ulAspectY;

        case ASPECTXY:
            return devcaps.ulAspectXY;

        case CLIPCAPS:
            return CP_RECTANGLE;

        case SIZEPALETTE:
            return devcaps.ulSizePalette;

        case NUMRESERVED:
            return 20;

        case COLORRES:
            return devcaps.ulColorRes;

        case DESKTOPVERTRES:
            return devcaps.ulVertRes;

        case DESKTOPHORZRES:
            return devcaps.ulHorzRes;

        case BLTALIGNMENT:
            return devcaps.ulBltAlignment;

        case SHADEBLENDCAPS:
            return devcaps.ulShadeBlend;

        case COLORMGMTCAPS:
            return devcaps.ulColorMgmtCaps;

        case PHYSICALWIDTH:
            return devcaps.ulPhysicalWidth;

        case PHYSICALHEIGHT:
            return devcaps.ulPhysicalHeight;

        case PHYSICALOFFSETX:
            return devcaps.ulPhysicalOffsetX;

        case PHYSICALOFFSETY:
            return devcaps.ulPhysicalOffsetY;

        case VREFRESH:
            return devcaps.ulVRefresh;

        case RASTERCAPS:
            return devcaps.ulRasterCaps;

        case CURVECAPS:
            return (CC_CIRCLES | CC_PIE | CC_CHORD | CC_ELLIPSES | CC_WIDE |
                    CC_STYLED | CC_WIDESTYLED | CC_INTERIORS | CC_ROUNDRECT);

        case LINECAPS:
            return (LC_POLYLINE | LC_MARKER | LC_POLYMARKER | LC_WIDE |
                    LC_STYLED | LC_WIDESTYLED | LC_INTERIORS);

        case POLYGONALCAPS:
            return (PC_POLYGON | PC_RECTANGLE | PC_WINDPOLYGON | PC_SCANLINE |
                    PC_WIDE | PC_STYLED | PC_WIDESTYLED | PC_INTERIORS);

        case TEXTCAPS:
            return devcaps.ulTextCaps;

        case CAPS1:
        case PDEVICESIZE:
        case SCALINGFACTORX:
        case SCALINGFACTORY:
        default:
            return 0;
    }

    return 0;
}

_Success_(return!=FALSE)
BOOL
APIENTRY
NtGdiGetDeviceCapsAll(
    IN HDC hDC,
    OUT PDEVCAPS pDevCaps)
{
    PDC pdc;
    DEVCAPS devcaps;
    BOOL bResult = TRUE;

    /* Lock the given DC */
    pdc = DC_LockDc(hDC);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Get the data */
    PDEVOBJ_vGetDeviceCaps(pdc->ppdev, &devcaps);

    /* Unlock the DC */
    DC_UnlockDc(pdc);

    /* Copy data to caller */
    _SEH2_TRY
    {
        ProbeForWrite(pDevCaps, sizeof(DEVCAPS), 1);
        RtlCopyMemory(pDevCaps, &devcaps, sizeof(DEVCAPS));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastNtError(_SEH2_GetExceptionCode());
        bResult = FALSE;
    }
    _SEH2_END;

    return bResult;
}

DHPDEV
APIENTRY
NtGdiGetDhpdev(
    IN HDEV hdev)
{
    PPDEVOBJ ppdev;
    DHPDEV dhpdev = NULL;

    /* Check parameter */
    if (!hdev || (PCHAR)hdev < (PCHAR)MmSystemRangeStart)
        return NULL;

    /* Lock PDEV list */
    EngAcquireSemaphoreShared(ghsemPDEV);

    /* Walk through the list of PDEVs */
    for (ppdev = gppdevList;  ppdev; ppdev = ppdev->ppdevNext)
    {
        /* Compare with the given HDEV */
        if (ppdev == (PPDEVOBJ)hdev)
        {
            /* Found the PDEV! Get it's dhpdev and break */
            dhpdev = ppdev->dhpdev;
            break;
        }
    }

    /* Unlock PDEV list */
    EngReleaseSemaphore(ghsemPDEV);

    return dhpdev;
}

PSIZEL
FASTCALL
PDEVOBJ_sizl(PPDEVOBJ ppdev, PSIZEL psizl)
{
    if (ppdev->flFlags & PDEV_META_DEVICE)
    {
        *psizl = ppdev->szlMetaRes;
    }
    else
    {
        psizl->cx = ppdev->gdiinfo.ulHorzRes;
        psizl->cy = ppdev->gdiinfo.ulVertRes;
    }
    return psizl;
}
