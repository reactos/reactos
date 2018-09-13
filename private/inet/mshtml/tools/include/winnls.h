/**************************************************************************
*                                                                         *
* winnls.h -- NLS procedure declarations, constant definitions and macros *
*                                                                         *
* Copyright (c) 1991-1996, Microsoft Corp. All rights reserved.           *
*                                                                         *
**************************************************************************/


#ifndef _WINNLS_
#define _WINNLS_


#ifdef __cplusplus
extern "C" {
#endif

#ifndef NONLS


/***************************************************************************\
* Constants
*
* Define all constants for the NLS component here.
\***************************************************************************/


/*
 *  String Length Maximums.
 */
#define MAX_LEADBYTES        12             /* 5 ranges, 2 bytes ea., 0 term. */
#define MAX_DEFAULTCHAR      2              /* single or double byte */


/*
 *  MBCS and Unicode Translation Flags.
 */
#define MB_PRECOMPOSED       0x00000001     /* use precomposed chars */
#define MB_COMPOSITE         0x00000002     /* use composite chars */
#define MB_USEGLYPHCHARS     0x00000004     /* use glyph chars, not ctrl chars */
#define MB_ERR_INVALID_CHARS 0x00000008     /* error for invalid chars */

#define WC_DEFAULTCHECK      0x00000100     /* check for default char */
#define WC_COMPOSITECHECK    0x00000200     /* convert composite to precomposed */
#define WC_DISCARDNS         0x00000010     /* discard non-spacing chars */
#define WC_SEPCHARS          0x00000020     /* generate separate chars */
#define WC_DEFAULTCHAR       0x00000040     /* replace w/ default char */



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
#define C1_ALPHA             0x0100         /* any linguistic character */

/*
 *  CType 2 Flag Bits.
 */
#define C2_LEFTTORIGHT       0x0001         /* left to right */
#define C2_RIGHTTOLEFT       0x0002         /* right to left */

#define C2_EUROPENUMBER      0x0003         /* European number, digit */
#define C2_EUROPESEPARATOR   0x0004         /* European numeric separator */
#define C2_EUROPETERMINATOR  0x0005         /* European numeric terminator */
#define C2_ARABICNUMBER      0x0006         /* Arabic number */
#define C2_COMMONSEPARATOR   0x0007         /* common numeric separator */

#define C2_BLOCKSEPARATOR    0x0008         /* block separator */
#define C2_SEGMENTSEPARATOR  0x0009         /* segment separator */
#define C2_WHITESPACE        0x000A         /* white space */
#define C2_OTHERNEUTRAL      0x000B         /* other neutrals */

#define C2_NOTAPPLICABLE     0x0000         /* no implicit directionality */

/*
 *  CType 3 Flag Bits.
 */
#define C3_NONSPACING        0x0001         /* nonspacing character */
#define C3_DIACRITIC         0x0002         /* diacritic mark */
#define C3_VOWELMARK         0x0004         /* vowel mark */
#define C3_SYMBOL            0x0008         /* symbols */

#define C3_KATAKANA          0x0010         /* katakana character */
#define C3_HIRAGANA          0x0020         /* hiragana character */
#define C3_HALFWIDTH         0x0040         /* half width character */
#define C3_FULLWIDTH         0x0080         /* full width character */
#define C3_IDEOGRAPH         0x0100         /* ideographic character */
#define C3_KASHIDA           0x0200         /* Arabic kashida character */
#define C3_LEXICAL           0x0400         /* lexical character */

#define C3_ALPHA             0x8000         /* any linguistic char (C1_ALPHA) */

#define C3_NOTAPPLICABLE     0x0000         /* ctype 3 is not applicable */


/*
 *  String Flags.
 */
#define NORM_IGNORECASE           0x00000001  /* ignore case */
#define NORM_IGNORENONSPACE       0x00000002  /* ignore nonspacing chars */
#define NORM_IGNORESYMBOLS        0x00000004  /* ignore symbols */

#define NORM_IGNOREKANATYPE       0x00010000  /* ignore kanatype */
#define NORM_IGNOREWIDTH          0x00020000  /* ignore width */



/*
 *  Locale Independent Mapping Flags.
 */
#define MAP_FOLDCZONE             0x00000010  /* fold compatibility zone chars */
#define MAP_PRECOMPOSED           0x00000020  /* convert to precomposed chars */
#define MAP_COMPOSITE             0x00000040  /* convert to composite chars */
#define MAP_FOLDDIGITS            0x00000080  /* all digits to ASCII 0-9 */


/*
 *  Locale Dependent Mapping Flags.
 */
#define LCMAP_LOWERCASE           0x00000100  /* lower case letters */
#define LCMAP_UPPERCASE           0x00000200  /* upper case letters */
#define LCMAP_SORTKEY             0x00000400  /* WC sort key (normalize) */
#define LCMAP_BYTEREV             0x00000800  /* byte reversal */

#define LCMAP_HIRAGANA            0x00100000  /* map katakana to hiragana */
#define LCMAP_KATAKANA            0x00200000  /* map hiragana to katakana */
#define LCMAP_HALFWIDTH           0x00400000  /* map double byte to single byte */
#define LCMAP_FULLWIDTH           0x00800000  /* map single byte to double byte */

#define LCMAP_LINGUISTIC_CASING   0x01000000  /* use linguistic rules for casing */

#define LCMAP_SIMPLIFIED_CHINESE  0x02000000  /* map traditional chinese to simplified chinese */
#define LCMAP_TRADITIONAL_CHINESE 0x04000000  /* map simplified chinese to traditional chinese */



/*
 *  Locale Enumeration Flags.
 */
#define LCID_INSTALLED            0x00000001  /* installed locale ids */
#define LCID_SUPPORTED            0x00000002  /* supported locale ids */


/*
 *  Code Page Enumeration Flags.
 */
#define CP_INSTALLED              0x00000001  /* installed code page ids */
#define CP_SUPPORTED              0x00000002  /* supported code page ids */


/*
 *  Sorting Flags.
 *
 *    WORD Sort:    culturally correct sort
 *                  hyphen and apostrophe are special cased
 *                  example: "coop" and "co-op" will sort together in a list
 *
 *                        co_op     <-------  underscore (symbol)
 *                        coat
 *                        comb
 *                        coop
 *                        co-op     <-------  hyphen (punctuation)
 *                        cork
 *                        went
 *                        were
 *                        we're     <-------  apostrophe (punctuation)
 *
 *
 *    STRING Sort:  hyphen and apostrophe will sort with all other symbols
 *
 *                        co-op     <-------  hyphen (punctuation)
 *                        co_op     <-------  underscore (symbol)
 *                        coat
 *                        comb
 *                        coop
 *                        cork
 *                        we're     <-------  apostrophe (punctuation)
 *                        went
 *                        were
 */
#define SORT_STRINGSORT           0x00001000  /* use string sort method */


/*
 *  Code Page Default Values.
 */
#define CP_ACP               0              /* default to ANSI code page */
#define CP_OEMCP             1              /* default to OEM  code page */
#define CP_MACCP             2              /* default to MAC  code page */

#define CP_UTF7              65000          /* UTF-7 translation */
#define CP_UTF8              65001          /* UTF-8 translation */


/*
 *  Country Codes.
 */
#define CTRY_DEFAULT                     0

#define CTRY_AUSTRALIA                   61      /* Australia */
#define CTRY_AUSTRIA                     43      /* Austria */
#define CTRY_BELGIUM                     32      /* Belgium */
#define CTRY_BRAZIL                      55      /* Brazil */
#define CTRY_BULGARIA                    359     /* Bulgaria */
#define CTRY_CANADA                      2       /* Canada */
#define CTRY_CROATIA                     385     /* Croatia */
#define CTRY_CZECH                       42      /* Czech Republic */
#define CTRY_DENMARK                     45      /* Denmark */
#define CTRY_FINLAND                     358     /* Finland */
#define CTRY_FRANCE                      33      /* France */
#define CTRY_GERMANY                     49      /* Germany */
#define CTRY_GREECE                      30      /* Greece */
#define CTRY_HONG_KONG                   852     /* Hong Kong */
#define CTRY_HUNGARY                     36      /* Hungary */
#define CTRY_ICELAND                     354     /* Iceland */
#define CTRY_IRELAND                     353     /* Ireland */
#define CTRY_ITALY                       39      /* Italy */
#define CTRY_JAPAN                       81      /* Japan */
#define CTRY_MEXICO                      52      /* Mexico */
#define CTRY_NETHERLANDS                 31      /* Netherlands */
#define CTRY_NEW_ZEALAND                 64      /* New Zealand */
#define CTRY_NORWAY                      47      /* Norway */
#define CTRY_POLAND                      48      /* Poland */
#define CTRY_PORTUGAL                    351     /* Portugal */
#define CTRY_PRCHINA                     86      /* Peoples' Republic of China */
#define CTRY_ROMANIA                     40      /* Romania */
#define CTRY_RUSSIA                      7       /* Russia */
#define CTRY_SINGAPORE                   65      /* Singapore */
#define CTRY_SLOVAK                      42      /* Slovak Republic */
#define CTRY_SLOVENIA                    386     /* Slovenia */
#define CTRY_SOUTH_KOREA                 82      /* South Korea */
#define CTRY_SPAIN                       34      /* Spain */
#define CTRY_SWEDEN                      46      /* Sweden */
#define CTRY_SWITZERLAND                 41      /* Switzerland */
#define CTRY_TAIWAN                      886     /* Taiwan */
#define CTRY_TURKEY                      90      /* Turkey */
#define CTRY_UNITED_KINGDOM              44      /* United Kingdom */
#define CTRY_UNITED_STATES               1       /* United States */


/*
 *  Locale Types.
 *
 *  These types are used for the GetLocaleInfoW NLS API routine.
 *
 *  LOCALE_NOUSEROVERRIDE is also used in GetTimeFormatW and GetDateFormatW.
 *
 *  LOCALE_USE_CP_ACP is used in many of the A (Ansi) apis that need to do 
 *  string translation.
 */
#define LOCALE_NOUSEROVERRIDE       0x80000000   /* do not use user overrides */
#define LOCALE_USE_CP_ACP           0x40000000   /* use the system ACP */

#define LOCALE_ILANGUAGE            0x00000001   /* language id */
#define LOCALE_SLANGUAGE            0x00000002   /* localized name of language */
#define LOCALE_SENGLANGUAGE         0x00001001   /* English name of language */
#define LOCALE_SABBREVLANGNAME      0x00000003   /* abbreviated language name */
#define LOCALE_SNATIVELANGNAME      0x00000004   /* native name of language */

#define LOCALE_ICOUNTRY             0x00000005   /* country code */
#define LOCALE_SCOUNTRY             0x00000006   /* localized name of country */
#define LOCALE_SENGCOUNTRY          0x00001002   /* English name of country */
#define LOCALE_SABBREVCTRYNAME      0x00000007   /* abbreviated country name */
#define LOCALE_SNATIVECTRYNAME      0x00000008   /* native name of country */

#define LOCALE_IDEFAULTLANGUAGE     0x00000009   /* default language id */
#define LOCALE_IDEFAULTCOUNTRY      0x0000000A   /* default country code */
#define LOCALE_IDEFAULTCODEPAGE     0x0000000B   /* default oem code page */
#define LOCALE_IDEFAULTANSICODEPAGE 0x00001004   /* default ansi code page */
#define LOCALE_IDEFAULTMACCODEPAGE  0x00001011   /* default mac code page */

#define LOCALE_SLIST                0x0000000C   /* list item separator */
#define LOCALE_IMEASURE             0x0000000D   /* 0 = metric, 1 = US */

#define LOCALE_SDECIMAL             0x0000000E   /* decimal separator */
#define LOCALE_STHOUSAND            0x0000000F   /* thousand separator */
#define LOCALE_SGROUPING            0x00000010   /* digit grouping */
#define LOCALE_IDIGITS              0x00000011   /* number of fractional digits */
#define LOCALE_ILZERO               0x00000012   /* leading zeros for decimal */
#define LOCALE_INEGNUMBER           0x00001010   /* negative number mode */
#define LOCALE_SNATIVEDIGITS        0x00000013   /* native ascii 0-9 */

#define LOCALE_SCURRENCY            0x00000014   /* local monetary symbol */
#define LOCALE_SINTLSYMBOL          0x00000015   /* intl monetary symbol */
#define LOCALE_SMONDECIMALSEP       0x00000016   /* monetary decimal separator */
#define LOCALE_SMONTHOUSANDSEP      0x00000017   /* monetary thousand separator */
#define LOCALE_SMONGROUPING         0x00000018   /* monetary grouping */
#define LOCALE_ICURRDIGITS          0x00000019   /* # local monetary digits */
#define LOCALE_IINTLCURRDIGITS      0x0000001A   /* # intl monetary digits */
#define LOCALE_ICURRENCY            0x0000001B   /* positive currency mode */
#define LOCALE_INEGCURR             0x0000001C   /* negative currency mode */

#define LOCALE_SDATE                0x0000001D   /* date separator */
#define LOCALE_STIME                0x0000001E   /* time separator */
#define LOCALE_SSHORTDATE           0x0000001F   /* short date format string */
#define LOCALE_SLONGDATE            0x00000020   /* long date format string */
#define LOCALE_STIMEFORMAT          0x00001003   /* time format string */
#define LOCALE_IDATE                0x00000021   /* short date format ordering */
#define LOCALE_ILDATE               0x00000022   /* long date format ordering */
#define LOCALE_ITIME                0x00000023   /* time format specifier */
#define LOCALE_ITIMEMARKPOSN        0x00001005   /* time marker position */
#define LOCALE_ICENTURY             0x00000024   /* century format specifier (short date) */
#define LOCALE_ITLZERO              0x00000025   /* leading zeros in time field */
#define LOCALE_IDAYLZERO            0x00000026   /* leading zeros in day field (short date) */
#define LOCALE_IMONLZERO            0x00000027   /* leading zeros in month field (short date) */
#define LOCALE_S1159                0x00000028   /* AM designator */
#define LOCALE_S2359                0x00000029   /* PM designator */

#define LOCALE_ICALENDARTYPE        0x00001009   /* type of calendar specifier */
#define LOCALE_IOPTIONALCALENDAR    0x0000100B   /* additional calendar types specifier */
#define LOCALE_IFIRSTDAYOFWEEK      0x0000100C   /* first day of week specifier */
#define LOCALE_IFIRSTWEEKOFYEAR     0x0000100D   /* first week of year specifier */

#define LOCALE_SDAYNAME1            0x0000002A   /* long name for Monday */
#define LOCALE_SDAYNAME2            0x0000002B   /* long name for Tuesday */
#define LOCALE_SDAYNAME3            0x0000002C   /* long name for Wednesday */
#define LOCALE_SDAYNAME4            0x0000002D   /* long name for Thursday */
#define LOCALE_SDAYNAME5            0x0000002E   /* long name for Friday */
#define LOCALE_SDAYNAME6            0x0000002F   /* long name for Saturday */
#define LOCALE_SDAYNAME7            0x00000030   /* long name for Sunday */
#define LOCALE_SABBREVDAYNAME1      0x00000031   /* abbreviated name for Monday */
#define LOCALE_SABBREVDAYNAME2      0x00000032   /* abbreviated name for Tuesday */
#define LOCALE_SABBREVDAYNAME3      0x00000033   /* abbreviated name for Wednesday */
#define LOCALE_SABBREVDAYNAME4      0x00000034   /* abbreviated name for Thursday */
#define LOCALE_SABBREVDAYNAME5      0x00000035   /* abbreviated name for Friday */
#define LOCALE_SABBREVDAYNAME6      0x00000036   /* abbreviated name for Saturday */
#define LOCALE_SABBREVDAYNAME7      0x00000037   /* abbreviated name for Sunday */
#define LOCALE_SMONTHNAME1          0x00000038   /* long name for January */
#define LOCALE_SMONTHNAME2          0x00000039   /* long name for February */
#define LOCALE_SMONTHNAME3          0x0000003A   /* long name for March */
#define LOCALE_SMONTHNAME4          0x0000003B   /* long name for April */
#define LOCALE_SMONTHNAME5          0x0000003C   /* long name for May */
#define LOCALE_SMONTHNAME6          0x0000003D   /* long name for June */
#define LOCALE_SMONTHNAME7          0x0000003E   /* long name for July */
#define LOCALE_SMONTHNAME8          0x0000003F   /* long name for August */
#define LOCALE_SMONTHNAME9          0x00000040   /* long name for September */
#define LOCALE_SMONTHNAME10         0x00000041   /* long name for October */
#define LOCALE_SMONTHNAME11         0x00000042   /* long name for November */
#define LOCALE_SMONTHNAME12         0x00000043   /* long name for December */
#define LOCALE_SMONTHNAME13         0x0000100E   /* long name for 13th month (if exists) */
#define LOCALE_SABBREVMONTHNAME1    0x00000044   /* abbreviated name for January */
#define LOCALE_SABBREVMONTHNAME2    0x00000045   /* abbreviated name for February */
#define LOCALE_SABBREVMONTHNAME3    0x00000046   /* abbreviated name for March */
#define LOCALE_SABBREVMONTHNAME4    0x00000047   /* abbreviated name for April */
#define LOCALE_SABBREVMONTHNAME5    0x00000048   /* abbreviated name for May */
#define LOCALE_SABBREVMONTHNAME6    0x00000049   /* abbreviated name for June */
#define LOCALE_SABBREVMONTHNAME7    0x0000004A   /* abbreviated name for July */
#define LOCALE_SABBREVMONTHNAME8    0x0000004B   /* abbreviated name for August */
#define LOCALE_SABBREVMONTHNAME9    0x0000004C   /* abbreviated name for September */
#define LOCALE_SABBREVMONTHNAME10   0x0000004D   /* abbreviated name for October */
#define LOCALE_SABBREVMONTHNAME11   0x0000004E   /* abbreviated name for November */
#define LOCALE_SABBREVMONTHNAME12   0x0000004F   /* abbreviated name for December */
#define LOCALE_SABBREVMONTHNAME13   0x0000100F   /* abbreviated name for 13th month (if exists) */

#define LOCALE_SPOSITIVESIGN        0x00000050   /* positive sign */
#define LOCALE_SNEGATIVESIGN        0x00000051   /* negative sign */
#define LOCALE_IPOSSIGNPOSN         0x00000052   /* positive sign position */
#define LOCALE_INEGSIGNPOSN         0x00000053   /* negative sign position */
#define LOCALE_IPOSSYMPRECEDES      0x00000054   /* mon sym precedes pos amt */
#define LOCALE_IPOSSEPBYSPACE       0x00000055   /* mon sym sep by space from pos amt */
#define LOCALE_INEGSYMPRECEDES      0x00000056   /* mon sym precedes neg amt */
#define LOCALE_INEGSEPBYSPACE       0x00000057   /* mon sym sep by space from neg amt */

#if(WINVER >= 0x0400)
#define LOCALE_FONTSIGNATURE        0x00000058   /* font signature */
#define LOCALE_SISO639LANGNAME      0x00000059   /* ISO abbreviated language name */
#define LOCALE_SISO3166CTRYNAME     0x0000005A   /* ISO abbreviated country name */
#endif /* WINVER >= 0x0400 */

/*
 *  Time Flags for GetTimeFormatW.
 */
#define TIME_NOMINUTESORSECONDS    0x00000001  /* do not use minutes or seconds */
#define TIME_NOSECONDS             0x00000002  /* do not use seconds */
#define TIME_NOTIMEMARKER          0x00000004  /* do not use time marker */
#define TIME_FORCE24HOURFORMAT     0x00000008  /* always use 24 hour format */


/*
 *  Date Flags for GetDateFormatW.
 */
#define DATE_SHORTDATE             0x00000001  /* use short date picture */
#define DATE_LONGDATE              0x00000002  /* use long date picture */
#define DATE_USE_ALT_CALENDAR      0x00000004  /* use alternate calendar (if any) */


/*
 *  Calendar Types.
 *
 *  These types are used for the GetALTCalendarInfoW NLS API routine.
 */
#define CAL_ICALINTVALUE           0x00000001   /* calendar type */
#define CAL_SCALNAME               0x00000002   /* native name of calendar */
#define CAL_IYEAROFFSETRANGE       0x00000003   /* starting years of eras */
#define CAL_SERASTRING             0x00000004   /* era name for IYearOffsetRanges */
#define CAL_SSHORTDATE             0x00000005   /* short date format string */
#define CAL_SLONGDATE              0x00000006   /* long date format string */
#define CAL_SDAYNAME1              0x00000007   /* native name for Monday */
#define CAL_SDAYNAME2              0x00000008   /* native name for Tuesday */
#define CAL_SDAYNAME3              0x00000009   /* native name for Wednesday */
#define CAL_SDAYNAME4              0x0000000a   /* native name for Thursday */
#define CAL_SDAYNAME5              0x0000000b   /* native name for Friday */
#define CAL_SDAYNAME6              0x0000000c   /* native name for Saturday */
#define CAL_SDAYNAME7              0x0000000d   /* native name for Sunday */
#define CAL_SABBREVDAYNAME1        0x0000000e   /* abbreviated name for Monday */
#define CAL_SABBREVDAYNAME2        0x0000000f   /* abbreviated name for Tuesday */
#define CAL_SABBREVDAYNAME3        0x00000010   /* abbreviated name for Wednesday */
#define CAL_SABBREVDAYNAME4        0x00000011   /* abbreviated name for Thursday */
#define CAL_SABBREVDAYNAME5        0x00000012   /* abbreviated name for Friday */
#define CAL_SABBREVDAYNAME6        0x00000013   /* abbreviated name for Saturday */
#define CAL_SABBREVDAYNAME7        0x00000014   /* abbreviated name for Sunday */
#define CAL_SMONTHNAME1            0x00000015   /* native name for January */
#define CAL_SMONTHNAME2            0x00000016   /* native name for February */
#define CAL_SMONTHNAME3            0x00000017   /* native name for March */
#define CAL_SMONTHNAME4            0x00000018   /* native name for April */
#define CAL_SMONTHNAME5            0x00000019   /* native name for May */
#define CAL_SMONTHNAME6            0x0000001a   /* native name for June */
#define CAL_SMONTHNAME7            0x0000001b   /* native name for July */
#define CAL_SMONTHNAME8            0x0000001c   /* native name for August */
#define CAL_SMONTHNAME9            0x0000001d   /* native name for September */
#define CAL_SMONTHNAME10           0x0000001e   /* native name for October */
#define CAL_SMONTHNAME11           0x0000001f   /* native name for November */
#define CAL_SMONTHNAME12           0x00000020   /* native name for December */
#define CAL_SMONTHNAME13           0x00000021   /* native name for 13th month (if any) */
#define CAL_SABBREVMONTHNAME1      0x00000022   /* abbreviated name for January */
#define CAL_SABBREVMONTHNAME2      0x00000023   /* abbreviated name for February */
#define CAL_SABBREVMONTHNAME3      0x00000024   /* abbreviated name for March */
#define CAL_SABBREVMONTHNAME4      0x00000025   /* abbreviated name for April */
#define CAL_SABBREVMONTHNAME5      0x00000026   /* abbreviated name for May */
#define CAL_SABBREVMONTHNAME6      0x00000027   /* abbreviated name for June */
#define CAL_SABBREVMONTHNAME7      0x00000028   /* abbreviated name for July */
#define CAL_SABBREVMONTHNAME8      0x00000029   /* abbreviated name for August */
#define CAL_SABBREVMONTHNAME9      0x0000002a   /* abbreviated name for September */
#define CAL_SABBREVMONTHNAME10     0x0000002b   /* abbreviated name for October */
#define CAL_SABBREVMONTHNAME11     0x0000002c   /* abbreviated name for November */
#define CAL_SABBREVMONTHNAME12     0x0000002d   /* abbreviated name for December */
#define CAL_SABBREVMONTHNAME13     0x0000002e   /* abbreviated name for 13th month (if any) */


/*
 *  Calendar Enumeration Value.
 */
#define ENUM_ALL_CALENDARS         0xffffffff   /* enumerate all calendars */


/*
 *  Calendar ID Values.
 */
#define CAL_GREGORIAN        1         /* Gregorian (localized) calendar */
#define CAL_GREGORIAN_US     2         /* Gregorian (U.S.) calendar */
#define CAL_JAPAN            3         /* Japanese Emperor Era calendar */
#define CAL_TAIWAN           4         /* Republic of China Era calendar */
#define CAL_KOREA            5         /* Korean Tangun Era calendar */
#define CAL_HIJRI            6         /* Hijri (Arabic Lunar) calendar */
#define CAL_THAI             7         /* Thai calendar */
#define CAL_HEBREW           8         /* Hebrew calendar */




/***************************************************************************\
* Typedefs
*
* Define all types for the NLS component here.
\***************************************************************************/

/*
 *  Locale type constant.
 */
typedef DWORD LCTYPE;

/*
 *  Calendar type constant.
 */
typedef DWORD CALTYPE;

/*
 *  Calendar ID.
 */
typedef DWORD CALID;


/*
 *  CP Info.
 */

typedef struct _cpinfo {
    UINT    MaxCharSize;                    /* max length (bytes) of a char */
    BYTE    DefaultChar[MAX_DEFAULTCHAR];   /* default character */
    BYTE    LeadByte[MAX_LEADBYTES];        /* lead byte ranges */
} CPINFO, *LPCPINFO;


/*
 *  Number format.
 */

typedef struct _numberfmtA {
    UINT    NumDigits;                 /* number of decimal digits */
    UINT    LeadingZero;               /* if leading zero in decimal fields */
    UINT    Grouping;                  /* group size left of decimal */
    LPSTR   lpDecimalSep;              /* ptr to decimal separator string */
    LPSTR   lpThousandSep;             /* ptr to thousand separator string */
    UINT    NegativeOrder;             /* negative number ordering */
} NUMBERFMTA, *LPNUMBERFMTA;
typedef struct _numberfmtW {
    UINT    NumDigits;                 /* number of decimal digits */
    UINT    LeadingZero;               /* if leading zero in decimal fields */
    UINT    Grouping;                  /* group size left of decimal */
    LPWSTR  lpDecimalSep;              /* ptr to decimal separator string */
    LPWSTR  lpThousandSep;             /* ptr to thousand separator string */
    UINT    NegativeOrder;             /* negative number ordering */
} NUMBERFMTW, *LPNUMBERFMTW;
#ifdef UNICODE
typedef NUMBERFMTW NUMBERFMT;
typedef LPNUMBERFMTW LPNUMBERFMT;
#else
typedef NUMBERFMTA NUMBERFMT;
typedef LPNUMBERFMTA LPNUMBERFMT;
#endif // UNICODE


/*
 *  Currency format.
 */

typedef struct _currencyfmtA {
    UINT    NumDigits;                 /* number of decimal digits */
    UINT    LeadingZero;               /* if leading zero in decimal fields */
    UINT    Grouping;                  /* group size left of decimal */
    LPSTR   lpDecimalSep;              /* ptr to decimal separator string */
    LPSTR   lpThousandSep;             /* ptr to thousand separator string */
    UINT    NegativeOrder;             /* negative currency ordering */
    UINT    PositiveOrder;             /* positive currency ordering */
    LPSTR   lpCurrencySymbol;          /* ptr to currency symbol string */
} CURRENCYFMTA, *LPCURRENCYFMTA;
typedef struct _currencyfmtW {
    UINT    NumDigits;                 /* number of decimal digits */
    UINT    LeadingZero;               /* if leading zero in decimal fields */
    UINT    Grouping;                  /* group size left of decimal */
    LPWSTR  lpDecimalSep;              /* ptr to decimal separator string */
    LPWSTR  lpThousandSep;             /* ptr to thousand separator string */
    UINT    NegativeOrder;             /* negative currency ordering */
    UINT    PositiveOrder;             /* positive currency ordering */
    LPWSTR  lpCurrencySymbol;          /* ptr to currency symbol string */
} CURRENCYFMTW, *LPCURRENCYFMTW;
#ifdef UNICODE
typedef CURRENCYFMTW CURRENCYFMT;
typedef LPCURRENCYFMTW LPCURRENCYFMT;
#else
typedef CURRENCYFMTA CURRENCYFMT;
typedef LPCURRENCYFMTA LPCURRENCYFMT;
#endif // UNICODE



/*
 *  Enumeration function constants.
 */

#ifdef STRICT

typedef BOOL (CALLBACK* LOCALE_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK* CODEPAGE_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK* DATEFMT_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK* TIMEFMT_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK* CALINFO_ENUMPROCA)(LPSTR);

typedef BOOL (CALLBACK* LOCALE_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK* CODEPAGE_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK* DATEFMT_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK* TIMEFMT_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK* CALINFO_ENUMPROCW)(LPWSTR);

#else /* !STRICT */

typedef FARPROC LOCALE_ENUMPROCA;
typedef FARPROC CODEPAGE_ENUMPROCA;
typedef FARPROC DATEFMT_ENUMPROCA;
typedef FARPROC TIMEFMT_ENUMPROCA;
typedef FARPROC CALINFO_ENUMPROCA;

typedef FARPROC LOCALE_ENUMPROCW;
typedef FARPROC CODEPAGE_ENUMPROCW;
typedef FARPROC DATEFMT_ENUMPROCW;
typedef FARPROC TIMEFMT_ENUMPROCW;
typedef FARPROC CALINFO_ENUMPROCW;

#endif /* !STRICT */

#ifdef UNICODE

#define LOCALE_ENUMPROC    LOCALE_ENUMPROCW
#define CODEPAGE_ENUMPROC  CODEPAGE_ENUMPROCW
#define DATEFMT_ENUMPROC   DATEFMT_ENUMPROCW
#define TIMEFMT_ENUMPROC   TIMEFMT_ENUMPROCW
#define CALINFO_ENUMPROC   CALINFO_ENUMPROCW

#else

#define LOCALE_ENUMPROC    LOCALE_ENUMPROCA
#define CODEPAGE_ENUMPROC  CODEPAGE_ENUMPROCA
#define DATEFMT_ENUMPROC   DATEFMT_ENUMPROCA
#define TIMEFMT_ENUMPROC   TIMEFMT_ENUMPROCA
#define CALINFO_ENUMPROC   CALINFO_ENUMPROCA

#endif // !UNICODE



/***************************************************************************\
* Macros
*
* Define all macros for the NLS component here.
\***************************************************************************/



/***************************************************************************\
* Function Prototypes
*
* Only prototypes for the NLS APIs should go here.
\***************************************************************************/

/*
 *  Code Page Dependent APIs.
 */

WINBASEAPI
BOOL
WINAPI
IsValidCodePage(
    UINT  CodePage);

WINBASEAPI
UINT
WINAPI
GetACP(void);

WINBASEAPI
UINT
WINAPI
GetOEMCP(void);

WINBASEAPI
BOOL
WINAPI
GetCPInfo(
    UINT      CodePage,
    LPCPINFO  lpCPInfo);

WINBASEAPI
BOOL
WINAPI
IsDBCSLeadByte(
    BYTE  TestChar);

WINBASEAPI
BOOL
WINAPI
IsDBCSLeadByteEx(
    UINT  CodePage,
    BYTE  TestChar);

WINBASEAPI
int
WINAPI
MultiByteToWideChar(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCSTR   lpMultiByteStr,
    int      cchMultiByte,
    LPWSTR   lpWideCharStr,
    int      cchWideChar);

WINBASEAPI
int
WINAPI
WideCharToMultiByte(
    UINT     CodePage,
    DWORD    dwFlags,
    LPCWSTR  lpWideCharStr,
    int      cchWideChar,
    LPSTR    lpMultiByteStr,
    int      cchMultiByte,
    LPCSTR   lpDefaultChar,
    LPBOOL   lpUsedDefaultChar);


/*
 *  Locale Dependent APIs.
 */

WINBASEAPI
int
WINAPI
CompareStringA(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCSTR lpString1,
    int      cchCount1,
    LPCSTR lpString2,
    int      cchCount2);
WINBASEAPI
int
WINAPI
CompareStringW(
    LCID     Locale,
    DWORD    dwCmpFlags,
    LPCWSTR lpString1,
    int      cchCount1,
    LPCWSTR lpString2,
    int      cchCount2);
#ifdef UNICODE
#define CompareString  CompareStringW
#else
#define CompareString  CompareStringA
#endif // !UNICODE

WINBASEAPI
int
WINAPI
LCMapStringA(
    LCID     Locale,
    DWORD    dwMapFlags,
    LPCSTR lpSrcStr,
    int      cchSrc,
    LPSTR  lpDestStr,
    int      cchDest);
WINBASEAPI
int
WINAPI
LCMapStringW(
    LCID     Locale,
    DWORD    dwMapFlags,
    LPCWSTR lpSrcStr,
    int      cchSrc,
    LPWSTR  lpDestStr,
    int      cchDest);
#ifdef UNICODE
#define LCMapString  LCMapStringW
#else
#define LCMapString  LCMapStringA
#endif // !UNICODE

WINBASEAPI
int
WINAPI
GetLocaleInfoA(
    LCID     Locale,
    LCTYPE   LCType,
    LPSTR  lpLCData,
    int      cchData);
WINBASEAPI
int
WINAPI
GetLocaleInfoW(
    LCID     Locale,
    LCTYPE   LCType,
    LPWSTR  lpLCData,
    int      cchData);
#ifdef UNICODE
#define GetLocaleInfo  GetLocaleInfoW
#else
#define GetLocaleInfo  GetLocaleInfoA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
SetLocaleInfoA(
    LCID     Locale,
    LCTYPE   LCType,
    LPCSTR lpLCData);
WINBASEAPI
BOOL
WINAPI
SetLocaleInfoW(
    LCID     Locale,
    LCTYPE   LCType,
    LPCWSTR lpLCData);
#ifdef UNICODE
#define SetLocaleInfo  SetLocaleInfoW
#else
#define SetLocaleInfo  SetLocaleInfoA
#endif // !UNICODE

WINBASEAPI
int
WINAPI
GetTimeFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCSTR lpFormat,
    LPSTR  lpTimeStr,
    int      cchTime);
WINBASEAPI
int
WINAPI
GetTimeFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCWSTR lpFormat,
    LPWSTR  lpTimeStr,
    int      cchTime);
#ifdef UNICODE
#define GetTimeFormat  GetTimeFormatW
#else
#define GetTimeFormat  GetTimeFormatA
#endif // !UNICODE

WINBASEAPI
int
WINAPI
GetDateFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCSTR lpFormat,
    LPSTR  lpDateStr,
    int      cchDate);
WINBASEAPI
int
WINAPI
GetDateFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCWSTR lpFormat,
    LPWSTR  lpDateStr,
    int      cchDate);
#ifdef UNICODE
#define GetDateFormat  GetDateFormatW
#else
#define GetDateFormat  GetDateFormatA
#endif // !UNICODE

WINBASEAPI
int
WINAPI
GetNumberFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    LPCSTR lpValue,
    CONST NUMBERFMTA *lpFormat,
    LPSTR  lpNumberStr,
    int      cchNumber);
WINBASEAPI
int
WINAPI
GetNumberFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    LPCWSTR lpValue,
    CONST NUMBERFMTW *lpFormat,
    LPWSTR  lpNumberStr,
    int      cchNumber);
#ifdef UNICODE
#define GetNumberFormat  GetNumberFormatW
#else
#define GetNumberFormat  GetNumberFormatA
#endif // !UNICODE

WINBASEAPI
int
WINAPI
GetCurrencyFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    LPCSTR lpValue,
    CONST CURRENCYFMTA *lpFormat,
    LPSTR  lpCurrencyStr,
    int      cchCurrency);
WINBASEAPI
int
WINAPI
GetCurrencyFormatW(
    LCID     Locale,
    DWORD    dwFlags,
    LPCWSTR lpValue,
    CONST CURRENCYFMTW *lpFormat,
    LPWSTR  lpCurrencyStr,
    int      cchCurrency);
#ifdef UNICODE
#define GetCurrencyFormat  GetCurrencyFormatW
#else
#define GetCurrencyFormat  GetCurrencyFormatA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EnumCalendarInfoA(
    CALINFO_ENUMPROCA lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType);
WINBASEAPI
BOOL
WINAPI
EnumCalendarInfoW(
    CALINFO_ENUMPROCW lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType);
#ifdef UNICODE
#define EnumCalendarInfo  EnumCalendarInfoW
#else
#define EnumCalendarInfo  EnumCalendarInfoA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EnumTimeFormatsA(
    TIMEFMT_ENUMPROCA lpTimeFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);
WINBASEAPI
BOOL
WINAPI
EnumTimeFormatsW(
    TIMEFMT_ENUMPROCW lpTimeFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);
#ifdef UNICODE
#define EnumTimeFormats  EnumTimeFormatsW
#else
#define EnumTimeFormats  EnumTimeFormatsA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EnumDateFormatsA(
    DATEFMT_ENUMPROCA lpDateFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);
WINBASEAPI
BOOL
WINAPI
EnumDateFormatsW(
    DATEFMT_ENUMPROCW lpDateFmtEnumProc,
    LCID              Locale,
    DWORD             dwFlags);
#ifdef UNICODE
#define EnumDateFormats  EnumDateFormatsW
#else
#define EnumDateFormats  EnumDateFormatsA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
IsValidLocale(
    LCID   Locale,
    DWORD  dwFlags);

WINBASEAPI
LCID
WINAPI
ConvertDefaultLocale(
    LCID   Locale);

WINBASEAPI
LCID
WINAPI
GetThreadLocale(void);

WINBASEAPI
BOOL
WINAPI
SetThreadLocale(
    LCID  Locale
    );

WINBASEAPI
LANGID
WINAPI
GetSystemDefaultLangID(void);

WINBASEAPI
LANGID
WINAPI
GetUserDefaultLangID(void);

WINBASEAPI
LCID
WINAPI
GetSystemDefaultLCID(void);

WINBASEAPI
LCID
WINAPI
GetUserDefaultLCID(void);

/*
 *  Locale Independent APIs.
 */

WINBASEAPI
BOOL
WINAPI
GetStringTypeExA(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCSTR lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);
WINBASEAPI
BOOL
WINAPI
GetStringTypeExW(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCWSTR lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);
#ifdef UNICODE
#define GetStringTypeEx  GetStringTypeExW
#else
#define GetStringTypeEx  GetStringTypeExA
#endif // !UNICODE

/*
 *  NOTE: The parameters for GetStringTypeA and GetStringTypeW are
 *        NOT the same.  The W version was shipped in NT 3.1.  The
 *        A version was then shipped in 16-bit OLE with the wrong
 *        parameters (ported from Win95).  To be compatible, we
 *        must break the relationship between the A and W versions
 *        of GetStringType.  There will be NO function call for the
 *        generic GetStringType.
 *
 *        GetStringTypeEx (above) should be used instead.
 */
WINBASEAPI
BOOL
WINAPI
GetStringTypeA(
    LCID     Locale,
    DWORD    dwInfoType,
    LPCSTR   lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);

WINBASEAPI
BOOL
WINAPI
GetStringTypeW(
    DWORD    dwInfoType,
    LPCWSTR  lpSrcStr,
    int      cchSrc,
    LPWORD   lpCharType);


WINBASEAPI
int
WINAPI
FoldStringA(
    DWORD    dwMapFlags,
    LPCSTR lpSrcStr,
    int      cchSrc,
    LPSTR  lpDestStr,
    int      cchDest);
WINBASEAPI
int
WINAPI
FoldStringW(
    DWORD    dwMapFlags,
    LPCWSTR lpSrcStr,
    int      cchSrc,
    LPWSTR  lpDestStr,
    int      cchDest);
#ifdef UNICODE
#define FoldString  FoldStringW
#else
#define FoldString  FoldStringA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EnumSystemLocalesA(
    LOCALE_ENUMPROCA lpLocaleEnumProc,
    DWORD            dwFlags);
WINBASEAPI
BOOL
WINAPI
EnumSystemLocalesW(
    LOCALE_ENUMPROCW lpLocaleEnumProc,
    DWORD            dwFlags);
#ifdef UNICODE
#define EnumSystemLocales  EnumSystemLocalesW
#else
#define EnumSystemLocales  EnumSystemLocalesA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
EnumSystemCodePagesA(
    CODEPAGE_ENUMPROCA lpCodePageEnumProc,
    DWORD              dwFlags);
WINBASEAPI
BOOL
WINAPI
EnumSystemCodePagesW(
    CODEPAGE_ENUMPROCW lpCodePageEnumProc,
    DWORD              dwFlags);
#ifdef UNICODE
#define EnumSystemCodePages  EnumSystemCodePagesW
#else
#define EnumSystemCodePages  EnumSystemCodePagesA
#endif // !UNICODE



#endif   // NONLS

#ifdef __cplusplus
}
#endif

#endif   // _WINNLS_
