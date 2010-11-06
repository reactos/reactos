/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#ifndef __LOENGINE_H
#define __LOENGINE_H

#include "LETypes.h"

/**
 * \file 
 * \brief C API for complex text layout.
 * \internal
 *
 * This is a technology preview. The API may
 * change significantly.
 *
 */

/**
 * The opaque type for a LayoutEngine.
 *
 * @internal
 */
typedef void le_engine;

/**
 * The opaque type for a font instance.
 *
 * @internal
 */
typedef void le_font;

/**
 * This function returns an le_engine capable of laying out text
 * in the given font, script and langauge. Note that the LayoutEngine
 * returned may be a subclass of LayoutEngine.
 *
 * @param font - the font of the text
 * @param scriptCode - the script of the text
 * @param languageCode - the language of the text
 * @param typo_flags - flags that control layout features like kerning and ligatures.
 * @param success - output parameter set to an error code if the operation fails
 *
 * @return an le_engine which can layout text in the given font.
 *
 * @internal
 */
U_INTERNAL le_engine * U_EXPORT2
le_create(const le_font *font,
          le_int32 scriptCode,
          le_int32 languageCode,
          le_int32 typo_flags,
          LEErrorCode *success);

/**
 * This function closes the given LayoutEngine. After
 * it returns, the le_engine is no longer valid.
 *
 * @param engine - the LayoutEngine to close.
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
le_close(le_engine *engine);

/**
 * This routine will compute the glyph, character index and position arrays.
 *
 * @param engine - the LayoutEngine
 * @param chars - the input character context
 * @param offset - the offset of the first character to process
 * @param count - the number of characters to process
 * @param max - the number of characters in the input context
 * @param rightToLeft - TRUE if the characers are in a right to left directional run
 * @param x - the initial X position
 * @param y - the initial Y position
 * @param success - output parameter set to an error code if the operation fails
 *
 * @return the number of glyphs in the glyph array
 *
 * Note: The glyph, character index and position array can be accessed
 * using the getter routines below.
 *
 * Note: If you call this function more than once, you must call the reset()
 * function first to free the glyph, character index and position arrays
 * allocated by the previous call.
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
le_layoutChars(le_engine *engine,
               const LEUnicode chars[],
               le_int32 offset,
               le_int32 count,
               le_int32 max,
               le_bool rightToLeft,
               float x,
               float y,
               LEErrorCode *success);

/**
 * This function returns the number of glyphs in the glyph array. Note
 * that the number of glyphs will be greater than or equal to the number
 * of characters used to create the LayoutEngine.
 *
 * @param engine - the LayoutEngine
 * @param success - output parameter set to an error code if the operation fails.
 *
 * @return the number of glyphs in the glyph array
 *
 * @internal
 */
U_INTERNAL le_int32 U_EXPORT2
le_getGlyphCount(le_engine *engine,
                 LEErrorCode *success);

/**
 * This function copies the glyph array into a caller supplied array.
 * The caller must ensure that the array is large enough to hold all
 * the glyphs.
 *
 * @param engine - the LayoutEngine
 * @param glyphs - the destiniation glyph array
 * @param success - set to an error code if the operation fails
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
le_getGlyphs(le_engine *engine,
             LEGlyphID glyphs[],
             LEErrorCode *success);

/**
 * This function copies the character index array into a caller supplied array.
 * The caller must ensure that the array is large enough to hold a
 * character index for each glyph.
 *
 * @param engine - the LayoutEngine
 * @param charIndices - the destiniation character index array
 * @param success - set to an error code if the operation fails
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
le_getCharIndices(le_engine *engine,
                  le_int32 charIndices[],
                  LEErrorCode *success);

/**
 * This function copies the character index array into a caller supplied array.
 * The caller must ensure that the array is large enough to hold a
 * character index for each glyph.
 *
 * @param engine - the LayoutEngine
 * @param charIndices - the destiniation character index array
 * @param indexBase - an offset that will be added to each index.
 * @param success - set to an error code if the operation fails
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
le_getCharIndicesWithBase(le_engine *engine,
                  le_int32 charIndices[],
                  le_int32 indexBase,
                  LEErrorCode *success);

/**
 * This function copies the position array into a caller supplied array.
 * The caller must ensure that the array is large enough to hold an
 * X and Y position for each glyph, plus an extra X and Y for the
 * advance of the last glyph.
 *
 * @param engine - the LayoutEngine
 * @param positions - the destiniation position array
 * @param success - set to an error code if the operation fails
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
le_getGlyphPositions(le_engine *engine,
                     float positions[],
                     LEErrorCode *success);

/**
 * This function returns the X and Y position of the glyph at
 * the given index.
 *
 * Input parameters:
 * @param engine - the LayoutEngine
 * @param glyphIndex - the index of the glyph
 *
 * Output parameters:
 * @param x - the glyph's X position
 * @param y - the glyph's Y position
 * @param success - set to an error code if the operation fails
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
le_getGlyphPosition(le_engine *engine,
                    le_int32 glyphIndex,
                    float *x,
                    float *y,
                    LEErrorCode *success);

/**
 * This function frees the glyph, character index and position arrays
 * so that the LayoutEngine can be reused to layout a different
 * characer array. (This function is also called by le_close)
 *
 * @param engine - the LayoutEngine
 * @param success - set to an error code if the operation fails
 *
 * @internal
 */
U_INTERNAL void U_EXPORT2
le_reset(le_engine *engine,
         LEErrorCode *success);

#endif
