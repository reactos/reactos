/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __LOOKUPS_H
#define __LOOKUPS_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

enum LookupFlags
{
    lfBaselineIsLogicalEnd  = 0x0001,  // The MS spec. calls this flag "RightToLeft" but this name is more accurate 
    lfIgnoreBaseGlyphs      = 0x0002,
    lfIgnoreLigatures       = 0x0004,
    lfIgnoreMarks           = 0x0008,
    lfReservedMask          = 0x00F0,
    lfMarkAttachTypeMask    = 0xFF00,
    lfMarkAttachTypeShift   = 8
};

struct LookupSubtable
{
    le_uint16 subtableFormat;
    Offset    coverageTableOffset;

    inline le_int32  getGlyphCoverage(LEGlyphID glyphID) const;

    le_int32  getGlyphCoverage(Offset tableOffset, LEGlyphID glyphID) const;
};

struct LookupTable
{
    le_uint16       lookupType;
    le_uint16       lookupFlags;
    le_uint16       subTableCount;
    Offset          subTableOffsetArray[ANY_NUMBER];

    const LookupSubtable  *getLookupSubtable(le_uint16 subtableIndex) const;
};

struct LookupListTable
{
    le_uint16   lookupCount;
    Offset      lookupTableOffsetArray[ANY_NUMBER];

    const LookupTable *getLookupTable(le_uint16 lookupTableIndex) const;
};

inline le_int32 LookupSubtable::getGlyphCoverage(LEGlyphID glyphID) const
{
    return getGlyphCoverage(coverageTableOffset, glyphID);
}

U_NAMESPACE_END
#endif
