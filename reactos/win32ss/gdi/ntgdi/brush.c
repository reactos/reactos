/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS win32 subsystem
 * PURPOSE:           Functions for brushes
 * FILE:              subsystem/win32/win32k/objects/brush.c
 * PROGRAMER:
 */

#include <win32k.h>

#define NDEBUG
#include <debug.h>

#define GDIOBJATTRFREE 170

typedef struct _GDI_OBJ_ATTR_FREELIST
{
  LIST_ENTRY Entry;
  DWORD nEntries;
  PVOID AttrList[GDIOBJATTRFREE];
} GDI_OBJ_ATTR_FREELIST, *PGDI_OBJ_ATTR_FREELIST;

typedef struct _GDI_OBJ_ATTR_ENTRY
{
  RGN_ATTR Attr[GDIOBJATTRFREE];
} GDI_OBJ_ATTR_ENTRY, *PGDI_OBJ_ATTR_ENTRY;

BOOL
FASTCALL
IntGdiSetBrushOwner(PBRUSH pbr, ULONG ulOwner)
{
    // FIXME:
    if (pbr->flAttrs & BR_IS_GLOBAL) return TRUE;

    if ((ulOwner == GDI_OBJ_HMGR_PUBLIC) || ulOwner == GDI_OBJ_HMGR_NONE)
    {
        // Deny user access to User Data.
        GDIOBJ_vSetObjectAttr(&pbr->BaseObject, NULL);
        // FIXME: deallocate brush attr
    }

    if (ulOwner == GDI_OBJ_HMGR_POWNED)
    {
        // Allow user access to User Data.
        GDIOBJ_vSetObjectAttr(&pbr->BaseObject, pbr->pBrushAttr);
        // FIXME: Allocate brush attr
    }

    GDIOBJ_vSetObjectOwner(&pbr->BaseObject, ulOwner);

    return TRUE;
}

BOOL
FASTCALL
GreSetBrushOwner(HBRUSH hBrush, ULONG ulOwner)
{
    BOOL Ret;
    PBRUSH pbrush;

    pbrush = BRUSH_ShareLockBrush(hBrush);
    Ret = IntGdiSetBrushOwner(pbrush, ulOwner);
    BRUSH_ShareUnlockBrush(pbrush);
    return Ret;
}

BOOL
NTAPI
BRUSH_bAllocBrushAttr(PBRUSH pbr)
{
    PPROCESSINFO ppi;
    BRUSH_ATTR *pBrushAttr;

    ppi = PsGetCurrentProcessWin32Process();
    ASSERT(ppi);

    pBrushAttr = GdiPoolAllocate(ppi->pPoolDcAttr);
    if (!pBrushAttr)
    {
        DPRINT1("Could not allocate brush attr\n");
        return FALSE;
    }

    /* Copy the content from the kernel mode dc attr */
    pbr->pBrushAttr = pBrushAttr;
    *pbr->pBrushAttr = pbr->BrushAttr;

    /* Set the object attribute in the handle table */
    GDIOBJ_vSetObjectAttr(&pbr->BaseObject, pBrushAttr);

    DPRINT("BRUSH_bAllocBrushAttr: pbr=%p, pbr->pdcattr=%p\n", pbr, pbr->pBrushAttr);
    return TRUE;
}


VOID
NTAPI
BRUSH_vFreeBrushAttr(PBRUSH pbr)
{
#if 0
    PPROCESSINFO ppi;

    if (pbrush->pBrushAttr == &pbrush->BrushAttr) return;

    /* Reset the object attribute in the handle table */
    GDIOBJ_vSetObjectAttr(&pbr->BaseObject, NULL);

    /* Free memory from the process gdi pool */
    ppi = PsGetCurrentProcessWin32Process();
    ASSERT(ppi);
    GdiPoolFree(ppi->pPoolBrushAttr, pbr->pBrushAttr);
#endif
    /* Reset to kmode brush attribute */
    pbr->pBrushAttr = &pbr->BrushAttr;
}

VOID
NTAPI
BRUSH_vCleanup(PVOID ObjectBody)
{
    PBRUSH pbrush = (PBRUSH)ObjectBody;
    if (pbrush->hbmPattern)
    {
        GreSetObjectOwner(pbrush->hbmPattern, GDI_OBJ_HMGR_POWNED);
        GreDeleteObject(pbrush->hbmPattern);
    }

    /* Check if there is a usermode attribute */
    if (pbrush->pBrushAttr != &pbrush->BrushAttr)
    {
        BRUSH_vFreeBrushAttr(pbrush);
    }

    /* Free the kmode styles array of EXTPENS */
    if (pbrush->pStyle)
    {
        ExFreePool(pbrush->pStyle);
    }
}

INT
FASTCALL
BRUSH_GetObject(PBRUSH pbrush, INT cjSize, LPLOGBRUSH plogbrush)
{
    /* Check if only size is requested */
    if (plogbrush == NULL) return sizeof(LOGBRUSH);

    /* Check if size is ok */
    if (cjSize == 0) return 0;

    /* Set colour */
    plogbrush->lbColor = pbrush->BrushAttr.lbColor;

    /* Default to 0 */
    plogbrush->lbHatch = 0;

    /* Get the type of style */
    if (pbrush->flAttrs & BR_IS_SOLID)
    {
        plogbrush->lbStyle = BS_SOLID;
    }
    else if (pbrush->flAttrs & BR_IS_NULL)
    {
        plogbrush->lbStyle = BS_NULL; // BS_HOLLOW
    }
    else if (pbrush->flAttrs & BR_IS_HATCH)
    {
        plogbrush->lbStyle = BS_HATCHED;
        plogbrush->lbHatch = pbrush->ulStyle;
    }
    else if (pbrush->flAttrs & BR_IS_DIB)
    {
        plogbrush->lbStyle = BS_DIBPATTERN;
        plogbrush->lbHatch = (ULONG_PTR)pbrush->hbmClient;
    }
    else if (pbrush->flAttrs & BR_IS_BITMAP)
    {
        plogbrush->lbStyle = BS_PATTERN;
    }
    else
    {
        plogbrush->lbStyle = 0; // ???
    }

    /* FIXME
    else if (pbrush->flAttrs & )
    {
        plogbrush->lbStyle = BS_INDEXED;
    }
    else if (pbrush->flAttrs & )
    {
        plogbrush->lbStyle = BS_DIBPATTERNPT;
    }
    */

    /* FIXME */
    return sizeof(LOGBRUSH);
}

HBRUSH
APIENTRY
IntGdiCreateDIBBrush(
    const BITMAPINFO *BitmapInfo,
    UINT uUsage,
    UINT BitmapInfoSize,
    const VOID* pvClient)
{
    HBRUSH hBrush;
    PBRUSH pbrush;
    HBITMAP hPattern;
    ULONG_PTR DataPtr;
    PVOID pvDIBits;

    if (BitmapInfo->bmiHeader.biSize < sizeof(BITMAPINFOHEADER))
    {
        EngSetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    DataPtr = (ULONG_PTR)BitmapInfo + DIB_BitmapInfoSize(BitmapInfo, uUsage);

    hPattern = DIB_CreateDIBSection(NULL, BitmapInfo, uUsage, &pvDIBits, NULL, 0, 0);
    if (hPattern == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
	RtlCopyMemory(pvDIBits,
		          (PVOID)DataPtr,
				  DIB_GetDIBImageBytes(BitmapInfo->bmiHeader.biWidth,
                                       BitmapInfo->bmiHeader.biHeight,
                                       BitmapInfo->bmiHeader.biBitCount * BitmapInfo->bmiHeader.biPlanes));

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        GreDeleteObject(hPattern);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= BR_IS_BITMAP | BR_IS_DIB;
    if (uUsage == DIB_PAL_COLORS)
        pbrush->flAttrs |= BR_IS_DIBPALCOLORS;
    pbrush->hbmPattern = hPattern;
    pbrush->hbmClient = (HBITMAP)pvClient;
    /* FIXME: Fill in the rest of fields!!! */

    GreSetObjectOwner(hPattern, GDI_OBJ_HMGR_PUBLIC);

    GDIOBJ_vUnlockObject(&pbrush->BaseObject);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreateHatchBrush(
    INT Style,
    COLORREF Color)
{
    HBRUSH hBrush;
    PBRUSH pbrush;

    if (Style < 0 || Style >= NB_HATCH_STYLES)
    {
        return 0;
    }

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= BR_IS_HATCH;
    pbrush->BrushAttr.lbColor = Color & 0xFFFFFF;
    pbrush->ulStyle = Style;

    GDIOBJ_vUnlockObject(&pbrush->BaseObject);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreatePatternBrush(
    HBITMAP hBitmap)
{
    HBRUSH hBrush;
    PBRUSH pbrush;
    HBITMAP hPattern;

    hPattern = BITMAP_CopyBitmap(hBitmap);
    if (hPattern == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        GreDeleteObject(hPattern);
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= BR_IS_BITMAP;
    pbrush->hbmPattern = hPattern;
    /* FIXME: Fill in the rest of fields!!! */

    GreSetObjectOwner(hPattern, GDI_OBJ_HMGR_PUBLIC);

    GDIOBJ_vUnlockObject(&pbrush->BaseObject);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreateSolidBrush(
    COLORREF Color)
{
    HBRUSH hBrush;
    PBRUSH pbrush;

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= BR_IS_SOLID;

    pbrush->BrushAttr.lbColor = Color & 0x00FFFFFF;
    /* FIXME: Fill in the rest of fields!!! */

    GDIOBJ_vUnlockObject(&pbrush->BaseObject);

    return hBrush;
}

HBRUSH
APIENTRY
IntGdiCreateNullBrush(VOID)
{
    HBRUSH hBrush;
    PBRUSH pbrush;

    pbrush = BRUSH_AllocBrushWithHandle();
    if (pbrush == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    hBrush = pbrush->BaseObject.hHmgr;

    pbrush->flAttrs |= BR_IS_NULL;
    GDIOBJ_vUnlockObject(&pbrush->BaseObject);

    return hBrush;
}

VOID
FASTCALL
IntGdiSetSolidBrushColor(HBRUSH hBrush, COLORREF Color)
{
    PBRUSH pbrush;

    pbrush = BRUSH_ShareLockBrush(hBrush);
    if (pbrush->flAttrs & BR_IS_SOLID)
    {
        pbrush->BrushAttr.lbColor = Color & 0xFFFFFF;
    }
    BRUSH_ShareUnlockBrush(pbrush);
}


/* PUBLIC FUNCTIONS ***********************************************************/

HBRUSH
APIENTRY
NtGdiCreateDIBBrush(
    IN PVOID BitmapInfoAndData,
    IN FLONG ColorSpec,
    IN UINT BitmapInfoSize,
    IN BOOL  b8X8,
    IN BOOL bPen,
    IN PVOID PackedDIB)
{
    BITMAPINFO *SafeBitmapInfoAndData;
    NTSTATUS Status = STATUS_SUCCESS;
    HBRUSH hBrush;

    SafeBitmapInfoAndData = EngAllocMem(FL_ZERO_MEMORY, BitmapInfoSize, TAG_DIB);
    if (SafeBitmapInfoAndData == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    _SEH2_TRY
    {
        ProbeForRead(BitmapInfoAndData, BitmapInfoSize, 1);
        RtlCopyMemory(SafeBitmapInfoAndData, BitmapInfoAndData, BitmapInfoSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (!NT_SUCCESS(Status))
    {
        EngFreeMem(SafeBitmapInfoAndData);
        SetLastNtError(Status);
        return 0;
    }

    hBrush = IntGdiCreateDIBBrush(SafeBitmapInfoAndData,
                                  ColorSpec,
                                  BitmapInfoSize,
                                  PackedDIB);

    EngFreeMem(SafeBitmapInfoAndData);

    return hBrush;
}

HBRUSH
APIENTRY
NtGdiCreateHatchBrushInternal(
    ULONG Style,
    COLORREF Color,
    BOOL bPen)
{
    return IntGdiCreateHatchBrush(Style, Color);
}

HBRUSH
APIENTRY
NtGdiCreatePatternBrushInternal(
    HBITMAP hBitmap,
    BOOL bPen,
    BOOL b8x8)
{
    return IntGdiCreatePatternBrush(hBitmap);
}

HBRUSH
APIENTRY
NtGdiCreateSolidBrush(COLORREF Color,
                      IN OPTIONAL HBRUSH hbr)
{
    return IntGdiCreateSolidBrush(Color);
}

HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(
    _In_ HBRUSH hbr,
    _Out_ UINT *piUsage)
{
    HBITMAP hbmPattern;
    PBRUSH pbr;

    /* Lock the brush */
    pbr = BRUSH_ShareLockBrush(hbr);
    if (pbr == NULL)
    {
        DPRINT1("Could not lock brush\n");
        return NULL;
    }

    /* Get the pattern bitmap handle */
    hbmPattern = pbr->hbmPattern;

    _SEH2_TRY
    {
        ProbeForWrite(piUsage, sizeof(*piUsage), sizeof(*piUsage));

        /* Set usage according to flags */
        if (pbr->flAttrs & BR_IS_DIBPALCOLORS)
            *piUsage = DIB_PAL_COLORS;
        else
            *piUsage = DIB_RGB_COLORS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("Got exception!\n");
        hbmPattern = NULL;
    }
    _SEH2_END;

    /* Unlock the brush */
    BRUSH_ShareUnlockBrush(pbr);

    /* Return the pattern bitmap handle */
    return hbmPattern;
}

/*
 * @unimplemented
 */
HBRUSH
APIENTRY
NtGdiSetBrushAttributes(
    IN HBRUSH hbm,
    IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}


/*
 * @unimplemented
 */
HBRUSH
APIENTRY
NtGdiClearBrushAttributes(
    IN HBRUSH hbr,
    IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}


/* EOF */
