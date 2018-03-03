
#include <precomp.h>
#include "gdi_private.h"
#undef SetWorldTransform

#define NDEBUG
#include <debug.h>

WINEDC *get_nulldrv_dc( PHYSDEV dev );

BOOL nulldrv_BeginPath( PHYSDEV dev );
BOOL nulldrv_EndPath( PHYSDEV dev );
BOOL nulldrv_AbortPath( PHYSDEV dev );
BOOL nulldrv_CloseFigure( PHYSDEV dev );
BOOL nulldrv_SelectClipPath( PHYSDEV dev, INT mode );
BOOL nulldrv_FillPath( PHYSDEV dev );
BOOL nulldrv_StrokeAndFillPath( PHYSDEV dev );
BOOL nulldrv_StrokePath( PHYSDEV dev );
BOOL nulldrv_FlattenPath( PHYSDEV dev );
BOOL nulldrv_WidenPath( PHYSDEV dev );

static INT i = 0;

static
INT_PTR
NULL_Unused()
{
    DPRINT1("NULL_Unused %d\n",i);
    // __debugbreak();
    return 0;
}

static INT NULL_SetMapMode(PHYSDEV dev, INT iMode)
{
    WINEDC* pWineDc = get_dc_ptr(dev->hdc);

    if(!pWineDc)
        return 0;

    if (GDI_HANDLE_GET_TYPE(dev->hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        INT ret = pWineDc->MapMode;
        pWineDc->MapMode = iMode;
        return ret;
    }

    /* Do not fail to let regular GDI32 go on with the call */
    return 1;
}

static INT NULL_SetRelAbs(PHYSDEV dev, INT iMode)
{
    /* Do not fail to let regular GDI32 go on with the call */
    return 1;
}

static INT   NULL_ExtSelectClipRgn(PHYSDEV dev, HRGN hrgn, INT iMode)
{
    return NtGdiExtSelectClipRgn(dev->hdc, hrgn, iMode);
}

static INT   NULL_SaveDC(PHYSDEV dev) { return 1; }
static BOOL  NULL_RestoreDC(PHYSDEV dev, INT level) { return TRUE; }
static HFONT NULL_SelectFont(PHYSDEV dev, HFONT hFont, UINT *aa_flags) { return NULL; }
static INT   NULL_SetArcDirection(PHYSDEV dev, INT iMode) { return 0; }
static BOOL  NULL_SetWindowExtEx(PHYSDEV dev, INT cx, INT cy, SIZE *size) { return TRUE; }
static BOOL  NULL_SetViewportExtEx(PHYSDEV dev, INT cx, INT cy, SIZE *size) { return TRUE; }
static BOOL  NULL_SetWindowOrgEx(PHYSDEV dev, INT x, INT y, POINT *pt) { return TRUE; }
static BOOL  NULL_SetViewportOrgEx(PHYSDEV dev, INT x, INT y, POINT *pt) { return TRUE; }
static INT   NULL_IntersectClipRect(PHYSDEV dev, INT left, INT top, INT right, INT bottom) { return 1; }
static INT   NULL_OffsetClipRgn(PHYSDEV dev, INT x, INT y) { return SIMPLEREGION; }
static INT   NULL_ExcludeClipRect(PHYSDEV dev, INT left, INT top, INT right, INT bottom) { return 1; }
static BOOL  NULL_ExtTextOutW(PHYSDEV dev, INT x, INT y, UINT fuOptions, const RECT *lprc, LPCWSTR lpString, UINT cwc, const INT *lpDx) { return TRUE; }
static BOOL  NULL_ModifyWorldTransform( PHYSDEV dev, const XFORM* xform, DWORD mode ) { return TRUE; }
static BOOL  NULL_SetWorldTransform( PHYSDEV dev, const XFORM* xform ) { return TRUE; }
static BOOL  NULL_PolyPolyline(PHYSDEV dev, const POINT *pt, const DWORD *lpt, DWORD cw) { return TRUE; }

static const struct gdi_dc_funcs DummyPhysDevFuncs =
{
    (PVOID)NULL_Unused, //INT      (*pAbortDoc)(PHYSDEV);
    nulldrv_AbortPath,  //BOOL     (*pAbortPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pAlphaBlend)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,BLENDFUNCTION);
    (PVOID)NULL_Unused, //BOOL     (*pAngleArc)(PHYSDEV,INT,INT,DWORD,FLOAT,FLOAT);
    (PVOID)NULL_Unused, //BOOL     (*pArc)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pArcTo)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    nulldrv_BeginPath,  //BOOL     (*pBeginPath)(PHYSDEV);
    (PVOID)NULL_Unused, //DWORD    (*pBlendImage)(PHYSDEV,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,BLENDFUNCTION);
    (PVOID)NULL_Unused, //BOOL     (*pChord)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT);
    nulldrv_CloseFigure, //BOOL     (*pCloseFigure)(PHYSDEV);

    (PVOID)NULL_Unused, //BOOL     (*pCreateCompatibleDC)(PHYSDEV,PHYSDEV*);
    (PVOID)NULL_Unused, //BOOL     (*pCreateDC)(PHYSDEV*,LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODEW*);
    (PVOID)NULL_Unused, //BOOL     (*pDeleteDC)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pDeleteObject)(PHYSDEV,HGDIOBJ);
    (PVOID)NULL_Unused, //DWORD    (*pDeviceCapabilities)(LPSTR,LPCSTR,LPCSTR,WORD,LPSTR,LPDEVMODEA);
    (PVOID)NULL_Unused, //BOOL     (*pEllipse)(PHYSDEV,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //INT      (*pEndDoc)(PHYSDEV);
    (PVOID)NULL_Unused, //INT      (*pEndPage)(PHYSDEV);
    nulldrv_EndPath,    //BOOL     (*pEndPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pEnumFonts)(PHYSDEV,LPLOGFONTW,FONTENUMPROCW,LPARAM);

    (PVOID)NULL_Unused, //INT      (*pEnumICMProfiles)(PHYSDEV,ICMENUMPROCW,LPARAM);
    NULL_ExcludeClipRect, //INT      (*pExcludeClipRect)(PHYSDEV,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //INT      (*pExtDeviceMode)(LPSTR,HWND,LPDEVMODEA,LPSTR,LPSTR,LPDEVMODEA,LPSTR,DWORD);
    (PVOID)NULL_Unused, //INT      (*pExtEscape)(PHYSDEV,INT,INT,LPCVOID,INT,LPVOID);
    (PVOID)NULL_Unused, //BOOL     (*pExtFloodFill)(PHYSDEV,INT,INT,COLORREF,UINT);
    NULL_ExtSelectClipRgn, //INT      (*pExtSelectClipRgn)(PHYSDEV,HRGN,INT);
    NULL_ExtTextOutW, //BOOL     (*pExtTextOut)(PHYSDEV,INT,INT,UINT,const RECT*,LPCWSTR,UINT,const INT*);
    nulldrv_FillPath,   //BOOL     (*pFillPath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pFillRgn)(PHYSDEV,HRGN,HBRUSH);
    nulldrv_FlattenPath, //BOOL     (*pFlattenPath)(PHYSDEV);

    (PVOID)NULL_Unused, //BOOL     (*pFontIsLinked)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pFrameRgn)(PHYSDEV,HRGN,HBRUSH,INT,INT);
    (PVOID)NULL_Unused, //BOOL     (*pGdiComment)(PHYSDEV,UINT,const BYTE*);
    (PVOID)NULL_Unused, //UINT     (*pGetBoundsRect)(PHYSDEV,RECT*,UINT);
    (PVOID)NULL_Unused, //BOOL     (*pGetCharABCWidths)(PHYSDEV,UINT,UINT,LPABC);
    (PVOID)NULL_Unused, //BOOL     (*pGetCharABCWidthsI)(PHYSDEV,UINT,UINT,WORD*,LPABC);
    (PVOID)NULL_Unused, //BOOL     (*pGetCharWidth)(PHYSDEV,UINT,UINT,LPINT);
    (PVOID)NULL_Unused, //INT      (*pGetDeviceCaps)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //BOOL     (*pGetDeviceGammaRamp)(PHYSDEV,LPVOID);
    (PVOID)NULL_Unused, //DWORD    (*pGetFontData)(PHYSDEV,DWORD,DWORD,LPVOID,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pGetFontRealizationInfo)(PHYSDEV,void*);
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
    NULL_ModifyWorldTransform, //BOOL     (*pModifyWorldTransform)(PHYSDEV,const XFORM*,DWORD);
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
    NULL_PolyPolyline,  //BOOL     (*pPolyPolyline)(PHYSDEV,const POINT*,const DWORD*,DWORD);
    (PVOID)NULL_Unused, //BOOL     (*pPolygon)(PHYSDEV,const POINT*,INT);
    (PVOID)NULL_Unused, //BOOL     (*pPolyline)(PHYSDEV,const POINT*,INT);
    (PVOID)NULL_Unused, //BOOL     (*pPolylineTo)(PHYSDEV,const POINT*,INT);
    (PVOID)NULL_Unused, //DWORD    (*pPutImage)(PHYSDEV,HRGN,BITMAPINFO*,const struct gdi_image_bits*,struct bitblt_coords*,struct bitblt_coords*,DWORD);
    (PVOID)NULL_Unused, //UINT     (*pRealizeDefaultPalette)(PHYSDEV);
    (PVOID)NULL_Unused, //UINT     (*pRealizePalette)(PHYSDEV,HPALETTE,BOOL);
    (PVOID)NULL_Unused, //BOOL     (*pRectangle)(PHYSDEV,INT,INT,INT,INT);
    (PVOID)NULL_Unused, //HDC      (*pResetDC)(PHYSDEV,const DEVMODEW*);
    NULL_RestoreDC,     //BOOL     (*pRestoreDC)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //BOOL     (*pRoundRect)(PHYSDEV,INT,INT,INT,INT,INT,INT);
    NULL_SaveDC,        //INT      (*pSaveDC)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pScaleViewportExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    (PVOID)NULL_Unused, //BOOL     (*pScaleWindowExtEx)(PHYSDEV,INT,INT,INT,INT,SIZE*);
    (PVOID)NULL_Unused, //HBITMAP  (*pSelectBitmap)(PHYSDEV,HBITMAP);
    (PVOID)NULL_Unused, //HBRUSH   (*pSelectBrush)(PHYSDEV,HBRUSH,const struct brush_pattern*);
    nulldrv_SelectClipPath, //BOOL     (*pSelectClipPath)(PHYSDEV,INT);
    NULL_SelectFont,    //HFONT    (*pSelectFont)(PHYSDEV,HFONT,UINT*);
    (PVOID)NULL_Unused, //HPALETTE (*pSelectPalette)(PHYSDEV,HPALETTE,BOOL);
    (PVOID)NULL_Unused, //HPEN     (*pSelectPen)(PHYSDEV,HPEN,const struct brush_pattern*);
    NULL_SetArcDirection, //INT      (*pSetArcDirection)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //COLORREF (*pSetBkColor)(PHYSDEV,COLORREF);
    (PVOID)NULL_Unused, //INT      (*pSetBkMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //UINT     (*pSetBoundsRect)(PHYSDEV,RECT*,UINT);
    (PVOID)NULL_Unused, //COLORREF (*pSetDCBrushColor)(PHYSDEV, COLORREF);
    (PVOID)NULL_Unused, //COLORREF (*pSetDCPenColor)(PHYSDEV, COLORREF);
    (PVOID)NULL_Unused, //INT      (*pSetDIBitsToDevice)(PHYSDEV,INT,INT,DWORD,DWORD,INT,INT,UINT,UINT,LPCVOID,BITMAPINFO*,UINT);
    (PVOID)NULL_Unused, //VOID     (*pSetDeviceClipping)(PHYSDEV,HRGN);
    (PVOID)NULL_Unused, //BOOL     (*pSetDeviceGammaRamp)(PHYSDEV,LPVOID);
    (PVOID)NULL_Unused, //DWORD    (*pSetLayout)(PHYSDEV,DWORD);
    NULL_SetMapMode,    //INT      (*pSetMapMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //DWORD    (*pSetMapperFlags)(PHYSDEV,DWORD);
    (PVOID)NULL_Unused, //COLORREF (*pSetPixel)(PHYSDEV,INT,INT,COLORREF);
    (PVOID)NULL_Unused, //INT      (*pSetPolyFillMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //INT      (*pSetROP2)(PHYSDEV,INT);
    NULL_SetRelAbs,     //INT      (*pSetRelAbs)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //INT      (*pSetStretchBltMode)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //UINT     (*pSetTextAlign)(PHYSDEV,UINT);
    (PVOID)NULL_Unused, //INT      (*pSetTextCharacterExtra)(PHYSDEV,INT);
    (PVOID)NULL_Unused, //COLORREF (*pSetTextColor)(PHYSDEV,COLORREF);
    (PVOID)NULL_Unused, //BOOL     (*pSetTextJustification)(PHYSDEV,INT,INT);
    NULL_SetViewportExtEx, //BOOL     (*pSetViewportExtEx)(PHYSDEV,INT,INT,SIZE*);
    NULL_SetViewportOrgEx, //BOOL     (*pSetViewportOrgEx)(PHYSDEV,INT,INT,POINT*);
    NULL_SetWindowExtEx, //BOOL     (*pSetWindowExtEx)(PHYSDEV,INT,INT,SIZE*);
    NULL_SetWindowOrgEx, //BOOL     (*pSetWindowOrgEx)(PHYSDEV,INT,INT,POINT*);
    NULL_SetWorldTransform, //BOOL     (*pSetWorldTransform)(PHYSDEV,const XFORM*);
    (PVOID)NULL_Unused, //INT      (*pStartDoc)(PHYSDEV,const DOCINFOW*);
    (PVOID)NULL_Unused, //INT      (*pStartPage)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pStretchBlt)(PHYSDEV,struct bitblt_coords*,PHYSDEV,struct bitblt_coords*,DWORD);
    (PVOID)NULL_Unused, //INT      (*pStretchDIBits)(PHYSDEV,INT,INT,INT,INT,INT,INT,INT,INT,const void*,BITMAPINFO*,UINT,DWORD);
    nulldrv_StrokeAndFillPath, //BOOL     (*pStrokeAndFillPath)(PHYSDEV);
    nulldrv_StrokePath, //BOOL     (*pStrokePath)(PHYSDEV);
    (PVOID)NULL_Unused, //BOOL     (*pUnrealizePalette)(HPALETTE);
    nulldrv_WidenPath,  //BOOL     (*pWidenPath)(PHYSDEV);
    (PVOID)NULL_Unused, //struct opengl_funcs * (*wine_get_wgl_driver)(PHYSDEV,UINT);
    0 // UINT       priority;
};

static
VOID
sync_dc_ptr(WINEDC* pWineDc)
{
    GetCurrentPositionEx(pWineDc->hdc, &pWineDc->cur_pos);
    pWineDc->GraphicsMode = GetGraphicsMode(pWineDc->hdc);
    pWineDc->layout = GetLayout(pWineDc->hdc);
    pWineDc->textAlign = GetTextAlign(pWineDc->hdc);
    pWineDc->hBrush = GetCurrentObject(pWineDc->hdc, OBJ_BRUSH);
    pWineDc->hPen = GetCurrentObject(pWineDc->hdc, OBJ_PEN);
}

WINEDC *get_nulldrv_dc( PHYSDEV dev )
{
    WINEDC* WineDc = CONTAINING_RECORD( dev, WINEDC, NullPhysDev );

    if (GDI_HANDLE_GET_TYPE(WineDc->hdc) == GDILoObjType_LO_ALTDC_TYPE)
        sync_dc_ptr(WineDc);

    return WineDc;
}

WINEDC* get_physdev_dc( PHYSDEV dev )
{
    while (dev->funcs != &DummyPhysDevFuncs)
        dev = dev->next;
    return get_nulldrv_dc( dev );
}

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

        /* Sync it */
        sync_dc_ptr(pWineDc);
    }
    else if (magic == OBJ_METADC)
    {
        pWineDc->hdc = GdiCreateClientObj(pWineDc, GDILoObjType_LO_METADC16_TYPE);
        if (pWineDc->hdc == NULL)
        {
            HeapFree(GetProcessHeap(), 0, pWineDc);
            return NULL;
        }
        pWineDc->hFont = GetStockObject(SYSTEM_FONT);
        pWineDc->hBrush = GetStockObject(WHITE_BRUSH);
        pWineDc->hPen = GetStockObject(BLACK_PEN);
        pWineDc->hPalette = GetStockObject(DEFAULT_PALETTE);
        pWineDc->MapMode = MM_TEXT;
        pWineDc->RelAbsMode = ABSOLUTE;
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
        WINEDC* pWineDc;

        /* The Wine DC is stored as the LDC */
        pWineDc = (WINEDC*)GdiGetLDC(hdc);
        if (!pWineDc)
            return NULL;

        /* Sync it */
        sync_dc_ptr(pWineDc);

        return pWineDc;
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
    while ((*dev)->funcs->priority > funcs->priority) dev = &(*dev)->next;
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

/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
int
bitmap_info_size(
    const BITMAPINFO * info,
    WORD coloruse)
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        if (info->bmiHeader.biClrUsed) colors = min( info->bmiHeader.biClrUsed, 256 );
        else colors = info->bmiHeader.biBitCount > 8 ? 0 : 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
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
#if 0
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
#endif
void
__cdecl
_assert (
    const char *exp,
    const char *file,
    unsigned line)
{
    DbgRaiseAssertionFailure();
}

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
DRIVER_SelectFont(WINEDC* pWineDc, HFONT Font)
{
    UINT aa_flags;
    PHYSDEV physdev = GET_DC_PHYSDEV(pWineDc, pSelectFont);
    if (!physdev->funcs->pSelectFont(physdev, Font, &aa_flags))
    {
        return NULL;
    }

    /* In case of META DC, everything stays in u-mode. Keep memory of objects */
    if (GDI_HANDLE_GET_TYPE(pWineDc->hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        HFONT Swap = pWineDc->hFont;
        pWineDc->hFont = Font;
        Font = Swap;
    }

    return Font;
}

static
HBRUSH
DRIVER_SelectBrush(WINEDC* pWineDc, HBRUSH Brush)
{
    PHYSDEV physdev = GET_DC_PHYSDEV(pWineDc, pSelectBrush);
    if (!physdev->funcs->pSelectBrush(physdev, Brush, NULL))
    {
        return NULL;
    }

    /* In case of META DC, everything stays in u-mode. Keep memory of objects */
    if (GDI_HANDLE_GET_TYPE(pWineDc->hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        HBRUSH Swap = pWineDc->hBrush;
        pWineDc->hBrush = Brush;
        Brush = Swap;
    }

    return Brush;
}

static
HPEN
DRIVER_SelectPen(WINEDC* pWineDc, HPEN Pen)
{
    PHYSDEV physdev = GET_DC_PHYSDEV(pWineDc, pSelectPen);
    if (!physdev->funcs->pSelectPen(physdev, Pen, NULL))
    {
        return NULL;
    }

    /* In case of META DC, everything stays in u-mode. Keep memory of objects */
    if (GDI_HANDLE_GET_TYPE(pWineDc->hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        HPEN Swap = pWineDc->hPen;
        pWineDc->hPen = Pen;
        Pen = Swap;
    }

    return Pen;
}

static
HPALETTE
DRIVER_SelectPalette(WINEDC* pWineDc, HPALETTE Palette, BOOL ForceBackground)
{
    PHYSDEV physdev = GET_DC_PHYSDEV(pWineDc, pSelectPalette);
    if (!physdev->funcs->pSelectPalette(physdev, Palette, ForceBackground))
    {
        return NULL;
    }

    /* In case of META DC, everything stays in u-mode. Keep memory of objects */
    if (GDI_HANDLE_GET_TYPE(pWineDc->hdc) == GDILoObjType_LO_METADC16_TYPE)
    {
        HPALETTE Swap = pWineDc->hPalette;
        pWineDc->hPalette = Palette;
        Palette = Swap;
    }

    return Palette;
}

static INT DRIVER_SetRelAbs(WINEDC* pWineDc, INT Mode)
{
    INT ret;
    PHYSDEV physdev = GET_DC_PHYSDEV(pWineDc, pSetRelAbs);
    ret = physdev->funcs->pSetRelAbs(physdev, Mode);
    if (ret && (GDI_HANDLE_GET_TYPE(pWineDc->hdc) == GDILoObjType_LO_METADC16_TYPE))
    {
        ret = pWineDc->RelAbsMode;
        pWineDc->RelAbsMode = Mode;
    }
    return ret;
}

static
DWORD_PTR
DRIVER_Dispatch(
    _In_ WINEDC* pWineDc,
    _In_ DCFUNC eFunction,
    _In_ va_list argptr)
{
    PHYSDEV physdev;

    switch (eFunction)
    {
#define HANDLE_FUNC0(__func)                            \
    case DCFUNC_##__func:                               \
    {                                                   \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);   \
        return physdev->funcs->p##__func(physdev);      \
    }
#define HANDLE_FUNC1(__func, __type1)                                                                   \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1);                                                \
    }
#define HANDLE_FUNC2(__func, __type1, __type2)                                                          \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2);                                          \
    }
#define HANDLE_FUNC3(__func, __type1, __type2, __type3)                                                 \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        __type3 arg3 = va_arg(argptr, __type3);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2, arg3);                                    \
    }
#define HANDLE_FUNC4(__func, __type1, __type2, __type3, __type4)                                        \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        __type3 arg3 = va_arg(argptr, __type3);                                                         \
        __type4 arg4 = va_arg(argptr, __type4);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2, arg3, arg4);                              \
    }
#define HANDLE_FUNC5(__func, __type1, __type2, __type3, __type4, __type5)                               \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        __type3 arg3 = va_arg(argptr, __type3);                                                         \
        __type4 arg4 = va_arg(argptr, __type4);                                                         \
        __type5 arg5 = va_arg(argptr, __type5);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2, arg3, arg4, arg5);                        \
    }
#define HANDLE_FUNC6(__func, __type1, __type2, __type3, __type4, __type5, __type6)                      \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        __type3 arg3 = va_arg(argptr, __type3);                                                         \
        __type4 arg4 = va_arg(argptr, __type4);                                                         \
        __type5 arg5 = va_arg(argptr, __type5);                                                         \
        __type6 arg6 = va_arg(argptr, __type6);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2, arg3, arg4, arg5, arg6);                  \
    }
#define HANDLE_FUNC7(__func, __type1, __type2, __type3, __type4, __type5, __type6, __type7)             \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        __type3 arg3 = va_arg(argptr, __type3);                                                         \
        __type4 arg4 = va_arg(argptr, __type4);                                                         \
        __type5 arg5 = va_arg(argptr, __type5);                                                         \
        __type6 arg6 = va_arg(argptr, __type6);                                                         \
        __type7 arg7 = va_arg(argptr, __type7);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2, arg3, arg4, arg5, arg6, arg7);            \
    }
#define HANDLE_FUNC8(__func, __type1, __type2, __type3, __type4, __type5, __type6, __type7, __type8)    \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        __type3 arg3 = va_arg(argptr, __type3);                                                         \
        __type4 arg4 = va_arg(argptr, __type4);                                                         \
        __type5 arg5 = va_arg(argptr, __type5);                                                         \
        __type6 arg6 = va_arg(argptr, __type6);                                                         \
        __type7 arg7 = va_arg(argptr, __type7);                                                         \
        __type8 arg8 = va_arg(argptr, __type8);                                                         \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8);      \
    }
#define HANDLE_FUNC11(__func, __type1, __type2, __type3, __type4, __type5, __type6, __type7, __type8,   \
        __type9, __type10, __type11)                                                                    \
    case DCFUNC_##__func:                                                                               \
    {                                                                                                   \
        __type1 arg1 = va_arg(argptr, __type1);                                                         \
        __type2 arg2 = va_arg(argptr, __type2);                                                         \
        __type3 arg3 = va_arg(argptr, __type3);                                                         \
        __type4 arg4 = va_arg(argptr, __type4);                                                         \
        __type5 arg5 = va_arg(argptr, __type5);                                                         \
        __type6 arg6 = va_arg(argptr, __type6);                                                         \
        __type7 arg7 = va_arg(argptr, __type7);                                                         \
        __type8 arg8 = va_arg(argptr, __type8);                                                         \
        __type9 arg9 = va_arg(argptr, __type9);                                                         \
        __type10 arg10 = va_arg(argptr, __type10);                                                      \
        __type11 arg11 = va_arg(argptr, __type11);                                                      \
        physdev = GET_DC_PHYSDEV(pWineDc, p##__func);                                                   \
        return physdev->funcs->p##__func(physdev, arg1, arg2, arg3, arg4, arg5,                         \
            arg6, arg7, arg8, arg9, arg10, arg11);                                                      \
    }


        HANDLE_FUNC0(AbortPath)
        HANDLE_FUNC8(Arc, INT, INT, INT, INT, INT, INT, INT, INT)
        HANDLE_FUNC0(BeginPath)
        HANDLE_FUNC8(Chord, INT, INT, INT, INT, INT, INT, INT, INT)
        HANDLE_FUNC0(CloseFigure)
        HANDLE_FUNC4(Ellipse, INT, INT, INT, INT)
        HANDLE_FUNC0(EndPath)
        HANDLE_FUNC4(ExcludeClipRect, INT, INT, INT, INT)
        HANDLE_FUNC5(ExtEscape, INT, INT, LPCVOID, INT, LPVOID)
        HANDLE_FUNC4(ExtFloodFill, INT, INT, COLORREF, INT)
        HANDLE_FUNC2(ExtSelectClipRgn, HRGN, INT)
        HANDLE_FUNC7(ExtTextOut, INT, INT, UINT, const RECT*, LPCWSTR, UINT, const INT *)
        HANDLE_FUNC0(FillPath)
        HANDLE_FUNC2(FillRgn, HRGN, HBRUSH)
        HANDLE_FUNC0(FlattenPath)
        HANDLE_FUNC4(FrameRgn, HRGN, HBRUSH, INT, INT)
        HANDLE_FUNC1(GetDeviceCaps, INT)
        HANDLE_FUNC2(GdiComment, INT, const BYTE *)
        HANDLE_FUNC4(IntersectClipRect, INT, INT, INT, INT)
        HANDLE_FUNC1(InvertRgn, HRGN)
        HANDLE_FUNC2(LineTo, INT, INT)
        HANDLE_FUNC2(ModifyWorldTransform, const XFORM*, DWORD)
        HANDLE_FUNC2(MoveTo, INT, INT)
        HANDLE_FUNC2(OffsetClipRgn, INT, INT)
        HANDLE_FUNC3(OffsetViewportOrgEx, INT, INT, LPPOINT)
        HANDLE_FUNC3(OffsetWindowOrgEx, INT, INT, LPPOINT)
        case DCFUNC_PatBlt:
        {
            INT xLeft = va_arg(argptr, INT);
            INT yTop = va_arg(argptr, INT);
            INT cx = va_arg(argptr, INT);
            INT cy = va_arg(argptr, INT);
            DWORD dwRop = va_arg(argptr, DWORD);
            physdev = GET_DC_PHYSDEV(pWineDc, pStretchBlt);
            return DRIVER_PatBlt(physdev, physdev->hdc, xLeft, yTop, cx, cy, dwRop);
        }
        HANDLE_FUNC8(Pie, INT, INT, INT, INT, INT, INT, INT, INT)
        HANDLE_FUNC2(PolyBezier, const POINT *, DWORD)
        HANDLE_FUNC2(PolyBezierTo, const POINT *, DWORD)
        HANDLE_FUNC3(PolyDraw, const POINT*, const BYTE*, DWORD)
        HANDLE_FUNC2(Polygon, const POINT *, INT)
        HANDLE_FUNC2(Polyline, const POINT *, INT)
        HANDLE_FUNC2(PolylineTo, const POINT *, INT)
        HANDLE_FUNC3(PolyPolygon, const POINT*, const INT*, DWORD)
        HANDLE_FUNC3(PolyPolyline, const POINT*, const DWORD*, DWORD)
        case DCFUNC_RealizePalette:
            physdev = GET_DC_PHYSDEV(pWineDc, pRealizePalette);
            return physdev->funcs->pRealizePalette(physdev, NULL, FALSE);
        HANDLE_FUNC4(Rectangle, INT, INT, INT, INT)
        case DCFUNC_RestoreDC:
            physdev = GET_DC_PHYSDEV(pWineDc, pRestoreDC);
            return DRIVER_RestoreDC(physdev, va_arg(argptr, INT));
        HANDLE_FUNC6(RoundRect, INT, INT, INT, INT, INT, INT)
        HANDLE_FUNC0(SaveDC)
        HANDLE_FUNC5(ScaleViewportExtEx, INT, INT, INT, INT, LPSIZE)
        HANDLE_FUNC5(ScaleWindowExtEx, INT, INT, INT, INT, LPSIZE)
        case DCFUNC_SelectBrush:
            return (DWORD_PTR)DRIVER_SelectBrush(pWineDc, va_arg(argptr, HBRUSH));
        HANDLE_FUNC1(SelectClipPath, INT)
        case DCFUNC_SelectFont:
            return (DWORD_PTR)DRIVER_SelectFont(pWineDc, va_arg(argptr, HFONT));
        case DCFUNC_SelectPalette:
        {
            HPALETTE Palette = va_arg(argptr, HPALETTE);
            BOOL ForceBackground = va_arg(argptr, BOOL);
            return (DWORD_PTR)DRIVER_SelectPalette(pWineDc, Palette, ForceBackground);
        }
        case DCFUNC_SelectPen:
            return (DWORD_PTR)DRIVER_SelectPen(pWineDc, va_arg(argptr, HPEN));
        HANDLE_FUNC1(SetArcDirection, INT)
        HANDLE_FUNC1(SetDCBrushColor, COLORREF)
        HANDLE_FUNC1(SetDCPenColor, COLORREF)
        HANDLE_FUNC11(SetDIBitsToDevice, INT, INT, DWORD, DWORD, INT, INT, UINT, UINT, LPCVOID, BITMAPINFO*, UINT)
        HANDLE_FUNC1(SetBkColor, COLORREF)
        HANDLE_FUNC1(SetBkMode, INT)
        HANDLE_FUNC1(SetLayout, DWORD)
        HANDLE_FUNC1(SetMapMode, INT)
        HANDLE_FUNC3(SetPixel, INT, INT, COLORREF)
        HANDLE_FUNC1(SetPolyFillMode, INT)
        HANDLE_FUNC1(SetROP2, INT)
        case DCFUNC_SetRelAbs:
        {
            INT Mode = va_arg(argptr, INT);
            return DRIVER_SetRelAbs(pWineDc, Mode);
        }
        HANDLE_FUNC1(SetStretchBltMode, INT)
        HANDLE_FUNC1(SetTextAlign, UINT)
        HANDLE_FUNC1(SetTextCharacterExtra, INT)
        HANDLE_FUNC1(SetTextColor, COLORREF)
        HANDLE_FUNC2(SetTextJustification, INT, INT)
        HANDLE_FUNC3(SetViewportExtEx, INT, INT, LPSIZE)
        HANDLE_FUNC3(SetViewportOrgEx, INT, INT, LPPOINT)
        HANDLE_FUNC3(SetWindowExtEx, INT, INT, LPSIZE)
        HANDLE_FUNC3(SetWindowOrgEx, INT, INT, LPPOINT)
        HANDLE_FUNC1(SetWorldTransform, const XFORM *)
        case DCFUNC_StretchBlt:
        {
            INT xDst = va_arg(argptr, INT);
            INT yDst = va_arg(argptr, INT);
            INT cxDst = va_arg(argptr, INT);
            INT cyDst = va_arg(argptr, INT);
            HDC hdcSrc = va_arg(argptr, HDC);
            INT xSrc = va_arg(argptr, INT);
            INT ySrc = va_arg(argptr, INT);
            INT cxSrc = va_arg(argptr, INT);
            INT cySrc = va_arg(argptr, INT);
            DWORD dwRop = va_arg(argptr, DWORD);
            physdev = GET_DC_PHYSDEV(pWineDc, pStretchBlt);
            return DRIVER_StretchBlt(physdev, physdev->hdc, xDst, yDst, cxDst, cyDst, hdcSrc, xSrc, ySrc, cxSrc, cySrc, dwRop);
        }
        HANDLE_FUNC0(StrokeAndFillPath)
        HANDLE_FUNC0(StrokePath)
        HANDLE_FUNC0(WidenPath)
        case DCFUNC_AngleArc:
        {
            INT x = va_arg(argptr, INT);
            INT y = va_arg(argptr, INT);
            DWORD Radius = va_arg(argptr, INT);
            DWORD StartAngle = va_arg(argptr, DWORD);
            DWORD SweepAngle = va_arg(argptr, DWORD);
            physdev = GET_DC_PHYSDEV(pWineDc, pAngleArc);
            return physdev->funcs->pAngleArc(physdev, x, y, Radius, RCAST(FLOAT, StartAngle), RCAST(FLOAT, SweepAngle));
        }
        HANDLE_FUNC8(ArcTo, INT, INT, INT, INT, INT, INT, INT, INT)
        HANDLE_FUNC5(GradientFill, TRIVERTEX*, ULONG, void *, ULONG, ULONG)
        /* These are not implemented in wine */
        case DCFUNC_AlphaBlend:
        case DCFUNC_MaskBlt:
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
    WINEDC* pWineDc;
    va_list argptr;

    /* Handle only METADC16 and ALTDC */
    if ((GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_ALTDC_TYPE) &&
        (GDI_HANDLE_GET_TYPE(hdc) != GDILoObjType_LO_METADC16_TYPE))
    {
        /* Let the caller handle it */
        return FALSE;
    }

    pWineDc = get_dc_ptr(hdc);
    if (pWineDc == NULL)
    {
        DPRINT1("No DC for HDC %p, function %u!\n", hdc, eFunction);
        SetLastError(ERROR_INVALID_HANDLE);
        *pdwResult = dwError;
        return TRUE;
    }

    i = eFunction;
    va_start(argptr, hdc);
    *pdwResult = DRIVER_Dispatch(pWineDc, eFunction, argptr);
    va_end(argptr);
    i = 0;

    /* Return TRUE to indicate that we want to return from the parent  */
    return ((GDI_HANDLE_GET_TYPE(hdc) == GDILoObjType_LO_METADC16_TYPE) ||
            (*pdwResult == dwError) ||
            (eFunction == DCFUNC_ExtSelectClipRgn) ||
            (eFunction == DCFUNC_SelectClipPath));
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
