/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2007, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  Paragraph.cpp
 *
 *   created on: 09/06/2000
 *   created by: Eric R. Mader
 */

#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ubidi.h"
#include "unicode/ustring.h"

#include "layout/ParagraphLayout.h"

#include "RenderingSurface.h"

#include "paragraph.h"
#include "UnicodeReader.h"

#define MARGIN 10
#define LINE_GROW 32
#define PARA_GROW 8

#define CH_LF 0x000A
#define CH_CR 0x000D
#define CH_LSEP 0x2028
#define CH_PSEP 0x2029

static LEUnicode *skipLineEnd(LEUnicode *ptr)
{
    if (ptr[0] == CH_CR && ptr[1] == CH_LF) {
        ptr += 1;
    }

    return ptr + 1;
}

static le_int32 findRun(const RunArray *runArray, le_int32 offset)
{
    le_int32 runCount = runArray->getCount();

    for (le_int32 run = 0; run < runCount; run += 1) {
        if (runArray->getLimit(run) > offset) {
            return run;
        }
    }

    return -1;
}

static void subsetFontRuns(const FontRuns *fontRuns, le_int32 start, le_int32 limit, FontRuns *sub)
{
    le_int32 startRun = findRun(fontRuns, start);
    le_int32 endRun   = findRun(fontRuns, limit - 1);

    sub->reset();

    for (le_int32 run = startRun; run <= endRun; run += 1) {
        const LEFontInstance *runFont = fontRuns->getFont(run);
        le_int32 runLimit = fontRuns->getLimit(run) - start;

        if (run == endRun) {
            runLimit = limit - start;
        }

        sub->add(runFont, runLimit);
    }
}

Paragraph::Paragraph(const LEUnicode chars[], int32_t charCount, const FontRuns *fontRuns, LEErrorCode &status)
  : fParagraphLayout(NULL), fParagraphCount(0), fParagraphMax(PARA_GROW), fParagraphGrow(PARA_GROW),
    fLineCount(0), fLinesMax(LINE_GROW), fLinesGrow(LINE_GROW), fLines(NULL), fChars(NULL),
    fLineHeight(-1), fAscent(-1), fWidth(-1), fHeight(-1), fParagraphLevel(UBIDI_DEFAULT_LTR)
{
    static const LEUnicode separators[] = {CH_LF, CH_CR, CH_LSEP, CH_PSEP, 0x0000};

	if (LE_FAILURE(status)) {
		return;
	}

    le_int32 ascent  = 0;
    le_int32 descent = 0;
    le_int32 leading = 0;

	LocaleRuns *locales = NULL;
    FontRuns fr(0);

    fLines = LE_NEW_ARRAY(const ParagraphLayout::Line *, fLinesMax);
    fParagraphLayout = LE_NEW_ARRAY(ParagraphLayout *, fParagraphMax);

    fChars = LE_NEW_ARRAY(LEUnicode, charCount + 1);
    LE_ARRAY_COPY(fChars, chars, charCount);
    fChars[charCount] = 0;

    LEUnicode *pStart = &fChars[0];

    while (*pStart != 0) {
        LEUnicode *pEnd = u_strpbrk(pStart, separators);
        le_int32 pAscent, pDescent, pLeading;
        ParagraphLayout *paragraphLayout = NULL;

        if (pEnd == NULL) {
            pEnd = &fChars[charCount];
        }

        if (pEnd != pStart) {
            subsetFontRuns(fontRuns, pStart - fChars, pEnd - fChars, &fr);

            paragraphLayout = new ParagraphLayout(pStart, pEnd - pStart, &fr, NULL, NULL, locales, fParagraphLevel, FALSE, status);

            if (LE_FAILURE(status)) {
                break; // return? something else?
            }

            if (fParagraphLevel == UBIDI_DEFAULT_LTR) {
                fParagraphLevel = paragraphLayout->getParagraphLevel();
            }

            pAscent  = paragraphLayout->getAscent();
            pDescent = paragraphLayout->getDescent();
            pLeading = paragraphLayout->getLeading();

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

        if (fParagraphCount >= fParagraphMax) {
            fParagraphLayout = (ParagraphLayout **) LE_GROW_ARRAY(fParagraphLayout, fParagraphMax + fParagraphGrow);
            fParagraphMax += fParagraphGrow;
        }

        fParagraphLayout[fParagraphCount++] = paragraphLayout;

        if (*pEnd == 0) {
            break;
        }

        pStart = skipLineEnd(pEnd);
    }

    fLineHeight = ascent + descent + leading;
    fAscent     = ascent;
}

Paragraph::~Paragraph()
{
    for (le_int32 line = 0; line < fLineCount; line += 1) {
        delete /*(LineInfo *)*/ fLines[line];
    }

    LE_DELETE_ARRAY(fLines);
    delete fParagraphLayout;
    LE_DELETE_ARRAY(fChars);
}

void Paragraph::addLine(const ParagraphLayout::Line *line)
{
    if (fLineCount >= fLinesMax) {
        fLines = (const ParagraphLayout::Line **) LE_GROW_ARRAY(fLines, fLinesMax + fLinesGrow);
        fLinesMax += fLinesGrow;
    }

    fLines[fLineCount++] = line;
}

void Paragraph::breakLines(le_int32 width, le_int32 height)
{
    fHeight = height;

    // don't re-break if the width hasn't changed
    if (fWidth == width) {
        return;
    }

    fWidth  = width;

    float lineWidth = (float) (width - 2 * MARGIN);
    const ParagraphLayout::Line *line;

    // Free the old LineInfo's...
    for (le_int32 li = 0; li < fLineCount; li += 1) {
        delete fLines[li];
    }

    fLineCount = 0;

    for (le_int32 p = 0; p < fParagraphCount; p += 1) {
        ParagraphLayout *paragraphLayout = fParagraphLayout[p];

        if (paragraphLayout != NULL) {
            paragraphLayout->reflow();
            while ((line = paragraphLayout->nextLine(lineWidth)) != NULL) {
                addLine(line);
            }
        } else {
            addLine(NULL);
        }
    }
}

void Paragraph::draw(RenderingSurface *surface, le_int32 firstLine, le_int32 lastLine)
{
    le_int32 li, x, y;

    x = MARGIN;
    y = fAscent;

    for (li = firstLine; li <= lastLine; li += 1) {
        const ParagraphLayout::Line *line = fLines[li];

        if (line != NULL) {
            le_int32 runCount = line->countRuns();
            le_int32 run;

		    if (fParagraphLevel == UBIDI_RTL) {
			    le_int32 lastX = line->getWidth();

			    x = (fWidth - lastX - MARGIN);
		    }


            for (run = 0; run < runCount; run += 1) {
                const ParagraphLayout::VisualRun *visualRun = line->getVisualRun(run);
                le_int32 glyphCount = visualRun->getGlyphCount();
                const LEFontInstance *font = visualRun->getFont();
                const LEGlyphID *glyphs = visualRun->getGlyphs();
                const float *positions = visualRun->getPositions();

                surface->drawGlyphs(font, glyphs, glyphCount, positions, x, y, fWidth, fHeight);
            }
        }

        y += fLineHeight;
    }
}

Paragraph *Paragraph::paragraphFactory(const char *fileName, const LEFontInstance *font, GUISupport *guiSupport)
{
    LEErrorCode status  = LE_NO_ERROR;
    le_int32 charCount;
    const UChar *text = UnicodeReader::readFile(fileName, guiSupport, charCount);
    Paragraph *result = NULL;

    if (text == NULL) {
        return NULL;
    }

    FontRuns  fontRuns(0);

    fontRuns.add(font, charCount);

    result = new Paragraph(text, charCount, &fontRuns, status);

	if (LE_FAILURE(status)) {
		delete result;
		result = NULL;
	}

    LE_DELETE_ARRAY(text);

    return result;    
}

