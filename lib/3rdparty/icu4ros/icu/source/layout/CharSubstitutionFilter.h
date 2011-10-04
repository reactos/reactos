/*
 *
 * (C) Copyright IBM Corp. 1998-2004 - All Rights Reserved
 *
 */

#ifndef __CHARSUBSTITUTIONFILTER_H
#define __CHARSUBSTITUTIONFILTER_H

#include "LETypes.h"
#include "LEGlyphFilter.h"

U_NAMESPACE_BEGIN

class LEFontInstance;

/**
 * This filter is used by character-based GSUB processors. It
 * accepts only those characters which the given font can display.
 *
 * @internal
 */
class CharSubstitutionFilter : public UMemory, public LEGlyphFilter
{
private:
    /**
     * Holds the font which is used to test the characters.
     *
     * @internal
     */
    const LEFontInstance *fFontInstance;

    /**
     * The copy constructor. Not allowed!
     *
     * @internal
     */
    CharSubstitutionFilter(const CharSubstitutionFilter &other); // forbid copying of this class

    /**
     * The replacement operator. Not allowed!
     *
     * @internal
     */
    CharSubstitutionFilter &operator=(const CharSubstitutionFilter &other); // forbid copying of this class

public:
    /**
     * The constructor.
     *
     * @param fontInstance - the font to use to test the characters.
     *
     * @internal
     */
    CharSubstitutionFilter(const LEFontInstance *fontInstance);

    /**
     * The destructor.
     *
     * @internal
     */
    ~CharSubstitutionFilter();

    /**
     * This method is used to test if a particular
     * character can be displayed by the filter's
     * font.
     *
     * @param glyph - the Unicode character code to be tested
     *
     * @return TRUE if the filter's font can display this character.
     *
     * @internal
     */
    le_bool accept(LEGlyphID glyph) const;
};

U_NAMESPACE_END
#endif


