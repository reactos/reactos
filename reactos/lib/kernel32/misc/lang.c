/* $Id: lang.c,v 1.1 2003/09/20 23:37:56 weiden Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/lang.c
 * AUTHOR   : ???
 */

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

//#define _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
LCID
STDCALL
ConvertDefaultLocale (
    LCID    Locale
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumCalendarInfoW (
    CALINFO_ENUMPROCW lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumCalendarInfoA (
    CALINFO_ENUMPROCA lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumCalendarInfoExA(
    CALINFO_ENUMPROCEXA lpCalInfoEnumProcEx,
    LCID                Locale,
    CALID               Calendar,
    CALTYPE             CalType)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumCalendarInfoExW(
    CALINFO_ENUMPROCEXW lpCalInfoEnumProcEx,
    LCID                Locale,
    CALID               Calendar,
    CALTYPE             CalType)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDateFormatsW (
    DATEFMT_ENUMPROCW  lpDateFmtEnumProc,
    LCID               Locale,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDateFormatsA (
    DATEFMT_ENUMPROCA  lpDateFmtEnumProc,
    LCID               Locale,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDateFormatsExA(
    DATEFMT_ENUMPROCEXA lpDateFmtEnumProcEx,
    LCID                Locale,
    DWORD               dwFlags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumDateFormatsExW(
    DATEFMT_ENUMPROCEXW lpDateFmtEnumProcEx,
    LCID                Locale,
    DWORD               dwFlags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumLanguageGroupLocalesA(
    LANGGROUPLOCALE_ENUMPROCA lpLangGroupLocaleEnumProc,
    LGRPID                    LanguageGroup,
    DWORD                     dwFlags,
    LONG_PTR                  lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumLanguageGroupLocalesW(
    LANGGROUPLOCALE_ENUMPROCW lpLangGroupLocaleEnumProc,
    LGRPID                    LanguageGroup,
    DWORD                     dwFlags,
    LONG_PTR                  lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumSystemCodePagesW (
    CODEPAGE_ENUMPROCW  lpCodePageEnumProc,
    DWORD               dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumSystemCodePagesA (
    CODEPAGE_ENUMPROCA lpCodePageEnumProc,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumSystemGeoID(
    GEOCLASS        GeoClass,
    GEOID           ParentGeoId,
    GEO_ENUMPROC    lpGeoEnumProc)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumSystemLanguageGroupsA(
    LANGUAGEGROUP_ENUMPROCA lpLanguageGroupEnumProc,
    DWORD                   dwFlags,
    LONG_PTR                lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumSystemLocalesW (
    LOCALE_ENUMPROCW lpLocaleEnumProc,
    DWORD            dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumSystemLocalesA (
    LOCALE_ENUMPROCA lpLocaleEnumProc,
    DWORD            dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumTimeFormatsW (
    TIMEFMT_ENUMPROCW    lpTimeFmtEnumProc,
    LCID            Locale,
    DWORD           dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumTimeFormatsA (
    TIMEFMT_ENUMPROCA  lpTimeFmtEnumProc,
    LCID               Locale,
    DWORD              dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumUILanguagesA(
    UILANGUAGE_ENUMPROCA lpUILanguageEnumProc,
    DWORD                dwFlags,
    LONG_PTR             lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
EnumUILanguagesW(
    UILANGUAGE_ENUMPROCW lpUILanguageEnumProc,
    DWORD                dwFlags,
    LONG_PTR             lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
UINT
STDCALL
GetACP (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 1252;
}

#endif


/*
 * @unimplemented
 */
int
STDCALL
GetCalendarInfoA(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPSTR   lpCalData,
    int      cchData,
    LPDWORD  lpValue)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetCalendarInfoW(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPWSTR   lpCalData,
    int      cchData,
    LPDWORD  lpValue)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetCPInfo (
    UINT        CodePage,
    LPCPINFO    CodePageInfo
    )
{
    unsigned i;

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);

    CodePageInfo->MaxCharSize = 1;
    CodePageInfo->DefaultChar[0] = '?';
    for (i = 1; i < MAX_DEFAULTCHAR; i++)
	{
	CodePageInfo->DefaultChar[i] = '\0';
	}
    for (i = 0; i < MAX_LEADBYTES; i++)
	{
	CodePageInfo->LeadByte[i] = '\0';
	}

    return TRUE;
}

#endif


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetCPInfoExW(
    UINT          CodePage,
    DWORD         dwFlags,
    LPCPINFOEXW  lpCPInfoEx)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetCPInfoExA(
    UINT          CodePage,
    DWORD         dwFlags,
    LPCPINFOEXA  lpCPInfoEx)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetCurrencyFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    LPCWSTR         lpValue,
    CONST CURRENCYFMTW   * lpFormat,
    LPWSTR          lpCurrencyStr,
    int         cchCurrency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetCurrencyFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    LPCSTR          lpValue,
    CONST CURRENCYFMTA   * lpFormat,
    LPSTR           lpCurrencyStr,
    int         cchCurrency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
int
STDCALL
GetDateFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpDate,
    LPCWSTR         lpFormat,
    LPWSTR          lpDateStr,
    int         cchDate
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetDateFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpDate,
    LPCSTR          lpFormat,
    LPSTR           lpDateStr,
    int         cchDate
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

#endif


/*
 * @unimplemented
 */
int
STDCALL
GetGeoInfoW(
    GEOID       Location,
    GEOTYPE     GeoType,
    LPWSTR     lpGeoData,
    int         cchData,
    LANGID      LangId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetGeoInfoA(
    GEOID       Location,
    GEOTYPE     GeoType,
    LPSTR     lpGeoData,
    int         cchData,
    LANGID      LangId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetLocaleInfoW (
    LCID    Locale,
    LCTYPE  LCType,
    LPWSTR  lpLCData,
    int cchData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetLocaleInfoA (
    LCID    Locale,
    LCTYPE  LCType,
    LPSTR   lpLCData,
    int cchData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetNumberFormatW (
    LCID        Locale,
    DWORD       dwFlags,
    LPCWSTR     lpValue,
    CONST NUMBERFMTW * lpFormat,
    LPWSTR      lpNumberStr,
    int     cchNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetNumberFormatA (
    LCID        Locale,
    DWORD       dwFlags,
    LPCSTR      lpValue,
    CONST NUMBERFMTA * lpFormat,
    LPSTR       lpNumberStr,
    int     cchNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
GetOEMCP (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 437; /* FIXME: call csrss.exe */
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
LANGID
STDCALL
GetSystemDefaultLangID (VOID)
{
     /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELANGID(
        LANG_ENGLISH,
        SUBLANG_ENGLISH_US
        );
}


/*
 * @unimplemented
 */
LCID
STDCALL
GetSystemDefaultLCID (VOID)
{
    /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELCID(
        LANG_ENGLISH,
        SORT_DEFAULT
        );
}

#endif


/*
 * @unimplemented
 */
LANGID
STDCALL
GetSystemDefaultUILanguage(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
LCID
STDCALL
GetThreadLocale (VOID)
{
    /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELCID(
        LANG_ENGLISH,
        SORT_DEFAULT
        );
}

#endif


/*
 * @unimplemented
 */
int
STDCALL
GetTimeFormatW (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpTime,
    LPCWSTR         lpFormat,
    LPWSTR          lpTimeStr,
    int         cchTime
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetTimeFormatA (
    LCID            Locale,
    DWORD           dwFlags,
    CONST SYSTEMTIME    * lpTime,
    LPCSTR          lpFormat,
    LPSTR           lpTimeStr,
    int         cchTime
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
LANGID
STDCALL
GetUserDefaultLangID (VOID)
{
     /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELANGID(
        LANG_ENGLISH,
        SUBLANG_ENGLISH_US
        );
}


/*
 * @unimplemented
 */
LCID
STDCALL
GetUserDefaultLCID (VOID)
{
    /* FIXME: ??? */
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return MAKELCID(
        LANG_ENGLISH,
        SORT_DEFAULT
        );
}

#endif


/*
 * @unimplemented
 */
LANGID
STDCALL
GetUserDefaultUILanguage(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
GEOID
STDCALL
GetUserGeoID(
    GEOCLASS    GeoClass)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsValidCodePage (
    UINT    CodePage
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsValidLanguageGroup(
    LGRPID  LanguageGroup,
    DWORD   dwFlags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
IsValidLocale (
    LCID    Locale,
    DWORD   dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
int
STDCALL
LCMapStringA (
    LCID    Locale,
    DWORD   dwMapFlags,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPSTR   lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
LCMapStringW (
    LCID    Locale,
    DWORD   dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR  lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

#endif


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetCalendarInfoA(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPCSTR  lpCalData)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetCalendarInfoW(
    LCID     Locale,
    CALID    Calendar,
    CALTYPE  CalType,
    LPCWSTR  lpCalData)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetLocaleInfoA (
    LCID    Locale,
    LCTYPE  LCType,
    LPCSTR  lpLCData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetLocaleInfoW (
    LCID    Locale,
    LCTYPE  LCType,
    LPCWSTR lpLCData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetThreadLocale (
    LCID    Locale
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SetUserGeoID(
    GEOID       GeoId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
