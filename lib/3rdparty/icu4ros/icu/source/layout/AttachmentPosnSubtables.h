/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __ATTACHMENTPOSITIONINGSUBTABLES_H
#define __ATTACHMENTPOSITIONINGSUBTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "GlyphPositioningTables.h"
#include "ValueRecords.h"
#include "GlyphIterator.h"

U_NAMESPACE_BEGIN

struct AttachmentPositioningSubtable : GlyphPositioningSubtable
{
    Offset    baseCoverageTableOffset;
    le_uint16 classCount;
    Offset    markArrayOffset;
    Offset    baseArrayOffset;

    inline le_int32  getBaseCoverage(LEGlyphID baseGlyphId) const;

    le_uint32 process(GlyphIterator *glyphIterator) const;
};

inline le_int32 AttachmentPositioningSubtable::getBaseCoverage(LEGlyphID baseGlyphID) const
{
    return getGlyphCoverage(baseCoverageTableOffset, baseGlyphID);
}

U_NAMESPACE_END
#endif

