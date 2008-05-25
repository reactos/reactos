/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#ifndef __PLAYOUT_H
#define __PLAYOUT_H

/*
 * ParagraphLayout doesn't make much sense without
 * BreakIterator...
 */
#include "unicode/ubidi.h"
#if ! UCONFIG_NO_BREAK_ITERATION

#include "layout/LETypes.h"
#include "plruns.h"

/**
 * \file 
 * \brief C API for paragraph layout.
 *
 * This is a technology preview. The API may
 * change significantly.
 *
 */

/**
 * The opaque type for a paragraph layout.
 *
 * @internal
 */
typedef void pl_paragraph;

/**
 * The opaque type for a line in a paragraph layout.
 *
 * @internal
 */
typedef void pl_line;

/**
 * The opaque type for a visual run in a line.
 *
 * @internal
 */
typedef void pl_visualRun;

/**
 * Construct a <code>ParagraphLayout</code> object for a styled paragraph. The paragraph is specified
 * as runs of text all in the same font. An <code>LEFontInstance</code> object and a limit offset
 * are specified for each font run. The limit offset is the offset of the character immediately
 * after the font run.
 *
 * Clients can optionally specify directional runs and / or script runs. If these aren't specified
 * they will be computed.
 *
 * If any errors are encountered during construction, <code>status</code> will be set, and the object
 * will be set to be empty.
 *
 * @param chars is an array of the characters in the paragraph
 *
 * @param count is the number of characters in the paragraph.
 *
 * @param fontRuns a pointer to a <code>pl_fontRuns</code> object representing the font runs.
 *
 * @param levelRuns is a pointer to a <code>pl_valueRuns</code> object representing the directional levels.
 *        If this pointer in <code>NULL</code> the levels will be determined by running the Unicde
 *        Bidi algorithm.
 *
 * @param scriptRuns is a pointer to a <code>pl_valueRuns</code> object representing script runs.
 *        If this pointer in <code>NULL</code> the script runs will be determined using the
 *        Unicode code points.
 *
 * @param localeRuns is a pointer to a <code>pl_localeRuns</code> object representing locale runs.
 *        The <code>Locale</code> objects are used to determind the language of the text. If this
 *        pointer is <code>NULL</code> the default locale will be used for all of the text. 
 *
 * @param paragraphLevel is the directionality of the paragraph, as in the UBiDi object.
 *
 * @param vertical is <code>TRUE</code> if the paragraph should be set vertically.
 *
 * @param status will be set to any error code encountered during construction.
 *
 * @return a pointer to the newly created <code>pl_paragraph</code> object. The object
 *         will remain valid until <code>pl_close</code> is called.
 *
 * @see ubidi.h
 * @see longine.h
 * @see plruns.h
 *
 * @internal
 */
U_INTERNAL pl_paragraph * U_EXPORT2
pl_create(const LEUnicode chars[],
          le_int32 count,
          const pl_fontRuns *fontRuns,
          const pl_valueRuns *levelRuns,
          const pl_valueRuns *scriptRuns,
          const pl_localeRuns *localeRuns,
          UBiDiLevel paragraphLevel,
          le_bool vertical,
          LEErrorCode *status);

/**
 * Close the given paragraph layout object.
 *
 * @param paragraph the <code>pl_paragraph</code> object to be
 *                  closed. Once this routine returns the object
 *                  can no longer be referenced
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_close(pl_paragraph *paragraph);

/**
 * Examine the given text and determine if it contains characters in any
 * script which requires complex processing to be rendered correctly.
 *
 * @param chars is an array of the characters in the paragraph
 *
 * @param count is the number of characters in the paragraph.
 *
 * @return <code>TRUE</code> if any of the text requires complex processing.
 *
 * @internal
 */

U_INTERNAL le_bool U_EXPORT2
pl_isComplex(const LEUnicode chars[],
          le_int32 count);

/**
 * Return the resolved paragraph level. This is useful for those cases
 * where the bidi analysis has determined the level based on the first
 * strong character in the paragraph.
 *
 * @param paragraph the <code>pl_paragraph</code>
 *
 * @return the resolved paragraph level.
 *
 * @internal
 */
U_INTERNAL UBiDiLevel U_EXPORT2
pl_getParagraphLevel(pl_paragraph *paragraph);

/**
 * Return the directionality of the text in the paragraph.
 *
 * @param paragraph the <code>pl_paragraph</code>
 *
 * @return <code>UBIDI_LTR</code> if the text is all left to right,
 *         <code>UBIDI_RTL</code> if the text is all right to left,
 *         or <code>UBIDI_MIXED</code> if the text has mixed direction.
 *
 * @internal
 */
U_INTERNAL UBiDiDirection U_EXPORT2
pl_getTextDirection(pl_paragraph *paragraph);

/**
 * Get the max ascent value for all the fonts
 * in the paragraph.
 *
 * @param paragraph the <code>pl_paragraph</code>
 *
 * Return the max ascent value for all the fonts
 * in the paragraph.
 *
 * @param paragraph the <code>pl_paragraph</code>
 *
 * @return the ascent value.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getAscent(const pl_paragraph *paragraph);

/**
 * Return the max descent value for all the fonts
 * in the paragraph.
 *
 * @param paragraph the <code>pl_paragraph</code>
 *
 * @return the decent value.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getDescent(const pl_paragraph *paragraph);

/**
 * Return the max leading value for all the fonts
 * in the paragraph.
 *
 * @param paragraph the <code>pl_paragraph</code>
 *
 * @return the leading value.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLeading(const pl_paragraph *paragraph);

/**
 * Reset line breaking to start from the beginning of the paragraph.
 *
 * @param paragraph the <code>pl_paragraph</code>
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_reflow(pl_paragraph *paragraph);

/**
 * Return a <code>pl_line</code> object which represents next line
 * in the paragraph. The width of the line is specified each time so that it can
 * be varied to support arbitrary paragraph shapes.
 *
 * @param paragraph the <code>pl_paragraph</code>
 * @param width is the width of the line. If <code>width</code> is less than or equal
 *              to zero, a <code>ParagraphLayout::Line</code> object representing the
 *              rest of the paragraph will be returned.
 *
 * @return a <code>ParagraphLayout::Line</code> object which represents the line. The caller
 *         is responsible for deleting the object. Returns <code>NULL</code> if there are no
 *         more lines in the paragraph.
 *
 * @see pl_line
 *
 * @internal
 */
U_INTERNAL pl_line * U_EXPORT2
pl_nextLine(pl_paragraph *paragraph, float width);

/**
 * Close the given line object. Line objects are created
 * by <code>pl_nextLine</code> but it is the client's responsibility
 * to close them by calling this routine.
 *
 * @param line the <code>pl_line</code> object to close.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
pl_closeLine(pl_line *line);

/**
 * Count the number of visual runs in the line.
 *
 * @param line the <code>pl_line</code> object.
 *
 * @return the number of visual runs.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_countLineRuns(const pl_line *line);

/**
 * Get the ascent of the line. This is the maximum ascent
 * of all the fonts on the line.
 *
 * @param line the <code>pl_line</code> object.
 *
 * @return the ascent of the line.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLineAscent(const pl_line *line);

/**
 * Get the descent of the line. This is the maximum descent
 * of all the fonts on the line.
 *
 * @param line the <code>pl_line</code> object.
 *
 * @return the descent of the line.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLineDescent(const pl_line *line);

/**
 * Get the leading of the line. This is the maximum leading
 * of all the fonts on the line.
 *
 * @param line the <code>pl_line</code> object.
 *
 * @return the leading of the line.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLineLeading(const pl_line *line);

/**
 * Get the width of the line. This is a convenience method
 * which returns the last X position of the last visual run
 * in the line.
 *
 * @param line the <code>pl_line</code> object.
 *
 * @return the width of the line.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getLineWidth(const pl_line *line);

/**
 * Get a <code>ParagraphLayout::VisualRun</code> object for a given
 * visual run in the line.
 *
 * @param line the <code>pl_line</code> object.
 * @param runIndex is the index of the run, in visual order.
 *
 * @return the <code>pl_visualRun</code> object representing the
 *         visual run. This object is owned by the <code>pl_line</code> object which
 *         created it, and will remain valid for as long as the <code>pl_line</code>
 *         object is valid.
 *
 * @see pl_visualRun
 *
 * @internal
 */
U_INTERNAL const pl_visualRun * U_EXPORT2
pl_getLineVisualRun(const pl_line *line, le_int32 runIndex);

/**
 * Get the <code>le_font</code> object which
 * represents the font of the visual run. This will always
 * be a non-composite font.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the <code>le_font</code> object which represents the
 *         font of the visual run.
 *
 * @see le_font
 *
 * @internal
 */
U_INTERNAL const le_font * U_EXPORT2
pl_getVisualRunFont(const pl_visualRun *run);

/**
 * Get the direction of the visual run.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the direction of the run. This will be <code>UBIDI_LTR</code> if the
 *         run is left-to-right and <code>UBIDI_RTL</code> if the line is right-to-left.
 *
 * @internal
 */
U_INTERNAL UBiDiDirection U_EXPORT2
pl_getVisualRunDirection(const pl_visualRun *run);

/**
 * Get the number of glyphs in the visual run.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the number of glyphs.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getVisualRunGlyphCount(const pl_visualRun *run);

/**
 * Get the glyphs in the visual run. Glyphs with the values <code>0xFFFE</code> and
 * <code>0xFFFF</code> should be ignored.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the address of the array of glyphs for this visual run. The storage
 *         is owned by the <code>pl_visualRun</code> object and must not be deleted.
 *         It will remain valid as long as the <code>pl_visualRun</code> object is valid.
 *
 * @internal
 */
U_INTERNAL const LEGlyphID * U_EXPORT2
pl_getVisualRunGlyphs(const pl_visualRun *run);

/**
 * Get the (x, y) positions of the glyphs in the visual run. To simplify storage
 * management, the x and y positions are stored in a single array with the x positions
 * at even offsets in the array and the corresponding y position in the following odd offset.
 * There is an extra (x, y) pair at the end of the array which represents the advance of
 * the final glyph in the run.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the address of the array of glyph positions for this visual run. The storage
 *         is owned by the <code>pl_visualRun</code> object and must not be deleted.
 *         It will remain valid as long as the <code>pl_visualRun</code> object is valid.
 *
 * @internal
 */
U_INTERNAL const float * U_EXPORT2
pl_getVisualRunPositions(const pl_visualRun *run);

/**
 * Get the glyph-to-character map for this visual run. This maps the indices into
 * the glyph array to indices into the character array used to create the paragraph.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the address of the character-to-glyph map for this visual run. The storage
 *         is owned by the <code>pl_visualRun</code> object and must not be deleted.
 *         It will remain valid as long as the <code>pl_visualRun</code> object is valid.
 *
 * @internal
 */
U_INTERNAL const le_int32 * U_EXPORT2
pl_getVisualRunGlyphToCharMap(const pl_visualRun *run);

/**
 * A convenience method which returns the ascent value for the font
 * associated with this run.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the ascent value of this run's font.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getVisualRunAscent(const pl_visualRun *run);

/**
 * A convenience method which returns the descent value for the font
 * associated with this run.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the descent value of this run's font.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getVisualRunDescent(const pl_visualRun *run);

/**
 * A convenience method which returns the leading value for the font
 * associated with this run.
 *
 * @param run the <code>pl_visualRun</code> object.
 *
 * @return the leading value of this run's font.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
pl_getVisualRunLeading(const pl_visualRun *run);

#endif
#endif
