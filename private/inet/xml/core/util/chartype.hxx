/*
 * @(#)CharType.hxx 1.0 6/15/98
 * 
 * Character type constants and functions
 *
*  Copyright (C) 1998,1999 Microsoft Corporation. All rights reserved. * 
 */
 
#ifndef _CORE_UTIL_CHARTYPE_HXX
#define _CORE_UTIL_CHARTYPE_HXX


//==============================================================================

static const short TABLE_SIZE = 128;

enum
{
    FWHITESPACE    = 1,
    FDIGIT         = 2,
    FLETTER        = 4,
    FMISCNAME      = 8,
    FSTARTNAME     = 16,
    FCHARDATA      = 32
};

extern int g_anCharType[TABLE_SIZE];

inline int isWhiteSpace(WCHAR ch)
{
    return (ch < TABLE_SIZE) ? g_anCharType[ch] & FWHITESPACE : ::IsCharSpace(ch);
}

inline bool isLetter(WCHAR ch)
{
    return (ch >= 0x41) && ::IsCharAlpha(ch);
        // isBaseChar(ch) || isIdeographic(ch);
}

inline bool isAlphaNumeric(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39) || ((ch >= 0x41) && ::IsCharAlpha(ch));
        // isBaseChar(ch) || isIdeographic(ch);
}

inline bool isDigit(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39);
}

inline bool isHexDigit(WCHAR ch)
{
    return (ch >= 0x30 && ch <= 0x39) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

inline bool isCombiningChar(WCHAR ch)
{
    return false;
}

inline bool isExtender(WCHAR ch)
{
    return (ch == 0xb7);
}

inline int isNameChar(WCHAR ch)
{
    return  (ch < TABLE_SIZE ? (g_anCharType[ch] & (FLETTER | FDIGIT | FMISCNAME | FSTARTNAME)) :
              ( isAlphaNumeric(ch) || 
                ch == '-' ||  
                ch == '_' ||
                ch == '.' ||
                ch == ':' ||
                isCombiningChar(ch) ||
                isExtender(ch)));
}

inline int isStartNameChar(WCHAR ch)
{
    return  (ch < TABLE_SIZE) ? (g_anCharType[ch] & (FLETTER | FSTARTNAME))
        : (isLetter(ch) || (ch == '_' || ch == ':'));
        
}

inline int isCharData(WCHAR ch)
{
    // it is in the valid range if it is greater than or equal to
    // 0x20, or it is white space.
    return (ch < TABLE_SIZE) ?  (g_anCharType[ch] & FCHARDATA)
        : ((ch < 0xD800 && ch >= 0x20) ||   // Section 2.2 of spec.
            (ch >= 0xE000 && ch < 0xfffe));
}

// [13] PubidChar ::=  #x20 | #xD | #xA | [a-zA-Z0-9] | [-'()+,./:=?;!*#@$_%] Section 2.3 of spec
inline bool isPubidChar(WCHAR ch)
{
    return (ch > 0x1F && ch < 0x5B) || (ch > 0x60 && ch < 0x7B) || isWhiteSpace(ch);
}

const WCHAR * ParseName(const WCHAR *pwc, ULONG * pNamespaceLen);

const WCHAR * ParseNameToken(const WCHAR *pwc, ULONG * pNamespaceLen);

bool isValidName(const WCHAR *pwc, ULONG * pnColonPos);

bool isValidLanguageID(const WCHAR * pwcText, ULONG ulLen);

bool isValidPublicID(const WCHAR * pwcText, ULONG ulLen);

// resolve built-in entities.
WCHAR BuiltinEntity(const WCHAR* text, ULONG len);

HRESULT HexToUnicode(const WCHAR* text, ULONG len, WCHAR& ch);
HRESULT DecimalToUnicode(const WCHAR* text, ULONG len, WCHAR& ch);


#ifdef UNIX
    extern const unsigned char s_ByteOrderMarkTrident[sizeof(WCHAR)];
    extern const unsigned char s_ByteOrderMark[sizeof(WCHAR)];
    extern const unsigned char s_ByteOrderMarkUCS2[2];  // This is 2 for a reason!
#else
    extern const unsigned char s_ByteOrderMark[2];
    #define s_ByteOrderMarkTrident s_ByteOrderMark
    #define s_ByteOrderMarkUCS2    s_ByteOrderMark
#endif


#endif _CORE_UTIL_CHARTYPE_HXX
