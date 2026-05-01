/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Test for MUI functions
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include <apitest.h>
#include <shlwapi.h>
#include <stdio.h>
#include <versionhelpers.h>

#define ok_wstr_i(x, y) \
    ok(_wcsicmp(x, y) == 0, "Wrong string. Expected '%S', got '%S'\n", y, x)

typedef HMODULE (WINAPI *FN_MLLoadLibraryA)(LPCSTR, HMODULE, DWORD);
typedef HMODULE (WINAPI *FN_MLLoadLibraryW)(LPCWSTR, HMODULE, DWORD);
typedef BOOL (WINAPI *FN_MLFreeLibrary)(HMODULE);
typedef BOOL (WINAPI *FN_MLIsMLHInstance)(HINSTANCE);
typedef HRESULT (WINAPI *FN_MLSetMLHInstance)(HINSTANCE, LANGID);
typedef HRESULT (WINAPI *FN_MLClearMLHInstance)(HINSTANCE);
typedef HRESULT (WINAPI *FN_MLBuildResURLA)(PCSTR, HMODULE, DWORD, PCSTR, PSTR, INT);
typedef HRESULT (WINAPI *FN_MLBuildResURLW)(PCWSTR, HMODULE, DWORD, PCWSTR, PWSTR, INT);

static FN_MLLoadLibraryA g_fnMLLoadLibraryA = NULL;
static FN_MLLoadLibraryW g_fnMLLoadLibraryW = NULL;
static FN_MLFreeLibrary g_fnMLFreeLibrary = NULL;
static FN_MLIsMLHInstance g_fnMLIsMLHInstance = NULL;
static FN_MLSetMLHInstance g_fnMLSetMLHInstance = NULL;
static FN_MLClearMLHInstance g_fnMLClearMLHInstance = NULL;
static FN_MLBuildResURLA g_fnMLBuildResURLA = NULL;
static FN_MLBuildResURLW g_fnMLBuildResURLW = NULL;

static void Test_SetIsHInstance_NullHandle(void)
{
    HRESULT hr = g_fnMLSetMLHInstance(NULL, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT));
    ok(FAILED(hr), "hr was 0x%08X\n", hr);
}

static void Test_SetIsHInstance_ValidHandle(void)
{
    HINSTANCE hFake = GetModuleHandleW(NULL);
    LANGID lang = MAKELANGID(LANG_JAPANESE, SUBLANG_DEFAULT);
    HRESULT hr = g_fnMLSetMLHInstance(hFake, lang);
    ok_hr(hr, S_OK);
    ok_int(g_fnMLIsMLHInstance(hFake), TRUE);
    g_fnMLClearMLHInstance(hFake);
}

static void Test_ClearHInstance_Registered(void)
{
    HINSTANCE hFake = GetModuleHandleW(NULL);
    g_fnMLSetMLHInstance(hFake, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT));
    ok_int(g_fnMLIsMLHInstance(hFake), TRUE);

    HRESULT hr = g_fnMLClearMLHInstance(hFake);
    ok_hr(hr, S_OK);
    ok_int(g_fnMLIsMLHInstance(hFake), FALSE);
}

static void Test_ClearHInstance_NotRegistered(void)
{
    HRESULT hr = g_fnMLClearMLHInstance((HINSTANCE)UlongToHandle(0xDEADBEEF));
    ok_hr(hr, S_OK);
}

static void Test_IsMLHInstance_EmptyList(void)
{
    ok_int(g_fnMLIsMLHInstance(GetModuleHandleW(NULL)), FALSE);
}

static void Test_MLFreeLibrary_UnregistersInstance(void)
{
    HINSTANCE hLib = LoadLibraryW(L"kernel32.dll");
    ok(hLib != NULL, "hLib was NULL\n");
    if (!hLib)
    {
        skip("hLib was NULL\n");
        return;
    }

    g_fnMLSetMLHInstance(hLib, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT));
    ok_int(g_fnMLIsMLHInstance(hLib), TRUE);

    BOOL ret = g_fnMLFreeLibrary(hLib);
    ok_int(ret, TRUE);
    ok_int(g_fnMLIsMLHInstance(hLib), FALSE);
}

static void Test_MLBuildResURLW_NullLibName(void)
{
    WCHAR szDest[MAX_PATH] = L"";
    HRESULT hr = g_fnMLBuildResURLW(NULL, GetModuleHandleW(NULL), 0, L"res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLW_NullHInst(void)
{
    WCHAR szDest[MAX_PATH] = L"";
    HRESULT hr = g_fnMLBuildResURLW(L"foo.dll", NULL, 0, L"res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLW_InvalidHInstSentinel(void)
{
    WCHAR szDest[MAX_PATH] = L"";
    HRESULT hr = g_fnMLBuildResURLW(L"foo.dll", INVALID_HANDLE_VALUE, 0,
                                    L"res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLW_InvalidFlags(void)
{
    WCHAR szDest[MAX_PATH] = L"";
    HRESULT hr = g_fnMLBuildResURLW(L"foo.dll", GetModuleHandleW(NULL),
                                    999, L"res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLW_NullResource(void)
{
    WCHAR szDest[MAX_PATH] = L"";
    HRESULT hr = g_fnMLBuildResURLW(L"foo.dll", GetModuleHandleW(NULL),
                                    0, NULL, szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLW_NullDest(void)
{
    HRESULT hr = g_fnMLBuildResURLW(L"foo.dll", GetModuleHandleW(NULL),
                                    0, L"res", NULL, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLW_BufferTooSmall(void)
{
    WCHAR szDest[5] = L"";
    HRESULT hr = g_fnMLBuildResURLW(L"kernel32.dll", GetModuleHandleW(NULL),
                                    0, L"#1", szDest, 5);
    ok(FAILED(hr), "hr was 0x%X\n", hr);
}

static void Test_MLBuildResURLW_ValidCallProducesResPrefix(void)
{
    WCHAR szDest[MAX_PATH] = L"";
    HRESULT hr = g_fnMLBuildResURLW(L"kernel32.dll", GetModuleHandleW(NULL),
                                    0, L"#1", szDest, MAX_PATH);
    if (SUCCEEDED(hr))
    {
        WCHAR szURL[MAX_PATH], szSysDir[MAX_PATH];
        GetSystemDirectoryW(szSysDir, _countof(szSysDir));
        lstrcpynW(szURL, L"res://", _countof(szURL));
        lstrcatW(szURL, szSysDir);
        lstrcatW(szURL, L"\\kernel32.dll/#1");

        ok_wstr_i(szDest, szURL);
    }
    else
    {
        ok_int(szDest[0], UNICODE_NULL);
    }
}

static void Test_MLBuildResURLA_NullLibName(void)
{
    CHAR szDest[MAX_PATH] = "";
    HRESULT hr = g_fnMLBuildResURLA(NULL, GetModuleHandleW(NULL), 0, "res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLA_NullHInst(void)
{
    CHAR szDest[MAX_PATH] = "";
    HRESULT hr = g_fnMLBuildResURLA("foo.dll", NULL, 0, "res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLA_InvalidFlags(void)
{
    CHAR szDest[MAX_PATH] = "";
    HRESULT hr = g_fnMLBuildResURLA("foo.dll", GetModuleHandleW(NULL),
                                5, "res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLBuildResURLA_SentinelHInst(void)
{
    CHAR szDest[MAX_PATH] = "";
    HRESULT hr = g_fnMLBuildResURLA("foo.dll", INVALID_HANDLE_VALUE,
                                    0, "res", szDest, MAX_PATH);
    ok_hr(hr, E_INVALIDARG);
}

static void Test_MLLoadLibraryW_NullFileName(void)
{
    HINSTANCE h = g_fnMLLoadLibraryW(NULL, GetModuleHandleW(NULL), 0);
    ok(h == NULL, "\n");
}

static void Test_MLLoadLibraryW_ValidSystemDll(void)
{
    HINSTANCE h = g_fnMLLoadLibraryW(L"kernel32.dll", GetModuleHandleW(NULL), 0);
    ok(h != NULL, "\n");
    if (h)
    {
        ok_int(g_fnMLIsMLHInstance(h), TRUE);
        g_fnMLFreeLibrary(h);
    }
}

static void Test_MLLoadLibraryW_NonExistentDll(void)
{
    HINSTANCE h = g_fnMLLoadLibraryW(L"__nonexistent__.dll", GetModuleHandleW(NULL), 0);
    ok(h == NULL, "\n");
}

static void Test_MLLoadLibraryA_NullFileName(void)
{
    HINSTANCE h = g_fnMLLoadLibraryA(NULL, GetModuleHandleW(NULL), 0);
    ok(h == NULL, "\n");
}

static void Test_MLLoadLibraryA_ValidSystemDll(void)
{
    HINSTANCE h = g_fnMLLoadLibraryA("kernel32.dll", GetModuleHandleW(NULL), 0);
    ok(h != NULL, "\n");
    if (h)
        g_fnMLFreeLibrary(h);
}

START_TEST(MUI)
{
    HRESULT hrCoInit;
    HINSTANCE hSHLWAPI;

    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+ doesn't have MUI\n");
        return;
    }

    hrCoInit = CoInitialize(NULL);
    hSHLWAPI = LoadLibraryW(L"shlwapi.dll");

    g_fnMLLoadLibraryA = (FN_MLLoadLibraryA)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(377));
    g_fnMLLoadLibraryW = (FN_MLLoadLibraryW)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(378));
    g_fnMLFreeLibrary = (FN_MLFreeLibrary)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(418));
    g_fnMLIsMLHInstance = (FN_MLIsMLHInstance)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(429));
    g_fnMLSetMLHInstance = (FN_MLSetMLHInstance)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(430));
    g_fnMLClearMLHInstance = (FN_MLClearMLHInstance)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(431));
    g_fnMLBuildResURLA = (FN_MLBuildResURLA)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(405));
    g_fnMLBuildResURLW = (FN_MLBuildResURLW)GetProcAddress(hSHLWAPI, MAKEINTRESOURCEA(406));
    if (!g_fnMLLoadLibraryA ||
        !g_fnMLLoadLibraryW ||
        !g_fnMLFreeLibrary ||
        !g_fnMLIsMLHInstance ||
        !g_fnMLSetMLHInstance ||
        !g_fnMLClearMLHInstance ||
        !g_fnMLBuildResURLA ||
        !g_fnMLBuildResURLW)
    {
        skip("Some MUI functions not found\n");
        goto Skip;
    }

    Test_SetIsHInstance_NullHandle();
    Test_SetIsHInstance_ValidHandle();
    Test_ClearHInstance_Registered();
    Test_ClearHInstance_NotRegistered();
    Test_IsMLHInstance_EmptyList();

    Test_MLFreeLibrary_UnregistersInstance();

    Test_MLBuildResURLW_NullLibName();
    Test_MLBuildResURLW_NullHInst();
    Test_MLBuildResURLW_InvalidHInstSentinel();
    Test_MLBuildResURLW_InvalidFlags();
    Test_MLBuildResURLW_NullResource();
    Test_MLBuildResURLW_NullDest();
    Test_MLBuildResURLW_BufferTooSmall();
    Test_MLBuildResURLW_ValidCallProducesResPrefix();

    Test_MLBuildResURLA_NullLibName();
    Test_MLBuildResURLA_NullHInst();
    Test_MLBuildResURLA_InvalidFlags();
    Test_MLBuildResURLA_SentinelHInst();

    Test_MLLoadLibraryW_NullFileName();
    Test_MLLoadLibraryW_ValidSystemDll();
    Test_MLLoadLibraryW_NonExistentDll();

    Test_MLLoadLibraryA_NullFileName();
    Test_MLLoadLibraryA_ValidSystemDll();

Skip:
    FreeLibrary(hSHLWAPI);

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
