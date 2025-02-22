/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for SHLoadIndirectString
 * COPYRIGHT:   Copyright 2020 Mark Jansen (mark.jansen@reactos.org)
 */

#include <apitest.h>
#include <shlwapi.h>
#include "resource.h"

#include <pseh/pseh2.h>

static void execute_test(LPCWSTR DllFile)
{
    WCHAR DllBuffer[MAX_PATH + 20];
    WCHAR Buffer[MAX_PATH * 2] = {0};
    HRESULT hr;
    HANDLE hEvent;
    DWORD dwRet;
    HMODULE mod;

    hEvent = CreateEventA(NULL, TRUE, FALSE, "Local\\shlwapi_apitest_evt");

    // Show that it is not signaled
    dwRet = WaitForSingleObject(hEvent, 1);
    ok_hex(dwRet, WAIT_TIMEOUT);

    // Ensure the module is not loaded yet...
    mod = GetModuleHandleW(DllFile);
    if (mod != NULL)
    {
        CloseHandle(hEvent);
        skip("%S loaded, cannot continue\n", DllFile);
        return;
    }

    wsprintfW(DllBuffer, L"@%s,-3", DllFile);

    // Load a string from the dll
    hr = SHLoadIndirectString(DllBuffer, Buffer, _countof(Buffer), NULL);
    ok_hex(hr, S_OK);
    if (SUCCEEDED(hr))
    {
        // Module should still not be loaded
        mod = GetModuleHandleW(DllFile);
        ok_ptr(mod, 0);

        ok_wstr(Buffer, L"Test string one.");

        // DllMain not called..
        dwRet = WaitForSingleObject(hEvent, 1);
        ok_hex(dwRet, WAIT_TIMEOUT);

        // Show that calling DllMain will set the event
        mod = LoadLibraryW(DllFile);
        ok(mod != NULL, "Failed to load %S\n", DllFile);

        if (mod)
        {
            dwRet = WaitForSingleObject(hEvent, 1);
            ok_hex(dwRet, WAIT_OBJECT_0);
            FreeLibrary(mod);
        }
    }
    CloseHandle(hEvent);
}


BOOL extract_resource(const WCHAR* Filename, LPCWSTR ResourceName)
{
    BOOL Success;
    DWORD dwWritten, Size;
    HGLOBAL hGlobal;
    LPVOID pData;
    HANDLE Handle;
    HRSRC hRsrc = FindResourceW(GetModuleHandleW(NULL), ResourceName, (LPCWSTR)10);
    ok(!!hRsrc, "Unable to find %s\n", wine_dbgstr_w(ResourceName));
    if (!hRsrc)
        return FALSE;

    hGlobal = LoadResource(GetModuleHandleW(NULL), hRsrc);
    Size = SizeofResource(GetModuleHandleW(NULL), hRsrc);
    pData = LockResource(hGlobal);

    ok(Size && !!pData, "Unable to load %s\n", wine_dbgstr_w(ResourceName));
    if (!Size || !pData)
        return FALSE;

    Handle = CreateFileW(Filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (Handle == INVALID_HANDLE_VALUE)
    {
        skip("Failed to create temp file %ls, error %lu\n", Filename, GetLastError());
        return FALSE;
    }
    Success = WriteFile(Handle, pData, Size, &dwWritten, NULL);
    ok(Success == TRUE, "WriteFile failed with %lu\n", GetLastError());
    ok(dwWritten == Size, "WriteFile wrote %lu bytes instead of %lu\n", dwWritten, Size);
    CloseHandle(Handle);
    Success = Success && (dwWritten == Size);

    UnlockResource(pData);
    return Success;
}

START_TEST(SHLoadIndirectString)
{
    WCHAR workdir[MAX_PATH], dllpath[MAX_PATH];
    BOOL ret;
    UINT Length;

    ret = GetTempPathW(_countof(workdir), workdir);
    ok(ret, "GetTempPathW error: %lu\n", GetLastError());

    Length = GetTempFileNameW(workdir, L"ntdll", 0, dllpath);
    ok(Length != 0, "GetTempFileNameW failed with %lu\n", GetLastError());

    if (extract_resource(dllpath, (LPCWSTR)101))
    {
        _SEH2_TRY
        {
            execute_test(dllpath);
        }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            ok(0, "Ldr didnt handle exception\n");
        }
        _SEH2_END;
    }
    else
    {
        ok(0, "Failed to extract resource\n");
    }

    DeleteFileW(dllpath);
}
