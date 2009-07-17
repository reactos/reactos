/*
 * GDI font objects
 *
 * Copyright 1993 Alexandre Julliard
 *           1997 Alex Korobka
 * Copyright 2002,2003 Shachar Shemesh
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wownt32.h"
#include "gdi_private.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(font);

  /* Device -> World size conversion */

/* Performs a device to world transformation on the specified width (which
 * is in integer format).
 */
static inline INT INTERNAL_XDSTOWS(DC *dc, INT width)
{
    double floatWidth;

    /* Perform operation with floating point */
    floatWidth = (double)width * dc->xformVport2World.eM11;
    /* Round to integers */
    return GDI_ROUND(floatWidth);
}

/* Performs a device to world transformation on the specified size (which
 * is in integer format).
 */
static inline INT INTERNAL_YDSTOWS(DC *dc, INT height)
{
    double floatHeight;

    /* Perform operation with floating point */
    floatHeight = (double)height * dc->xformVport2World.eM22;
    /* Round to integers */
    return GDI_ROUND(floatHeight);
}

static inline INT INTERNAL_XWSTODS(DC *dc, INT width)
{
    POINT pt[2];
    pt[0].x = pt[0].y = 0;
    pt[1].x = width;
    pt[1].y = 0;
    LPtoDP(dc->hSelf, pt, 2);
    return pt[1].x - pt[0].x;
}

static inline INT INTERNAL_YWSTODS(DC *dc, INT height)
{
    POINT pt[2];
    pt[0].x = pt[0].y = 0;
    pt[1].x = 0;
    pt[1].y = height;
    LPtoDP(dc->hSelf, pt, 2);
    return pt[1].y - pt[0].y;
}

static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, HDC hdc );
static INT FONT_GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer );
static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer );
static BOOL FONT_DeleteObject( HGDIOBJ handle );

static const struct gdi_obj_funcs font_funcs =
{
    FONT_SelectObject,  /* pSelectObject */
    FONT_GetObjectA,    /* pGetObjectA */
    FONT_GetObjectW,    /* pGetObjectW */
    NULL,               /* pUnrealizeObject */
    FONT_DeleteObject   /* pDeleteObject */
};

#define ENUM_UNICODE	0x00000001
#define ENUM_CALLED     0x00000002

typedef struct
{
    GDIOBJHDR   header;
    LOGFONTW    logfont;
} FONTOBJ;

typedef struct
{
  LPLOGFONTW          lpLogFontParam;
  FONTENUMPROCW       lpEnumFunc;
  LPARAM              lpData;
  DWORD               dwFlags;
  HDC                 hdc;
} fontEnum32;

/*
 *  For TranslateCharsetInfo
 */
#define MAXTCIINDEX 32
static const CHARSETINFO FONT_tci[MAXTCIINDEX] = {
  /* ANSI */
  { ANSI_CHARSET, 1252, {{0,0,0,0},{FS_LATIN1,0}} },
  { EASTEUROPE_CHARSET, 1250, {{0,0,0,0},{FS_LATIN2,0}} },
  { RUSSIAN_CHARSET, 1251, {{0,0,0,0},{FS_CYRILLIC,0}} },
  { GREEK_CHARSET, 1253, {{0,0,0,0},{FS_GREEK,0}} },
  { TURKISH_CHARSET, 1254, {{0,0,0,0},{FS_TURKISH,0}} },
  { HEBREW_CHARSET, 1255, {{0,0,0,0},{FS_HEBREW,0}} },
  { ARABIC_CHARSET, 1256, {{0,0,0,0},{FS_ARABIC,0}} },
  { BALTIC_CHARSET, 1257, {{0,0,0,0},{FS_BALTIC,0}} },
  { VIETNAMESE_CHARSET, 1258, {{0,0,0,0},{FS_VIETNAMESE,0}} },
  /* reserved by ANSI */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  /* ANSI and OEM */
  { THAI_CHARSET, 874, {{0,0,0,0},{FS_THAI,0}} },
  { SHIFTJIS_CHARSET, 932, {{0,0,0,0},{FS_JISJAPAN,0}} },
  { GB2312_CHARSET, 936, {{0,0,0,0},{FS_CHINESESIMP,0}} },
  { HANGEUL_CHARSET, 949, {{0,0,0,0},{FS_WANSUNG,0}} },
  { CHINESEBIG5_CHARSET, 950, {{0,0,0,0},{FS_CHINESETRAD,0}} },
  { JOHAB_CHARSET, 1361, {{0,0,0,0},{FS_JOHAB,0}} },
  /* reserved for alternate ANSI and OEM */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  /* reserved for system */
  { DEFAULT_CHARSET, 0, {{0,0,0,0},{FS_LATIN1,0}} },
  { SYMBOL_CHARSET, CP_SYMBOL, {{0,0,0,0},{FS_SYMBOL,0}} }
};

static void FONT_LogFontAToW( const LOGFONTA *fontA, LPLOGFONTW fontW )
{
    memcpy(fontW, fontA, sizeof(LOGFONTA) - LF_FACESIZE);
    MultiByteToWideChar(CP_ACP, 0, fontA->lfFaceName, -1, fontW->lfFaceName,
			LF_FACESIZE);
    fontW->lfFaceName[LF_FACESIZE-1] = 0;
}

static void FONT_LogFontWToA( const LOGFONTW *fontW, LPLOGFONTA fontA )
{
    memcpy(fontA, fontW, sizeof(LOGFONTA) - LF_FACESIZE);
    WideCharToMultiByte(CP_ACP, 0, fontW->lfFaceName, -1, fontA->lfFaceName,
			LF_FACESIZE, NULL, NULL);
    fontA->lfFaceName[LF_FACESIZE-1] = 0;
}

static void FONT_EnumLogFontExWToA( const ENUMLOGFONTEXW *fontW, LPENUMLOGFONTEXA fontA )
{
    FONT_LogFontWToA( (const LOGFONTW *)fontW, (LPLOGFONTA)fontA);

    WideCharToMultiByte( CP_ACP, 0, fontW->elfFullName, -1,
			 (LPSTR) fontA->elfFullName, LF_FULLFACESIZE, NULL, NULL );
    fontA->elfFullName[LF_FULLFACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfStyle, -1,
			 (LPSTR) fontA->elfStyle, LF_FACESIZE, NULL, NULL );
    fontA->elfStyle[LF_FACESIZE-1] = '\0';
    WideCharToMultiByte( CP_ACP, 0, fontW->elfScript, -1,
			 (LPSTR) fontA->elfScript, LF_FACESIZE, NULL, NULL );
    fontA->elfScript[LF_FACESIZE-1] = '\0';
}

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
static void FONT_TextMetricWToA(const TEXTMETRICW *ptmW, LPTEXTMETRICA ptmA )
{
    ptmA->tmHeight = ptmW->tmHeight;
    ptmA->tmAscent = ptmW->tmAscent;
    ptmA->tmDescent = ptmW->tmDescent;
    ptmA->tmInternalLeading = ptmW->tmInternalLeading;
    ptmA->tmExternalLeading = ptmW->tmExternalLeading;
    ptmA->tmAveCharWidth = ptmW->tmAveCharWidth;
    ptmA->tmMaxCharWidth = ptmW->tmMaxCharWidth;
    ptmA->tmWeight = ptmW->tmWeight;
    ptmA->tmOverhang = ptmW->tmOverhang;
    ptmA->tmDigitizedAspectX = ptmW->tmDigitizedAspectX;
    ptmA->tmDigitizedAspectY = ptmW->tmDigitizedAspectY;
    ptmA->tmFirstChar = min(ptmW->tmFirstChar, 255);
    if (ptmW->tmCharSet == SYMBOL_CHARSET)
    {
        ptmA->tmFirstChar = 0x1e;
        ptmA->tmLastChar = 0xff;  /* win9x behaviour - we need the OS2 table data to calculate correctly */
    }
    else
    {
        ptmA->tmFirstChar = ptmW->tmDefaultChar - 1;
        ptmA->tmLastChar = min(ptmW->tmLastChar, 0xff);
    }
    ptmA->tmDefaultChar = ptmW->tmDefaultChar;
    ptmA->tmBreakChar = ptmW->tmBreakChar;
    ptmA->tmItalic = ptmW->tmItalic;
    ptmA->tmUnderlined = ptmW->tmUnderlined;
    ptmA->tmStruckOut = ptmW->tmStruckOut;
    ptmA->tmPitchAndFamily = ptmW->tmPitchAndFamily;
    ptmA->tmCharSet = ptmW->tmCharSet;
}


static void FONT_NewTextMetricExWToA(const NEWTEXTMETRICEXW *ptmW, NEWTEXTMETRICEXA *ptmA )
{
    FONT_TextMetricWToA((const TEXTMETRICW *)ptmW, (LPTEXTMETRICA)ptmA);
    ptmA->ntmTm.ntmFlags = ptmW->ntmTm.ntmFlags;
    ptmA->ntmTm.ntmSizeEM = ptmW->ntmTm.ntmSizeEM;
    ptmA->ntmTm.ntmCellHeight = ptmW->ntmTm.ntmCellHeight;
    ptmA->ntmTm.ntmAvgWidth = ptmW->ntmTm.ntmAvgWidth;
    memcpy(&ptmA->ntmFontSig, &ptmW->ntmFontSig, sizeof(FONTSIGNATURE));
}


/***********************************************************************
 *           GdiGetCodePage   (GDI32.@)
 */
DWORD WINAPI GdiGetCodePage( HDC hdc )
{
    UINT cp = CP_ACP;
    DC *dc = get_dc_ptr( hdc );

    if (dc)
    {
        cp = dc->font_code_page;
        release_dc_ptr( dc );
    }
    return cp;
}

/***********************************************************************
 *           FONT_mbtowc
 *
 * Returns a Unicode translation of str using the charset of the
 * currently selected font in hdc.  If count is -1 then str is assumed
 * to be '\0' terminated, otherwise it contains the number of bytes to
 * convert.  If plenW is non-NULL, on return it will point to the
 * number of WCHARs that have been written.  If pCP is non-NULL, on
 * return it will point to the codepage used in the conversion.  The
 * caller should free the returned LPWSTR from the process heap
 * itself.
 */
static LPWSTR FONT_mbtowc(HDC hdc, LPCSTR str, INT count, INT *plenW, UINT *pCP)
{
    UINT cp;
    INT lenW;
    LPWSTR strW;

    cp = GdiGetCodePage( hdc );

    if(count == -1) count = strlen(str);
    lenW = MultiByteToWideChar(cp, 0, str, count, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW*sizeof(WCHAR));
    MultiByteToWideChar(cp, 0, str, count, strW, lenW);
    TRACE("mapped %s -> %s\n", debugstr_an(str, count), debugstr_wn(strW, lenW));
    if(plenW) *plenW = lenW;
    if(pCP) *pCP = cp;
    return strW;
}


/***********************************************************************
 *           CreateFontIndirectA   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectA( const LOGFONTA *plfA )
{
    LOGFONTW lfW;

    if (!plfA) return 0;

    FONT_LogFontAToW( plfA, &lfW );
    return CreateFontIndirectW( &lfW );
}

/***********************************************************************
 *           CreateFontIndirectW   (GDI32.@)
 */
HFONT WINAPI CreateFontIndirectW( const LOGFONTW *plf )
{
    static const WCHAR ItalicW[] = {' ','I','t','a','l','i','c','\0'};
    static const WCHAR BoldW[]   = {' ','B','o','l','d','\0'};
    WCHAR *pFaceNameItalicSuffix, *pFaceNameBoldSuffix;
    WCHAR *pFaceNameSuffix = NULL;
    HFONT hFont;
    FONTOBJ *fontPtr;

    if (!plf) return 0;

    if (!(fontPtr = HeapAlloc( GetProcessHeap(), 0, sizeof(*fontPtr) ))) return 0;

    fontPtr->logfont = *plf;

    if (plf->lfEscapement != plf->lfOrientation)
    {
        /* this should really depend on whether GM_ADVANCED is set */
        fontPtr->logfont.lfOrientation = fontPtr->logfont.lfEscapement;
        WARN("orientation angle %f set to "
             "escapement angle %f for new font %p\n",
             plf->lfOrientation/10., plf->lfEscapement/10., fontPtr);
    }

    pFaceNameItalicSuffix = strstrW(fontPtr->logfont.lfFaceName, ItalicW);
    if (pFaceNameItalicSuffix)
    {
        fontPtr->logfont.lfItalic = TRUE;
        pFaceNameSuffix = pFaceNameItalicSuffix;
    }

    pFaceNameBoldSuffix = strstrW(fontPtr->logfont.lfFaceName, BoldW);
    if (pFaceNameBoldSuffix)
    {
        if (fontPtr->logfont.lfWeight < FW_BOLD)
            fontPtr->logfont.lfWeight = FW_BOLD;
        if (!pFaceNameSuffix || (pFaceNameBoldSuffix < pFaceNameSuffix))
            pFaceNameSuffix = pFaceNameBoldSuffix;
    }

    if (pFaceNameSuffix) *pFaceNameSuffix = 0;

    if (!(hFont = alloc_gdi_handle( &fontPtr->header, OBJ_FONT, &font_funcs )))
    {
        HeapFree( GetProcessHeap(), 0, fontPtr );
        return 0;
    }

    TRACE("(%d %d %d %d %x %d %x %d %d) %s %s %s %s => %p\n",
          plf->lfHeight, plf->lfWidth,
          plf->lfEscapement, plf->lfOrientation,
          plf->lfPitchAndFamily,
          plf->lfOutPrecision, plf->lfClipPrecision,
          plf->lfQuality, plf->lfCharSet,
          debugstr_w(plf->lfFaceName),
          plf->lfWeight > 400 ? "Bold" : "",
          plf->lfItalic ? "Italic" : "",
          plf->lfUnderline ? "Underline" : "", hFont);

    return hFont;
}

/*************************************************************************
 *           CreateFontA    (GDI32.@)
 */
HFONT WINAPI CreateFontA( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCSTR name )
{
    LOGFONTA logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;

    if (name)
	lstrcpynA(logfont.lfFaceName,name,sizeof(logfont.lfFaceName));
    else
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectA( &logfont );
}

/*************************************************************************
 *           CreateFontW    (GDI32.@)
 */
HFONT WINAPI CreateFontW( INT height, INT width, INT esc,
                              INT orient, INT weight, DWORD italic,
                              DWORD underline, DWORD strikeout, DWORD charset,
                              DWORD outpres, DWORD clippres, DWORD quality,
                              DWORD pitch, LPCWSTR name )
{
    LOGFONTW logfont;

    logfont.lfHeight = height;
    logfont.lfWidth = width;
    logfont.lfEscapement = esc;
    logfont.lfOrientation = orient;
    logfont.lfWeight = weight;
    logfont.lfItalic = italic;
    logfont.lfUnderline = underline;
    logfont.lfStrikeOut = strikeout;
    logfont.lfCharSet = charset;
    logfont.lfOutPrecision = outpres;
    logfont.lfClipPrecision = clippres;
    logfont.lfQuality = quality;
    logfont.lfPitchAndFamily = pitch;

    if (name)
	lstrcpynW(logfont.lfFaceName, name,
		  sizeof(logfont.lfFaceName) / sizeof(WCHAR));
    else
	logfont.lfFaceName[0] = '\0';

    return CreateFontIndirectW( &logfont );
}

static void update_font_code_page( DC *dc )
{
    CHARSETINFO csi;
    int charset = DEFAULT_CHARSET;

    if (dc->gdiFont)
        charset = WineEngGetTextCharsetInfo( dc->gdiFont, NULL, 0 );

    /* Hmm, nicely designed api this one! */
    if (TranslateCharsetInfo( ULongToPtr(charset), &csi, TCI_SRCCHARSET) )
        dc->font_code_page = csi.ciACP;
    else {
        switch(charset) {
        case OEM_CHARSET:
            dc->font_code_page = GetOEMCP();
            break;
        case DEFAULT_CHARSET:
            dc->font_code_page = GetACP();
            break;

        case VISCII_CHARSET:
        case TCVN_CHARSET:
        case KOI8_CHARSET:
        case ISO3_CHARSET:
        case ISO4_CHARSET:
        case ISO10_CHARSET:
        case CELTIC_CHARSET:
            /* FIXME: These have no place here, but because x11drv
               enumerates fonts with these (made up) charsets some apps
               might use them and then the FIXME below would become
               annoying.  Now we could pick the intended codepage for
               each of these, but since it's broken anyway we'll just
               use CP_ACP and hope it'll go away...
            */
            dc->font_code_page = CP_ACP;
            break;

        default:
            FIXME("Can't find codepage for charset %d\n", charset);
            dc->font_code_page = CP_ACP;
            break;
        }
    }

    TRACE("charset %d => cp %d\n", charset, dc->font_code_page);
}

/***********************************************************************
 *           FONT_SelectObject
 *
 * If the driver supports vector fonts we create a gdi font first and
 * then call the driver to give it a chance to supply its own device
 * font.  If the driver wants to do this it returns TRUE and we can
 * delete the gdi font, if the driver wants to use the gdi font it
 * should return FALSE, to signal an error return GDI_ERROR.  For
 * drivers that don't support vector fonts they must supply their own
 * font.
 */
static HGDIOBJ FONT_SelectObject( HGDIOBJ handle, HDC hdc )
{
    HGDIOBJ ret = 0;
    DC *dc = get_dc_ptr( hdc );

    if (!dc) return 0;

    if (!GDI_inc_ref_count( handle ))
    {
        release_dc_ptr( dc );
        return 0;
    }

    if (GetDeviceCaps( dc->hSelf, TEXTCAPS ) & TC_VA_ABLE)
        dc->gdiFont = WineEngCreateFontInstance( dc, handle );

    if (dc->funcs->pSelectFont) ret = dc->funcs->pSelectFont( dc->physDev, handle, dc->gdiFont );

    if (ret && dc->gdiFont) dc->gdiFont = 0;

    if (ret == HGDI_ERROR)
    {
        GDI_dec_ref_count( handle );
        ret = 0; /* SelectObject returns 0 on error */
    }
    else
    {
        ret = dc->hFont;
        dc->hFont = handle;
        update_font_code_page( dc );
        GDI_dec_ref_count( ret );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           FONT_GetObjectA
 */
static INT FONT_GetObjectA( HGDIOBJ handle, INT count, LPVOID buffer )
{
    FONTOBJ *font = GDI_GetObjPtr( handle, OBJ_FONT );
    LOGFONTA lfA;

    if (!font) return 0;
    if (buffer)
    {
        FONT_LogFontWToA( &font->logfont, &lfA );
        if (count > sizeof(lfA)) count = sizeof(lfA);
        memcpy( buffer, &lfA, count );
    }
    else count = sizeof(lfA);
    GDI_ReleaseObj( handle );
    return count;
}

/***********************************************************************
 *           FONT_GetObjectW
 */
static INT FONT_GetObjectW( HGDIOBJ handle, INT count, LPVOID buffer )
{
    FONTOBJ *font = GDI_GetObjPtr( handle, OBJ_FONT );

    if (!font) return 0;
    if (buffer)
    {
        if (count > sizeof(LOGFONTW)) count = sizeof(LOGFONTW);
        memcpy( buffer, &font->logfont, count );
    }
    else count = sizeof(LOGFONTW);
    GDI_ReleaseObj( handle );
    return count;
}


/***********************************************************************
 *           FONT_DeleteObject
 */
static BOOL FONT_DeleteObject( HGDIOBJ handle )
{
    FONTOBJ *obj;

    WineEngDestroyFontInstance( handle );

    if (!(obj = free_gdi_handle( handle ))) return FALSE;
    return HeapFree( GetProcessHeap(), 0, obj );
}


/***********************************************************************
 *              FONT_EnumInstance
 *
 * Note: plf is really an ENUMLOGFONTEXW, and ptm is a NEWTEXTMETRICEXW.
 *       We have to use other types because of the FONTENUMPROCW definition.
 */
static INT CALLBACK FONT_EnumInstance( const LOGFONTW *plf, const TEXTMETRICW *ptm,
                                       DWORD fType, LPARAM lp )
{
    fontEnum32 *pfe = (fontEnum32*)lp;
    INT ret = 1;

    /* lfCharSet is at the same offset in both LOGFONTA and LOGFONTW */
    if ((!pfe->lpLogFontParam ||
        pfe->lpLogFontParam->lfCharSet == DEFAULT_CHARSET ||
        pfe->lpLogFontParam->lfCharSet == plf->lfCharSet) &&
       (!(fType & RASTER_FONTTYPE) || GetDeviceCaps(pfe->hdc, TEXTCAPS) & TC_RA_ABLE) )
    {
	/* convert font metrics */
        ENUMLOGFONTEXA logfont;
        NEWTEXTMETRICEXA tmA;

        pfe->dwFlags |= ENUM_CALLED;
        if (!(pfe->dwFlags & ENUM_UNICODE))
        {
            FONT_EnumLogFontExWToA( (const ENUMLOGFONTEXW *)plf, &logfont);
            FONT_NewTextMetricExWToA( (const NEWTEXTMETRICEXW *)ptm, &tmA );
            plf = (LOGFONTW *)&logfont.elfLogFont;
            ptm = (TEXTMETRICW *)&tmA;
        }

        ret = pfe->lpEnumFunc( plf, ptm, fType, pfe->lpData );
    }
    return ret;
}

/***********************************************************************
 *		FONT_EnumFontFamiliesEx
 */
static INT FONT_EnumFontFamiliesEx( HDC hDC, LPLOGFONTW plf,
				    FONTENUMPROCW efproc,
				    LPARAM lParam, DWORD dwUnicode)
{
    INT ret = 1, ret2;
    DC *dc = get_dc_ptr( hDC );
    fontEnum32 fe32;
    BOOL enum_gdi_fonts;

    if (!dc) return 0;

    if (plf)
        TRACE("lfFaceName = %s lfCharset = %d\n", debugstr_w(plf->lfFaceName),
	    plf->lfCharSet);
    fe32.lpLogFontParam = plf;
    fe32.lpEnumFunc = efproc;
    fe32.lpData = lParam;
    fe32.dwFlags = dwUnicode;
    fe32.hdc = hDC;

    enum_gdi_fonts = GetDeviceCaps(hDC, TEXTCAPS) & TC_VA_ABLE;

    if (!dc->funcs->pEnumDeviceFonts && !enum_gdi_fonts)
    {
        ret = 0;
        goto done;
    }

    if (enum_gdi_fonts)
        ret = WineEngEnumFonts( plf, FONT_EnumInstance, (LPARAM)&fe32 );
    fe32.dwFlags &= ~ENUM_CALLED;
    if (ret && dc->funcs->pEnumDeviceFonts) {
	ret2 = dc->funcs->pEnumDeviceFonts( dc->physDev, plf, FONT_EnumInstance, (LPARAM)&fe32 );
	if(fe32.dwFlags & ENUM_CALLED) /* update ret iff a font gets enumed */
	    ret = ret2;
    }
 done:
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *              EnumFontFamiliesExW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExW( HDC hDC, LPLOGFONTW plf,
                                    FONTENUMPROCW efproc,
                                    LPARAM lParam, DWORD dwFlags )
{
    return  FONT_EnumFontFamiliesEx( hDC, plf, efproc, lParam, ENUM_UNICODE );
}

/***********************************************************************
 *              EnumFontFamiliesExA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesExA( HDC hDC, LPLOGFONTA plf,
                                    FONTENUMPROCA efproc,
                                    LPARAM lParam, DWORD dwFlags)
{
    LOGFONTW lfW, *plfW;

    if (plf)
    {
        FONT_LogFontAToW( plf, &lfW );
        plfW = &lfW;
    }
    else plfW = NULL;

    return FONT_EnumFontFamiliesEx( hDC, plfW, (FONTENUMPROCW)efproc, lParam, 0);
}

/***********************************************************************
 *              EnumFontFamiliesA	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesA( HDC hDC, LPCSTR lpFamily,
                                  FONTENUMPROCA efproc, LPARAM lpData )
{
    LOGFONTA lf, *plf;

    if (lpFamily)
    {
        if (!*lpFamily) return 1;
        lstrcpynA( lf.lfFaceName, lpFamily, LF_FACESIZE );
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        plf = &lf;
    }
    else plf = NULL;

    return EnumFontFamiliesExA( hDC, plf, efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontFamiliesW	(GDI32.@)
 */
INT WINAPI EnumFontFamiliesW( HDC hDC, LPCWSTR lpFamily,
                                  FONTENUMPROCW efproc, LPARAM lpData )
{
    LOGFONTW lf, *plf;

    if (lpFamily)
    {
        if (!*lpFamily) return 1;
        lstrcpynW( lf.lfFaceName, lpFamily, LF_FACESIZE );
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfPitchAndFamily = 0;
        plf = &lf;
    }
    else plf = NULL;

    return EnumFontFamiliesExW( hDC, plf, efproc, lpData, 0 );
}

/***********************************************************************
 *              EnumFontsA		(GDI32.@)
 */
INT WINAPI EnumFontsA( HDC hDC, LPCSTR lpName, FONTENUMPROCA efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesA( hDC, lpName, efproc, lpData );
}

/***********************************************************************
 *              EnumFontsW		(GDI32.@)
 */
INT WINAPI EnumFontsW( HDC hDC, LPCWSTR lpName, FONTENUMPROCW efproc,
                           LPARAM lpData )
{
    return EnumFontFamiliesW( hDC, lpName, efproc, lpData );
}


/***********************************************************************
 *           GetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI GetTextCharacterExtra( HDC hdc )
{
    INT ret;
    DC *dc = get_dc_ptr( hdc );
    if (!dc) return 0x80000000;
    ret = dc->charExtra;
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           SetTextCharacterExtra    (GDI32.@)
 */
INT WINAPI SetTextCharacterExtra( HDC hdc, INT extra )
{
    INT prev;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return 0x80000000;
    if (dc->funcs->pSetTextCharacterExtra)
        prev = dc->funcs->pSetTextCharacterExtra( dc->physDev, extra );
    else
    {
        prev = dc->charExtra;
        dc->charExtra = extra;
    }
    release_dc_ptr( dc );
    return prev;
}


/***********************************************************************
 *           SetTextJustification    (GDI32.@)
 */
BOOL WINAPI SetTextJustification( HDC hdc, INT extra, INT breaks )
{
    BOOL ret = TRUE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;
    if (dc->funcs->pSetTextJustification)
        ret = dc->funcs->pSetTextJustification( dc->physDev, extra, breaks );
    else
    {
        extra = abs((extra * dc->vportExtX + dc->wndExtX / 2) / dc->wndExtX);
        if (!extra) breaks = 0;
        if (breaks)
        {
            dc->breakExtra = extra / breaks;
            dc->breakRem   = extra - (breaks * dc->breakExtra);
        }
        else
        {
            dc->breakExtra = 0;
            dc->breakRem   = 0;
        }
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetTextFaceA    (GDI32.@)
 */
INT WINAPI GetTextFaceA( HDC hdc, INT count, LPSTR name )
{
    INT res = GetTextFaceW(hdc, 0, NULL);
    LPWSTR nameW = HeapAlloc( GetProcessHeap(), 0, res * 2 );
    GetTextFaceW( hdc, res, nameW );

    if (name)
    {
        if (count)
        {
            res = WideCharToMultiByte(CP_ACP, 0, nameW, -1, name, count, NULL, NULL);
            if (res == 0)
                res = count;
            name[count-1] = 0;
            /* GetTextFaceA does NOT include the nul byte in the return count.  */
            res--;
        }
        else
            res = 0;
    }
    else
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL);
    HeapFree( GetProcessHeap(), 0, nameW );
    return res;
}

/***********************************************************************
 *           GetTextFaceW    (GDI32.@)
 */
INT WINAPI GetTextFaceW( HDC hdc, INT count, LPWSTR name )
{
    FONTOBJ *font;
    INT     ret = 0;

    DC * dc = get_dc_ptr( hdc );
    if (!dc) return 0;

    if(dc->gdiFont)
        ret = WineEngGetTextFace(dc->gdiFont, count, name);
    else if ((font = GDI_GetObjPtr( dc->hFont, OBJ_FONT )))
    {
        INT n = strlenW(font->logfont.lfFaceName) + 1;
        if (name)
        {
            lstrcpynW( name, font->logfont.lfFaceName, count );
            ret = min(count, n);
        }
        else ret = n;
        GDI_ReleaseObj( dc->hFont );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetTextExtentPoint32A    (GDI32.@)
 *
 * See GetTextExtentPoint32W.
 */
BOOL WINAPI GetTextExtentPoint32A( HDC hdc, LPCSTR str, INT count,
                                     LPSIZE size )
{
    BOOL ret = FALSE;
    INT wlen;
    LPWSTR p = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    if (p) {
	ret = GetTextExtentPoint32W( hdc, p, wlen, size );
	HeapFree( GetProcessHeap(), 0, p );
    }

    TRACE("(%p %s %d %p): returning %d x %d\n",
          hdc, debugstr_an (str, count), count, size, size->cx, size->cy );
    return ret;
}


/***********************************************************************
 * GetTextExtentPoint32W [GDI32.@]
 *
 * Computes width/height for a string.
 *
 * Computes width and height of the specified string.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentPoint32W(
    HDC hdc,     /* [in]  Handle of device context */
    LPCWSTR str,   /* [in]  Address of text string */
    INT count,   /* [in]  Number of characters in string */
    LPSIZE size) /* [out] Address of structure for string size */
{
    return GetTextExtentExPointW(hdc, str, count, 0, NULL, NULL, size);
}

/***********************************************************************
 * GetTextExtentExPointI [GDI32.@]
 *
 * Computes width and height of the array of glyph indices.
 *
 * PARAMS
 *    hdc     [I] Handle of device context.
 *    indices [I] Glyph index array.
 *    count   [I] Number of glyphs in array.
 *    max_ext [I] Maximum width in glyphs.
 *    nfit    [O] Maximum number of characters.
 *    dxs     [O] Partial string widths.
 *    size    [O] Returned string size.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentExPointI( HDC hdc, const WORD *indices, INT count, INT max_ext,
                                   LPINT nfit, LPINT dxs, LPSIZE size )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if(dc->gdiFont) {
        ret = WineEngGetTextExtentExPointI(dc->gdiFont, indices, count, max_ext, nfit, dxs, size);
        size->cx = abs(INTERNAL_XDSTOWS(dc, size->cx));
        size->cy = abs(INTERNAL_YDSTOWS(dc, size->cy));
        size->cx += count * dc->charExtra;
    }
    else if(dc->funcs->pGetTextExtentExPoint) {
        FIXME("calling GetTextExtentExPoint\n");
        ret = dc->funcs->pGetTextExtentExPoint( dc->physDev, indices, count,
                                                max_ext, nfit, dxs, size );
    }

    release_dc_ptr( dc );

    TRACE("(%p %p %d %p): returning %d x %d\n",
          hdc, indices, count, size, size->cx, size->cy );
    return ret;
}

/***********************************************************************
 * GetTextExtentPointI [GDI32.@]
 *
 * Computes width and height of the array of glyph indices.
 *
 * PARAMS
 *    hdc     [I] Handle of device context.
 *    indices [I] Glyph index array.
 *    count   [I] Number of glyphs in array.
 *    size    [O] Returned string size.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetTextExtentPointI( HDC hdc, const WORD *indices, INT count, LPSIZE size )
{
    return GetTextExtentExPointI( hdc, indices, count, 0, NULL, NULL, size );
}


/***********************************************************************
 *           GetTextExtentPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointA( HDC hdc, LPCSTR str, INT count,
                                          LPSIZE size )
{
    TRACE("not bug compatible.\n");
    return GetTextExtentPoint32A( hdc, str, count, size );
}

/***********************************************************************
 *           GetTextExtentPointW   (GDI32.@)
 */
BOOL WINAPI GetTextExtentPointW( HDC hdc, LPCWSTR str, INT count,
                                          LPSIZE size )
{
    TRACE("not bug compatible.\n");
    return GetTextExtentPoint32W( hdc, str, count, size );
}


/***********************************************************************
 *           GetTextExtentExPointA    (GDI32.@)
 */
BOOL WINAPI GetTextExtentExPointA( HDC hdc, LPCSTR str, INT count,
				   INT maxExt, LPINT lpnFit,
				   LPINT alpDx, LPSIZE size )
{
    BOOL ret;
    INT wlen;
    INT *walpDx = NULL;
    LPWSTR p = NULL;
    
    if (alpDx &&
       NULL == (walpDx = HeapAlloc(GetProcessHeap(), 0, count * sizeof(INT))))
       return FALSE;
    
    p = FONT_mbtowc(hdc, str, count, &wlen, NULL);
    ret = GetTextExtentExPointW( hdc, p, wlen, maxExt, lpnFit, walpDx, size);
    if (walpDx)
    {
        INT n = lpnFit ? *lpnFit : wlen;
        INT i, j;
        for(i = 0, j = 0; i < n; i++, j++)
        {
            alpDx[j] = walpDx[i];
            if (IsDBCSLeadByte(str[j])) alpDx[++j] = walpDx[i];
        }
    }
    if (lpnFit) *lpnFit = WideCharToMultiByte(CP_ACP,0,p,*lpnFit,NULL,0,NULL,NULL);
    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, walpDx );
    return ret;
}


/***********************************************************************
 *           GetTextExtentExPointW    (GDI32.@)
 *
 * Return the size of the string as it would be if it was output properly by
 * e.g. TextOut.
 *
 * This should include
 * - Intercharacter spacing
 * - justification spacing (not yet done)
 * - kerning? see below
 *
 * Kerning.  Since kerning would be carried out by the rendering code it should
 * be done by the driver.  However they don't support it yet.  Also I am not
 * yet persuaded that (certainly under Win95) any kerning is actually done.
 *
 * str: According to MSDN this should be null-terminated.  That is not true; a
 *      null will not terminate it early.
 * size: Certainly under Win95 this appears buggy or weird if *lpnFit is less
 *       than count.  I have seen it be either the size of the full string or
 *       1 less than the size of the full string.  I have not seen it bear any
 *       resemblance to the portion that would fit.
 * lpnFit: What exactly is fitting?  Stupidly, in my opinion, it includes the
 *         trailing intercharacter spacing and any trailing justification.
 *
 * FIXME
 * Currently we do this by measuring each character etc.  We should do it by
 * passing the request to the driver, perhaps by extending the
 * pGetTextExtentPoint function to take the alpDx argument.  That would avoid
 * thinking about kerning issues and rounding issues in the justification.
 */

BOOL WINAPI GetTextExtentExPointW( HDC hdc, LPCWSTR str, INT count,
				   INT maxExt, LPINT lpnFit,
				   LPINT alpDx, LPSIZE size )
{
    INT nFit = 0;
    LPINT dxs = NULL;
    DC *dc;
    BOOL ret = FALSE;
    TEXTMETRICW tm;

    TRACE("(%p, %s, %d)\n",hdc,debugstr_wn(str,count),maxExt);

    dc = get_dc_ptr(hdc);
    if (! dc)
        return FALSE;

    GetTextMetricsW(hdc, &tm);

    /* If we need to calculate nFit, then we need the partial extents even if
       the user hasn't provided us with an array.  */
    if (lpnFit)
    {
	dxs = alpDx ? alpDx : HeapAlloc(GetProcessHeap(), 0, count * sizeof alpDx[0]);
	if (! dxs)
	{
	    release_dc_ptr(dc);
	    SetLastError(ERROR_OUTOFMEMORY);
	    return FALSE;
	}
    }
    else
	dxs = alpDx;

    if (dc->gdiFont)
	ret = WineEngGetTextExtentExPoint(dc->gdiFont, str, count,
					  0, NULL, dxs, size);
    else if (dc->funcs->pGetTextExtentExPoint)
	ret = dc->funcs->pGetTextExtentExPoint(dc->physDev, str, count,
					       0, NULL, dxs, size);

    /* Perform device size to world size transformations.  */
    if (ret)
    {
	INT extra      = dc->charExtra,
        breakExtra = dc->breakExtra,
        breakRem   = dc->breakRem,
        i;

	if (dxs)
	{
	    for (i = 0; i < count; ++i)
	    {
		dxs[i] = abs(INTERNAL_XDSTOWS(dc, dxs[i]));
		dxs[i] += (i+1) * extra;
                if (count > 1 && (breakExtra || breakRem) && str[i] == tm.tmBreakChar)
                {
                    dxs[i] += breakExtra;
                    if (breakRem > 0)
                    {
                        breakRem--;
                        dxs[i]++;
                    }
                }
		if (dxs[i] <= maxExt)
		    ++nFit;
	    }
            breakRem = dc->breakRem;
	}
	size->cx = abs(INTERNAL_XDSTOWS(dc, size->cx));
	size->cy = abs(INTERNAL_YDSTOWS(dc, size->cy));

        if (!dxs && count > 1 && (breakExtra || breakRem))
        {
            for (i = 0; i < count; i++)
            {
                if (str[i] == tm.tmBreakChar)
                {
                    size->cx += breakExtra;
                    if (breakRem > 0)
                    {
                        breakRem--;
                        (size->cx)++;
                    }
                }
            }
        }
    }

    if (lpnFit)
	*lpnFit = nFit;

    if (! alpDx)
        HeapFree(GetProcessHeap(), 0, dxs);

    release_dc_ptr( dc );

    TRACE("returning %d %d x %d\n",nFit,size->cx,size->cy);
    return ret;
}

/***********************************************************************
 *           GetTextMetricsA    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsA( HDC hdc, TEXTMETRICA *metrics )
{
    TEXTMETRICW tm32;

    if (!GetTextMetricsW( hdc, &tm32 )) return FALSE;
    FONT_TextMetricWToA( &tm32, metrics );
    return TRUE;
}

/***********************************************************************
 *           GetTextMetricsW    (GDI32.@)
 */
BOOL WINAPI GetTextMetricsW( HDC hdc, TEXTMETRICW *metrics )
{
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if (dc->gdiFont)
        ret = WineEngGetTextMetrics(dc->gdiFont, metrics);
    else if (dc->funcs->pGetTextMetrics)
        ret = dc->funcs->pGetTextMetrics( dc->physDev, metrics );

    if (ret)
    {
    /* device layer returns values in device units
     * therefore we have to convert them to logical */

        metrics->tmDigitizedAspectX = GetDeviceCaps(hdc, LOGPIXELSX);
        metrics->tmDigitizedAspectY = GetDeviceCaps(hdc, LOGPIXELSY);

#define WDPTOLP(x) ((x<0)?					\
		(-abs(INTERNAL_XDSTOWS(dc, (x)))):		\
		(abs(INTERNAL_XDSTOWS(dc, (x)))))
#define HDPTOLP(y) ((y<0)?					\
		(-abs(INTERNAL_YDSTOWS(dc, (y)))):		\
		(abs(INTERNAL_YDSTOWS(dc, (y)))))

    metrics->tmHeight           = HDPTOLP(metrics->tmHeight);
    metrics->tmAscent           = HDPTOLP(metrics->tmAscent);
    metrics->tmDescent          = HDPTOLP(metrics->tmDescent);
    metrics->tmInternalLeading  = HDPTOLP(metrics->tmInternalLeading);
    metrics->tmExternalLeading  = HDPTOLP(metrics->tmExternalLeading);
    metrics->tmAveCharWidth     = WDPTOLP(metrics->tmAveCharWidth);
    metrics->tmMaxCharWidth     = WDPTOLP(metrics->tmMaxCharWidth);
    metrics->tmOverhang         = WDPTOLP(metrics->tmOverhang);
        ret = TRUE;
#undef WDPTOLP
#undef HDPTOLP
    TRACE("text metrics:\n"
          "    Weight = %03i\t FirstChar = %i\t AveCharWidth = %i\n"
          "    Italic = % 3i\t LastChar = %i\t\t MaxCharWidth = %i\n"
          "    UnderLined = %01i\t DefaultChar = %i\t Overhang = %i\n"
          "    StruckOut = %01i\t BreakChar = %i\t CharSet = %i\n"
          "    PitchAndFamily = %02x\n"
          "    --------------------\n"
          "    InternalLeading = %i\n"
          "    Ascent = %i\n"
          "    Descent = %i\n"
          "    Height = %i\n",
          metrics->tmWeight, metrics->tmFirstChar, metrics->tmAveCharWidth,
          metrics->tmItalic, metrics->tmLastChar, metrics->tmMaxCharWidth,
          metrics->tmUnderlined, metrics->tmDefaultChar, metrics->tmOverhang,
          metrics->tmStruckOut, metrics->tmBreakChar, metrics->tmCharSet,
          metrics->tmPitchAndFamily,
          metrics->tmInternalLeading,
          metrics->tmAscent,
          metrics->tmDescent,
          metrics->tmHeight );
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *		GetOutlineTextMetricsA (GDI32.@)
 * Gets metrics for TrueType fonts.
 *
 * NOTES
 *    If the supplied buffer isn't big enough Windows partially fills it up to
 *    its given length and returns that length.
 *
 * RETURNS
 *    Success: Non-zero or size of required buffer
 *    Failure: 0
 */
UINT WINAPI GetOutlineTextMetricsA(
    HDC hdc,    /* [in]  Handle of device context */
    UINT cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRICA lpOTM)  /* [out] Address of metric data array */
{
    char buf[512], *ptr;
    UINT ret, needed;
    OUTLINETEXTMETRICW *lpOTMW = (OUTLINETEXTMETRICW *)buf;
    OUTLINETEXTMETRICA *output = lpOTM;
    INT left, len;

    if((ret = GetOutlineTextMetricsW(hdc, 0, NULL)) == 0)
        return 0;
    if(ret > sizeof(buf))
	lpOTMW = HeapAlloc(GetProcessHeap(), 0, ret);
    GetOutlineTextMetricsW(hdc, ret, lpOTMW);

    needed = sizeof(OUTLINETEXTMETRICA);
    if(lpOTMW->otmpFamilyName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFamilyName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFaceName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFaceName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpStyleName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpStyleName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFullName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFullName), -1,
				      NULL, 0, NULL, NULL);

    if(!lpOTM) {
        ret = needed;
	goto end;
    }

    TRACE("needed = %d\n", needed);
    if(needed > cbData)
        /* Since the supplied buffer isn't big enough, we'll alloc one
           that is and memcpy the first cbData bytes into the lpOTM at
           the end. */
        output = HeapAlloc(GetProcessHeap(), 0, needed);

    ret = output->otmSize = min(needed, cbData);
    FONT_TextMetricWToA( &lpOTMW->otmTextMetrics, &output->otmTextMetrics );
    output->otmFiller = 0;
    output->otmPanoseNumber = lpOTMW->otmPanoseNumber;
    output->otmfsSelection = lpOTMW->otmfsSelection;
    output->otmfsType = lpOTMW->otmfsType;
    output->otmsCharSlopeRise = lpOTMW->otmsCharSlopeRise;
    output->otmsCharSlopeRun = lpOTMW->otmsCharSlopeRun;
    output->otmItalicAngle = lpOTMW->otmItalicAngle;
    output->otmEMSquare = lpOTMW->otmEMSquare;
    output->otmAscent = lpOTMW->otmAscent;
    output->otmDescent = lpOTMW->otmDescent;
    output->otmLineGap = lpOTMW->otmLineGap;
    output->otmsCapEmHeight = lpOTMW->otmsCapEmHeight;
    output->otmsXHeight = lpOTMW->otmsXHeight;
    output->otmrcFontBox = lpOTMW->otmrcFontBox;
    output->otmMacAscent = lpOTMW->otmMacAscent;
    output->otmMacDescent = lpOTMW->otmMacDescent;
    output->otmMacLineGap = lpOTMW->otmMacLineGap;
    output->otmusMinimumPPEM = lpOTMW->otmusMinimumPPEM;
    output->otmptSubscriptSize = lpOTMW->otmptSubscriptSize;
    output->otmptSubscriptOffset = lpOTMW->otmptSubscriptOffset;
    output->otmptSuperscriptSize = lpOTMW->otmptSuperscriptSize;
    output->otmptSuperscriptOffset = lpOTMW->otmptSuperscriptOffset;
    output->otmsStrikeoutSize = lpOTMW->otmsStrikeoutSize;
    output->otmsStrikeoutPosition = lpOTMW->otmsStrikeoutPosition;
    output->otmsUnderscoreSize = lpOTMW->otmsUnderscoreSize;
    output->otmsUnderscorePosition = lpOTMW->otmsUnderscorePosition;


    ptr = (char*)(output + 1);
    left = needed - sizeof(*output);

    if(lpOTMW->otmpFamilyName) {
        output->otmpFamilyName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFamilyName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFamilyName = 0;

    if(lpOTMW->otmpFaceName) {
        output->otmpFaceName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFaceName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFaceName = 0;

    if(lpOTMW->otmpStyleName) {
        output->otmpStyleName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpStyleName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpStyleName = 0;

    if(lpOTMW->otmpFullName) {
        output->otmpFullName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (ptrdiff_t)lpOTMW->otmpFullName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
    } else
        output->otmpFullName = 0;

    assert(left == 0);

    if(output != lpOTM) {
        memcpy(lpOTM, output, cbData);
        HeapFree(GetProcessHeap(), 0, output);

        /* check if the string offsets really fit into the provided size */
        /* FIXME: should we check string length as well? */
        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFamilyName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFamilyName >= lpOTM->otmSize)
                lpOTM->otmpFamilyName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFaceName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFaceName >= lpOTM->otmSize)
                lpOTM->otmpFaceName = 0; /* doesn't fit */
        }

            /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpStyleName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpStyleName >= lpOTM->otmSize)
                lpOTM->otmpStyleName = 0; /* doesn't fit */
        }

        /* make sure that we don't read/write beyond the provided buffer */
        if (lpOTM->otmSize >= FIELD_OFFSET(OUTLINETEXTMETRICA, otmpFullName) + sizeof(LPSTR))
        {
            if ((UINT_PTR)lpOTM->otmpFullName >= lpOTM->otmSize)
                lpOTM->otmpFullName = 0; /* doesn't fit */
        }
    }

end:
    if(lpOTMW != (OUTLINETEXTMETRICW *)buf)
        HeapFree(GetProcessHeap(), 0, lpOTMW);

    return ret;
}


/***********************************************************************
 *           GetOutlineTextMetricsW [GDI32.@]
 */
UINT WINAPI GetOutlineTextMetricsW(
    HDC hdc,    /* [in]  Handle of device context */
    UINT cbData, /* [in]  Size of metric data array */
    LPOUTLINETEXTMETRICW lpOTM)  /* [out] Address of metric data array */
{
    DC *dc = get_dc_ptr( hdc );
    OUTLINETEXTMETRICW *output = lpOTM;
    UINT ret;

    TRACE("(%p,%d,%p)\n", hdc, cbData, lpOTM);
    if(!dc) return 0;

    if(dc->gdiFont) {
        ret = WineEngGetOutlineTextMetrics(dc->gdiFont, cbData, output);
        if(lpOTM && ret) {
            if(ret > cbData) {
                output = HeapAlloc(GetProcessHeap(), 0, ret);
                WineEngGetOutlineTextMetrics(dc->gdiFont, ret, output);
            }

        output->otmTextMetrics.tmDigitizedAspectX = GetDeviceCaps(hdc, LOGPIXELSX);
        output->otmTextMetrics.tmDigitizedAspectY = GetDeviceCaps(hdc, LOGPIXELSY);

#define WDPTOLP(x) ((x<0)?					\
		(-abs(INTERNAL_XDSTOWS(dc, (x)))):		\
		(abs(INTERNAL_XDSTOWS(dc, (x)))))
#define HDPTOLP(y) ((y<0)?					\
		(-abs(INTERNAL_YDSTOWS(dc, (y)))):		\
		(abs(INTERNAL_YDSTOWS(dc, (y)))))

	    output->otmTextMetrics.tmHeight           = HDPTOLP(output->otmTextMetrics.tmHeight);
	    output->otmTextMetrics.tmAscent           = HDPTOLP(output->otmTextMetrics.tmAscent);
	    output->otmTextMetrics.tmDescent          = HDPTOLP(output->otmTextMetrics.tmDescent);
	    output->otmTextMetrics.tmInternalLeading  = HDPTOLP(output->otmTextMetrics.tmInternalLeading);
	    output->otmTextMetrics.tmExternalLeading  = HDPTOLP(output->otmTextMetrics.tmExternalLeading);
	    output->otmTextMetrics.tmAveCharWidth     = WDPTOLP(output->otmTextMetrics.tmAveCharWidth);
	    output->otmTextMetrics.tmMaxCharWidth     = WDPTOLP(output->otmTextMetrics.tmMaxCharWidth);
	    output->otmTextMetrics.tmOverhang         = WDPTOLP(output->otmTextMetrics.tmOverhang);
	    output->otmAscent = HDPTOLP(output->otmAscent);
	    output->otmDescent = HDPTOLP(output->otmDescent);
	    output->otmLineGap = abs(INTERNAL_YDSTOWS(dc,output->otmLineGap));
	    output->otmsCapEmHeight = abs(INTERNAL_YDSTOWS(dc,output->otmsCapEmHeight));
	    output->otmsXHeight = abs(INTERNAL_YDSTOWS(dc,output->otmsXHeight));
	    output->otmrcFontBox.top = HDPTOLP(output->otmrcFontBox.top);
	    output->otmrcFontBox.bottom = HDPTOLP(output->otmrcFontBox.bottom);
	    output->otmrcFontBox.left = WDPTOLP(output->otmrcFontBox.left);
	    output->otmrcFontBox.right = WDPTOLP(output->otmrcFontBox.right);
	    output->otmMacAscent = HDPTOLP(output->otmMacAscent);
	    output->otmMacDescent = HDPTOLP(output->otmMacDescent);
	    output->otmMacLineGap = abs(INTERNAL_YDSTOWS(dc,output->otmMacLineGap));
	    output->otmptSubscriptSize.x = WDPTOLP(output->otmptSubscriptSize.x);
	    output->otmptSubscriptSize.y = HDPTOLP(output->otmptSubscriptSize.y);
	    output->otmptSubscriptOffset.x = WDPTOLP(output->otmptSubscriptOffset.x);
	    output->otmptSubscriptOffset.y = HDPTOLP(output->otmptSubscriptOffset.y);
	    output->otmptSuperscriptSize.x = WDPTOLP(output->otmptSuperscriptSize.x);
	    output->otmptSuperscriptSize.y = HDPTOLP(output->otmptSuperscriptSize.y);
	    output->otmptSuperscriptOffset.x = WDPTOLP(output->otmptSuperscriptOffset.x);
	    output->otmptSuperscriptOffset.y = HDPTOLP(output->otmptSuperscriptOffset.y);
	    output->otmsStrikeoutSize = abs(INTERNAL_YDSTOWS(dc,output->otmsStrikeoutSize));
	    output->otmsStrikeoutPosition = HDPTOLP(output->otmsStrikeoutPosition);
	    output->otmsUnderscoreSize = HDPTOLP(output->otmsUnderscoreSize);
	    output->otmsUnderscorePosition = HDPTOLP(output->otmsUnderscorePosition);
#undef WDPTOLP
#undef HDPTOLP
            if(output != lpOTM) {
                memcpy(lpOTM, output, cbData);
                HeapFree(GetProcessHeap(), 0, output);
                ret = cbData;
            }
	}
    }

    else { /* This stuff was in GetOutlineTextMetricsA, I've moved it here
	      but really this should just be a return 0. */

        ret = sizeof(*lpOTM);
        if (lpOTM) {
	    if(cbData < ret)
	        ret = 0;
	    else {
	        memset(lpOTM, 0, ret);
		lpOTM->otmSize = sizeof(*lpOTM);
		GetTextMetricsW(hdc, &lpOTM->otmTextMetrics);
		/*
		  Further fill of the structure not implemented,
		  Needs real values for the structure members
		*/
	    }
	}
    }
    release_dc_ptr(dc);
    return ret;
}


/***********************************************************************
 *           GetCharWidthW      (GDI32.@)
 *           GetCharWidth32W    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32W( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    UINT i;
    BOOL ret = FALSE;
    DC * dc = get_dc_ptr( hdc );
    if (!dc) return FALSE;

    if (dc->gdiFont)
        ret = WineEngGetCharWidth( dc->gdiFont, firstChar, lastChar, buffer );
    else if (dc->funcs->pGetCharWidth)
        ret = dc->funcs->pGetCharWidth( dc->physDev, firstChar, lastChar, buffer);

    if (ret)
    {
        /* convert device units to logical */
        for( i = firstChar; i <= lastChar; i++, buffer++ )
            *buffer = INTERNAL_XDSTOWS(dc, *buffer);
        ret = TRUE;
    }
    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetCharWidthA      (GDI32.@)
 *           GetCharWidth32A    (GDI32.@)
 */
BOOL WINAPI GetCharWidth32A( HDC hdc, UINT firstChar, UINT lastChar,
                               LPINT buffer )
{
    INT i, wlen, count = (INT)(lastChar - firstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(firstChar + i);

    wstr = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    for(i = 0; i < wlen; i++)
    {
	if(!GetCharWidth32W(hdc, wstr[i], wstr[i], buffer))
	{
	    ret = FALSE;
	    break;
	}
	buffer++;
    }

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}


/***********************************************************************
 *           ExtTextOutA    (GDI32.@)
 *
 * See ExtTextOutW.
 */
BOOL WINAPI ExtTextOutA( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCSTR str, UINT count, const INT *lpDx )
{
    INT wlen;
    UINT codepage;
    LPWSTR p;
    BOOL ret;
    LPINT lpDxW = NULL;

    if (flags & ETO_GLYPH_INDEX)
        return ExtTextOutW( hdc, x, y, flags, lprect, (LPCWSTR)str, count, lpDx );

    p = FONT_mbtowc(hdc, str, count, &wlen, &codepage);

    if (lpDx) {
        unsigned int i = 0, j = 0;

        lpDxW = HeapAlloc( GetProcessHeap(), 0, wlen*sizeof(INT));
        while(i < count) {
            if(IsDBCSLeadByteEx(codepage, str[i])) {
                lpDxW[j++] = lpDx[i] + lpDx[i+1];
                i = i + 2;
            } else {
                lpDxW[j++] = lpDx[i];
                i = i + 1;
            }
        }
    }

    ret = ExtTextOutW( hdc, x, y, flags, lprect, p, wlen, lpDxW );

    HeapFree( GetProcessHeap(), 0, p );
    HeapFree( GetProcessHeap(), 0, lpDxW );
    return ret;
}


/***********************************************************************
 *           ExtTextOutW    (GDI32.@)
 *
 * Draws text using the currently selected font, background color, and text color.
 * 
 * 
 * PARAMS
 *    x,y    [I] coordinates of string
 *    flags  [I]
 *        ETO_GRAYED - undocumented on MSDN
 *        ETO_OPAQUE - use background color for fill the rectangle
 *        ETO_CLIPPED - clipping text to the rectangle
 *        ETO_GLYPH_INDEX - Buffer is of glyph locations in fonts rather
 *                          than encoded characters. Implies ETO_IGNORELANGUAGE
 *        ETO_RTLREADING - Paragraph is basically a right-to-left paragraph.
 *                         Affects BiDi ordering
 *        ETO_IGNORELANGUAGE - Undocumented in MSDN - instructs ExtTextOut not to do BiDi reordering
 *        ETO_PDY - unimplemented
 *        ETO_NUMERICSLATIN - unimplemented always assumed -
 *                            do not translate numbers into locale representations
 *        ETO_NUMERICSLOCAL - unimplemented - Numerals in Arabic/Farsi context should assume local form
 *    lprect [I] dimensions for clipping or/and opaquing
 *    str    [I] text string
 *    count  [I] number of symbols in string
 *    lpDx   [I] optional parameter with distance between drawing characters
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI ExtTextOutW( HDC hdc, INT x, INT y, UINT flags,
                         const RECT *lprect, LPCWSTR str, UINT count, const INT *lpDx )
{
    BOOL ret = FALSE;
    LPWSTR reordered_str = (LPWSTR)str;
    WORD *glyphs = NULL;
    UINT align = GetTextAlign( hdc );
    POINT pt;
    TEXTMETRICW tm;
    LOGFONTW lf;
    double cosEsc, sinEsc;
    INT *deltas = NULL, char_extra;
    SIZE sz;
    RECT rc;
    BOOL done_extents = FALSE;
    INT width = 0, xwidth = 0, ywidth = 0;
    DWORD type;
    DC * dc = get_dc_ptr( hdc );
    INT breakRem;
    static int quietfixme = 0;

    if (!dc) return FALSE;

    breakRem = dc->breakRem;

    if (quietfixme == 0 && flags & (ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN | ETO_PDY))
    {
        FIXME("flags ETO_NUMERICSLOCAL | ETO_NUMERICSLATIN | ETO_PDY unimplemented\n");
        quietfixme = 1;
    }
    if (!dc->funcs->pExtTextOut && !PATH_IsPathOpen(dc->path))
    {
        release_dc_ptr( dc );
        return ret;
    }

    update_dc( dc );
    type = GetObjectType(hdc);
    if(type == OBJ_METADC || type == OBJ_ENHMETADC)
    {
        ret = dc->funcs->pExtTextOut(dc->physDev, x, y, flags, lprect, str, count, lpDx);
        release_dc_ptr( dc );
        return ret;
    }

    if (!lprect)
        flags &= ~ETO_CLIPPED;
        
    if( !(flags & (ETO_GLYPH_INDEX | ETO_IGNORELANGUAGE)) && count > 0 )
    {
        reordered_str = HeapAlloc(GetProcessHeap(), 0, count*sizeof(WCHAR));

        BIDI_Reorder( str, count, GCP_REORDER,
                      ((flags&ETO_RTLREADING)!=0 || (GetTextAlign(hdc)&TA_RTLREADING)!=0)?
                      WINE_GCPW_FORCE_RTL:WINE_GCPW_FORCE_LTR,
                      reordered_str, count, NULL );
    
        flags |= ETO_IGNORELANGUAGE;
    }

    TRACE("%p, %d, %d, %08x, %p, %s, %d, %p)\n", hdc, x, y, flags,
          lprect, debugstr_wn(str, count), count, lpDx);

    if(flags & ETO_GLYPH_INDEX)
        glyphs = reordered_str;

    if(lprect)
        TRACE("rect: %d,%d - %d,%d\n", lprect->left, lprect->top, lprect->right,
              lprect->bottom);
    TRACE("align = %x bkmode = %x mapmode = %x\n", align, GetBkMode(hdc), GetMapMode(hdc));

    if(align & TA_UPDATECP)
    {
        GetCurrentPositionEx( hdc, &pt );
        x = pt.x;
        y = pt.y;
    }

    GetTextMetricsW(hdc, &tm);
    GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(lf), &lf);

    if(!(tm.tmPitchAndFamily & TMPF_VECTOR)) /* Non-scalable fonts shouldn't be rotated */
        lf.lfEscapement = 0;

    if(lf.lfEscapement != 0)
    {
        cosEsc = cos(lf.lfEscapement * M_PI / 1800);
        sinEsc = sin(lf.lfEscapement * M_PI / 1800);
    }
    else
    {
        cosEsc = 1;
        sinEsc = 0;
    }

    if(flags & (ETO_CLIPPED | ETO_OPAQUE))
    {
        if(!lprect)
        {
            if(flags & ETO_GLYPH_INDEX)
                GetTextExtentPointI(hdc, glyphs, count, &sz);
            else
                GetTextExtentPointW(hdc, reordered_str, count, &sz);

            done_extents = TRUE;
            rc.left = x;
            rc.top = y;
            rc.right = x + sz.cx;
            rc.bottom = y + sz.cy;
        }
        else
        {
            rc = *lprect;
        }

        LPtoDP(hdc, (POINT*)&rc, 2);

        if(rc.left > rc.right) {INT tmp = rc.left; rc.left = rc.right; rc.right = tmp;}
        if(rc.top > rc.bottom) {INT tmp = rc.top; rc.top = rc.bottom; rc.bottom = tmp;}
    }

    if ((flags & ETO_OPAQUE) && !PATH_IsPathOpen(dc->path))
        dc->funcs->pExtTextOut(dc->physDev, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    if(count == 0)
    {
        ret = TRUE;
        goto done;
    }

    pt.x = x;
    pt.y = y;
    LPtoDP(hdc, &pt, 1);
    x = pt.x;
    y = pt.y;

    char_extra = GetTextCharacterExtra(hdc);
    if(char_extra || dc->breakExtra || breakRem || lpDx || lf.lfEscapement != 0)
    {
        UINT i;
        SIZE tmpsz;
        deltas = HeapAlloc(GetProcessHeap(), 0, count * sizeof(INT));
        for(i = 0; i < count; i++)
        {
            if(lpDx && (flags & ETO_PDY))
                deltas[i] = lpDx[i*2] + char_extra;
            else if(lpDx)
                deltas[i] = lpDx[i] + char_extra;
            else
            {
                if(flags & ETO_GLYPH_INDEX)
                    GetTextExtentPointI(hdc, glyphs + i, 1, &tmpsz);
                else
                    GetTextExtentPointW(hdc, reordered_str + i, 1, &tmpsz);

                deltas[i] = tmpsz.cx;
            }
            
            if (!(flags & ETO_GLYPH_INDEX) && (dc->breakExtra || breakRem) && reordered_str[i] == tm.tmBreakChar)
            {
                deltas[i] = deltas[i] + dc->breakExtra;
                if (breakRem > 0)
                {
                    breakRem--;
                    deltas[i]++;
                }
            }
            deltas[i] = INTERNAL_XWSTODS(dc, deltas[i]);
            width += deltas[i];
        }
    }
    else
    {
        if(!done_extents)
        {
            if(flags & ETO_GLYPH_INDEX)
                GetTextExtentPointI(hdc, glyphs, count, &sz);
            else
                GetTextExtentPointW(hdc, reordered_str, count, &sz);
            done_extents = TRUE;
        }
        width = INTERNAL_XWSTODS(dc, sz.cx);
    }
    xwidth = width * cosEsc;
    ywidth = width * sinEsc;

    tm.tmAscent = abs(INTERNAL_YWSTODS(dc, tm.tmAscent));
    tm.tmDescent = abs(INTERNAL_YWSTODS(dc, tm.tmDescent));
    switch( align & (TA_LEFT | TA_RIGHT | TA_CENTER) )
    {
    case TA_LEFT:
        if (align & TA_UPDATECP)
        {
            pt.x = x + xwidth;
            pt.y = y - ywidth;
            DPtoLP(hdc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;

    case TA_CENTER:
        x -= xwidth / 2;
        y += ywidth / 2;
        break;

    case TA_RIGHT:
        x -= xwidth;
        y += ywidth;
        if (align & TA_UPDATECP)
        {
            pt.x = x;
            pt.y = y;
            DPtoLP(hdc, &pt, 1);
            MoveToEx(hdc, pt.x, pt.y, NULL);
        }
        break;
    }

    switch( align & (TA_TOP | TA_BOTTOM | TA_BASELINE) )
    {
    case TA_TOP:
        y += tm.tmAscent * cosEsc;
        x += tm.tmAscent * sinEsc;
        break;

    case TA_BOTTOM:
        y -= tm.tmDescent * cosEsc;
        x -= tm.tmDescent * sinEsc;
        break;

    case TA_BASELINE:
        break;
    }

    if (GetBkMode(hdc) != TRANSPARENT && !PATH_IsPathOpen(dc->path))
    {
        if(!((flags & ETO_CLIPPED) && (flags & ETO_OPAQUE)))
        {
            if(!(flags & ETO_OPAQUE) || x < rc.left || x + width >= rc.right ||
               y - tm.tmAscent < rc.top || y + tm.tmDescent >= rc.bottom)
            {
                RECT rc;
                rc.left = x;
                rc.right = x + width;
                rc.top = y - tm.tmAscent;
                rc.bottom = y + tm.tmDescent;
                dc->funcs->pExtTextOut(dc->physDev, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
            }
        }
    }

    if(FontIsLinked(hdc) && !(flags & ETO_GLYPH_INDEX))
    {
        HFONT orig_font = dc->hFont, cur_font;
        UINT glyph;
        INT span = 0, *offsets = NULL;
        unsigned int i;

        glyphs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WORD));
        for(i = 0; i < count; i++)
        {
            WineEngGetLinkedHFont(dc, reordered_str[i], &cur_font, &glyph);
            if(cur_font != dc->hFont)
            {
                if(!offsets)
                {
                    unsigned int j;
                    offsets = HeapAlloc(GetProcessHeap(), 0, count * sizeof(*deltas));
                    offsets[0] = 0;
                    if(!deltas)
                    {
                        SIZE tmpsz;
                        for(j = 1; j < count; j++)
                        {
                            GetTextExtentPointW(hdc, reordered_str + j - 1, 1, &tmpsz);
                            offsets[j] = offsets[j-1] + INTERNAL_XWSTODS(dc, tmpsz.cx);
                        }
                    }
                    else
                    {
                        for(j = 1; j < count; j++)
                            offsets[j] = offsets[j-1] + deltas[j];
                    }
                }
                if(span)
                {
                    if (PATH_IsPathOpen(dc->path))
                        ret = PATH_ExtTextOut(dc, x + offsets[i - span] * cosEsc, y - offsets[i - span] * sinEsc,
                                              (flags & ~ETO_OPAQUE) | ETO_GLYPH_INDEX, &rc,
                                              glyphs, span, deltas ? deltas + i - span : NULL);
                    else
                        dc->funcs->pExtTextOut(dc->physDev, x + offsets[i - span] * cosEsc, y - offsets[i - span] * sinEsc,
                                           (flags & ~ETO_OPAQUE) | ETO_GLYPH_INDEX, &rc,
                                           glyphs, span, deltas ? deltas + i - span : NULL);
                    span = 0;
                }
                SelectObject(hdc, cur_font);
            }
            glyphs[span++] = glyph;

            if(i == count - 1)
            {
                if (PATH_IsPathOpen(dc->path))
                    ret = PATH_ExtTextOut(dc, x + (offsets ? offsets[count - span] * cosEsc : 0),
                                          y - (offsets ? offsets[count - span] * sinEsc : 0),
                                          (flags & ~ETO_OPAQUE) | ETO_GLYPH_INDEX, &rc,
                                          glyphs, span, deltas ? deltas + count - span : NULL);
                else
                    ret = dc->funcs->pExtTextOut(dc->physDev, x + (offsets ? offsets[count - span] * cosEsc : 0),
                                             y - (offsets ? offsets[count - span] * sinEsc : 0),
                                             (flags & ~ETO_OPAQUE) | ETO_GLYPH_INDEX, &rc,
                                             glyphs, span, deltas ? deltas + count - span : NULL);
                SelectObject(hdc, orig_font);
                HeapFree(GetProcessHeap(), 0, offsets);
           }
        }
    }
    else
    {
        if(!(flags & ETO_GLYPH_INDEX) && dc->gdiFont)
        {
            glyphs = HeapAlloc(GetProcessHeap(), 0, count * sizeof(WORD));
            GetGlyphIndicesW(hdc, reordered_str, count, glyphs, 0);
            flags |= ETO_GLYPH_INDEX;
        }

        if (PATH_IsPathOpen(dc->path))
            ret = PATH_ExtTextOut(dc, x, y, (flags & ~ETO_OPAQUE), &rc,
                                  glyphs ? glyphs : reordered_str, count, deltas);
        else
            ret = dc->funcs->pExtTextOut(dc->physDev, x, y, (flags & ~ETO_OPAQUE), &rc,
                                     glyphs ? glyphs : reordered_str, count, deltas);
    }

done:
    HeapFree(GetProcessHeap(), 0, deltas);
    if(glyphs != reordered_str)
        HeapFree(GetProcessHeap(), 0, glyphs);
    if(reordered_str != str)
        HeapFree(GetProcessHeap(), 0, reordered_str);

    release_dc_ptr( dc );

    if (ret && (lf.lfUnderline || lf.lfStrikeOut))
    {
        int underlinePos, strikeoutPos;
        int underlineWidth, strikeoutWidth;
        UINT size = GetOutlineTextMetricsW(hdc, 0, NULL);
        OUTLINETEXTMETRICW* otm = NULL;

        if(!size)
        {
            underlinePos = 0;
            underlineWidth = tm.tmAscent / 20 + 1;
            strikeoutPos = tm.tmAscent / 2;
            strikeoutWidth = underlineWidth;
        }
        else
        {
            otm = HeapAlloc(GetProcessHeap(), 0, size);
            GetOutlineTextMetricsW(hdc, size, otm);
            underlinePos = otm->otmsUnderscorePosition;
            underlineWidth = otm->otmsUnderscoreSize;
            strikeoutPos = otm->otmsStrikeoutPosition;
            strikeoutWidth = otm->otmsStrikeoutSize;
            HeapFree(GetProcessHeap(), 0, otm);
        }

        if (PATH_IsPathOpen(dc->path))
        {
            POINT pts[5];
            HPEN hpen;
            HBRUSH hbrush = CreateSolidBrush(GetTextColor(hdc));

            hbrush = SelectObject(hdc, hbrush);
            hpen = SelectObject(hdc, GetStockObject(NULL_PEN));

            if (lf.lfUnderline)
            {
                pts[0].x = x - underlinePos * sinEsc;
                pts[0].y = y - underlinePos * cosEsc;
                pts[1].x = x + xwidth - underlinePos * sinEsc;
                pts[1].y = y - ywidth - underlinePos * cosEsc;
                pts[2].x = pts[1].x + underlineWidth * sinEsc;
                pts[2].y = pts[1].y + underlineWidth * cosEsc;
                pts[3].x = pts[0].x + underlineWidth * sinEsc;
                pts[3].y = pts[0].y + underlineWidth * cosEsc;
                pts[4].x = pts[0].x;
                pts[4].y = pts[0].y;
                DPtoLP(hdc, pts, 5);
                Polygon(hdc, pts, 5);
            }

            if (lf.lfStrikeOut)
            {
                pts[0].x = x - strikeoutPos * sinEsc;
                pts[0].y = y - strikeoutPos * cosEsc;
                pts[1].x = x + xwidth - strikeoutPos * sinEsc;
                pts[1].y = y - ywidth - strikeoutPos * cosEsc;
                pts[2].x = pts[1].x + strikeoutWidth * sinEsc;
                pts[2].y = pts[1].y + strikeoutWidth * cosEsc;
                pts[3].x = pts[0].x + strikeoutWidth * sinEsc;
                pts[3].y = pts[0].y + strikeoutWidth * cosEsc;
                pts[4].x = pts[0].x;
                pts[4].y = pts[0].y;
                DPtoLP(hdc, pts, 5);
                Polygon(hdc, pts, 5);
            }

            SelectObject(hdc, hpen);
            hbrush = SelectObject(hdc, hbrush);
            DeleteObject(hbrush);
        }
        else
        {
            POINT pts[2], oldpt;
            HPEN hpen;

            if (lf.lfUnderline)
            {
                hpen = CreatePen(PS_SOLID, underlineWidth, GetTextColor(hdc));
                hpen = SelectObject(hdc, hpen);
                pts[0].x = x;
                pts[0].y = y;
                pts[1].x = x + xwidth;
                pts[1].y = y - ywidth;
                DPtoLP(hdc, pts, 2);
                MoveToEx(hdc, pts[0].x - underlinePos * sinEsc, pts[0].y - underlinePos * cosEsc, &oldpt);
                LineTo(hdc, pts[1].x - underlinePos * sinEsc, pts[1].y - underlinePos * cosEsc);
                MoveToEx(hdc, oldpt.x, oldpt.y, NULL);
                DeleteObject(SelectObject(hdc, hpen));
            }

            if (lf.lfStrikeOut)
            {
                hpen = CreatePen(PS_SOLID, strikeoutWidth, GetTextColor(hdc));
                hpen = SelectObject(hdc, hpen);
                pts[0].x = x;
                pts[0].y = y;
                pts[1].x = x + xwidth;
                pts[1].y = y - ywidth;
                DPtoLP(hdc, pts, 2);
                MoveToEx(hdc, pts[0].x - strikeoutPos * sinEsc, pts[0].y - strikeoutPos * cosEsc, &oldpt);
                LineTo(hdc, pts[1].x - strikeoutPos * sinEsc, pts[1].y - strikeoutPos * cosEsc);
                MoveToEx(hdc, oldpt.x, oldpt.y, NULL);
                DeleteObject(SelectObject(hdc, hpen));
            }
        }
    }

    return ret;
}


/***********************************************************************
 *           TextOutA    (GDI32.@)
 */
BOOL WINAPI TextOutA( HDC hdc, INT x, INT y, LPCSTR str, INT count )
{
    return ExtTextOutA( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *           TextOutW    (GDI32.@)
 */
BOOL WINAPI TextOutW(HDC hdc, INT x, INT y, LPCWSTR str, INT count)
{
    return ExtTextOutW( hdc, x, y, 0, NULL, str, count, NULL );
}


/***********************************************************************
 *		PolyTextOutA (GDI32.@)
 *
 * See PolyTextOutW.
 */
BOOL WINAPI PolyTextOutA( HDC hdc, const POLYTEXTA *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutA( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}



/***********************************************************************
 *		PolyTextOutW (GDI32.@)
 *
 * Draw several Strings
 *
 * RETURNS
 *  TRUE:  Success.
 *  FALSE: Failure.
 */
BOOL WINAPI PolyTextOutW( HDC hdc, const POLYTEXTW *pptxt, INT cStrings )
{
    for (; cStrings>0; cStrings--, pptxt++)
        if (!ExtTextOutW( hdc, pptxt->x, pptxt->y, pptxt->uiFlags, &pptxt->rcl, pptxt->lpstr, pptxt->n, pptxt->pdx ))
            return FALSE;
    return TRUE;
}


/* FIXME: all following APIs ******************************************/


/***********************************************************************
 *           SetMapperFlags    (GDI32.@)
 */
DWORD WINAPI SetMapperFlags( HDC hDC, DWORD dwFlag )
{
    DC *dc = get_dc_ptr( hDC );
    DWORD ret = 0;
    if(!dc) return 0;
    if(dc->funcs->pSetMapperFlags)
    {
        ret = dc->funcs->pSetMapperFlags( dc->physDev, dwFlag );
        /* FIXME: ret is just a success flag, we should return a proper value */
    }
    else
        FIXME("(%p, 0x%08x): stub - harmless\n", hDC, dwFlag);
    release_dc_ptr( dc );
    return ret;
}

/***********************************************************************
 *          GetAspectRatioFilterEx  (GDI32.@)
 */
BOOL WINAPI GetAspectRatioFilterEx( HDC hdc, LPSIZE pAspectRatio )
{
  FIXME("(%p, %p): -- Empty Stub !\n", hdc, pAspectRatio);
  return FALSE;
}


/***********************************************************************
 *           GetCharABCWidthsA   (GDI32.@)
 *
 * See GetCharABCWidthsW.
 */
BOOL WINAPI GetCharABCWidthsA(HDC hdc, UINT firstChar, UINT lastChar,
                                  LPABC abc )
{
    INT i, wlen, count = (INT)(lastChar - firstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(firstChar + i);

    wstr = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    for(i = 0; i < wlen; i++)
    {
	if(!GetCharABCWidthsW(hdc, wstr[i], wstr[i], abc))
	{
	    ret = FALSE;
	    break;
	}
	abc++;
    }

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}


/******************************************************************************
 * GetCharABCWidthsW [GDI32.@]
 *
 * Retrieves widths of characters in range.
 *
 * PARAMS
 *    hdc       [I] Handle of device context
 *    firstChar [I] First character in range to query
 *    lastChar  [I] Last character in range to query
 *    abc       [O] Address of character-width structure
 *
 * NOTES
 *    Only works with TrueType fonts
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsW( HDC hdc, UINT firstChar, UINT lastChar,
                                   LPABC abc )
{
    DC *dc = get_dc_ptr(hdc);
    unsigned int i;
    BOOL ret = FALSE;

    if (!dc) return FALSE;

    if (!abc)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    if(dc->gdiFont)
        ret = WineEngGetCharABCWidths( dc->gdiFont, firstChar, lastChar, abc );
    else
        FIXME(": stub\n");

    if (ret)
    {
        /* convert device units to logical */
        for( i = firstChar; i <= lastChar; i++, abc++ ) {
            abc->abcA = INTERNAL_XDSTOWS(dc, abc->abcA);
            abc->abcB = INTERNAL_XDSTOWS(dc, abc->abcB);
            abc->abcC = INTERNAL_XDSTOWS(dc, abc->abcC);
	}
        ret = TRUE;
    }

    release_dc_ptr( dc );
    return ret;
}


/******************************************************************************
 * GetCharABCWidthsI [GDI32.@]
 *
 * Retrieves widths of characters in range.
 *
 * PARAMS
 *    hdc       [I] Handle of device context
 *    firstChar [I] First glyphs in range to query
 *    count     [I] Last glyphs in range to query
 *    pgi       [i] Array of glyphs to query
 *    abc       [O] Address of character-width structure
 *
 * NOTES
 *    Only works with TrueType fonts
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 */
BOOL WINAPI GetCharABCWidthsI( HDC hdc, UINT firstChar, UINT count,
                               LPWORD pgi, LPABC abc)
{
    DC *dc = get_dc_ptr(hdc);
    unsigned int i;
    BOOL ret = FALSE;

    if (!dc) return FALSE;

    if (!abc)
    {
        release_dc_ptr( dc );
        return FALSE;
    }

    if(dc->gdiFont)
        ret = WineEngGetCharABCWidthsI( dc->gdiFont, firstChar, count, pgi, abc );
    else
        FIXME(": stub\n");

    if (ret)
    {
        /* convert device units to logical */
        for( i = 0; i < count; i++, abc++ ) {
            abc->abcA = INTERNAL_XDSTOWS(dc, abc->abcA);
            abc->abcB = INTERNAL_XDSTOWS(dc, abc->abcB);
            abc->abcC = INTERNAL_XDSTOWS(dc, abc->abcC);
	}
        ret = TRUE;
    }

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           GetGlyphOutlineA    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineA( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    LPWSTR p = NULL;
    DWORD ret;
    UINT c;

    if (!lpmat2) return GDI_ERROR;

    if(!(fuFormat & GGO_GLYPH_INDEX)) {
        int len;
        char mbchs[2];
        if(uChar > 0xff) { /* but, 2 bytes character only */
            len = 2;
            mbchs[0] = (uChar & 0xff00) >> 8;
            mbchs[1] = (uChar & 0xff);
        } else {
            len = 1;
            mbchs[0] = (uChar & 0xff);
        }
        p = FONT_mbtowc(hdc, mbchs, len, NULL, NULL);
	c = p[0];
    } else
        c = uChar;
    ret = GetGlyphOutlineW(hdc, c, fuFormat, lpgm, cbBuffer, lpBuffer,
			   lpmat2);
    HeapFree(GetProcessHeap(), 0, p);
    return ret;
}

/***********************************************************************
 *           GetGlyphOutlineW    (GDI32.@)
 */
DWORD WINAPI GetGlyphOutlineW( HDC hdc, UINT uChar, UINT fuFormat,
                                 LPGLYPHMETRICS lpgm, DWORD cbBuffer,
                                 LPVOID lpBuffer, const MAT2 *lpmat2 )
{
    DC *dc;
    DWORD ret;

    TRACE("(%p, %04x, %04x, %p, %d, %p, %p)\n",
	  hdc, uChar, fuFormat, lpgm, cbBuffer, lpBuffer, lpmat2 );

    if (!lpmat2) return GDI_ERROR;

    dc = get_dc_ptr(hdc);
    if(!dc) return GDI_ERROR;

    if(dc->gdiFont)
      ret = WineEngGetGlyphOutline(dc->gdiFont, uChar, fuFormat, lpgm,
				   cbBuffer, lpBuffer, lpmat2);
    else
      ret = GDI_ERROR;

    release_dc_ptr( dc );
    return ret;
}


/***********************************************************************
 *           CreateScalableFontResourceA   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceA( DWORD fHidden,
                                             LPCSTR lpszResourceFile,
                                             LPCSTR lpszFontFile,
                                             LPCSTR lpszCurrentPath )
{
    LPWSTR lpszResourceFileW = NULL;
    LPWSTR lpszFontFileW = NULL;
    LPWSTR lpszCurrentPathW = NULL;
    int len;
    BOOL ret;

    if (lpszResourceFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszResourceFile, -1, NULL, 0);
        lpszResourceFileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszResourceFile, -1, lpszResourceFileW, len);
    }

    if (lpszFontFile)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszFontFile, -1, NULL, 0);
        lpszFontFileW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszFontFile, -1, lpszFontFileW, len);
    }

    if (lpszCurrentPath)
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszCurrentPath, -1, NULL, 0);
        lpszCurrentPathW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszCurrentPath, -1, lpszCurrentPathW, len);
    }

    ret = CreateScalableFontResourceW(fHidden, lpszResourceFileW,
            lpszFontFileW, lpszCurrentPathW);

    HeapFree(GetProcessHeap(), 0, lpszResourceFileW);
    HeapFree(GetProcessHeap(), 0, lpszFontFileW);
    HeapFree(GetProcessHeap(), 0, lpszCurrentPathW);

    return ret;
}

/***********************************************************************
 *           CreateScalableFontResourceW   (GDI32.@)
 */
BOOL WINAPI CreateScalableFontResourceW( DWORD fHidden,
                                             LPCWSTR lpszResourceFile,
                                             LPCWSTR lpszFontFile,
                                             LPCWSTR lpszCurrentPath )
{
    HANDLE f;
    FIXME("(%d,%s,%s,%s): stub\n",
          fHidden, debugstr_w(lpszResourceFile), debugstr_w(lpszFontFile),
          debugstr_w(lpszCurrentPath) );

    /* fHidden=1 - only visible for the calling app, read-only, not
     * enumerated with EnumFonts/EnumFontFamilies
     * lpszCurrentPath can be NULL
     */

    /* If the output file already exists, return the ERROR_FILE_EXISTS error as specified in MSDN */
    if ((f = CreateFileW(lpszResourceFile, 0, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0)) != INVALID_HANDLE_VALUE) {
        CloseHandle(f);
        SetLastError(ERROR_FILE_EXISTS);
        return FALSE;
    }
    return FALSE; /* create failed */
}

/*************************************************************************
 *             GetKerningPairsA   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsA( HDC hDC, DWORD cPairs,
                               LPKERNINGPAIR kern_pairA )
{
    UINT cp;
    CPINFO cpi;
    DWORD i, total_kern_pairs, kern_pairs_copied = 0;
    KERNINGPAIR *kern_pairW;

    if (!cPairs && kern_pairA)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    cp = GdiGetCodePage(hDC);

    /* GetCPInfo() will fail on CP_SYMBOL, and WideCharToMultiByte is supposed
     * to fail on an invalid character for CP_SYMBOL.
     */
    cpi.DefaultChar[0] = 0;
    if (cp != CP_SYMBOL && !GetCPInfo(cp, &cpi))
    {
        FIXME("Can't find codepage %u info\n", cp);
        return 0;
    }

    total_kern_pairs = GetKerningPairsW(hDC, 0, NULL);
    if (!total_kern_pairs) return 0;

    kern_pairW = HeapAlloc(GetProcessHeap(), 0, total_kern_pairs * sizeof(*kern_pairW));
    GetKerningPairsW(hDC, total_kern_pairs, kern_pairW);

    for (i = 0; i < total_kern_pairs; i++)
    {
        char first, second;

        if (!WideCharToMultiByte(cp, 0, &kern_pairW[i].wFirst, 1, &first, 1, NULL, NULL))
            continue;

        if (!WideCharToMultiByte(cp, 0, &kern_pairW[i].wSecond, 1, &second, 1, NULL, NULL))
            continue;

        if (first == cpi.DefaultChar[0] || second == cpi.DefaultChar[0])
            continue;

        if (kern_pairA)
        {
            if (kern_pairs_copied >= cPairs) break;

            kern_pairA->wFirst = (BYTE)first;
            kern_pairA->wSecond = (BYTE)second;
            kern_pairA->iKernAmount = kern_pairW[i].iKernAmount;
            kern_pairA++;
        }
        kern_pairs_copied++;
    }

    HeapFree(GetProcessHeap(), 0, kern_pairW);

    return kern_pairs_copied;
}

/*************************************************************************
 *             GetKerningPairsW   (GDI32.@)
 */
DWORD WINAPI GetKerningPairsW( HDC hDC, DWORD cPairs,
                                 LPKERNINGPAIR lpKerningPairs )
{
    DC *dc;
    DWORD ret = 0;

    TRACE("(%p,%d,%p)\n", hDC, cPairs, lpKerningPairs);

    if (!cPairs && lpKerningPairs)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    dc = get_dc_ptr(hDC);
    if (!dc) return 0;

    if (dc->gdiFont)
        ret = WineEngGetKerningPairs(dc->gdiFont, cPairs, lpKerningPairs);

    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.@]
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondence between different labels
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges)
 * of the same encoding.
 *
 * Only one codepage will be set in lpCs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *lpSrc.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
BOOL WINAPI TranslateCharsetInfo(
  LPDWORD lpSrc, /* [in]
       if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
       if flags == TCI_SRCCHARSET: a character set value
       if flags == TCI_SRCCODEPAGE: a code page value
		 */
  LPCHARSETINFO lpCs, /* [out] structure to receive charset information */
  DWORD flags /* [in] determines interpretation of lpSrc */)
{
    int index = 0;
    switch (flags) {
    case TCI_SRCFONTSIG:
	while (!(*lpSrc>>index & 0x0001) && index<MAXTCIINDEX) index++;
      break;
    case TCI_SRCCODEPAGE:
      while (PtrToUlong(lpSrc) != FONT_tci[index].ciACP && index < MAXTCIINDEX) index++;
      break;
    case TCI_SRCCHARSET:
      while (PtrToUlong(lpSrc) != FONT_tci[index].ciCharset && index < MAXTCIINDEX) index++;
      break;
    default:
      return FALSE;
    }
    if (index >= MAXTCIINDEX || FONT_tci[index].ciCharset == DEFAULT_CHARSET) return FALSE;
    *lpCs = FONT_tci[index];
    return TRUE;
}

/*************************************************************************
 *             GetFontLanguageInfo   (GDI32.@)
 */
DWORD WINAPI GetFontLanguageInfo(HDC hdc)
{
	FONTSIGNATURE fontsig;
	static const DWORD GCP_DBCS_MASK=0x003F0000,
		GCP_DIACRITIC_MASK=0x00000000,
		FLI_GLYPHS_MASK=0x00000000,
		GCP_GLYPHSHAPE_MASK=0x00000040,
		GCP_KASHIDA_MASK=0x00000000,
		GCP_LIGATE_MASK=0x00000000,
		GCP_USEKERNING_MASK=0x00000000,
		GCP_REORDER_MASK=0x00000060;

	DWORD result=0;

	GetTextCharsetInfo( hdc, &fontsig, 0 );
	/* We detect each flag we return using a bitmask on the Codepage Bitfields */

	if( (fontsig.fsCsb[0]&GCP_DBCS_MASK)!=0 )
		result|=GCP_DBCS;

	if( (fontsig.fsCsb[0]&GCP_DIACRITIC_MASK)!=0 )
		result|=GCP_DIACRITIC;

	if( (fontsig.fsCsb[0]&FLI_GLYPHS_MASK)!=0 )
		result|=FLI_GLYPHS;

	if( (fontsig.fsCsb[0]&GCP_GLYPHSHAPE_MASK)!=0 )
		result|=GCP_GLYPHSHAPE;

	if( (fontsig.fsCsb[0]&GCP_KASHIDA_MASK)!=0 )
		result|=GCP_KASHIDA;

	if( (fontsig.fsCsb[0]&GCP_LIGATE_MASK)!=0 )
		result|=GCP_LIGATE;

	if( (fontsig.fsCsb[0]&GCP_USEKERNING_MASK)!=0 )
		result|=GCP_USEKERNING;

        /* this might need a test for a HEBREW- or ARABIC_CHARSET as well */
        if( GetTextAlign( hdc) & TA_RTLREADING )
            if( (fontsig.fsCsb[0]&GCP_REORDER_MASK)!=0 )
                    result|=GCP_REORDER;

	return result;
}


/*************************************************************************
 * GetFontData [GDI32.@]
 *
 * Retrieve data for TrueType font.
 *
 * RETURNS
 *
 * success: Number of bytes returned
 * failure: GDI_ERROR
 *
 * NOTES
 *
 * Calls SetLastError()
 *
 */
DWORD WINAPI GetFontData(HDC hdc, DWORD table, DWORD offset,
    LPVOID buffer, DWORD length)
{
    DC *dc = get_dc_ptr(hdc);
    DWORD ret = GDI_ERROR;

    if(!dc) return GDI_ERROR;

    if(dc->gdiFont)
      ret = WineEngGetFontData(dc->gdiFont, table, offset, buffer, length);

    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * GetGlyphIndicesA [GDI32.@]
 */
DWORD WINAPI GetGlyphIndicesA(HDC hdc, LPCSTR lpstr, INT count,
			      LPWORD pgi, DWORD flags)
{
    DWORD ret;
    WCHAR *lpstrW;
    INT countW;

    TRACE("(%p, %s, %d, %p, 0x%x)\n",
          hdc, debugstr_an(lpstr, count), count, pgi, flags);

    lpstrW = FONT_mbtowc(hdc, lpstr, count, &countW, NULL);
    ret = GetGlyphIndicesW(hdc, lpstrW, countW, pgi, flags);
    HeapFree(GetProcessHeap(), 0, lpstrW);

    return ret;
}

/*************************************************************************
 * GetGlyphIndicesW [GDI32.@]
 */
DWORD WINAPI GetGlyphIndicesW(HDC hdc, LPCWSTR lpstr, INT count,
			      LPWORD pgi, DWORD flags)
{
    DC *dc = get_dc_ptr(hdc);
    DWORD ret = GDI_ERROR;

    TRACE("(%p, %s, %d, %p, 0x%x)\n",
          hdc, debugstr_wn(lpstr, count), count, pgi, flags);

    if(!dc) return GDI_ERROR;

    if(dc->gdiFont)
	ret = WineEngGetGlyphIndices(dc->gdiFont, lpstr, count, pgi, flags);

    release_dc_ptr( dc );
    return ret;
}

/*************************************************************************
 * GetCharacterPlacementA [GDI32.@]
 *
 * See GetCharacterPlacementW.
 *
 * NOTES:
 *  the web browser control of ie4 calls this with dwFlags=0
 */
DWORD WINAPI
GetCharacterPlacementA(HDC hdc, LPCSTR lpString, INT uCount,
			 INT nMaxExtent, GCP_RESULTSA *lpResults,
			 DWORD dwFlags)
{
    WCHAR *lpStringW;
    INT uCountW;
    GCP_RESULTSW resultsW;
    DWORD ret;
    UINT font_cp;

    TRACE("%s, %d, %d, 0x%08x\n",
          debugstr_an(lpString, uCount), uCount, nMaxExtent, dwFlags);

    /* both structs are equal in size */
    memcpy(&resultsW, lpResults, sizeof(resultsW));

    lpStringW = FONT_mbtowc(hdc, lpString, uCount, &uCountW, &font_cp);
    if(lpResults->lpOutString)
        resultsW.lpOutString = HeapAlloc(GetProcessHeap(), 0, sizeof(WCHAR)*uCountW);

    ret = GetCharacterPlacementW(hdc, lpStringW, uCountW, nMaxExtent, &resultsW, dwFlags);

    lpResults->nGlyphs = resultsW.nGlyphs;
    lpResults->nMaxFit = resultsW.nMaxFit;

    if(lpResults->lpOutString) {
        WideCharToMultiByte(font_cp, 0, resultsW.lpOutString, uCountW,
                            lpResults->lpOutString, uCount, NULL, NULL );
    }

    HeapFree(GetProcessHeap(), 0, lpStringW);
    HeapFree(GetProcessHeap(), 0, resultsW.lpOutString);

    return ret;
}

/*************************************************************************
 * GetCharacterPlacementW [GDI32.@]
 *
 *   Retrieve information about a string. This includes the width, reordering,
 *   Glyphing and so on.
 *
 * RETURNS
 *
 *   The width and height of the string if successful, 0 if failed.
 *
 * BUGS
 *
 *   All flags except GCP_REORDER are not yet implemented.
 *   Reordering is not 100% compliant to the Windows BiDi method.
 *   Caret positioning is not yet implemented for BiDi.
 *   Classes are not yet implemented.
 *
 */
DWORD WINAPI
GetCharacterPlacementW(
		HDC hdc,		/* [in] Device context for which the rendering is to be done */
		LPCWSTR lpString,	/* [in] The string for which information is to be returned */
		INT uCount,		/* [in] Number of WORDS in string. */
		INT nMaxExtent,		/* [in] Maximum extent the string is to take (in HDC logical units) */
		GCP_RESULTSW *lpResults,/* [in/out] A pointer to a GCP_RESULTSW struct */
		DWORD dwFlags 		/* [in] Flags specifying how to process the string */
		)
{
    DWORD ret=0;
    SIZE size;
    UINT i, nSet;

    TRACE("%s, %d, %d, 0x%08x\n",
          debugstr_wn(lpString, uCount), uCount, nMaxExtent, dwFlags);

    TRACE("lStructSize=%d, lpOutString=%p, lpOrder=%p, lpDx=%p, lpCaretPos=%p\n"
          "lpClass=%p, lpGlyphs=%p, nGlyphs=%u, nMaxFit=%d\n",
	    lpResults->lStructSize, lpResults->lpOutString, lpResults->lpOrder,
	    lpResults->lpDx, lpResults->lpCaretPos, lpResults->lpClass,
	    lpResults->lpGlyphs, lpResults->nGlyphs, lpResults->nMaxFit);

    if(dwFlags&(~GCP_REORDER))			FIXME("flags 0x%08x ignored\n", dwFlags);
    if(lpResults->lpClass)	FIXME("classes not implemented\n");
    if (lpResults->lpCaretPos && (dwFlags & GCP_REORDER))
        FIXME("Caret positions for complex scripts not implemented\n");

	nSet = (UINT)uCount;
	if(nSet > lpResults->nGlyphs)
		nSet = lpResults->nGlyphs;

	/* return number of initialized fields */
	lpResults->nGlyphs = nSet;

	if((dwFlags&GCP_REORDER)==0 )
	{
		/* Treat the case where no special handling was requested in a fastpath way */
		/* copy will do if the GCP_REORDER flag is not set */
		if(lpResults->lpOutString)
                    memcpy( lpResults->lpOutString, lpString, nSet * sizeof(WCHAR));

		if(lpResults->lpOrder)
		{
			for(i = 0; i < nSet; i++)
				lpResults->lpOrder[i] = i;
		}
	} else
	{
            BIDI_Reorder( lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpResults->lpOutString,
                          nSet, lpResults->lpOrder );
	}

	/* FIXME: Will use the placement chars */
	if (lpResults->lpDx)
	{
		int c;
		for (i = 0; i < nSet; i++)
		{
			if (GetCharWidth32W(hdc, lpString[i], lpString[i], &c))
				lpResults->lpDx[i]= c;
		}
	}

    if (lpResults->lpCaretPos && !(dwFlags & GCP_REORDER))
    {
        int pos = 0;
       
        lpResults->lpCaretPos[0] = 0;
        for (i = 1; i < nSet; i++)
            if (GetTextExtentPoint32W(hdc, &(lpString[i - 1]), 1, &size))
                lpResults->lpCaretPos[i] = (pos += size.cx);
    }
   
    if(lpResults->lpGlyphs)
	GetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);

    if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
      ret = MAKELONG(size.cx, size.cy);

    return ret;
}

/*************************************************************************
 *      GetCharABCWidthsFloatA [GDI32.@]
 *
 * See GetCharABCWidthsFloatW.
 */
BOOL WINAPI GetCharABCWidthsFloatA( HDC hdc, UINT first, UINT last, LPABCFLOAT abcf )
{
    INT i, wlen, count = (INT)(last - first + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if (count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);

    for(i = 0; i < count; i++)
        str[i] = (BYTE)(first + i);

    wstr = FONT_mbtowc( hdc, str, count, &wlen, NULL );

    for (i = 0; i < wlen; i++)
    {
        if (!GetCharABCWidthsFloatW( hdc, wstr[i], wstr[i], abcf ))
        {
            ret = FALSE;
            break;
        }
        abcf++;
    }

    HeapFree( GetProcessHeap(), 0, str );
    HeapFree( GetProcessHeap(), 0, wstr );

    return ret;
}

/*************************************************************************
 *      GetCharABCWidthsFloatW [GDI32.@]
 *
 * Retrieves widths of a range of characters.
 *
 * PARAMS
 *    hdc   [I] Handle to device context.
 *    first [I] First character in range to query.
 *    last  [I] Last character in range to query.
 *    abcf  [O] Array of LPABCFLOAT structures.
 *
 * RETURNS
 *    Success: TRUE
 *    Failure: FALSE
 *
 * BUGS
 *    Only works with TrueType fonts. It also doesn't return real
 *    floats but converted integers because it's implemented on
 *    top of GetCharABCWidthsW.
 */
BOOL WINAPI GetCharABCWidthsFloatW( HDC hdc, UINT first, UINT last, LPABCFLOAT abcf )
{
    ABC *abc, *abc_base;
    unsigned int i, size = sizeof(ABC) * (last - first + 1);
    BOOL ret;

    TRACE("%p, %d, %d, %p - partial stub\n", hdc, first, last, abcf);

    abc = abc_base = HeapAlloc( GetProcessHeap(), 0, size );
    if (!abc) return FALSE;

    ret = GetCharABCWidthsW( hdc, first, last, abc );
    if (ret)
    {
        for (i = first; i <= last; i++, abc++, abcf++)
        {
            abcf->abcfA = abc->abcA;
            abcf->abcfB = abc->abcB;
            abcf->abcfC = abc->abcC;
        }
    }
    HeapFree( GetProcessHeap(), 0, abc_base );
    return ret;
}

/*************************************************************************
 *      GetCharWidthFloatA [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatA(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
    FIXME("%p, %u, %u, %p: stub!\n", hdc, iFirstChar, iLastChar, pxBuffer);
    return 0;
}

/*************************************************************************
 *      GetCharWidthFloatW [GDI32.@]
 */
BOOL WINAPI GetCharWidthFloatW(HDC hdc, UINT iFirstChar,
		                    UINT iLastChar, PFLOAT pxBuffer)
{
    FIXME("%p, %u, %u, %p: stub!\n", hdc, iFirstChar, iLastChar, pxBuffer);
    return 0;
}


/***********************************************************************
 *								       *
 *           Font Resource API					       *
 *								       *
 ***********************************************************************/

/***********************************************************************
 *           AddFontResourceA    (GDI32.@)
 */
INT WINAPI AddFontResourceA( LPCSTR str )
{
    return AddFontResourceExA( str, 0, NULL);
}

/***********************************************************************
 *           AddFontResourceW    (GDI32.@)
 */
INT WINAPI AddFontResourceW( LPCWSTR str )
{
    return AddFontResourceExW(str, 0, NULL);
}


/***********************************************************************
 *           AddFontResourceExA    (GDI32.@)
 */
INT WINAPI AddFontResourceExA( LPCSTR str, DWORD fl, PVOID pdv )
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len);
    ret = AddFontResourceExW(strW, fl, pdv);
    HeapFree(GetProcessHeap(), 0, strW);
    return ret;
}

static BOOL CALLBACK load_enumed_resource(HMODULE hModule, LPCWSTR type, LPWSTR name, LONG_PTR lParam)
{
    HRSRC rsrc = FindResourceW(hModule, name, type);
    HGLOBAL hMem = LoadResource(hModule, rsrc);
    LPVOID *pMem = LockResource(hMem);
    int *num_total = (int *)lParam;
    DWORD num_in_res;

    TRACE("Found resource %s - trying to load\n", wine_dbgstr_w(type));
    if (!AddFontMemResourceEx(pMem, SizeofResource(hModule, rsrc), NULL, &num_in_res))
    {
        ERR("Failed to load PE font resource mod=%p ptr=%p\n", hModule, hMem);
        return FALSE;
    }

    *num_total += num_in_res;
    return TRUE;
}

/***********************************************************************
 *           AddFontResourceExW    (GDI32.@)
 */
INT WINAPI AddFontResourceExW( LPCWSTR str, DWORD fl, PVOID pdv )
{
    int ret = WineEngAddFontResourceEx(str, fl, pdv);
    if (ret == 0)
    {
        /* FreeType <2.3.5 has problems reading resources wrapped in PE files. */
        HMODULE hModule = LoadLibraryExW(str, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (hModule != NULL)
        {
            int num_resources = 0;
            LPWSTR rt_font = (LPWSTR)((ULONG_PTR)8);  /* we don't want to include winuser.h */

            TRACE("WineEndAddFontResourceEx failed on PE file %s - trying to load resources manually\n",
                wine_dbgstr_w(str));
            if (EnumResourceNamesW(hModule, rt_font, load_enumed_resource, (LONG_PTR)&num_resources))
                ret = num_resources;
            FreeLibrary(hModule);
        }
    }
    return ret;
}

/***********************************************************************
 *           RemoveFontResourceA    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceA( LPCSTR str )
{
    return RemoveFontResourceExA(str, 0, 0);
}

/***********************************************************************
 *           RemoveFontResourceW    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceW( LPCWSTR str )
{
    return RemoveFontResourceExW(str, 0, 0);
}

/***********************************************************************
 *           AddFontMemResourceEx    (GDI32.@)
 */
HANDLE WINAPI AddFontMemResourceEx( PVOID pbFont, DWORD cbFont, PVOID pdv, DWORD *pcFonts)
{
    return WineEngAddFontMemResourceEx(pbFont, cbFont, pdv, pcFonts);
}

/***********************************************************************
 *           RemoveFontMemResourceEx    (GDI32.@)
 */
BOOL WINAPI RemoveFontMemResourceEx( HANDLE fh )
{
    FIXME("(%p) stub\n", fh);
    return TRUE;
}

/***********************************************************************
 *           RemoveFontResourceExA    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceExA( LPCSTR str, DWORD fl, PVOID pdv )
{
    DWORD len = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR strW = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    INT ret;

    MultiByteToWideChar(CP_ACP, 0, str, -1, strW, len);
    ret = RemoveFontResourceExW(strW, fl, pdv);
    HeapFree(GetProcessHeap(), 0, strW);
    return ret;
}

/***********************************************************************
 *           RemoveFontResourceExW    (GDI32.@)
 */
BOOL WINAPI RemoveFontResourceExW( LPCWSTR str, DWORD fl, PVOID pdv )
{
    return WineEngRemoveFontResourceEx(str, fl, pdv);
}

/***********************************************************************
 *           GetTextCharset    (GDI32.@)
 */
UINT WINAPI GetTextCharset(HDC hdc)
{
    /* MSDN docs say this is equivalent */
    return GetTextCharsetInfo(hdc, NULL, 0);
}

/***********************************************************************
 *           GetTextCharsetInfo    (GDI32.@)
 */
UINT WINAPI GetTextCharsetInfo(HDC hdc, LPFONTSIGNATURE fs, DWORD flags)
{
    UINT ret = DEFAULT_CHARSET;
    DC *dc = get_dc_ptr(hdc);

    if (dc)
    {
        if (dc->gdiFont)
            ret = WineEngGetTextCharsetInfo(dc->gdiFont, fs, flags);

        release_dc_ptr( dc );
    }

    if (ret == DEFAULT_CHARSET && fs)
        memset(fs, 0, sizeof(FONTSIGNATURE));
    return ret;
}

/***********************************************************************
 *           GdiGetCharDimensions    (GDI32.@)
 *
 * Gets the average width of the characters in the English alphabet.
 *
 * PARAMS
 *  hdc    [I] Handle to the device context to measure on.
 *  lptm   [O] Pointer to memory to store the text metrics into.
 *  height [O] On exit, the maximum height of characters in the English alphabet.
 *
 * RETURNS
 *  The average width of characters in the English alphabet.
 *
 * NOTES
 *  This function is used by the dialog manager to get the size of a dialog
 *  unit. It should also be used by other pieces of code that need to know
 *  the size of a dialog unit in logical units without having access to the
 *  window handle of the dialog.
 *  Windows caches the font metrics from this function, but we don't and
 *  there doesn't appear to be an immediate advantage to do so.
 *
 * SEE ALSO
 *  GetTextExtentPointW, GetTextMetricsW, MapDialogRect.
 */
LONG WINAPI GdiGetCharDimensions(HDC hdc, LPTEXTMETRICW lptm, LONG *height)
{
    SIZE sz;
    static const WCHAR alphabet[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0};

    if(lptm && !GetTextMetricsW(hdc, lptm)) return 0;

    if(!GetTextExtentPointW(hdc, alphabet, 52, &sz)) return 0;

    if (height) *height = sz.cy;
    return (sz.cx / 26 + 1) / 2;
}

BOOL WINAPI EnableEUDC(BOOL fEnableEUDC)
{
    FIXME("(%d): stub\n", fEnableEUDC);
    return FALSE;
}

/***********************************************************************
 *           GetCharWidthI    (GDI32.@)
 *
 * Retrieve widths of characters.
 *
 * PARAMS
 *  hdc    [I] Handle to a device context.
 *  first  [I] First glyph in range to query.
 *  count  [I] Number of glyph indices to query.
 *  glyphs [I] Array of glyphs to query.
 *  buffer [O] Buffer to receive character widths.
 *
 * NOTES
 *  Only works with TrueType fonts.
 *
 * RETURNS
 *  Success: TRUE
 *  Failure: FALSE
 */
BOOL WINAPI GetCharWidthI(HDC hdc, UINT first, UINT count, LPWORD glyphs, LPINT buffer)
{
    ABC *abc;
    unsigned int i;

    TRACE("(%p, %d, %d, %p, %p)\n", hdc, first, count, glyphs, buffer);

    if (!(abc = HeapAlloc(GetProcessHeap(), 0, count * sizeof(ABC))))
        return FALSE;

    if (!GetCharABCWidthsI(hdc, first, count, glyphs, abc))
    {
        HeapFree(GetProcessHeap(), 0, abc);
        return FALSE;
    }

    for (i = 0; i < count; i++)
        buffer[i] = abc->abcA + abc->abcB + abc->abcC;

    HeapFree(GetProcessHeap(), 0, abc);
    return TRUE;
}

/***********************************************************************
 *           GetFontUnicodeRanges    (GDI32.@)
 *
 *  Retrieve a list of supported Unicode characters in a font.
 *
 *  PARAMS
 *   hdc  [I] Handle to a device context.
 *   lpgs [O] GLYPHSET structure specifying supported character ranges.
 *
 *  RETURNS
 *   Success: Number of bytes written to the buffer pointed to by lpgs.
 *   Failure: 0
 *
 */
DWORD WINAPI GetFontUnicodeRanges(HDC hdc, LPGLYPHSET lpgs)
{
    DWORD ret = 0;
    DC *dc = get_dc_ptr(hdc);

    TRACE("(%p, %p)\n", hdc, lpgs);

    if (!dc) return 0;

    if (dc->gdiFont) ret = WineEngGetFontUnicodeRanges(dc->gdiFont, lpgs);
    release_dc_ptr(dc);
    return ret;
}


/*************************************************************
 *           FontIsLinked    (GDI32.@)
 */
BOOL WINAPI FontIsLinked(HDC hdc)
{
    DC *dc = get_dc_ptr(hdc);
    BOOL ret = FALSE;

    if (!dc) return FALSE;
    if (dc->gdiFont) ret = WineEngFontIsLinked(dc->gdiFont);
    release_dc_ptr(dc);
    TRACE("returning %d\n", ret);
    return ret;
}

/*************************************************************
 *           GdiRealizationInfo    (GDI32.@)
 *
 * Returns a structure that contains some font information.
 */
BOOL WINAPI GdiRealizationInfo(HDC hdc, realization_info_t *info)
{
    DC *dc = get_dc_ptr(hdc);
    BOOL ret = FALSE;

    if (!dc) return FALSE;
    if (dc->gdiFont) ret = WineEngRealizationInfo(dc->gdiFont, info);
    release_dc_ptr(dc);

    return ret;
}
