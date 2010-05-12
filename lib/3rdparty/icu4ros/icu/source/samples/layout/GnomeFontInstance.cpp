/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  GnomeFontInstance.cpp
 *
 *   created on: 08/30/2001
 *   created by: Eric R. Mader
 */

#include <gnome.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_RENDER_H
#include FT_TRUETYPE_TABLES_H
#include <cairo.h>
#include <cairo-ft.h>

#include "layout/LETypes.h"
#include "layout/LESwaps.h"

#include "GnomeFontInstance.h"
#include "sfnt.h"
#include "cmaps.h"

GnomeSurface::GnomeSurface(GtkWidget *theWidget)
    : fWidget(theWidget)
{
    fCairo = gdk_cairo_create(fWidget->window);
}

GnomeSurface::~GnomeSurface()
{
    cairo_destroy(fCairo);
}

void GnomeSurface::drawGlyphs(const LEFontInstance *font, const LEGlyphID *glyphs, le_int32 count,
                              const float *positions, le_int32 x, le_int32 y, le_int32 /*width*/, le_int32 /*height*/)
{
    GnomeFontInstance *gFont = (GnomeFontInstance *) font;
    
    gFont->rasterizeGlyphs(fCairo, glyphs, count, positions, x, y);
}

GnomeFontInstance::GnomeFontInstance(FT_Library engine, const char *fontPathName, le_int16 pointSize, LEErrorCode &status)
    : FontTableCache(), fPointSize(pointSize), fUnitsPerEM(0), fAscent(0), fDescent(0), fLeading(0),
      fDeviceScaleX(1), fDeviceScaleY(1), fMapper(NULL)
{
    FT_Error error;

    fFace      = NULL;
    fCairoFace = NULL;

    error = FT_New_Face(engine, fontPathName, 0, &fFace);

    if (error != 0) {
        printf("OOPS! Got error code %d\n", error);
        status = LE_FONT_FILE_NOT_FOUND_ERROR;
        return;
    }

    // FIXME: what about the display resolution?
    fDeviceScaleX = ((float) 96) / 72;
    fDeviceScaleY = ((float) 96) / 72;

    error = FT_Set_Char_Size(fFace, 0, pointSize << 6, 92, 92);
    
    fCairoFace = cairo_ft_font_face_create_for_ft_face(fFace, 0);

    fUnitsPerEM = fFace->units_per_EM;

    fAscent  = (le_int32) (yUnitsToPoints(fFace->ascender) * fDeviceScaleY);
    fDescent = (le_int32) -(yUnitsToPoints(fFace->descender) * fDeviceScaleY);
    fLeading = (le_int32) (yUnitsToPoints(fFace->height) * fDeviceScaleY) - fAscent - fDescent;

    // printf("Face = %s, unitsPerEM = %d, ascent = %d, descent = %d\n", fontPathName, fUnitsPerEM, fAscent, fDescent);

    if (error != 0) {
        status = LE_MEMORY_ALLOCATION_ERROR;
        return;
    }

    status = initMapper();
}

GnomeFontInstance::~GnomeFontInstance()
{
    cairo_font_face_destroy(fCairoFace);
    
    if (fFace != NULL) {
        FT_Done_Face(fFace);
    }
}

LEErrorCode GnomeFontInstance::initMapper()
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

const void *GnomeFontInstance::getFontTable(LETag tableTag) const
{
    return FontTableCache::find(tableTag);
}

const void *GnomeFontInstance::readFontTable(LETag tableTag) const
{
    FT_ULong len = 0;
    FT_Byte *result = NULL;

    FT_Load_Sfnt_Table(fFace, tableTag, 0, NULL, &len);

    if (len > 0) {
        result = LE_NEW_ARRAY(FT_Byte, len);
        FT_Load_Sfnt_Table(fFace, tableTag, 0, result, &len);
    }

    return result;
}

void GnomeFontInstance::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    advance.fX = 0;
    advance.fY = 0;

    if (glyph >= 0xFFFE) {
        return;
    }

    FT_Error error;

    error = FT_Load_Glyph(fFace, glyph, FT_LOAD_DEFAULT);

    if (error != 0) {
        return;
    }

    advance.fX = fFace->glyph->metrics.horiAdvance >> 6;
    return;
}

le_bool GnomeFontInstance::getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const
{
    FT_Error error;

    error = FT_Load_Glyph(fFace, glyph, FT_LOAD_DEFAULT);

    if (error != 0) {
        return FALSE;
    }

    if (pointNumber >= fFace->glyph->outline.n_points) {
        return FALSE;
    }

    point.fX = fFace->glyph->outline.points[pointNumber].x >> 6;
    point.fY = fFace->glyph->outline.points[pointNumber].y >> 6;

    return TRUE;
}

void GnomeFontInstance::rasterizeGlyphs(cairo_t *cairo, const LEGlyphID *glyphs, le_int32 glyphCount, const float *positions,
                                        le_int32 x, le_int32 y) const
{
    cairo_glyph_t *glyph_t = LE_NEW_ARRAY(cairo_glyph_t, glyphCount);
    le_int32 in, out;
    
    for (in = 0, out = 0; in < glyphCount; in += 1) {
        TTGlyphID glyph = LE_GET_GLYPH(glyphs[in]);
        
        if (glyph < 0xFFFE) {
            glyph_t[out].index = glyph;
            glyph_t[out].x     = x + positions[in*2];
            glyph_t[out].y     = y + positions[in*2 + 1];
            
            out += 1;
        }
    }
    
    cairo_set_font_face(cairo, fCairoFace);
    cairo_set_font_size(cairo, getXPixelsPerEm() * getScaleFactorX());
    cairo_show_glyphs(cairo, glyph_t, out);
    
    LE_DELETE_ARRAY(glyph_t);
}
