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
#include "SimpleArrayProcessor.h"
#include "LEGlyphStorage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SimpleArrayProcessor)

SimpleArrayProcessor::SimpleArrayProcessor()
{
}

SimpleArrayProcessor::SimpleArrayProcessor(const MorphSubtableHeader *morphSubtableHeader)
  : NonContextualGlyphSubstitutionProcessor(morphSubtableHeader)
{
    const NonContextualGlyphSubstitutionHeader *header = (const NonContextualGlyphSubstitutionHeader *) morphSubtableHeader;

    simpleArrayLookupTable = (const SimpleArrayLookupTable *) &header->table;
}

SimpleArrayProcessor::~SimpleArrayProcessor()
{
}

void SimpleArrayProcessor::process(LEGlyphStorage &glyphStorage)
{
    le_int32 glyphCount = glyphStorage.getGlyphCount();
    le_int32 glyph;

    for (glyph = 0; glyph < glyphCount; glyph += 1) {
        LEGlyphID thisGlyph = glyphStorage[glyph];
        if (LE_GET_GLYPH(thisGlyph) < 0xFFFF) {
            TTGlyphID newGlyph = SWAPW(simpleArrayLookupTable->valueArray[LE_GET_GLYPH(thisGlyph)]);

            glyphStorage[glyph] = LE_SET_GLYPH(thisGlyph, newGlyph);
        }
    }
}
 
U_NAMESPACE_END
