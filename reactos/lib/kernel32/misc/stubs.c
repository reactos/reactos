/* $Id: stubs.c,v 1.58 2003/09/12 17:51:47 vizzini Exp $
 *
 * KERNEL32.DLL stubs (unimplemented functions)
 * Remove from this file, if you implement them.
 */
#include <k32.h>

//#define _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
BOOL
STDCALL
BaseAttachCompleteThunk (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
CmdBatNotification (
    DWORD   Unknown
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
CompareStringA (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCSTR  lpString1,
    int cchCount1,
    LPCSTR  lpString2,
    int cchCount2
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
CompareStringW (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


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
DWORD
STDCALL
CreateVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
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
DWORD
STDCALL
ExitVDM (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ExtendVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
FoldStringW (
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


/*
 * @unimplemented
 */
int
STDCALL
FoldStringA (
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
BOOL
STDCALL
FreeVirtualBuffer (
    HANDLE  hVirtualBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
WINBOOL
STDCALL
GetBinaryTypeW (
    LPCWSTR lpApplicationName,
    LPDWORD lpBinaryType
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
GetBinaryTypeA (
    LPCSTR  lpApplicationName,
    LPDWORD lpBinaryType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
DWORD
STDCALL
GetNextVDMCommand (
    DWORD   Unknown0
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


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetStringTypeExW (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
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
GetStringTypeExA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
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
GetStringTypeW (
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
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
GetStringTypeA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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

#endif

/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetSystemPowerStatus (
    DWORD   Unknown0
    )
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

#endif

/*
 * @unimplemented
 */
DWORD
STDCALL
GetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
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
IsDBCSLeadByte (
    BYTE    TestChar
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
IsDBCSLeadByteEx (
    UINT    CodePage,
    BYTE    TestChar
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
IsValidLocale (
    LCID    Locale,
    DWORD   dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


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
DWORD
STDCALL
LoadModule (
    LPCSTR  lpModuleName,
    LPVOID  lpParameterBlock
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
RegisterConsoleVDM (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2,
    DWORD   Unknown3,
    DWORD   Unknown4,
    DWORD   Unknown5,
    DWORD   Unknown6,
    DWORD   Unknown7,
    DWORD   Unknown8,
    DWORD   Unknown9,
    DWORD   Unknown10
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
RegisterWowBaseHandlers (
    DWORD   Unknown0
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
RegisterWowExec (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
WINBOOL STDCALL
SetSystemPowerState (
    WINBOOL fSuspend,
    WINBOOL fForce
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
SetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
TrimVirtualBuffer (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VDMConsoleOperation (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VDMOperationStarted (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
DWORD
STDCALL
VerLanguageNameA (
    DWORD   wLang,
    LPSTR   szLang,
    DWORD   nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VerLanguageNameW (
    DWORD   wLang,
    LPWSTR  szLang,
    DWORD   nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

#endif

/*
 * @unimplemented
 */
DWORD
STDCALL
VirtualBufferExceptionHandler (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
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
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
AddRefActCtx(
    HANDLE hActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
AllocateUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
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
AssignProcessToJobObject(
    HANDLE hJob,
    HANDLE hProcess
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
BindIoCompletionCallback (
    HANDLE FileHandle,
    LPOVERLAPPED_COMPLETION_ROUTINE Function,
    ULONG Flags
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
CancelDeviceWakeupRequest(
    HANDLE hDevice
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
CancelTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer
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
ChangeTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateActCtxA(
    PCACTCTXA pActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateActCtxW(
    PCACTCTXW pActCtx
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
CreateJobSet (
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateMemoryResourceNotification(
    MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateTimerQueue(
    VOID
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
CreateTimerQueueTimer(
    PHANDLE phNewTimer,
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags
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
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
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
DeleteTimerQueue(
    HANDLE TimerQueue
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
DeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
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
DeleteTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
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
FindActCtxSectionGuid(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    const GUID *lpGuidToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
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
FindVolumeClose(
    HANDLE hFindVolume
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
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
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
FreeUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
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
GetCurrentActCtx(
    HANDLE *lphActCtx)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetDevicePowerState(
    HANDLE hDevice,
    WINBOOL *pfOn
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
GetFileSizeEx(
    HANDLE hFile,
    PLARGE_INTEGER lpFileSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
GetNativeSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
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
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
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
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
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
GetProcessHandleCount(
    HANDLE hProcess,
    PDWORD pdwHandleCount
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetProcessId(
    HANDLE Process
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
GetProcessIoCounters(
    HANDLE hProcess,
    PIO_COUNTERS lpIoCounters
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
GetProcessPriorityBoost(
    HANDLE hProcess,
    PWINBOOL pDisablePriorityBoost
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
GetSystemRegistryQuota(
    PDWORD pdwQuotaAllowed,
    PDWORD pdwQuotaUsed
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
GetSystemTimes(
    LPFILETIME lpIdleTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
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
GetThreadIOPendingFlag(
    HANDLE hThread,
    PWINBOOL lpIOIsPending
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
GetWriteWatch(
    DWORD  dwFlags,
    PVOID  lpBaseAddress,
    SIZE_T dwRegionSize,
    PVOID *lpAddresses,
    PULONG_PTR lpdwCount,
    PULONG lpdwGranularity
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
GlobalMemoryStatusEx(
    LPMEMORYSTATUSEX lpBuffer
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
HeapQueryInformation (
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL,
    PSIZE_T ReturnLength OPTIONAL
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
HeapSetInformation (
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL
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
InitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
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
IsProcessInJob (
    HANDLE ProcessHandle,
    HANDLE JobHandle,
    PWINBOOL Result
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
IsSystemResumeAutomatic(
    VOID
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
IsWow64Process(
    HANDLE hProcess,
    PWINBOOL Wow64Process
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
MapUserPhysicalPages(
    PVOID VirtualAddress,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
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
MapUserPhysicalPagesScatter(
    PVOID *VirtualAddresses,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenThread(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    DWORD dwThreadId
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
QueryActCtxW(
    DWORD dwFlags,
    HANDLE hActCtx,
    PVOID pvSubInstance,
    ULONG ulInfoClass,
    PVOID pvBuffer,
    SIZE_T cbBuffer OPTIONAL,
    SIZE_T *pcbWrittenOrRequired OPTIONAL
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
QueryInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength,
    LPDWORD lpReturnLength
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
QueryMemoryResourceNotification(
     HANDLE ResourceNotificationHandle,
    PWINBOOL  ResourceState
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
QueueUserAPC(
    PAPCFUNC pfnAPC,
    HANDLE hThread,
    ULONG_PTR dwData
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
QueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
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
ReadDirectoryChangesW(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    WINBOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
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
ReadFileScatter(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToRead,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
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
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ReleaseActCtx(
    HANDLE hActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
RemoveVectoredExceptionHandler(
    PVOID VectoredHandlerHandle
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
RequestDeviceWakeup(
    HANDLE hDevice
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
RequestWakeupLatency(
    LATENCY_TIME latency
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
ResetWriteWatch(
    LPVOID lpBaseAddress,
    SIZE_T dwRegionSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
RestoreLastError(
    DWORD dwErrCode
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
SetCriticalSectionSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
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
SetFilePointerEx(
    HANDLE hFile,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER lpNewFilePointer,
    DWORD dwMoveMethod
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
SetFileValidData(
    HANDLE hFile,
    LONGLONG ValidDataLength
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
SetInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength
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
SetMessageWaitingIndicator(
    HANDLE hMsgIndicator,
    ULONG ulMsgCount
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
SetProcessPriorityBoost(
    HANDLE hProcess,
    WINBOOL bDisablePriorityBoost
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
EXECUTION_STATE
STDCALL
SetThreadExecutionState(
    EXECUTION_STATE esFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
SetTimerQueueTimer(
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    WINBOOL PreferIo
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
TerminateJobObject(
    HANDLE hJob,
    UINT uExitCode
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
TzSpecificLocalTimeToSystemTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpLocalTime,
    LPSYSTEMTIME lpUniversalTime
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
UnregisterWait(
    HANDLE WaitHandle
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
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
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
WriteFileGather(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
WTSGetActiveConsoleSessionId(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
ZombifyActCtx(
    HANDLE hActCtx
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
CheckNameLegalDOS8Dot3W(
    LPCWSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PWINBOOL pbNameContainsSpaces OPTIONAL,
    PWINBOOL pbNameLegal
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
CreateHardLinkW(
    LPCWSTR lpFileName,
    LPCWSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateJobObjectW(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCWSTR lpName
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
DeleteVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint
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
DnsHostnameToComputerNameW (
    LPCWSTR Hostname,
    LPWSTR ComputerName,
    LPDWORD nSize
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
FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeW(
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeMountPointW(
    LPCWSTR lpszRootPathName,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
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
FindNextVolumeW(
    HANDLE hFindVolume,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
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
FindNextVolumeMountPointW(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
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
GetComputerNameExW (
    COMPUTER_NAME_FORMAT NameType,
    LPWSTR lpBuffer,
    LPDWORD nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetDllDirectoryW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetLongPathNameW(
    LPCWSTR lpszShortPath,
    LPWSTR  lpszLongPath,
    DWORD    cchBuffer
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
GetModuleHandleExW(
    DWORD        dwFlags,
    LPCWSTR     lpModuleName,
    HMODULE*    phModule
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
GetSystemWow64DirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
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
GetVolumeNameForVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
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
GetVolumePathNameW(
    LPCWSTR lpszFileName,
    LPWSTR lpszVolumePathName,
    DWORD cchBufferLength
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
GetVolumePathNamesForVolumeNameW(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenJobObjectW(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCWSTR lpName
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
ReplaceFileW(
    LPCWSTR lpReplacedFileName,
    LPCWSTR lpReplacementFileName,
    LPCWSTR lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
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
SetComputerNameExW (
    COMPUTER_NAME_FORMAT NameType,
    LPCWSTR lpBuffer
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
SetDllDirectoryW(
    LPCWSTR lpPathName
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
SetFileShortNameW(
    HANDLE hFile,
    LPCWSTR lpShortName
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
SetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
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
SetVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPCWSTR lpszVolumeName
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
VerifyVersionInfoW(
    LPOSVERSIONINFOEXW lpVersionInformation,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask
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
CheckNameLegalDOS8Dot3A(
    LPCSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PWINBOOL pbNameContainsSpaces OPTIONAL,
    PWINBOOL pbNameLegal
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
CreateHardLinkA(
    LPCSTR lpFileName,
    LPCSTR lpExistingFileName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateJobObjectA(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCSTR lpName
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
DeleteVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint
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
DnsHostnameToComputerNameA (
    LPCSTR Hostname,
    LPSTR ComputerName,
    LPDWORD nSize
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
FindActCtxSectionStringA(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeA(
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeMountPointA(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
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
FindNextVolumeA(
    HANDLE hFindVolume,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
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
FindNextVolumeMountPointA(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
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
GetComputerNameExA (
    COMPUTER_NAME_FORMAT NameType,
    LPSTR lpBuffer,
    LPDWORD nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetDllDirectoryA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetLongPathNameA(
    LPCSTR lpszShortPath,
    LPSTR  lpszLongPath,
    DWORD    cchBuffer
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
GetModuleHandleExA(
    DWORD        dwFlags,
    LPCSTR     lpModuleName,
    HMODULE*    phModule
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
GetSystemWow64DirectoryA(
    LPSTR lpBuffer,
    UINT uSize
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
GetVolumeNameForVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
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
GetVolumePathNameA(
    LPCSTR lpszFileName,
    LPSTR lpszVolumePathName,
    DWORD cchBufferLength
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
GetVolumePathNamesForVolumeNameA(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenJobObjectA(
    DWORD dwDesiredAccess,
    WINBOOL bInheritHandle,
    LPCSTR lpName
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
ReplaceFileA(
    LPCSTR  lpReplacedFileName,
    LPCSTR  lpReplacementFileName,
    LPCSTR  lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
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
SetComputerNameExA (
    COMPUTER_NAME_FORMAT NameType,
    LPCSTR lpBuffer
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
SetDllDirectoryA(
    LPCSTR lpPathName
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
SetFileShortNameA(
    HANDLE hFile,
    LPCSTR lpShortName
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
SetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
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
SetVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPCSTR lpszVolumeName
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
VerifyVersionInfoA(
    LPOSVERSIONINFOEXA lpVersionInformation,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

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
LANGID
STDCALL
GetSystemDefaultUILanguage(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

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
EnumSystemLanguageGroupsA(
    LANGUAGEGROUP_ENUMPROCA lpLanguageGroupEnumProc,
    DWORD                   dwFlags,
    LONG_PTR                lParam)
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
EnumSystemLanguageGroupsW(
    LANGUAGEGROUP_ENUMPROCW lpLanguageGroupEnumProc,
    DWORD                   dwFlags,
    LONG_PTR                lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

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
ULONGLONG
STDCALL
VerSetConditionMask(
        ULONGLONG   ConditionMask,
        DWORD   TypeMask,
        BYTE    Condition
        )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
WINBOOL STDCALL GetConsoleKeyboardLayoutNameA(LPSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
WINBOOL STDCALL GetConsoleKeyboardLayoutNameW(LPWSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetConsoleIcon(HICON hicon)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetHandleContext(HANDLE hnd)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL CreateSocketHandle(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetHandleContext(HANDLE hnd,DWORD context)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetConsoleInputExeNameA(LPCSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetConsoleInputExeNameW(LPCWSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL,
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack,
                        LPVOID lpBuff )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL UTUnRegister( HMODULE hModule )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
FARPROC STDCALL DelayLoadFailureHook(unsigned int dliNotify, PDelayLoadInfo pdli)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL CreateNlsSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,ULONG Size,ULONG AccessMask)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleInputExeNameA(ULONG length,LPCSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleInputExeNameW(ULONG length,LPCWSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL IsValidUILanguage(LANGID langid)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL NlsConvertIntegerToString(ULONG Value,ULONG Base,ULONG strsize, LPWSTR str, ULONG strsize2)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
UINT STDCALL SetCPGlobal(UINT CodePage)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetClientTimeZoneInformation(
		       CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
		       )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
