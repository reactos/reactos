/*
 * PROJECT:     ReactOS shlwapi
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement SHAutoComplete
 * COPYRIGHT:   Copyright 2020 Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */
#include <windef.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <browseui_undoc.h>
#include <shlwapi_undoc.h>
#include <shlguid_undoc.h>
#include <atlbase.h>
#include <atlcom.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HRESULT
AutoComplete_AddMRU(CComPtr<IObjMgr> pManager, LPCWSTR pszKey)
{
    CComPtr<IACLCustomMRU> pMRU; // Create an MRU list
    HRESULT hr = CoCreateInstance(CLSID_ACLCustomMRU, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IACLCustomMRU, (LPVOID *)&pMRU);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_ACLMRU) failed with 0x%08lX\n", hr);
        return hr;
    }

    hr = pMRU->Initialize(pszKey, 'z' - 'a' + 1); // Load the list from registry
    if (FAILED(hr))
    {
        ERR("pMRU->Initialize(%ls) failed with 0x%08lX\n", pszKey, hr);
        return hr;
    }

    hr = pManager->Append(pMRU); // Add to the manager
    if (FAILED(hr))
        ERR("pManager->Append for '%ls' failed with 0x%08lX\n", pszKey, hr);
    return hr;
}

static HRESULT
IUnknown_AddOptions(CComPtr<IUnknown> punk, DWORD dwACLO)
{
    CComPtr<IACList2> pList;
    HRESULT hr = punk->QueryInterface(IID_IACList2, (LPVOID *)&pList);
    if (FAILED(hr))
    {
        ERR("punk->QueryInterface failed: 0x%08lX\n", hr);
        return hr;
    }

    DWORD dwOptions = 0;
    pList->GetOptions(&dwOptions);
    dwOptions |= dwACLO;
    hr = pList->SetOptions(dwOptions);
    if (FAILED(hr))
        ERR("pList->SetOptions failed: 0x%08lX\n", hr);
    return hr;
}

static CComPtr<IUnknown>
AutoComplete_LoadList(DWORD dwSHACF, DWORD dwACLO)
{
    // Create a multiple list (with IEnumString interface)
    CComPtr<IUnknown> pList;
    HRESULT hr = CoCreateInstance(CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IUnknown, (LPVOID *)&pList);
    if (FAILED(hr)) // Failed to create the list
    {
        ERR("CoCreateInstance(CLSID_ACLMulti) failed with 0x%08lX\n", hr);
        return NULL;
    }

    CComPtr<IObjMgr> pManager; // This is the manager of the multiple list
    hr = pList->QueryInterface(IID_IObjMgr, (LPVOID *)&pManager);
    if (FAILED(hr)) // Failed to get interface
    {
        ERR("pList->QueryInterface failed: 0x%08lX\n", hr);
        return NULL;
    }

    if (dwSHACF & SHACF_URLMRU)
    {
        // The MRU (Most-Recently-Used) lists (with IEnumString interface)
#define RUN_MRU_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"
#define TYPED_URLS_KEY L"Software\\Microsoft\\Internet Explorer\\TypedURLs"
        AutoComplete_AddMRU(pManager, RUN_MRU_KEY);
        AutoComplete_AddMRU(pManager, TYPED_URLS_KEY);
    }

    if (dwSHACF & SHACF_URLHISTORY)
    {
        // The history list (with IEnumString interface)
        CComPtr<IUnknown> pHistory;
        hr = CoCreateInstance(CLSID_ACLHistory, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUnknown, (LPVOID *)&pHistory);
        if (SUCCEEDED(hr))
        {
            pManager->Append(pHistory); // Add to the manager
            // Add ACLO_* options
            IUnknown_AddOptions(pHistory, dwACLO);
        }
        else
        {
            ERR("CLSID_ACLHistory hr:%08lX\n", hr);
        }
    }

    if (dwSHACF & (SHACF_FILESYSTEM | SHACF_FILESYS_ONLY | SHACF_FILESYS_DIRS))
    {
        // The filesystem list (with IEnumString interface)
        CComPtr<IUnknown> pISF;
        hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUnknown, (LPVOID *)&pISF);
        if (SUCCEEDED(hr))
        {
            pManager->Append(pISF); // Add to the manager
            // Add ACLO_* options
            IUnknown_AddOptions(pISF, dwACLO);
        }
        else
        {
            ERR("CLSID_ACListISF hr:%08lX\n", hr);
        }
    }

    return pList; // The list
}

static VOID
AutoComplete_AdaptFlags(IN HWND hwndEdit,
                        IN OUT LPDWORD pdwSHACF,
                        OUT LPDWORD pdwACO,
                        OUT LPDWORD pdwACLO)
{
#define AUTOCOMPLETE_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete"
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

    if (dwSHACF & SHACF_FILESYS_DIRS)
        dwACLO |= ACLO_FILESYSDIRS;
    if (dwSHACF & SHACF_FILESYS_ONLY)
        dwACLO |= ACLO_FILESYSONLY;

    static BOOL s_bAlwaysUseTab = 99;
    if (s_bAlwaysUseTab == 99)
        s_bAlwaysUseTab = SHRegGetBoolUSValueW(AUTOCOMPLETE_KEY, L"Always Use Tab", FALSE, FALSE);

    if (s_bAlwaysUseTab || (dwSHACF & SHACF_USETAB))
        dwACO |= ACO_USETAB;

    if (GetWindowLongPtrW(hwndEdit, GWL_EXSTYLE) & WS_EX_LAYOUTRTL)
        dwACO |= ACO_RTLREADING;

    *pdwSHACF = dwSHACF;
    *pdwACO = dwACO;
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

    DWORD dwSHACF = dwFlags, dwACO = 0, dwACLO = 0;
    AutoComplete_AdaptFlags(hwndEdit, &dwSHACF, &dwACO, &dwACLO);

    // Load the list (with IEnumString interface)
    CComPtr<IUnknown> pList = AutoComplete_LoadList(dwSHACF, dwACLO);
    if (!pList)
    {
        ERR("Out of memory\n");
        return E_OUTOFMEMORY;
    }

    // Create an auto-completion (IAutoComplete2)
    CComPtr<IAutoComplete2> pAC2;
    HRESULT hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IAutoComplete2, (LPVOID *)&pAC2);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_AutoComplete) failed: 0x%lX\n", hr);
        return hr;
    }

    // Keep the DLLs of CLSID_ACListISF and CLSID_AutoComplete loaded
    hr = E_FAIL;
    if (SHPinDllOfCLSID(CLSID_ACListISF) && SHPinDllOfCLSID(CLSID_AutoComplete))
    {
        // Initialize IAutoComplete2 for auto-completion
        hr = pAC2->Init(hwndEdit, pList, NULL, NULL);
        if (SUCCEEDED(hr))
            pAC2->SetOptions(dwACO); // Set ACO_* flags
        else
            ERR("IAutoComplete2::Init failed: 0x%lX\n", hr);
    }

    return hr;
}
