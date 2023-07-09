/*
 *  @doc    INTERNAL
 *
 *  @module UNIWBK.HXX -- Unicode Word-breaking Classes
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      06/19/98     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I__UNIWBK_H_
#define I__UNIWBK_H_
#pragma INCMSG("--- Beg 'uniwbk.h'")

typedef BYTE CHAR_CLASS;
typedef BYTE WBKCLS;

enum
{
    wbkclsPunctSymb,     // 0
    wbkclsKanaFollow,    // 1
    wbkclsKatakanaW,     // 2
    wbkclsHiragana,      // 3
    wbkclsTab,           // 4
    wbkclsKanaDelim,     // 5
    wbkclsPrefix,        // 6
    wbkclsPostfix,       // 7
    wbkclsSpaceA,        // 8
    wbkclsAlpha,         // 9
    wbkclsIdeoW,         // 10
    wbkclsSuperSub,      // 11
    wbkclsDigitsN,       // 12
    wbkclsPunctInText,   // 13
    wbkclsDigitsW,       // 14
    wbkclsKatakanaN,     // 15
    wbkclsHangul,        // 16
    wbkclsLatinW,        // 17
    wbkclsLim
};

WBKCLS WordBreakClassFromCharClass( CHAR_CLASS cc );
BOOL   IsWordBreakBoundaryDefault( WCHAR, WCHAR );
BOOL   IsProofWordBreakBoundary( WCHAR, WCHAR );

#pragma INCMSG("--- End 'uniwbk.h'")
#else
#pragma INCMSG("*** Dup 'uniwbk.h'")
#endif
