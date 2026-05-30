/*
 * PROJECT:     ReactOS Shell
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Implement QuerySourceCreateFromKey
 * COPYRIGHT:   Copyright 2026 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"
#include <shlwapi.h>
#include <shlwapi_undoc.h>
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell);

static HRESULT SHAllocBlob(ULONG cbData, const BYTE *pbData, FLAGGED_BYTE_BLOB** ppBlob)
{
    FLAGGED_BYTE_BLOB* pBlob = (FLAGGED_BYTE_BLOB*)SHAlloc(sizeof(FLAGGED_BYTE_BLOB) + cbData);
    if (!pBlob)
        return E_OUTOFMEMORY;

    pBlob->clSize = cbData;
    if (pbData)
        CopyMemory(pBlob->abData, pbData, cbData);

    *ppBlob = pBlob;
    return S_OK;
}

class CRegistrySource;

/******************************************************************************
 * CRegistryEnumBase
 */
class CRegistryEnumBase : public IEnumString
{
protected:
    LONG m_cRefs = 1;
    DWORD m_dwIndex = 0;
    CRegistrySource *m_pSource = NULL;
    HKEY m_hKey = NULL;
    LPWSTR m_pszName = NULL;
    WCHAR m_szBuf[64] = {};
    DWORD m_cchNameMax = 0;

    BOOL _Next(LPWSTR *ppwsz);
    virtual BOOL _RegNext(DWORD dwIndex) = 0;
    virtual DWORD _MaxLen() = 0;

public:
    virtual ~CRegistryEnumBase();

    HRESULT Init(HKEY hKey, CRegistrySource *pSource);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    // IEnumString methods
    STDMETHODIMP Next(ULONG celt, LPWSTR* rgelt, ULONG* pceltFetched) override;
    STDMETHODIMP Skip(ULONG celt) override;
    STDMETHODIMP Reset() override;
    STDMETHODIMP Clone(IEnumString ** ppenum) override;
};

/******************************************************************************
 * CRegistryEnumValues
 */
class CRegistryEnumValues : public CRegistryEnumBase
{
public:
    BOOL _RegNext(DWORD dwIndex) override;
    DWORD _MaxLen() override;
};

/******************************************************************************
 * CRegistryEnumKeys
 */
class CRegistryEnumKeys : public CRegistryEnumBase
{
public:
    BOOL _RegNext(DWORD dwIndex) override;
    DWORD _MaxLen() override;
};

/******************************************************************************
 * CRegistrySource
 */
class CRegistrySource
    : public IQuerySourceOld
    , public IObjectWithRegistryKeyOld
{
    LONG m_cRefs = 1;
    HKEY m_hKey = NULL;

public:
    virtual ~CRegistrySource();

    HRESULT Init(HKEY hKey, LPCWSTR pszSubKey, BOOL bCreate);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;
    // IQuerySourceOld methods
    STDMETHODIMP EnumValues(IEnumString **ppEnum) override;
    STDMETHODIMP EnumSources(IEnumString **ppEnum) override;
    STDMETHODIMP QueryValueString(PCWSTR keyName, PCWSTR valueName, PWSTR *ppszValue) override;
    STDMETHODIMP QueryValueDword(PCWSTR keyName, PCWSTR valueName, DWORD *pdwValue) override;
    STDMETHODIMP QueryValueExists(PCWSTR keyName, PCWSTR valueName) override;
    STDMETHODIMP QueryValueDirect(PCWSTR keyName, PCWSTR valueName, FLAGGED_BYTE_BLOB **ppBlob) override;
    STDMETHODIMP OpenSource(PCWSTR keyName, BOOL bCreate, IQuerySourceOld **ppSource) override;
    STDMETHODIMP SetValueDirect(PCWSTR keyName, PCWSTR valueName, DWORD dwType, DWORD cbData, LPCVOID pvData) override;
    // IObjectWithRegistryKeyOld methods
    STDMETHODIMP SetKey(HKEY hKey) override;
    STDMETHODIMP GetKey(HKEY *phKey) override;
};

/******************************************************************************/

CRegistryEnumBase::~CRegistryEnumBase()
{
    if (m_pszName && m_pszName != m_szBuf)
        LocalFree(m_pszName);
    if (m_pSource)
        m_pSource->Release();
}

HRESULT CRegistryEnumBase::Init(HKEY hKey, CRegistrySource *pSource)
{
    m_hKey = hKey;
    m_pSource = pSource;
    m_pSource->AddRef();
    m_cchNameMax = _MaxLen();
    if (m_cchNameMax > _countof(m_szBuf))
    {
        m_pszName = (LPWSTR)LocalAlloc(LPTR, m_cchNameMax * sizeof(WCHAR));
    }
    else
    {
        m_cchNameMax = _countof(m_szBuf);
        m_pszName = m_szBuf;
    }
    return m_pszName ? S_OK : E_OUTOFMEMORY;
}

BOOL CRegistryEnumBase::_Next(LPWSTR *ppwsz)
{
    return _RegNext(m_dwIndex) && SUCCEEDED(SHStrDupW(m_pszName, ppwsz));
}

HRESULT CRegistryEnumBase::QueryInterface(REFIID riid, PVOID* ppv)
{
    if (riid == IID_IEnumString)
    {
        *ppv = static_cast<IEnumString*>(this);
        return S_OK;
    }
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CRegistryEnumBase::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CRegistryEnumBase::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP CRegistryEnumBase::Next(ULONG celt, LPWSTR* rgelt, ULONG* pceltFetched)
{
    if (!rgelt || (celt > 1 && !pceltFetched))
        return E_INVALIDARG;

    ULONG cFetched = 0;

    while (cFetched < celt)
    {
        if (!_Next(&rgelt[cFetched]))
            break;
        ++m_dwIndex;
        ++cFetched;
    }

    if (pceltFetched)
        *pceltFetched = cFetched;

    return (cFetched == celt) ? S_OK : S_FALSE;
}

STDMETHODIMP CRegistryEnumBase::Skip(ULONG celt)
{
    return E_NOTIMPL;
}

STDMETHODIMP CRegistryEnumBase::Reset()
{
    return E_NOTIMPL;
}

STDMETHODIMP CRegistryEnumBase::Clone(IEnumString ** ppenum)
{
    return E_NOTIMPL;
}

/******************************************************************************/

BOOL CRegistryEnumKeys::_RegNext(DWORD dwIndex)
{
    return RegEnumKeyW(m_hKey, dwIndex, m_pszName, m_cchNameMax) == ERROR_SUCCESS;
}

DWORD CRegistryEnumKeys::_MaxLen()
{
    DWORD cchKeyNameMax = 0;
    RegQueryInfoKeyW(m_hKey, NULL, NULL, NULL, NULL, &cchKeyNameMax, NULL, NULL, NULL, NULL, NULL, NULL);
    return cchKeyNameMax;
}

/******************************************************************************/

BOOL CRegistryEnumValues::_RegNext(DWORD dwIndex)
{
    DWORD cchValueNameMax = m_cchNameMax;
    return RegEnumValueW(m_hKey, dwIndex, m_pszName, &cchValueNameMax, NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
}

DWORD CRegistryEnumValues::_MaxLen()
{
    DWORD cchValueNameMax = 0;
    RegQueryInfoKeyW(m_hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cchValueNameMax, NULL, NULL, NULL);
    return cchValueNameMax;
}

/******************************************************************************/

CRegistrySource::~CRegistrySource()
{
    if (m_hKey)
        RegCloseKey(m_hKey);
}

HRESULT CRegistrySource::Init(HKEY hKey, LPCWSTR pszSubKey, BOOL bCreate)
{
    LSTATUS error;
    if (bCreate)
        error = RegCreateKeyExW(hKey, pszSubKey, 0, NULL, 0, MAXIMUM_ALLOWED, NULL, &m_hKey, NULL);
    else
        error = RegOpenKeyExW(hKey, pszSubKey, 0, MAXIMUM_ALLOWED, &m_hKey);

    return error ? HRESULT_FROM_WIN32(error) : S_OK;
}

STDMETHODIMP CRegistrySource::QueryInterface(REFIID riid, void **ppvObject)
{
    if (!ppvObject)
        return E_POINTER;

    if (riid == IID_IQuerySourceOld)
    {
        *ppvObject = static_cast<IQuerySourceOld*>(this);
        return S_OK;
    }

    if (riid == IID_IObjectWithRegistryKeyOld)
    {
        *ppvObject = static_cast<IObjectWithRegistryKeyOld*>(this);
        return S_OK;
    }

    *ppvObject = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CRegistrySource::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CRegistrySource::Release()
{
    if (::InterlockedDecrement(&m_cRefs) == 0)
    {
        delete this;
        return 0;
    }
    return m_cRefs;
}

STDMETHODIMP CRegistrySource::EnumValues(IEnumString **ppEnum)
{
    CRegistryEnumValues* pEnum = new CRegistryEnumValues();
    HRESULT hr = pEnum->Init(m_hKey, this);
    if (FAILED(hr))
    {
        pEnum->Release();
        pEnum = NULL;
    }
    *ppEnum = pEnum;
    return pEnum ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CRegistrySource::EnumSources(IEnumString **ppEnum)
{
    CRegistryEnumKeys* pEnum = new CRegistryEnumKeys();
    HRESULT hr = pEnum->Init(m_hKey, this);
    if (FAILED(hr))
    {
        pEnum->Release();
        pEnum = NULL;
    }
    *ppEnum = pEnum;
    return pEnum ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CRegistrySource::QueryValueString(
    LPCWSTR keyName,
    LPCWSTR valueName,
    LPWSTR *ppszValue)
{
    *ppszValue = NULL;

    WCHAR szData[128];
    DWORD dwType = REG_NONE, cbData = sizeof(szData);
    LSTATUS error = SHGetValueW(m_hKey, keyName, valueName, &dwType, szData, &cbData);
    if (error == ERROR_SUCCESS)
    {
        if (dwType != REG_SZ)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);
        if (!valueName && !szData[0])
            return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
        return SHStrDupW(szData, ppszValue);
    }

    if (error != ERROR_MORE_DATA)
        return error ? HRESULT_FROM_WIN32(error) : S_OK;

    *ppszValue = (LPWSTR)SHAlloc(cbData);
    if (!*ppszValue)
        return E_OUTOFMEMORY;

    HRESULT hr = S_OK;
    error = SHGetValueW(m_hKey, keyName, valueName, &dwType, *ppszValue, &cbData);
    if (error)
    {
        CoTaskMemFree(*ppszValue);
        *ppszValue = NULL;
        hr = HRESULT_FROM_WIN32(error);
    }

    if (dwType != REG_SZ && SUCCEEDED(hr))
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATATYPE);

    return hr;
}

STDMETHODIMP CRegistrySource::QueryValueDword(PCWSTR keyName, PCWSTR valueName, DWORD *pdwValue)
{
    DWORD cbValue = sizeof(*pdwValue);
    LSTATUS error = SHGetValueW(m_hKey, keyName, valueName, NULL, pdwValue, &cbValue);
    if (error)
        return HRESULT_FROM_WIN32(error);
    return S_OK;
}

STDMETHODIMP CRegistrySource::QueryValueExists(PCWSTR keyName, PCWSTR valueName)
{
    LSTATUS error = SHGetValueW(m_hKey, keyName, valueName, NULL, NULL, NULL);
    if (error)
        return HRESULT_FROM_WIN32(error);
    return S_OK;
}

STDMETHODIMP CRegistrySource::QueryValueDirect(
    LPCWSTR keyName,
    LPCWSTR valueName,
    FLAGGED_BYTE_BLOB **ppBlob)
{
    HRESULT hr = E_FAIL;
    HKEY hKey = m_hKey;
    DWORD dwType = REG_NONE;
    DWORD cbData = 256;
    BYTE abData[256];

    *ppBlob = NULL;

    if (keyName && *keyName &&
        RegOpenKeyExW(m_hKey, keyName, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return E_FAIL;
    }

    LSTATUS error = RegQueryValueExW(hKey, valueName, NULL, &dwType, abData, &cbData);
    if (error == ERROR_SUCCESS)
    {
        hr = SHAllocBlob(cbData, abData, ppBlob);
    }
    else if (error == ERROR_MORE_DATA)
    {
        hr = SHAllocBlob(cbData, NULL, ppBlob);
        if (SUCCEEDED(hr))
        {
            error = RegQueryValueExW(hKey, valueName, NULL, &dwType, (*ppBlob)->abData, &cbData);
            if (error != ERROR_SUCCESS)
            {
                CoTaskMemFree(*ppBlob);
                *ppBlob = NULL;
                hr = error ? HRESULT_FROM_WIN32(error) : S_OK;
            }
        }
    }
    else
    {
        hr = HRESULT_FROM_WIN32(error);
    }

    if (hKey != m_hKey)
        RegCloseKey(hKey);

    if (SUCCEEDED(hr))
        (*ppBlob)->fFlags = dwType;

    return hr;
}

STDMETHODIMP CRegistrySource::OpenSource(PCWSTR keyName, BOOL bCreate, IQuerySourceOld **ppSource)
{
    return QuerySourceCreateFromKey(m_hKey, keyName, bCreate, IID_IQuerySource, (PVOID*)ppSource);
}

STDMETHODIMP CRegistrySource::SetValueDirect(
    PCWSTR keyName,
    PCWSTR valueName,
    DWORD dwType,
    DWORD cbData,
    LPCVOID pvData)
{
    LSTATUS error = SHSetValueW(m_hKey, keyName, valueName, dwType, pvData, cbData);
    if (error)
        return HRESULT_FROM_WIN32(error);
    return S_OK;
}

STDMETHODIMP CRegistrySource::SetKey(HKEY hKey)
{
    if (m_hKey)
        return E_UNEXPECTED;

    m_hKey = SHRegDuplicateHKey(hKey);
    return m_hKey ? S_OK : E_UNEXPECTED;
}

STDMETHODIMP CRegistrySource::GetKey(HKEY *phKey)
{
    if (!m_hKey)
        return E_UNEXPECTED;

    *phKey = SHRegDuplicateHKey(m_hKey);
    if (*phKey)
        return S_OK;

    return E_UNEXPECTED;
}

/**************************************************************************
 *  QuerySourceCreateFromKey (SHLWAPI.544)
 *
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/regsrc/createfromkey.htm?tx=116
 */
EXTERN_C
HRESULT WINAPI
QuerySourceCreateFromKey(
    _In_ HKEY hKey,
    _In_ LPCWSTR lpSubKey,
    _In_ BOOL bCreate,
    _In_ REFIID riid,
    _Outptr_ PVOID *ppv)
{
    *ppv = NULL;

    CRegistrySource* pRS = new CRegistrySource();

    HRESULT hr = pRS->Init(hKey, lpSubKey, bCreate);
    if (SUCCEEDED(hr))
        hr = pRS->QueryInterface(riid, ppv);
    pRS->Release();

    return hr;
}
