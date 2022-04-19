/*
 * Copyright (C) the Wine project
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

#ifndef __WINE_WINE_WINGDI16_H
#define __WINE_WINE_WINGDI16_H

#include <stdarg.h>
#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <wine/winbase16.h>

#include <pshpack1.h>

typedef HANDLE16 HPQ16;
typedef HANDLE16 HPJOB16;

typedef struct
{
    WORD   wFirst;
    WORD   wSecond;
    INT16  iKernAmount;
} KERNINGPAIR16, *LPKERNINGPAIR16;

typedef struct
{
    INT16  bmType;
    INT16  bmWidth;
    INT16  bmHeight;
    INT16  bmWidthBytes;
    BYTE   bmPlanes;
    BYTE   bmBitsPixel;
    SEGPTR bmBits;
} BITMAP16, *LPBITMAP16;

typedef struct
{
    UINT16     lbStyle;
    COLORREF   lbColor;
    INT16      lbHatch;
} LOGBRUSH16, *LPLOGBRUSH16;

typedef struct
{
    INT16  lfHeight;
    INT16  lfWidth;
    INT16  lfEscapement;
    INT16  lfOrientation;
    INT16  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    CHAR   lfFaceName[LF_FACESIZE];
} LOGFONT16, *LPLOGFONT16;

typedef struct
{
  LOGFONT16  elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE];
  BYTE       elfStyle[LF_FACESIZE];
} ENUMLOGFONT16, *LPENUMLOGFONT16;

typedef struct
{
  LOGFONT16  elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE];
  BYTE       elfStyle[LF_FACESIZE];
  BYTE       elfScript[LF_FACESIZE];
} ENUMLOGFONTEX16, *LPENUMLOGFONTEX16;

typedef struct
{
    INT16  txfHeight;
    INT16  txfWidth;
    INT16  txfEscapement;
    INT16  txfOrientation;
    INT16  txfWeight;
    CHAR   txfItalic;
    CHAR   txfUnderline;
    CHAR   txfStrikeOut;
    CHAR   txfOutPrecision;
    CHAR   txfClipPrecision;
    INT16  txfAccelerator;
    INT16  txfOverhang;
} TEXTXFORM16, *LPTEXTXFORM16;

typedef struct
{
    INT16 dfType;
    INT16 dfPoints;
    INT16 dfVertRes;
    INT16 dfHorizRes;
    INT16 dfAscent;
    INT16 dfInternalLeading;
    INT16 dfExternalLeading;
    CHAR  dfItalic;
    CHAR  dfUnderline;
    CHAR  dfStrikeOut;
    INT16 dfWeight;
    BYTE  dfCharSet;
    INT16 dfPixWidth;
    INT16 dfPixHeight;
    CHAR  dfPitchAndFamily;
    INT16 dfAvgWidth;
    INT16 dfMaxWidth;
    CHAR  dfFirstChar;
    CHAR  dfLastChar;
    CHAR  dfDefaultChar;
    CHAR  dfBreakChar;
    INT16 dfWidthBytes;
    LONG  dfDevice;
    LONG  dfFace;
    LONG  dfBitsPointer;
    LONG  dfBitsOffset;
    CHAR  dfReserved;
    /* Fields, introduced for Windows 3.x fonts */
    LONG  dfFlags;
    INT16 dfAspace;
    INT16 dfBspace;
    INT16 dfCspace;
    LONG  dfColorPointer;
    LONG  dfReserved1[4];
} FONTINFO16, *LPFONTINFO16;

typedef struct {
    WORD   dfVersion;
    DWORD  dfSize;
    CHAR   dfCopyright[60];
    WORD   dfType;
    WORD   dfPoints;
    WORD   dfVertRes;
    WORD   dfHorizRes;
    WORD   dfAscent;
    WORD   dfInternalLeading;
    WORD   dfExternalLeading;
    BYTE   dfItalic;
    BYTE   dfUnderline;
    BYTE   dfStrikeOut;
    WORD   dfWeight;
    BYTE   dfCharSet;
    WORD   dfPixWidth;
    WORD   dfPixHeight;
    BYTE   dfPitchAndFamily;
    WORD   dfAvgWidth;
    WORD   dfMaxWidth;
    BYTE   dfFirstChar;
    BYTE   dfLastChar;
    BYTE   dfDefaultChar;
    BYTE   dfBreakChar;
    WORD   dfWidthBytes;
    DWORD  dfDevice;
    DWORD  dfFace;
    DWORD  dfReserved;
    CHAR   szDeviceName[60]; /* FIXME: length unknown */
    CHAR   szFaceName[60];   /* ditto */
} FONTDIR16, *LPFONTDIR16;

typedef struct
{
    INT16     tmHeight;
    INT16     tmAscent;
    INT16     tmDescent;
    INT16     tmInternalLeading;
    INT16     tmExternalLeading;
    INT16     tmAveCharWidth;
    INT16     tmMaxCharWidth;
    INT16     tmWeight;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    INT16     tmOverhang;
    INT16     tmDigitizedAspectX;
    INT16     tmDigitizedAspectY;
} TEXTMETRIC16, *LPTEXTMETRIC16;

typedef struct _OUTLINETEXTMETRIC16
{
    UINT16          otmSize;
    TEXTMETRIC16    otmTextMetrics;
    BYTE            otmFiller;
    PANOSE          otmPanoseNumber;
    UINT16          otmfsSelection;
    UINT16          otmfsType;
    INT16           otmsCharSlopeRise;
    INT16           otmsCharSlopeRun;
    INT16           otmItalicAngle;
    UINT16          otmEMSquare;
    INT16           otmAscent;
    INT16           otmDescent;
    UINT16          otmLineGap;
    UINT16          otmsCapEmHeight;
    UINT16          otmsXHeight;
    RECT16          otmrcFontBox;
    INT16           otmMacAscent;
    INT16           otmMacDescent;
    UINT16          otmMacLineGap;
    UINT16          otmusMinimumPPEM;
    POINT16         otmptSubscriptSize;
    POINT16         otmptSubscriptOffset;
    POINT16         otmptSuperscriptSize;
    POINT16         otmptSuperscriptOffset;
    UINT16          otmsStrikeoutSize;
    INT16           otmsStrikeoutPosition;
    INT16           otmsUnderscoreSize;
    INT           otmsUnderscorePosition;
    LPSTR           otmpFamilyName;
    LPSTR           otmpFaceName;
    LPSTR           otmpStyleName;
    LPSTR           otmpFullName;
} OUTLINETEXTMETRIC16,*LPOUTLINETEXTMETRIC16;

typedef struct
{
    INT16     tmHeight;
    INT16     tmAscent;
    INT16     tmDescent;
    INT16     tmInternalLeading;
    INT16     tmExternalLeading;
    INT16     tmAveCharWidth;
    INT16     tmMaxCharWidth;
    INT16     tmWeight;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    INT16     tmOverhang;
    INT16     tmDigitizedAspectX;
    INT16     tmDigitizedAspectY;
    DWORD     ntmFlags;
    UINT16    ntmSizeEM;
    UINT16    ntmCellHeight;
    UINT16    ntmAvgWidth;
} NEWTEXTMETRIC16,*LPNEWTEXTMETRIC16;

typedef struct
{
    NEWTEXTMETRIC16	ntmTm;
    FONTSIGNATURE       ntmFontSig;
} NEWTEXTMETRICEX16,*LPNEWTEXTMETRICEX16;

typedef INT16 (CALLBACK *FONTENUMPROC16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef VOID  (CALLBACK *LINEDDAPROC16)(INT16,INT16,LPARAM);
typedef INT16 (CALLBACK *GOBJENUMPROC16)(SEGPTR,LPARAM);


typedef struct
{
    UINT16	gmBlackBoxX;
    UINT16	gmBlackBoxY;
    POINT16	gmptGlyphOrigin;
    INT16	gmCellIncX;
    INT16	gmCellIncY;
} GLYPHMETRICS16, *LPGLYPHMETRICS16;

typedef struct
{
    INT16   abcA;
    UINT16  abcB;
    INT16   abcC;
} ABC16, *LPABC16;

typedef struct
{
    UINT16   lopnStyle;
    POINT16  lopnWidth;
    COLORREF lopnColor;
} LOGPEN16, *LPLOGPEN16;

typedef struct
{
    HGDIOBJ16 objectHandle[1];
} HANDLETABLE16, *LPHANDLETABLE16;

typedef struct
{
    INT16        mm;
    INT16        xExt;
    INT16        yExt;
    HMETAFILE16  hMF;
} METAFILEPICT16, *LPMETAFILEPICT16;

typedef INT16 (CALLBACK *MFENUMPROC16)(HDC16,HANDLETABLE16*,METARECORD*,
                                       INT16,LPARAM);
typedef struct
{
    INT16    cbSize;
    SEGPTR   lpszDocName;
    SEGPTR   lpszOutput;
    SEGPTR   lpszDatatype;
    DWORD    fwType;
} DOCINFO16, *LPDOCINFO16;

typedef BOOL16 (CALLBACK *ABORTPROC16)(HDC16, INT16);

#define INT_PD_DEFAULT_DEVMODE  1
#define INT_PD_DEFAULT_MODEL    2

/* Escape: CLIP_TO_PATH modes */
#define CLIP_SAVE 0
#define CLIP_RESTORE 1
#define CLIP_INCLUSIVE 2
#define CLIP_EXCLUSIVE 3

/* Escape: END_PATH info */
struct PATH_INFO {
    short RenderMode;
    BYTE FillMode;
    BYTE BkMode;
    LOGPEN16 Pen;
    LOGBRUSH16 Brush;
    DWORD BkColor;
};

/* RenderMode */
#define RENDERMODE_NO_DISPLAY 0
#define RENDERMODE_OPEN 1
#define RENDERMODE_CLOSED 2

/* For DRAWPATTERNRECT Escape, 16bit mode */
typedef struct _DRAWPATRECT16
{
    POINT16	ptPosition;
    POINT16	ptSize;
    WORD	wStyle;
    WORD	wPattern;
} DRAWPATRECT16, *PDRAWPATRECT16;

#include <poppack.h>


INT16       WINAPI AbortDoc16(HDC16);
BOOL16      WINAPI AbortPath16(HDC16);
INT16       WINAPI AddFontResource16(LPCSTR);
void        WINAPI AnimatePalette16(HPALETTE16,UINT16,UINT16,const PALETTEENTRY*);
BOOL16      WINAPI Arc16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI BeginPath16(HDC16);
BOOL16      WINAPI BitBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,INT16,DWORD);
BOOL16      WINAPI Chord16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI CloseFigure16(HDC16);
INT16       WINAPI CloseJob16(HPJOB16);
HMETAFILE16 WINAPI CloseMetaFile16(HDC16);
INT16       WINAPI CombineRgn16(HRGN16,HRGN16,HRGN16,INT16);
HMETAFILE16 WINAPI CopyMetaFile16(HMETAFILE16,LPCSTR);
HBITMAP16   WINAPI CreateBitmap16(INT16,INT16,UINT16,UINT16,LPCVOID);
HBITMAP16   WINAPI CreateBitmapIndirect16(const BITMAP16*);
HBRUSH16    WINAPI CreateBrushIndirect16(const LOGBRUSH16*);
HBITMAP16   WINAPI CreateCompatibleBitmap16(HDC16,INT16,INT16);
HDC16       WINAPI CreateCompatibleDC16(HDC16);
HDC16       WINAPI CreateDC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODEA*);
HBITMAP16   WINAPI CreateDIBitmap16(HDC16,const BITMAPINFOHEADER*,DWORD,
                                    LPCVOID,const BITMAPINFO*,UINT16);
HBRUSH16    WINAPI CreateDIBPatternBrush16(HGLOBAL16,UINT16);
HBITMAP16   WINAPI CreateDIBSection16 (HDC16, const BITMAPINFO *, UINT16,
				       SEGPTR *, HANDLE, DWORD offset);
HBITMAP16   WINAPI CreateDiscardableBitmap16(HDC16,INT16,INT16);
HRGN16      WINAPI CreateEllipticRgn16(INT16,INT16,INT16,INT16);
HRGN16      WINAPI CreateEllipticRgnIndirect16(const RECT16 *);
HFONT16     WINAPI CreateFont16(INT16,INT16,INT16,INT16,INT16,BYTE,BYTE,BYTE,
                                BYTE,BYTE,BYTE,BYTE,BYTE,LPCSTR);
HFONT16     WINAPI CreateFontIndirect16(const LOGFONT16*);
HPALETTE16  WINAPI CreateHalftonePalette16(HDC16);
HBRUSH16    WINAPI CreateHatchBrush16(INT16,COLORREF);
HDC16       WINAPI CreateIC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODEA*);
HDC16       WINAPI CreateMetaFile16(LPCSTR);
HPALETTE16  WINAPI CreatePalette16(const LOGPALETTE*);
HBRUSH16    WINAPI CreatePatternBrush16(HBITMAP16);
HPEN16      WINAPI CreatePen16(INT16,INT16,COLORREF);
HPEN16      WINAPI CreatePenIndirect16(const LOGPEN16*);
HRGN16      WINAPI CreatePolyPolygonRgn16(const POINT16*,const INT16*,INT16,INT16);
HRGN16      WINAPI CreatePolygonRgn16(const POINT16*,INT16,INT16);
HRGN16      WINAPI CreateRectRgn16(INT16,INT16,INT16,INT16);
HRGN16      WINAPI CreateRectRgnIndirect16(const RECT16*);
HRGN16      WINAPI CreateRoundRectRgn16(INT16,INT16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI CreateScalableFontResource16(UINT16,LPCSTR,LPCSTR,LPCSTR);
HBRUSH16    WINAPI CreateSolidBrush16(COLORREF);
VOID        WINAPI Death16(HDC16);
BOOL16      WINAPI DeleteDC16(HDC16);
INT16       WINAPI DeleteJob16(HPJOB16,INT16);
BOOL16      WINAPI DeleteMetaFile16(HMETAFILE16);
BOOL16      WINAPI DeleteObject16(HGDIOBJ16);
BOOL16      WINAPI DPtoLP16(HDC16,LPPOINT16,INT16);
DWORD       WINAPI DrvGetPrinterData16(LPSTR,LPSTR,LPDWORD,LPBYTE,int cbData,LPDWORD);
DWORD       WINAPI DrvSetPrinterData16(LPSTR,LPSTR,DWORD,LPBYTE,DWORD);
BOOL16      WINAPI Ellipse16(HDC16,INT16,INT16,INT16,INT16);
INT16       WINAPI EndDoc16(HDC16);
INT16       WINAPI EndPage16(HDC16);
BOOL16      WINAPI EndPath16(HDC16);
INT16       WINAPI EndSpoolPage16(HPJOB16);
INT16       WINAPI EnumFontFamilies16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT16       WINAPI EnumFontFamiliesEx16(HDC16,LPLOGFONT16,FONTENUMPROC16,LPARAM,DWORD);
INT16       WINAPI EnumFonts16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
BOOL16      WINAPI EnumMetaFile16(HDC16,HMETAFILE16,MFENUMPROC16,LPARAM);
INT16       WINAPI EnumObjects16(HDC16,INT16,GOBJENUMPROC16,LPARAM);
BOOL16      WINAPI EqualRgn16(HRGN16,HRGN16);
INT16       WINAPI Escape16(HDC16,INT16,INT16,SEGPTR,LPVOID);
INT16       WINAPI ExcludeClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT16       WINAPI ExcludeVisRect16(HDC16,INT16,INT16,INT16,INT16);
HPEN16      WINAPI ExtCreatePen16(DWORD,DWORD,const LOGBRUSH16*,DWORD,const DWORD*);
BOOL16      WINAPI ExtFloodFill16(HDC16,INT16,INT16,COLORREF,UINT16);
BOOL16      WINAPI ExtTextOut16(HDC16,INT16,INT16,UINT16,const RECT16*,
                                LPCSTR,UINT16,const INT16*);
BOOL16      WINAPI FastWindowFrame16(HDC16,const RECT16*,INT16,INT16,DWORD);
BOOL16      WINAPI FillPath16(HDC16);
BOOL16      WINAPI FillRgn16(HDC16,HRGN16,HBRUSH16);
BOOL16      WINAPI FlattenPath16(HDC16);
BOOL16      WINAPI FloodFill16(HDC16,INT16,INT16,COLORREF);
BOOL16      WINAPI FrameRgn16(HDC16,HRGN16,HBRUSH16,INT16,INT16);
UINT16      WINAPI GDIRealizePalette16(HDC16);
DWORD       WINAPI GdiSeeGdiDo16(WORD,WORD,WORD,WORD);
HPALETTE16  WINAPI GDISelectPalette16(HDC16,HPALETTE16,WORD);
INT16       WINAPI GetArcDirection16(HDC16);
BOOL16      WINAPI GetAspectRatioFilterEx16(HDC16,LPSIZE16);
LONG        WINAPI GetBitmapBits16(HBITMAP16,LONG,LPVOID);
DWORD       WINAPI GetBitmapDimension16(HBITMAP16);
BOOL16      WINAPI GetBitmapDimensionEx16(HBITMAP16,LPSIZE16);
DWORD       WINAPI GetBrushOrg16(HDC16);
BOOL16      WINAPI GetBrushOrgEx16(HDC16,LPPOINT16);
COLORREF    WINAPI GetBkColor16(HDC16);
INT16       WINAPI GetBkMode16(HDC16);
UINT16      WINAPI GetBoundsRect16(HDC16,LPRECT16,UINT16);
BOOL16      WINAPI GetCharABCWidths16(HDC16,UINT16,UINT16,LPABC16);
BOOL16      WINAPI GetCharWidth16(HDC16,UINT16,UINT16,LPINT16);
INT16       WINAPI GetClipBox16(HDC16,LPRECT16);
HRGN16      WINAPI GetClipRgn16(HDC16);
HFONT16     WINAPI GetCurLogFont16(HDC16);
DWORD       WINAPI GetCurrentPosition16(HDC16);
BOOL16      WINAPI GetCurrentPositionEx16(HDC16,LPPOINT16);
DWORD       WINAPI GetDCHook16(HDC16,FARPROC16*);
DWORD       WINAPI GetDCOrg16(HDC16);
HDC16       WINAPI GetDCState16(HDC16);
INT16       WINAPI GetDeviceCaps16(HDC16,INT16);
UINT16      WINAPI GetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
INT16       WINAPI GetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPVOID,LPBITMAPINFO,UINT16);
INT16       WINAPI GetEnvironment16(LPCSTR,LPDEVMODEA,UINT16);
DWORD       WINAPI GetFontData16(HDC16,DWORD,DWORD,LPVOID,DWORD);
DWORD       WINAPI GetFontLanguageInfo16(HDC16);
DWORD       WINAPI GetGlyphOutline16(HDC16,UINT16,UINT16,LPGLYPHMETRICS16,DWORD,LPVOID,const MAT2*);
INT16       WINAPI GetKerningPairs16(HDC16,INT16,LPKERNINGPAIR16);
INT16       WINAPI GetMapMode16(HDC16);
HMETAFILE16 WINAPI GetMetaFile16(LPCSTR);
HGLOBAL16   WINAPI GetMetaFileBits16(HMETAFILE16);
DWORD       WINAPI GetNearestColor16(HDC16,DWORD);
UINT16      WINAPI GetNearestPaletteIndex16(HPALETTE16,COLORREF);
INT16       WINAPI GetObject16(HANDLE16,INT16,LPVOID);
UINT16      WINAPI GetOutlineTextMetrics16(HDC16,UINT16,LPOUTLINETEXTMETRIC16);
UINT16      WINAPI GetPaletteEntries16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
INT16       WINAPI GetPath16(HDC16,LPPOINT16,LPBYTE,INT16);
COLORREF    WINAPI GetPixel16(HDC16,INT16,INT16);
INT16       WINAPI GetPolyFillMode16(HDC16);
BOOL16      WINAPI GetRasterizerCaps16(LPRASTERIZER_STATUS,UINT16);
DWORD       WINAPI GetRegionData16(HRGN16,DWORD,LPRGNDATA);
INT16       WINAPI GetRelAbs16(HDC16);
INT16       WINAPI GetRgnBox16(HRGN16,LPRECT16);
INT16       WINAPI GetROP216(HDC16);
DWORD       WINAPI GetSpoolJob16(int,LONG);
HGDIOBJ16   WINAPI GetStockObject16(INT16);
INT16       WINAPI GetStretchBltMode16(HDC16);
UINT16      WINAPI GetSystemPaletteEntries16(HDC16,UINT16,UINT16,LPPALETTEENTRY);
UINT16      WINAPI GetSystemPaletteUse16(HDC16);
UINT16      WINAPI GetTextAlign16(HDC16);
INT16       WINAPI GetTextCharacterExtra16(HDC16);
UINT16      WINAPI GetTextCharset16(HDC16);
COLORREF    WINAPI GetTextColor16(HDC16);
DWORD       WINAPI GetTextExtent16(HDC16,LPCSTR,INT16);
BOOL16      WINAPI GetTextExtentPoint16(HDC16,LPCSTR,INT16,LPSIZE16);
INT16       WINAPI GetTextFace16(HDC16,INT16,LPSTR);
BOOL16      WINAPI GetTextMetrics16(HDC16,LPTEXTMETRIC16);
DWORD       WINAPI GetViewportExt16(HDC16);
BOOL16      WINAPI GetViewportExtEx16(HDC16,LPSIZE16);
BOOL16      WINAPI GetViewportOrgEx16(HDC16,LPPOINT16);
DWORD       WINAPI GetViewportOrg16(HDC16);
DWORD       WINAPI GetWindowExt16(HDC16);
DWORD       WINAPI GetWindowOrg16(HDC16);
BOOL16      WINAPI GetWindowExtEx16(HDC16,LPSIZE16);
BOOL16      WINAPI GetWindowOrgEx16(HDC16,LPPOINT16);
HRGN16      WINAPI InquireVisRgn16(HDC16);
INT16       WINAPI IntersectClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT16       WINAPI IntersectVisRect16(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI InvertRgn16(HDC16,HRGN16);
BOOL16      WINAPI IsDCCurrentPalette16(HDC16);
BOOL16      WINAPI IsGDIObject16(HGDIOBJ16);
BOOL16      WINAPI IsValidMetaFile16(HMETAFILE16);
VOID        WINAPI LineDDA16(INT16,INT16,INT16,INT16,LINEDDAPROC16,LPARAM);
BOOL16      WINAPI LineTo16(HDC16,INT16,INT16);
BOOL16      WINAPI LPtoDP16(HDC16,LPPOINT16,INT16);
DWORD       WINAPI MoveTo16(HDC16,INT16,INT16);
BOOL16      WINAPI MoveToEx16(HDC16,INT16,INT16,LPPOINT16);
INT16       WINAPI MulDiv16(INT16,INT16,INT16);
INT16       WINAPI OffsetClipRgn16(HDC16,INT16,INT16);
INT16       WINAPI OffsetRgn16(HRGN16,INT16,INT16);
DWORD       WINAPI OffsetViewportOrg16(HDC16,INT16,INT16);
BOOL16      WINAPI OffsetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
INT16       WINAPI OffsetVisRgn16(HDC16,INT16,INT16);
DWORD       WINAPI OffsetWindowOrg16(HDC16,INT16,INT16);
BOOL16      WINAPI OffsetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
HANDLE16    WINAPI OpenJob16(LPCSTR,LPCSTR,HDC16);
BOOL16      WINAPI PaintRgn16(HDC16,HRGN16);
BOOL16      WINAPI PatBlt16(HDC16,INT16,INT16,INT16,INT16,DWORD);
HRGN16      WINAPI PathToRegion16(HDC16);
BOOL16      WINAPI Pie16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI PlayMetaFile16(HDC16,HMETAFILE16);
VOID        WINAPI PlayMetaFileRecord16(HDC16,LPHANDLETABLE16,LPMETARECORD,UINT16);
BOOL16      WINAPI PolyBezier16(HDC16,const POINT16*,INT16);
BOOL16      WINAPI PolyBezierTo16(HDC16,const POINT16*,INT16);
BOOL16      WINAPI PolyPolygon16(HDC16,const POINT16*,const INT16*,UINT16);
BOOL16      WINAPI Polygon16(HDC16,const POINT16*,INT16);
BOOL16      WINAPI Polyline16(HDC16,const POINT16*,INT16);
BOOL16      WINAPI PtInRegion16(HRGN16,INT16,INT16);
BOOL16      WINAPI PtVisible16(HDC16,INT16,INT16);
BOOL16      WINAPI QueryAbort16(HDC16,INT16);
UINT16      WINAPI RealizeDefaultPalette16(HDC16);
BOOL16      WINAPI Rectangle16(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI RectInRegion16(HRGN16,const RECT16 *);
BOOL16      WINAPI RectVisible16(HDC16,const RECT16*);
BOOL16      WINAPI RemoveFontResource16(LPCSTR);
HDC16       WINAPI ResetDC16(HDC16,const DEVMODEA *);
BOOL16      WINAPI ResizePalette16(HPALETTE16,UINT16);
BOOL16      WINAPI RestoreDC16(HDC16,INT16);
INT16       WINAPI RestoreVisRgn16(HDC16);
VOID        WINAPI Resurrection16(HDC16,WORD,WORD,WORD,WORD,WORD,WORD);
BOOL16      WINAPI RoundRect16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16);
INT16       WINAPI SaveDC16(HDC16);
HRGN16      WINAPI SaveVisRgn16(HDC16);
DWORD       WINAPI ScaleViewportExt16(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI ScaleViewportExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
DWORD       WINAPI ScaleWindowExt16(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI ScaleWindowExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL16      WINAPI SelectClipPath16(HDC16,INT16);
INT16       WINAPI SelectClipRgn16(HDC16,HRGN16);
HGDIOBJ16   WINAPI SelectObject16(HDC16,HGDIOBJ16);
INT16       WINAPI SelectVisRgn16(HDC16,HRGN16);
INT16       WINAPI SetAbortProc16(HDC16,ABORTPROC16);
INT16       WINAPI SetArcDirection16(HDC16,INT16);
LONG        WINAPI SetBitmapBits16(HBITMAP16,LONG,LPCVOID);
DWORD       WINAPI SetBitmapDimension16(HBITMAP16,INT16,INT16);
BOOL16      WINAPI SetBitmapDimensionEx16(HBITMAP16,INT16,INT16,LPSIZE16);
COLORREF    WINAPI SetBkColor16(HDC16,COLORREF);
INT16       WINAPI SetBkMode16(HDC16,INT16);
UINT16      WINAPI SetBoundsRect16(HDC16,const RECT16*,UINT16);
DWORD       WINAPI SetBrushOrg16(HDC16,INT16,INT16);
BOOL16      WINAPI SetDCHook16(HDC16,FARPROC16,DWORD);
DWORD       WINAPI SetDCOrg16(HDC16,INT16,INT16);
VOID        WINAPI SetDCState16(HDC16,HDC16);
UINT16      WINAPI SetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
INT16       WINAPI SetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT16       WINAPI SetDIBitsToDevice16(HDC16,INT16,INT16,INT16,INT16,INT16,
                         INT16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT16       WINAPI SetEnvironment16(LPCSTR,LPDEVMODEA,UINT16);
WORD        WINAPI SetHookFlags16(HDC16,WORD);
INT16       WINAPI SetMapMode16(HDC16,INT16);
DWORD       WINAPI SetMapperFlags16(HDC16,DWORD);
HMETAFILE16 WINAPI SetMetaFileBits16(HGLOBAL16);
UINT16      WINAPI SetPaletteEntries16(HPALETTE16,UINT16,UINT16,const PALETTEENTRY*);
COLORREF    WINAPI SetPixel16(HDC16,INT16,INT16,COLORREF);
INT16       WINAPI SetPolyFillMode16(HDC16,INT16);
VOID        WINAPI SetRectRgn16(HRGN16,INT16,INT16,INT16,INT16);
INT16       WINAPI SetRelAbs16(HDC16,INT16);
INT16       WINAPI SetROP216(HDC16,INT16);
INT16       WINAPI SetStretchBltMode16(HDC16,INT16);
UINT16      WINAPI SetSystemPaletteUse16(HDC16,UINT16);
UINT16      WINAPI SetTextAlign16(HDC16,UINT16);
INT16       WINAPI SetTextCharacterExtra16(HDC16,INT16);
COLORREF    WINAPI SetTextColor16(HDC16,COLORREF);
INT16       WINAPI SetTextJustification16(HDC16,INT16,INT16);
DWORD       WINAPI SetViewportExt16(HDC16,INT16,INT16);
BOOL16      WINAPI SetViewportExtEx16(HDC16,INT16,INT16,LPSIZE16);
DWORD       WINAPI SetViewportOrg16(HDC16,INT16,INT16);
BOOL16      WINAPI SetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
DWORD       WINAPI SetWindowExt16(HDC16,INT16,INT16);
BOOL16      WINAPI SetWindowExtEx16(HDC16,INT16,INT16,LPSIZE16);
DWORD       WINAPI SetWindowOrg16(HDC16,INT16,INT16);
BOOL16      WINAPI SetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
INT16       WINAPI StartDoc16(HDC16,const DOCINFO16*);
INT16       WINAPI StartPage16(HDC16);
INT16       WINAPI StartSpoolPage16(HPJOB16);
BOOL16      WINAPI StretchBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,
                                INT16,INT16,INT16,DWORD);
INT16       WINAPI StretchDIBits16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,
                       INT16,INT16,const VOID*,const BITMAPINFO*,UINT16,DWORD);
BOOL16      WINAPI StrokeAndFillPath16(HDC16);
BOOL16      WINAPI StrokePath16(HDC16);
BOOL16      WINAPI TextOut16(HDC16,INT16,INT16,LPCSTR,INT16);
BOOL16      WINAPI UnrealizeObject16(HGDIOBJ16);
INT16       WINAPI UpdateColors16(HDC16);
BOOL16      WINAPI WidenPath16(HDC16);
INT16       WINAPI WriteDialog16(HPJOB16,LPSTR,INT16);
INT16       WINAPI WriteSpool16(HPJOB16,LPSTR,INT16);

#endif /* __WINE_WINE_WINGDI16_H */
