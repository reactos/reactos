/*
 * %W% %E%
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __LOOKUPPROCESSOR_H
#define __LOOKUPPROCESSOR_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
//#include "Lookups.h"
//#include "Features.h"

U_NAMESPACE_BEGIN

class  LEFontInstance;
class  LEGlyphStorage;
class  GlyphIterator;
class  GlyphPositionAdjustments;
struct FeatureTable;
struct FeatureListTable;
struct GlyphDefinitionTableHeader;
struct LookupSubtable;
struct LookupTable;

class LookupProcessor : public UMemory {
public:
    le_int32 process(LEGlyphStorage &glyphStorage, GlyphPositionAdjustments *glyphPositionAdjustments,
                 le_bool rightToLeft, const GlyphDefinitionTableHeader *glyphDefinitionTableHeader, const LEFontInstance *fontInstance) const;

    le_uint32 applyLookupTable(const LookupTable *lookupTable, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;

    le_uint32 applySingleLookup(le_uint16 lookupTableIndex, GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const;

    virtual le_uint32 applySubtable(const LookupSubtable *lookupSubtable, le_uint16 subtableType,
        GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const = 0;

    virtual ~LookupProcessor();

protected:
     LookupProcessor(const char *baseAddress,
        Offset scriptListOffset, Offset featureListOffset, Offset lookupListOffset,
        LETag scriptTag, LETag languageTag, const FeatureMap *featureMap, le_int32 featureMapCount, le_bool orderFeatures);

   LookupProcessor();

    le_int32 selectLookups(const FeatureTable *featureTable, FeatureMask featureMask, le_int32 order);

    const LookupListTable   *lookupListTable;
    const FeatureListTable  *featureListTable;

    FeatureMask            *lookupSelectArray;

    le_uint16               *lookupOrderArray;
    le_uint32               lookupOrderCount;

private:

    LookupProcessor(const LookupProcessor &other); // forbid copying of this class
    LookupProcessor &operator=(const LookupProcessor &other); // forbid copying of this class
};

U_NAMESPACE_END
#endif
