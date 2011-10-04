/*
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __GLYPHPOSITIONINGTABLES_H
#define __GLYPHPOSITIONINGTABLES_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "Lookups.h"
#include "GlyphLookupTables.h"

U_NAMESPACE_BEGIN

class  LEFontInstance;
class  LEGlyphStorage;
class  LEGlyphFilter;
class  GlyphPositionAdjustments;
struct GlyphDefinitionTableHeader;

struct GlyphPositioningTableHeader : public GlyphLookupTableHeader
{
    void    process(LEGlyphStorage &glyphStorage, GlyphPositionAdjustments *glyphPositionAdjustments,
                le_bool rightToLeft, LETag scriptTag, LETag languageTag,
                const GlyphDefinitionTableHeader *glyphDefinitionTableHeader,
                const LEFontInstance *fontInstance, const FeatureMap *featureMap, le_int32 featureMapCount, le_bool featureOrder) const;
};

enum GlyphPositioningSubtableTypes
{
    gpstSingle          = 1,
    gpstPair            = 2,
    gpstCursive         = 3,
    gpstMarkToBase      = 4,
    gpstMarkToLigature  = 5,
    gpstMarkToMark      = 6,
    gpstContext         = 7,
    gpstChainedContext  = 8,
    gpstExtension       = 9
};

typedef LookupSubtable GlyphPositioningSubtable;

U_NAMESPACE_END
#endif
