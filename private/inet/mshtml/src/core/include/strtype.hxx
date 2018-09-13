//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       strtype.hxx
//
//  Contents:   Forms^3 (Trident) utilities
//
//----------------------------------------------------------------------------

#ifndef I_STRTYPE_HXX_
#define I_STRTYPE_HXX_
#pragma INCMSG("--- Beg 'strtype.hxx'")

#undef IsCharAlpha
#undef IsCharUpper
#undef IsCharLower
#undef IsCharAlphaNumeric

#define IsCharAlpha FormsIsCharAlpha
#define IsCharSpace FormsIsCharSpace
#define IsCharPunct FormsIsCharPunct
#define IsCharDigit FormsIsCharDigit
#define IsCharCntrl FormsIsCharCntrl
#define IsCharXDigit FormsIsCharXDigit
#define IsCharUpper FormsIsCharUpper
#define IsCharLower FormsIsCharLower
#define IsCharBlank FormsIsCharBlank
#define IsCharBlank FormsIsCharBlank
#define IsCharAlphaNumeric FormsIsCharAlphaNumeric

extern "C" DWORD adwData[218];
extern "C" BYTE abIndex[98][8];
extern "C" BYTE abType1Alpha[256];
extern "C" BYTE abType1Punct[256];
extern "C" BYTE abType1Digit[256];
// extern BYTE abType1Upper[256];  // unused
// extern BYTE abType1Lower[256];  // unused

#define __BIT_SHIFT 0
#define __INDEX_SHIFT 5
#define __PAGE_SHIFT 8

#define __BIT_MASK 31
#define __INDEX_MASK 7

// straight lookup functions are inlined.

#define ISCHARFUNC(type, wch) \
{ \
    return (adwData[abIndex[abType1##type[wch>>__PAGE_SHIFT]] \
                          [(wch>>__INDEX_SHIFT)&__INDEX_MASK]] \
            >> (wch&__BIT_MASK)) & 1; \
}
    
extern "C" { inline BOOL FormsIsCharAlpha(WCHAR wch) ISCHARFUNC(Alpha, wch) }
extern "C" { inline BOOL FormsIsCharPunct(WCHAR wch) ISCHARFUNC(Punct, wch) }
extern "C" { inline BOOL FormsIsCharDigit(WCHAR wch) ISCHARFUNC(Digit, wch) }
// extern "C" { inline BOOL FormsIsCharUpper(WCHAR wch) ISCHARFUNC(Upper, wch) }
// extern "C" { inline BOOL FormsIsCharLower(WCHAR wch) ISCHARFUNC(Lower, wch) }

// switched lookup functions are not inlined.

extern "C" BOOL FormsIsCharCntrl(WCHAR wch);
extern "C" BOOL FormsIsCharXDigit(WCHAR wch);
extern "C" BOOL FormsIsCharSpace(WCHAR wch);
extern "C" BOOL FormsIsCharAlphaNumeric(WCHAR wch);
extern "C" BOOL FormsIsCharBlank(WCHAR wch);

//
//  Trident CType 3 Flag Bits.
//
//  In the interest of reducing our table complexity, we've here a reduced
//  bitfield.  Only those bits currently used by Trident are returned by
//  GetStringType3Ex().
//

#undef C3_NONSPACING    // was 0x0001
#undef C3_DIACRITIC     // was 0x0002
#undef C3_VOWELMARK     // was 0x0004
#undef C3_SYMBOL        // was 0x0008
#undef C3_KATAKANA      // was 0x0010
#undef C3_HIRAGANA      // was 0x0020
#undef C3_HALFWIDTH     // was 0x0040
#undef C3_FULLWIDTH     // was 0x0080
#undef C3_IDEOGRAPH     // was 0x0100
#undef C3_KASHIDA       // was 0x0200
#undef C3_LEXICAL       // was 0x0400
#undef C3_ALPHA         // was 0x8000

#define C3_SYMBOL       0x0001
#define C3_KATAKANA     0x0002
#define C3_HIRAGANA     0x0004
#define C3_HALFWIDTH    0x0008
#define C3_FULLWIDTH    0x0010
#define C3_IDEOGRAPH    0x0020

BOOL _stdcall GetStringType3Ex( LPCWSTR, int, LPBYTE );

#pragma INCMSG("--- End 'strtype.hxx'")
#else
#pragma INCMSG("*** Dup 'strtype.hxx'")
#endif
