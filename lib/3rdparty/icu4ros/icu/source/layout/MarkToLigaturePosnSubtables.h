/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __MARKTOLIGATUREPOSITIONINGSUBTABLES_H
#define __MARKTOLIGATUREPOSITIONINGSUBTABLES_H

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

struct MarkToLigaturePositioningSubtable : AttachmentPositioningSubtable
{
    le_int32   process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;
    LEGlyphID  findLigatureGlyph(GlyphIterator *glyphIterator) const;
};

struct ComponentRecord
{
    Offset ligatureAnchorTableOffsetArray[ANY_NUMBER];
};

struct LigatureAttachTable
{
    le_uint16 componentCount;
    ComponentRecord componentRecordArray[ANY_NUMBER];
};

struct LigatureArray
{
    le_uint16 ligatureCount;
    Offset ligatureAttachTableOffsetArray[ANY_NUMBER];
};

U_NAMESPACE_END
#endif

