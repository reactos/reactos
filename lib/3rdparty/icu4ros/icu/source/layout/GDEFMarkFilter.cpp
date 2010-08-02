/*
 *
 * (C) Copyright IBM Corp. 1998 - 2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "GDEFMarkFilter.h"
#include "GlyphDefinitionTables.h"

U_NAMESPACE_BEGIN

GDEFMarkFilter::GDEFMarkFilter(const GlyphDefinitionTableHeader *gdefTable)
{
    classDefTable = gdefTable->getGlyphClassDefinitionTable();
}

GDEFMarkFilter::~GDEFMarkFilter()
{
    // nothing to do?
}

le_bool GDEFMarkFilter::accept(LEGlyphID glyph) const
{
    le_int32 glyphClass = classDefTable->getGlyphClass(glyph);

    return glyphClass == gcdMarkGlyph;
}

U_NAMESPACE_END
