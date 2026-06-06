/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Time zone API definitions.
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#pragma once

#define _TIMEZONEAPI_H_

//#include <apiset.h>
//#include <apisetcconv.h>
#include <minwindef.h>
#include <minwinbase.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_ZONE_ID_INVALID 0xFFFFFFFF

typedef struct _TIME_ZONE_INFORMATION
{
	LONG Bias;
	WCHAR StandardName[32];
	SYSTEMTIME StandardDate;
	LONG StandardBias;
	WCHAR DaylightName[32];
	SYSTEMTIME DaylightDate;
	LONG DaylightBias;
} TIME_ZONE_INFORMATION,*PTIME_ZONE_INFORMATION,*LPTIME_ZONE_INFORMATION;

typedef struct _TIME_DYNAMIC_ZONE_INFORMATION
{
    LONG Bias;
    WCHAR StandardName[32];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[32];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[128];
    BOOLEAN DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
WINBASEAPI
_Success_(return == ERROR_SUCCESS)
DWORD
WINAPI
EnumDynamicTimeZoneInformation(
    _In_ CONST DWORD dwIndex,
    _Out_ PDYNAMIC_TIME_ZONE_INFORMATION lpTimeZoneInformation);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

WINBASEAPI
BOOL
WINAPI
FileTimeToSystemTime(
    CONST FILETIME *,
    LPSYSTEMTIME);

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
WINBASEAPI
_Success_(return != TIME_ZONE_ID_INVALID)
DWORD
WINAPI
GetDynamicTimeZoneInformation(
    _Out_ PDYNAMIC_TIME_ZONE_INFORMATION pTimeZoneInformation);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_VISTA */

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
WINBASEAPI
_Success_(return == ERROR_SUCCESS)
DWORD
WINAPI
GetDynamicTimeZoneInformationEffectiveYears(
    _In_ CONST DYNAMIC_TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    _Out_ LPDWORD FirstYear,
    _Out_ LPDWORD LastYear);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

WINBASEAPI
DWORD
WINAPI
GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
_Success_(return != FALSE)
BOOL
WINAPI
GetTimeZoneInformationForYear(
    _In_ USHORT wYear,
    _In_opt_ PDYNAMIC_TIME_ZONE_INFORMATION pdtzi,
    _Out_ LPTIME_ZONE_INFORMATION ptzi);
#endif // _WIN32_WINNT >= _WIN32_WINNT_WIN7

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
LocalFileTimeToLocalSystemTime(
    _In_opt_ CONST TIME_ZONE_INFORMATION* timeZoneInformation,
    _In_ CONST FILETIME* localFileTime,
    _Out_ SYSTEMTIME* localSystemTime);
#endif /* NTDDI_VERSION >= NTDDI_WIN10_RS5 */

#if (NTDDI_VERSION >= NTDDI_WIN10_RS5)
WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
LocalSystemTimeToLocalFileTime(
    _In_opt_ CONST TIME_ZONE_INFORMATION* timeZoneInformation,
    _In_ CONST SYSTEMTIME* localSystemTime,
    _Out_ FILETIME* localFileTime);
#endif /* NTDDI_VERSION >= NTDDI_WIN10_RS5 */

#if (_WIN32_WINNT >= _WIN32_WINNT_VISTA)
WINBASEAPI
BOOL
WINAPI
SetDynamicTimeZoneInformation(
    _In_ CONST DYNAMIC_TIME_ZONE_INFORMATION* lpTimeZoneInformation);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_VISTA */

WINBASEAPI
BOOL
WINAPI
SetTimeZoneInformation(
    CONST TIME_ZONE_INFORMATION *);

WINBASEAPI
BOOL
WINAPI
SystemTimeToFileTime(
    const SYSTEMTIME*,
    LPFILETIME);

WINBASEAPI
BOOL
WINAPI
SystemTimeToTzSpecificLocalTime(
    CONST TIME_ZONE_INFORMATION*,
    CONST SYSTEMTIME*,LPSYSTEMTIME);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
SystemTimeToTzSpecificLocalTimeEx(
    _In_opt_ CONST DYNAMIC_TIME_ZONE_INFORMATION* lpTimeZoneInformation,
    _In_ CONST SYSTEMTIME* lpUniversalTime,
    _Out_ LPSYSTEMTIME lpLocalTime);
#endif /* _WIN32_WINNT >= _WIN32_WINNT_WIN8 */

WINBASEAPI
BOOL
WINAPI
TzSpecificLocalTimeToSystemTime(
    CONST TIME_ZONE_INFORMATION* lpTimeZoneInformation,
    CONST SYSTEMTIME* lpLocalTime,
    LPSYSTEMTIME lpUniversalTime);

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
TzSpecificLocalTimeToSystemTimeEx(
    _In_opt_ CONST DYNAMIC_TIME_ZONE_INFORMATION* lpTimeZoneInformation,
    _In_ CONST SYSTEMTIME* lpLocalTime,
    _Out_ LPSYSTEMTIME lpUniversalTime);
#endif /* (_WIN32_WINNT >= _WIN32_WINNT_WIN8) */

#ifdef __cplusplus
} // extern "C"
#endif
