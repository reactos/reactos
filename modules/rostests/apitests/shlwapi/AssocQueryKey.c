/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for AssocQueryKey
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <apitest.h>
#include <windef.h>
#include <shlwapi.h>
#include <pseh/pseh2.h>

typedef enum _MY_KEY_INFORMATION_CLASS
{
    MyKeyBasicInformation = 0,
    MyKeyNodeInformation  = 1,
    MyKeyFullInformation  = 2,
    MyKeyNameInformation  = 3,
} MY_KEY_INFORMATION_CLASS;

typedef struct _MY_KEY_NAME_INFORMATION
{
    ULONG NameLength;
    WCHAR Name[1];
} MY_KEY_NAME_INFORMATION, *PMY_KEY_NAME_INFORMATION;

// ntdll!NtQueryKey can get the key path from HKEY
typedef NTSTATUS (__stdcall *FN_NtQueryKey)(HANDLE, MY_KEY_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static FN_NtQueryKey g_NtQueryKey = NULL;

static BOOL InitNtQueryKey(VOID)
{
    HMODULE hNTDLL = GetModuleHandleW(L"ntdll.dll");
    if (!hNTDLL) return FALSE;
    g_NtQueryKey = (FN_NtQueryKey)GetProcAddress(hNTDLL, "NtQueryKey");
    return g_NtQueryKey != NULL;
}

// Get path of HKEY
static LPWSTR GetKeyPath(HKEY hKey)
{
    if (!g_NtQueryKey || !hKey)
        return NULL;

    ULONG needed = 0;
    g_NtQueryKey(hKey, MyKeyNameInformation, NULL, 0, &needed);

    if (!needed)
        return NULL;

    SIZE_T cb = needed + sizeof(WCHAR);
    PBYTE buf = (PBYTE)_alloca(cb);
    ZeroMemory(buf, cb);

    ULONG returned = 0;
    NTSTATUS status = g_NtQueryKey(hKey, MyKeyNameInformation, buf, (ULONG)cb, &returned);

    if (status < 0)
        return NULL;

    PMY_KEY_NAME_INFORMATION info = (PMY_KEY_NAME_INFORMATION)buf;
    return StrDupW(info->Name); // needs LocalFree
}

// Test ASSOCKEYs
static void TEST_AssocKeys(void)
{
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_SHELLEXECCLASS, L".reg", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path &&
           (StrStrIW(path, L"\\REGISTRY\\MACHINE\\") || StrStrIW(path, L"\\REGISTRY\\USER\\")) &&
           StrStrIW(path, L"regfile"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_APP, L".reg", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path && StrStrIW(path,
                            L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Applications\\regedit.exe"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L".reg", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path &&
           (StrStrIW(path, L"\\REGISTRY\\MACHINE\\") || StrStrIW(path, L"\\REGISTRY\\USER\\")) &&
           StrStrIW(path, L"Classes\\regfile"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L".exe", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path && !_wcsicmp(path, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\exefile"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }
}

// Test ASSOCF flags
static void TEST_AssocF_Flags(void)
{
    const ASSOCF cases[] =
    {
        ASSOCF_NONE,
        ASSOCF_INIT_NOREMAPCLSID,
        ASSOCF_INIT_DEFAULTTOSTAR,
        ASSOCF_INIT_DEFAULTTOFOLDER,
        ASSOCF_NOUSERSETTINGS,
        ASSOCF_NOTRUNCATE,
        ASSOCF_VERIFY,
        ASSOCF_REMAPRUNDLL,
        ASSOCF_NOFIXUPS,
        ASSOCF_IGNOREBASECLASS,
        ASSOCF_INIT_IGNOREUNKNOWN,
        ASSOCF_INIT_NOREMAPCLSID | ASSOCF_NOUSERSETTINGS,
        ASSOCF_NOTRUNCATE | ASSOCF_VERIFY,
    };

    for (size_t i = 0; i < _countof(cases); ++i)
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(cases[i], ASSOCKEY_CLASS, L".reg", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path &&
           (StrStrIW(path, L"\\REGISTRY\\MACHINE\\") || StrStrIW(path, L"\\REGISTRY\\USER\\")),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }
}

// Test pszAssoc
static void TEST_PszAssoc(void)
{
    const wchar_t* exts[] =
    {
        L".txt", L".htm", L".reg", L".xml", L".png", L".jpg", L".zip"
    };

    for (size_t i = 0; i < _countof(exts); ++i)
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, exts[i], NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path &&
           (StrStrIW(path, L"\\REGISTRY\\MACHINE\\") || StrStrIW(path, L"\\REGISTRY\\USER\\")),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    // Direct ProgID
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L"regfile", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path &&
           (StrStrIW(path, L"\\REGISTRY\\MACHINE\\") || StrStrIW(path, L"\\REGISTRY\\USER\\")) &&
           StrStrIW(path, L"regfile"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L"exefile", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path && !_wcsicmp(path, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\exefile"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    // ProgID: InternetShortcut
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L"InternetShortcut", NULL, &hKey);
        ok_hr(hr, S_OK);
        if (hKey)
            RegCloseKey(hKey);
    }
}

// Test invalid arguments
static void TEST_InvalidArgs(void)
{
    // phkeyOut == NULL
    {
        BOOL threw = FALSE;
        HRESULT hr = E_UNEXPECTED;

        _SEH2_TRY
        {
            hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L".txt", NULL, NULL);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = 0xDEADFACE;
            threw = TRUE;
        }
        _SEH2_END;

        ok_hr(hr, 0xDEADFACE);
        ok_int(threw, TRUE);
    }

    // pszAssoc == NULL
    {
        BOOL threw = FALSE;
        HRESULT hr = E_UNEXPECTED;
        HKEY hKey = NULL;

        _SEH2_TRY
        {
            hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, NULL, NULL, &hKey);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = 0xDEADFACE;
            threw = TRUE;
        }
        _SEH2_END;

        ok(hKey == NULL, "hKey was not NULL\n");
        if (hKey)
            RegCloseKey(hKey);

        ok_hr(hr, HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        ok_int(threw, FALSE);
    }

    // pszAssoc == ""
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L"", NULL, &hKey);
        if (hKey)
            RegCloseKey(hKey);
        ok_hr(hr, HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
    }

    // Invalid ASSOCKEY value
    {
        HKEY hKey = NULL;
        BOOL threw = FALSE;
        HRESULT hr = E_UNEXPECTED;

        _SEH2_TRY
        {
            hr = AssocQueryKeyW(ASSOCF_NONE, (ASSOCKEY)0xFFFF, L".txt", NULL, &hKey);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            hr = 0xDEADFACE;
            threw = TRUE;
        }
        _SEH2_END;

        ok(hKey == NULL, "hKey was not NULL\n");
        if (hKey)
            RegCloseKey(hKey);

        ok_hr(hr, HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER));
        ok_int(threw, FALSE);
    }
}

static void TEST_ByExeName(void)
{
    WCHAR notepadPath[MAX_PATH];
    GetSystemDirectoryW(notepadPath, MAX_PATH);
    lstrcatW(notepadPath, L"\\notepad.exe");

    // Full path
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_INIT_BYEXENAME, ASSOCKEY_APP, notepadPath, NULL, &hKey);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok_hr(hr, S_OK);

        ok(path &&
           !_wcsicmp(path, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Applications\\notepad.exe"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    // Short name
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_OPEN_BYEXENAME, ASSOCKEY_APP,
                                    L"regedit.exe", NULL, &hKey);
        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);
        ok_hr(hr, S_OK);

        ok(path &&
           !_wcsicmp(path, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\Applications\\regedit.exe"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    // Non existent
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_INIT_BYEXENAME, ASSOCKEY_APP,
                                    L"__ghost__.exe", NULL, &hKey);
        ok(hKey == NULL, "hKey was not NULL\n");
        if (hKey)
            RegCloseKey(hKey);
        ok(FAILED(hr), "hr was 0x%08lX\n", hr);
    }
}

// Check ASSOCKEY_SHELLEXECCLASS
static void TEST_ShellExecClass(void)
{
    static const LPCWSTR cases[] = { L".txt", L".htm", L".html", L"txtfile" };
    for (size_t i = 0; i < _countof(cases); ++i)
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_SHELLEXECCLASS, cases[i], NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        ok(path && StrStrIW(path, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\"),
           "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }
}

START_TEST(AssocQueryKey)
{
    if (!InitNtQueryKey())
    {
        skip("NtQueryKey not found\n");
        return;
    }

    HRESULT hrCoInit = CoInitialize(NULL);

    TEST_AssocKeys();
    TEST_AssocF_Flags();
    TEST_PszAssoc();
    TEST_InvalidArgs();
    TEST_ByExeName();
    TEST_ShellExecClass();

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
