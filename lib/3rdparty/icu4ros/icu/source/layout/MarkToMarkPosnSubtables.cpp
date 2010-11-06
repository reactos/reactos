/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "AnchorTables.h"
#include "MarkArrays.h"
#include "GlyphPositioningTables.h"
#include "AttachmentPosnSubtables.h"
#include "MarkToMarkPosnSubtables.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

LEGlyphID MarkToMarkPositioningSubtable::findMark2Glyph(GlyphIterator *glyphIterator) const
{
    if (glyphIterator->findMark2Glyph()) {
        return glyphIterator->getCurrGlyphID();
    }

    return 0xFFFF;
}

le_int32 MarkToMarkPositioningSubtable::process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const
{
    LEGlyphID markGlyph = glyphIterator->getCurrGlyphID();
    le_int32 markCoverage = getGlyphCoverage((LEGlyphID) markGlyph);

    if (markCoverage < 0) {
        // markGlyph isn't a covered mark glyph
        return 0;
    }

    LEPoint markAnchor;
    const MarkArray *markArray = (const MarkArray *) ((char *) this + SWAPW(markArrayOffset));
    le_int32 markClass = markArray->getMarkClass(markGlyph, markCoverage, fontInstance, markAnchor);
    le_uint16 mcCount = SWAPW(classCount);

    if (markClass < 0 || markClass >= mcCount) {
        // markGlyph isn't in the mark array or its
        // mark class is too big. The table is mal-formed!
        return 0;
    }

    GlyphIterator mark2Iterator(*glyphIterator);
    LEGlyphID mark2Glyph = findMark2Glyph(&mark2Iterator);
    le_int32 mark2Coverage = getBaseCoverage((LEGlyphID) mark2Glyph);
    const Mark2Array *mark2Array = (const Mark2Array *) ((char *) this + SWAPW(baseArrayOffset));
    le_uint16 mark2Count = SWAPW(mark2Array->mark2RecordCount);

    if (mark2Coverage < 0 || mark2Coverage >= mark2Count) {
        // The mark2 glyph isn't covered, or the coverage
        // index is too big. The latter means that the
        // table is mal-formed...
        return 0;
    }

    const Mark2Record *mark2Record = &mark2Array->mark2RecordArray[mark2Coverage * mcCount];
    Offset anchorTableOffset = SWAPW(mark2Record->mark2AnchorTableOffsetArray[markClass]);
    const AnchorTable *anchorTable = (const AnchorTable *) ((char *) mark2Array + anchorTableOffset);
    LEPoint mark2Anchor, markAdvance, pixels;

    if (anchorTableOffset == 0) {
        // this seems to mean that the marks don't attach...
        return 0;
    }

    anchorTable->getAnchor(mark2Glyph, fontInstance, mark2Anchor);

    fontInstance->getGlyphAdvance(markGlyph, pixels);
    fontInstance->pixelsToUnits(pixels, markAdvance);

    float anchorDiffX = mark2Anchor.fX - markAnchor.fX;
    float anchorDiffY = mark2Anchor.fY - markAnchor.fY;

    glyphIterator->setCurrGlyphBaseOffset(mark2Iterator.getCurrStreamPosition());

    if (glyphIterator->isRightToLeft()) {
        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX, anchorDiffY, -markAdvance.fX, -markAdvance.fY);
    } else {
        LEPoint mark2Advance;

        fontInstance->getGlyphAdvance(mark2Glyph, pixels);
        fontInstance->pixelsToUnits(pixels, mark2Advance);

        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX - mark2Advance.fX, anchorDiffY - mark2Advance.fY, -markAdvance.fX, -markAdvance.fY);
    }

    return 1;
}

U_NAMESPACE_END
