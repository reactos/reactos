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
#include <atlbase.h>
#include <atlcom.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HRESULT
IUnknown_SetOptions(IUnknown *punk, DWORD dwACLO)
{
    CComPtr<IACList2> pAL2;
    HRESULT hr = punk->QueryInterface(IID_IACList2, (LPVOID *)&pAL2);
    if (FAILED(hr))
    {
        ERR("QueryInterface(IID_IACList2) failed: 0x%08lX\n", hr);
        return hr;
    }

    hr = pAL2->SetOptions(dwACLO);
    if (FAILED(hr))
        ERR("pAL2->SetOptions failed: 0x%08lX\n", hr);
    return hr;
}

static CComPtr<IUnknown>
AutoComplete_CreateLists(DWORD dwSHACF, DWORD dwACLO)
{
    CComPtr<IUnknown> pLists;
    HRESULT hr = CoCreateInstance(CLSID_ACLMulti, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IUnknown, (LPVOID *)&pLists);
    if (FAILED(hr))
    {
        ERR("CoCreateInstance(CLSID_ACLMulti) failed: 0x%08lX\n", hr);
        return NULL;
    }

    CComPtr<IObjMgr> pManager;
    hr = pLists->QueryInterface(IID_IObjMgr, (LPVOID *)&pManager);
    if (FAILED(hr))
    {
        ERR("IObjMgr::QueryInterface failed: 0x%08lX\n", hr);
        return NULL;
    }

    if (dwSHACF & SHACF_URLMRU)
    {
        CComPtr<IUnknown> pACLMRU;
        hr = CoCreateInstance(CLSID_ACLMRU, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUnknown, (LPVOID *)&pACLMRU);
        if (SUCCEEDED(hr))
        {
            pManager->Append(pACLMRU);
            IUnknown_SetOptions(pACLMRU, dwACLO);
        }
        else
        {
            ERR("hr: 0x%08lX", hr);
        }
    }

    if (dwSHACF & SHACF_URLHISTORY)
    {
        CComPtr<IUnknown> pACLHistory;
        hr = CoCreateInstance(CLSID_ACLHistory, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUnknown, (LPVOID *)&pACLHistory);
        if (SUCCEEDED(hr))
        {
            pManager->Append(pACLHistory);
            IUnknown_SetOptions(pACLHistory, dwACLO);
        }
        else
        {
            ERR("hr: 0x%08lX", hr);
        }
    }

    if (dwSHACF & SHACF_FILESYSTEM)
    {
        CComPtr<IUnknown> pACListISF;
        hr = CoCreateInstance(CLSID_ACListISF, NULL, CLSCTX_INPROC_SERVER,
                              IID_IUnknown, (LPVOID *)&pACListISF);
        if (SUCCEEDED(hr))
        {
            pManager->Append(pACListISF);
            IUnknown_SetOptions(pACListISF, dwACLO);
        }
        else
        {
            ERR("hr: 0x%08lX", hr);
        }
    }

    return pLists;
}

static BOOL
AutoComplete_AdaptFlags(IN HWND hwndEdit,
                        IN OUT LPDWORD pdwSHACF,
                        OUT LPDWORD pdwACO,
                        OUT LPDWORD pdwACLO)
{
#define AUTOCOMPLETE_KEY L"Software\\Microsoft\\Internet Explorer\\AutoComplete"

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
         SHRegGetBoolUSValueW(AUTOCOMPLETE_KEY, L"Append AutoSuggest", FALSE, FALSE)))
    {
        dwACO |= ACO_AUTOSUGGEST;
    }

    if (!(dwACO & (ACO_AUTOSUGGEST | ACO_AUTOAPPEND)))
        return FALSE;

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
    return TRUE;
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
    if (!AutoComplete_AdaptFlags(hwndEdit, &dwACO, &dwACLO, &dwSHACF))
        return S_OK;

    CComPtr<IUnknown> pACL;
    pACL = AutoComplete_CreateLists(dwSHACF, dwACLO);
    if (!pACL)
    {
        ERR("AutoComplete_CreateLists failed\n");
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

    hr = pAC2->Init(hwndEdit, pACL, NULL, L"www.%s.com");
    if (SUCCEEDED(hr))
        pAC2->SetOptions(dwACO);
    else
        ERR("IAutoComplete2::Init failed: 0x%lX\n", hr);

    return hr;
}
