/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later.html)
 * PURPOSE:     RTL locale support
 * COPYRIGHT:   Copyright 2016 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2021 Jérôme Gardou <jerome.gardou@reactos.org>
 */

#include "precomp.h"

static BOOL (WINAPI *pWow64DisableWow64FsRedirection)(PVOID *);
static BOOL (WINAPI *pWow64RevertWow64FsRedirection)(PVOID);
HANDLE hKernel32;

static PVOID LoadCodePageData(_In_ ULONG Code)
{
    char filename[MAX_PATH], sysdir[MAX_PATH];
    HANDLE hFile;
    PVOID Data = NULL;
    PVOID FsRedir;

    if (!hKernel32)
    {
        hKernel32 = GetModuleHandleA("kernel32.dll");

        pWow64DisableWow64FsRedirection = (void*)GetProcAddress(hKernel32, "Wow64DisableWow64FsRedirection");
        pWow64RevertWow64FsRedirection = (void*)GetProcAddress(hKernel32, "Wow64RevertWow64FsRedirection");
    }

    if (pWow64DisableWow64FsRedirection)
        pWow64DisableWow64FsRedirection(&FsRedir);

    GetSystemDirectoryA(sysdir, MAX_PATH);

    if (Code != -1)
        StringCbPrintfA(filename, sizeof(filename), "%s\\c_%lu.nls", sysdir, Code);
    else
        StringCbPrintfA(filename, sizeof(filename), "%s\\l_intl.nls", sysdir);

    hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Failed to open %s, error %u\n", filename, (UINT)GetLastError());
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD dwRead;
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        Data = malloc(dwFileSize);
        ReadFile(hFile, Data, dwFileSize, &dwRead, NULL);
        CloseHandle(hFile);
    }

    if (pWow64RevertWow64FsRedirection)
        pWow64RevertWow64FsRedirection(FsRedir);

    return Data;
}

/* https://www.microsoft.com/resources/msdn/goglobal/default.mspx */
void SetupLocale(
    _In_ ULONG AnsiCode,
    _In_ ULONG OemCode,
    _In_ ULONG Unicode)
{
    NLSTABLEINFO NlsTable;
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;

    AnsiCodePageData = LoadCodePageData(AnsiCode);
    OemCodePageData = LoadCodePageData(OemCode);
    UnicodeCaseTableData = LoadCodePageData(Unicode);

    RtlInitNlsTables(AnsiCodePageData, OemCodePageData, UnicodeCaseTableData, &NlsTable);
    RtlResetRtlTranslations(&NlsTable);
    /*
     * Do NOT free the buffers here, they are directly used!
     * Yes, we leak the old buffers, but this is a test anyway...
     */
}
