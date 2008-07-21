/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __SINGLEPOSITIONINGSUBTABLES_H
#define __SINGLEPOSITIONINGSUBTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphPositioningTables.h"
#include "ValueRecords.h"
#include "GlyphIterator.h"

U_NAMESPACE_BEGIN

struct SinglePositioningSubtable : GlyphPositioningSubtable
{
    le_uint32  process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;
};

struct SinglePositioningFormat1Subtable : SinglePositioningSubtable
{
    ValueFormat valueFormat;
    ValueRecord valueRecord;

    le_uint32  process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;
};

struct SinglePositioningFormat2Subtable : SinglePositioningSubtable
{
    ValueFormat valueFormat;
    le_uint16   valueCount;
    ValueRecord valueRecordArray[ANY_NUMBER];

    le_uint32  process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;
};

U_NAMESPACE_END
#endif


