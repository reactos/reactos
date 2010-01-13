/*
 *******************************************************************************
 *
 *   Copyright (C) 1999-2003, International Business Machines
 *   Corporation and others.  All Rights Reserved.
 *
 *******************************************************************************
 *   file name:  ScriptCompositeFontInstance.cpp
 *
 *   created on: 02/05/2003
 *   created by: Eric R. Mader
 */

#include "layout/LETypes.h"

#include "unicode/uscript.h"

#include "FontMap.h"

#include "ScriptCompositeFontInstance.h"

const char ScriptCompositeFontInstance::fgClassID=0;

ScriptCompositeFontInstance::ScriptCompositeFontInstance(FontMap *fontMap)
    : fFontMap(fontMap)
{
    // nothing else to do
}

ScriptCompositeFontInstance::~ScriptCompositeFontInstance()
{
    delete fFontMap;
    fFontMap = NULL;
}

void ScriptCompositeFontInstance::getGlyphAdvance(LEGlyphID glyph, LEPoint &advance) const
{
    LEErrorCode status = LE_NO_ERROR;
    le_int32 script = LE_GET_SUB_FONT(glyph);
    const LEFontInstance *font = fFontMap->getScriptFont(script, status);

    advance.fX = 0;
    advance.fY = 0;

    if (LE_SUCCESS(status)) {
        font->getGlyphAdvance(LE_GET_GLYPH(glyph), advance);
    }
}

le_bool ScriptCompositeFontInstance::getGlyphPoint(LEGlyphID glyph, le_int32 pointNumber, LEPoint &point) const
{
    LEErrorCode status = LE_NO_ERROR;
    le_int32 script = LE_GET_SUB_FONT(glyph);
    const LEFontInstance *font = fFontMap->getScriptFont(script, status);

    if (LE_SUCCESS(status)) {
        return font->getGlyphPoint(LE_GET_GLYPH(glyph), pointNumber, point);
    }

    return FALSE;
}

const LEFontInstance *ScriptCompositeFontInstance::getSubFont(const LEUnicode chars[], le_int32 *offset, le_int32 limit, le_int32 script, LEErrorCode &success) const
{
    if (LE_FAILURE(success)) {
        return NULL;
    }

    if (chars == NULL || *offset < 0 || limit < 0 || *offset >= limit || script < 0 || script >= scriptCodeCount) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return NULL;
    }

    const LEFontInstance *result = fFontMap->getScriptFont(script, success);

    if (LE_FAILURE(success)) {
        return NULL;
    }

    *offset = limit;
    return result;
}

// This is the really stupid version that doesn't look in any other font for the character...
// It would be better to also look in the "DEFAULT:" font. Another thing to do would be to
// look in all the fonts in some order, script code order being the most obvious...
LEGlyphID ScriptCompositeFontInstance::mapCharToGlyph(LEUnicode32 ch) const
{
    UErrorCode  error  = U_ZERO_ERROR;
    LEErrorCode status = LE_NO_ERROR;
    le_int32 script = uscript_getScript(ch, &error);
    const LEFontInstance *scriptFont = fFontMap->getScriptFont(script, status);
    LEGlyphID subFont = LE_SET_SUB_FONT(0, script);

    if (LE_FAILURE(status)) {
        return subFont;
    }

    LEGlyphID glyph = scriptFont->mapCharToGlyph(ch);

    return LE_SET_GLYPH(subFont, glyph);
}

