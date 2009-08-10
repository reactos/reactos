/*
 * PROJECT:         avicap32
 * FILE:            dll\win32\avicap32\avicap32.c
 * PURPOSE:         Main file
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>
#include <winternl.h>
#include <vfw.h>

#include "wine/debug.h"

#define CAP_DESC_MAX 32

WINE_DEFAULT_DEBUG_CHANNEL(avicap32);


/*
 * unimplemented
 */
HWND
VFWAPI
capCreateCaptureWindowW(LPCWSTR lpszWindowName,
                        DWORD dwStyle,
                        INT x,
                        INT y,
                        INT nWidth,
                        INT nHeight,
                        HWND hWnd,
                        INT nID)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * implemented
 */
HWND
VFWAPI
capCreateCaptureWindowA(LPCSTR lpszWindowName,
                        DWORD dwStyle,
                        INT x,
                        INT y,
                        INT nWidth,
                        INT nHeight,
                        HWND hWnd,
                        INT nID)
{
    UNICODE_STRING Name;
    HWND Wnd;

    if (lpszWindowName)
        RtlCreateUnicodeStringFromAsciiz(&Name, lpszWindowName);
    else
        Name.Buffer = NULL;

    Wnd = capCreateCaptureWindowW(Name.Buffer,
                                  dwStyle,
                                  x, y,
                                  nWidth,
                                  nHeight,
                                  hWnd,
                                  nID);

    RtlFreeUnicodeString(&Name);
    return Wnd;
}


/*
 * unimplemented
 */
BOOL
VFWAPI
capGetDriverDescriptionW(WORD wDriverIndex,
                         LPWSTR lpszName,
                         INT cbName,
                         LPWSTR lpszVer,
                         INT cbVer)
{
    UNIMPLEMENTED;
    return FALSE;
}


/*
 * implemented
 */
BOOL
VFWAPI
capGetDriverDescriptionA(WORD wDriverIndex,
                         LPSTR lpszName,
                         INT cbName,
                         LPSTR lpszVer,
                         INT cbVer)
{
    WCHAR DevName[CAP_DESC_MAX], DevVer[CAP_DESC_MAX];
    BOOL Result;

    Result = capGetDriverDescriptionW(wDriverIndex, DevName, CAP_DESC_MAX, DevVer, CAP_DESC_MAX);
    if (Result)
    {
        WideCharToMultiByte(CP_ACP, 0, DevName, -1, lpszName, cbName, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, DevVer, -1, lpszVer, cbVer, NULL, NULL);
    }

    return Result;
}


BOOL
WINAPI
DllMain(IN HINSTANCE hinstDLL,
        IN DWORD dwReason,
        IN LPVOID lpvReserved)
{
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            break;
    }

    return TRUE;
}
