/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __MARKTOBASEPOSITIONINGSUBTABLES_H
#define __MARKTOBASEPOSITIONINGSUBTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphPositioningTables.h"
#include "AttachmentPosnSubtables.h"
#include "GlyphIterator.h"

U_NAMESPACE_BEGIN

struct MarkToBasePositioningSubtable : AttachmentPositioningSubtable
{
    le_int32   process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;
    LEGlyphID  findBaseGlyph(GlyphIterator *glyphIterator) const;
};

struct BaseRecord
{
    Offset baseAnchorTableOffsetArray[ANY_NUMBER];
};

struct BaseArray
{
    le_int16 baseRecordCount;
    BaseRecord baseRecordArray[ANY_NUMBER];
};

U_NAMESPACE_END
#endif

