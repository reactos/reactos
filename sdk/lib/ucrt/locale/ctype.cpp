//
// ctype.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Definition of the character classification data used by the character
// classification macros in <ctype.h>.
//
#include <corecrt_internal.h>
#include <ctype.h>



extern "C" unsigned short const* __cdecl __pwctype_func()
{
    return _pwctype;
}

extern "C" unsigned short const* __cdecl __pctype_func()
{
    // This is a leaf function and thus use of _LocaleUpdate is not required.
    __acrt_ptd* const ptd{__acrt_getptd()};
    __crt_locale_data* locale_info{ptd->_locale_info};

    __acrt_update_locale_info(ptd, &locale_info);
    return locale_info->_public._locale_pctype;
}



// NOTE:  The characteristics stored in this lookup table are not always
// consistent with what is required by C.  For example, GetStringTypeW returns
// _DIGIT and _PUNCT for U+00B2 and U+00B3 (SUBSCRIPT TWO and SUBSCRIPT THREE),
// but these categories are mutually exclusive in C.  We have chosen to
// implement this lookup table for consistency with GetStringTypeW, rather than
// try to implement something completely different.  We expect this is sufficient,
// as C character classification does not map well to Unicode.
extern "C" unsigned short const _wctype[]
{
    0,                              // -1 EOF
    _CONTROL ,                      // 00 (NUL)
    _CONTROL ,                      // 01 (SOH)
    _CONTROL ,                      // 02 (STX)
    _CONTROL ,                      // 03 (ETX)
    _CONTROL ,                      // 04 (EOT)
    _CONTROL ,                      // 05 (ENQ)
    _CONTROL ,                      // 06 (ACK)
    _CONTROL ,                      // 07 (BEL)
    _CONTROL ,                      // 08 (BS)
    // \t is a blank character, but is not registered as _Blank on the table, because that will make it
    //printable. Also Windows (via GetStringType()) considered all _BLANK characters to also be _PRINT characters,
    //so does not have a way to specify blank, non-printable.
    _SPACE | _CONTROL ,             // 09 (HT)
    _SPACE | _CONTROL ,             // 0A (LF)
    _SPACE | _CONTROL ,             // 0B (VT)
    _SPACE | _CONTROL ,             // 0C (FF)
    _SPACE | _CONTROL ,             // 0D (CR)
    _CONTROL ,                      // 0E (SI)
    _CONTROL ,                      // 0F (SO)
    _CONTROL ,                      // 10 (DLE)
    _CONTROL ,                      // 11 (DC1)
    _CONTROL ,                      // 12 (DC2)
    _CONTROL ,                      // 13 (DC3)
    _CONTROL ,                      // 14 (DC4)
    _CONTROL ,                      // 15 (NAK)
    _CONTROL ,                      // 16 (SYN)
    _CONTROL ,                      // 17 (ETB)
    _CONTROL ,                      // 18 (CAN)
    _CONTROL ,                      // 19 (EM)
    _CONTROL ,                      // 1A (SUB)
    _CONTROL ,                      // 1B (ESC)
    _CONTROL ,                      // 1C (FS)
    _CONTROL ,                      // 1D (GS)
    _CONTROL ,                      // 1E (RS)
    _CONTROL ,                      // 1F (US)
    _SPACE | _BLANK ,               // 20 SPACE
    _PUNCT ,                        // 21 !
    _PUNCT ,                        // 22 "
    _PUNCT ,                        // 23 #
    _PUNCT ,                        // 24 $
    _PUNCT ,                        // 25 %
    _PUNCT ,                        // 26 &
    _PUNCT ,                        // 27 '
    _PUNCT ,                        // 28 (
    _PUNCT ,                        // 29 )
    _PUNCT ,                        // 2A *
    _PUNCT ,                        // 2B +
    _PUNCT ,                        // 2C ,
    _PUNCT ,                        // 2D -
    _PUNCT ,                        // 2E .
    _PUNCT ,                        // 2F /
    _DIGIT | _HEX ,                 // 30 0
    _DIGIT | _HEX ,                 // 31 1
    _DIGIT | _HEX ,                 // 32 2
    _DIGIT | _HEX ,                 // 33 3
    _DIGIT | _HEX ,                 // 34 4
    _DIGIT | _HEX ,                 // 35 5
    _DIGIT | _HEX ,                 // 36 6
    _DIGIT | _HEX ,                 // 37 7
    _DIGIT | _HEX ,                 // 38 8
    _DIGIT | _HEX ,                 // 39 9
    _PUNCT ,                        // 3A :
    _PUNCT ,                        // 3B ;
    _PUNCT ,                        // 3C <
    _PUNCT ,                        // 3D =
    _PUNCT ,                        // 3E >
    _PUNCT ,                        // 3F ?
    _PUNCT ,                        // 40 @
    _UPPER | _HEX | C1_ALPHA ,      // 41 A
    _UPPER | _HEX | C1_ALPHA ,      // 42 B
    _UPPER | _HEX | C1_ALPHA ,      // 43 C
    _UPPER | _HEX | C1_ALPHA ,      // 44 D
    _UPPER | _HEX | C1_ALPHA ,      // 45 E
    _UPPER | _HEX | C1_ALPHA ,      // 46 F
    _UPPER | C1_ALPHA ,             // 47 G
    _UPPER | C1_ALPHA ,             // 48 H
    _UPPER | C1_ALPHA ,             // 49 I
    _UPPER | C1_ALPHA ,             // 4A J
    _UPPER | C1_ALPHA ,             // 4B K
    _UPPER | C1_ALPHA ,             // 4C L
    _UPPER | C1_ALPHA ,             // 4D M
    _UPPER | C1_ALPHA ,             // 4E N
    _UPPER | C1_ALPHA ,             // 4F O
    _UPPER | C1_ALPHA ,             // 50 P
    _UPPER | C1_ALPHA ,             // 51 Q
    _UPPER | C1_ALPHA ,             // 52 R
    _UPPER | C1_ALPHA ,             // 53 S
    _UPPER | C1_ALPHA ,             // 54 T
    _UPPER | C1_ALPHA ,             // 55 U
    _UPPER | C1_ALPHA ,             // 56 V
    _UPPER | C1_ALPHA ,             // 57 W
    _UPPER | C1_ALPHA ,             // 58 X
    _UPPER | C1_ALPHA ,             // 59 Y
    _UPPER | C1_ALPHA ,             // 5A Z
    _PUNCT ,                        // 5B [
    _PUNCT ,                        // 5C '\'
    _PUNCT ,                        // 5D ]
    _PUNCT ,                        // 5E ^
    _PUNCT ,                        // 5F _
    _PUNCT ,                        // 60 `
    _LOWER | _HEX | C1_ALPHA ,      // 61 a
    _LOWER | _HEX | C1_ALPHA ,      // 62 b
    _LOWER | _HEX | C1_ALPHA ,      // 63 c
    _LOWER | _HEX | C1_ALPHA ,      // 64 d
    _LOWER | _HEX | C1_ALPHA ,      // 65 e
    _LOWER | _HEX | C1_ALPHA ,      // 66 f
    _LOWER | C1_ALPHA ,             // 67 g
    _LOWER | C1_ALPHA ,             // 68 h
    _LOWER | C1_ALPHA ,             // 69 i
    _LOWER | C1_ALPHA ,             // 6A j
    _LOWER | C1_ALPHA ,             // 6B k
    _LOWER | C1_ALPHA ,             // 6C l
    _LOWER | C1_ALPHA ,             // 6D m
    _LOWER | C1_ALPHA ,             // 6E n
    _LOWER | C1_ALPHA ,             // 6F o
    _LOWER | C1_ALPHA ,             // 70 p
    _LOWER | C1_ALPHA ,             // 71 q
    _LOWER | C1_ALPHA ,             // 72 r
    _LOWER | C1_ALPHA ,             // 73 s
    _LOWER | C1_ALPHA ,             // 74 t
    _LOWER | C1_ALPHA ,             // 75 u
    _LOWER | C1_ALPHA ,             // 76 v
    _LOWER | C1_ALPHA ,             // 77 w
    _LOWER | C1_ALPHA ,             // 78 x
    _LOWER | C1_ALPHA ,             // 79 y
    _LOWER | C1_ALPHA ,             // 7A z
    _PUNCT ,                        // 7B {
    _PUNCT ,                        // 7C |
    _PUNCT ,                        // 7D }
    _PUNCT ,                        // 7E ~
    _CONTROL ,                      // 7F (DEL)
    _CONTROL ,                      // 80 (XXX)
    _CONTROL ,                      // 81 (XXX)
    _CONTROL ,                      // 82 (BPH / BREAK PERMITTED HERE)
    _CONTROL ,                      // 83 (NBH / NO BREAK HERE)
    _CONTROL ,                      // 84 (IND / formerly known as INDEX)
    _SPACE | _CONTROL ,             // 85 (NEL / NEXT LINE)
    _CONTROL ,                      // 86 (SSA / START OF SELCETED AREA)
    _CONTROL ,                      // 87 (ESA / END OF SELECTED AREA)
    _CONTROL ,                      // 88 (HTS / CHARACTER TABULATION SET)
    _CONTROL ,                      // 89 (HTJ / CHARACTER TABULATION WITH JUSTIFICATION)
    _CONTROL ,                      // 8A (VTS / LINE TABULATION SET)
    _CONTROL ,                      // 8B (PLD / PARTIAL LINE FORWARD)
    _CONTROL ,                      // 8C (PLU / PARTIAL LINE BACKWARD)
    _CONTROL ,                      // 8D (RI  / REVERSE LINE FEED)
    _CONTROL ,                      // 8E (SS2 / SINGLE SHIFT TWO)
    _CONTROL ,                      // 8F (SS3 / SINGLE SHIFT THREE)
    _CONTROL ,                      // 90 (DCS / DEVICE CONTROL STRING)
    _CONTROL ,                      // 91 (PU1 / PRIVATE USE ONE)
    _CONTROL ,                      // 92 (PU2 / PRIVATE USE TWO)
    _CONTROL ,                      // 93 (STS / SET TRANSMIT STATE)
    _CONTROL ,                      // 94 (CCH / CANCEL CHARACTER)
    _CONTROL ,                      // 95 (MW  / MESSAGE WAITING)
    _CONTROL ,                      // 96 (SPA / START OF GUARDED AREA)
    _CONTROL ,                      // 97 (EPA / END OF GUARDED AREA)
    _CONTROL ,                      // 98 (SOS / START OF STRING)
    _CONTROL ,                      // 99 (XXX)
    _CONTROL ,                      // 9A (SCI / SINGLE CHARACTER INTRODUCER)
    _CONTROL ,                      // 9B (CSI / CONTROL SEQUENCE INTRODUCER)
    _CONTROL ,                      // 9C (ST  / STRING TERMINATOR)
    _CONTROL ,                      // 9D (OSC / OPERATING SYSTEM COMMAND)
    _CONTROL ,                      // 9E (PM  / PRIVACY MESSAGE)
    _CONTROL ,                      // 9F (APC / APPLICATION PROGRAM COMMAND)
    _SPACE ,                        // A0 (NBSP / NO-BREAK SPACE)
    _PUNCT ,                        // A1 (INVERTED EXCLAMATION MARK)
    _PUNCT ,                        // A2 (CENT SIGN)
    _PUNCT ,                        // A3 (POUND SIGN)
    _PUNCT ,                        // A4 (CURRENCY SIGN)
    _PUNCT ,                        // A5 (YEN SIGN)
    _PUNCT ,                        // A6 (BROKEN BAR)
    _PUNCT ,                        // A7 (SECTION SIGN)
    _PUNCT ,                        // A8 (DIAERESIS)
    _PUNCT ,                        // A9 (COPYRIGHT SIGN)
    _LOWER | _PUNCT | C1_ALPHA,     // AA (FEMININE ORDINAL INDICATOR)
    _PUNCT ,                        // AB (LEFT-POINTING DOUBLE ANGLE QUOTATION MARK)
    _PUNCT ,                        // AC (NOT SIGN)
    _PUNCT | _CONTROL ,             // AD (SOFT HYPHEN)
    _PUNCT ,                        // AE (REGISTERED SIGN)
    _PUNCT ,                        // AF (MACRON)
    _PUNCT ,                        // B0 (DEGREE SIGN)
    _PUNCT ,                        // B1 (PLUS-MINUS SIGN)
    _DIGIT | _PUNCT ,               // B2 (SUPERSCRIPT TWO)
    _DIGIT | _PUNCT ,               // B3 (SUPERSCRIPT THREE)
    _PUNCT ,                        // B4 (ACUTE ACCENT)
    _LOWER | _PUNCT | C1_ALPHA,     // B5 (MICRO SIGN)
    _PUNCT ,                        // B6 (PILCROW SIGN)
    _PUNCT ,                        // B7 (MIDDLE DOT)
    _PUNCT ,                        // B8 (CEDILLA)
    _DIGIT | _PUNCT ,               // B9 (SUPERSCRIPT ONE)
    _LOWER | _PUNCT | C1_ALPHA,     // BA (MASCULINE ORDINAL INDICATOR)
    _PUNCT ,                        // BB (RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK)
    _PUNCT ,                        // BC (VULGAR FRACTION ONE QUARTER)
    _PUNCT ,                        // BD (VULGAR FRACTION ONE HALF)
    _PUNCT ,                        // BE (VULGAR FRACTION THREE QUARTERS)
    _PUNCT ,                        // BF (INVERTED QUESTION MARK)
    _UPPER | C1_ALPHA ,             // C0 (LATIN CAPTIAL LETTER A WITH GRAVE)
    _UPPER | C1_ALPHA ,             // C1 (LATIN CAPITAL LETTER A WITH ACUTE)
    _UPPER | C1_ALPHA ,             // C2 (LATIN CAPITAL LETTER A WITH CIRCUMFLEX)
    _UPPER | C1_ALPHA ,             // C3 (LATIN CAPITAL LETTER A WITH TILDE)
    _UPPER | C1_ALPHA ,             // C4 (LATIN CAPITAL LETTER A WITH DIAERESIS)
    _UPPER | C1_ALPHA ,             // C5 (LATIN CAPITAL LETTER A WITH RING ABOVE)
    _UPPER | C1_ALPHA ,             // C6 (LATIN CAPITAL LETTER AE)
    _UPPER | C1_ALPHA ,             // C7 (LATIN CAPITAL LETTER C WITH CEDILLA)
    _UPPER | C1_ALPHA ,             // C8 (LATIN CAPITAL LETTER E WITH GRAVE)
    _UPPER | C1_ALPHA ,             // C9 (LATIN CAPITAL LETTER E WITH ACUTE)
    _UPPER | C1_ALPHA ,             // CA (LATIN CAPITAL LETTER E WITH CIRCUMFLEX)
    _UPPER | C1_ALPHA ,             // CB (LATIN CAPITAL LETTER E WITH DIAERESIS)
    _UPPER | C1_ALPHA ,             // CC (LATIN CAPITAL LETTER I WITH GRAVE)
    _UPPER | C1_ALPHA ,             // CD (LATIN CAPITAL LETTER I WITH ACUTE)
    _UPPER | C1_ALPHA ,             // CE (LATIN CAPITAL LETTER I WITH CIRCUMFLEX)
    _UPPER | C1_ALPHA ,             // CF (LATIN CAPITAL LETTER I WITH DIAERESIS
    _UPPER | C1_ALPHA ,             // D0 (LATIN CAPITAL LETTER ETH)
    _UPPER | C1_ALPHA ,             // D1 (LATIN CAPITAL LETTER N WITH TILDE)
    _UPPER | C1_ALPHA ,             // D2 (LATIN CAPITAL LETTER O WITH GRAVE)
    _UPPER | C1_ALPHA ,             // D3 (LATIN CAPITAL LETTER O WITH ACUTE)
    _UPPER | C1_ALPHA ,             // D4 (LATIN CAPITAL LETTER O WITH CIRCUMFLEX)
    _UPPER | C1_ALPHA ,             // D5 (LATIN CAPITAL LETTER O WITH TILDE)
    _UPPER | C1_ALPHA ,             // D6 (LATIN CAPITAL LETTER O WITH DIAERESIS)
    _PUNCT ,                        // D7 (MULTIPLICATION SIGN)
    _UPPER | C1_ALPHA ,             // D8 (LATIN CAPITAL LETTER O WITH STROKE)
    _UPPER | C1_ALPHA ,             // D9 (LATIN CAPITAL LETTER U WITH GRAVE)
    _UPPER | C1_ALPHA ,             // DA (LATIN CAPITAL LETTER U WITH ACUTE)
    _UPPER | C1_ALPHA ,             // DB (LATIN CAPITAL LETTER U WITH CIRCUMFLEX)
    _UPPER | C1_ALPHA ,             // DC (LATIN CAPITAL LETTER U WITH DIAERESIS)
    _UPPER | C1_ALPHA ,             // DD (LATIN CAPITAL LETTER Y WITH ACUTE)
    _UPPER | C1_ALPHA ,             // DE (LATIN CAPITAL LETTER THORN)
    _LOWER | C1_ALPHA ,             // DF (LATIN SMALL LETTER SHARP S)
    _LOWER | C1_ALPHA ,             // E0 (LATIN SMALL LETTER A WITH GRAVE)
    _LOWER | C1_ALPHA ,             // E1 (LATIN SMALL LETTER A WITH ACUTE)
    _LOWER | C1_ALPHA ,             // E2 (LATIN SMALL LETTER A WITH CIRCUMFLEX)
    _LOWER | C1_ALPHA ,             // E3 (LATIN SMALL LETTER A WITH TILDE)
    _LOWER | C1_ALPHA ,             // E4 (LATIN SMALL LETTER A WITH DIAERESIS)
    _LOWER | C1_ALPHA ,             // E5 (LATIN SMALL LETTER A WITH RING ABOVE)
    _LOWER | C1_ALPHA ,             // E6 (LATIN SMALL LETTER AE)
    _LOWER | C1_ALPHA ,             // E7 (LATIN SMALL LETTER C WITH CEDILLA)
    _LOWER | C1_ALPHA ,             // E8 (LATIN SMALL LETTER E WITH GRAVE)
    _LOWER | C1_ALPHA ,             // E9 (LATIN SMALL LETTER E WITH ACUTE)
    _LOWER | C1_ALPHA ,             // EA (LATIN SMALL LETTER E WITH CIRCUMFLEX)
    _LOWER | C1_ALPHA ,             // EB (LATIN SMALL LETTER E WITH DIAERESIS)
    _LOWER | C1_ALPHA ,             // EC (LATIN SMALL LETTER I WITH GRAVE)
    _LOWER | C1_ALPHA ,             // ED (LATIN SMALL LETTER I WITH ACUTE)
    _LOWER | C1_ALPHA ,             // EE (LATIN SMALL LETTER I WITH CIRCUMFLEX)
    _LOWER | C1_ALPHA ,             // EF (LATIN SMALL LETTER I WITH DIAERESIS)
    _LOWER | C1_ALPHA ,             // F0 (LATIN SMALL LETTER ETH)
    _LOWER | C1_ALPHA ,             // F1 (LATIN SMALL LETTER N WITH TILDE)
    _LOWER | C1_ALPHA ,             // F2 (LATIN SMALL LETTER O WITH GRAVE)
    _LOWER | C1_ALPHA ,             // F3 (LATIN SMALL LETTER O WITH ACUTE)
    _LOWER | C1_ALPHA ,             // F4 (LATIN SMALL LETTER O WITH CIRCUMFLEX)
    _LOWER | C1_ALPHA ,             // F5 (LATIN SMALL LETTER O WITH TILDE)
    _LOWER | C1_ALPHA ,             // F6 (LATIN SMALL LETTER O WITH DIAERESIS)
    _PUNCT ,                        // F7 (DIVISION SIGN)
    _LOWER | C1_ALPHA ,             // F8 (LATIN SMALL LETTER O WITH STROKE)
    _LOWER | C1_ALPHA ,             // F9 (LATIN SMALL LETTER U WITH GRAVE)
    _LOWER | C1_ALPHA ,             // FA (LATIN SMALL LETTER U WITH ACUTE)
    _LOWER | C1_ALPHA ,             // FB (LATIN SMALL LETTER U WITH CIRCUMFLEX)
    _LOWER | C1_ALPHA ,             // FC (LATIN SMALL LETTER U WITH DIAERESIS)
    _LOWER | C1_ALPHA ,             // FD (LATIN SMALL LETTER Y WITH ACUTE)
    _LOWER | C1_ALPHA ,             // FE (LATIN SMALL LETTER THORN)
    _LOWER | C1_ALPHA ,             // FF (LATIN SMALL LETTER Y WITH DIAERESIS)
    _UPPER | C1_ALPHA ,             //100 (LATIN CAPITAL LETTER A WITH MACRON)
};

extern "C" unsigned short const __newctype[384]
{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
    0,                         // -1 EOF
    _CONTROL,                  // 00 (NUL)
    _CONTROL,                  // 01 (SOH)
    _CONTROL,                  // 02 (STX)
    _CONTROL,                  // 03 (ETX)
    _CONTROL,                  // 04 (EOT)
    _CONTROL,                  // 05 (ENQ)
    _CONTROL,                  // 06 (ACK)
    _CONTROL,                  // 07 (BEL)
    _CONTROL,                  // 08 (BS)
    // \t is a blank character, but is not registered as _Blank on the table, because that will make it
    //printable. Also Windows (via GetStringType()) considered all _BLANK characters to also be _PRINT characters,
    //so does not have a way to specify blank, non-printable.
    _SPACE+_CONTROL,           // 09 (HT)
    _SPACE+_CONTROL,           // 0A (LF)
    _SPACE+_CONTROL,           // 0B (VT)
    _SPACE+_CONTROL,           // 0C (FF)
    _SPACE+_CONTROL,           // 0D (CR)
    _CONTROL,                  // 0E (SI)
    _CONTROL,                  // 0F (SO)
    _CONTROL,                  // 10 (DLE)
    _CONTROL,                  // 11 (DC1)
    _CONTROL,                  // 12 (DC2)
    _CONTROL,                  // 13 (DC3)
    _CONTROL,                  // 14 (DC4)
    _CONTROL,                  // 15 (NAK)
    _CONTROL,                  // 16 (SYN)
    _CONTROL,                  // 17 (ETB)
    _CONTROL,                  // 18 (CAN)
    _CONTROL,                  // 19 (EM)
    _CONTROL,                  // 1A (SUB)
    _CONTROL,                  // 1B (ESC)
    _CONTROL,                  // 1C (FS)
    _CONTROL,                  // 1D (GS)
    _CONTROL,                  // 1E (RS)
    _CONTROL,                  // 1F (US)
    _SPACE+_BLANK,             // 20 SPACE
    _PUNCT,                    // 21 !
    _PUNCT,                    // 22 "
    _PUNCT,                    // 23 #
    _PUNCT,                    // 24 $
    _PUNCT,                    // 25 %
    _PUNCT,                    // 26 &
    _PUNCT,                    // 27 '
    _PUNCT,                    // 28 (
    _PUNCT,                    // 29 )
    _PUNCT,                    // 2A *
    _PUNCT,                    // 2B +
    _PUNCT,                    // 2C ,
    _PUNCT,                    // 2D -
    _PUNCT,                    // 2E .
    _PUNCT,                    // 2F /
    _DIGIT+_HEX,               // 30 0
    _DIGIT+_HEX,               // 31 1
    _DIGIT+_HEX,               // 32 2
    _DIGIT+_HEX,               // 33 3
    _DIGIT+_HEX,               // 34 4
    _DIGIT+_HEX,               // 35 5
    _DIGIT+_HEX,               // 36 6
    _DIGIT+_HEX,               // 37 7
    _DIGIT+_HEX,               // 38 8
    _DIGIT+_HEX,               // 39 9
    _PUNCT,                    // 3A :
    _PUNCT,                    // 3B ;
    _PUNCT,                    // 3C <
    _PUNCT,                    // 3D =
    _PUNCT,                    // 3E >
    _PUNCT,                    // 3F ?
    _PUNCT,                    // 40 @
    _UPPER+_HEX,               // 41 A
    _UPPER+_HEX,               // 42 B
    _UPPER+_HEX,               // 43 C
    _UPPER+_HEX,               // 44 D
    _UPPER+_HEX,               // 45 E
    _UPPER+_HEX,               // 46 F
    _UPPER,                    // 47 G
    _UPPER,                    // 48 H
    _UPPER,                    // 49 I
    _UPPER,                    // 4A J
    _UPPER,                    // 4B K
    _UPPER,                    // 4C L
    _UPPER,                    // 4D M
    _UPPER,                    // 4E N
    _UPPER,                    // 4F O
    _UPPER,                    // 50 P
    _UPPER,                    // 51 Q
    _UPPER,                    // 52 R
    _UPPER,                    // 53 S
    _UPPER,                    // 54 T
    _UPPER,                    // 55 U
    _UPPER,                    // 56 V
    _UPPER,                    // 57 W
    _UPPER,                    // 58 X
    _UPPER,                    // 59 Y
    _UPPER,                    // 5A Z
    _PUNCT,                    // 5B [
    _PUNCT,                    // 5C \ backslash
    _PUNCT,                    // 5D ]
    _PUNCT,                    // 5E ^
    _PUNCT,                    // 5F _
    _PUNCT,                    // 60 `
    _LOWER+_HEX,               // 61 a
    _LOWER+_HEX,               // 62 b
    _LOWER+_HEX,               // 63 c
    _LOWER+_HEX,               // 64 d
    _LOWER+_HEX,               // 65 e
    _LOWER+_HEX,               // 66 f
    _LOWER,                    // 67 g
    _LOWER,                    // 68 h
    _LOWER,                    // 69 i
    _LOWER,                    // 6A j
    _LOWER,                    // 6B k
    _LOWER,                    // 6C l
    _LOWER,                    // 6D m
    _LOWER,                    // 6E n
    _LOWER,                    // 6F o
    _LOWER,                    // 70 p
    _LOWER,                    // 71 q
    _LOWER,                    // 72 r
    _LOWER,                    // 73 s
    _LOWER,                    // 74 t
    _LOWER,                    // 75 u
    _LOWER,                    // 76 v
    _LOWER,                    // 77 w
    _LOWER,                    // 78 x
    _LOWER,                    // 79 y
    _LOWER,                    // 7A z
    _PUNCT,                    // 7B {
    _PUNCT,                    // 7C |
    _PUNCT,                    // 7D }
    _PUNCT,                    // 7E ~
    _CONTROL,                  // 7F (DEL)
    // and the rest are 0...
};

extern "C" unsigned char const __newclmap[384]
{
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    0x00,   // 00 (NUL)
    0x01,   // 01 (SOH)
    0x02,   // 02 (STX)
    0x03,   // 03 (ETX)
    0x04,   // 04 (EOT)
    0x05,   // 05 (ENQ)
    0x06,   // 06 (ACK)
    0x07,   // 07 (BEL)
    0x08,   // 08 (BS)
    0x09,   // 09 (HT)
    0x0A,   // 0A (LF)
    0x0B,   // 0B (VT)
    0x0C,   // 0C (FF)
    0x0D,   // 0D (CR)
    0x0E,   // 0E (SI)
    0x0F,   // 0F (SO)
    0x10,   // 10 (DLE)
    0x11,   // 11 (DC1)
    0x12,   // 12 (DC2)
    0x13,   // 13 (DC3)
    0x14,   // 14 (DC4)
    0x15,   // 15 (NAK)
    0x16,   // 16 (SYN)
    0x17,   // 17 (ETB)
    0x18,   // 18 (CAN)
    0x19,   // 19 (EM)
    0x1A,   // 1A (SUB)
    0x1B,   // 1B (ESC)
    0x1C,   // 1C (FS)
    0x1D,   // 1D (GS)
    0x1E,   // 1E (RS)
    0x1F,   // 1F (US)
    0x20,   // 20 SPACE
    0x21,   // 21 !
    0x22,   // 22 "
    0x23,   // 23 #
    0x24,   // 24 $
    0x25,   // 25 %
    0x26,   // 26 &
    0x27,   // 27 '
    0x28,   // 28 (
    0x29,   // 29 )
    0x2A,   // 2A *
    0x2B,   // 2B +
    0x2C,   // 2C ,
    0x2D,   // 2D -
    0x2E,   // 2E .
    0x2F,   // 2F /
    0x30,   // 30 0
    0x31,   // 31 1
    0x32,   // 32 2
    0x33,   // 33 3
    0x34,   // 34 4
    0x35,   // 35 5
    0x36,   // 36 6
    0x37,   // 37 7
    0x38,   // 38 8
    0x39,   // 39 9
    0x3A,   // 3A :
    0x3B,   // 3B ;
    0x3C,   // 3C <
    0x3D,   // 3D =
    0x3E,   // 3E >
    0x3F,   // 3F ?
    0x40,   // 40 @
    0x61,   // 41 A
    0x62,   // 42 B
    0x63,   // 43 C
    0x64,   // 44 D
    0x65,   // 45 E
    0x66,   // 46 F
    0x67,   // 47 G
    0x68,   // 48 H
    0x69,   // 49 I
    0x6A,   // 4A J
    0x6B,   // 4B K
    0x6C,   // 4C L
    0x6D,   // 4D M
    0x6E,   // 4E N
    0x6F,   // 4F O
    0x70,   // 50 P
    0x71,   // 51 Q
    0x72,   // 52 R
    0x73,   // 53 S
    0x74,   // 54 T
    0x75,   // 55 U
    0x76,   // 56 V
    0x77,   // 57 W
    0x78,   // 58 X
    0x79,   // 59 Y
    0x7A,   // 5A Z
    0x5B,   // 5B [
    0x5C,   // 5C '\'
    0x5D,   // 5D ]
    0x5E,   // 5E ^
    0x5F,   // 5F _
    0x60,   // 60 `
    0x61,   // 61 a
    0x62,   // 62 b
    0x63,   // 63 c
    0x64,   // 64 d
    0x65,   // 65 e
    0x66,   // 66 f
    0x67,   // 67 g
    0x68,   // 68 h
    0x69,   // 69 i
    0x6A,   // 6A j
    0x6B,   // 6B k
    0x6C,   // 6C l
    0x6D,   // 6D m
    0x6E,   // 6E n
    0x6F,   // 6F o
    0x70,   // 70 p
    0x71,   // 71 q
    0x72,   // 72 r
    0x73,   // 73 s
    0x74,   // 74 t
    0x75,   // 75 u
    0x76,   // 76 v
    0x77,   // 77 w
    0x78,   // 78 x
    0x79,   // 79 y
    0x7A,   // 7A z
    0x7B,   // 7B {
    0x7C,   // 7C |
    0x7D,   // 7D }
    0x7E,   // 7E ~
    0x7F,   // 7F (DEL)
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};

extern "C" unsigned char const __newcumap[384]
{
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
    0x00,   // 00 (NUL)
    0x01,   // 01 (SOH)
    0x02,   // 02 (STX)
    0x03,   // 03 (ETX)
    0x04,   // 04 (EOT)
    0x05,   // 05 (ENQ)
    0x06,   // 06 (ACK)
    0x07,   // 07 (BEL)
    0x08,   // 08 (BS)
    0x09,   // 09 (HT)
    0x0A,   // 0A (LF)
    0x0B,   // 0B (VT)
    0x0C,   // 0C (FF)
    0x0D,   // 0D (CR)
    0x0E,   // 0E (SI)
    0x0F,   // 0F (SO)
    0x10,   // 10 (DLE)
    0x11,   // 11 (DC1)
    0x12,   // 12 (DC2)
    0x13,   // 13 (DC3)
    0x14,   // 14 (DC4)
    0x15,   // 15 (NAK)
    0x16,   // 16 (SYN)
    0x17,   // 17 (ETB)
    0x18,   // 18 (CAN)
    0x19,   // 19 (EM)
    0x1A,   // 1A (SUB)
    0x1B,   // 1B (ESC)
    0x1C,   // 1C (FS)
    0x1D,   // 1D (GS)
    0x1E,   // 1E (RS)
    0x1F,   // 1F (US)
    0x20,   // 20 SPACE
    0x21,   // 21 !
    0x22,   // 22 "
    0x23,   // 23 #
    0x24,   // 24 $
    0x25,   // 25 %
    0x26,   // 26 &
    0x27,   // 27 '
    0x28,   // 28 (
    0x29,   // 29 )
    0x2A,   // 2A *
    0x2B,   // 2B +
    0x2C,   // 2C ,
    0x2D,   // 2D -
    0x2E,   // 2E .
    0x2F,   // 2F /
    0x30,   // 30 0
    0x31,   // 31 1
    0x32,   // 32 2
    0x33,   // 33 3
    0x34,   // 34 4
    0x35,   // 35 5
    0x36,   // 36 6
    0x37,   // 37 7
    0x38,   // 38 8
    0x39,   // 39 9
    0x3A,   // 3A :
    0x3B,   // 3B ;
    0x3C,   // 3C <
    0x3D,   // 3D =
    0x3E,   // 3E >
    0x3F,   // 3F ?
    0x40,   // 40 @
    0x41,   // 41 A
    0x42,   // 42 B
    0x43,   // 43 C
    0x44,   // 44 D
    0x45,   // 45 E
    0x46,   // 46 F
    0x47,   // 47 G
    0x48,   // 48 H
    0x49,   // 49 I
    0x4A,   // 4A J
    0x4B,   // 4B K
    0x4C,   // 4C L
    0x4D,   // 4D M
    0x4E,   // 4E N
    0x4F,   // 4F O
    0x50,   // 50 P
    0x51,   // 51 Q
    0x52,   // 52 R
    0x53,   // 53 S
    0x54,   // 54 T
    0x55,   // 55 U
    0x56,   // 56 V
    0x57,   // 57 W
    0x58,   // 58 X
    0x59,   // 59 Y
    0x5A,   // 5A Z
    0x5B,   // 5B [
    0x5C,   // 5C '\'
    0x5D,   // 5D ]
    0x5E,   // 5E ^
    0x5F,   // 5F _
    0x60,   // 60 `
    0x41,   // 61 a
    0x42,   // 62 b
    0x43,   // 63 c
    0x44,   // 64 d
    0x45,   // 65 e
    0x46,   // 66 f
    0x47,   // 67 g
    0x48,   // 68 h
    0x49,   // 69 i
    0x4A,   // 6A j
    0x4B,   // 6B k
    0x4C,   // 6C l
    0x4D,   // 6D m
    0x4E,   // 6E n
    0x4F,   // 6F o
    0x50,   // 70 p
    0x51,   // 71 q
    0x52,   // 72 r
    0x53,   // 73 s
    0x54,   // 74 t
    0x55,   // 75 u
    0x56,   // 76 v
    0x57,   // 77 w
    0x58,   // 78 x
    0x59,   // 79 y
    0x5A,   // 7A z
    0x7B,   // 7B {
    0x7C,   // 7C |
    0x7D,   // 7D }
    0x7E,   // 7E ~
    0x7F,   // 7F (DEL)
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};



extern "C" { unsigned short const* _pctype {__newctype + 128}; } // Pointer to table for char's
extern "C" { unsigned short const* _pwctype{_wctype    + 1  }; } // Pointer to table for wchar_t's
