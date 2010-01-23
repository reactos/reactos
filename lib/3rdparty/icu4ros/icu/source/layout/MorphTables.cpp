/*
 * %W% %W%
 *
 * (C) Copyright IBM Corp. 1998 - 2004 - All Rights Reserved
 *
 */


#include "LETypes.h"
#include "LayoutTables.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "IndicRearrangementProcessor.h"
#include "ContextualGlyphSubstProc.h"
#include "LigatureSubstProc.h"
#include "NonContextualGlyphSubstProc.h"
//#include "ContextualGlyphInsertionProcessor.h"
#include "LEGlyphStorage.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

void MorphTableHeader::process(LEGlyphStorage &glyphStorage) const
{
    const ChainHeader *chainHeader = chains;
    le_uint32 chainCount = SWAPL(this->nChains);
    le_uint32 chain;

    for (chain = 0; chain < chainCount; chain += 1) {
        FeatureFlags defaultFlags = SWAPL(chainHeader->defaultFlags);
        le_uint32 chainLength = SWAPL(chainHeader->chainLength);
        le_int16 nFeatureEntries = SWAPW(chainHeader->nFeatureEntries);
        le_int16 nSubtables = SWAPW(chainHeader->nSubtables);
        const MorphSubtableHeader *subtableHeader =
            (const MorphSubtableHeader *)&chainHeader->featureTable[nFeatureEntries];
        le_int16 subtable;

        for (subtable = 0; subtable < nSubtables; subtable += 1) {
            le_int16 length = SWAPW(subtableHeader->length);
            SubtableCoverage coverage = SWAPW(subtableHeader->coverage);
            FeatureFlags subtableFeatures = SWAPL(subtableHeader->subtableFeatures);

            // should check coverage more carefully...
            if ((coverage & scfVertical) == 0 && (subtableFeatures & defaultFlags) != 0) {
                subtableHeader->process(glyphStorage);
            }

            subtableHeader = (const MorphSubtableHeader *) ((char *)subtableHeader + length);
        }

        chainHeader = (const ChainHeader *)((char *)chainHeader + chainLength);
    }
}

void MorphSubtableHeader::process(LEGlyphStorage &glyphStorage) const
{
    SubtableProcessor *processor = NULL;

    switch (SWAPW(coverage) & scfTypeMask)
    {
    case mstIndicRearrangement:
        processor = new IndicRearrangementProcessor(this);
        break;

    case mstContextualGlyphSubstitution:
        processor = new ContextualGlyphSubstitutionProcessor(this);
        break;

    case mstLigatureSubstitution:
        processor = new LigatureSubstitutionProcessor(this);
        break;

    case mstReservedUnused:
        break;

    case mstNonContextualGlyphSubstitution:
        processor = NonContextualGlyphSubstitutionProcessor::createInstance(this);
        break;

    /*
    case mstContextualGlyphInsertion:
        processor = new ContextualGlyphInsertionProcessor(this);
        break;
    */

    default:
        break;
    }

    if (processor != NULL) {
        processor->process(glyphStorage);
        delete processor;
    }
}

U_NAMESPACE_END
