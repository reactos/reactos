/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for AssocQueryKey
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 *              Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#include <apitest.h>
#include <windef.h>
#include <shlwapi.h>
#include <pseh/pseh2.h>
#include <shlobj.h> // LPDBLIST, LPSHELLFOLDER, OLECMD for shlwapi_undoc.h:
#include <shlwapi_undoc.h>
#if NTDDI_VERSION < NTDDI_WIN10_RS1
#define ASSOCF_PER_MACHINE_ONLY 0x00008000
#endif

static const WCHAR pszUniqueExt[] = L".RosTestExtShlwapi";
static const WCHAR pszUniquePid[] = L"FEF31D544EA64C8EBC50B8719887688A";
static const WCHAR pszUniquePid2[] = L"88FDE0879CD24AEBBEB690988BD87706";

static void Cleanup(void)
{
    SHDeleteKeyW(HKEY_CLASSES_ROOT, pszUniqueExt), SHDeleteKeyW(HKEY_CLASSES_ROOT, pszUniqueExt);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, pszUniquePid), SHDeleteKeyW(HKEY_CLASSES_ROOT, pszUniquePid);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, pszUniquePid2), SHDeleteKeyW(HKEY_CLASSES_ROOT, pszUniquePid2);
}

enum
{
    HR_FNF = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
    HR_NF = HRESULT_FROM_WIN32(ERROR_NOT_FOUND),
    HR_NOASSOC = HRESULT_FROM_WIN32(ERROR_NO_ASSOCIATION),
    HR_ACCDENIED = HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED),
    AF_DEFSTAR = ASSOCF_INIT_DEFAULTTOSTAR,
    AF_DEFFOLDER = ASSOCF_INIT_DEFAULTTOFOLDER,
};

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

static BOOL IsValidClassesRootPath(LPCWSTR path)
{
    LPCWSTR base;
    if (!path)
        return FALSE;
    if (path == StrStrIW(path, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Classes\\"))
        return TRUE;
    base = StrStrIW(path, L"\\REGISTRY\\USER\\");
    return base && StrStrIW(path, L"_CLASSES\\") > base;
}

static HRESULT SHELL_SetRegString(HKEY hKey, LPCWSTR pszSubKey, LPCWSTR pszName, UINT Type, LPCWSTR pszData)
{
    ULONG err = SHSetValueW(hKey, pszSubKey, pszName, Type, pszData, (lstrlenW(pszData) + 1) * sizeof(*pszData));
    return HRESULT_FROM_WIN32(err);
}

static HRESULT AQS(ASSOCF flags, ASSOCSTR str, LPCWSTR pszAsso, LPCWSTR pszExtra, LPWSTR pszOut)
{
    DWORD cch = MAX_PATH;
    return AssocQueryStringW(flags, str, pszAsso, pszExtra, pszOut, &cch);
}

static HRESULT GetAqkPath(UINT InitF, ASSOCKEY AssocKey, LPCWSTR pszInit, LPCWSTR pszExtra, LPWSTR *ppszKey)
{
    HKEY hKey = NULL;
    HRESULT hr = AssocQueryKeyW((ASSOCF)InitF, AssocKey, pszInit, pszExtra, &hKey);
    if (ppszKey)
        *ppszKey = GetKeyPath(hKey);
    if (SUCCEEDED(hr) && hKey)
        RegCloseKey(hKey);
    return hr == S_OK && !hKey ? S_FALSE : hr;
}

#define ok_AqkPath(InitF, AssocKey, pszInit, pszExtra, hrExpected, pszExpectedWin32) do \
{ \
    LPCWSTR pszExpected = L"\\REGISTRY\\MACHINE\\" pszExpectedWin32; \
    LPWSTR pszPath; \
    HRESULT hr = GetAqkPath((InitF), (AssocKey), (pszInit), (pszExtra), &pszPath); \
    ok_hr(hr, (hrExpected)); \
    if (SUCCEEDED(hr)) \
    { \
        ok(pszPath && !_wcsicmp(pszPath, pszExpected), "Got \"%s\", expected \"%s\"\n", \
                                                       wine_dbgstr_w(pszPath), wine_dbgstr_w(pszExpected)); \
    } \
    LocalFree(pszPath); \
} while (0)

static HKEY CreateClassesKey(HKEY hRoot)
{
    if (hRoot == HKEY_CLASSES_ROOT)
        return hRoot;
    if (RegCreateKeyExW(hRoot, L"Software\\Classes", 0, NULL, 0, KEY_WRITE | KEY_READ, NULL, &hRoot, NULL))
        return NULL;
    return hRoot;
}

static HKEY CreateClassesSubKey(HKEY hRoot, LPCWSTR pszSubKey)
{
    hRoot = CreateClassesKey(hRoot);
    if (hRoot && pszSubKey)
    {
        HKEY hSubKey;
        LONG err = RegCreateKeyExW(hRoot, pszSubKey, 0, NULL, 0, KEY_WRITE | KEY_READ, NULL, &hSubKey, NULL);
        if (hRoot != HKEY_CLASSES_ROOT)
            RegCloseKey(hRoot);
        return err ? NULL : hSubKey;
    }
    return hRoot;
}

static void TestClassKey(ASSOCKEY AssocKey)
{
    BOOL nt5 = LOBYTE(GetVersion()) < 6;
    HKEY hSource, hClass, hClass2;
    HRESULT hr, hrExpect;
    WCHAR buf[MAX_PATH], *pszPath;
    Cleanup();

    // A ProgId that does not exist
    hr = GetAqkPath(ASSOCF_NONE, AssocKey, pszUniquePid, NULL, NULL);
    hrExpect = nt5 ? (E_FAIL) : (AssocKey == ASSOCKEY_SHELLEXECCLASS ? HR_NOASSOC : HR_NF);
    ok_hr(hr, hrExpect);

    // An extension that does not exist (could map to Unknown)
    hr = GetAqkPath(ASSOCF_NONE, AssocKey, pszUniqueExt, NULL, NULL);
    hrExpect = nt5 ? E_FAIL : S_OK;
    ok_hr(hr, hrExpect);

    // An extension that does not exist
    hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, NULL, NULL);
    hrExpect = nt5 ? E_FAIL : (AssocKey == ASSOCKEY_SHELLEXECCLASS ? HR_NOASSOC : S_OK);
    ok_hr(hr, hrExpect);

    hSource = CreateClassesSubKey(HKEY_CURRENT_USER, pszUniqueExt);
    if (!hSource)
    {
        skip("Could not create test key\n");
        return;
    }

    // An extension with no ProgId (could map to Unknown)
    hr = GetAqkPath(ASSOCF_NONE, AssocKey, pszUniqueExt, NULL, NULL);
    hrExpect = nt5 && AssocKey == ASSOCKEY_SHELLEXECCLASS ? HR_FNF : S_OK;
    ok_hr(hr, hrExpect);

    // An extension with no ProgId
    hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, NULL, NULL);
    hrExpect = AssocKey == ASSOCKEY_SHELLEXECCLASS ? (nt5 ? HR_FNF : HR_NOASSOC) : S_OK;
    ok_hr(hr, hrExpect);

    // An extension with only a fallback command
    if (SHELL_SetRegString(hSource, L"shell\\open\\command", NULL, REG_SZ, L"calc.exe"))
    {
        skip("Could not create test key\n");
    }
    else
    {
        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, NULL, NULL);
        ok_hr(hr, S_OK);

        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, L"open", NULL);
        ok_hr(hr, S_OK);

        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, L"", NULL);
        ok_hr(hr, S_OK); // Strange but true

        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, L"badverb", NULL);
        hrExpect = AssocKey == ASSOCKEY_SHELLEXECCLASS ? (nt5 ? HR_FNF : HR_NOASSOC) : S_OK;
        ok_hr(hr, hrExpect);
        SHDeleteKeyW(hSource, L"shell");
    }

    // ProgId
    if ((hClass = CreateClassesSubKey(HKEY_CURRENT_USER, pszUniquePid)) == NULL)
    {
        skip("Could not create test key\n");
    }
    else
    {
        SHELL_SetRegString(hSource, NULL, NULL, REG_SZ, pszUniquePid);
        if (SHELL_SetRegString(hClass, L"shell\\calcverb\\command", NULL, REG_SZ, L"calc.exe"))
        {
            skip("Could not create test key\n");
            goto skip_progidtests;
        }

        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, NULL, NULL);
        ok_hr(hr, S_OK);

        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, L"calcverb", NULL);
        ok_hr(hr, S_OK);

        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN, AssocKey, pszUniqueExt, L"open", NULL);
        hrExpect = AssocKey == ASSOCKEY_SHELLEXECCLASS ? (nt5 ? HR_FNF : HR_NOASSOC) : S_OK;
        ok_hr(hr, hrExpect);

        hr = GetAqkPath(ASSOCF_INIT_IGNOREUNKNOWN | ASSOCF_NOUSERSETTINGS, AssocKey, pszUniqueExt, NULL, NULL);
        ok_hr(hr, S_OK);

        // CurVer redirection
        if ((hClass2 = CreateClassesSubKey(HKEY_CURRENT_USER, pszUniquePid2)) == NULL)
        {
            skip("Could not create test key\n");
            goto skip_progidtests;
        }
        else
        {
            LPCWSTR pszExe2 = L"calc.exe";
            ASSOCMAKEVERB verbs2[] =
            {
                { L"calcverb2", NULL, NULL, pszExe2, L"CurVerParam" },
            };
            ASSOCMAKESHELL shell2 = { verbs2, _countof(verbs2), 0 };
            AssocMakeShell(0, hClass2, pszExe2, &shell2);
            SHELL_SetRegString(hClass, L"CurVer", NULL, REG_SZ, pszUniquePid2);

            hr = GetAqkPath(ASSOCF_NONE, AssocKey, pszUniqueExt, NULL, &pszPath);
            ok_hr(hr, S_OK);
            ok(pszPath && StrStrIW(pszPath, pszUniquePid2), "CurVer ProgId redirection\n");
            LocalFree(pszPath), pszPath = NULL;

            hr = AQS(ASSOCF_NONE, ASSOCSTR_COMMAND, pszUniqueExt, NULL, buf);
            ok(SUCCEEDED(hr) && StrStrIW(buf, verbs2[0].pszArgs), "CurVer ProgId redirection\n");

            SHDeleteKeyW(hClass2, L"");
            RegCloseKey(hClass2);
        }

skip_progidtests:
        SHDeleteKeyW(hClass, L"");
        RegCloseKey(hClass);
    }

    if (hSource)
    {
        SHDeleteKeyW(hSource, L"");
        RegCloseKey(hSource);
    }
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
        ASSOCF_PER_MACHINE_ONLY,
    };

    for (size_t i = 0; i < _countof(cases); ++i)
    {
        HKEY hKey = NULL;
        HRESULT hr = AssocQueryKeyW(cases[i], ASSOCKEY_CLASS, L".reg", NULL, &hKey);
        ok_hr(hr, S_OK);

        LPWSTR path = GetKeyPath(hKey);
        if (hKey)
            RegCloseKey(hKey);

        LPCWSTR pszExpectedLM = L"\\REGISTRY\\MACHINE\\";
        LPCWSTR pszExpectedCU = L"\\REGISTRY\\USER\\";
        if (cases[i] & ASSOCF_PER_MACHINE_ONLY)
            pszExpectedCU = pszExpectedLM;
        ok(path &&
           (StrStrIW(path, pszExpectedLM) || StrStrIW(path, pszExpectedCU)),
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
    HRESULT hr;
    BOOL nt5 = LOBYTE(GetVersion()) < 6;
    WCHAR notepadPath[MAX_PATH];
    GetSystemDirectoryW(notepadPath, MAX_PATH);
    lstrcatW(notepadPath, L"\\notepad.exe");

    // Full path
    ok_AqkPath(ASSOCF_INIT_BYEXENAME, ASSOCKEY_APP, notepadPath, NULL,
               S_OK, L"SOFTWARE\\Classes\\Applications\\notepad.exe");

    // Filename.exe
    ok_AqkPath(ASSOCF_OPEN_BYEXENAME, ASSOCKEY_APP, L"regedit.exe", NULL,
               S_OK, L"SOFTWARE\\Classes\\Applications\\regedit.exe");

    // Filename (no extension)
    ok_AqkPath(ASSOCF_OPEN_BYEXENAME, ASSOCKEY_APP, L"regedit", NULL,
               S_OK, L"SOFTWARE\\Classes\\Applications\\regedit.exe");

    // Filename.exe with Extra
    ok_AqkPath(ASSOCF_OPEN_BYEXENAME, ASSOCKEY_APP, L"regedit.exe", L"Extra",
               S_OK, L"SOFTWARE\\Classes\\Applications\\regedit.exe");

    // Non existent
    ok_AqkPath(ASSOCF_INIT_BYEXENAME, ASSOCKEY_APP, L"__does_not_exist__.exe", NULL,
               nt5 ? HR_FNF : HR_NOASSOC, -0);

    // Without BYEXENAME
    hr = GetAqkPath(ASSOCF_NONE, ASSOCKEY_APP, L"regedit.exe", NULL, NULL);
    ok_hr(hr, nt5 ? E_FAIL : HR_NOASSOC);
}

// Check ASSOCKEY_CLASS
static void TEST_Class(void)
{
    TestClassKey(ASSOCKEY_CLASS);
}

// Check ASSOCKEY_BASECLASS
static void TEST_BaseClass(void)
{
    ok_AqkPath(AF_DEFSTAR, ASSOCKEY_BASECLASS, pszUniqueExt, NULL, S_OK, L"SOFTWARE\\Classes\\*");
    ok_AqkPath(AF_DEFFOLDER, ASSOCKEY_BASECLASS, L"Folder", NULL, S_OK, L"SOFTWARE\\Classes\\Folder");
    ok_AqkPath(AF_DEFFOLDER, ASSOCKEY_BASECLASS, L"Directory", NULL, S_OK, L"SOFTWARE\\Classes\\Folder");
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

        ok(IsValidClassesRootPath(path), "path was %s\n", wine_dbgstr_w(path));
        LocalFree(path);
    }

    TestClassKey(ASSOCKEY_SHELLEXECCLASS);
    // TODO: ASSOCF_VERIFY
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
    TEST_Class();
    TEST_BaseClass();
    TEST_ShellExecClass();

    Cleanup();
    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
