/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "ScriptAndLanguage.h"
#include "GlyphLookupTables.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

le_bool GlyphLookupTableHeader::coversScript(LETag scriptTag) const
{
    const ScriptListTable *scriptListTable = (const ScriptListTable *) ((char *)this + SWAPW(scriptListOffset));

    return scriptListOffset != 0 && scriptListTable->findScript(scriptTag) != NULL;
}

le_bool GlyphLookupTableHeader::coversScriptAndLanguage(LETag scriptTag, LETag languageTag, le_bool exactMatch) const
{
    const ScriptListTable *scriptListTable = (const ScriptListTable *) ((char *)this + SWAPW(scriptListOffset));
    const LangSysTable    *langSysTable    = scriptListTable->findLanguage(scriptTag, languageTag, exactMatch);

    // FIXME: could check featureListOffset, lookupListOffset, and lookup count...
    // Note: don't have to SWAPW langSysTable->featureCount to check for non-zero.
    return langSysTable != NULL && langSysTable->featureCount != 0;
}

U_NAMESPACE_END
