//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       intl.hxx
//
//  Contents:   Codepage definitions
//
//----------------------------------------------------------------------------

#ifndef I_CODEPAGE_H_
#define I_CODEPAGE_H_
#pragma INCMSG("--- Beg 'codepage.h'")

typedef UINT CODEPAGE;              // Codepage corresponds to Mlang ID

#define CP_UNDEFINED            CODEPAGE(-1)
#define CP_US_OEM               437
#define CP_ASMO_708             708   // Arabic DOS/ASMO 708 -- added for COMPLEXSCRIPT
#define CP_ASMO_720             720   // Arabic DOS/ASMO 720 -- added for COMPLEXSCRIPT
#define CP_HEB_862              862   // Hebrew DOS-862 -- added for COMPLEXSCRIPT
#define CP_THAI                 874   // DOS and Windows
#define CP_JPN_SJ               932   // ShiftJis
#define CP_CHN_GB               936   // GB2312
#define CP_KOR_5601             949
#define CP_TWN                  950   // Big5 Chinese
#define CP_UCS_2                1200  // Unicode, ISO 10646
#define CP_UCS_2_BIGENDIAN      1201  // Unicode
#define CP_1250                 1250  // Eastern Europe
#define CP_1251                 1251  // Russian, Cyrillic 
#define CP_1252                 1252  // Ansi, Western Europe
#define CP_1253                 1253  // Greek
#define CP_1254                 1254  // Turkish
#define CP_1255                 1255  // Hebrew
#define CP_1256                 1256  // Arabic
#define CP_1257                 1257  // Baltic
#define CP_1258                 1258  // Vietnamese

#define CP_KOI8R                20866

#define CP_ISO_8859_1           28591 // Latin 1
#define CP_ISO_8859_2           28592
#define CP_ISO_8859_3           28593
#define CP_ISO_8859_4           28594
#define CP_ISO_8859_5           28595 // Cyrillic
#define CP_ISO_8859_6           28596 // Arabic -- Logical Ordered
#define CP_ISO_8859_7           28597 // Greek
#define CP_ISO_8859_8           28598 // Hebrew -- ISO Visual Ordered
#define CP_ISO_8859_9           28599
#define CP_ISO_8859_8_I         38598 // Hebrew -- ISO Logical Ordered

#define CP_AUTO                 50001 // cross language detection

#define CP_ISO_2022_JP          50220
#define CP_ISO_2022_JP_ESC1     50221
#define CP_ISO_2022_JP_ESC2     50222
#define CP_ISO_2022_KR          50225
#define CP_ISO_2022_TW          50226
#define CP_ISO_2022_CH          50227

#define CP_AUTO_JP              50932

#define CP_EUC_JP               51932
#define CP_EUC_CH               51936
#define CP_EUC_KR               51949
#define CP_EUC_TW               51950

#define CP_CHN_HZ               55555 
#define CP_KOR_2022             55556

#define CP_UTF_7                65000
#define CP_UTF_8                65001

#define CP_UCS_4                12000  // Unicode
#define CP_UCS_4_BIGENDIAN      12001  // Unicode

#pragma INCMSG("--- End 'codepage.h'")
#else
#pragma INCMSG("*** Dup 'codepage.h'")
#endif
