/*++

Copyright (c) 1991-1999,  Microsoft Corporation  All rights reserved.

Module Name:

    nls.h

Abstract:

    This file contains the header information shared by all of the modules
    of NLS.

Revision History:

    05-31-91    JulieB    Created.

--*/



#ifndef _NLS_
#define _NLS_




////////////////////////////////////////////////////////////////////////////
//
//  RTL Includes Files.
//
////////////////////////////////////////////////////////////////////////////

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif


////////////////////////////////////////////////////////////////////////////
//
//  Include Files.
//
////////////////////////////////////////////////////////////////////////////

#include <base.h>
#include <ntcsrdll.h>
#include <ntcsrsrv.h>
#include <basemsg.h>
#include <windows.h>
#include <winnlsp.h>
#include <winerror.h>
#include <string.h>




////////////////////////////////////////////////////////////////////////////
//
//  Constant Declarations.
//
////////////////////////////////////////////////////////////////////////////

//
//  Code Page Ranges.
//
#define NLS_CP_TABLE_RANGE        0         // begin code page Table range
#define NLS_CP_DLL_RANGE          50000     // begin code page DLL range
#define NLS_CP_ALGORITHM_RANGE    60000     // begin code page Algorithm range


//
//  Table Values.
//
#define MB_TBL_SIZE               256  // size of MB tables
#define GLYPH_TBL_SIZE            256  // size of GLYPH tables
#define DBCS_TBL_SIZE             256  // size of DBCS tables

#define CP_TBL_SIZE               197  // size of code page hash table (prime #)
#define LOC_TBL_SIZE              197  // size of locale hash table (prime #)


//
//  String Constants.
//
#define MAX_PATH_LEN              512  // max length of path name
#define MAX_STRING_LEN            128  // max string length for static buffer
#define MAX_SMALL_BUF_LEN         64   // max length of small buffer

#define MAX_COMPOSITE             5    // max number of composite characters
#define MAX_EXPANSION             3    // max number of expansion characters
#define MAX_TBL_EXPANSION         2    // max expansion chars per table entry
#define MAX_WEIGHTS               6    // max number of words in all weights

// length of sortkey static buffer
#define MAX_SKEYBUFLEN       ( MAX_STRING_LEN * MAX_EXPANSION * MAX_WEIGHTS )


#define MAX_FONTSIGNATURE         16   // length of font signature string

// SetLocaleInfo string constants
#define MAX_SLIST                 4    // max wide chars in sList
#define MAX_IMEASURE              2    // max wide chars in iMeasure
#define MAX_SDECIMAL              4    // max wide chars in sDecimal
#define MAX_STHOUSAND             4    // max wide chars in sThousand
#define MAX_SGROUPING             10   // max wide chars in sGrouping
#define MAX_IDIGITS               2    // max wide chars in iDigits
#define MAX_ILZERO                2    // max wide chars in iLZero
#define MAX_INEGNUMBER            2    // max wide chars in iNegNumber
#define MAX_SNATIVEDIGITS         11   // max wide chars in sNativeDigits
#define MAX_IDIGITSUBST           2    // max wide chars in iDigitSubstitution
#define MAX_SCURRENCY             6    // max wide chars in sCurrency
#define MAX_SMONDECSEP            4    // max wide chars in sMonDecimalSep
#define MAX_SMONTHOUSEP           4    // max wide chars in sMonThousandSep
#define MAX_SMONGROUPING          10   // max wide chars in sMonGrouping
#define MAX_ICURRENCY             2    // max wide chars in iCurrency
#define MAX_SPOSSIGN              5    // max wide chars in sPositiveSign
#define MAX_SNEGSIGN              5    // max wide chars in sNegativeSign
#define MAX_STIMEFORMAT           MAX_REG_VAL_SIZE   // max wide chars in sTimeFormat
#define MAX_STIME                 4    // max wide chars in sTime
#define MAX_ITIME                 2    // max wide chars in iTime
#define MAX_S1159                 9    // max wide chars in s1159
#define MAX_S2359                 9    // max wide chars in s2359
#define MAX_SSHORTDATE            MAX_REG_VAL_SIZE   // max wide chars in sShortDate
#define MAX_SDATE                 4    // max wide chars in sDate
#define MAX_SYEARMONTH            MAX_REG_VAL_SIZE   // max wide chars in sYearMonth
#define MAX_SLONGDATE             MAX_REG_VAL_SIZE   // max wide chars in sLongDate
#define MAX_ICALTYPE              3    // max wide chars in iCalendarType
#define MAX_IFIRSTDAY             2    // max wide chars in iFirstDayOfWeek
#define MAX_IFIRSTWEEK            2    // max wide chars in iFirstWeekOfYear

//
//  NOTE:  If any of the MAX_VALUE_ values change, then the corresponding
//         MAX_CHAR_ value must also change.
//
#define MAX_VALUE_IMEASURE        1    // max value for iMeasure
#define MAX_VALUE_IDIGITS         9    // max value for iDigits
#define MAX_VALUE_ILZERO          1    // max value for iLZero
#define MAX_VALUE_INEGNUMBER      4    // max value for iNegNumber
#define MAX_VALUE_IDIGITSUBST     2    // max value for iDigitSubstitution
#define MAX_VALUE_ICURRDIGITS     99   // max value for iCurrDigits
#define MAX_VALUE_ICURRENCY       3    // max value for iCurrency
#define MAX_VALUE_INEGCURR        15   // max value for iNegCurr
#define MAX_VALUE_ITIME           1    // max value for iTime
#define MAX_VALUE_IFIRSTDAY       6    // max value for iFirstDayOfWeek
#define MAX_VALUE_IFIRSTWEEK      2    // max value for iFirstWeekOfYear

#define MAX_CHAR_IMEASURE       L'1'   // max char value for iMeasure
#define MAX_CHAR_IDIGITS        L'9'   // max char value for iDigits
#define MAX_CHAR_ILZERO         L'1'   // max char value for iLZero
#define MAX_CHAR_INEGNUMBER     L'4'   // max char value for iNegNumber
#define MAX_CHAR_IDIGITSUBST    L'2'   // max char value for iDigitSubstitution
#define MAX_CHAR_ICURRENCY      L'3'   // max char value for iCurrency
#define MAX_CHAR_ITIME          L'1'   // max char value for iTime
#define MAX_CHAR_IFIRSTDAY      L'6'   // max char value for iFirstDayOfWeek
#define MAX_CHAR_IFIRSTWEEK     L'2'   // max char value for iFirstWeekOfYear


// SetCalendarInfo string constants
#define MAX_ITWODIGITYEAR         5    // max wide chars in TwoDigitYearMax


#define NLS_CHAR_ZERO           L'0'   // digit 0 character
#define NLS_CHAR_ONE            L'1'   // digit 1 character
#define NLS_CHAR_NINE           L'9'   // digit 9 character
#define NLS_CHAR_SEMICOLON      L';'   // semicolon character
#define NLS_CHAR_PERIOD         L'.'   // period character
#define NLS_CHAR_QUOTE          L'\''  // single quote character
#define NLS_CHAR_SPACE          L' '   // space character
#define NLS_CHAR_HYPHEN         L'-'   // hyphen/minus character
#define NLS_CHAR_OPEN_PAREN     L'('   // open parenthesis character
#define NLS_CHAR_CLOSE_PAREN    L')'   // close parenthesis character

#define MAX_BLANKS                1    // max successive blanks in number string


//
//  RC File Constants.
//
#define NLS_SORT_RES_PREFIX       L"SORT_"
#define NLS_SORT_RES_DEFAULT      L"SORT_00000000"


//
//  Size of stack buffer for PKEY_VALUE_FULL_INFORMATION pointer.
//
#define MAX_KEY_VALUE_FULLINFO                                             \
    ( FIELD_OFFSET( KEY_VALUE_FULL_INFORMATION, Name ) + MAX_PATH_LEN )


//
//  Paths to registry keys.
//
#define NLS_HKLM_SYSTEM    L"\\Registry\\Machine\\System\\CurrentControlSet\\Control"
#define NLS_HKLM_SOFTWARE  L"\\Registry\\Machine\\Software\\Microsoft\\Windows NT\\CurrentVersion"


//
//  Names of Registry Key Entries.
//
#define NLS_CODEPAGE_KEY           L"\\Nls\\Codepage"
#define NLS_LANGUAGE_GROUPS_KEY    L"\\Nls\\Language Groups"
#define NLS_LOCALE_KEY             L"\\Nls\\Locale"
#define NLS_ALT_SORTS_KEY          L"\\Nls\\Locale\\Alternate Sorts"
#define NLS_MUILANG_KEY            L"\\Nls\\MUILanguages"

//  User Info
#define NLS_CTRL_PANEL_KEY         L"Control Panel\\International"
#define NLS_CALENDARS_KEY          L"Control Panel\\International\\Calendars"
#define NLS_TWO_DIGIT_YEAR_KEY     L"Control Panel\\International\\Calendars\\TwoDigitYearMax"
#define NLS_POLICY_TWO_DIGIT_YEAR_KEY L"Software\\Policies\\Microsoft\\Control Panel\\International\\Calendars\\TwoDigitYearMax"


//
//  Name of NLS Object Directory.
//  Must create this in order to have "create access" on the fly.
//
#define NLS_OBJECT_DIRECTORY_NAME  L"\\NLS"

//
//  Default values.
//
#define NLS_DEFAULT_ACP           1252
#define NLS_DEFAULT_OEMCP         437
#define NLS_DEFAULT_MACCP         10000
#define NLS_DEFAULT_LANGID        0x0409
#define NLS_DEFAULT_UILANG        0x0409


//
//  DLL Translation Function Names.
//
//  ****  Must be an ANSI string for GetProcAddress call.  ****
//
#define NLS_CP_DLL_PROC_NAME      "NlsDllCodePageTranslation"


//
//  Flag Constants.
//
#define MSB_FLAG         0x80000000    // most significant bit set


//
//  Table Header Constants  (all sizes in WORDS).
//
#define CP_HEADER                 1    // size of CP Info table header
#define MB_HEADER                 1    // size of MB table header
#define GLYPH_HEADER              1    // size of GLYPH table header
#define DBCS_HEADER               1    // size of DBCS table header
#define WC_HEADER                 1    // size of WC table header
#define CT_HEADER                 2    // size of CTYPE table header
#define LANG_HDR_OFFSET           0    // offset to LANGUAGE file header
#define LANG_HEADER               1    // size of language file header
#define UP_HEADER                 1    // size of UPPERCASE table header
#define LO_HEADER                 1    // size of LOWERCASE table header
#define L_EXCEPT_HDR_OFFSET       2    // offset to LANGUAGE EXCEPTION header
#define AD_HEADER                 1    // size of ASCIIDIGITS table header
#define CZ_HEADER                 1    // size of FOLDCZONE table header
#define HG_HEADER                 1    // size of HIRAGANA table header
#define KK_HEADER                 1    // size of KATAKANA table header
#define HW_HEADER                 1    // size of HALF WIDTH table header
#define FW_HEADER                 1    // size of FULL WIDTH table header
#define TR_HEADER                 1    // size of TRADITIONAL CHINESE table header
#define SP_HEADER                 1    // size of SIMPLIFIED CHINESE table header
#define PC_HEADER                 1    // size of PRECOMPOSED table header
#define CO_HEADER                 3    // size of COMPOSITE table header
#define SORTKEY_HEADER            2    // size of SORTKEY table header
#define REV_DW_HEADER             2    // size of REVERSE DW table header
#define DBL_COMP_HEADER           2    // size of DOUBLE COMPRESS table header
#define IDEO_LCID_HEADER          2    // size of IDEOGRAPH LCID table header
#define EXPAND_HEADER             2    // size of EXPANSION table header
#define COMPRESS_HDR_OFFSET       2    // offset to COMPRESSION header
#define EXCEPT_HDR_OFFSET         2    // offset to EXCEPTION header
#define MULTI_WT_HEADER           1    // size of MULTIPLE WEIGHTS table header


//
//  Invalid Flag Checks.
//
#define MB_INVALID_FLAG   (~(MB_PRECOMPOSED   | MB_COMPOSITE |              \
                             MB_USEGLYPHCHARS | MB_ERR_INVALID_CHARS))
#define WC_INVALID_FLAG   (~(WC_NO_BEST_FIT_CHARS | WC_COMPOSITECHECK |     \
                             WC_DISCARDNS | WC_SEPCHARS | WC_DEFAULTCHAR))
#define CS_INVALID_FLAG   (~(NORM_IGNORECASE    | NORM_IGNORENONSPACE |     \
                             NORM_IGNORESYMBOLS | NORM_IGNOREKANATYPE |     \
                             NORM_IGNOREWIDTH   | SORT_STRINGSORT     |     \
                             LOCALE_USE_CP_ACP  | NORM_STOP_ON_NULL))
#define FS_INVALID_FLAG   (~(MAP_FOLDCZONE | MAP_PRECOMPOSED |              \
                             MAP_COMPOSITE | MAP_FOLDDIGITS))
#define LCMS_INVALID_FLAG (~(LCMAP_LOWERCASE | LCMAP_UPPERCASE |            \
                             LCMAP_LINGUISTIC_CASING |                      \
                             LCMAP_SORTKEY   | LCMAP_BYTEREV   |            \
                             LCMAP_HIRAGANA  | LCMAP_KATAKANA  |            \
                             LCMAP_HALFWIDTH | LCMAP_FULLWIDTH |            \
                             LCMAP_TRADITIONAL_CHINESE |                    \
                             LCMAP_SIMPLIFIED_CHINESE  |                    \
                             NORM_IGNORECASE    | NORM_IGNORENONSPACE |     \
                             NORM_IGNORESYMBOLS | NORM_IGNOREKANATYPE |     \
                             NORM_IGNOREWIDTH   | SORT_STRINGSORT     |     \
                             LOCALE_USE_CP_ACP))
#define GTF_INVALID_FLAG  (~(LOCALE_NOUSEROVERRIDE | LOCALE_USE_CP_ACP |    \
                             TIME_NOMINUTESORSECONDS                   |    \
                             TIME_NOSECONDS | TIME_NOTIMEMARKER        |    \
                             TIME_FORCE24HOURFORMAT))
#define GDF_INVALID_FLAG  (~(LOCALE_NOUSEROVERRIDE | LOCALE_USE_CP_ACP |    \
                             DATE_LTRREADING       | DATE_RTLREADING   |    \
                             DATE_SHORTDATE        | DATE_LONGDATE     |    \
                             DATE_YEARMONTH        | DATE_USE_ALT_CALENDAR |\
                             DATE_ADDHIJRIDATETEMP))
#define IVLG_INVALID_FLAG (~(LGRPID_INSTALLED | LGRPID_SUPPORTED))
#define IVL_INVALID_FLAG  (~(LCID_INSTALLED | LCID_SUPPORTED))
#define GNF_INVALID_FLAG  (~(LOCALE_NOUSEROVERRIDE | LOCALE_USE_CP_ACP))
#define GCF_INVALID_FLAG  (~(LOCALE_NOUSEROVERRIDE | LOCALE_USE_CP_ACP))
#define ESLG_INVALID_FLAG (~(LGRPID_INSTALLED | LGRPID_SUPPORTED))
#define ESL_INVALID_FLAG  (~(LCID_INSTALLED | LCID_SUPPORTED |             \
                             LCID_ALTERNATE_SORTS))
#define ESCP_INVALID_FLAG (~(CP_INSTALLED | CP_SUPPORTED))
#define ETF_INVALID_FLAG  (~(LOCALE_USE_CP_ACP))


//
//  Single Flags (only one at a time is valid).
//
#define LCMS1_SINGLE_FLAG (LCMAP_LOWERCASE | LCMAP_UPPERCASE |             \
                           LCMAP_SORTKEY)
#define LCMS2_SINGLE_FLAG (LCMAP_HIRAGANA  | LCMAP_KATAKANA  |             \
                           LCMAP_SORTKEY)
#define LCMS3_SINGLE_FLAG (LCMAP_HALFWIDTH | LCMAP_FULLWIDTH |             \
                           LCMAP_SORTKEY)
#define LCMS4_SINGLE_FLAG (LCMAP_TRADITIONAL_CHINESE |                     \
                           LCMAP_SIMPLIFIED_CHINESE  |                     \
                           LCMAP_SORTKEY)
#define GDF_SINGLE_FLAG   (DATE_LTRREADING | DATE_RTLREADING)
#define IVLG_SINGLE_FLAG  (LGRPID_INSTALLED  | LGRPID_SUPPORTED)
#define IVL_SINGLE_FLAG   (LCID_INSTALLED  | LCID_SUPPORTED)
#define ESLG_SINGLE_FLAG  (LGRPID_INSTALLED  | LGRPID_SUPPORTED)
#define ESL_SINGLE_FLAG   (LCID_INSTALLED  | LCID_SUPPORTED)
#define ESCP_SINGLE_FLAG  (CP_INSTALLED    | CP_SUPPORTED)


//
//  Flag combinations.
//
#define WC_COMPCHK_FLAGS  (WC_DISCARDNS | WC_SEPCHARS | WC_DEFAULTCHAR)
#define NORM_ALL          (NORM_IGNORECASE    | NORM_IGNORENONSPACE |      \
                           NORM_IGNORESYMBOLS | NORM_IGNOREKANATYPE |      \
                           NORM_IGNOREWIDTH)
#define NORM_SORTKEY_ONLY (NORM_IGNORECASE    | NORM_IGNOREKANATYPE |      \
                           NORM_IGNOREWIDTH   | SORT_STRINGSORT)
#define NORM_ALL_CASE     (NORM_IGNORECASE    | NORM_IGNOREKANATYPE |      \
                           NORM_IGNOREWIDTH)
#define LCMAP_NO_NORM     (LCMAP_LOWERCASE    | LCMAP_UPPERCASE     |      \
                           LCMAP_HIRAGANA     | LCMAP_KATAKANA      |      \
                           LCMAP_HALFWIDTH    | LCMAP_FULLWIDTH     |      \
                           LCMAP_TRADITIONAL_CHINESE                |      \
                           LCMAP_SIMPLIFIED_CHINESE)

//
// Get the LCType value from an LCType
//
#define NLS_GET_LCTYPE_VALUE(x)  (x & ~(LOCALE_NOUSEROVERRIDE |  \
                                        LOCALE_USE_CP_ACP     |  \
                                        LOCALE_RETURN_NUMBER))


//
// Get the CalType from  a CalType
//
#define NLS_GET_CALTYPE_VALUE(x) (x & ~(CAL_NOUSEROVERRIDE |  \
                                        CAL_USE_CP_ACP     |  \
                                        CAL_RETURN_NUMBER))

//
//  Separator and Terminator Values - Sortkey String.
//
#define SORTKEY_SEPARATOR    0x01
#define SORTKEY_TERMINATOR   0x00


//
//  Lowest weight values.
//  Used to remove trailing DW and CW values.
//
#define MIN_DW  2
#define MIN_CW  2


//
//  Bit mask values.
//
//  Case Weight (CW) - 8 bits:
//    bit 0   => width
//    bit 1,2 => small kana, sei-on
//    bit 3,4 => upper/lower case
//    bit 5   => kana
//    bit 6,7 => compression
//
#define COMPRESS_3_MASK      0xc0      // compress 3-to-1 or 2-to-1
#define COMPRESS_2_MASK      0x80      // compress 2-to-1

#define CASE_MASK            0x3f      // zero out compression bits

#define CASE_UPPER_MASK      0xe7      // zero out case bits
#define CASE_SMALL_MASK      0xf9      // zero out small modifier bits
#define CASE_KANA_MASK       0xdf      // zero out kana bit
#define CASE_WIDTH_MASK      0xfe      // zero out width bit

#define SW_POSITION_MASK     0x8003    // avoid 0 or 1 in bytes of word

//
//  Bit Mask Values for CompareString.
//
//  NOTE: Due to intel byte reversal, the DWORD value is backwards:
//                CW   DW   SM   AW
//
//  Case Weight (CW) - 8 bits:
//    bit 0   => width
//    bit 4   => case
//    bit 5   => kana
//    bit 6,7 => compression
//
#define CMP_MASKOFF_NONE          0xffffffff
#define CMP_MASKOFF_DW            0xff00ffff
#define CMP_MASKOFF_CW            0xe7ffffff
#define CMP_MASKOFF_DW_CW         0xe700ffff
#define CMP_MASKOFF_COMPRESSION   0x3fffffff

#define CMP_MASKOFF_KANA          0xdfffffff
#define CMP_MASKOFF_WIDTH         0xfeffffff
#define CMP_MASKOFF_KANA_WIDTH    0xdeffffff

//
//  Masks to isolate the various bits in the case weight.
//
//  NOTE: Bit 2 must always equal 1 to avoid getting a byte value
//        of either 0 or 1.
//
#define CASE_XW_MASK         0xc4

#define ISOLATE_SMALL        ( (BYTE)((~CASE_SMALL_MASK) | CASE_XW_MASK) )
#define ISOLATE_KANA         ( (BYTE)((~CASE_KANA_MASK)  | CASE_XW_MASK) )
#define ISOLATE_WIDTH        ( (BYTE)((~CASE_WIDTH_MASK) | CASE_XW_MASK) )

//
//  UW Mask for Cho-On:
//    Leaves bit 7 on in AW, so it becomes Repeat if it follows Kana N.
//
#define CHO_ON_UW_MASK       0xff87

//
//  Values for fareast special case alphanumeric weights.
//
#define AW_REPEAT            0
#define AW_CHO_ON            1
#define MAX_SPECIAL_AW       AW_CHO_ON

//
//  Values for weight 5.
//
#define WT_FIVE_KANA         3
#define WT_FIVE_REPEAT       4
#define WT_FIVE_CHO_ON       5



//
//  Script Member Values.
//
#define UNSORTABLE           0
#define NONSPACE_MARK        1
#define EXPANSION            2
#define FAREAST_SPECIAL      3

  //  Values 4 thru 5 are available for other special cases
#define RESERVED_2           4
#define RESERVED_3           5

#define PUNCTUATION          6

#define SYMBOL_1             7
#define SYMBOL_2             8
#define SYMBOL_3             9
#define SYMBOL_4             10
#define SYMBOL_5             11

#define NUMERIC_1            12
#define NUMERIC_2            13

#define LATIN                14
#define GREEK                15
#define CYRILLIC             16
#define ARMENIAN             17
#define HEBREW               18
#define ARABIC               19
#define DEVANAGARI           20
#define BENGALI              21
#define GURMUKKHI            22
#define GUJARATI             23
#define ORIYA                24
#define TAMIL                25
#define TELUGU               26
#define KANNADA              27
#define MALAYLAM             28
#define SINHALESE            29
#define THAI                 30
#define LAO                  31
#define TIBETAN              32
#define GEORGIAN             33
#define KANA                 34
#define BOPOMOFO             35
#define HANGUL               36
#define IDEOGRAPH            128

#define MAX_SPECIAL_CASE     SYMBOL_5
#define FIRST_SCRIPT         LATIN


//
//  Calendar Type Values.
//
#define CAL_NO_OPTIONAL      0                          // no optional calendars
#define CAL_LAST             CAL_GREGORIAN_XLIT_FRENCH  // greatest calendar value

//
//  The following calendars are defined in winnls.w:
//
//  #define CAL_GREGORIAN                  1
//  #define CAL_GREGORIAN_US               2
//  #define CAL_JAPAN                      3
//  #define CAL_TAIWAN                     4
//  #define CAL_KOREA                      5
//  #define CAL_HIJRI                      6
//  #define CAL_THAI                       7
//  #define CAL_HEBREW                     8
//  #define CAL_GREGORIAN_ME_FRENCH        9
//  #define CAL_GREGORIAN_ARABIC           10
//  #define CAL_GREGORIAN_XLIT_ENGLISH     11
//  #define CAL_GREGORIAN_XLIT_FRENCH      12
//


//
//  Constants to define range of Unicode private use area.
//
#define PRIVATE_USE_BEGIN    0xe000
#define PRIVATE_USE_END      0xf8ff


//
//  Internal flag for SpecialMBToWC routine.
//
#define MB_INVALID_CHAR_CHECK     MB_ERR_INVALID_CHARS




////////////////////////////////////////////////////////////////////////////
//
//  Typedef Declarations.
//
////////////////////////////////////////////////////////////////////////////

//
//  Constant Types
//
typedef  LPWORD        P844_TABLE;     // ptr to 8:4:4 table

typedef  LPWORD        PMB_TABLE;      // ptr to MB translation table
typedef  PMB_TABLE     PGLYPH_TABLE;   // ptr to GLYPH translation table
typedef  LPWORD        PDBCS_RANGE;    // ptr to DBCS range
typedef  LPWORD        PDBCS_OFFSETS;  // ptr to DBCS offset section
typedef  LPWORD        PDBCS_TABLE;    // ptr to DBCS translation table
typedef  PVOID         PWC_TABLE;      // ptr to WC translation table
typedef  P844_TABLE    PCTYPE;         // ptr to Character Type table
typedef  P844_TABLE    PCASE;          // ptr to Lower or Upper Case table
typedef  P844_TABLE    PADIGIT;        // ptr to Ascii Digits table
typedef  P844_TABLE    PCZONE;         // ptr to Fold Compat. Zone table
typedef  P844_TABLE    PKANA;          // ptr to Hiragana/Katakana table
typedef  P844_TABLE    PHALFWIDTH;     // ptr to Half Width table
typedef  P844_TABLE    PFULLWIDTH;     // ptr to Full Width table
typedef  P844_TABLE    PCHINESE;       // ptr to Traditional/Simplified Chinese table
typedef  P844_TABLE    PPRECOMP;       // ptr to PreComposed table
typedef  LPWORD        PCOMP_GRID;     // ptr to Composite table 2D grid
typedef  LPWORD        PLOC_INFO;      // ptr to locale information
typedef  LPWORD        PCAL_INFO;      // ptr to calendar information

typedef  DWORD         REVERSE_DW;     // reverse diacritic table
typedef  REVERSE_DW   *PREVERSE_DW;    // ptr to reverse diacritic table
typedef  DWORD         DBL_COMPRESS;   // double compression table
typedef  DBL_COMPRESS *PDBL_COMPRESS;  // ptr to double compression table
typedef  LPWORD        PCOMPRESS;      // ptr to compression table (2 or 3)


//
//  Proc Definition for Code Page DLL Routine.
//
typedef DWORD (*LPFN_CP_PROC)(DWORD, DWORD, LPSTR, int, LPWSTR, int, LPCPINFO);


//
//  CP Information Table Structure (as it is in the data file).
//
typedef struct cp_table_s {
    WORD      CodePage;                // code page number
    WORD      MaxCharSize;             // max length (bytes) of a char
    WORD      wDefaultChar;            // default character (MB)
    WORD      wUniDefaultChar;         // default character (Unicode)
    WORD      wTransDefaultChar;       // translation of wDefaultChar (Unicode)
    WORD      wTransUniDefaultChar;    // translation of wUniDefaultChar (MB)
    BYTE      LeadByte[MAX_LEADBYTES]; // lead byte ranges
} CP_TABLE, *PCP_TABLE;


//
//  Composite Information Structure.
//
typedef struct comp_info_s {
    BYTE           NumBase;            // number base chars in grid
    BYTE           NumNonSp;           // number non space chars in grid
    P844_TABLE     pBase;              // ptr to base char table
    P844_TABLE     pNonSp;             // ptr to nonspace char table
    PCOMP_GRID     pGrid;              // ptr to 2D grid
} COMP_INFO, *PCOMP_INFO;


//
//  Code Page Hash Table Structure.
//
typedef struct cp_hash_s {
    UINT           CodePage;           // codepage ID
    LPFN_CP_PROC   pfnCPProc;          // ptr to code page function proc
    PCP_TABLE      pCPInfo;            // ptr to CPINFO table
    PMB_TABLE      pMBTbl;             // ptr to MB translation table
    PGLYPH_TABLE   pGlyphTbl;          // ptr to GLYPH translation table
    PDBCS_RANGE    pDBCSRanges;        // ptr to DBCS ranges
    PDBCS_OFFSETS  pDBCSOffsets;       // ptr to DBCS offsets
    PWC_TABLE      pWC;                // ptr to WC table
    struct cp_hash_s *pNext;           // ptr to next CP hash node
} CP_HASH, *PCP_HASH;


//
//  Language Exception Header Structure.
//
typedef struct l_except_hdr_s {
    DWORD     Locale;                  // locale id
    DWORD     Offset;                  // offset to exception nodes (words)
    DWORD     NumUpEntries;            // number of upper case entries
    DWORD     NumLoEntries;            // number of lower case entries
} L_EXCEPT_HDR, *PL_EXCEPT_HDR;


//
//  Language Exception Structure.
//
typedef struct l_except_s
{
    WORD      UCP;                     // unicode code point
    WORD      AddAmount;               // amount to add to code point
} L_EXCEPT, *PL_EXCEPT;


//
//  CType header structure.
//
typedef struct ctype_hdr_s {
    WORD      TblSize;                 // size of ctype table
    WORD      MapSize;                 // size of mapping table
} CTYPE_HDR, *PCTYPE_HDR;


//
//  CType Table Structure (Mapping table structure).
//
typedef struct ct_values_s {
    WORD      CType1;                  // ctype 1 value
    WORD      CType2;                  // ctype 2 value
    WORD      CType3;                  // ctype 3 value
} CT_VALUES, *PCT_VALUES;


//
//  Sortkey Structure.
//
typedef struct sortkey_s {

    union {
        struct sm_aw_s {
            BYTE   Alpha;              // alphanumeric weight
            BYTE   Script;             // script member
        } SM_AW;

        WORD  Unicode;                 // unicode weight

    } UW;

    BYTE      Diacritic;               // diacritic weight
    BYTE      Case;                    // case weight (with COMP)

} SORTKEY, *PSORTKEY;


//
//  Extra Weight Structure.
//
typedef struct extra_wt_s {
    BYTE      Four;                    // weight 4
    BYTE      Five;                    // weight 5
    BYTE      Six;                     // weight 6
    BYTE      Seven;                   // weight 7
} EXTRA_WT, *PEXTRA_WT;


//
//  Compression Header Structure.
//  This is the header for the compression tables.
//
typedef struct compress_hdr_s {
    DWORD     Locale;                  // locale id
    DWORD     Offset;                  // offset (in words)
    WORD      Num2;                    // Number of 2 compressions
    WORD      Num3;                    // Number of 3 compressions
} COMPRESS_HDR, *PCOMPRESS_HDR;


//
//  Compression 2 Structure.
//  This is for a 2 code point compression - 2 code points
//  compress to ONE weight.
//
typedef struct compress_2_s {
    WCHAR     UCP1;                    // Unicode code point 1
    WCHAR     UCP2;                    // Unicode code point 2
    SORTKEY   Weights;                 // sortkey weights
} COMPRESS_2, *PCOMPRESS_2;


//
//  Compression 3 Structure.
//  This is for a 3 code point compression - 3 code points
//  compress to ONE weight.
//
typedef struct compress_3_s {
    WCHAR     UCP1;                    // Unicode code point 1
    WCHAR     UCP2;                    // Unicode code point 2
    WCHAR     UCP3;                    // Unicode code point 3
    WCHAR     Reserved;                // dword alignment
    SORTKEY   Weights;                 // sortkey weights
} COMPRESS_3, *PCOMPRESS_3;


//
//  Multiple Weight Structure.
//
typedef struct multiwt_s {
    BYTE      FirstSM;                 // value of first script member
    BYTE      NumSM;                   // number of script members in range
} MULTI_WT, *PMULTI_WT;


//
//  Ideograph Lcid Exception Structure.
//
typedef struct ideograph_lcid_s {
    DWORD     Locale;                  // locale id
    WORD      pFileName[14];           // ptr to file name
} IDEOGRAPH_LCID, *PIDEOGRAPH_LCID;


//
//  Expansion Structure.
//
typedef struct expand_s {
    WCHAR     UCP1;                    // Unicode code point 1
    WCHAR     UCP2;                    // Unicode code point 2
} EXPAND, *PEXPAND;


//
//  Exception Header Structure.
//  This is the header for the exception tables.
//
typedef struct except_hdr_s {
    DWORD     Locale;                  // locale id
    DWORD     Offset;                  // offset to exception nodes (words)
    DWORD     NumEntries;              // number of entries for locale id
} EXCEPT_HDR, *PEXCEPT_HDR;


//
//  Exception Structure.
//
//  NOTE: May also be used for Ideograph Exceptions (4 column tables).
//
typedef struct except_s
{
    WORD      UCP;                     // unicode code point
    WORD      Unicode;                 // unicode weight
    BYTE      Diacritic;               // diacritic weight
    BYTE      Case;                    // case weight
} EXCEPT, *PEXCEPT;


//
//  Ideograph Exception Header Structure.
//
typedef struct ideograph_except_hdr_s
{
    DWORD     NumEntries;              // number of entries in table
    DWORD     NumColumns;              // number of columns in table (2 or 4)
} IDEOGRAPH_EXCEPT_HDR, *PIDEOGRAPH_EXCEPT_HDR;


//
//  Ideograph Exception Structure.
//
typedef struct ideograph_except_s
{
    WORD      UCP;                     // unicode code point
    WORD      Unicode;                 // unicode weight
} IDEOGRAPH_EXCEPT, *PIDEOGRAPH_EXCEPT;


//
//  Locale Information Structures.
//
//  This is the format in which the locale information is kept in the
//  locale data file.  These structures are only used for offsets into
//  the data file, not to store information.
//

//
//  Header at the top of the locale.nls file.
//
typedef struct loc_cal_hdr_s
{
    DWORD     NumLocales;              // number of locales
    DWORD     NumCalendars;            // number of calendars
    DWORD     CalOffset;               // offset to calendar info (words)
} LOC_CAL_HDR, *PLOC_CAL_HDR;

#define LOCALE_HDR_OFFSET    (sizeof(LOC_CAL_HDR) / sizeof(WORD))

//
//  Per entry locale header.
//
//  The locale header structure contains the information given in one entry
//  of the header for the locale information.
//
typedef struct locale_hdr_s {
    DWORD     Locale;                  // locale ID
    DWORD     Offset;                  // offset to locale information
} LOCALE_HDR, *PLOCALE_HDR;

#define LOCALE_HDR_SIZE  (sizeof(LOCALE_HDR) / sizeof(WORD))

//
//  The fixed structure contains the locale information that is of
//  fixed length and in the order in which it is given in the file.
//
typedef struct locale_fixed_s
{
    WORD      DefaultACP;              // default ACP - integer format
    WORD      szILanguage[5];          // language id
    WORD      szICountry[6];           // country id
    WORD      szIDefaultLang[5];       // default language ID
    WORD      szIDefaultCtry[6];       // default country ID
    WORD      szIDefaultACP[6];        // default ansi code page ID
    WORD      szIDefaultOCP[6];        // default oem code page ID
    WORD      szIDefaultMACCP[6];      // default mac code page ID
    WORD      szIDefaultEBCDICCP[6];   // default ebcdic code page ID
    WORD      szIMeasure[2];           // system of measurement
    WORD      szIPaperSize[2];         // default paper size
    WORD      szIDigits[3];            // number of fractional digits
    WORD      szILZero[2];             // leading zeros for decimal
    WORD      szINegNumber[2];         // negative number format
    WORD      szIDigitSubstitution[2]; // digit substitution
    WORD      szICurrDigits[3];        // # local monetary fractional digits
    WORD      szIIntlCurrDigits[3];    // # intl monetary fractional digits
    WORD      szICurrency[2];          // positive currency format
    WORD      szINegCurr[3];           // negative currency format
    WORD      szIPosSignPosn[2];       // format of positive sign
    WORD      szIPosSymPrecedes[2];    // if mon symbol precedes positive
    WORD      szIPosSepBySpace[2];     // if mon symbol separated by space
    WORD      szINegSignPosn[2];       // format of negative sign
    WORD      szINegSymPrecedes[2];    // if mon symbol precedes negative
    WORD      szINegSepBySpace[2];     // if mon symbol separated by space
    WORD      szITime[2];              // time format
    WORD      szITLZero[2];            // leading zeros for time field
    WORD      szITimeMarkPosn[2];      // time marker position
    WORD      szIDate[2];              // short date order
    WORD      szICentury[2];           // century format (short date)
    WORD      szIDayLZero[2];          // leading zeros for day field (short date)
    WORD      szIMonLZero[2];          // leading zeros for month field (short date)
    WORD      szILDate[2];             // long date order
    WORD      szICalendarType[2];      // type of calendar
    WORD      szIFirstDayOfWk[2];      // first day of week
    WORD      szIFirstWkOfYr[2];       // first week of year
    WORD      szFontSignature[MAX_FONTSIGNATURE];
} LOCALE_FIXED, *PLOCALE_FIXED;

//
//  The variable structure contains the offsets to the various pieces of
//  locale information that is of variable length.  It is in the order
//  in which it is given in the file.
//
typedef struct locale_var_s
{
    WORD      SEngLanguage;            // English language name
    WORD      SAbbrevLang;             // abbreviated language name
    WORD      SAbbrevLangISO;          // ISO abbreviated language name
    WORD      SNativeLang;             // native language name
    WORD      SEngCountry;             // English country name
    WORD      SAbbrevCtry;             // abbreviated country name
    WORD      SAbbrevCtryISO;          // ISO abbreviated country name
    WORD      SNativeCtry;             // native country name
    WORD      SList;                   // list separator
    WORD      SDecimal;                // decimal separator
    WORD      SThousand;               // thousands separator
    WORD      SGrouping;               // grouping of digits
    WORD      SNativeDigits;           // native digits 0-9
    WORD      SCurrency;               // local monetary symbol
    WORD      SIntlSymbol;             // international monetary symbol
    WORD      SEngCurrName;            // English currency name
    WORD      SNativeCurrName;         // native currency name
    WORD      SMonDecSep;              // monetary decimal separator
    WORD      SMonThousSep;            // monetary thousands separator
    WORD      SMonGrouping;            // monetary grouping of digits
    WORD      SPositiveSign;           // positive sign
    WORD      SNegativeSign;           // negative sign
    WORD      STimeFormat;             // time format
    WORD      STime;                   // time separator
    WORD      S1159;                   // AM designator
    WORD      S2359;                   // PM designator
    WORD      SShortDate;              // short date format
    WORD      SDate;                   // date separator
    WORD      SYearMonth;              // year month format
    WORD      SLongDate;               // long date format
    WORD      IOptionalCal;            // additional calendar type(s)
    WORD      SDayName1;               // day name 1
    WORD      SDayName2;               // day name 2
    WORD      SDayName3;               // day name 3
    WORD      SDayName4;               // day name 4
    WORD      SDayName5;               // day name 5
    WORD      SDayName6;               // day name 6
    WORD      SDayName7;               // day name 7
    WORD      SAbbrevDayName1;         // abbreviated day name 1
    WORD      SAbbrevDayName2;         // abbreviated day name 2
    WORD      SAbbrevDayName3;         // abbreviated day name 3
    WORD      SAbbrevDayName4;         // abbreviated day name 4
    WORD      SAbbrevDayName5;         // abbreviated day name 5
    WORD      SAbbrevDayName6;         // abbreviated day name 6
    WORD      SAbbrevDayName7;         // abbreviated day name 7
    WORD      SMonthName1;             // month name 1
    WORD      SMonthName2;             // month name 2
    WORD      SMonthName3;             // month name 3
    WORD      SMonthName4;             // month name 4
    WORD      SMonthName5;             // month name 5
    WORD      SMonthName6;             // month name 6
    WORD      SMonthName7;             // month name 7
    WORD      SMonthName8;             // month name 8
    WORD      SMonthName9;             // month name 9
    WORD      SMonthName10;            // month name 10
    WORD      SMonthName11;            // month name 11
    WORD      SMonthName12;            // month name 12
    WORD      SMonthName13;            // month name 13 (if exists)
    WORD      SAbbrevMonthName1;       // abbreviated month name 1
    WORD      SAbbrevMonthName2;       // abbreviated month name 2
    WORD      SAbbrevMonthName3;       // abbreviated month name 3
    WORD      SAbbrevMonthName4;       // abbreviated month name 4
    WORD      SAbbrevMonthName5;       // abbreviated month name 5
    WORD      SAbbrevMonthName6;       // abbreviated month name 6
    WORD      SAbbrevMonthName7;       // abbreviated month name 7
    WORD      SAbbrevMonthName8;       // abbreviated month name 8
    WORD      SAbbrevMonthName9;       // abbreviated month name 9
    WORD      SAbbrevMonthName10;      // abbreviated month name 10
    WORD      SAbbrevMonthName11;      // abbreviated month name 11
    WORD      SAbbrevMonthName12;      // abbreviated month name 12
    WORD      SAbbrevMonthName13;      // abbreviated month name 13 (if exists)
    WORD      SEndOfLocale;            // end of locale information
} LOCALE_VAR, *PLOCALE_VAR;


//
//  Per entry calendar header.
//
//  The calendar header structure contains the information given in one entry
//  of the header for the calendar information.
//
typedef struct calendar_hdr_s
{
    WORD      Calendar;                // calendar id
    WORD      Offset;                  // offset to calendar info (words)
} CALENDAR_HDR, *PCALENDAR_HDR;

#define CALENDAR_HDR_SIZE  (sizeof(CALENDAR_HDR) / sizeof(WORD))

//
//  The variable structure contains the offsets to the various pieces of
//  calendar information that is of variable length.  It is in the order
//  in which it is given in the file.
//
//  The NumRanges value is the number of era ranges.  If this value is zero,
//  then there are no year offsets.
//
//  The IfNames value is a boolean.  If it is 0, then there are NO special
//  day or month names for the calendar.  If it is 1, then there ARE
//  special day and month names for the calendar.
//
//  The rest of the values are offsets to the appropriate strings.
//
typedef struct calendar_var_s
{
    WORD      NumRanges;               // number of era ranges
    WORD      IfNames;                 // if any day or month names exist
    WORD      SCalendar;               // calendar id
    WORD      STwoDigitYearMax;        // two digit year max
    WORD      SEraRanges;              // era ranges
    WORD      SShortDate;              // short date format
    WORD      SYearMonth;              // year month format
    WORD      SLongDate;               // long date format
    WORD      SDayName1;               // day name 1
    WORD      SDayName2;               // day name 2
    WORD      SDayName3;               // day name 3
    WORD      SDayName4;               // day name 4
    WORD      SDayName5;               // day name 5
    WORD      SDayName6;               // day name 6
    WORD      SDayName7;               // day name 7
    WORD      SAbbrevDayName1;         // abbreviated day name 1
    WORD      SAbbrevDayName2;         // abbreviated day name 2
    WORD      SAbbrevDayName3;         // abbreviated day name 3
    WORD      SAbbrevDayName4;         // abbreviated day name 4
    WORD      SAbbrevDayName5;         // abbreviated day name 5
    WORD      SAbbrevDayName6;         // abbreviated day name 6
    WORD      SAbbrevDayName7;         // abbreviated day name 7
    WORD      SMonthName1;             // month name 1
    WORD      SMonthName2;             // month name 2
    WORD      SMonthName3;             // month name 3
    WORD      SMonthName4;             // month name 4
    WORD      SMonthName5;             // month name 5
    WORD      SMonthName6;             // month name 6
    WORD      SMonthName7;             // month name 7
    WORD      SMonthName8;             // month name 8
    WORD      SMonthName9;             // month name 9
    WORD      SMonthName10;            // month name 10
    WORD      SMonthName11;            // month name 11
    WORD      SMonthName12;            // month name 12
    WORD      SMonthName13;            // month name 13
    WORD      SAbbrevMonthName1;       // abbreviated month name 1
    WORD      SAbbrevMonthName2;       // abbreviated month name 2
    WORD      SAbbrevMonthName3;       // abbreviated month name 3
    WORD      SAbbrevMonthName4;       // abbreviated month name 4
    WORD      SAbbrevMonthName5;       // abbreviated month name 5
    WORD      SAbbrevMonthName6;       // abbreviated month name 6
    WORD      SAbbrevMonthName7;       // abbreviated month name 7
    WORD      SAbbrevMonthName8;       // abbreviated month name 8
    WORD      SAbbrevMonthName9;       // abbreviated month name 9
    WORD      SAbbrevMonthName10;      // abbreviated month name 10
    WORD      SAbbrevMonthName11;      // abbreviated month name 11
    WORD      SAbbrevMonthName12;      // abbreviated month name 12
    WORD      SAbbrevMonthName13;      // abbreviated month name 13
    WORD      SEndOfCalendar;          // end of calendar information
} CALENDAR_VAR, *PCALENDAR_VAR;

//
//  IOptionalCalendar structure (locale info).
//
typedef struct opt_cal_s
{
    WORD      CalId;                   // calendar id
    WORD      Offset;                  // offset to next optional calendar
    WORD      pCalStr[1];              // calendar id string (variable length)
//  WORD      pCalNameStr[1];          // calendar name string (variable length)
} OPT_CAL, *POPT_CAL;


//
//  SEraRanges structure inside calendar info.
//
typedef struct era_range_s
{
    WORD      Month;                   // month of era beginning
    WORD      Day;                     // day of era beginning
    WORD      Year;                    // year of era beginning
    WORD      Offset;                  // offset to next era info block
    WORD      pYearStr[1];             // year string (variable length)
//  WORD      pEraNameStr[1];          // era name string (variable length)
} ERA_RANGE, *PERA_RANGE;


//
//  Locale Hash Table Structure.
//
typedef struct loc_hash_s {
    LCID           Locale;             // locale ID
    PLOCALE_VAR    pLocaleHdr;         // ptr to locale header info
    PLOCALE_FIXED  pLocaleFixed;       // ptr to locale fixed size info
    PCASE          pUpperCase;         // ptr to Upper Case table
    PCASE          pLowerCase;         // ptr to Lower Case table
    PCASE          pUpperLinguist;     // ptr to Upper Case Linguistic table
    PCASE          pLowerLinguist;     // ptr to Lower Case Linguistic table
    PSORTKEY       pSortkey;           // ptr to sortkey table
    BOOL           IfReverseDW;        // if DW should go from right to left
    BOOL           IfCompression;      // if compression code points exist
    BOOL           IfDblCompression;   // if double compression exists
    PCOMPRESS_HDR  pCompHdr;           // ptr to compression header
    PCOMPRESS_2    pCompress2;         // ptr to 2 compression table
    PCOMPRESS_3    pCompress3;         // ptr to 3 compression table
    struct loc_hash_s *pNext;          // ptr to next locale hash node
} LOC_HASH, *PLOC_HASH;


//
//  Hash Table Pointers.
//
typedef  PCP_HASH  *PCP_HASH_TBL;      // ptr to a code page hash table
typedef  PLOC_HASH *PLOC_HASH_TBL;     // ptr to a locale hash table


//
//  Table Pointers Structure.  This structure contains pointers to
//  the various tables needed for the NLS APIs.  There should be only
//  ONE of these for each process, and the information should be
//  global to the process.
//
#define NUM_SM     256                  // total number of script members
#define NUM_CAL    64                   // total number calendars allowed

typedef struct tbl_ptrs_s {
    PCP_HASH_TBL    pCPHashTbl;         // ptr to Code Page hash table
    PLOC_HASH_TBL   pLocHashTbl;        // ptr to Locale hash table
    PLOC_INFO       pLocaleInfo;        // ptr to locale table (all locales)
    DWORD           NumCalendars;       // number of calendars
    PCAL_INFO       pCalendarInfo;      // ptr to beginning of calendar info
    PCAL_INFO       pCalTbl[NUM_CAL];   // ptr to calendar table array
    P844_TABLE      pDefaultLanguage;   // ptr to default language table
    P844_TABLE      pLinguistLanguage;  // ptr to default linguistic lang table
    LARGE_INTEGER   LinguistLangSize;   // size of linguistic lang table
    int             NumLangException;   // number of language exceptions
    PL_EXCEPT_HDR   pLangExceptHdr;     // ptr to lang exception table header
    PL_EXCEPT       pLangException;     // ptr to lang exception tables
    PCT_VALUES      pCTypeMap;          // ptr to Ctype Mapping table
    PCTYPE          pCType844;          // ptr to Ctype 8:4:4 table
    PADIGIT         pADigit;            // ptr to Ascii Digits table
    PCZONE          pCZone;             // ptr to Compatibility Zone table
    PKANA           pHiragana;          // ptr to Hiragana table
    PKANA           pKatakana;          // ptr to Katakana table
    PHALFWIDTH      pHalfWidth;         // ptr to Half Width table
    PFULLWIDTH      pFullWidth;         // ptr to Full Width table
    PCHINESE        pTraditional;       // ptr to Traditional Chinese table
    PCHINESE        pSimplified;        // ptr to Simplified Chinese table
    PPRECOMP        pPreComposed;       // ptr to PreComposed Table
    PCOMP_INFO      pComposite;         // ptr to Composite info structure
    DWORD           NumReverseDW;       // number of REVERSE DIACRITICS
    DWORD           NumDblCompression;  // number of DOUBLE COMPRESSION locales
    DWORD           NumIdeographLcid;   // number of IDEOGRAPH LCIDs
    DWORD           NumExpansion;       // number of EXPANSIONS
    DWORD           NumCompression;     // number of COMPRESSION locales
    DWORD           NumException;       // number of EXCEPTION locales
    DWORD           NumMultiWeight;     // number of MULTIPLE WEIGHTS
    PSORTKEY        pDefaultSortkey;    // ptr to default sortkey table
    LARGE_INTEGER   DefaultSortkeySize; // size of default sortkey section
    PREVERSE_DW     pReverseDW;         // ptr to reverse diacritic table
    PDBL_COMPRESS   pDblCompression;    // ptr to double compression table
    PIDEOGRAPH_LCID pIdeographLcid;     // ptr to ideograph lcid table
    PEXPAND         pExpansion;         // ptr to expansion table
    PCOMPRESS_HDR   pCompressHdr;       // ptr to compression table header
    PCOMPRESS       pCompression;       // ptr to compression tables
    PEXCEPT_HDR     pExceptHdr;         // ptr to exception table header
    PEXCEPT         pException;         // ptr to exception tables
    PMULTI_WT       pMultiWeight;       // ptr to multiple weights table
    BYTE            SMWeight[NUM_SM];   // script member weights
} TBL_PTRS, *PTBL_PTRS;

typedef struct nls_locale_cache
{
    NLS_USER_INFO NlsInfo;              // NLS cached information
    HKEY CurrentUserKeyHandle;          // Cached key handle thread impersonation

} NLS_LOCAL_CACHE, *PNLS_LOCAL_CACHE;

//
//  Generic Enum Proc Definitions.
//
typedef BOOL (CALLBACK* NLS_ENUMPROC)(PVOID);
typedef BOOL (CALLBACK* NLS_ENUMPROCEX)(PVOID, DWORD);
typedef BOOL (CALLBACK* NLS_ENUMPROC2)(DWORD, DWORD, PVOID, LONG_PTR);
typedef BOOL (CALLBACK* NLS_ENUMPROC3)(DWORD, PVOID, PVOID, DWORD, LONG_PTR);
typedef BOOL (CALLBACK* NLS_ENUMPROC4)(PVOID, LONG_PTR);


////////////////////////////////////////////////////////////////////////////
//
//  Macro Definitions.
//
////////////////////////////////////////////////////////////////////////////

//
//  Get the wide character count from a byte count.
//
#define GET_WC_COUNT(bc)          ((bc) / sizeof(WCHAR))

//
//  Get the data pointer for the KEY_VALUE_FULL_INFORMATION structure.
//
#define GET_VALUE_DATA_PTR(p)     ((LPWSTR)((PBYTE)(p) + (p)->DataOffset))

//
//  Macros For High and Low Nibbles of a BYTE.
//
#define LO_NIBBLE(b)              ((BYTE)((BYTE)(b) & 0xF))
#define HI_NIBBLE(b)              ((BYTE)(((BYTE)(b) >> 4) & 0xF))

//
//  Macros for Extracting the 8:4:4 Index Values.
//
#define GET8(w)                   (HIBYTE(w))
#define GETHI4(w)                 (HI_NIBBLE(LOBYTE(w)))
#define GETLO4(w)                 (LO_NIBBLE(LOBYTE(w)))


//
//  Macros for setting and checking most significant bit of flag.
//
#define SET_MSB(fl)               (fl |= MSB_FLAG)
#define IS_MSB(fl)                (fl & MSB_FLAG)

//
//  Macro to check if more than one bit is set.
//  Returns 1 if more than one bit set, 0 otherwise.
//
#define MORE_THAN_ONE(f, bits)    (((f & bits) - 1) & (f & bits))

//
//  Macros for single and double byte code pages.
//
#define IS_SBCS_CP(pHash)         (pHash->pCPInfo->MaxCharSize == 1)
#define IS_DBCS_CP(pHash)         (pHash->pCPInfo->MaxCharSize == 2)


////////////////////////////////////////////////////////////////////////////
//
//  TRAVERSE_844_B
//
//  Traverses the 8:4:4 translation table for the given wide character.  It
//  returns the final value of the 8:4:4 table, which is a BYTE in length.
//
//  NOTE: Offsets in table are in BYTES.
//
//    Broken Down Version:
//    --------------------
//        Incr = pTable[GET8(wch)] / sizeof(WORD);
//        Incr = pTable[Incr + GETHI4(wch)];
//        Value = (BYTE *)pTable[Incr + GETLO4(wch)];
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define TRAVERSE_844_B(pTable, wch)                                        \
    (((BYTE *)pTable)[pTable[(pTable[GET8(wch)] / sizeof(WORD)) +          \
                               GETHI4(wch)] +                              \
                      GETLO4(wch)])


////////////////////////////////////////////////////////////////////////////
//
//  TRAVERSE_844_W
//
//  Traverses the 8:4:4 translation table for the given wide character.  It
//  returns the final value of the 8:4:4 table, which is a WORD in length.
//
//    Broken Down Version:
//    --------------------
//        Incr = pTable[GET8(wch)];
//        Incr = pTable[Incr + GETHI4(wch)];
//        Value = pTable[Incr + GETLO4(wch)];
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define TRAVERSE_844_W(pTable, wch)                                        \
    (pTable[pTable[pTable[GET8(wch)] + GETHI4(wch)] + GETLO4(wch)])


////////////////////////////////////////////////////////////////////////////
//
//  TRAVERSE_844_D
//
//  Traverses the 8:4:4 translation table for the given wide character.  It
//  fills in the final word values, Value1 and Value2.  The final value of the
//  8:4:4 table is a DWORD, so both Value1 and Value2 are filled in.
//
//    Broken Down Version:
//    --------------------
//        Incr = pTable[GET8(wch)];
//        Incr = pTable[Incr + GETHI4(wch)];
//        pTable += Incr + (GETLO4(wch) * 2);
//        Value1 = pTable[0];
//        Value2 = pTable[1];
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define TRAVERSE_844_D(pTable, wch, Value1, Value2)                        \
{                                                                          \
    pTable += pTable[pTable[GET8(wch)] + GETHI4(wch)] + (GETLO4(wch) * 2); \
    Value1 = pTable[0];                                                    \
    Value2 = pTable[1];                                                    \
}


////////////////////////////////////////////////////////////////////////////
//
//  GET_INCR_VALUE
//
//  Gets the value of a given wide character from the given 8:4:4 table.  It
//  then uses the value as an increment by adding it to the given wide
//  character code point.
//
//  NOTE:  Whenever there is no translation for the given code point, the
//         tables will return an increment value of 0.  This way, the
//         wide character passed in is the same value that is returned.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_INCR_VALUE(p844Tbl, wch)                                       \
     ((WCHAR)(wch + TRAVERSE_844_W(p844Tbl, wch)))


////////////////////////////////////////////////////////////////////////////
//
//  GET_LOWER_UPPER_CASE
//
//  Gets the lower/upper case value of a given wide character.  If a
//  lower/upper case value exists, it returns the lower/upper case wide
//  character.  Otherwise, it returns the same character passed in wch.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_LOWER_UPPER_CASE(pCaseTbl, wch)                                \
    (GET_INCR_VALUE(pCaseTbl, wch))


////////////////////////////////////////////////////////////////////////////
//
//  GET_ASCII_DIGITS
//
//  Gets the ascii translation for the given digit character.  If no
//  translation is found, then the given character is returned.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_ASCII_DIGITS(pADigit, wch)                                     \
    (GET_INCR_VALUE(pADigit, wch))


////////////////////////////////////////////////////////////////////////////
//
//  GET_FOLD_CZONE
//
//  Gets the translation for the given compatibility zone character.  If no
//  translation is found, then the given character is returned.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_FOLD_CZONE(pCZone, wch)                                        \
    (GET_INCR_VALUE(pCZone, wch))


////////////////////////////////////////////////////////////////////////////
//
//  GET_KANA
//
//  Gets the Hiragana/Katakana equivalent for the given Katakana/Hiragana
//  character.  If no translation is found, then the given character is
//  returned.
//
//  DEFINED AS A MACRO.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_KANA(pKana, wch)                                               \
    (GET_INCR_VALUE(pKana, wch))


////////////////////////////////////////////////////////////////////////////
//
//  GET_HALF_WIDTH
//
//  Gets the Half Width equivalent for the given Full Width character.  If no
//  translation is found, then the given character is returned.
//
//  DEFINED AS A MACRO.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_HALF_WIDTH(pHalf, wch)                                         \
    (GET_INCR_VALUE(pHalf, wch))


////////////////////////////////////////////////////////////////////////////
//
//  GET_FULL_WIDTH
//
//  Gets the Full Width equivalent for the given Half Width character.  If no
//  translation is found, then the given character is returned.
//
//  DEFINED AS A MACRO.
//
//  07-14-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_FULL_WIDTH(pFull, wch)                                         \
    (GET_INCR_VALUE(pFull, wch))


////////////////////////////////////////////////////////////////////////////
//
//  GET_CHINESE
//
//  Gets the Traditional/Simplified Chinese translation for the given
//  Simplified/Traditional Chinese character.  If no translation is found,
//  then the given character is returned.
//
//  DEFINED AS A MACRO.
//
//  05-07-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_CHINESE(pChinese, wch)                                         \
    (GET_INCR_VALUE(pChinese, wch))


////////////////////////////////////////////////////////////////////////////
//
//  GET_CTYPE
//
//  Gets the ctype information for a given wide character.  If the ctype
//  information exists, it returns it.  Otherwise, it returns 0.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_CTYPE(wch, offset)                                             \
    ((((PCT_VALUES)(pTblPtrs->pCTypeMap)) +                                \
      (TRAVERSE_844_B((pTblPtrs->pCType844), wch)))->offset)


////////////////////////////////////////////////////////////////////////////
//
//  GET_BASE_CHAR
//
//  Gets the base character of a given precomposed character.  If the
//  composite form is found, it returns the base character.  Otherwise,
//  it returns 0 for failure.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_BASE_CHAR(wch, Base)                                           \
{                                                                          \
    WCHAR NonSp;                  /* nonspacing character */               \
    WCHAR NewBase;                /* base character - temp holder */       \
                                                                           \
                                                                           \
    /*                                                                     \
     *  Get composite characters.                                          \
     */                                                                    \
    if (GetCompositeChars(wch, &NonSp, &Base))                             \
    {                                                                      \
        while (GetCompositeChars(Base, &NonSp, &NewBase))                  \
        {                                                                  \
            Base = NewBase;                                                \
        }                                                                  \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Return failure - no composite form.                            \
         */                                                                \
        Base = 0;                                                          \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  SORTKEY WEIGHT MACROS
//
//  Parse out the different sortkey weights from a DWORD value.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define GET_SCRIPT_MEMBER(pwt)  ( (BYTE)(((PSORTKEY)(pwt))->UW.SM_AW.Script) )
#define GET_ALPHA_NUMERIC(pwt)  ( (BYTE)(((PSORTKEY)(pwt))->UW.SM_AW.Alpha) )

#define GET_UNICODE(pwt)        ( (WORD)(((PSORTKEY)(pwt))->UW.Unicode) )

#define GET_UNICODE_SM(pwt, sm) ( (WORD)(((PSORTKEY)(pwt))->UW.Unicode) )

#define GET_UNICODE_MOD(pwt, modify_sm)                                    \
    ( (modify_sm) ?                                                        \
        ((WORD)                                                            \
         ((((WORD)((pTblPtrs->SMWeight)[GET_SCRIPT_MEMBER(pwt)])) << 8) |  \
          (WORD)GET_ALPHA_NUMERIC(pwt))) :                                 \
        ((WORD)(((PSORTKEY)(pwt))->UW.Unicode)) )

#define GET_UNICODE_SM_MOD(pwt, sm, modify_sm)                             \
    ( (modify_sm) ?                                                        \
        ((WORD)                                                            \
         ((((WORD)((pTblPtrs->SMWeight)[sm])) << 8) |                      \
          (WORD)GET_ALPHA_NUMERIC(pwt))) :                                 \
        ((WORD)(((PSORTKEY)(pwt))->UW.Unicode)) )

#define MAKE_UNICODE_WT(sm, aw, modify_sm)                                 \
    ( (modify_sm) ?                                                        \
        ((WORD)((((WORD)((pTblPtrs->SMWeight)[sm])) << 8) | (WORD)(aw))) : \
        ((WORD)((((WORD)(sm)) << 8) | (WORD)(aw))) )

#define UNICODE_WT(pwt)           ( (WORD)(((PSORTKEY)(pwt))->UW.Unicode) )

#define GET_DIACRITIC(pwt)        ( (BYTE)(((PSORTKEY)(pwt))->Diacritic) )

#define GET_CASE(pwt)             ( (BYTE)((((PSORTKEY)(pwt))->Case) & CASE_MASK) )

#define CASE_WT(pwt)              ( (BYTE)(((PSORTKEY)(pwt))->Case) )

#define GET_COMPRESSION(pwt)      ( (BYTE)((((PSORTKEY)(pwt))->Case) & COMPRESS_3_MASK) )

#define GET_EXPAND_INDEX(pwt)     ( (BYTE)(((PSORTKEY)(pwt))->UW.SM_AW.Alpha) )

#define GET_SPECIAL_WEIGHT(pwt)   ( (WORD)(((PSORTKEY)(pwt))->UW.Unicode) )

//  position returned is backwards - byte reversal
#define GET_POSITION_SW(pos)      ( (WORD)(((pos) << 2) | SW_POSITION_MASK) )


#define GET_WT_FOUR(pwt)          ( (BYTE)(((PEXTRA_WT)(pwt))->Four) )
#define GET_WT_FIVE(pwt)          ( (BYTE)(((PEXTRA_WT)(pwt))->Five) )
#define GET_WT_SIX(pwt)           ( (BYTE)(((PEXTRA_WT)(pwt))->Six) )
#define GET_WT_SEVEN(pwt)         ( (BYTE)(((PEXTRA_WT)(pwt))->Seven) )


#define MAKE_SORTKEY_DWORD(wt)    ( (DWORD)(*((LPDWORD)(&(wt)))) )

#define MAKE_EXTRA_WT_DWORD(wt)   ( (DWORD)(*((LPDWORD)(&(wt)))) )

#define GET_DWORD_WEIGHT(pHashN, wch)                                      \
    ( MAKE_SORTKEY_DWORD(((pHashN)->pSortkey)[wch]) )

#define GET_EXPANSION_1(pwt)                                               \
    ( ((pTblPtrs->pExpansion)[GET_EXPAND_INDEX(pwt)]).UCP1 )

#define GET_EXPANSION_2(pwt)                                               \
    ( ((pTblPtrs->pExpansion)[GET_EXPAND_INDEX(pwt)]).UCP2 )




#define IS_SYMBOL(pSkey, wch)                                              \
    ( (GET_SCRIPT_MEMBER(&((pSkey)[wch])) >= PUNCTUATION) &&               \
      (GET_SCRIPT_MEMBER(&((pSkey)[wch])) <= SYMBOL_5) )

#define IS_NONSPACE_ONLY(pSkey, wch)                                       \
    ( GET_SCRIPT_MEMBER(&((pSkey)[wch])) == NONSPACE_MARK )

#define IS_NONSPACE(pSkey, wch)                                            \
    ( IS_NONSPACE_ONLY(pSkey, wch) ||                                      \
      (GET_DIACRITIC(&((pSkey)[wch])) > MIN_DW) )

#define IS_ALPHA(ctype1)          ( (ctype1) & C1_ALPHA )




#define IS_KOREAN(lcid)                                                    \
    ( LANGIDFROMLCID(lcid) == MAKELANGID(LANG_KOREAN, SUBLANG_KOREAN) )




////////////////////////////////////////////////////////////////////////////
//
//  CHECK_SPECIAL_LOCALES
//
//  Checks for the special locale values and sets the Locale to the
//  appropriate value.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define CHECK_SPECIAL_LOCALES( Locale , UseCachedLocaleId )                 \
{                                                                           \
    /*                                                                      \
     *  Check for special locale values.                                    \
     */                                                                     \
    if (Locale == LOCALE_SYSTEM_DEFAULT)                                    \
    {                                                                       \
        /*                                                                  \
         *  Get the System Default locale value.                            \
         */                                                                 \
        Locale = gSystemLocale;                                             \
    }                                                                       \
    else if ((Locale == LOCALE_NEUTRAL) || (Locale == LOCALE_USER_DEFAULT)) \
    {                                                                       \
        /*                                                                  \
         *  Get the User locale value.                                      \
         */                                                                 \
        if (!UseCachedLocaleId)                                             \
        {                                                                   \
            Locale = GetUserDefaultLCID();                                  \
        }                                                                   \
        else                                                                \
        {                                                                   \
            Locale = pNlsUserInfo->UserLocaleId;                            \
        }                                                                   \
    }                                                                       \
    /*                                                                      \
     *  Check for a valid primary language and a neutral sublanguage.       \
     */                                                                     \
    else if (SUBLANGID(LANGIDFROMLCID(Locale)) == SUBLANG_NEUTRAL)          \
    {                                                                       \
        /*                                                                  \
         *  Re-form the locale id using the primary language and the        \
         *  default sublanguage.                                            \
         */                                                                 \
        Locale = MAKELCID(MAKELANGID(PRIMARYLANGID(LANGIDFROMLCID(Locale)), \
                                     SUBLANG_DEFAULT),                      \
                          SORTIDFROMLCID(Locale));                          \
    }                                                                       \
}


////////////////////////////////////////////////////////////////////////////
//
//  IS_INVALID_LOCALE
//
//  Checks to see that only the proper bits are used in the locale.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_VALID_LOCALE_MASK          0x000fffff
#define IS_INVALID_LOCALE(Locale)      ( Locale & ~NLS_VALID_LOCALE_MASK )


////////////////////////////////////////////////////////////////////////////
//
//  VALIDATE_LANGUAGE
//
//  Checks that the given Locale contains a valid language id.  It does so
//  by making sure the appropriate casing and sorting tables are present.
//  If the language is valid, pLocHashN will be non-NULL.  Otherwise,
//  pLocHashN will be NULL.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define VALIDATE_LANGUAGE(Locale, pLocHashN, dwFlags, UseCachedLocaleId)   \
{                                                                          \
    /*                                                                     \
     *  Check the system locale first for speed.  This is the most         \
     *  likely one to be used.                                             \
     */                                                                    \
    if (Locale == gSystemLocale)                                           \
    {                                                                      \
        pLocHashN = gpSysLocHashN;                                         \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Check special locale values.                                   \
         */                                                                \
        CHECK_SPECIAL_LOCALES(Locale,UseCachedLocaleId);                   \
                                                                           \
        /*                                                                 \
         *  If the locale is the system default, then the hash node is     \
         *  already stored in a global.                                    \
         */                                                                \
        if (Locale == gSystemLocale)                                       \
        {                                                                  \
            pLocHashN = gpSysLocHashN;                                     \
        }                                                                  \
        else if (IS_INVALID_LOCALE(Locale))                                \
        {                                                                  \
            pLocHashN = NULL;                                              \
        }                                                                  \
        else                                                               \
        {                                                                  \
            /*                                                             \
             *  Need to make sure the locale value is valid.  Need to      \
             *  check the locale file to see if the locale is supported.   \
             */                                                            \
            pLocHashN = GetLocHashNode(Locale);                            \
                                                                           \
            if (pLocHashN != NULL)                                         \
            {                                                              \
                /*                                                         \
                 *  Make sure the appropriate casing and sorting tables    \
                 *  are in the system.                                     \
                 *                                                         \
                 *  NOTE:  If the call fails, pLocHashN will be NULL.      \
                 */                                                        \
                pLocHashN = GetLangHashNode(Locale, dwFlags);              \
            }                                                              \
        }                                                                  \
    }                                                                      \
                                                                           \
    /*                                                                     \
     *  Make sure we don't need to get the linguistic tables.              \
     */                                                                    \
    if ((dwFlags) && (pLocHashN) && (pLocHashN->pLowerLinguist == NULL))   \
    {                                                                      \
        /*                                                                 \
         *  Get locale hash node to make sure the appropriate              \
         *  casing and sorting tables are in the system.                   \
         */                                                                \
        pLocHashN = GetLangHashNode(Locale, dwFlags);                      \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  VALIDATE_LOCALE
//
//  Checks that the given LCID contains a valid locale id.  It does so
//  by making sure the appropriate locale information is present.  If the
//  locale is valid, pLocHashN will be non-NULL.  Otherwise, pLocHashN
//  will be NULL.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define VALIDATE_LOCALE(Locale, pLocHashN, UseCachedLocaleId)              \
{                                                                          \
    /*                                                                     \
     *  Check the system locale first for speed.  This is the most         \
     *  likely one to be used.                                             \
     */                                                                    \
    if (Locale == gSystemLocale)                                           \
    {                                                                      \
        pLocHashN = gpSysLocHashN;                                         \
    }                                                                      \
    else                                                                   \
    {                                                                      \
        /*                                                                 \
         *  Check special locale values.                                   \
         */                                                                \
        CHECK_SPECIAL_LOCALES(Locale,UseCachedLocaleId);                   \
                                                                           \
        /*                                                                 \
         *  If the locale is the system default, then the hash node        \
         *  is already stored in a global.                                 \
         */                                                                \
        if (Locale == gSystemLocale)                                       \
        {                                                                  \
            pLocHashN = gpSysLocHashN;                                     \
        }                                                                  \
        else if (IS_INVALID_LOCALE(Locale))                                \
        {                                                                  \
            pLocHashN = NULL;                                              \
        }                                                                  \
        else                                                               \
        {                                                                  \
            /*                                                             \
             *  Get locale hash node to make sure the appropriate          \
             *  locale table is in the system.                             \
             *                                                             \
             *  NOTE:  If the call fails, pLocHashN will be NULL.          \
             */                                                            \
            pLocHashN = GetLocHashNode(Locale);                            \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  OPEN_CODEPAGE_KEY
//
//  Opens the key for the code page section of the registry for read access.
//
//  DEFINED AS A MACRO.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define OPEN_CODEPAGE_KEY(ReturnVal)                                       \
{                                                                          \
    /*                                                                     \
     *  Make sure code page key is open.                                   \
     */                                                                    \
    if (hCodePageKey == NULL)                                              \
    {                                                                      \
        RtlEnterCriticalSection(&gcsTblPtrs);                              \
        if (hCodePageKey == NULL)                                          \
        {                                                                  \
            if (OpenRegKey( &hCodePageKey,                                 \
                            NLS_HKLM_SYSTEM,                               \
                            NLS_CODEPAGE_KEY,                              \
                            KEY_READ ))                                    \
            {                                                              \
                SetLastError(ERROR_BADDB);                                 \
                RtlLeaveCriticalSection(&gcsTblPtrs);                      \
                return (ReturnVal);                                        \
            }                                                              \
        }                                                                  \
        RtlLeaveCriticalSection(&gcsTblPtrs);                              \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  OPEN_LOCALE_KEY
//
//  Opens the key for the locale section of the registry for read access.
//
//  DEFINED AS A MACRO.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define OPEN_LOCALE_KEY(ReturnVal)                                         \
{                                                                          \
    /*                                                                     \
     *  Make sure locale key is open.                                      \
     */                                                                    \
    if (hLocaleKey == NULL)                                                \
    {                                                                      \
        RtlEnterCriticalSection(&gcsTblPtrs);                              \
        if (hLocaleKey == NULL)                                            \
        {                                                                  \
            if (OpenRegKey( &hLocaleKey,                                   \
                            NLS_HKLM_SYSTEM,                               \
                            NLS_LOCALE_KEY,                                \
                            KEY_READ ))                                    \
            {                                                              \
                SetLastError(ERROR_BADDB);                                 \
                RtlLeaveCriticalSection(&gcsTblPtrs);                      \
                return (ReturnVal);                                        \
            }                                                              \
        }                                                                  \
        RtlLeaveCriticalSection(&gcsTblPtrs);                              \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  OPEN_ALT_SORTS_KEY
//
//  Opens the key for the alternate sorts section of the registry for read
//  access.
//
//  DEFINED AS A MACRO.
//
//  11-15-96    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define OPEN_ALT_SORTS_KEY(ReturnVal)                                      \
{                                                                          \
    /*                                                                     \
     *  Make sure alternate sorts key is open.                             \
     */                                                                    \
    if (hAltSortsKey == NULL)                                              \
    {                                                                      \
        RtlEnterCriticalSection(&gcsTblPtrs);                              \
        if (hAltSortsKey == NULL)                                          \
        {                                                                  \
            if (OpenRegKey( &hAltSortsKey,                                 \
                            NLS_HKLM_SYSTEM,                               \
                            NLS_ALT_SORTS_KEY,                             \
                            KEY_READ ))                                    \
            {                                                              \
                SetLastError(ERROR_BADDB);                                 \
                RtlLeaveCriticalSection(&gcsTblPtrs);                      \
                return (ReturnVal);                                        \
            }                                                              \
        }                                                                  \
        RtlLeaveCriticalSection(&gcsTblPtrs);                              \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  OPEN_LANG_GROUPS_KEY
//
//  Opens the key for the language groups section of the registry for
//  read access.
//
//  DEFINED AS A MACRO.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define OPEN_LANG_GROUPS_KEY(ReturnVal)                                    \
{                                                                          \
    /*                                                                     \
     *  Make sure language groups key is open.                             \
     */                                                                    \
    if (hLangGroupsKey == NULL)                                            \
    {                                                                      \
        RtlEnterCriticalSection(&gcsTblPtrs);                              \
        if (hLangGroupsKey == NULL)                                        \
        {                                                                  \
            if (OpenRegKey( &hLangGroupsKey,                               \
                            NLS_HKLM_SYSTEM,                               \
                            NLS_LANGUAGE_GROUPS_KEY,                       \
                            KEY_READ ))                                    \
            {                                                              \
                SetLastError(ERROR_BADDB);                                 \
                RtlLeaveCriticalSection(&gcsTblPtrs);                      \
                return (ReturnVal);                                        \
            }                                                              \
        }                                                                  \
        RtlLeaveCriticalSection(&gcsTblPtrs);                              \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  OPEN_MUILANG_KEY
//
//  Opens the key for the multilingual UI language section of the registry
//  for read access.  It is acceptable if this key is not in the registry,
//  so do not call SetLastError if the key cannot be opened.
//
//  DEFINED AS A MACRO.
//
//  03-10-98    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define OPEN_MUILANG_KEY(hKey, ReturnVal)                                  \
{                                                                          \
    if ((hKey) == NULL)                                                    \
    {                                                                      \
        if (OpenRegKey( &(hKey),                                           \
                        NLS_HKLM_SYSTEM,                                   \
                        NLS_MUILANG_KEY,                                   \
                        KEY_READ ))                                        \
        {                                                                  \
            return (ReturnVal);                                            \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  OPEN_CPANEL_INTL_KEY
//
//  Opens the key for the control panel international section of the
//  registry for the given access.
//
//  DEFINED AS A MACRO.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define OPEN_CPANEL_INTL_KEY(hKey, ReturnVal, Access)                      \
{                                                                          \
    if ((hKey) == NULL)                                                    \
    {                                                                      \
        if (OpenRegKey( &(hKey),                                           \
                        NULL,                                              \
                        NLS_CTRL_PANEL_KEY,                                \
                        Access ))                                          \
        {                                                                  \
            SetLastError(ERROR_BADDB);                                     \
            return (ReturnVal);                                            \
        }                                                                  \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  CLOSE_REG_KEY
//
//  Closes the given registry key.
//
//  DEFINED AS A MACRO.
//
//  09-01-93    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define CLOSE_REG_KEY(hKey)                                                \
{                                                                          \
    if ((hKey) != NULL)                                                    \
    {                                                                      \
        NtClose(hKey);                                                     \
        hKey = NULL;                                                       \
    }                                                                      \
}


////////////////////////////////////////////////////////////////////////////
//
//  NLS_ALLOC_MEM
//
//  Allocates the given number of bytes of memory from the process heap,
//  zeros the memory buffer, and returns the handle.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_ALLOC_MEM(dwBytes)                                             \
    ( RtlAllocateHeap( RtlProcessHeap(),                                   \
                       HEAP_ZERO_MEMORY,                                   \
                       dwBytes ) )


////////////////////////////////////////////////////////////////////////////
//
//  NLS_FREE_MEM
//
//  Frees the memory of the given handle from the process heap.
//
//  DEFINED AS A MACRO.
//
//  05-31-91    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_FREE_MEM(hMem)                                                 \
    ( (hMem) ? (RtlFreeHeap( RtlProcessHeap(),                             \
                             0,                                            \
                             (PVOID)hMem ))                                \
             : 0 )


////////////////////////////////////////////////////////////////////////////
//
//  NLS_FREE_TMP_BUFFER
//
//  Checks to see if the buffer is the same as the static buffer.  If it
//  is NOT the same, then the buffer is freed.
//
//  11-04-92    JulieB    Created.
////////////////////////////////////////////////////////////////////////////

#define NLS_FREE_TMP_BUFFER(pBuf, pStaticBuf)                              \
{                                                                          \
    if (pBuf != pStaticBuf)                                                \
    {                                                                      \
        NLS_FREE_MEM(pBuf);                                                \
    }                                                                      \
}




////////////////////////////////////////////////////////////////////////////
//
//  Function Prototypes
//
////////////////////////////////////////////////////////////////////////////

//
//  Table Routines - tables.c.
//
ULONG
AllocTables(void);

ULONG
GetUnicodeFileInfo(void);

ULONG
GetCTypeFileInfo(void);

ULONG
GetDefaultSortkeyFileInfo(void);

ULONG
GetDefaultSortTablesFileInfo(void);

ULONG
GetSortkeyFileInfo(
    LCID Locale,
    PLOC_HASH pHashN);

void
GetSortTablesFileInfo(
    LCID Locale,
    PLOC_HASH pHashN);

ULONG
GetCodePageFileInfo(
    UINT CodePage,
    PCP_HASH *ppNode);

ULONG
GetLanguageFileInfo(
    LCID Locale,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode,
    DWORD dwFlags);

ULONG
GetLocaleFileInfo(
    LCID Locale,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode);

ULONG
MakeCPHashNode(
    UINT CodePage,
    LPWORD pBaseAddr,
    PCP_HASH *ppNode,
    BOOL IsDLL,
    LPFN_CP_PROC pfnCPProc);

ULONG
MakeLangHashNode(
    LCID Locale,
    LPWORD pBaseAddr,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode);

ULONG
MakeLocHashNode(
    LCID Locale,
    LPWORD pBaseAddr,
    PLOC_HASH *ppNode,
    BOOLEAN fCreateNode);

PCP_HASH FASTCALL
GetCPHashNode(
    UINT CodePage);

PLOC_HASH FASTCALL
GetLangHashNode(
    LCID Locale,
    DWORD dwFlags);

PLOC_HASH FASTCALL
GetLocHashNode(
    LCID Locale);

ULONG
GetCalendar(
    CALID Calendar,
    PCAL_INFO *ppCalInfo);


//
//  Section Routines - section.c.
//
ULONG
CreateNlsObjectDirectory(void);

ULONG
CreateRegKey(
    PHANDLE phKeyHandle,
    LPWSTR pBaseName,
    LPWSTR pKey,
    ULONG fAccess);

ULONG
OpenRegKey(
    PHANDLE phKeyHandle,
    LPWSTR pBaseName,
    LPWSTR pKey,
    ULONG fAccess);

ULONG
QueryRegValue(
    HANDLE hKeyHandle,
    LPWSTR pValue,
    PKEY_VALUE_FULL_INFORMATION *ppKeyValueFull,
    ULONG Length,
    LPBOOL pIfAlloc);

ULONG
SetRegValue(
    HANDLE hKeyHandle,
    LPCWSTR pValue,
    LPCWSTR pData,
    ULONG DataLength);

ULONG
CreateSectionTemp(
    HANDLE *phSec,
    LPWSTR pwszFileName);

ULONG
OpenSection(
    HANDLE *phSec,
    PUNICODE_STRING pObSectionName,
    PVOID *ppBaseAddr,
    ULONG AccessMask,
    BOOL bCloseHandle);

ULONG
MapSection(
    HANDLE hSec,
    PVOID *ppBaseAddr,
    ULONG PageProtection,
    BOOL bCloseHandle);

ULONG
UnMapSection(
    PVOID pBaseAddr);

ULONG
GetNlsSectionName(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pwszPrefix,
    LPWSTR pwszSecName);

ULONG
GetCodePageDLLPathName(
    UINT CodePage,
    LPWSTR pDllName,
    USHORT cchLen);


//
//  Utility Routines - util.c.
//
BOOL GetUserInfoFromRegistry(
    LPWSTR pValue,
    LPWSTR pOutput);

int
GetNameFromStringTable(
    WORD ResourceID,
    LPWSTR *ppName);

BOOL
IsValidSeparatorString(
    LPCWSTR pString,
    ULONG MaxLength,
    BOOL fCheckZeroLen);

BOOL
IsValidGroupingString(
    LPCWSTR pString,
    ULONG MaxLength,
    BOOL fCheckZeroLen);

LPWORD
IsValidCalendarType(
    PLOC_HASH pHashN,
    CALID CalId);

LPWORD
IsValidCalendarTypeStr(
    PLOC_HASH pHashN,
    LPCWSTR pCalStr);

BOOL
GetUserInfo(
    LCID Locale,
    LCTYPE LCType ,
    LPWSTR pCacheString,
    LPWSTR pValue,
    LPWSTR pOutput,
    BOOL fCheckNull);

WCHAR FASTCALL
GetPreComposedChar(
    WCHAR wcNonSp,
    WCHAR wcBase);

BOOL FASTCALL
GetCompositeChars(
    WCHAR wch,
    WCHAR *pNonSp,
    WCHAR *pBase);

int FASTCALL
InsertPreComposedForm(
    LPCWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPWSTR pPreComp);

int FASTCALL
InsertFullWidthPreComposedForm(
    LPCWSTR pWCStr,
    LPWSTR pEndWCStr,
    LPWSTR pPreComp,
    PCASE pCase);

int FASTCALL
InsertCompositeForm(
    LPWSTR pWCStr,
    LPWSTR pEndWCStr);

ULONG
NlsConvertIntegerToString(
    UINT Value,
    UINT Base,
    UINT Padding,
    LPWSTR pResultBuf,
    UINT Size);

LPWSTR FASTCALL
NlsStrCpyW(
    LPWSTR pwszDest,
    LPCWSTR pwszSrc);

LPWSTR FASTCALL
NlsStrCatW(
    LPWSTR pwsz1,
    LPCWSTR pwsz2);

int FASTCALL
NlsStrLenW(
    LPCWSTR pwsz);

LPWSTR FASTCALL
NlsStrNCatW(
    LPWSTR pwszFront,
    LPCWSTR pwszBack,
    int Count);

int FASTCALL
NlsStrEqualW(
    LPCWSTR pwszFirst,
    LPCWSTR pwszSecond);

int FASTCALL
NlsStrNEqualW(
    LPCWSTR pwszFirst,
    LPCWSTR pwszSecond,
    int Count);

//
//  Security Routines - security.c.
//

NTSTATUS
NlsCheckForInteractiveUser();

NTSTATUS
NlsIsInteractiveUserProcess();

NTSTATUS
NlsGetUserLocale(
    LCID *Lcid);

NTSTATUS
NlsGetCurrentUserNlsInfo(
    LCID Locale,
    LCTYPE LCType,
    PWSTR RegistryValue,
    PWSTR pOutputBuffer,
    BOOL IgnoreLocaleValue);


NTSTATUS
NlsQueryCurrentUserInfo(
    PNLS_LOCAL_CACHE pNlsCache,
    LPWSTR pValue,
    LPWSTR pOutput);

NTSTATUS
NlsFlushProcessCache(
    LCTYPE LCType);


//
//  Internal Enumeration routines - enum.c.
//
BOOL
Internal_EnumSystemLanguageGroups(
    NLS_ENUMPROC lpLanguageGroupEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam,
    BOOL fUnicodeVer);

BOOL
Internal_EnumLanguageGroupLocales(
    NLS_ENUMPROC lpLangGroupLocaleEnumProc,
    LGRPID LanguageGroup,
    DWORD dwFlags,
    LONG_PTR lParam,
    BOOL fUnicodeVer);

BOOL
Internal_EnumUILanguages(
    NLS_ENUMPROC lpUILanguageEnumProc,
    DWORD dwFlags,
    LONG_PTR lParam,
    BOOL fUnicodeVer);

BOOL
Internal_EnumSystemLocales(
    NLS_ENUMPROC lpLocaleEnumProc,
    DWORD dwFlags,
    BOOL fUnicodeVer);

BOOL
Internal_EnumSystemCodePages(
    NLS_ENUMPROC lpCodePageEnumProc,
    DWORD dwFlags,
    BOOL fUnicodeVer);

BOOL
Internal_EnumCalendarInfo(
    NLS_ENUMPROC lpCalInfoEnumProc,
    LCID Locale,
    CALID Calendar,
    CALTYPE CalType,
    BOOL fUnicodeVer,
    BOOL fExVersion);

BOOL
Internal_EnumTimeFormats(
    NLS_ENUMPROC lpTimeFmtEnumProc,
    LCID Locale,
    DWORD dwFlags,
    BOOL fUnicodeVer);

BOOL
Internal_EnumDateFormats(
    NLS_ENUMPROC lpDateFmtEnumProc,
    LCID Locale,
    DWORD dwFlags,
    BOOL fUnicodeVer,
    BOOL fExVersion);


//
//  Ansi routines - ansi.c.
//
BOOL
NlsDispatchAnsiEnumProc(
    LCID Locale,
    NLS_ENUMPROC pNlsEnumProc,
    DWORD dwFlags,
    LPWSTR pUnicodeBuffer1,
    LPWSTR pUnicodeBuffer2,
    DWORD dwValue1,
    DWORD dwValue2,
    LONG_PTR lParam,
    BOOL fVersion);


//
//  Translation Routines - mbcs.c.
//
int
SpecialMBToWC(
    PCP_HASH pHashN,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar);


//
//  UTF Translation Routines - utf.c.
//
BOOL
UTFCPInfo(
    UINT CodePage,
    LPCPINFO lpCPInfo,
    BOOL fExVer);

int
UTFToUnicode(
    UINT CodePage,
    DWORD dwFlags,
    LPCSTR lpMultiByteStr,
    int cbMultiByte,
    LPWSTR lpWideCharStr,
    int cchWideChar);

int
UnicodeToUTF(
    UINT CodePage,
    DWORD dwFlags,
    LPCWSTR lpWideCharStr,
    int cchWideChar,
    LPSTR lpMultiByteStr,
    int cbMultiByte,
    LPCSTR lpDefaultChar,
    LPBOOL lpUsedDefaultChar);


//
// Locale/Calendar Info (locale.c)
//
BOOL
GetTwoDigitYearInfo(
    CALID Calendar,
    LPWSTR pYearInfo,
    PWSTR pwszKeyPath);


////////////////////////////////////////////////////////////////////////////
//
//  Global Variables
//
//  All of the global variables for the NLSAPI should be put here.  These are
//  all instance-specific.  In general, there shouldn't be much reason to
//  create instance globals.
//
//  Globals are included last because they may require some of the types
//  being defined above.
//
////////////////////////////////////////////////////////////////////////////

extern PTBL_PTRS        pTblPtrs;           // ptr to structure of table ptrs
extern HANDLE           hModule;            // handle to module
extern RTL_CRITICAL_SECTION gcsTblPtrs;     // critical section for tbl ptrs

extern UINT             gAnsiCodePage;      // Ansi code page value
extern UINT             gOemCodePage;       // OEM code page value
extern UINT             gMacCodePage;       // MAC code page value
extern LCID             gSystemLocale;      // system locale value
extern LANGID           gSystemInstallLang; // system's original install language
extern PLOC_HASH        gpSysLocHashN;      // ptr to system loc hash node
extern PCP_HASH         gpACPHashN;         // ptr to ACP hash node
extern PCP_HASH         gpOEMCPHashN;       // ptr to OEMCP hash node
extern PCP_HASH         gpMACCPHashN;       // ptr to MAC hash node

extern HANDLE           hCodePageKey;       // handle to System\Nls\CodePage key
extern HANDLE           hLocaleKey;         // handle to System\Nls\Locale key
extern HANDLE           hAltSortsKey;       // handle to Locale\Alternate Sorts key
extern HANDLE           hLangGroupsKey;     // handle to System\Nls\Language Groups key

extern PNLS_USER_INFO   pNlsUserInfo;       // ptr to the user info cache
extern HANDLE           hNlsCacheMutant;    // handle to cache semaphore

extern BOOL gInteractiveLogonUserProcess;    // running in interactive user session or not.
extern RTL_CRITICAL_SECTION  gcsNlsProcessCache; // Nls process cache critical section

////////////////////////////////////////////////////////////////////////////////////
//
//   Functions used to communicate with CSRSS.
//
////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
CsrBasepNlsSetUserInfo(
    IN LCTYPE   LCType,
    IN LPWSTR pData,
    IN ULONG DataLength
    );

NTSTATUS
CsrBasepNlsSetMultipleUserInfo(
    IN DWORD dwFlags,
    IN int cchData,
    IN LPCWSTR pPicture,
    IN LPCWSTR pSeparator,
    IN LPCWSTR pOrder,
    IN LPCWSTR pTLZero,
    IN LPCWSTR pTimeMarkPosn
    );

NTSTATUS
CsrBasepNlsCreateSection(
    IN UINT uiType,
    IN LCID Locale,
    OUT PHANDLE phSection
    );

NTSTATUS
CsrBasepNlsUpdateCacheCount(
    VOID
    );

#endif   // _NLS_
