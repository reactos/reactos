/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ITfLangBarMgr implementation
 * COPYRIGHT:   Copyright 2010 Justin Chevrier
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

//*****************************************************************************************

class CLangBarMgr : public ITfLangBarMgr_P
{
public:
    CLangBarMgr();
    virtual ~CLangBarMgr();

    // ** IUnknown interface **
    STDMETHODIMP QueryInterface(_In_ REFIID riid, _Out_ PVOID *ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfLangBarMgr interface **
    STDMETHODIMP AdviseEventSink(
        _In_ ITfLangBarEventSink *pSink,
        _In_ HWND hwnd,
        _In_ DWORD dwflags,
        _In_ DWORD *pdwCookie) override;
    STDMETHODIMP UnAdviseEventSink(_In_ DWORD dwCookie) override;
    STDMETHODIMP GetThreadMarshalInterface(
        _In_ DWORD dwThreadId,
        _In_ DWORD dwType,
        _In_ REFIID riid,
        _Out_ IUnknown **ppunk) override;
    STDMETHODIMP GetThreadLangBarItemMgr(
        _In_ DWORD dwThreadId,
        _Out_ ITfLangBarItemMgr **pplbie,
        _Out_ DWORD *pdwThreadid) override;
    STDMETHODIMP GetInputProcessorProfiles(
        _In_ DWORD dwThreadId,
        _Out_ ITfInputProcessorProfiles **ppaip,
        _Out_ DWORD *pdwThreadid) override;
    STDMETHODIMP RestoreLastFocus(_Out_ DWORD *dwThreadId, _In_ BOOL fPrev) override;
    STDMETHODIMP SetModalInput(
        _In_ ITfLangBarEventSink *pSink,
        _In_ DWORD dwThreadId,
        _In_ DWORD dwFlags) override;
    STDMETHODIMP ShowFloating(_In_ DWORD dwFlags) override;
    STDMETHODIMP GetShowFloatingStatus(_Out_ DWORD *pdwFlags) override;

    // ** ITfLangBarMgr_P interface **
    STDMETHODIMP GetPrevShowFloatingStatus(_Inout_ DWORD* pdwStatus) override;

protected:
    LONG m_cRefs;

    static BOOL CheckFloatingBits(_In_ DWORD dwBits);
    static HRESULT s_GetShowFloatingStatus(_Out_ DWORD *pdwFlags);
    static HRESULT s_ShowFloating(_In_ DWORD dwFlags);
};

// The groups of mutually exclusive TF_SFT_... bits
#define TF_SFT_VISIBILITY_GROUP \
    (TF_SFT_SHOWNORMAL | TF_SFT_DOCK | TF_SFT_MINIMIZED | TF_SFT_HIDDEN | TF_SFT_DESKBAND)
#define TF_SFT_TRANSPARENCY_GROUP \
    (TF_SFT_NOTRANSPARENCY | TF_SFT_LOWTRANSPARENCY | TF_SFT_HIGHTRANSPARENCY)
#define TF_SFT_LABEL_GROUP (TF_SFT_LABELS | TF_SFT_NOLABELS)
#define TF_SFT_EXTRA_ICON_GROUP (TF_SFT_EXTRAICONSONMINIMIZED | TF_SFT_NOEXTRAICONSONMINIMIZED)

static inline BOOL
IsSingleBitSet(DWORD dwValue)
{
    return (dwValue != 0) && ((dwValue & (dwValue - 1)) == 0);
}

//*****************************************************************************************

CLangBarMgr::CLangBarMgr() : m_cRefs(1)
{
}

CLangBarMgr::~CLangBarMgr()
{
}

BOOL CLangBarMgr::CheckFloatingBits(_In_ DWORD dwBits)
{
    return IsSingleBitSet(dwBits & TF_SFT_VISIBILITY_GROUP) &&
           IsSingleBitSet(dwBits & TF_SFT_TRANSPARENCY_GROUP) &&
           IsSingleBitSet(dwBits & TF_SFT_LABEL_GROUP) &&
           IsSingleBitSet(dwBits & TF_SFT_EXTRA_ICON_GROUP);
}

HRESULT CLangBarMgr::s_GetShowFloatingStatus(_Out_ DWORD *pdwFlags)
{
    return E_NOTIMPL;
}

HRESULT CLangBarMgr::s_ShowFloating(_In_ DWORD dwFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::QueryInterface(
    _In_ REFIID riid,
    _Out_ PVOID *ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;
    if (riid == IID_IUnknown || riid == IID_ITfLangBarMgr || riid == IID_ITfLangBarMgr_P)
        *ppvObj = static_cast<ITfLangBarMgr_P *>(this);

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CLangBarMgr::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CLangBarMgr::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP
CLangBarMgr::AdviseEventSink(
    _In_ ITfLangBarEventSink *pSink,
    _In_ HWND hwnd,
    _In_ DWORD dwflags,
    _In_ DWORD *pdwCookie)
{
    FIXME("(%p, %p, 0x%X, %p)\n", pSink, hwnd, dwflags, pdwCookie);
    if (!pSink)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::UnAdviseEventSink(
    _In_ DWORD dwCookie)
{
    FIXME("(0x%lX)\n", dwCookie);
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::GetThreadMarshalInterface(
    _In_ DWORD dwThreadId,
    _In_ DWORD dwType,
    _In_ REFIID riid,
    _Out_ IUnknown **ppunk)
{
    FIXME("(%lu, %lu, %s, %p)\n", dwThreadId, dwType, wine_dbgstr_guid(&riid), ppunk);
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::GetThreadLangBarItemMgr(
    _In_ DWORD dwThreadId,
    _Out_ ITfLangBarItemMgr **pplbie,
    _Out_ DWORD *pdwThreadid)
{
    FIXME("(%lu, %p, %p)\n", dwThreadId, pplbie, pdwThreadid);
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::GetInputProcessorProfiles(
    _In_ DWORD dwThreadId,
    _Out_ ITfInputProcessorProfiles **ppaip,
    _Out_ DWORD *pdwThreadid)
{
    FIXME("(%lu, %p, %p)\n", dwThreadId, ppaip, pdwThreadid);
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::RestoreLastFocus(
    _Out_ DWORD *dwThreadId,
    _In_ BOOL fPrev)
{
    FIXME("(%p, %d)\n", dwThreadId, fPrev);
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::SetModalInput(
    _In_ ITfLangBarEventSink *pSink,
    _In_ DWORD dwThreadId,
    _In_ DWORD dwFlags)
{
    FIXME("(%p, %lu, 0x%lX)\n", pSink, dwThreadId, dwFlags);
    return E_NOTIMPL;
}

STDMETHODIMP
CLangBarMgr::ShowFloating(
    _In_ DWORD dwFlags)
{
    FIXME("(0x%lX)\n", dwFlags);
    if (!CheckFloatingBits(dwFlags))
        return E_INVALIDARG;
    return s_ShowFloating(dwFlags);
}

STDMETHODIMP
CLangBarMgr::GetShowFloatingStatus(
    _Out_ DWORD *pdwFlags)
{
    FIXME("(%p)\n", pdwFlags);

    if (!pdwFlags)
        return E_INVALIDARG;

    return s_GetShowFloatingStatus(pdwFlags);
}

STDMETHODIMP
CLangBarMgr::GetPrevShowFloatingStatus(_Inout_ DWORD* pdwStatus)
{
    FIXME("(%p)\n", pdwStatus);
    if (!pdwStatus)
        return E_INVALIDARG;
    return E_NOTIMPL;
}

//*****************************************************************************************

EXTERN_C HRESULT
LangBarMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CLangBarMgr *pLangBarMgr = new(cicNoThrow) CLangBarMgr();
    if (!pLangBarMgr)
        return E_OUTOFMEMORY;

    HRESULT hr = pLangBarMgr->QueryInterface(IID_ITfLangBarMgr, (PVOID *)ppOut);
    TRACE("returning %p\n", *ppOut);
    pLangBarMgr->Release();
    return hr;
}
