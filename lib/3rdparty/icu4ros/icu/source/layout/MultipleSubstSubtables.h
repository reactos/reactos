/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __MULTIPLESUBSTITUTIONSUBTABLES_H
#define __MULTIPLESUBSTITUTIONSUBTABLES_H

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

struct SequenceTable
{
    le_uint16 glyphCount;
    TTGlyphID substituteArray[ANY_NUMBER];
};

struct MultipleSubstitutionSubtable : GlyphSubstitutionSubtable
{
    le_uint16 sequenceCount;
    Offset    sequenceTableOffsetArray[ANY_NUMBER];

    le_uint32 process(GlyphIterator *glyphIterator, const LEGlyphFilter *filter = NULL) const;
};

U_NAMESPACE_END
#endif
