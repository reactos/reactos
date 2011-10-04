
/*
 *
 * (C) Copyright IBM Corp. 1998-2005 - All Rights Reserved
 *
 */

#ifndef __THAILAYOUTENGINE_H
#define __THAILAYOUTENGINE_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "LayoutEngine.h"

#include "ThaiShaping.h"

U_NAMESPACE_BEGIN

class LEGlyphStorage;

/**
 * This class implements layout for the Thai script, using the ThaiShapingClass.
 * All existing Thai fonts use an encoding which assigns character codes to all
 * the variant forms needed to display accents and tone marks correctly in context.
 * This class can deal with fonts using the Microsoft, Macintosh, and WorldType encodings.
 *
 * @internal
 */
class ThaiLayoutEngine : public LayoutEngine
{
public:
    /**
     * This constructs an instance of ThaiLayoutEngine for the given font, script and
     * language. It examines the font, using LEFontInstance::canDisplay, to set fGlyphSet
     * and fErrorChar. (see below)
     *
     * @param fontInstance - the font
     * @param scriptCode - the script
     * @param languageCode - the language
     *
     * @see LEFontInstance
     * @see ScriptAndLanguageTags.h for script and language codes
     *
     * @internal
     */
    ThaiLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, le_int32 typoFlags);

    /**
     * The destructor, virtual for correct polymorphic invocation.
     *
     * @internal
     */
    virtual ~ThaiLayoutEngine();

    /**
     * ICU "poor man's RTTI", returns a UClassID for the actual class.
     *
     * @stable ICU 2.8
     */
    virtual UClassID getDynamicClassID() const;

    /**
     * ICU "poor man's RTTI", returns a UClassID for this class.
     *
     * @stable ICU 2.8
     */
    static UClassID getStaticClassID();

protected:
    /**
     * A small integer indicating which Thai encoding
     * the font uses.
     *
     * @see ThaiShaping
     *
     * @internal
     */
    le_uint8 fGlyphSet;

    /**
     * The character used as a base for vowels and
     * tone marks that are out of sequence. Usually
     * this will be Unicode 0x25CC, if the font can
     * display it.
     *
     * @see ThaiShaping
     *
     * @internal
     */
    LEUnicode fErrorChar;

    /**
     * This method performs Thai layout. It calls ThaiShaping::compose to
     * generate the correct contextual character codes, and then calls
     * mapCharsToGlyphs to generate the glyph indices.
     *
     * Input parameters:
     * @param chars - the input character context
     * @param offset - the index of the first character to process
     * @param count - the number of characters to process
     * @param max - the number of characters in the input context
     * @param rightToLeft - <code>TRUE</code> if the text is in a right to left directional run
     * @param glyphStorage - the glyph storage object. The glyph and char index arrays will be set.
     *
     * Output parameters:
     * @param success - set to an error code if the operation fails
     *
     * @return the number of glyphs in the glyph index array
     *
     * @see ThaiShaping
     *
     * @internal
     */
    virtual le_int32 computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEGlyphStorage &glyphStorage, LEErrorCode &success);

};

U_NAMESPACE_END
#endif

