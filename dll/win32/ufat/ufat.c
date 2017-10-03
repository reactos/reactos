/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         FAT File System Management
 * FILE:            dll/win32/ufat/ufat.c
 * PURPOSE:         ufat DLL initialisation
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
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
