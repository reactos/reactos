/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Win32k subsystem
 * PURPOSE:           Functions for saving and restoring dc states
 * FILE:              subsystems/win32/win32k/objects/dcstate.c
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@rectos.org)
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

VOID
FASTCALL
DC_vCopyState(PDC pdcSrc, PDC pdcDst, BOOL To)
{
    DPRINT("DC_vCopyState(%p, %p)\n", pdcSrc->BaseObject.hHmgr, pdcDst->BaseObject.hHmgr);

    /* Copy full DC attribute */
    *pdcDst->pdcattr = *pdcSrc->pdcattr;

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */
    /* The VisRectRegion field needs to be set to a valid state */

    /* Mark some fields as dirty */
    pdcDst->pdcattr->ulDirty_ |= (DIRTY_FILL|DIRTY_LINE|DIRTY_TEXT|DIRTY_BACKGROUND|DIRTY_CHARSET|DC_ICM_NOT_CALIBRATED|DC_ICM_NOT_SET); // Note: Use if, To is FALSE....

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
    pdcDst->dclevel.hpal            = pdcSrc->dclevel.hpal;

    /* Handle references here correctly */
    DC_vSelectFillBrush(pdcDst, pdcSrc->dclevel.pbrFill);
    DC_vSelectLineBrush(pdcDst, pdcSrc->dclevel.pbrLine);
    DC_vSelectPalette(pdcDst, pdcSrc->dclevel.ppal);

    /* Dereference the old font, reference the new one */
    if (pdcDst->dclevel.plfnt) LFONT_ShareUnlockFont(pdcDst->dclevel.plfnt); /// @todo should aways be != NULL
    GDIOBJ_vReferenceObjectByPointer(&pdcSrc->dclevel.plfnt->BaseObject);
    pdcDst->dclevel.plfnt           = pdcSrc->dclevel.plfnt;

    /* Get/SetDCState() don't change hVisRgn field ("Undoc. Windows" p.559). */
    if (!To)
    {
        IntGdiExtSelectClipRgn(pdcDst, pdcSrc->dclevel.prgnClip, RGN_COPY);
        if (pdcDst->dclevel.prgnMeta)
        {
            REGION_Delete(pdcDst->dclevel.prgnMeta);
            pdcDst->dclevel.prgnMeta = NULL;
        }
        if (pdcSrc->dclevel.prgnMeta)
        {
            pdcDst->dclevel.prgnMeta = IntSysCreateRectpRgn(0, 0, 0, 0);
            IntGdiCombineRgn(pdcDst->dclevel.prgnMeta, pdcSrc->dclevel.prgnMeta, NULL, RGN_COPY);
        }
        pdcDst->fs |= DC_FLAG_DIRTY_RAO;
    }
}


BOOL
FASTCALL
IntGdiCleanDC(HDC hDC)
{
    PDC dc;
    if (!hDC) return FALSE;
    dc = DC_LockDc(hDC);
    if (!dc) return FALSE;
    // Clean the DC
    if (defaultDCstate)
    {
        DC_vCopyState(defaultDCstate, dc, FALSE);
        /* Update the brushes now, because they reference some objects (the DC palette)
         * Which belong to the current process, and this DC might be used for another process
         * after being cleaned up (for GetDC(0) for instance) */
        DC_vUpdateFillBrush(dc);
        DC_vUpdateBackgroundBrush(dc);
        DC_vUpdateLineBrush(dc);
        DC_vUpdateTextBrush(dc);
    }

    /* DC_vCopyState frees the Clip rgn and the Meta rgn. Take care of the other ones
     * There is no need to clear prgnVis, as UserGetDC updates it immediately. */
    if (dc->prgnRao)
        REGION_Delete(dc->prgnRao);
    if (dc->prgnAPI)
        REGION_Delete(dc->prgnAPI);
    dc->prgnRao = dc->prgnAPI = NULL;

    dc->fs |= DC_FLAG_DIRTY_RAO;

    DC_UnlockDc(dc);

    return TRUE;
}

BOOL
APIENTRY
NtGdiResetDC(
    IN HDC hdc,
    IN LPDEVMODEW pdm,
    OUT PBOOL pbBanding,
    IN OPTIONAL DRIVER_INFO_2W *pDriverInfo2,
    OUT VOID *ppUMdhpdev)
{
    UNIMPLEMENTED;
    return 0;
}


VOID
NTAPI
DC_vRestoreDC(
    IN PDC pdc,
    INT iSaveLevel)
{
    HDC hdcSave;
    PDC pdcSave;

    ASSERT(iSaveLevel > 0);
    DPRINT("DC_vRestoreDC(%p, %ld)\n", pdc->BaseObject.hHmgr, iSaveLevel);

    /* Loop the save levels */
    while (pdc->dclevel.lSaveDepth > iSaveLevel)
    {
        hdcSave = pdc->dclevel.hdcSave;
        DPRINT("RestoreDC = %p\n", hdcSave);

        /* Set us as the owner */
        if (!GreSetObjectOwner(hdcSave, GDI_OBJ_HMGR_POWNED))
        {
            /* Could not get ownership. That's bad! */
            DPRINT1("Could not get ownership of saved DC (%p) for hdc %p!\n",
                    hdcSave, pdc->BaseObject.hHmgr);
            return;// FALSE;
        }

        /* Lock the saved dc */
        pdcSave = DC_LockDc(hdcSave);
        if (!pdcSave)
        {
            /* WTF? Internal error! */
            DPRINT1("Could not lock the saved DC (%p) for dc %p!\n",
                    hdcSave, pdc->BaseObject.hHmgr);
            return;// FALSE;
        }

        /* Remove the saved dc from the queue */
        pdc->dclevel.hdcSave = pdcSave->dclevel.hdcSave;

        /* Decrement save level */
        pdc->dclevel.lSaveDepth--;

        /* Is this the state we want? */
        if (pdc->dclevel.lSaveDepth == iSaveLevel)
        {
            /* Copy the state back */
            DC_vCopyState(pdcSave, pdc, FALSE);

            /* Only memory DC's change their surface */
            if (pdc->dctype == DCTYPE_MEMORY)
                DC_vSelectSurface(pdc, pdcSave->dclevel.pSurface);

            // Restore Path by removing it, if the Save flag is set.
            // BeginPath will takecare of the rest.
            if (pdc->dclevel.hPath && pdc->dclevel.flPath & DCPATH_SAVE)
            {
                PATH_Delete(pdc->dclevel.hPath);
                pdc->dclevel.hPath = 0;
                pdc->dclevel.flPath &= ~DCPATH_SAVE;
            }
        }

        /* Prevent save dc from being restored */
        pdcSave->dclevel.lSaveDepth = 1;

        /* Unlock it */
        DC_UnlockDc(pdcSave);
        /* Delete the saved dc */
        GreDeleteObject(hdcSave);
    }

    DPRINT("Leave DC_vRestoreDC()\n");
}



BOOL
APIENTRY
NtGdiRestoreDC(
    HDC hdc,
    INT iSaveLevel)
{
    PDC pdc;

    DPRINT("NtGdiRestoreDC(%p, %d)\n", hdc, iSaveLevel);

    /* Lock the original DC */
    pdc = DC_LockDc(hdc);
    if (!pdc)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
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
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Call the internal function */
    DC_vRestoreDC(pdc, iSaveLevel);

    DC_UnlockDc(pdc);

    DPRINT("Leave NtGdiRestoreDC\n");
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

    DPRINT("NtGdiSaveDC(%p)\n", hDC);

    /* Lock the original dc */
    pdc = DC_LockDc(hDC);
    if (pdc == NULL)
    {
        DPRINT("Could not lock DC\n");
        EngSetLastError(ERROR_INVALID_HANDLE);
        return 0;
    }

    /* Allocate a new dc */
    pdcSave = DC_AllocDcWithHandle(GDILoObjType_LO_DC_TYPE);
    if (pdcSave == NULL)
    {
        DPRINT("Could not allocate a new DC\n");
        DC_UnlockDc(pdc);
        return 0;
    }
    hdcSave = pdcSave->BaseObject.hHmgr;

    InterlockedIncrement(&pdc->ppdev->cPdevRefs);
    DC_vInitDc(pdcSave, DCTYPE_MEMORY, pdc->ppdev);

    /* Handle references here correctly */
      //pdcSrc->dclevel.pSurface = NULL;
      //pdcSrc->dclevel.pbrFill = NULL;
      //pdcSrc->dclevel.pbrLine = NULL;
      //pdcSrc->dclevel.ppal = NULL;

    /* Make it a kernel handle
       (FIXME: Windows handles this differently, see Wiki) */
    GDIOBJ_vSetObjectOwner(&pdcSave->BaseObject, GDI_OBJ_HMGR_PUBLIC);

    /* Copy the current state */
    DC_vCopyState(pdc, pdcSave, FALSE);

    /* Only memory DC's change their surface */
    if (pdc->dctype == DCTYPE_MEMORY)
        DC_vSelectSurface(pdcSave, pdc->dclevel.pSurface);

    /* Copy path */
    /* FIXME: Why this way? */
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

    DPRINT("Leave NtGdiSaveDC: %ld, hdcSave = %p\n", lSaveDepth, hdcSave);
    return lSaveDepth;
}

/* EOF */
