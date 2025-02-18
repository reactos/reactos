/*
 * PROJECT:     ReactOS shdocvw
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Connection Point
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "objects.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

CConnectionPoint::CConnectionPoint()
    : m_nRefs(1)
    , m_pSlots(NULL)
    , m_nValidItems(0)
    , m_nSlots(0)
    , m_pContainer(NULL)
    , m_pIID(NULL)
{
    SHDOCVW_LockModule();
}

HRESULT
CConnectionPoint::Init(
    _Inout_ IConnectionPointContainer *pContainer,
    _In_ const IID *pIID)
{
    ATLASSERT(pContainer != NULL);
    ATLASSERT(pIID != NULL);

    m_pContainer = pContainer;
    m_pIID = pIID;
    return S_OK;
}

CConnectionPoint::~CConnectionPoint()
{
    UnadviseAll();
    m_pSlots = (PCONNECTION)LocalFree(m_pSlots);
    m_pContainer = NULL;
    SHDOCVW_UnlockModule();
}

STDMETHODIMP
CConnectionPoint::QueryInterface(
    _In_ REFIID riid,
    _Out_ LPVOID *ppvObj)
{
    if (riid == IID_IConnectionPoint || riid == IID_IUnknown)
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG)
CConnectionPoint::AddRef()
{
    return ++m_nRefs;
}

STDMETHODIMP_(ULONG)
CConnectionPoint::Release()
{
    if (--m_nRefs == 0)
    {
        delete this;
        return 0;
    }
    return m_nRefs;
}

STDMETHODIMP
CConnectionPoint::GetConnectionInterface(
    _Out_ IID *pIID)
{
    *pIID = *m_pIID;
    return S_OK;
}

STDMETHODIMP
CConnectionPoint::GetConnectionPointContainer(
    _Out_ IConnectionPointContainer **ppCPC)
{
    return m_pContainer->QueryInterface(IID_PPV_ARG(IConnectionPointContainer, ppCPC));
}

static HLOCAL
LocalReAllocHelp(
    _Inout_opt_ HLOCAL hLocal,
    _In_ SIZE_T uBytes)
{
    if (!uBytes)
        return LocalFree(hLocal);
    if (!hLocal)
        return LocalAlloc(LPTR, uBytes);
    return LocalReAlloc(hLocal, uBytes, LPTR | LMEM_MOVEABLE);
}

STDMETHODIMP
CConnectionPoint::Advise(
    _In_ IUnknown *pUnkSink,
    _Out_ DWORD *pdwCookie)
{
    if (!pdwCookie)
        return E_POINTER;

    *pdwCookie = 0; // Zero is the invalid cookie

    CComPtr<IDispatch> pDispatch;
    HRESULT hr = pUnkSink->QueryInterface(*m_pIID, (PVOID *)&pDispatch);
    if (FAILED(hr))
    {
        if (*m_pIID != IID_IPropertyNotifySink)
            hr = pUnkSink->QueryInterface(IID_PPV_ARG(IDispatch, &pDispatch));
        if (FAILED(hr))
            return CONNECT_E_CANNOTCONNECT;
    }

    if (m_nValidItems >= m_nSlots) // Needs grow?
    {
#define GROW_COUNT 8
        UINT nNewSlots = m_nSlots + GROW_COUNT;
        SIZE_T cbNewSlots = nNewSlots * sizeof(CONNECTION);
        PCONNECTION pNewSlots = (PCONNECTION)LocalReAllocHelp(m_pSlots, cbNewSlots);
        if (!pNewSlots)
            return E_OUTOFMEMORY;
        m_pSlots = pNewSlots;
        m_nSlots = nNewSlots;
    }

    // Find an empty room
    DWORD iSlot;
    for (iSlot = 0; m_pSlots[iSlot].m_pDispatch; ++iSlot)
        ;

    ATLASSERT(iSlot < m_nSlots);
    m_pSlots[iSlot].m_pDispatch = pDispatch;
    *pdwCookie = iSlot + 1; // This cookie is 1-based
    ++m_nValidItems;

    CComPtr<IConnectionPointCB> pCallback;
    if (SUCCEEDED(m_pContainer->QueryInterface(IID_PPV_ARG(IConnectionPointCB, &pCallback))))
        pCallback->OnAdvise(*m_pIID, m_nValidItems, *pdwCookie);

    return hr;
}

STDMETHODIMP
CConnectionPoint::Unadvise(
    _In_ DWORD dwCookie)
{
    if (!dwCookie)
        return S_OK;

    DWORD iSlot = dwCookie - 1; // This cookie is 1-based
    if (iSlot >= m_nSlots || !m_pSlots[iSlot].m_pDispatch)
        return CONNECT_E_NOCONNECTION;

    CComPtr<IConnectionPointCB> pCallback;
    if (SUCCEEDED(m_pContainer->QueryInterface(IID_PPV_ARG(IConnectionPointCB, &pCallback))))
        pCallback->OnUnadvise(*m_pIID, m_nValidItems - 1, dwCookie);

    m_pSlots[iSlot].m_pDispatch = NULL;
    --m_nValidItems;
    return S_OK;
}

STDMETHODIMP
CConnectionPoint::EnumConnections(
    _Out_ IEnumConnections **ppEnum)
{
    FIXME("\n");
    return E_NOTIMPL;
}

STDMETHODIMP
CConnectionPoint::DoInvokeIE4(
    _In_ DWORD unknown1,
    _In_ DWORD unknown2,
    _In_ DISPID dispId,
    _Inout_ DISPPARAMS *dispParams)
{
    return IConnectionPoint_InvokeWithCancel(this, dispId, dispParams, unknown1, unknown2);
}

STDMETHODIMP
CConnectionPoint::UnknownMethod0(
    _In_ DWORD unknown1,
    _In_ DWORD unknown2)
{
    UNREFERENCED_PARAMETER(unknown1);
    UNREFERENCED_PARAMETER(unknown2);
    return E_NOTIMPL;
}

HRESULT
CConnectionPoint::InvokeDispid(
    _In_ DISPID dispId)
{
    return IConnectionPoint_SimpleInvoke(this, dispId, 0);
}

HRESULT
CConnectionPoint::UnadviseAll()
{
    if (!m_pSlots || !m_nSlots)
        return S_OK;

    for (UINT iSlot = 0; iSlot < m_nSlots; ++iSlot)
        m_pSlots[iSlot].m_pDispatch = NULL;

    return S_OK;
}
