/*
 * Enhanced MetaFile driver definitions
 *
 * Copyright 1999 Huw D M Davies
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_ENHMETAFILEDRV_H
#define __WINE_ENHMETAFILEDRV_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"

/* Enhanced Metafile driver physical DC */

typedef struct
{
    struct gdi_physdev dev;
    ENHMETAHEADER  *emh;           /* Pointer to enhanced metafile header */
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    HANDLE     hFile;              /* Handle for disk based MetaFile */
    HBRUSH     dc_brush;
    HPEN       dc_pen;
    HDC        ref_dc;             /* Reference device */
    HDC        screen_dc;          /* Screen DC if no reference device specified */
    INT        restoring;          /* RestoreDC counter */
} EMFDRV_PDEVICE;


extern BOOL EMFDRV_WriteRecord( PHYSDEV dev, EMR *emr ) DECLSPEC_HIDDEN;
extern void EMFDRV_UpdateBBox( PHYSDEV dev, RECTL *rect ) DECLSPEC_HIDDEN;
extern DWORD EMFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush ) DECLSPEC_HIDDEN;

#define HANDLE_LIST_INC 20

/* Metafile driver functions */
extern BOOL     EMFDRV_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right,
                            INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_BitBlt( PHYSDEV devDst, INT xDst, INT yDst, INT width, INT height,
                               PHYSDEV devSrc, INT xSrc, INT ySrc, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                              INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_DeleteObject( PHYSDEV dev, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect, LPCWSTR str,
                                   UINT count, const INT *lpDx ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT width, INT height ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_GdiComment( PHYSDEV dev, UINT bytes, const BYTE *buffer ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_GetDeviceCaps( PHYSDEV dev, INT cap ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, DWORD mode ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_MoveTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                            INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_PolyBezier( PHYSDEV dev, const POINT *pts, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_PolyBezierTo( PHYSDEV dev, const POINT *pts, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polys) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_PolyPolyline( PHYSDEV dev, const POINT* pt, const DWORD* counts, DWORD polys) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_Polygon( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_Polyline( PHYSDEV dev, const POINT* pt,INT count) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                                  INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SaveDC( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom,
                                           INT yNum, INT yDenom, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom,
                                         INT yNum, INT yDenom, SIZE *size ) DECLSPEC_HIDDEN;
extern HBITMAP  EMFDRV_SelectBitmap( PHYSDEV dev, HBITMAP handle ) DECLSPEC_HIDDEN;
extern HBRUSH   EMFDRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_SelectClipPath( PHYSDEV dev, INT iMode ) DECLSPEC_HIDDEN;
extern HFONT    EMFDRV_SelectFont( PHYSDEV dev, HFONT handle, UINT *aa_flags ) DECLSPEC_HIDDEN;
extern HPEN     EMFDRV_SelectPen( PHYSDEV dev, HPEN handle, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern HPALETTE EMFDRV_SelectPalette( PHYSDEV dev, HPALETTE hPal, BOOL force ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SetArcDirection( PHYSDEV dev, INT arcDirection ) DECLSPEC_HIDDEN;
extern COLORREF EMFDRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SetBkMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern COLORREF EMFDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF EMFDRV_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDest, INT yDest, DWORD cx, DWORD cy, INT xSrc,
                                          INT ySrc, UINT startscan, UINT lines, LPCVOID bits,
                                          BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern DWORD    EMFDRV_SetLayout( PHYSDEV dev, DWORD layout ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SetMapMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern DWORD    EMFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags ) DECLSPEC_HIDDEN;
extern COLORREF EMFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SetPolyFillMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SetROP2( PHYSDEV dev, INT rop ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_SetStretchBltMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern UINT     EMFDRV_SetTextAlign( PHYSDEV dev, UINT align ) DECLSPEC_HIDDEN;
extern COLORREF EMFDRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_SetTextJustification( PHYSDEV dev, INT nBreakExtra, INT nBreakCount ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_SetViewportExtEx( PHYSDEV dev, INT x, INT y, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_SetWindowExtEx( PHYSDEV dev, INT x, INT y, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_SetWorldTransform( PHYSDEV dev, const XFORM *xform ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_StretchBlt( PHYSDEV devDst, struct bitblt_coords *dst,
                                   PHYSDEV devSrc, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern INT      EMFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst, INT heightDst,
                                      INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                      const void *bits, BITMAPINFO *info, UINT wUsage, DWORD dwRop ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     EMFDRV_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;


#endif  /* __WINE_METAFILEDRV_H */
