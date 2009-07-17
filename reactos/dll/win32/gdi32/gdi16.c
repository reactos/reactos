/*
 * GDI 16-bit functions
 *
 * Copyright 2002 Alexandre Julliard
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wownt32.h"
#include "wine/wingdi16.h"
#include "gdi_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

#define HGDIOBJ_32(handle16)    ((HGDIOBJ)(ULONG_PTR)(handle16))
#define HGDIOBJ_16(handle32)    ((HGDIOBJ16)(ULONG_PTR)(handle32))

struct callback16_info
{
    FARPROC16 proc;
    LPARAM    param;
};

/* callback for LineDDA16 */
static void CALLBACK linedda_callback( INT x, INT y, LPARAM param )
{
    const struct callback16_info *info = (struct callback16_info *)param;
    WORD args[4];

    args[3] = x;
    args[2] = y;
    args[1] = HIWORD(info->param);
    args[0] = LOWORD(info->param);
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, NULL );
}

/* callback for EnumObjects16 */
static INT CALLBACK enum_pens_callback( void *ptr, LPARAM param )
{
    const struct callback16_info *info = (struct callback16_info *)param;
    LOGPEN *pen = ptr;
    LOGPEN16 pen16;
    SEGPTR segptr;
    DWORD ret;
    WORD args[4];

    pen16.lopnStyle   = pen->lopnStyle;
    pen16.lopnWidth.x = pen->lopnWidth.x;
    pen16.lopnWidth.y = pen->lopnWidth.y;
    pen16.lopnColor   = pen->lopnColor;
    segptr = MapLS( &pen16 );
    args[3] = SELECTOROF(segptr);
    args[2] = OFFSETOF(segptr);
    args[1] = HIWORD(info->param);
    args[0] = LOWORD(info->param);
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    UnMapLS( segptr );
    return LOWORD(ret);
}

/* callback for EnumObjects16 */
static INT CALLBACK enum_brushes_callback( void *ptr, LPARAM param )
{
    const struct callback16_info *info = (struct callback16_info *)param;
    LOGBRUSH *brush = ptr;
    LOGBRUSH16 brush16;
    SEGPTR segptr;
    DWORD ret;
    WORD args[4];

    brush16.lbStyle = brush->lbStyle;
    brush16.lbColor = brush->lbColor;
    brush16.lbHatch = brush->lbHatch;
    segptr = MapLS( &brush16 );
    args[3] = SELECTOROF(segptr);
    args[2] = OFFSETOF(segptr);
    args[1] = HIWORD(info->param);
    args[0] = LOWORD(info->param);
    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    UnMapLS( segptr );
    return ret;
}

/* convert a LOGFONT16 to a LOGFONTW */
static void logfont_16_to_W( const LOGFONT16 *font16, LPLOGFONTW font32 )
{
    font32->lfHeight = font16->lfHeight;
    font32->lfWidth = font16->lfWidth;
    font32->lfEscapement = font16->lfEscapement;
    font32->lfOrientation = font16->lfOrientation;
    font32->lfWeight = font16->lfWeight;
    font32->lfItalic = font16->lfItalic;
    font32->lfUnderline = font16->lfUnderline;
    font32->lfStrikeOut = font16->lfStrikeOut;
    font32->lfCharSet = font16->lfCharSet;
    font32->lfOutPrecision = font16->lfOutPrecision;
    font32->lfClipPrecision = font16->lfClipPrecision;
    font32->lfQuality = font16->lfQuality;
    font32->lfPitchAndFamily = font16->lfPitchAndFamily;
    MultiByteToWideChar( CP_ACP, 0, font16->lfFaceName, -1, font32->lfFaceName, LF_FACESIZE );
    font32->lfFaceName[LF_FACESIZE-1] = 0;
}

/* convert a LOGFONTW to a LOGFONT16 */
static void logfont_W_to_16( const LOGFONTW* font32, LPLOGFONT16 font16 )
{
    font16->lfHeight = font32->lfHeight;
    font16->lfWidth = font32->lfWidth;
    font16->lfEscapement = font32->lfEscapement;
    font16->lfOrientation = font32->lfOrientation;
    font16->lfWeight = font32->lfWeight;
    font16->lfItalic = font32->lfItalic;
    font16->lfUnderline = font32->lfUnderline;
    font16->lfStrikeOut = font32->lfStrikeOut;
    font16->lfCharSet = font32->lfCharSet;
    font16->lfOutPrecision = font32->lfOutPrecision;
    font16->lfClipPrecision = font32->lfClipPrecision;
    font16->lfQuality = font32->lfQuality;
    font16->lfPitchAndFamily = font32->lfPitchAndFamily;
    WideCharToMultiByte( CP_ACP, 0, font32->lfFaceName, -1, font16->lfFaceName, LF_FACESIZE, NULL, NULL );
    font16->lfFaceName[LF_FACESIZE-1] = 0;
}

/* convert a ENUMLOGFONTEXW to a ENUMLOGFONTEX16 */
static void enumlogfontex_W_to_16( const ENUMLOGFONTEXW *fontW,
                                   LPENUMLOGFONTEX16 font16 )
{
    logfont_W_to_16( (const LOGFONTW *)fontW, (LPLOGFONT16)font16);

    WideCharToMultiByte( CP_ACP, 0, fontW->elfFullName, -1,
                         (LPSTR) font16->elfFullName, LF_FULLFACESIZE, NULL, NULL );
    font16->elfFullName[LF_FULLFACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfStyle, -1,
                         (LPSTR) font16->elfStyle, LF_FACESIZE, NULL, NULL );
    font16->elfStyle[LF_FACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfScript, -1,
                         (LPSTR) font16->elfScript, LF_FACESIZE, NULL, NULL );
    font16->elfScript[LF_FACESIZE-1] = '\0';
}

/* convert a NEWTEXTMETRICEXW to a NEWTEXTMETRICEX16 */
static void newtextmetricex_W_to_16( const NEWTEXTMETRICEXW *ptmW,
                                     LPNEWTEXTMETRICEX16 ptm16 )
{
    ptm16->ntmTm.tmHeight = ptmW->ntmTm.tmHeight;
    ptm16->ntmTm.tmAscent = ptmW->ntmTm.tmAscent;
    ptm16->ntmTm.tmDescent = ptmW->ntmTm.tmDescent;
    ptm16->ntmTm.tmInternalLeading = ptmW->ntmTm.tmInternalLeading;
    ptm16->ntmTm.tmExternalLeading = ptmW->ntmTm.tmExternalLeading;
    ptm16->ntmTm.tmAveCharWidth = ptmW->ntmTm.tmAveCharWidth;
    ptm16->ntmTm.tmMaxCharWidth = ptmW->ntmTm.tmMaxCharWidth;
    ptm16->ntmTm.tmWeight = ptmW->ntmTm.tmWeight;
    ptm16->ntmTm.tmOverhang = ptmW->ntmTm.tmOverhang;
    ptm16->ntmTm.tmDigitizedAspectX = ptmW->ntmTm.tmDigitizedAspectX;
    ptm16->ntmTm.tmDigitizedAspectY = ptmW->ntmTm.tmDigitizedAspectY;
    ptm16->ntmTm.tmFirstChar = ptmW->ntmTm.tmFirstChar > 255 ? 255 : ptmW->ntmTm.tmFirstChar;
    ptm16->ntmTm.tmLastChar = ptmW->ntmTm.tmLastChar > 255 ? 255 : ptmW->ntmTm.tmLastChar;
    ptm16->ntmTm.tmDefaultChar = ptmW->ntmTm.tmDefaultChar > 255 ? 255 : ptmW->ntmTm.tmDefaultChar;
    ptm16->ntmTm.tmBreakChar = ptmW->ntmTm.tmBreakChar > 255 ? 255 : ptmW->ntmTm.tmBreakChar;
    ptm16->ntmTm.tmItalic = ptmW->ntmTm.tmItalic;
    ptm16->ntmTm.tmUnderlined = ptmW->ntmTm.tmUnderlined;
    ptm16->ntmTm.tmStruckOut = ptmW->ntmTm.tmStruckOut;
    ptm16->ntmTm.tmPitchAndFamily = ptmW->ntmTm.tmPitchAndFamily;
    ptm16->ntmTm.tmCharSet = ptmW->ntmTm.tmCharSet;
    ptm16->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptm16->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptm16->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptm16->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    ptm16->ntmFontSig = ptmW->ntmFontSig;
}

/*
 * callback for EnumFontFamiliesEx16
 * Note: plf is really an ENUMLOGFONTEXW, and ptm is a NEWTEXTMETRICEXW.
 *       We have to use other types because of the FONTENUMPROCW definition.
 */
static INT CALLBACK enum_font_callback( const LOGFONTW *plf,
                                        const TEXTMETRICW *ptm, DWORD fType,
                                        LPARAM param )
{
    const struct callback16_info *info = (struct callback16_info *)param;
    ENUMLOGFONTEX16 elfe16;
    NEWTEXTMETRICEX16 ntm16;
    SEGPTR segelfe16;
    SEGPTR segntm16;
    WORD args[7];
    DWORD ret;

    enumlogfontex_W_to_16((const ENUMLOGFONTEXW *)plf, &elfe16);
    newtextmetricex_W_to_16((const NEWTEXTMETRICEXW *)ptm, &ntm16);
    segelfe16 = MapLS( &elfe16 );
    segntm16 = MapLS( &ntm16 );
    args[6] = SELECTOROF(segelfe16);
    args[5] = OFFSETOF(segelfe16);
    args[4] = SELECTOROF(segntm16);
    args[3] = OFFSETOF(segntm16);
    args[2] = fType;
    args[1] = HIWORD(info->param);
    args[0] = LOWORD(info->param);

    WOWCallback16Ex( (DWORD)info->proc, WCB16_PASCAL, sizeof(args), args, &ret );
    UnMapLS( segelfe16 );
    UnMapLS( segntm16 );
    return LOWORD(ret);
}


/***********************************************************************
 *           SetBkColor    (GDI.1)
 */
COLORREF WINAPI SetBkColor16( HDC16 hdc, COLORREF color )
{
    return SetBkColor( HDC_32(hdc), color );
}


/***********************************************************************
 *		SetBkMode (GDI.2)
 */
INT16 WINAPI SetBkMode16( HDC16 hdc, INT16 mode )
{
    return SetBkMode( HDC_32(hdc), mode );
}


/***********************************************************************
 *           SetMapMode    (GDI.3)
 */
INT16 WINAPI SetMapMode16( HDC16 hdc, INT16 mode )
{
    return SetMapMode( HDC_32(hdc), mode );
}


/***********************************************************************
 *		SetROP2	(GDI.4)
 */
INT16 WINAPI SetROP216( HDC16 hdc, INT16 mode )
{
    return SetROP2( HDC_32(hdc), mode );
}


/***********************************************************************
 *		SetRelAbs (GDI.5)
 */
INT16 WINAPI SetRelAbs16( HDC16 hdc, INT16 mode )
{
    return SetRelAbs( HDC_32(hdc), mode );
}


/***********************************************************************
 *		SetPolyFillMode	(GDI.6)
 */
INT16 WINAPI SetPolyFillMode16( HDC16 hdc, INT16 mode )
{
    return SetPolyFillMode( HDC_32(hdc), mode );
}


/***********************************************************************
 *		SetStretchBltMode (GDI.7)
 */
INT16 WINAPI SetStretchBltMode16( HDC16 hdc, INT16 mode )
{
    return SetStretchBltMode( HDC_32(hdc), mode );
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI.8)
 */
INT16 WINAPI SetTextCharacterExtra16( HDC16 hdc, INT16 extra )
{
    return SetTextCharacterExtra( HDC_32(hdc), extra );
}


/***********************************************************************
 *           SetTextColor    (GDI.9)
 */
COLORREF WINAPI SetTextColor16( HDC16 hdc, COLORREF color )
{
    return SetTextColor( HDC_32(hdc), color );
}


/***********************************************************************
 *           SetTextJustification    (GDI.10)
 */
INT16 WINAPI SetTextJustification16( HDC16 hdc, INT16 extra, INT16 breaks )
{
    return SetTextJustification( HDC_32(hdc), extra, breaks );
}


/***********************************************************************
 *           SetWindowOrg    (GDI.11)
 */
DWORD WINAPI SetWindowOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!SetWindowOrgEx( HDC_32(hdc), x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetWindowExt    (GDI.12)
 */
DWORD WINAPI SetWindowExt16( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE size;
    if (!SetWindowExtEx( HDC_32(hdc), x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetViewportOrg    (GDI.13)
 */
DWORD WINAPI SetViewportOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!SetViewportOrgEx( HDC_32(hdc), x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           SetViewportExt    (GDI.14)
 */
DWORD WINAPI SetViewportExt16( HDC16 hdc, INT16 x, INT16 y )
{
    SIZE size;
    if (!SetViewportExtEx( HDC_32(hdc), x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           OffsetWindowOrg    (GDI.15)
 */
DWORD WINAPI OffsetWindowOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!OffsetWindowOrgEx( HDC_32(hdc), x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           ScaleWindowExt    (GDI.16)
 */
DWORD WINAPI ScaleWindowExt16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                             INT16 yNum, INT16 yDenom )
{
    SIZE size;
    if (!ScaleWindowExtEx( HDC_32(hdc), xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           OffsetViewportOrg    (GDI.17)
 */
DWORD WINAPI OffsetViewportOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;
    if (!OffsetViewportOrgEx( HDC_32(hdc), x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           ScaleViewportExt    (GDI.18)
 */
DWORD WINAPI ScaleViewportExt16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                               INT16 yNum, INT16 yDenom )
{
    SIZE size;
    if (!ScaleViewportExtEx( HDC_32(hdc), xNum, xDenom, yNum, yDenom, &size ))
        return FALSE;
    return MAKELONG( size.cx,  size.cy );
}


/***********************************************************************
 *           LineTo    (GDI.19)
 */
BOOL16 WINAPI LineTo16( HDC16 hdc, INT16 x, INT16 y )
{
    return LineTo( HDC_32(hdc), x, y );
}


/***********************************************************************
 *           MoveTo    (GDI.20)
 */
DWORD WINAPI MoveTo16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;

    if (!MoveToEx( HDC_32(hdc), x, y, &pt )) return 0;
    return MAKELONG(pt.x,pt.y);
}


/***********************************************************************
 *           ExcludeClipRect    (GDI.21)
 */
INT16 WINAPI ExcludeClipRect16( HDC16 hdc, INT16 left, INT16 top,
                                INT16 right, INT16 bottom )
{
    return ExcludeClipRect( HDC_32(hdc), left, top, right, bottom );
}


/***********************************************************************
 *           IntersectClipRect    (GDI.22)
 */
INT16 WINAPI IntersectClipRect16( HDC16 hdc, INT16 left, INT16 top,
                                  INT16 right, INT16 bottom )
{
    return IntersectClipRect( HDC_32(hdc), left, top, right, bottom );
}


/***********************************************************************
 *           Arc    (GDI.23)
 */
BOOL16 WINAPI Arc16( HDC16 hdc, INT16 left, INT16 top, INT16 right,
                     INT16 bottom, INT16 xstart, INT16 ystart,
                     INT16 xend, INT16 yend )
{
    return Arc( HDC_32(hdc), left, top, right, bottom, xstart, ystart, xend, yend );
}


/***********************************************************************
 *           Ellipse    (GDI.24)
 */
BOOL16 WINAPI Ellipse16( HDC16 hdc, INT16 left, INT16 top,
                         INT16 right, INT16 bottom )
{
    return Ellipse( HDC_32(hdc), left, top, right, bottom );
}


/**********************************************************************
 *          FloodFill   (GDI.25)
 */
BOOL16 WINAPI FloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return ExtFloodFill( HDC_32(hdc), x, y, color, FLOODFILLBORDER );
}


/***********************************************************************
 *           Pie    (GDI.26)
 */
BOOL16 WINAPI Pie16( HDC16 hdc, INT16 left, INT16 top,
                     INT16 right, INT16 bottom, INT16 xstart, INT16 ystart,
                     INT16 xend, INT16 yend )
{
    return Pie( HDC_32(hdc), left, top, right, bottom, xstart, ystart, xend, yend );
}


/***********************************************************************
 *           Rectangle    (GDI.27)
 */
BOOL16 WINAPI Rectangle16( HDC16 hdc, INT16 left, INT16 top,
                           INT16 right, INT16 bottom )
{
    return Rectangle( HDC_32(hdc), left, top, right, bottom );
}


/***********************************************************************
 *           RoundRect    (GDI.28)
 */
BOOL16 WINAPI RoundRect16( HDC16 hdc, INT16 left, INT16 top, INT16 right,
                           INT16 bottom, INT16 ell_width, INT16 ell_height )
{
    return RoundRect( HDC_32(hdc), left, top, right, bottom, ell_width, ell_height );
}


/***********************************************************************
 *           PatBlt    (GDI.29)
 */
BOOL16 WINAPI PatBlt16( HDC16 hdc, INT16 left, INT16 top,
                        INT16 width, INT16 height, DWORD rop)
{
    return PatBlt( HDC_32(hdc), left, top, width, height, rop );
}


/***********************************************************************
 *           SaveDC    (GDI.30)
 */
INT16 WINAPI SaveDC16( HDC16 hdc )
{
    return SaveDC( HDC_32(hdc) );
}


/***********************************************************************
 *           SetPixel    (GDI.31)
 */
COLORREF WINAPI SetPixel16( HDC16 hdc, INT16 x, INT16 y, COLORREF color )
{
    return SetPixel( HDC_32(hdc), x, y, color );
}


/***********************************************************************
 *           OffsetClipRgn    (GDI.32)
 */
INT16 WINAPI OffsetClipRgn16( HDC16 hdc, INT16 x, INT16 y )
{
    return OffsetClipRgn( HDC_32(hdc), x, y );
}


/***********************************************************************
 *           TextOut    (GDI.33)
 */
BOOL16 WINAPI TextOut16( HDC16 hdc, INT16 x, INT16 y, LPCSTR str, INT16 count )
{
    return TextOutA( HDC_32(hdc), x, y, str, count );
}


/***********************************************************************
 *           BitBlt    (GDI.34)
 */
BOOL16 WINAPI BitBlt16( HDC16 hdcDst, INT16 xDst, INT16 yDst, INT16 width,
                        INT16 height, HDC16 hdcSrc, INT16 xSrc, INT16 ySrc,
                        DWORD rop )
{
    return BitBlt( HDC_32(hdcDst), xDst, yDst, width, height, HDC_32(hdcSrc), xSrc, ySrc, rop );
}


/***********************************************************************
 *           StretchBlt    (GDI.35)
 */
BOOL16 WINAPI StretchBlt16( HDC16 hdcDst, INT16 xDst, INT16 yDst,
                            INT16 widthDst, INT16 heightDst,
                            HDC16 hdcSrc, INT16 xSrc, INT16 ySrc,
                            INT16 widthSrc, INT16 heightSrc, DWORD rop )
{
    return StretchBlt( HDC_32(hdcDst), xDst, yDst, widthDst, heightDst,
                       HDC_32(hdcSrc), xSrc, ySrc, widthSrc, heightSrc, rop );
}


/**********************************************************************
 *          Polygon  (GDI.36)
 */
BOOL16 WINAPI Polygon16( HDC16 hdc, const POINT16* pt, INT16 count )
{
    register int i;
    BOOL ret;
    LPPOINT pt32 = HeapAlloc( GetProcessHeap(), 0, count*sizeof(POINT) );

    if (!pt32) return FALSE;
    for (i=count;i--;)
    {
        pt32[i].x = pt[i].x;
        pt32[i].y = pt[i].y;
    }
    ret = Polygon(HDC_32(hdc),pt32,count);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/**********************************************************************
 *          Polyline  (GDI.37)
 */
BOOL16 WINAPI Polyline16( HDC16 hdc, const POINT16* pt, INT16 count )
{
    register int i;
    BOOL16 ret;
    LPPOINT pt32 = HeapAlloc( GetProcessHeap(), 0, count*sizeof(POINT) );

    if (!pt32) return FALSE;
    for (i=count;i--;)
    {
        pt32[i].x = pt[i].x;
        pt32[i].y = pt[i].y;
    }
    ret = Polyline(HDC_32(hdc),pt32,count);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/***********************************************************************
 *            Escape   (GDI.38)
 */
INT16 WINAPI Escape16( HDC16 hdc, INT16 escape, INT16 in_count, SEGPTR in_data, LPVOID out_data )
{
    INT ret;

    switch(escape)
    {
    /* Escape(hdc,CLIP_TO_PATH,LPINT16,NULL) */
    /* Escape(hdc,DRAFTMODE,LPINT16,NULL) */
    /* Escape(hdc,ENUMPAPERBINS,LPINT16,LPSTR); */
    /* Escape(hdc,EPSPRINTING,LPINT16,NULL) */
    /* Escape(hdc,EXT_DEVICE_CAPS,LPINT16,LPDWORD) */
    /* Escape(hdc,GETCOLORTABLE,LPINT16,LPDWORD) */
    /* Escape(hdc,MOUSETRAILS,LPINT16,NULL) */
    /* Escape(hdc,POSTSCRIPT_IGNORE,LPINT16,NULL) */
    /* Escape(hdc,QUERYESCSUPPORT,LPINT16,NULL) */
    /* Escape(hdc,SET_ARC_DIRECTION,LPINT16,NULL) */
    /* Escape(hdc,SET_POLY_MODE,LPINT16,NULL) */
    /* Escape(hdc,SET_SCREEN_ANGLE,LPINT16,NULL) */
    /* Escape(hdc,SET_SPREAD,LPINT16,NULL) */
    case CLIP_TO_PATH:
    case DRAFTMODE:
    case ENUMPAPERBINS:
    case EPSPRINTING:
    case EXT_DEVICE_CAPS:
    case GETCOLORTABLE:
    case MOUSETRAILS:
    case POSTSCRIPT_IGNORE:
    case QUERYESCSUPPORT:
    case SET_ARC_DIRECTION:
    case SET_POLY_MODE:
    case SET_SCREEN_ANGLE:
    case SET_SPREAD:
    {
        INT16 *ptr = MapSL(in_data);
        INT data = *ptr;
        return Escape( HDC_32(hdc), escape, sizeof(data), (LPCSTR)&data, out_data );
    }

    /* Escape(hdc,ENABLEDUPLEX,LPUINT16,NULL) */
    case ENABLEDUPLEX:
    {
        UINT16 *ptr = MapSL(in_data);
        UINT data = *ptr;
        return Escape( HDC_32(hdc), escape, sizeof(data), (LPCSTR)&data, NULL );
    }

    /* Escape(hdc,GETPHYSPAGESIZE,NULL,LPPOINT16) */
    /* Escape(hdc,GETPRINTINGOFFSET,NULL,LPPOINT16) */
    /* Escape(hdc,GETSCALINGFACTOR,NULL,LPPOINT16) */
    case GETPHYSPAGESIZE:
    case GETPRINTINGOFFSET:
    case GETSCALINGFACTOR:
    {
        POINT16 *ptr = out_data;
        POINT pt32;
        ret = Escape( HDC_32(hdc), escape, 0, NULL, &pt32 );
        ptr->x = pt32.x;
        ptr->y = pt32.y;
        return ret;
    }

    /* Escape(hdc,ENABLEPAIRKERNING,LPINT16,LPINT16); */
    /* Escape(hdc,ENABLERELATIVEWIDTHS,LPINT16,LPINT16); */
    /* Escape(hdc,SETCOPYCOUNT,LPINT16,LPINT16) */
    /* Escape(hdc,SETKERNTRACK,LPINT16,LPINT16) */
    /* Escape(hdc,SETLINECAP,LPINT16,LPINT16) */
    /* Escape(hdc,SETLINEJOIN,LPINT16,LPINT16) */
    /* Escape(hdc,SETMITERLIMIT,LPINT16,LPINT16) */
    case ENABLEPAIRKERNING:
    case ENABLERELATIVEWIDTHS:
    case SETCOPYCOUNT:
    case SETKERNTRACK:
    case SETLINECAP:
    case SETLINEJOIN:
    case SETMITERLIMIT:
    {
        INT16 *new = MapSL(in_data);
        INT16 *old = out_data;
        INT out, in = *new;
        ret = Escape( HDC_32(hdc), escape, sizeof(in), (LPCSTR)&in, &out );
        *old = out;
        return ret;
    }

    /* Escape(hdc,SETABORTPROC,ABORTPROC,NULL); */
    case SETABORTPROC:
        return SetAbortProc16( hdc, (ABORTPROC16)in_data );

    /* Escape(hdc,STARTDOC,LPSTR,LPDOCINFO16);
     * lpvOutData is actually a pointer to the DocInfo structure and used as
     * a second input parameter */
    case STARTDOC:
        if (out_data)
        {
            ret = StartDoc16( hdc, out_data );
            if (ret > 0) ret = StartPage( HDC_32(hdc) );
            return ret;
        }
        return Escape( HDC_32(hdc), escape, in_count, MapSL(in_data), NULL );

    /* Escape(hdc,SET_BOUNDS,LPRECT16,NULL); */
    /* Escape(hdc,SET_CLIP_BOX,LPRECT16,NULL); */
    case SET_BOUNDS:
    case SET_CLIP_BOX:
    {
        RECT16 *rc16 = MapSL(in_data);
        RECT rc;
        rc.left   = rc16->left;
        rc.top    = rc16->top;
        rc.right  = rc16->right;
        rc.bottom = rc16->bottom;
        return Escape( HDC_32(hdc), escape, sizeof(rc), (LPCSTR)&rc, NULL );
    }

    /* Escape(hdc,NEXTBAND,NULL,LPRECT16); */
    case NEXTBAND:
    {
        RECT rc;
        RECT16 *rc16 = out_data;
        ret = Escape( HDC_32(hdc), escape, 0, NULL, &rc );
        rc16->left   = rc.left;
        rc16->top    = rc.top;
        rc16->right  = rc.right;
        rc16->bottom = rc.bottom;
        return ret;
    }
    /* Escape(hdc,DRAWPATTERNRECT,PRECT_STRUCT*,NULL); */
    case DRAWPATTERNRECT:
    {
        DRAWPATRECT pr;
        DRAWPATRECT16 *pr16 = MapSL(in_data);

        pr.ptPosition.x = pr16->ptPosition.x;
        pr.ptPosition.y = pr16->ptPosition.y;
        pr.ptSize.x	= pr16->ptSize.x;
        pr.ptSize.y	= pr16->ptSize.y;
        pr.wStyle	= pr16->wStyle;
        pr.wPattern	= pr16->wPattern;
        return Escape( HDC_32(hdc), escape, sizeof(pr), (LPCSTR)&pr, NULL );
    }

    /* Escape(hdc,ABORTDOC,NULL,NULL); */
    /* Escape(hdc,BANDINFO,BANDINFOSTRUCT*,BANDINFOSTRUCT*); */
    /* Escape(hdc,BEGIN_PATH,NULL,NULL); */
    /* Escape(hdc,ENDDOC,NULL,NULL); */
    /* Escape(hdc,END_PATH,PATHINFO,NULL); */
    /* Escape(hdc,EXTTEXTOUT,EXTTEXT_STRUCT*,NULL); */
    /* Escape(hdc,FLUSHOUTPUT,NULL,NULL); */
    /* Escape(hdc,GETFACENAME,NULL,LPSTR); */
    /* Escape(hdc,GETPAIRKERNTABLE,NULL,KERNPAIR*); */
    /* Escape(hdc,GETSETPAPERBINS,BinInfo*,BinInfo*); */
    /* Escape(hdc,GETSETPRINTORIENT,ORIENT*,NULL); */
    /* Escape(hdc,GETSETSCREENPARAMS,SCREENPARAMS*,SCREENPARAMS*); */
    /* Escape(hdc,GETTECHNOLOGY,NULL,LPSTR); */
    /* Escape(hdc,GETTRACKKERNTABLE,NULL,KERNTRACK*); */
    /* Escape(hdc,MFCOMMENT,LPSTR,NULL); */
    /* Escape(hdc,NEWFRAME,NULL,NULL); */
    /* Escape(hdc,PASSTHROUGH,LPSTR,NULL); */
    /* Escape(hdc,RESTORE_CTM,NULL,NULL); */
    /* Escape(hdc,SAVE_CTM,NULL,NULL); */
    /* Escape(hdc,SETALLJUSTVALUES,EXTTEXTDATA*,NULL); */
    /* Escape(hdc,SETCOLORTABLE,COLORTABLE_STRUCT*,LPDWORD); */
    /* Escape(hdc,SET_BACKGROUND_COLOR,LPDWORD,LPDWORD); */
    /* Escape(hdc,TRANSFORM_CTM,LPSTR,NULL); */
    case ABORTDOC:
    case BANDINFO:
    case BEGIN_PATH:
    case ENDDOC:
    case END_PATH:
    case EXTTEXTOUT:
    case FLUSHOUTPUT:
    case GETFACENAME:
    case GETPAIRKERNTABLE:
    case GETSETPAPERBINS:
    case GETSETPRINTORIENT:
    case GETSETSCREENPARAMS:
    case GETTECHNOLOGY:
    case GETTRACKKERNTABLE:
    case MFCOMMENT:
    case NEWFRAME:
    case PASSTHROUGH:
    case RESTORE_CTM:
    case SAVE_CTM:
    case SETALLJUSTVALUES:
    case SETCOLORTABLE:
    case SET_BACKGROUND_COLOR:
    case TRANSFORM_CTM:
        /* pass it unmodified to the 32-bit function */
        return Escape( HDC_32(hdc), escape, in_count, MapSL(in_data), out_data );

    /* Escape(hdc,ENUMPAPERMETRICS,LPINT16,LPRECT16); */
    /* Escape(hdc,GETEXTENDEDTEXTMETRICS,LPUINT16,EXTTEXTMETRIC*); */
    /* Escape(hdc,GETEXTENTTABLE,LPSTR,LPINT16); */
    /* Escape(hdc,GETSETPAPERMETRICS,LPRECT16,LPRECT16); */
    /* Escape(hdc,GETVECTORBRUSHSIZE,LPLOGBRUSH16,LPPOINT16); */
    /* Escape(hdc,GETVECTORPENSIZE,LPLOGPEN16,LPPOINT16); */
    case ENUMPAPERMETRICS:
    case GETEXTENDEDTEXTMETRICS:
    case GETEXTENTTABLE:
    case GETSETPAPERMETRICS:
    case GETVECTORBRUSHSIZE:
    case GETVECTORPENSIZE:
    default:
        FIXME("unknown/unsupported 16-bit escape %x (%d,%p,%p\n",
              escape, in_count, MapSL(in_data), out_data );
        return Escape( HDC_32(hdc), escape, in_count, MapSL(in_data), out_data );
    }
}


/***********************************************************************
 *           RestoreDC    (GDI.39)
 */
BOOL16 WINAPI RestoreDC16( HDC16 hdc, INT16 level )
{
    return RestoreDC( HDC_32(hdc), level );
}


/***********************************************************************
 *           FillRgn    (GDI.40)
 */
BOOL16 WINAPI FillRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush )
{
    return FillRgn( HDC_32(hdc), HRGN_32(hrgn), HBRUSH_32(hbrush) );
}


/***********************************************************************
 *           FrameRgn     (GDI.41)
 */
BOOL16 WINAPI FrameRgn16( HDC16 hdc, HRGN16 hrgn, HBRUSH16 hbrush,
                          INT16 nWidth, INT16 nHeight )
{
    return FrameRgn( HDC_32(hdc), HRGN_32(hrgn), HBRUSH_32(hbrush), nWidth, nHeight );
}


/***********************************************************************
 *           InvertRgn    (GDI.42)
 */
BOOL16 WINAPI InvertRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return InvertRgn( HDC_32(hdc), HRGN_32(hrgn) );
}


/***********************************************************************
 *           PaintRgn    (GDI.43)
 */
BOOL16 WINAPI PaintRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return PaintRgn( HDC_32(hdc), HRGN_32(hrgn) );
}


/***********************************************************************
 *           SelectClipRgn    (GDI.44)
 */
INT16 WINAPI SelectClipRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return SelectClipRgn( HDC_32(hdc), HRGN_32(hrgn) );
}


/***********************************************************************
 *           SelectObject    (GDI.45)
 */
HGDIOBJ16 WINAPI SelectObject16( HDC16 hdc, HGDIOBJ16 handle )
{
    return HGDIOBJ_16( SelectObject( HDC_32(hdc), HGDIOBJ_32(handle) ) );
}


/***********************************************************************
 *           CombineRgn    (GDI.47)
 */
INT16 WINAPI CombineRgn16(HRGN16 hDest, HRGN16 hSrc1, HRGN16 hSrc2, INT16 mode)
{
    return CombineRgn( HRGN_32(hDest), HRGN_32(hSrc1), HRGN_32(hSrc2), mode );
}


/***********************************************************************
 *           CreateBitmap    (GDI.48)
 */
HBITMAP16 WINAPI CreateBitmap16( INT16 width, INT16 height, UINT16 planes,
                                 UINT16 bpp, LPCVOID bits )
{
    return HBITMAP_16( CreateBitmap( width, height, planes & 0xff, bpp & 0xff, bits ) );
}


/***********************************************************************
 *           CreateBitmapIndirect    (GDI.49)
 */
HBITMAP16 WINAPI CreateBitmapIndirect16( const BITMAP16 * bmp )
{
    return CreateBitmap16( bmp->bmWidth, bmp->bmHeight, bmp->bmPlanes,
                           bmp->bmBitsPixel, MapSL( bmp->bmBits ) );
}


/***********************************************************************
 *           CreateBrushIndirect    (GDI.50)
 */
HBRUSH16 WINAPI CreateBrushIndirect16( const LOGBRUSH16 * brush )
{
    LOGBRUSH brush32;

    if (brush->lbStyle == BS_DIBPATTERN || brush->lbStyle == BS_DIBPATTERN8X8)
        return CreateDIBPatternBrush16( brush->lbHatch, brush->lbColor );

    brush32.lbStyle = brush->lbStyle;
    brush32.lbColor = brush->lbColor;
    brush32.lbHatch = brush->lbHatch;
    return HBRUSH_16( CreateBrushIndirect(&brush32) );
}


/***********************************************************************
 *           CreateCompatibleBitmap    (GDI.51)
 */
HBITMAP16 WINAPI CreateCompatibleBitmap16( HDC16 hdc, INT16 width, INT16 height )
{
    return HBITMAP_16( CreateCompatibleBitmap( HDC_32(hdc), width, height ) );
}


/***********************************************************************
 *           CreateCompatibleDC    (GDI.52)
 */
HDC16 WINAPI CreateCompatibleDC16( HDC16 hdc )
{
    return HDC_16( CreateCompatibleDC( HDC_32(hdc) ) );
}


/***********************************************************************
 *           CreateDC    (GDI.53)
 */
HDC16 WINAPI CreateDC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                         const DEVMODEA *initData )
{
    return HDC_16( CreateDCA( driver, device, output, initData ) );
}


/***********************************************************************
 *           CreateEllipticRgn    (GDI.54)
 */
HRGN16 WINAPI CreateEllipticRgn16( INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    return HRGN_16( CreateEllipticRgn( left, top, right, bottom ) );
}


/***********************************************************************
 *           CreateEllipticRgnIndirect    (GDI.55)
 */
HRGN16 WINAPI CreateEllipticRgnIndirect16( const RECT16 *rect )
{
    return HRGN_16( CreateEllipticRgn( rect->left, rect->top, rect->right, rect->bottom ) );
}


/***********************************************************************
 *           CreateFont    (GDI.56)
 */
HFONT16 WINAPI CreateFont16(INT16 height, INT16 width, INT16 esc, INT16 orient,
                            INT16 weight, BYTE italic, BYTE underline,
                            BYTE strikeout, BYTE charset, BYTE outpres,
                            BYTE clippres, BYTE quality, BYTE pitch,
                            LPCSTR name )
{
    return HFONT_16( CreateFontA( height, width, esc, orient, weight, italic, underline,
                                  strikeout, charset, outpres, clippres, quality, pitch, name ));
}

/***********************************************************************
 *           CreateFontIndirect   (GDI.57)
 */
HFONT16 WINAPI CreateFontIndirect16( const LOGFONT16 *plf16 )
{
    HFONT ret;

    if (plf16)
    {
        LOGFONTW lfW;
        logfont_16_to_W( plf16, &lfW );
        ret = CreateFontIndirectW( &lfW );
    }
    else ret = CreateFontIndirectW( NULL );
    return HFONT_16(ret);
}


/***********************************************************************
 *           CreateHatchBrush    (GDI.58)
 */
HBRUSH16 WINAPI CreateHatchBrush16( INT16 style, COLORREF color )
{
    return HBRUSH_16( CreateHatchBrush( style, color ) );
}


/***********************************************************************
 *           CreatePatternBrush    (GDI.60)
 */
HBRUSH16 WINAPI CreatePatternBrush16( HBITMAP16 hbitmap )
{
    return HBRUSH_16( CreatePatternBrush( HBITMAP_32(hbitmap) ));
}


/***********************************************************************
 *           CreatePen    (GDI.61)
 */
HPEN16 WINAPI CreatePen16( INT16 style, INT16 width, COLORREF color )
{
    LOGPEN logpen;

    logpen.lopnStyle = style;
    logpen.lopnWidth.x = width;
    logpen.lopnWidth.y = 0;
    logpen.lopnColor = color;
    return HPEN_16( CreatePenIndirect( &logpen ) );
}


/***********************************************************************
 *           CreatePenIndirect    (GDI.62)
 */
HPEN16 WINAPI CreatePenIndirect16( const LOGPEN16 * pen )
{
    LOGPEN logpen;

    if (pen->lopnStyle > PS_INSIDEFRAME) return 0;
    logpen.lopnStyle   = pen->lopnStyle;
    logpen.lopnWidth.x = pen->lopnWidth.x;
    logpen.lopnWidth.y = pen->lopnWidth.y;
    logpen.lopnColor   = pen->lopnColor;
    return HPEN_16( CreatePenIndirect( &logpen ) );
}


/***********************************************************************
 *           CreatePolygonRgn    (GDI.63)
 */
HRGN16 WINAPI CreatePolygonRgn16( const POINT16 * points, INT16 count, INT16 mode )
{
    return CreatePolyPolygonRgn16( points, &count, 1, mode );
}


/***********************************************************************
 *           CreateRectRgn    (GDI.64)
 *
 * NOTE: cf. SetRectRgn16
 */
HRGN16 WINAPI CreateRectRgn16( INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    HRGN hrgn;

    if (left < right) hrgn = CreateRectRgn( left, top, right, bottom );
    else hrgn = CreateRectRgn( 0, 0, 0, 0 );
    return HRGN_16(hrgn);
}


/***********************************************************************
 *           CreateRectRgnIndirect    (GDI.65)
 */
HRGN16 WINAPI CreateRectRgnIndirect16( const RECT16* rect )
{
    return CreateRectRgn16( rect->left, rect->top, rect->right, rect->bottom );
}


/***********************************************************************
 *           CreateSolidBrush    (GDI.66)
 */
HBRUSH16 WINAPI CreateSolidBrush16( COLORREF color )
{
    return HBRUSH_16( CreateSolidBrush( color ) );
}


/***********************************************************************
 *           DeleteDC    (GDI.68)
 */
BOOL16 WINAPI DeleteDC16( HDC16 hdc )
{
    return DeleteDC( HDC_32(hdc) );
}


/***********************************************************************
 *           DeleteObject    (GDI.69)
 *           SysDeleteObject (GDI.605)
 */
BOOL16 WINAPI DeleteObject16( HGDIOBJ16 obj )
{
    return DeleteObject( HGDIOBJ_32(obj) );
}


/***********************************************************************
 *           EnumFonts      (GDI.70)
 */
INT16 WINAPI EnumFonts16( HDC16 hDC, LPCSTR lpName, FONTENUMPROC16 efproc,
                          LPARAM lpData )
{
    return EnumFontFamilies16( hDC, lpName, efproc, lpData );
}


/***********************************************************************
 *           EnumObjects    (GDI.71)
 */
INT16 WINAPI EnumObjects16( HDC16 hdc, INT16 obj, GOBJENUMPROC16 proc, LPARAM lParam )
{
    struct callback16_info info;

    info.proc  = (FARPROC16)proc;
    info.param = lParam;
    switch(obj)
    {
    case OBJ_PEN:
        return EnumObjects( HDC_32(hdc), OBJ_PEN, enum_pens_callback, (LPARAM)&info );
    case OBJ_BRUSH:
        return EnumObjects( HDC_32(hdc), OBJ_BRUSH, enum_brushes_callback, (LPARAM)&info );
    }
    return 0;
}


/***********************************************************************
 *           EqualRgn    (GDI.72)
 */
BOOL16 WINAPI EqualRgn16( HRGN16 rgn1, HRGN16 rgn2 )
{
    return EqualRgn( HRGN_32(rgn1), HRGN_32(rgn2) );
}


/***********************************************************************
 *           GetBitmapBits    (GDI.74)
 */
LONG WINAPI GetBitmapBits16( HBITMAP16 hbitmap, LONG count, LPVOID buffer )
{
    return GetBitmapBits( HBITMAP_32(hbitmap), count, buffer );
}


/***********************************************************************
 *		GetBkColor (GDI.75)
 */
COLORREF WINAPI GetBkColor16( HDC16 hdc )
{
    return GetBkColor( HDC_32(hdc) );
}


/***********************************************************************
 *		GetBkMode (GDI.76)
 */
INT16 WINAPI GetBkMode16( HDC16 hdc )
{
    return GetBkMode( HDC_32(hdc) );
}


/***********************************************************************
 *           GetClipBox    (GDI.77)
 */
INT16 WINAPI GetClipBox16( HDC16 hdc, LPRECT16 rect )
{
    RECT rect32;
    INT ret = GetClipBox( HDC_32(hdc), &rect32 );

    if (ret != ERROR)
    {
        rect->left   = rect32.left;
        rect->top    = rect32.top;
        rect->right  = rect32.right;
        rect->bottom = rect32.bottom;
    }
    return ret;
}


/***********************************************************************
 *		GetCurrentPosition (GDI.78)
 */
DWORD WINAPI GetCurrentPosition16( HDC16 hdc )
{
    POINT pt32;
    if (!GetCurrentPositionEx( HDC_32(hdc), &pt32 )) return 0;
    return MAKELONG( pt32.x, pt32.y );
}


/***********************************************************************
 *           GetDCOrg    (GDI.79)
 */
DWORD WINAPI GetDCOrg16( HDC16 hdc )
{
    POINT pt;
    if (GetDCOrgEx( HDC_32(hdc), &pt )) return MAKELONG( pt.x, pt.y );
    return 0;
}


/***********************************************************************
 *           GetDeviceCaps    (GDI.80)
 */
INT16 WINAPI GetDeviceCaps16( HDC16 hdc, INT16 cap )
{
    INT16 ret = GetDeviceCaps( HDC_32(hdc), cap );
    /* some apps don't expect -1 and think it's a B&W screen */
    if ((cap == NUMCOLORS) && (ret == -1)) ret = 2048;
    return ret;
}


/***********************************************************************
 *		GetMapMode (GDI.81)
 */
INT16 WINAPI GetMapMode16( HDC16 hdc )
{
    return GetMapMode( HDC_32(hdc) );
}


/***********************************************************************
 *           GetObject    (GDI.82)
 */
INT16 WINAPI GetObject16( HGDIOBJ16 handle16, INT16 count, LPVOID buffer )
{
    HGDIOBJ handle = HGDIOBJ_32( handle16 );
    switch( GetObjectType( handle ))
    {
    case OBJ_PEN:
        if (buffer)
        {
            LOGPEN16 *pen16 = buffer;
            LOGPEN pen;

            if (count < sizeof(LOGPEN16)) return 0;
            if (!GetObjectW( handle, sizeof(pen), &pen )) return 0;

            pen16->lopnStyle   = pen.lopnStyle;
            pen16->lopnColor   = pen.lopnColor;
            pen16->lopnWidth.x = pen.lopnWidth.x;
            pen16->lopnWidth.y = pen.lopnWidth.y;
        }
        return sizeof(LOGPEN16);

    case OBJ_BRUSH:
        if (buffer)
        {
            LOGBRUSH brush;
            LOGBRUSH16 brush16;

            if (!GetObjectW( handle, sizeof(brush), &brush )) return 0;
            brush16.lbStyle = brush.lbStyle;
            brush16.lbColor = brush.lbColor;
            brush16.lbHatch = brush.lbHatch;
            if (count > sizeof(brush16)) count = sizeof(brush16);
            memcpy( buffer, &brush16, count );
            return count;
        }
        return sizeof(LOGBRUSH16);

    case OBJ_PAL:
        return GetObjectW( handle, count, buffer );

    case OBJ_FONT:
        if (buffer)
        {
            LOGFONTW font;
            LOGFONT16 font16;

            if (!GetObjectW( handle, sizeof(font), &font )) return 0;
            logfont_W_to_16( &font, &font16 );
            if (count > sizeof(font16)) count = sizeof(font16);
            memcpy( buffer, &font16, count );
            return count;
        }
        return sizeof(LOGFONT16);

    case OBJ_BITMAP:
        {
            DIBSECTION dib;
            INT size;
            BITMAP16 *bmp16 = buffer;

            if (!(size = GetObjectW( handle, sizeof(dib), &dib ))) return 0;
            if (size == sizeof(DIBSECTION) && count > sizeof(BITMAP16))
            {
                FIXME("not implemented for DIBs: count %d\n", count);
                return 0;
            }
            else
            {
                if (count < sizeof(BITMAP16)) return 0;
                bmp16->bmType       = dib.dsBm.bmType;
                bmp16->bmWidth      = dib.dsBm.bmWidth;
                bmp16->bmHeight     = dib.dsBm.bmHeight;
                bmp16->bmWidthBytes = dib.dsBm.bmWidthBytes;
                bmp16->bmPlanes     = dib.dsBm.bmPlanes;
                bmp16->bmBitsPixel  = dib.dsBm.bmBitsPixel;
                bmp16->bmBits       = 0;
                return sizeof(BITMAP16);
            }
        }

    default:
        return 0;
    }
}


/***********************************************************************
 *           GetPixel    (GDI.83)
 */
COLORREF WINAPI GetPixel16( HDC16 hdc, INT16 x, INT16 y )
{
    return GetPixel( HDC_32(hdc), x, y );
}


/***********************************************************************
 *		GetPolyFillMode (GDI.84)
 */
INT16 WINAPI GetPolyFillMode16( HDC16 hdc )
{
    return GetPolyFillMode( HDC_32(hdc) );
}


/***********************************************************************
 *		GetROP2 (GDI.85)
 */
INT16 WINAPI GetROP216( HDC16 hdc )
{
    return GetROP2( HDC_32(hdc) );
}


/***********************************************************************
 *		GetRelAbs (GDI.86)
 */
INT16 WINAPI GetRelAbs16( HDC16 hdc )
{
    return GetRelAbs( HDC_32(hdc), 0 );
}


/***********************************************************************
 *           GetStockObject    (GDI.87)
 */
HGDIOBJ16 WINAPI GetStockObject16( INT16 obj )
{
    return HGDIOBJ_16( GetStockObject( obj ) );
}


/***********************************************************************
 *		GetStretchBltMode (GDI.88)
 */
INT16 WINAPI GetStretchBltMode16( HDC16 hdc )
{
    return GetStretchBltMode( HDC_32(hdc) );
}


/***********************************************************************
 *           GetTextCharacterExtra    (GDI.89)
 */
INT16 WINAPI GetTextCharacterExtra16( HDC16 hdc )
{
    return GetTextCharacterExtra( HDC_32(hdc) );
}


/***********************************************************************
 *		GetTextColor (GDI.90)
 */
COLORREF WINAPI GetTextColor16( HDC16 hdc )
{
    return GetTextColor( HDC_32(hdc) );
}


/***********************************************************************
 *           GetTextExtent    (GDI.91)
 */
DWORD WINAPI GetTextExtent16( HDC16 hdc, LPCSTR str, INT16 count )
{
    SIZE size;
    if (!GetTextExtentPoint32A( HDC_32(hdc), str, count, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           GetTextFace    (GDI.92)
 */
INT16 WINAPI GetTextFace16( HDC16 hdc, INT16 count, LPSTR name )
{
    return GetTextFaceA( HDC_32(hdc), count, name );
}


/***********************************************************************
 *           GetTextMetrics    (GDI.93)
 */
BOOL16 WINAPI GetTextMetrics16( HDC16 hdc, TEXTMETRIC16 *tm )
{
    TEXTMETRICW tm32;

    if (!GetTextMetricsW( HDC_32(hdc), &tm32 )) return FALSE;

    tm->tmHeight           = tm32.tmHeight;
    tm->tmAscent           = tm32.tmAscent;
    tm->tmDescent          = tm32.tmDescent;
    tm->tmInternalLeading  = tm32.tmInternalLeading;
    tm->tmExternalLeading  = tm32.tmExternalLeading;
    tm->tmAveCharWidth     = tm32.tmAveCharWidth;
    tm->tmMaxCharWidth     = tm32.tmMaxCharWidth;
    tm->tmWeight           = tm32.tmWeight;
    tm->tmOverhang         = tm32.tmOverhang;
    tm->tmDigitizedAspectX = tm32.tmDigitizedAspectX;
    tm->tmDigitizedAspectY = tm32.tmDigitizedAspectY;
    tm->tmFirstChar        = tm32.tmFirstChar;
    tm->tmLastChar         = tm32.tmLastChar;
    tm->tmDefaultChar      = tm32.tmDefaultChar;
    tm->tmBreakChar        = tm32.tmBreakChar;
    tm->tmItalic           = tm32.tmItalic;
    tm->tmUnderlined       = tm32.tmUnderlined;
    tm->tmStruckOut        = tm32.tmStruckOut;
    tm->tmPitchAndFamily   = tm32.tmPitchAndFamily;
    tm->tmCharSet          = tm32.tmCharSet;
    return TRUE;
}


/***********************************************************************
 *		GetViewportExt (GDI.94)
 */
DWORD WINAPI GetViewportExt16( HDC16 hdc )
{
    SIZE size;
    if (!GetViewportExtEx( HDC_32(hdc), &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *		GetViewportOrg (GDI.95)
 */
DWORD WINAPI GetViewportOrg16( HDC16 hdc )
{
    POINT pt;
    if (!GetViewportOrgEx( HDC_32(hdc), &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *		GetWindowExt (GDI.96)
 */
DWORD WINAPI GetWindowExt16( HDC16 hdc )
{
    SIZE size;
    if (!GetWindowExtEx( HDC_32(hdc), &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *		GetWindowOrg (GDI.97)
 */
DWORD WINAPI GetWindowOrg16( HDC16 hdc )
{
    POINT pt;
    if (!GetWindowOrgEx( HDC_32(hdc), &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}




/**********************************************************************
 *           LineDDA   (GDI.100)
 */
void WINAPI LineDDA16( INT16 nXStart, INT16 nYStart, INT16 nXEnd,
                       INT16 nYEnd, LINEDDAPROC16 proc, LPARAM lParam )
{
    struct callback16_info info;
    info.proc  = (FARPROC16)proc;
    info.param = lParam;
    LineDDA( nXStart, nYStart, nXEnd, nYEnd, linedda_callback, (LPARAM)&info );
}


/***********************************************************************
 *           OffsetRgn    (GDI.101)
 */
INT16 WINAPI OffsetRgn16( HRGN16 hrgn, INT16 x, INT16 y )
{
    return OffsetRgn( HRGN_32(hrgn), x, y );
}


/***********************************************************************
 *           PtVisible    (GDI.103)
 */
BOOL16 WINAPI PtVisible16( HDC16 hdc, INT16 x, INT16 y )
{
    return PtVisible( HDC_32(hdc), x, y );
}


/***********************************************************************
 *           SelectVisRgn   (GDI.105)
 */
INT16 WINAPI SelectVisRgn16( HDC16 hdc, HRGN16 hrgn )
{
    return SelectVisRgn( HDC_32(hdc), HRGN_32(hrgn) );
}


/***********************************************************************
 *           SetBitmapBits    (GDI.106)
 */
LONG WINAPI SetBitmapBits16( HBITMAP16 hbitmap, LONG count, LPCVOID buffer )
{
    return SetBitmapBits( HBITMAP_32(hbitmap), count, buffer );
}


/***********************************************************************
 *           AddFontResource    (GDI.119)
 */
INT16 WINAPI AddFontResource16( LPCSTR filename )
{
    return AddFontResourceA( filename );
}


/***********************************************************************
 *           Death    (GDI.121)
 *
 * Disables GDI, switches back to text mode.
 * We don't have to do anything here,
 * just let console support handle everything
 */
void WINAPI Death16(HDC16 hdc)
{
    MESSAGE("Death(%04x) called. Application enters text mode...\n", hdc);
}


/***********************************************************************
 *           Resurrection    (GDI.122)
 *
 * Restores GDI functionality
 */
void WINAPI Resurrection16(HDC16 hdc,
                           WORD w1, WORD w2, WORD w3, WORD w4, WORD w5, WORD w6)
{
    MESSAGE("Resurrection(%04x, %04x, %04x, %04x, %04x, %04x, %04x) called. Application left text mode.\n",
            hdc, w1, w2, w3, w4, w5, w6);
}


/**********************************************************************
 *	     CreateMetaFile     (GDI.125)
 */
HDC16 WINAPI CreateMetaFile16( LPCSTR filename )
{
    return HDC_16( CreateMetaFileA( filename ) );
}


/***********************************************************************
 *           MulDiv   (GDI.128)
 */
INT16 WINAPI MulDiv16( INT16 nMultiplicand, INT16 nMultiplier, INT16 nDivisor)
{
    INT ret;
    if (!nDivisor) return -32768;
    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }
    /* If the result is positive, we "add" to round. else,
     * we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
         ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
        ret = (((int)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
        ret = (((int)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;
    if ((ret > 32767) || (ret < -32767)) return -32768;
    return (INT16) ret;
}


/***********************************************************************
 *           GetRgnBox    (GDI.134)
 */
INT16 WINAPI GetRgnBox16( HRGN16 hrgn, LPRECT16 rect )
{
    RECT r;
    INT16 ret = GetRgnBox( HRGN_32(hrgn), &r );
    rect->left   = r.left;
    rect->top    = r.top;
    rect->right  = r.right;
    rect->bottom = r.bottom;
    return ret;
}


/***********************************************************************
 *           RemoveFontResource    (GDI.136)
 */
BOOL16 WINAPI RemoveFontResource16( LPCSTR str )
{
    return RemoveFontResourceA(str);
}


/***********************************************************************
 *           SetBrushOrg    (GDI.148)
 */
DWORD WINAPI SetBrushOrg16( HDC16 hdc, INT16 x, INT16 y )
{
    POINT pt;

    if (!SetBrushOrgEx( HDC_32(hdc), x, y, &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *		GetBrushOrg (GDI.149)
 */
DWORD WINAPI GetBrushOrg16( HDC16 hdc )
{
    POINT pt;
    if (!GetBrushOrgEx( HDC_32(hdc), &pt )) return 0;
    return MAKELONG( pt.x, pt.y );
}


/***********************************************************************
 *           UnrealizeObject    (GDI.150)
 */
BOOL16 WINAPI UnrealizeObject16( HGDIOBJ16 obj )
{
    return UnrealizeObject( HGDIOBJ_32(obj) );
}


/***********************************************************************
 *           CreateIC    (GDI.153)
 */
HDC16 WINAPI CreateIC16( LPCSTR driver, LPCSTR device, LPCSTR output,
                         const DEVMODEA* initData )
{
    return HDC_16( CreateICA( driver, device, output, initData ) );
}


/***********************************************************************
 *           GetNearestColor   (GDI.154)
 */
COLORREF WINAPI GetNearestColor16( HDC16 hdc, COLORREF color )
{
    return GetNearestColor( HDC_32(hdc), color );
}


/***********************************************************************
 *           CreateDiscardableBitmap    (GDI.156)
 */
HBITMAP16 WINAPI CreateDiscardableBitmap16( HDC16 hdc, INT16 width, INT16 height )
{
    return HBITMAP_16( CreateDiscardableBitmap( HDC_32(hdc), width, height ) );
}


/***********************************************************************
 *           PtInRegion    (GDI.161)
 */
BOOL16 WINAPI PtInRegion16( HRGN16 hrgn, INT16 x, INT16 y )
{
    return PtInRegion( HRGN_32(hrgn), x, y );
}


/***********************************************************************
 *           GetBitmapDimension    (GDI.162)
 */
DWORD WINAPI GetBitmapDimension16( HBITMAP16 hbitmap )
{
    SIZE16 size;
    if (!GetBitmapDimensionEx16( hbitmap, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetBitmapDimension    (GDI.163)
 */
DWORD WINAPI SetBitmapDimension16( HBITMAP16 hbitmap, INT16 x, INT16 y )
{
    SIZE16 size;
    if (!SetBitmapDimensionEx16( hbitmap, x, y, &size )) return 0;
    return MAKELONG( size.cx, size.cy );
}


/***********************************************************************
 *           SetRectRgn    (GDI.172)
 *
 * NOTE: Win 3.1 sets region to empty if left > right
 */
void WINAPI SetRectRgn16( HRGN16 hrgn, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    if (left < right) SetRectRgn( HRGN_32(hrgn), left, top, right, bottom );
    else SetRectRgn( HRGN_32(hrgn), 0, 0, 0, 0 );
}


/******************************************************************
 *             PlayMetaFileRecord   (GDI.176)
 */
void WINAPI PlayMetaFileRecord16( HDC16 hdc, HANDLETABLE16 *ht, METARECORD *mr, UINT16 handles )
{
    HANDLETABLE *ht32 = HeapAlloc( GetProcessHeap(), 0, handles * sizeof(*ht32) );
    unsigned int i;

    for (i = 0; i < handles; i++) ht32->objectHandle[i] = HGDIOBJ_32(ht->objectHandle[i]);
    PlayMetaFileRecord( HDC_32(hdc), ht32, mr, handles );
    for (i = 0; i < handles; i++) ht->objectHandle[i] = HGDIOBJ_16(ht32->objectHandle[i]);
    HeapFree( GetProcessHeap(), 0, ht32 );
}


/***********************************************************************
 *           SetHookFlags   (GDI.192)
 */
WORD WINAPI SetHookFlags16( HDC16 hdc, WORD flags )
{
    return SetHookFlags( HDC_32(hdc), flags );
}


/***********************************************************************
 *           SetBoundsRect    (GDI.193)
 */
UINT16 WINAPI SetBoundsRect16( HDC16 hdc, const RECT16* rect, UINT16 flags )
{
    if (rect)
    {
        RECT rect32;
        rect32.left   = rect->left;
        rect32.top    = rect->top;
        rect32.right  = rect->right;
        rect32.bottom = rect->bottom;
        return SetBoundsRect( HDC_32( hdc ), &rect32, flags );
    }
    else return SetBoundsRect( HDC_32( hdc ), NULL, flags );
}


/***********************************************************************
 *           GetBoundsRect    (GDI.194)
 */
UINT16 WINAPI GetBoundsRect16( HDC16 hdc, LPRECT16 rect, UINT16 flags)
{
    RECT rect32;
    UINT ret = GetBoundsRect( HDC_32( hdc ), &rect32, flags );
    if (rect)
    {
        rect->left   = rect32.left;
        rect->top    = rect32.top;
        rect->right  = rect32.right;
        rect->bottom = rect32.bottom;
    }
    return ret;
}


/***********************************************************************
 *		EngineEnumerateFont (GDI.300)
 */
WORD WINAPI EngineEnumerateFont16(LPSTR fontname, FARPROC16 proc, DWORD data )
{
    FIXME("(%s,%p,%x),stub\n",fontname,proc,data);
    return 0;
}


/***********************************************************************
 *		EngineDeleteFont (GDI.301)
 */
WORD WINAPI EngineDeleteFont16(LPFONTINFO16 lpFontInfo)
{
    WORD handle;

    /*	untested, don't know if it works.
	We seem to access some structure that is located after the
	FONTINFO. The FONTINFO documentation says that there may
	follow some char-width table or font bitmap or vector info.
	I think it is some kind of font bitmap that begins at offset 0x52,
	as FONTINFO goes up to 0x51.
	If this is correct, everything should be implemented correctly.
    */
    if ( ((lpFontInfo->dfType & (RASTER_FONTTYPE|DEVICE_FONTTYPE)) == (RASTER_FONTTYPE|DEVICE_FONTTYPE))
         && (LOWORD(lpFontInfo->dfFace) == LOWORD(lpFontInfo)+0x6e)
         && (handle = *(WORD *)(lpFontInfo+0x54)) )
    {
        *(WORD *)(lpFontInfo+0x54) = 0;
        GlobalFree16(handle);
    }
    return 1;
}


/***********************************************************************
 *		EngineRealizeFont (GDI.302)
 */
WORD WINAPI EngineRealizeFont16(LPLOGFONT16 lplogFont, LPTEXTXFORM16 lptextxform, LPFONTINFO16 lpfontInfo)
{
    FIXME("(%p,%p,%p),stub\n",lplogFont,lptextxform,lpfontInfo);

    return 0;
}


/***********************************************************************
 *		EngineRealizeFontExt (GDI.315)
 */
WORD WINAPI EngineRealizeFontExt16(LONG l1, LONG l2, LONG l3, LONG l4)
{
    FIXME("(%08x,%08x,%08x,%08x),stub\n",l1,l2,l3,l4);

    return 0;
}


/***********************************************************************
 *		EngineGetCharWidth (GDI.303)
 */
WORD WINAPI EngineGetCharWidth16(LPFONTINFO16 lpFontInfo, BYTE firstChar, BYTE lastChar, LPINT16 buffer)
{
    int i;

    for (i = firstChar; i <= lastChar; i++)
       FIXME(" returns font's average width for range %d to %d\n", firstChar, lastChar);
    *buffer++ = lpFontInfo->dfAvgWidth; /* insert some charwidth functionality here; use average width for now */
    return 1;
}


/***********************************************************************
 *		EngineSetFontContext (GDI.304)
 */
WORD WINAPI EngineSetFontContext16(LPFONTINFO16 lpFontInfo, WORD data)
{
   FIXME("stub?\n");
   return 0;
}

/***********************************************************************
 *		EngineGetGlyphBMP (GDI.305)
 */
WORD WINAPI EngineGetGlyphBMP16(WORD word, LPFONTINFO16 lpFontInfo, WORD w1, WORD w2,
                              LPSTR string, DWORD dword, /*LPBITMAPMETRICS16*/ LPVOID metrics)
{
    FIXME("stub?\n");
    return 0;
}


/***********************************************************************
 *		EngineMakeFontDir (GDI.306)
 */
DWORD WINAPI EngineMakeFontDir16(HDC16 hdc, LPFONTDIR16 fontdir, LPCSTR string)
{
    FIXME(" stub! (always fails)\n");
    return ~0UL; /* error */
}


/***********************************************************************
 *           GetCharABCWidths   (GDI.307)
 */
BOOL16 WINAPI GetCharABCWidths16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar, LPABC16 abc )
{
    BOOL ret;
    UINT i;
    LPABC abc32 = HeapAlloc( GetProcessHeap(), 0, sizeof(ABC) * (lastChar - firstChar + 1) );

    if ((ret = GetCharABCWidthsA( HDC_32(hdc), firstChar, lastChar, abc32 )))
    {
        for (i = firstChar; i <= lastChar; i++)
        {
            abc[i-firstChar].abcA = abc32[i-firstChar].abcA;
            abc[i-firstChar].abcB = abc32[i-firstChar].abcB;
            abc[i-firstChar].abcC = abc32[i-firstChar].abcC;
        }
    }
    HeapFree( GetProcessHeap(), 0, abc32 );
    return ret;
}


/***********************************************************************
 *           GetOutlineTextMetrics (GDI.308)
 *
 * Gets metrics for TrueType fonts.
 *
 * PARAMS
 *    hdc    [In]  Handle of device context
 *    cbData [In]  Size of metric data array
 *    lpOTM  [Out] Address of metric data array
 *
 * RETURNS
 *    Success: Non-zero or size of required buffer
 *    Failure: 0
 *
 * NOTES
 *    lpOTM should be LPOUTLINETEXTMETRIC
 */
UINT16 WINAPI GetOutlineTextMetrics16( HDC16 hdc, UINT16 cbData,
                                       LPOUTLINETEXTMETRIC16 lpOTM )
{
    FIXME("(%04x,%04x,%p): stub\n", hdc,cbData,lpOTM);
    return 0;
}


/***********************************************************************
 *           GetGlyphOutline    (GDI.309)
 */
DWORD WINAPI GetGlyphOutline16( HDC16 hdc, UINT16 uChar, UINT16 fuFormat,
                                LPGLYPHMETRICS16 lpgm, DWORD cbBuffer,
                                LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    DWORD ret;
    GLYPHMETRICS gm32;

    ret = GetGlyphOutlineA( HDC_32(hdc), uChar, fuFormat, &gm32, cbBuffer, lpBuffer, lpmat2);
    if (ret && ret != GDI_ERROR)
    {
        lpgm->gmBlackBoxX = gm32.gmBlackBoxX;
        lpgm->gmBlackBoxY = gm32.gmBlackBoxY;
        lpgm->gmptGlyphOrigin.x = gm32.gmptGlyphOrigin.x;
        lpgm->gmptGlyphOrigin.y = gm32.gmptGlyphOrigin.y;
        lpgm->gmCellIncX = gm32.gmCellIncX;
        lpgm->gmCellIncY = gm32.gmCellIncY;
    }
    return ret;
}


/***********************************************************************
 *           CreateScalableFontResource   (GDI.310)
 */
BOOL16 WINAPI CreateScalableFontResource16( UINT16 fHidden, LPCSTR lpszResourceFile,
                                            LPCSTR fontFile, LPCSTR path )
{
    return CreateScalableFontResourceA( fHidden, lpszResourceFile, fontFile, path );
}


/*************************************************************************
 *             GetFontData    (GDI.311)
 *
 */
DWORD WINAPI GetFontData16( HDC16 hdc, DWORD table, DWORD offset, LPVOID buffer, DWORD count )
{
    return GetFontData( HDC_32(hdc), table, offset, buffer, count );
}


/*************************************************************************
 *             GetRasterizerCaps   (GDI.313)
 */
BOOL16 WINAPI GetRasterizerCaps16( LPRASTERIZER_STATUS lprs, UINT16 cbNumBytes )
{
    return GetRasterizerCaps( lprs, cbNumBytes );
}


/***********************************************************************
 *             EnumFontFamilies    (GDI.330)
 */
INT16 WINAPI EnumFontFamilies16( HDC16 hDC, LPCSTR lpFamily,
                                 FONTENUMPROC16 efproc, LPARAM lpData )
{
    LOGFONT16 lf, *plf;

    if (lpFamily)
    {
        if (!*lpFamily) return 1;
        lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        plf = &lf;
    }
    else plf = NULL;

    return EnumFontFamiliesEx16( hDC, plf, efproc, lpData, 0 );
}


/*************************************************************************
 *             GetKerningPairs   (GDI.332)
 *
 */
INT16 WINAPI GetKerningPairs16( HDC16 hdc, INT16 count, LPKERNINGPAIR16 pairs )
{
    KERNINGPAIR *pairs32;
    INT i, ret;

    if (!count) return 0;

    if (!(pairs32 = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*pairs32) ))) return 0;
    if ((ret = GetKerningPairsA( HDC_32(hdc), count, pairs32 )))
    {
        for (i = 0; i < ret; i++)
        {
            pairs->wFirst      = pairs32->wFirst;
            pairs->wSecond     = pairs32->wSecond;
            pairs->iKernAmount = pairs32->iKernAmount;
        }
    }
    HeapFree( GetProcessHeap(), 0, pairs32 );
    return ret;
}



/***********************************************************************
 *		GetTextAlign (GDI.345)
 */
UINT16 WINAPI GetTextAlign16( HDC16 hdc )
{
    return GetTextAlign( HDC_32(hdc) );
}


/***********************************************************************
 *           SetTextAlign    (GDI.346)
 */
UINT16 WINAPI SetTextAlign16( HDC16 hdc, UINT16 align )
{
    return SetTextAlign( HDC_32(hdc), align );
}


/***********************************************************************
 *           Chord    (GDI.348)
 */
BOOL16 WINAPI Chord16( HDC16 hdc, INT16 left, INT16 top,
                       INT16 right, INT16 bottom, INT16 xstart, INT16 ystart,
                       INT16 xend, INT16 yend )
{
    return Chord( HDC_32(hdc), left, top, right, bottom, xstart, ystart, xend, yend );
}


/***********************************************************************
 *           SetMapperFlags    (GDI.349)
 */
DWORD WINAPI SetMapperFlags16( HDC16 hdc, DWORD flags )
{
    return SetMapperFlags( HDC_32(hdc), flags );
}


/***********************************************************************
 *           GetCharWidth    (GDI.350)
 */
BOOL16 WINAPI GetCharWidth16( HDC16 hdc, UINT16 firstChar, UINT16 lastChar, LPINT16 buffer )
{
    BOOL retVal = FALSE;

    if( firstChar != lastChar )
    {
        LPINT buf32 = HeapAlloc(GetProcessHeap(), 0, sizeof(INT)*(1 + (lastChar - firstChar)));
        if( buf32 )
        {
            LPINT obuf32 = buf32;
            int i;

            retVal = GetCharWidth32A( HDC_32(hdc), firstChar, lastChar, buf32);
            if (retVal)
            {
                for (i = firstChar; i <= lastChar; i++) *buffer++ = *buf32++;
            }
            HeapFree(GetProcessHeap(), 0, obuf32);
        }
    }
    else /* happens quite often to warrant a special treatment */
    {
        INT chWidth;
        retVal = GetCharWidth32A( HDC_32(hdc), firstChar, lastChar, &chWidth );
        *buffer = chWidth;
    }
    return retVal;
}


/***********************************************************************
 *           ExtTextOut   (GDI.351)
 */
BOOL16 WINAPI ExtTextOut16( HDC16 hdc, INT16 x, INT16 y, UINT16 flags,
                            const RECT16 *lprect, LPCSTR str, UINT16 count,
                            const INT16 *lpDx )
{
    BOOL        ret;
    int         i;
    RECT        rect32;
    LPINT       lpdx32 = NULL;

    if (lpDx) {
        lpdx32 = HeapAlloc( GetProcessHeap(),0, sizeof(INT)*count );
        if(lpdx32 == NULL) return FALSE;
        for (i=count;i--;) lpdx32[i]=lpDx[i];
    }
    if (lprect)
    {
        rect32.left   = lprect->left;
        rect32.top    = lprect->top;
        rect32.right  = lprect->right;
        rect32.bottom = lprect->bottom;
    }
    ret = ExtTextOutA(HDC_32(hdc),x,y,flags,lprect?&rect32:NULL,str,count,lpdx32);
    HeapFree( GetProcessHeap(), 0, lpdx32 );
    return ret;
}


/***********************************************************************
 *           CreatePalette    (GDI.360)
 */
HPALETTE16 WINAPI CreatePalette16( const LOGPALETTE* palette )
{
    return HPALETTE_16( CreatePalette( palette ) );
}


/***********************************************************************
 *           GDISelectPalette   (GDI.361)
 */
HPALETTE16 WINAPI GDISelectPalette16( HDC16 hdc, HPALETTE16 hpalette, WORD wBkg )
{
    return HPALETTE_16( GDISelectPalette( HDC_32(hdc), HPALETTE_32(hpalette), wBkg ));
}


/***********************************************************************
 *           GDIRealizePalette   (GDI.362)
 */
UINT16 WINAPI GDIRealizePalette16( HDC16 hdc )
{
    return GDIRealizePalette( HDC_32(hdc) );
}


/***********************************************************************
 *           GetPaletteEntries    (GDI.363)
 */
UINT16 WINAPI GetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, LPPALETTEENTRY entries )
{
    return GetPaletteEntries( HPALETTE_32(hpalette), start, count, entries );
}


/***********************************************************************
 *           SetPaletteEntries    (GDI.364)
 */
UINT16 WINAPI SetPaletteEntries16( HPALETTE16 hpalette, UINT16 start,
                                   UINT16 count, const PALETTEENTRY *entries )
{
    return SetPaletteEntries( HPALETTE_32(hpalette), start, count, entries );
}


/**********************************************************************
 *            UpdateColors   (GDI.366)
 */
INT16 WINAPI UpdateColors16( HDC16 hdc )
{
    UpdateColors( HDC_32(hdc) );
    return TRUE;
}


/***********************************************************************
 *           AnimatePalette   (GDI.367)
 */
void WINAPI AnimatePalette16( HPALETTE16 hpalette, UINT16 StartIndex,
                              UINT16 NumEntries, const PALETTEENTRY* PaletteColors)
{
    AnimatePalette( HPALETTE_32(hpalette), StartIndex, NumEntries, PaletteColors );
}


/***********************************************************************
 *           ResizePalette   (GDI.368)
 */
BOOL16 WINAPI ResizePalette16( HPALETTE16 hpalette, UINT16 cEntries )
{
    return ResizePalette( HPALETTE_32(hpalette), cEntries );
}


/***********************************************************************
 *           GetNearestPaletteIndex   (GDI.370)
 */
UINT16 WINAPI GetNearestPaletteIndex16( HPALETTE16 hpalette, COLORREF color )
{
    return GetNearestPaletteIndex( HPALETTE_32(hpalette), color );
}


/**********************************************************************
 *          ExtFloodFill   (GDI.372)
 */
BOOL16 WINAPI ExtFloodFill16( HDC16 hdc, INT16 x, INT16 y, COLORREF color,
                              UINT16 fillType )
{
    return ExtFloodFill( HDC_32(hdc), x, y, color, fillType );
}


/***********************************************************************
 *           SetSystemPaletteUse   (GDI.373)
 */
UINT16 WINAPI SetSystemPaletteUse16( HDC16 hdc, UINT16 use )
{
    return SetSystemPaletteUse( HDC_32(hdc), use );
}


/***********************************************************************
 *           GetSystemPaletteUse   (GDI.374)
 */
UINT16 WINAPI GetSystemPaletteUse16( HDC16 hdc )
{
    return GetSystemPaletteUse( HDC_32(hdc) );
}


/***********************************************************************
 *           GetSystemPaletteEntries   (GDI.375)
 */
UINT16 WINAPI GetSystemPaletteEntries16( HDC16 hdc, UINT16 start, UINT16 count,
                                         LPPALETTEENTRY entries )
{
    return GetSystemPaletteEntries( HDC_32(hdc), start, count, entries );
}


/***********************************************************************
 *           ResetDC    (GDI.376)
 */
HDC16 WINAPI ResetDC16( HDC16 hdc, const DEVMODEA *devmode )
{
    return HDC_16( ResetDCA(HDC_32(hdc), devmode) );
}


/******************************************************************
 *           StartDoc   (GDI.377)
 */
INT16 WINAPI StartDoc16( HDC16 hdc, const DOCINFO16 *lpdoc )
{
    DOCINFOA docA;

    docA.cbSize = lpdoc->cbSize;
    docA.lpszDocName = MapSL(lpdoc->lpszDocName);
    docA.lpszOutput = MapSL(lpdoc->lpszOutput);
    if(lpdoc->cbSize > offsetof(DOCINFO16,lpszDatatype))
        docA.lpszDatatype = MapSL(lpdoc->lpszDatatype);
    else
        docA.lpszDatatype = NULL;
    if(lpdoc->cbSize > offsetof(DOCINFO16,fwType))
        docA.fwType = lpdoc->fwType;
    else
        docA.fwType = 0;
    return StartDocA( HDC_32(hdc), &docA );
}


/******************************************************************
 *           EndDoc   (GDI.378)
 */
INT16 WINAPI EndDoc16( HDC16 hdc )
{
    return EndDoc( HDC_32(hdc) );
}


/******************************************************************
 *           StartPage   (GDI.379)
 */
INT16 WINAPI StartPage16( HDC16 hdc )
{
    return StartPage( HDC_32(hdc) );
}


/******************************************************************
 *           EndPage   (GDI.380)
 */
INT16 WINAPI EndPage16( HDC16 hdc )
{
    return EndPage( HDC_32(hdc) );
}


/******************************************************************************
 *           AbortDoc   (GDI.382)
 */
INT16 WINAPI AbortDoc16( HDC16 hdc )
{
    return AbortDoc( HDC_32(hdc) );
}


/***********************************************************************
 *           FastWindowFrame    (GDI.400)
 */
BOOL16 WINAPI FastWindowFrame16( HDC16 hdc, const RECT16 *rect,
                               INT16 width, INT16 height, DWORD rop )
{
    HDC hdc32 = HDC_32(hdc);
    HBRUSH hbrush = SelectObject( hdc32, GetStockObject( GRAY_BRUSH ) );
    PatBlt( hdc32, rect->left, rect->top,
            rect->right - rect->left - width, height, rop );
    PatBlt( hdc32, rect->left, rect->top + height, width,
            rect->bottom - rect->top - height, rop );
    PatBlt( hdc32, rect->left + width, rect->bottom - 1,
            rect->right - rect->left - width, -height, rop );
    PatBlt( hdc32, rect->right - 1, rect->top, -width,
            rect->bottom - rect->top - height, rop );
    SelectObject( hdc32, hbrush );
    return TRUE;
}


/***********************************************************************
 *           GdiInit2     (GDI.403)
 *
 * See "Undocumented Windows"
 *
 * PARAMS
 *   h1 [I] GDI object
 *   h2 [I] global data
 */
HANDLE16 WINAPI GdiInit216( HANDLE16 h1, HANDLE16 h2 )
{
    FIXME("(%04x, %04x), stub.\n", h1, h2);
    if (h2 == 0xffff) return 0xffff; /* undefined return value */
    return h1; /* FIXME: should be the memory handle of h1 */
}


/***********************************************************************
 *           FinalGdiInit     (GDI.405)
 */
void WINAPI FinalGdiInit16( HBRUSH16 hPattern /* [in] fill pattern of desktop */ )
{
}


/***********************************************************************
 *           CreateUserBitmap    (GDI.407)
 */
HBITMAP16 WINAPI CreateUserBitmap16( INT16 width, INT16 height, UINT16 planes,
                                     UINT16 bpp, LPCVOID bits )
{
    return CreateBitmap16( width, height, planes, bpp, bits );
}


/***********************************************************************
 *           CreateUserDiscardableBitmap    (GDI.409)
 */
HBITMAP16 WINAPI CreateUserDiscardableBitmap16( WORD dummy, INT16 width, INT16 height )
{
    HDC hdc = CreateDCA( "DISPLAY", NULL, NULL, NULL );
    HBITMAP ret = CreateCompatibleBitmap( hdc, width, height );
    DeleteDC( hdc );
    return HBITMAP_16(ret);
}


/***********************************************************************
 *		GetCurLogFont (GDI.411)
 */
HFONT16 WINAPI GetCurLogFont16( HDC16 hdc )
{
    return HFONT_16( GetCurrentObject( HDC_32(hdc), OBJ_FONT ) );
}


/***********************************************************************
 *           StretchDIBits   (GDI.439)
 */
INT16 WINAPI StretchDIBits16( HDC16 hdc, INT16 xDst, INT16 yDst, INT16 widthDst,
                              INT16 heightDst, INT16 xSrc, INT16 ySrc, INT16 widthSrc,
                              INT16 heightSrc, const VOID *bits,
                              const BITMAPINFO *info, UINT16 wUsage, DWORD dwRop )
{
    return StretchDIBits( HDC_32(hdc), xDst, yDst, widthDst, heightDst,
                          xSrc, ySrc, widthSrc, heightSrc, bits,
                          info, wUsage, dwRop );
}


/***********************************************************************
 *           SetDIBits    (GDI.440)
 */
INT16 WINAPI SetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                          UINT16 lines, LPCVOID bits, const BITMAPINFO *info,
                          UINT16 coloruse )
{
    return SetDIBits( HDC_32(hdc), HBITMAP_32(hbitmap), startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           GetDIBits    (GDI.441)
 */
INT16 WINAPI GetDIBits16( HDC16 hdc, HBITMAP16 hbitmap, UINT16 startscan,
                          UINT16 lines, LPVOID bits, BITMAPINFO * info,
                          UINT16 coloruse )
{
    return GetDIBits( HDC_32(hdc), HBITMAP_32(hbitmap), startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           CreateDIBitmap    (GDI.442)
 */
HBITMAP16 WINAPI CreateDIBitmap16( HDC16 hdc, const BITMAPINFOHEADER * header,
                                   DWORD init, LPCVOID bits, const BITMAPINFO * data,
                                   UINT16 coloruse )
{
    return HBITMAP_16( CreateDIBitmap( HDC_32(hdc), header, init, bits, data, coloruse ) );
}


/***********************************************************************
 *           SetDIBitsToDevice    (GDI.443)
 */
INT16 WINAPI SetDIBitsToDevice16( HDC16 hdc, INT16 xDest, INT16 yDest, INT16 cx,
                                  INT16 cy, INT16 xSrc, INT16 ySrc, UINT16 startscan,
                                  UINT16 lines, LPCVOID bits, const BITMAPINFO *info,
                                  UINT16 coloruse )
{
    return SetDIBitsToDevice( HDC_32(hdc), xDest, yDest, cx, cy, xSrc, ySrc,
                              startscan, lines, bits, info, coloruse );
}


/***********************************************************************
 *           CreateRoundRectRgn    (GDI.444)
 *
 * If either ellipse dimension is zero we call CreateRectRgn16 for its
 * `special' behaviour. -ve ellipse dimensions can result in GPFs under win3.1
 * we just let CreateRoundRectRgn convert them to +ve values.
 */

HRGN16 WINAPI CreateRoundRectRgn16( INT16 left, INT16 top, INT16 right, INT16 bottom,
                                    INT16 ellipse_width, INT16 ellipse_height )
{
    if( ellipse_width == 0 || ellipse_height == 0 )
        return CreateRectRgn16( left, top, right, bottom );
    else
        return HRGN_16( CreateRoundRectRgn( left, top, right, bottom,
                                            ellipse_width, ellipse_height ));
}


/***********************************************************************
 *           CreateDIBPatternBrush    (GDI.445)
 */
HBRUSH16 WINAPI CreateDIBPatternBrush16( HGLOBAL16 hbitmap, UINT16 coloruse )
{
    BITMAPINFO *bmi;
    HBRUSH16 ret;

    if (!(bmi = GlobalLock16( hbitmap ))) return 0;
    ret = HBRUSH_16( CreateDIBPatternBrushPt( bmi, coloruse ));
    GlobalUnlock16( hbitmap );
    return ret;
}


/**********************************************************************
 *          PolyPolygon (GDI.450)
 */
BOOL16 WINAPI PolyPolygon16( HDC16 hdc, const POINT16* pt, const INT16* counts,
                             UINT16 polygons )
{
    int         i,nrpts;
    LPPOINT     pt32;
    LPINT       counts32;
    BOOL16      ret;

    nrpts=0;
    for (i=polygons;i--;)
        nrpts+=counts[i];
    pt32 = HeapAlloc( GetProcessHeap(), 0, sizeof(POINT)*nrpts);
    if(pt32 == NULL) return FALSE;
    for (i=nrpts;i--;)
    {
        pt32[i].x = pt[i].x;
        pt32[i].y = pt[i].y;
    }
    counts32 = HeapAlloc( GetProcessHeap(), 0, polygons*sizeof(INT) );
    if(counts32 == NULL) {
        HeapFree( GetProcessHeap(), 0, pt32 );
        return FALSE;
    }
    for (i=polygons;i--;) counts32[i]=counts[i];

    ret = PolyPolygon(HDC_32(hdc),pt32,counts32,polygons);
    HeapFree( GetProcessHeap(), 0, counts32 );
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/***********************************************************************
 *           CreatePolyPolygonRgn    (GDI.451)
 */
HRGN16 WINAPI CreatePolyPolygonRgn16( const POINT16 *points,
                                      const INT16 *count, INT16 nbpolygons, INT16 mode )
{
    HRGN hrgn;
    int i, npts = 0;
    INT *count32;
    POINT *points32;

    for (i = 0; i < nbpolygons; i++) npts += count[i];
    points32 = HeapAlloc( GetProcessHeap(), 0, npts * sizeof(POINT) );
    for (i = 0; i < npts; i++)
    {
        points32[i].x = points[i].x;
        points32[i].y = points[i].y;
    }

    count32 = HeapAlloc( GetProcessHeap(), 0, nbpolygons * sizeof(INT) );
    for (i = 0; i < nbpolygons; i++) count32[i] = count[i];
    hrgn = CreatePolyPolygonRgn( points32, count32, nbpolygons, mode );
    HeapFree( GetProcessHeap(), 0, count32 );
    HeapFree( GetProcessHeap(), 0, points32 );
    return HRGN_16(hrgn);
}


/***********************************************************************
 *           GdiSeeGdiDo   (GDI.452)
 */
DWORD WINAPI GdiSeeGdiDo16( WORD wReqType, WORD wParam1, WORD wParam2,
                          WORD wParam3 )
{
    DWORD ret = ~0U;

    switch (wReqType)
    {
    case 0x0001:  /* LocalAlloc */
        WARN("LocalAlloc16(%x, %x): ignoring\n", wParam1, wParam3);
        ret = 0;
        break;
    case 0x0002:  /* LocalFree */
        WARN("LocalFree16(%x): ignoring\n", wParam1);
        ret = 0;
        break;
    case 0x0003:  /* LocalCompact */
        WARN("LocalCompact16(%x): ignoring\n", wParam3);
        ret = 65000; /* lie about the amount of free space */
        break;
    case 0x0103:  /* LocalHeap */
        WARN("LocalHeap16(): ignoring\n");
        break;
    default:
        WARN("(wReqType=%04x): Unknown\n", wReqType);
        break;
    }
    return ret;
}


/***********************************************************************
 *           SetObjectOwner    (GDI.461)
 */
void WINAPI SetObjectOwner16( HGDIOBJ16 handle, HANDLE16 owner )
{
    /* Nothing to do */
}


/***********************************************************************
 *           IsGDIObject    (GDI.462)
 *
 * returns type of object if valid (W95 system programming secrets p. 264-5)
 */
BOOL16 WINAPI IsGDIObject16( HGDIOBJ16 handle16 )
{
    static const BYTE type_map[] =
    {
        0,  /* bad */
        1,  /* OBJ_PEN */
        2,  /* OBJ_BRUSH */
        7,  /* OBJ_DC */
        9,  /* OBJ_METADC */
        4,  /* OBJ_PAL */
        3,  /* OBJ_FONT */
        5,  /* OBJ_BITMAP */
        6,  /* OBJ_REGION */
        10, /* OBJ_METAFILE */
        7,  /* OBJ_MEMDC */
        0,  /* OBJ_EXTPEN */
        9,  /* OBJ_ENHMETADC */
        12, /* OBJ_ENHMETAFILE */
        0   /* OBJ_COLORSPACE */
    };

    UINT type = GetObjectType( HGDIOBJ_32( handle16 ));

    if (type >= sizeof(type_map)/sizeof(type_map[0])) return 0;
    return type_map[type];
}


/***********************************************************************
 *           RectVisible    (GDI.465)
 *           RectVisibleOld (GDI.104)
 */
BOOL16 WINAPI RectVisible16( HDC16 hdc, const RECT16* rect16 )
{
    RECT rect;

    rect.left   = rect16->left;
    rect.top    = rect16->top;
    rect.right  = rect16->right;
    rect.bottom = rect16->bottom;
    return RectVisible( HDC_32(hdc), &rect );
}


/***********************************************************************
 *           RectInRegion    (GDI.466)
 *           RectInRegionOld (GDI.181)
 */
BOOL16 WINAPI RectInRegion16( HRGN16 hrgn, const RECT16 *rect )
{
    RECT r32;

    r32.left   = rect->left;
    r32.top    = rect->top;
    r32.right  = rect->right;
    r32.bottom = rect->bottom;
    return RectInRegion( HRGN_32(hrgn), &r32 );
}


/***********************************************************************
 *           GetBitmapDimensionEx    (GDI.468)
 */
BOOL16 WINAPI GetBitmapDimensionEx16( HBITMAP16 hbitmap, LPSIZE16 size )
{
    SIZE size32;
    BOOL ret = GetBitmapDimensionEx( HBITMAP_32(hbitmap), &size32 );

    if (ret)
    {
        size->cx = size32.cx;
        size->cy = size32.cy;
    }
    return ret;
}


/***********************************************************************
 *		GetBrushOrgEx (GDI.469)
 */
BOOL16 WINAPI GetBrushOrgEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetBrushOrgEx( HDC_32(hdc), &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}


/***********************************************************************
 *		GetCurrentPositionEx (GDI.470)
 */
BOOL16 WINAPI GetCurrentPositionEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetCurrentPositionEx( HDC_32(hdc), &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}


/***********************************************************************
 *           GetTextExtentPoint    (GDI.471)
 *
 * FIXME: Should this have a bug for compatibility?
 * Original Windows versions of GetTextExtentPoint{A,W} have documented
 * bugs (-> MSDN KB q147647.txt).
 */
BOOL16 WINAPI GetTextExtentPoint16( HDC16 hdc, LPCSTR str, INT16 count, LPSIZE16 size )
{
    SIZE size32;
    BOOL ret = GetTextExtentPoint32A( HDC_32(hdc), str, count, &size32 );

    if (ret)
    {
        size->cx = size32.cx;
        size->cy = size32.cy;
    }
    return ret;
}


/***********************************************************************
 *		GetViewportExtEx (GDI.472)
 */
BOOL16 WINAPI GetViewportExtEx16( HDC16 hdc, LPSIZE16 size )
{
    SIZE size32;
    if (!GetViewportExtEx( HDC_32(hdc), &size32 )) return FALSE;
    size->cx = size32.cx;
    size->cy = size32.cy;
    return TRUE;
}


/***********************************************************************
 *		GetViewportOrgEx (GDI.473)
 */
BOOL16 WINAPI GetViewportOrgEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetViewportOrgEx( HDC_32(hdc), &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}


/***********************************************************************
 *		GetWindowExtEx (GDI.474)
 */
BOOL16 WINAPI GetWindowExtEx16( HDC16 hdc, LPSIZE16 size )
{
    SIZE size32;
    if (!GetWindowExtEx( HDC_32(hdc), &size32 )) return FALSE;
    size->cx = size32.cx;
    size->cy = size32.cy;
    return TRUE;
}


/***********************************************************************
 *		GetWindowOrgEx (GDI.475)
 */
BOOL16 WINAPI GetWindowOrgEx16( HDC16 hdc, LPPOINT16 pt )
{
    POINT pt32;
    if (!GetWindowOrgEx( HDC_32(hdc), &pt32 )) return FALSE;
    pt->x = pt32.x;
    pt->y = pt32.y;
    return TRUE;
}


/***********************************************************************
 *           OffsetViewportOrgEx    (GDI.476)
 */
BOOL16 WINAPI OffsetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt)
{
    POINT pt32;
    BOOL16 ret = OffsetViewportOrgEx( HDC_32(hdc), x, y, &pt32 );
    if (pt)
    {
        pt->x = pt32.x;
        pt->y = pt32.y;
    }
    return ret;
}


/***********************************************************************
 *           OffsetWindowOrgEx    (GDI.477)
 */
BOOL16 WINAPI OffsetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = OffsetWindowOrgEx( HDC_32(hdc), x, y, &pt32 );
    if (pt)
    {
        pt->x = pt32.x;
        pt->y = pt32.y;
    }
    return ret;
}


/***********************************************************************
 *           SetBitmapDimensionEx    (GDI.478)
 */
BOOL16 WINAPI SetBitmapDimensionEx16( HBITMAP16 hbitmap, INT16 x, INT16 y, LPSIZE16 prevSize )
{
    SIZE size32;
    BOOL ret = SetBitmapDimensionEx( HBITMAP_32(hbitmap), x, y, &size32 );

    if (ret && prevSize)
    {
        prevSize->cx = size32.cx;
        prevSize->cy = size32.cy;
    }
    return ret;
}


/***********************************************************************
 *           SetViewportExtEx    (GDI.479)
 */
BOOL16 WINAPI SetViewportExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = SetViewportExtEx( HDC_32(hdc), x, y, &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           SetViewportOrgEx    (GDI.480)
 */
BOOL16 WINAPI SetViewportOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = SetViewportOrgEx( HDC_32(hdc), x, y, &pt32 );
    if (pt)
    {
        pt->x = pt32.x;
        pt->y = pt32.y;
    }
    return ret;
}


/***********************************************************************
 *           SetWindowExtEx    (GDI.481)
 */
BOOL16 WINAPI SetWindowExtEx16( HDC16 hdc, INT16 x, INT16 y, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = SetWindowExtEx( HDC_32(hdc), x, y, &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           SetWindowOrgEx    (GDI.482)
 */
BOOL16 WINAPI SetWindowOrgEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;
    BOOL16 ret = SetWindowOrgEx( HDC_32(hdc), x, y, &pt32 );
    if (pt)
    {
        pt->x = pt32.x;
        pt->y = pt32.y;
    }
    return ret;
}


/***********************************************************************
 *           MoveToEx    (GDI.483)
 */
BOOL16 WINAPI MoveToEx16( HDC16 hdc, INT16 x, INT16 y, LPPOINT16 pt )
{
    POINT pt32;

    if (!MoveToEx( HDC_32(hdc), x, y, &pt32 )) return FALSE;
    if (pt)
    {
        pt->x = pt32.x;
        pt->y = pt32.y;
    }
    return TRUE;
}


/***********************************************************************
 *           ScaleViewportExtEx    (GDI.484)
 */
BOOL16 WINAPI ScaleViewportExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                    INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = ScaleViewportExtEx( HDC_32(hdc), xNum, xDenom, yNum, yDenom,
                                       &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           ScaleWindowExtEx    (GDI.485)
 */
BOOL16 WINAPI ScaleWindowExtEx16( HDC16 hdc, INT16 xNum, INT16 xDenom,
                                  INT16 yNum, INT16 yDenom, LPSIZE16 size )
{
    SIZE size32;
    BOOL16 ret = ScaleWindowExtEx( HDC_32(hdc), xNum, xDenom, yNum, yDenom,
                                     &size32 );
    if (size) { size->cx = size32.cx; size->cy = size32.cy; }
    return ret;
}


/***********************************************************************
 *           GetAspectRatioFilterEx  (GDI.486)
 */
BOOL16 WINAPI GetAspectRatioFilterEx16( HDC16 hdc, LPSIZE16 pAspectRatio )
{
    FIXME("(%04x, %p): -- Empty Stub !\n", hdc, pAspectRatio);
    return FALSE;
}


/******************************************************************************
 *           PolyBezier  (GDI.502)
 */
BOOL16 WINAPI PolyBezier16( HDC16 hdc, const POINT16* lppt, INT16 cPoints )
{
    int i;
    BOOL16 ret;
    LPPOINT pt32 = HeapAlloc( GetProcessHeap(), 0, cPoints*sizeof(POINT) );
    if(!pt32) return FALSE;
    for (i=cPoints;i--;)
    {
        pt32[i].x = lppt[i].x;
        pt32[i].y = lppt[i].y;
    }
    ret= PolyBezier(HDC_32(hdc), pt32, cPoints);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/******************************************************************************
 *           PolyBezierTo  (GDI.503)
 */
BOOL16 WINAPI PolyBezierTo16( HDC16 hdc, const POINT16* lppt, INT16 cPoints )
{
    int i;
    BOOL16 ret;
    LPPOINT pt32 = HeapAlloc( GetProcessHeap(), 0,
                                           cPoints*sizeof(POINT) );
    if(!pt32) return FALSE;
    for (i=cPoints;i--;)
    {
        pt32[i].x = lppt[i].x;
        pt32[i].y = lppt[i].y;
    }
    ret= PolyBezierTo(HDC_32(hdc), pt32, cPoints);
    HeapFree( GetProcessHeap(), 0, pt32 );
    return ret;
}


/******************************************************************************
 *           ExtSelectClipRgn   (GDI.508)
 */
INT16 WINAPI ExtSelectClipRgn16( HDC16 hdc, HRGN16 hrgn, INT16 fnMode )
{
  return ExtSelectClipRgn( HDC_32(hdc), HRGN_32(hrgn), fnMode);
}


/***********************************************************************
 *           AbortPath    (GDI.511)
 */
BOOL16 WINAPI AbortPath16(HDC16 hdc)
{
    return AbortPath( HDC_32(hdc) );
}


/***********************************************************************
 *           BeginPath    (GDI.512)
 */
BOOL16 WINAPI BeginPath16(HDC16 hdc)
{
    return BeginPath( HDC_32(hdc) );
}


/***********************************************************************
 *           CloseFigure    (GDI.513)
 */
BOOL16 WINAPI CloseFigure16(HDC16 hdc)
{
    return CloseFigure( HDC_32(hdc) );
}


/***********************************************************************
 *           EndPath    (GDI.514)
 */
BOOL16 WINAPI EndPath16(HDC16 hdc)
{
    return EndPath( HDC_32(hdc) );
}


/***********************************************************************
 *           FillPath    (GDI.515)
 */
BOOL16 WINAPI FillPath16(HDC16 hdc)
{
    return FillPath( HDC_32(hdc) );
}


/*******************************************************************
 *           FlattenPath    (GDI.516)
 */
BOOL16 WINAPI FlattenPath16(HDC16 hdc)
{
    return FlattenPath( HDC_32(hdc) );
}


/***********************************************************************
 *           GetPath    (GDI.517)
 */
INT16 WINAPI GetPath16(HDC16 hdc, LPPOINT16 pPoints, LPBYTE pTypes, INT16 nSize)
{
    FIXME("(%d,%p,%p): stub\n",hdc,pPoints,pTypes);
    return 0;
}


/***********************************************************************
 *           PathToRegion    (GDI.518)
 */
HRGN16 WINAPI PathToRegion16(HDC16 hdc)
{
    return HRGN_16( PathToRegion( HDC_32(hdc) ));
}


/***********************************************************************
 *           SelectClipPath    (GDI.519)
 */
BOOL16 WINAPI SelectClipPath16(HDC16 hdc, INT16 iMode)
{
    return SelectClipPath( HDC_32(hdc), iMode );
}


/*******************************************************************
 *           StrokeAndFillPath    (GDI.520)
 */
BOOL16 WINAPI StrokeAndFillPath16(HDC16 hdc)
{
    return StrokeAndFillPath( HDC_32(hdc) );
}


/*******************************************************************
 *           StrokePath    (GDI.521)
 */
BOOL16 WINAPI StrokePath16(HDC16 hdc)
{
    return StrokePath( HDC_32(hdc) );
}


/*******************************************************************
 *           WidenPath    (GDI.522)
 */
BOOL16 WINAPI WidenPath16(HDC16 hdc)
{
    return WidenPath( HDC_32(hdc) );
}


/***********************************************************************
 *		GetArcDirection (GDI.524)
 */
INT16 WINAPI GetArcDirection16( HDC16 hdc )
{
    return GetArcDirection( HDC_32(hdc) );
}


/***********************************************************************
 *           SetArcDirection    (GDI.525)
 */
INT16 WINAPI SetArcDirection16( HDC16 hdc, INT16 nDirection )
{
    return SetArcDirection( HDC_32(hdc), (INT)nDirection );
}


/***********************************************************************
 *           CreateHalftonePalette (GDI.529)
 */
HPALETTE16 WINAPI CreateHalftonePalette16( HDC16 hdc )
{
    return HPALETTE_16( CreateHalftonePalette( HDC_32(hdc) ));
}


/***********************************************************************
 *           SetDIBColorTable    (GDI.602)
 */
UINT16 WINAPI SetDIBColorTable16( HDC16 hdc, UINT16 startpos, UINT16 entries, RGBQUAD *colors )
{
    return SetDIBColorTable( HDC_32(hdc), startpos, entries, colors );
}


/***********************************************************************
 *           GetDIBColorTable    (GDI.603)
 */
UINT16 WINAPI GetDIBColorTable16( HDC16 hdc, UINT16 startpos, UINT16 entries, RGBQUAD *colors )
{
    return GetDIBColorTable( HDC_32(hdc), startpos, entries, colors );
}


/***********************************************************************
 *           GetRegionData   (GDI.607)
 *
 * FIXME: is LPRGNDATA the same in Win16 and Win32 ?
 */
DWORD WINAPI GetRegionData16( HRGN16 hrgn, DWORD count, LPRGNDATA rgndata )
{
    return GetRegionData( HRGN_32(hrgn), count, rgndata );
}


/***********************************************************************
 *           GdiFreeResources   (GDI.609)
 */
WORD WINAPI GdiFreeResources16( DWORD reserve )
{
    return 90; /* lie about it, it shouldn't matter */
}


/***********************************************************************
 *           GdiSignalProc32     (GDI.610)
 */
WORD WINAPI GdiSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                           DWORD dwFlags, HMODULE16 hModule )
{
    return 0;
}


/***********************************************************************
 *           GetTextCharset   (GDI.612)
 */
UINT16 WINAPI GetTextCharset16( HDC16 hdc )
{
    return GetTextCharset( HDC_32(hdc) );
}


/***********************************************************************
 *           EnumFontFamiliesEx (GDI.613)
 */
INT16 WINAPI EnumFontFamiliesEx16( HDC16 hdc, LPLOGFONT16 plf,
                                   FONTENUMPROC16 proc, LPARAM lParam,
                                   DWORD dwFlags)
{
    struct callback16_info info;
    LOGFONTW lfW, *plfW;

    info.proc  = (FARPROC16)proc;
    info.param = lParam;

    if (plf)
    {
        logfont_16_to_W(plf, &lfW);
        plfW = &lfW;
    }
    else plfW = NULL;

    return EnumFontFamiliesExW( HDC_32(hdc), plfW, enum_font_callback,
                                (LPARAM)&info, dwFlags );
}


/*************************************************************************
 *             GetFontLanguageInfo   (GDI.616)
 */
DWORD WINAPI GetFontLanguageInfo16( HDC16 hdc )
{
    return GetFontLanguageInfo( HDC_32(hdc) );
}


/***********************************************************************
 *           SetLayout   (GDI.1000)
 *
 * Sets left->right or right->left text layout flags of a dc.
 */
BOOL16 WINAPI SetLayout16( HDC16 hdc, DWORD layout )
{
    return SetLayout( HDC_32(hdc), layout );
}


/***********************************************************************
 *           SetSolidBrush   (GDI.604)
 *
 * Change the color of a solid brush.
 *
 * PARAMS
 *  hBrush   [I] Brush to change the color of
 *  newColor [I] New color for hBrush
 *
 * RETURNS
 *  Success: TRUE. The color of hBrush is set to newColor.
 *  Failure: FALSE.
 *
 * FIXME
 *  This function is undocumented and untested. The implementation may
 *  not be correct.
 */
BOOL16 WINAPI SetSolidBrush16(HBRUSH16 hBrush, COLORREF newColor )
{
    TRACE("(hBrush %04x, newColor %08x)\n", hBrush, newColor);

    return BRUSH_SetSolid( HBRUSH_32(hBrush), newColor );
}


/***********************************************************************
 *           Copy   (GDI.250)
 */
void WINAPI Copy16( LPVOID src, LPVOID dst, WORD size )
{
    memcpy( dst, src, size );
}

/***********************************************************************
 *           RealizeDefaultPalette    (GDI.365)
 */
UINT16 WINAPI RealizeDefaultPalette16( HDC16 hdc )
{
    UINT16 ret = 0;
    DC          *dc;

    TRACE("%04x\n", hdc );

    if (!(dc = get_dc_ptr( HDC_32(hdc) ))) return 0;

    if (dc->funcs->pRealizeDefaultPalette) ret = dc->funcs->pRealizeDefaultPalette( dc->physDev );
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *           IsDCCurrentPalette   (GDI.412)
 */
BOOL16 WINAPI IsDCCurrentPalette16(HDC16 hDC)
{
    DC *dc = get_dc_ptr( HDC_32(hDC) );
    if (dc)
    {
      BOOL bRet = dc->hPalette == hPrimaryPalette;
      release_dc_ptr( dc );
      return bRet;
    }
    return FALSE;
}

/*********************************************************************
 *           SetMagicColors   (GDI.606)
 */
VOID WINAPI SetMagicColors16(HDC16 hDC, COLORREF color, UINT16 index)
{
    FIXME("(hDC %04x, color %04x, index %04x): stub\n", hDC, (int)color, index);

}


/***********************************************************************
 *           DPtoLP    (GDI.67)
 */
BOOL16 WINAPI DPtoLP16( HDC16 hdc, LPPOINT16 points, INT16 count )
{
    DC * dc = get_dc_ptr( HDC_32(hdc) );
    if (!dc) return FALSE;

    while (count--)
    {
        points->x = MulDiv( points->x - dc->vportOrgX, dc->wndExtX, dc->vportExtX ) + dc->wndOrgX;
        points->y = MulDiv( points->y - dc->vportOrgY, dc->wndExtY, dc->vportExtY ) + dc->wndOrgY;
        points++;
    }
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           LPtoDP    (GDI.99)
 */
BOOL16 WINAPI LPtoDP16( HDC16 hdc, LPPOINT16 points, INT16 count )
{
    DC * dc = get_dc_ptr( HDC_32(hdc) );
    if (!dc) return FALSE;

    while (count--)
    {
        points->x = MulDiv( points->x - dc->wndOrgX, dc->vportExtX, dc->wndExtX ) + dc->vportOrgX;
        points->y = MulDiv( points->y - dc->wndOrgY, dc->vportExtY, dc->wndExtY ) + dc->vportOrgY;
        points++;
    }
    release_dc_ptr( dc );
    return TRUE;
}


/***********************************************************************
 *           GetDCState   (GDI.179)
 */
HDC16 WINAPI GetDCState16( HDC16 hdc )
{
    ERR( "no longer supported\n" );
    return 0;
}


/***********************************************************************
 *           SetDCState   (GDI.180)
 */
void WINAPI SetDCState16( HDC16 hdc, HDC16 hdcs )
{
    ERR( "no longer supported\n" );
}

/***********************************************************************
 *           SetDCOrg   (GDI.117)
 */
DWORD WINAPI SetDCOrg16( HDC16 hdc16, INT16 x, INT16 y )
{
    DWORD prevOrg = 0;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return 0;
    if (dc->funcs->pSetDCOrg) prevOrg = dc->funcs->pSetDCOrg( dc->physDev, x, y );
    release_dc_ptr( dc );
    return prevOrg;
}


/***********************************************************************
 *		InquireVisRgn   (GDI.131)
 */
HRGN16 WINAPI InquireVisRgn16( HDC16 hdc )
{
    HRGN16 ret = 0;
    DC * dc = get_dc_ptr( HDC_32(hdc) );
    if (dc)
    {
        ret = HRGN_16(dc->hVisRgn);
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           OffsetVisRgn    (GDI.102)
 */
INT16 WINAPI OffsetVisRgn16( HDC16 hdc16, INT16 x, INT16 y )
{
    INT16 retval;
    HDC hdc = HDC_32( hdc16 );
    DC * dc = get_dc_ptr( hdc );

    if (!dc) return ERROR;
    TRACE("%p %d,%d\n", hdc, x, y );
    update_dc( dc );
    retval = OffsetRgn( dc->hVisRgn, x, y );
    CLIPPING_UpdateGCRegion( dc );
    release_dc_ptr( dc );
    return retval;
}


/***********************************************************************
 *           ExcludeVisRect   (GDI.73)
 */
INT16 WINAPI ExcludeVisRect16( HDC16 hdc16, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    HRGN tempRgn;
    INT16 ret;
    POINT pt[2];
    HDC hdc = HDC_32( hdc16 );
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return ERROR;

    pt[0].x = left;
    pt[0].y = top;
    pt[1].x = right;
    pt[1].y = bottom;

    LPtoDP( hdc, pt, 2 );

    TRACE("%p %d,%d - %d,%d\n", hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

    if (!(tempRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
    else
    {
        update_dc( dc );
        ret = CombineRgn( dc->hVisRgn, dc->hVisRgn, tempRgn, RGN_DIFF );
        DeleteObject( tempRgn );
    }
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           IntersectVisRect   (GDI.98)
 */
INT16 WINAPI IntersectVisRect16( HDC16 hdc16, INT16 left, INT16 top, INT16 right, INT16 bottom )
{
    HRGN tempRgn;
    INT16 ret;
    POINT pt[2];
    HDC hdc = HDC_32( hdc16 );
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return ERROR;

    pt[0].x = left;
    pt[0].y = top;
    pt[1].x = right;
    pt[1].y = bottom;

    LPtoDP( hdc, pt, 2 );

    TRACE("%p %d,%d - %d,%d\n", hdc, pt[0].x, pt[0].y, pt[1].x, pt[1].y);

    if (!(tempRgn = CreateRectRgn( pt[0].x, pt[0].y, pt[1].x, pt[1].y ))) ret = ERROR;
    else
    {
        update_dc( dc );
        ret = CombineRgn( dc->hVisRgn, dc->hVisRgn, tempRgn, RGN_AND );
        DeleteObject( tempRgn );
    }
    if (ret != ERROR) CLIPPING_UpdateGCRegion( dc );
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SaveVisRgn   (GDI.129)
 */
HRGN16 WINAPI SaveVisRgn16( HDC16 hdc16 )
{
    struct saved_visrgn *saved;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return 0;
    TRACE("%p\n", hdc );

    update_dc( dc );
    if (!(saved = HeapAlloc( GetProcessHeap(), 0, sizeof(*saved) ))) goto error;
    if (!(saved->hrgn = CreateRectRgn( 0, 0, 0, 0 ))) goto error;
    CombineRgn( saved->hrgn, dc->hVisRgn, 0, RGN_COPY );
    saved->next = dc->saved_visrgn;
    dc->saved_visrgn = saved;
    release_dc_ptr( dc );
    return HRGN_16(saved->hrgn);

error:
    release_dc_ptr( dc );
    HeapFree( GetProcessHeap(), 0, saved );
    return 0;
}


/***********************************************************************
 *           RestoreVisRgn   (GDI.130)
 */
INT16 WINAPI RestoreVisRgn16( HDC16 hdc16 )
{
    struct saved_visrgn *saved;
    HDC hdc = HDC_32( hdc16 );
    DC *dc = get_dc_ptr( hdc );
    INT16 ret = ERROR;

    if (!dc) return ERROR;

    TRACE("%p\n", hdc );

    if (!(saved = dc->saved_visrgn)) goto done;

    ret = CombineRgn( dc->hVisRgn, saved->hrgn, 0, RGN_COPY );
    dc->saved_visrgn = saved->next;
    DeleteObject( saved->hrgn );
    HeapFree( GetProcessHeap(), 0, saved );
    CLIPPING_UpdateGCRegion( dc );
 done:
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetClipRgn (GDI.173)
 */
HRGN16 WINAPI GetClipRgn16( HDC16 hdc )
{
    HRGN16 ret = 0;
    DC * dc = get_dc_ptr( HDC_32(hdc) );
    if (dc)
    {
        ret = HRGN_16(dc->hClipRgn);
        release_dc_ptr( dc );
    }
    return ret;
}


/***********************************************************************
 *           MakeObjectPrivate    (GDI.463)
 *
 * What does that mean ?
 * Some little docu can be found in "Undocumented Windows",
 * but this is basically useless.
 */
void WINAPI MakeObjectPrivate16( HGDIOBJ16 handle16, BOOL16 private )
{
    FIXME( "stub: %x %u\n", handle16, private );
}

/***********************************************************************
 *           CreateDIBSection    (GDI.489)
 */
HBITMAP16 WINAPI CreateDIBSection16 (HDC16 hdc, const BITMAPINFO *bmi, UINT16 usage,
                                     SEGPTR *bits16, HANDLE section, DWORD offset)
{
    LPVOID bits32;
    HBITMAP hbitmap;

    hbitmap = CreateDIBSection( HDC_32(hdc), bmi, usage, &bits32, section, offset );
    if (hbitmap)
    {
        BITMAPOBJ *bmp = GDI_GetObjPtr(hbitmap, OBJ_BITMAP);
        if (bmp && bmp->dib && bits32)
        {
            const BITMAPINFOHEADER *bi = &bmi->bmiHeader;
            LONG width, height;
            WORD planes, bpp;
            DWORD compr, size;
            INT width_bytes;
            WORD count, sel;
            int i;

            DIB_GetBitmapInfo(bi, &width, &height, &planes, &bpp, &compr, &size);

            height = height >= 0 ? height : -height;
            width_bytes = DIB_GetDIBWidthBytes(width, bpp);

            if (!size || (compr != BI_RLE4 && compr != BI_RLE8)) size = width_bytes * height;

            /* calculate number of sel's needed for size with 64K steps */
            count = (size + 0xffff) / 0x10000;
            sel = AllocSelectorArray16(count);

            for (i = 0; i < count; i++)
            {
                SetSelectorBase(sel + (i << __AHSHIFT), (DWORD)bits32 + i * 0x10000);
                SetSelectorLimit16(sel + (i << __AHSHIFT), size - 1); /* yep, limit is correct */
                size -= 0x10000;
            }
            bmp->segptr_bits = MAKESEGPTR( sel, 0 );
            if (bits16) *bits16 = bmp->segptr_bits;
        }
        if (bmp) GDI_ReleaseObj( hbitmap );
    }
    return HBITMAP_16(hbitmap);
}
