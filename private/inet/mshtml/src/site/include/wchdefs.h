/*
 *  @doc    INTERNAL
 *
 *  @module WCHDEFS.H -- Wide chararacter definitions for Trident
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      01/09/98     cthrash created
 *      01/23/98     a-pauln added complex script support
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I_WCHDEFS_H_
#define I_WCHDEFS_H_
#pragma INCMSG("--- Beg 'wchdefs.h'")

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

//
// BUGBUG (cthrash) characters marked with a ! need review
//

//
// UNICODE special characters for Trident.
//

//
// If you need to allocate a special character for Trident, redefine one of
// the WCH_UNUSED characters.  Do no redefine WCH_RESERVED characters, as
// this would break rendering of symbol fonts.  If you run out of special
// chars the first candidate to be removed from the RESERVED list is 008A
// (only breaks 1253+1255) and the next is 009A (breaks 1253+1255+1256).
//
// If you make any modification, you need to modify the abUsed table so
// that our IsSyntheticChar and IsValidWideChar functions continue to work.
// Note also that WCH_EMBEDDING must be the first non-reserved character.
//
// Here's a bit of an explanation: Although U+0080 through U+009F are defined
// to be control characters in Unicode, many codepages used codepoints in this
// range for roundtripping characters not in their codepage.  For example,
// windows-1252 does not have a glyph for MB 0x80, but if you convert this to
// WC, you'll get U+0080.  This is useful to know because someone might try
// to use that codepoint (especially in a symbol font) and don't want to
// reject them.  To accomodate as many codepages as possible, I've reserved
// all the unused glyphs in Windows-125x.  This should allow us to use symbol
// fonts in any of these codepages (with, of course, the exception of U+00A0,
// which we'll always treat as an NBSP even if the font has a non-spacing
// glyph.)  Any questions? I'm just e-mail away. (cthrash)
//

#undef WCH_EMBEDDING

#ifdef UNICODE
inline BOOL IsValidWideChar(TCHAR ch)
{
    return ch <= 0xffef;
}
#else
#define IsValidWideChar(ch) FALSE
#endif

// BUGBUG (cthrash) Needs review.
 
#define WCH_NULL                WCHAR(0x0000)
#define WCH_UNDEF               WCHAR(0x0001)
#define WCH_TAB                 WCHAR(0x0009)
#define WCH_LF                  WCHAR(0x000a)
#define WCH_CR                  WCHAR(0x000d)
#define WCH_SPACE               WCHAR(0x0020)
#define WCH_QUOTATIONMARK       WCHAR(0x0022)
#define WCH_AMPERSAND           WCHAR(0x0026)
#define WCH_APOSTROPHE          WCHAR(0x0027)
#define WCH_HYPHEN              WCHAR(0x002d)
#define WCH_DOT                 WCHAR(0x002e)
#define WCH_LESSTHAN            WCHAR(0x003c)
#define WCH_GREATERTHAN         WCHAR(0x003e)
#define WCH_NONBREAKSPACE       WCHAR(0x00a0) // &nbsp;
#define WCH_KASHIDA             WCHAR(0x0640)
#define WCH_ENQUAD              WCHAR(0x2000) 
#define WCH_EMQUAD              WCHAR(0x2001) 
#define WCH_ENSPACE             WCHAR(0x2002) // &ensp;
#define WCH_EMSPACE             WCHAR(0x2003) // &emsp;
#define WCH_THREE_PER_EM_SPACE  WCHAR(0x2004) 
#define WCH_FOUR_PER_EM_SPACE   WCHAR(0x2005) 
#define WCH_SIX_PER_EM_SPACE    WCHAR(0x2006) 
#define WCH_FIGURE_SPACE        WCHAR(0x2007) 
#define WCH_PUNCTUATION_SPACE   WCHAR(0x2008) 
#define WCH_NARROWSPACE         WCHAR(0x2009) // &thinsp;
#define WCH_NONREQHYPHEN        WCHAR(0x00AD) // &shy;
#define WCH_NONBREAKHYPHEN      WCHAR(0x2011)
#define WCH_FIGUREDASH          WCHAR(0x2012)
#define WCH_ENDASH              WCHAR(0x2013) // &ndash;
#define WCH_EMDASH              WCHAR(0x2014) // &mdash;
#define WCH_ZWSP                WCHAR(0x200b) // &zwsp; Zero width space
#define WCH_ZWNJ                WCHAR(0x200c) // &zwnj; Zero width non-joiner
#define WCH_ZWJ                 WCHAR(0x200d) // &zwj;  Zero width joiner
#define WCH_LRM                 WCHAR(0x200e) // &lrm;  Left-to-right mark
#define WCH_RLM                 WCHAR(0x200f) // &rlm;  Right-to-left mark
#define WCH_LQUOTE              WCHAR(0x2018) // &lsquo;
#define WCH_RQUOTE              WCHAR(0x2019) // &rsquo;
#define WCH_LDBLQUOTE           WCHAR(0x201c) // &ldquo;
#define WCH_RDBLQUOTE           WCHAR(0x201d) // &rdquo;
#define WCH_BULLET              WCHAR(0x2022) // &bull;
#define WCH_LRE                 WCHAR(0x202a) // &lre;  Left-to-right embedding
#define WCH_RLE                 WCHAR(0x202b) // &rle;  Right-to-left embedding
#define WCH_PDF                 WCHAR(0x202c) // &pdf;  Pop direction format
#define WCH_LRO                 WCHAR(0x202d) // &lro;  Left-to-right override
#define WCH_RLO                 WCHAR(0x202e) // &rlo;  Right-to-left override
#define WCH_ISS                 WCHAR(0x206a) // &iss;  Inhibit symmetric swapping
#define WCH_ASS                 WCHAR(0x206b) // &ass;  Activate symmetric swapping
#define WCH_IAFS                WCHAR(0x206c) // &iafs; Inhibit Arabic form shaping
#define WCH_AAFS                WCHAR(0x206d) // &aafx; Activate Arabic form shaping
#define WCH_NADS                WCHAR(0x206e) // &nads; National digit shapes
#define WCH_NODS                WCHAR(0x206f) // &nods; Nominal digit shapes
#define WCH_EURO                WCHAR(0x20ac) // &euro;
#define WCH_FESPACE             WCHAR(0x3000)
#define WCH_UTF16_HIGH_FIRST    WCHAR(0xd800)
#define WCH_UTF16_HIGH_LAST     WCHAR(0xdbff)
#define WCH_UTF16_LOW_FIRST     WCHAR(0xdc00)
#define WCH_UTF16_LOW_LAST      WCHAR(0xdfff)
#define WCH_ZWNBSP              WCHAR(0xfeff) // aka BOM (Byte Order Mark)
#define WCH_SYNTHETICLINEBREAK  WCHAR(0xfffa)
#define WCH_SYNTHETICBLOCKBREAK WCHAR(0xfffb)
#define WCH_SYNTHETICEMBEDDING  WCHAR(0xfffc) // Unicode 2.x, ala RichEdit
#define WCH_SYNTHETICTXTSITEBREAK WCHAR(0xfffd)
#define WCH_NODE                WCHAR(0xfffe)


//
// Trident Aliases
//

#define WCH_WORDBREAK          WCH_ZWSP      // We treat <WBR>==&zwsp;

//
// Line Services Aliases
//

#define WCH_ENDPARA1           WCH_CR
#define WCH_ENDPARA2           WCH_LF
#define WCH_ALTENDPARA         WCH_SYNTHETICBLOCKBREAK
#define WCH_ENDLINEINPARA      WCH_SYNTHETICLINEBREAK
#define WCH_COLUMNBREAK        WCH_UNDEF
#define WCH_SECTIONBREAK       WCH_SYNTHETICTXTSITEBREAK // zero-width
#define WCH_PAGEBREAK          WCH_UNDEF
#define WCH_OPTBREAK           WCH_UNDEF
#define WCH_NOBREAK            WCH_ZWNBSP
#define WCH_TOREPLACE          WCH_UNDEF
#define WCH_REPLACE            WCH_UNDEF

//
// Line Services Visi support (Not currently used by Trident)
//

#define WCH_VISINULL           WCHAR(0x2050) // !
#define WCH_VISIALTENDPARA     WCHAR(0x2051) // !
#define WCH_VISIENDLINEINPARA  WCHAR(0x2052) // !
#define WCH_VISIENDPARA        WCHAR(0x2053) // !
#define WCH_VISISPACE          WCHAR(0x2054) // !
#define WCH_VISINONBREAKSPACE  WCHAR(0x2055) // !
#define WCH_VISINONBREAKHYPHEN WCHAR(0x2056) // !
#define WCH_VISINONREQHYPHEN   WCHAR(0x2057) // !
#define WCH_VISITAB            WCHAR(0x2058) // !
#define WCH_VISIEMSPACE        WCHAR(0x2059) // !
#define WCH_VISIENSPACE        WCHAR(0x205a) // !
#define WCH_VISINARROWSPACE    WCHAR(0x205b) // !
#define WCH_VISIOPTBREAK       WCHAR(0x205c) // !
#define WCH_VISINOBREAK        WCHAR(0x205d) // !
#define WCH_VISIFESPACE        WCHAR(0x205e) // !

//
// Line Services installed object handler support
//

#define WCH_ESCRUBY            WCHAR(0xfff0) // !
#define WCH_ESCMAIN            WCHAR(0xfff1) // !
#define WCH_ENDTATENAKAYOKO    WCHAR(0xfff2) // !
#define WCH_ENDHIH             WCHAR(0xfff3) // !
#define WCH_ENDFIRSTBRACKET    WCHAR(0xfff4) // !
#define WCH_ENDTEXT            WCHAR(0xfff5) // !
#define WCH_ENDWARICHU         WCHAR(0xfff6) // !
#define WCH_ENDREVERSE         WCHAR(0xfff7) // !
#define WCH_REVERSE            WCHAR(0xfff8) // !
#define WCH_NOBRBLOCK          WCHAR(0xfff9) // !

//
// Line Services autonumbering support
//

#define WCH_ESCANMRUN          WCH_NOBRBLOCK // !

//
// Hanguel Syllable/Jamo range specification
//

#define WCH_HANGUL_START       WCHAR(0xac00)
#define WCH_HANGUL_END         WCHAR(0xd7ff)
#define WCH_JAMO_START         WCHAR(0x3131)
#define WCH_JAMO_END           WCHAR(0x318e)

inline BOOL IsKorean(TCHAR ch)
{
#ifdef UNICODE
    return InRange( ch, WCH_HANGUL_START, WCH_HANGUL_END ) ||
           InRange( ch, WCH_JAMO_START, WCH_JAMO_END );
#else
    return FALSE;
#endif
}

//
// "Far East" characters (implemented in intl.cxx)
//

BOOL IsFEChar(TCHAR ch);

//
// ASCII
//

inline BOOL IsAscii(TCHAR ch)
{
    return ch < 128;
}

//
// End-User Defined Characters (EUDC) code range
// This range corresponds to the Unicode Private Use Area
//
// Usage: Japanese:             U+E000-U+E757
//        Simplified Chinese:   U+E000-U+E4DF  
//        Traditional Chinese:  U+E000-U+F8FF
//        Korean:               U+E000-U+E0BB
//

#define WCH_EUDC_FIRST   WCHAR(0xE000)
#define WCH_EUDC_LAST    WCHAR(0xF8FF)

inline BOOL IsEUDCChar(TCHAR ch)
{
    return InRange( ch, WCH_EUDC_FIRST, WCH_EUDC_LAST );
}

// Non-breaking space

#ifndef WCH_NBSP
  #ifdef WIN16
    #define WCH_NBSP           '\xA0'
  #else
    #define WCH_NBSP           TCHAR(0x00A0)
  #endif
#endif

//
// UNICODE characters in complex scripts
//
// Hebrew
// Arabic
// Devanagari
// Bengali
// Gurmukhi
// Gujarati
// Oriya
// Tamil
// Telugu
// Kannada
// Malayalam
// Thai
// Lao
// Tibetan

BOOL
inline IsComplexScriptChar(
    TCHAR ch)
{
    if (InRange(ch, 0x0590, 0x0FBF))
    {
        if (InRange(ch, 0x0590, 0x06FF))
        {
            return TRUE;
        }
        else if (InRange(ch, 0x0900, 0x0D7F))
        {
            return TRUE;
        }
        else if (InRange(ch, 0x0E00, 0x0FBF))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//
// UNICODE characters which control formatting
//
// WCH_ZWNJ
// WCH_ZWJ
// WCH_LRM
// WCH_RLM
// WCH_LRE
// WCH_RLE
// WCH_PDF
// WCH_LRO
// WCH_RLO
// WCH_ISS
// WCH_ASS
// WCH_IAFS
// WCH_AAFS
// WCH_NADS
// WCH_NODS

inline BOOL
IsFormattingControlChar(
    TCHAR ch)
{
    static const DWORD s_adwComplexPunct[] =
    {
        0x0000F000, // ZWNJ, ZWJ, LRM, RLM
        0x00007600, // LRE, RLE, PDF, LRO, RLO
        0x00000000,
        0x0000F600  // ISS, ASS, IAFS, AAFS, NADS, NODS
    };

    if (InRange(ch, 0x2000, 0x206F))
    {
        if (s_adwComplexPunct[(ch >> 5) & 0x0003] & (1 << (ch & 0x001F)))
        {
            return TRUE;
        }
    }
    return FALSE;
}

//
// UNICODE characters which control bidi embedding
//
// WCH_LRE
// WCH_RLE
// WCH_PDF
// WCH_LRO
// WCH_RLO

inline BOOL
IsBidiEmbeddingChar(
    TCHAR ch)
{
    return InRange(ch, WCH_LRE, WCH_RLO);
}

//
// UNICODE characters which symmetric swapping
//
// WCH_ISS
// WCH_ASS

inline BOOL
IsSymSwapControl(
    TCHAR ch)
{
    return ((ch | 0x0001) == WCH_ASS);
}

//
// UNICODE characters which control shaping of Arabic presentation form
// characters
//
// WCH_IAFS
// WCH_AAFS

inline BOOL
IsArabicShapingControl(
    TCHAR ch)
{
    return ((ch | 0x0001) == WCH_AAFS);
}

//
// UNICODE characters which digit shapes
//
// WCH_NADS
// WCH_NODS

inline BOOL
IsDigitShapeControl(
    TCHAR ch)
{
    return ((ch | 0x0001) == WCH_NODS);
}


//
// UNICODE surrogate range for UTF-16 support
//
// High Surrogate D800-DBFF
// Low Surrogate  DC00-DFFF
//

inline BOOL
IsSurrogateChar(TCHAR ch)
{
    return InRange( ch, WCH_UTF16_HIGH_FIRST, WCH_UTF16_LOW_LAST );
}

inline BOOL
IsHighSurrogateChar(TCHAR ch)
{
    return InRange( ch, WCH_UTF16_HIGH_FIRST, WCH_UTF16_HIGH_LAST );
}

inline BOOL
IsLowSurrogateChar(TCHAR ch)
{
    return InRange( ch, WCH_UTF16_LOW_FIRST, WCH_UTF16_LOW_LAST );

}

inline WCHAR
HighSurrogateCharFromUcs4(DWORD ch)
{
    return 0xd800 + ((ch - 0x10000) >> 10);
}

inline WCHAR
LowSurrogateCharFromUcs4(DWORD ch)
{
    return 0xdc00 + (ch & 0x3ff);
}

//
// Quick lookup table for Windows-1252 to Latin-1 conversion in the 0x80-0x9f range
// The data resides in mshtml\src\site\util\intl.cxx
//

extern const WCHAR g_achLatin1MappingInUnicodeControlArea[32];

#ifdef __cplusplus
}
#endif // __cplusplus

#pragma INCMSG("--- End 'wchdefs.h'")
#else
#pragma INCMSG("*** Dup 'wchdefs.h'")
#endif
