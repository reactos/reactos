/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

/*
 * paragraphLayout doesn't make much sense without
 * BreakIterator...
 */
#include "layout/LETypes.h"
#include "layout/loengine.h"
#include "layout/plruns.h"
#include "layout/playout.h"

#include "unicode/locid.h"

#include "layout/LayoutEngine.h"
#include "layout/ParagraphLayout.h"

#if ! UCONFIG_NO_BREAK_ITERATION

U_NAMESPACE_USE

U_CAPI pl_paragraph * U_EXPORT2
pl_create(const LEUnicode chars[],
          le_int32 count,
          const pl_fontRuns *fontRuns,
          const pl_valueRuns *levelRuns,
          const pl_valueRuns *scriptRuns,
          const pl_localeRuns *localeRuns,
          UBiDiLevel paragraphLevel,
          le_bool vertical,
          LEErrorCode *status)
{
    ParagraphLayout *pl = new ParagraphLayout(chars, count, (const FontRuns *) fontRuns,
        (const ValueRuns *) levelRuns, (const ValueRuns *) scriptRuns, (const LocaleRuns *) localeRuns,
        paragraphLevel, vertical, *status);

    return (pl_paragraph *) pl;
}

U_CAPI void U_EXPORT2
pl_close(pl_paragraph *paragraph)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    delete pl;
}

U_CAPI le_bool U_EXPORT2
pl_isComplex(const LEUnicode chars[],
             le_int32 count)
{
    return ParagraphLayout::isComplex(chars, count);
}

U_CAPI UBiDiLevel U_EXPORT2
pl_getParagraphLevel(pl_paragraph *paragraph)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    if (pl == NULL) {
        return 0;
    }

    return pl->getParagraphLevel();
}

U_CAPI UBiDiDirection U_EXPORT2
pl_getTextDirection(pl_paragraph *paragraph)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    if (pl == NULL) {
        return UBIDI_LTR;
    }

    return pl->getTextDirection();
}

U_CAPI le_int32 U_EXPORT2
pl_getAscent(const pl_paragraph *paragraph)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    if (pl == NULL) {
        return 0;
    }

    return pl->getAscent();
}

U_CAPI le_int32 U_EXPORT2
pl_getDescent(const pl_paragraph *paragraph)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    if (pl == NULL) {
        return 0;
    }

    return pl->getDescent();
}

U_CAPI le_int32 U_EXPORT2
pl_getLeading(const pl_paragraph *paragraph)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    if (pl == NULL) {
        return 0;
    }

    return pl->getLeading();
}

U_CAPI void U_EXPORT2
pl_reflow(pl_paragraph *paragraph)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    if (pl == NULL) {
        return;
    }

    return pl->reflow();
}

U_CAPI pl_line * U_EXPORT2
pl_nextLine(pl_paragraph *paragraph, float width)
{
    ParagraphLayout *pl = (ParagraphLayout *) paragraph;

    if (pl == NULL) {
        return NULL;
    }

    return (pl_line *) pl->nextLine(width);
}

U_CAPI void U_EXPORT2
pl_closeLine(pl_line *line)
{
    ParagraphLayout::Line *ll = (ParagraphLayout::Line *) line;

    delete ll;
}

U_CAPI le_int32 U_EXPORT2
pl_countLineRuns(const pl_line *line)
{
    ParagraphLayout::Line *ll = (ParagraphLayout::Line *) line;

    if (ll == NULL) {
        return 0;
    }

    return ll->countRuns();
}

U_CAPI le_int32 U_EXPORT2
pl_getLineAscent(const pl_line *line)
{
    ParagraphLayout::Line *ll = (ParagraphLayout::Line *) line;

    if (ll == NULL) {
        return 0;
    }

    return ll->getAscent();
}

U_CAPI le_int32 U_EXPORT2
pl_getLineDescent(const pl_line *line)
{
    ParagraphLayout::Line *ll = (ParagraphLayout::Line *) line;

    if (ll == NULL) {
        return 0;
    }

    return ll->getDescent();
}

U_CAPI le_int32 U_EXPORT2
pl_getLineLeading(const pl_line *line)
{
    ParagraphLayout::Line *ll = (ParagraphLayout::Line *) line;

    if (ll == NULL) {
        return 0;
    }

    return ll->getLeading();
}

U_CAPI le_int32 U_EXPORT2
pl_getLineWidth(const pl_line *line)
{
    ParagraphLayout::Line *ll = (ParagraphLayout::Line *) line;

    if (ll == NULL) {
        return 0;
    }

    return ll->getWidth();
}

U_CAPI const pl_visualRun * U_EXPORT2
pl_getLineVisualRun(const pl_line *line, le_int32 runIndex)
{
    ParagraphLayout::Line *ll = (ParagraphLayout::Line *) line;

    if (ll == NULL) {
        return 0;
    }

    return (pl_visualRun *) ll->getVisualRun(runIndex);
}

U_CAPI const le_font * U_EXPORT2
pl_getVisualRunFont(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return NULL;
    }

    return (const le_font *) vr->getFont();
}

U_CAPI UBiDiDirection U_EXPORT2
pl_getVisualRunDirection(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return UBIDI_LTR;
    }

    return vr->getDirection();
}

U_CAPI le_int32 U_EXPORT2
pl_getVisualRunGlyphCount(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return -1;
    }

    return vr->getGlyphCount();
}

U_CAPI const LEGlyphID * U_EXPORT2
pl_getVisualRunGlyphs(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return NULL;
    }

    return vr->getGlyphs();
}

U_CAPI const float * U_EXPORT2
pl_getVisualRunPositions(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return NULL;
    }

    return vr->getPositions();
}

U_CAPI const le_int32 * U_EXPORT2
pl_getVisualRunGlyphToCharMap(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return NULL;
    }

    return vr->getGlyphToCharMap();
}

U_CAPI le_int32 U_EXPORT2
pl_getVisualRunAscent(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return 0;
    }

    return vr->getAscent();
}

U_CAPI le_int32 U_EXPORT2
pl_getVisualRunDescent(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return 0;
    }

    return vr->getDescent();
}

U_CAPI le_int32 U_EXPORT2
pl_getVisualRunLeading(const pl_visualRun *run)
{
    ParagraphLayout::VisualRun *vr = (ParagraphLayout::VisualRun *) run;

    if (vr == NULL) {
        return 0;
    }

    return vr->getLeading();
}

#endif
