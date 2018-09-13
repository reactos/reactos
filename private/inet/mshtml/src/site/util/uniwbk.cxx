/*
 *  @doc    INTERNAL
 *
 *  @module UNIWBK.CXX -- Unicode Word-breaking classes
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

#include "headers.hxx"

#ifndef X__UNIWBK_H_
#define X__UNIWBK_H_
#include "uniwbk.h"
#endif

#ifndef X__TXTDEFS_H
#define X__TXTDEFS_H
#include <txtdefs.h>
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#ifndef X__UNIPART_H
#define X__UNIPART_H
#include <unipart.h>
#endif

const WBKCLS awbkclsWordBreakClassFromCharClass[]=
{
    // CC           wbkcls
    /* WOB_   1*/   wbkclsPunctSymb,
    /* NOPP   2*/   wbkclsPunctSymb,
    /* NOPA   2*/   wbkclsPunctSymb,
    /* NOPW   2*/   wbkclsPunctSymb,
    /* HOP_   3*/   wbkclsPunctSymb,
    /* WOP_   4*/   wbkclsPunctSymb,
    /* WOP5   5*/   wbkclsPunctSymb,
    /* NOQW   6*/   wbkclsPunctSymb,
    /* AOQW   7*/   wbkclsPunctSymb,
    /* WOQ_   8*/   wbkclsPunctSymb,
    /* WCB_   9*/   wbkclsPunctSymb,
    /* NCPP  10*/   wbkclsPunctSymb,
    /* NCPA  10*/   wbkclsPunctSymb,
    /* NCPW  10*/   wbkclsPunctSymb,
    /* HCP_  11*/   wbkclsPunctSymb,
    /* WCP_  12*/   wbkclsPunctSymb,
    /* WCP5  13*/   wbkclsPunctSymb,
    /* NCQW  14*/   wbkclsPunctSymb,
    /* ACQW  15*/   wbkclsPunctSymb,
    /* WCQ_  16*/   wbkclsPunctSymb,
    /* ARQW  17*/   wbkclsPunctInText,
    /* NCSA  18*/   wbkclsPunctSymb,
    /* HCO_  19*/   wbkclsPunctSymb,
    /* WC__  20*/   wbkclsPunctSymb,
    /* WCS_  20*/   wbkclsPunctSymb,
    /* WC5_  21*/   wbkclsPunctSymb,
    /* WC5S  21*/   wbkclsPunctSymb,
    /* NKS_  22*/   wbkclsKanaFollow,
    /* WKSM  23*/   wbkclsKanaFollow,
    /* WIM_  24*/   wbkclsIdeoW,
    /* NSSW  25*/   wbkclsPunctSymb,
    /* WSS_  26*/   wbkclsPunctSymb,
    /* WHIM  27*/   wbkclsHiragana,
    /* WKIM  28*/   wbkclsKatakanaW,
    /* NKSL  29*/   wbkclsKatakanaN,
    /* WKS_  30*/   wbkclsKatakanaW,
    /* WKSC  30*/   wbkclsKatakanaW,
    /* WHS_  31*/   wbkclsHiragana,
    /* NQFP  32*/   wbkclsPunctSymb,
    /* NQFA  32*/   wbkclsPunctSymb,
    /* WQE_  33*/   wbkclsPunctSymb,
    /* WQE5  34*/   wbkclsPunctSymb,
    /* NKCC  35*/   wbkclsKanaDelim,
    /* WKC_  36*/   wbkclsKanaDelim,
    /* NOCP  37*/   wbkclsPunctSymb,
    /* NOCA  37*/   wbkclsPunctSymb,
    /* NOCW  37*/   wbkclsPunctSymb,
    /* WOC_  38*/   wbkclsPunctSymb,
    /* WOCS  38*/   wbkclsPunctSymb,
    /* WOC5  39*/   wbkclsPunctSymb,
    /* WOC6  39*/   wbkclsPunctSymb,
    /* AHPW  40*/   wbkclsPunctInText,
    /* NPEP  41*/   wbkclsPunctSymb,
    /* NPAR  41*/   wbkclsPunctSymb,
    /* HPE_  42*/   wbkclsPunctSymb,
    /* WPE_  43*/   wbkclsPunctSymb,
    /* WPES  43*/   wbkclsPunctSymb,
    /* WPE5  44*/   wbkclsPunctSymb,
    /* NISW  45*/   wbkclsPunctSymb,
    /* AISW  46*/   wbkclsPunctSymb,
    /* NQCS  47*/   wbkclsPunctInText,
    /* NQCW  47*/   wbkclsPunctInText,
    /* NQCC  47*/   wbkclsPunctInText,
    /* NPTA  48*/   wbkclsPrefix,
    /* NPNA  48*/   wbkclsPrefix,
    /* NPEW  48*/   wbkclsPrefix,
    /* NPEH  48*/   wbkclsPrefix,
    /* APNW  49*/   wbkclsPrefix,
    /* HPEW  50*/   wbkclsPrefix,
    /* WPR_  51*/   wbkclsPrefix,
    /* NQEP  52*/   wbkclsPostfix,
    /* NQEW  52*/   wbkclsPostfix,
    /* NQNW  52*/   wbkclsPostfix,
    /* AQEW  53*/   wbkclsPostfix,
    /* AQNW  53*/   wbkclsPostfix,
    /* AQLW  53*/   wbkclsPostfix,
    /* WQO_  54*/   wbkclsPostfix,
    /* NSBL  55*/   wbkclsSpaceA,
    /* WSP_  56*/   wbkclsSpaceA,
    /* WHI_  57*/   wbkclsHiragana,
    /* NKA_  58*/   wbkclsKatakanaN,
    /* WKA_  59*/   wbkclsKatakanaW,
    /* ASNW  60*/   wbkclsPunctSymb,
    /* ASEW  60*/   wbkclsPunctSymb,
    /* ASRN  60*/   wbkclsPunctSymb,
    /* ASEN  60*/   wbkclsPunctSymb,
    /* ALA_  61*/   wbkclsAlpha,
    /* AGR_  62*/   wbkclsAlpha,
    /* ACY_  63*/   wbkclsAlpha,
    /* WID_  64*/   wbkclsIdeoW,
    /* WPUA  65*/   wbkclsIdeoW,
    /* NHG_  66*/   wbkclsHangul,
    /* WHG_  67*/   wbkclsHangul,
    /* WCI_  68*/   wbkclsIdeoW,
    /* NOI_  69*/   wbkclsIdeoW,
    /* WOI_  70*/   wbkclsIdeoW,
    /* WOIC  70*/   wbkclsIdeoW,
    /* WOIL  70*/   wbkclsIdeoW,
    /* WOIS  70*/   wbkclsIdeoW,
    /* WOIT  70*/   wbkclsIdeoW,
    /* NSEN  71*/   wbkclsSuperSub,
    /* NSET  71*/   wbkclsSuperSub,
    /* NSNW  71*/   wbkclsSuperSub,
    /* ASAN  72*/   wbkclsSuperSub,
    /* ASAE  72*/   wbkclsSuperSub,
    /* NDEA  73*/   wbkclsDigitsN,
    /* WD__  74*/   wbkclsDigitsW,
    /* NLLA  75*/   wbkclsAlpha,
    /* WLA_  76*/   wbkclsLatinW,
    /* NWBL  77*/   wbkclsSpaceA,
    /* NWZW  77*/   wbkclsSpaceA,
    /* NPLW  78*/   wbkclsPunctInText,
    /* NPZW  78*/   wbkclsPunctInText,
    /* NPF_  78*/   wbkclsPunctInText,
    /* NPFL  78*/   wbkclsPunctInText,
    /* NPNW  78*/   wbkclsPunctInText,
    /* APLW  79*/   wbkclsPunctInText,
    /* APCO  79*/   wbkclsPunctInText,
    /* ASYW  80*/   wbkclsPunctInText,
    /* NHYP  81*/   wbkclsPunctSymb,
    /* NHYW  81*/   wbkclsPunctSymb,
    /* AHYW  82*/   wbkclsPunctSymb,
    /* NAPA  83*/   wbkclsPunctInText,
    /* NQMP  84*/   wbkclsPunctSymb,
    /* NSLS  85*/   wbkclsPostfix,
    /* NSF_  86*/   wbkclsTab,
    /* NSBS  86*/   wbkclsTab,
    /* NLA_  87*/   wbkclsAlpha,
    /* NLQ_  88*/   wbkclsAlpha,
    /* NLQN  88*/   wbkclsAlpha,
    /* ALQ_  89*/   wbkclsAlpha,
    /* NGR_  90*/   wbkclsAlpha,
    /* NGRN  90*/   wbkclsAlpha,
    /* NGQ_  91*/   wbkclsAlpha,
    /* NGQN  91*/   wbkclsAlpha,
    /* NCY_  92*/   wbkclsAlpha,
    /* NCYP  93*/   wbkclsAlpha,
    /* NCYC  93*/   wbkclsAlpha,
    /* NAR_  94*/   wbkclsAlpha,
    /* NAQN  95*/   wbkclsAlpha,
    /* NHB_  96*/   wbkclsAlpha,
    /* NHBC  96*/   wbkclsAlpha,
    /* NHBW  96*/   wbkclsAlpha,
    /* NHBR  96*/   wbkclsAlpha,
    /* NASR  97*/   wbkclsAlpha,
    /* NAAR  97*/   wbkclsAlpha,
    /* NAAC  97*/   wbkclsAlpha,
    /* NAAD  97*/   wbkclsAlpha,
    /* NAED  97*/   wbkclsAlpha,
    /* NANW  97*/   wbkclsAlpha,
    /* NAEW  97*/   wbkclsAlpha,
    /* NAAS  97*/   wbkclsAlpha,
    /* NHI_  98*/   wbkclsAlpha,
    /* NHIN  98*/   wbkclsAlpha,
    /* NHIC  98*/   wbkclsAlpha,
    /* NHID  98*/   wbkclsAlpha,
    /* NBE_  99*/   wbkclsAlpha,
    /* NBEC  99*/   wbkclsAlpha,
    /* NBED  99*/   wbkclsAlpha,
    /* NGM_ 100*/   wbkclsAlpha,
    /* NGMC 100*/   wbkclsAlpha,
    /* NGMD 100*/   wbkclsAlpha,
    /* NGJ_ 101*/   wbkclsAlpha,
    /* NGJC 101*/   wbkclsAlpha,
    /* NGJD 101*/   wbkclsAlpha,
    /* NOR_ 102*/   wbkclsAlpha,
    /* NORC 102*/   wbkclsAlpha,
    /* NORD 102*/   wbkclsAlpha,
    /* NTA_ 103*/   wbkclsAlpha,
    /* NTAC 103*/   wbkclsAlpha,
    /* NTAD 103*/   wbkclsAlpha,
    /* NTE_ 104*/   wbkclsAlpha,
    /* NTEC 104*/   wbkclsAlpha,
    /* NTED 104*/   wbkclsAlpha,
    /* NKD_ 105*/   wbkclsAlpha,
    /* NKDC 105*/   wbkclsAlpha,
    /* NKDD 105*/   wbkclsAlpha,
    /* NMA_ 106*/   wbkclsAlpha,
    /* NMAC 106*/   wbkclsAlpha,
    /* NMAD 106*/   wbkclsAlpha,
    /* NTH_ 107*/   wbkclsAlpha,
    /* NTHC 107*/   wbkclsAlpha,
    /* NTHD 107*/   wbkclsAlpha,
    /* NTHT 107*/   wbkclsAlpha,
    /* NLO_ 108*/   wbkclsAlpha,
    /* NLOC 108*/   wbkclsAlpha,
    /* NLOD 108*/   wbkclsAlpha,
    /* NTI_ 109*/   wbkclsAlpha,
    /* NTIC 109*/   wbkclsAlpha,
    /* NTID 109*/   wbkclsAlpha,
    /* NGE_ 110*/   wbkclsAlpha,
    /* NGEQ 111*/   wbkclsAlpha,
    /* NBO_ 112*/   wbkclsAlpha,
    /* NBSP 113*/   wbkclsSpaceA,
    /* NOF_ 114*/   wbkclsPunctSymb,
    /* NOBS 114*/   wbkclsPunctSymb,
    /* NOEA 114*/   wbkclsPunctSymb,
    /* NONA 114*/   wbkclsPunctSymb,
    /* NONP 114*/   wbkclsPunctSymb,
    /* NOEP 114*/   wbkclsPunctSymb,
    /* NONW 114*/   wbkclsPunctSymb,
    /* NOEW 114*/   wbkclsPunctSymb,
    /* NOLW 114*/   wbkclsPunctSymb,
    /* NOCO 114*/   wbkclsPunctSymb,
    /* NOSP 114*/   wbkclsPunctSymb,
    /* NOEN 114*/   wbkclsPunctSymb,
    /* NET_ 115*/   wbkclsAlpha,
    /* NCA_ 116*/   wbkclsAlpha,
    /* NCH_ 117*/   wbkclsAlpha,
    /* WYI_ 118*/   wbkclsIdeoW,
    /* NBR_ 119*/   wbkclsAlpha,
    /* NRU_ 120*/   wbkclsAlpha,
    /* NOG_ 121*/   wbkclsAlpha,
    /* NSI_ 122*/   wbkclsAlpha,
    /* NSIC 122*/   wbkclsAlpha,
    /* NTN_ 123*/   wbkclsAlpha,
    /* NTNC 123*/   wbkclsAlpha,
    /* NKH_ 124*/   wbkclsAlpha,
    /* NKHC 124*/   wbkclsAlpha,
    /* NKHD 124*/   wbkclsAlpha,
    /* NBU_ 125*/   wbkclsAlpha,
    /* NBUC 125*/   wbkclsAlpha,
    /* NBUD 125*/   wbkclsAlpha,
    /* NSY_ 126*/   wbkclsAlpha,
    /* NSYC 126*/   wbkclsAlpha,
    /* NSYW 126*/   wbkclsAlpha,
    /* NMO_ 127*/   wbkclsAlpha,
    /* NMOC 127*/   wbkclsAlpha,
    /* NMOD 127*/   wbkclsAlpha,
    /* NHS_ 128*/   wbkclsAlpha,
    /* WHT_ 129*/   wbkclsAlpha,
    /* LS__ 130*/   wbkclsAlpha,
    /* XNW_ 131*/   wbkclsPunctSymb,
};

#define PACKEDWORDBREAKLENGTH 3
enum
{
    WORDBREAK_DEFAULT   = 0x1,
    WORDBREAK_KOREAN    = 0x2,
    WORDBREAK_PROOF     = 0x4,
};

#ifndef _MAC
struct PACKEDWORDBREAKBITS
{
    LONGLONG i1 :3;
    LONGLONG i2 :3;
    LONGLONG i3 :3;
    LONGLONG i4 :3;
    LONGLONG i5 :3;
    LONGLONG i6 :3;
    LONGLONG i7 :3;
    LONGLONG i8 :3;
    LONGLONG i9 :3;
    LONGLONG i10:3;
    LONGLONG i11:3;
    LONGLONG i12:3;
    LONGLONG i13:3;
    LONGLONG i14:3;
    LONGLONG i15:3;
    LONGLONG i16:3;
    LONGLONG i17:3;
    LONGLONG i18:3;
};
#endif //_MAC
  
// Note the indices (enum wbkcls) really are 0-base, but the comments are
// written to reflect the spec, which is 1 based.
const BYTE aWordBreakBits[wbkclsLim][wbkclsLim] =
{
   // 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18
    { 0, 7, 7, 7, 3, 7, 7, 0, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7 }, // 1  PunctSymb 
    { 7, 0, 0, 7, 7, 0, 7, 0, 7, 7, 7, 7, 7, 0, 7, 0, 7, 7 }, // 2  KanaFollow
    { 7, 0, 0, 7, 7, 0, 7, 0, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7 }, // 3  KatakanaW 
    { 7, 0, 7, 0, 7, 0, 7, 0, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7 }, // 4  Hiragana  
    { 3, 7, 7, 7, 3, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7 }, // 5  Tab       
    { 7, 0, 0, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 0, 7, 0, 7, 7 }, // 6  KanaDelim 
    { 7, 7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 0, 7, 7, 7 }, // 7  Prefix    
    { 7, 7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 }, // 8  Postfix
    { 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7, 7 }, // 9  SpaceA    
    { 3, 7, 7, 7, 7, 7, 7, 3, 7, 0, 7, 0, 0, 0, 7, 7, 7, 7 }, // 10 Alpha     
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 7, 7, 0, 7, 7, 1, 7 }, // 11 IdeoW     
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7 }, // 12 SuperSub  
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 0, 7, 0, 0, 0, 7, 7, 7, 7 }, // 13 DigitsN   
    { 7, 7, 7, 7, 7, 7, 7, 0, 7, 0, 7, 0, 0, 0, 0, 7, 0, 0 }, // 14 PunctInText
    { 7, 7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 0, 7, 0, 0, 7, 0, 0 }, // 15 DigitsW   
    { 7, 0, 7, 7, 7, 0, 7, 0, 7, 7, 7, 7, 7, 0, 7, 0, 7, 7 }, // 16 KatakanaN 
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 1, 0, 7, 0, 0, 7, 0, 7 }, // 17 Hangul    
    { 7, 7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 0, 7, 0, 0, 7, 7, 0 }, // 18 LatinW    
};

#if 0
const BYTE aWordBreakBits[wbkclsLim][wbkclsLim] =
{
   //18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1    // After/Before
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 0, 7, 7, 3, 7, 7, 7, 0 }, // 1  PunctSymb
    { 7, 7, 0, 7, 0, 7, 7, 7, 7, 7, 0, 7, 0, 7, 7, 0, 0, 7 }, // 2  KanaFollow
    { 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 0, 7, 0, 7, 7, 0, 0, 7 }, // 3  KatakanaW
    { 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 0, 7, 0, 7, 0, 7, 0, 7 }, // 4  Hiragana
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 3, 7, 7, 7, 3 }, // 5  Tab
    { 7, 7, 0, 7, 0, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 0, 0, 7 }, // 6  KanaDelim
    { 7, 7, 7, 0, 7, 0, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7 }, // 7  Prefix
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 }, // 8  Postfix
    { 7, 7, 7, 7, 7, 7, 7, 7, 7, 3, 7, 7, 7, 7, 7, 7, 7, 7 }, // 9  SpaceA
    { 7, 7, 7, 7, 0, 0, 0, 7, 0, 7, 0, 7, 7, 7, 7, 7, 7, 3 }, // 10 Alpha
    { 7, 1, 7, 7, 0, 7, 7, 1, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 }, // 11 IdeoW
    { 7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7 }, // 12 SuperSub
    { 7, 7, 7, 7, 0, 0, 0, 7, 0, 7, 0, 7, 7, 7, 7, 7, 7, 7 }, // 13 DigitsN
    { 0, 0, 7, 0, 0, 0, 0, 7, 0, 7, 0, 7, 7, 7, 7, 7, 7, 7 }, // 14 PunctInText
    { 0, 0, 7, 0, 0, 7, 0, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7 }, // 15 DigitsW
    { 7, 7, 0, 7, 0, 7, 7, 7, 7, 7, 0, 7, 0, 7, 7, 7, 0, 7 }, // 16 KatakanaN
    { 7, 0, 7, 0, 0, 7, 0, 1, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7 }, // 17 Hangul
    { 0, 7, 7, 0, 0, 7, 0, 7, 7, 7, 0, 7, 7, 7, 7, 7, 7, 7 }, // 18 LatinW
};
#endif // 0
    
    
BOOL
IsWordBreakBoundaryDefault( WCHAR chBefore, WCHAR chAfter )
{
    const CHAR_CLASS ccBefore = CharClassFromCh(chBefore);
    const CHAR_CLASS ccAfter  = CharClassFromCh(chAfter);
    const WBKCLS wbkclsBefore = WordBreakClassFromCharClass(ccBefore);
    const WBKCLS wbkclsAfter  = WordBreakClassFromCharClass(ccAfter);

#if 0
    // Find appropriate row
    return ((*(LONGLONG *)(aWordBreakBits+wbkclsBefore)) >> 
                // Shift over to appropriate column
                ( wbkclsAfter * PACKEDWORDBREAKLENGTH ) & 
                    // Mask off the correct level
                    ( IsKoreanSelectionMode() ? WORDBREAK_KOREAN : WORDBREAK_DEFAULT ) );
#endif // 0
    return ( aWordBreakBits[wbkclsBefore][wbkclsAfter] & 
        ( IsKoreanSelectionMode() ? WORDBREAK_KOREAN : WORDBREAK_DEFAULT ) );
}

BOOL
IsProofWordBreakBoundary( WCHAR chBefore, WCHAR chAfter )
{
    const CHAR_CLASS ccBefore   = CharClassFromCh( chBefore );
    const CHAR_CLASS ccAfter    = CharClassFromCh( chAfter );
    const WBKCLS wbkclsBefore   = WordBreakClassFromCharClass( ccBefore );
    const WBKCLS wbkclsAfter    = WordBreakClassFromCharClass( ccAfter );

#if 0
    return ((*(DWORD *)( aWordBreakBits+wbkclsBefore )) >>
                ( wbkclsAfter * PACKEDWORDBREAKLENGTH ) & WORDBREAK_PROOF );
#endif 0
    return ( aWordBreakBits[wbkclsBefore][wbkclsAfter] & WORDBREAK_PROOF );    
}

//+----------------------------------------------------------------------------
//
//  Function:   WordBreakClassFromCharClass
//
//  Synopsis:   Given a character class, this function returns the proper
//              word breaking class.
//
//-----------------------------------------------------------------------------

WBKCLS
WordBreakClassFromCharClass(CHAR_CLASS cc)
{
    Assert(cc >=0 && cc < CHAR_CLASS_MAX);

    return awbkclsWordBreakClassFromCharClass[cc];
}
