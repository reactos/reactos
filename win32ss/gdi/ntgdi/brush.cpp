/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS win32 subsystem
 * PURPOSE:           BRUSH class implementation
 * PROGRAMER:         Timo Kreuzer (timo.kreuzer@reactos.org)
 *
 * REFERENCES:        https://www.betaarchive.com/wiki/index.php?title=Microsoft_KB_Archive/108497
 */

#include "brush.hpp"

DBG_DEFAULT_CHANNEL(GdiBrush);

BRUSH::BRUSH(
    _In_ FLONG flAttrs,
    _In_ COLORREF crColor,
    _In_ ULONG iHatch,
    _In_opt_ HBITMAP hbmPattern,
    _In_opt_ PVOID pvClient,
    _In_ GDILOOBJTYPE loobjtype = GDILoObjType_LO_BRUSH_TYPE)
    : BASEOBJECT(loobjtype)
{
    static ULONG ulGlobalBrushUnique = 0;

    /* Get a unique value */
    this->ulBrushUnique = InterlockedIncrementUL(&ulGlobalBrushUnique);

    /* Start with kmode brush attribute */
    this->pBrushAttr = &this->BrushAttr;

    /* Set parameters */
    this->flAttrs = flAttrs;
    this->iHatch = iHatch;
    this->hbmPattern = hbmPattern;
    this->hbmClient = (HBITMAP)pvClient;
    this->pBrushAttr->lbColor = crColor;

    /* Initialize the other fields */
    this->ptOrigin.x = 0;
    this->ptOrigin.y = 0;
    this->bCacheGrabbed = FALSE;
    this->crBack = 0;
    this->crFore = 0;
    this->ulPalTime = 0;
    this->ulSurfTime = 0;
    this->pvRBrush = NULL;
    this->hdev = NULL;

    /* FIXME: should be done only in PEN constructor,
       but our destructor needs it! */
    this->dwStyleCount = 0;
    this->pStyle = NULL;
}

BRUSH::~BRUSH(
    VOID)
{
    /* Check if we have a user mode brush attribute */
    if (this->pBrushAttr != &this->BrushAttr)
    {
        /* Free memory to the process GDI pool */
        GdiPoolFree(GetBrushAttrPool(), this->pBrushAttr);
    }

    /* Delete the pattern bitmap (may have already been deleted during gdi cleanup) */
    if (this->hbmPattern != NULL && GreIsHandleValid(this->hbmPattern))
    {
        GreSetBitmapOwner(this->hbmPattern, BASEOBJECT::OWNER::POWNED);
        GreDeleteObject(this->hbmPattern);
    }

    /* Delete styles */
    if ((this->pStyle != NULL) && !(this->flAttrs & BR_IS_DEFAULTSTYLE))
    {
        ExFreePoolWithTag(this->pStyle, GDITAG_PENSTYLE);
    }
}

VOID
BRUSH::vReleaseAttribute(VOID)
{
    if (this->pBrushAttr != &this->BrushAttr)
    {
        this->BrushAttr = *this->pBrushAttr;
        GdiPoolFree(GetBrushAttrPool(), this->pBrushAttr);
        this->pBrushAttr = &this->BrushAttr;
    }
}

VOID
BRUSH::vDeleteObject(
    _In_ PVOID pvObject)
{
    PBRUSH pbr = static_cast<PBRUSH>(pvObject);
    NT_ASSERT((GDI_HANDLE_GET_TYPE(pbr->hHmgr()) == GDILoObjType_LO_BRUSH_TYPE) ||
              (GDI_HANDLE_GET_TYPE(pbr->hHmgr()) == GDILoObjType_LO_PEN_TYPE) ||
              (GDI_HANDLE_GET_TYPE(pbr->hHmgr()) == GDILoObjType_LO_EXTPEN_TYPE));
    delete pbr;
}

BOOL
BRUSH::bAllocateBrushAttr(
    VOID)
{
    PBRUSH_ATTR pBrushAttr;
    NT_ASSERT(this->pBrushAttr == &this->BrushAttr);

    /* Allocate a brush attribute from the pool */
    pBrushAttr = static_cast<PBRUSH_ATTR>(GdiPoolAllocate(GetBrushAttrPool()));
    if (pBrushAttr == NULL)
    {
        ERR("Could not allocate brush attr\n");
        return FALSE;
    }

    /* Copy the content from the kernel mode brush attribute */
    this->pBrushAttr = pBrushAttr;
    *this->pBrushAttr = this->BrushAttr;

    /* Set the object attribute in the handle table */
    vSetObjectAttr(pBrushAttr);

    return TRUE;
}

VOID
BRUSH::vSetSolidColor(
    _In_ COLORREF crColor)
{
    NT_ASSERT(this->flAttrs & BR_IS_SOLID);

    /* Set new color and reset the pal times */
    this->pBrushAttr->lbColor = crColor & 0xFFFFFF;
    this->ulPalTime = -1;
    this->ulSurfTime = -1;
}

HBITMAP
BRUSH::hbmGetBitmapHandle(
    _Out_ PUINT puUsage) const
{
    /* Return the color usage based on flags */
    *puUsage = (this->flAttrs & BR_IS_DIBPALCOLORS) ? DIB_PAL_COLORS :
               (this->flAttrs & BR_IS_DIBPALINDICES) ? DIB_PAL_INDICES :
               DIB_RGB_COLORS;

    return this->hbmPattern;
}

UINT
BRUSH::cjGetObject(
    _In_ UINT cjSize,
    _Out_bytecap_(cjSize) PLOGBRUSH plb) const
{
    /* Check if only size is requested */
    if (plb == NULL)
        return sizeof(LOGBRUSH);

    /* Check if size is ok */
    if (cjSize == 0)
        return 0;

    /* Set color */
    plb->lbColor = this->BrushAttr.lbColor;

    /* Set style and hatch based on the attribute flags */
    if (this->flAttrs & BR_IS_SOLID)
    {
        plb->lbStyle = BS_SOLID;
        plb->lbHatch = 0;
    }
    else if (this->flAttrs & BR_IS_HATCH)
    {
        plb->lbStyle = BS_HATCHED;
        plb->lbHatch = this->iHatch;
    }
    else if (this->flAttrs & BR_IS_DIB)
    {
        plb->lbStyle = BS_DIBPATTERN;
        plb->lbHatch = (ULONG_PTR)this->hbmClient;
    }
    else if (this->flAttrs & BR_IS_BITMAP)
    {
        plb->lbStyle = BS_PATTERN;
        plb->lbHatch = (ULONG_PTR)this->hbmClient;
    }
    else if (this->flAttrs & BR_IS_NULL)
    {
        plb->lbStyle = BS_NULL;
        plb->lbHatch = 0;
    }
    else
    {
        NT_ASSERT(FALSE);
    }

    return sizeof(LOGBRUSH);
}

static
HBRUSH
CreateBrushInternal(
    _In_ ULONG flAttrs,
    _In_ COLORREF crColor,
    _In_ ULONG iHatch,
    _In_opt_ HBITMAP hbmPattern,
    _In_opt_ PVOID pvClient)
{
    BASEOBJECT::OWNER owner;
    PBRUSH pbr;
    HBRUSH hbr;

    NT_ASSERT(((flAttrs & BR_IS_BITMAP) == 0) || (hbmPattern != NULL));

    /* Create the brush (brush takes ownership of the bitmap) */
    pbr = new BRUSH(flAttrs, crColor, iHatch, hbmPattern, pvClient);
    if (pbr == NULL)
    {
        ERR("Failed to allocate a brush\n");
        GreSetBitmapOwner(hbmPattern, BASEOBJECT::OWNER::POWNED);
        GreDeleteObject(hbmPattern);
        return NULL;
    }

    /* Check if this is a global brush */
    if (!(flAttrs & BR_IS_GLOBAL))
    {
        /* Not a global brush, so allocate a user mode brush attribute */
        if (!pbr->bAllocateBrushAttr())
        {
            ERR("Failed to allocate brush attribute\n");
            delete pbr;
            return NULL;
        }
    }

    /* Set the owner, either public or process owned */
    owner = (flAttrs & BR_IS_GLOBAL) ? BASEOBJECT::OWNER::PUBLIC :
                                       BASEOBJECT::OWNER::POWNED;

    /* Insert the object into the GDI handle table */
    hbr =  static_cast<HBRUSH>(pbr->hInsertObject(owner));
    if (hbr == NULL)
    {
        ERR("Failed to insert brush\n");
        delete pbr;
        return NULL;
    }

    /* Unlock the brush */
    pbr->vUnlock();

    return hbr;
}


/* C interface ***************************************************************/

extern "C" {

VOID
NTAPI
BRUSH_vDeleteObject(
    PVOID pvObject)
{
    BRUSH::vDeleteObject(pvObject);
}

INT
FASTCALL
BRUSH_GetObject(
    PBRUSH pbr,
    INT cjBuffer,
    LPLOGBRUSH plbBuffer)
{
    return pbr->cjGetObject(cjBuffer, plbBuffer);
}

HBRUSH
NTAPI
IntGdiCreateNullBrush(
    VOID)
{
    /* Call the internal function */
    return CreateBrushInternal(BR_IS_NULL | BR_IS_GLOBAL, 0, 0, NULL, NULL);
}

HBRUSH
APIENTRY
IntGdiCreateSolidBrush(
    COLORREF crColor)
{
    /* Call the internal function */
    return CreateBrushInternal(BR_IS_SOLID | BR_IS_GLOBAL,
                               crColor,
                               0,
                               NULL,
                               NULL);
}

HBRUSH
NTAPI
IntGdiCreatePatternBrush(
    HBITMAP hbmPattern)
{
    NT_ASSERT(hbmPattern != NULL);
    GreSetBitmapOwner(hbmPattern, BASEOBJECT::OWNER::PUBLIC);
    return CreateBrushInternal(BR_IS_BITMAP | BR_IS_GLOBAL,
                               0,
                               0,
                               hbmPattern,
                               NULL);
}

VOID
NTAPI
IntGdiSetSolidBrushColor(
    _In_ HBRUSH hbr,
    _In_ COLORREF crColor)
{
    PBRUSH pbr;

    /* Lock the brush */
    pbr = BRUSH::LockAny(hbr);
    if (pbr == NULL)
    {
        ERR("Failed to lock brush %p\n", hbr);
        return;
    }

    /* Call the member function */
    pbr->vSetSolidColor(crColor);

    /* Unlock the brush */
    pbr->vUnlock();
}

__kernel_entry
HBRUSH
APIENTRY
NtGdiCreateSolidBrush(
    _In_ COLORREF crColor,
    _In_opt_ HBRUSH hbr)
{
    if (hbr != NULL)
    {
        WARN("hbr is not supported, ignoring\n");
    }

    /* Call the internal function */
    return CreateBrushInternal(BR_IS_SOLID, crColor, 0, NULL, NULL);
}

__kernel_entry
HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(
    _In_ ULONG iHatch,
    _In_ COLORREF crColor,
    _In_ BOOL bPen)
{
    FLONG flAttr;

    if (bPen)
    {
        WARN("bPen is not supported, ignoring\n");
    }

    /* Check what kind if hatch style this is */
    if (iHatch < HS_DDI_MAX)
    {
        flAttr = BR_IS_HATCH;
    }
    else if (iHatch < HS_API_MAX)
    {
        flAttr = BR_IS_SOLID;
    }
    else
    {
        ERR("Invalid iHatch: %lu\n", iHatch);
        return NULL;
    }

    /* Call the internal function */
    return CreateBrushInternal(flAttr, crColor, iHatch, NULL, NULL);
}

__kernel_entry
HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(
    _In_ HBITMAP hbmClient,
    _In_ BOOL bPen,
    _In_ BOOL b8X8)
{
    HBITMAP hbmPattern;

    if (b8X8)
    {
        WARN("b8X8 is not supported, ignoring\n");
    }

    if (bPen)
    {
        WARN("bPen is not supported, ignoring\n");
    }

    /* Copy the bitmap */
    hbmPattern = BITMAP_CopyBitmap(hbmClient);
    if (hbmPattern == NULL)
    {
        ERR("Failed to copy the bitmap %p\n", hbmPattern);
        return NULL;
    }

    /* Call the internal function (will delete hbmPattern on failure) */
    return CreateBrushInternal(BR_IS_BITMAP, 0, 0, hbmPattern, hbmClient);
}

__kernel_entry
HBRUSH
APIENTRY
NtGdiCreateDIBBrush(
    _In_reads_bytes_(cj) PVOID pv,
    _In_ FLONG uUsage,
    _In_ UINT cj,
    _In_ BOOL b8X8,
    _In_ BOOL bPen,
    _In_ PVOID pvClient)
{
    PVOID pvPackedDIB;
    FLONG flAttrs;
    HBITMAP hbm;
    HBRUSH hbr = NULL;

    if (b8X8)
    {
        WARN("b8X8 is not supported, ignoring\n");
    }

    if (bPen)
    {
        WARN("bPen is not supported, ignoring\n");
    }

    if (uUsage > DIB_PAL_INDICES)
    {
        ERR("Invalid uUsage value: %lu\n", uUsage);
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Allocate a buffer for the packed DIB */
    pvPackedDIB = ExAllocatePoolWithTag(PagedPool, cj, GDITAG_TEMP);
    if (pvPackedDIB == NULL)
    {
        ERR("Failed to allocate temp buffer of %u bytes\n", cj);
        return NULL;
    }

    /* Probe and copy the packed DIB */
    _SEH2_TRY
    {
        ProbeForRead(pv, cj, 1);
        RtlCopyMemory(pvPackedDIB, pv, cj);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Got exception, pv = %p, cj = %lu\n", pv, cj);
        goto cleanup;
    }
    _SEH2_END;

    flAttrs = BR_IS_BITMAP | BR_IS_DIB;

    /* Check what kind of color table we have */
    if (uUsage == DIB_PAL_COLORS)
    {
        /* Remember it and use DIB_PAL_BRUSHHACK to create a "special" palette */
        flAttrs |= BR_IS_DIBPALCOLORS;
        uUsage = DIB_PAL_BRUSHHACK;
    }
    else if (uUsage == DIB_PAL_INDICES)
    {
        /* No color table, bitmap contains device palette indices */
        flAttrs |= BR_IS_DIBPALINDICES;

        /* FIXME: This makes tests pass, but needs investigation. */
        flAttrs |= BR_IS_NULL;
    }

    /* Create a bitmap from the DIB */
    hbm = GreCreateDIBitmapFromPackedDIB(pvPackedDIB, cj, uUsage);
    if (hbm == NULL)
    {
        ERR("Failed to create bitmap from DIB\n");
        goto cleanup;
    }

    /* Call the internal function (will delete hbm on failure) */
    hbr = CreateBrushInternal(flAttrs, 0, 0, hbm, pvClient);

cleanup:

    ExFreePoolWithTag(pvPackedDIB, GDITAG_TEMP);

    return hbr;
}

__kernel_entry
HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    _In_ HBRUSH hbr,
    _Out_ UINT *piUsage)
{
    PBRUSH pbr;
    HBITMAP hbm;
    UINT uUsage;

    /* Lock the brush */
    pbr = BRUSH::LockForRead(hbr);
    if (pbr == NULL)
    {
        ERR("Failed to lock brush %p\n", hbr);
        return NULL;
    }

    /* Call the member function */
    hbm = pbr->hbmGetBitmapHandle(&uUsage);

    /* Unlock the brush */
    pbr->vUnlock();

    _SEH2_TRY
    {
        ProbeForWrite(piUsage, sizeof(*piUsage), 1);
        *piUsage = uUsage;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ERR("Got exception! piUsage = %p\n", piUsage);
        hbm = NULL;
    }
    _SEH2_END;

    return hbm;
}

__kernel_entry
HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    _In_ HBRUSH hbr,
    _In_ DWORD dwFlags)
{
    PBRUSH pbr;
    if ( dwFlags & SC_BB_STOCKOBJ )
    {
        if (GDIOBJ_ConvertToStockObj((HGDIOBJ*)&hbr))
        {
            pbr = BRUSH::LockAny(hbr);
            if (pbr == NULL)
            {
                ERR("Failed to lock brush %p\n", hbr);
                return NULL;
            }
            pbr->vReleaseAttribute();
            pbr->vUnlock();
            return hbr;
        }
    }
    return NULL;
}

__kernel_entry
HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    _In_ HBRUSH hbr,
    _In_ DWORD dwFlags)
{
    PBRUSH pbr;
    if ( dwFlags & SC_BB_STOCKOBJ )
    {
        if (GDIOBJ_ConvertFromStockObj((HGDIOBJ*)&hbr))
        {
            pbr = BRUSH::LockAny(hbr);
            if (pbr == NULL)
            {
                ERR("Failed to lock brush %p\n", hbr);
                return NULL;
            }
            if (!pbr->bAllocateBrushAttr())
            {
                ERR("Failed to allocate brush attribute\n");
            }
            pbr->vUnlock();
            return hbr;
        }
    }
    return NULL;
}

} /* extern "C" */
