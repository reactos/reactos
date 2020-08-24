/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReiserFS File System Management
 * PURPOSE:         ureiserfs DLL initialisation
 * PROGRAMMERS:     Pierre Schweitzer
 */

#include <windef.h>

INT WINAPI
DllMain(
    IN HINSTANCE hinstDLL,
    IN DWORD     dwReason,
    IN LPVOID    lpvReserved)
{
    UNREFERENCED_PARAMETER(hinstDLL);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpvReserved);

    return TRUE;
}
