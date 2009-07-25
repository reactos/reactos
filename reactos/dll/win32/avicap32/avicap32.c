/*
 * PROJECT:         avicap32
 * FILE:            dll\win32\avicap32\avicap32.c
 * PURPOSE:         Main file
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>
#include <vfw.h>

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(avicap32);

/*
 * unimplemented
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
    UNIMPLEMENTED;
    return NULL;
}


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
 * unimplemented
 */
BOOL
VFWAPI
capGetDriverDescriptionA(WORD wDriverIndex,
                         LPSTR lpszName,
                         INT cbName,
                         LPSTR lpszVer,
                         INT cbVer)
{
    UNIMPLEMENTED;
    return FALSE;
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
