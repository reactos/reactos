/* $Id: lang.c,v 1.11 2004/01/23 21:16:03 ekohl Exp $
 *
 * COPYRIGHT: See COPYING in the top level directory
 * PROJECT  : ReactOS user mode libraries
 * MODULE   : kernel32.dll
 * FILE     : reactos/lib/kernel32/misc/lang.c
 * AUTHOR   : ???
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

/* FIXME:  these are included in winnls.h, however including this file causes alot of 
           conflicting type errors. */

#define LOCALE_SYEARMONTH 0x1006
#define LOCALE_IPAPERSIZE 0x100A
#define LOCALE_RETURN_NUMBER 0x20000000
#define LOCALE_USE_CP_ACP 0x40000000
#define LOCALE_LOCALEINFOFLAGSMASK (LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP|LOCALE_RETURN_NUMBER)
#define LOCALE_NEUTRAL		(MAKELCID(MAKELANGID(LANG_NEUTRAL,SUBLANG_NEUTRAL),SORT_DEFAULT))
#define LOCALE_FONTSIGNATURE 0x00000058

static LCID SystemLocale = MAKELCID(LANG_ENGLISH, SORT_DEFAULT);

//#define _OLE2NLS_IN_BUILD_

/*
 * @implemented
 */
LCID
STDCALL
ConvertDefaultLocale (
    LCID    Locale
    )
{
  switch(Locale)
  {
    case LOCALE_SYSTEM_DEFAULT:
      return GetSystemDefaultLCID();
    
    case LOCALE_USER_DEFAULT:
      return GetUserDefaultLCID();
    
    /*case LOCALE_NEUTRAL:
      return MAKELCID(LANG_NEUTRAL, SUBLANG_NEUTRAL);*/
  }
  
  /* ported from wine, is that right? */
  return MAKELANGID(PRIMARYLANGID(Locale), SUBLANG_NEUTRAL);
}


/*
 * @unimplemented
 */
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
STDCALL
EnumSystemGeoID(
    GEOCLASS        GeoClass,
    GEOID           ParentGeoId,
    GEO_ENUMPROC    lpGeoEnumProc)
{
  if(!lpGeoEnumProc)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  switch(GeoClass)
  {
    case GEOCLASS_NATION:
      /*RtlEnterCriticalSection(&DllLock);
      
        FIXME - Get GEO IDs calling Csr
      
      RtlLeaveCriticalSection(&DllLock);*/
      
      SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
      break;
    
    default:
      SetLastError(ERROR_INVALID_FLAGS);
      return FALSE;
  }
 
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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

const WCHAR *RosGetLocaleValueName( DWORD lctype )
{
    static const WCHAR iCalendarTypeW[] = {'i','C','a','l','e','n','d','a','r','T','y','p','e',0};
    static const WCHAR iCountryW[] = {'i','C','o','u','n','t','r','y',0};
    static const WCHAR iCurrDigitsW[] = {'i','C','u','r','r','D','i','g','i','t','s',0};
    static const WCHAR iCurrencyW[] = {'i','C','u','r','r','e','n','c','y',0};
    static const WCHAR iDateW[] = {'i','D','a','t','e',0};
    static const WCHAR iDigitsW[] = {'i','D','i','g','i','t','s',0};
    static const WCHAR iFirstDayOfWeekW[] = {'i','F','i','r','s','t','D','a','y','O','f','W','e','e','k',0};
    static const WCHAR iFirstWeekOfYearW[] = {'i','F','i','r','s','t','W','e','e','k','O','f','Y','e','a','r',0};
    static const WCHAR iLDateW[] = {'i','L','D','a','t','e',0};
    static const WCHAR iLZeroW[] = {'i','L','Z','e','r','o',0};
    static const WCHAR iMeasureW[] = {'i','M','e','a','s','u','r','e',0};
    static const WCHAR iNegCurrW[] = {'i','N','e','g','C','u','r','r',0};
    static const WCHAR iNegNumberW[] = {'i','N','e','g','N','u','m','b','e','r',0};
    static const WCHAR iPaperSizeW[] = {'i','P','a','p','e','r','S','i','z','e',0};
    static const WCHAR iTLZeroW[] = {'i','T','L','Z','e','r','o',0};
    static const WCHAR iTimeW[] = {'i','T','i','m','e',0};
    static const WCHAR s1159W[] = {'s','1','1','5','9',0};
    static const WCHAR s2359W[] = {'s','2','3','5','9',0};
    static const WCHAR sCountryW[] = {'s','C','o','u','n','t','r','y',0};
    static const WCHAR sCurrencyW[] = {'s','C','u','r','r','e','n','c','y',0};
    static const WCHAR sDateW[] = {'s','D','a','t','e',0};
    static const WCHAR sDecimalW[] = {'s','D','e','c','i','m','a','l',0};
    static const WCHAR sGroupingW[] = {'s','G','r','o','u','p','i','n','g',0};
    static const WCHAR sLanguageW[] = {'s','L','a','n','g','u','a','g','e',0};
    static const WCHAR sListW[] = {'s','L','i','s','t',0};
    static const WCHAR sLongDateW[] = {'s','L','o','n','g','D','a','t','e',0};
    static const WCHAR sMonDecimalSepW[] = {'s','M','o','n','D','e','c','i','m','a','l','S','e','p',0};
    static const WCHAR sMonGroupingW[] = {'s','M','o','n','G','r','o','u','p','i','n','g',0};
    static const WCHAR sMonThousandSepW[] = {'s','M','o','n','T','h','o','u','s','a','n','d','S','e','p',0};
    static const WCHAR sNegativeSignW[] = {'s','N','e','g','a','t','i','v','e','S','i','g','n',0};
    static const WCHAR sPositiveSignW[] = {'s','P','o','s','i','t','i','v','e','S','i','g','n',0};
    static const WCHAR sShortDateW[] = {'s','S','h','o','r','t','D','a','t','e',0};
    static const WCHAR sThousandW[] = {'s','T','h','o','u','s','a','n','d',0};
    static const WCHAR sTimeFormatW[] = {'s','T','i','m','e','F','o','r','m','a','t',0};
    static const WCHAR sTimeW[] = {'s','T','i','m','e',0};
    static const WCHAR sYearMonthW[] = {'s','Y','e','a','r','M','o','n','t','h',0};

    switch (lctype & ~LOCALE_LOCALEINFOFLAGSMASK)
    {
    /* These values are used by SetLocaleInfo and GetLocaleInfo, and
     * the values are stored in the registry, confirmed under Windows.
     */
    case LOCALE_ICALENDARTYPE:    return iCalendarTypeW;
    case LOCALE_ICURRDIGITS:      return iCurrDigitsW;
    case LOCALE_ICURRENCY:        return iCurrencyW;
    case LOCALE_IDIGITS:          return iDigitsW;
    case LOCALE_IFIRSTDAYOFWEEK:  return iFirstDayOfWeekW;
    case LOCALE_IFIRSTWEEKOFYEAR: return iFirstWeekOfYearW;
    case LOCALE_ILZERO:           return iLZeroW;
    case LOCALE_IMEASURE:         return iMeasureW;
    case LOCALE_INEGCURR:         return iNegCurrW;
    case LOCALE_INEGNUMBER:       return iNegNumberW;
    case LOCALE_IPAPERSIZE:       return iPaperSizeW;
    case LOCALE_ITIME:            return iTimeW;
    case LOCALE_S1159:            return s1159W;
    case LOCALE_S2359:            return s2359W;
    case LOCALE_SCURRENCY:        return sCurrencyW;
    case LOCALE_SDATE:            return sDateW;
    case LOCALE_SDECIMAL:         return sDecimalW;
    case LOCALE_SGROUPING:        return sGroupingW;
    case LOCALE_SLIST:            return sListW;
    case LOCALE_SLONGDATE:        return sLongDateW;
    case LOCALE_SMONDECIMALSEP:   return sMonDecimalSepW;
    case LOCALE_SMONGROUPING:     return sMonGroupingW;
    case LOCALE_SMONTHOUSANDSEP:  return sMonThousandSepW;
    case LOCALE_SNEGATIVESIGN:    return sNegativeSignW;
    case LOCALE_SPOSITIVESIGN:    return sPositiveSignW;
    case LOCALE_SSHORTDATE:       return sShortDateW;
    case LOCALE_STHOUSAND:        return sThousandW;
    case LOCALE_STIME:            return sTimeW;
    case LOCALE_STIMEFORMAT:      return sTimeFormatW;
    case LOCALE_SYEARMONTH:       return sYearMonthW;

    /* The following are not listed under MSDN as supported,
     * but seem to be used and also stored in the registry.
     */
    case LOCALE_ICOUNTRY:         return iCountryW;
    case LOCALE_IDATE:            return iDateW;
    case LOCALE_ILDATE:           return iLDateW;
    case LOCALE_ITLZERO:          return iTLZeroW;
    case LOCALE_SCOUNTRY:         return sCountryW;
    case LOCALE_SLANGUAGE:        return sLanguageW;
    }
    return NULL;
}

HKEY RosCreateRegistryKey(void)
{
    static const WCHAR intlW[] = {'C','o','n','t','r','o','l',' ','P','a','n','e','l','\\',
                                  'I','n','t','e','r','n','a','t','i','o','n','a','l',0};
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING nameW;
    HKEY hKey;

    if (RtlOpenCurrentUser( KEY_ALL_ACCESS, &hKey ) != STATUS_SUCCESS) return 0;

    objAttr.Length = sizeof(objAttr);
    objAttr.RootDirectory = hKey;
    objAttr.ObjectName = &nameW;
    objAttr.Attributes = 0;
    objAttr.SecurityDescriptor = NULL;
    objAttr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, intlW );

    if (NtCreateKey( &hKey, KEY_ALL_ACCESS, &objAttr, 0, NULL, 0, NULL ) != STATUS_SUCCESS) hKey = 0;
    NtClose( objAttr.RootDirectory );
    return hKey;
}

INT RosGetRegistryLocaleInfo( LPCWSTR lpValue, LPWSTR lpBuffer, INT nLen )
{
    DWORD dwSize;
    HKEY hKey;
	INT nRet;
    NTSTATUS ntStatus;
    UNICODE_STRING usNameW;
    KEY_VALUE_PARTIAL_INFORMATION *kvpiInfo;
    const int nInfoSize = FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    if (!(hKey = RosCreateRegistryKey())) return -1;

    RtlInitUnicodeString( &usNameW, lpValue );
    dwSize = nInfoSize + nLen * sizeof(WCHAR);

    if (!(kvpiInfo = HeapAlloc( GetProcessHeap(), 0, dwSize )))
    {
        NtClose( hKey );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    ntStatus = NtQueryValueKey( hKey, &usNameW, KeyValuePartialInformation, kvpiInfo, dwSize, &dwSize );
    if (ntStatus == STATUS_BUFFER_OVERFLOW && !lpBuffer) ntStatus = 0;

    if (!ntStatus)
    {
        nRet = (dwSize - nInfoSize) / sizeof(WCHAR);
        
        if (!nRet || ((WCHAR *)kvpiInfo->Data)[nRet - 1])
        {
            if (nRet < nLen || !lpBuffer) nRet++;
            else
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                nRet = 0;
            }
        }
        if (nRet && lpBuffer)
        {
            memcpy( lpBuffer, kvpiInfo->Data, (nRet - 1) * sizeof(WCHAR) );
            lpBuffer[nRet - 1] = 0;
        }
    }
    else
    {
        if (ntStatus == STATUS_OBJECT_NAME_NOT_FOUND) nRet = -1;
        else
        {
            SetLastError( RtlNtStatusToDosError(ntStatus) );
            nRet = 0;
        }
    }
    NtClose( hKey );
    HeapFree( GetProcessHeap(), 0, kvpiInfo );
    return nRet;
}

/*
 * @implemented
 */
int
STDCALL
GetLocaleInfoW (
	LCID Locale,
    LCTYPE  LCType,
    LPWSTR  lpLCData,
    int cchData
    )
{
    LANGID liLangID;
    HRSRC hRsrc;
    HGLOBAL hMem;
    HMODULE hModule;
    INT nRet;
    UINT uiFlags;
    const WCHAR *ch;
    int i;

    if (cchData < 0 || (cchData && !lpLCData))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!cchData) lpLCData = NULL;

    if (Locale == LOCALE_NEUTRAL || Locale == LOCALE_SYSTEM_DEFAULT) Locale = GetSystemDefaultLCID();
    else if (Locale == LOCALE_USER_DEFAULT) Locale = GetUserDefaultLCID();

    uiFlags = LCType & LOCALE_LOCALEINFOFLAGSMASK;
    LCType &= ~LOCALE_LOCALEINFOFLAGSMASK;

    if (!(uiFlags & LOCALE_NOUSEROVERRIDE) && Locale == GetUserDefaultLCID())
    {
        const WCHAR *value = RosGetLocaleValueName(LCType);

        if (value && ((nRet = RosGetRegistryLocaleInfo( value, lpLCData, cchData )) != -1)) return nRet;
    }

    liLangID = LANGIDFROMLCID( Locale );

    if (SUBLANGID(liLangID) == SUBLANG_NEUTRAL)
        liLangID = MAKELANGID(PRIMARYLANGID(liLangID), SUBLANG_DEFAULT);

    hModule = GetModuleHandleA( "kernel32.dll" );
    if (!(hRsrc = FindResourceExW( hModule, RT_STRINGW, (LPCWSTR)((LCType >> 4) + 1), liLangID )))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!(hMem = LoadResource( hModule, hRsrc )))
        return 0;

    ch = LockResource( hMem );
    for (i = 0; i < (LCType & 0x0f); i++) ch += *ch + 1;

    if (uiFlags & LOCALE_RETURN_NUMBER) nRet = sizeof(UINT) / sizeof(WCHAR);
    else nRet = (LCType == LOCALE_FONTSIGNATURE) ? *ch : *ch + 1;

    if (!lpLCData) return nRet;

    if (nRet > cchData)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (uiFlags & LOCALE_RETURN_NUMBER)
    {
        UINT uiNum;
        WCHAR *chEnd, *chTmp = HeapAlloc( GetProcessHeap(), 0, (*ch + 1) * sizeof(WCHAR) );

        if (!chTmp) 
			return 0;

        memcpy( chTmp, ch + 1, *ch * sizeof(WCHAR) );
        chTmp[*ch] = 0;
        uiNum = wcstol( chTmp, &chEnd, 10 );

        if (!*chEnd)
            memcpy( lpLCData, &uiNum, sizeof(uiNum) );
        else
        {
            SetLastError( ERROR_INVALID_FLAGS );
            nRet = 0;
        }
        HeapFree( GetProcessHeap(), 0, chTmp );
    }
    else
    {
        memcpy( lpLCData, ch + 1, *ch * sizeof(WCHAR) );
        if (LCType != LOCALE_FONTSIGNATURE) lpLCData[nRet-1] = 0;
    }
    return nRet;
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
 * @implemented
 */
LCID
STDCALL
GetSystemDefaultLCID (VOID)
{
  return SystemLocale;
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

INT RosGetTimeFormat(LCID Locale, DWORD dwFlags, CONST SYSTEMTIME *lpTime, LPCWSTR lpFormat, LPWSTR lpTimeStr, int cchTime)
{
	INT nPos = 0, nLastFormatPos = 0;
	BOOL bDrop = FALSE;

	while( *lpFormat )
	{
		if (*lpFormat == (WCHAR) '\'')
		{
			lpFormat++;

			while(*lpFormat)
			{
				if (*lpFormat == (WCHAR) '\'')
				{
					lpFormat++;
					if(*lpFormat != (WCHAR) '\'')
						break;
				}
				if (!cchTime)
					nPos++;
				else if(nPos > cchTime)
				{
					SetLastError(ERROR_INSUFFICIENT_BUFFER);
					return 0;
				}
				else
				{
					if(!bDrop)
					{
						lpTimeStr[nPos] = *lpFormat;
						nPos++;
					}
				}
				*lpFormat++;
			}
		}
		else if(*lpFormat=='H' || *lpFormat=='h' || *lpFormat=='m' || *lpFormat=='s' || *lpFormat=='t' )
		{
			int nCount, nBufLen;
			int nType = *lpFormat;
			WCHAR Buffer[40];
			char ch[16];

			bDrop = FALSE;

			Buffer[0] = 0;

			for(nCount = 1; *lpFormat == nType; lpFormat++)
				nCount++;

			switch(nType)
			{
			case 'h':
				{
					if(!(dwFlags & TIME_FORCE24HOURFORMAT))
					{
						sprintf( ch, "%.*d", nCount > 2 ? 2 : nCount,
							lpTime->wHour == 0 ? 12 : (lpTime->wHour - 1) % 12 + 1);
						
		                MultiByteToWideChar( CP_ACP, 0, ch, -1, Buffer, sizeof(Buffer) / sizeof(WCHAR) );

						break;
					}
				}
			case 'H':
				{
					sprintf( ch, "%.*d", nCount > 2 ? 2 : nCount, lpTime->wHour );
					MultiByteToWideChar( CP_ACP, 0, ch, -1, Buffer, sizeof(Buffer)/sizeof(WCHAR) );
					
					break;
				}
			case 'm':
				{
					if(!(dwFlags & TIME_NOMINUTESORSECONDS))
					{
						sprintf( ch, "%.*d", nCount > 2 ? 2 : nCount, lpTime->wMinute );
						MultiByteToWideChar( CP_ACP, 0, ch, -1, Buffer, sizeof(Buffer) / sizeof(WCHAR) );
					}
					else
						nPos = nLastFormatPos;

					break;
				}
			case 's':
				{
					if(!(dwFlags & (TIME_NOSECONDS|TIME_NOMINUTESORSECONDS)))
					{
					    sprintf( ch, "%.*d", nCount > 2 ? 2 : nCount, lpTime->wSecond );
						MultiByteToWideChar( CP_ACP, 0, ch, -1, Buffer, sizeof(Buffer) / sizeof(WCHAR) );				
					}
					else
						nPos = nLastFormatPos;

					break;
				}
			case 't':
				{
					if(!(dwFlags & TIME_NOTIMEMARKER))
					{
						GetLocaleInfoW(Locale, (lpTime->wHour < 12) ? LOCALE_S1159 : LOCALE_S2359, Buffer, sizeof(Buffer) );
						if(nCount == 1)
							Buffer[1] = 0;
					}
					else
					{
						nPos = nLastFormatPos;
						bDrop = TRUE;
					}
					break;
				}
			}
			nBufLen = wcslen(Buffer);

			if(!cchTime)
			{
				/* wine does nothing here?!? */
			}
			else if(nPos + nBufLen < cchTime)
				wcscpy( lpTimeStr + nPos, Buffer );
			else
			{
				lstrcpynW( lpTimeStr + nPos, Buffer, cchTime - nPos );
				
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return 0;
			}
			nPos += nBufLen;
			nLastFormatPos = nPos;
		}
		else
		{
			if(!cchTime)
				nPos++;
			else if(nPos > cchTime)
			{
				SetLastError(ERROR_INSUFFICIENT_BUFFER);
				return 0;
			}
			else
			{
				if(!bDrop)
				{
					lpTimeStr[nPos] = *lpFormat;
					nPos++;
				}
			}
		lpFormat++;
		}
	}

	if (!cchTime)
      /* We are counting */;
	else if (nPos >= cchTime)
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}
	else
		lpTimeStr[nPos] = '\0';

	nPos++;
	return cchTime;
}

/*
 * @implemented
 */
INT
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
	WCHAR Buffer[40];
	SYSTEMTIME t;

    if (!Locale)
		Locale = LOCALE_SYSTEM_DEFAULT;
        
	Locale = ConvertDefaultLocale( Locale );

	if (lpFormat == NULL)
	{
	  if (dwFlags & LOCALE_NOUSEROVERRIDE)
		  Locale = GetSystemDefaultLCID();

	  if (!GetLocaleInfoW(Locale, LOCALE_STIMEFORMAT, Buffer, 40))
		return 0;

	  lpFormat = Buffer;
	}
	if (dwFlags & LOCALE_NOUSEROVERRIDE)
    {
		SetLastError(ERROR_INVALID_FLAGS);
	    return 0;
    }
	if (lpTime == NULL)
	{
		GetLocalTime(&t);
		lpTime = &t;
	}
	if((lpTime->wHour > 24) || (lpTime->wMinute >= 60) || (lpTime->wSecond >= 60))
    {
		SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

	return RosGetTimeFormat(Locale, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);
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
{    return sizeof(lpTimeStr);
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @implemented
 */
LANGID
STDCALL
GetUserDefaultLangID (VOID)
{
    return LANGIDFROMLCID(GetUserDefaultLCID());
}


/*
 * @implemented
 */
LCID
STDCALL
GetUserDefaultLCID (VOID)
{
    LCID lcid;
    NtQueryDefaultLocale(TRUE, &lcid);
    return lcid;
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
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
BOOL
STDCALL
SetUserGeoID(
    GEOID       GeoId)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
