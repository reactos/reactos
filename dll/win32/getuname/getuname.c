/*
 * PROJECT:         Unicode name dll
 * FILE:            dll\win32\getuname\getuname.c
 * PURPOSE:         Main file
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Baruch Rutman (peterooch at gmail dot com)
 */

#include <stdarg.h>
#include <windef.h>
#include <winuser.h>

HINSTANCE hInstance;

int
WINAPI
GetUName(IN WORD wCharCode,
         OUT LPWSTR lpBuf)
{
    WCHAR szDescription[256];
    int res = LoadStringW(hInstance, wCharCode, szDescription, 256);
    if (res != 0)
    {
        wcscpy(lpBuf, szDescription);
        return 0;
    }
    else
    {
        wcscpy(lpBuf, L"Undefined");
        return 0;
    }
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
            hInstance = hinstDLL;
            break;
    }

    return TRUE;
}
