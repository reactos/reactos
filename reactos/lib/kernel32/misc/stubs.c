/* $Id: stubs.c,v 1.35 2002/09/30 21:01:32 hbirr Exp $
 *
 * KERNEL32.DLL stubs (unimplemented functions)
 * Remove from this file, if you implement them.
 */
#include <windows.h>



BOOL
STDCALL
BaseAttachCompleteThunk (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


BOOL
STDCALL
CmdBatNotification (
	DWORD	Unknown
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
CompareStringA (
	LCID	Locale,
	DWORD	dwCmpFlags,
	LPCSTR	lpString1,
	int	cchCount1,
	LPCSTR	lpString2,
	int	cchCount2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
CompareStringW (
	LCID	Locale,
	DWORD	dwCmpFlags,
	LPCWSTR	lpString1,
	int	cchCount1,
	LPCWSTR	lpString2,
	int	cchCount2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


LCID
STDCALL
ConvertDefaultLocale (
	LCID	Locale
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
CreateVirtualBuffer (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
EnumCalendarInfoW (
    CALINFO_ENUMPROC lpCalInfoEnumProc,
    LCID              Locale,
    CALID             Calendar,
    CALTYPE           CalType
    )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}



WINBOOL
STDCALL
EnumCalendarInfoA (
	CALINFO_ENUMPROC	lpCalInfoEnumProc,
	LCID			Locale,
	CALID			Calendar,
	CALTYPE			CalType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumDateFormatsW (
	DATEFMT_ENUMPROC	lpDateFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumDateFormatsA (
	DATEFMT_ENUMPROC	lpDateFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}




WINBOOL
STDCALL
EnumSystemCodePagesW (
	CODEPAGE_ENUMPROC	lpCodePageEnumProc,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumSystemCodePagesA (
	CODEPAGE_ENUMPROC	lpCodePageEnumProc,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumSystemLocalesW (
	LOCALE_ENUMPROC	lpLocaleEnumProc,
	DWORD		dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumSystemLocalesA (
	LOCALE_ENUMPROC	lpLocaleEnumProc,
	DWORD		dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumTimeFormatsW (
	TIMEFMT_ENUMPROC	lpTimeFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
EnumTimeFormatsA (
	TIMEFMT_ENUMPROC	lpTimeFmtEnumProc,
	LCID			Locale,
	DWORD			dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}







DWORD
STDCALL
ExitVDM (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}




BOOL
STDCALL
ExtendVirtualBuffer (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
FoldStringW (
	DWORD	dwMapFlags,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
FoldStringA (
	DWORD	dwMapFlags,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
FormatMessageW (
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPWSTR	lpBuffer,
	DWORD	nSize,
	va_list	* Arguments
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
FormatMessageA (
	DWORD	dwFlags,
	LPCVOID	lpSource,
	DWORD	dwMessageId,
	DWORD	dwLanguageId,
	LPSTR	lpBuffer,
	DWORD	nSize,
	va_list	* Arguments
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


BOOL
STDCALL
FreeVirtualBuffer (
	HANDLE	hVirtualBuffer
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


UINT
STDCALL
GetACP (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

WINBOOL
STDCALL
GetBinaryTypeW (
	LPCWSTR	lpApplicationName,
	LPDWORD	lpBinaryType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetBinaryTypeA (
	LPCSTR	lpApplicationName,
	LPDWORD	lpBinaryType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetCPInfo (
	UINT		a0,
	LPCPINFO	a1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}




WINBOOL
STDCALL
GetComputerNameW (
	LPWSTR lpBuffer,
	LPDWORD nSize
	)
{
	WCHAR	Name [MAX_COMPUTERNAME_LENGTH + 1];
	DWORD	Size = 0;

	/*
	 * FIXME: get the computer's name from
	 * the registry.
	 */
	lstrcpyW( Name, L"ROSHost" ); /* <-- FIXME -- */
	Size = lstrlenW(Name) + 1;
	if (Size > *nSize)
	{
		*nSize = Size;
		SetLastError(ERROR_BUFFER_OVERFLOW);
		return FALSE;
	}
	lstrcpyW( lpBuffer, Name );
	return TRUE;
}


WINBOOL
STDCALL
GetComputerNameA (
	LPSTR	lpBuffer,
	LPDWORD	nSize
	)
{
	WCHAR	Name [MAX_COMPUTERNAME_LENGTH + 1];
	int i;

	if (FALSE == GetComputerNameW(
			Name,
			nSize
			))
	{
		return FALSE;
	}
/* FIXME --> */
/* Use UNICODE to ANSI */
	for ( i=0; Name[i]; ++i )
	{
		lpBuffer[i] = (CHAR) Name[i];
	}
	lpBuffer[i] = '\0';
/* FIXME <-- */
	return TRUE;
}




int
STDCALL
GetCurrencyFormatW (
	LCID			Locale,
	DWORD			dwFlags,
	LPCWSTR			lpValue,
	CONST CURRENCYFMT	* lpFormat,
	LPWSTR			lpCurrencyStr,
	int			cchCurrency
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetCurrencyFormatA (
	LCID			Locale,
	DWORD			dwFlags,
	LPCSTR			lpValue,
	CONST CURRENCYFMT	* lpFormat,
	LPSTR			lpCurrencyStr,
	int			cchCurrency
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}





int
STDCALL
GetDateFormatW (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpDate,
	LPCWSTR			lpFormat,
	LPWSTR			lpDateStr,
	int			cchDate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetDateFormatA (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpDate,
	LPCSTR			lpFormat,
	LPSTR			lpDateStr,
	int			cchDate
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetLocaleInfoW (
	LCID	Locale,
	LCTYPE	LCType,
	LPWSTR	lpLCData,
	int	cchData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetLocaleInfoA (
	LCID	Locale,
	LCTYPE	LCType,
	LPSTR	lpLCData,
	int	cchData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
GetNextVDMCommand (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetNumberFormatW (
	LCID		Locale,
	DWORD		dwFlags,
	LPCWSTR		lpValue,
	CONST NUMBERFMT	* lpFormat,
	LPWSTR		lpNumberStr,
	int		cchNumber
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetNumberFormatA (
	LCID		Locale,
	DWORD		dwFlags,
	LPCSTR		lpValue,
	CONST NUMBERFMT	* lpFormat,
	LPSTR		lpNumberStr,
	int		cchNumber
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


UINT
STDCALL
GetOEMCP (VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 437; /* FIXME: call csrss.exe */
}


WINBOOL
STDCALL
GetStringTypeExW (
	LCID	Locale,
	DWORD	dwInfoType,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetStringTypeExA (
	LCID	Locale,
	DWORD	dwInfoType,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetStringTypeW (
	DWORD	dwInfoType,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
GetStringTypeA (
	LCID	Locale,
	DWORD	dwInfoType,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPWORD	lpCharType
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


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


DWORD
STDCALL
GetSystemPowerStatus (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


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

WINBOOL
STDCALL
GetThreadPriorityBoost (
	HANDLE		hThread,
	DWORD		dwSelector,
	LPLDT_ENTRY	lpSelectorEntry
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
GetThreadSelectorEntry (
	HANDLE		hThread,
	DWORD		dwSelector,
	LPLDT_ENTRY	lpSelectorEntry
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
GetTimeFormatW (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpTime,
	LPCWSTR			lpFormat,
	LPWSTR			lpTimeStr,
	int			cchTime
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
GetTimeFormatA (
	LCID			Locale,
	DWORD			dwFlags,
	CONST SYSTEMTIME	* lpTime,
	LPCSTR			lpFormat,
	LPSTR			lpTimeStr,
	int			cchTime
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


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


DWORD
STDCALL
GetVDMCurrentDirectories (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


WINBOOL
STDCALL
IsDBCSLeadByte (
	BYTE	TestChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsDBCSLeadByteEx (
	UINT	CodePage,
	BYTE	TestChar
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


/**********************************************************************
 * NAME							PRIVATE
 * 	IsInstalledCP@4
 *
 * RETURN VALUE
 * 	TRUE if CodePage is installed in the system.
 */
static
BOOL
STDCALL
IsInstalledCP (
	UINT	CodePage
	)
{
	/* FIXME */
	return TRUE;
}


WINBOOL
STDCALL
IsValidCodePage (
	UINT	CodePage
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
IsValidLocale (
	LCID	Locale,
	DWORD	dwFlags
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


int
STDCALL
LCMapStringA (
	LCID	Locale,
	DWORD	dwMapFlags,
	LPCSTR	lpSrcStr,
	int	cchSrc,
	LPSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


int
STDCALL
LCMapStringW (
	LCID	Locale,
	DWORD	dwMapFlags,
	LPCWSTR	lpSrcStr,
	int	cchSrc,
	LPWSTR	lpDestStr,
	int	cchDest
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
LoadModule (
	LPCSTR	lpModuleName,
	LPVOID	lpParameterBlock
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/***********************************************************************
 *           MulDiv   (KERNEL32.@)
 * RETURNS
 *	Result of multiplication and division
 *	-1: Overflow occurred or Divisor was 0
 */

//FIXME! move to correct file
INT STDCALL MulDiv(
	     INT nMultiplicand,
	     INT nMultiplier,
	     INT nDivisor)
{
#if SIZEOF_LONG_LONG >= 8
    long long ret;

    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      ret = (((long long)nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;
    else
      ret = (((long long)nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

    if ((ret > 2147483647) || (ret < -2147483647)) return -1;
    return ret;
#else
    if (!nDivisor) return -1;

    /* We want to deal with a positive divisor to simplify the logic. */
    if (nDivisor < 0)
    {
      nMultiplicand = - nMultiplicand;
      nDivisor = -nDivisor;
    }

    /* If the result is positive, we "add" to round. else, we subtract to round. */
    if ( ( (nMultiplicand <  0) && (nMultiplier <  0) ) ||
	 ( (nMultiplicand >= 0) && (nMultiplier >= 0) ) )
      return ((nMultiplicand * nMultiplier) + (nDivisor/2)) / nDivisor;

    return ((nMultiplicand * nMultiplier) - (nDivisor/2)) / nDivisor;

#endif
}


/**********************************************************************
 * NAME							EXPORTED
 * 	MultiByteToWideChar@24
 *
 * ARGUMENTS
 * 	CodePage
 *		CP_ACP		ANSI code page
 *		CP_MACCP	Macintosh code page
 *		CP_OEMCP	OEM code page
 *		(UINT)		Any installed code page
 *
 * 	dwFlags
 * 		MB_PRECOMPOSED
 * 		MB_COMPOSITE
 *		MB_ERR_INVALID_CHARS
 *		MB_USEGLYPHCHARS
 *
 * 	lpMultiByteStr
 * 		Input buffer;
 *
 * 	cchMultiByte
 * 		Size of MultiByteStr, or -1 if MultiByteStr is
 * 		NULL terminated;
 *
 * 	lpWideCharStr
 * 		Output buffer;
 *
 * 	cchWideChar
 * 		Size (in WCHAR unit) of WideCharStr, or 0
 * 		if the caller just wants to know how large
 * 		WideCharStr should be for a successful
 * 		conversion.
 *
 * RETURN VALUE
 * 	0 on error; otherwise the number of WCHAR written
 * 	in the WideCharStr buffer.
 *
 * NOTE
 * 	A raw converter for now. It assumes lpMultiByteStr is
 * 	NEVER multi-byte (that is each input character is
 * 	8-bit ASCII) and is ALWAYS NULL terminated.
 * 	FIXME-FIXME-FIXME-FIXME
 */
INT
STDCALL
MultiByteToWideChar (
	UINT	CodePage,
	DWORD	dwFlags,
	LPCSTR	lpMultiByteStr,
	int	cchMultiByte,
	LPWSTR	lpWideCharStr,
	int	cchWideChar
	)
{
	int	InStringLength = 0;
	PCHAR	r;
	PWCHAR	w;
	int	cchConverted;

	/*
	 * Check the parameters.
	 */
	if (	/* --- CODE PAGE --- */
		(	(CP_ACP != CodePage)
			&& (CP_MACCP != CodePage)
			&& (CP_OEMCP != CodePage)
			&& (FALSE == IsInstalledCP (CodePage))
			)
		/* --- FLAGS --- */
		|| (dwFlags & ~(MB_PRECOMPOSED
				| MB_COMPOSITE
				| MB_ERR_INVALID_CHARS
				| MB_USEGLYPHCHARS
				)
			)
		/* --- INPUT BUFFER --- */
		|| (NULL == lpMultiByteStr)
		)
	{
		SetLastError (ERROR_INVALID_PARAMETER);
		return 0;
	}
	/*
	 * Compute the input buffer length.
	 */
	if (-1 == cchMultiByte)
	{
		InStringLength = lstrlen (lpMultiByteStr) + 1;
	}
	else
	{
		InStringLength = cchMultiByte;
	}
	/*
	 * Does caller query for output
	 * buffer size?
	 */
	if (0 == cchWideChar)
	{
		SetLastError (ERROR_SUCCESS);
		return InStringLength;
	}
	/*
	 * Is space provided for the translated
	 * string enough?
	 */
	if (cchWideChar < InStringLength)
	{
		SetLastError (ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}
	/*
	 * Raw 8- to 16-bit conversion.
	 */
	for (	cchConverted = 0,
		r = (PCHAR) lpMultiByteStr,
		w = (PWCHAR) lpWideCharStr;

		cchConverted < InStringLength;

		r++,
		w++,
		cchConverted++
		)
	{
		*w = (WCHAR) *r;
	}
	/*
	 * Return how many characters we
	 * wrote in the output buffer.
	 */
	SetLastError (ERROR_SUCCESS);
	return cchConverted;
}


WINBOOL
STDCALL
QueryPerformanceCounter (
	LARGE_INTEGER	* lpPerformanceCount
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
QueryPerformanceFrequency (
	LARGE_INTEGER	* lpFrequency
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
RegisterConsoleVDM (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8,
	DWORD	Unknown9,
	DWORD	Unknown10
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
RegisterWowBaseHandlers (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
RegisterWowExec (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetComputerNameA (
	LPCSTR	lpComputerName
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetComputerNameW (
    LPCWSTR lpComputerName
    )
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetLocaleInfoA (
	LCID	Locale,
	LCTYPE	LCType,
	LPCSTR	lpLCData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetLocaleInfoW (
	LCID	Locale,
	LCTYPE	LCType,
	LPCWSTR	lpLCData
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


WINBOOL
STDCALL
SetSystemPowerState (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetThreadIdealProcessor(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

WINBOOL
STDCALL
SetThreadLocale (
	LCID	Locale
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}

WINBOOL
STDCALL
SetThreadPriorityBoost(VOID)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

WINBOOL
STDCALL
SetVDMCurrentDirectories (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return FALSE;
}


DWORD
STDCALL
TrimVirtualBuffer (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VDMConsoleOperation (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VDMOperationStarted (
	DWORD	Unknown0
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerLanguageNameA (
	DWORD	wLang,
	LPSTR	szLang,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VerLanguageNameW (
	DWORD	wLang,
	LPWSTR	szLang,
	DWORD	nSize
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


DWORD
STDCALL
VirtualBufferExceptionHandler (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}


/**********************************************************************
 * NAME							EXPORTED
 * 	MultiByteToWideChar@32
 * 
 *	Not yet implemented complete (without NLS so far)
 *
 * ARGUMENTS
 * 	CodePage
 *		CP_ACP ANSI code page 
 *		CP_MACCP Macintosh code page 
 *		CP_OEMCP OEM code page 
 *		CP_SYMBOL Symbol code page (42) 
 *		CP_THREAD_ACP Current thread's ANSI code page 
 *		CP_UTF7 Translate using UTF-7 
 *		CP_UTF8 Translate using UTF-8 
 *		(UINT)		Any installed code page
 *
 * 	dwFlags
 *		WC_NO_BEST_FIT_CHARS	
 *		WC_COMPOSITECHECK Convert composite characters to precomposed characters. 
 *		WC_DISCARDNS Discard nonspacing characters during conversion. 
 *		WC_SEPCHARS Generate separate characters during conversion. This is the default conversion behavior. 
 *		WC_DEFAULTCHAR Replace exceptions with the default character during conversion. 
 *
 * 	lpWideCharStr 
 *		Points to the wide-character string to be converted. 
 *
 * 	cchWideChar
 * 		Size (in WCHAR unit) of WideCharStr, or 0
 * 		if the caller just wants to know how large
 * 		WideCharStr should be for a successful
 * 		conversion.
 *	lpMultiByteStr 
 *		Points to the buffer to receive the translated string. 
 *	cchMultiByte 
 *		Specifies the size in bytes of the buffer pointed to by the 
 *		lpMultiByteStr parameter. If this value is zero, the function 
 *		returns the number of bytes required for the buffer. 
 *	lpDefaultChar 
 *		Points to the character used if a wide character cannot be 
 *		represented in the specified code page. If this parameter is 
 *		NULL, a system default value is used. 
		FIXME: ignored
 *	lpUsedDefaultChar 
 *		Points to a flag that indicates whether a default character was used. 
 *		This parameter may be NULL. 
		FIXME: allways set to FALSE.
 *
 *
 *
 * RETURN VALUE
 * 	0 on error; otherwise the number of bytes written
 * 	in the lpMultiByteStr buffer. Or the number of
 *	bytes needed for the lpMultiByteStr buffer if cchMultiByte is zero.
 *
 * NOTE
 * 	A raw converter for now. It just cuts off the upper 9 Bit.
 *	So the MBCS-string does not contain any LeadCharacters
 * 	FIXME - FIXME - FIXME - FIXME
 */

int
STDCALL
WideCharToMultiByte (
	UINT	CodePage,
	DWORD	dwFlags,
	LPCWSTR	lpWideCharStr,
	int	cchWideChar,
	LPSTR	lpMultiByteStr,
	int	cchMultiByte,
	LPCSTR	lpDefaultChar,
	LPBOOL	lpUsedDefaultChar
	)
{
	int wi, di;  // wide counter, dbcs byte count

	/*
	 * Check the parameters.
	 */
	if (	/* --- CODE PAGE --- */
		(	(CP_ACP != CodePage)
			&& (CP_MACCP != CodePage)
			&& (CP_OEMCP != CodePage)
			&& (CP_SYMBOL != CodePage)
			&& (CP_THREAD_ACP != CodePage)
			&& (CP_UTF7 != CodePage)
			&& (CP_UTF8 != CodePage)
			&& (FALSE == IsInstalledCP (CodePage))
			)
		/* --- FLAGS --- */
		|| (dwFlags & ~(/*WC_NO_BEST_FIT_CHARS
				|*/ WC_COMPOSITECHECK
				| WC_DISCARDNS
				| WC_SEPCHARS
				| WC_DEFAULTCHAR
				)
			)
		/* --- INPUT BUFFER --- */
		|| (NULL == lpWideCharStr)
		)
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	// for now, make no difference but only convert cut the characters to 7Bit
	if( cchWideChar == -1 ) // assume its a 0-terminated str
	{			// and determine its length
		for( cchWideChar=0; lpWideCharStr[cchWideChar]!=0; cchWideChar++)
		cchWideChar++;
	}

	// user wants to determine needed space
	if( cchMultiByte == 0 ) 
	{
		SetLastError(ERROR_SUCCESS);
		return cchWideChar;			// FIXME: determine correct.
	}
	// the lpWideCharStr is cchWideChar characters long.
	for( wi=0, di=0; wi<cchWideChar && di<cchMultiByte; ++wi, ++di)
	{
		// Flag and a not displayable char    FIXME
		/*if( (dwFlags&WC_NO_BEST_FIT_CHARS) && (lpWideCharStr[wi] >127) ) 
		{
			lpMultiByteStr[di]=
			*lpUsedDefaultChar = TRUE;

		}*/
		// FIXME
		// just cut off the upper 9 Bit, since vals>=128 mean LeadByte.
		lpMultiByteStr[di] = lpWideCharStr[wi] & 0x007F;
	}
	// has MultiByte exceeded but Wide is still in the string?
	if( wi < cchWideChar && di >= cchMultiByte)
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}
	// else return # of bytes wirtten to MBCSbuffer (di)
	SetLastError(ERROR_SUCCESS);
	// FIXME: move that elsewhere
	if( lpUsedDefaultChar!=NULL ) *lpUsedDefaultChar=FALSE; 
	return di;
}











/* EOF */
