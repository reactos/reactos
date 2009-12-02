/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "AnchorTables.h"
#include "MarkArrays.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

le_int32 MarkArray::getMarkClass(LEGlyphID glyphID, le_int32 coverageIndex, const LEFontInstance *fontInstance,
                              LEPoint &anchor) const
{
    le_int32 markClass = -1;

    if (coverageIndex >= 0) {
        le_uint16 mCount = SWAPW(markCount);

        if (coverageIndex < mCount) {
            const MarkRecord *markRecord = &markRecordArray[coverageIndex];
            Offset anchorTableOffset = SWAPW(markRecord->markAnchorTableOffset);
            const AnchorTable *anchorTable = (AnchorTable *) ((char *) this + anchorTableOffset);

            anchorTable->getAnchor(glyphID, fontInstance, anchor);
            markClass = SWAPW(markRecord->markClass);
        }

        // XXXX If we get here, the table is mal-formed
    }

    return markClass;
}

U_NAMESPACE_END
