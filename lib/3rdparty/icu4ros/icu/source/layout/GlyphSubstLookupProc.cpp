/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEGlyphFilter.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "Features.h"
#include "Lookups.h"
#include "ScriptAndLanguage.h"
#include "GlyphDefinitionTables.h"
#include "GlyphSubstitutionTables.h"
#include "SingleSubstitutionSubtables.h"
#include "MultipleSubstSubtables.h"
#include "AlternateSubstSubtables.h"
#include "LigatureSubstSubtables.h"
#include "ContextualSubstSubtables.h"
#include "ExtensionSubtables.h"
#include "LookupProcessor.h"
#include "GlyphSubstLookupProc.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

GlyphSubstitutionLookupProcessor::GlyphSubstitutionLookupProcessor(
        const GlyphSubstitutionTableHeader *glyphSubstitutionTableHeader,
        LETag scriptTag, LETag languageTag, const LEGlyphFilter *filter, const FeatureMap *featureMap, le_int32 featureMapCount, le_bool featureOrder)
    : LookupProcessor(
                      (char *) glyphSubstitutionTableHeader,
                      SWAPW(glyphSubstitutionTableHeader->scriptListOffset),
                      SWAPW(glyphSubstitutionTableHeader->featureListOffset),
                      SWAPW(glyphSubstitutionTableHeader->lookupListOffset),
                      scriptTag, languageTag, featureMap, featureMapCount, featureOrder), fFilter(filter)
{
    // anything?
}

GlyphSubstitutionLookupProcessor::GlyphSubstitutionLookupProcessor()
{
}

le_uint32 GlyphSubstitutionLookupProcessor::applySubtable(const LookupSubtable *lookupSubtable, le_uint16 lookupType,
                                                       GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const
{
    le_uint32 delta = 0;

    switch(lookupType)
    {
    case 0:
        break;

    case gsstSingle:
    {
        const SingleSubstitutionSubtable *subtable = (const SingleSubstitutionSubtable *) lookupSubtable;

        delta = subtable->process(glyphIterator, fFilter);
        break;
    }

    case gsstMultiple:
    {
        const MultipleSubstitutionSubtable *subtable = (const MultipleSubstitutionSubtable *) lookupSubtable;

        delta = subtable->process(glyphIterator, fFilter);
        break;
    }

    case gsstAlternate:
    {
        const AlternateSubstitutionSubtable *subtable = (const AlternateSubstitutionSubtable *) lookupSubtable;

        delta = subtable->process(glyphIterator, fFilter);
        break;
    }

    case gsstLigature:
    {
        const LigatureSubstitutionSubtable *subtable = (const LigatureSubstitutionSubtable *) lookupSubtable;

        delta = subtable->process(glyphIterator, fFilter);
        break;
    }

    case gsstContext:
    {
        const ContextualSubstitutionSubtable *subtable = (const ContextualSubstitutionSubtable *) lookupSubtable;

        delta = subtable->process(this, glyphIterator, fontInstance);
        break;
    }

    case gsstChainingContext:
    {
        const ChainingContextualSubstitutionSubtable *subtable = (const ChainingContextualSubstitutionSubtable *) lookupSubtable;

        delta = subtable->process(this, glyphIterator, fontInstance);
        break;
    }

    case gsstExtension:
    {
        const ExtensionSubtable *subtable = (const ExtensionSubtable *) lookupSubtable;

        delta = subtable->process(this, lookupType, glyphIterator, fontInstance);
        break;
    }

    default:
        break;
    }

    return delta;
}

GlyphSubstitutionLookupProcessor::~GlyphSubstitutionLookupProcessor()
{
}

U_NAMESPACE_END
