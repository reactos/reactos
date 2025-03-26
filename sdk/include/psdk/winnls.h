#ifndef _WINNLS_
#define _WINNLS_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4820)
#endif

#define GEOID_NOT_AVAILABLE (-1)
#define MAX_LEADBYTES 	12
#define MAX_DEFAULTCHAR	2

#define LOCALE_ALL 0x00

#define LOCALE_NOUSEROVERRIDE	0x80000000
#define LOCALE_USE_CP_ACP	0x40000000
#if (WINVER >= 0x0400)
#define LOCALE_RETURN_NUMBER	0x20000000
#endif
#define LOCALE_RETURN_GENITIVE_NAMES  0x10000000
#define LOCALE_ILANGUAGE	1
#define LOCALE_SLANGUAGE	2
#define LOCALE_SENGLANGUAGE	0x1001
#define LOCALE_SENGLISHLANGUAGENAME 0x1001
#define LOCALE_SABBREVLANGNAME	3
#define LOCALE_SNATIVELANGNAME	4
#define LOCALE_ICOUNTRY	5
#define LOCALE_SCOUNTRY	6
#define LOCALE_SENGCOUNTRY	0x1002
#define LOCALE_SENGLISHCOUNTRYNAME  0x1002
#define LOCALE_SABBREVCTRYNAME	7
#define LOCALE_SNATIVECTRYNAME	8
#define LOCALE_IDEFAULTLANGUAGE	9
#define LOCALE_IDEFAULTCOUNTRY	10
#define LOCALE_IDEFAULTCODEPAGE	11
#define LOCALE_IDEFAULTANSICODEPAGE 0x1004
#define LOCALE_IDEFAULTMACCODEPAGE 0x1011
#define LOCALE_SLIST	12
#define LOCALE_IMEASURE	13
#define LOCALE_SDECIMAL	14
#define LOCALE_STHOUSAND	15
#define LOCALE_SGROUPING	16
#define LOCALE_IDIGITS	17
#define LOCALE_ILZERO	18
#define LOCALE_INEGNUMBER	0x1010
#define LOCALE_SNATIVEDIGITS	19
#define LOCALE_SCURRENCY	20
#define LOCALE_SINTLSYMBOL	21
#define LOCALE_SMONDECIMALSEP	22
#define LOCALE_SMONTHOUSANDSEP	23
#define LOCALE_SMONGROUPING	24
#define LOCALE_ICURRDIGITS	25
#define LOCALE_IINTLCURRDIGITS	26
#define LOCALE_ICURRENCY	27
#define LOCALE_INEGCURR	28
#define LOCALE_SDATE	29
#define LOCALE_STIME	30
#define LOCALE_SSHORTDATE	31
#define LOCALE_SLONGDATE	32
#define LOCALE_STIMEFORMAT	0x1003
#define LOCALE_IDATE	33
#define LOCALE_ILDATE	34
#define LOCALE_ITIME	35
#define LOCALE_ITIMEMARKPOSN	0x1005
#define LOCALE_ICENTURY	36
#define LOCALE_ITLZERO	37
#define LOCALE_IDAYLZERO	38
#define LOCALE_IMONLZERO	39
#define LOCALE_S1159	40
#define LOCALE_S2359	41
#define LOCALE_ICALENDARTYPE	0x1009
#define LOCALE_IOPTIONALCALENDAR	0x100B
#define LOCALE_IFIRSTDAYOFWEEK	0x100C
#define LOCALE_IFIRSTWEEKOFYEAR	0x100D
#define LOCALE_SDAYNAME1	42
#define LOCALE_SDAYNAME2	43
#define LOCALE_SDAYNAME3	44
#define LOCALE_SDAYNAME4	45
#define LOCALE_SDAYNAME5	46
#define LOCALE_SDAYNAME6	47
#define LOCALE_SDAYNAME7	48
#define LOCALE_SABBREVDAYNAME1	49
#define LOCALE_SABBREVDAYNAME2	50
#define LOCALE_SABBREVDAYNAME3	51
#define LOCALE_SABBREVDAYNAME4	52
#define LOCALE_SABBREVDAYNAME5	53
#define LOCALE_SABBREVDAYNAME6	54
#define LOCALE_SABBREVDAYNAME7	55
#define LOCALE_SMONTHNAME1	56
#define LOCALE_SMONTHNAME2	57
#define LOCALE_SMONTHNAME3	58
#define LOCALE_SMONTHNAME4	59
#define LOCALE_SMONTHNAME5	60
#define LOCALE_SMONTHNAME6	61
#define LOCALE_SMONTHNAME7	62
#define LOCALE_SMONTHNAME8	63
#define LOCALE_SMONTHNAME9	64
#define LOCALE_SMONTHNAME10	65
#define LOCALE_SMONTHNAME11	66
#define LOCALE_SMONTHNAME12	67
#define LOCALE_SMONTHNAME13	0x100E
#define LOCALE_SABBREVMONTHNAME1	68
#define LOCALE_SABBREVMONTHNAME2	69
#define LOCALE_SABBREVMONTHNAME3	70
#define LOCALE_SABBREVMONTHNAME4	71
#define LOCALE_SABBREVMONTHNAME5	72
#define LOCALE_SABBREVMONTHNAME6	73
#define LOCALE_SABBREVMONTHNAME7	74
#define LOCALE_SABBREVMONTHNAME8	75
#define LOCALE_SABBREVMONTHNAME9	76
#define LOCALE_SABBREVMONTHNAME10	77
#define LOCALE_SABBREVMONTHNAME11	78
#define LOCALE_SABBREVMONTHNAME12	79
#define LOCALE_SABBREVMONTHNAME13	0x100F
#define LOCALE_SPOSITIVESIGN	80
#define LOCALE_SNEGATIVESIGN	81
#define LOCALE_IPOSSIGNPOSN         82
#define LOCALE_INEGSIGNPOSN         83
#define LOCALE_IPOSSYMPRECEDES      84
#define LOCALE_IPOSSEPBYSPACE       85
#define LOCALE_INEGSYMPRECEDES      86
#define LOCALE_INEGSEPBYSPACE       87
#define LOCALE_FONTSIGNATURE        88
#define LOCALE_SISO639LANGNAME      89
#define LOCALE_SISO3166CTRYNAME     90

/* FIXME: This value should be in the guarded block below */
#define LOCALE_SNAME                92

#if (WINVER >= 0x0600) || (defined(__REACTOS__) && defined(_KERNEL32_))
#define LOCALE_IGEOID               91
#define LOCALE_SNAME                92
#define LOCALE_SDURATION            93
#define LOCALE_SKEYBOARDSTOINSTALL  94
#define LOCALE_SSHORTESTDAYNAME1    96
#define LOCALE_SSHORTESTDAYNAME2    97
#define LOCALE_SSHORTESTDAYNAME3    98
#define LOCALE_SSHORTESTDAYNAME4    99
#define LOCALE_SSHORTESTDAYNAME5    100
#define LOCALE_SSHORTESTDAYNAME6    101
#define LOCALE_SSHORTESTDAYNAME7    102
#define LOCALE_SISO639LANGNAME2     103
#define LOCALE_SISO3166CTRYNAME2    104
#define LOCALE_SNAN                 105
#define LOCALE_SPOSINFINITY         106
#define LOCALE_SNEGINFINITY         107
#define LOCALE_SSCRIPTS             108
#define LOCALE_SPARENT              109
#define LOCALE_SCONSOLEFALLBACKNAME 110
#endif /* (WINVER >= 0x0600) */

//#if (WINVER >= _WIN32_WINNT_WIN7)
#define LOCALE_IREADINGLAYOUT       0x0070
#define LOCALE_INEUTRAL             0x0071
#define LOCALE_SNATIVEDISPLAYNAME   0x0073
#define LOCALE_INEGATIVEPERCENT     0x0074
#define LOCALE_IPOSITIVEPERCENT     0x0075
#define LOCALE_SPERCENT             0x0076
#define LOCALE_SPERMILLE            0x0077
#define LOCALE_SMONTHDAY            0x0078
#define LOCALE_SSHORTTIME           0x0079
#define LOCALE_SOPENTYPELANGUAGETAG 0x007a
#define LOCALE_SSORTLOCALE          0x007b
//#endif /* (WINVER >= _WIN32_WINNT_WIN7) */

#if (WINVER >= 0x0600)
#define LOCALE_NAME_USER_DEFAULT    NULL
#define LOCALE_NAME_INVARIANT      L""
#define LOCALE_NAME_SYSTEM_DEFAULT      L"!sys-default-locale"
#endif

#define LOCALE_IDEFAULTUNIXCODEPAGE   0x1030 /* Wine extension */

#define NORM_IGNORECASE	1
#define NORM_IGNOREKANATYPE	65536
#define NORM_IGNORENONSPACE	2
#define NORM_IGNORESYMBOLS	4
#define NORM_IGNOREWIDTH	131072
#define LINGUISTIC_IGNORECASE 0x00000010
#define NORM_LINGUISTIC_CASING 0x08000000
#define SORT_STRINGSORT	4096
#define LCMAP_LOWERCASE 0x00000100
#define LCMAP_UPPERCASE 0x00000200
#define LCMAP_SORTKEY 0x00000400
#define LCMAP_BYTEREV 0x00000800
#define LCMAP_HIRAGANA 0x00100000
#define LCMAP_KATAKANA 0x00200000
#define LCMAP_HALFWIDTH 0x00400000
#define LCMAP_FULLWIDTH 0x00800000
#define LCMAP_LINGUISTIC_CASING 0x01000000
#define LCMAP_SIMPLIFIED_CHINESE 0x02000000
#define LCMAP_TRADITIONAL_CHINESE 0x04000000
#define ENUM_ALL_CALENDARS (-1)
#define DATE_SHORTDATE 1
#define DATE_LONGDATE 2
#define DATE_USE_ALT_CALENDAR 4
#define CP_INSTALLED 1
#define CP_SUPPORTED 2
#define LCID_INSTALLED 1
#define LCID_SUPPORTED 2
#define LCID_ALTERNATE_SORTS 4

#define LOCALE_ALL                  0x00
#define LOCALE_WINDOWS              0x01
#define LOCALE_SUPPLEMENTAL         0x02
#define LOCALE_ALTERNATE_SORTS      0x04
#define LOCALE_REPLACEMENT          0x08
#define LOCALE_NEUTRALDATA          0x10
#define LOCALE_SPECIFICDATA         0x20

#define MAP_FOLDCZONE 16
#define MAP_FOLDDIGITS 128
#define MAP_PRECOMPOSED 32
#define MAP_COMPOSITE 64

#define WC_DISCARDNS         0x0010
#define WC_SEPCHARS          0x0020
#define WC_DEFAULTCHAR       0x0040
#define WC_ERR_INVALID_CHARS 0x0080
#define WC_COMPOSITECHECK    0x0200
#if (WINVER >= 0x0500)
#define WC_NO_BEST_FIT_CHARS 0x0400
#endif

#define CP_ACP 0
#ifdef _WINE
#define CP_UNIXCP CP_ACP
#endif
#define CP_OEMCP 1
#define CP_MACCP 2
#define CP_THREAD_ACP 3
#define CP_SYMBOL 42
#define CP_UTF7 65000
#define CP_UTF8 65001
#define CT_CTYPE1 1
#define CT_CTYPE2 2
#define CT_CTYPE3 4
#define C1_UPPER 1
#define C1_LOWER 2
#define C1_DIGIT 4
#define C1_SPACE 8
#define C1_PUNCT 16
#define C1_CNTRL 32
#define C1_BLANK 64
#define C1_XDIGIT 128
#define C1_ALPHA 256
#define C1_DEFINED 512
#define C2_LEFTTORIGHT 1
#define C2_RIGHTTOLEFT 2
#define C2_EUROPENUMBER 3
#define C2_EUROPESEPARATOR 4
#define C2_EUROPETERMINATOR 5
#define C2_ARABICNUMBER 6
#define C2_COMMONSEPARATOR 7
#define C2_BLOCKSEPARATOR 8
#define C2_SEGMENTSEPARATOR 9
#define C2_WHITESPACE 10
#define C2_OTHERNEUTRAL 11
#define C2_NOTAPPLICABLE 0
#define C3_NONSPACING 1
#define C3_DIACRITIC 2
#define C3_VOWELMARK 4
#define C3_SYMBOL 8
#define C3_KATAKANA 16
#define C3_HIRAGANA 32
#define C3_HALFWIDTH 64
#define C3_FULLWIDTH 128
#define C3_IDEOGRAPH 256
#define C3_KASHIDA 512
#define C3_LEXICAL 1024
#define C3_ALPHA 32768
#define C3_NOTAPPLICABLE 0
#define C3_HIGHSURROGATE 0x0800
#define C3_LOWSURROGATE 0x1000
#define TIME_NOMINUTESORSECONDS 1
#define TIME_NOSECONDS 2
#define TIME_NOTIMEMARKER 4
#define TIME_FORCE24HOURFORMAT 8
#define MB_PRECOMPOSED 1
#define MB_COMPOSITE 2
#define MB_ERR_INVALID_CHARS 8
#define MB_USEGLYPHCHARS 4
#define CTRY_DEFAULT 0
#define CTRY_ALBANIA 355
#define CTRY_ALGERIA 213
#define CTRY_ARGENTINA 54
#define CTRY_ARMENIA 374
#define CTRY_AUSTRALIA 61
#define CTRY_AUSTRIA 43
#define CTRY_AZERBAIJAN 994
#define CTRY_BAHRAIN 973
#define CTRY_BELARUS 375
#define CTRY_BELGIUM 32
#define CTRY_BELIZE 501
#define CTRY_BOLIVIA 591
#define CTRY_BRAZIL 55
#define CTRY_BRUNEI_DARUSSALAM 673
#define CTRY_BULGARIA 359
#define CTRY_CANADA 2
#define CTRY_CARIBBEAN 1
#define CTRY_CHILE 56
#define CTRY_COLOMBIA 57
#define CTRY_COSTA_RICA 506
#define CTRY_CROATIA 385
#define CTRY_CZECH 420
#define CTRY_DENMARK 45
#define CTRY_DOMINICAN_REPUBLIC 1
#define CTRY_ECUADOR 593
#define CTRY_EGYPT 20
#define CTRY_EL_SALVADOR 503
#define CTRY_ESTONIA 372
#define CTRY_FAEROE_ISLANDS 298
#define CTRY_FINLAND 358
#define CTRY_FRANCE 33
#define CTRY_GEORGIA 995
#define CTRY_GERMANY 49
#define CTRY_GREECE 30
#define CTRY_GUATEMALA 502
#define CTRY_HONDURAS 504
#define CTRY_HONG_KONG 852
#define CTRY_HUNGARY 36
#define CTRY_ICELAND 354
#define CTRY_INDIA 91
#define CTRY_INDONESIA 62
#define CTRY_IRAN 981
#define CTRY_IRAQ 964
#define CTRY_IRELAND 353
#define CTRY_ISRAEL 972
#define CTRY_ITALY 39
#define CTRY_JAMAICA 1
#define CTRY_JAPAN 81
#define CTRY_JORDAN 962
#define CTRY_KAZAKSTAN 7
#define CTRY_KENYA 254
#define CTRY_KUWAIT 965
#define CTRY_KYRGYZSTAN 996
#define CTRY_LATVIA 371
#define CTRY_LEBANON 961
#define CTRY_LIBYA 218
#define CTRY_LIECHTENSTEIN 41
#define CTRY_LITHUANIA 370
#define CTRY_LUXEMBOURG 352
#define CTRY_MACAU 853
#define CTRY_MACEDONIA 389
#define CTRY_MALAYSIA 60
#define CTRY_MALDIVES 960
#define CTRY_MEXICO 52
#define CTRY_MONACO 33
#define CTRY_MONGOLIA 976
#define CTRY_MOROCCO 212
#define CTRY_NETHERLANDS 31
#define CTRY_NEW_ZEALAND 64
#define CTRY_NICARAGUA 505
#define CTRY_NORWAY 47
#define CTRY_OMAN 968
#define CTRY_PAKISTAN 92
#define CTRY_PANAMA 507
#define CTRY_PARAGUAY 595
#define CTRY_PERU 51
#define CTRY_PHILIPPINES 63
#define CTRY_POLAND 48
#define CTRY_PORTUGAL 351
#define CTRY_PRCHINA 86
#define CTRY_PUERTO_RICO 1
#define CTRY_QATAR 974
#define CTRY_ROMANIA 40
#define CTRY_RUSSIA 7
#define CTRY_SAUDI_ARABIA 966
#define CTRY_SERBIA 381
#define CTRY_SINGAPORE 65
#define CTRY_SLOVAK 421
#define CTRY_SLOVENIA 386
#define CTRY_SOUTH_AFRICA 27
#define CTRY_SOUTH_KOREA 82
#define CTRY_SPAIN 34
#define CTRY_SWEDEN 46
#define CTRY_SWITZERLAND 41
#define CTRY_SYRIA 963
#define CTRY_TAIWAN 886
#define CTRY_TATARSTAN 7
#define CTRY_THAILAND 66
#define CTRY_TRINIDAD_Y_TOBAGO 1
#define CTRY_TUNISIA 216
#define CTRY_TURKEY 90
#define CTRY_UAE 971
#define CTRY_UKRAINE 380
#define CTRY_UNITED_KINGDOM 44
#define CTRY_UNITED_STATES 1
#define CTRY_URUGUAY 598
#define CTRY_UZBEKISTAN 7
#define CTRY_VENEZUELA 58
#define CTRY_VIET_NAM 84
#define CTRY_YEMEN 967
#define CTRY_ZIMBABWE 263
#define CAL_ICALINTVALUE 1
#define CAL_SCALNAME 2
#define CAL_IYEAROFFSETRANGE 3
#define CAL_SERASTRING 4
#define CAL_SSHORTDATE 5
#define CAL_SLONGDATE 6
#define CAL_SDAYNAME1 7
#define CAL_SDAYNAME2 8
#define CAL_SDAYNAME3 9
#define CAL_SDAYNAME4 10
#define CAL_SDAYNAME5 11
#define CAL_SDAYNAME6 12
#define CAL_SDAYNAME7 13
#define CAL_SABBREVDAYNAME1 14
#define CAL_SABBREVDAYNAME2 15
#define CAL_SABBREVDAYNAME3 16
#define CAL_SABBREVDAYNAME4 17
#define CAL_SABBREVDAYNAME5 18
#define CAL_SABBREVDAYNAME6 19
#define CAL_SABBREVDAYNAME7 20
#define CAL_SMONTHNAME1 21
#define CAL_SMONTHNAME2 22
#define CAL_SMONTHNAME3 23
#define CAL_SMONTHNAME4 24
#define CAL_SMONTHNAME5 25
#define CAL_SMONTHNAME6 26
#define CAL_SMONTHNAME7 27
#define CAL_SMONTHNAME8 28
#define CAL_SMONTHNAME9 29
#define CAL_SMONTHNAME10 30
#define CAL_SMONTHNAME11 31
#define CAL_SMONTHNAME12 32
#define CAL_SMONTHNAME13 33
#define CAL_SABBREVMONTHNAME1 34
#define CAL_SABBREVMONTHNAME2 35
#define CAL_SABBREVMONTHNAME3 36
#define CAL_SABBREVMONTHNAME4 37
#define CAL_SABBREVMONTHNAME5 38
#define CAL_SABBREVMONTHNAME6 39
#define CAL_SABBREVMONTHNAME7 40
#define CAL_SABBREVMONTHNAME8 41
#define CAL_SABBREVMONTHNAME9 42
#define CAL_SABBREVMONTHNAME10 43
#define CAL_SABBREVMONTHNAME11 44
#define CAL_SABBREVMONTHNAME12 45
#define CAL_SABBREVMONTHNAME13 46
#define CAL_GREGORIAN 1
#define CAL_GREGORIAN_US 2
#define CAL_JAPAN 3
#define CAL_TAIWAN 4
#define CAL_KOREA 5
#define CAL_HIJRI 6
#define CAL_THAI 7
#define CAL_HEBREW 8
#define CAL_GREGORIAN_ME_FRENCH 9
#define CAL_GREGORIAN_ARABIC 10
#define CAL_GREGORIAN_XLIT_ENGLISH 11
#define CAL_GREGORIAN_XLIT_FRENCH 12
#define CSTR_LESS_THAN 1
#define CSTR_EQUAL 2
#define CSTR_GREATER_THAN 3
#define LGRPID_INSTALLED 1
#define LGRPID_SUPPORTED 2
#define LGRPID_WESTERN_EUROPE 1
#define LGRPID_CENTRAL_EUROPE 2
#define LGRPID_BALTIC 3
#define LGRPID_GREEK 4
#define LGRPID_CYRILLIC 5
#define LGRPID_TURKISH 6
#define LGRPID_JAPANESE 7
#define LGRPID_KOREAN 8
#define LGRPID_TRADITIONAL_CHINESE 9
#define LGRPID_SIMPLIFIED_CHINESE 10
#define LGRPID_THAI 11
#define LGRPID_HEBREW 12
#define LGRPID_ARABIC 13
#define LGRPID_VIETNAMESE 14
#define LGRPID_INDIC 15
#define LGRPID_GEORGIAN 16
#define LGRPID_ARMENIAN 17

#if (WINVER >= 0x0500)
#define LOCALE_SYEARMONTH 0x1006
#define LOCALE_SENGCURRNAME 0x1007
#define LOCALE_SNATIVECURRNAME 0x1008
#define LOCALE_IDEFAULTEBCDICCODEPAGE 0x1012
#define LOCALE_SSORTNAME 0x1013
#define LOCALE_IDIGITSUBSTITUTION 0x1014
#define LOCALE_IPAPERSIZE 0x100A
#define DATE_YEARMONTH 8
#define DATE_LTRREADING 16
#define DATE_RTLREADING 32
#define MAP_EXPAND_LIGATURES   0x2000
#define CAL_SYEARMONTH 47
#define CAL_ITWODIGITYEARMAX 48
#define CAL_NOUSEROVERRIDE LOCALE_NOUSEROVERRIDE
#define CAL_RETURN_NUMBER LOCALE_RETURN_NUMBER
#define CAL_USE_CP_ACP LOCALE_USE_CP_ACP
#endif /* (WINVER >= 0x0500) */
#if WINVER >= 0x0600
#define IDN_ALLOW_UNASSIGNED 0x1
#define IDN_USE_STD3_ASCII_RULES 0x2
#define VS_ALLOW_LATIN 0x1
#define GSS_ALLOW_INHERITED_COMMON 0x1
#endif
#ifndef  _BASETSD_H_
typedef long LONG_PTR;
#endif

#if (WINVER >= 0x0600)
#define MUI_FULL_LANGUAGE             0x01
#define MUI_LANGUAGE_ID               0x04
#define MUI_LANGUAGE_NAME             0x08
#define MUI_MERGE_SYSTEM_FALLBACK     0x10
#define MUI_MERGE_USER_FALLBACK       0x20
#define MUI_UI_FALLBACK               MUI_MERGE_SYSTEM_FALLBACK | MUI_MERGE_USER_FALLBACK
#define MUI_MACHINE_LANGUAGE_SETTINGS 0x400
#endif /* (WINVER >= 0x0600) */

#ifndef RC_INVOKED
typedef DWORD LCTYPE;
typedef DWORD CALTYPE;
typedef DWORD CALID;
typedef DWORD LGRPID;
typedef DWORD GEOID;
typedef DWORD GEOTYPE;
typedef DWORD GEOCLASS;
typedef BOOL (CALLBACK *CALINFO_ENUMPROCEXEX)(LPWSTR, CALID, LPWSTR, LPARAM);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCEXEX)(LPWSTR, CALID, LPARAM);
typedef BOOL (CALLBACK *TIMEFMT_ENUMPROCEX)(LPWSTR, LPARAM);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCEXA)(LPSTR, CALID);
typedef BOOL (CALLBACK *CALINFO_ENUMPROCEXW)(LPWSTR, CALID);
typedef BOOL (CALLBACK *LANGUAGEGROUP_ENUMPROCA)(LGRPID, LPSTR, LPSTR, DWORD, LONG_PTR);
typedef BOOL (CALLBACK *LANGUAGEGROUP_ENUMPROCW)(LGRPID, LPWSTR, LPWSTR, DWORD, LONG_PTR);
typedef BOOL (CALLBACK *LANGGROUPLOCALE_ENUMPROCA)(LGRPID, LCID, LPSTR, LONG_PTR);
typedef BOOL (CALLBACK *LANGGROUPLOCALE_ENUMPROCW)(LGRPID, LCID, LPWSTR, LONG_PTR);
typedef BOOL (CALLBACK *UILANGUAGE_ENUMPROCW)(LPWSTR, LONG_PTR);
typedef BOOL (CALLBACK *UILANGUAGE_ENUMPROCA)(LPSTR, LONG_PTR);
typedef BOOL (CALLBACK *LOCALE_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK *LOCALE_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK *LOCALE_ENUMPROCEX)(LPWSTR, DWORD, LPARAM);
typedef BOOL (CALLBACK *CODEPAGE_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK *CODEPAGE_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCEXA)(LPSTR, CALID);
typedef BOOL (CALLBACK *DATEFMT_ENUMPROCEXW)(LPWSTR, CALID);
typedef BOOL (CALLBACK *TIMEFMT_ENUMPROCA)(LPSTR);
typedef BOOL (CALLBACK *TIMEFMT_ENUMPROCW)(LPWSTR);
typedef BOOL (CALLBACK *GEO_ENUMPROC)(GEOID);

enum NLS_FUNCTION {
	COMPARE_STRING = 0x0001
};
typedef enum NLS_FUNCTION NLS_FUNCTION;
enum SYSGEOCLASS {
	GEOCLASS_NATION = 16,
	GEOCLASS_REGION = 14
};

/* Geographic Information types */
enum SYSGEOTYPE
{
    GEO_NATION = 1,
    GEO_LATITUDE,
    GEO_LONGITUDE,
    GEO_ISO2,
    GEO_ISO3,
    GEO_RFC1766,
    GEO_LCID,
    GEO_FRIENDLYNAME,
    GEO_OFFICIALNAME,
    GEO_TIMEZONES,
    GEO_OFFICIALLANGUAGES,
    GEO_ISO_UN_NUMBER,
    GEO_PARENT,
    GEO_DIALINGCODE,
    GEO_CURRENCYCODE,
    GEO_CURRENCYSYMBOL
};

typedef struct _cpinfo {
	UINT MaxCharSize;
	BYTE DefaultChar[MAX_DEFAULTCHAR];
	BYTE LeadByte[MAX_LEADBYTES];
} CPINFO,*LPCPINFO;
typedef struct _cpinfoexA {
	UINT MaxCharSize;
	BYTE DefaultChar[MAX_DEFAULTCHAR];
	BYTE LeadByte[MAX_LEADBYTES];
	WCHAR UnicodeDefaultChar;
	UINT CodePage;
	CHAR CodePageName[MAX_PATH];
} CPINFOEXA,*LPCPINFOEXA;
typedef struct _cpinfoexW {
	UINT MaxCharSize;
	BYTE DefaultChar[MAX_DEFAULTCHAR];
	BYTE LeadByte[MAX_LEADBYTES];
	WCHAR UnicodeDefaultChar;
	UINT CodePage;
	WCHAR CodePageName[MAX_PATH];
} CPINFOEXW,*LPCPINFOEXW;
typedef struct _currencyfmtA {
	UINT NumDigits;
	UINT LeadingZero;
	UINT Grouping;
	LPSTR lpDecimalSep;
	LPSTR lpThousandSep;
	UINT NegativeOrder;
	UINT PositiveOrder;
	LPSTR lpCurrencySymbol;
} CURRENCYFMTA,*LPCURRENCYFMTA;
typedef struct _currencyfmtW {
	UINT NumDigits;
	UINT LeadingZero;
	UINT Grouping;
	LPWSTR lpDecimalSep;
	LPWSTR lpThousandSep;
	UINT NegativeOrder;
	UINT PositiveOrder;
	LPWSTR lpCurrencySymbol;
} CURRENCYFMTW,*LPCURRENCYFMTW;
typedef struct nlsversioninfo {
	DWORD dwNLSVersionInfoSize;
	DWORD dwNLSVersion;
	DWORD dwDefinedVersion;
} NLSVERSIONINFO,*LPNLSVERSIONINFO;
typedef struct _nlsversioninfoex {
    DWORD dwNLSVersionInfoSize;
    DWORD dwNLSVersion;
    DWORD dwDefinedVersion;
    DWORD dwEffectiveId;
    GUID  guidCustomVersion;
} NLSVERSIONINFOEX, *LPNLSVERSIONINFOEX;
typedef struct _numberfmtA {
	UINT NumDigits;
	UINT LeadingZero;
	UINT Grouping;
	LPSTR lpDecimalSep;
	LPSTR lpThousandSep;
	UINT NegativeOrder;
} NUMBERFMTA,*LPNUMBERFMTA;
typedef struct _numberfmtW {
	UINT NumDigits;
	UINT LeadingZero;
	UINT Grouping;
	LPWSTR lpDecimalSep;
	LPWSTR lpThousandSep;
	UINT NegativeOrder;
} NUMBERFMTW,*LPNUMBERFMTW;
#if (WINVER >= 0x0600)
typedef enum _NORM_FORM {
	NormalizationOther = 0,
	NormalizationC = 0x1,
	NormalizationD = 0x2,
	NormalizationKC = 0x5,
	NormalizationKD = 0x6
} NORM_FORM;
#endif /* (WINVER >= 0x0600) */
typedef struct _FILEMUIINFO {
    DWORD dwSize;
    DWORD dwVersion;
    DWORD dwFileType;
    BYTE pChecksum[16];
    BYTE pServiceChecksum[16];
    DWORD dwLanguageNameOffset;
    DWORD dwTypeIDMainSize;
    DWORD dwTypeIDMainOffset;
    DWORD dwTypeNameMainOffset;
    DWORD dwTypeIDMUISize;
    DWORD dwTypeIDMUIOffset;
    DWORD dwTypeNameMUIOffset;
    BYTE abBuffer[8];
} FILEMUIINFO, *PFILEMUIINFO;

#define HIGH_SURROGATE_START 0xd800
#define HIGH_SURROGATE_END   0xdbff
#define LOW_SURROGATE_START  0xdc00
#define LOW_SURROGATE_END    0xdfff

#define IS_HIGH_SURROGATE(ch) ((ch) >= HIGH_SURROGATE_START && (ch) <= HIGH_SURROGATE_END)
#define IS_LOW_SURROGATE(ch) ((ch) >= LOW_SURROGATE_START  && (ch) <= LOW_SURROGATE_END)
#define IS_SURROGATE_PAIR(high,low) (IS_HIGH_SURROGATE(high) && IS_LOW_SURROGATE(low))

int
WINAPI
CompareStringA(
  _In_ LCID Locale,
  _In_ DWORD dwCmpFlags,
  _In_reads_(cchCount1) LPCSTR lpString1,
  _In_ int cchCount1,
  _In_reads_(cchCount2) LPCSTR lpString2,
  _In_ int cchCount2);

int
WINAPI
CompareStringW(
  _In_ LCID Locale,
  _In_ DWORD dwCmpFlags,
  _In_reads_(cchCount1) LPCWSTR lpString1,
  _In_ int cchCount1,
  _In_reads_(cchCount2) LPCWSTR lpString2,
  _In_ int cchCount2);

LCID WINAPI ConvertDefaultLocale(_In_ LCID);
BOOL WINAPI EnumCalendarInfoA(_In_ CALINFO_ENUMPROCA, _In_ LCID, _In_ CALID, _In_ CALTYPE);
BOOL WINAPI EnumCalendarInfoW(_In_ CALINFO_ENUMPROCW, _In_ LCID, _In_ CALID, _In_ CALTYPE);
BOOL WINAPI EnumDateFormatsA(_In_ DATEFMT_ENUMPROCA, _In_ LCID, _In_ DWORD);
BOOL WINAPI EnumDateFormatsW(_In_ DATEFMT_ENUMPROCW, _In_ LCID, _In_ DWORD);
BOOL WINAPI EnumSystemCodePagesA(_In_ CODEPAGE_ENUMPROCA, _In_ DWORD);
BOOL WINAPI EnumSystemCodePagesW(_In_ CODEPAGE_ENUMPROCW, _In_ DWORD);
BOOL WINAPI EnumSystemGeoID(_In_ GEOCLASS, _In_ GEOID, _In_ GEO_ENUMPROC);
BOOL WINAPI EnumSystemLocalesA(_In_ LOCALE_ENUMPROCA, _In_ DWORD);
BOOL WINAPI EnumSystemLocalesW(_In_ LOCALE_ENUMPROCW, _In_ DWORD);

typedef BOOL (CALLBACK* LOCALE_ENUMPROCEX)(LPWSTR, DWORD, LPARAM);

WINBASEAPI
BOOL
WINAPI
EnumSystemLocalesEx(
  _In_ LOCALE_ENUMPROCEX lpLocaleEnumProcEx,
  _In_ DWORD dwFlags,
  _In_ LPARAM lParam,
  _In_opt_ LPVOID lpReserved);

BOOL WINAPI EnumTimeFormatsA(_In_ TIMEFMT_ENUMPROCA, _In_ LCID, _In_ DWORD);
BOOL WINAPI EnumTimeFormatsW(_In_ TIMEFMT_ENUMPROCW, _In_ LCID, _In_ DWORD);

int
WINAPI
FoldStringA(
  _In_ DWORD dwMapFlags,
  _In_reads_(cchSrc) LPCSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_writes_opt_(cchDest) LPSTR lpDestStr,
  _In_ int cchDest);

int
WINAPI
FoldStringW(
  _In_ DWORD dwMapFlags,
  _In_reads_(cchSrc) LPCWSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_writes_opt_(cchDest) LPWSTR lpDestStr,
  _In_ int cchDest);

UINT WINAPI GetACP(void);

int
WINAPI
GetCalendarInfoA(
  _In_ LCID Locale,
  _In_ CALID Calendar,
  _In_ CALTYPE CalType,
  _Out_writes_opt_(cchData) LPSTR lpCalData,
  _In_ int cchData,
  _Out_opt_ LPDWORD lpValue);

int
WINAPI
GetCalendarInfoW(
  _In_ LCID Locale,
  _In_ CALID Calendar,
  _In_ CALTYPE CalType,
  _Out_writes_opt_(cchData) LPWSTR lpCalData,
  _In_ int cchData,
  _Out_opt_ LPDWORD lpValue);

BOOL WINAPI GetCPInfo(_In_ UINT, _Out_ LPCPINFO);
BOOL WINAPI GetCPInfoExA(_In_ UINT, _In_ DWORD, _Out_ LPCPINFOEXA);
BOOL WINAPI GetCPInfoExW(_In_ UINT, _In_ DWORD, _Out_ LPCPINFOEXW);

int
WINAPI
GetCurrencyFormatA(
  _In_ LCID Locale,
  _In_ DWORD dwFlags,
  _In_ LPCSTR lpValue,
  _In_opt_ const CURRENCYFMTA *lpFormat,
  _Out_writes_opt_(cchCurrency) LPSTR lpCurrencyStr,
  _In_ int cchCurrency);

int
WINAPI
GetCurrencyFormatW(
  _In_ LCID Locale,
  _In_ DWORD dwFlags,
  _In_ LPCWSTR lpValue,
  _In_opt_ const CURRENCYFMTW *lpFormat,
  _Out_writes_opt_(cchCurrency) LPWSTR lpCurrencyStr,
  _In_ int cchCurrency);

int WINAPI GetDateFormatA(LCID,DWORD,const SYSTEMTIME*,LPCSTR,LPSTR,int);
int WINAPI GetDateFormatW(LCID,DWORD,const SYSTEMTIME*,LPCWSTR,LPWSTR,int);
int WINAPI GetDateFormatEx(LPCWSTR,DWORD,const SYSTEMTIME*,LPCWSTR,LPWSTR,int,LPCWSTR);

int
WINAPI
GetGeoInfoA(
  _In_ GEOID Location,
  _In_ GEOTYPE GeoType,
  _Out_writes_opt_(cchData) LPSTR lpGeoData,
  _In_ int cchData,
  _In_ LANGID LangId);

int
WINAPI
GetGeoInfoW(
  _In_ GEOID Location,
  _In_ GEOTYPE GeoType,
  _Out_writes_opt_(cchData) LPWSTR lpGeoData,
  _In_ int cchData,
  _In_ LANGID LangId);

int
WINAPI
GetLocaleInfoA(
  _In_ LCID Locale,
  _In_ LCTYPE LCType,
  _Out_writes_opt_(cchData) LPSTR lpLCData,
  _In_ int cchData);

int
WINAPI
GetLocaleInfoW(
  _In_ LCID Locale,
  _In_ LCTYPE LCType,
  _Out_writes_opt_(cchData) LPWSTR lpLCData,
  _In_ int cchData);

BOOL WINAPI GetNLSVersion(_In_ NLS_FUNCTION, _In_ LCID, _Inout_ LPNLSVERSIONINFO);

BOOL
WINAPI
GetNLSVersionEx(
  _In_ NLS_FUNCTION function,
  _In_ LPCWSTR lpLocaleName,
  _Inout_ LPNLSVERSIONINFOEX lpVersionInformation);

int
WINAPI
GetNumberFormatA(
  _In_ LCID Locale,
  _In_ DWORD dwFlags,
  _In_ LPCSTR lpValue,
  _In_opt_ const NUMBERFMTA *lpFormat,
  _Out_writes_opt_(cchNumber) LPSTR lpNumberStr,
  _In_ int cchNumber);

int
WINAPI
GetNumberFormatW(
  _In_ LCID Locale,
  _In_ DWORD dwFlags,
  _In_ LPCWSTR lpValue,
  _In_opt_ const NUMBERFMTW *lpFormat,
  _Out_writes_opt_(cchNumber) LPWSTR lpNumberStr,
  _In_ int cchNumber);

UINT WINAPI GetOEMCP(void);

BOOL
WINAPI
GetStringTypeA(
  _In_ LCID Locale,
  _In_ DWORD dwInfoType,
  _In_reads_(cchSrc) LPCSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_ LPWORD lpCharType);

BOOL
WINAPI
GetStringTypeW(
  _In_ DWORD dwInfoType,
  _In_reads_(cchSrc) LPCWSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_ LPWORD lpCharType);

BOOL
WINAPI
GetStringTypeExA(
  _In_ LCID Locale,
  _In_ DWORD dwInfoType,
  _In_reads_(cchSrc) LPCSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_writes_(cchSrc) LPWORD lpCharType);

BOOL
WINAPI
GetStringTypeExW(
  _In_ LCID Locale,
  _In_ DWORD dwInfoType,
  _In_reads_(cchSrc) LPCWSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_writes_(cchSrc) LPWORD lpCharType);

LANGID WINAPI GetSystemDefaultLangID(void);
LCID WINAPI GetSystemDefaultLCID(void);
LCID WINAPI GetThreadLocale(void);
int WINAPI GetTimeFormatA(LCID,DWORD,const SYSTEMTIME*,LPCSTR,LPSTR,int);
int WINAPI GetTimeFormatW(LCID,DWORD,const SYSTEMTIME*,LPCWSTR,LPWSTR,int);
int WINAPI GetTimeFormatEx(LPCWSTR,DWORD,const SYSTEMTIME*,LPCWSTR,LPWSTR,int);
LANGID WINAPI GetUserDefaultLangID(void);

WINBASEAPI
int
WINAPI
GetUserDefaultLocaleName(
    _Out_writes_(cchLocaleName) LPWSTR lpLocaleName,
    _In_ int cchLocaleName);

LCID WINAPI GetUserDefaultLCID(void);
GEOID WINAPI GetUserGeoID(_In_ GEOCLASS);

#if (WINVER >= 0x0600)

int
WINAPI
IdnToAscii(
  _In_ DWORD dwFlags,
  _In_reads_(cchUnicodeChar) LPCWSTR lpUnicodeCharStr,
  _In_ int cchUnicodeChar,
  _Out_writes_opt_(cchASCIIChar) LPWSTR lpASCIICharStr,
  _In_ int cchASCIIChar);

int
WINAPI
IdnToUnicode(
  _In_ DWORD dwFlags,
  _In_reads_(cchASCIIChar) LPCWSTR lpASCIICharStr,
  _In_ int cchASCIIChar,
  _Out_writes_opt_(cchUnicodeChar) LPWSTR lpUnicodeCharStr,
  _In_ int cchUnicodeChar);

#endif /* WINVER >= 0x0600 */

BOOL WINAPI IsDBCSLeadByte(_In_ BYTE);
BOOL WINAPI IsDBCSLeadByteEx(_In_ UINT, _In_ BYTE);

BOOL
WINAPI
IsNLSDefinedString(
  _In_ NLS_FUNCTION Function,
  _In_ DWORD dwFlags,
  _In_ LPNLSVERSIONINFO lpVersionInformation,
  _In_reads_(cchStr) LPCWSTR lpString,
  _In_ int cchStr);

BOOL WINAPI IsValidCodePage(_In_ UINT);
BOOL WINAPI IsValidLocale(_In_ LCID, _In_ DWORD);

int
WINAPI
LCMapStringA(
  _In_ LCID Locale,
  _In_ DWORD dwMapFlags,
  _In_reads_(cchSrc) LPCSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_writes_opt_(_Inexpressible_(cchDest)) LPSTR lpDestStr,
  _In_ int cchDest);

int
WINAPI
LCMapStringW(
  _In_ LCID Locale,
  _In_ DWORD dwMapFlags,
  _In_reads_(cchSrc) LPCWSTR lpSrcStr,
  _In_ int cchSrc,
  _Out_writes_opt_(_Inexpressible_(cchDest)) LPWSTR lpDestStr,
  _In_ int cchDest);

int WINAPI MultiByteToWideChar(UINT,DWORD,LPCSTR,int,LPWSTR,int);
int WINAPI SetCalendarInfoA(_In_ LCID, _In_ CALID, _In_ CALTYPE, _In_ LPCSTR);
int WINAPI SetCalendarInfoW(_In_ LCID, _In_ CALID, _In_ CALTYPE, _In_ LPCWSTR);
BOOL WINAPI SetLocaleInfoA(_In_ LCID, _In_ LCTYPE, _In_ LPCSTR);
BOOL WINAPI SetLocaleInfoW(_In_ LCID, _In_ LCTYPE, _In_ LPCWSTR);
BOOL WINAPI SetThreadLocale(_In_ LCID);
LANGID WINAPI SetThreadUILanguage(_In_ LANGID);
BOOL WINAPI SetUserDefaultLCID(LCID);
BOOL WINAPI SetUserDefaultUILanguage(LANGID);
BOOL WINAPI SetUserGeoID(_In_ GEOID);
int WINAPI WideCharToMultiByte(UINT,DWORD,LPCWSTR,int,LPSTR,int,LPCSTR,LPBOOL);
#if (WINVER >= 0x0500)
BOOL WINAPI EnumCalendarInfoExA(_In_ CALINFO_ENUMPROCEXA, _In_ LCID, _In_ CALID, _In_ CALTYPE);
BOOL WINAPI EnumCalendarInfoExW(_In_ CALINFO_ENUMPROCEXW, _In_ LCID, _In_ CALID, _In_ CALTYPE);
BOOL WINAPI EnumDateFormatsExA(_In_ DATEFMT_ENUMPROCEXA, _In_ LCID, _In_ DWORD);
BOOL WINAPI EnumDateFormatsExW(_In_ DATEFMT_ENUMPROCEXW, _In_ LCID, _In_ DWORD);
BOOL WINAPI EnumSystemLanguageGroupsA(_In_ LANGUAGEGROUP_ENUMPROCA, _In_ DWORD, _In_ LONG_PTR);
BOOL WINAPI EnumSystemLanguageGroupsW(_In_ LANGUAGEGROUP_ENUMPROCW, _In_ DWORD, _In_ LONG_PTR);
BOOL WINAPI EnumLanguageGroupLocalesA(_In_ LANGGROUPLOCALE_ENUMPROCA, _In_ LGRPID, _In_ DWORD, _In_ LONG_PTR);
BOOL WINAPI EnumLanguageGroupLocalesW(_In_ LANGGROUPLOCALE_ENUMPROCW, _In_ LGRPID, _In_ DWORD, _In_ LONG_PTR);
BOOL WINAPI EnumUILanguagesA(_In_ UILANGUAGE_ENUMPROCA, _In_ DWORD, _In_ LONG_PTR);
BOOL WINAPI EnumUILanguagesW(_In_ UILANGUAGE_ENUMPROCW, _In_ DWORD, _In_ LONG_PTR);
LANGID WINAPI GetSystemDefaultUILanguage(void);
LANGID WINAPI GetUserDefaultUILanguage(void);
BOOL WINAPI IsValidLanguageGroup(_In_ LGRPID, _In_ DWORD);
#endif /* (WINVER >= 0x0500) */

#if (WINVER >= 0x0600)

_Success_(return != FALSE)
BOOL
WINAPI
GetFileMUIInfo(
  _In_ DWORD dwFlags,
  _In_ PCWSTR pcwszFilePath,
  _Inout_updates_bytes_to_opt_(*pcbFileMUIInfo, *pcbFileMUIInfo) PFILEMUIINFO pFileMUIInfo,
  _Inout_ DWORD *pcbFileMUIInfo);

BOOL
WINAPI
GetFileMUIPath(
  _In_ DWORD dwFlags,
  _In_ PCWSTR pcwszFilePath,
  _Inout_updates_opt_(*pcchLanguage) PWSTR pwszLanguage,
  _Inout_ PULONG pcchLanguage,
  _Out_writes_opt_(*pcchFileMUIPath) PWSTR pwszFileMUIPath,
  _Inout_ PULONG pcchFileMUIPath,
  _Inout_ PULONGLONG pululEnumerator);

WINBASEAPI
int
WINAPI
GetLocaleInfoEx(
  _In_opt_ LPCWSTR lpLocaleName,
  _In_ LCTYPE LCType,
  _Out_writes_opt_(cchData) LPWSTR lpLCData,
  _In_ int cchData);

WINBASEAPI
BOOL
WINAPI
IsValidLocaleName(
  _In_ LPCWSTR lpLocaleName);

BOOL
WINAPI
GetProcessPreferredUILanguages(
  _In_ DWORD dwFlags,
  _Out_ PULONG pulNumLanguages,
  _Out_writes_opt_(*pcchLanguagesBuffer) PZZWSTR pwszLanguagesBuffer,
  _Inout_ PULONG pcchLanguagesBuffer);

BOOL
WINAPI
GetSystemPreferredUILanguages(
  _In_ DWORD dwFlags,
  _Out_ PULONG pulNumLanguages,
  _Out_writes_opt_(*pcchLanguagesBuffer) PZZWSTR pwszLanguagesBuffer,
  _Inout_ PULONG pcchLanguagesBuffer);

BOOL
WINAPI
GetThreadPreferredUILanguages(
  _In_ DWORD dwFlags,
  _Out_ PULONG pulNumLanguages,
  _Out_writes_opt_(*pcchLanguagesBuffer) PZZWSTR pwszLanguagesBuffer,
  _Inout_ PULONG pcchLanguagesBuffer);

LANGID WINAPI GetThreadUILanguage(void);

BOOL
WINAPI
GetUILanguageInfo(
  _In_ DWORD dwFlags,
  _In_ PCZZWSTR pwmszLanguage,
  _Out_writes_opt_(*pcchFallbackLanguages) PZZWSTR pwszFallbackLanguages,
  _Inout_opt_ PDWORD pcchFallbackLanguages,
  _Out_ PDWORD pAttributes);

BOOL
WINAPI
GetUserPreferredUILanguages(
  _In_ DWORD dwFlags,
  _Out_ PULONG pulNumLanguages,
  _Out_writes_opt_(*pcchLanguagesBuffer) PZZWSTR pwszLanguagesBuffer,
  _Inout_ PULONG pcchLanguagesBuffer);

int
WINAPI
IdnToAscii(
  _In_ DWORD dwFlags,
  _In_reads_(cchUnicodeChar) LPCWSTR lpUnicodeCharStr,
  _In_ int cchUnicodeChar,
  _Out_writes_opt_(cchASCIIChar) LPWSTR lpASCIICharStr,
  _In_ int cchASCIIChar);

int
WINAPI
IdnToNameprepUnicode(
  _In_ DWORD dwFlags,
  _In_reads_(cchUnicodeChar) LPCWSTR lpUnicodeCharStr,
  _In_ int cchUnicodeChar,
  _Out_writes_opt_(cchNameprepChar) LPWSTR lpNameprepCharStr,
  _In_ int cchNameprepChar);

int
WINAPI
IdnToUnicode(
  _In_ DWORD dwFlags,
  _In_reads_(cchASCIIChar) LPCWSTR lpASCIICharStr,
  _In_ int cchASCIIChar,
  _Out_writes_opt_(cchUnicodeChar) LPWSTR lpUnicodeCharStr,
  _In_ int cchUnicodeChar);

BOOL
WINAPI
IsNormalizedString(
  _In_ NORM_FORM NormForm,
  _In_reads_(cwLength) LPCWSTR lpString,
  _In_ int cwLength);

int
WINAPI
NormalizeString(
  _In_ NORM_FORM NormForm,
  _In_reads_(cwSrcLength) LPCWSTR lpSrcString,
  _In_ int cwSrcLength,
  _Out_writes_opt_(cwDstLength) LPWSTR lpDstString,
  _In_ int cwDstLength);

int
WINAPI
GetStringScripts(
  _In_ DWORD dwFlags,
  _In_ LPCWSTR lpString,
  _In_ int cchString,
  _Out_writes_opt_(cchScripts) LPWSTR lpScripts,
  _In_ int cchScripts);

BOOL WINAPI SetProcessPreferredUILanguages(_In_ DWORD, _In_opt_ PCZZWSTR, _Out_opt_ PULONG);
BOOL WINAPI SetThreadPreferredUILanguages(_In_ DWORD, _In_opt_ PCZZWSTR, _Out_opt_ PULONG);
BOOL WINAPI VerifyScripts(_In_ DWORD, _In_ LPCWSTR, _In_ int, _In_ LPCWSTR, _In_ int);

#if (WINVER >= _WIN32_WINNT_WIN8)
_When_((dwMapFlags & (LCMAP_SORTKEY | LCMAP_BYTEREV | LCMAP_HASH | LCMAP_SORTHANDLE)) != 0, _At_((LPBYTE) lpDestStr, _Out_writes_bytes_opt_(cchDest)))
#else
_When_((dwMapFlags & (LCMAP_SORTKEY | LCMAP_BYTEREV)) != 0, _At_((LPBYTE) lpDestStr, _Out_writes_bytes_opt_(cchDest)))
#endif
_When_(cchSrc != -1,  _At_((WCHAR *) lpSrcStr, _Out_writes_opt_(cchSrc)))
_When_(cchDest != -1, _At_((WCHAR *) lpDestStr, _Out_writes_opt_(cchDest)))
WINBASEAPI
int
WINAPI
LCMapStringEx(
    _In_opt_ LPCWSTR lpLocaleName,
    _In_ DWORD dwMapFlags,
    _In_reads_(cchSrc) LPCWSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_writes_opt_(cchDest) LPWSTR lpDestStr,
    _In_ int cchDest,
    _In_opt_ LPNLSVERSIONINFO lpVersionInformation,
    _In_opt_ LPVOID lpReserved,
    _In_opt_ LPARAM sortHandle);

LCID WINAPI LocaleNameToLCID(_In_ LPCWSTR, _In_ DWORD);

#endif /* (WINVER >= 0x0600) */

WINBASEAPI
int
WINAPI
LCIDToLocaleName(
    _In_ LCID Locale,
    _Out_writes_opt_(cchName) LPWSTR  lpName,
    _In_ int cchName,
    _In_ DWORD dwFlags);

#ifdef UNICODE
#define CALINFO_ENUMPROC CALINFO_ENUMPROCW
#define CALINFO_ENUMPROCEX CALINFO_ENUMPROCEXW
#define LOCALE_ENUMPROC LOCALE_ENUMPROCW
#define CODEPAGE_ENUMPROC CODEPAGE_ENUMPROCW
#define DATEFMT_ENUMPROC DATEFMT_ENUMPROCW
#define DATEFMT_ENUMPROCEX DATEFMT_ENUMPROCEXW
#define TIMEFMT_ENUMPROC TIMEFMT_ENUMPROCW
#define LANGUAGEGROUP_ENUMPROC LANGUAGEGROUP_ENUMPROCW
#define LANGGROUPLOCALE_ENUMPROC LANGGROUPLOCALE_ENUMPROCW
#define UILANGUAGE_ENUMPROC UILANGUAGE_ENUMPROCW
typedef CPINFOEXW CPINFOEX;
typedef LPCPINFOEXW LPCPINFOEX;
typedef CURRENCYFMTW CURRENCYFMT;
typedef LPCURRENCYFMTW LPCURRENCYFMT;
typedef NUMBERFMTW NUMBERFMT;
typedef LPNUMBERFMTW LPNUMBERFMT;
#define CompareString CompareStringW
#define EnumCalendarInfo EnumCalendarInfoW
#define EnumDateFormats EnumDateFormatsW
#define EnumSystemCodePages EnumSystemCodePagesW
#define EnumSystemLocales EnumSystemLocalesW
#define EnumTimeFormats EnumTimeFormatsW
#define FoldString FoldStringW
#define GetCalendarInfo GetCalendarInfoW
#define GetCPInfoEx GetCPInfoExW
#define GetCurrencyFormat GetCurrencyFormatW
#define GetDateFormat GetDateFormatW
#define GetGeoInfo GetGeoInfoW
#define GetLocaleInfo GetLocaleInfoW
#define GetNumberFormat GetNumberFormatW
#define GetStringTypeEx GetStringTypeExW
#define GetTimeFormat GetTimeFormatW
#define LCMapString LCMapStringW
#define SetCalendarInfo  SetCalendarInfoW
#define SetLocaleInfo SetLocaleInfoW
#if (WINVER >= 0x0500)
#define EnumCalendarInfoEx EnumCalendarInfoExW
#define EnumDateFormatsEx EnumDateFormatsExW
#define EnumSystemLanguageGroups EnumSystemLanguageGroupsW
#define EnumLanguageGroupLocales EnumLanguageGroupLocalesW
#define EnumUILanguages EnumUILanguagesW
#endif /* (WINVER >= 0x0500) */
#else
#define CALINFO_ENUMPROC CALINFO_ENUMPROCA
#define CALINFO_ENUMPROCEX CALINFO_ENUMPROCEXA
#define LOCALE_ENUMPROC LOCALE_ENUMPROCA
#define CODEPAGE_ENUMPROC CODEPAGE_ENUMPROCA
#define DATEFMT_ENUMPROC DATEFMT_ENUMPROCA
#define DATEFMT_ENUMPROCEX DATEFMT_ENUMPROCEXA
#define TIMEFMT_ENUMPROC TIMEFMT_ENUMPROCA
#define LANGUAGEGROUP_ENUMPROC LANGUAGEGROUP_ENUMPROCA
#define LANGGROUPLOCALE_ENUMPROC LANGGROUPLOCALE_ENUMPROCA
#define UILANGUAGE_ENUMPROC UILANGUAGE_ENUMPROCA
typedef CPINFOEXA CPINFOEX;
typedef LPCPINFOEXA LPCPINFOEX;
typedef CURRENCYFMTA CURRENCYFMT;
typedef LPCURRENCYFMTA LPCURRENCYFMT;
typedef NUMBERFMTA NUMBERFMT;
typedef LPNUMBERFMTA LPNUMBERFMT;
#define CompareString CompareStringA
#define EnumCalendarInfo EnumCalendarInfoA
#define EnumDateFormats EnumDateFormatsA
#define EnumSystemCodePages EnumSystemCodePagesA
#define EnumSystemLocales EnumSystemLocalesA
#define EnumTimeFormats EnumTimeFormatsA
#define FoldString FoldStringA
#define GetCalendarInfo GetCalendarInfoA
#define GetCPInfoEx GetCPInfoExA
#define GetCurrencyFormat GetCurrencyFormatA
#define GetDateFormat GetDateFormatA
#define GetGeoInfo GetGeoInfoA
#define GetLocaleInfo GetLocaleInfoA
#define GetNumberFormat GetNumberFormatA
#define GetStringTypeEx GetStringTypeExA
#define GetTimeFormat GetTimeFormatA
#define LCMapString LCMapStringA
#define SetCalendarInfo SetCalendarInfoA
#define SetLocaleInfo SetLocaleInfoA
#if (WINVER >= 0x0500)
#define EnumCalendarInfoEx EnumCalendarInfoExA
#define EnumDateFormatsEx EnumDateFormatsExA
#define EnumSystemLanguageGroups EnumSystemLanguageGroupsA
#define EnumLanguageGroupLocales EnumLanguageGroupLocalesA
#define EnumUILanguages EnumUILanguagesA
#endif /* (WINVER >= 0x0500) */
#endif /* UNICODE */
#endif /* RC_INVOKED */

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#ifndef NOAPISET
#include <stringapiset.h>
#endif

#endif
