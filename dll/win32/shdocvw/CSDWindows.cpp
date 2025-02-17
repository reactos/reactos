/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Shell Window List
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

CSDWindows::CSDWindows()
    : m_hDpa1(NULL)
    , m_hDpa2(NULL)
    , m_dwUnknown(0)
    , m_hwndWorker(NULL)
    , m_dwThreadId(0)
{
}

HRESULT CSDWindows::Init(CComPtr<CSDWindows> pSDWindows, const IID *pIID)
{
    return m_ConnectionPoint.Init(pSDWindows, pIID);
}

STDMETHODIMP CSDWindows::get_Count(_Out_ LONG *Count)
{
    FIXME("%p\n", Count);
    *Count = 0;
    return S_OK;
}

EXTERN_C HRESULT
CSDWindows_CreateInstance(_Out_ IShellWindows **ppShellWindows)
{
    CComPtr<CSDWindows> pSDWindows;
    HRESULT hr = ShellObjectCreator(pSDWindows);
    if (FAILED_UNEXPECTEDLY(hr))
        return E_OUTOFMEMORY;
    hr = pSDWindows->Init(pSDWindows, &DIID_DShellWindowsEvents);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;
    return pSDWindows->QueryInterface(IID_PPV_ARG(IShellWindows, ppShellWindows));
}
