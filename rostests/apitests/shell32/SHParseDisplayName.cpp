/*
 * PROJECT:         ReactOS API tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for SHParseDisplayName
 * PROGRAMMERS:     Thomas Faber <thomas.faber@reactos.org>
 */

#include "shelltest.h"
#include "apitest.h"
#include <ndk/umtypes.h>
#include <strsafe.h>

START_TEST(SHParseDisplayName)
{
    HRESULT hr;
    PIDLIST_ABSOLUTE pidl;
    WCHAR systemDir[MAX_PATH];
    WCHAR path[MAX_PATH];
    WCHAR resultPath[MAX_PATH];
    BOOL winv6 = LOBYTE(LOWORD(GetVersion())) >= 6;

    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    GetSystemDirectoryW(systemDir, RTL_NUMBER_OF(systemDir));
    SetCurrentDirectoryW(systemDir);

    /* The code below relies on these properties */
    ok(systemDir[1] == L':', "systemDir = %ls\n", systemDir);
    ok(systemDir[2] == L'\\', "systemDir = %ls\n", systemDir);
    ok(systemDir[wcslen(systemDir) - 1] != L'\\', "systemDir = %ls\n", systemDir);
    ok(wcschr(systemDir + 3, L'\\') != NULL, "systemDir = %ls\n", systemDir);

    /* NULL */
    pidl = NULL;
    StartSeh()
        hr = SHParseDisplayName(NULL, NULL, &pidl, 0, NULL);
    EndSeh(STATUS_SUCCESS);
    ok(hr == E_OUTOFMEMORY || hr == E_INVALIDARG, "hr = %lx\n", hr);
    ok(pidl == NULL, "pidl = %p\n", pidl);
    if (pidl) CoTaskMemFree(pidl);

    /* empty string */
    pidl = NULL;
    hr = SHParseDisplayName(L"", NULL, &pidl, 0, NULL);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl != NULL, "pidl = %p\n", pidl);
    resultPath[0] = UNICODE_NULL;
    SHGetPathFromIDListW(pidl, resultPath);
    ok_wstr(resultPath, L"");
    if (pidl) CoTaskMemFree(pidl);

    /* C: */
    path[0] = systemDir[0];
    path[1] = L':';
    path[2] = UNICODE_NULL;
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    if (winv6)
    {
        /* Win7 accepts this and returns C:\ */
        ok(hr == S_OK, "hr = %lx\n", hr);
        ok(pidl != NULL, "pidl = %p\n", pidl);
        resultPath[0] = UNICODE_NULL;
        SHGetPathFromIDListW(pidl, resultPath);
        path[2] = L'\\';
        path[3] = UNICODE_NULL;
        ok(!wcsicmp(resultPath, path), "Got %ls, expected %ls\n", resultPath, path);
    }
    else
    {
        /* Win2003 fails this */
        ok(hr == E_INVALIDARG, "hr = %lx\n", hr);
        ok(pidl == NULL, "pidl = %p\n", pidl);
    }
    if (pidl) CoTaskMemFree(pidl);

    /* C:\ */
    path[0] = systemDir[0];
    path[1] = L':';
    path[2] = L'\\';
    path[3] = UNICODE_NULL;
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl != NULL, "pidl = %p\n", pidl);
    resultPath[0] = UNICODE_NULL;
    SHGetPathFromIDListW(pidl, resultPath);
    ok(!wcsicmp(resultPath, path), "Got %ls, expected %ls\n", resultPath, path);
    if (pidl) CoTaskMemFree(pidl);

    /* C:\\ */
    path[0] = systemDir[0];
    path[1] = L':';
    path[2] = L'\\';
    path[3] = L'\\';
    path[4] = UNICODE_NULL;
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == E_INVALIDARG, "hr = %lx\n", hr);
    ok(pidl == NULL, "pidl = %p\n", pidl);
    if (pidl) CoTaskMemFree(pidl);

    /* C:\ReactOS */
    StringCbCopyW(path, sizeof(path), systemDir);
    wcschr(path + 3, L'\\')[0] = UNICODE_NULL;
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl != NULL, "pidl = %p\n", pidl);
    resultPath[0] = UNICODE_NULL;
    SHGetPathFromIDListW(pidl, resultPath);
    ok(!wcsicmp(resultPath, path), "Got %ls, expected %ls\n", resultPath, path);
    if (pidl) CoTaskMemFree(pidl);

    /* C:\ReactOS\ */
    StringCbCopyW(path, sizeof(path), systemDir);
    wcschr(path + 3, L'\\')[1] = UNICODE_NULL;
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl != NULL, "pidl = %p\n", pidl);
    resultPath[0] = UNICODE_NULL;
    SHGetPathFromIDListW(pidl, resultPath);
    path[wcslen(path) - 1] = UNICODE_NULL;
    ok(!wcsicmp(resultPath, path), "Got %ls, expected %ls\n", resultPath, path);
    if (pidl) CoTaskMemFree(pidl);

    /* C:\ReactOS\system32 */
    StringCbCopyW(path, sizeof(path), systemDir);
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == S_OK, "hr = %lx\n", hr);
    ok(pidl != NULL, "pidl = %p\n", pidl);
    resultPath[0] = UNICODE_NULL;
    SHGetPathFromIDListW(pidl, resultPath);
    ok(!wcsicmp(resultPath, path), "Got %ls, expected %ls\n", resultPath, path);
    if (pidl) CoTaskMemFree(pidl);

    /* C:ntoskrnl.exe */
    path[0] = systemDir[0];
    path[1] = L':';
    path[2] = UNICODE_NULL;
    StringCbCatW(path, sizeof(path), L"ntoskrnl.exe");
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == E_INVALIDARG, "hr = %lx\n", hr);
    ok(pidl == NULL, "pidl = %p\n", pidl);
    if (pidl) CoTaskMemFree(pidl);

    /* ntoskrnl.exe */
    StringCbCopyW(path, sizeof(path), L"ntoskrnl.exe");
    pidl = NULL;
    hr = SHParseDisplayName(path, NULL, &pidl, 0, NULL);
    ok(hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), "hr = %lx\n", hr);
    ok(pidl == NULL, "pidl = %p\n", pidl);
    if (pidl) CoTaskMemFree(pidl);

    CoUninitialize();
}
