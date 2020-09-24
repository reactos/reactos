/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement SHAutoComplete
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <windef.h>
#include <shlobj.h>
#include <shldisp.h>
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <browseui_undoc.h>
#include <shlguid_undoc.h>
#include <atlbase.h>
#include <atlcom.h>
#include <atlstr.h>
#include <strsafe.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static LONG
RegQueryCStringW(CRegKey& key, LPCWSTR pszValueName, CStringW& str)
{
    // Check type and size
    DWORD dwType, cbData;
    LONG ret = key.QueryValue(pszValueName, &dwType, NULL, &cbData);
    if (ret != ERROR_SUCCESS)
        return ret;
    if (dwType != REG_SZ && dwType != REG_EXPAND_SZ)
        return ERROR_INVALID_DATA;

    // Allocate buffer
    LPWSTR pszBuffer = str.GetBuffer(cbData / sizeof(WCHAR) + 1);
    if (pszBuffer == NULL)
        return ERROR_OUTOFMEMORY;

    // Get the data
    ret = key.QueryValue(pszValueName, NULL, pszBuffer, &cbData);

    // Release buffer
    str.ReleaseBuffer();
    return ret;
}

static VOID
AutoComplete_AddRunMRU(CComPtr<IACLCustomMRU> pMRU)
{
#define RUN_MRU_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"
    // Open the registry key
    CRegKey key;
    LONG result = key.Open(HKEY_CURRENT_USER, RUN_MRU_KEY, KEY_READ);
    if (result != ERROR_SUCCESS)
    {
        TRACE("Opening RunMRU failed: 0x%lX\n", result);
        return;
    }

    // Read the MRUList
    CStringW strMRUList;
    result = RegQueryCStringW(key, L"MRUList", strMRUList);
    if (result != ERROR_SUCCESS)
    {
        TRACE("No MRUList\n");
        return;
    }

    // for all the MRU items
    CStringW strData;
    for (INT i = 0; i <= L'z' - L'a' && i < strMRUList.GetLength(); ++i)
    {
        // Build a registry value name
        WCHAR szName[2];
        szName[0] = strMRUList[i];
        szName[1] = 0;

        // Read a registry value
        result = RegQueryCStringW(key, szName, strData);
        if (result != ERROR_SUCCESS)
            continue;

        // Fix up for special case of "\\1"
        INT cch = INT(wcslen(strData));
        if (cch >= 2 && wcscmp(&strData[cch - 2], L"\\1") == 0)
            strData = strData.Left(cch - 2);

        pMRU->AddMRUString(strData);
    }
}

static VOID
AutoComplete_AddTypedURLs(CComPtr<IACLCustomMRU> pMRU)
{
#define TYPED_URLS_KEY L"Software\\Microsoft\\Internet Explorer\\TypedURLs"
    // Open the registry key
    CRegKey key;
    LONG result = key.Open(HKEY_CURRENT_USER, TYPED_URLS_KEY, KEY_READ);
    if (result != ERROR_SUCCESS)
    {
        TRACE("%ld\n", result);
        return;
    }

    for (LONG i = 0; i < 50; ++i)
    {
        // Build a registry value name
        WCHAR szName[32];
        StringCbPrintfW(szName, sizeof(szName), L"url%lu", i);

        // Read a registry value
        CString strData;
        result = RegQueryCStringW(key, szName, strData);
        if (result != ERROR_SUCCESS)
            break;

        pMRU->AddMRUString(strData);
    }
}

static CComPtr<IUnknown>
AutoComplete_CreateList(DWORD dwSHACF, DWORD dwACLO)
{
    CComPtr<IUnknown> pList;
    HRESULT hr = CoCreateInstance(CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IUnknown, (LPVOID *)&pList);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_ACLMulti) failed with 0x%08lX\n", hr);
        return NULL;
    }

    CComPtr<IObjMgr> pManager;
    hr = pList->QueryInterface(IID_IObjMgr, (LPVOID *)&pManager);
    if (FAILED(hr))
    {
        ERR("pList->QueryInterface failed: 0x%08lX\n", hr);
        return NULL;
    }

    if (dwSHACF & SHACF_URLMRU)
    {
        CComPtr<IACLCustomMRU> pMRU;
        hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_INPROC_SERVER,
                              IID_IACLCustomMRU, (LPVOID *)&pMRU);
        if (SUCCEEDED(hr))
        {
            AutoComplete_AddRunMRU(pMRU);
            AutoComplete_AddTypedURLs(pMRU);
            pManager->Append(pMRU);
        }
        else
        {
            ERR("hr:%08lX\n", hr);
        }
    }

    if (dwSHACF & SHACF_URLHISTORY)
    {
        CComPtr<IACList2> pHistory;
        hr = CoCreateInstance(CLSID_ACLHistory, NULL, CLSCTX_INPROC_SERVER,
                              IID_IACList2, (LPVOID *)&pHistory);
        if (SUCCEEDED(hr))
        {
            pManager->Append(pHistory);
            pHistory->SetOptions(dwACLO);
        }
        else
        {
            ERR("hr:%08lX\n", hr);
        }
    }

    if (dwSHACF & (SHACF_FILESYSTEM | SHACF_FILESYS_ONLY | SHACF_FILESYS_DIRS))
    {
        CComPtr<IACList2> pISF;
        hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER,
                              IID_IACList2, (LPVOID *)&pISF);
        if (SUCCEEDED(hr))
        {
            pManager->Append(pISF);
            pISF->SetOptions(dwACLO);
        }
        else
        {
            ERR("hr:%08lX\n", hr);
        }
    }

    return pList;
}

#define AUTOCOMPLETE_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete"

static VOID
AutoComplete_AdaptFlags(IN HWND hwndEdit,
                        IN OUT LPDWORD pdwSHACF,
                        OUT LPDWORD pdwACO,
                        OUT LPDWORD pdwACLO)
{
    DWORD dwSHACF = *pdwSHACF, dwACO = 0, dwACLO = 0;
    if (dwSHACF == SHACF_DEFAULT)
        dwSHACF = SHACF_FILESYSTEM | SHACF_URLALL;

    if (!(dwSHACF & SHACF_AUTOAPPEND_FORCE_OFF) &&
        ((dwSHACF & SHACF_AUTOAPPEND_FORCE_ON) ||
         SHRegGetBoolUSValueW(AUTOCOMPLETE_KEY, L"Append Completion", FALSE, FALSE)))
    {
        dwACO |= ACO_AUTOAPPEND;
    }

    if (!(dwSHACF & SHACF_AUTOSUGGEST_FORCE_OFF) &&
        ((dwSHACF & SHACF_AUTOSUGGEST_FORCE_ON) ||
         SHRegGetBoolUSValueW(AUTOCOMPLETE_KEY, L"AutoSuggest", FALSE, TRUE)))
    {
        dwACO |= ACO_AUTOSUGGEST;
    }

    if (dwSHACF & (SHACF_FILESYSTEM | SHACF_FILESYS_ONLY | SHACF_FILESYS_DIRS))
        dwACLO |= ACLO_CURRENTDIR | ACLO_MYCOMPUTER;
    if (dwSHACF & SHACF_FILESYS_DIRS)
        dwACLO |= ACLO_FILESYSDIRS;
    if (dwSHACF & SHACF_FILESYS_ONLY)
        dwACLO |= ACLO_FILESYSONLY;

    if ((dwSHACF & SHACF_USETAB) ||
        SHRegGetBoolUSValueW(AUTOCOMPLETE_KEY, L"Always Use Tab", FALSE, FALSE))
    {
        dwACO |= ACO_USETAB;
    }

    if (GetWindowLongPtrW(hwndEdit, GWL_EXSTYLE) & WS_EX_RTLREADING)
        dwACO |= ACO_RTLREADING;

    *pdwACO = dwACO;
    *pdwSHACF = dwSHACF;
    *pdwACLO = dwACLO;
}

/*************************************************************************
 *      SHAutoComplete  	[SHLWAPI.@]
 *
 * Enable auto-completion for an edit control.
 *
 * PARAMS
 *  hwndEdit [I] Handle of control to enable auto-completion for
 *  dwFlags  [I] SHACF_ flags from "shlwapi.h"
 *
 * RETURNS
 *  Success: S_OK. Auto-completion is enabled for the control.
 *  Failure: An HRESULT error code indicating the error.
 */
HRESULT WINAPI SHAutoComplete(HWND hwndEdit, DWORD dwFlags)
{
    TRACE("SHAutoComplete(%p, 0x%lX)\n", hwndEdit, dwFlags);

    DWORD dwACO = 0, dwACLO = 0, dwSHACF = dwFlags;
    AutoComplete_AdaptFlags(hwndEdit, &dwACO, &dwACLO, &dwSHACF);

    CComPtr<IUnknown> pList = AutoComplete_CreateList(dwSHACF, dwACLO);
    if (!pList)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    CComPtr<IAutoComplete2> pAC2;
    HRESULT hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IAutoComplete, (LPVOID *)&pAC2);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_AutoComplete) failed: 0x%lX\n", hr);
        return hr;
    }

    if (SHPinDllOfCLSID(CLSID_ACListISF) && SHPinDllOfCLSID(CLSID_AutoComplete))
    {
        hr = pAC2->Init(hwndEdit, pList, NULL, L"www.%s.com");
        if (SUCCEEDED(hr))
            pAC2->SetOptions(dwACO);
        else
            ERR("IAutoComplete2::Init failed: 0x%lX\n", hr);
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}
