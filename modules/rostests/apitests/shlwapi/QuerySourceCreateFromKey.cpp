/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Tests for QuerySourceCreateFromKey
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlguid_undoc.h>
#include <shlobj_undoc.h>
#include <shlwapi_undoc.h>
#include <versionhelpers.h>

typedef HRESULT (WINAPI *FN_QuerySourceCreateFromKey)(HKEY, PCWSTR, BOOL, REFIID, PVOID*);
static FN_QuerySourceCreateFromKey g_pQuerySourceCreateFromKey = NULL;

static const WCHAR k_Root[]    = L"Software\\QuerySrcTest";
static const WCHAR k_SubKeyA[] = L"SubKeyA";
static const WCHAR k_SubKeyB[] = L"SubKeyB";

static void SetupRegistry(void)
{
    HKEY hRoot;
    RegCreateKeyExW(HKEY_CURRENT_USER, k_Root, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hRoot, NULL);

    static const WCHAR valA[] = L"hello";
    static const WCHAR valB[] = L"world";
    RegSetValueExW(hRoot, L"ValueA", 0, REG_SZ, (const BYTE*)valA, (DWORD)sizeof(valA));
    RegSetValueExW(hRoot, L"ValueB", 0, REG_SZ, (const BYTE*)valB, (DWORD)sizeof(valB));

    HKEY hSub;
    RegCreateKeyExW(hRoot, k_SubKeyA, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hSub, NULL);
    RegCloseKey(hSub);
    RegCreateKeyExW(hRoot, k_SubKeyB, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hSub, NULL);
    RegCloseKey(hSub);

    RegCloseKey(hRoot);
}

static void CleanupRegistry(void)
{
    SHDeleteKeyW(HKEY_CURRENT_USER, k_Root);
}

static void Test_EnumValues(void)
{
    IQuerySourceOld *pSrc = NULL;
    HRESULT hr = g_pQuerySourceCreateFromKey(HKEY_CURRENT_USER, k_Root, FALSE,
                                             IID_IQuerySourceOld, (PVOID*)&pSrc);
    ok_hr(hr, S_OK);
    ok(pSrc != NULL, "pSrc was NULL\n");

    IEnumString *pEnum = NULL;
    hr = 0xDEADFACE;
    if (pSrc)
        hr = pSrc->EnumValues(&pEnum);
    ok(pSrc && hr == S_OK, "EnumValues failed: 0x%08X\n", hr);

    LPWSTR psz = NULL;
    ULONG fetched = 0;
    hr = 0xDEADFACE;

    if (pEnum)
        hr = pEnum->Next(1, &psz, &fetched);
    ok_hr(hr, S_OK);
    ok(lstrcmpiW(psz, L"ValueA") == 0, "psz was %s\n", wine_dbgstr_w(psz));
    ok_int(fetched, 1);
    CoTaskMemFree(psz);

    psz = NULL;
    fetched = 0;
    hr = 0xDEADFACE;

    if (pEnum)
        hr = pEnum->Next(1, &psz, &fetched);
    ok_hr(hr, S_OK);
    ok(lstrcmpiW(psz, L"ValueB") == 0, "psz was %s\n", wine_dbgstr_w(psz));
    ok_int(fetched, 1);
    CoTaskMemFree(psz);

    psz = NULL;
    fetched = 0;
    hr = 0xDEADFACE;

    if (pEnum)
        hr = pEnum->Next(1, &psz, &fetched);
    ok_hr(hr, S_FALSE);
    ok(psz == NULL, "psz was %s\n", wine_dbgstr_w(psz));
    ok_int(fetched, 0);
    CoTaskMemFree(psz);

    if (pEnum)
        pEnum->Release();
    if (pSrc)
        pSrc->Release();
}

static void Test_EnumSources(void)
{
    IQuerySourceOld *pSrc = NULL;
    HRESULT hr = g_pQuerySourceCreateFromKey(HKEY_CURRENT_USER, k_Root, FALSE,
                                             IID_IQuerySourceOld, (PVOID*)&pSrc);
    ok_hr(hr, S_OK);
    ok(pSrc != NULL, "pSrc was NULL\n");

    IEnumString *pEnum = NULL;
    hr = pSrc->EnumSources(&pEnum);
    ok_hr(hr, S_OK);
    ok(pEnum != NULL, "pEnum was NULL\n");

    LPWSTR psz = NULL;
    ULONG fetched = 0;
    hr = 0xDEADFACE;

    if (pEnum)
        hr = pEnum->Next(1, &psz, &fetched);
    ok_hr(hr, S_OK);
    ok(lstrcmpiW(psz, k_SubKeyA) == 0, "psz was %s\n", wine_dbgstr_w(psz));
    ok_int(fetched, 1);
    CoTaskMemFree(psz);

    psz = NULL;
    fetched = 0;
    hr = 0xDEADFACE;

    if (pEnum)
        hr = pEnum->Next(1, &psz, &fetched);
    ok_hr(hr, S_OK);
    ok(lstrcmpiW(psz, k_SubKeyB) == 0, "psz was %s\n", wine_dbgstr_w(psz));
    ok_int(fetched, 1);
    CoTaskMemFree(psz);

    psz = NULL;
    fetched = 0;
    hr = 0xDEADFACE;

    if (pEnum)
        hr = pEnum->Next(1, &psz, &fetched);
    ok_hr(hr, S_FALSE);
    ok(psz == NULL, "psz was %s\n", wine_dbgstr_w(psz));
    ok_int(fetched, 0);
    CoTaskMemFree(psz);

    if (pSrc)
        pSrc->Release();
}

START_TEST(QuerySourceCreateFromKey)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+ is not tested well\n");
        return;
    }

    g_pQuerySourceCreateFromKey = (FN_QuerySourceCreateFromKey)
        GetProcAddress(GetModuleHandleA("shlwapi"), MAKEINTRESOURCEA(544));
    if (!g_pQuerySourceCreateFromKey)
    {
        skip("QuerySourceCreateFromKey not found\n");
        return;
    }

    HRESULT hrCoInit = CoInitialize(NULL);

    SetupRegistry();

    Test_EnumValues();
    Test_EnumSources();

    CleanupRegistry();

    if (SUCCEEDED(hrCoInit))
        CoUninitialize();
}
