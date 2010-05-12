/*
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LEFontInstance.h"
#include "OpenTypeTables.h"
#include "AnchorTables.h"
#include "MarkArrays.h"
#include "GlyphPositioningTables.h"
#include "AttachmentPosnSubtables.h"
#include "MarkToLigaturePosnSubtables.h"
#include "GlyphIterator.h"
#include "LESwaps.h"

U_NAMESPACE_BEGIN

LEGlyphID MarkToLigaturePositioningSubtable::findLigatureGlyph(GlyphIterator *glyphIterator) const
{
    if (glyphIterator->prev()) {
        return glyphIterator->getCurrGlyphID();
    }

    return 0xFFFF;
}

le_int32 MarkToLigaturePositioningSubtable::process(GlyphIterator *glyphIterator, const LEFontInstance *fontInstance) const
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

    // FIXME: we probably don't want to find a ligature before a previous base glyph...
    GlyphIterator ligatureIterator(*glyphIterator, (le_uint16) (lfIgnoreMarks /*| lfIgnoreBaseGlyphs*/));
    LEGlyphID ligatureGlyph = findLigatureGlyph(&ligatureIterator);
    le_int32 ligatureCoverage = getBaseCoverage((LEGlyphID) ligatureGlyph);
    const LigatureArray *ligatureArray = (const LigatureArray *) ((char *) this + SWAPW(baseArrayOffset));
    le_uint16 ligatureCount = SWAPW(ligatureArray->ligatureCount);

    if (ligatureCoverage < 0 || ligatureCoverage >= ligatureCount) {
        // The ligature glyph isn't covered, or the coverage
        // index is too big. The latter means that the
        // table is mal-formed...
        return 0;
    }

    le_int32 markPosition = glyphIterator->getCurrStreamPosition();
    Offset ligatureAttachOffset = SWAPW(ligatureArray->ligatureAttachTableOffsetArray[ligatureCoverage]);
    const LigatureAttachTable *ligatureAttachTable = (const LigatureAttachTable *) ((char *) ligatureArray + ligatureAttachOffset);
    le_int32 componentCount = SWAPW(ligatureAttachTable->componentCount);
    le_int32 component = ligatureIterator.getMarkComponent(markPosition);

    if (component >= componentCount) {
        // should really just bail at this point...
        component = componentCount - 1;
    }

    const ComponentRecord *componentRecord = &ligatureAttachTable->componentRecordArray[component * mcCount];
    Offset anchorTableOffset = SWAPW(componentRecord->ligatureAnchorTableOffsetArray[markClass]);
    const AnchorTable *anchorTable = (const AnchorTable *) ((char *) ligatureAttachTable + anchorTableOffset);
    LEPoint ligatureAnchor, markAdvance, pixels;

    anchorTable->getAnchor(ligatureGlyph, fontInstance, ligatureAnchor);

    fontInstance->getGlyphAdvance(markGlyph, pixels);
    fontInstance->pixelsToUnits(pixels, markAdvance);

    float anchorDiffX = ligatureAnchor.fX - markAnchor.fX;
    float anchorDiffY = ligatureAnchor.fY - markAnchor.fY;

    glyphIterator->setCurrGlyphBaseOffset(ligatureIterator.getCurrStreamPosition());

    if (glyphIterator->isRightToLeft()) {
        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX, anchorDiffY, -markAdvance.fX, -markAdvance.fY);
    } else {
        LEPoint ligatureAdvance;

        fontInstance->getGlyphAdvance(ligatureGlyph, pixels);
        fontInstance->pixelsToUnits(pixels, ligatureAdvance);

        glyphIterator->setCurrGlyphPositionAdjustment(anchorDiffX - ligatureAdvance.fX, anchorDiffY - ligatureAdvance.fY, -markAdvance.fX, -markAdvance.fY);
    }

    return 1;
}

U_NAMESPACE_END
