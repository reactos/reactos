/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Functions for saving and restoring dc states
 * FILE:              subsystem/win32/win32k/objects/dcstate.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <w32k.h>

#define NDEBUG
#include <debug.h>

VOID
FASTCALL
DC_vCopyState(PDC pdcSrc, PDC pdcDst)
{
    /* Copy full DC attribute */
    *pdcDst->pdcattr = *pdcSrc->pdcattr;

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */
    /* The VisRectRegion field needs to be set to a valid state */

    /* Mark some fields as dirty */
    pdcDst->pdcattr->ulDirty_ |= 0x0012001f;

    /* Copy DC level */
    pdcDst->dclevel.pColorSpace     = pdcSrc->dclevel.pColorSpace;
    pdcDst->dclevel.lSaveDepth      = pdcSrc->dclevel.lSaveDepth;
    pdcDst->dclevel.hdcSave         = pdcSrc->dclevel.hdcSave;
    pdcDst->dclevel.laPath          = pdcSrc->dclevel.laPath;
    pdcDst->dclevel.ca              = pdcSrc->dclevel.ca;
    pdcDst->dclevel.mxWorldToDevice = pdcSrc->dclevel.mxWorldToDevice;
    pdcDst->dclevel.mxDeviceToWorld = pdcSrc->dclevel.mxDeviceToWorld;
    pdcDst->dclevel.mxWorldToPage   = pdcSrc->dclevel.mxWorldToPage;
    pdcDst->dclevel.efM11PtoD       = pdcSrc->dclevel.efM11PtoD;
    pdcDst->dclevel.efM22PtoD       = pdcSrc->dclevel.efM22PtoD;
    pdcDst->dclevel.sizl            = pdcSrc->dclevel.sizl;

    /* Handle references here correctly */
    DC_vSelectSurface(pdcDst, pdcSrc->dclevel.pSurface);
    DC_vSelectFillBrush(pdcDst, pdcSrc->dclevel.pbrFill);
    DC_vSelectLineBrush(pdcDst, pdcSrc->dclevel.pbrLine);

    // FIXME: handle refs
    pdcDst->dclevel.hpal            = pdcSrc->dclevel.hpal;
    pdcDst->dclevel.ppal            = pdcSrc->dclevel.ppal;
    pdcDst->dclevel.plfnt           = pdcSrc->dclevel.plfnt;

    /* ROS hacks */
    pdcDst->rosdc.hBitmap            = pdcSrc->rosdc.hBitmap;

    if (pdcDst->dctype != DC_TYPE_MEMORY)
    {
        pdcDst->rosdc.bitsPerPixel = pdcSrc->rosdc.bitsPerPixel;
    }

    GdiExtSelectClipRgn(pdcDst, pdcSrc->rosdc.hClipRgn, RGN_COPY);

}


BOOL FASTCALL
IntGdiCleanDC(HDC hDC)
{
    PDC dc;
    if (!hDC) return FALSE;
    dc = DC_LockDc(hDC);
    if (!dc) return FALSE;
    // Clean the DC
    if (defaultDCstate) DC_vCopyState(defaultDCstate, dc);

    if (dc->dctype != DC_TYPE_MEMORY)
    {
        dc->rosdc.bitsPerPixel = defaultDCstate->rosdc.bitsPerPixel;
    }

    DC_UnlockDc(dc);

    return TRUE;
}


BOOL
APIENTRY
NtGdiResetDC(
    IN HDC hdc,
    IN LPDEVMODEW pdm,
    OUT PBOOL pbBanding,
    IN OPTIONAL VOID *pDriverInfo2,
    OUT VOID *ppUMdhpdev)
{
    UNIMPLEMENTED;
    return 0;
}


BOOL
APIENTRY
NtGdiRestoreDC(
    HDC hdc,
    INT iSaveLevel)
{
    PDC pdc, pdcSave;
    HDC hdcSave;
    PEPROCESS pepCurrentProcess;

    DPRINT("NtGdiRestoreDC(%lx, %d)\n", hdc, iSaveLevel);

    /* Lock the original DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    ASSERT(pdc->dclevel.lSaveDepth > 0);

    /* Negative values are relative to the stack top */
    if (iSaveLevel < 0)
        iSaveLevel = pdc->dclevel.lSaveDepth + iSaveLevel;

    /* Check if we have a valid instance */
    if (iSaveLevel <= 0 || iSaveLevel >= pdc->dclevel.lSaveDepth)
    {
        DPRINT("Illegal save level, requested: %ld, current: %ld\n", 
               iSaveLevel, pdc->dclevel.lSaveDepth);
        DC_UnlockDc(pdc);
        SetLastWin32Error(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Get current process */
    pepCurrentProcess = PsGetCurrentProcess();

    /* Loop the save levels */
    while (pdc->dclevel.lSaveDepth > iSaveLevel)
    {
        hdcSave = pdc->dclevel.hdcSave;

        /* Set us as the owner */
        if (!GDIOBJ_SetOwnership(hdcSave, pepCurrentProcess))
        {
            /* Could not get ownership. That's bad! */
            DPRINT1("Could not get ownership of saved DC (%p) for dc %p!\n",
                    hdcSave, hdc);
            return FALSE;
        }

        /* Lock the saved dc */
        pdcSave = DC_LockDc(hdcSave);
        if (!pdcSave)
        {
            /* WTF? Internal error! */
            DPRINT1("Could not lock the saved DC (%p) for dc %p!\n",
                    hdcSave, hdc);
            DC_UnlockDc(pdc);
            return FALSE;
        }

        /* Remove the saved dc from the queue */
        pdc->dclevel.hdcSave = pdcSave->dclevel.hdcSave;

        /* Decrement save level */
        pdc->dclevel.lSaveDepth--;

        /* Is this the state we want? */
        if (pdc->dclevel.lSaveDepth == iSaveLevel)
        {
            /* Copy the state back */
            DC_vCopyState(pdcSave, pdc);

            // Restore Path by removing it, if the Save flag is set.
            // BeginPath will takecare of the rest.
            if (pdc->dclevel.hPath && pdc->dclevel.flPath & DCPATH_SAVE)
            {
                PATH_Delete(pdc->dclevel.hPath);
                pdc->dclevel.hPath = 0;
                pdc->dclevel.flPath &= ~DCPATH_SAVE;
            }
        }

        /* Delete the saved dc */
        DC_FreeDC(hdcSave);
    }

    DC_UnlockDc(pdc);

    DPRINT("Leaving NtGdiRestoreDC\n");
    return TRUE;
}


INT
APIENTRY
NtGdiSaveDC(
    HDC hDC)
{
    HDC hdcSave;
    PDC pdc, pdcSave;
    INT lSaveDepth;

    DPRINT("NtGdiSaveDC(%lx)\n", hDC);

    /* Lock the original dc */
    pdc = DC_LockDc(hDC);
    if (pdc == NULL)
    {
        DPRINT("Could not lock DC\n");
        SetLastWin32Error(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Allocate a new dc */
    pdcSave = DC_AllocDC(NULL);
    if (pdcSave == NULL)
    {
        DPRINT("Could not allocate a new DC\n");
        DC_UnlockDc(pdc);
        return 0;
    }
    hdcSave = pdcSave->BaseObject.hHmgr;

    /* Make it a kernel handle 
       (FIXME: windows handles this different, see wiki)*/
    GDIOBJ_SetOwnership(hdcSave, NULL);

    /* Copy the current state */
    DC_vCopyState(pdc, pdcSave);

    /* Copy path. FIXME: why this way? */
    pdcSave->dclevel.hPath = pdc->dclevel.hPath;
    pdcSave->dclevel.flPath = pdc->dclevel.flPath | DCPATH_SAVESTATE;
    if (pdcSave->dclevel.hPath) pdcSave->dclevel.flPath |= DCPATH_SAVE;

    /* Set new dc as save dc */
    pdc->dclevel.hdcSave = hdcSave;

    /* Increase save depth, return old value */
    lSaveDepth = pdc->dclevel.lSaveDepth++;

    /* Cleanup and return */
    DC_UnlockDc(pdcSave);
    DC_UnlockDc(pdc);

    DPRINT("Leave NtGdiSaveDC: %ld\n", lSaveDepth);
    return lSaveDepth;
}

