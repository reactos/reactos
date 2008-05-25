/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc.h"
#include "TrimmedArrayProcessor.h"
#include "LEGlyphStorage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(TrimmedArrayProcessor)

TrimmedArrayProcessor::TrimmedArrayProcessor()
{
}

TrimmedArrayProcessor::TrimmedArrayProcessor(const MorphSubtableHeader *morphSubtableHeader)
  : NonContextualGlyphSubstitutionProcessor(morphSubtableHeader)
{
    const NonContextualGlyphSubstitutionHeader *header = (const NonContextualGlyphSubstitutionHeader *) morphSubtableHeader;

    trimmedArrayLookupTable = (const TrimmedArrayLookupTable *) &header->table;
    firstGlyph = SWAPW(trimmedArrayLookupTable->firstGlyph);
    lastGlyph = firstGlyph + SWAPW(trimmedArrayLookupTable->glyphCount);
}

TrimmedArrayProcessor::~TrimmedArrayProcessor()
{
}

void TrimmedArrayProcessor::process(LEGlyphStorage &glyphStorage)
{
    le_int32 glyphCount = glyphStorage.getGlyphCount();
    le_int32 glyph;

    for (glyph = 0; glyph < glyphCount; glyph += 1) {
        LEGlyphID thisGlyph = glyphStorage[glyph];
        TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(thisGlyph);

        if ((ttGlyph > firstGlyph) && (ttGlyph < lastGlyph)) {
            TTGlyphID newGlyph = SWAPW(trimmedArrayLookupTable->valueArray[ttGlyph - firstGlyph]);

            glyphStorage[glyph] = LE_SET_GLYPH(thisGlyph, newGlyph);
        }
    }
} 

U_NAMESPACE_END
