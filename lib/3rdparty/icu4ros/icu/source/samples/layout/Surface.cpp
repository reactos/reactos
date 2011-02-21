/*
 *   Copyright (C) 2003, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 */
void GDISurface::setFont(RenderingFontInstance *font)
{
    GDIFontInstance *gFont = (GDIFontInstance *) font;

    if (fCurrentFont != font) {
        fCurrentFont = font;
        SelectObject(fHdc, gFont->fFont);
    }
}

void GDISurface::drawGlyphs(RenderingFontInstance *font, const LEGlyphID *glyphs, le_int32 count, const le_int32 *dx,
    le_int32 x, le_int32 y, le_int32 width, le_int32 height)
{
    RECT clip;

    clip.top    = 0;
    clip.left   = 0;
    clip.bottom = height;
    clip.right  = width;

    setFont(font);

    ExtTextOut(fHdc, x, y - fAscent, ETO_CLIPPED | ETO_GLYPH_INDEX, &clip,
        glyphs, count, (INT *) dx);
}

