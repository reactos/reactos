/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __SUBSTITUTIONLOOKUPS_H
#define __SUBSTITUTIONLOOKUPS_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "GlyphIterator.h"
#include "LookupProcessor.h"

U_NAMESPACE_BEGIN

struct SubstitutionLookupRecord
{
    le_uint16  sequenceIndex;
    le_uint16  lookupListIndex;
};

struct SubstitutionLookup
{
    static void applySubstitutionLookups(
        LookupProcessor *lookupProcessor, 
        SubstitutionLookupRecord *substLookupRecordArray,
        le_uint16 substCount,
        GlyphIterator *glyphIterator,
        const LEFontInstance *fontInstance,
        le_int32 position);
};

U_NAMESPACE_END
#endif

