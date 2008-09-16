/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2006, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  GDIFontInstance.cpp
 *
 *   created on: 08/09/2000
 *   created by: Eric R. Mader
 */

#include <windows.h>

#include "layout/LETypes.h"
#include "layout/LESwaps.h"
#include "layout/LEFontInstance.h"

#include "GDIFontInstance.h"
#include "sfnt.h"
#include "cmaps.h"

GDISurface::GDISurface(HDC theHDC)
    : fHdc(theHDC), fCurrentFont(NULL)
{
    // nothing else to do
}

GDISurface::~GDISurface()
{
    // nothing to do
}

void GDISurface::setHDC(HDC theHDC)
{
    fHdc         = theHDC;
    fCurrentFont = NULL;
}

void GDISurface::setFont(const GDIFontInstance *font)
{
#if 0
    if (fCurrentFont != font) {
        fCurrentFont = font;
        SelectObject(fHdc, font->getFont());
    }
#else
    SelectObject(fHdc, font->getFont());
#endif
}

void GDISurface::drawGlyphs(const LEFontInstance *font, const LEGlyphID *glyphs, le_int32 count, const float *positions,
    le_int32 x, le_int32 y, le_int32 width, le_int32 height)
{
    TTGlyphID *ttGlyphs = LE_NEW_ARRAY(TTGlyphID, count);
    le_int32  *dx = LE_NEW_ARRAY(le_int32, count);
    float     *ps = LE_NEW_ARRAY(float, count * 2 + 2);
    le_int32   out = 0;
    RECT clip;

    clip.top    = 0;
    clip.left   = 0;
    clip.bottom = height;
    clip.right  = width;

    for (le_int32 g = 0; g < count; g += 1) {
        TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyphs[g]);

        if (ttGlyph < 0xFFFE) {
            ttGlyphs[out] = ttGlyph;
            dx[out] = (le_int32) (positions[g * 2 + 2] - positions[g * 2]);
            ps[out * 2] = positions[g * 2];
            ps[out * 2 + 1] = positions[g * 2 + 1];
            out += 1;
        }
    }

    le_int32 dyStart, dyEnd;

    setFont((GDIFontInstance *) font);

    dyStart = dyEnd = 0;

    while (dyEnd < out) {
        float yOffset = ps[dyStart * 2 + 1];
        float xOffset = ps[dyStart * 2];

        while (dyEnd < out && yOffset == ps[dyEnd * 2 + 1]) {
            dyEnd += 1;
        }

        ExtTextOut(fHdc, x + (le_int32) xOffset, y + (le_int32) yOffset - font->getAscent(), ETO_CLIPPED | ETO_GLYPH_INDEX, &clip,
            (LPCWSTR) &ttGlyphs[dyStart], dyEnd - dyStart, (INT *) &dx[dyStart]);

        dyStart = dyEnd;
    }

    LE_DELETE_ARRAY(ps);
    LE_DELETE_ARRAY(dx);
    LE_DELETE_ARRAY(ttGlyphs);
}

GDIFontInstance::GDIFontInstance(GDISurface *surface, TCHAR *faceName, le_int16 pointSize, LEErrorCode &status)
    : FontTableCache(), fSurface(surface), fFont(NULL),
      fPointSize(pointSize), fUnitsPerEM(0), fAscent(0), fDescent(0), fLeading(0),
      fDeviceScaleX(1), fDeviceScaleY(1), fMapper(NULL)
{
    LOGFONT lf;
    FLOAT dpiX, dpiY;
    POINT pt;
    OUTLINETEXTMETRIC otm;
    HDC hdc = surface->getHDC();

    if (LE_FAILURE(status)) {
        return;
    }

    SaveDC(hdc);

    SetGraphicsMode(hdc, GM_ADVANCED);
    ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
    SetViewportOrgEx(hdc, 0, 0, NULL);
    SetWindowOrgEx(hdc, 0, 0, NULL);

    dpiX = (FLOAT) GetDeviceCaps(hdc, LOGPIXELSX);
    dpiY = (FLOAT) GetDeviceCaps(hdc, LOGPIXELSY);

#if 1
    pt.x = (int) (pointSize * dpiX / 72);
    pt.y = (int) (pointSize * dpiY / 72);

    DPtoLP(hdc, &pt, 1);
#else
    pt.x = pt.y = pointSize;
#endif

    lf.lfHeight = - pt.y;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 0;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    lf.lfQuality = 0;
    lf.lfPitchAndFamily = 0;

    lstrcpy(lf.lfFaceName, faceName);

    fFont = CreateFontIndirect(&lf);

    if (fFont == NULL) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

    SelectObject(hdc, fFont);

    UINT ret = GetOutlineTextMetrics(hdc, sizeof otm, &otm);

    if (ret == 0) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto restore;
    }

    fUnitsPerEM = otm.otmEMSquare;
    fAscent  = otm.otmTextMetrics.tmAscent;
    fDescent = otm.otmTextMetrics.tmDescent;
    fLeading = otm.otmTextMetrics.tmExternalLeading;

    status = initMapper();

    if (LE_FAILURE(status)) {
        goto restore;
    }

#if 0
    status = initFontTableCache();
#endif

restore:
    RestoreDC(hdc, -1);
}

GDIFontInstance::GDIFontInstance(GDISurface *surface, const char *faceName, le_int16 pointSize, LEErrorCode &status)
    : FontTableCache(), fSurface(surface), fFont(NULL),
      fPointSize(pointSize), fUnitsPerEM(0), fAscent(0), fDescent(0), fLeading(0),
      fDeviceScaleX(1), fDeviceScaleY(1), fMapper(NULL)
{
    LOGFONTA lf;
    FLOAT dpiX, dpiY;
    POINT pt;
    OUTLINETEXTMETRIC otm;
    HDC hdc = surface->getHDC();

    if (LE_FAILURE(status)) {
        return;
    }

    SaveDC(hdc);

    SetGraphicsMode(hdc, GM_ADVANCED);
    ModifyWorldTransform(hdc, NULL, MWT_IDENTITY);
    SetViewportOrgEx(hdc, 0, 0, NULL);
    SetWindowOrgEx(hdc, 0, 0, NULL);

    dpiX = (FLOAT) GetDeviceCaps(hdc, LOGPIXELSX);
    dpiY = (FLOAT) GetDeviceCaps(hdc, LOGPIXELSY);

    fDeviceScaleX = dpiX / 72;
    fDeviceScaleY = dpiY / 72;

#if 1
    pt.x = (int) (pointSize * fDeviceScaleX);
    pt.y = (int) (pointSize * fDeviceScaleY);

    DPtoLP(hdc, &pt, 1);
#else
    pt.x = pt.y = pointSize;
#endif

    lf.lfHeight = - pt.y;
    lf.lfWidth = 0;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfWeight = 0;
    lf.lfItalic = 0;
    lf.lfUnderline = 0;
    lf.lfStrikeOut = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfOutPrecision = 0;
    lf.lfClipPrecision = 0;
    lf.lfQuality = 0;
    lf.lfPitchAndFamily = 0;

    strcpy(lf.lfFaceName, faceName);

    fFont = CreateFontIndirectA(&lf);

    if (fFont == NULL) {
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

    SelectObject(hdc, fFont);

    UINT ret = GetOutlineTextMetrics(hdc, sizeof otm, &otm);

    if (ret == 0) {
        status = LE_MISSING_FONT_TABLE_ERROR;
        goto restore;
    }

    fUnitsPerEM = otm.otmEMSquare;
    fAscent  = otm.otmTextMetrics.tmAscent;
    fDescent = otm.otmTextMetrics.tmDescent;
    fLeading = otm.otmTextMetrics.tmExternalLeading;

    status = initMapper();

    if (LE_FAILURE(status)) {
        goto restore;
    }

#if 0
    status = initFontTableCache();
#endif

restore:
    RestoreDC(hdc, -1);
}

GDIFontInstance::~GDIFontInstance()
{
#if 0
    flushFontTableCache();
    delete[] fTableCache;
#endif

    if (fFont != NULL) {
        // FIXME: call RemoveObject first?
        DeleteObject(fFont);
    }

    delete fMapper;
    fMapper = NULL;
}

LEErrorCode GDIFontInstance::initMapper()
{
    LETag cmapTag = LE_CMAP_TABLE_TAG;
    const CMAPTable *cmap = (const CMAPTable *) readFontTable(cmapTag);

    if (cmap == NULL) {
        return LE_MISSING_FONT_TABLE_ERROR;
    }

    fMapper = CMAPMapper::createUnicodeMapper(cmap);

    if (fMapper == NULL) {
        return LE_MISSING_FONT_TABLE_ERROR;
    }

    return LE_NO_ERROR;
}

const void *GDIFontInstance::getFontTable(LETag tableTag) const
{
    return FontTableCache::find(tableTag);
}

const void *GDIFontInstance::readFontTable(LETag tableTag) const
{
    fSurface->setFont(this);

    HDC   hdc    = fSurface->getHDC();
    DWORD stag   = SWAPL(tableTag);
    DWORD len    = GetFontData(hdc, stag, 0, NULL, 0);
    void *result = NULL;

    if (len != GDI_ERROR) {
        result = LE_NEW_ARRAY(char, len);
        GetFontData(hdc, stag, 0, result, len);
    }

    return result;
}

void GDIFontInstance::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    advance.fX = 0;
    advance.fY = 0;

    if (glyph == 0xFFFE || glyph == 0xFFFF) {
        return;
    }


    GLYPHMETRICS metrics;
    DWORD result;
    MAT2 identity = {{0, 1}, {0, 0}, {0, 0}, {0, 1}};
    HDC hdc = fSurface->getHDC();

    fSurface->setFont(this);

    result = GetGlyphOutline(hdc, glyph, GGO_GLYPH_INDEX | GGO_METRICS, &metrics, 0, NULL, &identity);

    if (result == GDI_ERROR) {
        return;
    }

    advance.fX = metrics.gmCellIncX;
    return;
}

le_bool GDIFontInstance::getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const
{
#if 0
    hsFixedPoint2 pt;
    le_bool result;

    result = fFontInstance->getGlyphPoint(glyph, pointNumber, pt);

    if (result) {
        point.fX = xUnitsToPoints(pt.fX);
        point.fY = yUnitsToPoints(pt.fY);
    }

    return result;
#else
    return FALSE;
#endif
}

