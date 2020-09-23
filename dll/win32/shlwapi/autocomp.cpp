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
AutoComplete_CreateUnknownFromCLSID(const CLSID& clsid, LPCSTR name)
{
    CComPtr<IUnknown> ret;
    HRESULT hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IUnknown, (LPVOID *)&ret);
    if (FAILED(hr))
    {
        ERR("%s: hr:0x%08lX", name, hr);
        return NULL;
    }
    return ret;
}
#define CREATE_FROM_CLSID(name) AutoComplete_CreateUnknownFromCLSID(name, #name)

static CComPtr<IUnknown>
AutoComplete_CreateList(DWORD dwSHACF, DWORD dwACLO)
{
    CComPtr<IUnknown> pList = CREATE_FROM_CLSID(CLSID_ACLMulti);
    if (!pList)
        return NULL;

    CComPtr<IObjMgr> pManager;
    HRESULT hr = pList->QueryInterface(IID_IObjMgr, (LPVOID *)&pManager);
    if (FAILED(hr))
    {
        ERR("pList->QueryInterface failed: 0x%08lX\n", hr);
        return NULL;
    }

    if (dwSHACF & SHACF_URLMRU)
    {
        CComPtr<IUnknown> pMRU = CREATE_FROM_CLSID(CLSID_ACLMRU);
        if (pMRU)
        {
            pManager->Append(pMRU);
            IUnknown_SetOptions(pMRU, dwACLO);
        }
    }

    if (dwSHACF & SHACF_URLHISTORY)
    {
        CComPtr<IUnknown> pHistory = CREATE_FROM_CLSID(CLSID_ACLHistory);
        if (pHistory)
        {
            pManager->Append(pHistory);
            IUnknown_SetOptions(pHistory, dwACLO);
        }
    }

    if (dwSHACF & SHACF_FILESYSTEM)
    {
        CComPtr<IUnknown> pISF = CREATE_FROM_CLSID(CLSID_ACListISF);
        if (pISF)
        {
            pManager->Append(pISF);
            IUnknown_SetOptions(pISF, dwACLO);
        }
    }

    return pList;
}

#define AUTOCOMPLETE_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete"

static BOOL
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

    CComPtr<IUnknown> pACL = AutoComplete_CreateList(dwSHACF, dwACLO);
    if (!pACL)
    {
        ERR("AutoComplete_CreateList failed\n");
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
