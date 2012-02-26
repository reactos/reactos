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

static const ULONG HatchBrushes[NB_HATCH_STYLES][8] =
{
    {0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF}, /* HS_HORIZONTAL */
    {0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7}, /* HS_VERTICAL   */
    {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F}, /* HS_FDIAGONAL  */
    {0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE}, /* HS_BDIAGONAL  */
    {0xF7, 0xF7, 0xF7, 0xF7, 0x00, 0xF7, 0xF7, 0xF7}, /* HS_CROSS      */
    {0x7E, 0xBD, 0xDB, 0xE7, 0xE7, 0xDB, 0xBD, 0x7E}  /* HS_DIAGCROSS  */
};

BOOL
FASTCALL
IntGdiSetBrushOwner(PBRUSH pbr, ULONG ulOwner)
{
    // FIXME:
    if (pbr->flAttrs & GDIBRUSH_IS_GLOBAL) return TRUE;

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

BOOL
NTAPI
BRUSH_Cleanup(PVOID ObjectBody)
{
    PBRUSH pbrush = (PBRUSH)ObjectBody;
    if (pbrush->flAttrs & (GDIBRUSH_IS_HATCH | GDIBRUSH_IS_BITMAP))
    {
        ASSERT(pbrush->hbmPattern);
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

    return TRUE;
}

INT
FASTCALL
BRUSH_GetObject(PBRUSH pbrush, INT Count, LPLOGBRUSH Buffer)
{
    if (Buffer == NULL) return sizeof(LOGBRUSH);
    if (Count == 0) return 0;

    /* Set colour */
    Buffer->lbColor = pbrush->BrushAttr.lbColor;

    /* Set Hatch */
    if ((pbrush->flAttrs & GDIBRUSH_IS_HATCH)!=0)
    {
        /* FIXME: This is not the right value */
        Buffer->lbHatch = (LONG)pbrush->hbmPattern;
    }
    else
    {
        Buffer->lbHatch = 0;
    }

    Buffer->lbStyle = 0;

    /* Get the type of style */
    if ((pbrush->flAttrs & GDIBRUSH_IS_SOLID)!=0)
    {
        Buffer->lbStyle = BS_SOLID;
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_NULL)!=0)
    {
        Buffer->lbStyle = BS_NULL; // BS_HOLLOW
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_HATCH)!=0)
    {
        Buffer->lbStyle = BS_HATCHED;
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_BITMAP)!=0)
    {
        Buffer->lbStyle = BS_PATTERN;
    }
    else if ((pbrush->flAttrs & GDIBRUSH_IS_DIB)!=0)
    {
        Buffer->lbStyle = BS_DIBPATTERN;
    }

    /* FIXME
    else if ((pbrush->flAttrs & )!=0)
    {
        Buffer->lbStyle = BS_INDEXED;
    }
    else if ((pbrush->flAttrs & )!=0)
    {
        Buffer->lbStyle = BS_DIBPATTERNPT;
    }
    */

    /* FIXME */
    return sizeof(LOGBRUSH);
}

HBRUSH
APIENTRY
IntGdiCreateDIBBrush(
    CONST BITMAPINFO *BitmapInfo,
    UINT ColorSpec,
    UINT BitmapInfoSize,
    CONST VOID *PackedDIB)
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

    DataPtr = (ULONG_PTR)BitmapInfo + DIB_BitmapInfoSize(BitmapInfo, ColorSpec);

    hPattern = DIB_CreateDIBSection(NULL, BitmapInfo, ColorSpec, &pvDIBits, NULL, 0, 0);
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

    pbrush->flAttrs |= GDIBRUSH_IS_BITMAP | GDIBRUSH_IS_DIB;
    pbrush->hbmPattern = hPattern;
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
    HBITMAP hPattern;

    if (Style < 0 || Style >= NB_HATCH_STYLES)
    {
        return 0;
    }

    hPattern = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)HatchBrushes[Style]);
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

    pbrush->flAttrs |= GDIBRUSH_IS_HATCH;
    pbrush->hbmPattern = hPattern;
    pbrush->BrushAttr.lbColor = Color & 0xFFFFFF;

    GreSetObjectOwner(hPattern, GDI_OBJ_HMGR_PUBLIC);

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

    pbrush->flAttrs |= GDIBRUSH_IS_BITMAP;
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

    pbrush->flAttrs |= GDIBRUSH_IS_SOLID;

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

    pbrush->flAttrs |= GDIBRUSH_IS_NULL;
    GDIOBJ_vUnlockObject(&pbrush->BaseObject);

    return hBrush;
}

VOID
FASTCALL
IntGdiSetSolidBrushColor(HBRUSH hBrush, COLORREF Color)
{
    PBRUSH pbrush;

    pbrush = BRUSH_ShareLockBrush(hBrush);
    if (pbrush->flAttrs & GDIBRUSH_IS_SOLID)
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

/**
 * \name NtGdiSetBrushOrg
 *
 * \brief Sets the brush origin that GDI assigns to
 * the next brush an application selects into the specified device context.
 *
 * @implemented
 */
BOOL
APIENTRY
NtGdiSetBrushOrg(HDC hDC, INT XOrg, INT YOrg, LPPOINT Point)
{
    PDC dc;
    PDC_ATTR pdcattr;

    dc = DC_LockDc(hDC);
    if (dc == NULL)
    {
        EngSetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }
    pdcattr = dc->pdcattr;

    if (Point != NULL)
    {
        NTSTATUS Status = STATUS_SUCCESS;
        POINT SafePoint;
        SafePoint.x = pdcattr->ptlBrushOrigin.x;
        SafePoint.y = pdcattr->ptlBrushOrigin.y;
        _SEH2_TRY
        {
            ProbeForWrite(Point, sizeof(POINT), 1);
            *Point = SafePoint;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
        {
            DC_UnlockDc(dc);
            SetLastNtError(Status);
            return FALSE;
        }
    }

    pdcattr->ptlBrushOrigin.x = XOrg;
    pdcattr->ptlBrushOrigin.y = YOrg;
    IntptlBrushOrigin(dc, XOrg, YOrg );
    DC_UnlockDc(dc);

    return TRUE;
}

/* EOF */
