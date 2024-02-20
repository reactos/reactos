/*
 * PROJECT:     ReactOS msctfime.ime
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Functions
 * COPYRIGHT:   Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "msctfime.h"

WINE_DEFAULT_DEBUG_CHANNEL(msctfime);

/// @implemented
CFunctionProviderBase::CFunctionProviderBase(_In_ TfClientId clientId)
{
    m_clientId = clientId;
    m_guid = GUID_NULL;
    m_bstr = NULL;
    m_cRefs = 1;
}

/// @implemented
CFunctionProviderBase::~CFunctionProviderBase()
{
    if (!DllShutdownInProgress())
        ::SysFreeString(m_bstr);
}

/// @implemented
BOOL
CFunctionProviderBase::Init(
    _In_ REFGUID rguid,
    _In_ LPCWSTR psz)
{
    m_bstr = ::SysAllocString(psz);
    m_guid = rguid;
    return (m_bstr != NULL);
}

/// @implemented
STDMETHODIMP
CFunctionProviderBase::QueryInterface(
    _In_ REFIID riid,
    _Out_ LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ITfFunctionProvider))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CFunctionProviderBase::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CFunctionProviderBase::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @implemented
STDMETHODIMP CFunctionProviderBase::GetType(_Out_ GUID *guid)
{
    *guid = m_guid;
    return S_OK;
}

/// @implemented
STDMETHODIMP CFunctionProviderBase::GetDescription(_Out_ BSTR *desc)
{
    *desc = ::SysAllocString(m_bstr);
    return (*desc ? S_OK : E_OUTOFMEMORY);
}

/***********************************************************************/

/// @implemented
CFunctionProvider::CFunctionProvider(_In_ TfClientId clientId) : CFunctionProviderBase(clientId)
{
    Init(CLSID_CAImmLayer, L"MSCTFIME::Function Provider");
}

/// @implemented
STDMETHODIMP
CFunctionProvider::GetFunction(
    _In_ REFGUID guid,
    _In_ REFIID riid,
    _Out_ IUnknown **func)
{
    *func = NULL;

    if (IsEqualGUID(guid, GUID_NULL) &&
        IsEqualIID(riid, IID_IAImmFnDocFeed))
    {
        *func = new(cicNoThrow) CFnDocFeed();
        if (*func)
            return S_OK;
    }

    return E_NOINTERFACE;
}

/***********************************************************************/

CFnDocFeed::CFnDocFeed()
{
    m_cRefs = 1;
}

CFnDocFeed::~CFnDocFeed()
{
}

/// @implemented
STDMETHODIMP CFnDocFeed::QueryInterface(_In_ REFIID riid, _Out_ LPVOID* ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IAImmFnDocFeed))
    {
        *ppvObj = this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

/// @implemented
STDMETHODIMP_(ULONG) CFnDocFeed::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

/// @implemented
STDMETHODIMP_(ULONG) CFnDocFeed::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::DocFeed()
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::ClearDocFeedBuffer()
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::StartReconvert()
{
    return E_NOTIMPL;
}

/// @unimplemented
STDMETHODIMP CFnDocFeed::StartUndoCompositionString()
{
    return E_NOTIMPL;
}
