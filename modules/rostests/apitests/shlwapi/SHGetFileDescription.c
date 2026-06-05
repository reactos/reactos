/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for SHGetFileDescriptionA/W
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlwapi.h>
#include <strsafe.h>

static WCHAR g_notepad[MAX_PATH];
static WCHAR g_winver[MAX_PATH];

typedef BOOL (WINAPI *FN_SHGetFileDescriptionW)(PCWSTR, PCWSTR, PCWSTR, PWSTR, PUINT);
static FN_SHGetFileDescriptionW g_fnSHGetFileDescriptionW = NULL;

static BOOL LoadFunctions(void)
{
    HMODULE hSHLWAPI = GetModuleHandleW(L"shlwapi.dll");
    if (!hSHLWAPI)
        hSHLWAPI = LoadLibraryW(L"shlwapi.dll");
    if (!hSHLWAPI)
        return FALSE;

    g_fnSHGetFileDescriptionW =
        (FN_SHGetFileDescriptionW)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(348));

    return g_fnSHGetFileDescriptionW != NULL;
}

static BOOL BuildSystemPaths(void)
{
    WCHAR sys[MAX_PATH];
    if (!GetSystemDirectoryW(sys, _countof(sys)))
        return FALSE;

    StringCchPrintfW(g_notepad, _countof(sys), L"%ls\\notepad.exe", sys);
    StringCchPrintfW(g_winver,  _countof(sys), L"%ls\\winver.exe",  sys);
    return PathFileExistsW(g_notepad) && PathFileExistsW(g_winver);
}

static void TEST_QuerySizeOnly(void)
{
    UINT cch = 0;
    BOOL ret = g_fnSHGetFileDescriptionW(g_notepad, NULL, NULL, NULL, &cch);

    ok_int(ret, TRUE);
    ok(cch > 0, "cch was %u\n", cch);
}

static void TEST_GetDescriptionNotepad(void)
{
    WCHAR buf[256] = L"";
    UINT cch = _countof(buf);
    BOOL ret = g_fnSHGetFileDescriptionW(g_notepad, NULL, NULL, buf, &cch);

    ok_int(ret, TRUE);
    ok(buf[0] != UNICODE_NULL, "buf was empty\n");
    ok(0 < cch && cch <= _countof(buf), "cch was %u\n", cch);
    trace("buf: %s\n", wine_dbgstr_w(buf));
}

static void TEST_GetDescriptionWinver(void)
{
    WCHAR buf[256] = L"";
    UINT cch = _countof(buf);
    BOOL ret = g_fnSHGetFileDescriptionW(g_winver, NULL, NULL, buf, &cch);

    ok_int(ret, TRUE);
    ok(buf[0] != UNICODE_NULL, "buf was empty\n");
    ok(0 < cch && cch <= _countof(buf), "cch was %u\n", cch);
    trace("buf: %s\n", wine_dbgstr_w(buf));
}

static void TEST_NonExistentFile(void)
{
    WCHAR buf[256] = L"";
    UINT  cch = _countof(buf);
    BOOL  ret = g_fnSHGetFileDescriptionW(L"C:\\This\\Does\\Not\\Exist.exe", NULL, NULL,
                                          buf, &cch);
    ok_int(ret, FALSE);
}

static void TEST_DirectoryPath(void)
{
    WCHAR sys[MAX_PATH];
    GetSystemDirectoryW(sys, _countof(sys));

    WCHAR buf[256] = L"";
    UINT cch = _countof(buf);
    BOOL ret = g_fnSHGetFileDescriptionW(sys, NULL, NULL, buf, &cch);

    ok_int(ret, TRUE);
    ok(_wcsicmp(buf, L"System32") == 0, "buf was %s\n", wine_dbgstr_w(buf));
}

static void TEST_TinyBuffer(void)
{
    WCHAR buf[1] = { L'X' };
    UINT cch = 1;
    BOOL ret = g_fnSHGetFileDescriptionW(g_notepad, NULL, NULL, buf, &cch);

    ok_int(ret, TRUE);
    ok_int(buf[0], UNICODE_NULL);
    ok_int(cch, 1);
}

static void TEST_CustomVerKey(void)
{
    WCHAR buf[256] = L"";
    UINT  cch = _countof(buf);
    BOOL  ret = g_fnSHGetFileDescriptionW(g_notepad,
                                          L"\\StringFileInfo\\040904B0\\FileDescription",
                                          NULL, buf, &cch);
    ok_int(ret, TRUE);
    ok(buf[0] != UNICODE_NULL, "buf was empty\n");
}

static void TEST_InvalidVerKey(void)
{
    WCHAR buf[256] = L"", bufNorm[256] = L"";
    UINT cch = _countof(buf), cchNorm = _countof(bufNorm);

    BOOL retInvalid = g_fnSHGetFileDescriptionW(g_notepad,
                                                L"\\StringFileInfo\\FFFFFFFF\\NoSuchKey",
                                                NULL, buf, &cch);
    BOOL retNormal = g_fnSHGetFileDescriptionW(g_notepad, NULL, NULL, bufNorm, &cchNorm);

    ok_int(retInvalid, TRUE);
    ok_int(retNormal, TRUE);
    ok(buf[0] != UNICODE_NULL, "buf was empty\n");
    ok(lstrcmpW(buf, bufNorm) == 0, "buf was %s, bufNorm was %s\n", wine_dbgstr_w(buf),
       wine_dbgstr_w(bufNorm));
}

START_TEST(SHGetFileDescription)
{
    if (!LoadFunctions())
    {
        skip("SHGetFileDescription not found\n");
        return;
    }

    if (!BuildSystemPaths())
    {
        skip("notepad.exe and/or winver.exe not found\n");
        return;
    }

    TEST_QuerySizeOnly();
    TEST_GetDescriptionNotepad();
    TEST_GetDescriptionWinver();
    TEST_NonExistentFile();
    TEST_DirectoryPath();
    TEST_TinyBuffer();
    TEST_CustomVerKey();
    TEST_InvalidVerKey();
}
