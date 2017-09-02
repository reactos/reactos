/*
 * PROJECT:     CDFS File System Management
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     DLL Initialization
 * COPYRIGHT:   Copyright 2017 Colin Finck <colin@reactos.org>
 */

#include <windef.h>

INT WINAPI
DllMain(IN HINSTANCE hinstDLL,
    IN DWORD dwReason,
    IN LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(hinstDLL);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpvReserved);

    return TRUE;
}
