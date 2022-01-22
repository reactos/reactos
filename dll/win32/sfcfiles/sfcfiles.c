/*
 * PROJECT:     ReactOS System File Checker
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:        dll/win32/sfcfiles/sfcfiles.c
 * PURPOSE:     List of protected files
 * PROGRAMMERS: Copyright 2022 Eric Kohl (eric.kohl@reactos.org)
 */

#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <ndk/umtypes.h>
#include <sfcfiles.h>


static
PROTECT_FILE_ENTRY
ProtectedFiles[] =
{
    {NULL, L"%systemroot%\\system32\\advapi32.dll", NULL},
    {NULL, L"%systemroot%\\system32\\comctl32.dll", NULL},
    {NULL, L"%systemroot%\\system32\\comdlg32.dll", NULL},
    {NULL, L"%systemroot%\\system32\\kernel32.dll", NULL},
    {NULL, L"%systemroot%\\system32\\ntdll.dll", NULL},
    {NULL, L"%systemroot%\\system32\\ntoskrnl.exe", NULL}
};


BOOL
WINAPI
DllMain(
    _In_ HINSTANCE hInstDLL,
    _In_ DWORD fdwReason,
    _In_ LPVOID lpvReserved)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            break;

        case DLL_PROCESS_DETACH:
            break;
    }

    return TRUE;
}


NTSTATUS
WINAPI
SfcGetFiles(
    _Out_ PPROTECT_FILE_ENTRY *ProtFileData,
    _Out_ PULONG FileCount)
{
    *ProtFileData = ProtectedFiles;
    *FileCount = ARRAYSIZE(ProtectedFiles);
    return STATUS_SUCCESS;
}
