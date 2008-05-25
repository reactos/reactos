/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __LOOKUPTABLES_H
#define __LOOKUPTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LayoutTables.h"

U_NAMESPACE_BEGIN

enum LookupTableFormat
{
    ltfSimpleArray      = 0,
    ltfSegmentSingle    = 2,
    ltfSegmentArray     = 4,
    ltfSingleTable      = 6,
    ltfTrimmedArray     = 8
};

typedef le_int16 LookupValue;

struct LookupTable
{
    le_int16 format;
};

struct LookupSegment
{
    TTGlyphID   lastGlyph;
    TTGlyphID   firstGlyph;
    LookupValue value;
};

struct LookupSingle
{
    TTGlyphID   glyph;
    LookupValue value;
};

struct BinarySearchLookupTable : LookupTable
{
    le_int16 unitSize;
    le_int16 nUnits;
    le_int16 searchRange;
    le_int16 entrySelector;
    le_int16 rangeShift;

    const LookupSegment *lookupSegment(const LookupSegment *segments, LEGlyphID glyph) const;

    const LookupSingle *lookupSingle(const LookupSingle *entries, LEGlyphID glyph) const;
};

struct SimpleArrayLookupTable : LookupTable
{
    LookupValue valueArray[ANY_NUMBER];
};

struct SegmentSingleLookupTable : BinarySearchLookupTable
{
    LookupSegment segments[ANY_NUMBER];
};

struct SegmentArrayLookupTable : BinarySearchLookupTable
{
    LookupSegment segments[ANY_NUMBER];
};

struct SingleTableLookupTable : BinarySearchLookupTable
{
    LookupSingle entries[ANY_NUMBER];
};

struct TrimmedArrayLookupTable : LookupTable
{
    TTGlyphID   firstGlyph;
    TTGlyphID   glyphCount;
    LookupValue valueArray[ANY_NUMBER];
};

U_NAMESPACE_END
#endif
