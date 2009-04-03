/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphPositioningTables.h"
#include "SinglePositioningSubtables.h"
#include "ValueRecords.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

le_uint32 SinglePositioningSubtable::process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const
{
    switch(SWAPW(subtableFormat))
    {
    case 0:
        return 0;

    case 1:
    {
        const SinglePositioningFormat1Subtable *subtable = (const SinglePositioningFormat1Subtable *) this;

        return subtable->process(glyphIterator, fontInstance);
    }

    case 2:
    {
        const SinglePositioningFormat2Subtable *subtable = (const SinglePositioningFormat2Subtable *) this;

        return subtable->process(glyphIterator, fontInstance);
    }

    default:
        return 0;
    }
}

le_uint32 SinglePositioningFormat1Subtable::process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const
{
    LEGlyphID glyph = glyphIterator->getCurrGlyphID();
    le_int32 coverageIndex = getGlyphCoverage(glyph);

    if (coverageIndex >= 0) {
        valueRecord.adjustPosition(SWAPW(valueFormat), (const char *) this, *glyphIterator, fontInstance);

        return 1;
    }

    return 0;
}

le_uint32 SinglePositioningFormat2Subtable::process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const
{
    LEGlyphID glyph = glyphIterator->getCurrGlyphID();
    le_int16 coverageIndex = (le_int16) getGlyphCoverage(glyph);

    if (coverageIndex >= 0) {
        valueRecordArray[0].adjustPosition(coverageIndex, SWAPW(valueFormat), (const char *) this, *glyphIterator, fontInstance);

        return 1;
    }

    return 0;
}

U_NAMESPACE_END
