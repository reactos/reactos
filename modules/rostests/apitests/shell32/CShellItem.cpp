/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     License: MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Tests for IShellItem
 * COPYRIGHT:   Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#include "shelltest.h"
#include <shellutils.h>
#include <ntquery.h> // PID_STG_*

static const PROPERTYKEY PKEY_DoesNotExist = { { 0x071ED77F,0x6DEE,0x4800,0x94,0xE7,0x1F,0xDD,0x89,0x6B,0x43,0x66 }, 42 };
static const PROPERTYKEY PKEY_Size = { PSGUID_STORAGE, PID_STG_SIZE };
static const PROPERTYKEY PKEY_DateModified = { PSGUID_STORAGE, PID_STG_WRITETIME };

static IShellItem* CreateItem(PCWSTR pszPath = NULL)
{
    WCHAR szPath[MAX_PATH];
    if (!pszPath)
    {
        pszPath = szPath;
        GetModuleFileName(NULL, szPath, _countof(szPath));
    }

    PIDLIST_ABSOLUTE pidl;
    HRESULT hr = SHParseDisplayName(pszPath, NULL, &pidl, 0, NULL);
    if (FAILED(hr))
        return NULL;
    IShellItem *pSI;
    hr = SHCreateShellItem(NULL, NULL, pidl, &pSI);
    ILFree(pidl);
    return SUCCEEDED(hr) ? pSI : NULL;
}

static IShellItem2* CreateItem2(PCWSTR pszPath = NULL)
{
    IShellItem2 *pSI2;
    CComPtr<IShellItem> pSI(CreateItem(pszPath));
    HRESULT hr = pSI ? pSI->QueryInterface(IID_IShellItem2, (void**)&pSI2) : E_FAIL;
    return SUCCEEDED(hr) ? pSI2 : NULL;
}

static void TestInterfaces()
{
    HRESULT hr;
    CComPtr<IShellItem> pItem(CreateItem());
    if (!pItem)
    {
        skip("Unable to create IShellItem\n");
        return;
    }

#if 0 // Not sure when this exact change happened, don't test until it can be nailed down
    CComPtr<IPersist> pPersist;
    hr = pItem->QueryInterface(IID_PPV_ARG(IPersist, &pPersist));
    if (LOBYTE(GetVersion()) >= 6 && !IsReactOS())
        ok_hr(hr, S_OK);
    else
        ok_hr(hr, E_NOINTERFACE); // Windows incorrectly forgets to support QI for IPersist
#endif

    CComPtr<IPersistIDList> pPIL;
    hr = pItem->QueryInterface(IID_PPV_ARG(IPersistIDList, &pPIL));
    ok_hr(hr, S_OK);
    if (SUCCEEDED(hr) && pPIL)
    {
        CLSID clsid = PKEY_DoesNotExist.fmtid;
        hr = pPIL->GetClassID(&clsid);
        ok(hr == S_OK && clsid == CLSID_ShellItem, "ClassID\n");

        hr = pPIL->SetIDList(NULL);
        ok(FAILED(hr), "Can't set a NULL item\n");
    }

    CComPtr<IParentAndItem> pPAI;
    hr = pItem->QueryInterface(IID_PPV_ARG(IParentAndItem, &pPAI));
    ok_hr(hr, S_OK);
    if (SUCCEEDED(hr) && pPAI)
    {
        hr = pPAI->GetParentAndItem(NULL, NULL, NULL);
        ok(SUCCEEDED(hr), "Asking for nothing is allowed\n");

        hr = pPAI->SetParentAndItem(NULL, NULL, NULL);
        ok_hr(hr, E_INVALIDARG);
    }

    // Windows does not check these pointers
    // ok(FAILED(hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, NULL)), "NULL output\n");
    // ok(FAILED(hr = pItem->GetParent(NULL)), "NULL output\n");
    // ok(FAILED(hr = pItem->GetAttributes(0, NULL)), "NULL output\n");
    // ok(FAILED(hr = pItem->Compare(NULL, 0, NULL)), "NULL output\n");
}

static void TestGetAttributes()
{
    SFGAOF query, result;
    HRESULT hr;
    CComPtr<IShellItem> pItem(CreateItem());
    if (!pItem)
    {
        skip("Unable to create IShellItem\n");
        return;
    }

    query = SFGAO_FILESYSTEM;
    hr = pItem->GetAttributes(query, (result = ~query, &result));
    ok(hr == S_OK && query == result, "All matching is S_OK\n");

    query = SFGAO_FILESYSTEM | SFGAO_COMPRESSED | SFGAO_ENCRYPTED | SFGAO_GHOSTED;
    hr = pItem->GetAttributes(query, (result = ~query, &result));
    ok(hr == S_FALSE && (query & result) != 0 && query > result, "Some matching is S_FALSE\n");

    query = SFGAO_FOLDER | SFGAO_STREAM;
    hr = pItem->GetAttributes(query, (result = ~query, &result));
    ok(!(result & SFGAO_FILESYSTEM), "Filter away flags not in query\n");
}

static void TestShellItemPropSys()
{
    HRESULT hr;
    PROPVARIANT pv;
    WCHAR szPath[MAX_PATH];
    GetModuleFileName(NULL, szPath, _countof(szPath));
    WIN32_FIND_DATAW fd = {};
    FindClose(FindFirstFileW(szPath, &fd));

    CComPtr<IShellItem2> pItem(CreateItem2(szPath));
    if (!pItem)
    {
        if (LOBYTE(GetVersion()) >= 6)
            skip("Unable to create IShellItem2 from %ls\n", szPath);
        else
            skip("IShellItem2 not supported\n");
        return;
    }

    //ok_hr(pItem->GetInt32(PKEY_DoesNotExist, NULL), E_INVALIDARG); // Windows does not verify the pointer
    ok_hr(pItem->GetUInt32(PKEY_DoesNotExist, &pv.ulVal), HResultFromWin32(ERROR_NOT_FOUND));

    #define TestFailedDefVal(method, param, expected) \
    { \
        pv.uhVal.QuadPart = (expected), pv.ulVal = ~(UINT)(expected); \
        hr = pItem->method(PKEY_DoesNotExist, param); \
        ok(FAILED(hr) && pv.uhVal.QuadPart == (expected), #method " failure value\n"); \
    }
    TestFailedDefVal(GetUInt32, &pv.ulVal, 0);
    TestFailedDefVal(GetInt32, (int*)&pv.lVal, 0);
    TestFailedDefVal(GetUInt64, &pv.uhVal.QuadPart, 0);
    TestFailedDefVal(GetBool, (BOOL*)&pv.lVal, FALSE);
    TestFailedDefVal(GetFileTime, &pv.filetime, 0);
    TestFailedDefVal(GetString, &pv.pwszVal, 0);
    #undef TestFailedDefVal

    pv.lVal = -1;
    hr = pItem->GetUInt32(PKEY_Size, &pv.ulVal);
    ok(hr == S_OK && pv.ulVal == fd.nFileSizeLow && fd.nFileSizeLow, "Size UInt32\n");
    pv.lVal = -1;
    hr = pItem->GetInt32(PKEY_Size, (int*)&pv.lVal);
    ok(hr == S_OK && pv.ulVal == fd.nFileSizeLow && fd.nFileSizeLow, "Size Int32\n");
    pv.lVal = -1;
    hr = pItem->GetUInt64(PKEY_Size, &pv.uhVal.QuadPart);
    ok(hr == S_OK && pv.uhVal.LowPart == fd.nFileSizeLow && fd.nFileSizeLow, "Size UInt64\n");

    pv.lVal = -1;
    hr = pItem->GetFileTime(PKEY_DateModified, &pv.filetime);
    ok(hr == S_OK && pv.filetime.dwHighDateTime == fd.ftLastWriteTime.dwHighDateTime, "MDate\n"); // Only checking the high dword to avoid PIDL DOS time resolution
}

START_TEST(CShellItem)
{
    CCoInit ComInit;

    TestInterfaces();
    TestGetAttributes();
    TestShellItemPropSys();
}
