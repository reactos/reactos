//
//
// Do not remove this file, "Justin Case" future maintenance issues with Path arises.......
//
//

#include <precomp.h>
#include "gdi_private.h"

#define NDEBUG
#include <debug.h>

WINEDC *get_nulldrv_dc( PHYSDEV dev );
const struct gdi_dc_funcs path_driver DECLSPEC_HIDDEN;

struct path_physdev
{
    struct gdi_physdev dev;
    //struct gdi_path   *path;
    BOOL HasPathHook;
};

static inline struct path_physdev *get_path_physdev( PHYSDEV dev )
{
    return CONTAINING_RECORD( dev, struct path_physdev, dev );
}

/***********************************************************************
 *           pathdrv_BeginPath
 */
static BOOL pathdrv_BeginPath( PHYSDEV dev )
{
    DPRINT("pathdrv_BeginPath dev %p\n",dev);
    return TRUE;
}


/***********************************************************************
 *           pathdrv_AbortPath
 */
static BOOL pathdrv_AbortPath( PHYSDEV dev )
{
    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_AbortPath dev %p\n",dev);
    path_driver.pDeleteDC( pop_dc_driver( dc, &path_driver ));
    return TRUE;
}


/***********************************************************************
 *           pathdrv_EndPath
 */
static BOOL pathdrv_EndPath( PHYSDEV dev )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DC *dc = get_physdev_dc( dev );

    DPRINT("pathdrv_EndPath dev %p\n",dev);

    pop_dc_driver( dc, &path_driver );
    HeapFree( GetProcessHeap(), 0, physdev );

    return TRUE;
}


/***********************************************************************
 *           pathdrv_CreateDC
 */
static BOOL pathdrv_CreateDC( PHYSDEV *dev, LPCWSTR driver, LPCWSTR device,
                              LPCWSTR output, const DEVMODEW *devmode )
{
    struct path_physdev *physdev = HeapAlloc( GetProcessHeap(), 0, sizeof(*physdev) );
    DPRINT("pathdrv_CreateDC dev %p\n",dev);
    if (!physdev) return FALSE;
    push_dc_driver( dev, &physdev->dev, &path_driver );
    return TRUE;
}


/*************************************************************
 *           pathdrv_DeleteDC
 */
static BOOL pathdrv_DeleteDC( PHYSDEV dev )
{
    struct path_physdev *physdev = get_path_physdev( dev );
    DPRINT("pathdrv_DeleteDC dev %p\n",dev);
    HeapFree( GetProcessHeap(), 0, physdev );
    return TRUE;
}

/*************************************************************
 *           pathdrv_MoveTo
 */
static BOOL pathdrv_MoveTo( PHYSDEV dev, INT x, INT y )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_MoveTo dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_LineTo
 */
static BOOL pathdrv_LineTo( PHYSDEV dev, INT x, INT y )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_LineTo dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_Rectangle
 */
static BOOL pathdrv_Rectangle( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_Rectangle dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_RoundRect
 */
static BOOL pathdrv_RoundRect( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2, INT ell_width, INT ell_height )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_RoundRect dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_Ellipse
 */
static BOOL pathdrv_Ellipse( PHYSDEV dev, INT x1, INT y1, INT x2, INT y2 )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_Ellipse dev %p\n",dev);
    return TRUE;
}

/*************************************************************
 *           pathdrv_AngleArc
 */
static BOOL pathdrv_AngleArc( PHYSDEV dev, INT x, INT y, DWORD radius, FLOAT eStartAngle, FLOAT eSweepAngle)
{
    DPRINT("pathdrv_AngleArc dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_Arc
 */
static BOOL pathdrv_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_Arc dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_ArcTo
 */
static BOOL pathdrv_ArcTo( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                           INT xstart, INT ystart, INT xend, INT yend )
{
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_ArcTo dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_Chord
 */
static BOOL pathdrv_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                           INT xstart, INT ystart, INT xend, INT yend )
{
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_Chord dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_Pie
 */
static BOOL pathdrv_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                         INT xstart, INT ystart, INT xend, INT yend )
{
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_Pie dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyBezierTo
 */
static BOOL pathdrv_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD cbPoints )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_PolyBezierTo dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyBezier
 */
static BOOL pathdrv_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD cbPoints )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_PolyBezier dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyDraw
 */
static BOOL pathdrv_PolyDraw( PHYSDEV dev, const POINT *pts, const BYTE *types, DWORD cbPoints )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_PolyDraw dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_Polyline
 */
static BOOL pathdrv_Polyline( PHYSDEV dev, const POINT *pts, INT count )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_PolyLine dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolylineTo
 */
static BOOL pathdrv_PolylineTo( PHYSDEV dev, const POINT *pts, INT count )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_PolyLineTo dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_Polygon
 */
static BOOL pathdrv_Polygon( PHYSDEV dev, const POINT *pts, INT count )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_Polygon dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyPolygon
 */
static BOOL pathdrv_PolyPolygon( PHYSDEV dev, const POINT* pts, const INT* counts, UINT polygons )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_PolyPolygon dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_PolyPolyline
 */
static BOOL pathdrv_PolyPolyline( PHYSDEV dev, const POINT* pts, const DWORD* counts, DWORD polylines )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
//    DC *dc = get_physdev_dc( dev );
    DPRINT("pathdrv_PolyPolyline dev %p\n",dev);
    return TRUE;
}


/*************************************************************
 *           pathdrv_ExtTextOut
 */
static BOOL pathdrv_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprc,
                                LPCWSTR str, UINT count, const INT *dx )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
    DPRINT("pathdrv_ExtTextOut dev %p\n",dev);
    return TRUE;
}

/*************************************************************
 *           pathdrv_CloseFigure
 */
static BOOL pathdrv_CloseFigure( PHYSDEV dev )
{
//    struct path_physdev *physdev = get_path_physdev( dev );
    DPRINT("pathdrv_CloseFigure dev %p\n",dev);
    return TRUE;
}


/***********************************************************************
 *           null driver fallback implementations
 */

BOOL nulldrv_BeginPath( PHYSDEV dev )
{
    DC *dc = get_nulldrv_dc( dev );
    struct path_physdev *physdev;

    if (!path_driver.pCreateDC( &dc->physDev, NULL, NULL, NULL, NULL ))
    {
        return FALSE;
    }
    physdev = get_path_physdev( find_dc_driver( dc, &path_driver ));
    physdev->HasPathHook = TRUE;
    DPRINT("nulldrv_BeginPath dev %p\n",dev);
    DPRINT("nulldrv_BeginPath pd %p\n",physdev);
    return TRUE;
}

BOOL nulldrv_EndPath( PHYSDEV dev )
{
    DPRINT("nulldrv_EndPath dev %p\n",dev);
    SetLastError( ERROR_CAN_NOT_COMPLETE );
    return FALSE;
}

BOOL nulldrv_AbortPath( PHYSDEV dev )
{
    //DC *dc = get_nulldrv_dc( dev );
    DPRINT("nulldrv_AbortPath dev %p\n",dev);
    //if (dc->path) free_gdi_path( dc->path );
    //dc->path = NULL;
    return TRUE;
}

BOOL nulldrv_CloseFigure( PHYSDEV dev )
{
    DPRINT("nulldrv_CloseFigure dev %p\n",dev);
    SetLastError( ERROR_CAN_NOT_COMPLETE );
    return FALSE;
}

BOOL nulldrv_SelectClipPath( PHYSDEV dev, INT mode )
{
    BOOL ret = FALSE;
    HRGN hrgn = PathToRegion( dev->hdc );
    DPRINT("nulldrv_SelectClipPath dev %p\n",dev);
    if (hrgn)
    {
        ret = ExtSelectClipRgn( dev->hdc, hrgn, mode ) != ERROR;
        DeleteObject( hrgn );
    }
    return ret;
//    return TRUE;
}

BOOL nulldrv_FillPath( PHYSDEV dev )
{
    DPRINT("nulldrv_FillPath dev %p\n",dev);
    //if (GetPath( dev->hdc, NULL, NULL, 0 ) == -1) return FALSE;
    //AbortPath( dev->hdc );
    return TRUE;
}

BOOL nulldrv_StrokeAndFillPath( PHYSDEV dev )
{
    DPRINT("nulldrv_StrokeAndFillPath dev %p\n",dev);
    //if (GetPath( dev->hdc, NULL, NULL, 0 ) == -1) return FALSE;
    //AbortPath( dev->hdc );
    return TRUE;
}

BOOL nulldrv_StrokePath( PHYSDEV dev )
{
    DPRINT("nulldrv_StrokePath dev %p\n",dev);
    //if (GetPath( dev->hdc, NULL, NULL, 0 ) == -1) return FALSE;
    //AbortPath( dev->hdc );
    return TRUE;
}

BOOL nulldrv_FlattenPath( PHYSDEV dev )
{
/*    DC *dc = get_nulldrv_dc( dev );
    struct gdi_path *path; */
    DPRINT("nulldrv_FlattenPath dev %p\n",dev);
/*    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!(path = PATH_FlattenPath( dc->path ))) return FALSE;
    free_gdi_path( dc->path );
    dc->path = path;*/
    return TRUE;
}

BOOL nulldrv_WidenPath( PHYSDEV dev )
{
/*    DC *dc = get_nulldrv_dc( dev );
    struct gdi_path *path;*/
    DPRINT("nulldrv_WidenPath dev %p\n",dev);
/*    if (!dc->path)
    {
        SetLastError( ERROR_CAN_NOT_COMPLETE );
        return FALSE;
    }
    if (!(path = PATH_WidenPath( dc ))) return FALSE;
    free_gdi_path( dc->path );
    dc->path = path;*/
    return TRUE;
}

const struct gdi_dc_funcs path_driver =
{
    NULL,                               /* pAbortDoc */
    pathdrv_AbortPath,                  /* pAbortPath */
    NULL,                               /* pAlphaBlend */
    pathdrv_AngleArc,                   /* pAngleArc */
    pathdrv_Arc,                        /* pArc */
    pathdrv_ArcTo,                      /* pArcTo */
    pathdrv_BeginPath,                  /* pBeginPath */
    NULL,                               /* pBlendImage */
    pathdrv_Chord,                      /* pChord */
    pathdrv_CloseFigure,                /* pCloseFigure */
    NULL,                               /* pCreateCompatibleDC */
    pathdrv_CreateDC,                   /* pCreateDC */
    pathdrv_DeleteDC,                   /* pDeleteDC */
    NULL,                               /* pDeleteObject */
    NULL,                               /* pDeviceCapabilities */
    pathdrv_Ellipse,                    /* pEllipse */
    NULL,                               /* pEndDoc */
    NULL,                               /* pEndPage */
    pathdrv_EndPath,                    /* pEndPath */
    NULL,                               /* pEnumFonts */
    NULL,                               /* pEnumICMProfiles */
    NULL,                               /* pExcludeClipRect */
    NULL,                               /* pExtDeviceMode */
    NULL,                               /* pExtEscape */
    NULL,                               /* pExtFloodFill */
    NULL,                               /* pExtSelectClipRgn */
    pathdrv_ExtTextOut,                 /* pExtTextOut */
    NULL,                               /* pFillPath */
    NULL,                               /* pFillRgn */
    NULL,                               /* pFlattenPath */
    NULL,                               /* pFontIsLinked */
    NULL,                               /* pFrameRgn */
    NULL,                               /* pGdiComment */
    NULL,                               /* pGetBoundsRect */
    NULL,                               /* pGetCharABCWidths */
    NULL,                               /* pGetCharABCWidthsI */
    NULL,                               /* pGetCharWidth */
    NULL,                               /* pGetDeviceCaps */
    NULL,                               /* pGetDeviceGammaRamp */
    NULL,                               /* pGetFontData */
    NULL,                               /* pGetFontRealizationInfo */
    NULL,                               /* pGetFontUnicodeRanges */
    NULL,                               /* pGetGlyphIndices */
    NULL,                               /* pGetGlyphOutline */
    NULL,                               /* pGetICMProfile */
    NULL,                               /* pGetImage */
    NULL,                               /* pGetKerningPairs */
    NULL,                               /* pGetNearestColor */
    NULL,                               /* pGetOutlineTextMetrics */
    NULL,                               /* pGetPixel */
    NULL,                               /* pGetSystemPaletteEntries */
    NULL,                               /* pGetTextCharsetInfo */
    NULL,                               /* pGetTextExtentExPoint */
    NULL,                               /* pGetTextExtentExPointI */
    NULL,                               /* pGetTextFace */
    NULL,                               /* pGetTextMetrics */
    NULL,                               /* pGradientFill */
    NULL,                               /* pIntersectClipRect */
    NULL,                               /* pInvertRgn */
    pathdrv_LineTo,                     /* pLineTo */
    NULL,                               /* pModifyWorldTransform */
    pathdrv_MoveTo,                     /* pMoveTo */
    NULL,                               /* pOffsetClipRgn */
    NULL,                               /* pOffsetViewportOrg */
    NULL,                               /* pOffsetWindowOrg */
    NULL,                               /* pPaintRgn */
    NULL,                               /* pPatBlt */
    pathdrv_Pie,                        /* pPie */
    pathdrv_PolyBezier,                 /* pPolyBezier */
    pathdrv_PolyBezierTo,               /* pPolyBezierTo */
    pathdrv_PolyDraw,                   /* pPolyDraw */
    pathdrv_PolyPolygon,                /* pPolyPolygon */
    pathdrv_PolyPolyline,               /* pPolyPolyline */
    pathdrv_Polygon,                    /* pPolygon */
    pathdrv_Polyline,                   /* pPolyline */
    pathdrv_PolylineTo,                 /* pPolylineTo */
    NULL,                               /* pPutImage */
    NULL,                               /* pRealizeDefaultPalette */
    NULL,                               /* pRealizePalette */
    pathdrv_Rectangle,                  /* pRectangle */
    NULL,                               /* pResetDC */
    NULL,                               /* pRestoreDC */
    pathdrv_RoundRect,                  /* pRoundRect */
    NULL,                               /* pSaveDC */
    NULL,                               /* pScaleViewportExt */
    NULL,                               /* pScaleWindowExt */
    NULL,                               /* pSelectBitmap */
    NULL,                               /* pSelectBrush */
    NULL,                               /* pSelectClipPath */
    NULL,                               /* pSelectFont */
    NULL,                               /* pSelectPalette */
    NULL,                               /* pSelectPen */
    NULL,                               /* pSetArcDirection */
    NULL,                               /* pSetBkColor */
    NULL,                               /* pSetBkMode */
    NULL,                               /* pSetDCBrushColor */
    NULL,                               /* pSetDCPenColor */
    NULL,                               /* pSetDIBColorTable */
    NULL,                               /* pSetDIBitsToDevice */
    NULL,                               /* pSetDeviceClipping */
    NULL,                               /* pSetDeviceGammaRamp */
    NULL,                               /* pSetLayout */
    NULL,                               /* pSetMapMode */
    NULL,                               /* pSetMapperFlags */
    NULL,                               /* pSetPixel */
    NULL,                               /* pSetPolyFillMode */
    NULL,                               /* pSetROP2 */
    NULL,                               /* pSetRelAbs */
    NULL,                               /* pSetStretchBltMode */
    NULL,                               /* pSetTextAlign */
    NULL,                               /* pSetTextCharacterExtra */
    NULL,                               /* pSetTextColor */
    NULL,                               /* pSetTextJustification */
    NULL,                               /* pSetViewportExt */
    NULL,                               /* pSetViewportOrg */
    NULL,                               /* pSetWindowExt */
    NULL,                               /* pSetWindowOrg */
    NULL,                               /* pSetWorldTransform */
    NULL,                               /* pStartDoc */
    NULL,                               /* pStartPage */
    NULL,                               /* pStretchBlt */
    NULL,                               /* pStretchDIBits */
    NULL,                               /* pStrokeAndFillPath */
    NULL,                               /* pStrokePath */
    NULL,                               /* pUnrealizePalette */
    NULL,                               /* pWidenPath */
    NULL,                               /* wine_get_wgl_driver */
    GDI_PRIORITY_PATH_DRV               /* priority */
};
