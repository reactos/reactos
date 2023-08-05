/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement shell property bags
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define _ATL_NO_EXCEPTIONS
#include "precomp.h"
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <atlstr.h>         // for CStringW
#include <atlsimpcoll.h>    // for CSimpleMap
#include <atlcomcli.h>      // for CComVariant

WINE_DEFAULT_DEBUG_CHANNEL(shell);

class CBasePropertyBag
    : public IPropertyBag
#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    , public IPropertyBag2
#endif
{
protected:
    LONG m_cRefs;   // reference count
    DWORD m_dwMode; // STGM_* flags

public:
    CBasePropertyBag(DWORD dwMode)
        : m_cRefs(0)
        , m_dwMode(dwMode)
    {
    }

    virtual ~CBasePropertyBag() { }

    // IUnknown interface
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override
    {
#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
        if (::IsEqualGUID(riid, IID_IPropertyBag2))
        {
            AddRef();
            *ppvObject = static_cast<IPropertyBag2*>(this);
            return S_OK;
        }
#endif
        if (::IsEqualGUID(riid, IID_IUnknown) || ::IsEqualGUID(riid, IID_IPropertyBag))
        {
            AddRef();
            *ppvObject = static_cast<IPropertyBag*>(this);
            return S_OK;
        }
        return E_NOTIMPL;
    }
    STDMETHODIMP_(ULONG) AddRef() override
    {
        return ::InterlockedIncrement(&m_cRefs);
    }
    STDMETHODIMP_(ULONG) Release() override
    {
        if (::InterlockedDecrement(&m_cRefs) == 0)
        {
            delete this;
            return 0;
        }
        return m_cRefs;
    }

#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    // IPropertyBag2 interface (stubs)
    STDMETHODIMP Read(
        _In_ ULONG cProperties,
        _In_ PROPBAG2 *pPropBag,
        _In_opt_ IErrorLog *pErrorLog,
        _Out_ VARIANT *pvarValue,
        _Out_ HRESULT *phrError) override
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP Write(
        _In_ ULONG cProperties,
        _In_ PROPBAG2 *pPropBag,
        _In_ VARIANT *pvarValue)
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP CountProperties(_Out_ ULONG *pcProperties) override
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP GetPropertyInfo(
        _In_ ULONG iProperty,
        _In_ ULONG cProperties,
        _Out_ PROPBAG2 *pPropBag,
        _Out_ ULONG *pcProperties) override
    {
        return E_NOTIMPL;
    }
    STDMETHODIMP LoadObject(
        _In_z_ LPCWSTR pstrName,
        _In_ DWORD dwHint,
        _In_ IUnknown *pUnkObject,
        _In_opt_ IErrorLog *pErrorLog) override
    {
        return E_NOTIMPL;
    }
#endif
};

struct CPropMapEqual
{
    static bool IsEqualKey(const ATL::CStringW& k1, const ATL::CStringW& k2)
    {
        return k1.CompareNoCase(k2) == 0;
    }

    static bool IsEqualValue(const ATL::CComVariant& v1, const ATL::CComVariant& v2)
    {
        return false;
    }
};

class CMemPropertyBag : public CBasePropertyBag
{
protected:
    ATL::CSimpleMap<ATL::CStringW, ATL::CComVariant, CPropMapEqual> m_PropMap;

public:
    CMemPropertyBag(DWORD dwFlags) : CBasePropertyBag(dwFlags) { }

    STDMETHODIMP Read(_In_z_ LPCWSTR pszPropName, _Inout_ VARIANT *pvari,
                      _Inout_opt_ IErrorLog *pErrorLog) override;
    STDMETHODIMP Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari) override;
};

STDMETHODIMP
CMemPropertyBag::Read(
    _In_z_ LPCWSTR pszPropName,
    _Inout_ VARIANT *pvari,
    _Inout_opt_ IErrorLog *pErrorLog)
{
    UNREFERENCED_PARAMETER(pErrorLog);

    TRACE("%p: %s %p %p\n", this, debugstr_w(pszPropName), pvari, pErrorLog);

    VARTYPE vt = V_VT(pvari);

    ::VariantInit(pvari);

#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    if ((m_dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) == STGM_WRITE)
    {
        ERR("%p: 0x%X\n", this, m_dwMode);
        return E_ACCESSDENIED;
    }
#endif

    if (!pszPropName || !pvari)
    {
        ERR("%p: %s %p %p\n", this, debugstr_w(pszPropName), pvari, pErrorLog);
        return E_INVALIDARG;
    }

    INT iItem = m_PropMap.FindKey(pszPropName);
    if (iItem == -1)
    {
        ERR("%p: %s %p %p\n", this, debugstr_w(pszPropName), pvari, pErrorLog);
        return E_FAIL;
    }

    HRESULT hr = ::VariantCopy(pvari, &m_PropMap.GetValueAt(iItem));
    if (FAILED(hr))
    {
        ERR("%p: 0x%08X %p\n", this, hr, pvari);
        return hr;
    }

    hr = ::VariantChangeTypeForRead(pvari, vt);
    if (FAILED(hr))
    {
        ERR("%p: 0x%08X %p\n", this, hr, pvari);
        return hr;
    }

    return hr;
}

STDMETHODIMP
CMemPropertyBag::Write(
    _In_z_ LPCWSTR pszPropName,
    _In_ VARIANT *pvari)
{
    TRACE("%p: %s %p\n", this, debugstr_w(pszPropName), pvari);

#if (_WIN32_WINNT < _WIN32_WINNT_VISTA)
    if ((m_dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) == STGM_READ)
    {
        ERR("%p: 0x%X\n", this, m_dwMode);
        return E_ACCESSDENIED;
    }
#endif

    if (!pszPropName || !pvari)
    {
        ERR("%p: %s %p\n", this, debugstr_w(pszPropName), pvari);
        return E_INVALIDARG;
    }

    ATL::CComVariant vari;
    HRESULT hr = vari.Copy(pvari);
    if (FAILED(hr))
    {
        ERR("%p: %s %p: 0x%08X\n", this, debugstr_w(pszPropName), pvari, hr);
        return hr;
    }

    if (!m_PropMap.SetAt(pszPropName, vari))
    {
        ERR("%p: %s %p\n", this, debugstr_w(pszPropName), pvari);
        return E_FAIL;
    }

    return hr;
}

/**************************************************************************
 *  SHCreatePropertyBagOnMemory (SHLWAPI.477)
 *
 * Creates a property bag object on memory.
 *
 * @param dwMode  Specifies either STGM_READ, STGM_WRITE or STGM_READWRITE. Ignored on Vista+.
 * @param riid    Specifies either IID_IUnknown, IID_IPropertyBag or IID_IPropertyBag2.
 *                Vista+ rejects IID_IPropertyBag2.
 * @param ppvObj  Receives an IPropertyBag pointer.
 * @return        An HRESULT value. S_OK on success, non-zero on failure.
 * @see           http://undoc.airesoft.co.uk/shlwapi.dll/SHCreatePropertyBagOnMemory.php
 */
EXTERN_C HRESULT WINAPI
SHCreatePropertyBagOnMemory(_In_ DWORD dwMode, _In_ REFIID riid, _Out_ void **ppvObj)
{
    TRACE("0x%08X, %s, %p\n", dwMode, debugstr_guid(&riid), ppvObj);

    *ppvObj = NULL;

    CComPtr<CMemPropertyBag> pMemBag(new CMemPropertyBag(dwMode));

    HRESULT hr = pMemBag->QueryInterface(riid, ppvObj);
    if (FAILED(hr))
        ERR("0x%08X %s\n", hr, debugstr_guid(&riid));

    return hr;
}
