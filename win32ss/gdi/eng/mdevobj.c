/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Support for meta devices
 * FILE:             win32ss/gdi/eng/mdevobj.c
 * PROGRAMER:        Hervé Poussineau
 */

#include <win32k.h>
#define NDEBUG
#include <debug.h>
DBG_DEFAULT_CHANNEL(EngMDev);

PMDEVOBJ gpmdev = NULL; /* FIXME: should be stored in gpDispInfo->pmdev */

VOID
MDEVOBJ_vEnable(
    _Inout_ PMDEVOBJ pmdev)
{
    ULONG i;

    for (i = 0; i < pmdev->cDev; i++)
    {
        PDEVOBJ_vEnableDisplay(pmdev->dev[i].ppdev);
    }
}

BOOL
MDEVOBJ_bDisable(
    _Inout_ PMDEVOBJ pmdev)
{
    BOOL bSuccess = TRUE;
    ULONG i, j;

    for (i = 0; i < pmdev->cDev; i++)
    {
        if (!PDEVOBJ_bDisableDisplay(pmdev->dev[i].ppdev))
        {
            bSuccess = FALSE;
            break;
        }
    }

    if (!bSuccess)
    {
        /* Failed to disable all PDEVs. Reenable those we have disabled */
        for (j = 0; j < i; j++)
        {
            PDEVOBJ_vEnableDisplay(pmdev->dev[i].ppdev);
        }
    }

    return bSuccess;
}

PMDEVOBJ
MDEVOBJ_Create(
    _In_opt_ PUNICODE_STRING pustrDeviceName,
    _In_opt_ PDEVMODEW pdm)
{
    PMDEVOBJ pmdev = NULL;
    PPDEVOBJ ppdev;
    PGRAPHICS_DEVICE pGraphicsDevice;
    DEVMODEW dmDefault;
    PDEVMODEW localPdm;
    ULONG iDevNum = 0;
    ULONG dwAccelerationLevel = 0;

    TRACE("MDEVOBJ_Create('%wZ' '%dx%dx%d (%d Hz)')\n",
        pustrDeviceName,
        pdm ? pdm->dmPelsWidth : 0,
        pdm ? pdm->dmPelsHeight : 0,
        pdm ? pdm->dmBitsPerPel : 0,
        pdm ? pdm->dmDisplayFrequency : 0);

    pmdev = ExAllocatePoolZero(PagedPool, sizeof(MDEVOBJ) + sizeof(MDEVDISPLAY), GDITAG_MDEV);
    if (!pmdev)
    {
        ERR("Failed to allocate memory for MDEV\n");
        return NULL;
    }

    pmdev->cDev = 0;

    while (TRUE)
    {
        /* Get the right graphics devices: either the specified one, or all of them (one after one) */
        if (pustrDeviceName)
            pGraphicsDevice = (iDevNum == 0) ? EngpFindGraphicsDevice(pustrDeviceName, 0) : NULL;
        else
            pGraphicsDevice = EngpFindGraphicsDevice(NULL, iDevNum);
        iDevNum++;

        if (!pGraphicsDevice)
        {
            TRACE("Done enumeration of graphic devices (DeviceName '%wZ' iDevNum %d)\n", pustrDeviceName, iDevNum);
            break;
        }

        if (!pdm)
        {
            /* No settings requested. Provide nothing and LDEVOBJ_bProbeAndCaptureDevmode
             * will read default settings from registry */
            RtlZeroMemory(&dmDefault, sizeof(dmDefault));
            dmDefault.dmSize = sizeof(dmDefault);
        }

        /* Get or create a PDEV for these settings */
        if (LDEVOBJ_bProbeAndCaptureDevmode(pGraphicsDevice, pdm ? pdm : &dmDefault, &localPdm, !pdm))
        {
            ppdev = PDEVOBJ_Create(pGraphicsDevice, localPdm, dwAccelerationLevel, LDEV_DEVICE_DISPLAY);
        }
        else
        {
            ppdev = NULL;
        }

        if (ppdev)
        {
            /* Great. We have a found a matching PDEV. Store it in MDEV */
            if (pmdev->cDev >= 1)
            {
                /* We have to reallocate MDEV to add space for the new display */
                PMDEVOBJ pmdevBigger = ExAllocatePoolZero(PagedPool, sizeof(MDEVOBJ) + (pmdev->cDev + 1) * sizeof(MDEVDISPLAY), GDITAG_MDEV);
                if (!pmdevBigger)
                {
                    WARN("Failed to allocate memory for MDEV. Skipping display '%S'\n", pGraphicsDevice->szWinDeviceName);
                    continue;
                }
                else
                {
                    /* Copy existing data */
                    RtlCopyMemory(pmdevBigger, pmdev, sizeof(MDEVOBJ) + pmdev->cDev * sizeof(MDEVDISPLAY));
                    ExFreePoolWithTag(pmdev, GDITAG_MDEV);
                    pmdev = pmdevBigger;
                }
            }

            TRACE("Adding '%S' to MDEV %p\n", pGraphicsDevice->szWinDeviceName, pmdev);
            PDEVOBJ_vReference(ppdev);
            pmdev->dev[pmdev->cDev].ppdev = ppdev;
            pmdev->cDev++;
        }
        else
        {
            WARN("Failed to add '%S' to MDEV %p\n", pGraphicsDevice->szWinDeviceName, pmdev);
        }
    }

    if (pmdev->cDev == 0)
    {
        TRACE("Failed to add any device to MDEV. Returning NULL\n");
        MDEVOBJ_vDestroy(pmdev);
        return NULL;
    }

    TRACE("Returning new MDEV %p with %d devices\n", pmdev, pmdev->cDev);
    return pmdev;
}

VOID
MDEVOBJ_vDestroy(
    _Inout_ PMDEVOBJ pmdev)
{
    ULONG i;

    for (i = 0; i < pmdev->cDev; i++)
    {
        PDEVOBJ_vRelease(pmdev->dev[i].ppdev);
    }

    if (pmdev->cDev > 1)
        PDEVOBJ_vRelease(pmdev->ppdevGlobal);

    ExFreePoolWithTag(pmdev, GDITAG_MDEV);
}
