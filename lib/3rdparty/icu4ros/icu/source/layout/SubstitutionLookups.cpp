/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "GlyphIterator.h"
#include "LookupProcessor.h"
#include "SubstitutionLookups.h"
#include "CoverageTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

/*
    NOTE: This could be optimized somewhat by keeping track
    of the previous sequenceIndex in the loop and doing next()
    or prev() of the delta between that and the current
    sequenceIndex instead of always resetting to the front.
*/
void SubstitutionLookup::applySubstitutionLookups(
        LookupProcessor *lookupProcessor,
        SubstitutionLookupRecord *substLookupRecordArray,
        le_uint16 substCount,
        GlyphIterator *glyphIterator,
        const LEFontInstance *fontInstance,
        le_int32 position)
{
    GlyphIterator tempIterator(*glyphIterator);

    for (le_uint16 subst = 0; subst < substCount; subst += 1) {
        le_uint16 sequenceIndex = SWAPW(substLookupRecordArray[subst].sequenceIndex);
        le_uint16 lookupListIndex = SWAPW(substLookupRecordArray[subst].lookupListIndex);

        tempIterator.setCurrStreamPosition(position);
        tempIterator.next(sequenceIndex);

        lookupProcessor->applySingleLookup(lookupListIndex, &tempIterator, fontInstance);
    }
}

U_NAMESPACE_END
