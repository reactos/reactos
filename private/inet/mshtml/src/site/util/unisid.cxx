/*
 *  @doc    INTERNAL
 *
 *  @module UNISID.CXX -- Unicode Script ID
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
#ifndef X_UNISID_H_
#define X_UNISID_H_
#include "unisid.h"
#endif

#ifndef X_UNIPART_H_
#define X_UNIPART_H_
#include "unipart.h"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include <codepage.h>
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include <intl.hxx>
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include <_fontlnk.h>
#endif

const SCRIPT_ID asidScriptIDFromCharClass[]=
{
    // CC           SCRIPT_ID
    /* WOB_   1*/   sidHan,
    /* NOPP   2*/   sidAsciiSym,
    /* NOPA   2*/   sidAsciiSym,
    /* NOPW   2*/   sidLatin,//sidDefault,
    /* HOP_   3*/   sidHalfWidthKana,
    /* WOP_   4*/   sidHan,
    /* WOP5   5*/   sidHan,
    /* NOQW   6*/   sidLatin,//sidDefault,
    /* AOQW   7*/   sidAmbiguous,
    /* WOQ_   8*/   sidHan,
    /* WCB_   9*/   sidHan,
    /* NCPP  10*/   sidAsciiSym,
    /* NCPA  10*/   sidAsciiSym,
    /* NCPW  10*/   sidDefault,
    /* HCP_  11*/   sidHalfWidthKana,
    /* WCP_  12*/   sidHan,
    /* WCP5  13*/   sidHan,
    /* NCQW  14*/   sidLatin,//sidDefault,
    /* ACQW  15*/   sidAmbiguous,
    /* WCQ_  16*/   sidHan,
    /* ARQW  17*/   sidAmbiguous,
    /* NCSA  18*/   sidAsciiSym,
    /* HCO_  19*/   sidHalfWidthKana,
    /* WC__  20*/   sidHan,
    /* WCS_  20*/   sidHan,
    /* WC5_  21*/   sidHan,
    /* WC5S  21*/   sidHan,
    /* NKS_  22*/   sidHalfWidthKana,
    /* WKSM  23*/   sidKana,
    /* WIM_  24*/   sidHan,
    /* NSSW  25*/   sidDefault,
    /* WSS_  26*/   sidHan,
    /* WHIM  27*/   sidKana,
    /* WKIM  28*/   sidKana,
    /* NKSL  29*/   sidHalfWidthKana,
    /* WKS_  30*/   sidKana,
    /* WKSC  30*/   sidKana,
    /* WHS_  31*/   sidKana,
    /* NQFP  32*/   sidAsciiSym,
    /* NQFA  32*/   sidAsciiSym,
    /* WQE_  33*/   sidHan,
    /* WQE5  34*/   sidHan,
    /* NKCC  35*/   sidHalfWidthKana,
    /* WKC_  36*/   sidKana,
    /* NOCP  37*/   sidAsciiSym,
    /* NOCA  37*/   sidAsciiSym,
    /* NOCW  37*/   sidLatin,
    /* WOC_  38*/   sidHan,
    /* WOCS  38*/   sidHan,
    /* WOC5  39*/   sidHan,
    /* WOC6  39*/   sidHan,
    /* AHPW  40*/   sidAmbiguous,
    /* NPEP  41*/   sidAsciiSym,
    /* NPAR  41*/   sidAmbiguous,
    /* HPE_  42*/   sidHalfWidthKana,
    /* WPE_  43*/   sidHan,
    /* WPES  43*/   sidHan,
    /* WPE5  44*/   sidHan,
    /* NISW  45*/   sidDefault,
    /* AISW  46*/   sidAmbiguous,
    /* NQCS  47*/   sidAmbiguous,
    /* NQCW  47*/   sidAmbiguous,
    /* NQCC  47*/   sidAmbiguous,
    /* NPTA  48*/   sidAsciiSym,
    /* NPNA  48*/   sidAsciiSym,
    /* NPEW  48*/   sidLatin,//sidDefault,
    /* NPEH  48*/   sidHebrew,
    /* APNW  49*/   sidAmbiguous,
    /* HPEW  50*/   sidHangul,
    /* WPR_  51*/   sidHan,
    /* NQEP  52*/   sidAsciiSym,
    /* NQEW  52*/   sidLatin,//sidDefault,
    /* NQNW  52*/   sidDefault,
    /* AQEW  53*/   sidAmbiguous,
    /* AQNW  53*/   sidAmbiguous,
    /* AQLW  53*/   sidAmbiguous,
    /* WQO_  54*/   sidHan,
    /* NSBL  55*/   sidAsciiSym,
    /* WSP_  56*/   sidHan,
    /* WHI_  57*/   sidKana,
    /* NKA_  58*/   sidHalfWidthKana,
    /* WKA_  59*/   sidKana,
    /* ASNW  60*/   sidAmbiguous,
    /* ASEW  60*/   sidAmbiguous,
    /* ASRN  60*/   sidAmbiguous,
    /* ASEN  60*/   sidAmbiguous,
    /* ALA_  61*/   sidAmbiguous, // sidLatin    | sidAncillary,
    /* AGR_  62*/   sidAmbiguous, // sidGreek    | sidAncillary,
    /* ACY_  63*/   sidAmbiguous, // sidCyrillic | sidAncillary,
    /* WID_  64*/   sidHan,
    /* WPUA  65*/   sidEUDC,
    /* NHG_  66*/   sidHangul,
    /* WHG_  67*/   sidHangul,
    /* WCI_  68*/   sidHan,
    /* NOI_  69*/   sidHan,
    /* WOI_  70*/   sidHan,
    /* WOIC  70*/   sidHan,
    /* WOIL  70*/   sidHan,
    /* WOIS  70*/   sidHan,
    /* WOIT  70*/   sidHan,
    /* NSEN  71*/   sidDefault,
    /* NSET  71*/   sidDefault,
    /* NSNW  71*/   sidDefault,
    /* ASAN  72*/   sidAmbiguous,
    /* ASAE  72*/   sidAmbiguous,
    /* NDEA  73*/   sidAsciiLatin,
    /* WD__  74*/   sidHan,
    /* NLLA  75*/   sidAsciiLatin,
    /* WLA_  76*/   sidHan,
    /* NWBL  77*/   sidDefault,
    /* NWZW  77*/   sidDefault,
    /* NPLW  78*/   sidAmbiguous,
    /* NPZW  78*/   sidAmbiguous,
    /* NPF_  78*/   sidAmbiguous,
    /* NPFL  78*/   sidAmbiguous,
    /* NPNW  78*/   sidAmbiguous,
    /* APLW  79*/   sidAmbiguous,
    /* APCO  79*/   sidAmbiguous,
    /* ASYW  80*/   sidAmbiguous,
    /* NHYP  81*/   sidDefault,
    /* NHYW  81*/   sidDefault,
    /* AHYW  82*/   sidAmbiguous,
    /* NAPA  83*/   sidAsciiSym,
    /* NQMP  84*/   sidAsciiSym,
    /* NSLS  85*/   sidAsciiSym,
    /* NSF_  86*/   sidAmbiguous,
    /* NSBS  86*/   sidAmbiguous,
    /* NLA_  87*/   sidLatin,
    /* NLQ_  88*/   sidLatin,
    /* NLQN  88*/   sidLatin,
    /* ALQ_  89*/   sidAmbiguous,
    /* NGR_  90*/   sidGreek,
    /* NGRN  90*/   sidGreek,
    /* NGQ_  91*/   sidGreek,
    /* NGQN  91*/   sidGreek,
    /* NCY_  92*/   sidCyrillic,
    /* NCYP  93*/   sidCyrillic,
    /* NCYC  93*/   sidCyrillic,
    /* NAR_  94*/   sidArmenian,
    /* NAQN  95*/   sidArmenian,
    /* NHB_  96*/   sidHebrew,
    /* NHBC  96*/   sidHebrew,
    /* NHBW  96*/   sidHebrew,
    /* NHBR  96*/   sidHebrew,
    /* NASR  97*/   sidArabic,
    /* NAAR  97*/   sidArabic,
    /* NAAC  97*/   sidArabic,
    /* NAAD  97*/   sidArabic,
    /* NAED  97*/   sidArabic,
    /* NANW  97*/   sidArabic,
    /* NAEW  97*/   sidArabic,
    /* NAAS  97*/   sidArabic,
    /* NHI_  98*/   sidDevanagari,
    /* NHIN  98*/   sidDevanagari,
    /* NHIC  98*/   sidDevanagari,
    /* NHID  98*/   sidDevanagari,
    /* NBE_  99*/   sidBengali,
    /* NBEC  99*/   sidBengali,
    /* NBED  99*/   sidBengali,
    /* NGM_ 100*/   sidGurmukhi,
    /* NGMC 100*/   sidGurmukhi,
    /* NGMD 100*/   sidGurmukhi,
    /* NGJ_ 101*/   sidGujarati,
    /* NGJC 101*/   sidGujarati,
    /* NGJD 101*/   sidGujarati,
    /* NOR_ 102*/   sidOriya,
    /* NORC 102*/   sidOriya,
    /* NORD 102*/   sidOriya,
    /* NTA_ 103*/   sidTamil,
    /* NTAC 103*/   sidTamil,
    /* NTAD 103*/   sidTamil,
    /* NTE_ 104*/   sidTelugu,
    /* NTEC 104*/   sidTelugu,
    /* NTED 104*/   sidTelugu,
    /* NKD_ 105*/   sidKannada,
    /* NKDC 105*/   sidKannada,
    /* NKDD 105*/   sidKannada,
    /* NMA_ 106*/   sidMalayalam,
    /* NMAC 106*/   sidMalayalam,
    /* NMAD 106*/   sidMalayalam,
    /* NTH_ 107*/   sidThai,
    /* NTHC 107*/   sidThai,
    /* NTHD 107*/   sidThai,
    /* NTHT 107*/   sidThai,
    /* NLO_ 108*/   sidLao,
    /* NLOC 108*/   sidLao,
    /* NLOD 108*/   sidLao,
    /* NTI_ 109*/   sidTibetan,
    /* NTIC 109*/   sidTibetan,
    /* NTID 109*/   sidTibetan,
    /* NGE_ 110*/   sidGeorgian,
    /* NGEQ 111*/   sidGeorgian,
    /* NBO_ 112*/   sidBopomofo,
    /* NBSP 113*/   sidMerge,
    /* NOF_ 114*/   sidAmbiguous,
    /* NOBS 114*/   sidAmbiguous,
    /* NOEA 114*/   sidAsciiSym,
    /* NONA 114*/   sidAsciiSym,
    /* NONP 114*/   sidAsciiSym,
    /* NOEP 114*/   sidAsciiSym,
    /* NONW 114*/   sidLatin,
    /* NOEW 114*/   sidLatin,
    /* NOLW 114*/   sidLatin,
    /* NOCO 114*/   sidLatin,
    /* NOSP 114*/   sidAmbiguous,
    /* NOEN 114*/   sidDefault,
    /* NET_ 115*/   sidEthiopic,
    /* NCA_ 116*/   sidCanSyllabic,
    /* NCH_ 117*/   sidCherokee,
    /* WYI_ 118*/   sidYi,
    /* NBR_ 119*/   sidBraille,
    /* NRU_ 120*/   sidRunic,
    /* NOG_ 121*/   sidOgham,
    /* NSI_ 122*/   sidSinhala,
    /* NSIC 122*/   sidSinhala,
    /* NTN_ 123*/   sidThaana,
    /* NTNC 123*/   sidThaana,
    /* NKH_ 124*/   sidKhmer,
    /* NKHC 124*/   sidKhmer,
    /* NKHD 124*/   sidKhmer,
    /* NBU_ 125*/   sidBurmese,
    /* NBUC 125*/   sidBurmese,
    /* NBUD 125*/   sidBurmese,
    /* NSY_ 126*/   sidSyriac,
    /* NSYC 126*/   sidSyriac,
    /* NSYW 126*/   sidSyriac,
    /* NMO_ 127*/   sidMongolian,
    /* NMOC 127*/   sidMongolian,
    /* NMOD 127*/   sidMongolian,
#ifndef NO_UTF16
    /* NHS_ 128*/   sidSurrogateA,
    /* WHT_ 129*/   sidSurrogateB,
#else
    /* NHS_ 128*/   sidDefault,
    /* WHT_ 129*/   sidDefault,
#endif
    /* LS__ 130*/   sidMerge,
    /* XNW_ 131*/   sidDefault,
};

// NB (cthrash) This table name is a little misleading.  Obviously not all
// script ids in the ASCII range are sidAscii(Latin or Sym).  This is just
// a quick lookup for the most common characters on the web.

const SCRIPT_ID asidAscii[128] =
{
    sidMerge,            // U+0000
    sidMerge,            // U+0001
    sidMerge,            // U+0002
    sidMerge,            // U+0003
    sidMerge,            // U+0004
    sidMerge,            // U+0005
    sidMerge,            // U+0006
    sidMerge,            // U+0007
    sidMerge,            // U+0008
    sidMerge,            // U+0009
    sidMerge,            // U+000A
    sidMerge,            // U+000B
    sidMerge,            // U+000C
    sidMerge,            // U+000D
    sidMerge,            // U+000E
    sidMerge,            // U+000F
    sidMerge,            // U+0010
    sidMerge,            // U+0011
    sidMerge,            // U+0012
    sidMerge,            // U+0013
    sidMerge,            // U+0014
    sidMerge,            // U+0015
    sidMerge,            // U+0016
    sidMerge,            // U+0017
    sidMerge,            // U+0018
    sidMerge,            // U+0019
    sidMerge,            // U+001A
    sidMerge,            // U+001B
    sidMerge,            // U+001C
    sidMerge,            // U+001D
    sidMerge,            // U+001E
    sidMerge,            // U+001F
    sidMerge,            // U+0020
    sidAsciiSym,         // U+0021
    sidAsciiSym,         // U+0022
    sidAsciiSym,         // U+0023
    sidAsciiSym,         // U+0024
    sidAsciiSym,         // U+0025
    sidAsciiSym,         // U+0026
    sidAsciiSym,         // U+0027
    sidAsciiSym,         // U+0028
    sidAsciiSym,         // U+0029
    sidAsciiSym,         // U+002A
    sidAsciiSym,         // U+002B
    sidAsciiSym,         // U+002C
    sidDefault,          // U+002D
    sidAsciiSym,         // U+002E
    sidAsciiSym,         // U+002F
    sidAsciiLatin,       // U+0030
    sidAsciiLatin,       // U+0031
    sidAsciiLatin,       // U+0032
    sidAsciiLatin,       // U+0033
    sidAsciiLatin,       // U+0034
    sidAsciiLatin,       // U+0035
    sidAsciiLatin,       // U+0036
    sidAsciiLatin,       // U+0037
    sidAsciiLatin,       // U+0038
    sidAsciiLatin,       // U+0039
    sidAsciiSym,         // U+003A
    sidAsciiSym,         // U+003B
    sidAsciiSym,         // U+003C
    sidAsciiSym,         // U+003D
    sidAsciiSym,         // U+003E
    sidAsciiSym,         // U+003F
    sidAsciiSym,         // U+0040
    sidAsciiLatin,       // U+0041
    sidAsciiLatin,       // U+0042
    sidAsciiLatin,       // U+0043
    sidAsciiLatin,       // U+0044
    sidAsciiLatin,       // U+0045
    sidAsciiLatin,       // U+0046
    sidAsciiLatin,       // U+0047
    sidAsciiLatin,       // U+0048
    sidAsciiLatin,       // U+0049
    sidAsciiLatin,       // U+004A
    sidAsciiLatin,       // U+004B
    sidAsciiLatin,       // U+004C
    sidAsciiLatin,       // U+004D
    sidAsciiLatin,       // U+004E
    sidAsciiLatin,       // U+004F
    sidAsciiLatin,       // U+0050
    sidAsciiLatin,       // U+0051
    sidAsciiLatin,       // U+0052
    sidAsciiLatin,       // U+0053
    sidAsciiLatin,       // U+0054
    sidAsciiLatin,       // U+0055
    sidAsciiLatin,       // U+0056
    sidAsciiLatin,       // U+0057
    sidAsciiLatin,       // U+0058
    sidAsciiLatin,       // U+0059
    sidAsciiLatin,       // U+005A
    sidAsciiSym,         // U+005B
    sidAsciiSym,         // U+005C
    sidAsciiSym,         // U+005D
    sidAsciiSym,         // U+005E
    sidAsciiSym,         // U+005F
    sidAsciiSym,         // U+0060
    sidAsciiLatin,       // U+0061
    sidAsciiLatin,       // U+0062
    sidAsciiLatin,       // U+0063
    sidAsciiLatin,       // U+0064
    sidAsciiLatin,       // U+0065
    sidAsciiLatin,       // U+0066
    sidAsciiLatin,       // U+0067
    sidAsciiLatin,       // U+0068
    sidAsciiLatin,       // U+0069
    sidAsciiLatin,       // U+006A
    sidAsciiLatin,       // U+006B
    sidAsciiLatin,       // U+006C
    sidAsciiLatin,       // U+006D
    sidAsciiLatin,       // U+006E
    sidAsciiLatin,       // U+006F
    sidAsciiLatin,       // U+0070
    sidAsciiLatin,       // U+0071
    sidAsciiLatin,       // U+0072
    sidAsciiLatin,       // U+0073
    sidAsciiLatin,       // U+0074
    sidAsciiLatin,       // U+0075
    sidAsciiLatin,       // U+0076
    sidAsciiLatin,       // U+0077
    sidAsciiLatin,       // U+0078
    sidAsciiLatin,       // U+0079
    sidAsciiLatin,       // U+007A
    sidAsciiSym,         // U+007B
    sidAsciiSym,         // U+007C
    sidAsciiSym,         // U+007D
    sidAsciiSym,         // U+007E
    sidAsciiSym,         // U+007F
};

//+----------------------------------------------------------------------------
//
//  Function:   ScriptIDFromCharClass
//
//  Synopsis:   Given a character class, this function returns the proper
//              script id.
//
//-----------------------------------------------------------------------------

SCRIPT_ID
ScriptIDFromCharClass(CHAR_CLASS cc)
{
    Assert(cc >= 0 && cc < CHAR_CLASS_MAX);

    return asidScriptIDFromCharClass[cc];
}

//
// We must not fontlink for sidEUDC -- GDI will handle
//

#define SCRIPT_BIT_CONST ScriptBit(sidEUDC)

//+----------------------------------------------------------------------------
//
//  Function:       ScriptIDsFromFontSignature
//
//  Synopsis:       Compute the SCRIPT_IDS information based on the Unicode
//                  subrange coverage of the TrueType font.
//
//  Returns:        SCRIPT_IDS
//
//-----------------------------------------------------------------------------

// NB: sidDefault (==0) is not a legal value here.

static SCRIPT_ID s_asidUnicodeSubRangeScriptMapping[] =
{
    sidAsciiLatin, sidLatin,      sidLatin,      sidLatin,        // 128-131
    sidLatin,      sidLatin,      0,             sidGreek,        // 132-135
    sidGreek,      sidCyrillic,   sidArmenian,   sidHebrew,       // 136-139
    sidHebrew,     sidArabic,     sidArabic,     sidDevanagari,   // 140-143
    sidBengali,    sidGurmukhi,   sidGujarati,   sidOriya,        // 144-147
    sidTamil,      sidTelugu,     sidKannada,    sidMalayalam,    // 148-151
    sidThai,       sidLao,        sidGeorgian,   sidGeorgian,     // 152-155
    sidHangul,     sidLatin,      sidGreek,      0,               // 156-159
    0,             0,             0,             0,               // 160-163
    0,             0,             0,             0,               // 164-167
    0,             0,             0,             0,               // 168-171
    0,             0,             0,             0,               // 172-175
    sidHan,        sidKana,       sidKana,       sidBopomofo,     // 176-179
    sidHangul,     0,             0,             0,               // 180-183
    sidHangul,     sidHangul,     sidHangul,     sidHan,          // 184-187
    0,             sidHan,        0,             0,               // 188-191
    0,             0,             0,             0,               // 192-195
    0,             0,             sidHangul,     0,               // 196-199
};

#define UNICODE_SUBRANGES ARRAY_SIZE(s_asidUnicodeSubRangeScriptMapping)

SCRIPT_IDS
ScriptIDsFromFontSignature(
    FONTSIGNATURE * pfs )
{
    SCRIPT_IDS sids = sidsNotSet;
    int i;
    DWORD dwUsbBits;
    BOOL fHangul = FALSE;

    dwUsbBits = pfs->fsUsb[0];

    if (dwUsbBits)
    {
        for (i=0; i<32; i++)
        {
            if (dwUsbBits & 1)
            {
                SCRIPT_ID sid = s_asidUnicodeSubRangeScriptMapping[i];

                if (sid)
                {
                    sids |= ScriptBit(sid);
                }
            }

            dwUsbBits >>= 1;
        }
    }

    dwUsbBits = pfs->fsUsb[1];

    if (dwUsbBits)
    {
        fHangul = dwUsbBits & 0x01000000; // USR#56 = 32 + 24

        for (i=32; i<64; i++)
        {
            if (dwUsbBits & 1)
            {
                SCRIPT_ID sid = s_asidUnicodeSubRangeScriptMapping[i];

                if (sid)
                {
                    sids |= ScriptBit(sid);
                }
            }

            dwUsbBits >>= 1;
        }
    }

    dwUsbBits = pfs->fsUsb[2];

    if (dwUsbBits)
    {
        // Hack for half-width Kana; Half-width Kana characters fall in
        // U+FFxx Halfwidth and Fullwidth Forms (Unicode Subrange #68),
        // but the subrange contains a mixture of Hangul/Alphanumeric/Kana
        // characters.  To work around this, we claim the font supports
        // half-width kana if it claims to support Unicode Subrange #68,
        // but not #56 (Hangul)

        if ( !fHangul && (dwUsbBits & 0x00000010) ) // USR#68 = 64 + 4
        {
            sids |= ScriptBit(sidHalfWidthKana);
        }

        for (i=64; i<UNICODE_SUBRANGES; i++)
        {
            if (dwUsbBits & 1)
            {
                SCRIPT_ID sid = s_asidUnicodeSubRangeScriptMapping[i];

                if (sid)
                {
                    sids |= ScriptBit(sid);
                }
            }

            dwUsbBits >>= 1;
        }
    }

    //
    // Do some additional tweaking
    //

    if (sids)
    {
        if (sids & ScriptBit(sidAsciiLatin))
        {
            // BUGBUG (cthrash) This is a hack.  We want to be able to
            // turn off, via CSS, this bit.  This will allow FE users
            // to pick a Latin font for their punctuation.  For now,
            // we just will basically never fontlink for sidAsciiSym
            // because virtually no font is lacking sidAsciiLatin
            // coverage.

            sids |= ScriptBit(sidAsciiSym);
        }

        if (sids & ScriptBit(sidLatin))
        {
            sids |= ScriptBit(sidDefault);
        }

        sids |= SCRIPT_BIT_CONST;
    }

    return sids;
}

//+----------------------------------------------------------------------------
//
//  Function:   ScriptIDsFromCharSet (inline)
//
//  Synopsis:   Data used by this inline function follows.
//
//-----------------------------------------------------------------------------

const SCRIPT_IDS s_sidsTable[] =
{
    #define SIDS_BASIC_LATIN 0
    ScriptBit(sidDefault) |
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidLatin) |
    SCRIPT_BIT_CONST,

    #define SIDS_CYRILLIC 1
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidCyrillic) |
    SCRIPT_BIT_CONST,

    #define SIDS_GREEK 2
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidGreek) |
    SCRIPT_BIT_CONST,

    #define SIDS_HEBREW 3
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidHebrew) |
    SCRIPT_BIT_CONST,

    #define SIDS_ARABIC 4
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidArabic) |
    SCRIPT_BIT_CONST,

    #define SIDS_THAI 5
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidThai) |
    SCRIPT_BIT_CONST,

    #define SIDS_JAPANESE 6
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidKana) |
    ScriptBit(sidHalfWidthKana) |
    ScriptBit(sidHan) |
    SCRIPT_BIT_CONST,

    #define SIDS_CHINESE 7
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidKana) |
    ScriptBit(sidBopomofo) |
    ScriptBit(sidHan) |
    SCRIPT_BIT_CONST,

    #define SIDS_HANGUL 8
    ScriptBit(sidAsciiLatin) |
    ScriptBit(sidAsciiSym) |
    ScriptBit(sidHangul) |
    ScriptBit(sidKana) |
    ScriptBit(sidHan) |
    SCRIPT_BIT_CONST,

    #define SIDS_ALL 9
    sidsAll
};

const BYTE s_abScriptIDsIndex[256] =
{
    SIDS_BASIC_LATIN,    //   0 ANSI_CHARSET
    SIDS_ALL,            //   1 DEFAULT_CHARSET
    SIDS_ALL,            //   2 SYMBOL_CHARSET
    SIDS_ALL,            //   3 
    SIDS_ALL,            //   4 
    SIDS_ALL,            //   5 
    SIDS_ALL,            //   6 
    SIDS_ALL,            //   7 
    SIDS_ALL,            //   8 
    SIDS_ALL,            //   9 
    SIDS_ALL,            //  10 
    SIDS_ALL,            //  11 
    SIDS_ALL,            //  12 
    SIDS_ALL,            //  13 
    SIDS_ALL,            //  14 
    SIDS_ALL,            //  15 
    SIDS_ALL,            //  16 
    SIDS_ALL,            //  17 
    SIDS_ALL,            //  18 
    SIDS_ALL,            //  19 
    SIDS_ALL,            //  20 
    SIDS_ALL,            //  21 
    SIDS_ALL,            //  22 
    SIDS_ALL,            //  23 
    SIDS_ALL,            //  24 
    SIDS_ALL,            //  25 
    SIDS_ALL,            //  26 
    SIDS_ALL,            //  27 
    SIDS_ALL,            //  28 
    SIDS_ALL,            //  29 
    SIDS_ALL,            //  30 
    SIDS_ALL,            //  31 
    SIDS_ALL,            //  32 
    SIDS_ALL,            //  33 
    SIDS_ALL,            //  34 
    SIDS_ALL,            //  35 
    SIDS_ALL,            //  36 
    SIDS_ALL,            //  37 
    SIDS_ALL,            //  38 
    SIDS_ALL,            //  39 
    SIDS_ALL,            //  40 
    SIDS_ALL,            //  41 
    SIDS_ALL,            //  42 
    SIDS_ALL,            //  43 
    SIDS_ALL,            //  44 
    SIDS_ALL,            //  45 
    SIDS_ALL,            //  46 
    SIDS_ALL,            //  47 
    SIDS_ALL,            //  48 
    SIDS_ALL,            //  49 
    SIDS_ALL,            //  50 
    SIDS_ALL,            //  51 
    SIDS_ALL,            //  52 
    SIDS_ALL,            //  53 
    SIDS_ALL,            //  54 
    SIDS_ALL,            //  55 
    SIDS_ALL,            //  56 
    SIDS_ALL,            //  57 
    SIDS_ALL,            //  58 
    SIDS_ALL,            //  59 
    SIDS_ALL,            //  60 
    SIDS_ALL,            //  61 
    SIDS_ALL,            //  62 
    SIDS_ALL,            //  63 
    SIDS_ALL,            //  64 
    SIDS_ALL,            //  65 
    SIDS_ALL,            //  66 
    SIDS_ALL,            //  67 
    SIDS_ALL,            //  68 
    SIDS_ALL,            //  69 
    SIDS_ALL,            //  70 
    SIDS_ALL,            //  71 
    SIDS_ALL,            //  72 
    SIDS_ALL,            //  73 
    SIDS_ALL,            //  74 
    SIDS_ALL,            //  75 
    SIDS_ALL,            //  76 
    SIDS_BASIC_LATIN,    //  77 MAC_CHARSET
    SIDS_ALL,            //  78 
    SIDS_ALL,            //  79 
    SIDS_ALL,            //  80 
    SIDS_ALL,            //  81 
    SIDS_ALL,            //  82 
    SIDS_ALL,            //  83 
    SIDS_ALL,            //  84 
    SIDS_ALL,            //  85 
    SIDS_ALL,            //  86 
    SIDS_ALL,            //  87 
    SIDS_ALL,            //  88 
    SIDS_ALL,            //  89 
    SIDS_ALL,            //  90 
    SIDS_ALL,            //  91 
    SIDS_ALL,            //  92 
    SIDS_ALL,            //  93 
    SIDS_ALL,            //  94 
    SIDS_ALL,            //  95 
    SIDS_ALL,            //  96 
    SIDS_ALL,            //  97 
    SIDS_ALL,            //  98 
    SIDS_ALL,            //  99 
    SIDS_ALL,            // 100 
    SIDS_ALL,            // 101 
    SIDS_ALL,            // 102 
    SIDS_ALL,            // 103 
    SIDS_ALL,            // 104 
    SIDS_ALL,            // 105 
    SIDS_ALL,            // 106 
    SIDS_ALL,            // 107 
    SIDS_ALL,            // 108 
    SIDS_ALL,            // 109 
    SIDS_ALL,            // 110 
    SIDS_ALL,            // 111 
    SIDS_ALL,            // 112 
    SIDS_ALL,            // 113 
    SIDS_ALL,            // 114 
    SIDS_ALL,            // 115 
    SIDS_ALL,            // 116 
    SIDS_ALL,            // 117 
    SIDS_ALL,            // 118 
    SIDS_ALL,            // 119 
    SIDS_ALL,            // 120 
    SIDS_ALL,            // 121 
    SIDS_ALL,            // 122 
    SIDS_ALL,            // 123 
    SIDS_ALL,            // 124 
    SIDS_ALL,            // 125 
    SIDS_ALL,            // 126 
    SIDS_ALL,            // 127 
    SIDS_JAPANESE,       // 128 SHIFTJIS_CHARSET
    SIDS_HANGUL,         // 129 HANGEUL_CHARSET
    SIDS_HANGUL,         // 130 JOHAB_CHARSET
    SIDS_ALL,            // 131 
    SIDS_ALL,            // 132 
    SIDS_ALL,            // 133 
    SIDS_CHINESE,        // 134 GB2312_CHARSET
    SIDS_ALL,            // 135 
    SIDS_CHINESE,        // 136 CHINESEBIG5_CHARSET
    SIDS_ALL,            // 137 
    SIDS_ALL,            // 138 
    SIDS_ALL,            // 139 
    SIDS_ALL,            // 140 
    SIDS_ALL,            // 141 
    SIDS_ALL,            // 142 
    SIDS_ALL,            // 143 
    SIDS_ALL,            // 144 
    SIDS_ALL,            // 145 
    SIDS_ALL,            // 146 
    SIDS_ALL,            // 147 
    SIDS_ALL,            // 148 
    SIDS_ALL,            // 149 
    SIDS_ALL,            // 150 
    SIDS_ALL,            // 151 
    SIDS_ALL,            // 152 
    SIDS_ALL,            // 153 
    SIDS_ALL,            // 154 
    SIDS_ALL,            // 155 
    SIDS_ALL,            // 156 
    SIDS_ALL,            // 157 
    SIDS_ALL,            // 158 
    SIDS_ALL,            // 159 
    SIDS_ALL,            // 160 
    SIDS_GREEK,          // 161 GREEK_CHARSET
    SIDS_BASIC_LATIN,    // 162 TURKISH_CHARSET
    SIDS_BASIC_LATIN,    // 163 VIETNAMESE_CHARSET
    SIDS_ALL,            // 164 
    SIDS_ALL,            // 165 
    SIDS_ALL,            // 166 
    SIDS_ALL,            // 167 
    SIDS_ALL,            // 168 
    SIDS_ALL,            // 169 
    SIDS_ALL,            // 170 
    SIDS_ALL,            // 171 
    SIDS_ALL,            // 172 
    SIDS_ALL,            // 173 
    SIDS_ALL,            // 174 
    SIDS_ALL,            // 175 
    SIDS_ALL,            // 176 
    SIDS_HEBREW,         // 177 HEBREW_CHARSET
    SIDS_ARABIC,         // 178 ARABIC_CHARSET
    SIDS_ALL,            // 179 
    SIDS_ALL,            // 180 
    SIDS_ALL,            // 181 
    SIDS_ALL,            // 182 
    SIDS_ALL,            // 183 
    SIDS_ALL,            // 184 
    SIDS_ALL,            // 185 
    SIDS_BASIC_LATIN,    // 186 BALTIC_CHARSET
    SIDS_ALL,            // 187 
    SIDS_ALL,            // 188 
    SIDS_ALL,            // 189 
    SIDS_ALL,            // 190 
    SIDS_ALL,            // 191 
    SIDS_ALL,            // 192 
    SIDS_ALL,            // 193 
    SIDS_ALL,            // 194 
    SIDS_ALL,            // 195 
    SIDS_ALL,            // 196 
    SIDS_ALL,            // 197 
    SIDS_ALL,            // 198 
    SIDS_ALL,            // 199 
    SIDS_ALL,            // 200 
    SIDS_ALL,            // 201 
    SIDS_ALL,            // 202 
    SIDS_ALL,            // 203 
    SIDS_CYRILLIC,       // 204 RUSSIAN_CHARSET
    SIDS_ALL,            // 205 
    SIDS_ALL,            // 206 
    SIDS_ALL,            // 207 
    SIDS_ALL,            // 208 
    SIDS_ALL,            // 209 
    SIDS_ALL,            // 210 
    SIDS_ALL,            // 211 
    SIDS_ALL,            // 212 
    SIDS_ALL,            // 213 
    SIDS_ALL,            // 214 
    SIDS_ALL,            // 215 
    SIDS_ALL,            // 216 
    SIDS_ALL,            // 217 
    SIDS_ALL,            // 218 
    SIDS_ALL,            // 219 
    SIDS_ALL,            // 220 
    SIDS_ALL,            // 221 
    SIDS_THAI,           // 222 THAI_CHARSET
    SIDS_ALL,            // 223 
    SIDS_ALL,            // 224 
    SIDS_ALL,            // 225 
    SIDS_ALL,            // 226 
    SIDS_ALL,            // 227 
    SIDS_ALL,            // 228 
    SIDS_ALL,            // 229 
    SIDS_ALL,            // 230 
    SIDS_ALL,            // 231 
    SIDS_ALL,            // 232 
    SIDS_ALL,            // 233 
    SIDS_ALL,            // 234 
    SIDS_ALL,            // 235 
    SIDS_ALL,            // 236 
    SIDS_ALL,            // 237 
    SIDS_BASIC_LATIN,    // 238 EASTEUROPE_CHARSET
    SIDS_ALL,            // 239 
    SIDS_ALL,            // 240 
    SIDS_ALL,            // 241 
    SIDS_ALL,            // 242 
    SIDS_ALL,            // 243 
    SIDS_ALL,            // 244 
    SIDS_ALL,            // 245 
    SIDS_ALL,            // 246 
    SIDS_ALL,            // 247 
    SIDS_ALL,            // 248 
    SIDS_ALL,            // 249 
    SIDS_ALL,            // 250 
    SIDS_ALL,            // 251 
    SIDS_ALL,            // 252 
    SIDS_ALL,            // 253 
    SIDS_ALL,            // 254 
    SIDS_ALL             // 255 OEM_CHARSET
};

//+----------------------------------------------------------------------------
//
//  Function:   ScriptIDsFromFaceName
//
//  Synopsis:   Compute the SCRIPT_IDS of a given font.  The SCRIPT_IDS is a
//              bitfield for the SCRIPT_ID enumerated type.  It is an
//              approximation of glyph coverage for the font.
//
//              Under NT, we utilize the NEWTEXTMETRICEX information that is
//              available for TrueType fonts.  If this information is not
//              available, we use the font charset coverage to estimate the
//              glyph coverage.  The charset method is essentially what IE4
//              did.
//
//
//              Note there's special code for MS Sans Serif -- this is a
//              really evil font - in spite of the fact that it contains
//              virtually none of the Latin-1 characters, it claims it supports
//              Latin-1.  Unfortunately, MS Sans Serif is an extremely common
//              font for use as DEFAULT_GUI_FONT.  This means that intrinsic
//              controls will often use MS Sans Serif.  By setting our
//              SCRIPT_IDS to sidAsciiLatin + sidAsciiSym, we make certain we
//              always font link for non-ASCII characters.
//
//-----------------------------------------------------------------------------

SCRIPT_IDS
ScriptIDsFromFont(
    HDC hdc,            // IN
    HFONT hfont,        // IN
    BOOL fTrueType )    // IN
{
    FONTSIGNATURE fs;
    BYTE uCharSet;
    HGDIOBJ hfontOld;
    SCRIPT_IDS sids;

    hfontOld = SelectObject( hdc, hfont );
    uCharSet = (BYTE)GetTextCharsetInfo( hdc, &fs, 0 );
    SelectObject( hdc, hfontOld );

    if (fTrueType)
    {
        sids = ScriptIDsFromFontSignature(&fs);

        // NOTE (cthrash) FE fonts will rarely cover enough of the Greek & Cyrillic
        // codepoints.  This hack basically forces us to fontlink for these.

        if (IsFECharset(uCharSet))
        {
            sids &= ~(ScriptBit(sidLatin) | ScriptBit(sidCyrillic) | ScriptBit(sidGreek));
        }
    }
    else
    {
        sids = ScriptIDsFromCharSet(uCharSet);
    }

    if (sids == sidsNotSet)
    {
        sids = sidsAll;
    }

    return sids;
}

//+----------------------------------------------------------------------------
//
//  Function:   UnUnifyHan, static
//
//  Synopsis:   Use a heuristic to best approximate the script actually
//              represented by sidHan.  This is necessary because of the
//              infamous Han-Unification brought upon us by the Unicode
//              consortium.
//
//              We prioritize the lcid if set.  This is set in HTML through
//              the use of the LANG attribute.  If this is not set, we
//              take the document codepage as reference.
//
//              The fallout case picks Japanese, as there is biggest market
//              share there today.
//
//  Returns:    Best guess script id.
//
//-----------------------------------------------------------------------------

SCRIPT_ID
UnUnifyHan(
    UINT uiFamilyCodePage,
    LCID lcid )
{
    SCRIPT_ID sid;

    if ( !lcid )
    {
        // No lang info.  Try codepages

        if (uiFamilyCodePage == CP_UCS_2 || uiFamilyCodePage == CP_UCS_2_BIGENDIAN)
        {
            // Do something here, possibly call MLANG.
        }

        if (uiFamilyCodePage == CP_CHN_GB)
        {
            lcid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);
        }
        else if (uiFamilyCodePage == CP_KOR_5601)
        {
            lcid = MAKELANGID(LANG_KOREAN, SUBLANG_NEUTRAL);
        }
        else if (uiFamilyCodePage == CP_TWN)
        {
            lcid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_TRADITIONAL);
        }
        else
        {
            lcid = MAKELANGID(LANG_JAPANESE, SUBLANG_NEUTRAL);
        }
    }

    LANGID lid = LANGIDFROMLCID(lcid);
    WORD plid = PRIMARYLANGID(lid);

    if (plid == LANG_CHINESE)
    {
        if (SUBLANGID(lid) == SUBLANG_CHINESE_TRADITIONAL)
        {
            sid = sidBopomofo;
        }
        else
        {
            sid = sidHan;
        }
    }
    else if (plid == LANG_KOREAN)
    {
        sid = sidHangul;
    }
    else
    {
        sid = sidKana;
    }

    return sid;
}

//+----------------------------------------------------------------------------
//
//  Function:   DefaultCodePageFromScript, static
//
//  Synopsis:   we return the best-guess default codepage based on the script
//              id passed.  It is a best-guess because scripts can cover
//              multiple codepages.
//
//  Returns:    Best-guess codepage for given information.
//              Also returns the ununified sid for sidHan.
//
//-----------------------------------------------------------------------------

CODEPAGE
DefaultCodePageFromScript(
    SCRIPT_ID * psid,   // IN/OUT
    CODEPAGE cpDoc,  // IN
    LCID lcid )         // IN
{
    AssertSz(psid, "Not an optional parameter.");
    AssertSz(cpDoc == WindowsCodePageFromCodePage(cpDoc),
             "Get an Internet codepage, expected a Windows codepage.");

    CODEPAGE cp;
    SCRIPT_ID sid = *psid;

    if (sid == sidMerge || sid == sidAmbiguous)
    {
        //
        // This is a hack -- the only time we should be called with sidMerge
        // is when the person asking about the font doesn't know what the sid
        // for the run is (e.g. treepos is non-text.)  In this event, we need
        // to pick a codepage which will give us the highest likelyhood of
        // being the correct one for ReExtTextOutW.
        //

        return cpDoc;
    }
    else if (cpDoc == CP_1250 && sid == sidDefault || sid == sidLatin)
    {
        // HACK (cthrash) CP_1250 (Eastern Europe) doesn't have it's own sid,
        // because its codepoints are covered by AsciiLatin, AsciiSym, and
        // Latin.  When printing, though, we may need to do a WC2MB (on a PCL
        // printer, for example) so we need the best approximation for the
        // actual codepage of the text.  In the simplest case, take the
        // document codepage over cp1252.

        cp = CP_1250;
    }
    else
    {
        // NB (cthrash) We assume the sidHan is the unified script for Han.
        // we use the usual heurisitics to pick amongst them.

        sid = (sid != sidHan) ? sid : UnUnifyHan( cpDoc, lcid );

        switch (sid)
        {
            default:            cp = CP_1252;       break;
            case sidGreek:      cp = CP_1253;       break;
            case sidCyrillic:   cp = CP_1251;       break;
            case sidHebrew:     cp = CP_1255;       break;
            case sidArabic:     cp = CP_1256;       break;
            case sidThai:       cp = CP_THAI;       break;
            case sidHangul:     cp = CP_KOR_5601;   break;
            case sidKana:       cp = CP_JPN_SJ;     break;
            case sidBopomofo:   cp = CP_TWN;        break;
            case sidHan:        cp = CP_CHN_GB;     break;
        }

        *psid = sid;
    }

    return cp;
}

//+----------------------------------------------------------------------------
//
//  Function:   DefaultCharSetFromScriptAndCharset/CodePage, static
//
//  Synopsis:   we return the best-guess default GDI charset based on the
//              script id passed.  We use the charformat charset is the tie-
//              breaker.
//
//              Note the sid should already been UnUnifyHan'd.
//
//  Returns:    Best-guess GDI charset for given information.
//
//-----------------------------------------------------------------------------

#define SPECIAL_CHARSET 3

static const BYTE s_abCharSetDefault[sidTridentLim] =
{
    DEFAULT_CHARSET,      // sidDefault (0)
    DEFAULT_CHARSET,      // sidMerge (1)
    ANSI_CHARSET,         // sidAsciiSym (2)
    SPECIAL_CHARSET,      // sidAsciiLatin (3)
    SPECIAL_CHARSET,      // sidLatin (4)
    GREEK_CHARSET,        // sidGreek (5)
    RUSSIAN_CHARSET,      // sidCyrillic (6)
    DEFAULT_CHARSET,      // sidArmenian (7)
    HEBREW_CHARSET,       // sidHebrew (8)
    ARABIC_CHARSET,       // sidArabic (9)
    DEFAULT_CHARSET,      // sidDevanagari (10)
    DEFAULT_CHARSET,      // sidBengali (11)
    DEFAULT_CHARSET,      // sidGurmukhi (12)
    DEFAULT_CHARSET,      // sidGujarati (13)
    DEFAULT_CHARSET,      // sidOriya (14)
    DEFAULT_CHARSET,      // sidTamil (15)
    DEFAULT_CHARSET,      // sidTelugu (16)
    DEFAULT_CHARSET,      // sidKannada (17)
    DEFAULT_CHARSET,      // sidMalayalam (18)
    THAI_CHARSET,         // sidThai (19)
    DEFAULT_CHARSET,      // sidLao (20)
    DEFAULT_CHARSET,      // sidTibetan (21)
    DEFAULT_CHARSET,      // sidGeorgian (22)
    HANGUL_CHARSET,       // sidHangul (23)
    SHIFTJIS_CHARSET,     // sidKana (24)
    CHINESEBIG5_CHARSET,  // sidBopomofo (25)
    GB2312_CHARSET,       // sidHan (26)
    DEFAULT_CHARSET,      // sidEthiopic (27)
    DEFAULT_CHARSET,      // sidCanSyllabic (28)
    DEFAULT_CHARSET,      // sidCherokee (29)
    DEFAULT_CHARSET,      // sidYi (30)
    DEFAULT_CHARSET,      // sidBraille (31)
    DEFAULT_CHARSET,      // sidRunic (32)
    DEFAULT_CHARSET,      // sidOgham (33)
    DEFAULT_CHARSET,      // sidSinhala (34)
    DEFAULT_CHARSET,      // sidSyriac (35)
    DEFAULT_CHARSET,      // sidBurmese (36)
    DEFAULT_CHARSET,      // sidKhmer (37)
    DEFAULT_CHARSET,      // sidThaana (38)
    DEFAULT_CHARSET,      // sidMongolian (39)
    DEFAULT_CHARSET,      // sidUserDefined (40)
    DEFAULT_CHARSET,      // sidSurrogateA (41)     *** Trident internal ***
    DEFAULT_CHARSET,      // sidSurrogateB (42)     *** Trident internal ***
    DEFAULT_CHARSET,      // sidAmbiguous (43)     *** Trident internal ***
    DEFAULT_CHARSET,      // sidEUDC (44)          *** Trident internal ***
    SHIFTJIS_CHARSET,     // sidHalfWidthKana (45) *** Trident internal ***
};

BYTE
DefaultCharSetFromScriptAndCharset(
    SCRIPT_ID sid,
    BYTE bCharSetCF )
{
    BYTE bCharSet = s_abCharSetDefault[sid];

    if (bCharSet == SPECIAL_CHARSET)
    {
        if (   sid == sidLatin
            && (   bCharSetCF == TURKISH_CHARSET
                || bCharSetCF == ANSI_CHARSET
                || bCharSetCF == VIETNAMESE_CHARSET
                || bCharSetCF == BALTIC_CHARSET
                || bCharSetCF == EASTEUROPE_CHARSET
               )
           )
        {
            bCharSet = bCharSetCF;
        }
        else if (sid == sidHangul)
        {
            bCharSet = bCharSetCF == JOHAB_CHARSET
                       ? JOHAB_CHARSET
                       : HANGUL_CHARSET;
        }
        else
        {
            bCharSet = DEFAULT_CHARSET;
        }
    }

    return bCharSet;
}

static const BYTE s_ab125xCharSets[] = 
{
    EASTEUROPE_CHARSET, // 1250
    RUSSIAN_CHARSET,    // 1251
    ANSI_CHARSET,       // 1252
    GREEK_CHARSET,      // 1253
    TURKISH_CHARSET,    // 1254
    HEBREW_CHARSET,     // 1255
    ARABIC_CHARSET,     // 1256
    BALTIC_CHARSET,     // 1257
    VIETNAMESE_CHARSET  // 1258
};

BYTE
DefaultCharSetFromScriptAndCodePage(
    SCRIPT_ID sid,
    UINT uiFamilyCodePage )
{
    BYTE bCharSet = s_abCharSetDefault[sid];

    if (bCharSet == SPECIAL_CHARSET)
    {
        bCharSet = (uiFamilyCodePage >= 1250 && uiFamilyCodePage <= 1258)
                   ? s_ab125xCharSets[uiFamilyCodePage - 1250]
                   : ANSI_CHARSET;
    }

    return bCharSet;
}

//+-----------------------------------------------------------------------------
//
//  Function:   DefaultSidForCodePage
//
//  Returns:    The default SCRIPT_ID for a codepage.  For stock codepages, we
//              cache the answer.  For the rest, we ask MLANG.
//
//------------------------------------------------------------------------------

SCRIPT_ID
DefaultSidForCodePage( UINT uiFamilyCodePage )
{
    SCRIPT_ID sid = sidLatin;
    
    switch (uiFamilyCodePage)
    {
        case CP_THAI: sid = sidThai; break;
        case CP_JPN_SJ: sid = sidKana; break;
        case CP_CHN_GB: sid = sidHan; break;
        case CP_KOR_5601: sid = sidHangul; break;
        case CP_TWN: sid = sidBopomofo; break;
        case CP_1250: sid = sidLatin; break;
        case CP_1251: sid = sidCyrillic; break;
        case CP_1252: sid = sidLatin; break;
        case CP_1253: sid = sidGreek; break;
        case CP_1254: sid = sidLatin; break;
        case CP_1255: sid = sidHebrew; break;
        case CP_1256: sid = sidArabic; break;
        case CP_1257: sid = sidLatin; break;
        default:
        {
            extern IMultiLanguage *g_pMultiLanguage;
            HRESULT hr;

            hr = THR( EnsureMultiLanguage() );

            if (OK(hr) && g_pMultiLanguage != NULL)
            {
                IMLangFontLink2 * pMLangFontLink2 = NULL;

                hr = THR(g_pMultiLanguage->QueryInterface( IID_IMLangFontLink2,
                                                           (void **)&pMLangFontLink2) );

                if (OK(hr))
                {
                    hr = THR( pMLangFontLink2->CodePageToScriptID( uiFamilyCodePage, &sid ));
                }

                ReleaseInterface( pMLangFontLink2 );
            }

            if (FAILED(hr))
            {
                sid = sidLatin;
            }
        }
        break;
    }

    return sid;
}

//+-----------------------------------------------------------------------------
//
//  Function:   RegistryAppropriateSidFromSid
//
//  Synopsis:   We have a reduced set of sid entries in the registry for
//              default font information.   This code makes sure you don't
//              try and lookup a sid which doesn't exist.
//
//------------------------------------------------------------------------------

SCRIPT_ID
RegistryAppropriateSidFromSid( SCRIPT_ID sid )
{
    // Make sure a bad sid isn't being passed in

#ifndef NO_UTF16
    Assert(   sid == sidHalfWidthKana
           || sid == sidSurrogateA
           || sid == sidSurrogateB
           || sid < sidLim );
#else
    Assert( sid == sidHalfWidthKana || sid < sidLim );
#endif

    // Make sure our sid order hasn't changed.

    Assert(   sidDefault < sidMerge
           && sidMerge < sidAsciiSym
           && sidAsciiSym < sidAsciiLatin
           && sidAsciiLatin	< sidLatin);

    return (sid <= sidLatin)
            ? sidAsciiLatin
            : (sid == sidHalfWidthKana)
              ? sidKana
              : sid;
}

//+----------------------------------------------------------------------------
//
//  Function:   DefaultScriptIDFromLang, static
//
//  Synopsis:   We return a default script ID based on the language id passed
//              in. There is usually a 1:1 mapping here, but there are
//              exceptions for FE langs. In these cases we try to pick a unique
//              script ID.
//
//  Returns:    Script ID matching the lang.
//
//-----------------------------------------------------------------------------

static const SCRIPT_ID s_asidScriptFromLang[] =
{
    sidDefault,     // LANG_NEUTRAL     0x00
    sidArabic,      // LANG_ARABIC      0x01
    sidCyrillic,    // LANG_BULGARIAN   0x02
    sidLatin,       // LANG_CATALAN     0x03
    sidHan,         // LANG_CHINESE     0x04
    sidLatin,       // LANG_CZECH       0x05
    sidLatin,       // LANG_DANISH      0x06
    sidLatin,       // LANG_GERMAN      0x07
    sidGreek,       // LANG_GREEK       0x08
    sidLatin,       // LANG_ENGLISH     0x09
    sidLatin,       // LANG_SPANISH     0x0a
    sidLatin,       // LANG_FINNISH     0x0b
    sidLatin,       // LANG_FRENCH      0x0c
    sidHebrew,      // LANG_HEBREW      0x0d
    sidLatin,       // LANG_HUNGARIAN   0x0e
    sidLatin,       // LANG_ICELANDIC   0x0f
    sidLatin,       // LANG_ITALIAN     0x10
    sidKana,        // LANG_JAPANESE    0x11
    sidHangul,      // LANG_KOREAN      0x12
    sidLatin,       // LANG_DUTCH       0x13
    sidLatin,       // LANG_NORWEGIAN   0x14
    sidLatin,       // LANG_POLISH      0x15
    sidLatin,       // LANG_PORTUGUESE  0x16
    sidDefault,     //                  0x17
    sidLatin,       // LANG_ROMANIAN    0x18
    sidCyrillic,    // LANG_RUSSIAN     0x19
    sidCyrillic,    // LANG_SERBIAN     0x1a
    sidLatin,       // LANG_SLOVAK      0x1b
    sidLatin,       // LANG_ALBANIAN    0x1c
    sidLatin,       // LANG_SWEDISH     0x1d
    sidThai,        // LANG_THAI        0x1e
    sidLatin,       // LANG_TURKISH     0x1f
    sidArabic,      // LANG_URDU        0x20
    sidLatin,       // LANG_INDONESIAN  0x21
    sidCyrillic,    // LANG_UKRAINIAN   0x22
    sidCyrillic,    // LANG_BELARUSIAN  0x23
    sidLatin,       // LANG_SLOVENIAN   0x24
    sidLatin,       // LANG_ESTONIAN    0x25
    sidLatin,       // LANG_LATVIAN     0x26
    sidLatin,       // LANG_LITHUANIAN  0x27
    sidDefault,     //                  0x28
    sidArabic,      // LANG_FARSI       0x29
    sidLatin,       // LANG_VIETNAMESE  0x2a
    sidArmenian,    // LANG_ARMENIAN    0x2b
    sidCyrillic,    // LANG_AZERI       0x2c
    sidLatin,       // LANG_BASQUE      0x2d
    sidDefault,     //                  0x2e
    sidCyrillic,    // LANG_MACEDONIAN  0x2f
    sidDefault,     //                  0x30
    sidDefault,     //                  0x31
    sidDefault,     //                  0x32
    sidDefault,     //                  0x33
    sidDefault,     //                  0x34
    sidDefault,     //                  0x35
    sidLatin,       // LANG_AFRIKAANS   0x36
    sidGeorgian,    // LANG_GEORGIAN    0x37
    sidLatin,       // LANG_FAEROESE    0x38
    sidDevanagari,  // LANG_HINDI       0x39
    sidDefault,     //                  0x3a
    sidDefault,     //                  0x3b
    sidDefault,     //                  0x3c
    sidDefault,     //                  0x3d
    sidLatin,       // LANG_MALAY       0x3e
    sidCyrillic,    // LANG_KAZAK       0x3f
    sidDefault,     //                  0x40
    sidLatin,       // LANG_SWAHILI     0x41
    sidDefault,     //                  0x42
    sidCyrillic,    // LANG_UZBEK       0x43
    sidCyrillic,    // LANG_TATAR       0x44
    sidBengali,     // LANG_BENGALI     0x45
    sidGurmukhi,    // LANG_PUNJABI     0x46
    sidGujarati,    // LANG_GUJARATI    0x47
    sidOriya,       // LANG_ORIYA       0x48
    sidTamil,       // LANG_TAMIL       0x49
    sidTelugu,      // LANG_TELUGU      0x4a
    sidKannada,     // LANG_KANNADA     0x4b
    sidMalayalam,   // LANG_MALAYALAM   0x4c
    sidBengali,     // LANG_ASSAMESE    0x4d
    sidDevanagari,  // LANG_MARATHI     0x4e
    sidDevanagari,  // LANG_SANSKRIT    0x4f
    sidDefault,     //                  0x50
    sidDefault,     //                  0x51
    sidDefault,     //                  0x52
    sidDefault,     //                  0x53
    sidDefault,     //                  0x54
    sidDefault,     //                  0x55
    sidDefault,     //                  0x56
    sidDevanagari,  // LANG_KONKANI     0x57
    sidBengali,     // LANG_MANIPURI    0x58
    sidArabic,      // LANG_SINDHI      0x59
    sidDefault,     //                  0x5a
    sidDefault,     //                  0x5b
    sidDefault,     //                  0x5c
    sidDefault,     //                  0x5d
    sidDefault,     //                  0x5e
    sidDefault,     //                  0x5f
    sidArabic,      // LANG_KASHMIRI    0x60
    sidDevanagari,  // LANG_NEPALI      0x61
};

SCRIPT_ID
DefaultScriptIDFromLang(
    LANGID lang)
{
    const size_t i = (size_t)PRIMARYLANGID(lang);

    if (i >= 0 && i < sizeof(s_asidScriptFromLang) / sizeof(SCRIPT_ID))
    {
        return s_asidScriptFromLang[i];
    }
    else
    {
        return sidDefault;
    }
}

#if DBG==1

const TCHAR * achSidNames[sidTridentLim] =
{
    _T("Default"),          // 0
    _T("Merge"),            // 1
    _T("AsciiSym"),         // 2
    _T("AsciiLatin"),       // 3
    _T("Latin"),            // 4
    _T("Greek"),            // 5
    _T("Cyrillic"),         // 6
    _T("Armenian"),         // 7
    _T("Hebrew"),           // 8
    _T("Arabic"),           // 9
    _T("Devanagari"),       // 10
    _T("Bengali"),          // 11
    _T("Gurmukhi"),         // 12
    _T("Gujarati"),         // 13
    _T("Oriya"),            // 14
    _T("Tamil"),            // 15
    _T("Telugu"),           // 16
    _T("Kannada"),          // 17
    _T("Malayalam"),        // 18
    _T("Thai"),             // 19
    _T("Lao"),              // 20
    _T("Tibetan"),          // 21
    _T("Georgian"),         // 22
    _T("Hangul"),           // 23
    _T("Kana"),             // 24
    _T("Bopomofo"),         // 25
    _T("Han"),              // 26
    _T("Ethiopic"),         // 27
    _T("CanSyllabic"),      // 28
    _T("Cherokee"),         // 29
    _T("Yi"),               // 30
    _T("Braille"),          // 31
    _T("Runic"),            // 32
    _T("Ogham"),            // 33
    _T("Sinhala"),          // 34
    _T("Syriac"),           // 35
    _T("Burmese"),          // 36
    _T("Khmer"),            // 37
    _T("Thaana"),           // 38
    _T("Mongolian"),        // 39
    _T("UserDefined"),      // 40
    _T("SurrogateA"),       // 41 *** Trident internal ***
    _T("SurrogateB"),       // 42 *** Trident internal ***
    _T("Ambiguous"),        // 43 *** Trident internal ***
    _T("EUDC"),             // 44 *** Trident internal ***
    _T("HalfWidthKana"),    // 45 *** Trident internal ***
};

//+----------------------------------------------------------------------------
//
//  Functions:  SidName
//
//  Returns:    Human-intelligible name for a script_id
//
//-----------------------------------------------------------------------------

const TCHAR *
SidName( SCRIPT_ID sid )
{
    return ( sid >= 0 && sid < sidTridentLim ) ? achSidNames[sid] : _T("#ERR");
}

//+----------------------------------------------------------------------------
//
//  Functions:  DumpSids
//
//  Synopsis:   Dump to the output window a human-intelligible list of names
//              of scripts coverted by the sids.
//
//  Returns:    Nada.
//
//-----------------------------------------------------------------------------

void
DumpSids( SCRIPT_IDS sids )
{
    SCRIPT_ID sid;

    for (sid=0; sid < sidTridentLim; sid++)
    {
        if (sids & ScriptBit(sid))
        {
            OutputDebugString(SidName(sid));
            OutputDebugString(_T("\r\n"));
        }
    }
}
#endif // DBG==1

