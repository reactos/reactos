/*
 * PROJECT:         ReactOS Win32k Subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32k/objects/icm.c
 * PURPOSE:         Icm functions
 * PROGRAMMERS:     ...
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

HCOLORSPACE hStockColorSpace = NULL;


HCOLORSPACE
FASTCALL
IntGdiCreateColorSpace(
    PLOGCOLORSPACEEXW pLogColorSpace)
{
    PCOLORSPACE pCS;
    HCOLORSPACE hCS;

    pCS = COLORSPACEOBJ_AllocCSWithHandle();
    if (pCS == NULL) return NULL;

    hCS = pCS->BaseObject.hHmgr;

    pCS->lcsColorSpace = pLogColorSpace->lcsColorSpace;
    pCS->dwFlags = pLogColorSpace->dwFlags;

    COLORSPACEOBJ_UnlockCS(pCS);
    return hCS;
}

BOOL
FASTCALL
IntGdiDeleteColorSpace(
    HCOLORSPACE hColorSpace)
{
    BOOL Ret = FALSE;

    if ((hColorSpace != hStockColorSpace) &&
        (GDI_HANDLE_GET_TYPE(hColorSpace) == GDILoObjType_LO_ICMLCS_TYPE))
    {
        Ret = GreDeleteObject(hColorSpace);
        if (!Ret) EngSetLastError(ERROR_INVALID_PARAMETER);
    }

    return Ret;
}

HANDLE
APIENTRY
NtGdiCreateColorSpace(
    IN PLOGCOLORSPACEEXW pLogColorSpace)
{
    LOGCOLORSPACEEXW Safelcs;
    NTSTATUS Status = STATUS_SUCCESS;

    _SEH2_TRY
    {
        ProbeForRead( pLogColorSpace, sizeof(LOGCOLORSPACEEXW), 1);
        RtlCopyMemory(&Safelcs, pLogColorSpace, sizeof(LOGCOLORSPACEEXW));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return NULL;
    }

    return IntGdiCreateColorSpace(&Safelcs);
}

BOOL
APIENTRY
NtGdiDeleteColorSpace(
    IN HANDLE hColorSpace)
{
    return IntGdiDeleteColorSpace(hColorSpace);
}

BOOL
FASTCALL
IntGetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp)
{
    PPDEVOBJ pGDev = (PPDEVOBJ) hPDev;
    int i;

    if (!(pGDev->flFlags & PDEV_DISPLAY)) return FALSE;

    if ((pGDev->devinfo.iDitherFormat == BMF_8BPP)  ||
        (pGDev->devinfo.iDitherFormat == BMF_16BPP) ||
        (pGDev->devinfo.iDitherFormat == BMF_24BPP) ||
        (pGDev->devinfo.iDitherFormat == BMF_32BPP))
    {
        if (pGDev->flFlags & PDEV_GAMMARAMP_TABLE)
        {
            RtlCopyMemory(Ramp, pGDev->pvGammaRamp, sizeof(GAMMARAMP));
        }
        else
        {
            // Generate the 256-colors array
            for (i = 0; i < 256; i++ )
            {
                int NewValue = i * 256;

                Ramp->Red[i] = Ramp->Green[i] = Ramp->Blue[i] = ((WORD)NewValue);
            }
        }
        return TRUE;
    }

    return FALSE;
}

BOOL
APIENTRY
NtGdiGetDeviceGammaRamp(
    HDC hDC,
    LPVOID Ramp)
{
    BOOL Ret;
    PDC dc;
    NTSTATUS Status = STATUS_SUCCESS;
    PGAMMARAMP SafeRamp;

    if (!Ramp) return FALSE;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    SafeRamp = ExAllocatePoolWithTag(PagedPool, sizeof(GAMMARAMP), GDITAG_ICM);
    if (!SafeRamp)
    {
        DC_UnlockDc(dc);
        EngSetLastError(STATUS_NO_MEMORY);
        return FALSE;
    }

    Ret = IntGetDeviceGammaRamp((HDEV)dc->ppdev, SafeRamp);

    if (!Ret) return Ret;

    _SEH2_TRY
    {
        ProbeForWrite(Ramp, sizeof(GAMMARAMP), 1);
        RtlCopyMemory(Ramp, SafeRamp, sizeof(GAMMARAMP));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    DC_UnlockDc(dc);
    ExFreePoolWithTag(SafeRamp, GDITAG_ICM);

    if (!NT_SUCCESS(Status))
    {
        SetLastNtError(Status);
        return FALSE;
    }
    return Ret;
}

BOOL
APIENTRY
NtGdiSetColorSpace(IN HDC hdc,
                   IN HCOLORSPACE hColorSpace)
{
    PDC pDC;
    PDC_ATTR pdcattr;
    PCOLORSPACE pCS;

    pDC = DC_LockDc(hdc);
    if (!pDC)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = pDC->pdcattr;

    if (pdcattr->hColorSpace == hColorSpace)
    {
        DC_UnlockDc(pDC);
        return TRUE;
    }

    pCS = COLORSPACEOBJ_LockCS(hColorSpace);
    if (!pCS)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if (pDC->dclevel.pColorSpace)
    {
        GDIOBJ_vDereferenceObject((POBJ) pDC->dclevel.pColorSpace);
    }

    pDC->dclevel.pColorSpace = pCS;
    pdcattr->hColorSpace = hColorSpace;

    COLORSPACEOBJ_UnlockCS(pCS);
    DC_UnlockDc(pDC);
    return TRUE;
}

BOOL
FASTCALL
UpdateDeviceGammaRamp(HDEV hPDev)
{
    BOOL Ret = FALSE;
    PPALETTE palGDI;
    PALOBJ *palPtr;
    PPDEVOBJ pGDev = (PPDEVOBJ)hPDev;

    if ((pGDev->devinfo.iDitherFormat == BMF_8BPP)  ||
        (pGDev->devinfo.iDitherFormat == BMF_16BPP) ||
        (pGDev->devinfo.iDitherFormat == BMF_24BPP) ||
        (pGDev->devinfo.iDitherFormat == BMF_32BPP))
    {
        if (pGDev->DriverFunctions.IcmSetDeviceGammaRamp)
            return pGDev->DriverFunctions.IcmSetDeviceGammaRamp( pGDev->dhpdev,
                    IGRF_RGB_256WORDS,
                    pGDev->pvGammaRamp);

        if ((pGDev->devinfo.iDitherFormat != BMF_8BPP) ||
            !(pGDev->gdiinfo.flRaster & RC_PALETTE)) return FALSE;

        if (!(pGDev->flFlags & PDEV_GAMMARAMP_TABLE)) return FALSE;

        palGDI = PALETTE_ShareLockPalette(pGDev->devinfo.hpalDefault);
        if(!palGDI) return FALSE;
        palPtr = (PALOBJ*) palGDI;

        if (pGDev->flFlags & PDEV_GAMMARAMP_TABLE)
            palGDI->flFlags |= PAL_GAMMACORRECTION;
        else
            palGDI->flFlags &= ~PAL_GAMMACORRECTION;

        if (!(pGDev->flFlags & PDEV_DRIVER_PUNTED_CALL)) // No punting, we hook
        {
            // BMF_8BPP only!
            // PALOBJ_cGetColors check mode flags and update Gamma Correction.
            // Set the HDEV to pal and go.
            palGDI->hPDev = hPDev;
            Ret = pGDev->DriverFunctions.SetPalette(pGDev->dhpdev,
                                                    palPtr,
                                                    0,
                                                    0,
                                                    palGDI->NumColors);
        }
        PALETTE_ShareUnlockPalette(palGDI);
        return Ret;
    }
    else
        return FALSE;
}

//
// ICM registry subkey sets internal brightness range, gamma range is 128 or
// 256 when icm is init.
INT IcmGammaRangeSet = 128; // <- Make it global

BOOL
FASTCALL
IntSetDeviceGammaRamp(HDEV hPDev, PGAMMARAMP Ramp, BOOL Test)
{
    WORD IcmGR, i, R, G, B;
    BOOL Ret = FALSE, TstPeak;
    PPDEVOBJ pGDev = (PPDEVOBJ) hPDev;

    if (!hPDev) return FALSE;

    if (!(pGDev->flFlags & PDEV_DISPLAY )) return FALSE;

    if ((pGDev->devinfo.iDitherFormat == BMF_8BPP)  ||
        (pGDev->devinfo.iDitherFormat == BMF_16BPP) ||
        (pGDev->devinfo.iDitherFormat == BMF_24BPP) ||
        (pGDev->devinfo.iDitherFormat == BMF_32BPP))
    {
        if (!pGDev->DriverFunctions.IcmSetDeviceGammaRamp)
        {
            // No driver support
            if (!(pGDev->devinfo.flGraphicsCaps2 & GCAPS2_CHANGEGAMMARAMP))
            {
                // Driver does not support Gamma Ramp, so test to see we
                // have BMF_8BPP only and palette operation support.
                if ((pGDev->devinfo.iDitherFormat != BMF_8BPP) ||
                    !(pGDev->gdiinfo.flRaster & RC_PALETTE))  return FALSE;
            }
        }

        if (pGDev->flFlags & PDEV_GAMMARAMP_TABLE)
        {
            if (RtlCompareMemory(pGDev->pvGammaRamp, Ramp, sizeof(GAMMARAMP)) ==
                    sizeof(GAMMARAMP)) return TRUE;
        }

        // Verify Ramp is inside range.
        IcmGR = -IcmGammaRangeSet;
        TstPeak = (Test == FALSE);
        for (i = 0; i < 256; i++)
        {
            R = Ramp->Red[i]   / 256;
            G = Ramp->Green[i] / 256;
            B = Ramp->Blue[i]  / 256;

            if (R >= IcmGR)
            {
                if (R <= IcmGammaRangeSet + i)
                {
                    if ((G >= IcmGR) &&
                        (G <= IcmGammaRangeSet + i) &&
                        (B >= IcmGR) &&
                        (B <= IcmGammaRangeSet + i) ) continue;
                }
            }

            if (Test) return Ret; // Don't set and return.

            // No test override, check max range
            if (TstPeak)
            {
                if ((R != (IcmGR * 256)) ||
                    (G != (IcmGR * 256)) ||
                    (B != (IcmGR * 256)) ) TstPeak = FALSE; // W/i range.
            }
        }
        // ReactOS allocates a ramp even if it is 8BPP and Palette only.
        // This way we have a record of the change in memory.
        if (!pGDev->pvGammaRamp && !(pGDev->flFlags & PDEV_GAMMARAMP_TABLE))
        {
            // If the above is true and we have nothing allocated, create it.
            pGDev->pvGammaRamp = ExAllocatePoolWithTag(PagedPool, sizeof(GAMMARAMP), GDITAG_ICM);
            pGDev->flFlags |= PDEV_GAMMARAMP_TABLE;
        }

        if (pGDev->pvGammaRamp)
            RtlCopyMemory(pGDev->pvGammaRamp, Ramp, sizeof(GAMMARAMP));

        Ret = UpdateDeviceGammaRamp(hPDev);
    }

    return Ret;
}

BOOL
APIENTRY
NtGdiSetDeviceGammaRamp(
    HDC hDC,
    LPVOID Ramp)
{
    BOOL Ret;
    PDC dc;
    NTSTATUS Status = STATUS_SUCCESS;
    PGAMMARAMP SafeRamp;
    if (!Ramp) return FALSE;

    dc = DC_LockDc(hDC);
    if (!dc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    SafeRamp = ExAllocatePoolWithTag(PagedPool, sizeof(GAMMARAMP), GDITAG_ICM);
    if (!SafeRamp)
    {
        DC_UnlockDc(dc);
        EngSetLastError(STATUS_NO_MEMORY);
        return FALSE;
    }
    _SEH2_TRY
    {
        ProbeForRead(Ramp, sizeof(GAMMARAMP), 1);
        RtlCopyMemory(SafeRamp, Ramp, sizeof(GAMMARAMP));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        DC_UnlockDc(dc);
        ExFreePoolWithTag(SafeRamp, GDITAG_ICM);
        SetLastNtError(Status);
        return FALSE;
    }

    Ret = IntSetDeviceGammaRamp((HDEV)dc->ppdev, SafeRamp, TRUE);
    DC_UnlockDc(dc);
    ExFreePoolWithTag(SafeRamp, GDITAG_ICM);
    return Ret;
}

INT
APIENTRY
NtGdiSetIcmMode(HDC  hDC,
                ULONG nCommand,
                ULONG EnableICM) // ulMode
{
    /* FIXME: This should be coded someday  */
    if (EnableICM == ICM_OFF)
    {
        return  ICM_OFF;
    }
    if (EnableICM == ICM_ON)
    {
        return  0;
    }
    if (EnableICM == ICM_QUERY)
    {
        return  ICM_OFF;
    }

    return  0;
}

/* EOF */
