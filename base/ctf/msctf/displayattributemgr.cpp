/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfDisplayAttributeMgr implementation
 * COPYRIGHT:   Copyright 2010 CodeWeavers, Aric Stewart
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include "displayattributemgr.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

CDisplayAttributeMgr::CDisplayAttributeMgr()
    : m_cRefs(1)
{
}

CDisplayAttributeMgr::~CDisplayAttributeMgr()
{
    TRACE("destroying %p\n", this);
}

BOOL CDisplayAttributeMgr::_IsInCollection(REFGUID rguid)
{
    FIXME("(%p)\n", wine_dbgstr_guid(&rguid));
    return FALSE;
}

void CDisplayAttributeMgr::_AdviseMarkupCollection(ITfTextInputProcessor *pProcessor, DWORD dwCookie)
{
    FIXME("(%p, %u)\n", pProcessor, dwCookie);
}

void CDisplayAttributeMgr::_UnadviseMarkupCollection(DWORD dwCookie)
{
    FIXME("(%u)\n", dwCookie);
}

void CDisplayAttributeMgr::_SetThis()
{
    FIXME("()\n");
}

STDMETHODIMP
CDisplayAttributeMgr::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (riid == IID_IUnknown ||
        riid == IID_ITfDisplayAttributeMgr ||
        riid == IID_CDisplayAttributeMgr)
    {
        *ppvObj = this;
    }
    else if (riid == IID_ITfDisplayAttributeCollectionMgr)
    {
        *ppvObj = static_cast<ITfDisplayAttributeCollectionMgr *>(this);
    }

    if (!*ppvObj)
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG)
CDisplayAttributeMgr::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG)
CDisplayAttributeMgr::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CDisplayAttributeMgr::OnUpdateInfo()
{
    FIXME("()\n");
    return E_NOTIMPL;
}

STDMETHODIMP
CDisplayAttributeMgr::EnumDisplayAttributeInfo(_Out_ IEnumTfDisplayAttributeInfo **ppEnum)
{
    FIXME("(%p)\n", ppEnum);
    return E_NOTIMPL;
}

STDMETHODIMP
CDisplayAttributeMgr::GetDisplayAttributeInfo(
    _In_ REFGUID guid,
    _Out_ ITfDisplayAttributeInfo **ppInfo,
    _Out_ CLSID *pclsidOwner)
{
    FIXME("(%s, %p, %s)\n", wine_dbgstr_guid(&guid), ppInfo, wine_dbgstr_guid(pclsidOwner));
    return E_NOTIMPL;
}

STDMETHODIMP
CDisplayAttributeMgr::UnknownMethod(_In_ DWORD unused)
{
    FIXME("(0x%lX)\n", unused);
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT DisplayAttributeMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CDisplayAttributeMgr *This = new(cicNoThrow) CDisplayAttributeMgr();
    if (!This)
        return E_OUTOFMEMORY;

    HRESULT hr = This->QueryInterface(IID_ITfDisplayAttributeMgr, (void **)ppOut);
    This->Release();
    return hr;
}
