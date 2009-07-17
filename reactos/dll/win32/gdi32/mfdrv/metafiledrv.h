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
    HDC          hdc;
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

extern BOOL CDECL MFDRV_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right, INT bottom,
                             INT xstart, INT ystart, INT xend, INT yend ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_BitBlt( PHYSDEV devDst, INT xDst, INT yDst,  INT width,
                                INT height, PHYSDEV devSrc, INT xSrc, INT ySrc,
                                DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL CDECL MFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right,
                               INT bottom, INT xstart, INT ystart, INT xend,
                               INT yend ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_DeleteObject( PHYSDEV dev, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_Ellipse( PHYSDEV dev, INT left, INT top,
                                  INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right,
                                         INT bottom ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_ExtEscape( PHYSDEV dev, INT nEscape, INT cbInput, LPCVOID in_data,
                                   INT cbOutput, LPVOID out_data ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType ) DECLSPEC_HIDDEN;
extern INT   CDECL MFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y,
                                     UINT flags, const RECT *lprect, LPCWSTR str,
                                     UINT count, const INT *lpDx ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_GetDeviceCaps( PHYSDEV dev , INT cap ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_MoveTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT   CDECL MFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT   CDECL MFDRV_OffsetViewportOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT   CDECL MFDRV_OffsetWindowOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_PatBlt( PHYSDEV dev, INT left, INT top, INT width, INT height,
                                 DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                              INT bottom, INT xstart, INT ystart, INT xend,
                              INT yend ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_PolyBezier( PHYSDEV dev, const POINT* pt, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_PolyBezierTo( PHYSDEV dev, const POINT* pt, DWORD count ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt, const INT* counts,
                                      UINT polygons) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_Polygon( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_Polyline( PHYSDEV dev, const POINT* pt,INT count) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_Rectangle( PHYSDEV dev, INT left, INT top,
                                    INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_RoundRect( PHYSDEV dev, INT left, INT top,
                                    INT right, INT bottom, INT ell_width,
                                    INT ell_height ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SaveDC( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_ScaleViewportExt( PHYSDEV dev, INT xNum, INT xDenom, INT yNum,
                                          INT yDenom ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_ScaleWindowExt( PHYSDEV dev, INT xNum, INT xDenom, INT yNum,
                                        INT yDenom ) DECLSPEC_HIDDEN;
extern HBITMAP  CDECL MFDRV_SelectBitmap( PHYSDEV dev, HBITMAP handle ) DECLSPEC_HIDDEN;
extern HBRUSH  CDECL MFDRV_SelectBrush( PHYSDEV dev, HBRUSH handle ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_SelectClipPath( PHYSDEV dev, INT iMode ) DECLSPEC_HIDDEN;
extern HFONT  CDECL MFDRV_SelectFont( PHYSDEV dev, HFONT handle, HANDLE gdiFont ) DECLSPEC_HIDDEN;
extern HPEN  CDECL MFDRV_SelectPen( PHYSDEV dev, HPEN handle ) DECLSPEC_HIDDEN;
extern HPALETTE  CDECL MFDRV_SelectPalette( PHYSDEV dev, HPALETTE hPalette, BOOL bForceBackground) DECLSPEC_HIDDEN;
extern UINT  CDECL MFDRV_RealizePalette(PHYSDEV dev, HPALETTE hPalette, BOOL primary) DECLSPEC_HIDDEN;
extern COLORREF  CDECL MFDRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetBkMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetMapMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern DWORD  CDECL MFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags ) DECLSPEC_HIDDEN;
extern COLORREF  CDECL MFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetPolyFillMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetROP2( PHYSDEV dev, INT rop ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetRelAbs( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetStretchBltMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern UINT  CDECL MFDRV_SetTextAlign( PHYSDEV dev, UINT align ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetTextCharacterExtra( PHYSDEV dev, INT extra ) DECLSPEC_HIDDEN;
extern COLORREF  CDECL MFDRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetTextJustification( PHYSDEV dev, INT extra, INT breaks ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetViewportExt( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetViewportOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetWindowExt( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetWindowOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_StretchBlt( PHYSDEV devDst, INT xDst, INT yDst, INT widthDst,
                                     INT heightDst, PHYSDEV devSrc, INT xSrc, INT ySrc,
                                     INT widthSrc, INT heightSrc, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDest, INT yDest, DWORD cx,
                                           DWORD cy, INT xSrc, INT ySrc,
                                           UINT startscan, UINT lines, LPCVOID bits,
                                           const BITMAPINFO *info, UINT coloruse ) DECLSPEC_HIDDEN;
extern INT  CDECL MFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                                       INT heightDst, INT xSrc, INT ySrc,
                                       INT widthSrc, INT heightSrc, const void *bits,
                                       const BITMAPINFO *info, UINT wUsage,
                                       DWORD dwRop ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL  CDECL MFDRV_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;

#endif  /* __WINE_METAFILEDRV_H */
