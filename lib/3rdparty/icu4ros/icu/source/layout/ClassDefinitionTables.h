/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __CLASSDEFINITIONTABLES_H
#define __CLASSDEFINITIONTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"

U_NAMESPACE_BEGIN

struct ClassDefinitionTable
{
    le_uint16 classFormat;

    le_int32  getGlyphClass(LEGlyphID glyphID) const;
    le_bool   hasGlyphClass(le_int32 glyphClass) const;
};

struct ClassDefFormat1Table : ClassDefinitionTable
{
    TTGlyphID  startGlyph;
    le_uint16  glyphCount;
    le_uint16  classValueArray[ANY_NUMBER];

    le_int32 getGlyphClass(LEGlyphID glyphID) const;
    le_bool  hasGlyphClass(le_int32 glyphClass) const;
};

struct ClassRangeRecord
{
    TTGlyphID start;
    TTGlyphID end;
    le_uint16 classValue;
};

struct ClassDefFormat2Table : ClassDefinitionTable
{
    le_uint16        classRangeCount;
    GlyphRangeRecord classRangeRecordArray[ANY_NUMBER];

    le_int32 getGlyphClass(LEGlyphID glyphID) const;
    le_bool hasGlyphClass(le_int32 glyphClass) const;
};

U_NAMESPACE_END
#endif
