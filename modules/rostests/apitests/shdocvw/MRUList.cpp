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
#define TEXT0 L"This is a test."
#define TEXT1 L"ReactOS rocks!"

static void MRUList_List0(void)
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

static void MRUList_List0_Check(void)
{
    BYTE abData[512];
    DWORD cbData, dwType;

    cbData = sizeof(abData);
    LONG error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx", &dwType, abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
#if 1
    ok_int(memcmp(abData, "\x01\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 12), 0);
#else
    for (DWORD i = 0; i < cbData; ++i)
    {
        printf("%02X ", abData[i]);
    }
    printf("\n");
#endif
}

static void MRUList_List1(void)
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

static void MRUList_List1_Check(void)
{
    BYTE abData[512];
    DWORD cbData, dwType;

    cbData = sizeof(abData);
    LONG error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx", &dwType, abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
#if 1
    ok_int(memcmp(abData, "\x01\x00\x00\x00\xFF\xFF\xFF\xFF", 8), 0);
#else
    for (DWORD i = 0; i < cbData; ++i)
    {
        printf("%02X ", abData[i]);
    }
    printf("\n");
#endif
}

static void MRUList_List2(void)
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

static void MRUList_List2_Check(void)
{
    BYTE abData[512];
    DWORD cbData, dwType;

    cbData = sizeof(abData);
    LONG error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx", &dwType, abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
#if 1
    ok_int(memcmp(abData, "\x00\x00\x00\x00\x01\x00\x00\x00\xFF\xFF\xFF\xFF", 12), 0);
#else
    for (DWORD i = 0; i < cbData; ++i)
    {
        printf("%02X ", abData[i]);
    }
    printf("\n");
#endif
}

static void MRUList_List(void)
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

    MRUList_List0();
    MRUList_List0_Check();

    MRUList_List1();
    MRUList_List1_Check();

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

    MRUList_List2();
    MRUList_List2_Check();

    error = SHDeleteValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUList");
    ok_long(error, ERROR_FILE_NOT_FOUND);
    error = SHDeleteValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx");
    ok_long(error, ERROR_SUCCESS);

    SHDeleteKeyW(HKEY_CURRENT_USER, SUBKEY0);
}

START_TEST(MRUList)
{
    HRESULT hr = CoInitialize(NULL);
    ok_hex(hr, S_OK);

    MRUList_List();

    if (SUCCEEDED(hr))
        CoUninitialize();
}
