/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "loengine.h"
#include "LayoutEngine.h"

/**
 * \file 
 * \brief C API for complex text layout.
 */

U_NAMESPACE_USE

U_CAPI le_engine * U_EXPORT2
le_create(const le_font *font,
		  le_int32 scriptCode,
		  le_int32 languageCode,
		  le_int32 typo_flags,
		  LEErrorCode *success)
{
	LEFontInstance *fontInstance = (LEFontInstance *) font;

	return (le_engine *) LayoutEngine::layoutEngineFactory(fontInstance, scriptCode, languageCode, typo_flags, *success);
}

U_CAPI void U_EXPORT2
le_close(le_engine *engine)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	delete le;
}

U_CAPI le_int32 U_EXPORT2
le_layoutChars(le_engine *engine,
			   const LEUnicode chars[],
			   le_int32 offset,
			   le_int32 count,
			   le_int32 max,
			   le_bool rightToLeft,
			   float x,
			   float y,
			   LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return -1;
	}

	return le->layoutChars(chars, offset, count, max, rightToLeft, x, y, *success);
}

U_CAPI le_int32 U_EXPORT2
le_getGlyphCount(le_engine *engine,
				 LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return -1;
	}

	return le->getGlyphCount();
}

U_CAPI void U_EXPORT2
le_getGlyphs(le_engine *engine,
			 LEGlyphID glyphs[],
			 LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return;
	}

	le->getGlyphs(glyphs, *success);
}

U_CAPI void U_EXPORT2
le_getCharIndices(le_engine *engine,
				  le_int32 charIndices[],
				  LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return;
	}

	le->getCharIndices(charIndices, *success);
}

U_CAPI void U_EXPORT2
le_getCharIndicesWithBase(le_engine *engine,
				          le_int32 charIndices[],
				          le_int32 indexBase,
				          LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return;
	}

	le->getCharIndices(charIndices, indexBase, *success);
}

U_CAPI void U_EXPORT2
le_getGlyphPositions(le_engine *engine,
					 float positions[],
					 LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return;
	}

	le->getGlyphPositions(positions, *success);
}

U_CAPI void U_EXPORT2
le_getGlyphPosition(le_engine *engine,
					le_int32 glyphIndex,
					float *x,
					float *y,
					LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return;
	}

	le->getGlyphPosition(glyphIndex, *x, *y, *success);
}

U_CAPI void U_EXPORT2
le_reset(le_engine *engine,
		 LEErrorCode *success)
{
	LayoutEngine *le = (LayoutEngine *) engine;

	if (le == NULL) {
		*success = LE_ILLEGAL_ARGUMENT_ERROR;
		return;
	}

	le->reset();
}
