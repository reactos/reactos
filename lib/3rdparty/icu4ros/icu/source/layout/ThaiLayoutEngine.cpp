
/*
 *
 * (C) Copyright IBM Corp. 1998-2007 - All Rights Reserved
 *
 */

#include "LETypes.h"
#include "LayoutEngine.h"
#include "ThaiLayoutEngine.h"
#include "ScriptAndLanguageTags.h"
#include "LEGlyphStorage.h"

#include "ThaiShaping.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ThaiLayoutEngine)

ThaiLayoutEngine::ThaiLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, le_int32 typoFlags)
    : LayoutEngine(fontInstance, scriptCode, languageCode, typoFlags)
{
    fErrorChar = 0x25CC;

    // Figure out which presentation forms the font uses
    if (! fontInstance->canDisplay(0x0E01)) {
        // No Thai in font; don't use presentation forms.
        fGlyphSet = 3;
    } else if (fontInstance->canDisplay(0x0E64)) {
        // WorldType uses reserved space in Thai block
        fGlyphSet = 0;
    } else if (fontInstance->canDisplay(0xF701)) {
        // Microsoft corporate zone
        fGlyphSet = 1;

        if (!fontInstance->canDisplay(fErrorChar)) {
            fErrorChar = 0xF71B;
        }
    } else if (fontInstance->canDisplay(0xF885)) {
        // Apple corporate zone
        fGlyphSet = 2;
    } else {
        // no presentation forms in the font
        fGlyphSet = 3;
    }
}

ThaiLayoutEngine::~ThaiLayoutEngine()
{
    // nothing to do
}

// Input: characters (0..max provided for context)
// Output: glyphs, char indices
// Returns: the glyph count
// NOTE: this assumes that ThaiShaping::compose will allocate the outChars array...
le_int32 ThaiLayoutEngine::computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool /*rightToLeft*/, LEGlyphStorage &glyphStorage, LEErrorCode &success)
{
    if (LE_FAILURE(success)) {
        return 0;
    }

    if (chars == NULL || offset < 0 || count < 0 || max < 0 || offset >= max || offset + count > max) {
        success = LE_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    LEUnicode *outChars;
    le_int32 glyphCount;
    
    // This is enough room for the worst-case expansion
    // (it says here...)
    outChars = LE_NEW_ARRAY(LEUnicode, count * 2);

    if (outChars == NULL) {
        success = LE_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    glyphStorage.allocateGlyphArray(count * 2, FALSE, success);

    if (LE_FAILURE(success)) {
        LE_DELETE_ARRAY(outChars);
        success = LE_MEMORY_ALLOCATION_ERROR;
        return 0;
    }

    glyphCount = ThaiShaping::compose(chars, offset, count, fGlyphSet, fErrorChar, outChars, glyphStorage);
    mapCharsToGlyphs(outChars, 0, glyphCount, FALSE, FALSE, glyphStorage, success);

    LE_DELETE_ARRAY(outChars);

    glyphStorage.adoptGlyphCount(glyphCount);
    return glyphCount;
}

U_NAMESPACE_END
