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
#include "SegmentSingleProcessor.h"
#include "LEGlyphStorage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SegmentSingleProcessor)

SegmentSingleProcessor::SegmentSingleProcessor()
{
}

SegmentSingleProcessor::SegmentSingleProcessor(const MorphSubtableHeader *morphSubtableHeader)
  : NonContextualGlyphSubstitutionProcessor(morphSubtableHeader)
{
    const NonContextualGlyphSubstitutionHeader *header = (const NonContextualGlyphSubstitutionHeader *) morphSubtableHeader;

    segmentSingleLookupTable = (const SegmentSingleLookupTable *) &header->table;
}

SegmentSingleProcessor::~SegmentSingleProcessor()
{
}

void SegmentSingleProcessor::process(LEGlyphStorage &glyphStorage)
{
    const LookupSegment *segments = segmentSingleLookupTable->segments;
    le_int32 glyphCount = glyphStorage.getGlyphCount();
    le_int32 glyph;

    for (glyph = 0; glyph < glyphCount; glyph += 1) {
        LEGlyphID thisGlyph = glyphStorage[glyph];
        const LookupSegment *lookupSegment = segmentSingleLookupTable->lookupSegment(segments, thisGlyph);

        if (lookupSegment != NULL) {
            TTGlyphID   newGlyph  = (TTGlyphID) LE_GET_GLYPH(thisGlyph) + SWAPW(lookupSegment->value);

            glyphStorage[glyph] = LE_SET_GLYPH(thisGlyph, newGlyph);
        }
    }
}

U_NAMESPACE_END
