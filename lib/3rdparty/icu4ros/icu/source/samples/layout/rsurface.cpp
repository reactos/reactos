/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include "loengine.h"
#include "rsurface.h"

#include "LETypes.h"
#include "LEFontInstance.h"
#include "RenderingSurface.h"

U_CDECL_BEGIN

void rs_drawGlyphs(rs_surface *surface, const le_font *font, const LEGlyphID *glyphs, le_int32 count,
                   const float *positions, le_int32 x, le_int32 y, le_int32 width, le_int32 height)
{
    RenderingSurface *rs = (RenderingSurface *) surface;

    rs->drawGlyphs((const LEFontInstance *) font, glyphs, count, positions, x, y, width, height);
}

U_CDECL_END
