/* $Id: lang.c,v 1.3 2004/01/14 07:22:17 rcampbell Exp $
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
#include <string.h>

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
					if(!(dwFlags & TIME_NOSECONDS) || !(dwFlags & TIME_NOMINUTESORSECONDS))
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
	  GetLocaleInfoW(Locale, LOCALE_STIMEFORMAT, Buffer, 40);
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

	return RosGetTimeFormat(Locale, dwFlags & LOCALE_STIMEFORMAT, lpTime, lpFormat,
                                    lpTimeStr, cchTime);
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
