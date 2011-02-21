/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "StateTables.h"
#include "MorphStateTables.h"
#include "SubtableProcessor.h"
#include "StateTableProcessor.h"
#include "ContextualGlyphSubstProc.h"
#include "LEGlyphStorage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ContextualGlyphSubstitutionProcessor)

ContextualGlyphSubstitutionProcessor::ContextualGlyphSubstitutionProcessor(const MorphSubtableHeader *morphSubtableHeader)
  : StateTableProcessor(morphSubtableHeader)
{
    contextualGlyphSubstitutionHeader = (const ContextualGlyphSubstitutionHeader *) morphSubtableHeader;
    substitutionTableOffset = SWAPW(contextualGlyphSubstitutionHeader->substitutionTableOffset);

    entryTable = (const ContextualGlyphSubstitutionStateEntry *) ((char *) &stateTableHeader->stHeader + entryTableOffset);
}

ContextualGlyphSubstitutionProcessor::~ContextualGlyphSubstitutionProcessor()
{
}

void ContextualGlyphSubstitutionProcessor::beginStateTable()
{
    markGlyph = 0;
}

ByteOffset ContextualGlyphSubstitutionProcessor::processStateEntry(LEGlyphStorage &glyphStorage, le_int32 &currGlyph, EntryTableIndex index)
{
    const ContextualGlyphSubstitutionStateEntry *entry = &entryTable[index];
    ByteOffset newState = SWAPW(entry->newStateOffset);
    le_int16 flags = SWAPW(entry->flags);
    WordOffset markOffset = SWAPW(entry->markOffset);
    WordOffset currOffset = SWAPW(entry->currOffset);

    if (markOffset != 0) {
        const le_int16 *table = (const le_int16 *) ((char *) &stateTableHeader->stHeader + markOffset * 2);
        LEGlyphID mGlyph = glyphStorage[markGlyph];
        TTGlyphID newGlyph = SWAPW(table[LE_GET_GLYPH(mGlyph)]);

         glyphStorage[markGlyph] = LE_SET_GLYPH(mGlyph, newGlyph);
    }

    if (currOffset != 0) {
        const le_int16 *table = (const le_int16 *) ((char *) &stateTableHeader->stHeader + currOffset * 2);
        LEGlyphID thisGlyph = glyphStorage[currGlyph];
        TTGlyphID newGlyph = SWAPW(table[LE_GET_GLYPH(thisGlyph)]);

        glyphStorage[currGlyph] = LE_SET_GLYPH(thisGlyph, newGlyph);
    }

    if (flags & cgsSetMark) {
        markGlyph = currGlyph;
    }

    if (!(flags & cgsDontAdvance)) {
        // should handle reverse too!
        currGlyph += 1;
    }

    return newState;
}

void ContextualGlyphSubstitutionProcessor::endStateTable()
{
}

U_NAMESPACE_END
