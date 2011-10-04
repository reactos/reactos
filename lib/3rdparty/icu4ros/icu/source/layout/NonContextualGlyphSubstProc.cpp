/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "NonContextualGlyphSubst.h"
#include "NonContextualGlyphSubstProc.h"
#include "SimpleArrayProcessor.h"
#include "SegmentSingleProcessor.h"
#include "SegmentArrayProcessor.h"
#include "SingleTableProcessor.h"
#include "TrimmedArrayProcessor.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

NonContextualGlyphSubstitutionProcessor::NonContextualGlyphSubstitutionProcessor()
{
}

NonContextualGlyphSubstitutionProcessor::NonContextualGlyphSubstitutionProcessor(const MorphSubtableHeader *morphSubtableHeader)
    : SubtableProcessor(morphSubtableHeader)
{
}

NonContextualGlyphSubstitutionProcessor::~NonContextualGlyphSubstitutionProcessor()
{
}

SubtableProcessor *NonContextualGlyphSubstitutionProcessor::createInstance(const MorphSubtableHeader *morphSubtableHeader)
{
    const NonContextualGlyphSubstitutionHeader *header = (const NonContextualGlyphSubstitutionHeader *) morphSubtableHeader;

    switch (SWAPW(header->table.format))
    {
    case ltfSimpleArray:
        return new SimpleArrayProcessor(morphSubtableHeader);

    case ltfSegmentSingle:
        return new SegmentSingleProcessor(morphSubtableHeader);

    case ltfSegmentArray:
        return new SegmentArrayProcessor(morphSubtableHeader);

    case ltfSingleTable:
        return new SingleTableProcessor(morphSubtableHeader);

    case ltfTrimmedArray:
        return new TrimmedArrayProcessor(morphSubtableHeader);

    default:
        return NULL;
    }
}

U_NAMESPACE_END
