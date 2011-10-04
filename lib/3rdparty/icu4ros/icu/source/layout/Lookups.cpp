/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "Lookups.h"
#include "CoverageTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

const LookupTable *LookupListTable::getLookupTable(le_uint16 lookupTableIndex) const
{
    if (lookupTableIndex >= SWAPW(lookupCount)) {
        return 0;
    }

    Offset lookupTableOffset = lookupTableOffsetArray[lookupTableIndex];

    return (const LookupTable *) ((char *) this + SWAPW(lookupTableOffset));
}

const LookupSubtable *LookupTable::getLookupSubtable(le_uint16 subtableIndex) const
{
    if (subtableIndex >= SWAPW(subTableCount)) {
        return 0;
    }

    Offset subtableOffset = subTableOffsetArray[subtableIndex];

    return (const LookupSubtable *) ((char *) this + SWAPW(subtableOffset));
}

le_int32 LookupSubtable::getGlyphCoverage(Offset tableOffset, LEGlyphID glyphID) const
{
    const CoverageTable *coverageTable = (const CoverageTable *) ((char *) this + SWAPW(tableOffset));

    return coverageTable->getGlyphCoverage(glyphID);
}

U_NAMESPACE_END
