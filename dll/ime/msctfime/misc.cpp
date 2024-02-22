/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Miscellaneous of msctfime.ime
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

/// @implemented
HRESULT
GetCompartment(
    IUnknown *pUnknown,
    REFGUID rguid,
    ITfCompartment **ppComp,
    BOOL bThread)
{
    *ppComp = NULL;

    ITfThreadMgr *pThreadMgr = NULL;
    ITfCompartmentMgr *pCompMgr = NULL;

    HRESULT hr;
    if (bThread)
    {
        hr = pUnknown->QueryInterface(IID_ITfThreadMgr, (void **)&pThreadMgr);
        if (FAILED(hr))
            return hr;

        hr = pThreadMgr->GetGlobalCompartment(&pCompMgr);
    }
    else
    {
        hr = pUnknown->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr);
    }

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (pCompMgr)
        {
            hr = pCompMgr->GetCompartment(rguid, ppComp);
            pCompMgr->Release();
        }
    }

    if (pThreadMgr)
        pThreadMgr->Release();

    return hr;
}

/// @implemented
HRESULT
SetCompartmentDWORD(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    DWORD dwValue,
    BOOL bThread)
{
    ITfCompartment *pComp = NULL;
    HRESULT hr = GetCompartment(pUnknown, rguid, &pComp, bThread);
    if (FAILED(hr))
        return hr;

    VARIANT vari;
    V_I4(&vari) = dwValue;
    V_VT(&vari) = VT_I4;
    hr = pComp->SetValue(cookie, &vari);

    pComp->Release();
    return hr;
}

/// @implemented
HRESULT
GetCompartmentDWORD(
    IUnknown *pUnknown,
    REFGUID rguid,
    LPDWORD pdwValue,
    BOOL bThread)
{
    *pdwValue = 0;

    ITfCompartment *pComp = NULL;
    HRESULT hr = GetCompartment(pUnknown, rguid, &pComp, bThread);
    if (FAILED(hr))
        return hr;

    VARIANT vari;
    hr = pComp->GetValue(&vari);
    if (hr == S_OK)
        *pdwValue = V_I4(&vari);

    pComp->Release();
    return hr;
}

/// @implemented
HRESULT
SetCompartmentUnknown(
    TfEditCookie cookie,
    IUnknown *pUnknown,
    REFGUID rguid,
    IUnknown *punkValue)
{
    ITfCompartment *pComp = NULL;
    HRESULT hr = GetCompartment(pUnknown, rguid, &pComp, FALSE);
    if (FAILED(hr))
        return hr;

    VARIANT vari;
    V_UNKNOWN(&vari) = punkValue;
    V_VT(&vari) = VT_UNKNOWN;
    hr = pComp->SetValue(cookie, &vari);

    pComp->Release();
    return hr;
}

/// @implemented
HRESULT
ClearCompartment(
    TfClientId tid,
    IUnknown *pUnknown,
    REFGUID rguid,
    BOOL bThread)
{
    ITfCompartmentMgr *pCompMgr = NULL;
    ITfThreadMgr *pThreadMgr = NULL;

    HRESULT hr;
    if (bThread)
    {
        hr = pUnknown->QueryInterface(IID_ITfThreadMgr, (void **)&pThreadMgr);
        if (FAILED(hr))
            return hr;

        hr = pThreadMgr->GetGlobalCompartment(&pCompMgr);
    }
    else
    {
        hr = pUnknown->QueryInterface(IID_ITfCompartmentMgr, (void **)&pCompMgr);
    }

    if (SUCCEEDED(hr))
    {
        hr = E_FAIL;
        if (pCompMgr)
        {
            hr = pCompMgr->ClearCompartment(tid, rguid);
            pCompMgr->Release();
        }
    }

    if (pThreadMgr)
        pThreadMgr->Release();

    return hr;
}

/***********************************************************************/

struct MODEBIAS
{
    REFGUID m_guid;
    LONG m_bias;
};

static const MODEBIAS g_ModeBiasMap[] =
{
    { GUID_MODEBIAS_FILENAME,   0x00000001 },
    { GUID_MODEBIAS_NUMERIC,    0x00000004 },
    { GUID_MODEBIAS_URLHISTORY, 0x00010000 },
    { GUID_MODEBIAS_DEFAULT,    0x00000000 },
    { GUID_MODEBIAS_NONE,       0x00000000 },
};

void CModeBias::SetModeBias(REFGUID rguid)
{
    m_guid = rguid;
}

GUID CModeBias::ConvertModeBias(LONG bias)
{
    const GUID *pguid = &GUID_NULL;
    for (auto& item : g_ModeBiasMap)
    {
        if (item.m_bias == bias)
        {
            pguid = &item.m_guid;
            break;
        }
    }

    return *pguid;
}

LONG CModeBias::ConvertModeBias(REFGUID guid)
{
    for (auto& item : g_ModeBiasMap)
    {
        if (IsEqualGUID(guid, item.m_guid))
            return item.m_bias;
    }
    return 0;
}
