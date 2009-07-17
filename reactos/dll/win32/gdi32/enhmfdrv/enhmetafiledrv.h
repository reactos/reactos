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
    HDC             hdc;
    ENHMETAHEADER  *emh;           /* Pointer to enhanced metafile header */
    UINT       handles_size, cur_handles;
    HGDIOBJ   *handles;
    HANDLE     hFile;              /* Handle for disk based MetaFile */
    INT        horzres, vertres;
    INT        horzsize, vertsize;
    INT        logpixelsx, logpixelsy;
    INT        bitspixel;
    INT        textcaps;
    INT        rastercaps;
    INT        technology;
    INT        planes;
    INT        numcolors;
    INT        restoring;          /* RestoreDC counter */
} EMFDRV_PDEVICE;


extern BOOL EMFDRV_WriteRecord( PHYSDEV dev, EMR *emr ) DECLSPEC_HIDDEN;
extern void EMFDRV_UpdateBBox( PHYSDEV dev, RECTL *rect ) DECLSPEC_HIDDEN;
extern DWORD EMFDRV_CreateBrushIndirect( PHYSDEV dev, HBRUSH hBrush ) DECLSPEC_HIDDEN;

#define HANDLE_LIST_INC 20

/* Metafile driver functions */
extern BOOL     CDECL EMFDRV_AbortPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Arc( PHYSDEV dev, INT left, INT top, INT right,
                                  INT bottom, INT xstart, INT ystart, INT xend,
                                  INT yend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_BeginPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_BitBlt( PHYSDEV devDst, INT xDst, INT yDst,
                                     INT width, INT height, PHYSDEV devSrc,
                                     INT xSrc, INT ySrc, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Chord( PHYSDEV dev, INT left, INT top, INT right,
                                    INT bottom, INT xstart, INT ystart, INT xend,
                                    INT yend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_CloseFigure( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_DeleteObject( PHYSDEV dev, HGDIOBJ obj ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Ellipse( PHYSDEV dev, INT left, INT top,
                                      INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_EndPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_ExcludeClipRect( PHYSDEV dev, INT left, INT top, INT right, INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_ExtFloodFill( PHYSDEV dev, INT x, INT y, COLORREF color, UINT fillType ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_ExtSelectClipRgn( PHYSDEV dev, HRGN hrgn, INT mode ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_ExtTextOut( PHYSDEV dev, INT x, INT y,
                                         UINT flags, const RECT *lprect, LPCWSTR str,
                                         UINT count, const INT *lpDx ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FillRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FlattenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_FrameRgn( PHYSDEV dev, HRGN hrgn, HBRUSH hbrush, INT width,
                                       INT height ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_GdiComment( PHYSDEV dev, UINT bytes, CONST BYTE *buffer ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_GetDeviceCaps( PHYSDEV dev, INT cap ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_IntersectClipRect( PHYSDEV dev, INT left, INT top, INT right,
                                                INT bottom ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_InvertRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_LineTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_ModifyWorldTransform( PHYSDEV dev, const XFORM *xform, INT mode ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_MoveTo( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_OffsetClipRgn( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_OffsetViewportOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_OffsetWindowOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PaintRgn( PHYSDEV dev, HRGN hrgn ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PatBlt( PHYSDEV dev, INT left, INT top,
                                     INT width, INT height, DWORD rop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Pie( PHYSDEV dev, INT left, INT top, INT right,
                                  INT bottom, INT xstart, INT ystart, INT xend,
                                  INT yend ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolyPolygon( PHYSDEV dev, const POINT* pt,
                                          const INT* counts, UINT polys) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_PolyPolyline( PHYSDEV dev, const POINT* pt,
                                           const DWORD* counts, DWORD polys) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Polygon( PHYSDEV dev, const POINT* pt, INT count ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Polyline( PHYSDEV dev, const POINT* pt,INT count) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_Rectangle( PHYSDEV dev, INT left, INT top,
                                        INT right, INT bottom) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_RestoreDC( PHYSDEV dev, INT level ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_RoundRect( PHYSDEV dev, INT left, INT top,
                                        INT right, INT bottom, INT ell_width,
                                        INT ell_height ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SaveDC( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_ScaleViewportExt( PHYSDEV dev, INT xNum,
                                               INT xDenom, INT yNum, INT yDenom ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_ScaleWindowExt( PHYSDEV dev, INT xNum, INT xDenom,
                                             INT yNum, INT yDenom ) DECLSPEC_HIDDEN;
extern HBITMAP  CDECL EMFDRV_SelectBitmap( PHYSDEV dev, HBITMAP handle ) DECLSPEC_HIDDEN;
extern HBRUSH   CDECL EMFDRV_SelectBrush( PHYSDEV dev, HBRUSH handle ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_SelectClipPath( PHYSDEV dev, INT iMode ) DECLSPEC_HIDDEN;
extern HFONT    CDECL EMFDRV_SelectFont( PHYSDEV dev, HFONT handle, HANDLE gdiFont ) DECLSPEC_HIDDEN;
extern HPEN     CDECL EMFDRV_SelectPen( PHYSDEV dev, HPEN handle ) DECLSPEC_HIDDEN;
extern HPALETTE CDECL EMFDRV_SelectPalette( PHYSDEV dev, HPALETTE hPal, BOOL force ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetArcDirection( PHYSDEV dev, INT arcDirection ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetBkColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetBkMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetDIBitsToDevice( PHYSDEV dev, INT xDest, INT yDest,
                                                DWORD cx, DWORD cy, INT xSrc,
                                                INT ySrc, UINT startscan, UINT lines,
                                                LPCVOID bits, const BITMAPINFO *info,
                                                UINT coloruse ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetMapMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern DWORD    CDECL EMFDRV_SetMapperFlags( PHYSDEV dev, DWORD flags ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetPixel( PHYSDEV dev, INT x, INT y, COLORREF color ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetPolyFillMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetROP2( PHYSDEV dev, INT rop ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetStretchBltMode( PHYSDEV dev, INT mode ) DECLSPEC_HIDDEN;
extern UINT     CDECL EMFDRV_SetTextAlign( PHYSDEV dev, UINT align ) DECLSPEC_HIDDEN;
extern COLORREF CDECL EMFDRV_SetTextColor( PHYSDEV dev, COLORREF color ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_SetTextJustification( PHYSDEV dev, INT nBreakExtra,
                                                   INT nBreakCount ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetViewportExt( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetViewportOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetWindowExt( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_SetWindowOrg( PHYSDEV dev, INT x, INT y ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_SetWorldTransform( PHYSDEV dev, const XFORM *xform ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_StretchBlt( PHYSDEV devDst, INT xDst, INT yDst,
                                         INT widthDst, INT heightDst,
                                         PHYSDEV devSrc, INT xSrc, INT ySrc,
                                         INT widthSrc, INT heightSrc, DWORD rop ) DECLSPEC_HIDDEN;
extern INT      CDECL EMFDRV_StretchDIBits( PHYSDEV dev, INT xDst, INT yDst, INT widthDst,
                                            INT heightDst, INT xSrc, INT ySrc,
                                            INT widthSrc, INT heightSrc,
                                            const void *bits, const BITMAPINFO *info,
                                            UINT wUsage, DWORD dwRop ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_StrokeAndFillPath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_StrokePath( PHYSDEV dev ) DECLSPEC_HIDDEN;
extern BOOL     CDECL EMFDRV_WidenPath( PHYSDEV dev ) DECLSPEC_HIDDEN;


#endif  /* __WINE_METAFILEDRV_H */
