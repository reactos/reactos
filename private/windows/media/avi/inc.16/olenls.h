/***
*olenls.h - National language support functions.
*
*  Copyright (C) 1992, Microsoft Corporation.  All Rights Reserved.
*  Information Contained Herein Is Proprietary and Confidential.
*
*Purpose:
*  This describes the NLSAPI functions for Win16.  This is a subset
*  of Win32 NLSAPI, and is a non-Unicode version.
*
*Implementation Notes:
*  This files is largely ported from the Win32 header winnls.h.
*
*****************************************************************************/

#ifndef _OLENLS_
#define _OLENLS_

#ifndef NONLS

#ifdef __cplusplus
extern "C" {
#endif // __cpluscplus

/***************************************************************************\
* Constants
*
* Define all constants for the NLS component here.
\***************************************************************************/

/*
 *  Character Type Flags.
 */
#define CT_CTYPE1            0x00000001     /* ctype 1 information */
#define CT_CTYPE2            0x00000002     /* ctype 2 information */
#define CT_CTYPE3            0x00000004     /* ctype 3 information */

/*
 *  CType 1 Flag Bits.
 */
#define C1_UPPER             0x0001         /* upper case */
#define C1_LOWER             0x0002         /* lower case */
#define C1_DIGIT             0x0004         /* decimal digits */
#define C1_SPACE             0x0008         /* spacing characters */
#define C1_PUNCT             0x0010         /* punctuation characters */
#define C1_CNTRL             0x0020         /* control characters */
#define C1_BLANK             0x0040         /* blank characters */
#define C1_XDIGIT            0x0080         /* other digits */
#define C1_ALPHA             0x0100         /* any letter */

/*
 *  CType 2 Flag Bits.
 */
#define C2_LEFTTORIGHT       0x1            /* left to right */
#define C2_RIGHTTOLEFT       0x2            /* right to left */

#define C2_EUROPENUMBER      0x3            /* European number, digit */
#define C2_EUROPESEPARATOR   0x4            /* European numeric separator */
#define C2_EUROPETERMINATOR  0x5            /* European numeric terminator */
#define C2_ARABICNUMBER      0x6            /* Arabic number */
#define C2_COMMONSEPARATOR   0x7            /* common numeric separator */

#define C2_BLOCKSEPARATOR    0x8            /* block separator */
#define C2_SEGMENTSEPARATOR  0x9            /* segment separator */
#define C2_WHITESPACE        0xA            /* white space */
#define C2_OTHERNEUTRAL      0xB            /* other neutrals */

#define C2_NOTAPPLICABLE     0x0            /* no implicit directionality */

/*
 *  CType 3 Flag Bits.
 */
#define C3_NONSPACING        0x0001         /* nonspacing character */
#define C3_DIACRITIC         0x0002         /* diacritic mark */
#define C3_VOWELMARK         0x0004         /* vowel mark */
#define C3_SYMBOL            0x0008         /* symbols */

#define C3_NOTAPPLICABLE     0x0            /* ctype 3 is not applicable */


/*
 *  String Flags.
 */
#define NORM_IGNORECASE      0x00000001     /* ignore case */
#define NORM_IGNORENONSPACE  0x00000002     /* ignore nonspacing chars */
#define NORM_IGNORESYMBOLS   0x00000004     /* ignore symbols */


/*
 *  Locale Dependent Mapping Flags.
 */
#define LCMAP_LOWERCASE      0x00000100     /* lower case letters */
#define LCMAP_UPPERCASE      0x00000200     /* upper case letters */
#define LCMAP_SORTKEY        0x00000400     /* WC sort key (normalize) */



/*
 *  Language IDs.
 *
 *  The following two combinations of primary language ID and
 *  sublanguage ID have special semantics: 
 *
 *    Primary Language ID   Sublanguage ID      Result
 *    -------------------   ---------------     ------------------------
 *    LANG_NEUTRAL          SUBLANG_NEUTRAL     Language neutral
 *    LANG_NEUTRAL          SUBLANG_DEFAULT     Process default language
 */

/*
 *  Primary language IDs.
 */
#define LANG_NEUTRAL                     0x00

#define LANG_ALBANIAN                    0x1c
#define LANG_ARABIC                      0x01
#define LANG_BAHASA                      0x21
#define LANG_BULGARIAN                   0x02
#define LANG_CATALAN                     0x03
#define LANG_CHINESE                     0x04
#define LANG_CZECH                       0x05
#define LANG_DANISH                      0x06
#define LANG_DUTCH                       0x13
#define LANG_ENGLISH                     0x09
#define LANG_FINNISH                     0x0b
#define LANG_FRENCH                      0x0c
#define LANG_GERMAN                      0x07
#define LANG_GREEK                       0x08
#define LANG_HEBREW                      0x0d
#define LANG_HUNGARIAN                   0x0e
#define LANG_ICELANDIC                   0x0f
#define LANG_ITALIAN                     0x10
#define LANG_JAPANESE                    0x11
#define LANG_KOREAN                      0x12
#define LANG_NORWEGIAN                   0x14
#define LANG_POLISH                      0x15
#define LANG_PORTUGUESE                  0x16
#define LANG_RHAETO_ROMAN                0x17
#define LANG_ROMANIAN                    0x18
#define LANG_RUSSIAN                     0x19
#define LANG_SERBO_CROATIAN              0x1a
#define LANG_SLOVAK                      0x1b
#define LANG_SPANISH                     0x0a
#define LANG_SWEDISH                     0x1d
#define LANG_THAI                        0x1e
#define LANG_TURKISH                     0x1f
#define LANG_URDU                        0x20

/*
 *  Sublanguage IDs.
 *
 *  The name immediately following SUBLANG_ dictates which primary
 *  language ID that sublanguage ID can be combined with to form a
 *  valid language ID.
 */
#define SUBLANG_NEUTRAL                  0x00    /* language neutral */
#define SUBLANG_DEFAULT                  0x01    /* user default */
#define SUBLANG_SYS_DEFAULT              0x02    /* system default */

#define SUBLANG_CHINESE_SIMPLIFIED       0x02    /* Chinese (Simplified) */
#define SUBLANG_CHINESE_TRADITIONAL      0x01    /* Chinese (Traditional) */ 
#define SUBLANG_DUTCH                    0x01    /* Dutch */
#define SUBLANG_DUTCH_BELGIAN            0x02    /* Dutch (Belgian) */
#define SUBLANG_ENGLISH_US               0x01    /* English (USA) */
#define SUBLANG_ENGLISH_UK               0x02    /* English (UK) */
#define SUBLANG_ENGLISH_AUS              0x03    /* English (Australian) */
#define SUBLANG_ENGLISH_CAN              0x04    /* English (Canadian) */
#define SUBLANG_ENGLISH_NZ               0x05    /* English (New Zealand) */
#define SUBLANG_FRENCH                   0x01    /* French */
#define SUBLANG_FRENCH_BELGIAN           0x02    /* French (Belgian) */
#define SUBLANG_FRENCH_CANADIAN          0x03    /* French (Canadian) */
#define SUBLANG_FRENCH_SWISS             0x04    /* French (Swiss) */
#define SUBLANG_GERMAN                   0x01    /* German */
#define SUBLANG_GERMAN_SWISS             0x02    /* German (Swiss) */
#define SUBLANG_GERMAN_AUSTRIAN          0x03    /* German (Austrian) */
#define SUBLANG_ITALIAN                  0x01    /* Italian */
#define SUBLANG_ITALIAN_SWISS            0x02    /* Italian (Swiss) */
#define SUBLANG_NORWEGIAN_BOKMAL         0x01    /* Norwegian (Bokmal) */
#define SUBLANG_NORWEGIAN_NYNORSK        0x02    /* Norwegian (Nynorsk) */
#define SUBLANG_PORTUGUESE               0x02    /* Portuguese */
#define SUBLANG_PORTUGUESE_BRAZILIAN     0x01    /* Portuguese (Brazilian) */
#define SUBLANG_SERBO_CROATIAN_CYRILLIC  0x02    /* Serbo-Croatian (Cyrillic) */
#define SUBLANG_SERBO_CROATIAN_LATIN     0x01    /* Croato-Serbian (Latin) */
#define SUBLANG_SPANISH                  0x01    /* Spanish */
#define SUBLANG_SPANISH_MEXICAN          0x02    /* Spanish (Mexican) */
#define SUBLANG_SPANISH_MODERN           0x03    /* Spanish (Modern) */


/*
 *  Country Codes.
 */
#define CTRY_DEFAULT                     0

#define CTRY_AUSTRALIA                   61      /* Australia */
#define CTRY_AUSTRIA                     43      /* Austria */
#define CTRY_BELGIUM                     32      /* Belgium */
#define CTRY_BRAZIL                      55      /* Brazil */
#define CTRY_CANADA                      2       /* Canada */
#define CTRY_DENMARK                     45      /* Denmark */
#define CTRY_FINLAND                     358     /* Finland */
#define CTRY_FRANCE                      33      /* France */
#define CTRY_GERMANY                     49      /* Germany */
#define CTRY_ICELAND                     354     /* Iceland */
#define CTRY_IRELAND                     353     /* Ireland */
#define CTRY_ITALY                       39      /* Italy */
#define CTRY_JAPAN                       81      /* Japan */
#define CTRY_MEXICO                      52      /* Mexico */
#define CTRY_NETHERLANDS                 31      /* Netherlands */
#define CTRY_NEW_ZEALAND                 64      /* New Zealand */
#define CTRY_NORWAY                      47      /* Norway */
#define CTRY_PORTUGAL                    351     /* Portugal */
#define CTRY_PRCHINA                     86      /* PR China */
#define CTRY_SOUTH_KOREA                 82      /* South Korea */
#define CTRY_SPAIN                       34      /* Spain */
#define CTRY_SWEDEN                      46      /* Sweden */
#define CTRY_SWITZERLAND                 41      /* Switzerland */
#define CTRY_TAIWAN                      886     /* Taiwan */
#define CTRY_UNITED_KINGDOM              44      /* United Kingdom */
#define CTRY_UNITED_STATES               1       /* United States */


/*
 *  Locale Types.
 *
 *  These types are used for the GetLocaleInfoA NLS API routine.
 */
#define LOCALE_ILANGUAGE            0x0001    /* language id */
#define LOCALE_SLANGUAGE            0x0002    /* localized name of language */
#define LOCALE_SENGLANGUAGE         0x1001    /* English name of language */
#define LOCALE_SABBREVLANGNAME      0x0003    /* abbreviated language name */
#define LOCALE_SNATIVELANGNAME      0x0004    /* native name of language */
#define LOCALE_ICOUNTRY             0x0005    /* country code */
#define LOCALE_SCOUNTRY             0x0006    /* localized name of country */  
#define LOCALE_SENGCOUNTRY          0x1002    /* English name of country */  
#define LOCALE_SABBREVCTRYNAME      0x0007    /* abbreviated country name */
#define LOCALE_SNATIVECTRYNAME      0x0008    /* native name of country */  
#define LOCALE_IDEFAULTLANGUAGE     0x0009    /* default language id */
#define LOCALE_IDEFAULTCOUNTRY      0x000A    /* default country code */
#define LOCALE_IDEFAULTCODEPAGE     0x000B    /* default code page */
                                            
#define LOCALE_SLIST                0x000C    /* list item separator */
#define LOCALE_IMEASURE             0x000D    /* 0 = metric, 1 = US */
                                            
#define LOCALE_SDECIMAL             0x000E    /* decimal separator */
#define LOCALE_STHOUSAND            0x000F    /* thousand separator */
#define LOCALE_SGROUPING            0x0010    /* digit grouping */
#define LOCALE_IDIGITS              0x0011    /* number of fractional digits */
#define LOCALE_ILZERO               0x0012    /* leading zeros for decimal */
#define LOCALE_SNATIVEDIGITS        0x0013    /* native ascii 0-9 */
                                            
#define LOCALE_SCURRENCY            0x0014    /* local monetary symbol */
#define LOCALE_SINTLSYMBOL          0x0015    /* intl monetary symbol */
#define LOCALE_SMONDECIMALSEP       0x0016    /* monetary decimal separator */
#define LOCALE_SMONTHOUSANDSEP      0x0017    /* monetary thousand separator */
#define LOCALE_SMONGROUPING         0x0018    /* monetary grouping */
#define LOCALE_ICURRDIGITS          0x0019    /* # local monetary digits */
#define LOCALE_IINTLCURRDIGITS      0x001A    /* # intl monetary digits */
#define LOCALE_ICURRENCY            0x001B    /* positive currency mode */
#define LOCALE_INEGCURR             0x001C    /* negative currency mode */
                                            
#define LOCALE_SDATE                0x001D    /* date separator */
#define LOCALE_STIME                0x001E    /* time separator */
#define LOCALE_SSHORTDATE           0x001F    /* short date-time separator */
#define LOCALE_SLONGDATE            0x0020    /* long date-time separator */
#define LOCALE_IDATE                0x0021    /* short date format ordering */
#define LOCALE_ILDATE               0x0022    /* long date format ordering */
#define LOCALE_ITIME                0x0023    /* time format specifier */
#define LOCALE_ICENTURY             0x0024    /* century format specifier */
#define LOCALE_ITLZERO              0x0025    /* leading zeros in time field */
#define LOCALE_IDAYLZERO            0x0026    /* leading zeros in day field */
#define LOCALE_IMONLZERO            0x0027    /* leading zeros in month field */
#define LOCALE_S1159                0x0028    /* AM designator */
#define LOCALE_S2359                0x0029    /* PM designator */
                                            
#define LOCALE_SDAYNAME1            0x002A    /* long name for Monday */
#define LOCALE_SDAYNAME2            0x002B    /* long name for Tuesday */
#define LOCALE_SDAYNAME3            0x002C    /* long name for Wednesday */
#define LOCALE_SDAYNAME4            0x002D    /* long name for Thursday */
#define LOCALE_SDAYNAME5            0x002E    /* long name for Friday */
#define LOCALE_SDAYNAME6            0x002F    /* long name for Saturday */
#define LOCALE_SDAYNAME7            0x0030    /* long name for Sunday */
#define LOCALE_SABBREVDAYNAME1      0x0031    /* abbreviated name for Monday */   
#define LOCALE_SABBREVDAYNAME2      0x0032    /* abbreviated name for Tuesday */  
#define LOCALE_SABBREVDAYNAME3      0x0033    /* abbreviated name for Wednesday */
#define LOCALE_SABBREVDAYNAME4      0x0034    /* abbreviated name for Thursday */ 
#define LOCALE_SABBREVDAYNAME5      0x0035    /* abbreviated name for Friday */   
#define LOCALE_SABBREVDAYNAME6      0x0036    /* abbreviated name for Saturday */ 
#define LOCALE_SABBREVDAYNAME7      0x0037    /* abbreviated name for Sunday */   
#define LOCALE_SMONTHNAME1          0x0038    /* long name for January */
#define LOCALE_SMONTHNAME2          0x0039    /* long name for February */
#define LOCALE_SMONTHNAME3          0x003A    /* long name for March */
#define LOCALE_SMONTHNAME4          0x003B    /* long name for April */
#define LOCALE_SMONTHNAME5          0x003C    /* long name for May */
#define LOCALE_SMONTHNAME6          0x003D    /* long name for June */
#define LOCALE_SMONTHNAME7          0x003E    /* long name for July */
#define LOCALE_SMONTHNAME8          0x003F    /* long name for August */
#define LOCALE_SMONTHNAME9          0x0040    /* long name for September */
#define LOCALE_SMONTHNAME10         0x0041    /* long name for October */
#define LOCALE_SMONTHNAME11         0x0042    /* long name for November */
#define LOCALE_SMONTHNAME12         0x0043    /* long name for December */
#define LOCALE_SABBREVMONTHNAME1    0x0044    /* abbreviated name for January */
#define LOCALE_SABBREVMONTHNAME2    0x0045    /* abbreviated name for February */
#define LOCALE_SABBREVMONTHNAME3    0x0046    /* abbreviated name for March */
#define LOCALE_SABBREVMONTHNAME4    0x0047    /* abbreviated name for April */
#define LOCALE_SABBREVMONTHNAME5    0x0048    /* abbreviated name for May */
#define LOCALE_SABBREVMONTHNAME6    0x0049    /* abbreviated name for June */
#define LOCALE_SABBREVMONTHNAME7    0x004A    /* abbreviated name for July */
#define LOCALE_SABBREVMONTHNAME8    0x004B    /* abbreviated name for August */
#define LOCALE_SABBREVMONTHNAME9    0x004C    /* abbreviated name for September */
#define LOCALE_SABBREVMONTHNAME10   0x004D    /* abbreviated name for October */
#define LOCALE_SABBREVMONTHNAME11   0x004E    /* abbreviated name for November */
#define LOCALE_SABBREVMONTHNAME12   0x004F    /* abbreviated name for December */
                                            
#define LOCALE_SPOSITIVESIGN        0x0050    /* positive sign */
#define LOCALE_SNEGATIVESIGN        0x0051    /* negative sign */
#define LOCALE_IPOSSIGNPOSN         0x0052    /* positive sign position */
#define LOCALE_INEGSIGNPOSN         0x0053    /* negative sign position */
#define LOCALE_IPOSSYMPRECEDES      0x0054    /* mon sym precedes pos amt */
#define LOCALE_IPOSSEPBYSPACE       0x0055    /* mon sym sep by space from pos */ 
#define LOCALE_INEGSYMPRECEDES      0x0056    /* mon sym precedes neg amt */
#define LOCALE_INEGSEPBYSPACE       0x0057    /* mon sym sep by space from neg */
        
#define LOCALE_NOUSEROVERRIDE   0x80000000    /* OR in to avoid user override */
        
/***************************************************************************\
* Typedefs
*
* Define all types for the NLS component here.
\***************************************************************************/

/*
 *  IDs.
 */
typedef DWORD   LCID;                       /* locale ID */
typedef WORD    LANGID;                     /* language ID */
typedef DWORD   LCTYPE;                     /* locale type constant */

#define _LCID_DEFINED



/***************************************************************************\
* Macros
*
* Define all macros for the NLS component here.
\***************************************************************************/

/*
 *  A language ID is a 16 bit value which is the combination of a
 *  primary language ID and a secondary language ID.  The bits are
 *  allocated as follows:
 *
 *       +-----------------------+-------------------------+
 *       |      Sublanguage ID   |   Primary Language ID   |
 *       +-----------------------+-------------------------+
 *        15                   10 9                       0   bit
 *
 *
 *  Language ID creation/extraction macros:
 *
 *    MAKELANGID    - construct language id from primary language id and
 *                    sublanguage id.
 *    PRIMARYLANGID - extract primary language id from a language id.
 *    SUBLANGID     - extract sublanguage id from a language id.
 */
#define MAKELANGID(p, s)       ((((WORD)(s)) << 10) | (WORD)(p))
#define PRIMARYLANGID(lgid)    ((WORD)(lgid) & 0x3ff)
#define SUBLANGID(lgid)        ((WORD)(lgid) >> 10)


/*
 *  A locale ID is a 32 bit value which is the combination of a
 *  language ID and a reserved area.  The bits are allocated as follows:
 *
 *       +-----------------------+-------------------------+
 *       |       Reserved        |      Language ID        |
 *       +-----------------------+-------------------------+
 *        31                   16 15                      0   bit
 *
 *
 *  Locale ID creation macro:
 *
 *    MAKELCID - construct locale id from a language id.
 */
#define MAKELCID(lgid)  ((DWORD)(((WORD)(lgid)) | (((DWORD)((WORD)(0))) << 16)))


/*
 *  Get the language id from a locale id.
 */
#define LANGIDFROMLCID(lcid)   ((WORD)(lcid))


/*
 *  Default System and User IDs for language and locale.
 */
#define LANG_SYSTEM_DEFAULT    (MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT))
#define LANG_USER_DEFAULT      (MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))

#define LOCALE_SYSTEM_DEFAULT  (MAKELCID(LANG_SYSTEM_DEFAULT))
#define LOCALE_USER_DEFAULT    (MAKELCID(LANG_USER_DEFAULT))



/***************************************************************************\
* Function Prototypes
*
* Only prototypes for the NLS APIs should go here.
\***************************************************************************/


int    WINAPI  CompareStringA(LCID, DWORD, LPCSTR, int, LPCSTR, int);
int    WINAPI  LCMapStringA(LCID, DWORD, LPCSTR, int, LPSTR, int);
int    WINAPI  GetLocaleInfoA(LCID, LCTYPE, LPSTR, int);
BOOL   WINAPI  GetStringTypeA(LCID, DWORD, LPCSTR, int, LPWORD);

LANGID WINAPI  GetSystemDefaultLangID(void);
LANGID WINAPI  GetUserDefaultLangID(void);
LCID   WINAPI  GetSystemDefaultLCID(void);
LCID   WINAPI  GetUserDefaultLCID(void);

#ifdef __cplusplus
}
#endif // __cpluscplus


#endif   // NONLS

#endif   // _OLENLS_
