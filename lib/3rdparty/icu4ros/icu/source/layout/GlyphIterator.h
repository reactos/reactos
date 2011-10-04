/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#ifndef __GLYPHITERATOR_H
#define __GLYPHITERATOR_H

/**
 * \file
 * \internal
 */

#include "LETypes.h"
#include "OpenTypeTables.h"
#include "GlyphDefinitionTables.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;
class GlyphPositionAdjustments;

class GlyphIterator : public UMemory {
public:
    GlyphIterator(LEGlyphStorage &theGlyphStorage, GlyphPositionAdjustments *theGlyphPositionAdjustments, le_bool rightToLeft, le_uint16 theLookupFlags,
        FeatureMask theFeatureMask, const GlyphDefinitionTableHeader *theGlyphDefinitionTableHeader);

    GlyphIterator(GlyphIterator &that);

    GlyphIterator(GlyphIterator &that, FeatureMask newFeatureMask);

    GlyphIterator(GlyphIterator &that, le_uint16 newLookupFlags);

    virtual ~GlyphIterator();

    void reset(le_uint16 newLookupFlags, LETag newFeatureTag);

    le_bool next(le_uint32 delta = 1);
    le_bool prev(le_uint32 delta = 1);
    le_bool findFeatureTag();

    le_bool isRightToLeft() const;
    le_bool ignoresMarks() const;

    le_bool baselineIsLogicalEnd() const;

    LEGlyphID getCurrGlyphID() const;
    le_int32  getCurrStreamPosition() const;

    le_int32  getMarkComponent(le_int32 markPosition) const;
    le_bool   findMark2Glyph();

    void getCursiveEntryPoint(LEPoint &entryPoint) const;
    void getCursiveExitPoint(LEPoint &exitPoint) const;

    void setCurrGlyphID(TTGlyphID glyphID);
    void setCurrStreamPosition(le_int32 position);
    void setCurrGlyphBaseOffset(le_int32 baseOffset);
    void adjustCurrGlyphPositionAdjustment(float xPlacementAdjust, float yPlacementAdjust,
                                           float xAdvanceAdjust,   float yAdvanceAdjust);

    void setCurrGlyphPositionAdjustment(float xPlacementAdjust, float yPlacementAdjust,
                                        float xAdvanceAdjust,   float yAdvanceAdjust);

    void clearCursiveEntryPoint();
    void clearCursiveExitPoint();
    void setCursiveEntryPoint(LEPoint &entryPoint);
    void setCursiveExitPoint(LEPoint &exitPoint);
    void setCursiveGlyph();

    LEGlyphID *insertGlyphs(le_int32 count);
    le_int32 applyInsertions();

private:
    le_bool filterGlyph(le_uint32 index) const;
    le_bool hasFeatureTag(le_bool matchGroup) const;
    le_bool nextInternal(le_uint32 delta = 1);
    le_bool prevInternal(le_uint32 delta = 1);

    le_int32  direction;
    le_int32  position;
    le_int32  nextLimit;
    le_int32  prevLimit;

    LEGlyphStorage &glyphStorage;
    GlyphPositionAdjustments *glyphPositionAdjustments;

    le_int32    srcIndex;
    le_int32    destIndex;
    le_uint16   lookupFlags;
    FeatureMask featureMask;
    le_int32    glyphGroup;

    const GlyphClassDefinitionTable *glyphClassDefinitionTable;
    const MarkAttachClassDefinitionTable *markAttachClassDefinitionTable;

    GlyphIterator &operator=(const GlyphIterator &other); // forbid copying of this class
};

U_NAMESPACE_END
#endif
