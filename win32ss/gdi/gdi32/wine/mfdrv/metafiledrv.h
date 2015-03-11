/*
 * Metafile driver definitions
 *
 * Copyright 1996 Alexandre Julliard
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

#ifndef __WINE_METAFILEDRV_H
#define __WINE_METAFILEDRV_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "gdi_private.h"

/* Metafile driver physical DC */

typedef struct
{
    struct gdi_physdev dev;
    METAHEADER  *mh;           /* Pointer to metafile header */
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    HANDLE     hFile;          /* Handle for disk based MetaFile */
} METAFILEDRV_PDEVICE;

#define HANDLE_LIST_INC 20


extern BOOL MFDRV_MetaParam0(PHYSDEV dev, short func) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam1(PHYSDEV dev, short func, short param1) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam2(PHYSDEV dev, short func, short param1, short param2) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam4(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam6(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4, short param5,
                             short param6) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MetaParam8(PHYSDEV dev, short func, short param1, short param2,
                             short param3, short param4, short param5,
                             short param6, short param7, short param8) DECLSPEC_HIDDEN;
extern BOOL MFDRV_WriteRecord(PHYSDEV dev, METARECORD *mr, DWORD rlen) DECLSPEC_HIDDEN;
extern UINT MFDRV_AddHandle( PHYSDEV dev, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_RemoveHandle( PHYSDEV dev, UINT index ) DECLSPEC_HIDDEN;
extern INT16 MFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush ) DECLSPEC_HIDDEN;

/* Metafile driver functions */

extern BOOL MFDRV_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                             INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right,
                         INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_DeleteObject( PHYSDEV dev, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_Ellipse( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT  MFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType ) DECLSPEC_HIDDEN;
extern INT  MFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y, UINT flags, const RECT *lprect, LPCWSTR str,
                              UINT count, const INT *lpDx ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  MFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_MoveTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  MFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_OffsetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_OffsetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_PatBlt( PHYSDEV dev, struct bitblt_coords *dst, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                       INT bottom, INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_PolyBezier( PHYSDEV dev, const POINT* pt, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_PolyBezierTo( PHYSDEV dev, const POINT* pt, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts, UINT polygons) DECLSPEC_HIDDEN;
extern BOOL MFDRV_Polygon( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_Polyline( PHYSDEV dev, const POINT* pt,INT count) DECLSPEC_HIDDEN;
extern BOOL MFDRV_Rectangle( PHYSDEV dev, INT left, INT top, INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL MFDRV_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_RoundRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                             INT ell_width, INT ell_height ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SaveDC( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_ScaleViewportExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_ScaleWindowExtEx( PHYSDEV dev, INT xNum, INT xDenom, INT yNum, INT yDenom, SIZE *size ) DECLSPEC_HIDDEN;
extern HBITMAP MFDRV_SelectBitmap( PHYSDEV dev, HBITMAP handle ) DECLSPEC_HIDDEN;
extern HBRUSH  MFDRV_SelectBrush( PHYSDEV dev, HBRUSH hbrush, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_SelectClipPath( PHYSDEV dev, INT iMode ) DECLSPEC_HIDDEN;
extern HFONT MFDRV_SelectFont( PHYSDEV dev, HFONT handle, UINT *aa_flags ) DECLSPEC_HIDDEN;
extern HPEN MFDRV_SelectPen( PHYSDEV dev, HPEN handle, const struct brush_pattern *pattern ) DECLSPEC_HIDDEN;
extern HPALETTE MFDRV_SelectPalette( PHYSDEV dev, HPALETTE hPalette, BOOL bForceBackground) DECLSPEC_HIDDEN;
extern UINT MFDRV_RealizePalette(PHYSDEV dev, HPALETTE hPalette, BOOL primary) DECLSPEC_HIDDEN;
extern COLORREF MFDRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetBkMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern COLORREF MFDRV_SetDCBrushColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern COLORREF MFDRV_SetDCPenColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetMapMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern DWORD MFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags ) DECLSPEC_HIDDEN;
extern COLORREF MFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetPolyFillMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetROP2( PHYSDEV dev, INT rop ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetRelAbs( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetStretchBltMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern UINT MFDRV_SetTextAlign( PHYSDEV dev, UINT align ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetTextCharacterExtra( PHYSDEV dev, INT extra ) DECLSPEC_HIDDEN;
extern COLORREF  MFDRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_SetTextJustification( PHYSDEV dev, INT extra, INT breaks ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_SetViewportExtEx( PHYSDEV dev, INT x, INT y, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_SetViewportOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_SetWindowExtEx( PHYSDEV dev, INT x, INT y, SIZE *size ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_SetWindowOrgEx( PHYSDEV dev, INT x, INT y, POINT *pt ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_StretchBlt( PHYSDEV devDst, struct bitblt_coords *dst,
                              PHYSDEV devSrc, struct bitblt_coords *src, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern INT  MFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDest, INT yDest, DWORD cx,
                                     DWORD cy, INT xSrc, INT ySrc,
                                     UINT startscan, UINT lines, LPCVOID bits,
                                     BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern INT  MFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                                 INT heightDst, INT xSrc, INT ySrc, INT widthSrc, INT heightSrc,
                                 const void *bits, BITMAPINFO *info, UINT wUsage, DWORD dwRop ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL MFDRV_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;

#endif  /* __WINE_METAFILEDRV_H */
