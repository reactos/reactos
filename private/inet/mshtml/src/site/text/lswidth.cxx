/*
 *  @doc    INTERNAL
 *
 *  @module LSWIDTH.CXX -- line services width callbacks
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *      Sujal Parikh <nl>
 *
 *  History: <nl>
 *      08/11/98     cthrash created
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

#ifndef X_LSEMS_H_
#define X_LSEMS_H_
#include <lsems.h>
#endif

#ifndef X_LSCBK_H_
#define X_LSCBK_H_
#include <lscbk.h>
#endif

#ifndef X_BRKCLS_H_
#define X_BRKCLS_H_
#include <brkcls.h>
#endif

#ifndef X__UNIPART_H
#define X__UNIPART_H
#include "unipart.h"
#endif

//
// Documentation for CSS text-justify can be found on http://ie/specs/secure/trident/text/Line_just.htm
// Documentation for CSS text-autospace can be found on http://ie/specs/secure/trident/text/csc.htm
//

ExternTag(tagLSCallBack);

#define prior5 5        // REVIEW elik (asmusf)
#define priorNum 5

enum CSC
{
    cscUndefined,                       // 0
    cscOpenParenthesisNH,               // 1
    cscOpenParenthesisW,                // 2
    cscCloseParenthesisNH,              // 3
    cscCloseParenthesisW,               // 4
    cscNeutralCharactersNH,             // 5
    cscCommaW,                          // 6
    cscHiraganaOrKatakanaW,             // 7
    cscQuestionOrExclamationW,          // 8
    cscPunctuationN,                    // 9
    cscCenteredCharactersW,             // 10
    cscHiraganaOrKatakanaN,             // 11
    cscPeriodW,                         // 12
    cscSpaceN,                          // 13
    cscSpaceW,                          // 14
    cscIdeographicW,                    // 15
    cscAlphabetsOptionsN,               // 16
    cscDigitsWOrAutospacingOnN,         // 17
    cscInseparableCharactersA,          // 18
    cscGlueCharactersN,                 // 19
    cscPrefixCurrenciesAndSymbol,       // 20
    cscPostfixCurrenciesAndSymbols,     // 21
    cscDigitsWOrAutospacingOffN,        // 22
    cscOpenQuotesA,                     // 23
    cscLim
};

typedef struct
{
    unsigned short csc:5;       // CSC
    unsigned short cscAlt:5;    // Alternate CSC
    unsigned short csco:6;      // CSCOPTION
} PACKEDCSC;

const PACKEDCSC s_aCscMap[] =
{
    { cscOpenParenthesisW,            cscUndefined,               cscoNone             }, // WOB_(0)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // NOPP(1)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // NOPA(2)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // NOPW(3)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // HOP_(4)
    { cscOpenParenthesisW,            cscUndefined,               cscoNone             }, // WOP_(5)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // WOP5(6)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // NOQW(7)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // AOQW(8)
    { cscNeutralCharactersNH,         cscOpenParenthesisNH,       cscoAutospacingParen }, // WOQ_(9)
    { cscCloseParenthesisW,           cscUndefined,               cscoNone             }, // WCB_(10)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // NCPP(11)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // NCPA(12)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // NCPW(13)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // HCP_(14)
    { cscCloseParenthesisW,           cscUndefined,               cscoNone             }, // WCP_(15)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // WCP5(16)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // NCQW(17)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // ACQW(18)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // WCQ_(19)
    { cscNeutralCharactersNH,         cscCloseParenthesisNH,      cscoAutospacingParen }, // ARQW(20)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NCSA(21)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // HCO_(22)
    { cscCommaW,                      cscUndefined,               cscoNone             }, // WC__(23)
    { cscCommaW,                      cscUndefined,               cscoNone             }, // WCS_(24)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // WC5_(25)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // WC5S(26)
    { cscHiraganaOrKatakanaN,         cscUndefined,               cscoNone             }, // NKS_(27)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WKSM(28)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WIM_(29)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NSSW(30)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WSS_(31)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WHIM(32)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WKIM(33)
    { cscHiraganaOrKatakanaN,         cscUndefined,               cscoNone             }, // NKSL(34)
    { cscIdeographicW,                cscHiraganaOrKatakanaW,     cscoCompressKana     }, // WKS_(35)
    { cscIdeographicW,                cscHiraganaOrKatakanaW,     cscoCompressKana     }, // WKSC(36)
    { cscIdeographicW,                cscHiraganaOrKatakanaW,     cscoCompressKana     }, // WHS_(37)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NQFP(38)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NQFA(39)
    { cscQuestionOrExclamationW,      cscUndefined,               cscoNone             }, // WQE_(40)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // WQE5(41)
    { cscHiraganaOrKatakanaN,         cscUndefined,               cscoNone             }, // NKCC(42)
    { cscCenteredCharactersW,         cscUndefined,               cscoNone             }, // WKC_(43)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // NOCP(44)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // NOCA(45)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // NOCW(46)
    { cscIdeographicW,                cscUndefined,               cscoVerticalFont     }, // WOC_(47)
    { cscIdeographicW,                cscUndefined,               cscoVerticalFont     }, // WOCS(48)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // WOC5(49)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // WOC6(50)
    { cscPunctuationN,                cscCenteredCharactersW,     cscoWide             }, // AHPW(51)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // NPEP(52)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // NPAR(53)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // HPE_(54)
    { cscPeriodW,                     cscUndefined,               cscoNone             }, // WPE_(55)
    { cscPeriodW,                     cscUndefined,               cscoNone             }, // WPES(56)
    { cscPunctuationN,                cscUndefined,               cscoNone             }, // WPE5(57)
    { cscInseparableCharactersA,      cscUndefined,               cscoNone             }, // NISW(58)
    { cscInseparableCharactersA,      cscUndefined,               cscoNone             }, // AISW(59)
    { cscGlueCharactersN,             cscUndefined,               cscoNone             }, // NQCS(60)
    { cscGlueCharactersN,             cscUndefined,               cscoNone             }, // NQCW(61)
    { cscGlueCharactersN,             cscUndefined,               cscoNone             }, // NQCC(62)
    { cscPrefixCurrenciesAndSymbol,   cscUndefined,               cscoNone             }, // NPTA(63)
    { cscPrefixCurrenciesAndSymbol,   cscUndefined,               cscoNone             }, // NPNA(64)
    { cscPrefixCurrenciesAndSymbol,   cscUndefined,               cscoNone             }, // NPEW(65)
    { cscPrefixCurrenciesAndSymbol,   cscUndefined,               cscoNone             }, // NPEH(66)
    { cscPrefixCurrenciesAndSymbol,   cscUndefined,               cscoNone             }, // APNW(67)
    { cscPrefixCurrenciesAndSymbol,   cscUndefined,               cscoNone             }, // NPEW(68)
    { cscPrefixCurrenciesAndSymbol,   cscUndefined,               cscoNone             }, // WPR_(69)
    { cscPostfixCurrenciesAndSymbols, cscUndefined,               cscoNone             }, // NQEP(70)
    { cscPostfixCurrenciesAndSymbols, cscUndefined,               cscoNone             }, // NQEW(71)
    { cscPostfixCurrenciesAndSymbols, cscUndefined,               cscoNone             }, // NQNW(72)
    { cscPostfixCurrenciesAndSymbols, cscUndefined,               cscoNone             }, // AQEW(73)
    { cscPostfixCurrenciesAndSymbols, cscUndefined,               cscoNone             }, // AQNW(74)
    { cscPostfixCurrenciesAndSymbols, cscUndefined,               cscoNone             }, // AQLW(75)
    { cscPostfixCurrenciesAndSymbols, cscUndefined,               cscoNone             }, // WQO_(76)
    { cscSpaceN,                      cscUndefined,               cscoNone             }, // NSBL(77)
    { cscSpaceW,                      cscUndefined,               cscoNone             }, // WSP_(78)
    { cscIdeographicW,                cscHiraganaOrKatakanaW,     cscoCompressKana     }, // WHI_(79)
    { cscHiraganaOrKatakanaN,         cscUndefined,               cscoNone             }, // NKA_(80)
    { cscIdeographicW,                cscHiraganaOrKatakanaW,     cscoCompressKana     }, // WKA_(81)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ASNW(82)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ASEW(83)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ASRN(84)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ASEN(85)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ALA_(86)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // AGR_(87)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ACY_(88)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WID_(89)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WPUA(90)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NHG_(91)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WHG_(92)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WCI_(93)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOI_(94)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WOI_(95)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WOIC(96)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WOIL(97)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WOIS(98)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WOIT(99)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NSEN(100)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NSET(101)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NSNW(102)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ASAN(103)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ASAE(104)
    { cscDigitsWOrAutospacingOffN,    cscDigitsWOrAutospacingOnN, cscoAutospacingDigit }, // NDEA(105)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WD__(106)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NLLA(107)
    { cscIdeographicW,                cscUndefined,               cscoNone             }, // WLA_(108)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NWBL(109)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NWZW(110)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NPLW(111)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NPZW(112)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NPF_(113)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NPFL(114)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NPNW(115)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // APLW(116)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // APCO(117)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ASYW(118)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NHYP(119)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NHYW(120)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // AHYW(121)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAPA(122)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NQMP(123)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NSLS(124)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NSF_(125)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NSBS(126)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NLA_(127)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NLQ_(128)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NLQN(129)
    { cscNeutralCharactersNH,         cscIdeographicW,            cscoWide             }, // ALQ_(130)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGR_(131)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGRN(132)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGQN(133)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGQ_(134)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NCY_(135)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NCYP(136)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NCYC(137)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAR_(138)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAQN(139)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHB_(140)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHBC(141)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHBW(142)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHBR(143)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NASR(144)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAAR(145)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAAC(146)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAAD(147)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAED(148)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NANW(149)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAEW(150)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NAAS(151)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHI_(152)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHIN(153)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHIC(154)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHID(155)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBE_(156)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBEC(157)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBED(158)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGM_(159)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGMC(160)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGMD(161)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGJ_(162)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGJC(163)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGJD(164)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NOR_(165)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NORC(166)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NORD(167)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTA_(168)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTAC(169)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTAD(170)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTE_(171)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTEC(172)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTED(173)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NKD_(174)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NKDC(175)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NKDD(176)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NMA_(177)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NMAC(178)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NMAD(179)
    { cscUndefined,                   cscUndefined,               cscoNone             }, // NTH_(180)
    { cscUndefined,                   cscUndefined,               cscoNone             }, // NTHC(181)
    { cscUndefined,                   cscUndefined,               cscoNone             }, // NTHD(182)
    { cscUndefined,                   cscUndefined,               cscoNone             }, // NTHT(183)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NLO_(184)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NLOC(185)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NLOD(186)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTI_(187)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTIC(188)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTID(189)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGE_(190)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NGEQ(191)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBO_(192)
    { cscGlueCharactersN,             cscUndefined,               cscoNone             }, // NBSP(193)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOF_(194)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOBS(195)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOEA(196)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NONA(197)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NONP(198)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOEP(199)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NONW(200)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOEW(201)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOLW(202)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOCO(203)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOSP(204)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // NOEN(205)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NET_(206)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NCA_(207)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NCH_(208)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // WYI_(209)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBR_(210)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NRU_(211)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NOG_(212)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NSI_(213)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NSIC(214)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTN_(215)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NTNC(216)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NKH_(217)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NKHC(218)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NKHD(219)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBU_(220)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBUC(221)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NBUD(222)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NSY_(223)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NSYC(224)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NSYW(225)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NMO_(226)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NMOC(227)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NMOD(228)
    { cscNeutralCharactersNH,         cscAlphabetsOptionsN,       cscoAutospacingAlpha }, // NHS_(229)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // WHT_(230)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // LS__(231)
    { cscNeutralCharactersNH,         cscUndefined,               cscoNone             }, // XNW_(232)
};

//
// Following table maps character class (CHAR_CLASS) to width increment in the loose layout grid mode.
// We store this information on 5 bits only.
// The 4-th bit keeps information about class:
//   * 0 - stands for unambigous class (either wide or narrow),
//   * 1 - stands for ambigous class (wide or narrow - at this point we can not determine)
// Bits 2-3:
//   * for ambigous class stores width increment for narrow characters (see. table)
//   * for unambigous class are not used and set to 0
// Bits 0-1:
//   * for ambigous class stores width increment for wide characters (see. table)
//   * for unambigous class stores width increment for wide or narrow (depends on class) characters (see. table)
//
// Following table presents all possible combination:
//    | bit 1 (3) | bit 0 (2) |
//    |-----------|-----------|
//    |     0     |     0     | width is not incremented                   | width = char_width
//    |     0     |     1     | width is not incremented                   | width = char_width
//    |     1     |     0     | width is incremented by full grid size     | width = char_width + grid
//    |     1     |     1     | width is incremented by half of grid size  | width = char_width + grid/2
// 
const char s_aLooseTypeWidthIncrement[] =
{
    0x02, // WOB_(0)
    0x03, // NOPP(1)
    0x03, // NOPA(2)
    0x03, // NOPW(3)
    0x03, // HOP_(4)
    0x02, // WOP_(5)
    0x02, // WOP5(6)
    0x03, // NOQW(7)
    0x1e, // AOQW(8)
    0x02, // WOQ_(9)
    0x02, // WCB_(10)
    0x03, // NCPP(11)
    0x03, // NCPA(12)
    0x03, // NCPW(13)
    0x03, // HCP_(14)
    0x02, // WCP_(15)
    0x02, // WCP5(16)
    0x03, // NCQW(17)
    0x1e, // ACQW(18)
    0x02, // WCQ_(19)
    0x1e, // ARQW(20)
    0x03, // NCSA(21)
    0x03, // HCO_(22)
    0x02, // WC__(23)
    0x02, // WCS_(24)
    0x02, // WC5_(25)
    0x02, // WC5S(26)
    0x03, // NKS_(27)
    0x02, // WKSM(28)
    0x02, // WIM_(29)
    0x03, // NSSW(30)
    0x02, // WSS_(31)
    0x02, // WHIM(32)
    0x02, // WKIM(33)
    0x03, // NKSL(34)
    0x02, // WKS_(35)
    0x00, // WKSC(36)
    0x02, // WHS_(37)
    0x03, // NQFP(38)
    0x03, // NQFA(39)
    0x02, // WQE_(40)
    0x02, // WQE5(41)
    0x03, // NKCC(42)
    0x02, // WKC_(43)
    0x03, // NOCP(44)
    0x03, // NOCA(45)
    0x03, // NOCW(46)
    0x02, // WOC_(47)
    0x02, // WOCS(48)
    0x02, // WOC5(49)
    0x02, // WOC6(50)
    0x1e, // AHPW(51)
    0x03, // NPEP(52)
    0x03, // NPAR(53)
    0x03, // HPE_(54)
    0x02, // WPE_(55)
    0x02, // WPES(56) ???
    0x02, // WPE5(57)
    0x03, // NISW(58)
    0x1e, // AISW(59)
    0x03, // NQCS(60)
    0x03, // NQCW(61)
    0x00, // NQCC(62)
    0x03, // NPTA(63)
    0x03, // NPNA(64)
    0x03, // NPEW(65)
    0x03, // NPEH(66)
    0x1e, // APNW(67)
    0x03, // NPEW(68)
    0x02, // WPR_(69)
    0x03, // NQEP(70)
    0x03, // NQEW(71)
    0x03, // NQNW(72)
    0x1e, // AQEW(73)
    0x1e, // AQNW(74)
    0x1e, // AQLW(75)
    0x02, // WQO_(76)
    0x03, // NSBL(77)
    0x02, // WSP_(78)
    0x02, // WHI_(79)
    0x03, // NKA_(80)
    0x02, // WKA_(81)
    0x1e, // ASNW(82)
    0x1e, // ASEW(83)
    0x1e, // ASRN(84)
    0x1e, // ASEN(85)
    0x1e, // ALA_(86)
    0x1e, // AGR_(87)
    0x1e, // ACY_(88)
    0x02, // WID_(89)
    0x02, // WPUA(90)
    0x03, // NHG_(91)
    0x02, // WHG_(92)
    0x02, // WCI_(93)
    0x03, // NOI_(94)
    0x02, // WOI_(95)
    0x00, // WOIC(96)
    0x02, // WOIL(97)
    0x02, // WOIS(98)
    0x02, // WOIT(99)
    0x03, // NSEN(100)
    0x03, // NSET(101)
    0x03, // NSNW(102)
    0x1e, // ASAN(103)
    0x1e, // ASAE(104)
    0x03, // NDEA(105)
    0x02, // WD__(106)
    0x03, // NLLA(107)
    0x02, // WLA_(108)
    0x03, // NWBL(109)
    0x03, // NWZW(110)
    0x03, // NPLW(111)
    0x03, // NPZW(112)
    0x03, // NPF_(113)
    0x03, // NPFL(114)
    0x03, // NPNW(115)
    0x1e, // APLW(116)
    0x00, // APCO(117)
    0x00, // ASYW(118)
    0x03, // NHYP(119)
    0x03, // NHYW(120)
    0x1e, // AHYW(121)
    0x03, // NAPA(122)
    0x03, // NQMP(123)
    0x03, // NSLS(124)
    0x03, // NSF_(125)
    0x03, // NSBS(126)
    0x03, // NLA_(127)
    0x03, // NLQ_(128)
    0x03, // NLQN(129)
    0x1e, // ALQ_(130)
    0x03, // NGR_(131)
    0x03, // NGRN(132)
    0x03, // NGQN(133)
    0x03, // NGQ_(134)
    0x03, // NCY_(135)
    0x03, // NCYP(136)
    0x00, // NCYC(137)
    0x03, // NAR_(138)
    0x03, // NAQN(139)
    0x03, // NHB_(140)
    0x00, // NHBC(141)
    0x03, // NHBW(142)
    0x03, // NHBR(143)
    0x00, // NASR(144)
    0x00, // NAAR(145)
    0x00, // NAAC(146)
    0x00, // NAAD(147)
    0x00, // NAED(148)
    0x00, // NANW(149)
    0x00, // NAEW(150)
    0x00, // NAAS(151)
    0x00, // NHI_(152)
    0x00, // NHIN(153)
    0x00, // NHIC(154)
    0x00, // NHID(155)
    0x00, // NBE_(156)
    0x00, // NBEC(157)
    0x00, // NBED(158)
    0x00, // NGM_(159)
    0x00, // NGMC(160)
    0x00, // NGMD(161)
    0x00, // NGJ_(162)
    0x00, // NGJC(163)
    0x00, // NGJD(164)
    0x00, // NOR_(165)
    0x00, // NORC(166)
    0x00, // NORD(167)
    0x00, // NTA_(168)
    0x00, // NTAC(169)
    0x00, // NTAD(170)
    0x00, // NTE_(171)
    0x00, // NTEC(172)
    0x00, // NTED(173)
    0x00, // NKD_(174)
    0x00, // NKDC(175)
    0x00, // NKDD(176)
    0x00, // NMA_(177)
    0x00, // NMAC(178)
    0x00, // NMAD(179)
    0x00, // NTH_(180)
    0x00, // NTHC(181)
    0x00, // NTHD(182)
    0x00, // NTHT(183)
    0x00, // NLO_(184)
    0x00, // NLOC(185)
    0x00, // NLOD(186)
    0x00, // NTI_(187)
    0x00, // NTIC(188)
    0x00, // NTID(189)
    0x03, // NGE_(190)
    0x03, // NGEQ(191)
    0x03, // NBO_(192)
    0x03, // NBSP(193)
    0x03, // NOF_(194)
    0x03, // NOBS(195)
    0x03, // NOEA(196)
    0x03, // NONA(197)
    0x03, // NONP(198)
    0x03, // NOEP(199)
    0x03, // NONW(200)
    0x03, // NOEW(201)
    0x03, // NOLW(202)
    0x00, // NOCO(203)
    0x03, // NOSP(204)
    0x03, // NOEN(205)
    0x03, // NET_(206)
    0x03, // NCA_(207)
    0x03, // NCH_(208)
    0x02, // WYI_(209)
    0x03, // NBR_(210)
    0x03, // NRU_(211)
    0x03, // NOG_(212)
    0x00, // NSI_(213)
    0x00, // NSIC(214)
    0x03, // NTN_(215)
    0x00, // NTNC(216)
    0x00, // NKH_(217)
    0x00, // NKHC(218)
    0x00, // NKHD(219)
    0x00, // NBU_(220)
    0x00, // NBUC(221)
    0x00, // NBUD(222)
    0x00, // NSY_(223)
    0x00, // NSYC(224)
    0x00, // NSYW(225)
    0x00, // NMO_(226)
    0x00, // NMOC(227)
    0x00, // NMOD(228)
    0x00, // NHS_(229)
    0x02, // WHT_(230)
    0x00, // LS__(231)
    0x00, // XNW_(232)
};

//-----------------------------------------------------------------------------
//
//  Function:   LooseTypeWidthIncrement
//
//  Synopsis:   Calculate width increment for specified character in 
//              the loose grid layout.
//
//  Returns:    width increment
//
//-----------------------------------------------------------------------------

long 
LooseTypeWidthIncrement(
    TCHAR c, 
    BOOL fWide, 
    long grid)
{
    char flag = s_aLooseTypeWidthIncrement[CharClassFromCh(c)];

    // If character is narrow and class is ambigous we switch to bits 2-3.
    if ( !fWide && (flag & 0x10) )
    {
        flag = flag >> 2;
    }

    //
    // We look first at second bit and if it is set to:
    // * 0 - we don't increment character width, so we return '0'
    // * 1 - we look now at first bit and if it set to:
    //   * 0 - we increment character width by grid, so we return 'grid >> 0'
    //   * 1 - we increment character width by grid/2, so we return 'grid >> 1'
    // 
    return (flag & 0x02) ? grid >> (flag & 0x01): 0;
}

//-----------------------------------------------------------------------------
//
//  Function:   CscFromCh
//
//  Synopsis:   Determine the CSC from the char with additional information
//              about the run
//
//  Returns:    CSC
//
//-----------------------------------------------------------------------------

CSC
CscFromCh(
    const WCHAR wch,
    long csco )
{
    const PACKEDCSC pcsc = s_aCscMap[CharClassFromCh(wch)];
    CSC csc;

    if (!(pcsc.csco & csco))
    {
        csc = CSC(pcsc.csc);
    }
    else
    {
        csc = CSC(pcsc.cscAlt);
    }

    return csc;
}


//-----------------------------------------------------------------------------
//
//  Function:   PunctStartLine (member, LS callback)
//
//  Synopsis:   Unimplemented LineServices callback
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::PunctStartLine(
    PLSRUN plsrun,      // IN
    MWCLS mwcls,        // IN
    WCHAR wch,          // IN
    LSACT* plsact)      // OUT
{
    LSTRACE(PunctStartLine);
    LSNOTIMPL(PunctStartLine);
    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ModWidthOnRun (member, LS callback)
//
//  Synopsis:   Inter-run width modification
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ModWidthOnRun(
    PLSRUN plsrunFirst,     // IN
    WCHAR wchFirst,         // IN
    PLSRUN plsrunSecond,    // IN
    WCHAR wchSecond,        // IN
    LSACT* plsact)          // OUT
{
    LSTRACE(ModWidthOnRun);
    const CSC cscFirst = CscFromCh( wchFirst, plsrunFirst->_csco );
    const CSC cscSecond = CscFromCh( wchSecond, plsrunSecond->_csco );

    //
    // See http://ie/specs/secure/trident/text/csc.htm#Auto_spacing
    //

    if (   (   (cscSecond == 7 || cscSecond == 15)
            && (cscFirst == 3 || cscFirst == 16 || cscFirst == 17))
        || (   (cscFirst == 7 || cscFirst == 15)
            && (cscSecond == 1 || cscSecond == 16 || cscSecond == 17))
        || (   (cscFirst == 8)
            && (cscFirst == 16 || cscFirst == 17)))
    {
        plsact->side = sideRight;
        plsact->kamnt = kamntByQuarterEm;
    }
    else
    {
        plsact->side = sideNone;
        plsact->kamnt = kamntNone;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ModWidthSpace (member, LS callback)
//
//  Synopsis:   Space width modification
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ModWidthSpace(
    PLSRUN plsrunCur,   // IN
    PLSRUN plsrunPrev,  // IN
    WCHAR wchPrev,      // IN
    PLSRUN plsrunNext,  // IN
    WCHAR wchNext,      // IN
    LSACT* plsact)      // OUT
{
    LSTRACE(ModWidthSpace);

    plsact->side = sideNone;
    plsact->kamnt = kamntNone;

    if (plsrunPrev && plsrunNext)
    {
        const CSC cscFirst = CscFromCh( wchPrev, plsrunPrev->_csco );
        const CSC cscSecond = CscFromCh( wchNext, plsrunNext->_csco );

        //
        // See http://ie/specs/secure/trident/text/csc.htm#Auto_spacing
        //

        if (   (cscFirst == 7 || cscFirst == 15)
            && (cscSecond == 7 || cscSecond == 15))
        {
            plsact->side = sideRight;
            plsact->kamnt = kamntByHalfEm;
        }
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ExpandOrCompress (static)
//
//  Synopsis:   Whether we can expand or compress is based on the same csc
//              combination for expansion and compression.  The amount and
//              the side will vary.
//
//  Returns:    BOOL - Whether the char/option combination should be subject
//              of expansion or compression
//
//-----------------------------------------------------------------------------

BOOL
ExpandOrCompress(
    WCHAR wchFirst,     // IN
    long cscoFirst,     // IN
    WCHAR wchSecond,    // IN
    long cscoSecond )   // IN
{
    const CSC cscFirst  = CscFromCh( wchFirst,  cscoFirst );
    const CSC cscSecond = CscFromCh( wchSecond, cscoSecond );

    //
    // See http://ie/specs/secure/trident/text/Line_Just.htm
    //

    return (   (   (cscSecond == 7 || cscSecond == 15)
                && (cscFirst == 1 || cscFirst == 16 || cscFirst == 17))
            || (   (cscFirst == 7 || cscFirst == 15)
                && (cscSecond == 1 || cscSecond == 16 || cscSecond == 17))
            || (   (cscFirst == 8)
                && (cscFirst == 16 || cscFirst == 17)));
}

//-----------------------------------------------------------------------------
//
//  Function:   CompOnRun (member, LS callback)
//
//  Synopsis:   Compression on Run behavior
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CompOnRun(
    PLSRUN plsrunFirst,     // IN
    WCHAR wchFirst,         // IN
    PLSRUN plsrunSecond,    // IN
    WCHAR wchSecond,        // IN
    LSPRACT* plsract)       // OUT
{
    LSTRACE(CompOnRun);

    if (ExpandOrCompress( wchFirst, plsrunFirst->_csco, wchSecond, plsrunSecond->_csco ))
    {
        plsract->lsact.side = sideRight;
        plsract->lsact.kamnt = kamntByEighthEm;
    }
    else
    {
        plsract->lsact.side = sideNone;
        plsract->lsact.kamnt = kamntNone;
    }

    plsract->prior = prior4;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   CompWidthSpace (member, LS callback)
//
//  Synopsis:   Space width compression
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::CompWidthSpace(
    PLSRUN plsrunCur,   // IN
    PLSRUN plsrunPrev,  // IN
    WCHAR wchPrev,      // IN
    PLSRUN plsrunNext,  // IN
    WCHAR wchNext,      // IN
    LSPRACT* plspract)  // OUT
{
    LSTRACE(CompWidthSpace);

    if (_pPFFirst->_uTextJustify == styleTextJustifyNewspaper)
    {
        // Newspaper justification is independent of surrounding character classes
        plspract->lsact.kamnt = kamntToUserDefinedComp;
    }
    else
    {
        const BOOL fNonFixedPitch = 0 == (plsrunCur->GetCF()->_bPitchAndFamily & FIXED_PITCH);

        if (   plsrunPrev
            && plsrunNext
            && fNonFixedPitch
            && ExpandOrCompress(wchPrev, plsrunPrev->_csco, wchNext, plsrunNext->_csco))
        {
            plspract->lsact.kamnt = kamntToHalfEm;
        }
        else if (fNonFixedPitch)
        {
            plspract->lsact.kamnt = kamntToQuarterEm;
        }
        else
        {
            plspract->lsact.kamnt = kamntNone;
        }
    }

    //plspract->lsact.side = plspract->lsact.kamnt ? sideLeft : sideNone;
    plspract->lsact.side = sideRight; // LS requires sideRight
    plspract->prior = plspract->lsact.kamnt ? prior1 : prior0;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ExpOnRun (member, LS callback)
//
//  Synopsis:   Determine if we should expand inbetween a given pair of runs
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ExpOnRun(
    PLSRUN plsrunFirst,     // IN
    WCHAR wchFirst,         // IN
    PLSRUN plsrunSecond,    // IN
    WCHAR wchSecond,        // IN
    LSACT* plsact)          // OUT
{
    LSTRACE(ExpOnRun);

    if (ExpandOrCompress( wchFirst, plsrunFirst->_csco, wchSecond, plsrunSecond->_csco ))
    {
        plsact->side = sideRight;
        plsact->kamnt = kamntByQuarterEm;
    }
    else
    {
        plsact->side = sideNone;
        plsact->kamnt = kamntNone;
    }

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   ExpWidthSpace (member, LS callback)
//
//  Synopsis:   Space width expansion
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::ExpWidthSpace(
    PLSRUN plsrunCur,   // IN
    PLSRUN plsrunPrev,  // IN
    WCHAR wchPrev,      // IN
    PLSRUN plsrunNext,  // IN
    WCHAR wchNext,      // IN
    LSACT* plsact)      // OUT
{
    LSTRACE(ExpWidthSpace);

    if (_pPFFirst->_uTextJustify == styleTextJustifyNewspaper)
    {
        // Newspaper justification is independent of surrounding character classes
        plsact->kamnt = kamntToUserDefinedExpan;
    }
    else
    {
        plsact->kamnt = kamntToHalfEm;
    }

    plsact->side = sideRight;

    return lserrNone;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetModWidthClasses (member, LS callback)
//
//  Synopsis:   Return the width-modification index for each character
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::GetModWidthClasses(
    PLSRUN plsrun,      // IN
    const WCHAR* rgwch, // IN
    DWORD cwch,         // IN
    MWCLS* rgmwcls)     // OUT
{
    LSTRACE(GetModWidthClasses);

    const WCHAR * pchSrc = rgwch;
    MWCLS * pbDst = rgmwcls;
#ifdef _MAC
    PACKEDCSC pcscT  = { 0, 0, 0 };
    pcscT.csco = plsrun->_csco;
#else
    const PACKEDCSC pcscT = { 0, 0, plsrun->_csco };
#endif
    const unsigned short uValue = *(unsigned short *)&pcscT;

    while (cwch--)
    {
        WHEN_DBG( const CHAR_CLASS cc = CharClassFromCh(*pchSrc); Assert(!g_Zero.ab[0] || cc); )

        PACKEDCSC pcsc = s_aCscMap[CharClassFromCh(*pchSrc++)];

        if (!((*(unsigned short *)&pcsc) & uValue))
        {
            *pbDst++ = pcsc.csc;
        }
        else
        {
            *pbDst++ = pcsc.cscAlt;
        }
    }

    return lserrNone;
}

// The following table contains indices into the rgpairact table of actions
const BYTE s_rgmodwidthpairs[cscLim][cscLim] =
{
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 =cscAfter / cscBefore:
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  0 cscUndefined
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  1 cscOpenParenN
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,    //  2 cscOpenParenW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  3 cscCloseParenN
    0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2,    //  4 cscCloseParenW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  5 cscNeutralN
    0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 2,    //  6 cscCommaW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  7 cscKanaW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  8 cscPunctW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  9 cscCenteredN
    0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4,    // 10 cscCenteredW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 11 cscPeriodN
    0, 0, 2, 0, 2, 0, 2, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2,    // 12 cscPeriodW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 13 cscSpaceN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 14 cscSpaceW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 15 cscIdeoW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 16 cscAlphaN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 17 cscEllipse
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 18 cscGlueA
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 19 cscPrefixN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 20 cscPostfixN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 21 cscDigitsN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    // 22 cscDigits
    0, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3     // 23 cscOpenQuotesA
};

const LSPAIRACT s_rglspairactV[] =
// first:  side, amount, second: side, amount
{
    sideRight, kamntNone,       sideLeft, kamntNone,
    sideRight, kamntNone,       sideLeft, kamntByHalfEm,
    sideRight, kamntByHalfEm,   sideLeft, kamntNone,
    sideRight, kamntByHalfEm,   sideLeft, kamntNone,
    sideRight, kamntNone,       sideLeft, kamntNone,
    sideRight, kamntByHalfEm,   sideLeft, kamntByHalfEm,
};

HRESULT
CLineServices::SetModWidthPairs()
{
    WHEN_DBG(LSERR lserr = )
            LsSetModWidthPairs( _plsc, sizeof(s_rglspairactV) / sizeof(LSPAIRACT),
                                s_rglspairactV, cscLim, (BYTE *)s_rgmodwidthpairs );

    Assert(!g_Zero.ab[0] || lserr);

    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Function:   GetEms (member, LS callback)
//
//  Synopsis:   Determine the adjustment amount
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

const long kcchWide     = 41; // approx 5.5" at 10 points
const long kcchNarrow   = 14; // approx 2" at 10 points
const long klCompWide   = 85;
const long klCompNarrow = 75;
const long klExpWide    = 150;
const long klExpNarrow  = 250;

long
Interpolate(int cch, long lMinVal, long lMaxVal)
{
    return (cch <= kcchNarrow)
            ? lMinVal
            : ( (cch >= kcchWide)
                ? lMaxVal
                : lMinVal + MulDivQuick(lMaxVal - lMinVal, cch, kcchWide - kcchNarrow) );
}

LSERR WINAPI
CLineServices::GetEms(
    PLSRUN plsrun,      // IN
    LSTFLOW kTFlow,     // IN
    PLSEMS plems)       // OUT
{
    LSTRACE(GetEms);
    LSERR  lserr;
    CCcs  *pccs = GetCcs(plsrun, _pci->_hdc, _pci);
    
    if (pccs)
    {
        CBaseCcs *pBaseCcs = pccs->GetBaseCcs();
        const int em = pBaseCcs->_xAveCharWidth;

        plems->em   = em;
        plems->em2  = em / 2;
        plems->em3  = em / 3;
        plems->em4  = em / 4;
        plems->em8  = em / 8;
        plems->em16 = MulDivQuick( em, 15, 16 );

        if (_pPFFirst->_uTextJustify == styleTextJustifyNewspaper)
        {
            // get number of chars in column of text
            const int cchCol = _xWrappingWidth / pBaseCcs->_xAveCharWidth;

            plems->udExp  = max(1, MulDivQuick(em, Interpolate(cchCol, klExpNarrow,  klExpWide),  100));
            plems->udComp = max(1, MulDivQuick(em, Interpolate(cchCol, klCompNarrow, klCompWide), 100));
        }
        lserr = lserrNone;
    }
    else
    {
        lserr = lserrOutOfMemory;
    }
    return lserr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CheckSetExpansion (member)
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

// The following array contains indices into the rgexp array of expansion actions
#define A 3 // A means expansion allowed always
#define X 2 // X means expansion never
            // 0 means expansion allowed for distributed alignment only

// Expansion actions
const LSEXPAN s_alsexpanTable[] =
{
    // FullScaled  Distributed
    {      0,          1     }, // "0" Distributed only
    {      1,          0     }, //     Justified only -- NOT USED
    {      0,          0     }, // "X" No expansion ever
    {      1,          1     }  // "A" Distributed or Justified (FullScaled)
};

const BYTE s_abIndexExpansion[cscLim][cscLim] =
{
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 =cscAfter / cscBefore:
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  0 cscUndefined
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, 0, 0, X, 0, 0, 0, 0,    //  1 cscOpenParenN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, A, 0, 0, 0, 0, X, 0, 0, 0, 0,    //  2 cscOpenParenW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, 0, 0, X, 0, 0, 0, 0,    //  3 cscCloseParenN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, A, 0, 0, 0, 0, X, 0, 0, 0, 0,    //  4 cscCloseParenW
    0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, A, 0, A, 0, A, 0, 0, 0, X, A, A, 0, 0,    //  5 cscNeutralN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, 0, 0, X, 0, 0, 0, 0,    //  6 cscCommaW
    0, 0, 0, 0, 0, A, 0, A, 0, 0, 0, A, 0, A, A, A, A, A, 0, X, A, A, A, 0,    //  7 cscKanaW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, 0, 0, X, 0, 0, 0, 0,    //  8 cscPunctW
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, 0, 0, X, 0, 0, 0, 0,    //  9 cscCenteredN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, A, 0, 0, 0, 0, X, 0, 0, 0, 0,    // 10 cscCenteredW
    0, 0, 0, 0, 0, A, 0, A, 0, 0, 0, A, 0, A, A, A, A, A, 0, X, A, A, A, 0,    // 11 cscPeriodN
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, A, 0, 0, 0, 0, X, 0, 0, 0, 0,    // 12 cscPeriodW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 13 cscSpaceN
    0, 0, A, 0, A, 0, 0, A, 0, 0, A, A, A, A, A, A, 0, 0, 0, X, A, A, 0, 0,    // 14 cscSpaceW
    0, 0, 0, 0, 0, A, 0, A, 0, 0, 0, A, 0, A, A, A, A, A, 0, X, A, A, A, 0,    // 15 cscIdeoW
    0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, A, 0, A, 0, A, 0, 0, 0, X, A, A, 0, 0,    // 16 cscAlphaN
    0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, 0, A, A, 0, A, 0, 0, 0, X, A, 0, 0, 0,    // 17 cscEllipse
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, 0, 0, A, 0, X, X, 0, 0, 0, 0,    // 18 cscGlueA
    0, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,    // 19 cscPrefixN
    0, 0, 0, 0, 0, A, 0, A, 0, 0, 0, A, 0, A, A, A, A, 0, 0, X, A, A, 0, 0,    // 20 cscPostfixN
    0, 0, 0, 0, 0, A, 0, A, 0, 0, 0, A, 0, A, A, A, A, A, 0, X, A, A, A, 0,    // 21 cscDigitsN
    0, 0, 0, 0, 0, 0, 0, A, 0, 0, 0, 0, A, A, 0, A, 0, 0, 0, X, A, 0, 0, 0,    // 22 cscDigits
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, A, A, 0, 0, 0, 0, X, 0, 0, 0, 0     // 23 cscOpenQuotesA
};


const BYTE s_abIndexExpansionNewspaperNonFE[cscLim][cscLim] =
{
//  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22,23 =cscAfter / cscBefore:
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    //  0 cscUndefined
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  1 cscOpenParenN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  2 cscOpenParenW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  3 cscCloseParenN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  4 cscCloseParenW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  5 cscNeutralN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  6 cscCommaW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  7 cscKanaW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  8 cscPunctW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    //  9 cscCenteredN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 10 cscCenteredW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 11 cscPeriodN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 12 cscPeriodW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 13 cscSpaceN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 14 cscSpaceW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 15 cscIdeoW
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 16 cscAlphaN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 17 cscEllipse
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, X, A, A, A, A,    // 18 cscGlueA
    0, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,    // 19 cscPrefixN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 20 cscPostfixN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 21 cscDigitsN
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A,    // 22 cscDigits
    0, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, A, X, A, A, A, A     // 23 cscOpenQuotesA
};

#undef A
#undef X

LSERR
CLineServices::CheckSetExpansion()
{
    LSERR lserr;
    BYTE * abIndexExpansion = (BYTE *)
                              ( _pPFFirst->_uTextJustify == styleTextJustifyNewspaper 
                                ? s_abIndexExpansion
                                : s_abIndexExpansionNewspaperNonFE );

    if (_abIndexExpansion == abIndexExpansion)
    {
        lserr = lserrNone;
    }
    else
    {
        lserr = LsSetExpansion( _plsc,
                                ARRAY_SIZE(s_alsexpanTable), s_alsexpanTable,
                                cscLim, abIndexExpansion);

        _abIndexExpansion = abIndexExpansion;
    }

    return lserr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CheckSetCompression (member)
//
//  Returns:    LSERR
//
//-----------------------------------------------------------------------------

const LSPRACT s_alspractCompression[] =
{
    prior0,     sideNone,       kamntNone,          // not defined
    prior3,     sideLeft,       kamntToHalfEm,      // Open Paren
    prior3,     sideRight,      kamntToHalfEm,      // Close Paren, Comma+Period
    prior5,     sideLeftRight,  kamntTo15Sixteenth, // Kana Compression
    prior2,     sideLeftRight,  kamntToHalfEm,      // Centered Characters

    // Priority 4 is not defined here, reserved for CompOnRun
    // Priority 1 is not defined here, reserved for CompWidthSpace
};

const BYTE s_abIndexPract[cscLim] =
{
    0,  //  0 cscUndefined
    0,  //  1 cscOpenParenN
    1,  //  2 cscOpenParenW
    0,  //  3 cscCloseParenN
    2,  //  4 cscCloseParenW
    0,  //  5 cscNeutralN
    2,  //  6 cscCommaW
    3,  //  7 cscKanaW
    0,  //  8 cscPunctW
    0,  //  9 cscCenteredN
    4,  // 10 cscCenteredW
    0,  // 11 cscPeriodN
    2,  // 12 cscPeriodW
    0,  // 13 cscSpaceN
    0,  // 14 cscSpaceW
    0,  // 15 cscIdeoW
    0,  // 16 cscAlphaN
    0,  // 17 cscEllipse
    0,  // 18 cscGlueA
    0,  // 19 cscPrefixN
    0,  // 20 cscPostfixN
    0,  // 21 cscDigitsN
    0,  // 22 cscDigits
    0,  // 23 cscOpenQuotesA
};

LSERR
CLineServices::CheckSetCompression()
{
    LSERR lserr;

    if (!_abIndexPract)
    {
        lserr = LsSetCompression( _plsc, priorNum,
                                  ARRAY_SIZE(s_alspractCompression), s_alspractCompression,
                                  cscLim, (BYTE *)s_abIndexPract );

        _abIndexPract = (BYTE *)s_abIndexPract;
    }
    else
    {
        lserr = lserrNone;
    }

    return lserr;
}

//-----------------------------------------------------------------------------
//
//  Function:   FGetLastLineJustification (member, LS callback)
//
//  Synopsis:   When you have a justified line, you can either justify or not
//              the last line.  Under some circumstance, you may want to 
//              justfify the line at BRs in addition to block boundaries.
//
//  Returns:    lserrNone
//
//-----------------------------------------------------------------------------

LSERR WINAPI
CLineServices::FGetLastLineJustification(
    LSKJUST lskj,               // IN
    LSKALIGN lskal,             // IN
    ENDRES endr,                // IN
    BOOL * pfJustifyLastLine,   // OUT
    LSKALIGN * plskalLine)      // OUT
{
    LSTRACE(FGetLastLineJustification);

    BOOL fJustifyLastLine;

    //
    // We get endr == endrSoftCR whether we have a <BR> or a </P>.  We're interested
    // in the former, but we need to dig around some to determine it.  Note that the
    // assumption here is that if we're being called, any BR we seen in the COneRun
    // linklist is in fact on the line (ie we didn't end up breaking somewhere before
    // the BR after having scanned it.)
    //

    if (_pPFFirst->_uTextJustify == styleTextJustifyDistributeAllLines)
    {
        fJustifyLastLine = TRUE;
    }
    else if (_pPFFirst->_uTextJustify == styleTextJustifyNewspaper)
    {
        fJustifyLastLine =    endr == endrSoftCR
                           && _listCurrent._pTail->Branch()->Tag() == ETAG_BR;
    }
    else
    {
        fJustifyLastLine = FALSE;
    }

    *pfJustifyLastLine = fJustifyLastLine;
    *plskalLine = lskal;
    
    return lserrNone;
}

