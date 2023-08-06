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

        ERR("%p: %s: E_NOTIMPL\n", this, debugstr_guid(&riid));
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

class CRegPropertyBag : public CBasePropertyBag
{
protected:
    HKEY m_hKey;

    HRESULT _ReadDword(LPCWSTR pszPropName, VARIANT *pvari);
    HRESULT _ReadString(LPCWSTR pszPropName, VARIANTARG *pvarg, UINT len);
    HRESULT _ReadBinary(LPCWSTR pszPropName, VARIANT *pvari, VARTYPE vt, DWORD uBytes);
    HRESULT _ReadStream(VARIANT *pvari, BYTE *pInit, UINT cbInit);
    HRESULT _CopyStreamIntoBuff(IStream *pStream, void *pv, ULONG cb);
    HRESULT _GetStreamSize(IStream *pStream, LPDWORD pcbSize);
    HRESULT _WriteStream(LPCWSTR pszPropName, IStream *pStream);

public:
    CRegPropertyBag(DWORD dwMode)
        : CBasePropertyBag(dwMode)
        , m_hKey(NULL)
    {
    }

    ~CRegPropertyBag() override
    {
        if (m_hKey)
            ::RegCloseKey(m_hKey);
    }

    HRESULT Init(HKEY hKey, LPCWSTR lpSubKey);

    STDMETHODIMP Read(_In_z_ LPCWSTR pszPropName, _Inout_ VARIANT *pvari,
                      _Inout_opt_ IErrorLog *pErrorLog) override;
    STDMETHODIMP Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari) override;
};

HRESULT CRegPropertyBag::Init(HKEY hKey, LPCWSTR lpSubKey)
{
    REGSAM nAccess = 0;
    if ((m_dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) != STGM_WRITE)
        nAccess |= KEY_READ;
    if ((m_dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) != STGM_READ)
        nAccess |= KEY_WRITE;

    LONG error;
    if (m_dwMode & STGM_CREATE)
        error = ::RegCreateKeyExW(hKey, lpSubKey, 0, NULL, 0, nAccess, NULL, &m_hKey, NULL);
    else
        error = ::RegOpenKeyExW(hKey, lpSubKey, 0, nAccess, &m_hKey);

    if (error != ERROR_SUCCESS)
    {
        ERR("%p %s 0x%08X\n", hKey, debugstr_w(lpSubKey), error);
        return HRESULT_FROM_WIN32(error);
    }

    return S_OK;
}

HRESULT CRegPropertyBag::_ReadDword(LPCWSTR pszPropName, VARIANT *pvari)
{
    DWORD cbData = sizeof(DWORD);
    LONG error = SHGetValueW(m_hKey, NULL, pszPropName, NULL, &V_UI4(pvari), &cbData);
    if (error)
        return E_FAIL;

    V_VT(pvari) = VT_UI4;
    return S_OK;
}

HRESULT CRegPropertyBag::_ReadString(LPCWSTR pszPropName, VARIANTARG *pvarg, UINT len)
{
    BSTR bstr = ::SysAllocStringByteLen(NULL, len);
    V_BSTR(pvarg) = bstr;
    if (!bstr)
        return E_OUTOFMEMORY;

    V_VT(pvarg) = VT_BSTR;
    LONG error = SHGetValueW(m_hKey, NULL, pszPropName, NULL, bstr, (LPDWORD)&len);
    if (error)
    {
        ::VariantClear(pvarg);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT CRegPropertyBag::_ReadStream(VARIANT *pvari, BYTE *pInit, UINT cbInit)
{
    IStream *pStream = SHCreateMemStream(pInit, cbInit);
    V_UNKNOWN(pvari) = pStream;
    if (!pStream)
        return E_OUTOFMEMORY;
    V_VT(pvari) = VT_UNKNOWN;
    return S_OK;
}

HRESULT
CRegPropertyBag::_ReadBinary(
    LPCWSTR pszPropName,
    VARIANT *pvari,
    VARTYPE vt,
    DWORD uBytes)
{
    HRESULT hr = E_FAIL;
    if (vt != VT_UNKNOWN || uBytes < sizeof(GUID))
        return hr;

    LPBYTE pbData = (LPBYTE)::LocalAlloc(LMEM_ZEROINIT, uBytes);
    if (!pbData)
        return hr;

    if (!SHGetValueW(m_hKey, NULL, pszPropName, NULL, pbData, &uBytes) &&
        memcmp(&GUID_NULL, pbData, sizeof(GUID)) == 0)
    {
        hr = _ReadStream(pvari, pbData + sizeof(GUID), uBytes - sizeof(GUID));
    }

    ::LocalFree(pbData);

    return hr;
}

HRESULT CRegPropertyBag::_CopyStreamIntoBuff(IStream *pStream, void *pv, ULONG cb)
{
    LARGE_INTEGER li;
    li.QuadPart = 0;
    HRESULT hr = pStream->Seek(li, 0, NULL);
    if (FAILED(hr))
        return hr;
    return pStream->Read(pv, cb, NULL);
}

HRESULT CRegPropertyBag::_GetStreamSize(IStream *pStream, LPDWORD pcbSize)
{
    *pcbSize = 0;

    ULARGE_INTEGER ui;
    HRESULT hr = IStream_Size(pStream, &ui);
    if (FAILED(hr))
        return hr;

    if (ui.DUMMYSTRUCTNAME.HighPart)
        return E_FAIL; /* 64-bit value is not supported */

    *pcbSize = ui.DUMMYSTRUCTNAME.LowPart;
    return hr;
}

STDMETHODIMP
CRegPropertyBag::Read(
    _In_z_ LPCWSTR pszPropName,
    _Inout_ VARIANT *pvari,
    _Inout_opt_ IErrorLog *pErrorLog)
{
    UNREFERENCED_PARAMETER(pErrorLog);

    TRACE("%p: %s %p %p\n", this, debugstr_w(pszPropName), pvari, pErrorLog);

    if ((m_dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) == STGM_WRITE)
    {
        ERR("%p: 0x%X\n", this, m_dwMode);
        ::VariantInit(pvari);
        return E_ACCESSDENIED;
    }

    VARTYPE vt = V_VT(pvari);
    VariantInit(pvari);

    HRESULT hr;
    DWORD dwType, cbValue;
    LONG error = SHGetValueW(m_hKey, NULL, pszPropName, &dwType, NULL, &cbValue);
    if (error != ERROR_SUCCESS)
        hr = E_FAIL;
    else if (dwType == REG_SZ)
        hr = _ReadString(pszPropName, pvari, cbValue);
    else if (dwType == REG_BINARY)
        hr = _ReadBinary(pszPropName, pvari, vt, cbValue);
    else if (dwType == REG_DWORD)
        hr = _ReadDword(pszPropName, pvari);
    else
        hr = E_FAIL;

    if (FAILED(hr))
    {
        ERR("%p: 0x%08X %ld: %s %p\n", this, hr, dwType, debugstr_w(pszPropName), pvari);
        ::VariantInit(pvari);
        return hr;
    }

    hr = ::VariantChangeTypeForRead(pvari, vt);
    if (FAILED(hr))
    {
        ERR("%p: 0x%08X %ld: %s %p\n", this, hr, dwType, debugstr_w(pszPropName), pvari);
        ::VariantInit(pvari);
    }

    return hr;
}

HRESULT
CRegPropertyBag::_WriteStream(LPCWSTR pszPropName, IStream *pStream)
{
    DWORD cbData;
    HRESULT hr = _GetStreamSize(pStream, &cbData);
    if (FAILED(hr) || !cbData)
        return hr;

    DWORD cbBinary = cbData + sizeof(GUID);
    LPBYTE pbBinary = (LPBYTE)::LocalAlloc(LMEM_ZEROINIT, cbBinary);
    if (!pbBinary)
        return E_OUTOFMEMORY;

    hr = _CopyStreamIntoBuff(pStream, pbBinary + sizeof(GUID), cbData);
    if (SUCCEEDED(hr))
    {
        if (SHSetValueW(m_hKey, NULL, pszPropName, REG_BINARY, pbBinary, cbBinary))
            hr = E_FAIL;
    }

    ::LocalFree(pbBinary);
    return hr;
}

STDMETHODIMP
CRegPropertyBag::Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari)
{
    TRACE("%p: %s %p\n", this, debugstr_w(pszPropName), pvari);

    if ((m_dwMode & (STGM_READ | STGM_WRITE | STGM_READWRITE)) == STGM_READ)
    {
        ERR("%p: 0x%X\n", this, m_dwMode);
        return E_ACCESSDENIED;
    }

    HRESULT hr;
    LONG error;
    VARIANTARG vargTemp = { 0 };
    switch (V_VT(pvari))
    {
        case VT_EMPTY:
            SHDeleteValueW(m_hKey, NULL, pszPropName);
            hr = S_OK;
            break;

        case VT_BOOL:
        case VT_I1:
        case VT_I2:
        case VT_I4:
        case VT_UI1:
        case VT_UI2:
        case VT_UI4:
        case VT_INT:
        case VT_UINT:
        {
            hr = ::VariantChangeType(&vargTemp, pvari, 0, VT_UI4);
            if (FAILED(hr))
                return hr;

            error = SHSetValueW(m_hKey, NULL, pszPropName, REG_DWORD, &V_UI4(&vargTemp), sizeof(DWORD));
            if (error)
                hr = E_FAIL;

            ::VariantClear(&vargTemp);
            break;
        }

        case VT_UNKNOWN:
        {
            CComPtr<IStream> pStream;
            hr = V_UNKNOWN(pvari)->QueryInterface(IID_IStream, (void **)&pStream);
            if (FAILED(hr))
                return hr;

            hr = _WriteStream(pszPropName, pStream);
            break;
        }

        default:
        {
            hr = ::VariantChangeType(&vargTemp, pvari, 0, VT_BSTR);
            if (FAILED(hr))
                return hr;

            int cch = lstrlenW(V_BSTR(&vargTemp));
            DWORD cb = (cch + 1) * sizeof(WCHAR);
            error = SHSetValueW(m_hKey, NULL, pszPropName, REG_SZ, V_BSTR(&vargTemp), cb);
            if (error)
                hr = E_FAIL;

            ::VariantClear(&vargTemp);
            break;
        }
    }

    return hr;
}

/**************************************************************************
 *  SHCreatePropertyBagOnRegKey (SHLWAPI.471)
 *
 * Creates a property bag object on registry key.
 *
 * @param hKey       The registry key.
 * @param pszSubKey  The path of the sub-key.
 * @param dwMode     The combination of STGM_READ, STGM_WRITE, STGM_READWRITE, and STGM_CREATE.
 * @param riid       Specifies either IID_IUnknown, IID_IPropertyBag or IID_IPropertyBag2.
 *                   Vista+ rejects IID_IPropertyBag2.
 * @param ppvObj     Receives an IPropertyBag pointer.
 * @return           An HRESULT value. S_OK on success, non-zero on failure.
 * @see              https://source.winehq.org/WineAPI/SHCreatePropertyBagOnRegKey.html
 */
EXTERN_C HRESULT WINAPI
SHCreatePropertyBagOnRegKey(
    _In_ HKEY hKey,
    _In_z_ LPCWSTR pszSubKey,
    _In_ DWORD dwMode,
    _In_ REFIID riid,
    _Out_ void **ppvObj)
{
    TRACE("%p, %s, 0x%08X, %s, %p\n", hKey, debugstr_w(pszSubKey), dwMode,
          debugstr_guid(&riid), ppvObj);

    *ppvObj = NULL;

    CComPtr<CRegPropertyBag> pRegBag(new CRegPropertyBag(dwMode));

    HRESULT hr = pRegBag->Init(hKey, pszSubKey);
    if (FAILED(hr))
        return hr;

    return pRegBag->QueryInterface(riid, ppvObj);
}
