/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    nlstrans.h

Abstract:

    This file contains the header information shared by all of the modules
    of the NLSTRANS utility.

Revision History:

    07-30-91    JulieB    Created.

--*/



////////////////////////////////////////////////////////////////////////////
//
//  Includes Files.
//
////////////////////////////////////////////////////////////////////////////

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>




////////////////////////////////////////////////////////////////////////////
//
//  Constant Declarations.
//
////////////////////////////////////////////////////////////////////////////

//
//  Define boolean constants.
//
#define FALSE                0
#define TRUE                 1


//
//  Table Sizes.
//
#define MAX                  256            // max buffer size
#define MB_TABLE_SIZE        256            // size of MB table
#define GLYPH_TABLE_SIZE     MB_TABLE_SIZE  // size of Glyph table
#define DBCS_TABLE_SIZE      256            // size of DBCS table
#define DBCS_OFFSET_SIZE     256            // size of DBCS offset area
#define WC_TABLE_SIZE        (64 * 1024)    // size of WC table (char cnt)
#define TABLE_SIZE_8         256            // size for 8:4:4 array (8)
#define TABLE_SIZE_4          16            // size for 8:4:4 array (4)
#define MAX_844_TBL_SIZE     (64 * 1024)    // max size of 8:4:4 table
#define FILE_NAME_LEN         10            // max length of a file name
#define MAX_NUM_LEADBYTE      12            // max number of DBCS lead bytes
#define SKEY_TBL_SIZE        (64 * 1024)    // size of SORTKEY default table
#define MAX_CT_MAP_TBL_SIZE  256            // max size of CTYPE Mapping table

#define MAX_FONTSIGNATURE     16            // max size of font signature


//
//  Special Flags.
//
#define DUPLICATE_OFFSET     TABLE_SIZE_4   // offset to duplicate flag (ctype)


//
//  String Constants for Data Files.
//
#define CP_PREFIX            "c_"
#define DATA_FILE_SUFFIX     ".nls"
#define LANGUAGE_FILE        "l_intl.nls"
#define LANG_EXCEPT_FILE     "l_except.nls"
#define LOCALE_FILE          "locale.nls"
#define UNICODE_FILE         "unicode.nls"
#define CTYPE_FILE           "ctype.nls"
#define SORTKEY_FILE         "sortkey.nls"
#define SORTTBLS_FILE        "sorttbls.nls"


//
//  Flags denoting which tables to write to the output files.
//
#define F_CPINFO             0x80000000
#define F_MB                 0x00000001
#define F_GLYPH              0x00000002
#define F_DBCS               0x00000004
#define F_WC                 0x00000008

#define F_UPPER              0x00000010
#define F_LOWER              0x00000020

#define F_ADIGIT             0x00000100
#define F_CZONE              0x00000200
#define F_COMP               0x00000400
#define F_HIRAGANA           0x00001000
#define F_KATAKANA           0x00002000
#define F_HALFWIDTH          0x00004000
#define F_FULLWIDTH          0x00008000
#define F_TRADITIONAL        0x00010000
#define F_SIMPLIFIED         0x00020000

#define F_CTYPE_1            0x00010000
#define F_CTYPE_2            0x00020000
#define F_CTYPE_3            0x00040000

#define F_DEFAULT_SORTKEY    0x00100000
#define F_REVERSE_DW         0x00200000
#define F_DOUBLE_COMPRESS    0x00400000
#define F_MULTIPLE_WEIGHTS   0x00800000
#define F_EXPANSION          0x01000000
#define F_EXCEPTION          0x02000000
#define F_COMPRESSION        0x04000000
#define F_IDEOGRAPH_LCID     0x08000000




////////////////////////////////////////////////////////////////////////////
//
//  Typedef Declarations.
//
////////////////////////////////////////////////////////////////////////////

typedef unsigned long   DWORD;
typedef unsigned short  WORD;
typedef unsigned char   BYTE;
typedef int             BOOL;
typedef void           *PVOID;
typedef unsigned int    UINT;
typedef char           *PSZ;

typedef WORD           *PMB_TBL;
typedef WORD           *PGLYPH_TBL;
typedef PVOID          *PDBCS_TBL_ARRAY;
typedef WORD           *PDBCS_TBL;
typedef WORD           *PDBCS_OFFSETS;

typedef struct dbcs_range_s
{
    WORD                LowRange;
    WORD                HighRange;
    PDBCS_TBL_ARRAY     pDBCSTbls;
} DBCS_RANGE, *PDBCS_RANGE;

typedef PDBCS_RANGE    *PDBCS_ARRAY;

typedef PVOID          *P844_ARRAY;
typedef PVOID           PWC_ARRAY;
typedef P844_ARRAY      PCT_ARRAY;
typedef P844_ARRAY      PUP_ARRAY;
typedef P844_ARRAY      PLO_ARRAY;
typedef P844_ARRAY      PAD_ARRAY;
typedef P844_ARRAY      PCZ_ARRAY;
typedef P844_ARRAY      PHG_ARRAY;
typedef P844_ARRAY      PKK_ARRAY;
typedef P844_ARRAY      PHW_ARRAY;
typedef P844_ARRAY      PFW_ARRAY;
typedef P844_ARRAY      PTR_ARRAY;
typedef P844_ARRAY      PSP_ARRAY;
typedef P844_ARRAY      PPCOMP_ARRAY;
typedef P844_ARRAY      PCOMP_ARRAY;
typedef WORD           *PCOMP_GRID;



////////////////////////////////////////
//
//  CODEPAGE File Structures.
//
////////////////////////////////////////

typedef struct codepage_s
{
    PMB_TBL        pMB;                // ptr to MB array (1:1)
    PGLYPH_TBL     pGlyph;             // ptr to Glyph array (1:1)
    PDBCS_ARRAY    pDBCS;              // ptr to DBCS info
    PDBCS_OFFSETS  pDBCSOff;           // ptr to DBCS offset area
    PWC_ARRAY      pWC;                // ptr to WC 8:4:4 info
    DWORD          WriteFlags;         // which tables to write
    int            NumDBCSRanges;      // number of DBCS Ranges
    PSZ            pszName;            // ptr to codepage name (value)
    int            CodePageValue;      // code page value
    int            MaxCharSize;        // max character length
    WORD           DefaultChar;        // default character (mbcs)
    WORD           UniDefaultChar;     // unicode default char
    WORD           TransDefChar;       // translation of default char (unicode)
    WORD           TransUniDefChar;    // translation of uni def char (mbcs)
    BYTE           LeadBytes[MAX_NUM_LEADBYTE];
} CODEPAGE, *PCODEPAGE;

#define  CP_INFO_SIZE  13              // size of CPINFO + 1 (in words)



////////////////////////////////////////
//
//  LANGUAGE File Structures.
//
////////////////////////////////////////

typedef struct language_s
{
    PUP_ARRAY      pUpper;             // ptr to UPPERCASE 8:4:4 info
    PLO_ARRAY      pLower;             // ptr to LOWERCASE 8:4:4 info
    DWORD          WriteFlags;         // tables to write
    int            IfDefault;          // 1 for default file, 0 otherwise
    int            UPBuf2;             // number UPPERCASE Buffer 2
    int            UPBuf3;             // number UPPERCASE Buffer 3
    int            LOBuf2;             // number LOWERCASE Buffer 2
    int            LOBuf3;             // number LOWERCASE Buffer 3
} LANGUAGE, *PLANGUAGE;


typedef struct l_except_hdr_s
{
    DWORD          Locale;             // locale id
    DWORD          Offset;             // offset to exception nodes
    DWORD          NumUpEntries;       // number of upper case entries
    DWORD          NumLoEntries;       // number of lower case entries
} L_EXCEPT_HDR, *PL_EXCEPT_HDR;

typedef struct l_except_node_s
{
    WORD           UCP;                // unicode code point
    WORD           AddAmount;          // amount to add to initial code point
} L_EXCEPT_NODE, *PL_EXCEPT_NODE;

typedef PL_EXCEPT_NODE *PL_EXCEPT_TBL; // ptr to array of exception nodes

#define NUM_L_EXCEPT_WORDS        (sizeof(L_EXCEPT_NODE) / sizeof(WORD))

typedef struct lang_except_s
{
    int            NumException;       // number of EXCEPTION locales
    PL_EXCEPT_HDR  pExceptHdr;         // ptr to language exception header
    PL_EXCEPT_TBL  pExceptTbl;         // ptr to language exception table
} LANG_EXCEPT, *PLANG_EXCEPT;



////////////////////////////////////////
//
//  UNICODE File Structures.
//
////////////////////////////////////////

typedef struct unicode_s
{
    PAD_ARRAY      pADigit;            // ptr to ASCIIDIGITS 8:4:4 info
    PCZ_ARRAY      pCZone;             // ptr to FOLDCZONE 8:4:4 info
    PHG_ARRAY      pHiragana;          // ptr to HIRAGANA 8:4:4 info
    PKK_ARRAY      pKatakana;          // ptr to KATAKANA 8:4:4 info
    PHW_ARRAY      pHalfWidth;         // ptr to HALFWIDTH 8:4:4 info
    PFW_ARRAY      pFullWidth;         // ptr to FULLWIDTH 8:4:4 info
    PTR_ARRAY      pTraditional;       // ptr to TRADITIONAL 8:4:4 info
    PSP_ARRAY      pSimplified;        // ptr to SIMPLIFIED 8:4:4 info
    PPCOMP_ARRAY   pPreComp;           // ptr to PRECOMPOSED 8:4:4 info
    PCOMP_ARRAY    pBase;              // ptr to COMPOSITE Base 8:4:4
    PCOMP_ARRAY    pNonSp;             // ptr to COMPOSITE NonSpace 8:4:4
    PCOMP_GRID     pCompGrid;          // ptr to COMPOSITE 2D Grid
    int            NumBase;            // total number of BASE characters
    int            NumNonSp;           // total number of NONSPACE characters
    DWORD          WriteFlags;         // tables to write
    int            ADBuf2;             // number ASCIIDIGITS Buffer 2
    int            ADBuf3;             // number ASCIIDIGITS Buffer 3
    int            CZBuf2;             // number FOLDCZONE Buffer 2
    int            CZBuf3;             // number FOLDCZONE Buffer 3
    int            HGBuf2;             // number HIRAGANA Buffer 2
    int            HGBuf3;             // number HIRAGANA Buffer 3
    int            KKBuf2;             // number KATAKANA Buffer 2
    int            KKBuf3;             // number KATAKANA Buffer 3
    int            HWBuf2;             // number HALFWIDTH Buffer 2
    int            HWBuf3;             // number HALFWIDTH Buffer 3
    int            FWBuf2;             // number FULLWIDTH Buffer 2
    int            FWBuf3;             // number FULLWIDTH Buffer 3
    int            TRBuf2;             // number TRADITIONAL Buffer 2
    int            TRBuf3;             // number TRADITIONAL Buffer 3
    int            SPBuf2;             // number SIMPLIFIED Buffer 2
    int            SPBuf3;             // number SIMPLIFIED Buffer 3
    int            PCBuf2;             // number PRECOMPOSED Buffer 2
    int            PCBuf3;             // number PRECOMPOSED Buffer 3
    int            BSBuf2;             // number COMPOSITE BASE Buffer 2
    int            BSBuf3;             // number COMPOSITE BASE Buffer 3
    int            NSBuf2;             // number COMPOSITE NONSPACE Buffer 2
    int            NSBuf3;             // number COMPOSITE NONSPACE Buffer 3
} UNICODE, *PUNICODE;



////////////////////////////////////////
//
//  CTYPE File Structures.
//
////////////////////////////////////////

typedef struct ct_values_s
{
    WORD           CType1;             // ctype 1 value
    WORD           CType2;             // ctype 2 value
    WORD           CType3;             // ctype 3 value
} CT_VALUES, *PCT_VALUES;

typedef BYTE CT_MAP_VALUE, *PCT_MAP_VALUE;

typedef struct ct_map_s
{
    int            Length;             // length of mapping table
    PCT_VALUES     pCTValues;          // table of CTYPE values
} CT_MAP, *PCT_MAP;

typedef struct ctypes_s
{
    PCT_ARRAY      pCType;             // ptr to CTYPE 8:4:4 info
    PCT_MAP        pMap;               // ptr to mapping table
    DWORD          WriteFlags;         // tables to write
    int            CTBuf2;             // number CTYPE Buffer 2
    int            CTBuf3;             // number CTYPE Buffer 3
} CTYPES, *PCTYPES;



////////////////////////////////////////
//
//  SORTKEY & SORTTBLS File Structures.
//
////////////////////////////////////////

typedef struct skey_s
{
    BYTE           Alpha;              // alphanumeric weight
    BYTE           Script;             // script member
    BYTE           Diacritic;          // diacritic weight
    BYTE           Case;               // case weight (incl. compression)
} SKEY, *PSKEY;

typedef struct sortkey_s
{
    PSKEY          pDefault;           // ptr to DEFAULT SORTKEY info
    DWORD          WriteFlags;         // tables to write
} SORTKEY, *PSORTKEY;


typedef DWORD           REV_DW;
typedef REV_DW         *PREV_DW;

typedef DWORD           DBL_COMPRESS;
typedef DBL_COMPRESS   *PDBL_COMPRESS;

typedef struct ideograph_lcid_s
{
    DWORD          Locale;             // locale id
    WORD           pFileName[14];      // ptr to file name
} IDEOGRAPH_LCID, *PIDEOGRAPH_LCID;

typedef struct expand_s
{
    WORD           CP1;                // code point 1 - expansion
    WORD           CP2;                // code point 2 - expansion
} EXPAND, *PEXPAND;

typedef struct compress_hdr_s
{
    DWORD          Locale;             // locale id
    DWORD          Offset;             // offset to compression nodes
    WORD           Num2;               // number of 2 entries for lang id
    WORD           Num3;               // number of 3 entries for lang id
} COMPRESS_HDR, *PCOMPRESS_HDR;

typedef struct compress_2_node_s
{
    WORD           UCP1;               // unicode code point 1
    WORD           UCP2;               // unicode code point 2
    BYTE           Alpha;              // alphanumeric weight
    BYTE           Script;             // script member
    BYTE           Diacritic;          // diacritic weight
    BYTE           Case;               // case weight
} COMPRESS_2_NODE, *PCOMPRESS_2_NODE;

typedef struct compress_3_node_s
{
    WORD           UCP1;               // unicode code point 1
    WORD           UCP2;               // unicode code point 2
    WORD           UCP3;               // unicode code point 3
    WORD           Reserved;           // dword alignment of structure
    BYTE           Alpha;              // alphanumeric weight
    BYTE           Script;             // script member
    BYTE           Diacritic;          // diacritic weight
    BYTE           Case;               // case weight
} COMPRESS_3_NODE, *PCOMPRESS_3_NODE;

typedef PCOMPRESS_2_NODE *PCOMPRESS_2_TBL; // ptr to array of compression 2 nodes
typedef PCOMPRESS_3_NODE *PCOMPRESS_3_TBL; // ptr to array of compression 3 nodes

#define NUM_COMPRESS_2_WORDS      (sizeof(COMPRESS_2_NODE) / sizeof(WORD))
#define NUM_COMPRESS_3_WORDS      (sizeof(COMPRESS_3_NODE) / sizeof(WORD))

typedef struct except_hdr_s
{
    DWORD          Locale;             // locale id
    DWORD          Offset;             // offset to exception nodes
    DWORD          NumEntries;         // number of entries for locale id
} EXCEPT_HDR, *PEXCEPT_HDR;

typedef struct except_node_s
{
    WORD           UCP;                // unicode code point
    BYTE           Alpha;              // alphanumeric weight
    BYTE           Script;             // script member
    BYTE           Diacritic;          // diacritic weight
    BYTE           Case;               // case weight
} EXCEPT_NODE, *PEXCEPT_NODE;

typedef PEXCEPT_NODE *PEXCEPT_TBL;       // ptr to array of exception nodes

#define NUM_EXCEPT_WORDS          (sizeof(EXCEPT_NODE) / sizeof(WORD))

typedef struct multi_wt_s
{
    BYTE           FirstSM;            // value of first script member
    BYTE           NumSM;              // number of script members in range
} MULTI_WT, *PMULTI_WT;


typedef struct sort_tables_s
{
    int             NumReverseDW;      // number of REVERSE DIACRITICS
    int             NumDblCompression; // number of DOUBLE COMPRESSION locales
    int             NumIdeographLcid;  // number of IDEOGRAPH LCIDs
    int             NumExpansion;      // number of EXPANSIONS
    int             NumException;      // number of EXCEPTION locales
    int             NumCompression;    // number of COMPRESSION locales
    int             NumMultiWeight;    // number of MULTIPLE WEIGHTS
    PREV_DW         pReverseDW;        // ptr to REVERSE DIACRITICS info
    PDBL_COMPRESS   pDblCompression;   // ptr to DOUBLE COMPRESSION info
    PIDEOGRAPH_LCID pIdeographLcid;    // ptr to ideograph lcid table
    PEXPAND         pExpansion;        // ptr to EXPANSION info
    PEXCEPT_HDR     pExceptHdr;        // ptr to exception header
    PEXCEPT_TBL     pExceptTbl;        // ptr to exception table
    PCOMPRESS_HDR   pCompressHdr;      // ptr to compression header
    PCOMPRESS_2_TBL pCompress2Tbl;     // ptr to compression 2 table
    PCOMPRESS_3_TBL pCompress3Tbl;     // ptr to compression 3 table
    PMULTI_WT       pMultiWeight;      // ptr to MULTIPLE WEIGHTS info
    DWORD           WriteFlags;        // tables to write
} SORT_TABLES, *PSORT_TABLES;


typedef struct ideograph_node_s
{
    WORD           UCP;                // unicode code point
    BYTE           Alpha;              // alphanumeric weight
    BYTE           Script;             // script member
} IDEOGRAPH_NODE, *PIDEOGRAPH_NODE;

typedef struct ideograph_node_ex_s
{
    WORD           UCP;                // unicode code point
    BYTE           Alpha;              // alphanumeric weight
    BYTE           Script;             // script member
    BYTE           Diacritic;          // diacritic weight
    BYTE           Case;               // case weight
} IDEOGRAPH_NODE_EX, *PIDEOGRAPH_NODE_EX;

typedef struct ideograph_except_s
{
    DWORD              NumEntries;        // number of entries
    DWORD              NumColumns;        // number of columns in table
    BYTE               pFileName[14];     // ptr to file name - ANSI
    PIDEOGRAPH_NODE    pExcept;           // ptr to except nodes
    PIDEOGRAPH_NODE_EX pExceptEx;         // ptr to except nodes ex
} IDEOGRAPH_EXCEPT, *PIDEOGRAPH_EXCEPT;


typedef struct loc_cal_hdr_s
{
    DWORD  NumLocales;                 // number of locales
    DWORD  NumCalendars;               // number of calendars
    DWORD  CalOffset;                  // offset to calendar info (words)
} LOC_CAL_HDR, *PLOC_CAL_HDR;

#define LOC_NUM_CAL_WORDS    2         // number of words to NumCalendars

#define LOC_CAL_HDR_WORDS         (sizeof(LOC_CAL_HDR) / sizeof(WORD))



////////////////////////////////////////
//
//  LOCALE File Structures.
//
////////////////////////////////////////

typedef struct locale_hdr_s
{
    DWORD  Locale;                     // locale id
    DWORD  Offset;                     // offset to locale info (words)
} LOCALE_HDR, *PLOCALE_HDR;

#define LOCALE_HDR_WORDS          (sizeof(LOCALE_HDR) / sizeof(WORD))

typedef struct locale_header_s
{
    WORD   SLanguage;                  // language name in English
    WORD   SAbbrevLang;                // abbreviated language name
    WORD   SAbbrevLangISO;             // ISO abbreviated language name
    WORD   SNativeLang;                // native language name
    WORD   SCountry;                   // country name in English
    WORD   SAbbrevCtry;                // abbreviated country name
    WORD   SAbbrevCtryISO;             // ISO abbreviated country name
    WORD   SNativeCtry;                // native country name
    WORD   SList;                      // list separator
    WORD   SDecimal;                   // decimal separator
    WORD   SThousand;                  // thousands separator
    WORD   SGrouping;                  // grouping of digits
    WORD   SNativeDigits;              // native digits 0-9
    WORD   SCurrency;                  // local monetary symbol
    WORD   SIntlSymbol;                // international monetary symbol
    WORD   SEngCurrName;               // currency name in English
    WORD   SNativeCurrName;            // native currency name
    WORD   SMonDecSep;                 // monetary decimal separator
    WORD   SMonThousSep;               // monetary thousands separator
    WORD   SMonGrouping;               // monetary grouping of digits
    WORD   SPositiveSign;              // positive sign
    WORD   SNegativeSign;              // negative sign
    WORD   STimeFormat;                // time format
    WORD   STime;                      // time separator
    WORD   S1159;                      // AM designator
    WORD   S2359;                      // PM designator
    WORD   SShortDate;                 // short date format
    WORD   SDate;                      // date separator
    WORD   SYearMonth;                 // year month format
    WORD   SLongDate;                  // long date format
    WORD   IOptionalCalendar;          // additional calendar type(s)
    WORD   SDayName1;                  // day name 1
    WORD   SDayName2;                  // day name 2
    WORD   SDayName3;                  // day name 3
    WORD   SDayName4;                  // day name 4
    WORD   SDayName5;                  // day name 5
    WORD   SDayName6;                  // day name 6
    WORD   SDayName7;                  // day name 7
    WORD   SAbbrevDayName1;            // abbreviated day name 1
    WORD   SAbbrevDayName2;            // abbreviated day name 2
    WORD   SAbbrevDayName3;            // abbreviated day name 3
    WORD   SAbbrevDayName4;            // abbreviated day name 4
    WORD   SAbbrevDayName5;            // abbreviated day name 5
    WORD   SAbbrevDayName6;            // abbreviated day name 6
    WORD   SAbbrevDayName7;            // abbreviated day name 7
    WORD   SMonthName1;                // month name 1
    WORD   SMonthName2;                // month name 2
    WORD   SMonthName3;                // month name 3
    WORD   SMonthName4;                // month name 4
    WORD   SMonthName5;                // month name 5
    WORD   SMonthName6;                // month name 6
    WORD   SMonthName7;                // month name 7
    WORD   SMonthName8;                // month name 8
    WORD   SMonthName9;                // month name 9
    WORD   SMonthName10;               // month name 10
    WORD   SMonthName11;               // month name 11
    WORD   SMonthName12;               // month name 12
    WORD   SMonthName13;               // month name 13
    WORD   SAbbrevMonthName1;          // abbreviated month name 1
    WORD   SAbbrevMonthName2;          // abbreviated month name 2
    WORD   SAbbrevMonthName3;          // abbreviated month name 3
    WORD   SAbbrevMonthName4;          // abbreviated month name 4
    WORD   SAbbrevMonthName5;          // abbreviated month name 5
    WORD   SAbbrevMonthName6;          // abbreviated month name 6
    WORD   SAbbrevMonthName7;          // abbreviated month name 7
    WORD   SAbbrevMonthName8;          // abbreviated month name 8
    WORD   SAbbrevMonthName9;          // abbreviated month name 9
    WORD   SAbbrevMonthName10;         // abbreviated month name 10
    WORD   SAbbrevMonthName11;         // abbreviated month name 11
    WORD   SAbbrevMonthName12;         // abbreviated month name 12
    WORD   SAbbrevMonthName13;         // abbreviated month name 13
    WORD   SEndOfLocale;               // end of the locale info
} LOCALE_HEADER, *PLOCALE_HEADER;

typedef struct locale_static_s
{
    WORD   DefaultACP;                 // default ACP - integer format
    WORD   szILanguage[5];             // language id
    WORD   szICountry[6];              // country id
    WORD   szIDefaultLang[5];          // default language ID
    WORD   szIDefaultCtry[6];          // default country ID
    WORD   szIDefaultACP[6];           // default ansi code page ID
    WORD   szIDefaultOCP[6];           // default oem code page ID
    WORD   szIDefaultMACCP[6];         // default mac code page ID
    WORD   szIDefaultEBCDICCP[6];      // default ebcdic code page ID
    WORD   szIMeasure[2];              // system of measurement
    WORD   szIPaperSize[2];            // default paper size
    WORD   szIDigits[3];               // number of fractional digits
    WORD   szILZero[2];                // leading zeros for decimal
    WORD   szINegNumber[2];            // negative number format
    WORD   szIDigitSubstitution[2];    // digit substitution
    WORD   szICurrDigits[3];           // # local monetary fractional digits
    WORD   szIIntlCurrDigits[3];       // # intl monetary fractional digits
    WORD   szICurrency[2];             // positive currency format
    WORD   szINegCurr[3];              // negative currency format
    WORD   szIPosSignPosn[2];          // format of positive sign
    WORD   szIPosSymPrecedes[2];       // if mon symbol precedes positive
    WORD   szIPosSepBySpace[2];        // if mon symbol separated by space
    WORD   szINegSignPosn[2];          // format of negative sign
    WORD   szINegSymPrecedes[2];       // if mon symbol precedes negative
    WORD   szINegSepBySpace[2];        // if mon symbol separated by space
    WORD   szITime[2];                 // time format
    WORD   szITLZero[2];               // leading zeros for time field
    WORD   szITimeMarkPosn[2];         // time marker position
    WORD   szIDate[2];                 // short date order
    WORD   szICentury[2];              // century format (short date)
    WORD   szIDayLZero[2];             // leading zeros for day field (short date)
    WORD   szIMonLZero[2];             // leading zeros for month field (short date)
    WORD   szILDate[2];                // long date order
    WORD   szICalendarType[2];         // type of calendar to use
    WORD   szIFirstDayOfWeek[2];       // which day is first in week
    WORD   szIFirstWeekOfYear[2];      // which week is first in year
    WORD   szFontSignature[MAX_FONTSIGNATURE];  // font signature
} LOCALE_STATIC, *PLOCALE_STATIC;

typedef struct locale_variable_s
{
    WORD   szSLanguage[MAX];           // language name in English
    WORD   szSAbbrevLang[MAX];         // abbreviated language name
    WORD   szSAbbrevLangISO[MAX];      // ISO abbreviated language name
    WORD   szSNativeLang[MAX];         // native language name
    WORD   szSCountry[MAX];            // country name in English
    WORD   szSAbbrevCtry[MAX];         // abbreviated country name
    WORD   szSAbbrevCtryISO[MAX];      // ISO abbreviated country name
    WORD   szSNativeCtry[MAX];         // native country name
    WORD   szSList[MAX];               // list separator
    WORD   szSDecimal[MAX];            // decimal separator
    WORD   szSThousand[MAX];           // thousands separator
    WORD   szSGrouping[MAX];           // grouping of digits
    WORD   szSNativeDigits[MAX];       // native digits 0-9
    WORD   szSCurrency[MAX];           // local monetary symbol
    WORD   szSIntlSymbol[MAX];         // international monetary symbol
    WORD   szSEngCurrName[MAX];        // currency name in English
    WORD   szSNativeCurrName[MAX];     // native currency name
    WORD   szSMonDecSep[MAX];          // monetary decimal separator
    WORD   szSMonThousSep[MAX];        // monetary thousands separator
    WORD   szSMonGrouping[MAX];        // monetary grouping of digits
    WORD   szSPositiveSign[MAX];       // positive sign
    WORD   szSNegativeSign[MAX];       // negative sign
    WORD   szSTimeFormat[MAX];         // time format
    WORD   szSTime[MAX];               // time separator
    WORD   szS1159[MAX];               // AM designator
    WORD   szS2359[MAX];               // PM designator
    WORD   szSShortDate[MAX];          // short date format
    WORD   szSDate[MAX];               // short date separator
    WORD   szSYearMonth[MAX];          // year month format
    WORD   szSLongDate[MAX];           // long date format
    WORD   szIOptionalCalendar[MAX];   // additional calendar type(s)
    WORD   szSDayName1[MAX];           // day name 1
    WORD   szSDayName2[MAX];           // day name 2
    WORD   szSDayName3[MAX];           // day name 3
    WORD   szSDayName4[MAX];           // day name 4
    WORD   szSDayName5[MAX];           // day name 5
    WORD   szSDayName6[MAX];           // day name 6
    WORD   szSDayName7[MAX];           // day name 7
    WORD   szSAbbrevDayName1[MAX];     // abbreviated day name 1
    WORD   szSAbbrevDayName2[MAX];     // abbreviated day name 2
    WORD   szSAbbrevDayName3[MAX];     // abbreviated day name 3
    WORD   szSAbbrevDayName4[MAX];     // abbreviated day name 4
    WORD   szSAbbrevDayName5[MAX];     // abbreviated day name 5
    WORD   szSAbbrevDayName6[MAX];     // abbreviated day name 6
    WORD   szSAbbrevDayName7[MAX];     // abbreviated day name 7
    WORD   szSMonthName1[MAX];         // month name 1
    WORD   szSMonthName2[MAX];         // month name 2
    WORD   szSMonthName3[MAX];         // month name 3
    WORD   szSMonthName4[MAX];         // month name 4
    WORD   szSMonthName5[MAX];         // month name 5
    WORD   szSMonthName6[MAX];         // month name 6
    WORD   szSMonthName7[MAX];         // month name 7
    WORD   szSMonthName8[MAX];         // month name 8
    WORD   szSMonthName9[MAX];         // month name 9
    WORD   szSMonthName10[MAX];        // month name 10
    WORD   szSMonthName11[MAX];        // month name 11
    WORD   szSMonthName12[MAX];        // month name 12
    WORD   szSMonthName13[MAX];        // month name 13
    WORD   szSAbbrevMonthName1[MAX];   // abbreviated month name 1
    WORD   szSAbbrevMonthName2[MAX];   // abbreviated month name 2
    WORD   szSAbbrevMonthName3[MAX];   // abbreviated month name 3
    WORD   szSAbbrevMonthName4[MAX];   // abbreviated month name 4
    WORD   szSAbbrevMonthName5[MAX];   // abbreviated month name 5
    WORD   szSAbbrevMonthName6[MAX];   // abbreviated month name 6
    WORD   szSAbbrevMonthName7[MAX];   // abbreviated month name 7
    WORD   szSAbbrevMonthName8[MAX];   // abbreviated month name 8
    WORD   szSAbbrevMonthName9[MAX];   // abbreviated month name 9
    WORD   szSAbbrevMonthName10[MAX];  // abbreviated month name 10
    WORD   szSAbbrevMonthName11[MAX];  // abbreviated month name 11
    WORD   szSAbbrevMonthName12[MAX];  // abbreviated month name 12
    WORD   szSAbbrevMonthName13[MAX];  // abbreviated month name 13
} LOCALE_VARIABLE, *PLOCALE_VARIABLE;


typedef struct calendar_hdr_s
{
    WORD  Calendar;                    // calendar id
    WORD  Offset;                      // offset to calendar info (words)
} CALENDAR_HDR, *PCALENDAR_HDR;

#define CALENDAR_HDR_WORDS        (sizeof(CALENDAR_HDR) / sizeof(WORD))


typedef struct calendar_header_s
{
    WORD   NumRanges;                  // number of era ranges
    WORD   IfNames;                    // if any day or month names exist
    WORD   SCalendar;                  // calendar id
    WORD   STwoDigitYearMax;           // two digit year max
    WORD   SEraRanges;                 // era ranges
    WORD   SShortDate;                 // short date format
    WORD   SYearMonth;                 // year month format
    WORD   SLongDate;                  // long date format
    WORD   SDayName1;                  // day name 1
    WORD   SDayName2;                  // day name 2
    WORD   SDayName3;                  // day name 3
    WORD   SDayName4;                  // day name 4
    WORD   SDayName5;                  // day name 5
    WORD   SDayName6;                  // day name 6
    WORD   SDayName7;                  // day name 7
    WORD   SAbbrevDayName1;            // abbreviated day name 1
    WORD   SAbbrevDayName2;            // abbreviated day name 2
    WORD   SAbbrevDayName3;            // abbreviated day name 3
    WORD   SAbbrevDayName4;            // abbreviated day name 4
    WORD   SAbbrevDayName5;            // abbreviated day name 5
    WORD   SAbbrevDayName6;            // abbreviated day name 6
    WORD   SAbbrevDayName7;            // abbreviated day name 7
    WORD   SMonthName1;                // month name 1
    WORD   SMonthName2;                // month name 2
    WORD   SMonthName3;                // month name 3
    WORD   SMonthName4;                // month name 4
    WORD   SMonthName5;                // month name 5
    WORD   SMonthName6;                // month name 6
    WORD   SMonthName7;                // month name 7
    WORD   SMonthName8;                // month name 8
    WORD   SMonthName9;                // month name 9
    WORD   SMonthName10;               // month name 10
    WORD   SMonthName11;               // month name 11
    WORD   SMonthName12;               // month name 12
    WORD   SMonthName13;               // month name 13
    WORD   SAbbrevMonthName1;          // abbreviated month name 1
    WORD   SAbbrevMonthName2;          // abbreviated month name 2
    WORD   SAbbrevMonthName3;          // abbreviated month name 3
    WORD   SAbbrevMonthName4;          // abbreviated month name 4
    WORD   SAbbrevMonthName5;          // abbreviated month name 5
    WORD   SAbbrevMonthName6;          // abbreviated month name 6
    WORD   SAbbrevMonthName7;          // abbreviated month name 7
    WORD   SAbbrevMonthName8;          // abbreviated month name 8
    WORD   SAbbrevMonthName9;          // abbreviated month name 9
    WORD   SAbbrevMonthName10;         // abbreviated month name 10
    WORD   SAbbrevMonthName11;         // abbreviated month name 11
    WORD   SAbbrevMonthName12;         // abbreviated month name 12
    WORD   SAbbrevMonthName13;         // abbreviated month name 13
    WORD   SEndOfCalendar;             // end of the calendar info
} CALENDAR_HEADER, *PCALENDAR_HEADER;


#define FIELD_OFFSET(type, field)    ((DWORD)&(((type *)0)->field))

// size of the name header portion
#define CAL_NAME_HDR_SIZE   ((sizeof(CALENDAR_HEADER) -                     \
                              FIELD_OFFSET(CALENDAR_HEADER, SDayName2)) /   \
                              sizeof(WORD))


typedef struct calendar_variable_s
{
    WORD   szSCalendar[MAX];           // calendar id
    WORD   szSTwoDigitYearMax[MAX];    // two digit year max
    WORD   szSEraRanges[MAX];          // era ranges
    WORD   szSShortDate[MAX];          // short date format
    WORD   szSYearMonth[MAX];          // year month format
    WORD   szSLongDate[MAX];           // long date format
    WORD   szSDayName1[MAX];           // day name 1
    WORD   szSDayName2[MAX];           // day name 2
    WORD   szSDayName3[MAX];           // day name 3
    WORD   szSDayName4[MAX];           // day name 4
    WORD   szSDayName5[MAX];           // day name 5
    WORD   szSDayName6[MAX];           // day name 6
    WORD   szSDayName7[MAX];           // day name 7
    WORD   szSAbbrevDayName1[MAX];     // abbreviated day name 1
    WORD   szSAbbrevDayName2[MAX];     // abbreviated day name 2
    WORD   szSAbbrevDayName3[MAX];     // abbreviated day name 3
    WORD   szSAbbrevDayName4[MAX];     // abbreviated day name 4
    WORD   szSAbbrevDayName5[MAX];     // abbreviated day name 5
    WORD   szSAbbrevDayName6[MAX];     // abbreviated day name 6
    WORD   szSAbbrevDayName7[MAX];     // abbreviated day name 7
    WORD   szSMonthName1[MAX];         // month name 1
    WORD   szSMonthName2[MAX];         // month name 2
    WORD   szSMonthName3[MAX];         // month name 3
    WORD   szSMonthName4[MAX];         // month name 4
    WORD   szSMonthName5[MAX];         // month name 5
    WORD   szSMonthName6[MAX];         // month name 6
    WORD   szSMonthName7[MAX];         // month name 7
    WORD   szSMonthName8[MAX];         // month name 8
    WORD   szSMonthName9[MAX];         // month name 9
    WORD   szSMonthName10[MAX];        // month name 10
    WORD   szSMonthName11[MAX];        // month name 11
    WORD   szSMonthName12[MAX];        // month name 12
    WORD   szSMonthName13[MAX];        // month name 13
    WORD   szSAbbrevMonthName1[MAX];   // abbreviated month name 1
    WORD   szSAbbrevMonthName2[MAX];   // abbreviated month name 2
    WORD   szSAbbrevMonthName3[MAX];   // abbreviated month name 3
    WORD   szSAbbrevMonthName4[MAX];   // abbreviated month name 4
    WORD   szSAbbrevMonthName5[MAX];   // abbreviated month name 5
    WORD   szSAbbrevMonthName6[MAX];   // abbreviated month name 6
    WORD   szSAbbrevMonthName7[MAX];   // abbreviated month name 7
    WORD   szSAbbrevMonthName8[MAX];   // abbreviated month name 8
    WORD   szSAbbrevMonthName9[MAX];   // abbreviated month name 9
    WORD   szSAbbrevMonthName10[MAX];  // abbreviated month name 10
    WORD   szSAbbrevMonthName11[MAX];  // abbreviated month name 11
    WORD   szSAbbrevMonthName12[MAX];  // abbreviated month name 12
    WORD   szSAbbrevMonthName13[MAX];  // abbreviated month name 13
} CALENDAR_VARIABLE, *PCALENDAR_VARIABLE;




////////////////////////////////////////////////////////////////////////////
//
//  Macro Definitions.
//
////////////////////////////////////////////////////////////////////////////

//
//  Macro to make a DWORD from two WORDS.
//
#define MAKE_DWORD(a, b)     ((long)(((WORD)(a)) | ((DWORD)((WORD)(b))) << 16))

//
//  Macros for High and Low Bytes of a WORD.
//
#define LOBYTE(w)                 ((BYTE)(w))
#define HIBYTE(w)                 ((BYTE)(((WORD)(w) >> 8) & 0xFF))

//
//  Macros For High and Low Nibbles of a BYTE.
//
#define LONIBBLE(b)               ((BYTE)((BYTE)(b) & 0xF))
#define HINIBBLE(b)               ((BYTE)(((BYTE)(b) >> 4) & 0xF))

//
//  Macros for Extracting the 8:4:4 Index Values.
//
#define GET8(w)                   (HIBYTE(w))
#define GETHI4(w)                 (HINIBBLE(LOBYTE(w)))
#define GETLO4(w)                 (LONIBBLE(LOBYTE(w)))


//
//  Macro for getting the case weight and compression value byte.
//
#define MAKE_CASE_WT(cw, comp)    ((BYTE)(((BYTE)(comp) << 6) | ((BYTE)(cw))))




////////////////////////////////////////////////////////////////////////////
//
//  Function Prototypes.
//
////////////////////////////////////////////////////////////////////////////

//
//  CodePage Routines.
//
int
ParseCodePage(
    PCODEPAGE pCP,
    PSZ pszKeyWord);

int
WriteCodePage(
    PCODEPAGE pCP);


//
//  Language Routines.
//
int
ParseLanguage(
    PLANGUAGE pLang,
    PSZ pszKeyWord);

int
WriteLanguage(
    PLANGUAGE pLang);

int
ParseLangException(
    PLANG_EXCEPT pLangExcept,
    PSZ pszKeyWord);

int
WriteLangException(
    PLANG_EXCEPT pLangExcept);


//
//  Locale Routines.
//
int
ParseWriteLocale(
    PLOCALE_HEADER pLocHdr,
    PLOCALE_STATIC pLocStat,
    PLOCALE_VARIABLE pLocVar,
    PSZ pszKeyWord);


//
//  Locale Independent (Unicode) Routines.
//
int
ParseUnicode(
    PUNICODE pUnic,
    PSZ pszKeyWord);

int
WriteUnicode(
    PUNICODE pUnic);


//
//  Character Type Routines.
//
int
ParseCTypes(
    PCTYPES pCType);

int
WriteCTypes(
    PCTYPES pCType);


//
//  Sorting Routines.
//
int
ParseSortkey(
    PSORTKEY pSortkey,
    PSZ pszKeyWord);

int
ParseSortTables(
    PSORT_TABLES pSortTbls,
    PSZ pszKeyWord);

int
ParseIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeographExcept);

int
WriteSortkey(
    PSORTKEY pSortkey);

int
WriteSortTables(
    PSORT_TABLES pSortTbls);

int
WriteIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeographExcept);


//
//  Allocation and Free Routines.
//
int
AllocateMB(
    PCODEPAGE pCP);

int
AllocateGlyph(
    PCODEPAGE pCP);

int
AllocateTopDBCS(
    PCODEPAGE pCP,
    int Size);

int
AllocateDBCS(
    PCODEPAGE pCP,
    int Low,
    int High,
    int Index);

int
AllocateWCTable(
    PCODEPAGE pCP,
    int Size);

int
Allocate8(
    P844_ARRAY *pArr);

int
Insert844(
    P844_ARRAY pArr,
    WORD WChar,
    DWORD Value,
    int *cbBuf2,
    int *cbBuf3,
    int Size);

int
Insert844Map(
    P844_ARRAY pArr,
    PCT_MAP pMap,
    WORD WChar,
    WORD Value1,
    WORD Value2,
    WORD Value3,
    int *cbBuf2,
    int *cbBuf3);

int
AllocateTemp844(
    PVOID *ppArr,
    int TblSize,
    int Size);

int
AllocateCTMap(
    PCT_MAP *pMap);

int
AllocateGrid(
    PCOMP_GRID *pCompGrid,
    int TblSize);

int
AllocateLangException(
    PLANG_EXCEPT pLangExcept,
    int TblSize);

int
AllocateLangExceptionNodes(
    PLANG_EXCEPT pLangExcept,
    int TblSize,
    int Index);

int
AllocateSortDefault(
    PSORTKEY pSKey);

int
AllocateReverseDW(
    PSORT_TABLES pSTbl,
    int TblSize);

int
AllocateDoubleCompression(
    PSORT_TABLES pSTbl,
    int TblSize);

int
AllocateIdeographLcid(
    PSORT_TABLES pSTbl,
    int TblSize);

int
AllocateExpansion(
    PSORT_TABLES pSTbl,
    int TblSize);

int
AllocateCompression(
    PSORT_TABLES pSTbl,
    int TblSize);

int
AllocateCompression2Nodes(
    PSORT_TABLES pSTbl,
    int TblSize,
    int Index);

int
AllocateCompression3Nodes(
    PSORT_TABLES pSTbl,
    int TblSize,
    int Index);
int
AllocateException(
    PSORT_TABLES pSTbl,
    int TblSize);

int
AllocateExceptionNodes(
    PSORT_TABLES pSTbl,
    int TblSize,
    int Index);

int
AllocateMultipleWeights(
    PSORT_TABLES pSTbl,
    int TblSize);

int
AllocateIdeographExceptions(
    PIDEOGRAPH_EXCEPT pIdeographExcept,
    int TblSize,
    int NumColumns);

void
Free844(
    P844_ARRAY pArr);

void
FreeCTMap(
    PCT_MAP pMap);


//
//  Table Routines.
//
int
ComputeMBSize(
    PCODEPAGE pCP);

int
Compute844Size(
    int cbBuf2,
    int cbBuf3,
    int Size);

DWORD
ComputeCTMapSize(
    PCT_MAP pMap);

int
Write844Table(
    FILE *pOutputFile,
    P844_ARRAY pArr,
    int cbBuf2,
    int TblSize,
    int Size);

int
Write844TableMap(
    FILE *pOutputFile,
    P844_ARRAY pArr,
    WORD TblSize);

int
WriteCTMapTable(
    FILE *pOutputFile,
    PCT_MAP pMap,
    WORD MapSize);

int
WriteWords(
    FILE *pOutputFile,
    WORD Value,
    int Num);

int
FileWrite(
    FILE *pOutputFile,
    void *Buffer,
    int Size,
    int Count,
    char *ErrStr);

void
RemoveDuplicate844Levels(
    P844_ARRAY pArr,
    int *pBuf2,
    int *pBuf3,
    int Size);


//
//  Utility Routines.
//
int
GetSize(
    int *pSize);




////////////////////////////////////////////////////////////////////////////
//
//  Global Variables.
//
//  Globals are included last because they may require some of the types
//  being defined above.
//
////////////////////////////////////////////////////////////////////////////

extern FILE        *pInputFile;        // pointer to Input File
extern BOOL        Verbose;            // verbose flag
