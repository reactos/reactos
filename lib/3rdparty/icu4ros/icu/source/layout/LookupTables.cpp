/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LayoutTables.h"
#include "LookupTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

/*
    These are the rolled-up versions of the uniform binary search.
    Someday, if we need more performance, we can un-roll them.
    
    Note: I put these in the base class, so they only have to
    be written once. Since the base class doesn't define the
    segment table, these routines assume that it's right after
    the binary search header.

    Another way to do this is to put each of these routines in one
    of the derived classes, and implement it in the others by casting
    the "this" pointer to the type that has the implementation.
*/ 
const LookupSegment *BinarySearchLookupTable::lookupSegment(const LookupSegment *segments, LEGlyphID glyph) const
{
    le_int16  unity = SWAPW(unitSize);
    le_int16  probe = SWAPW(searchRange);
    le_int16  extra = SWAPW(rangeShift);
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);
    const LookupSegment *entry = segments;
    const LookupSegment *trial = (const LookupSegment *) ((char *) entry + extra);

    if (SWAPW(trial->lastGlyph) <= ttGlyph) {
        entry = trial;
    }

    while (probe > unity) {
        probe >>= 1;
        trial = (const LookupSegment *) ((char *) entry + probe);

        if (SWAPW(trial->lastGlyph) <= ttGlyph) {
            entry = trial;
        }
    }

    if (SWAPW(entry->firstGlyph) <= ttGlyph) {
        return entry;
    }

    return NULL;
}

const LookupSingle *BinarySearchLookupTable::lookupSingle(const LookupSingle *entries, LEGlyphID glyph) const
{
    le_int16  unity = SWAPW(unitSize);
    le_int16  probe = SWAPW(searchRange);
    le_int16  extra = SWAPW(rangeShift);
    TTGlyphID ttGlyph = (TTGlyphID) LE_GET_GLYPH(glyph);
    const LookupSingle *entry = entries;
    const LookupSingle *trial = (const LookupSingle *) ((char *) entry + extra);

    if (SWAPW(trial->glyph) <= ttGlyph) {
        entry = trial;
    }

    while (probe > unity) {
        probe >>= 1;
        trial = (const LookupSingle *) ((char *) entry + probe);

        if (SWAPW(trial->glyph) <= ttGlyph) {
            entry = trial;
        }
    }

    if (SWAPW(entry->glyph) == ttGlyph) {
        return entry;
    }

    return NULL;
}

U_NAMESPACE_END
