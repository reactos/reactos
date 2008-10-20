/*
 * PROJECT:         Unicode name dll
 * FILE:            dll\win32\getuname\getuname.c
 * PURPOSE:         Main file
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>

int
WINAPI
GetUName(IN WORD wCharCode,
         OUT LPWSTR lpBuf)
{
	wcscpy(lpBuf, L"Undefined");
    return 0;
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
