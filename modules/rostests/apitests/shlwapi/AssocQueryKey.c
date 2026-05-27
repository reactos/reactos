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

typedef NTSTATUS (__stdcall *FN_NtQueryKey)(HANDLE, MY_KEY_INFORMATION_CLASS, PVOID, ULONG, PULONG);
static FN_NtQueryKey g_NtQueryKey = NULL;

static BOOL InitNtQueryKey(VOID)
{
    HMODULE hNTDLL = GetModuleHandleW(L"ntdll.dll");
    if (!hNTDLL) return FALSE;
    g_NtQueryKey = (FN_NtQueryKey)GetProcAddress(hNTDLL, "NtQueryKey");
    return g_NtQueryKey != NULL;
}

// Get full path of HKEY
static LPWSTR GetHKeyFullPath(HKEY hKey)
{
    if (!g_NtQueryKey || !hKey)
        return NULL;

    ULONG needed = 0;
    g_NtQueryKey(hKey, MyKeyNameInformation, NULL, 0, &needed);

    if (needed == 0)
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
static void TEST_AssocKey_txt(void)
{
    static const ASSOCKEY cases[] =
    {
        ASSOCKEY_SHELLEXECCLASS,
        ASSOCKEY_APP,
        ASSOCKEY_CLASS
    };

    for (size_t i = 0; i < _countof(cases); ++i)
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, cases[i], L".txt", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR pszPath = GetHKeyFullPath(hKey);
        if (hKey)
            RegCloseKey(hKey);
        LocalFree(pszPath);
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
        HRESULT hr = AssocQueryKeyW(cases[i], ASSOCKEY_CLASS, L".txt", NULL, &hKey);
        ok_hr(hr, S_OK);

        if (hKey)
            RegCloseKey(hKey);
    }
}

// Test pszAssoc
static void TEST_PszAssoc(void)
{
    const wchar_t* exts[] =
    {
        L".txt", L".htm", L".html", L".xml", L".png", L".jpg",  L".zip"
    };

    for (size_t i = 0; i < _countof(exts); ++i)
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, exts[i], NULL, &hKey);
        ok_hr(hr, S_OK);
        if (hKey)
            RegCloseKey(hKey);
    }

    // Direct ProgID
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, L"txtfile", NULL, &hKey);
        ok_hr(hr, S_OK);

        if (hKey)
            RegCloseKey(hKey);
    }

    // ProgID: InternetShortcut
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS,
                                    L"InternetShortcut", NULL, &hKey);
        ok_hr(hr, S_OK);
        if (hKey)
            RegCloseKey(hKey);
    }

    // ASSOCF_INIT_DEFAULTTOSTAR
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_INIT_DEFAULTTOSTAR, ASSOCKEY_CLASS,
                                    L".xyzzy_nonexistent", NULL, &hKey);
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
    WCHAR notepadPath[MAX_PATH] = {};
    GetSystemDirectoryW(notepadPath, MAX_PATH);
    lstrcatW(notepadPath, L"\\notepad.exe");

    // Full path
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_INIT_BYEXENAME, ASSOCKEY_APP, notepadPath, NULL, &hKey);
        if (hKey)
            RegCloseKey(hKey);
        ok_hr(hr, S_OK);
    }

    // Short name
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(ASSOCF_OPEN_BYEXENAME, ASSOCKEY_APP,
                                    L"notepad.exe", NULL, &hKey);
        if (hKey)
            RegCloseKey(hKey);
        ok_hr(hr, S_OK);
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

// Check ASSOCF_NOUSERSETTINGS
static void TEST_NoUserSettings(void)
{
    static const LPCWSTR exts[] = { L".txt", L".htm", L".xml" };
    for (size_t i = 0; i < _countof(exts); ++i)
    {
        HKEY hkeyDef    = NULL;
        HKEY hkeyNoUser = NULL;
        HRESULT hrDef    = AssocQueryKeyW(ASSOCF_NONE, ASSOCKEY_CLASS, exts[i], NULL, &hkeyDef);
        HRESULT hrNoUser = AssocQueryKeyW(ASSOCF_NOUSERSETTINGS, ASSOCKEY_CLASS, exts[i], NULL,
                                          &hkeyNoUser);
        if (hkeyDef)
            RegCloseKey(hkeyDef);
        if (hkeyNoUser)
            RegCloseKey(hkeyNoUser);

        ok_hr(hrDef, S_OK);
        ok_hr(hrNoUser, S_OK);
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
        if (hKey)
            RegCloseKey(hKey);
    }
}

START_TEST(AssocQueryKey)
{
    if (!InitNtQueryKey())
    {
        skip("NtQueryKey not found\n");
        return;
    }

    TEST_AssocKey_txt();
    TEST_AssocF_Flags();
    TEST_PszAssoc();
    TEST_InvalidArgs();
    TEST_ByExeName();
    TEST_NoUserSettings();
    TEST_ShellExecClass();
}
