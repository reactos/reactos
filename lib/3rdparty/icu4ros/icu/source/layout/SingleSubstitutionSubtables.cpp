/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "SingleSubstitutionSubtables.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

le_uint32 SingleSubstitutionSubtable::process(GlyphIterator *glyphIterator, const LEGlyphFilter *filter) const
{
    switch(SWAPW(subtableFormat))
    {
    case 0:
        return 0;

    case 1:
    {
        const SingleSubstitutionFormat1Subtable *subtable = (const SingleSubstitutionFormat1Subtable *) this;

        return subtable->process(glyphIterator, filter);
    }

    case 2:
    {
        const SingleSubstitutionFormat2Subtable *subtable = (const SingleSubstitutionFormat2Subtable *) this;

        return subtable->process(glyphIterator, filter);
    }

    default:
        return 0;
    }
}

le_uint32 SingleSubstitutionFormat1Subtable::process(GlyphIterator *glyphIterator, const LEGlyphFilter *filter) const
{
    LEGlyphID glyph = glyphIterator->getCurrGlyphID();
    le_int32 coverageIndex = getGlyphCoverage(glyph);

    if (coverageIndex >= 0) {
        TTGlyphID substitute = ((TTGlyphID) LE_GET_GLYPH(glyph)) + SWAPW(deltaGlyphID);

        if (filter == NULL || filter->accept(LE_SET_GLYPH(glyph, substitute))) {
            glyphIterator->setCurrGlyphID(substitute);
        }

        return 1;
    }

    return 0;
}

le_uint32 SingleSubstitutionFormat2Subtable::process(GlyphIterator *glyphIterator, const LEGlyphFilter *filter) const
{
    LEGlyphID glyph = glyphIterator->getCurrGlyphID();
    le_int32 coverageIndex = getGlyphCoverage(glyph);

    if (coverageIndex >= 0) {
        TTGlyphID substitute = SWAPW(substituteArray[coverageIndex]);

        if (filter == NULL || filter->accept(LE_SET_GLYPH(glyph, substitute))) {
            glyphIterator->setCurrGlyphID(substitute);
        }

        return 1;
    }

    return 0;
}

U_NAMESPACE_END
