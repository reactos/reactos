/****************************** Module Header ******************************\
* Module Name: drawtext.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains common text drawing functions.
*
* History:
* 02-12-92 mikeke   Moved Drawtext to the client side
\***************************************************************************/


/***************************************************************************\
* Define some macros to test the format flags. We won't support them all
* on the kernel-mode side, since they're not all needed there.
\***************************************************************************/
#ifdef _USERK_
    #define CALCRECT(wFormat)               FALSE
    #define EDITCONTROL(wFormat)            FALSE
    #define EXPANDTABS(wFormat)             FALSE
    #define EXTERNALLEADING(wFormat)        FALSE
    #define MODIFYSTRING(wFormat)           FALSE
    #define NOPREFIX(wFormat)               TRUE
    #define PATHELLIPSIS(wFormat)           FALSE
    #define SINGLELINE(wFormat)             TRUE
    #define TABSTOP(wFormat)                FALSE
    #define WORDBREAK(wFormat)              FALSE
    #define WORDELLIPSIS(wFormat)           FALSE
    #define NOFULLWIDTHCHARBREAK(dwFormat)  FALSE
#else
    #define CALCRECT(wFormat)               (wFormat & DT_CALCRECT)
    #define EDITCONTROL(wFormat)            (wFormat & DT_EDITCONTROL)
    #define EXPANDTABS(wFormat)             (wFormat & DT_EXPANDTABS)
    #define EXTERNALLEADING(wFormat)        (wFormat & DT_EXTERNALLEADING)
    #define MODIFYSTRING(wFormat)           (wFormat & DT_MODIFYSTRING)
    #define NOPREFIX(wFormat)               (wFormat & DT_NOPREFIX)
    #define PATHELLIPSIS(wFormat)           (wFormat & DT_PATH_ELLIPSIS)
    #define SINGLELINE(wFormat)             (wFormat & DT_SINGLELINE)
    #define TABSTOP(wFormat)                (wFormat & DT_TABSTOP)
    #define WORDBREAK(wFormat)              (wFormat & DT_WORDBREAK)
    #define WORDELLIPSIS(wFormat)           (wFormat & DT_WORD_ELLIPSIS)
    // Note: DT_NOFULLWIDTHCHARBREAK exceeds WORD limit. Use dwFormat
    //  rather than wFormat.
    #define NOFULLWIDTHCHARBREAK(dwFormat)  (dwFormat & DT_NOFULLWIDTHCHARBREAK)
#endif
#define ENDELLIPSIS(wFormat)        (wFormat & DT_END_ELLIPSIS)
#define NOCLIP(wFormat)             (wFormat & DT_NOCLIP)
#define RTLREADING(wFormat)         (wFormat & DT_RTLREADING)
#define HIDEPREFIX(wFormat)         (wFormat & DT_HIDEPREFIX)

/***************************************************************************\
* Stuff used in DrawText code
\***************************************************************************/

#define CR 13
#define LF 10
#define DT_HFMTMASK 0x03
#define DT_VFMTMASK 0x0C
#define ETO_OPAQUEFGND 0x0A

static CONST WCHAR szEllipsis[CCHELLIPSIS+1] = TEXT("...");

extern HDC    ghdcBits2;

/* Max length of a full path is around 260. But, most of the time, it will
 * be less than 128. So, we alloc only this much on stack. If the string is
 * longer, we alloc from local heap (which is slower).
 *
 * BOGUS: For international versions, we need to give some more margin here.
 */
#define MAXBUFFSIZE     128

/***************************************************************************\
*  There are word breaking characters which are compatible with
* Japanese Windows 3.1 and FarEast Windows 95.
*
*  SJ - Country Japan , Charset SHIFTJIS, Codepage  932.
*  GB - Country PRC   , Charset GB2312  , Codepage  936.
*  B5 - Country Taiwan, Charset BIG5    , Codepage  950.
*  WS - Country Korea , Charset WANGSUNG, Codepage  949.
*  JB - Country Korea , Charset JOHAB   , Codepage 1361. *** LATER ***
*
* [START BREAK CHARACTERS]
*
*   These character should not be the last charatcer of the line.
*
*  Unicode   Japan      PRC     Taiwan     Korea
*  -------+---------+---------+---------+---------+
*
* + ASCII
*
*   U+0024 (SJ+0024)                     (WS+0024) Dollar sign
*   U+0028 (SJ+0028)                     (WS+0028) Opening parenthesis
*   U+003C (SJ+003C)                               Less-than sign
*   U+005C (SJ+005C)                               Backslash
*   U+005B (SJ+005B) (GB+005B)           (WS+005B) Opening square bracket
*   U+007B (SJ+007B) (GB+007B)           (WS+007B) Opening curly bracket
*
* + General punctuation
*
*   U+2018                               (WS+A1AE) Single Turned Comma Quotation Mark
*   U+201C                               (WS+A1B0) Double Comma Quotation Mark
*
* + CJK symbols and punctuation
*
*   U+3008                               (WS+A1B4) Opening Angle Bracket
*   U+300A (SJ+8173)                     (WS+A1B6) Opening Double Angle Bracket
*   U+300C (SJ+8175)                     (WS+A1B8) Opening Corner Bracket
*   U+300E (SJ+8177)                     (WS+A1BA) Opening White Corner Bracket
*   U+3010 (SJ+9179)                     (WS+A1BC) Opening Black Lenticular Bracket
*   U+3014 (SJ+816B)                     (WS+A1B2) Opening Tortoise Shell Bracket
*
* + Fullwidth ASCII variants
*
*   U+FF04                               (WS+A3A4) Fullwidth Dollar Sign
*   U+FF08 (SJ+8169)                     (WS+A3A8) Fullwidth opening parenthesis
*   U+FF1C (SJ+8183)                               Fullwidth less-than sign
*   U+FF3B (SJ+816D)                     (WS+A3DB) Fullwidth opening square bracket
*   U+FF5B (SJ+816F)                     (WS+A3FB) Fullwidth opening curly bracket
*
* + Halfwidth Katakana variants
*
*   U+FF62 (SJ+00A2)                               Halfwidth Opening Corner Bracket
*
* + Fullwidth symbol variants
*
*   U+FFE1                               (WS+A1CC) Fullwidth Pound Sign
*   U+FFE6                               (WS+A3DC) Fullwidth Won Sign
*
* [END BREAK CHARACTERS]
*
*   These character should not be the top charatcer of the line.
*
*  Unicode   Japan      PRC     Taiwan     Korea
*  -------+---------+---------+---------+---------+
*
* + ASCII
*
*   U+0021 (SJ+0021) (GB+0021) (B5+0021) (WS+0021) Exclamation mark
*   U+0025                               (WS+0025) Percent Sign
*   U+0029 (SJ+0029)                     (WS+0029) Closing parenthesis
*   U+002C (SJ+002C) (GB+002C) (B5+002C) (WS+002C) Comma
*   U+002E (SJ+002E) (GB+002E) (B5+002E) (WS+002E) Priod
*   U+003A                               (WS+003A) Colon
*   U+003B                               (WS+003B) Semicolon
*   U+003E (SJ+003E)                               Greater-than sign
*   U+003F (SJ+003F) (GB+003F) (B5+003F) (WS+003F) Question mark
*   U+005D (SJ+005D) (GB+005D) (B5+005D) (WS+005D) Closing square bracket
*   U+007D (SJ+007D) (GB+007D) (B5+007D) (WS+007D) Closing curly bracket
*
* + Latin1
*
*   U+00A8           (GB+A1A7)                     Spacing diaeresis
*   U+00B0                               (WS+A1C6) Degree Sign
*   U+00B7                     (B5+A150)           Middle Dot
*
* + Modifier letters
*
*   U+02C7           (GB+A1A6)                     Modifier latter hacek
*   U+02C9           (GB+A1A5)                     Modifier letter macron
*
* + General punctuation
*
*   U+2013                     (B5+A156)           En Dash
*   U+2014                     (b5+A158)           Em Dash
*   U+2015           (GB+A1AA)                     Quotation dash
*   U+2016           (GB+A1AC)                     Double vertical bar
*   U+2018           (GB+A1AE)                     Single turned comma quotation mark
*   U+2019           (GB+A1AF) (B5+A1A6) (WS+A1AF) Single comma quotation mark
*   U+201D           (GB+A1B1) (B5+A1A8) (WS+A1B1) Double comma quotation mark
*   U+2022           (GB+A1A4)                     Bullet
*   U+2025                     (B5+A14C)           Two Dot Leader
*   U+2026           (GB+A1AD) (B5+A14B)           Horizontal ellipsis
*   U+2027                     (B5+A145)           Hyphenation Point
*   U+2032                     (B5+A1AC) (WS+A1C7) Prime
*   U+2033                               (WS+A1C8) Double Prime
*
* + Letterlike symbols
*
*   U+2103                               (WS+A1C9) Degrees Centigrade
*
* + Mathemetical opetartors
*
*   U+2236           (GB+A1C3)                     Ratio
*
* + Form and Chart components
*
*   U+2574                     (B5+A15A)           Forms Light Left
*
* + CJK symbols and punctuation
*
*   U+3001 (SJ+8141) (GB+A1A2) (B5+A142)           Ideographic comma
*   U+3002 (SJ+8142) (GB+A1A3) (B5+A143)           Ideographic period
*   U+3003           (GB+A1A8)                     Ditto mark
*   U+3005           (GB+A1A9)                     Ideographic iteration
*   U+3009           (GB+A1B5) (B5+A172) (WS+A1B5) Closing angle bracket
*   U+300B (SJ+8174) (GB+A1B7) (B5+A16E) (WS+A1B7) Closing double angle bracket
*   U+300D (SJ+8176) (GB+A1B9) (B5+A176) (WS+A1B9) Closing corner bracket
*   U+300F (SJ+8178) (GB+A1BB) (B5+A17A) (WS+A1BB) Closing white corner bracket
*   U+3011 (SJ+817A) (GB+A1BF) (B5+A16A) (WS+A1BD) Closing black lenticular bracket
*   U+3015 (SJ+816C) (GB+A1B3) (B5+A166) (WS+A1B3) Closing tortoise shell bracket
*   U+3017           (GB+A1BD)                     Closing white lenticular bracket
*   U+301E                     (B5+A1AA)           Double Prime Quotation Mark
*
* + Hiragana
*
*   U+309B (SJ+814A)                               Katakana-Hiragana voiced sound mark
*   U+309C (SJ+814B)                               Katakana-Hiragana semi-voiced sound mark
*
* + CNS 11643 compatibility
*
*   U+FE30                     (B5+A14A)           Glyph for Vertical 2 Dot Leader
*   U+FE31                     (B5+A157)           Glyph For Vertical Em Dash
*   U+FE33                     (B5+A159)           Glyph for Vertical Spacing Underscore
*   U+FE34                     (B5+A15B)           Glyph for Vertical Spacing Wavy Underscore
*   U+FE36                     (B5+A160)           Glyph For Vertical Closing Parenthesis
*   U+FE38                     (B5+A164)           Glyph For Vertical Closing Curly Bracket
*   U+FE3A                     (B5+A168)           Glyph For Vertical Closing Tortoise Shell Bracket
*   U+FE3C                     (B5+A16C)           Glyph For Vertical Closing Black Lenticular Bracket
*   U+FE3E                     (B5+A16E)           Closing Double Angle Bracket
*   U+FE40                     (B5+A174)           Glyph For Vertical Closing Angle Bracket
*   U+FE42                     (B5+A178)           Glyph For Vertical Closing Corner Bracket
*   U+FE44                     (B5+A17C)           Glyph For Vertical Closing White Corner Bracket
*   U+FE4F                     (B5+A15C)           Spacing Wavy Underscore
*
* + Small variants
*
*   U+FE50                     (B5+A14D)           Small Comma
*   U+FE51                     (B5+A14E)           Small Ideographic Comma
*   U+FE52                     (B5+A14F)           Small Period
*   U+FE54                     (B5+A151)           Small Semicolon
*   U+FE55                     (B5+A152)           Small Colon
*   U+FE56                     (B5+A153)           Small Question Mark
*   U+FE57                     (B5+A154)           Small Exclamation Mark
*   U+FE5A                     (B5+A17E)           Small Closing Parenthesis
*   U+FE5C                     (B5+A1A2)           Small Closing Curly Bracket
*   U+FE5E                     (B5+A1A4)           Small Closing Tortoise Shell Bracket
*
* + Fullwidth ASCII variants
*
*   U+FF01 (SJ+8149) (GB+A3A1) (B5+A149) (WS+A3A1) Fullwidth exclamation mark
*   U+FF02           (GB+A3A2)                     Fullwidth Quotation mark
*   U+FF05                               (WS+A3A5) Fullwidth Percent Sign
*   U+FF07           (GB+A3A7)                     Fullwidth Apostrophe
*   U+FF09 (SJ+816A) (GB+A3A9) (B5+A15E) (WS+A3A9) Fullwidth Closing parenthesis
*   U+FF0C (SJ+8143) (GB+A3AC) (B5+A141) (WS+A3AC) Fullwidth comma
*   U+FF0D           (GB+A3AD)                     Fullwidth Hyphen-minus
*   U+FF0E (SJ+8144)           (B5+A144) (WS+A3AE) Fullwidth period
*   U+FF1A           (GB+A3BA) (B4+A147) (WS+A3BA) Fullwidth colon
*   U+FF1B           (GB+A3BB) (B5+A146) (WS+A3BB) Fullwidth semicolon
*   U+FF1E (SJ+8184)                               Fullwidth Greater-than sign
*   U+FF1F (SJ+8148) (GB+A3BF) (B5+A148) (WS+A3BF) Fullwidth question mark
*   U+FF3D (SJ+816E) (GB+A3DD)           (WS+A3DD) Fullwidth Closing square bracket
*   U+FF5C                     (B5+A155)           Fullwidth Vertical Bar
*   U+FF5D (SJ+8170)           (B5+A162) (WS+A3FD) Fullwidth Closing curly bracket
*   U+FF5E           (GB+A1AB)                     Fullwidth Spacing tilde
*
* + Halfwidth Katakana variants
*
*   U+FF61 (SJ+00A1)                               Halfwidth Ideographic period
*   U+FF63 (SJ+00A3)                               Halfwidth Closing corner bracket
*   U+FF64 (SJ+00A4)                               Halfwidth Ideographic comma
*   U+FF9E (SJ+00DE)                               Halfwidth Katakana voiced sound mark
*   U+FF9F (SJ+00DF)                               Halfwidth Katakana semi-voiced sound mark
*
* + Fullwidth symbol variants
*
*   U+FFE0                               (WS+A1CB) Fullwidth Cent Sign
*
\***************************************************************************/

#if 0   // not currently used --- FYI only
/***************************************************************************\
* Start Break table
*  These character should not be the last charatcer of the line.
\***************************************************************************/

CONST BYTE aASCII_StartBreak[] = {
/* 00       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 2X */                1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
/* 6X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 7X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
};

CONST BYTE aCJKSymbol_StartBreak[] = {
/* 30       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */                            1, 0, 1, 0, 1, 0, 1, 0,
/* 1X */    1, 0, 0, 0, 1
};

CONST BYTE aFullWidthHalfWidthVariants_StartBreak[] = {
/* FF       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */                1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
/* 1X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
/* 2X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
/* 6X */    0, 0, 1
};
#endif

/***************************************************************************\
* End Break table.
*  These character should not be the top charatcer of the line.
\***************************************************************************/

CONST BYTE aASCII_Latin1_EndBreak[] = {
/* 00       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 2X */       1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
/* 6X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 7X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
/* 8X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* AX */    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,
/* BX */    1, 0, 0, 0, 0, 0, 0, 1
};

CONST BYTE aGeneralPunctuation_EndBreak[] = {
/* 20       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 1X */             1, 1, 1, 1, 0, 1, 1, 0, 0, 0, 1, 0, 0,
/* 2X */    0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 1, 1
};

CONST BYTE aCJKSymbol_EndBreak[] = {
/* 30       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */       1, 1, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1,
/* 1X */    0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1
};

CONST BYTE aCNS11643_SmallVariants_EndBreak[] = {
/* FE       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 3X */    1, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0,
/* 4X */    1, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
/* 5X */    1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1
};

CONST BYTE aFullWidthHalfWidthVariants_EndBreak[] = {
/* FF       0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
/* 0X */       1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 0,
/* 1X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1,
/* 2X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 3X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
/* 4X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 5X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0,
/* 6X */    0, 1, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 7X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 8X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/* 9X */    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1
};

/***************************************************************************\
*  UserIsFELineBreak() - Detects Far East word breaking characters.         *
*                                                                           *
* History:                                                                  *
* 10-Mar-1996 HideyukN  Created.                                            *
\***************************************************************************/

#if 0   // not currently used --- FYI only
BOOL UserIsFELineBreakStart(WCHAR wch)
{
    switch (wch>>8) {
        case 0x00:
            //
            // Check if word breaking chars in ASCII.
            //
            if ((wch >= 0x0024) && (wch <= 0x007B))
                return((BOOL)(aASCII_StartBreak[wch - 0x0024]));
            else
                return(FALSE);

        case 0x20:
            //
            // Check if work breaking chars in "General punctuation"
            //
            if ((wch == 0x2018) || (wch == 0x201C))
                return(TRUE);
            else
                return(FALSE);

        case 0x30:
            //
            // Check if word breaking chars in "CJK symbols and punctuation"
            // and Hiragana.
            //
            if ((wch >= 0x3008) && (wch <= 0x3014))
                return((BOOL)(aCJKSymbol_StartBreak[wch - 0x3008]));
            else
                return(FALSE);

        case 0xFF:
            //
            // Check if word breaking chars in "Fullwidth ASCII variants",
            // "Halfwidth Katakana variants" or "Fullwidth Symbol variants".
            //
            if ((wch >= 0xFF04) && (wch <= 0xFF62))
                return((BOOL)(aFullWidthHalfWidthVariants_StartBreak[wch - 0xFF04]));
            else if ((wch == 0xFFE1) || (wch == 0xFFE6))
                return(TRUE);
            else
                return(FALSE);

        default:
            return(FALSE);
    }
}
#endif

BOOL UserIsFELineBreakEnd(WCHAR wch)
{
    switch (wch>>8) {
        case 0x00:
            //
            // Check if word breaking chars in ASCII or Latin1.
            //
            if ((wch >= 0x0021) && (wch <= 0x00B7))
                return((BOOL)(aASCII_Latin1_EndBreak[wch - 0x0021]));
            else
                return(FALSE);

        case 0x02:
            //
            // Check if work breaking chars in "Modifier letters"
            //
            if ((wch == 0x02C7) || (wch == 0x02C9))
                return(TRUE);
            else
                return(FALSE);

        case 0x20:
            //
            // Check if work breaking chars in "General punctuation"
            //
            if ((wch >= 0x2013) && (wch <= 0x2033))
                return((BOOL)(aGeneralPunctuation_EndBreak[wch - 0x2013]));
            else
                return(FALSE);

        case 0x21:
            //
            // Check if work breaking chars in "Letterlike symbols"
            //
            if (wch == 0x2103)
                return(TRUE);
            else
                return(FALSE);

        case 0x22:
            //
            // Check if work breaking chars in "Mathemetical opetartors"
            //
            if (wch == 0x2236)
                return(TRUE);
            else
                return(FALSE);

        case 0x25:
            //
            // Check if work breaking chars in "Form and Chart components"
            //
            if (wch == 0x2574)
                return(TRUE);
            else
                return(FALSE);

        case 0x30:
            //
            // Check if word breaking chars in "CJK symbols and punctuation"
            // and Hiragana.
            //
            if ((wch >= 0x3001) && (wch <= 0x301E))
                return((BOOL)(aCJKSymbol_EndBreak[wch - 0x3001]));
            else if ((wch == 0x309B) || (wch == 0x309C))
                return(TRUE);
            else
                return(FALSE);

        case 0xFE:
            //
            // Check if word breaking chars in "CNS 11643 compatibility"
            // or "Small variants".
            //
            if ((wch >= 0xFE30) && (wch <= 0xFE5E))
                return((BOOL)(aCNS11643_SmallVariants_EndBreak[wch - 0xFE30]));
            else
                return(FALSE);

        case 0xFF:
            //
            // Check if word breaking chars in "Fullwidth ASCII variants",
            // "Halfwidth Katakana variants" or "Fullwidth symbol variants".
            //
            if ((wch >= 0xFF01) && (wch <= 0xFF9F))
                return((BOOL)(aFullWidthHalfWidthVariants_EndBreak[wch - 0xFF01]));
            else if (wch >= 0xFFE0)
                return(TRUE);
            else
                return(FALSE);

        default:
            return(FALSE);
    }
}

#define UserIsFELineBreak(wChar)    UserIsFELineBreakEnd(wChar)

/***************************************************************************\
*  UserIsFullWidth() - Detects Far East FullWidth character.                *
*                                                                           *
* History:                                                                  *
* 10-Mar-1996 HideyukN  Created                                             *
\***************************************************************************/

typedef struct _FULLWIDTH_UNICODE {
    WCHAR Start;
    WCHAR End;
} FULLWIDTH_UNICODE, *PFULLWIDTH_UNICODE;

#define NUM_FULLWIDTH_UNICODES    4

CONST FULLWIDTH_UNICODE FullWidthUnicodes[] = {
   { 0x4E00, 0x9FFF }, // CJK_UNIFIED_IDOGRAPHS
   { 0x3040, 0x309F }, // HIRAGANA
   { 0x30A0, 0x30FF }, // KATAKANA
   { 0xAC00, 0xD7A3 }  // HANGUL
};

BOOL UserIsFullWidth(DWORD dwCodePage,WCHAR wChar)
{
    INT  index;
    INT  cChars;
#ifdef _USERK_
    CHAR aChars[2];
#endif // _USERK_

    //
    // Early out for ASCII.
    //
    if (wChar < 0x0080) {
        //
        // if the character < 0x0080, it should be a halfwidth character.
        //
        return (FALSE);
    }
    //
    // Scan FullWdith definition table... most of FullWidth character is
    // defined here... this is more faster than call NLS API.
    //
    for (index = 0; index < NUM_FULLWIDTH_UNICODES; index++) {
        if ((wChar >= FullWidthUnicodes[index].Start) &&
            (wChar <= FullWidthUnicodes[index].End)      ) {
            return (TRUE);
        }
    }
    //
    // if this Unicode character is mapped to Double-Byte character,
    // this is also FullWidth character..
    //
#ifdef _USERK_
    cChars = EngWideCharToMultiByte((UINT)dwCodePage,&wChar,sizeof(WCHAR),aChars,sizeof(aChars));
#else
    cChars = WideCharToMultiByte((UINT)dwCodePage,0,&wChar,1,NULL,0,NULL,NULL);
#endif // _USERK_

    return(cChars > 1 ? TRUE : FALSE);
}
/***************************************************************************\
*  UserTextOutWInternal
*  Wrapper for UserTextOutW, used to adjust the parameter passed to
*  PSMTextOut
*
\***************************************************************************/
BOOL UserTextOutWInternal(
    HDC     hdc,
    int     x,
    int     y,
    LPCWSTR lp,
    UINT    cc,
    DWORD   dwFlags)
{
    UNREFERENCED_PARAMETER(dwFlags);
    return UserTextOutW(hdc, x, y, lp, cc);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  KKGetPrefixWidth() -                                                    */
/*                                                                          */
/*  Returns total width of prefix character. Japanese Windows has           */
/*  three shortcut prefixes, '&',\036 and \037.  They may have              */
/*  different width.                                                        */
/*                                                                          */
/*    From Chicago ctlmgr.c HideyukN                                        */
/*--------------------------------------------------------------------------*/

int KKGetPrefixWidth(HDC hdc, LPCWSTR lpStr, int cch)
{
    SIZE size;
    SIZE iPrefix1 = {-1L,-1L};
    SIZE iPrefix2 = {-1L,-1L};
    SIZE iPrefix3 = {-1L,-1L};
    int  iTotal   = 0;

    while (cch-- > 0 && *lpStr) {
        switch(*lpStr) {
        case CH_PREFIX:
            if (lpStr[1] != CH_PREFIX) {
                if (iPrefix1.cx == -1) {
                    UserGetTextExtentPointW(hdc, lpStr, 1, &iPrefix1);
                }
                iTotal += iPrefix1.cx;
            } else {
                lpStr++;
                cch--;
            }
            break;
        case CH_ENGLISHPREFIX:
            if (iPrefix2.cx == -1) {
                 UserGetTextExtentPointW(hdc, lpStr, 1, &iPrefix2);
            }
            iTotal += iPrefix2.cx;
            break;
        case CH_KANJIPREFIX:
            if (iPrefix3.cx == -1) {
                 UserGetTextExtentPointW(hdc, lpStr, 1, &iPrefix3);
            }
            iTotal += iPrefix3.cx;
            //
            // In NT, always alpha numeric mode, Then we have to sum
            // KANA accel key prefix non visible char width.
            // so always add the extent for next char.
            //
            UserGetTextExtentPointW(hdc, lpStr, 1, &size);
            iTotal += size.cx;
            break;
        default:
            // No need to taking care of Double byte since 2nd byte of
            // DBC is grater than 0x2f but all shortcut keys are less
            // than 0x30.
            break;
        }
        lpStr++;
    }
    return iTotal;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  GetNextWordbreak() -                                                    */
/*    From Chicago ctlmgr.c  FritzS                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

LPCWSTR GetNextWordbreak(DWORD dwCodePage,
                         LPCWSTR lpch,
                         LPCWSTR lpchEnd,
                         DWORD  dwFormat,
                         LPDRAWTEXTDATA lpDrawInfo)

{
    /* ichNonWhite is used to make sure we always make progress. */
    int ichNonWhite = 1;
    int ichComplexBreak = 0;        // Breaking opportunity for complex scripts
#if ((DT_WORDBREAK & ~0xff) != 0)
#error cannot use BOOLEAN for DT_WORDBREAK, or you should use "!!" before assigning it
#endif
    BOOLEAN fBreakSpace = (BOOLEAN)WORDBREAK(dwFormat);
    /*
     * If DT_WORDBREAK and DT_NOFULLWIDTHCHARBREAK are both set, we must
     * stop assuming FullWidth characters as word as we're doing in
     * NT4 and Win95. Instead, CR/LF and/or white space will only be
     * a line-break characters.
     */
    BOOLEAN fDbcsCharBreak = (fBreakSpace && !NOFULLWIDTHCHARBREAK(dwFormat));

#ifdef _USERK_
    /*
     * Well, we actually should not and do not call GetNextWordBreak() in
     * kernel, since only Menu stuff (no word break!) calls DrawText from kernel.
     * In reality, thanks to a smart linker, word-break helper
     * functions even does not exist in win32k.sys.
     * Later, we should explicitly omit to compile those routines when we
     * build kernel.
     */
    UNREFERENCED_PARAMETER(dwFormat);
#endif

    // We must terminate this loop before lpch == lpchEnd, otherwise, we
    // may gp fault during *lpch.
    while (lpch < lpchEnd) {
        switch (*lpch) {
        case CR:
        case LF:
            return lpch;

        case '\t':
        case ' ':
            if (fBreakSpace)
                return (lpch + ichNonWhite);

            /*** FALL THRU ***/

        default:
            /*
             * Since most Japanese writing don't use space character
             * to separate each word, we define each Kanji character
             * as a word.
             */
            if (fDbcsCharBreak && UserIsFullWidth(dwCodePage, *lpch)) {
                if (!ichNonWhite)
                    return lpch;
                /*
                 * if the next character is the last character of this string,
                 * We return the character, even this is a "KINSOKU" charcter...
                 */
                if ((lpch+1) != lpchEnd) {
                    /*
                     * Check next character of FullWidth character.
                     * if the next character is "KINSOKU" character, the character
                     * should be handled as a part of previous FullWidth character.
                     * Never handle is as A character, and should not be a Word also.
                     */
                    if (UserIsFELineBreak(*(lpch+1))) {
                        /*
                         * Then if the character is "KINSOKU" character, we return
                         * the next of this character,...
                         */
                        return (lpch + 1 + 1);
                    }
                }
                /*
                 * Otherwise, we just return the chracter that is next of FullWidth
                 * Character. Because we treat A FullWidth chacter as A Word.
                 */
                return (lpch + 1);
            }
            /*
             * If the character is not a FullWidth character and the complex script
             * LPK is present. Call it to determine the breaking opportunity for
             * script that requires word break such as Thai. Note that if *lpch is
             * NOT a complex script character. The LPK will fail the call and return 0
             * since currently Uniscribe does not know how to handle FE break.
             */
            else if(fBreakSpace && lpDrawInfo->bCharsetDll) {
#ifdef _USERK_
                PTHREADINFO ptiCurrent = PtiCurrentShared();
                if(CALL_LPK(ptiCurrent))
#endif
                    ichComplexBreak = (*UserLpkDrawTextEx)(0, 0, 0, lpch, (int)(lpchEnd - lpch), 0,
                                        0, NULL, DT_GETNEXTWORD, -1);
                if (ichComplexBreak > 0)
                    return (lpch + ichComplexBreak);
            }
            lpch++;
            ichNonWhite = 0;
        }
    }

    return lpch;
}

/***************************************************************************\
* GetPrefixCount
*
* This routine returns the count of accelerator mnemonics and the
* character location (starting at 0) of the character to underline.
* A single CH_PREFIX character will be striped and the following character
* underlined, all double CH_PREFIX character sequences will be replaced by
* a single CH_PREFIX (this is done by PSMTextOut). This routine is used
* to determine the actual character length of the string that will be
* printed, and the location the underline should be placed. Only
* cch characters from the input string will be processed. If the lpstrCopy
* parameter is non-NULL, this routine will make a printable copy of the
* string with all single prefix characters removed and all double prefix
* characters collapsed to a single character. If copying, a maximum
* character count must be specified which will limit the number of
* characters copied.
*
* The location of the single CH_PREFIX is returned in the low order
* word, and the count of CH_PREFIX characters that will be striped
* from the string during printing is in the hi order word. If the
* high order word is 0, the low order word is meaningless. If there
* were no single prefix characters (i.e. nothing to underline), the
* low order word will be -1 (to distinguish from location 0).
*
* These routines assume that there is only one single CH_PREFIX character
* in the string.
*
* WARNING! this rountine returns information in BYTE count not CHAR count
* (so it can easily be passed onto GreExtTextOutW which takes byte
* counts as well)
*
* History:
* 11-13-90 JimA         Ported to NT
* 30-Nov-1992 mikeke    Client side version
\***************************************************************************/

LONG GetPrefixCount(
    LPCWSTR lpstr,
    int cch,
    LPWSTR lpstrCopy,
    int charcopycount)
{
    int chprintpos = 0;         /* Num of chars that will be printed */
    int chcount = 0;            /* Num of prefix chars that will be removed */
    int chprefixloc = -1;       /* Pos (in printed chars) of the prefix */
    WCHAR ch;

    /*
     * If not copying, use a large bogus count...
     */
    if (lpstrCopy == NULL)
        charcopycount = 32767;

    while ((cch-- > 0) && *lpstr && charcopycount-- != 0) {

        /*
         * Is this guy a prefix character ?
         */
        if ((ch = *lpstr++) == CH_PREFIX) {

            /*
             * Yup - increment the count of characters removed during print.
             */
            chcount++;

            /*
             * Is the next also a prefix char?
             */
            if (*lpstr != CH_PREFIX) {

                /*
                 * Nope - this is a real one, mark its location.
                 */
                chprefixloc = chprintpos;

            } else {

                /*
                 * yup - simply copy it if copying.
                 */
                if (lpstrCopy != NULL)
                    *(lpstrCopy++) = CH_PREFIX;
                cch--;
                lpstr++;
                chprintpos++;
            }
        } else if (ch == CH_ENGLISHPREFIX) {    // Still needs to be parsed
            /*
             * Yup - increment the count of characters removed during print.
             */
            chcount++;

            /*
             * Next character is a real one, mark its location.
             */
            chprefixloc = chprintpos;

        } else if (ch == CH_KANJIPREFIX) {      // Still needs to be parsed
            /*
             * We only support Alpha Numeric(CH_ENGLISHPREFIX).
             * no support for Kana(CH_KANJIPREFIX).
             */
            /*
             * Yup - increment the count of characters removed during print.
             */
            chcount++;

            if(cch) {
                /* don't copy the character */
                chcount++;
                lpstr++;
                cch--;
            }
        } else {

            /*
             * Nope - just inc count of char.  that will be printed
             */
            chprintpos++;
            if (lpstrCopy != NULL)
                *(lpstrCopy++) = ch;
        }
    }

    if (lpstrCopy != NULL)
        *lpstrCopy = 0;

    /*
     * Return the character counts
     */
    return MAKELONG(chprefixloc, chcount);
}

/***************************************************************************\
*  DT_GetExtentMinusPrefixes
\***************************************************************************/

int DT_GetExtentMinusPrefixes(HDC hdc, LPCWSTR lpchStr, int cchCount, UINT wFormat, 
                        int iOverhang, LPDRAWTEXTDATA  lpDrawInfo, int iCharSet)
{
  int  iPrefixCount;
  int  cxPrefixes = 0;
  WCHAR PrefixChar = CH_PREFIX;
  SIZE size;
  PCLIENTINFO pci = GetClientInfo();
#ifdef _USERK_
  PTHREADINFO ptiCurrent = PtiCurrentShared();
#endif
  UNREFERENCED_PARAMETER(wFormat);

  if(!NOPREFIX(wFormat) &&
      (iPrefixCount = HIWORD(GetPrefixCount(lpchStr, cchCount, NULL, 0)))) {
      //
      // Kanji Windows has three shortcut prefixes...
      //  (ported from Win95 ctlmgr.c)
      //
      if (IS_DBCS_ENABLED() && (pci->dwTIFlags & TIF_16BIT)) {
          // 16bit apps compatibility
          cxPrefixes = KKGetPrefixWidth(hdc, lpchStr, cchCount) - (iPrefixCount * iOverhang);
      }
      else {
          if(lpDrawInfo->bCharsetDll) {
#ifdef _USERK_
              if(CALL_LPK(ptiCurrent))
#endif // _USERK_
              {
                  // Call LPKDrawTextEx with fDraw = FALSE just to get the text extent.
                  return (*UserLpkDrawTextEx)(hdc, 0, 0, lpchStr, cchCount, FALSE,
                         wFormat, lpDrawInfo, DT_CHARSETDRAW, iCharSet);
              }
          } else {
              cxPrefixes = UserGetTextExtentPointW(hdc, &PrefixChar, 1, &size);
              cxPrefixes = size.cx - iOverhang;
              cxPrefixes *=  iPrefixCount;
          }
      }
  }
#ifdef _USERK_
  if(CALL_LPK(ptiCurrent))
    xxxClientGetTextExtentPointW(hdc, lpchStr, cchCount, &size);
  else
#endif // _USERK_
    UserGetTextExtentPointW(hdc, lpchStr, cchCount, &size);
  return (size.cx - cxPrefixes);
}

/***************************************************************************\
*   DT_DrawStr
*      This will draw the given string in the given location without worrying
*  about the left/right justification. Gets the extent and returns it.
*  If fDraw is TRUE and if NOT DT_CALCRECT, this draws the text.
*        NOTE: This returns the extent minus Overhang.
*
*   From Chicago ctlmgr.c  FritzS
\***************************************************************************/
int DT_DrawStr(HDC hdc, int  xLeft, int yTop, LPCWSTR lpchStr,
               int cchCount, BOOL fDraw, UINT wFormat,
               LPDRAWTEXTDATA  lpDrawInfo, int iCharSet)
{
    LPCWSTR        lpch;
    int   iLen;
    int   cxExtent;
    int   xOldLeft = xLeft;  // Save the xLeft given to compute the extent later
    int   xTabLength = lpDrawInfo->cxTabLength;
    int   iTabOrigin = lpDrawInfo->rcFormat.left;

#ifdef USE_MIRRORING
    //
    // Because xLeft and yTop is a point in a rect, and we shift the rect in a mirrored hdc to include
    // its most right pixel, then shift this point as well.
    //
    if (UserGetLayout(hdc) & LAYOUT_RTL) {
        --xOldLeft;
        --xLeft;
    }
#endif

    //
    // if there is a charset dll, let it draw the text.
    //
    if(lpDrawInfo->bCharsetDll) {
#ifdef _USERK_
        PTHREADINFO ptiCurrent = PtiCurrentShared();

        //
        // Don't perform a callback if in thread cleanup mode.
        //
        if(!CALL_LPK(ptiCurrent))
            return 0 ;
#endif // _USERK_
        return (*UserLpkDrawTextEx)(hdc, xLeft, yTop, lpchStr, cchCount, fDraw,
                   wFormat, lpDrawInfo, DT_CHARSETDRAW, iCharSet);
    }

    // Check if the tabs need to be expanded
    if(EXPANDTABS(wFormat)) {
        while(cchCount) {
            // Look for a tab
            for(iLen = 0, lpch = lpchStr; iLen < cchCount; iLen++)
                  if(*lpch++ == TEXT('\t'))
                    break;

                // Draw text, if any, upto the tab
            if (iLen) {
                // Draw the substring taking care of the prefixes.
                if (fDraw && !CALCRECT(wFormat)) { // Only if we need to draw text
                    (*(lpDrawInfo->lpfnTextDraw))(hdc, xLeft, yTop, (LPWSTR)lpchStr, iLen, wFormat);
                }
                // Get the extent of this sub string and add it to xLeft.
                xLeft += DT_GetExtentMinusPrefixes(hdc, lpchStr, iLen, wFormat, lpDrawInfo->cxOverhang, lpDrawInfo, iCharSet) - lpDrawInfo->cxOverhang;
            }

            //if a TAB was found earlier, calculate the start of next sub-string.
            if (iLen < cchCount) {
                iLen++;  // Skip the tab
                if (xTabLength) // Tab length could be zero
                    xLeft = (((xLeft - iTabOrigin)/xTabLength) + 1)*xTabLength + iTabOrigin;
            }

            // Calculate the details of the string that remains to be drawn.
            cchCount -= iLen;
            lpchStr = lpch;
        }
        cxExtent = xLeft - xOldLeft;
    } else {
        // If required, draw the text (with either PSMTextOut or PSTextOut)
        if (fDraw && !CALCRECT(wFormat)) {
            (*(lpDrawInfo->lpfnTextDraw))(hdc, xLeft, yTop, (LPWSTR)lpchStr, cchCount, wFormat);
        }
        // Compute the extent of the text.
        cxExtent = DT_GetExtentMinusPrefixes(hdc, lpchStr, cchCount, wFormat,
                                             lpDrawInfo->cxOverhang, lpDrawInfo, iCharSet) - lpDrawInfo->cxOverhang;
    }
    return cxExtent;
}

/***************************************************************************\
*  DT_DrawJustifiedLine
*      This function draws one complete line with proper justification
*
*   from Chicago ctlmgr.c  FritzS
\***************************************************************************/

void DT_DrawJustifiedLine(HDC  hdc, int yTop, LPCWSTR lpchLineSt,
                                 int cchCount, UINT wFormat,
                                 LPDRAWTEXTDATA lpDrawInfo, int iCharSet)
{
  LPRECT lprc;
  int   cxExtent;
  int   xLeft;

  lprc = &(lpDrawInfo->rcFormat);
  xLeft = lprc->left;

  // Handle the special justifications (right or centered) properly.
  if(wFormat & (DT_CENTER | DT_RIGHT)) {
      cxExtent = DT_DrawStr(hdc, xLeft, yTop, lpchLineSt, cchCount, FALSE,
                     wFormat, lpDrawInfo, iCharSet) + lpDrawInfo->cxOverhang;
      if(wFormat & DT_CENTER)
          xLeft = lprc->left + (((lprc->right - lprc->left) - cxExtent) >> 1);
      else
          xLeft = lprc->right - cxExtent;
    } else
      xLeft = lprc->left;

  // Draw the whole line.
  cxExtent = DT_DrawStr(hdc, xLeft, yTop, lpchLineSt, cchCount, TRUE, wFormat,
                        lpDrawInfo, iCharSet) +lpDrawInfo->cxOverhang;
  if(cxExtent > lpDrawInfo->cxMaxExtent)
      lpDrawInfo->cxMaxExtent = cxExtent;
}

/***************************************************************************\
* DT_InitDrawTextInfo
*      This is called at the begining of DrawText(); This initializes the
* DRAWTEXTDATA structure passed to this function with all the required info.
*
*  from Chicago ctlmgr.c  FritzS
\***************************************************************************/

BOOL DT_InitDrawTextInfo(
    HDC hdc,
    LPRECT lprc,
    UINT wFormat,
    LPDRAWTEXTDATA lpDrawInfo,
    LPDRAWTEXTPARAMS lpDTparams)
{
  SIZE   sizeViewPortExt = {0, 0},sizeWindowExt = {0, 0};
  TEXTMETRICW tm;
  LPRECT      lprcDest;
  int         iTabLength = 8;   // Default Tab length is 8 characters.
  int         iLeftMargin;
  int         iRightMargin;
  BOOL        fUseSystemFont;

  if (lpDTparams) {
      /*
       *  Only if DT_TABSTOP flag is mentioned, we must use the iTabLength field.
       */
      if (TABSTOP(wFormat))
          iTabLength = lpDTparams->iTabLength;
      iLeftMargin = lpDTparams->iLeftMargin;
      iRightMargin = lpDTparams->iRightMargin;
  } else {
      iLeftMargin = iRightMargin = 0;
  }

  /*
   *  Get the View port and Window extents for the given DC
   *  If this call fails, hdc must be invalid
   */
  if (!UserGetViewportExtEx(hdc,&sizeViewPortExt)) {
#ifndef _USERK_
      /*
       * This call fails on  standard Metafiles. So check
       * if the DC is really invalid to be compatible with
       * Win9x
       */
      if ((hdc == NULL) || !GdiValidateHandle(hdc))
#endif
          return FALSE;
  }
  UserGetWindowExtEx(hdc, &sizeWindowExt);

  /*
   *  For the current mapping mode,  find out the sign of x from left to right.
   */
  lpDrawInfo->iXSign =
      (((sizeViewPortExt.cx ^ sizeWindowExt.cx) & 0x80000000) ? -1 : 1);

  /*
   *  For the current mapping mode,  find out the sign of y from top to bottom.
   */
  lpDrawInfo->iYSign =
      (((sizeViewPortExt.cy ^ sizeWindowExt.cy) & 0x80000000) ? -1 : 1);

  /*
   *  Calculate the dimensions of the current font in this DC.
   * (If it is SysFont AND the mapping mode is MM_TEXT, use system font's data)
   */
  fUseSystemFont = ((wFormat & DT_INTERNAL) || IsSysFontAndDefaultMode(hdc));
  if (!fUseSystemFont) {
      /*
       *  Edit controls have their own way of calculating the aveCharWidth.
       */
      if (EDITCONTROL(wFormat)) {
          tm.tmAveCharWidth = UserGetCharDimensionsW(hdc, &tm, NULL);
          tm.tmCharSet = (BYTE)UserGetTextCharsetInfo(hdc, NULL, 0);
          if (tm.tmAveCharWidth == 0) {
              fUseSystemFont = TRUE;
          }
      } else if (!UserGetTextMetricsW(hdc, &tm)) {
          /*
           * This can fail in a hard error popup during logon or logoff
           * because UpdatePerUserSystemParameters destroys the server-side
           * font handle for the DC, and a repaint occurs before we switch
           * desktops (the switch recreates the popup from scratch with the
           * new font OK). ChrisWil's changes to move system-wide attributes
           * into desktops should take care of this in Kernel-mode.  This is
           * just a horrible, horrible hack for now.
           */
          RIPMSG0(RIP_WARNING, "UserGetTextMetricsW failed: only in logon/off?\n");
          tm.tmOverhang = 0;

          /*
           * We should probably set fUseSystemFont to TRUE here. But I
           *  assume that this "horrible hack" works fine plus it has been
           *  here for good. So I'll leave it alone. 6/3/96
           */
      }
  }

  if (fUseSystemFont) {
      /*
       *  Avoid GetTextMetrics for internal calls since they use sys font.
       */
      tm.tmHeight = gpsi->cySysFontChar;
      tm.tmExternalLeading = gpsi->tmSysFont.tmExternalLeading;
      tm.tmAveCharWidth = gpsi->tmSysFont.tmAveCharWidth;
      tm.tmOverhang = gpsi->tmSysFont.tmOverhang;
#ifdef _USERK_
      tm.tmCharSet = (BYTE)UserGetTextCharsetInfo(gpDispInfo->hdcScreen, NULL, 0);
#else
      tm.tmCharSet = (BYTE)UserGetTextCharsetInfo(ghdcBits2, NULL, 0);
#endif // _USERK_
  }


  // cyLineHeight is in pixels (This will be signed).
  lpDrawInfo->cyLineHeight = (tm.tmHeight +
            (EXTERNALLEADING(wFormat) ? tm.tmExternalLeading : 0)) *
            lpDrawInfo->iYSign;

  // cxTabLength is the tab length in pixels (This will not be signed)
  lpDrawInfo->cxTabLength = tm.tmAveCharWidth * iTabLength;

  // Set the cxOverhang
  lpDrawInfo->cxOverhang = tm.tmOverhang;

  // Pick up the proper TextOut function based on the prefix processing reqd.
#ifdef _USERK_
  lpDrawInfo->bCharsetDll = PpiCurrent()->dwLpkEntryPoints & LPK_DRAWTEXTEX;
  if (lpDrawInfo->bCharsetDll == FALSE) {
      lpDrawInfo->lpfnTextDraw = (NOPREFIX(wFormat) ? (LPFNTEXTDRAW)UserTextOutWInternal : xxxPSMTextOut);
  }
#else
  lpDrawInfo->bCharsetDll = (BOOL)(fpLpkDrawTextEx != (FPLPKDRAWTEXTEX)NULL);
  if (lpDrawInfo->bCharsetDll == FALSE) {
      lpDrawInfo->lpfnTextDraw = (NOPREFIX(wFormat) ? (LPFNTEXTDRAW)UserTextOutWInternal : PSMTextOut);
  }
#endif // _USERK_

  // Set up the format rectangle based on the margins.
//  LCopyStruct(lprc, lprcDest = (LPRECT)&(lpDrawInfo->rcFormat), sizeof(RECT));
  lprcDest = &(lpDrawInfo->rcFormat);
  *lprcDest = *lprc;

  // We need to do the following only if the margins are given
  if(iLeftMargin | iRightMargin) {
      lprcDest->left += iLeftMargin * lpDrawInfo->iXSign;
      lprcDest->right -= (lpDrawInfo->cxRightMargin = iRightMargin * lpDrawInfo->iXSign);
    } else
      lpDrawInfo->cxRightMargin = 0;  // Initialize to zero.

  // cxMaxWidth is unsigned.
  lpDrawInfo->cxMaxWidth = (lprcDest->right - lprcDest->left) * lpDrawInfo->iXSign;
  lpDrawInfo->cxMaxExtent = 0;  // Initialize this to zero.

  return TRUE;
}

/***************************************************************************\
* DT_AdjustWhiteSpaces
*      In the case of WORDWRAP, we need to treat the white spaces at the
* begining/end of each line specially. This function does that.
*  lpStNext = points to the begining of next line.
*  lpiCount = points to the count of characters in the current line.
\***************************************************************************/

LPCWSTR  DT_AdjustWhiteSpaces(LPCWSTR  lpStNext, LPINT lpiCount, UINT wFormat)
{
  switch(wFormat & DT_HFMTMASK) {
      case DT_LEFT:
        // Prevent a white space at the begining of a left justfied text.
        // Is there a white space at the begining of next line......
        if((*lpStNext == TEXT(' ')) || (*lpStNext == TEXT('\t'))) {
            // ...then, exclude it from next line.
            lpStNext++;
          }
        break;

      case DT_RIGHT:
        // Prevent a white space at the end of a RIGHT justified text.
        // Is there a white space at the end of current line,.......
        if((*(lpStNext-1) == TEXT(' ')) || (*(lpStNext - 1) == TEXT('\t'))) {
            // .....then, Skip the white space from the current line.
            (*lpiCount)--;
          }
        break;

      case DT_CENTER:
        // Exclude white spaces from the begining and end of CENTERed lines.
        // If there is a white space at the end of current line.......
        if((*(lpStNext-1) == TEXT(' ')) || (*(lpStNext - 1) == TEXT('\t')))
            (*lpiCount)--;    //...., don't count it for justification.
        // If there is a white space at the begining of next line.......
        if((*lpStNext == TEXT(' ')) || (*lpStNext == TEXT('\t')))
            lpStNext++;       //...., exclude it from next line.
        break;
    }
  return lpStNext;
}

/***************************************************************************\
*  DT_BreakAWord
*      A word needs to be broken across lines and this finds out where to
*  break it.
\***************************************************************************/
LPCWSTR  DT_BreakAWord(HDC  hdc, LPCWSTR lpchText,
              int iLength, int iWidth, UINT wFormat, int iOverhang, LPDRAWTEXTDATA  lpDrawInfo, int iCharSet)
{
  int  iLow = 0, iHigh = iLength;
  int  iNew;


  while((iHigh - iLow) > 1) {
      iNew = iLow + (iHigh - iLow)/2;
      if(DT_GetExtentMinusPrefixes(hdc, lpchText, iNew, wFormat, iOverhang, lpDrawInfo, iCharSet) > iWidth)
          iHigh = iNew;
      else
          iLow = iNew;
    }
  // If the width is too low, we must print atleast one char per line.
  // Else, we will be in an infinite loop.
  if(!iLow && iLength)
      iLow = 1;
  return (lpchText+iLow);
}

/***************************************************************************\
* DT_GetLineBreak
*      This finds out the location where we can break a line.
* Returns LPCSTR to the begining of next line.
* Also returns via lpiLineLength, the length of the current line.
* NOTE: (lpstNextLineStart - lpstCurrentLineStart) is not equal to the
* line length; This is because, we exclude some white spaces at the begining
* and/or end of lines; Also, CR/LF is excluded from the line length.
\***************************************************************************/

LPWSTR DT_GetLineBreak(
    HDC  hdc,
    LPCWSTR lpchLineStart,
    int   cchCount,
    DWORD dwFormat,
    LPINT lpiLineLength,
    LPDRAWTEXTDATA  lpDrawInfo,
    int iCharSet)
{
  LPCWSTR lpchText, lpchEnd, lpch, lpchLineEnd;
  int   cxStart, cxExtent, cxNewExtent;
  BOOL  fAdjustWhiteSpaces = FALSE;
  WCHAR  ch;
  DWORD dwCodePage = USERGETCODEPAGE(hdc);

  cxStart = lpDrawInfo->rcFormat.left;
  cxExtent = cxNewExtent = 0;
  lpchText = lpchLineStart;
  lpchEnd = lpchLineStart + cchCount;


  while(lpchText < lpchEnd) {
      lpchLineEnd = lpch = GetNextWordbreak(dwCodePage,lpchText, lpchEnd, dwFormat, lpDrawInfo);
      // DT_DrawStr does not return the overhang; Otherwise we will end up
      // adding one overhang for every word in the string.

      // For simulated Bold fonts, the summation of extents of individual
      // words in a line is greater than the extent of the whole line. So,
      // always calculate extent from the LineStart.
      // BUGTAG: #6054 -- Win95B -- SANKAR -- 3/9/95 --
      cxNewExtent = DT_DrawStr(hdc, cxStart, 0, lpchLineStart, (int)(((PBYTE)lpch - (PBYTE)lpchLineStart)/sizeof(WCHAR)), FALSE,
                 dwFormat, lpDrawInfo, iCharSet);

      if (WORDBREAK(dwFormat) && ((cxNewExtent + lpDrawInfo->cxOverhang) > lpDrawInfo->cxMaxWidth)) {
          // Are there more than one word in this line?
          if (lpchText != lpchLineStart)  {
              lpchLineEnd = lpch = lpchText;
              fAdjustWhiteSpaces = TRUE;
          } else {
              //One word is longer than the maximum width permissible.
              //See if we are allowed to break that single word.
              if(EDITCONTROL(dwFormat) && !WORDELLIPSIS(dwFormat)) {
                  lpchLineEnd = lpch = DT_BreakAWord(hdc, lpchText, (int)(((PBYTE)lpch - (PBYTE)lpchText)/sizeof(WCHAR)),
                        lpDrawInfo->cxMaxWidth - cxExtent,
                        dwFormat,
                        lpDrawInfo->cxOverhang, lpDrawInfo, iCharSet); //Break that word
                  //Note: Since we broke in the middle of a word, no need to
                  // adjust for white spaces.
              } else {
                  fAdjustWhiteSpaces = TRUE;
                  // Check if we need to end this line with ellipsis
                  if(WORDELLIPSIS(dwFormat))
                    {
                      // Don't do this if already at the end of the string.
                      if (lpch < lpchEnd)
                        {
                          // If there are CR/LF at the end, skip them.
                          if ((ch = *lpch) == CR || ch == LF)
                            {
                              if ((++lpch < lpchEnd) && (*lpch == (WCHAR)(ch ^ (LF ^ CR))))
                                  lpch++;
                              fAdjustWhiteSpaces = FALSE;
                            }
                        }
                    }
              }
          }
          // Well! We found a place to break the line. Let us break from this
          // loop;
          break;
      } else {
          // Don't do this if already at the end of the string.
          if (lpch < lpchEnd) {
              if ((ch = *lpch) == CR || ch == LF) {
                  if ((++lpch < lpchEnd) && (*lpch == (WCHAR)(ch ^ (LF ^ CR))))
                      lpch++;
                  fAdjustWhiteSpaces = FALSE;
                  break;
              }
          }
      }

      // Point at the beginning of the next word.
      lpchText = lpch;
      cxExtent = cxNewExtent;
  }

  // Calculate the length of current line.
  *lpiLineLength = (INT)((PBYTE)lpchLineEnd - (PBYTE)lpchLineStart)/sizeof(WCHAR);

  // Adjust the line length and lpch to take care of spaces.
  if(fAdjustWhiteSpaces && (lpch < lpchEnd))
      lpch = DT_AdjustWhiteSpaces(lpch, lpiLineLength, dwFormat);

  // return the begining of next line;
  return (LPWSTR)lpch;
}

/***************************************************************************\
*  NeedsEndEllipsis()
*      This function checks whether the given string fits within the given
*      width or we need to add end-ellipse. If it required end-ellipses, it
*      returns TRUE and it returns the number of characters that are saved
*      in the given string via lpCount.
\***************************************************************************/
BOOL  NeedsEndEllipsis(HDC        hdc,
                                     LPCWSTR     lpchText,
                                     LPINT      lpCount,
                                     LPDRAWTEXTDATA  lpDTdata,
                                     UINT       wFormat, LPDRAWTEXTDATA  lpDrawInfo, int iCharSet)
{
    int   cchText;
    int   ichMin, ichMax, ichMid;
    int   cxMaxWidth;
    int   iOverhang;
    int   cxExtent;
    SIZE size;
    cchText = *lpCount;  // Get the current count.

    if (cchText == 0)
        return FALSE;

    cxMaxWidth  = lpDTdata->cxMaxWidth;
    iOverhang   = lpDTdata->cxOverhang;

    cxExtent = DT_GetExtentMinusPrefixes(hdc, lpchText, cchText, wFormat, iOverhang, lpDrawInfo, iCharSet);

    if (cxExtent <= cxMaxWidth)
        return FALSE;
    // Reserve room for the "..." ellipses;
    // (Assumption: The ellipses don't have any prefixes!)
    UserGetTextExtentPointW(hdc, szEllipsis, CCHELLIPSIS, &size);
    cxMaxWidth -= size.cx - iOverhang;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxMaxWidth > 0) {
        // Binary search to find characters that will fit.
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax) {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            //
            ichMid = (ichMin + ichMax + 1) / 2;

            cxExtent = DT_GetExtentMinusPrefixes(hdc, lpchText, ichMid, wFormat, iOverhang, lpDrawInfo, iCharSet);

            if (cxExtent < cxMaxWidth)
                ichMin = ichMid;
            else {
                if (cxExtent > cxMaxWidth)
                    ichMax = ichMid - 1;
                else {
                    // Exact match up up to ichMid: just exit.
                    //
                    ichMax = ichMid;
                    break;
                  }
              }
          }

        // Make sure we always show at least the first character...
        //
        if (ichMax < 1)
            ichMax = 1;
      }

    *lpCount = ichMax;
    return TRUE;
}

/***************************************************************************\
* BOGUS: The same function is available in SHELL2.DLL also.
* We need to remove from one of the places.
\***************************************************************************/
// Returns a pointer to the last component of a path string.
//
// in:
//      path name, either fully qualified or not
//
// returns:
//      pointer into the path where the path is.  if none is found
//      returns a poiter to the start of the path
//
//  c:\foo\bar  -> bar
//  c:\foo      -> foo
//  c:\foo\     -> c:\foo\      (REVIEW: is this case busted?)
//  c:\         -> c:\          (REVIEW: this case is strange)
//  c:          -> c:
//  foo         -> foo
/***************************************************************************\
\***************************************************************************/


LPWSTR PathFindFileName(LPCWSTR pPath, int cchText)
{
    LPCWSTR pT;

    for (pT = pPath; cchText > 0 && *pPath; pPath++, cchText--) {
        if ((pPath[0] == TEXT('\\') || pPath[0] == TEXT(':')) && pPath[1])
            pT = pPath + 1;
    }

    return (LPWSTR)pT;   // REVIEW, should this be const?
}

/***************************************************************************\
* AddPathEllipse():
*      This adds a path ellipse to the given path name.
*      Returns TRUE if the resultant string's extent is less the the
* cxMaxWidth. FALSE, if otherwise.
\***************************************************************************/
int AddPathEllipsis(
    HDC    hDC,
    LPWSTR lpszPath,
    int    cchText,
    UINT   wFormat,
    int    cxMaxWidth,
    int    iOverhang, LPDRAWTEXTDATA  lpDrawInfo, int iCharSet)
{
  int    iLen;
  UINT   dxFixed, dxEllipsis;
  LPWSTR lpEnd;          /* end of the unfixed string */
  LPWSTR lpFixed;        /* start of text that we always display */
  BOOL   bEllipsisIn;
  int    iLenFixed;
  SIZE   size;

  lpFixed = PathFindFileName(lpszPath, cchText);
  if (lpFixed != lpszPath)
      lpFixed--;  // point at the slash
  else
      return cchText;

  lpEnd = lpFixed;
  bEllipsisIn = FALSE;
  iLenFixed = cchText - (int)(lpFixed - lpszPath);
  dxFixed = DT_GetExtentMinusPrefixes(hDC, lpFixed, iLenFixed, wFormat, iOverhang, lpDrawInfo, iCharSet);

  // It is assumed that the "..." string does not have any prefixes ('&').
  UserGetTextExtentPointW(hDC, szEllipsis, CCHELLIPSIS, &size);
  dxEllipsis = size.cx - iOverhang;

  while (TRUE) {
      iLen = dxFixed + DT_GetExtentMinusPrefixes(hDC, lpszPath, (int)((PBYTE)lpEnd - (PBYTE)lpszPath)/sizeof(WCHAR),
                                       wFormat, iOverhang, lpDrawInfo, iCharSet) - iOverhang;

      if (bEllipsisIn)
          iLen += dxEllipsis;

      if (iLen <= cxMaxWidth)
          break;

      bEllipsisIn = TRUE;

      if (lpEnd <= lpszPath) {
          /* Things didn't fit. */
          lpEnd = lpszPath;
          break;
      }

      /* Step back a character. */
      lpEnd--;
  }

  if (bEllipsisIn && (lpEnd + CCHELLIPSIS < lpFixed)) {
      // NOTE: the strings could over lap here. So, we use LCopyStruct.

      RtlMoveMemory((lpEnd + CCHELLIPSIS), lpFixed, iLenFixed * sizeof(WCHAR));
      RtlCopyMemory(lpEnd, szEllipsis, CCHELLIPSIS * sizeof(WCHAR));

      cchText = (int)(lpEnd - lpszPath) + CCHELLIPSIS + iLenFixed;

      // now we can NULL terminate the string
      *(lpszPath + cchText) = TEXT('\0');
  }

  return cchText;
}

//-----------------------------------------------------------------------
// This function returns the number of characters actually drawn.
//-----------------------------------------------------------------------
int AddEllipsisAndDrawLine(
    HDC            hdc,
    int            yLine,
    LPCWSTR        lpchText,
    int            cchText,
    DWORD          dwDTformat,
    LPDRAWTEXTDATA lpDrawInfo,
    int iCharSet)
{
    LPWSTR pEllipsis = NULL;
    WCHAR  szTempBuff[MAXBUFFSIZE];
    LPWSTR lpDest;
    BOOL   fAlreadyCopied = FALSE;

    // Check if this is a filename with a path AND
    // Check if the width is too narrow to hold all the text.
    if(PATHELLIPSIS(dwDTformat) &&
        ((DT_GetExtentMinusPrefixes(hdc, lpchText, cchText,
                   dwDTformat, lpDrawInfo->cxOverhang, lpDrawInfo, iCharSet)) > lpDrawInfo->cxMaxWidth)) {
        // We need to add Path-Ellipsis. See if we can do it in-place.
        if(!MODIFYSTRING(dwDTformat)) {
            // NOTE: When you add Path-Ellipsis, the string could grow by
            // CCHELLIPSIS bytes.
            if((cchText + CCHELLIPSIS + 1) <= MAXBUFFSIZE)
                lpDest = szTempBuff;
            else {   // Alloc from local heap.
                // Alloc the buffer from local heap.
                if(!(pEllipsis = (LPWSTR)UserRtlAllocMem(
                        (cchText+CCHELLIPSIS+1)*sizeof(WCHAR))))
                    return 0;
                lpDest = (LPWSTR)pEllipsis;
            }
            // Source String may not be NULL terminated. So, copy just
            // the given number of characters.
            RtlCopyMemory(lpDest, lpchText, cchText*sizeof(WCHAR));
            lpchText = lpDest;        // lpchText points to the copied buff.
            fAlreadyCopied = TRUE;    // Local copy has been made.
        }
        // Add the path ellipsis now!
        cchText = AddPathEllipsis(hdc, (LPWSTR)lpchText, cchText, dwDTformat,
            lpDrawInfo->cxMaxWidth, lpDrawInfo->cxOverhang, lpDrawInfo, iCharSet);
    }

    // Check if end-ellipsis are to be added.
    if((ENDELLIPSIS(dwDTformat) || WORDELLIPSIS(dwDTformat)) &&
        NeedsEndEllipsis(hdc, lpchText, &cchText, lpDrawInfo, dwDTformat, lpDrawInfo, iCharSet)) {
        // We need to add end-ellipsis; See if we can do it in-place.
        if(!MODIFYSTRING(dwDTformat) && !fAlreadyCopied) {
            // See if the string is small enough for the buff on stack.
            if((cchText+CCHELLIPSIS+1) <= MAXBUFFSIZE)
                lpDest = szTempBuff;  // If so, use it.
            else {
                // Alloc the buffer from local heap.
                if(!(pEllipsis = (LPWSTR)UserRtlAllocMem(
                        (cchText+CCHELLIPSIS+1)*sizeof(WCHAR))))
                    return 0;
                lpDest = pEllipsis;
            }
            // Make a copy of the string in the local buff.
            RtlCopyMemory(lpDest, lpchText, cchText*sizeof(WCHAR));
            lpchText = lpDest;
        }
        // Add an end-ellipsis at the proper place.
        RtlCopyMemory((LPWSTR)(lpchText+cchText), szEllipsis, (CCHELLIPSIS+1)*sizeof(WCHAR));
        cchText += CCHELLIPSIS;
    }

    // Draw the line that we just formed.
    DT_DrawJustifiedLine(hdc, yLine, lpchText, cchText, dwDTformat, lpDrawInfo, iCharSet);

    // Free the block allocated for End-Ellipsis.
    if(pEllipsis)
        UserRtlFreeMem(pEllipsis);

    return cchText;
}


/***************************************************************************\
* IDrawTextEx
*      This is the new DrawText API
\***************************************************************************/

/***************************************************************************\
* IDrawTextEx
*      This is the new DrawText API
\***************************************************************************/

int  DrawTextExW(
   HDC               hdc,
   LPWSTR            lpchText,
   int               cchText,
   LPRECT            lprc,
   UINT              dwDTformat,
   LPDRAWTEXTPARAMS  lpDTparams)
{
   /*
    * The LPK requires a charset.  The Unicode entry point always passes a -1,
    * but the ANSI entry point passes a more interesting value.  Both the
    * 'W' version and 'A' version of DrawTextEx call this common worker routine.
    */
   return DrawTextExWorker(hdc, lpchText, cchText, lprc, dwDTformat, lpDTparams, -1);
}

int  DrawTextExWorker(
   HDC               hdc,
   LPWSTR            lpchText,
   int               cchText,
   LPRECT            lprc,
   UINT              dwDTformat,
   LPDRAWTEXTPARAMS  lpDTparams,
   int               iCharset)
{
    DRAWTEXTDATA DrawInfo;
    WORD         wFormat = LOWORD(dwDTformat);
    LPWSTR       lpchTextBegin;
    LPWSTR       lpchEnd;
    LPWSTR       lpchNextLineSt;
    int          iLineLength;
    int          iySign;
    int          yLine;
    int          yLastLineHeight;
    HRGN         hrgnClip;
    int          iLineCount;
    RECT         rc;
    BOOL         fLastLine;
    WCHAR        ch;
    UINT         oldAlign;

#if DBG
    if (dwDTformat & ~DT_VALID)
        RIPMSG0 (RIP_WARNING, "DrawTextExW: Invalid dwDTformat flags");
#endif

    if (lpchText == NULL) {
        return 1;
    }

    if (cchText == 0 && *lpchText) {
        /*
         * infoview.exe passes lpchText that points to '\0'
         *
         * "Microsoft Expedia Streets and Trips 2000" and "MS MapPoint 2000"
         * tries cchText == 0 to detect if DrawTextW is supported.
         */

        /* Added by Chicago:
         * Lotus Notes doesn't like getting a zero return here
         */
        return 1;
    }

    if (cchText == -1)
        cchText = wcslen(lpchText);



    if ((lpDTparams) && (lpDTparams->cbSize != sizeof(DRAWTEXTPARAMS))) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "DrawTextEx: cbSize %ld is invalid",
                lpDTparams->cbSize);
        return 0;
    }


#ifdef LATER
    /*
     * If DT_MODIFYSTRING is specified, then check for read-write pointer.
     */
    if (MODIFYSTRING(dwDTformat) &&
            (ENDELLIPSIS(dwDTformat) || PATHELLIPSIS(dwDTformat))) {
        if(IsBadWritePtr(lpchText, cchText)) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "DrawTextEx: For DT_MODIFYSTRING, lpchText must be read-write");
            return(0);
        }
    }
#endif

    /*
     * Initialize the DrawInfo structure.
     */
    if (!DT_InitDrawTextInfo(hdc, lprc, dwDTformat, (LPDRAWTEXTDATA)&DrawInfo, lpDTparams))
        return 0;

    DrawInfo.iCharset = iCharset;
    /*
     * If the rect is too narrow or the margins are too wide.....Just forget it!
     *
     * If wordbreak is specified, the MaxWidth must be a reasonable value.
     * This check is sufficient because this will allow CALCRECT and NOCLIP
     * cases.  --SANKAR.
     *
     * This also fixed all of our known problems with AppStudio.
     */
    if (DrawInfo.cxMaxWidth <= 0) {

        /*
         * We used to return a non-zero value in win31.
         * If the kernel calls this we are always Ver 4.0 or above
         */
#ifdef _USERK_
        if (0) {
#else
        if (GETAPPVER() < VER40) {
#endif
            if((DrawInfo.cxMaxWidth == 0) && !CALCRECT(wFormat)) {
                return(1);
            }
        } else {
            if (WORDBREAK(wFormat)) {
                RIPMSG0 (RIP_WARNING, "DrawTextExW: FAILURE DrawInfo.cxMaxWidth <=0");
                return (1);
            }
        }
    }

    /*
     * if we're not doing the drawing, initialise the lpk-dll
     */
    if (RTLREADING(dwDTformat)) {
        oldAlign = UserSetTextAlign(hdc, TA_RTLREADING | UserGetTextAlign(hdc));
    }

    if (DrawInfo.bCharsetDll) {
#ifdef _USERK_
        PTHREADINFO ptiCurrent = PtiCurrentShared();

        if(CALL_LPK(ptiCurrent))
#endif // _USERK_
            (*UserLpkDrawTextEx)(hdc, 0, 0, lpchText, cchText, FALSE, dwDTformat,
                                 (LPDRAWTEXTDATA)&DrawInfo, DT_CHARSETINIT, iCharset);
    }

    /*
     * If we need to clip, let us do that.
     */
    if (!NOCLIP(wFormat)) {
        //
        // Save clipping region so we can restore it later.
        //
        // hrgnSave = SaveClipRgn(hdc);
        // IntersectClipRect(hdc, lprc->left, lprc->top, lprc->right, lprc->bottom);

        hrgnClip = UserCreateRectRgn(0,0,0,0);
        if (hrgnClip != NULL) {
            if (UserGetClipRgn(hdc, hrgnClip) != 1) {
                UserDeleteObject(hrgnClip);
                hrgnClip = (HRGN)-1;
            }
            rc = *lprc;
            UserIntersectClipRect(hdc, rc.left, rc.top, rc.right, rc.bottom);
        }
    } else {
        hrgnClip = NULL;
    }

    lpchTextBegin = lpchText;
    lpchEnd = lpchText + cchText;

ProcessDrawText:

    iLineCount = 0;  // Reset number of lines to 1.
    yLine = lprc->top;

    if (SINGLELINE(wFormat)) {
        iLineCount = 1;  // It is a single line.


        /*
         * Process single line DrawText.
         */
        switch (wFormat & DT_VFMTMASK) {
            case DT_BOTTOM:
                yLine = lprc->bottom - DrawInfo.cyLineHeight;
                break;

            case DT_VCENTER:
                yLine = lprc->top + ((lprc->bottom - lprc->top - DrawInfo.cyLineHeight) / 2);
                break;
        }

        cchText = AddEllipsisAndDrawLine(hdc, yLine, lpchText, cchText, dwDTformat, &DrawInfo, iCharset);
        yLine += DrawInfo.cyLineHeight;
        lpchText += cchText;
    } else  {

        /*
         * Multiline
         * If the height of the rectangle is not an integral multiple of the
         * average char height, then it is possible that the last line drawn
         * is only partially visible. However, if DT_EDITCONTROL style is
         * specified, then we must make sure that the last line is not drawn if
         * it is going to be partially visible. This will help imitate the
         * appearance of an edit control.
         */
        if (EDITCONTROL(wFormat))
            yLastLineHeight = DrawInfo.cyLineHeight;
        else
            yLastLineHeight = 0;

        iySign = DrawInfo.iYSign;
        fLastLine = FALSE;
        // Process multiline DrawText.
        while ((lpchText < lpchEnd) && (!fLastLine)) {
          // Check if the line we are about to draw is the last line that needs
          // to be drawn.
          // Let us check if the display goes out of the clip rect and if so
          // let us stop here, as an optimisation;
          if (!CALCRECT(wFormat) &&         // We don't need to calc rect?
                  (!NOCLIP(wFormat)) &&     // Must we clip the display ?
                                            // Are we outside the rect?
                  ((yLine + DrawInfo.cyLineHeight + yLastLineHeight)*iySign > (lprc->bottom*iySign))) {
              fLastLine = TRUE;    // Let us quit this loop
          }


          /*
           * We do the Ellipsis processing only for the last line.
           */
          if (fLastLine && (ENDELLIPSIS(dwDTformat) || PATHELLIPSIS(dwDTformat))) {
              lpchText += AddEllipsisAndDrawLine(hdc, yLine, lpchText, cchText, dwDTformat, &DrawInfo, iCharset);
          } else {
              lpchNextLineSt = (LPWSTR)DT_GetLineBreak(hdc, lpchText, cchText, dwDTformat, &iLineLength, &DrawInfo, iCharset);

              /*
               * Check if we need to put ellipsis at the end of this line.
               * Also check if this is the last line.
               */
              if (WORDELLIPSIS(dwDTformat) ||
                       ((lpchNextLineSt >= lpchEnd) && (ENDELLIPSIS(dwDTformat) || PATHELLIPSIS(dwDTformat))))
                  AddEllipsisAndDrawLine(hdc, yLine, lpchText, iLineLength, dwDTformat, &DrawInfo, iCharset);
              else
                  DT_DrawJustifiedLine(hdc, yLine, lpchText, iLineLength, dwDTformat, &DrawInfo, iCharset);
              cchText -= (int)((PBYTE)lpchNextLineSt - (PBYTE)lpchText) / sizeof(WCHAR);
              lpchText = lpchNextLineSt;
          }
            iLineCount++; // We draw one more line.
            yLine += DrawInfo.cyLineHeight;
        }


        /*
         * For Win3.1 and NT compatibility, if the last char is a CR or a LF
         * then the height returned includes one more line.
         */
        if (!EDITCONTROL(dwDTformat) &&
                (lpchEnd > lpchTextBegin)    &&   // If zero length it will fault.
                (((ch = (*(lpchEnd-1))) == CR) || (ch == LF)))
            yLine += DrawInfo.cyLineHeight;
    }


    /*
     * If DT_CALCRECT, modify width and height of rectangle to include
     * all of the text drawn.
     */
    if (CALCRECT(wFormat)) {
        DrawInfo.rcFormat.right = DrawInfo.rcFormat.left + DrawInfo.cxMaxExtent * DrawInfo.iXSign;
        lprc->right = DrawInfo.rcFormat.right + DrawInfo.cxRightMargin;

        // If the Width is more than what was provided, we have to redo all
        // the calculations, because, the number of lines can be less now.
        // (We need to do this only if we have more than one line).
        if((iLineCount > 1) && (DrawInfo.cxMaxExtent > DrawInfo.cxMaxWidth)) {
            DrawInfo.cxMaxWidth = DrawInfo.cxMaxExtent;
            lpchText = lpchTextBegin;
            cchText = (int)((PBYTE)lpchEnd - (PBYTE)lpchTextBegin)/sizeof(WCHAR);
            goto  ProcessDrawText;  // Start all over again!
        }
        lprc->bottom = yLine;
    }

// if (!NOCLIP(wFormat))
// {
//     RestoreClipRgn(hdc, hrgnClip);
// }

    if (hrgnClip != NULL) {
        if (hrgnClip == (HRGN)-1) {
            UserExtSelectClipRgn(hdc, NULL, RGN_COPY);
        } else {
            UserExtSelectClipRgn(hdc, hrgnClip, RGN_COPY);
            UserDeleteObject(hrgnClip);
        }
    }

    if(DrawInfo.bCharsetDll) {
#ifdef _USERK_
        PTHREADINFO ptiCurrent = PtiCurrentShared();

        if(CALL_LPK(ptiCurrent))
#endif // _USERK_
            (*UserLpkDrawTextEx)(hdc, 0, 0, lpchText, cchText, FALSE, dwDTformat,
                                 (LPDRAWTEXTDATA)&DrawInfo, DT_CHARSETDONE, iCharset);
    }

    if (RTLREADING(dwDTformat))
        UserSetTextAlign(hdc, oldAlign);

    /*
     * Copy the number of characters actually drawn
     */
    if(lpDTparams != NULL)
        lpDTparams->uiLengthDrawn = (UINT)((PBYTE)lpchText - (PBYTE)lpchTextBegin)/sizeof(WCHAR);

    if (yLine == lprc->top)
        return 1;

    return (yLine - lprc->top);
}

/***************************************************************************\
*
* IsSysFontAndDefaultMode()
*
* Returns TRUE if font selected into DC is the system font AND the current
* mapping mode of the DC is MM_TEXT (Default mode); else returns FALSE. This
* is called by interrupt time code so it needs to be in the fixed code
* segment.
*
* History:
* 07-Jul-95 BradG   Ported from Win95
\***************************************************************************/

BOOL IsSysFontAndDefaultMode(HDC hdc)
{
    return((UserGetHFONT(hdc) == ghFontSys) && (UserGetMapMode(hdc) == MM_TEXT));
}
