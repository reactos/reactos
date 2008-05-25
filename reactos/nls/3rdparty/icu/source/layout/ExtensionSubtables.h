/*
 * %W% %E%
 *
 * (C) Copyright IBM Corp. 2002-2003 - All Rights Reserved
 *
 */

#ifndef __EXTENSIONSUBTABLES_H
#define __EXTENSIONSUBTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "GlyphSubstitutionTables.h"
#include "LookupProcessor.h"
#include "GlyphIterator.h"

U_NAMESPACE_BEGIN

struct ExtensionSubtable //: GlyphSubstitutionSubtable
{
    le_uint16 substFormat;
    le_uint16 extensionLookupType;
    le_uint32 extensionOffset;

    le_uint32 process(const LookupProcessor *lookupProcessor, le_uint16 lookupType,
                      GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;
};

U_NAMESPACE_END
#endif
