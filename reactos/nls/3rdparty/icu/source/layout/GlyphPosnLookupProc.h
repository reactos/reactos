/*
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __GLYPHPOSITIONINGLOOKUPPROCESSOR_H
#define __GLYPHPOSITIONINGLOOKUPPROCESSOR_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "Lookups.h"
#include "Features.h"
#include "GlyphDefinitionTables.h"
#include "GlyphPositioningTables.h"
#include "GlyphIterator.h"
#include "LookupProcessor.h"

U_NAMESPACE_BEGIN

class GlyphPositioningLookupProcessor : public LookupProcessor
{
public:
    GlyphPositioningLookupProcessor(const GlyphPositioningTableHeader *glyphPositioningTableHeader,
        LETag scriptTag, LETag languageTag, const FeatureMap *featureMap, le_int32 featureMapCount, le_bool featureOrder);

    virtual ~GlyphPositioningLookupProcessor();

    virtual le_uint32 applySubtable(const LookupSubtable *lookupSubtable, le_uint16 lookupType, GlyphIterator *glyphIterator,
        const LEFontInstance *fontInstance) const;

protected:
    GlyphPositioningLookupProcessor();

private:

    GlyphPositioningLookupProcessor(const GlyphPositioningLookupProcessor &other); // forbid copying of this class
    GlyphPositioningLookupProcessor &operator=(const GlyphPositioningLookupProcessor &other); // forbid copying of this class
};

U_NAMESPACE_END
#endif
