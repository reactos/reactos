
/*
 *
 * (C) Copyright IBM Corp. 1998-2006 - All Rights Reserved
 *
 */

#ifndef __HANGULAYOUTENGINE_H
#define __HANGULAYOUTENGINE_H

#include "LETypes.h"
#include "LEFontInstance.h"
#include "LEGlyphFilter.h"
#include "LayoutEngine.h"
#include "OpenTypeLayoutEngine.h"

#include "GlyphSubstitutionTables.h"
#include "GlyphDefinitionTables.h"
#include "GlyphPositioningTables.h"

U_NAMESPACE_BEGIN

class MPreFixups;
class LEGlyphStorage;

/**
 * This class implements OpenType layout for Old Hangul OpenType fonts, as
 * specified by Microsoft in "Creating and Supporting OpenType Fonts for
 * The Korean Hangul Script" (http://www.microsoft.com/typography/otfntdev/hangulot/default.htm)
 *
 * This class overrides the characterProcessing method to do Hangul character processing.
 * (See the MS spec. for more details)
 *
 * @internal
 */
class HangulOpenTypeLayoutEngine : public OpenTypeLayoutEngine
{
public:
    /**
     * This is the main constructor. It constructs an instance of HangulOpenTypeLayoutEngine for
     * a particular font, script and language. It takes the GSUB table as a parameter since
     * LayoutEngine::layoutEngineFactory has to read the GSUB table to know that it has an
     * Hangul OpenType font.
     *
     * @param fontInstance - the font
     * @param scriptCode - the script
     * @param langaugeCode - the language
     * @param gsubTable - the GSUB table
     *
     * @see LayoutEngine::layoutEngineFactory
     * @see OpenTypeLayoutEngine
     * @see ScriptAndLangaugeTags.h for script and language codes
     *
     * @internal
     */
    HangulOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
                            le_int32 typoFlags, const GlyphSubstitutionTableHeader *gsubTable);

    /**
     * This constructor is used when the font requires a "canned" GSUB table which can't be known
     * until after this constructor has been invoked.
     *
     * @param fontInstance - the font
     * @param scriptCode - the script
     * @param langaugeCode - the language
     *
     * @see OpenTypeLayoutEngine
     * @see ScriptAndLangaugeTags.h for script and language codes
     *
     * @internal
     */
    HangulOpenTypeLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode,
			      le_int32 typoFlags);

    /**
     * The destructor, virtual for correct polymorphic invocation.
     *
     * @internal
     */
   virtual ~HangulOpenTypeLayoutEngine();

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
     * This method does Hangul OpenType character processing. It assigns the OpenType feature
     * tags to the characters, and may compose a character sequence into a modern Hangul syllable,
     * or decompose a modern Hangul syllable if it forms part of an old Hangul syllable.
     *
     * Input parameters:
     * @param chars - the input character context
     * @param offset - the index of the first character to process
     * @param count - the number of characters to process
     * @param max - the number of characters in the input context
     * @param rightToLeft - <code>TRUE</code> if the characters are in a right to left directional run
     * @param glyphStorage - the glyph storage object. The glyph and character index arrays will be set.
     *                       the auxillary data array will be set to the feature tags.
     *
     * Output parameters:
     * @param success - set to an error code if the operation fails
     *
     * @return the output character count
     *
     * @internal
     */
    virtual le_int32 characterProcessing(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
            LEUnicode *&outChars, LEGlyphStorage &glyphStorage, LEErrorCode &success);
};

U_NAMESPACE_END
#endif

