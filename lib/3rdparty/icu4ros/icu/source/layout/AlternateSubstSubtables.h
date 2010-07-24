/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __ALTERNATESUBSTITUTIONSUBTABLES_H
#define __ALTERNATESUBSTITUTIONSUBTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "GlyphIterator.h"

U_NAMESPACE_BEGIN

struct AlternateSetTable
{
    le_uint16 glyphCount;
    TTGlyphID alternateArray[ANY_NUMBER];
};

struct AlternateSubstitutionSubtable : GlyphSubstitutionSubtable
{
    le_uint16 alternateSetCount;
    Offset    alternateSetTableOffsetArray[ANY_NUMBER];

    le_uint32 process(GlyphIterator *glyphIterator, const LEGlyphFilter *filter = NULL) const;
};

U_NAMESPACE_END
#endif
