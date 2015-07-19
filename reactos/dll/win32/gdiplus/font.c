/*
 * Copyright (C) 2007 Google (Evan Stade)
 * Copyright (C) 2012 Dmitry Timoshkov
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

#include "gdiplus_private.h"

/* PANOSE is 10 bytes in size, need to pack the structure properly */
#include <pshpack2.h>
typedef struct
{
    USHORT version;
    SHORT xAvgCharWidth;
    USHORT usWeightClass;
    USHORT usWidthClass;
    SHORT fsType;
    SHORT ySubscriptXSize;
    SHORT ySubscriptYSize;
    SHORT ySubscriptXOffset;
    SHORT ySubscriptYOffset;
    SHORT ySuperscriptXSize;
    SHORT ySuperscriptYSize;
    SHORT ySuperscriptXOffset;
    SHORT ySuperscriptYOffset;
    SHORT yStrikeoutSize;
    SHORT yStrikeoutPosition;
    SHORT sFamilyClass;
    PANOSE panose;
    ULONG ulUnicodeRange1;
    ULONG ulUnicodeRange2;
    ULONG ulUnicodeRange3;
    ULONG ulUnicodeRange4;
    CHAR achVendID[4];
    USHORT fsSelection;
    USHORT usFirstCharIndex;
    USHORT usLastCharIndex;
    /* According to the Apple spec, original version didn't have the below fields,
     * version numbers were taken from the OpenType spec.
     */
    /* version 0 (TrueType 1.5) */
    USHORT sTypoAscender;
    USHORT sTypoDescender;
    USHORT sTypoLineGap;
    USHORT usWinAscent;
    USHORT usWinDescent;
    /* version 1 (TrueType 1.66) */
    ULONG ulCodePageRange1;
    ULONG ulCodePageRange2;
    /* version 2 (OpenType 1.2) */
    SHORT sxHeight;
    SHORT sCapHeight;
    USHORT usDefaultChar;
    USHORT usBreakChar;
    USHORT usMaxContext;
} TT_OS2_V2;

typedef struct
{
    ULONG Version;
    SHORT Ascender;
    SHORT Descender;
    SHORT LineGap;
    USHORT advanceWidthMax;
    SHORT minLeftSideBearing;
    SHORT minRightSideBearing;
    SHORT xMaxExtent;
    SHORT caretSlopeRise;
    SHORT caretSlopeRun;
    SHORT caretOffset;
    SHORT reserved[4];
    SHORT metricDataFormat;
    USHORT numberOfHMetrics;
} TT_HHEA;
#include <poppack.h>

#ifdef WORDS_BIGENDIAN
#define GET_BE_WORD(x) (x)
#define GET_BE_DWORD(x) (x)
#else
#define GET_BE_WORD(x) MAKEWORD(HIBYTE(x), LOBYTE(x))
#define GET_BE_DWORD(x) MAKELONG(GET_BE_WORD(HIWORD(x)), GET_BE_WORD(LOWORD(x)));
#endif

#define MS_MAKE_TAG(ch0, ch1, ch2, ch3) \
                    ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) | \
                    ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24))
#define MS_OS2_TAG MS_MAKE_TAG('O','S','/','2')
#define MS_HHEA_TAG MS_MAKE_TAG('h','h','e','a')

static GpStatus clone_font_family(const GpFontFamily *, GpFontFamily **);

static GpFontCollection installedFontCollection = {0};

/*******************************************************************************
 * GdipCreateFont [GDIPLUS.@]
 *
 * Create a new font based off of a FontFamily
 *
 * PARAMS
 *  *fontFamily     [I] Family to base the font off of
 *  emSize          [I] Size of the font
 *  style           [I] Bitwise OR of FontStyle enumeration
 *  unit            [I] Unit emSize is measured in
 *  **font          [I] the resulting Font object
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter if fontfamily or font is NULL.
 *  FAILURE: FontFamilyNotFound if an invalid FontFamily is given
 *
 * NOTES
 *  UnitDisplay is unsupported.
 *  emSize is stored separately from lfHeight, to hold the fraction.
 */
GpStatus WINGDIPAPI GdipCreateFont(GDIPCONST GpFontFamily *fontFamily,
                        REAL emSize, INT style, Unit unit, GpFont **font)
{
    HFONT hfont;
    OUTLINETEXTMETRICW otm;
    LOGFONTW lfw;
    HDC hdc;
    GpStatus stat;
    int ret;

    if (!fontFamily || !font || emSize < 0.0)
        return InvalidParameter;

    TRACE("%p (%s), %f, %d, %d, %p\n", fontFamily,
            debugstr_w(fontFamily->FamilyName), emSize, style, unit, font);

    memset(&lfw, 0, sizeof(lfw));

    stat = GdipGetFamilyName(fontFamily, lfw.lfFaceName, LANG_NEUTRAL);
    if (stat != Ok) return stat;

    lfw.lfHeight = -units_to_pixels(emSize, unit, fontFamily->dpi);
    lfw.lfWeight = style & FontStyleBold ? FW_BOLD : FW_REGULAR;
    lfw.lfItalic = style & FontStyleItalic;
    lfw.lfUnderline = style & FontStyleUnderline;
    lfw.lfStrikeOut = style & FontStyleStrikeout;

    hfont = CreateFontIndirectW(&lfw);
    hdc = CreateCompatibleDC(0);
    SelectObject(hdc, hfont);
    otm.otmSize = sizeof(otm);
    ret = GetOutlineTextMetricsW(hdc, otm.otmSize, &otm);
    DeleteDC(hdc);
    DeleteObject(hfont);

    if (!ret) return NotTrueTypeFont;

    *font = GdipAlloc(sizeof(GpFont));
    if (!*font) return OutOfMemory;

    (*font)->unit = unit;
    (*font)->emSize = emSize;
    (*font)->otm = otm;

    stat = clone_font_family(fontFamily, &(*font)->family);
    if (stat != Ok)
    {
        GdipFree(*font);
        return stat;
    }

    TRACE("<-- %p\n", *font);

    return Ok;
}

/*******************************************************************************
 * GdipCreateFontFromLogfontW [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateFontFromLogfontW(HDC hdc,
    GDIPCONST LOGFONTW *logfont, GpFont **font)
{
    HFONT hfont, oldfont;
    OUTLINETEXTMETRICW otm;
    WCHAR facename[LF_FACESIZE];
    GpStatus stat;
    int ret;

    TRACE("(%p, %p, %p)\n", hdc, logfont, font);

    if (!hdc || !logfont || !font)
        return InvalidParameter;

    hfont = CreateFontIndirectW(logfont);
    oldfont = SelectObject(hdc, hfont);
    otm.otmSize = sizeof(otm);
    ret = GetOutlineTextMetricsW(hdc, otm.otmSize, &otm);
    GetTextFaceW(hdc, LF_FACESIZE, facename);
    SelectObject(hdc, oldfont);
    DeleteObject(hfont);

    if (!ret) return NotTrueTypeFont;

    *font = GdipAlloc(sizeof(GpFont));
    if (!*font) return OutOfMemory;

    (*font)->unit = UnitWorld;
    (*font)->emSize = otm.otmTextMetrics.tmAscent;
    (*font)->otm = otm;

    stat = GdipCreateFontFamilyFromName(facename, NULL, &(*font)->family);
    if (stat != Ok)
    {
        GdipFree(*font);
        return NotTrueTypeFont;
    }

    TRACE("<-- %p\n", *font);

    return Ok;
}

/*******************************************************************************
 * GdipCreateFontFromLogfontA [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateFontFromLogfontA(HDC hdc,
    GDIPCONST LOGFONTA *lfa, GpFont **font)
{
    LOGFONTW lfw;

    TRACE("(%p, %p, %p)\n", hdc, lfa, font);

    if(!lfa || !font)
        return InvalidParameter;

    memcpy(&lfw, lfa, FIELD_OFFSET(LOGFONTA,lfFaceName) );

    if(!MultiByteToWideChar(CP_ACP, 0, lfa->lfFaceName, -1, lfw.lfFaceName, LF_FACESIZE))
        return GenericError;

    return GdipCreateFontFromLogfontW(hdc, &lfw, font);
}

/*******************************************************************************
 * GdipDeleteFont [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipDeleteFont(GpFont* font)
{
    TRACE("(%p)\n", font);

    if(!font)
        return InvalidParameter;

    GdipDeleteFontFamily(font->family);
    GdipFree(font);

    return Ok;
}

/*******************************************************************************
 * GdipCreateFontFromDC [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCreateFontFromDC(HDC hdc, GpFont **font)
{
    HFONT hfont;
    LOGFONTW lfw;

    TRACE("(%p, %p)\n", hdc, font);

    if(!font)
        return InvalidParameter;

    hfont = GetCurrentObject(hdc, OBJ_FONT);
    if(!hfont)
        return GenericError;

    if(!GetObjectW(hfont, sizeof(LOGFONTW), &lfw))
        return GenericError;

    return GdipCreateFontFromLogfontW(hdc, &lfw, font);
}

/*******************************************************************************
 * GdipGetFamily [GDIPLUS.@]
 *
 * Returns the FontFamily for the specified Font
 *
 * PARAMS
 *  font    [I] Font to request from
 *  family  [O] Resulting FontFamily object
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: An element of GpStatus
 */
GpStatus WINGDIPAPI GdipGetFamily(GpFont *font, GpFontFamily **family)
{
    TRACE("%p %p\n", font, family);

    if (!(font && family))
        return InvalidParameter;

    return GdipCloneFontFamily(font->family, family);
}

static REAL get_font_size(const GpFont *font)
{
    return font->emSize;
}

/******************************************************************************
 * GdipGetFontSize [GDIPLUS.@]
 *
 * Returns the size of the font in Units
 *
 * PARAMS
 *  *font       [I] The font to retrieve size from
 *  *size       [O] Pointer to hold retrieved value
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter (font or size was NULL)
 *
 * NOTES
 *  Size returned is actually emSize -- not internal size used for drawing.
 */
GpStatus WINGDIPAPI GdipGetFontSize(GpFont *font, REAL *size)
{
    TRACE("(%p, %p)\n", font, size);

    if (!(font && size)) return InvalidParameter;

    *size = get_font_size(font);
    TRACE("%s,%d => %f\n", debugstr_w(font->family->FamilyName), font->otm.otmTextMetrics.tmHeight, *size);

    return Ok;
}

static INT get_font_style(const GpFont *font)
{
    INT style;

    if (font->otm.otmTextMetrics.tmWeight > FW_REGULAR)
        style = FontStyleBold;
    else
        style = FontStyleRegular;
    if (font->otm.otmTextMetrics.tmItalic)
        style |= FontStyleItalic;
    if (font->otm.otmTextMetrics.tmUnderlined)
        style |= FontStyleUnderline;
    if (font->otm.otmTextMetrics.tmStruckOut)
        style |= FontStyleStrikeout;

    return style;
}

/*******************************************************************************
 * GdipGetFontStyle [GDIPLUS.@]
 *
 * Gets the font's style, returned in bitwise OR of FontStyle enumeration
 *
 * PARAMS
 *  font    [I] font to request from
 *  style   [O] resulting pointer to a FontStyle enumeration
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter
 */
GpStatus WINGDIPAPI GdipGetFontStyle(GpFont *font, INT *style)
{
    TRACE("%p %p\n", font, style);

    if (!(font && style))
        return InvalidParameter;

    *style = get_font_style(font);
    TRACE("%s,%d => %d\n", debugstr_w(font->family->FamilyName), font->otm.otmTextMetrics.tmHeight, *style);

    return Ok;
}

/*******************************************************************************
 * GdipGetFontUnit  [GDIPLUS.@]
 *
 * PARAMS
 *  font    [I] Font to retrieve from
 *  unit    [O] Return value
 *
 * RETURNS
 *  FAILURE: font or unit was NULL
 *  OK: otherwise
 */
GpStatus WINGDIPAPI GdipGetFontUnit(GpFont *font, Unit *unit)
{
    TRACE("(%p, %p)\n", font, unit);

    if (!(font && unit)) return InvalidParameter;

    *unit = font->unit;
    TRACE("%s,%d => %d\n", debugstr_w(font->family->FamilyName), font->otm.otmTextMetrics.tmHeight, *unit);

    return Ok;
}

/*******************************************************************************
 * GdipGetLogFontA [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetLogFontA(GpFont *font, GpGraphics *graphics,
    LOGFONTA *lfa)
{
    GpStatus status;
    LOGFONTW lfw;

    TRACE("(%p, %p, %p)\n", font, graphics, lfa);

    status = GdipGetLogFontW(font, graphics, &lfw);
    if(status != Ok)
        return status;

    memcpy(lfa, &lfw, FIELD_OFFSET(LOGFONTA,lfFaceName) );

    if(!WideCharToMultiByte(CP_ACP, 0, lfw.lfFaceName, -1, lfa->lfFaceName, LF_FACESIZE, NULL, NULL))
        return GenericError;

    return Ok;
}

/*******************************************************************************
 * GdipGetLogFontW [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetLogFontW(GpFont *font, GpGraphics *graphics, LOGFONTW *lf)
{
    REAL angle, rel_height, height;
    GpMatrix matrix;
    GpPointF pt[3];

    TRACE("(%p, %p, %p)\n", font, graphics, lf);

    if (!font || !graphics || !lf)
        return InvalidParameter;

    matrix = graphics->worldtrans;

    if (font->unit == UnitPixel || font->unit == UnitWorld)
    {
        height = units_to_pixels(font->emSize, graphics->unit, graphics->yres);
        if (graphics->unit != UnitDisplay)
            GdipScaleMatrix(&matrix, graphics->scale, graphics->scale, MatrixOrderAppend);
    }
    else
    {
        if (graphics->unit == UnitDisplay || graphics->unit == UnitPixel)
            height = units_to_pixels(font->emSize, font->unit, graphics->xres);
        else
            height = units_to_pixels(font->emSize, font->unit, graphics->yres);
    }

    pt[0].X = 0.0;
    pt[0].Y = 0.0;
    pt[1].X = 1.0;
    pt[1].Y = 0.0;
    pt[2].X = 0.0;
    pt[2].Y = 1.0;
    GdipTransformMatrixPoints(&matrix, pt, 3);
    angle = -gdiplus_atan2((pt[1].Y - pt[0].Y), (pt[1].X - pt[0].X));
    rel_height = sqrt((pt[2].Y - pt[0].Y) * (pt[2].Y - pt[0].Y)+
                      (pt[2].X - pt[0].X) * (pt[2].X - pt[0].X));

    lf->lfHeight = -gdip_round(height * rel_height);
    lf->lfWidth = 0;
    lf->lfEscapement = lf->lfOrientation = gdip_round((angle / M_PI) * 1800.0);
    if (lf->lfEscapement < 0)
    {
        lf->lfEscapement += 3600;
        lf->lfOrientation += 3600;
    }
    lf->lfWeight = font->otm.otmTextMetrics.tmWeight;
    lf->lfItalic = font->otm.otmTextMetrics.tmItalic ? 1 : 0;
    lf->lfUnderline = font->otm.otmTextMetrics.tmUnderlined ? 1 : 0;
    lf->lfStrikeOut = font->otm.otmTextMetrics.tmStruckOut ? 1 : 0;
    lf->lfCharSet = font->otm.otmTextMetrics.tmCharSet;
    lf->lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf->lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf->lfQuality = DEFAULT_QUALITY;
    lf->lfPitchAndFamily = 0;
    strcpyW(lf->lfFaceName, font->family->FamilyName);

    TRACE("=> %s,%d\n", debugstr_w(lf->lfFaceName), lf->lfHeight);

    return Ok;
}

/*******************************************************************************
 * GdipCloneFont [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipCloneFont(GpFont *font, GpFont **cloneFont)
{
    GpStatus stat;

    TRACE("(%p, %p)\n", font, cloneFont);

    if(!font || !cloneFont)
        return InvalidParameter;

    *cloneFont = GdipAlloc(sizeof(GpFont));
    if(!*cloneFont)    return OutOfMemory;

    **cloneFont = *font;
    stat = GdipCloneFontFamily(font->family, &(*cloneFont)->family);
    if (stat != Ok) GdipFree(*cloneFont);

    return stat;
}

/*******************************************************************************
 * GdipGetFontHeight [GDIPLUS.@]
 * PARAMS
 *  font        [I] Font to retrieve height from
 *  graphics    [I] The current graphics context
 *  height      [O] Resulting height
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: Another element of GpStatus
 *
 * NOTES
 *  Forwards to GdipGetFontHeightGivenDPI
 */
GpStatus WINGDIPAPI GdipGetFontHeight(GDIPCONST GpFont *font,
        GDIPCONST GpGraphics *graphics, REAL *height)
{
    REAL dpi;
    GpStatus stat;
    REAL font_height;

    TRACE("%p %p %p\n", font, graphics, height);

    stat = GdipGetFontHeightGivenDPI(font, font->family->dpi, &font_height);
    if (stat != Ok) return stat;

    if (!graphics)
    {
        *height = font_height;
        TRACE("%s,%d => %f\n",
              debugstr_w(font->family->FamilyName), font->otm.otmTextMetrics.tmHeight, *height);
        return Ok;
    }

    stat = GdipGetDpiY((GpGraphics *)graphics, &dpi);
    if (stat != Ok) return stat;

    *height = pixels_to_units(font_height, graphics->unit, dpi);

    TRACE("%s,%d(unit %d) => %f\n",
          debugstr_w(font->family->FamilyName), font->otm.otmTextMetrics.tmHeight, graphics->unit, *height);
    return Ok;
}

/*******************************************************************************
 * GdipGetFontHeightGivenDPI [GDIPLUS.@]
 * PARAMS
 *  font        [I] Font to retrieve DPI from
 *  dpi         [I] DPI to assume
 *  height      [O] Return value
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter if font or height is NULL
 *
 * NOTES
 *  According to MSDN, the result is (lineSpacing)*(fontSize / emHeight)*dpi
 *  (for anything other than unit Pixel)
 */
GpStatus WINGDIPAPI GdipGetFontHeightGivenDPI(GDIPCONST GpFont *font, REAL dpi, REAL *height)
{
    GpStatus stat;
    INT style;
    UINT16 line_spacing, em_height;
    REAL font_size;

    if (!font || !height) return InvalidParameter;

    TRACE("%p (%s), %f, %p\n", font,
            debugstr_w(font->family->FamilyName), dpi, height);

    font_size = units_to_pixels(get_font_size(font), font->unit, dpi);
    style = get_font_style(font);
    stat = GdipGetLineSpacing(font->family, style, &line_spacing);
    if (stat != Ok) return stat;
    stat = GdipGetEmHeight(font->family, style, &em_height);
    if (stat != Ok) return stat;

    *height = (REAL)line_spacing * font_size / (REAL)em_height;

    TRACE("%s,%d => %f\n",
          debugstr_w(font->family->FamilyName), font->otm.otmTextMetrics.tmHeight, *height);

    return Ok;
}

/***********************************************************************
 * Borrowed from GDI32:
 *
 * Elf is really an ENUMLOGFONTEXW, and ntm is a NEWTEXTMETRICEXW.
 *     We have to use other types because of the FONTENUMPROCW definition.
 */
static INT CALLBACK is_font_installed_proc(const LOGFONTW *elf,
                            const TEXTMETRICW *ntm, DWORD type, LPARAM lParam)
{
    if (type & RASTER_FONTTYPE)
        return 1;

    *(LOGFONTW *)lParam = *elf;

    return 0;
}

struct font_metrics
{
    WCHAR facename[LF_FACESIZE];
    UINT16 em_height, ascent, descent, line_spacing; /* in font units */
    int dpi;
};

static BOOL get_font_metrics(HDC hdc, struct font_metrics *fm)
{
    OUTLINETEXTMETRICW otm;
    TT_OS2_V2 tt_os2;
    TT_HHEA tt_hori;
    LONG size;
    UINT16 line_gap;

    otm.otmSize = sizeof(otm);
    if (!GetOutlineTextMetricsW(hdc, otm.otmSize, &otm)) return FALSE;

    GetTextFaceW(hdc, LF_FACESIZE, fm->facename);

    fm->em_height = otm.otmEMSquare;
    fm->dpi = GetDeviceCaps(hdc, LOGPIXELSY);

    memset(&tt_hori, 0, sizeof(tt_hori));
    if (GetFontData(hdc, MS_HHEA_TAG, 0, &tt_hori, sizeof(tt_hori)) != GDI_ERROR)
    {
        fm->ascent = GET_BE_WORD(tt_hori.Ascender);
        fm->descent = -GET_BE_WORD(tt_hori.Descender);
        TRACE("hhea: ascent %d, descent %d\n", fm->ascent, fm->descent);
        line_gap = GET_BE_WORD(tt_hori.LineGap);
        fm->line_spacing = fm->ascent + fm->descent + line_gap;
        TRACE("line_gap %u, line_spacing %u\n", line_gap, fm->line_spacing);
        if (fm->ascent + fm->descent != 0) return TRUE;
    }

    size = GetFontData(hdc, MS_OS2_TAG, 0, NULL, 0);
    if (size == GDI_ERROR) return FALSE;

    if (size > sizeof(tt_os2)) size = sizeof(tt_os2);

    memset(&tt_os2, 0, sizeof(tt_os2));
    if (GetFontData(hdc, MS_OS2_TAG, 0, &tt_os2, size) != size) return FALSE;

    fm->ascent = GET_BE_WORD(tt_os2.usWinAscent);
    fm->descent = GET_BE_WORD(tt_os2.usWinDescent);
    TRACE("usWinAscent %u, usWinDescent %u\n", fm->ascent, fm->descent);
    if (fm->ascent + fm->descent == 0)
    {
        fm->ascent = GET_BE_WORD(tt_os2.sTypoAscender);
        fm->descent = GET_BE_WORD(tt_os2.sTypoDescender);
        TRACE("sTypoAscender %u, sTypoDescender %u\n", fm->ascent, fm->descent);
    }
    line_gap = GET_BE_WORD(tt_os2.sTypoLineGap);
    fm->line_spacing = fm->ascent + fm->descent + line_gap;
    TRACE("line_gap %u, line_spacing %u\n", line_gap, fm->line_spacing);
    return TRUE;
}

static GpStatus find_installed_font(const WCHAR *name, struct font_metrics *fm)
{
    LOGFONTW lf;
    HDC hdc = CreateCompatibleDC(0);
    GpStatus ret = FontFamilyNotFound;

    if(!EnumFontFamiliesW(hdc, name, is_font_installed_proc, (LPARAM)&lf))
    {
        HFONT hfont, old_font;

        hfont = CreateFontIndirectW(&lf);
        old_font = SelectObject(hdc, hfont);
        ret = get_font_metrics(hdc, fm) ? Ok : NotTrueTypeFont;
        SelectObject(hdc, old_font);
        DeleteObject(hfont);
    }

    DeleteDC(hdc);
    return ret;
}

/*******************************************************************************
 * GdipCreateFontFamilyFromName [GDIPLUS.@]
 *
 * Creates a font family object based on a supplied name
 *
 * PARAMS
 *  name               [I] Name of the font
 *  fontCollection     [I] What font collection (if any) the font belongs to (may be NULL)
 *  FontFamily         [O] Pointer to the resulting FontFamily object
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: FamilyNotFound if the requested FontFamily does not exist on the system
 *  FAILURE: Invalid parameter if FontFamily or name is NULL
 *
 * NOTES
 *   If fontCollection is NULL then the object is not part of any collection
 *
 */

GpStatus WINGDIPAPI GdipCreateFontFamilyFromName(GDIPCONST WCHAR *name,
                                        GpFontCollection *fontCollection,
                                        GpFontFamily **FontFamily)
{
    GpStatus stat;
    GpFontFamily* ffamily;
    struct font_metrics fm;

    TRACE("%s, %p %p\n", debugstr_w(name), fontCollection, FontFamily);

    if (!(name && FontFamily))
        return InvalidParameter;
    if (fontCollection)
        FIXME("No support for FontCollections yet!\n");

    stat = find_installed_font(name, &fm);
    if (stat != Ok) return stat;

    ffamily = GdipAlloc(sizeof (GpFontFamily));
    if (!ffamily) return OutOfMemory;

    lstrcpyW(ffamily->FamilyName, fm.facename);
    ffamily->em_height = fm.em_height;
    ffamily->ascent = fm.ascent;
    ffamily->descent = fm.descent;
    ffamily->line_spacing = fm.line_spacing;
    ffamily->dpi = fm.dpi;

    *FontFamily = ffamily;

    TRACE("<-- %p\n", ffamily);

    return Ok;
}

static GpStatus clone_font_family(const GpFontFamily *family, GpFontFamily **clone)
{
    *clone = GdipAlloc(sizeof(GpFontFamily));
    if (!*clone) return OutOfMemory;

    **clone = *family;

    return Ok;
}

/*******************************************************************************
 * GdipCloneFontFamily [GDIPLUS.@]
 *
 * Creates a deep copy of a Font Family object
 *
 * PARAMS
 *  FontFamily          [I] Font to clone
 *  clonedFontFamily    [O] The resulting cloned font
 *
 * RETURNS
 *  SUCCESS: Ok
 */
GpStatus WINGDIPAPI GdipCloneFontFamily(GpFontFamily* FontFamily, GpFontFamily** clonedFontFamily)
{
    GpStatus status;

    if (!(FontFamily && clonedFontFamily)) return InvalidParameter;

    TRACE("%p (%s), %p\n", FontFamily,
            debugstr_w(FontFamily->FamilyName), clonedFontFamily);

    status = clone_font_family(FontFamily, clonedFontFamily);
    if (status != Ok) return status;

    TRACE("<-- %p\n", *clonedFontFamily);

    return Ok;
}

/*******************************************************************************
 * GdipGetFamilyName [GDIPLUS.@]
 *
 * Returns the family name into name
 *
 * PARAMS
 *  *family     [I] Family to retrieve from
 *  *name       [O] WCHARS of the family name
 *  LANGID      [I] charset
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter if family is NULL
 *
 * NOTES
 *   If name is a NULL ptr, then both XP and Vista will crash (so we do as well)
 */
GpStatus WINGDIPAPI GdipGetFamilyName (GDIPCONST GpFontFamily *family,
                                       WCHAR *name, LANGID language)
{
    static int lang_fixme;

    if (family == NULL)
         return InvalidParameter;

    TRACE("%p, %p, %d\n", family, name, language);

    if (language != LANG_NEUTRAL && !lang_fixme++)
        FIXME("No support for handling of multiple languages!\n");

    lstrcpynW (name, family->FamilyName, LF_FACESIZE);

    return Ok;
}


/*****************************************************************************
 * GdipDeleteFontFamily [GDIPLUS.@]
 *
 * Removes the specified FontFamily
 *
 * PARAMS
 *  *FontFamily         [I] The family to delete
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter if FontFamily is NULL.
 *
 */
GpStatus WINGDIPAPI GdipDeleteFontFamily(GpFontFamily *FontFamily)
{
    if (!FontFamily)
        return InvalidParameter;
    TRACE("Deleting %p (%s)\n", FontFamily, debugstr_w(FontFamily->FamilyName));

    GdipFree (FontFamily);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetCellAscent(GDIPCONST GpFontFamily *family,
        INT style, UINT16* CellAscent)
{
    if (!(family && CellAscent)) return InvalidParameter;

    *CellAscent = family->ascent;
    TRACE("%s => %u\n", debugstr_w(family->FamilyName), *CellAscent);

    return Ok;
}

GpStatus WINGDIPAPI GdipGetCellDescent(GDIPCONST GpFontFamily *family,
        INT style, UINT16* CellDescent)
{
    TRACE("(%p, %d, %p)\n", family, style, CellDescent);

    if (!(family && CellDescent)) return InvalidParameter;

    *CellDescent = family->descent;
    TRACE("%s => %u\n", debugstr_w(family->FamilyName), *CellDescent);

    return Ok;
}

/*******************************************************************************
 * GdipGetEmHeight [GDIPLUS.@]
 *
 * Gets the height of the specified family in EmHeights
 *
 * PARAMS
 *  family      [I] Family to retrieve from
 *  style       [I] (optional) style
 *  EmHeight    [O] return value
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter
 */
GpStatus WINGDIPAPI GdipGetEmHeight(GDIPCONST GpFontFamily *family, INT style, UINT16* EmHeight)
{
    if (!(family && EmHeight)) return InvalidParameter;

    TRACE("%p (%s), %d, %p\n", family, debugstr_w(family->FamilyName), style, EmHeight);

    *EmHeight = family->em_height;
    TRACE("%s => %u\n", debugstr_w(family->FamilyName), *EmHeight);

    return Ok;
}


/*******************************************************************************
 * GdipGetLineSpacing [GDIPLUS.@]
 *
 * Returns the line spacing in design units
 *
 * PARAMS
 *  family      [I] Family to retrieve from
 *  style       [I] (Optional) font style
 *  LineSpacing [O] Return value
 *
 * RETURNS
 *  SUCCESS: Ok
 *  FAILURE: InvalidParameter (family or LineSpacing was NULL)
 */
GpStatus WINGDIPAPI GdipGetLineSpacing(GDIPCONST GpFontFamily *family,
        INT style, UINT16* LineSpacing)
{
    TRACE("%p, %d, %p\n", family, style, LineSpacing);

    if (!(family && LineSpacing))
        return InvalidParameter;

    if (style) FIXME("ignoring style\n");

    *LineSpacing = family->line_spacing;
    TRACE("%s => %u\n", debugstr_w(family->FamilyName), *LineSpacing);

    return Ok;
}

static INT CALLBACK font_has_style_proc(const LOGFONTW *elf,
                            const TEXTMETRICW *ntm, DWORD type, LPARAM lParam)
{
    INT fontstyle = FontStyleRegular;

    if (!ntm) return 1;

    if (ntm->tmWeight >= FW_BOLD) fontstyle |= FontStyleBold;
    if (ntm->tmItalic) fontstyle |= FontStyleItalic;
    if (ntm->tmUnderlined) fontstyle |= FontStyleUnderline;
    if (ntm->tmStruckOut) fontstyle |= FontStyleStrikeout;

    return (INT)lParam != fontstyle;
}

GpStatus WINGDIPAPI GdipIsStyleAvailable(GDIPCONST GpFontFamily* family,
        INT style, BOOL* IsStyleAvailable)
{
    HDC hdc;

    TRACE("%p %d %p\n", family, style, IsStyleAvailable);

    if (!(family && IsStyleAvailable))
        return InvalidParameter;

    *IsStyleAvailable = FALSE;

    hdc = CreateCompatibleDC(0);

    if(!EnumFontFamiliesW(hdc, family->FamilyName, font_has_style_proc, (LPARAM)style))
        *IsStyleAvailable = TRUE;

    DeleteDC(hdc);

    return Ok;
}

/*****************************************************************************
 * GdipGetGenericFontFamilyMonospace [GDIPLUS.@]
 *
 * Obtains a serif family (Courier New on Windows)
 *
 * PARAMS
 *  **nativeFamily         [I] Where the font will be stored
 *
 * RETURNS
 *  InvalidParameter if nativeFamily is NULL.
 *  Ok otherwise.
 */
GpStatus WINGDIPAPI GdipGetGenericFontFamilyMonospace(GpFontFamily **nativeFamily)
{
    static const WCHAR CourierNew[] = {'C','o','u','r','i','e','r',' ','N','e','w','\0'};
    static const WCHAR LiberationMono[] = {'L','i','b','e','r','a','t','i','o','n',' ','M','o','n','o','\0'};
    GpStatus stat;

    if (nativeFamily == NULL) return InvalidParameter;

    stat = GdipCreateFontFamilyFromName(CourierNew, NULL, nativeFamily);

    if (stat == FontFamilyNotFound)
        stat = GdipCreateFontFamilyFromName(LiberationMono, NULL, nativeFamily);

    if (stat == FontFamilyNotFound)
        ERR("Missing 'Courier New' font\n");

    return stat;
}

/*****************************************************************************
 * GdipGetGenericFontFamilySerif [GDIPLUS.@]
 *
 * Obtains a serif family (Times New Roman on Windows)
 *
 * PARAMS
 *  **nativeFamily         [I] Where the font will be stored
 *
 * RETURNS
 *  InvalidParameter if nativeFamily is NULL.
 *  Ok otherwise.
 */
GpStatus WINGDIPAPI GdipGetGenericFontFamilySerif(GpFontFamily **nativeFamily)
{
    static const WCHAR TimesNewRoman[] = {'T','i','m','e','s',' ','N','e','w',' ','R','o','m','a','n','\0'};
    static const WCHAR LiberationSerif[] = {'L','i','b','e','r','a','t','i','o','n',' ','S','e','r','i','f','\0'};
    GpStatus stat;

    TRACE("(%p)\n", nativeFamily);

    if (nativeFamily == NULL) return InvalidParameter;

    stat = GdipCreateFontFamilyFromName(TimesNewRoman, NULL, nativeFamily);

    if (stat == FontFamilyNotFound)
        stat = GdipCreateFontFamilyFromName(LiberationSerif, NULL, nativeFamily);

    if (stat == FontFamilyNotFound)
        ERR("Missing 'Times New Roman' font\n");

    return stat;
}

/*****************************************************************************
 * GdipGetGenericFontFamilySansSerif [GDIPLUS.@]
 *
 * Obtains a serif family (Microsoft Sans Serif on Windows)
 *
 * PARAMS
 *  **nativeFamily         [I] Where the font will be stored
 *
 * RETURNS
 *  InvalidParameter if nativeFamily is NULL.
 *  Ok otherwise.
 */
GpStatus WINGDIPAPI GdipGetGenericFontFamilySansSerif(GpFontFamily **nativeFamily)
{
    GpStatus stat;
    static const WCHAR MicrosoftSansSerif[] = {'M','i','c','r','o','s','o','f','t',' ','S','a','n','s',' ','S','e','r','i','f','\0'};
    static const WCHAR Tahoma[] = {'T','a','h','o','m','a','\0'};

    TRACE("(%p)\n", nativeFamily);

    if (nativeFamily == NULL) return InvalidParameter;

    stat = GdipCreateFontFamilyFromName(MicrosoftSansSerif, NULL, nativeFamily);

    if (stat == FontFamilyNotFound)
        /* FIXME: Microsoft Sans Serif is not installed on Wine. */
        stat = GdipCreateFontFamilyFromName(Tahoma, NULL, nativeFamily);

    return stat;
}

/*****************************************************************************
 * GdipGetGenericFontFamilySansSerif [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipNewPrivateFontCollection(GpFontCollection** fontCollection)
{
    TRACE("%p\n", fontCollection);

    if (!fontCollection)
        return InvalidParameter;

    *fontCollection = GdipAlloc(sizeof(GpFontCollection));
    if (!*fontCollection) return OutOfMemory;

    (*fontCollection)->FontFamilies = NULL;
    (*fontCollection)->count = 0;
    (*fontCollection)->allocated = 0;

    TRACE("<-- %p\n", *fontCollection);

    return Ok;
}

/*****************************************************************************
 * GdipDeletePrivateFontCollection [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipDeletePrivateFontCollection(GpFontCollection **fontCollection)
{
    INT i;

    TRACE("%p\n", fontCollection);

    if (!fontCollection)
        return InvalidParameter;

    for (i = 0; i < (*fontCollection)->count; i++) GdipFree((*fontCollection)->FontFamilies[i]);
    GdipFree(*fontCollection);

    return Ok;
}

/*****************************************************************************
 * GdipPrivateAddFontFile [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipPrivateAddFontFile(GpFontCollection *collection, GDIPCONST WCHAR *name)
{
    HANDLE file, mapping;
    LARGE_INTEGER size;
    void *mem;
    GpStatus status;

    TRACE("%p, %s\n", collection, debugstr_w(name));

    if (!collection || !name) return InvalidParameter;

    file = CreateFileW(name, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (file == INVALID_HANDLE_VALUE) return InvalidParameter;

    if (!GetFileSizeEx(file, &size) || size.u.HighPart)
    {
        CloseHandle(file);
        return InvalidParameter;
    }

    mapping = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if (!mapping) return InvalidParameter;

    mem = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    if (!mem) return InvalidParameter;

    /* GdipPrivateAddMemoryFont creates a copy of the memory block */
    status = GdipPrivateAddMemoryFont(collection, mem, size.u.LowPart);
    UnmapViewOfFile(mem);

    return status;
}

#define TT_PLATFORM_APPLE_UNICODE   0
#define TT_PLATFORM_MACINTOSH       1
#define TT_PLATFORM_MICROSOFT       3

#define TT_APPLE_ID_DEFAULT     0
#define TT_APPLE_ID_ISO_10646   2
#define TT_APPLE_ID_UNICODE_2_0 3

#define TT_MS_ID_SYMBOL_CS  0
#define TT_MS_ID_UNICODE_CS 1

#define TT_MAC_ID_SIMPLIFIED_CHINESE    25

#define NAME_ID_FULL_FONT_NAME  4

typedef struct {
    USHORT major_version;
    USHORT minor_version;
    USHORT tables_no;
    USHORT search_range;
    USHORT entry_selector;
    USHORT range_shift;
} tt_header;

typedef struct {
    char tag[4];        /* table name */
    ULONG check_sum;    /* Check sum */
    ULONG offset;       /* Offset from beginning of file */
    ULONG length;       /* length of the table in bytes */
} tt_table_directory;

typedef struct {
    USHORT format;          /* format selector. Always 0 */
    USHORT count;           /* Name Records count */
    USHORT string_offset;   /* Offset for strings storage, * from start of the table */
} tt_name_table;

typedef struct {
    USHORT platform_id;
    USHORT encoding_id;
    USHORT language_id;
    USHORT name_id;
    USHORT length;
    USHORT offset;      /* from start of storage area */
} tt_name_record;

/* Copied from gdi32/freetype.c */

static const LANGID mac_langid_table[] =
{
    MAKELANGID(LANG_ENGLISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_ENGLISH */
    MAKELANGID(LANG_FRENCH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_FRENCH */
    MAKELANGID(LANG_GERMAN,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_GERMAN */
    MAKELANGID(LANG_ITALIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_ITALIAN */
    MAKELANGID(LANG_DUTCH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_DUTCH */
    MAKELANGID(LANG_SWEDISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SWEDISH */
    MAKELANGID(LANG_SPANISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SPANISH */
    MAKELANGID(LANG_DANISH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_DANISH */
    MAKELANGID(LANG_PORTUGUESE,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_PORTUGUESE */
    MAKELANGID(LANG_NORWEGIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_NORWEGIAN */
    MAKELANGID(LANG_HEBREW,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_HEBREW */
    MAKELANGID(LANG_JAPANESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_JAPANESE */
    MAKELANGID(LANG_ARABIC,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_ARABIC */
    MAKELANGID(LANG_FINNISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_FINNISH */
    MAKELANGID(LANG_GREEK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_GREEK */
    MAKELANGID(LANG_ICELANDIC,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_ICELANDIC */
    MAKELANGID(LANG_MALTESE,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_MALTESE */
    MAKELANGID(LANG_TURKISH,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TURKISH */
    MAKELANGID(LANG_CROATIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_CROATIAN */
    MAKELANGID(LANG_CHINESE_TRADITIONAL,SUBLANG_DEFAULT),    /* TT_MAC_LANGID_CHINESE_TRADITIONAL */
    MAKELANGID(LANG_URDU,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_URDU */
    MAKELANGID(LANG_HINDI,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_HINDI */
    MAKELANGID(LANG_THAI,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_THAI */
    MAKELANGID(LANG_KOREAN,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_KOREAN */
    MAKELANGID(LANG_LITHUANIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_LITHUANIAN */
    MAKELANGID(LANG_POLISH,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_POLISH */
    MAKELANGID(LANG_HUNGARIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_HUNGARIAN */
    MAKELANGID(LANG_ESTONIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ESTONIAN */
    MAKELANGID(LANG_LATVIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_LETTISH */
    MAKELANGID(LANG_SAMI,SUBLANG_DEFAULT),                   /* TT_MAC_LANGID_SAAMISK */
    MAKELANGID(LANG_FAEROESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_FAEROESE */
    MAKELANGID(LANG_FARSI,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_FARSI */
    MAKELANGID(LANG_RUSSIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_RUSSIAN */
    MAKELANGID(LANG_CHINESE_SIMPLIFIED,SUBLANG_DEFAULT),     /* TT_MAC_LANGID_CHINESE_SIMPLIFIED */
    MAKELANGID(LANG_DUTCH,SUBLANG_DUTCH_BELGIAN),            /* TT_MAC_LANGID_FLEMISH */
    MAKELANGID(LANG_IRISH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_IRISH */
    MAKELANGID(LANG_ALBANIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ALBANIAN */
    MAKELANGID(LANG_ROMANIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ROMANIAN */
    MAKELANGID(LANG_CZECH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_CZECH */
    MAKELANGID(LANG_SLOVAK,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_SLOVAK */
    MAKELANGID(LANG_SLOVENIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_SLOVENIAN */
    0,                                                       /* TT_MAC_LANGID_YIDDISH */
    MAKELANGID(LANG_SERBIAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SERBIAN */
    MAKELANGID(LANG_MACEDONIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_MACEDONIAN */
    MAKELANGID(LANG_BULGARIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_BULGARIAN */
    MAKELANGID(LANG_UKRAINIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_UKRAINIAN */
    MAKELANGID(LANG_BELARUSIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_BYELORUSSIAN */
    MAKELANGID(LANG_UZBEK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_UZBEK */
    MAKELANGID(LANG_KAZAK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_KAZAKH */
    MAKELANGID(LANG_AZERI,SUBLANG_AZERI_CYRILLIC),           /* TT_MAC_LANGID_AZERBAIJANI */
    0,                                                       /* TT_MAC_LANGID_AZERBAIJANI_ARABIC_SCRIPT */
    MAKELANGID(LANG_ARMENIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ARMENIAN */
    MAKELANGID(LANG_GEORGIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GEORGIAN */
    0,                                                       /* TT_MAC_LANGID_MOLDAVIAN */
    MAKELANGID(LANG_KYRGYZ,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_KIRGHIZ */
    MAKELANGID(LANG_TAJIK,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TAJIKI */
    MAKELANGID(LANG_TURKMEN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TURKMEN */
    MAKELANGID(LANG_MONGOLIAN,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_MONGOLIAN */
    MAKELANGID(LANG_MONGOLIAN,SUBLANG_MONGOLIAN_CYRILLIC_MONGOLIA), /* TT_MAC_LANGID_MONGOLIAN_CYRILLIC_SCRIPT */
    MAKELANGID(LANG_PASHTO,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_PASHTO */
    0,                                                       /* TT_MAC_LANGID_KURDISH */
    MAKELANGID(LANG_KASHMIRI,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_KASHMIRI */
    MAKELANGID(LANG_SINDHI,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_SINDHI */
    MAKELANGID(LANG_TIBETAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_TIBETAN */
    MAKELANGID(LANG_NEPALI,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_NEPALI */
    MAKELANGID(LANG_SANSKRIT,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_SANSKRIT */
    MAKELANGID(LANG_MARATHI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_MARATHI */
    MAKELANGID(LANG_BENGALI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_BENGALI */
    MAKELANGID(LANG_ASSAMESE,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_ASSAMESE */
    MAKELANGID(LANG_GUJARATI,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GUJARATI */
    MAKELANGID(LANG_PUNJABI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_PUNJABI */
    MAKELANGID(LANG_ORIYA,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_ORIYA */
    MAKELANGID(LANG_MALAYALAM,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_MALAYALAM */
    MAKELANGID(LANG_KANNADA,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_KANNADA */
    MAKELANGID(LANG_TAMIL,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TAMIL */
    MAKELANGID(LANG_TELUGU,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_TELUGU */
    MAKELANGID(LANG_SINHALESE,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_SINHALESE */
    0,                                                       /* TT_MAC_LANGID_BURMESE */
    MAKELANGID(LANG_KHMER,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_KHMER */
    MAKELANGID(LANG_LAO,SUBLANG_DEFAULT),                    /* TT_MAC_LANGID_LAO */
    MAKELANGID(LANG_VIETNAMESE,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_VIETNAMESE */
    MAKELANGID(LANG_INDONESIAN,SUBLANG_DEFAULT),             /* TT_MAC_LANGID_INDONESIAN */
    0,                                                       /* TT_MAC_LANGID_TAGALOG */
    MAKELANGID(LANG_MALAY,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_MALAY_ROMAN_SCRIPT */
    0,                                                       /* TT_MAC_LANGID_MALAY_ARABIC_SCRIPT */
    MAKELANGID(LANG_AMHARIC,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_AMHARIC */
    MAKELANGID(LANG_TIGRIGNA,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_TIGRINYA */
    0,                                                       /* TT_MAC_LANGID_GALLA */
    0,                                                       /* TT_MAC_LANGID_SOMALI */
    MAKELANGID(LANG_SWAHILI,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_SWAHILI */
    0,                                                       /* TT_MAC_LANGID_RUANDA */
    0,                                                       /* TT_MAC_LANGID_RUNDI */
    0,                                                       /* TT_MAC_LANGID_CHEWA */
    MAKELANGID(LANG_MALAGASY,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_MALAGASY */
    MAKELANGID(LANG_ESPERANTO,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_ESPERANTO */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,       /* 95-111 */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 112-127 */
    MAKELANGID(LANG_WELSH,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_WELSH */
    MAKELANGID(LANG_BASQUE,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_BASQUE */
    MAKELANGID(LANG_CATALAN,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_CATALAN */
    0,                                                       /* TT_MAC_LANGID_LATIN */
    MAKELANGID(LANG_QUECHUA,SUBLANG_DEFAULT),                /* TT_MAC_LANGID_QUECHUA */
    0,                                                       /* TT_MAC_LANGID_GUARANI */
    0,                                                       /* TT_MAC_LANGID_AYMARA */
    MAKELANGID(LANG_TATAR,SUBLANG_DEFAULT),                  /* TT_MAC_LANGID_TATAR */
    MAKELANGID(LANG_UIGHUR,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_UIGHUR */
    0,                                                       /* TT_MAC_LANGID_DZONGKHA */
    0,                                                       /* TT_MAC_LANGID_JAVANESE */
    0,                                                       /* TT_MAC_LANGID_SUNDANESE */
    MAKELANGID(LANG_GALICIAN,SUBLANG_DEFAULT),               /* TT_MAC_LANGID_GALICIAN */
    MAKELANGID(LANG_AFRIKAANS,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_AFRIKAANS */
    MAKELANGID(LANG_BRETON,SUBLANG_DEFAULT),                 /* TT_MAC_LANGID_BRETON */
    MAKELANGID(LANG_INUKTITUT,SUBLANG_DEFAULT),              /* TT_MAC_LANGID_INUKTITUT */
    MAKELANGID(LANG_SCOTTISH_GAELIC,SUBLANG_DEFAULT),        /* TT_MAC_LANGID_SCOTTISH_GAELIC */
    MAKELANGID(LANG_MANX_GAELIC,SUBLANG_DEFAULT),            /* TT_MAC_LANGID_MANX_GAELIC */
    MAKELANGID(LANG_IRISH,SUBLANG_IRISH_IRELAND),            /* TT_MAC_LANGID_IRISH_GAELIC */
    0,                                                       /* TT_MAC_LANGID_TONGAN */
    0,                                                       /* TT_MAC_LANGID_GREEK_POLYTONIC */
    MAKELANGID(LANG_GREENLANDIC,SUBLANG_DEFAULT),            /* TT_MAC_LANGID_GREELANDIC */
    MAKELANGID(LANG_AZERI,SUBLANG_AZERI_LATIN),              /* TT_MAC_LANGID_AZERBAIJANI_ROMAN_SCRIPT */
};

static inline WORD get_mac_code_page( const tt_name_record *name )
{
    WORD encoding_id = GET_BE_WORD(name->encoding_id);
    if (encoding_id == TT_MAC_ID_SIMPLIFIED_CHINESE) return 10008;  /* special case */
    return 10000 + encoding_id;
}

static int match_name_table_language( const tt_name_record *name, LANGID lang )
{
    LANGID name_lang;

    switch (GET_BE_WORD(name->platform_id))
    {
    case TT_PLATFORM_MICROSOFT:
        switch (GET_BE_WORD(name->encoding_id))
        {
        case TT_MS_ID_UNICODE_CS:
        case TT_MS_ID_SYMBOL_CS:
            name_lang = GET_BE_WORD(name->language_id);
            break;
        default:
            return 0;
        }
        break;
    case TT_PLATFORM_MACINTOSH:
        if (!IsValidCodePage( get_mac_code_page( name ))) return 0;
        name_lang = GET_BE_WORD(name->language_id);
        if (name_lang >= sizeof(mac_langid_table)/sizeof(mac_langid_table[0])) return 0;
        name_lang = mac_langid_table[name_lang];
        break;
    case TT_PLATFORM_APPLE_UNICODE:
        switch (GET_BE_WORD(name->encoding_id))
        {
        case TT_APPLE_ID_DEFAULT:
        case TT_APPLE_ID_ISO_10646:
        case TT_APPLE_ID_UNICODE_2_0:
            name_lang = GET_BE_WORD(name->language_id);
            if (name_lang >= sizeof(mac_langid_table)/sizeof(mac_langid_table[0])) return 0;
            name_lang = mac_langid_table[name_lang];
            break;
        default:
            return 0;
        }
        break;
    default:
        return 0;
    }
    if (name_lang == lang) return 3;
    if (PRIMARYLANGID( name_lang ) == PRIMARYLANGID( lang )) return 2;
    if (name_lang == MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT )) return 1;
    return 0;
}

static WCHAR *copy_name_table_string( const tt_name_record *name, const BYTE *data, WCHAR *ret, DWORD len )
{
    WORD name_len = GET_BE_WORD(name->length);
    WORD codepage;

    switch (GET_BE_WORD(name->platform_id))
    {
    case TT_PLATFORM_APPLE_UNICODE:
    case TT_PLATFORM_MICROSOFT:
        if (name_len >= len*sizeof(WCHAR))
            return NULL;
        for (len = 0; len < name_len / 2; len++)
            ret[len] = (data[len * 2] << 8) | data[len * 2 + 1];
        ret[len] = 0;
        return ret;
    case TT_PLATFORM_MACINTOSH:
        codepage = get_mac_code_page( name );
        len = MultiByteToWideChar( codepage, 0, (char *)data, name_len, ret, len-1 );
        if (!len)
            return NULL;
        ret[len] = 0;
        return ret;
    }
    return NULL;
}

static WCHAR *load_ttf_name_id( const BYTE *mem, DWORD_PTR size, DWORD id, WCHAR *ret, DWORD len )
{
    LANGID lang = GetSystemDefaultLangID();
    const tt_header *header;
    const tt_name_table *name_table;
    const tt_name_record *name_record;
    DWORD pos, ofs, count;
    int i, res, best_lang = 0, best_index = -1;

    if (sizeof(tt_header) > size)
        return NULL;
    header = (const tt_header*)mem;
    count = GET_BE_WORD(header->tables_no);

    if (GET_BE_WORD(header->major_version) != 1 || GET_BE_WORD(header->minor_version) != 0)
        return NULL;

    pos = sizeof(*header);
    for (i = 0; i < count; i++)
    {
        const tt_table_directory *table_directory = (const tt_table_directory*)&mem[pos];
        pos += sizeof(*table_directory);
        if (memcmp(table_directory->tag, "name", 4) == 0)
        {
            ofs = GET_BE_DWORD(table_directory->offset);
            break;
        }
    }
    if (i >= count)
        return NULL;

    if (ofs >= size)
        return NULL;
    pos = ofs + sizeof(*name_table);
    if (pos > size)
        return NULL;
    name_table = (const tt_name_table*)&mem[ofs];
    count =  GET_BE_WORD(name_table->count);
    if (GET_BE_WORD(name_table->string_offset) >= size - ofs) return NULL;
    ofs += GET_BE_WORD(name_table->string_offset);
    for (i=0; i<count; i++)
    {
        name_record = (const tt_name_record*)&mem[pos];
        pos += sizeof(*name_record);
        if (pos > size)
            return NULL;

        if (GET_BE_WORD(name_record->name_id) != id) continue;
        if (GET_BE_WORD(name_record->offset) >= size - ofs) return NULL;
        if (GET_BE_WORD(name_record->length) > size - ofs - GET_BE_WORD(name_record->offset)) return NULL;

        res = match_name_table_language( name_record, lang );
        if (res > best_lang)
        {
            best_lang = res;
            best_index = i;
        }
    }

    if (best_lang)
    {
        name_record = (const tt_name_record*)(name_table + 1) + best_index;
        ret = copy_name_table_string( name_record, mem+ofs+GET_BE_WORD(name_record->offset), ret, len );
        TRACE( "name %u found platform %u lang %04x %s\n", GET_BE_WORD(name_record->name_id),
                GET_BE_WORD(name_record->platform_id), GET_BE_WORD(name_record->language_id), debugstr_w( ret ));
        return ret;
    }
    return NULL;
}

static INT CALLBACK add_font_proc(const LOGFONTW *lfw, const TEXTMETRICW *ntm, DWORD type, LPARAM lParam);

/*****************************************************************************
 * GdipPrivateAddMemoryFont [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipPrivateAddMemoryFont(GpFontCollection* fontCollection,
        GDIPCONST void* memory, INT length)
{
    WCHAR buf[32], *name;
    DWORD count = 0;
    HANDLE font;
    TRACE("%p, %p, %d\n", fontCollection, memory, length);

    if (!fontCollection || !memory || !length)
        return InvalidParameter;

    name = load_ttf_name_id(memory, length, NAME_ID_FULL_FONT_NAME, buf, sizeof(buf)/sizeof(*buf));
    if (!name)
        return OutOfMemory;

    font = AddFontMemResourceEx((void*)memory, length, NULL, &count);
    TRACE("%s: %p/%u\n", debugstr_w(name), font, count);
    if (!font || !count)
        return InvalidParameter;

    if (count)
    {
        HDC hdc;
        LOGFONTW lfw;

        hdc = CreateCompatibleDC(0);

        lfw.lfCharSet = DEFAULT_CHARSET;
        lstrcpyW(lfw.lfFaceName, name);
        lfw.lfPitchAndFamily = 0;

        if (!EnumFontFamiliesExW(hdc, &lfw, add_font_proc, (LPARAM)fontCollection, 0))
        {
            DeleteDC(hdc);
            return OutOfMemory;
        }

        DeleteDC(hdc);
    }
    return Ok;
}

/*****************************************************************************
 * GdipGetFontCollectionFamilyCount [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetFontCollectionFamilyCount(
        GpFontCollection* fontCollection, INT* numFound)
{
    TRACE("%p, %p\n", fontCollection, numFound);

    if (!(fontCollection && numFound))
        return InvalidParameter;

    *numFound = fontCollection->count;
    return Ok;
}

/*****************************************************************************
 * GdipGetFontCollectionFamilyList [GDIPLUS.@]
 */
GpStatus WINGDIPAPI GdipGetFontCollectionFamilyList(
        GpFontCollection* fontCollection, INT numSought,
        GpFontFamily* gpfamilies[], INT* numFound)
{
    INT i;
    GpStatus stat=Ok;

    TRACE("%p, %d, %p, %p\n", fontCollection, numSought, gpfamilies, numFound);

    if (!(fontCollection && gpfamilies && numFound))
        return InvalidParameter;

    memset(gpfamilies, 0, sizeof(*gpfamilies) * numSought);

    for (i = 0; i < numSought && i < fontCollection->count && stat == Ok; i++)
    {
        stat = GdipCloneFontFamily(fontCollection->FontFamilies[i], &gpfamilies[i]);
    }

    if (stat == Ok)
        *numFound = i;
    else
    {
        int numToFree=i;
        for (i=0; i<numToFree; i++)
        {
            GdipDeleteFontFamily(gpfamilies[i]);
            gpfamilies[i] = NULL;
        }
    }

    return stat;
}

void free_installed_fonts(void)
{
    while (installedFontCollection.count)
        GdipDeleteFontFamily(installedFontCollection.FontFamilies[--installedFontCollection.count]);
    HeapFree(GetProcessHeap(), 0, installedFontCollection.FontFamilies);
    installedFontCollection.FontFamilies = NULL;
    installedFontCollection.allocated = 0;
}

static INT CALLBACK add_font_proc(const LOGFONTW *lfw, const TEXTMETRICW *ntm,
        DWORD type, LPARAM lParam)
{
    GpFontCollection* fonts = (GpFontCollection*)lParam;
    int i;

    if (type == RASTER_FONTTYPE)
        return 1;

    /* skip duplicates */
    for (i=0; i<fonts->count; i++)
        if (strcmpiW(lfw->lfFaceName, fonts->FontFamilies[i]->FamilyName) == 0)
            return 1;

    if (fonts->allocated == fonts->count)
    {
        INT new_alloc_count = fonts->allocated+50;
        GpFontFamily** new_family_list = HeapAlloc(GetProcessHeap(), 0, new_alloc_count*sizeof(void*));

        if (!new_family_list)
            return 0;

        memcpy(new_family_list, fonts->FontFamilies, fonts->count*sizeof(void*));
        HeapFree(GetProcessHeap(), 0, fonts->FontFamilies);
        fonts->FontFamilies = new_family_list;
        fonts->allocated = new_alloc_count;
    }

    if (GdipCreateFontFamilyFromName(lfw->lfFaceName, NULL, &fonts->FontFamilies[fonts->count]) == Ok)
        fonts->count++;
    else
        return 0;

    return 1;
}

GpStatus WINGDIPAPI GdipNewInstalledFontCollection(
        GpFontCollection** fontCollection)
{
    TRACE("(%p)\n",fontCollection);

    if (!fontCollection)
        return InvalidParameter;

    if (installedFontCollection.count == 0)
    {
        HDC hdc;
        LOGFONTW lfw;

        hdc = CreateCompatibleDC(0);

        lfw.lfCharSet = DEFAULT_CHARSET;
        lfw.lfFaceName[0] = 0;
        lfw.lfPitchAndFamily = 0;

        if (!EnumFontFamiliesExW(hdc, &lfw, add_font_proc, (LPARAM)&installedFontCollection, 0))
        {
            free_installed_fonts();
            DeleteDC(hdc);
            return OutOfMemory;
        }

        DeleteDC(hdc);
    }

    *fontCollection = &installedFontCollection;

    return Ok;
}
