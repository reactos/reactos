/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for MRU List
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include <apitest.h>
#include <winreg.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <stdio.h>
#include <shlwapi_undoc.h>
#include <versionhelpers.h>
#include <strsafe.h>
#include <wine/test.h>
#include <pseh/pseh2.h>

#define SUBKEY0 L"Software\\MRUListTest"
#define SUBSUBKEY0 L"Software\\MRUListTest\\0"
#define TEXT0 L"This is a test."
#define TEXT1 L"ReactOS rocks!"

static void MRUList_DataList_0(void)
{
    HRESULT hr;
    IMruDataList *pList = NULL;
    UINT iSlot1, iSlot2, iSlot3;
    DWORD cbText;
    WCHAR szText[MAX_PATH];

    hr = CoCreateInstance(CLSID_MruLongList, NULL, CLSCTX_INPROC_SERVER,
                          IID_IMruDataList, (LPVOID*)&pList);
    ok_hex(hr, S_OK);
    if (pList == NULL)
    {
        skip("pList was NULL\n");
        return;
    }

    hr = pList->InitData(26, 0, HKEY_CURRENT_USER, SUBKEY0, NULL);
    ok_hex(hr, S_OK);

    cbText = (wcslen(TEXT0) + 1) * sizeof(WCHAR);
    hr = pList->AddData((BYTE*)TEXT0, cbText, &iSlot1);
    ok_hex(hr, S_OK);
    ok_int(iSlot1, 0);

    hr = pList->FindData((BYTE*)TEXT0, cbText, &iSlot2);
    ok_hex(hr, S_OK);
    ok_int(iSlot1, iSlot2);

    cbText = sizeof(szText);
    hr = pList->GetData(iSlot1, (BYTE*)szText, cbText);
    ok_hex(hr, S_OK);
    ok_wstr(szText, TEXT0);

    cbText = (wcslen(TEXT1) + 1) * sizeof(WCHAR);
    hr = pList->AddData((BYTE*)TEXT1, cbText, &iSlot3);
    ok_hex(hr, S_OK);
    ok_int(iSlot3, 1);

    pList->Release();
}

static INT MRUList_Check(LPCWSTR pszSubKey, LPCWSTR pszValueName, LPCVOID pvData, DWORD cbData)
{
    BYTE abData[512];
    LONG error;
    DWORD dwSize = cbData;

    error = SHGetValueW(HKEY_CURRENT_USER, pszSubKey, pszValueName, NULL, abData, &dwSize);
    if (error != ERROR_SUCCESS)
        return -999;

#if 0
    printf("dwSize: %ld\n", dwSize);
    for (DWORD i = 0; i < dwSize; ++i)
    {
        printf("%02X ", abData[i]);
    }
    printf("\n");
#endif

    if (dwSize != cbData)
        return +999;

    if (!pvData)
        return TRUE;

    return memcmp(abData, pvData, cbData) == 0;
}

static void MRUList_DataList_1(void)
{
    HRESULT hr;
    IMruDataList *pList = NULL;
    UINT iSlot;

    hr = CoCreateInstance(CLSID_MruLongList, NULL, CLSCTX_INPROC_SERVER,
                          IID_IMruDataList, (LPVOID*)&pList);
    ok_hex(hr, S_OK);
    if (pList == NULL)
    {
        skip("pList was NULL\n");
        return;
    }

    hr = pList->InitData(26, 0, HKEY_CURRENT_USER, SUBKEY0, NULL);
    ok_hex(hr, S_OK);

    DWORD cbText = (wcslen(TEXT0) + 1) * sizeof(WCHAR);
    hr = pList->FindData((BYTE*)TEXT0, cbText, &iSlot);
    ok_hex(hr, S_OK);
    ok_int(iSlot, 1);

    hr = pList->Delete(iSlot);
    ok_hex(hr, S_OK);

    iSlot = 0xCAFE;
    cbText = (wcslen(TEXT0) + 1) * sizeof(WCHAR);
    hr = pList->FindData((BYTE*)TEXT0, cbText, &iSlot);
    ok_hex(hr, E_FAIL);
    ok_int(iSlot, 0xCAFE);

    pList->Release();
}

static void MRUList_DataList_2(void)
{
    HRESULT hr;
    IMruDataList *pList = NULL;

    hr = CoCreateInstance(CLSID_MruLongList, NULL, CLSCTX_INPROC_SERVER,
                          IID_IMruDataList, (LPVOID*)&pList);
    ok_hex(hr, S_OK);
    if (pList == NULL)
    {
        skip("pList was NULL\n");
        return;
    }

    hr = pList->InitData(26, 0, HKEY_CURRENT_USER, SUBKEY0, NULL);
    ok_hex(hr, S_OK);

    WCHAR szText[MAX_PATH];
    DWORD cbText = sizeof(szText);
    StringCchCopyW(szText, _countof(szText), L"====");
    hr = pList->GetData(0, (BYTE*)szText, cbText);
    ok_hex(hr, S_OK);
    ok_wstr(szText, L"ABC");

    StringCchCopyW(szText, _countof(szText), L"====");
    cbText = sizeof(szText);
    hr = pList->GetData(1, (BYTE*)szText, cbText);
    ok_hex(hr, S_OK);
    ok_wstr(szText, L"XYZ");

    pList->Release();
}

static void MRUList_DataList(void)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+ doesn't support CLSID_MruLongList\n");
        return;
    }

    SHDeleteKeyW(HKEY_CURRENT_USER, SUBKEY0);

    LONG error;
    error = SHSetValueW(HKEY_CURRENT_USER, SUBKEY0, NULL, REG_SZ, L"", sizeof(UNICODE_NULL));
    ok_long(error, ERROR_SUCCESS);

    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, NULL, NULL, NULL, NULL);
    ok_long(error, ERROR_SUCCESS);

    MRUList_DataList_0();
    ok_int(MRUList_Check(SUBKEY0, L"MRUListEx", "\x01\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 12), TRUE);

    MRUList_DataList_1();
    ok_int(MRUList_Check(SUBKEY0, L"MRUListEx", "\x01\x00\x00\x00\xFF\xFF\xFF\xFF", 8), TRUE);

    error = SHDeleteValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUList");
    ok_long(error, ERROR_FILE_NOT_FOUND);
    error = SHDeleteValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx");
    ok_long(error, ERROR_SUCCESS);

    error = SHSetValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUList", REG_SZ, L"ab", 3 * sizeof(WCHAR));
    ok_long(error, ERROR_SUCCESS);
    error = SHSetValueW(HKEY_CURRENT_USER, SUBKEY0, L"a", REG_BINARY, L"ABC", 4 * sizeof(WCHAR));
    ok_long(error, ERROR_SUCCESS);
    error = SHSetValueW(HKEY_CURRENT_USER, SUBKEY0, L"b", REG_BINARY, L"XYZ", 4 * sizeof(WCHAR));
    ok_long(error, ERROR_SUCCESS);

    MRUList_DataList_2();
    ok_int(MRUList_Check(SUBKEY0, L"MRUListEx", "\x00\x00\x00\x00\x01\x00\x00\x00\xFF\xFF\xFF\xFF", 12), TRUE);

    error = SHDeleteValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUList");
    ok_long(error, ERROR_FILE_NOT_FOUND);
    error = SHDeleteValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx");
    ok_long(error, ERROR_SUCCESS);

    SHDeleteKeyW(HKEY_CURRENT_USER, SUBKEY0);
}

static void MRUList_PidlList_0(void)
{
    HRESULT hr;
    IMruPidlList *pList = NULL;

    hr = CoCreateInstance(CLSID_MruPidlList, NULL, CLSCTX_INPROC_SERVER,
                          IID_IMruPidlList, (LPVOID*)&pList);
    ok_hex(hr, S_OK);
    if (pList == NULL)
    {
        skip("pList was NULL\n");
        return;
    }

    LONG error;

    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, NULL, NULL, NULL, NULL);
    ok_long(error, ERROR_FILE_NOT_FOUND);

    hr = pList->InitList(32, HKEY_CURRENT_USER, SUBKEY0);
    ok_hex(hr, S_OK);

    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, NULL, NULL, NULL, NULL);
    ok_long(error, ERROR_FILE_NOT_FOUND);

    LPITEMIDLIST pidl1, pidl2;
    SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl1);
    SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl2);

    UINT uNodeSlot1 = 0xDEADFACE;
    hr = pList->UsePidl(pidl1, &uNodeSlot1);
    ok_hex(uNodeSlot1, 1);

    // "NodeSlot" value
    ok_int(MRUList_Check(SUBKEY0, L"NodeSlot", "\x01\x00\x00\x00", 4), TRUE);

    // "NodeSlots" value (Not "NodeSlot")
    ok_int(MRUList_Check(SUBKEY0, L"NodeSlots", "\x02", 1), TRUE);

    UINT uNodeSlot2 = 0xDEADFACE;
    hr = pList->UsePidl(pidl2, &uNodeSlot2);
    ok_hex(uNodeSlot2, 2);

    // "0" value
    ok_int(MRUList_Check(SUBKEY0, L"0", NULL, 22), TRUE);

    // "MRUListEx" value
    ok_int(MRUList_Check(SUBKEY0, L"MRUListEx", "\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 8), TRUE);

    // "NodeSlot" value
    ok_int(MRUList_Check(SUBKEY0, L"NodeSlot", "\x01\x00\x00\x00", 4), TRUE);

    // "NodeSlots" value
    ok_int(MRUList_Check(SUBKEY0, L"NodeSlots", "\x02\x02", 2), TRUE);

    // SUBSUBKEY0: "MRUListEx" value
    ok_int(MRUList_Check(SUBSUBKEY0, L"MRUListEx", "\xFF\xFF\xFF\xFF", 4), TRUE);

    // SUBSUBKEY0: "NodeSlot" value
    ok_int(MRUList_Check(SUBSUBKEY0, L"NodeSlot", "\x02\x00\x00\x00", 4), TRUE);

    // QueryPidl
    UINT anNodeSlot[2], cNodeSlots;
    FillMemory(anNodeSlot, sizeof(anNodeSlot), 0xCC);
    cNodeSlots = 0xDEAD;
    hr = pList->QueryPidl(pidl1, _countof(anNodeSlot), anNodeSlot, &cNodeSlots);
    ok_long(hr, S_OK);
    ok_int(anNodeSlot[0], 1);
    ok_int(anNodeSlot[1], 0xCCCCCCCC);
    ok_int(cNodeSlots, 1);

    hr = pList->PruneKids(pidl1);

    // "MRUListEx" value
    ok_int(MRUList_Check(SUBKEY0, L"MRUListEx", "\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 8), TRUE);

    // "NodeSlot" value
    ok_int(MRUList_Check(SUBKEY0, L"NodeSlot", "\x01\x00\x00\x00", 4), TRUE);

    // "NodeSlots" value
    ok_int(MRUList_Check(SUBKEY0, L"NodeSlots", "\x02\x00", 2), TRUE);

    FillMemory(anNodeSlot, sizeof(anNodeSlot), 0xCC);
    cNodeSlots = 0xBEEF;
    hr = pList->QueryPidl(pidl1, 0, anNodeSlot, &cNodeSlots);
    ok_long(hr, E_FAIL);
    ok_int(anNodeSlot[0], 0xCCCCCCCC);
    ok_int(anNodeSlot[1], 0xCCCCCCCC);
    ok_int(cNodeSlots, 0);

    FillMemory(anNodeSlot, sizeof(anNodeSlot), 0xCC);
    cNodeSlots = 0xDEAD;
    hr = pList->QueryPidl(pidl1, _countof(anNodeSlot), anNodeSlot, &cNodeSlots);
    ok_long(hr, S_OK);
    ok_int(anNodeSlot[0], 1);
    ok_int(anNodeSlot[1], 0xCCCCCCCC);
    ok_int(cNodeSlots, 1);

    FillMemory(anNodeSlot, sizeof(anNodeSlot), 0xCC);
    cNodeSlots = 0xDEAD;
    hr = pList->QueryPidl(pidl2, _countof(anNodeSlot), anNodeSlot, &cNodeSlots);
    ok_long(hr, S_FALSE);
    ok_int(anNodeSlot[0], 1);
    ok_int(anNodeSlot[1], 0xCCCCCCCC);
    ok_int(cNodeSlots, 1);

    pList->Release();
    ILFree(pidl1);
    ILFree(pidl2);
}

static void MRUList_PidlList(void)
{
    if (IsWindowsVistaOrGreater())
    {
        skip("Vista+ doesn't support CLSID_MruPidlList\n");
        return;
    }

    SHDeleteKeyW(HKEY_CURRENT_USER, SUBKEY0);

    MRUList_PidlList_0();

    SHDeleteKeyW(HKEY_CURRENT_USER, SUBKEY0);
}

START_TEST(MRUList)
{
    HRESULT hr = CoInitialize(NULL);
    ok_hex(hr, S_OK);

    MRUList_DataList();
    MRUList_PidlList();

    if (SUCCEEDED(hr))
        CoUninitialize();
}
