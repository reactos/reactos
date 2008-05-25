/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "MorphTables.h"
#include "SubtableProcessor.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

SubtableProcessor::SubtableProcessor()
{
}

SubtableProcessor::SubtableProcessor(const MorphSubtableHeader *morphSubtableHeader)
{
    subtableHeader = morphSubtableHeader;

    length = SWAPW(subtableHeader->length);
    coverage = SWAPW(subtableHeader->coverage);
    subtableFeatures = SWAPL(subtableHeader->subtableFeatures);
}

SubtableProcessor::~SubtableProcessor()
{
}

U_NAMESPACE_END
