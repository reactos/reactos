/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "OpenTypeUtilities.h"
#include "ClassDefinitionTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

le_int32 ClassDefinitionTable::getGlyphClass(LEGlyphID glyphID) const
{
    switch(SWAPW(classFormat)) {
    case 0:
        return 0;

    case 1:
    {
        const ClassDefFormat1Table *f1Table = (const ClassDefFormat1Table *) this;

        return f1Table->getGlyphClass(glyphID);
    }

    case 2:
    {
        const ClassDefFormat2Table *f2Table = (const ClassDefFormat2Table *) this;

        return f2Table->getGlyphClass(glyphID);
    }

    default:
        return 0;
    }
}

le_bool ClassDefinitionTable::hasGlyphClass(le_int32 glyphClass) const
{
    switch(SWAPW(classFormat)) {
    case 0:
        return 0;

    case 1:
    {
        const ClassDefFormat1Table *f1Table = (const ClassDefFormat1Table *) this;

        return f1Table->hasGlyphClass(glyphClass);
    }

    case 2:
    {
        const ClassDefFormat2Table *f2Table = (const ClassDefFormat2Table *) this;

        return f2Table->hasGlyphClass(glyphClass);
    }

    default:
        return 0;
    }
}

le_int32 ClassDefFormat1Table::getGlyphClass(LEGlyphID glyphID) const
{
    TTGlyphID ttGlyphID  = (TTGlyphID) LE_GET_GLYPH(glyphID);
    TTGlyphID firstGlyph = SWAPW(startGlyph);
    TTGlyphID lastGlyph  = firstGlyph + SWAPW(glyphCount);

    if (ttGlyphID > firstGlyph && ttGlyphID < lastGlyph) {
        return SWAPW(classValueArray[ttGlyphID - firstGlyph]);
    }

    return 0;
}

le_bool ClassDefFormat1Table::hasGlyphClass(le_int32 glyphClass) const
{
    le_uint16 count  = SWAPW(glyphCount);
    int i;

    for (i = 0; i < count; i += 1) {
        if (SWAPW(classValueArray[i]) == glyphClass) {
            return TRUE;
        }
    }

    return FALSE;
}

le_int32 ClassDefFormat2Table::getGlyphClass(LEGlyphID glyphID) const
{
    TTGlyphID ttGlyph    = (TTGlyphID) LE_GET_GLYPH(glyphID);
    le_uint16 rangeCount = SWAPW(classRangeCount);
    le_int32  rangeIndex =
        OpenTypeUtilities::getGlyphRangeIndex(ttGlyph, classRangeRecordArray, rangeCount);

    if (rangeIndex < 0) {
        return 0;
    }

    return SWAPW(classRangeRecordArray[rangeIndex].rangeValue);
}

le_bool ClassDefFormat2Table::hasGlyphClass(le_int32 glyphClass) const
{
    le_uint16 rangeCount = SWAPW(classRangeCount);
    int i;

    for (i = 0; i < rangeCount; i += 1) {
        if (SWAPW(classRangeRecordArray[i].rangeValue) == glyphClass) {
            return TRUE;
        }
    }

    return FALSE;
}

U_NAMESPACE_END
