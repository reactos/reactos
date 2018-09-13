/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
/* @(#)Character.hxx    1.42 97/01/30
 */

#ifndef _CORE_LANG_CHARACTER
#define _CORE_LANG_CHARACTER


DEFINE_CLASS(Character);

/**
 */
class Character : public Base
{
    DECLARE_CLASS_MEMBERS(Character, Base);
    DECLARE_CLASS_CLONING(Character, Base);

    // cloning constructor, shouldn't do anything with data members...
    protected:  Character(CloningEnum e) : super(e) {}

    protected: Character()
    {
    }

    protected: Character(TCHAR c)
    {
        value = c;
    }

    public: DLLEXPORT static Character * newCharacter(TCHAR c);

    public: virtual int hashCode()
    {
        return (int)value;
    }

    public: TCHAR charValue()
    {
        return value;
    }

    public: virtual String * toString();

    /*
     */
    public: enum 
    {        
        UNASSIGNED        = 0,
        UPPERCASE_LETTER    = 1,
        LOWERCASE_LETTER    = 2,
        TITLECASE_LETTER    = 3,
        MODIFIER_LETTER        = 4,
        OTHER_LETTER        = 5,
        NON_SPACING_MARK    = 6,
        ENCLOSING_MARK        = 7,
        COMBINING_SPACING_MARK    = 8,
        DECIMAL_DIGIT_NUMBER    = 9,
        LETTER_NUMBER        = 10,
        OTHER_NUMBER        = 11,
        SPACE_SEPARATOR        = 12,
        LINE_SEPARATOR        = 13,
        PARAGRAPH_SEPARATOR    = 14,
        CONTROL            = 15,
        FORMAT            = 16,
        PRIVATE_USE        = 18,
        SURROGATE        = 19,
        DASH_PUNCTUATION    = 20,
        START_PUNCTUATION    = 21,
        END_PUNCTUATION        = 22,
        CONNECTOR_PUNCTUATION    = 23,
        OTHER_PUNCTUATION    = 24,
        MATH_SYMBOL        = 25,
        CURRENCY_SYMBOL        = 26,
        MODIFIER_SYMBOL        = 27,
        OTHER_SYMBOL        = 28
     };

   /**
     */
    public: static bool isLowerCase(TCHAR ch); 

   /**
     */
    public: static bool isUpperCase(TCHAR ch);

#if !USE_SHLWAPI
    /**
     */
    public: static bool isTitleCase(TCHAR ch);
#endif

    /**
     */
    public: DLLEXPORT static bool isDigit(TCHAR ch);

#if !USE_SHLWAPI
    /**
     */
    public: static bool isDefined(TCHAR ch);
#endif

    /**
     */
    public: DLLEXPORT static bool isLetter(TCHAR ch);

#if !USE_SHLWAPI
    /**
     */
    public: static bool isLetterOrDigit(TCHAR ch);

    /**
     */
    public: static bool isUnicodeIdentifierStart(TCHAR ch);

    /**
     */
    public: static bool isUnicodeIdentifierPart(TCHAR ch);

    /**
     */
    public: static bool isIdentifierIgnorable(TCHAR ch);
#endif

    /**
     */
    public: DLLEXPORT static TCHAR toLowerCase(TCHAR ch);

    /**
     */
    public: DLLEXPORT static TCHAR toUpperCase(TCHAR ch);

#if !USE_SHLWAPI
    /**
     */
    public: static TCHAR toTitleCase(TCHAR ch);

    /**
     */
    public: static int digit(TCHAR ch, int radix);

    /**
     */
    public: static int getNumericValue(TCHAR ch);
#endif

    /**
     */
    public: DLLEXPORT static bool isSpace(TCHAR ch);

    /**
     */
    public: static bool isSpaceChar(TCHAR ch);

    /**
     */
    public: DLLEXPORT static bool isWhitespace(TCHAR ch); 

#if !USE_SHLWAPI
    /**
     */
    public: static bool isISOControl(TCHAR ch);

    /**
     */
    public: static int getType(TCHAR ch);

    /**
     */
    public: static TCHAR forDigit(int digit, int radix);
#endif

    private: TCHAR value;

    /**
     */
#if !USE_SHLWAPI    
    public: static const int MIN_RADIX;

    /**
     */
    public: static const int MAX_RADIX;
#endif

    /**
     */
    public: static const TCHAR MIN_VALUE;

    /**
     */
    public: static const TCHAR MAX_VALUE;

#if !USE_SHLWAPI
    private: static const byte X[];
    private: static const byte Y[];
    private: static const int A[];
#endif
};


#endif _CORE_LANG_CHARACTER
