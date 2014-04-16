/*
 * reactos/lib/gdi32/misc/eng.c
 *
 * GDI32.DLL eng part
 *
 *
 */

#include "config.h"

/* Definitions */
#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H
#define NTOS_MODE_USER

#include <stdarg.h>

/* SDK/DDK/NDK Headers. */
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <objbase.h>
#include <ndk/rtlfuncs.h>
#include <wingdi.h>
#define _ENGINE_EXPORT_
#include <winddi.h>
#include <winuser.h>

#include "gdi_private.h"


/*
 * @implemented
 */
VOID
WINAPI
EngAcquireSemaphore ( IN HSEMAPHORE hsem )
{
    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)hsem);
}


/*
 * @implemented
 */
HSEMAPHORE
WINAPI
EngCreateSemaphore ( VOID )
{
    PRTL_CRITICAL_SECTION CritSect = RtlAllocateHeap(GetProcessHeap(), 0, sizeof(RTL_CRITICAL_SECTION));
    if (!CritSect)
    {
        return NULL;
    }

    RtlInitializeCriticalSection( CritSect );
    return (HSEMAPHORE)CritSect;
}

/*
 * @implemented
 */
VOID
WINAPI
EngDeleteSemaphore ( IN HSEMAPHORE hsem )
{
    if (hsem)
    {
        RtlDeleteCriticalSection( (PRTL_CRITICAL_SECTION) hsem );
        RtlFreeHeap( GetProcessHeap(), 0, hsem );
    }
}

/*
 * @implemented
 */
PVOID WINAPI
EngFindResource(HANDLE h,
                int iName,
                int iType,
                PULONG pulSize)
{
    HRSRC HRSrc;
    DWORD Size = 0;
    HGLOBAL Hg;
    LPVOID Lock = NULL;

    HRSrc = FindResourceW((HMODULE)h, MAKEINTRESOURCEW(iName), MAKEINTRESOURCEW(iType));
    if (HRSrc != NULL)
    {
        Size = SizeofResource((HMODULE)h, HRSrc);
        if (Size != 0)
        {
            Hg = LoadResource((HMODULE)h, HRSrc);
            if (Hg != NULL)
            {
                Lock = LockResource( Hg );
            }
        }
    }

    *pulSize = Size;
    return (PVOID) Lock;
}

/*
 * @implemented
 */
VOID WINAPI
EngFreeModule(HANDLE h)
{
    FreeLibrary(h);
}

/*
 * @implemented
 */

VOID WINAPI
EngGetCurrentCodePage( OUT PUSHORT OemCodePage,
                       OUT PUSHORT AnsiCodePage)
{
    *OemCodePage  = GetOEMCP();
    *AnsiCodePage = GetACP();
}

/*
 * @implemented
 */
HANDLE WINAPI
EngLoadModule(LPWSTR pwsz)
{
    return LoadLibraryExW ( pwsz, NULL, LOAD_LIBRARY_AS_DATAFILE);
}

/*
 * @implemented
 */
INT WINAPI
EngMultiByteToWideChar(UINT CodePage,
                       LPWSTR WideCharString,
                       INT BytesInWideCharString,
                       LPSTR MultiByteString,
                       INT BytesInMultiByteString)
{
    return MultiByteToWideChar(CodePage,0,MultiByteString,BytesInMultiByteString,WideCharString,BytesInWideCharString / sizeof(WCHAR));
}

/*
 * @implemented
 */
VOID WINAPI
EngQueryLocalTime(PENG_TIME_FIELDS etf)
{
    SYSTEMTIME SystemTime;
    GetLocalTime( &SystemTime );
    etf->usYear    = SystemTime.wYear;
    etf->usMonth   = SystemTime.wMonth;
    etf->usWeekday = SystemTime.wDayOfWeek;
    etf->usDay     = SystemTime.wDay;
    etf->usHour    = SystemTime.wHour;
    etf->usMinute  = SystemTime.wMinute;
    etf->usSecond  = SystemTime.wSecond;
    etf->usMilliseconds = SystemTime.wMilliseconds;
}

/*
 * @implemented
 */
VOID
WINAPI
EngReleaseSemaphore ( IN HSEMAPHORE hsem )
{
    RtlLeaveCriticalSection( (PRTL_CRITICAL_SECTION) hsem);
}




/*
 * @implemented
 */
INT
WINAPI
EngWideCharToMultiByte( UINT CodePage,
                        LPWSTR WideCharString,
                        INT BytesInWideCharString,
                        LPSTR MultiByteString,
                        INT BytesInMultiByteString)
{
    return WideCharToMultiByte(CodePage, 0, WideCharString, (BytesInWideCharString/sizeof(WCHAR)),
                               MultiByteString, BytesInMultiByteString, NULL, NULL);
}
