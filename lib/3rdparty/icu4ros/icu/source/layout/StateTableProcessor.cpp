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
#include "LEGlyphStorage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

StateTableProcessor::StateTableProcessor()
{
}

StateTableProcessor::StateTableProcessor(const MorphSubtableHeader *morphSubtableHeader)
  : SubtableProcessor(morphSubtableHeader)
{
    stateTableHeader = (const MorphStateTableHeader *) morphSubtableHeader;

    stateSize = SWAPW(stateTableHeader->stHeader.stateSize);
    classTableOffset = SWAPW(stateTableHeader->stHeader.classTableOffset);
    stateArrayOffset = SWAPW(stateTableHeader->stHeader.stateArrayOffset);
    entryTableOffset = SWAPW(stateTableHeader->stHeader.entryTableOffset);

    classTable = (const ClassTable *) ((char *) &stateTableHeader->stHeader + classTableOffset);
    firstGlyph = SWAPW(classTable->firstGlyph);
    lastGlyph  = firstGlyph + SWAPW(classTable->nGlyphs);
}

StateTableProcessor::~StateTableProcessor()
{
}

void StateTableProcessor::process(LEGlyphStorage &glyphStorage)
{
    // Start at state 0
    // XXX: How do we know when to start at state 1?
    ByteOffset currentState = stateArrayOffset;

    // XXX: reverse? 
    le_int32 currGlyph = 0;
    le_int32 glyphCount = glyphStorage.getGlyphCount();

    beginStateTable();

    while (currGlyph <= glyphCount) {
        ClassCode classCode = classCodeOOB;
        if (currGlyph == glyphCount) {
            // XXX: How do we handle EOT vs. EOL?
            classCode = classCodeEOT;
        } else {
            TTGlyphID glyphCode = (TTGlyphID) LE_GET_GLYPH(glyphStorage[currGlyph]);

            if (glyphCode == 0xFFFF) {
                classCode = classCodeDEL;
            } else if ((glyphCode >= firstGlyph) && (glyphCode < lastGlyph)) {
                classCode = classTable->classArray[glyphCode - firstGlyph];
            }
        }

        const EntryTableIndex *stateArray = (const EntryTableIndex *) ((char *) &stateTableHeader->stHeader + currentState);
        EntryTableIndex entryTableIndex = stateArray[(le_uint8)classCode];

        currentState = processStateEntry(glyphStorage, currGlyph, entryTableIndex);
    }

    endStateTable();
}

U_NAMESPACE_END
