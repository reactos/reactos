
/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __GXLAYOUTENGINE_H
#define __GXLAYOUTENGINE_H

#include "LETypes.h"
#include "LayoutEngine.h"

#include "MorphTables.h"

U_NAMESPACE_BEGIN

class LEFontInstance;
class LEGlyphStorage;

/**
 * This class implements layout for QuickDraw GX or Apple Advanced Typograyph (AAT)
 * fonts. A font is a GX or AAT font if it contains a 'mort' table. See Apple's
 * TrueType Reference Manual (http://fonts.apple.com/TTRefMan/index.html) for details.
 * Information about 'mort' tables is in the chapter titled "Font Files."
 *
 * @internal
 */
class GXLayoutEngine : public LayoutEngine
{
public:
    /**
     * This is the main constructor. It constructs an instance of GXLayoutEngine for
     * a particular font, script and language. It takes the 'mort' table as a parameter since
     * LayoutEngine::layoutEngineFactory has to read the 'mort' table to know that it has a
     * GX font.
     *
     * Note: GX and AAT fonts don't contain any script and language specific tables, so
     * the script and language are ignored.
     *
     * @param fontInstance - the font
     * @param scriptCode - the script
     * @param langaugeCode - the language
     * @param morphTable - the 'mort' table
     *
     * @see LayoutEngine::layoutEngineFactory
     * @see ScriptAndLangaugeTags.h for script and language codes
     *
     * @internal
     */
    GXLayoutEngine(const LEFontInstance *fontInstance, le_int32 scriptCode, le_int32 languageCode, const MorphTableHeader *morphTable);

    /**
     * The destructor, virtual for correct polymorphic invocation.
     *
     * @internal
     */
    virtual ~GXLayoutEngine();

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
     * The address of the 'mort' table
     *
     * @internal
     */
    const MorphTableHeader *fMorphTable;

    /**
     * This method does GX layout using the font's 'mort' table. It converts the
     * input character codes to glyph indices using mapCharsToGlyphs, and then
     * applies the 'mort' table.
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
     * @internal
     */
    virtual le_int32 computeGlyphs(const LEUnicode chars[], le_int32 offset, le_int32 count, le_int32 max, le_bool rightToLeft,
        LEGlyphStorage &glyphStorage, LEErrorCode &success);

    /**
     * This method adjusts the glyph positions using the font's
     * 'kern', 'trak', 'bsln', 'opbd' and 'just' tables.
     *
     * Input parameters:
     * @param glyphStorage - the object holding the glyph storage. The positions will be updated as needed.
     *
     * Output parameters:
     * @param success - set to an error code if the operation fails
     *
     * @internal
     */
    virtual void adjustGlyphPositions(const LEUnicode chars[], le_int32 offset, le_int32 count, le_bool reverse,
                                      LEGlyphStorage &glyphStorage, LEErrorCode &success);

};

U_NAMESPACE_END
#endif

