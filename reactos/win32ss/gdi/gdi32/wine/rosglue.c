
#include <precomp.h>
#include "gdi_private.h"
#undef SetWorldTransform

#define NDEBUG
#include <debug.h>

static
INT_PTR
NULL_Unused()
{
    __debugbreak();
    return 0;
}

static INT   NULL_SaveDC(PHYSDEV dev) { return 1; }
static BOOL  NULL_RestoreDC(PHYSDEV dev, INT level) { return TRUE; }
static INT   NULL_SetMapMode(PHYSDEV dev, INT iMode) { return 1; }
static HFONT NULL_SelectFont(PHYSDEV dev, HFONT hFont, UINT *aa_flags) { return NULL; }
static BOOL  NULL_SetWindowExtEx(PHYSDEV dev, INT cx, INT cy, SIZE *size) { return TRUE; }
static BOOL  NULL_SetViewportExtEx(PHYSDEV dev, INT cx, INT cy, SIZE *size) { return TRUE; }
static BOOL  NULL_SetWindowOrgEx(PHYSDEV dev, INT x, INT y, POINT *pt) { return TRUE; }
static BOOL  NULL_SetViewportOrgEx(PHYSDEV dev, INT x, INT y, POINT *pt) { return TRUE; }
static INT   NULL_ExtSelectClipRgn(PHYSDEV dev, HRGN hrgn, INT iMode) { return 1; }
static INT   NULL_IntersectClipRect(PHYSDEV dev, INT left, INT top, INT right, INT bottom) { return 1; }
static INT   NULL_OffsetClipRgn(PHYSDEV dev, INT x, INT y) { return SIMPLEREGION; }
static INT   NULL_ExcludeClipRect(PHYSDEV dev, INT left, INT top, INT right, INT bottom) { return 1; }

static const struct gdi_dc_funcs DummyPhysDevFuncs =
{
    (PVOID)NULL_Unused, //INT      (*pAbortDoc)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pAbortPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pAlphaBlend)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,BLENDFUNCTION);
    (PVOID)NULL_Unused, //BOOL     (*pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    (PVOID)NULL_Unused, //BOOL     (*pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pBeginPath)(PHYSDEV);
    (PVOID)NULL_Unused, //DWORD    (*pBlendImage)(PHYSDEV,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,BLENDFUNCTION);
    (PVOID)NULL_Unused, //BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pCloseFigure)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pCreateCompatibleDC)(PHYSDEV,PHYSDEV*);
    (PVOID)NULL_Unused, //BOOL     (*pCreateDC)(PHYSDEV*,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    (PVOID)NULL_Unused, //BOOL     (*pDeleteDC)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pDeleteObject)(PHYSDEV,HGDIOBJ);
    (PVOID)NULL_Unused, //DWORD    (*pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    (PVOID)NULL_Unused, //BOOL     (*pEllipse)(PHYSDEV,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //INT      (*pEndDoc)(PHYSDEV);
    (PVOID)NULL_Unused, //INT      (*pEndPage)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pEndPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pEnumFonts)(PHYSDEV,LPLOGFONTW,FONTENUMPROCW,LPARAM);
    (PVOID)NULL_Unused, //INT      (*pEnumICMProfiles)(PHYSDEV,ICMENUMPROCW,LPARAM);
    NULL_ExcludeClipRect, //INT      (*pExcludeClipRect)(PHYSDEV,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //INT      (*pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
    (PVOID)NULL_Unused, //INT      (*pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    (PVOID)NULL_Unused, //BOOL     (*pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    NULL_ExtSelectClipRgn, //INT      (*pExtSelectClipRgn)(PHYSDEV,HRGN,INT);
    (PVOID)NULL_Unused, //BOOL     (*pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    (PVOID)NULL_Unused, //BOOL     (*pFillPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    (PVOID)NULL_Unused, //BOOL     (*pFlattenPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pFontIsLinked)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pGdiComment)(PHYSDEV,UINT,const BYTE*);
    (PVOID)NULL_Unused, //BOOL     (*pGdiRealizationInfo)(PHYSDEV,void*);
    (PVOID)NULL_Unused, //UINT     (*pGetBoundsRect)(PHYSDEV,RECT*,UINT);
    (PVOID)NULL_Unused, //BOOL     (*pGetCharABCWidths)(PHYSDEV,UINT,UINT,LPABC);
    (PVOID)NULL_Unused, //BOOL     (*pGetCharABCWidthsI)(PHYSDEV,UINT,UINT,WORD*,LPABC);
    (PVOID)NULL_Unused, //BOOL     (*pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    (PVOID)NULL_Unused, //INT      (*pGetDeviceCaps)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //BOOL     (*pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    (PVOID)NULL_Unused, //DWORD    (*pGetFontData)(PHYSDEV,DWORD,DWORD,LPVOID,DWORD);
    (PVOID)NULL_Unused, //DWORD    (*pGetFontUnicodeRanges)(PHYSDEV,LPGLYPHSET);
    (PVOID)NULL_Unused, //DWORD    (*pGetGlyphIndices)(PHYSDEV,LPCWSTR,INT,LPWORD,DWORD);
    (PVOID)NULL_Unused, //DWORD    (*pGetGlyphOutline)(PHYSDEV,UINT,UINT,LPGLYPHMETRICS,DWORD,LPVOID,const MAT2*);
    (PVOID)NULL_Unused, //BOOL     (*pGetICMProfile)(PHYSDEV,LPDWORD,LPWSTR);
    (PVOID)NULL_Unused, //DWORD    (*pGetImage)(PHYSDEV,BITMAPINFO*,struct gdi_image_bits*,struct bitblt_coords*);
    (PVOID)NULL_Unused, //DWORD    (*pGetKerningPairs)(PHYSDEV,DWORD,LPKERNINGPAIR);
    (PVOID)NULL_Unused, //COLORREF (*pGetNearestColor)(PHYSDEV,COLORREF);
    (PVOID)NULL_Unused, //UINT     (*pGetOutlineTextMetrics)(PHYSDEV,UINT,LPOUTLINETEXTMETRICW);
    (PVOID)NULL_Unused, //COLORREF (*pGetPixel)(PHYSDEV,INT,INT);
    (PVOID)NULL_Unused, //UINT     (*pGetSystemPaletteEntries)(PHYSDEV,UINT,UINT,LPPALETTEENTRY);
    (PVOID)NULL_Unused, //UINT     (*pGetTextCharsetInfo)(PHYSDEV,LPFONTSIGNATURE,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pGetTextExtentExPoint)(PHYSDEV,LPCWSTR,INT,LPINT);
    (PVOID)NULL_Unused, //BOOL     (*pGetTextExtentExPointI)(PHYSDEV,const WORD*,INT,LPINT);
    (PVOID)NULL_Unused, //INT      (*pGetTextFace)(PHYSDEV,INT,LPWSTR);
    (PVOID)NULL_Unused, //BOOL     (*pGetTextMetrics)(PHYSDEV,TEXTMETRICW*);
    (PVOID)NULL_Unused, //BOOL     (*pGradientFill)(PHYSDEV,TRIVERTEX*,ULONG,void*,ULONG,ULONG);
    NULL_IntersectClipRect, //INT      (*pIntersectClipRect)(PHYSDEV,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pInvertRgn)(PHYSDEV,HRGN);
    (PVOID)NULL_Unused, //BOOL     (*pLineTo)(PHYSDEV,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pModifyWorldTransform)(PHYSDEV,const XFORM*,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pMoveTo)(PHYSDEV,INT,INT);
    NULL_OffsetClipRgn, //INT      (*pOffsetClipRgn)(PHYSDEV,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pOffsetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    (PVOID)NULL_Unused, //BOOL     (*pOffsetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    (PVOID)NULL_Unused, //BOOL     (*pPaintRgn)(PHYSDEV,HRGN);
    (PVOID)NULL_Unused, //BOOL     (*pPatBlt)(PHYSDEV,struct bitblt_coords*,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pPie)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pPolyBezier)(PHYSDEV,const POINT*,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pPolyBezierTo)(PHYSDEV,const POINT*,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pPolyDraw)(PHYSDEV,const POINT*,const BYTE *,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pPolyPolygon)(PHYSDEV,const POINT*,const INT*,UINT);
    (PVOID)NULL_Unused, //BOOL     (*pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pPolygon)(PHYSDEV,const POINT*,INT);
    (PVOID)NULL_Unused, //BOOL     (*pPolyline)(PHYSDEV,const POINT*,INT);
    (PVOID)NULL_Unused, //BOOL     (*pPolylineTo)(PHYSDEV,const POINT*,INT);
    (PVOID)NULL_Unused, //DWORD    (*pPutImage)(PHYSDEV,HRGN,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,DWORD);
    (PVOID)NULL_Unused, //UINT     (*pRealizeDefaultPalette)(PHYSDEV);
    (PVOID)NULL_Unused, //UINT     (*pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    (PVOID)NULL_Unused, //BOOL     (*pRectangle)(PHYSDEV,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //HDC      (*pResetDC)(PHYSDEV,const DEVMODEW*);
    NULL_RestoreDC, //BOOL     (*pRestoreDC)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    NULL_SaveDC, //INT      (*pSaveDC)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pScaleViewportExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    (PVOID)NULL_Unused, //BOOL     (*pScaleWindowExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    (PVOID)NULL_Unused, //HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    (PVOID)NULL_Unused, //HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH,const struct brush_pattern*);
    (PVOID)NULL_Unused, //BOOL     (*pSelectClipPath)(PHYSDEV,INT);
    NULL_SelectFont, //HFONT    (*pSelectFont)(PHYSDEV,HFONT,UINT*);
    (PVOID)NULL_Unused, //HPALETTE (*pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    (PVOID)NULL_Unused, //HPEN     (*pSelectPen)(PHYSDEV,HPEN,const struct brush_pattern*);
    (PVOID)NULL_Unused, //INT      (*pSetArcDirection)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    (PVOID)NULL_Unused, //INT      (*pSetBkMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //UINT     (*pSetBoundsRect)(PHYSDEV,RECT*,UINT);
    (PVOID)NULL_Unused, //COLORREF (*pSetDCBrushColor)(PHYSDEV, COLORREF);
    (PVOID)NULL_Unused, //COLORREF (*pSetDCPenColor)(PHYSDEV, COLORREF);
    (PVOID)NULL_Unused, //INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,BITMAPINFO*,UINT);
    (PVOID)NULL_Unused, //VOID     (*pSetDeviceClipping)(PHYSDEV,HRGN);
    (PVOID)NULL_Unused, //BOOL     (*pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    (PVOID)NULL_Unused, //DWORD    (*pSetLayout)(PHYSDEV,DWORD);
    NULL_SetMapMode, //INT      (*pSetMapMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //DWORD    (*pSetMapperFlags)(PHYSDEV,DWORD);
    (PVOID)NULL_Unused, //COLORREF (*pSetPixel)(PHYSDEV,INT,INT,COLORREF);
    (PVOID)NULL_Unused, //INT      (*pSetPolyFillMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //INT      (*pSetROP2)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //INT      (*pSetRelAbs)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //INT      (*pSetStretchBltMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //UINT     (*pSetTextAlign)(PHYSDEV,UINT);
    (PVOID)NULL_Unused, //INT      (*pSetTextCharacterExtra)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //COLORREF (*pSetTextColor)(PHYSDEV,COLORREF);
    (PVOID)NULL_Unused, //BOOL     (*pSetTextJustification)(PHYSDEV,INT,INT);
    NULL_SetViewportExtEx, //BOOL     (*pSetViewportExtEx)(PHYSDEV,INT,INT,SIZE*);
    NULL_SetViewportOrgEx, //BOOL     (*pSetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    NULL_SetWindowExtEx, //BOOL     (*pSetWindowExtEx)(PHYSDEV,INT,INT,SIZE*);
    NULL_SetWindowOrgEx, //BOOL     (*pSetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    (PVOID)NULL_Unused, //BOOL     (*pSetWorldTransform)(PHYSDEV,const XFORM*);
    (PVOID)NULL_Unused, //INT      (*pStartDoc)(PHYSDEV,const DOCINFOW*);
    (PVOID)NULL_Unused, //INT      (*pStartPage)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pStretchBlt)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,DWORD);
    (PVOID)NULL_Unused, //INT      (*pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void*,BITMAPINFO*,UINT,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pStrokeAndFillPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pStrokePath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pUnrealizePalette)(HPALETTE);
    (PVOID)NULL_Unused, //BOOL     (*pWidenPath)(PHYSDEV);
    (PVOID)NULL_Unused, //struct opengl_funcs * (*wine_get_wgl_driver)(PHYSDEV,UINT);
    0 // UINT       priority;
};

static
GDILOOBJTYPE
ConvertObjectType(
    WORD wType)
{
    /* Get the GDI object type */
    switch (wType)
    {
        case OBJ_PEN: return GDILoObjType_LO_PEN_TYPE;
        case OBJ_BRUSH: return GDILoObjType_LO_BRUSH_TYPE;
        case OBJ_DC: return GDILoObjType_LO_DC_TYPE;
        case OBJ_METADC: return GDILoObjType_LO_METADC16_TYPE;
        case OBJ_PAL: return GDILoObjType_LO_PALETTE_TYPE;
        case OBJ_FONT: return GDILoObjType_LO_FONT_TYPE;
        case OBJ_BITMAP: return GDILoObjType_LO_BITMAP_TYPE;
        case OBJ_REGION: return GDILoObjType_LO_REGION_TYPE;
        case OBJ_METAFILE: return GDILoObjType_LO_METAFILE16_TYPE;
        case OBJ_MEMDC: return GDILoObjType_LO_DC_TYPE;
        case OBJ_EXTPEN: return GDILoObjType_LO_EXTPEN_TYPE;
        case OBJ_ENHMETADC: return GDILoObjType_LO_ALTDC_TYPE;
        case OBJ_ENHMETAFILE: return GDILoObjType_LO_METAFILE_TYPE;
        case OBJ_COLORSPACE: return GDILoObjType_LO_ICMLCS_TYPE;
        default: return 0;
    }
}

HGDIOBJ
alloc_gdi_handle(
    PVOID pvObject,
    WORD wType,
    const struct gdi_obj_funcs *funcs)
{
    GDILOOBJTYPE eObjType;

    /* Get the GDI object type */
    eObjType = ConvertObjectType(wType);
    if ((eObjType != GDILoObjType_LO_METAFILE_TYPE) &&
        (eObjType != GDILoObjType_LO_METAFILE16_TYPE) &&
        (eObjType != GDILoObjType_LO_METADC16_TYPE))
    {
        /* This is not supported! */
        ASSERT(FALSE);
        return NULL;
    }

    /* Insert the client object */
    return GdiCreateClientObj(pvObject, eObjType);
}

PVOID
free_gdi_handle(HGDIOBJ hobj)
{
    /* Should be a client object */
    return GdiDeleteClientObj(hobj);
}

PVOID
GDI_GetObjPtr(
    HGDIOBJ hobj,
    WORD wType)
{
    GDILOOBJTYPE eObjType;

    /* Check if the object type matches */
    eObjType = ConvertObjectType(wType);
    if ((eObjType == 0) || (GDI_HANDLE_GET_TYPE(hobj) != eObjType))
    {
        return NULL;
    }

    /* Check if we have an ALTDC */
    if (eObjType == GDILoObjType_LO_ALTDC_TYPE)
    {
        /* Object is stored as LDC */
        return GdiGetLDC(hobj);
    }

    /* Check for client objects */
    if ((eObjType == GDILoObjType_LO_METAFILE_TYPE) ||
        (eObjType == GDILoObjType_LO_METAFILE16_TYPE) ||
        (eObjType == GDILoObjType_LO_METADC16_TYPE))
    {
        return GdiGetClientObjLink(hobj);
    }

    /* This should never happen! */
    ASSERT(FALSE);
    return NULL;
}

VOID
GDI_ReleaseObj(HGDIOBJ hobj)
{
    /* We don't do any reference-counting */
}

WINEDC*
alloc_dc_ptr(WORD magic)
{
    WINEDC* pWineDc;

    /* Allocate the Wine DC */
    pWineDc = HeapAlloc(GetProcessHeap(), 0, sizeof(*pWineDc));
    if (pWineDc == NULL)
    {
        return NULL;
    }

    ZeroMemory(pWineDc, sizeof(*pWineDc));
    pWineDc->refcount = 1;
    pWineDc->hFont = GetStockObject(SYSTEM_FONT);
    pWineDc->hBrush = GetStockObject(WHITE_BRUSH);
    pWineDc->hPen = GetStockObject(BLACK_PEN);
    pWineDc->hPalette = GetStockObject(DEFAULT_PALETTE);

    if (magic == OBJ_ENHMETADC)
    {
        /* We create a metafile DC, but we ignore the reference DC, this is
           handled by the wine code */
        pWineDc->hdc = NtGdiCreateMetafileDC(NULL);
        if (pWineDc->hdc == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pWineDc);
            return NULL;
        }

        /* Set the Wine DC as LDC */
        GdiSetLDC(pWineDc->hdc, pWineDc);
    }
    else if (magic == OBJ_METADC)
    {
        pWineDc->hdc = GdiCreateClientObj(pWineDc, GDILoObjType_LO_METADC16_TYPE);
        if (pWineDc->hdc == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pWineDc);
            return NULL;
        }
    }
    else
    {
        // nothing else supported!
        ASSERT(FALSE);
    }

    pWineDc->physDev = &pWineDc->NullPhysDev;
    pWineDc->NullPhysDev.funcs = &DummyPhysDevFuncs;
    pWineDc->NullPhysDev.next = NULL;

    pWineDc->NullPhysDev.hdc = pWineDc->hdc;
    return pWineDc;
}

VOID
free_dc_ptr(WINEDC* pWineDc)
{
    /* Invoke the DeleteDC callback to clean up the DC */
    pWineDc->physDev->funcs->pDeleteDC(pWineDc->physDev);

    /* FIXME */
    if (GDI_HANDLE_GET_TYPE(pWineDc->hdc) == GDILoObjType_LO_ALTDC_TYPE)
    {
        /* Get rid of the LDC */
        ASSERT((WINEDC*)GdiGetLDC(pWineDc->hdc) == pWineDc);
        GdiSetLDC(pWineDc->hdc, NULL);

        /* Free the DC */
        NtGdiDeleteObjectApp(pWineDc->hdc);
    }
    else if (GDI_HANDLE_GET_TYPE(pWineDc->hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        GdiDeleteClientObj(pWineDc->hdc);
    }

    /* Free the Wine DC */
    HeapFree(GetProcessHeap(), 0, pWineDc);
}

WINEDC*
get_dc_ptr(HDC hdc)
{
    /* Check for EMF DC */
    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_ALTDC_TYPE)
    {
        /* The Wine DC is stored as the LDC */
        return (WINEDC*)GdiGetLDC(hdc);
    }

    /* Check for METADC16 */
    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        return GdiGetClientObjLink(hdc);
    }

    return NULL;
}

VOID
release_dc_ptr(WINEDC* dc)
{
    /* We don't do any reference-counting */
}

void
push_dc_driver_ros(
    PHYSDEV *dev,
    PHYSDEV physdev,
    const struct gdi_dc_funcs *funcs)
{
    physdev->funcs = funcs;
    physdev->next = *dev;
    physdev->hdc = CONTAINING_RECORD(dev, WINEDC, physDev)->hdc;
    *dev = physdev;
}

VOID
GDI_hdc_using_object(
    HGDIOBJ hobj,
    HDC hdc)
{
    /* Record that we have an object in use by a METADC. We simply link the
       object to the HDC that we use. Wine API does not give us a way to
       respond to failure, so we silently ignore it */
    if (!GdiCreateClientObjLink(hobj, hdc))
    {
        /* Ignore failure, and return */
        DPRINT1("Failed to create link for selected METADC object.\n");
        return;
    }
}

VOID
GDI_hdc_not_using_object(
    HGDIOBJ hobj,
    HDC hdc)
{
    HDC hdcLink;

    /* Remove the HDC link for the object */
    hdcLink = GdiRemoveClientObjLink(hobj);
    ASSERT(hdcLink == hdc);
}

int
bitmap_info_size(
    const BITMAPINFO * info,
    WORD coloruse)
{
    __debugbreak();
    return 0;
}

BOOL
get_brush_bitmap_info(
    HBRUSH hbr,
    PBITMAPINFO pbmi,
    PVOID *ppvBits,
    PUINT puUsage)
{
    HBITMAP hbmp;
    HDC hdc;

    /* Call win32k to get the bitmap handle and color usage */
    hbmp = NtGdiGetObjectBitmapHandle(hbr, puUsage);
    if (hbmp == NULL)
        return FALSE;

    hdc = GetDC(NULL);
    if (hdc == NULL)
        return FALSE;

    /* Initialize the BITMAPINFO */
    ZeroMemory(pbmi, sizeof(*pbmi));
    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    /* Retrieve information about the bitmap */
    if (!GetDIBits(hdc, hbmp, 0, 0, NULL, pbmi, *puUsage))
        return FALSE;

    /* Now allocate a buffer for the bits */
    *ppvBits = HeapAlloc(GetProcessHeap(), 0, pbmi->bmiHeader.biSizeImage);
    if (*ppvBits == NULL)
        return FALSE;

    /* Retrieve the bitmap bits */
    if (!GetDIBits(hdc, hbmp, 0, pbmi->bmiHeader.biHeight, *ppvBits, pbmi, *puUsage))
    {
        HeapFree(GetProcessHeap(), 0, *ppvBits);
        *ppvBits = NULL;
        return FALSE;
    }

    /* GetDIBits doesn't set biClrUsed, but wine code needs it, so we set it */
    if (pbmi->bmiHeader.biBitCount <= 8)
    {
        pbmi->bmiHeader.biClrUsed =  1 << pbmi->bmiHeader.biBitCount;
    }

    return TRUE;
}

BOOL
WINAPI
SetVirtualResolution(
    HDC hdc,
    DWORD cxVirtualDevicePixel,
    DWORD cyVirtualDevicePixel,
    DWORD cxVirtualDeviceMm,
    DWORD cyVirtualDeviceMm)
{
    return NtGdiSetVirtualResolution(hdc,
                                     cxVirtualDevicePixel,
                                     cyVirtualDevicePixel,
                                     cxVirtualDeviceMm,
                                     cyVirtualDeviceMm);
}

BOOL
WINAPI
DeleteColorSpace(
    HCOLORSPACE hcs)
{
    return NtGdiDeleteColorSpace(hcs);
}

BOOL
WINAPI
SetWorldTransformForMetafile(
    _In_ HDC hdc,
    _Out_ CONST XFORM *pxform)
{
    if (GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
#if 0
        //HANDLE_METADC(BOOL, ModifyWorldTransform, FALSE, hdc, pxform, MWT_SET);
        /* Get the physdev */
        physdev = GetPhysDev(hdc);
        if (physdev == NULL)
        {
            DPRINT1("Failed to get physdev for meta DC %p\n", hdc);
            return FALSE;
        }

        physdev->funcs->pSetWorldTransform(physdev, pxform);
#endif
        // HACK!!!
        return TRUE;
    }

    return SetWorldTransform(hdc, pxform);
}

void
_assert (
    const char *exp,
    const char *file,
    unsigned line)
{
    DbgRaiseAssertionFailure();
}

#if (_MSC_VER < 1900) && (DBG != 1)
/* MSVC uses its own in this case. */
#else

double
__cdecl
atan2(
    double y,
    double x)
{
    __debugbreak();
    return 0.;
}

#endif

/******************************************************************************/

static
VOID
InitBitBltCoords(
    struct bitblt_coords *coords,
    HDC hdc,
    int x,
    int y,
    int cx,
    int cy)
{
    coords->log_x      = x;
    coords->log_y      = y;
    coords->log_width  = cx;
    coords->log_height = cy;
    coords->layout     = GetLayout(hdc);
}

static
PHYSDEV
GetPhysDev(
    HDC hdc)
{
    WINEDC *pWineDc;

    pWineDc = get_dc_ptr(hdc);
    if (pWineDc == NULL)
    {
        return NULL;
    }

    return pWineDc->physDev;
}

static
BOOL
DRIVER_PatBlt(
    _In_ PHYSDEV physdev,
    _In_ HDC hdc,
    _In_ INT xLeft,
    _In_ INT yTop,
    _In_ INT cx,
    _In_ INT cy,
    _In_ DWORD dwRop)
{
    struct bitblt_coords coords;

    InitBitBltCoords(&coords, hdc, xLeft, yTop, cx, cy);

    return physdev->funcs->pPatBlt(physdev, &coords, dwRop);
}

static
BOOL
DRIVER_StretchBlt(
    _In_ PHYSDEV physdev,
    _In_ HDC hdcDst,
    _In_ INT xDst,
    _In_ INT yDst,
    _In_ INT cxDst,
    _In_ INT cyDst,
    _In_opt_ HDC hdcSrc,
    _In_ INT xSrc,
    _In_ INT ySrc,
    _In_ INT cxSrc,
    _In_ INT cySrc,
    _In_ DWORD dwRop)
{
    struct bitblt_coords coordsDst, coordsSrc;
    struct gdi_physdev physdevSrc = {0};

    /* Source cannot be a metafile */
    if (GDI_HANDLE_GET_TYPE(hdcSrc) != GDILoObjType_LO_DC_TYPE)
        return FALSE;

    /* Source physdev uses only hdc and func */
    physdevSrc.hdc = hdcSrc;

    InitBitBltCoords(&coordsDst, hdcDst, xDst, yDst, cxDst, cyDst);
    InitBitBltCoords(&coordsSrc, hdcSrc, xSrc, ySrc, cxSrc, cySrc);

    return physdev->funcs->pStretchBlt(physdev, &coordsDst, &physdevSrc, &coordsSrc, dwRop);
}

static
BOOL
DRIVER_RestoreDC(PHYSDEV physdev, INT level)
{
    WINEDC *pWineDc = get_dc_ptr(physdev->hdc);

    if (GDI_HANDLE_GET_TYPE(physdev->hdc) == GDILoObjType_LO_ALTDC_TYPE)
    {
        /* The Restore DC function needs the save level to be set correctly.
           Note that wine's level is 0 based, while our's is (like win) 1 based. */
        pWineDc->saveLevel = GetDCDWord(physdev->hdc, GdiGetEMFRestorDc, 0)  - 1;

        /* Fail if the level is not valid */
        if ((abs(level) > pWineDc->saveLevel) || (level == 0))
            return FALSE;
    }

    return physdev->funcs->pRestoreDC(physdev,level);
}

static
HFONT
DRIVER_SelectFont(PHYSDEV physdev, HFONT hFont, UINT *aa_flags)
{
    WINEDC *pWineDc = get_dc_ptr(physdev->hdc);
    HFONT hOldFont;

    if (!physdev->funcs->pSelectFont(physdev, hFont, aa_flags))
        return 0;

    hOldFont = pWineDc->hFont;
    pWineDc->hFont = hFont;
    return hOldFont;
}

static
HPEN
DRIVER_SelectPen(PHYSDEV physdev, HPEN hpen, const struct brush_pattern *pattern)
{
    WINEDC *pWineDc = get_dc_ptr(physdev->hdc);
    HPEN hOldPen;

    if (!physdev->funcs->pSelectPen(physdev, hpen, pattern))
        return 0;

    hOldPen = pWineDc->hPen;
    pWineDc->hPen = hpen;
    return hOldPen;
}

static
HBRUSH
DRIVER_SelectBrush(PHYSDEV physdev, HBRUSH hbrush, const struct brush_pattern *pattern)
{
    WINEDC *pWineDc = get_dc_ptr(physdev->hdc);
    HBRUSH hOldBrush;

    if (!physdev->funcs->pSelectBrush(physdev, hbrush, pattern))
        return 0;

    hOldBrush = pWineDc->hBrush;
    pWineDc->hBrush = hbrush;
    return hOldBrush;
}

static
DWORD_PTR
DRIVER_Dispatch(
    _In_ PHYSDEV physdev,
    _In_ DCFUNC eFunction,
    _In_ va_list argptr)
{
    UINT aa_flags = 0;

/* Note that this is a hack that relies on some assumptions regarding the
   Windows ABI. It relies on the fact that all vararg functions put their
   parameters on the stack in the correct order. Additionally it relies
   on the fact that none of the functions we handle here, pass any 64
   bit arguments on a 32 bit architecture. */
#define _va_arg_n(p,t,i) (*(t*)((intptr_t*)(p) + i))

    switch (eFunction)
    {
        case DCFUNC_AbortPath:
            return physdev->funcs->pAbortPath(physdev);
        case DCFUNC_Arc:
            return physdev->funcs->pArc(physdev,
                                        _va_arg_n(argptr, INT, 0), // left
                                        _va_arg_n(argptr, INT, 1), // top
                                        _va_arg_n(argptr, INT, 2), // right
                                        _va_arg_n(argptr, INT, 3), // bottom
                                        _va_arg_n(argptr, INT, 4), // xstart
                                        _va_arg_n(argptr, INT, 5), // ystart
                                        _va_arg_n(argptr, INT, 6), // xend
                                        _va_arg_n(argptr, INT, 7)); // yend
        case DCFUNC_BeginPath:
            return physdev->funcs->pBeginPath(physdev);
        case DCFUNC_Chord:
            return physdev->funcs->pChord(physdev,
                                          _va_arg_n(argptr, INT, 0),
                                          _va_arg_n(argptr, INT, 1),
                                          _va_arg_n(argptr, INT, 2),
                                          _va_arg_n(argptr, INT, 3),
                                          _va_arg_n(argptr, INT, 4),
                                          _va_arg_n(argptr, INT, 5),
                                          _va_arg_n(argptr, INT, 6),
                                          _va_arg_n(argptr, INT, 7));
        case DCFUNC_CloseFigure:
            return physdev->funcs->pCloseFigure(physdev);
        case DCFUNC_Ellipse:
            return physdev->funcs->pEllipse(physdev,
                                            _va_arg_n(argptr, INT, 0),
                                            _va_arg_n(argptr, INT, 1),
                                            _va_arg_n(argptr, INT, 2),
                                            _va_arg_n(argptr, INT, 3));
        case DCFUNC_EndPath:
            return physdev->funcs->pEndPath(physdev);
        case DCFUNC_ExcludeClipRect:
            return physdev->funcs->pExcludeClipRect(physdev,
                                              _va_arg_n(argptr, INT, 0),
                                              _va_arg_n(argptr, INT, 1),
                                              _va_arg_n(argptr, INT, 2),
                                              _va_arg_n(argptr, INT, 3));
        case DCFUNC_ExtEscape:
            ASSERT(physdev->funcs->pExtEscape != NULL);
            return physdev->funcs->pExtEscape(physdev,
                                              _va_arg_n(argptr, INT, 0),
                                              _va_arg_n(argptr, INT, 1),
                                              _va_arg_n(argptr, LPCVOID, 2),
                                              _va_arg_n(argptr, INT, 3),
                                              _va_arg_n(argptr, LPVOID, 4));
        case DCFUNC_ExtFloodFill:
            return physdev->funcs->pExtFloodFill(physdev,
                                                 _va_arg_n(argptr, INT, 0),
                                                 _va_arg_n(argptr, INT, 1),
                                                 _va_arg_n(argptr, COLORREF, 2),
                                                 _va_arg_n(argptr, UINT, 3));
        case DCFUNC_ExtSelectClipRgn:
            return physdev->funcs->pExtSelectClipRgn(physdev,
                                                     _va_arg_n(argptr, HRGN, 0), // hrgn
                                                     _va_arg_n(argptr, INT, 1)); // iMode
        case DCFUNC_ExtTextOut:
            return physdev->funcs->pExtTextOut(physdev,
                                               _va_arg_n(argptr, INT, 0),// x
                                               _va_arg_n(argptr, INT, 1),// y
                                               _va_arg_n(argptr, UINT, 2),// fuOptions
                                               _va_arg_n(argptr, const RECT *, 3),// lprc,
                                               _va_arg_n(argptr, LPCWSTR, 4),// lpString,
                                               _va_arg_n(argptr, UINT, 5),// cchString,
                                               _va_arg_n(argptr, const INT *, 6));// lpDx);
        case DCFUNC_FillPath:
            return physdev->funcs->pFillPath(physdev);
        case DCFUNC_FillRgn:
            return physdev->funcs->pFillRgn(physdev,
                                            _va_arg_n(argptr, HRGN, 0),
                                            _va_arg_n(argptr, HBRUSH, 1));
        case DCFUNC_FlattenPath:
            return physdev->funcs->pFlattenPath(physdev);
        case DCFUNC_FrameRgn:
            return physdev->funcs->pFrameRgn(physdev,
                                             _va_arg_n(argptr, HRGN, 0),
                                             _va_arg_n(argptr, HBRUSH, 1),
                                             _va_arg_n(argptr, INT, 2),
                                             _va_arg_n(argptr, INT, 3));
        case DCFUNC_GetDeviceCaps:
            return physdev->funcs->pGetDeviceCaps(physdev, va_arg(argptr, INT));
        case DCFUNC_GdiComment:
            return physdev->funcs->pGdiComment(physdev,
                                               _va_arg_n(argptr, UINT, 0),
                                               _va_arg_n(argptr, const BYTE*, 1));
        case DCFUNC_IntersectClipRect:
            return physdev->funcs->pIntersectClipRect(physdev,
                                                      _va_arg_n(argptr, INT, 0),
                                                      _va_arg_n(argptr, INT, 1),
                                                      _va_arg_n(argptr, INT, 2),
                                                      _va_arg_n(argptr, INT, 3));
        case DCFUNC_InvertRgn:
            return physdev->funcs->pInvertRgn(physdev,
                                              va_arg(argptr, HRGN));
        case DCFUNC_LineTo:
            return physdev->funcs->pLineTo(physdev,
                                           _va_arg_n(argptr, INT, 0),
                                           _va_arg_n(argptr, INT, 1));
        case DCFUNC_ModifyWorldTransform:
            return physdev->funcs->pModifyWorldTransform(physdev,
                                                         _va_arg_n(argptr, const XFORM*, 0),
                                                         _va_arg_n(argptr, DWORD, 1));
        case DCFUNC_MoveTo:
            return physdev->funcs->pMoveTo(physdev,
                                           _va_arg_n(argptr, INT, 0),
                                           _va_arg_n(argptr, INT, 1));
        case DCFUNC_OffsetClipRgn:
            return physdev->funcs->pOffsetClipRgn(physdev,
                                                  _va_arg_n(argptr, INT, 0), // hrgn
                                                  _va_arg_n(argptr, INT, 1)); // iMode
        case DCFUNC_OffsetViewportOrgEx:
            return physdev->funcs->pOffsetViewportOrgEx(physdev,
                                                        _va_arg_n(argptr, INT, 0), // X
                                                        _va_arg_n(argptr, INT, 1), // Y
                                                        _va_arg_n(argptr, LPPOINT, 2)); // lpPoint
        case DCFUNC_OffsetWindowOrgEx:
            return physdev->funcs->pOffsetWindowOrgEx(physdev,
                                                        _va_arg_n(argptr, INT, 0), // X
                                                        _va_arg_n(argptr, INT, 1), // Y
                                                        _va_arg_n(argptr, LPPOINT, 2)); // lpPoint
        case DCFUNC_PatBlt:
            return DRIVER_PatBlt(physdev,
                                 physdev->hdc,
                                 _va_arg_n(argptr, INT, 0),
                                 _va_arg_n(argptr, INT, 1),
                                 _va_arg_n(argptr, INT, 2),
                                 _va_arg_n(argptr, INT, 3),
                                 _va_arg_n(argptr, DWORD, 4));
        case DCFUNC_Pie:
            return physdev->funcs->pPie(physdev,
                                        _va_arg_n(argptr, INT, 0),
                                        _va_arg_n(argptr, INT, 1),
                                        _va_arg_n(argptr, INT, 2),
                                        _va_arg_n(argptr, INT, 3),
                                        _va_arg_n(argptr, INT, 4),
                                        _va_arg_n(argptr, INT, 5),
                                        _va_arg_n(argptr, INT, 6),
                                        _va_arg_n(argptr, INT, 7));
        case DCFUNC_PolyBezier:
            return physdev->funcs->pPolyBezier(physdev,
                                               _va_arg_n(argptr, const POINT*, 0),
                                               _va_arg_n(argptr, DWORD, 1));
        case DCFUNC_PolyBezierTo:
            return physdev->funcs->pPolyBezierTo(physdev,
                                                 _va_arg_n(argptr, const POINT*, 0),
                                                 _va_arg_n(argptr, DWORD, 1));
        case DCFUNC_PolyDraw:
            DPRINT1("DCFUNC_PolyDraw not implemented\n");;
            return FALSE;
            return physdev->funcs->pPolyDraw(physdev,
                                             _va_arg_n(argptr, const POINT*, 1),
                                             _va_arg_n(argptr, const BYTE*, 1),
                                             _va_arg_n(argptr, DWORD, 2));
        case DCFUNC_Polygon:
            return physdev->funcs->pPolygon(physdev,
                                            _va_arg_n(argptr, const POINT*, 0),
                                            _va_arg_n(argptr, INT, 1));
        case DCFUNC_Polyline:
            return physdev->funcs->pPolyline(physdev,
                                             _va_arg_n(argptr, const POINT*, 0),
                                             _va_arg_n(argptr, INT, 1));
        case DCFUNC_PolylineTo:
            DPRINT1("DCFUNC_PolylineTo not implemented\n");;
            return FALSE;
            return physdev->funcs->pPolylineTo(physdev,
                                               _va_arg_n(argptr, const POINT*, 0),
                                               _va_arg_n(argptr, INT, 1));
        case DCFUNC_PolyPolygon:
            return physdev->funcs->pPolyPolygon(physdev,
                                                _va_arg_n(argptr, const POINT*, 0),
                                                _va_arg_n(argptr, const INT*, 1),
                                                _va_arg_n(argptr, DWORD, 2));
        case DCFUNC_PolyPolyline:
            return physdev->funcs->pPolyPolyline(physdev,
                                                 _va_arg_n(argptr, const POINT*, 0),
                                                 _va_arg_n(argptr, const DWORD*, 1),
                                                 _va_arg_n(argptr, DWORD, 2));
        case DCFUNC_RealizePalette:
            if (GDI_HANDLE_GET_TYPE(physdev->hdc) != GDILoObjType_LO_METADC16_TYPE)
            {
                UNIMPLEMENTED;
                return GDI_ERROR;
            }
            return physdev->funcs->pRealizePalette(physdev, NULL, FALSE);
        case DCFUNC_Rectangle:
            return physdev->funcs->pRectangle(physdev,
                                              _va_arg_n(argptr, INT, 0),
                                              _va_arg_n(argptr, INT, 1),
                                              _va_arg_n(argptr, INT, 2),
                                              _va_arg_n(argptr, INT, 3));
        case DCFUNC_RestoreDC:
            return DRIVER_RestoreDC(physdev, va_arg(argptr, INT));
        case DCFUNC_RoundRect:
            return physdev->funcs->pRoundRect(physdev,
                                              _va_arg_n(argptr, INT, 0),
                                              _va_arg_n(argptr, INT, 1),
                                              _va_arg_n(argptr, INT, 2),
                                              _va_arg_n(argptr, INT, 3),
                                              _va_arg_n(argptr, INT, 4),
                                              _va_arg_n(argptr, INT, 5));

        case DCFUNC_SaveDC:
            return physdev->funcs->pSaveDC(physdev);
        case DCFUNC_ScaleViewportExtEx:
            return physdev->funcs->pScaleViewportExtEx(physdev,
                                                     _va_arg_n(argptr, INT, 0), // xNum
                                                     _va_arg_n(argptr, INT, 1), // xDenom
                                                     _va_arg_n(argptr, INT, 2), // yNum
                                                     _va_arg_n(argptr, INT, 3), // yDenom
                                                     _va_arg_n(argptr, LPSIZE, 4)); // lpSize
        case DCFUNC_ScaleWindowExtEx:
            return physdev->funcs->pScaleWindowExtEx(physdev,
                                                     _va_arg_n(argptr, INT, 0), // xNum
                                                     _va_arg_n(argptr, INT, 1), // xDenom
                                                     _va_arg_n(argptr, INT, 2), // yNum
                                                     _va_arg_n(argptr, INT, 3), // yDenom
                                                     _va_arg_n(argptr, LPSIZE, 4)); // lpSize
        case DCFUNC_SelectBrush:
            return (DWORD_PTR)DRIVER_SelectBrush(physdev, va_arg(argptr, HBRUSH), NULL);
        case DCFUNC_SelectClipPath:
            return physdev->funcs->pSelectClipPath(physdev, va_arg(argptr, INT));
        case DCFUNC_SelectFont:
            return (DWORD_PTR)DRIVER_SelectFont(physdev, va_arg(argptr, HFONT), &aa_flags);
        case DCFUNC_SelectPalette:
            return (DWORD_PTR)physdev->funcs->pSelectPalette(physdev,
                                                  _va_arg_n(argptr, HPALETTE, 0),
                                                  _va_arg_n(argptr, BOOL, 1));
        case DCFUNC_SelectPen:
            return (DWORD_PTR)DRIVER_SelectPen(physdev, va_arg(argptr, HPEN), NULL);
        case DCFUNC_SetDCBrushColor:
            return physdev->funcs->pSetDCBrushColor(physdev, va_arg(argptr, COLORREF));
        case DCFUNC_SetDCPenColor:
            return physdev->funcs->pSetDCPenColor(physdev, va_arg(argptr, COLORREF));
        case DCFUNC_SetDIBitsToDevice:
            return physdev->funcs->pSetDIBitsToDevice(physdev,
                                                      _va_arg_n(argptr, INT, 0),
                                                      _va_arg_n(argptr, INT, 1),
                                                      _va_arg_n(argptr, DWORD, 2),
                                                      _va_arg_n(argptr, DWORD, 3),
                                                      _va_arg_n(argptr, INT, 4),
                                                      _va_arg_n(argptr, INT, 5),
                                                      _va_arg_n(argptr, UINT, 6),
                                                      _va_arg_n(argptr, UINT, 7),
                                                      _va_arg_n(argptr, LPCVOID, 8),
                                                      _va_arg_n(argptr, BITMAPINFO*, 9),
                                                      _va_arg_n(argptr, UINT, 10));
        case DCFUNC_SetBkColor:
            return physdev->funcs->pSetBkColor(physdev, va_arg(argptr, COLORREF));
        case DCFUNC_SetBkMode:
            return physdev->funcs->pSetBkMode(physdev, va_arg(argptr, INT));
        case DCFUNC_SetLayout:
            // FIXME: MF16 is UNIMPLEMENTED
            return physdev->funcs->pSetLayout(physdev,
                                              _va_arg_n(argptr, DWORD, 0));
        //case DCFUNC_SetMapMode:
        //    return physdev->funcs->pSetMapMode(physdev, va_arg(argptr, INT));
        case DCFUNC_SetPixel:
            return physdev->funcs->pSetPixel(physdev,
                                             _va_arg_n(argptr, INT, 0),
                                             _va_arg_n(argptr, INT, 1),
                                             _va_arg_n(argptr, COLORREF, 2));
        case DCFUNC_SetPolyFillMode:
            return physdev->funcs->pSetPolyFillMode(physdev, va_arg(argptr, INT));
        case DCFUNC_SetROP2:
            return physdev->funcs->pSetROP2(physdev, va_arg(argptr, INT));
        case DCFUNC_SetStretchBltMode:
            return physdev->funcs->pSetStretchBltMode(physdev, va_arg(argptr, INT));
        case DCFUNC_SetTextAlign:
            return physdev->funcs->pSetTextAlign(physdev, va_arg(argptr, UINT));
        case DCFUNC_SetTextCharacterExtra:
            return physdev->funcs->pSetTextCharacterExtra(physdev, va_arg(argptr, INT));
        case DCFUNC_SetTextColor:
            return physdev->funcs->pSetTextColor(physdev, va_arg(argptr, COLORREF));
        case DCFUNC_SetTextJustification:
            return physdev->funcs->pSetTextJustification(physdev,
                                                         _va_arg_n(argptr, INT, 0),
                                                         _va_arg_n(argptr, INT, 1));
        case DCFUNC_SetViewportExtEx:
            return physdev->funcs->pSetViewportExtEx(physdev,
                                                     _va_arg_n(argptr, INT, 0), // nXExtent
                                                     _va_arg_n(argptr, INT, 1), // nYExtent
                                                     _va_arg_n(argptr, LPSIZE, 2)); // lpSize
        case DCFUNC_SetViewportOrgEx:
            return physdev->funcs->pSetViewportOrgEx(physdev,
                                                     _va_arg_n(argptr, INT, 0), // X
                                                     _va_arg_n(argptr, INT, 1), // Y
                                                     _va_arg_n(argptr, LPPOINT, 2)); // lpPoint
        case DCFUNC_SetWindowExtEx:
            return physdev->funcs->pSetWindowExtEx(physdev,
                                                   _va_arg_n(argptr, INT, 0), // nXExtent
                                                   _va_arg_n(argptr, INT, 1), // nYExtent
                                                   _va_arg_n(argptr, LPSIZE, 2)); // lpSize
        case DCFUNC_SetWindowOrgEx:
            return physdev->funcs->pSetWindowOrgEx(physdev,
                                                   _va_arg_n(argptr, INT, 0), // X
                                                   _va_arg_n(argptr, INT, 1), // Y
                                                   _va_arg_n(argptr, LPPOINT, 2)); // lpPoint
        case DCFUNC_StretchBlt:
            return DRIVER_StretchBlt(physdev,
                                     physdev->hdc,
                                     _va_arg_n(argptr, INT, 0),
                                     _va_arg_n(argptr, INT, 1),
                                     _va_arg_n(argptr, INT, 2),
                                     _va_arg_n(argptr, INT, 3),
                                     _va_arg_n(argptr, HDC, 4),
                                     _va_arg_n(argptr, INT, 5),
                                     _va_arg_n(argptr, INT, 6),
                                     _va_arg_n(argptr, INT, 7),
                                     _va_arg_n(argptr, INT, 8),
                                     _va_arg_n(argptr, DWORD, 9));
        case DCFUNC_StrokeAndFillPath:
            return physdev->funcs->pStrokeAndFillPath(physdev);
        case DCFUNC_StrokePath:
            return physdev->funcs->pStrokePath(physdev);
        case DCFUNC_WidenPath:
            return physdev->funcs->pWidenPath(physdev);


        /* These are not implemented in wine */
        case DCFUNC_AlphaBlend:
        case DCFUNC_AngleArc:
        case DCFUNC_ArcTo:
        case DCFUNC_GradientFill:
        case DCFUNC_MaskBlt:
        case DCFUNC_PathToRegion:
        case DCFUNC_PlgBlt:
        case DCFUNC_TransparentBlt:
            UNIMPLEMENTED;
            return 0;

        default:
            __debugbreak();
            return 0;
    }
}

BOOL
METADC_Dispatch(
    _In_ DCFUNC eFunction,
    _Out_ PDWORD_PTR pdwResult,
    _In_ DWORD_PTR dwError,
    _In_ HDC hdc,
    ...)
{
    PHYSDEV physdev;
    va_list argptr;

    /* Handle only METADC16 and ALTDC */
    if ((GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_ALTDC_TYPE) &&
        (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_METADC16_TYPE))
    {
        /* Let the caller handle it */
        return FALSE;
    }

    physdev = GetPhysDev(hdc);
    if (physdev == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        *pdwResult = dwError;
        return TRUE;
    }

    va_start(argptr, hdc);
    *pdwResult = DRIVER_Dispatch(physdev, eFunction, argptr);
    va_end(argptr);

    /* Return TRUE to indicate that we want to return from the parent  */
    return ((GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) ||
            (*pdwResult == dwError));
}

BOOL
WINAPI
METADC_GetAndSetDCDWord(
    _Out_ DWORD* pdwResult,
    _In_ HDC hdc,
    _In_ UINT uFunction,
    _In_ DWORD dwIn,
    _In_ ULONG ulMFId,
    _In_ USHORT usMF16Id,
    _In_ DWORD dwError)
{
    PHYSDEV physdev;

    /* Ignore these, we let wine code handle this */
    UNREFERENCED_PARAMETER(ulMFId);
    UNREFERENCED_PARAMETER(usMF16Id);

    physdev = GetPhysDev(hdc);
    if (physdev == NULL)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        *pdwResult = dwError;
        return TRUE;
    }

    /* Check the function */
    switch (uFunction)
    {
        case GdiGetSetMapMode:
            *pdwResult = physdev->funcs->pSetMapMode(physdev, dwIn);
            break;

        case GdiGetSetArcDirection:
            if (GDI_HANDLE_GET_TYPE(physdev->hdc) == GDILoObjType_LO_METADC16_TYPE)
                pdwResult = 0;
            else
                *pdwResult = physdev->funcs->pSetArcDirection(physdev, dwIn);
            break;

        case GdiGetSetRelAbs:
            if (GDI_HANDLE_GET_TYPE(physdev->hdc) == GDILoObjType_LO_METADC16_TYPE)
                *pdwResult = physdev->funcs->pSetRelAbs(physdev, dwIn);
            else
            {
                UNIMPLEMENTED;
                *pdwResult = 0;
            }
            break;


        default:
            __debugbreak();
    }

    /* Return TRUE to indicate that we want to return from the parent  */
    return ((GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) ||
            (*pdwResult == dwError));
}

VOID
WINAPI
METADC_DeleteObject(HGDIOBJ hobj)
{
    GDILOOBJTYPE eObjectType;
    HDC hdc;
    PHYSDEV physdev;

    /* Check for one of the types we actually handle here */
    eObjectType = GDI_HANDLE_GET_TYPE(hobj);
    if ((eObjectType != GDILoObjType_LO_BRUSH_TYPE) &&
        (eObjectType != GDILoObjType_LO_PEN_TYPE) &&
        (eObjectType != GDILoObjType_LO_EXTPEN_TYPE) &&
        (eObjectType != GDILoObjType_LO_PALETTE_TYPE) &&
        (eObjectType != GDILoObjType_LO_FONT_TYPE))
    {
        return;
    }

    /* Check if we have a client object link and remove it if it was found.
       The link is the HDC that the object was selected into. */
    hdc = GdiRemoveClientObjLink(hobj);
    if (hdc == NULL)
    {
        /* The link was not found, so we are not handling this object here */
        return;
    }

    /* Get the physdev */
    physdev = GetPhysDev(hdc);
    if (physdev == NULL)
    {
        /* This happens, when the METADC is already closed, when we delete
           the object. Simply ignore it */
        DPRINT1("METADC was already closed, cannot delete object. Ignoring.\n");
        return;
    }

    physdev->funcs->pDeleteObject(physdev, hobj);
}

BOOL
WINAPI
METADC_DeleteDC(
    _In_ HDC hdc)
{
    /* Only ALTDCs are supported */
    if (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_ALTDC_TYPE)
    {
        DPRINT1("Trying to delete METADC %p\n", hdc);
        return FALSE;
    }
    // FIXME call the driver?
    return NtGdiDeleteObjectApp(hdc);
}

INT
WINAPI
METADC16_Escape(
    _In_ HDC hdc,
    _In_ INT nEscape,
    _In_ INT cbInput,
    _In_ LPCSTR lpvInData,
    _Out_ LPVOID lpvOutData)
{
    DWORD_PTR dwResult;

    /* Do not record MFCOMMENT */
    if (nEscape == MFCOMMENT)
    {
        // HACK required by wine code...
        //return 1;
    }

    METADC_Dispatch(DCFUNC_ExtEscape,
                    &dwResult,
                    SP_ERROR,
                    hdc,
                    nEscape,
                    cbInput,
                    lpvInData,
                    lpvOutData);

    return (INT)dwResult;
}
