/*
 *  @doc    INTERNAL
 *
 *  @module LSBREAK.CXX -- line services break callbacks
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      12/22/97     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X_LINESRV_HXX_
#define X_LINESRV_HXX_
#include "linesrv.hxx"
#endif

#ifndef X_TXTDEFS_H
#define X_TXTDEFS_H
#include "txtdefs.h"
#endif

#ifndef X_LSSETDOC_H_
#define X_LSSETDOC_H_
#include <lssetdoc.h>
#endif

#ifndef X_BRKCLS_H_
#define X_BRKCLS_H_
#include <brkcls.h>
#endif

#ifndef X__UNIPART_H
#define X__UNIPART_H
#include "unipart.h"
#endif

ExternTag(tagLSCallBack);

// The following tables depend on the fact that there is no more
// than a single alternate for each MWCLS and that at most one
// condition needs to be taken into account in resolving each MWCLS

// NB (cthrash) This is a packed table.  The first three elements (brkcls,
// brkclsAlt and brkopt) are indexed by CHAR_CLASS.  The fourth column we
// access (brkclsLow) we access by char value.  The fourth column is for a
// speed optimization.

#if defined(UNIX) || (defined(_MSC_VER) && (_MSC_VER >= 1200))
// Unix and Newer MS compilers can't use DWORD to initialize enum type BRKCLS.
#define BRKINFO(a,b,c,d) { CLineServices::a, CLineServices::b, CLineServices::c, CLineServices::d }
#else
#define BRKINFO(a,b,c,d) { DWORD(CLineServices::a), DWORD(CLineServices::b), DWORD(CLineServices::c), DWORD(CLineServices::d) }
#endif

const CLineServices::PACKEDBRKINFO CLineServices::s_rgBrkInfo[CHAR_CLASS_MAX] =
{
    //       brkcls             brkclsAlt         brkopt     brkclsLow                 CC     (QPID)
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), //   0 WOB_(1)  
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), //   1 NOPP(2)  
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), //   2 NOPA(2)  
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), //   3 NOPW(2)  
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), //   4 HOP_(3)  
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), //   5 WOP_(4)  
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsAlpha        ), //   6 WOP5(5)  
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //   7 NOQW(6)  
    BRKINFO( brkclsQuote,       brkclsOpen,       fCscWide,  brkclsAlpha        ), //   8 AOQW(7)  
    BRKINFO( brkclsOpen,        brkclsNil,        fBrkNone,  brkclsSpaceN       ), //   9 WOQ_(8)  
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsSpaceN       ), //  10 WCB_(9)  
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  11 NCPP(10) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsSpaceN       ), //  12 NCPA(10) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsSpaceN       ), //  13 NCPW(10) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  14 HCP_(11) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  15 WCP_(12) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  16 WCP5(13) 
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  17 NCQW(14) 
    BRKINFO( brkclsQuote,       brkclsClose,      fCscWide,  brkclsAlpha        ), //  18 ACQW(15) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  19 WCQ_(16) 
    BRKINFO( brkclsQuote,       brkclsClose,      fCscWide,  brkclsAlpha        ), //  20 ARQW(17) 
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsAlpha        ), //  21 NCSA(18) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  22 HCO_(19) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  23 WC__(20) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  24 WCS_(20)
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  25 WC5_(21) 
    BRKINFO( brkclsClose,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  26 WC5S(21)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  27 NKS_(22) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  28 WKSM(23) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  29 WIM_(24) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  30 NSSW(25) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  31 WSS_(26) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAsciiSpace   ), //  32 WHIM(27) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsExclaInterr  ), //  33 WKIM(28) 
    BRKINFO( brkclsIdeographic, brkclsNoStartIdeo,fBrkStrict,brkclsQuote        ), //  34 NKSL(29) 
    BRKINFO( brkclsIdeographic, brkclsNoStartIdeo,fBrkStrict,brkclsAlpha        ), //  35 WKS_(30) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsPrefix       ), //  36 WKSC(30) 
    BRKINFO( brkclsIdeographic, brkclsNoStartIdeo,fBrkStrict,brkclsPostfix      ), //  37 WHS_(31) 
    BRKINFO( brkclsExclaInterr, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  38 NQFP(32) 
    BRKINFO( brkclsExclaInterr, brkclsNil,        fBrkNone,  brkclsQuote        ), //  39 NQFA(32) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsOpen         ), //  40 WQE_(33) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsClose        ), //  41 WQE5(34) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  42 NKCC(35) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  43 WKC_(36) 
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumSeparator ), //  44 NOCP(37) 
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsSpaceN       ), //  45 NOCA(37) 
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumSeparator ), //  46 NOCW(37) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsSlash        ), //  47 WOC_(38) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  48 WOCS(38)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  49 WOC5(39) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  50 WOC6(39)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  51 AHPW(40) 
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumeral      ), //  52 NPEP(41) 
    BRKINFO( brkclsNumSeparator,brkclsNil,        fBrkNone,  brkclsNumeral      ), //  53 NPAR(41) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  54 HPE_(42) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  55 WPE_(43) 
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  56 WPES(43)
    BRKINFO( brkclsNoStartIdeo, brkclsNil,        fBrkNone,  brkclsNumeral      ), //  57 WPE5(44) 
    BRKINFO( brkclsInseparable, brkclsNil,        fBrkNone,  brkclsNumSeparator ), //  58 NISW(45) 
    BRKINFO( brkclsInseparable, brkclsNil,        fBrkNone,  brkclsNumSeparator ), //  59 AISW(46) 
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  60 NQCS(47) 
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  61 NQCW(47) 
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsAlpha        ), //  62 NQCC(47) 
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsExclaInterr  ), //  63 NPTA(48) 
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), //  64 NPNA(48) 
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), //  65 NPEW(48) 
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), //  66 NPEH(48) 
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), //  67 APNW(49) 
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), //  68 HPEW(50) 
    BRKINFO( brkclsPrefix,      brkclsNil,        fBrkNone,  brkclsAlpha        ), //  69 WPR_(51) 
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), //  70 NQEP(52) 
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), //  71 NQEW(52) 
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), //  72 NQNW(52) 
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), //  73 AQEW(53) 
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), //  74 AQNW(53) 
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), //  75 AQLW(53)
    BRKINFO( brkclsPostfix,     brkclsNil,        fBrkNone,  brkclsAlpha        ), //  76 WQO_(54) 
    BRKINFO( brkclsAsciiSpace,  brkclsNil,        fBrkNone,  brkclsAlpha        ), //  77 NSBL(55) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  78 WSP_(56) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  79 WHI_(57) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  80 NKA_(58) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  81 WKA_(59) 
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), //  82 ASNW(60) 
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), //  83 ASEW(60) 
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), //  84 ASRN(60) 
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), //  85 ASEN(60)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), //  86 ALA_(61) 
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), //  87 AGR_(62) 
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), //  88 ACY_(63) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  89 WID_(64) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  90 WPUA(65) 
    BRKINFO( brkclsHangul,      brkclsNil,        fBrkNone,  brkclsOpen         ), //  91 NHG_(66) 
    BRKINFO( brkclsHangul,      brkclsNil,        fBrkNone,  brkclsPrefix       ), //  92 WHG_(67) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsClose        ), //  93 WCI_(68) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  94 NOI_(69) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), //  95 WOI_(70) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), //  96 WOIC(70) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), //  97 WOIL(70)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), //  98 WOIS(70)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), //  99 WOIT(70)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 100 NSEN(71) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 101 NSET(71) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 102 NSNW(71) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 103 ASAN(72) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 104 ASAE(72) 
    BRKINFO( brkclsNumeral,     brkclsNil,        fBrkNone,  brkclsAlpha        ), // 105 NDEA(73) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // 106 WD__(74) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 107 NLLA(75) 
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsAlpha        ), // 108 WLA_(76) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // 109 NWBL(77) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // 110 NWZW(77) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 111 NPLW(78) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 112 NPZW(78) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 113 NPF_(78) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 114 NPFL(78)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 115 NPNW(78) 
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // 116 APLW(79) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // 117 APCO(79) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // 118 ASYW(80) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // 119 NHYP(81) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // 120 NHYW(81) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // 121 AHYW(82) 
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 122 NAPA(83) 
    BRKINFO( brkclsQuote,       brkclsNil,        fBrkNone,  brkclsOpen         ), // 123 NQMP(84) 
    BRKINFO( brkclsSlash,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 124 NSLS(85) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsClose        ), // 125 NSF_(86) 
    BRKINFO( brkclsSpaceN,      brkclsNil,        fBrkNone,  brkclsAlpha        ), // 126 NSBS(86) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 127 NLA_(87) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 128 NLQ_(88) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 129 NLQN(88)
    BRKINFO( brkclsAlpha,       brkclsIdeographic,fCscWide,  brkclsAlpha        ), // 130 ALQ_(89) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 131 NGR_(90) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 132 NGRN(90)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 133 NGQ_(91) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 134 NGQN(91)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 135 NCY_(92) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 136 NCYP(93) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // 137 NCYC(93) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 138 NAR_(94) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 139 NAQN(95) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 140 NHB_(96) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // 141 NHBC(96) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 142 NHBW(96) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 143 NHBR(96) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 144 NASR(97) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 145 NAAR(97) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // 146 NAAC(97) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 147 NAAD(97) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 148 NAED(97) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 149 NANW(97) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 150 NAEW(97) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 151 NAAS(97) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 152 NHI_(98) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 153 NHIN(98) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // 154 NHIC(98) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 155 NHID(98) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 156 NBE_(99) 
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsAlpha        ), // 157 NBEC(99) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 158 NBED(99) 
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsAlpha        ), // 159 NGM_(100)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsGlueA        ), // 160 NGMC(100)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 161 NGMD(100)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 162 NGJ_(101)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 163 NGJC(101)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 164 NGJD(101)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 165 NOR_(102)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 166 NORC(102)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 167 NORD(102)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 168 NTA_(103)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 169 NTAC(103)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 170 NTAD(103)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 171 NTE_(104)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 172 NTEC(104)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 173 NTED(104)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 174 NKD_(105)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 175 NKDC(105)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 176 NKDD(105)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 177 NMA_(106)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 178 NMAC(106)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 179 NMAD(106)
    BRKINFO( brkclsThaiFirst,   brkclsNil,        fBrkNone,  brkclsNil          ), // 180 NTH_(107)
    BRKINFO( brkclsThaiFirst,   brkclsNil,        fBrkNone,  brkclsNil          ), // 181 NTHC(107)
    BRKINFO( brkclsThaiFirst,   brkclsNil,        fBrkNone,  brkclsNil          ), // 182 NTHD(107)
    BRKINFO( brkclsThaiFirst,   brkclsNil,        fBrkNone,  brkclsNil          ), // 183 NTHT(107)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 184 NLO_(108)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 185 NLOC(108)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 186 NLOD(108)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 187 NTI_(109)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 188 NTIC(109)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 189 NTID(109)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 190 NGE_(110)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 191 NGEQ(111)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 192 NBO_(112)
    BRKINFO( brkclsGlueA,       brkclsNil,        fBrkNone,  brkclsNil          ), // 193 NBSP(113)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 194 NOF_(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 195 NOBS(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 196 NOEA(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 197 NONA(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 198 NONP(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 199 NOEP(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 200 NONW(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 201 NOEW(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 202 NOLW(114)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 203 NOCO(114)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 204 NOSP(114)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 205 NOEN(114)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 206 NET_(115)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 207 NCA_(116)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 208 NCH_(117)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsNil          ), // 209 WYI_(118)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 210 NBR_(119)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 211 NRU_(120)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 212 NOG_(121)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 213 NSI_(122)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 214 NSIC(122)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 215 NTN_(123)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 216 NTNC(123)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 217 NKH_(124)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 218 NKHC(124)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 219 NKHD(124)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 220 NBU_(125)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 221 NBUC(125)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 222 NBUD(125)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 223 NSY_(126)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 224 NSYC(126)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 225 NSYW(126)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 226 NMO_(127)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 227 NMOC(127)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 228 NMOD(127)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          ), // 229 NHS_(128)
    BRKINFO( brkclsIdeographic, brkclsNil,        fBrkNone,  brkclsNil          ), // 230 WHT_(129)
    BRKINFO( brkclsCombining,   brkclsNil,        fBrkNone,  brkclsNil          ), // 231 LS__(130)
    BRKINFO( brkclsAlpha,       brkclsNil,        fBrkNone,  brkclsNil          )  // 232 XNW_(131)
};

// Break pair information for normal or strict Kinsoku
const BYTE s_rgbrkpairsKinsoku[CLineServices::brkclsLim][CLineServices::brkclsLim] =
{
//1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21   = brclsAfter
                                                                                     //  brkclsBefore:
  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  2,  1, //   1 brkclsOpen    
  0,  1,  1,  1,  0,  0,  2,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //   2 brkclsClose   
  0,  1,  2,  1,  2,  0,  2,  4,  0,  0,  4,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1, //   3 brkclsNoStartIdeo
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //   4 brkclsExclamInt
  0,  1,  2,  1,  2,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //   5 brkclsInseparable
  2,  1,  2,  1,  0,  2,  0,  2,  2,  0,  3,  2,  1,  2,  1,  2,  2,  0,  0,  2,  1, //   6 brkclsPrefix  
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //   7 brkclsPostfix 
  0,  1,  2,  1,  2,  0,  2,  4,  0,  0,  4,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1, //   8 brkclsIdeoW   
  0,  1,  2,  1,  2,  0,  2,  0,  3,  0,  3,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //   9 brkclsNumeral 
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //  10 brkclsSpaceN  
  0,  1,  2,  1,  2,  2,  2,  4,  3,  0,  3,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1, //  11 brkclsAlpha   
  2,  1,  2,  1,  2,  2,  2,  2,  2,  2,  2,  2,  1,  2,  1,  2,  2,  2,  2,  2,  1, //  12 brkclsGlueA   
  0,  1,  2,  1,  2,  0,  2,  0,  2,  0,  3,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //  13 brkclsSlash   
  1,  1,  2,  1,  2,  2,  3,  2,  2,  2,  2,  2,  1,  2,  1,  2,  2,  2,  2,  2,  1, //  14 brkclsQuotation
  0,  1,  2,  1,  0,  2,  2,  0,  2,  0,  3,  2,  1,  2,  2,  0,  0,  0,  0,  2,  1, //  15 brkclsNumSeparator
  0,  1,  2,  1,  2,  0,  2,  4,  0,  0,  4,  2,  1,  2,  1,  4,  0,  0,  0,  2,  1, //  16 brkclsHangul  
  0,  1,  2,  1,  2,  0,  2,  0,  0,  0,  0,  2,  1,  2,  1,  4,  2,  2,  2,  2,  1, //  17 brkclsThaiFirst
  0,  1,  2,  1,  2,  0,  2,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  2,  2,  2,  1, //  18 brkclsThaiLast
  0,  1,  2,  1,  2,  0,  0,  0,  0,  0,  0,  2,  2,  2,  1,  0,  2,  2,  2,  2,  1, //  19 brkclsThaiAlpha
  0,  1,  2,  1,  2,  2,  2,  4,  3,  0,  3,  2,  1,  2,  1,  4,  0,  0,  0,  1,  1, //  20 brkclsCombining
  0,  1,  2,  1,  0,  0,  0,  0,  0,  0,  0,  2,  1,  2,  1,  0,  0,  0,  0,  2,  1, //  21 brkclsAsiiSpace
};

inline BOOL
CanQuickBrkclsLookup(WCHAR ch)
{
    return ( ch < BRKCLS_QUICKLOOKUPCUTOFF );
}

inline CLineServices::BRKCLS
QuickBrkclsFromCh(WCHAR ch)
{
    Assert(ch);  // This won't work for ch==0.
    Assert( CanQuickBrkclsLookup(ch) );
    return (CLineServices::BRKCLS)CLineServices::s_rgBrkInfo[ch].brkclsLow;
}

inline
CLineServices::BRKCLS
BrkclsFromCh(WCHAR ch, DWORD brkopt)
{
    Assert( !CanQuickBrkclsLookup(ch) ); // Should take another code path.

    CHAR_CLASS cc = CharClassFromCh(ch);
    Assert(cc < CHAR_CLASS_MAX);

    const CLineServices::PACKEDBRKINFO * p = CLineServices::s_rgBrkInfo + cc;

    return CLineServices::BRKCLS((p->brkopt & brkopt) ? p->brkclsAlt : p->brkcls);
}

// Line breaking behaviors

// Standard Breaking Behaviors retaining normal line break for non-FE text
static const LSBRK s_rglsbrkNormal[] = 
{
    /* 0*/ 1,1,  // always allowed
    /* 1*/ 0,0,  // always prohibited
    /* 2*/ 0,1,  // only allowed across space
    /* 3*/ 0,1,  // only allowed across space (word wrap case)
    /* 4*/ 1,1,  // always allowed (no CJK/Hangul word wrap case)
};

// Breaking Behaviors allowing FE style breaking in the middle of words (any language)
static const LSBRK s_rglsbrkBreakAll[] = 
{
    /* 0*/ 1,1,  // always allowed
    /* 1*/ 0,0,  // always prohibited
    /* 2*/ 0,1,  // only allowed across space
    /* 3*/ 1,1,  // always allowed (no word wrap case)
    /* 4*/ 1,1,  // always allowed (no CJK/Hangul word wrap case)
};

// Breaking Behaviors allowing Hangul style breaking 
static const LSBRK s_rglsbrkKeepAll[] = 
{
    /* 0*/ 1,1,  // always allowed
    /* 1*/ 0,0,  // always prohibited
    /* 2*/ 0,1,  // only allowed across space
    /* 3*/ 0,1,  // only allowed across space (word wrap case)
    /* 4*/ 0,1,  // only allowed across space (CJK/Hangul word wrap case)
};

const struct lsbrk * alsbrkTables[4] =
{                                                           
    s_rglsbrkNormal,
    s_rglsbrkNormal,
    s_rglsbrkBreakAll,
    s_rglsbrkKeepAll
};

LSERR
CLineServices::CheckSetBreaking()
{
    const struct lsbrk * lsbrkCurr = alsbrkTables[_pPFFirst->_fWordBreak];

    HRESULT hr;

    //
    // Are we in need of calling LsSetBreaking?
    //

    if (lsbrkCurr == _lsbrkCurr)
    {
        hr = S_OK;
    }
    else
    {
        hr = HRFromLSERR( LsSetBreaking( _plsc,
                                         sizeof( s_rglsbrkNormal ) / sizeof( LSBRK ),
                                         lsbrkCurr,
                                         brkclsLim,
                                         (const BYTE *)s_rgbrkpairsKinsoku ) );

        _lsbrkCurr = (struct lsbrk *)lsbrkCurr;
    }

    RRETURN(hr);
}


#ifdef _MAC
void ReverseByteSex(DWORD* pdw);
void ReverseByteSex(DWORD* pdw)
{
    DWORD In = (*pdw);
    BYTE* pIn = (BYTE*) &In;
    BYTE* pOut = ((BYTE*) pdw) + 3;
    
    for ( short i = 0; i < 4; i++, pIn++, pOut-- )
        (*pOut) = (*pIn);
}

void ClearDWORD(DWORD* pdw);
void ClearDWORD(DWORD* pdw)
{
    (*pdw) = 0;
}

#endif


LSERR WINAPI
CLineServices::GetBreakingClasses(
    PLSRUN plsrun,          // IN
    LSCP lscp,              // IN
    WCHAR wch,              // IN
    BRKCLS* pbrkclsFirst,    // OUT
    BRKCLS* pbrkclsSecond)  // OUT
{
    LSTRACE(GetBreakingClasses);
    LSERR lserr = lserrNone;

#ifdef _MAC
    ClearDWORD((DWORD*) pbrkclsFirst);
    ClearDWORD((DWORD*) pbrkclsSecond);
#endif

#if 0
    if (plsrun->GetCF()->_bCharSet == SYMBOL_CHARSET)
    {
        *pbrkclsFirst = *pbrkclsSecond = brkclsAlpha;
    }
    else
#endif
    if (CanQuickBrkclsLookup(wch))
    {
        // prefer ASCII (true block) for performance
        Assert( IsNotThaiTypeChar(wch) );
        *pbrkclsFirst = *pbrkclsSecond = QuickBrkclsFromCh(wch);
    }
    else if(IsNotThaiTypeChar(wch))  
    {
        *pbrkclsFirst = *pbrkclsSecond = BrkclsFromCh(wch, plsrun->_brkopt);
    }
    else
    {    
        CComplexRun * pcr = plsrun->GetComplexRun();

        if (pcr != NULL)
        {
            LONG cp = CPFromLSCP(lscp);

#if DBG==1
            Assert(LSCPFromCP(cp) == lscp);
#endif // DBG

            pcr->ThaiTypeBrkcls(_pMarkup, cp, (::BRKCLS*)pbrkclsFirst, (::BRKCLS*)pbrkclsSecond);
        }
        else
        {
            // BUGFIX 14717 (a-pauln)
            // A complex run has not been created so pass this through the normal
            // Kinsoku classes for clean failure.
            *pbrkclsFirst = *pbrkclsSecond = BrkclsFromCh(wch, plsrun->_brkopt);
        }
    }

#ifdef _MAC
    ReverseByteSex((DWORD*) pbrkclsFirst);
    ReverseByteSex((DWORD*) pbrkclsSecond);
#endif

    return lserr;
}

// NB (cthrash) This table came from Quill.

const BRKCOND CLineServices::s_rgbrkcondBeforeChar[brkclsLim] =
{
    brkcondPlease,  // brkclsOpen
    brkcondNever,   // brkclsClose
    brkcondNever,   // brkclsNoStartIdeo
    brkcondNever,   // brkclsExclaInterr
    brkcondCan,     // brkclsInseparable
    brkcondCan,     // brkclsPrefix
    brkcondCan,     // brkclsPostfix
    brkcondPlease,  // brkclsIdeographic
    brkcondCan,     // brkclsNumeral
    brkcondCan,     // brkclsSpaceN
    brkcondCan,     // brkclsAlpha
    brkcondCan,     // brkclsGlueA
    brkcondPlease,  // brkclsSlash
    brkcondCan,     // brkclsQuote
    brkcondCan,     // brkclsNumSeparator
    brkcondCan,     // brkclsHangul
    brkcondCan,     // brkclsThaiFirst
    brkcondNever,   // brkclsThaiLast
    brkcondNever,   // brkclsThaiMiddle
    brkcondCan,     // brkclsCombining
    brkcondCan,     // brkclsAsciiSpace
};

LSERR WINAPI
CLineServices::CanBreakBeforeChar(
    BRKCLS brkcls,          // IN
    BRKCOND* pbrktxtBefore) // OUT
{
    LSTRACE(CanBreakBeforeChar);

    Assert( brkcls >= 0 && brkcls < brkclsLim );

    *pbrktxtBefore = s_rgbrkcondBeforeChar[ brkcls ];

    return lserrNone;
}

const BRKCOND CLineServices::s_rgbrkcondAfterChar[brkclsLim] =
{
    brkcondPlease,  // brkclsOpen
    brkcondCan,     // brkclsClose
    brkcondCan,     // brkclsNoStartIdeo
    brkcondCan,     // brkclsExclaInterr
    brkcondCan,     // brkclsInseparable
    brkcondCan,     // brkclsPrefix
    brkcondCan,     // brkclsPostfix
    brkcondPlease,  // brkclsIdeographic
    brkcondCan,     // brkclsNumeral
    brkcondCan,     // brkclsSpaceN
    brkcondCan,     // brkclsAlpha
    brkcondNever,   // brkclsGlueA
    brkcondPlease,  // brkclsSlash
    brkcondCan,     // brkclsQuote
    brkcondCan,     // brkclsNumSeparator
    brkcondCan,     // brkclsHangul
    brkcondNever,   // brkclsThaiFirst
    brkcondCan,     // brkclsThaiLast
    brkcondNever,   // brkclsThaiAlpha
    brkcondCan,     // brkclsCombining
    brkcondCan,     // brkclsAsciiSpace
};

LSERR WINAPI
CLineServices::CanBreakAfterChar(
    BRKCLS brkcls,          // IN
    BRKCOND* pbrktxtAfter)  // OUT
{
    LSTRACE(CanBreakAfterChar);

    Assert( brkcls >= 0 && brkcls < brkclsLim );

    *pbrktxtAfter = s_rgbrkcondAfterChar[ brkcls ];

    return lserrNone;
}

#if DBG==1
const char * s_achBrkCls[] =  // Note brkclsNil is -1
{
    "brkclsNil",
    "brkclsOpen",
    "brkclsClose",
    "brkclsNoStartIdeo",
    "brkclsExclaInterr",
    "brkclsInseparable",
    "brkclsPrefix",
    "brkclsPostfix",
    "brkclsIdeographic",
    "brkclsNumeral",
    "brkclsSpaceN",
    "brkclsAlpha",
    "brkclsGlueA",
    "brkclsSlash",
    "brkclsQuote",
    "brkclsNumSeparator",
    "brkclsHangul",
    "brkclsThaiFirst",
    "brkclsThaiLast",
    "brkclsThaiMiddle",
    "brkclsCombining",
    "brkclsAsciiSpace"
};

const char *
CLineServices::BrkclsNameFromCh(TCHAR ch, BOOL fStrict)
{
    BRKCLS brkcls = CanQuickBrkclsLookup(ch) ? QuickBrkclsFromCh(ch) : BrkclsFromCh(ch, fStrict);

    return s_achBrkCls[int(brkcls)+1];    
}

#endif
