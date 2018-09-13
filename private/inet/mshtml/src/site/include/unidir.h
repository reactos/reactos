/*
 *  @doc    INTERNAL
 *
 *  @module UNIDIR.H -- Unicode direction classes
 *
 *
 *  Owner: <nl>
 *      Michael Jochimsen <nl>
 *
 *  History: <nl>
 *      08/12/98     mikejoch created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#ifndef I__UNIDIR_H_
#define I__UNIDIR_H_
#pragma INCMSG("--- Beg 'unidir.h'")

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

// Direction class of a character.
typedef BYTE DIRCLS;

//--WARNING----WARNING----WARNING----WARNING----WARNING----WARNING----WARNING--
//--WARNING----WARNING----WARNING----WARNING----WARNING----WARNING----WARNING--
//
//  The ordering of the entries in the dircls enum is extremely important. If
//  you change it be sure to correct all the Is???Class() functions below AND
//  also review the code in lscomplx.cxx. There are a couple of lookup tables
//  in CBidiLine which are dependent on this ordering.
//
//--WARNING----WARNING----WARNING----WARNING----WARNING----WARNING----WARNING--
//--WARNING----WARNING----WARNING----WARNING----WARNING----WARNING----WARNING--

enum dircls
{
    LTR, // Left to right
    RTL, // Right to left
    ARA, // Arabic
    ANM, // Arabic numeral
    ENL, // European numeral preceeded by LTR
    ENR, // European numeral preceeded by RTL
    ENM, // European numeral
    ETM, // European numeric terminator
    ESP, // European numeric separator
    CSP, // Common numeric separator
    UNK, // Unknown
    WSP, // Whitespace
    CBN, // Combining mark
    NEU, // Neutral, whitespace, undefined
    SEG, // Segment separator (tab)
    BLK, // Block separator
    LRE, // LTR embedding
    LRO, // LTR override
    RLO, // RTL override
    RLE, // RTL embedding
    PDF, // Pop directional formatting
    FMT, // Embedding format
};

extern const DIRCLS s_aDirClassFromCharClass[];

inline BOOL IsStrongClass(DIRCLS dc)
{
    return IN_RANGE(LTR, dc, ARA);
}
inline BOOL IsFinalClass(DIRCLS dc)
{
    return IN_RANGE(LTR, dc, ENR);
}
inline BOOL IsNumericClass(DIRCLS dc)
{
    return IN_RANGE(ANM, dc, ENM);
}
inline BOOL IsResolvedEuroNumClass(DIRCLS dc)
{
    return IN_RANGE(ENL, dc, ENR);
}
inline BOOL IsNumericPunctuationClass(DIRCLS dc)
{
    return IN_RANGE(ETM, dc, CSP);
}
inline BOOL IsNumericSeparatorClass(DIRCLS dc)
{
    return IN_RANGE(ESP, dc, CSP);
}
inline BOOL IsNeutralClass(DIRCLS dc)
{
    return IN_RANGE(UNK, dc, NEU);
}
inline BOOL IsIndeterminateClass(DIRCLS dc)
{
    return IN_RANGE(ETM, dc, NEU);
}
inline BOOL IsBreakOrEmbeddingClass(DIRCLS dc)
{
    return IN_RANGE(SEG, dc, PDF);
}
inline BOOL IsEmbeddingClass(DIRCLS dc)
{
    return IN_RANGE(LRE, dc, RLE);
}
inline BOOL IsOverrideClass(DIRCLS dc)
{
    return IN_RANGE(LRO, dc, RLO);
}
inline BOOL IsRTLEmbeddingClass(DIRCLS dc)
{
    return IN_RANGE(RLO, dc, RLE);
}

#pragma INCMSG("--- End 'unidir.h'")
#else
#pragma INCMSG("*** Dup 'unidir.h'")
#endif

