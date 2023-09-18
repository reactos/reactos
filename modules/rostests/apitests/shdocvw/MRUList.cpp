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

static void MRUList_DataList_0_Check(void)
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

static void MRUList_DataList_1_Check(void)
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

static void MRUList_DataList_2_Check(void)
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
    MRUList_DataList_0_Check();

    MRUList_DataList_1();
    MRUList_DataList_1_Check();

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
    MRUList_DataList_2_Check();

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

    LPITEMIDLIST pidl1, pidl2, pidl3;
    SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl1);
    SHGetSpecialFolderLocation(NULL, CSIDL_PERSONAL, &pidl2);
    SHGetSpecialFolderLocation(NULL, CSIDL_MYPICTURES, &pidl3);

    UINT uNodeSlot1 = 0xDEADFACE;
    hr = pList->UsePidl(pidl1, &uNodeSlot1);
    ok_hex(uNodeSlot1, 1);

    BYTE abData[256];
    DWORD cbData, dwValue, dwType;

    // "NodeSlot" value
    cbData = sizeof(dwValue);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"NodeSlot", &dwType, &dwValue, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_DWORD);
    ok_long(cbData, 4);
    ok_long(dwValue, 1);

    // "NodeSlots" value (Not "NodeSlot")
    cbData = sizeof(abData);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"NodeSlots", &dwType, &abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok_long(cbData, 1);
    ok_int(abData[0], 0x02);

    UINT uNodeSlot2 = 0xDEADFACE;
    hr = pList->UsePidl(pidl2, &uNodeSlot2);
    ok_hex(uNodeSlot2, 2);

    // "0" value
    cbData = sizeof(abData);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"0", &dwType, &abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok_long(cbData, 22);

    // "MRUListEx" value
    cbData = sizeof(abData);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx", &dwType, &abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok_long(cbData, 8);
    ok(memcmp(abData, "\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 8) == 0, "abData differs\n");

    // "NodeSlot" value
    cbData = sizeof(dwValue);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"NodeSlot", &dwType, &dwValue, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_DWORD);
    ok_long(dwValue, 1);

    // "NodeSlots" value
    cbData = sizeof(abData);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"NodeSlots", &dwType, &abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok_long(cbData, 2);
    ok(memcmp(abData, "\x02\x02", 2) == 0, "abData differs\n");

    // SUBSUBKEY0: "MRUListEx" value
    cbData = sizeof(abData);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBSUBKEY0, L"MRUListEx", &dwType, &abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok_long(cbData, 4);
    ok(memcmp(abData, "\xFF\xFF\xFF\xFF", 4) == 0, "abData differs\n");

    // SUBSUBKEY0: "NodeSlot" value
    cbData = sizeof(dwValue);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBSUBKEY0, L"NodeSlot", &dwType, &dwValue, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_DWORD);
    ok_long(dwValue, 2);

    UINT uNodeSlot3 = 0xFEEDF00D;
    hr = pList->UsePidl(pidl3, &uNodeSlot3);
    ok(uNodeSlot3 == 1 || uNodeSlot3 == 3, "uNodeSlot3 was %u\n", uNodeSlot3);

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
    cbData = sizeof(abData);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"MRUListEx", &dwType, &abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok_long(cbData, 8);
    ok(memcmp(abData, "\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 8) == 0, "abData differs\n");

    // "NodeSlot" value
    cbData = sizeof(dwValue);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"NodeSlot", &dwType, &dwValue, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_DWORD);
    ok_long(dwValue, 1);

    // "NodeSlots" value
    cbData = sizeof(abData);
    error = SHGetValueW(HKEY_CURRENT_USER, SUBKEY0, L"NodeSlots", &dwType, &abData, &cbData);
    ok_long(error, ERROR_SUCCESS);
    ok_long(dwType, REG_BINARY);
    ok(memcmp(abData, "\x02\x00", 2) == 0, "abData differs\n");

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

    FillMemory(anNodeSlot, sizeof(anNodeSlot), 0xCC);
    cNodeSlots = 0xDEAD;
    hr = pList->QueryPidl(pidl3, _countof(anNodeSlot), anNodeSlot, &cNodeSlots);
    ok(hr == S_OK || hr == S_FALSE, "hr was 0x%08lX\n", hr);
    ok_int(anNodeSlot[0], 1);
    ok_int(anNodeSlot[1], 0xCCCCCCCC);
    ok_int(cNodeSlots, 1);

    pList->Release();
    ILFree(pidl1);
    ILFree(pidl2);
    ILFree(pidl3);
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
