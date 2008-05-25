/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "unicode/ustring.h"

#include "layout/LETypes.h"

#include "layout/loengine.h"
#include "layout/playout.h"
#include "layout/plruns.h"

#include "pflow.h"

#include "arraymem.h"
#include "ucreader.h"

/*
 * Move the line below out of this comment
 * to add a locale run to the pl_paragraphs
 * that are created.
#define TEST_LOCALE "zh_TW"
 */

#define MARGIN 10
#define LINE_GROW 32
#define PARA_GROW 8

#define CH_LF 0x000A
#define CH_CR 0x000D
#define CH_LSEP 0x2028
#define CH_PSEP 0x2029

struct pf_object
{
    pl_paragraph    **fParagraphLayout;

    le_int32          fParagraphCount;
    le_int32          fParagraphMax;
    le_int32          fParagraphGrow;
    
    le_int32          fLineCount;
    le_int32          fLinesMax;
    le_int32          fLinesGrow;

    pl_line         **fLines;

   LEUnicode         *fChars;

    le_int32          fLineHeight;
    le_int32          fAscent;
    le_int32          fWidth;
    le_int32          fHeight;
    UBiDiLevel        fParagraphLevel;
};

typedef struct pf_object pf_object;


static LEUnicode *skipLineEnd(LEUnicode *ptr)
{
    if (ptr[0] == CH_CR && ptr[1] == CH_LF) {
        ptr += 1;
    }

    return ptr + 1;
}

static le_int32 findFontRun(const pl_fontRuns *fontRuns, le_int32 offset)
{
    le_int32 runCount = pl_getFontRunCount(fontRuns);
    le_int32 run;

    for (run = 0; run < runCount; run += 1) {
        if (pl_getFontRunLimit(fontRuns, run) > offset) {
            return run;
        }
    }

    return -1;
}

static void subsetFontRuns(const pl_fontRuns *fontRuns, le_int32 start, le_int32 limit, pl_fontRuns *sub)
{
    le_int32 startRun = findFontRun(fontRuns, start);
    le_int32 endRun   = findFontRun(fontRuns, limit - 1);
    le_int32 run;

    pl_resetFontRuns(sub);

    for (run = startRun; run <= endRun; run += 1) {
        const le_font *runFont = pl_getFontRunFont(fontRuns, run);
        le_int32 runLimit = pl_getFontRunLimit(fontRuns, run) - start;

        if (run == endRun) {
            runLimit = limit - start;
        }

        pl_addFontRun(sub, runFont, runLimit);
    }
}

pf_flow *pf_create(const LEUnicode chars[], le_int32 charCount, const pl_fontRuns *fontRuns, LEErrorCode *status)
{
    pf_object *flow;
    le_int32 ascent  = 0;
    le_int32 descent = 0;
    le_int32 leading = 0;
	pl_localeRuns *locales = NULL;
    pl_fontRuns *fr;
    LEUnicode *pStart;
    static const LEUnicode separators[] = {CH_LF, CH_CR, CH_LSEP, CH_PSEP, 0x0000};

	if (LE_FAILURE(*status)) {
		return NULL;
	}

    flow = NEW_ARRAY(pf_object, 1);

    flow->fParagraphLayout = NULL;
    flow->fParagraphCount  = 0;
    flow->fParagraphMax    = PARA_GROW;
    flow->fParagraphGrow   = PARA_GROW;
    flow->fLineCount       = 0;
    flow->fLinesMax        = LINE_GROW;
    flow->fLinesGrow       = LINE_GROW;
    flow->fLines           = NULL;
    flow->fChars           = NULL;
    flow->fLineHeight      = -1;
    flow->fAscent          = -1;
    flow->fWidth           = -1;
    flow->fHeight          = -1;
    flow->fParagraphLevel  = UBIDI_DEFAULT_LTR;

    fr = pl_openEmptyFontRuns(0);

#ifdef TEST_LOCALE
    locales = pl_openEmptyLocaleRuns(0);
#endif

    flow->fLines = NEW_ARRAY(pl_line *, flow->fLinesMax);
    flow->fParagraphLayout = NEW_ARRAY(pl_paragraph *, flow->fParagraphMax);

    flow->fChars = NEW_ARRAY(LEUnicode, charCount + 1);
    LE_ARRAY_COPY(flow->fChars, chars, charCount);
    flow->fChars[charCount] = 0;

    pStart = &flow->fChars[0];

    while (*pStart != 0) {
        LEUnicode *pEnd = u_strpbrk(pStart, separators);
        le_int32 pAscent, pDescent, pLeading;
        pl_paragraph *paragraphLayout = NULL;

        if (pEnd == NULL) {
            pEnd = &flow->fChars[charCount];
        }

        if (pEnd != pStart) {
            subsetFontRuns(fontRuns, pStart - flow->fChars, pEnd - flow->fChars, fr);

#ifdef TEST_LOCALE
            pl_resetLocaleRuns(locales);
            pl_addLocaleRun(locales, TEST_LOCALE, pEnd - pStart);
#endif

            paragraphLayout = pl_create(pStart, pEnd - pStart, fr, NULL, NULL, locales, flow->fParagraphLevel, FALSE, status);

            if (LE_FAILURE(*status)) {
                break; /* return? something else? */
            }

            if (flow->fParagraphLevel == UBIDI_DEFAULT_LTR) {
                flow->fParagraphLevel = pl_getParagraphLevel(paragraphLayout);
            }

            pAscent  = pl_getAscent(paragraphLayout);
            pDescent = pl_getDescent(paragraphLayout);
            pLeading = pl_getLeading(paragraphLayout);

            if (pAscent > ascent) {
                ascent = pAscent;
            }

            if (pDescent > descent) {
                descent = pDescent;
            }

            if (pLeading > leading) {
                leading = pLeading;
            }
        }

        if (flow->fParagraphCount >= flow->fParagraphMax) {
            flow->fParagraphLayout = (pl_paragraph **) GROW_ARRAY(flow->fParagraphLayout, flow->fParagraphMax + flow->fParagraphGrow);
            flow->fParagraphMax += flow->fParagraphGrow;
        }

        flow->fParagraphLayout[flow->fParagraphCount++] = paragraphLayout;

        if (*pEnd == 0) {
            break;
        }

        pStart = skipLineEnd(pEnd);
    }

    flow->fLineHeight = ascent + descent + leading;
    flow->fAscent     = ascent;

    pl_closeLocaleRuns(locales);
    pl_closeFontRuns(fr);

    return (pf_flow *) flow;
}

void pf_close(pf_flow *flow)
{
    pf_object *obj = (pf_object *) flow;
    le_int32 i;

    for (i = 0; i < obj->fLineCount; i += 1) {
        DELETE_ARRAY(obj->fLines[i]);
    }

    DELETE_ARRAY(obj->fLines);

    for (i = 0; i < obj->fParagraphCount; i += 1) {
        pl_close(obj->fParagraphLayout[i]);
    }

    DELETE_ARRAY(obj->fParagraphLayout);

    DELETE_ARRAY(obj->fChars);

    DELETE_ARRAY(obj);
}


le_int32 pf_getAscent(pf_flow *flow)
{
    pf_object *obj = (pf_object *) flow;

    return obj->fAscent;
}

le_int32 pf_getLineHeight(pf_flow *flow)
{
    pf_object *obj = (pf_object *) flow;

    return obj->fLineHeight;
}

le_int32 pf_getLineCount(pf_flow *flow)
{
    pf_object *obj = (pf_object *) flow;

    return obj->fLineCount;
}

static void addLine(pf_object *obj, pl_line *line)
{
    if (obj->fLineCount >= obj->fLinesMax) {
        obj->fLines = (pl_line **) GROW_ARRAY(obj->fLines, obj->fLinesMax + obj->fLinesGrow);
        obj->fLinesMax += obj->fLinesGrow;
    }

    obj->fLines[obj->fLineCount++] = line;
}

void pf_breakLines(pf_flow *flow, le_int32 width, le_int32 height)
{
    pf_object *obj = (pf_object *) flow;
    le_int32 li, p;
    float lineWidth;
    pl_line *line;

    obj->fHeight = height;

    /* don't re-break if the width hasn't changed */
    if (obj->fWidth == width) {
        return;
    }

    obj->fWidth  = width;

    lineWidth = (float) (width - 2 * MARGIN);

    /* Free the old Lines... */
    for (li = 0; li < obj->fLineCount; li += 1) {
        pl_closeLine(obj->fLines[li]);
    }

    obj->fLineCount = 0;

    for (p = 0; p < obj->fParagraphCount; p += 1) {
        pl_paragraph *paragraphLayout = obj->fParagraphLayout[p];

        if (paragraphLayout != NULL) {
            pl_reflow(paragraphLayout);
            while ((line = pl_nextLine(paragraphLayout, lineWidth)) != NULL) {
                addLine(obj, line);
            }
        } else {
            addLine(obj, NULL);
        }
    }
}

void pf_draw(pf_flow *flow, rs_surface *surface, le_int32 firstLine, le_int32 lastLine)
{
    pf_object *obj = (pf_object *) flow;
    le_int32 li, x, y;

    x = MARGIN;
    y = obj->fAscent;

    for (li = firstLine; li <= lastLine; li += 1) {
        const pl_line *line = obj->fLines[li];

        if (line != NULL) {
            le_int32 runCount = pl_countLineRuns(line);
            le_int32 run;

		    if (obj->fParagraphLevel == UBIDI_RTL) {
			    le_int32 lastX = pl_getLineWidth(line);

			    x = (obj->fWidth - lastX - MARGIN);
		    }


            for (run = 0; run < runCount; run += 1) {
                const pl_visualRun *visualRun = pl_getLineVisualRun(line, run);
                le_int32 glyphCount = pl_getVisualRunGlyphCount(visualRun);
                const le_font *font = pl_getVisualRunFont(visualRun);
                const LEGlyphID *glyphs = pl_getVisualRunGlyphs(visualRun);
                const float *positions = pl_getVisualRunPositions(visualRun);

                rs_drawGlyphs(surface, font, glyphs, glyphCount, positions, x, y, obj->fWidth, obj->fHeight);
            }
        }

        y += obj->fLineHeight;
    }
}

pf_flow *pf_factory(const char *fileName, const le_font *font, gs_guiSupport *guiSupport)
{
    LEErrorCode status  = LE_NO_ERROR;
    le_int32 charCount;
    const UChar *text = uc_readFile(fileName, guiSupport, &charCount);
    pl_fontRuns *fontRuns;
    pf_flow *result = NULL;

    if (text == NULL) {
        return NULL;
    }

    fontRuns = pl_openEmptyFontRuns(0);

    pl_addFontRun(fontRuns, font, charCount);

    result = pf_create(text, charCount, fontRuns, &status);

	if (LE_FAILURE(status)) {
		pf_close(result);
		result = NULL;
	}

    pl_closeFontRuns(fontRuns);

    DELETE_ARRAY(text);

    return result;    
}

