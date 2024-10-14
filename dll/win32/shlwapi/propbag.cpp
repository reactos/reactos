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
#include <shlobj_undoc.h>
#include <shlguid_undoc.h>
#include <atlstr.h>         // for CStringW
#include <atlsimpcoll.h>    // for CSimpleMap
#include <atlcomcli.h>      // for CComVariant
#include <atlconv.h>        // for CA2W and CW2A
#include <strsafe.h>        // for StringC... functions

WINE_DEFAULT_DEBUG_CHANNEL(shell);

#define MODE_CAN_READ(dwMode) \
    (((dwMode) & (STGM_READ | STGM_WRITE | STGM_READWRITE)) != STGM_WRITE)
#define MODE_CAN_WRITE(dwMode) \
    (((dwMode) & (STGM_READ | STGM_WRITE | STGM_READWRITE)) != STGM_READ)

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
        if (!ppvObject)
            return E_POINTER;

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

        ERR("%p: %s: E_NOINTERFACE\n", this, debugstr_guid(&riid));
        return E_NOINTERFACE;
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
    CMemPropertyBag(DWORD dwMode) : CBasePropertyBag(dwMode) { }

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
    if (!MODE_CAN_READ(m_dwMode))
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
    if (!MODE_CAN_WRITE(m_dwMode))
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

    return pMemBag->QueryInterface(riid, ppvObj);
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
    if (MODE_CAN_READ(m_dwMode))
        nAccess |= KEY_READ;
    if (MODE_CAN_WRITE(m_dwMode))
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

    if (!MODE_CAN_READ(m_dwMode))
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

    if (!MODE_CAN_WRITE(m_dwMode))
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

/**************************************************************************
 *  SHGetIniStringW (SHLWAPI.294)
 *
 * @see https://source.winehq.org/WineAPI/SHGetIniStringW.html
 */
EXTERN_C DWORD WINAPI
SHGetIniStringW(
    _In_z_ LPCWSTR appName,
    _In_z_ LPCWSTR keyName,
    _Out_writes_to_(outLen, return + 1) LPWSTR out,
    _In_ DWORD outLen,
    _In_z_ LPCWSTR filename)
{
    TRACE("(%s,%s,%p,%08x,%s)\n", debugstr_w(appName), debugstr_w(keyName),
          out, outLen, debugstr_w(filename));

    if (outLen == 0)
        return 0;

    // Try ".W"-appended section name. See also SHSetIniStringW
    CStringW szSection(appName);
    szSection += L".W";
    CStringW pszWideBuff;
    const INT cchWideMax = 4 * MAX_PATH; // UTF-7 needs 4 times length buffer.
    GetPrivateProfileStringW(szSection, keyName, NULL,
                             pszWideBuff.GetBuffer(cchWideMax), cchWideMax, filename);
    pszWideBuff.ReleaseBuffer();

    if (pszWideBuff.IsEmpty()) // It's empty or not found
    {
        // Try the normal section name
        return GetPrivateProfileStringW(appName, keyName, NULL, out, outLen, filename);
    }

    // Okay, now ".W" version is valid. Its value is a UTF-7 string in UTF-16
    CW2A wide2utf7(pszWideBuff);
    MultiByteToWideChar(CP_UTF7, 0, wide2utf7, -1, out, outLen);
    out[outLen - 1] = UNICODE_NULL;

    return lstrlenW(out);
}

static BOOL Is7BitClean(LPCWSTR psz)
{
    if (!psz)
        return TRUE;

    while (*psz)
    {
        if (*psz > 0x7F)
            return FALSE;
        ++psz;
    }
    return TRUE;
}

/**************************************************************************
 *  SHSetIniStringW (SHLWAPI.295)
 *
 * @see https://source.winehq.org/WineAPI/SHSetIniStringW.html
 */
EXTERN_C BOOL WINAPI
SHSetIniStringW(
    _In_z_ LPCWSTR appName,
    _In_z_ LPCWSTR keyName,
    _In_opt_z_ LPCWSTR str,
    _In_z_ LPCWSTR filename)
{
    TRACE("(%s, %p, %s, %s)\n", debugstr_w(appName), keyName, debugstr_w(str),
          debugstr_w(filename));

    // Write a normal profile string. If str was NULL, then key will be deleted
    if (!WritePrivateProfileStringW(appName, keyName, str, filename))
        return FALSE;

    if (Is7BitClean(str))
    {
        // Delete ".A" version
        CStringW szSection(appName);
        szSection += L".A";
        WritePrivateProfileStringW(szSection, keyName, NULL, filename);

        // Delete ".W" version
        szSection = appName;
        szSection += L".W";
        WritePrivateProfileStringW(szSection, keyName, NULL, filename);

        return TRUE;
    }

    // Now str is not 7-bit clean. It needs UTF-7 encoding in UTF-16.
    // We write ".A" and ".W"-appended sections
    CW2A wide2utf7(str, CP_UTF7);
    CA2W utf72wide(wide2utf7, CP_ACP);

    BOOL ret = TRUE;

    // Write ".A" version
    CStringW szSection(appName);
    szSection += L".A";
    if (!WritePrivateProfileStringW(szSection, keyName, str, filename))
        ret = FALSE;

    // Write ".W" version
    szSection = appName;
    szSection += L".W";
    if (!WritePrivateProfileStringW(szSection, keyName, utf72wide, filename))
        ret = FALSE;

    return ret;
}

/**************************************************************************
 *  SHGetIniStringUTF7W (SHLWAPI.473)
 *
 * Retrieves a string value from an INI file.
 *
 * @param lpAppName         The section name.
 * @param lpKeyName         The key name.
 *                          If this string begins from '@', the value will be interpreted as UTF-7.
 * @param lpReturnedString  Receives a wide string value.
 * @param nSize             The number of characters in lpReturnedString.
 * @param lpFileName        The INI file.
 * @return                  The number of characters copied to the buffer if succeeded.
 */
EXTERN_C DWORD WINAPI
SHGetIniStringUTF7W(
    _In_opt_z_ LPCWSTR lpAppName,
    _In_z_ LPCWSTR lpKeyName,
    _Out_writes_to_(nSize, return + 1) _Post_z_ LPWSTR lpReturnedString,
    _In_ DWORD nSize,
    _In_z_ LPCWSTR lpFileName)
{
    if (*lpKeyName == L'@') // UTF-7
        return SHGetIniStringW(lpAppName, lpKeyName + 1, lpReturnedString, nSize, lpFileName);

    return GetPrivateProfileStringW(lpAppName, lpKeyName, L"", lpReturnedString, nSize, lpFileName);
}

/**************************************************************************
 *  SHSetIniStringUTF7W (SHLWAPI.474)
 *
 * Sets a string value on an INI file.
 *
 * @param lpAppName   The section name.
 * @param lpKeyName   The key name.
 *                    If this begins from '@', the value will be stored as UTF-7.
 * @param lpString    The wide string value to be set.
 * @param lpFileName  The INI file.
 * @return            TRUE if successful. FALSE if failed.
 */
EXTERN_C BOOL WINAPI
SHSetIniStringUTF7W(
    _In_z_ LPCWSTR lpAppName,
    _In_z_ LPCWSTR lpKeyName,
    _In_opt_z_ LPCWSTR lpString,
    _In_z_ LPCWSTR lpFileName)
{
    if (*lpKeyName == L'@') // UTF-7
        return SHSetIniStringW(lpAppName, lpKeyName + 1, lpString, lpFileName);

    return WritePrivateProfileStringW(lpAppName, lpKeyName, lpString, lpFileName);
}

class CIniPropertyBag : public CBasePropertyBag
{
protected:
    LPWSTR m_pszFileName;
    LPWSTR m_pszSection;
    BOOL m_bAlternateStream; // ADS (Alternate Data Stream)

    static BOOL LooksLikeAnAlternateStream(LPCWSTR pszStart)
    {
        LPCWSTR pch = StrRChrW(pszStart, NULL, L'\\');
        if (!pch)
            pch = pszStart;
        return StrChrW(pch, L':') != NULL;
    }

    HRESULT
    _GetSectionAndName(
        LPCWSTR pszStart,
        LPWSTR pszSection,
        UINT cchSectionMax,
        LPWSTR pszName,
        UINT cchNameMax);

public:
    CIniPropertyBag(DWORD dwMode)
        : CBasePropertyBag(dwMode)
        , m_pszFileName(NULL)
        , m_pszSection(NULL)
        , m_bAlternateStream(FALSE)
    {
    }

    ~CIniPropertyBag() override
    {
        ::LocalFree(m_pszFileName);
        ::LocalFree(m_pszSection);
    }

    HRESULT Init(LPCWSTR pszIniFile, LPCWSTR pszSection);

    STDMETHODIMP Read(
        _In_z_ LPCWSTR pszPropName,
        _Inout_ VARIANT *pvari,
        _Inout_opt_ IErrorLog *pErrorLog) override;

    STDMETHODIMP Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari) override;
};

HRESULT CIniPropertyBag::Init(LPCWSTR pszIniFile, LPCWSTR pszSection)
{
    m_pszFileName = StrDupW(pszIniFile);
    if (!m_pszFileName)
        return E_OUTOFMEMORY;

    // Is it an ADS (Alternate Data Stream) pathname?
    m_bAlternateStream = LooksLikeAnAlternateStream(m_pszFileName);

    if (pszSection)
    {
        m_pszSection = StrDupW(pszSection);
        if (!m_pszSection)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

HRESULT
CIniPropertyBag::_GetSectionAndName(
    LPCWSTR pszStart,
    LPWSTR pszSection,
    UINT cchSectionMax,
    LPWSTR pszName,
    UINT cchNameMax)
{
    LPCWSTR pchSep = StrChrW(pszStart, L'\\');
    if (pchSep)
    {
        UINT cchSep = (UINT)(pchSep - pszStart + 1);
        StrCpyNW(pszSection, pszStart, min(cchSep, cchSectionMax));
        StrCpyNW(pszName, pchSep + 1, cchNameMax);
        return S_OK;
    }

    if (m_pszSection)
    {
        StrCpyNW(pszSection, m_pszSection, cchSectionMax);
        StrCpyNW(pszName, pszStart, cchNameMax);
        return S_OK;
    }

    ERR("%p: %s\n", this, debugstr_w(pszStart));
    return E_INVALIDARG;
}

STDMETHODIMP
CIniPropertyBag::Read(
    _In_z_ LPCWSTR pszPropName,
    _Inout_ VARIANT *pvari,
    _Inout_opt_ IErrorLog *pErrorLog)
{
    UNREFERENCED_PARAMETER(pErrorLog);

    TRACE("%p: %s %p %p\n", this, debugstr_w(pszPropName), pvari, pErrorLog);

    VARTYPE vt = V_VT(pvari);

    ::VariantInit(pvari);

    if (!MODE_CAN_READ(m_dwMode))
    {
        ERR("%p: 0x%X\n", this, m_dwMode);
        return E_ACCESSDENIED;
    }

    WCHAR szSection[64], szName[64];
    HRESULT hr =
        _GetSectionAndName(pszPropName, szSection, _countof(szSection), szName, _countof(szName));
    if (FAILED(hr))
        return hr;

    const INT cchBuffMax = 4 * MAX_PATH; // UTF-7 needs 4 times length buffer.
    CComHeapPtr<WCHAR> pszBuff;
    if (!pszBuff.Allocate(cchBuffMax * sizeof(WCHAR)))
        return E_OUTOFMEMORY;

    if (!SHGetIniStringUTF7W(szSection, szName, pszBuff, cchBuffMax, m_pszFileName))
        return E_FAIL;

    BSTR bstr = ::SysAllocString(pszBuff);
    V_BSTR(pvari) = bstr;
    if (!bstr)
        return E_OUTOFMEMORY;

    V_VT(pvari) = VT_BSTR;
    return ::VariantChangeTypeForRead(pvari, vt);
}

STDMETHODIMP
CIniPropertyBag::Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari)
{
    TRACE("%p: %s %p\n", this, debugstr_w(pszPropName), pvari);

    if (!MODE_CAN_WRITE(m_dwMode))
    {
        ERR("%p: 0x%X\n", this, m_dwMode);
        return E_ACCESSDENIED;
    }

    HRESULT hr;
    BSTR bstr;
    VARIANTARG vargTemp = { 0 };
    switch (V_VT(pvari))
    {
        case VT_EMPTY:
            bstr = NULL;
            break;

        case VT_BSTR:
            bstr = V_BSTR(pvari);
            break;

        default:
            hr = ::VariantChangeType(&vargTemp, pvari, 0, VT_BSTR);
            if (FAILED(hr))
                goto Quit;

            bstr = V_BSTR(&vargTemp);
            break;
    }

    WCHAR szSection[64], szName[64];
    hr = _GetSectionAndName(pszPropName, szSection, _countof(szSection), szName, _countof(szName));
    if (SUCCEEDED(hr))
    {
        if (SHSetIniStringUTF7W(szSection, szName, bstr, m_pszFileName))
        {
            if (!m_bAlternateStream)
                SHChangeNotify(SHCNE_UPDATEITEM, SHCNF_PATHW, m_pszFileName, NULL);
        }
        else
        {
            hr = E_FAIL;
        }
    }

Quit:
    ::VariantClear(&vargTemp);
    return hr;
}

/**************************************************************************
 *  SHCreatePropertyBagOnProfileSection (SHLWAPI.472)
 *
 * Creates a property bag object on INI file.
 *
 * @param lpFileName  The INI filename.
 * @param pszSection  The optional section name.
 * @param dwMode      The combination of STGM_READ, STGM_WRITE, STGM_READWRITE, and STGM_CREATE.
 * @param riid        Specifies either IID_IUnknown, IID_IPropertyBag or IID_IPropertyBag2.
 * @param ppvObj      Receives an IPropertyBag pointer.
 * @return            An HRESULT value. S_OK on success, non-zero on failure.
 * @see               https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/propbag/createonprofilesection.htm
 */
EXTERN_C HRESULT WINAPI
SHCreatePropertyBagOnProfileSection(
    _In_z_ LPCWSTR lpFileName,
    _In_opt_z_ LPCWSTR pszSection,
    _In_ DWORD dwMode,
    _In_ REFIID riid,
    _Out_ void **ppvObj)
{
    HANDLE hFile;
    PWCHAR pchFileTitle;
    WCHAR szBuff[MAX_PATH];

    if (dwMode & STGM_CREATE)
    {
        hFile = ::CreateFileW(lpFileName, 0, FILE_SHARE_DELETE, 0, CREATE_NEW,
                              FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            pchFileTitle = PathFindFileNameW(lpFileName);
            if (lstrcmpiW(pchFileTitle, L"desktop.ini") == 0)
            {
                StrCpyNW(szBuff, lpFileName, _countof(szBuff));
                if (PathRemoveFileSpecW(szBuff))
                    PathMakeSystemFolderW(szBuff);
            }
            ::CloseHandle(hFile);
        }
    }

    *ppvObj = NULL;

    if (!PathFileExistsW(lpFileName))
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);

    CComPtr<CIniPropertyBag> pIniPB(new CIniPropertyBag(dwMode));

    HRESULT hr = pIniPB->Init(lpFileName, pszSection);
    if (FAILED(hr))
    {
        ERR("0x%08X\n", hr);
        return hr;
    }

    return pIniPB->QueryInterface(riid, ppvObj);
}

class CDesktopUpgradePropertyBag : public CBasePropertyBag
{
protected:
    BOOL _AlreadyUpgraded(HKEY hKey);
    VOID _MarkAsUpgraded(HKEY hkey);
    HRESULT _ReadFlags(VARIANT *pvari);
    HRESULT _ReadItemPositions(VARIANT *pvari);
    IStream* _GetOldDesktopViewStream();
    IStream* _NewStreamFromOld(IStream *pOldStream);

public:
    CDesktopUpgradePropertyBag() : CBasePropertyBag(STGM_READ) { }

    STDMETHODIMP Read(
        _In_z_ LPCWSTR pszPropName,
        _Inout_ VARIANT *pvari,
        _Inout_opt_ IErrorLog *pErrorLog) override;

    STDMETHODIMP Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari) override
    {
        ERR("%p: %s: Read-only\n", this, debugstr_w(pszPropName));
        return E_NOTIMPL;
    }
};

VOID CDesktopUpgradePropertyBag::_MarkAsUpgraded(HKEY hkey)
{
    DWORD dwValue = TRUE;
    SHSetValueW(hkey, NULL, L"Upgrade", REG_DWORD, &dwValue, sizeof(dwValue));
}

BOOL CDesktopUpgradePropertyBag::_AlreadyUpgraded(HKEY hKey)
{
    // Check the existence of the value written in _MarkAsUpgraded.
    DWORD dwValue, cbData = sizeof(dwValue);
    return SHGetValueW(hKey, NULL, L"Upgrade", NULL, &dwValue, &cbData) == ERROR_SUCCESS;
}

typedef DWORDLONG DESKVIEW_FLAGS; // 64-bit data

HRESULT CDesktopUpgradePropertyBag::_ReadFlags(VARIANT *pvari)
{
    DESKVIEW_FLAGS Flags;
    DWORD cbValue = sizeof(Flags);
    if (SHGetValueW(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\DeskView",
                    L"Settings",
                    NULL,
                    &Flags,
                    &cbValue) != ERROR_SUCCESS || cbValue < sizeof(Flags))
    {
        return E_FAIL;
    }

    V_UINT(pvari) = ((UINT)(Flags >> 32)) | 0x220; // FIXME: Magic number
    V_VT(pvari) = VT_UINT;
    return S_OK;
}

typedef struct tagOLD_STREAM_HEADER
{
    WORD wMagic;
    WORD awUnknown[6];
    WORD wSize;
} OLD_STREAM_HEADER, *POLD_STREAM_HEADER;

IStream* CDesktopUpgradePropertyBag::_NewStreamFromOld(IStream *pOldStream)
{
    OLD_STREAM_HEADER Header;
    HRESULT hr = pOldStream->Read(&Header, sizeof(Header), NULL);
    if (FAILED(hr) || Header.wMagic != 28)
        return NULL;

    // Move stream pointer
    LARGE_INTEGER li;
    li.QuadPart = Header.wSize - sizeof(Header);
    hr = pOldStream->Seek(li, STREAM_SEEK_CUR, NULL);
    if (FAILED(hr))
        return NULL;

    // Get the size
    ULARGE_INTEGER uli;
    hr = IStream_Size(pOldStream, &uli);
    if (FAILED(hr))
        return NULL;

    // Create new stream and attach
    CComPtr<IStream> pNewStream;
    pNewStream.Attach(SHCreateMemStream(NULL, 0));
    if (!pNewStream)
        return NULL;

    // Subtract Header.wSize from the size
    uli.QuadPart -= Header.wSize;

    // Copy to pNewStream
    hr = pOldStream->CopyTo(pNewStream, uli, NULL, NULL);
    if (FAILED(hr))
        return NULL;

    li.QuadPart = 0;
    pNewStream->Seek(li, STREAM_SEEK_SET, NULL);

    return pNewStream.Detach();
}

IStream* CDesktopUpgradePropertyBag::_GetOldDesktopViewStream()
{
    HKEY hKey = SHGetShellKey(SHKEY_Root_HKCU, L"Streams\\Desktop", FALSE);
    if (!hKey)
        return NULL;

    CComPtr<IStream> pOldStream;
    if (!_AlreadyUpgraded(hKey))
    {
        pOldStream.Attach(SHOpenRegStream2W(hKey, NULL, L"ViewView2", 0));
        if (pOldStream)
        {
            ULARGE_INTEGER uli;
            HRESULT hr = IStream_Size(pOldStream, &uli);
            if (SUCCEEDED(hr) && !uli.QuadPart)
                pOldStream.Release();
        }

        if (!pOldStream)
            pOldStream.Attach(SHOpenRegStream2W(hKey, NULL, L"ViewView", 0));

        _MarkAsUpgraded(hKey);
    }

    ::RegCloseKey(hKey);
    return pOldStream.Detach();
}

HRESULT CDesktopUpgradePropertyBag::_ReadItemPositions(VARIANT *pvari)
{
    CComPtr<IStream> pOldStream;
    pOldStream.Attach(_GetOldDesktopViewStream());
    if (!pOldStream)
        return E_FAIL;

    HRESULT hr = E_FAIL;
    IStream *pNewStream = _NewStreamFromOld(pOldStream);
    if (pNewStream)
    {
        V_UNKNOWN(pvari) = pNewStream;
        V_VT(pvari) = VT_UNKNOWN;
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP
CDesktopUpgradePropertyBag::Read(
    _In_z_ LPCWSTR pszPropName,
    _Inout_ VARIANT *pvari,
    _Inout_opt_ IErrorLog *pErrorLog)
{
    UNREFERENCED_PARAMETER(pErrorLog);

    VARTYPE vt = V_VT(pvari);

    HRESULT hr = E_FAIL;
    if (StrCmpW(L"FFlags", pszPropName) == 0)
        hr = _ReadFlags(pvari);
    else if (StrCmpNW(L"ItemPos", pszPropName, 7) == 0)
        hr = _ReadItemPositions(pvari);

    if (FAILED(hr))
    {
        ::VariantInit(pvari);
        return hr;
    }

    return ::VariantChangeType(pvari, pvari, 0, vt);
}

/**************************************************************************
 *  SHGetDesktopUpgradePropertyBag (Internal)
 *
 * Creates or gets a property bag object for desktop upgrade
 *
 * @param riid    Specifies either IID_IUnknown, IID_IPropertyBag or IID_IPropertyBag2.
 * @param ppvObj  Receives an IPropertyBag pointer.
 * @return        An HRESULT value. S_OK on success, non-zero on failure.
 */
HRESULT SHGetDesktopUpgradePropertyBag(REFIID riid, void **ppvObj)
{
    *ppvObj = NULL;
    CComPtr<CDesktopUpgradePropertyBag> pPropBag(new CDesktopUpgradePropertyBag());
    return pPropBag->QueryInterface(riid, ppvObj);
}

class CViewStatePropertyBag : public CBasePropertyBag
{
protected:
    LPITEMIDLIST m_pidl = NULL;
    LPWSTR m_pszPath = NULL;
    DWORD m_dwVspbFlags = 0; // SHGVSPB_... flags
    CComPtr<IPropertyBag> m_pPidlBag;
    CComPtr<IPropertyBag> m_pUpgradeBag;
    CComPtr<IPropertyBag> m_pInheritBag;
    CComPtr<IPropertyBag> m_pUserDefaultsBag;
    CComPtr<IPropertyBag> m_pFolderDefaultsBag;
    CComPtr<IPropertyBag> m_pGlobalDefaultsBag;
    CComPtr<IPropertyBag> m_pReadBag;
    CComPtr<IPropertyBag> m_pWriteBag;
    BOOL m_bPidlBag = FALSE;
    BOOL m_bUpgradeBag = FALSE;
    BOOL m_bInheritBag = FALSE;
    BOOL m_bUserDefaultsBag = FALSE;
    BOOL m_bFolderDefaultsBag = FALSE;
    BOOL m_bGlobalDefaultsBag = FALSE;
    BOOL m_bReadBag = FALSE;
    BOOL m_bWriteBag = FALSE;

    BOOL _IsSamePidl(LPCITEMIDLIST pidlOther) const;
    BOOL _IsSystemFolder() const;
    BOOL _CanAccessPidlBag() const;
    BOOL _CanAccessUserDefaultsBag() const;
    BOOL _CanAccessFolderDefaultsBag() const;
    BOOL _CanAccessGlobalDefaultsBag() const;
    BOOL _CanAccessInheritBag() const;
    BOOL _CanAccessUpgradeBag() const;

    HKEY _GetHKey(DWORD dwVspbFlags);

    UINT _GetMRUSize(HKEY hKey);

    HRESULT _GetMRUSlots(
        LPCITEMIDLIST pidl,
        DWORD dwMode,
        HKEY hKey,
        UINT *puSlots,
        UINT cSlots,
        UINT *pcSlots);

    HRESULT _GetMRUSlot(LPCITEMIDLIST pidl, DWORD dwMode, HKEY hKey, UINT *pSlot);

    HRESULT _GetRegKey(
        LPCITEMIDLIST pidl,
        LPCWSTR pszBagName,
        DWORD dwFlags,
        DWORD dwMode,
        HKEY hKey,
        LPWSTR pszDest,
        INT cchDest);

    HRESULT _CreateBag(
        LPITEMIDLIST pidl,
        LPCWSTR pszPath,
        DWORD dwVspbFlags,
        DWORD dwMode,
        REFIID riid,
        IPropertyBag **pppb);

    HRESULT _FindNearestInheritBag(REFIID riid, IPropertyBag **pppb);

    void _ResetTryAgainFlag();

    BOOL _EnsureReadBag(DWORD dwMode, REFIID riid);
    BOOL _EnsurePidlBag(DWORD dwMode, REFIID riid);
    BOOL _EnsureInheritBag(DWORD dwMode, REFIID riid);
    BOOL _EnsureUpgradeBag(DWORD dwMode, REFIID riid);
    BOOL _EnsureUserDefaultsBag(DWORD dwMode, REFIID riid);
    BOOL _EnsureFolderDefaultsBag(DWORD dwMode, REFIID riid);
    BOOL _EnsureGlobalDefaultsBag(DWORD dwMode, REFIID riid);
    BOOL _EnsureWriteBag(DWORD dwMode, REFIID riid);
    HRESULT _ReadPidlBag(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog);
    HRESULT _ReadInheritBag(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog);
    HRESULT _ReadUpgradeBag(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog);
    HRESULT _ReadUserDefaultsBag(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog);
    HRESULT _ReadFolderDefaultsBag(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog);
    HRESULT _ReadGlobalDefaultsBag(LPCWSTR pszPropName, VARIANT *pvari, IErrorLog *pErrorLog);
    void _PruneMRUTree();

public:
    CViewStatePropertyBag() : CBasePropertyBag(STGM_READ) { }

    ~CViewStatePropertyBag() override
    {
        ::ILFree(m_pidl);
        ::LocalFree(m_pszPath);
    }

    HRESULT Init(_In_opt_ LPCITEMIDLIST pidl, _In_opt_ LPCWSTR pszPath, _In_ DWORD dwVspbFlags);
    BOOL IsSameBag(LPCITEMIDLIST pidl, LPCWSTR pszPath, DWORD dwVspbFlags) const;

    STDMETHODIMP Read(
        _In_z_ LPCWSTR pszPropName,
        _Inout_ VARIANT *pvari,
        _Inout_opt_ IErrorLog *pErrorLog) override;

    STDMETHODIMP Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari) override;
};

// CViewStatePropertyBag is cached
CComPtr<CViewStatePropertyBag> g_pCachedBag;
extern "C"
{
    CRITICAL_SECTION g_csBagCacheLock;
}

HRESULT
CViewStatePropertyBag::Init(
    _In_opt_ LPCITEMIDLIST pidl,
    _In_opt_ LPCWSTR pszPath,
    _In_ DWORD dwVspbFlags)
{
    if (pidl)
    {
        m_pidl = ILClone(pidl);
        if (!m_pidl)
            return E_OUTOFMEMORY;
    }

    if (pszPath)
    {
        m_pszPath = StrDupW(pszPath);
        if (!m_pszPath)
            return E_OUTOFMEMORY;

        m_dwVspbFlags = dwVspbFlags;
    }

    return S_OK;
}

BOOL CViewStatePropertyBag::_IsSamePidl(LPCITEMIDLIST pidlOther) const
{
    if (!pidlOther && !m_pidl)
        return TRUE;

    return (pidlOther && m_pidl && ILIsEqual(pidlOther, m_pidl));
}

BOOL CViewStatePropertyBag::IsSameBag(LPCITEMIDLIST pidl, LPCWSTR pszPath, DWORD dwVspbFlags) const
{
    return (dwVspbFlags == m_dwVspbFlags && StrCmpW(pszPath, m_pszPath) == 0 && _IsSamePidl(pidl));
}

BOOL CViewStatePropertyBag::_IsSystemFolder() const
{
    LPCITEMIDLIST ppidlLast;
    CComPtr<IShellFolder> psf;

    HRESULT hr = SHBindToParent(m_pidl, IID_IShellFolder, (void **)&psf, &ppidlLast);
    if (FAILED(hr))
        return FALSE;

    WIN32_FIND_DATAW FindData;
    hr = SHGetDataFromIDListW(psf, ppidlLast, SHGDFIL_FINDDATA, &FindData, sizeof(FindData));
    if (FAILED(hr))
        return FALSE;

    return PathIsSystemFolderW(NULL, FindData.dwFileAttributes);
}

BOOL CViewStatePropertyBag::_CanAccessPidlBag() const
{
    return ((m_dwVspbFlags & SHGVSPB_FOLDER) == SHGVSPB_FOLDER);
}

BOOL CViewStatePropertyBag::_CanAccessUserDefaultsBag() const
{
    if (_CanAccessPidlBag())
        return TRUE;

    return ((m_dwVspbFlags & SHGVSPB_USERDEFAULTS) == SHGVSPB_USERDEFAULTS);
}

BOOL CViewStatePropertyBag::_CanAccessFolderDefaultsBag() const
{
    if (_CanAccessUserDefaultsBag())
        return TRUE;

    return ((m_dwVspbFlags & SHGVSPB_ALLUSERS) && (m_dwVspbFlags & SHGVSPB_PERFOLDER));
}

BOOL CViewStatePropertyBag::_CanAccessGlobalDefaultsBag() const
{
    if (_CanAccessFolderDefaultsBag())
        return TRUE;

    return ((m_dwVspbFlags & SHGVSPB_GLOBALDEAFAULTS) == SHGVSPB_GLOBALDEAFAULTS);
}

BOOL CViewStatePropertyBag::_CanAccessInheritBag() const
{
    return (_CanAccessPidlBag() || (m_dwVspbFlags & SHGVSPB_INHERIT));
}

BOOL CViewStatePropertyBag::_CanAccessUpgradeBag() const
{
    return StrCmpW(m_pszPath, L"Desktop") == 0;
}

void CViewStatePropertyBag::_ResetTryAgainFlag()
{
    if (m_dwVspbFlags & SHGVSPB_NOAUTODEFAULTS)
        m_bReadBag = FALSE;
    else if ((m_dwVspbFlags & SHGVSPB_FOLDER) == SHGVSPB_FOLDER)
        m_bPidlBag = FALSE;
    else if (m_dwVspbFlags & SHGVSPB_INHERIT)
        m_bInheritBag = FALSE;
    else if ((m_dwVspbFlags & SHGVSPB_USERDEFAULTS) == SHGVSPB_USERDEFAULTS)
        m_bUserDefaultsBag = FALSE;
    else if ((m_dwVspbFlags & SHGVSPB_ALLUSERS) && (m_dwVspbFlags & SHGVSPB_PERFOLDER))
        m_bFolderDefaultsBag = FALSE;
    else if ((m_dwVspbFlags & SHGVSPB_GLOBALDEAFAULTS) == SHGVSPB_GLOBALDEAFAULTS)
        m_bGlobalDefaultsBag = FALSE;
}

HKEY CViewStatePropertyBag::_GetHKey(DWORD dwVspbFlags)
{
    if (!(dwVspbFlags & (SHGVSPB_INHERIT | SHGVSPB_PERUSER)))
        return SHGetShellKey((SHKEY_Key_Shell | SHKEY_Root_HKLM), NULL, TRUE);

    if ((m_dwVspbFlags & SHGVSPB_ROAM) && (dwVspbFlags & SHGVSPB_PERFOLDER))
        return SHGetShellKey((SHKEY_Key_Shell | SHKEY_Root_HKCU), NULL, TRUE);

    return SHGetShellKey(SHKEY_Key_ShellNoRoam | SHKEY_Root_HKCU, NULL, TRUE);
}

UINT CViewStatePropertyBag::_GetMRUSize(HKEY hKey)
{
    DWORD dwValue, cbValue = sizeof(dwValue);

    if (SHGetValueW(hKey, NULL, L"BagMRU Size", NULL, &dwValue, &cbValue) != ERROR_SUCCESS)
        return 400; // The default size of the MRU (most recently used) list

    return (UINT)dwValue;
}

HRESULT
CViewStatePropertyBag::_GetMRUSlots(
    LPCITEMIDLIST pidl,
    DWORD dwMode,
    HKEY hKey,
    UINT *puSlots,
    UINT cSlots,
    UINT *pcSlots)
{
    CComPtr<IMruPidlList> pMruList;
    HRESULT hr = ::CoCreateInstance(CLSID_MruPidlList, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IMruPidlList, (void**)&pMruList);
    if (FAILED(hr))
        return hr;

    UINT cMRUSize = _GetMRUSize(hKey);
    hr = pMruList->InitList(cMRUSize, hKey, L"BagMRU");
    if (FAILED(hr))
        return hr;

    hr = pMruList->QueryPidl(pidl, cSlots, puSlots, pcSlots);
    if (hr == S_OK && MODE_CAN_WRITE(dwMode))
        hr = pMruList->UsePidl(pidl, puSlots);
    else if (cSlots == 1)
        hr = E_FAIL;

    return hr;
}

HRESULT
CViewStatePropertyBag::_GetMRUSlot(LPCITEMIDLIST pidl, DWORD dwMode, HKEY hKey, UINT *pSlot)
{
    UINT cSlots;
    return _GetMRUSlots(pidl, dwMode, hKey, pSlot, 1, &cSlots);
}

HRESULT
CViewStatePropertyBag::_GetRegKey(
    LPCITEMIDLIST pidl,
    LPCWSTR pszBagName,
    DWORD dwFlags,
    DWORD dwMode,
    HKEY hKey,
    LPWSTR pszDest,
    INT cchDest)
{
    HRESULT hr = S_OK;
    UINT nSlot;

    if (dwFlags & (SHGVSPB_INHERIT | SHGVSPB_PERFOLDER))
    {
        hr = _GetMRUSlot(pidl, dwMode, hKey, &nSlot);
        if (SUCCEEDED(hr))
        {
            if (dwFlags & SHGVSPB_INHERIT)
                StringCchPrintfW(pszDest, cchDest, L"Bags\\%d\\%s\\Inherit", nSlot, pszBagName);
            else
                StringCchPrintfW(pszDest, cchDest, L"Bags\\%d\\%s", nSlot, pszBagName);
        }
    }
    else
    {
        StringCchPrintfW(pszDest, cchDest, L"Bags\\AllFolders\\%s", pszBagName);
    }

    return hr;
}

static HRESULT BindCtx_CreateWithMode(DWORD dwMode, IBindCtx **ppbc)
{
    HRESULT hr = ::CreateBindCtx(0, ppbc);
    if (FAILED(hr))
        return hr;

    IBindCtx *pbc = *ppbc;

    BIND_OPTS opts = { sizeof(opts) };
    opts.grfMode = dwMode;
    hr = pbc->SetBindOptions(&opts);
    if (FAILED(hr))
    {
        pbc->Release();
        *ppbc = NULL;
    }

    return hr;
}

HRESULT
CViewStatePropertyBag::_CreateBag(
    LPITEMIDLIST pidl,
    LPCWSTR pszPath,
    DWORD dwVspbFlags,
    DWORD dwMode,
    REFIID riid,
    IPropertyBag **pppb)
{
    HRESULT hr;
    HKEY hKey;
    CComPtr<IBindCtx> pBC;
    CComPtr<IShellFolder> psf;
    WCHAR szBuff[64];

    if (MODE_CAN_WRITE(dwMode))
        dwMode |= STGM_CREATE;

    if ((dwVspbFlags & SHGVSPB_ALLUSERS) && (dwVspbFlags & SHGVSPB_PERFOLDER))
    {
        hr = BindCtx_CreateWithMode(dwMode, &pBC);
        if (SUCCEEDED(hr))
        {
            hr = SHGetDesktopFolder(&psf);
            if (SUCCEEDED(hr))
            {
                hr = psf->BindToObject(m_pidl, pBC, riid, (void **)pppb);
                if (SUCCEEDED(hr) && !*pppb)
                    hr = E_FAIL;
            }
        }
    }
    else
    {
        hKey = _GetHKey(dwVspbFlags);
        if (!hKey)
            return E_FAIL;

        hr = _GetRegKey(pidl, pszPath, dwVspbFlags, dwMode, hKey, szBuff, _countof(szBuff));
        if (SUCCEEDED(hr))
            hr = SHCreatePropertyBagOnRegKey(hKey, szBuff, dwMode, riid, (void**)pppb);

        ::RegCloseKey(hKey);
    }

    return hr;
}

HRESULT
CViewStatePropertyBag::_FindNearestInheritBag(REFIID riid, IPropertyBag **pppb)
{
    *pppb = NULL;

    HKEY hKey = _GetHKey(SHGVSPB_INHERIT);
    if (!hKey)
        return E_FAIL;

    UINT cSlots, anSlots[64];
    if (FAILED(_GetMRUSlots(m_pidl, 0, hKey, anSlots, _countof(anSlots), &cSlots)) || !cSlots)
    {
        ::RegCloseKey(hKey);
        return E_FAIL;
    }

    HRESULT hr = E_FAIL;
    WCHAR szBuff[64];
    for (UINT iSlot = 0; iSlot < cSlots; ++iSlot)
    {
        StringCchPrintfW(szBuff, _countof(szBuff), L"Bags\\%d\\%s\\Inherit", anSlots[iSlot],
                         m_pszPath);
        hr = SHCreatePropertyBagOnRegKey(hKey, szBuff, STGM_READ, riid, (void**)pppb);
        if (SUCCEEDED(hr))
            break;
    }

    ::RegCloseKey(hKey);
    return hr;
}

BOOL CViewStatePropertyBag::_EnsureReadBag(DWORD dwMode, REFIID riid)
{
    if (!m_pReadBag && !m_bReadBag)
    {
        m_bReadBag = TRUE;
        _CreateBag(m_pidl, m_pszPath, m_dwVspbFlags, dwMode, riid, &m_pReadBag);
    }
    return (m_pReadBag != NULL);
}

BOOL CViewStatePropertyBag::_EnsurePidlBag(DWORD dwMode, REFIID riid)
{
    if (!m_pPidlBag && !m_bPidlBag && _CanAccessPidlBag())
    {
        m_bPidlBag = TRUE;
        _CreateBag(m_pidl, m_pszPath, SHGVSPB_FOLDER, dwMode, riid, &m_pPidlBag);
    }
    return (m_pPidlBag != NULL);
}

BOOL CViewStatePropertyBag::_EnsureInheritBag(DWORD dwMode, REFIID riid)
{
    if (!m_pInheritBag && !m_bInheritBag && _CanAccessInheritBag())
    {
        m_bInheritBag = TRUE;
        _FindNearestInheritBag(riid, &m_pInheritBag);
    }
    return (m_pInheritBag != NULL);
}

BOOL CViewStatePropertyBag::_EnsureUpgradeBag(DWORD dwMode, REFIID riid)
{
    if (!m_pUpgradeBag && !m_bUpgradeBag && _CanAccessUpgradeBag())
    {
        m_bUpgradeBag = TRUE;
        SHGetDesktopUpgradePropertyBag(riid, (void**)&m_pUpgradeBag);
    }
    return (m_pUpgradeBag != NULL);
}

BOOL CViewStatePropertyBag::_EnsureUserDefaultsBag(DWORD dwMode, REFIID riid)
{
    if (!m_pUserDefaultsBag && !m_bUserDefaultsBag && _CanAccessUserDefaultsBag())
    {
        m_bUserDefaultsBag = TRUE;
        _CreateBag(NULL, m_pszPath, SHGVSPB_USERDEFAULTS, dwMode, riid, &m_pUserDefaultsBag);
    }
    return (m_pUserDefaultsBag != NULL);
}

BOOL CViewStatePropertyBag::_EnsureFolderDefaultsBag(DWORD dwMode, REFIID riid)
{
    if (!m_pFolderDefaultsBag && !m_bFolderDefaultsBag && _CanAccessFolderDefaultsBag())
    {
        m_bFolderDefaultsBag = TRUE;
        if (_IsSystemFolder())
        {
            _CreateBag(m_pidl, m_pszPath, SHGVSPB_PERFOLDER | SHGVSPB_ALLUSERS,
                       dwMode, riid, &m_pFolderDefaultsBag);
        }
    }
    return (m_pFolderDefaultsBag != NULL);
}

BOOL CViewStatePropertyBag::_EnsureGlobalDefaultsBag(DWORD dwMode, REFIID riid)
{
    if (!m_pGlobalDefaultsBag && !m_bGlobalDefaultsBag && _CanAccessGlobalDefaultsBag())
    {
        m_bGlobalDefaultsBag = TRUE;
        _CreateBag(NULL, m_pszPath, SHGVSPB_GLOBALDEAFAULTS, dwMode, riid, &m_pGlobalDefaultsBag);
    }
    return (m_pGlobalDefaultsBag != NULL);
}

HRESULT
CViewStatePropertyBag::_ReadPidlBag(
    LPCWSTR pszPropName,
    VARIANT *pvari,
    IErrorLog *pErrorLog)
{
    if (!_EnsurePidlBag(STGM_READ, IID_IPropertyBag))
        return E_FAIL;

    return m_pPidlBag->Read(pszPropName, pvari, pErrorLog);
}

HRESULT
CViewStatePropertyBag::_ReadInheritBag(
    LPCWSTR pszPropName,
    VARIANT *pvari,
    IErrorLog *pErrorLog)
{
    if (!_EnsureInheritBag(STGM_READ, IID_IPropertyBag))
        return E_FAIL;

    return m_pInheritBag->Read(pszPropName, pvari, pErrorLog);
}

HRESULT
CViewStatePropertyBag::_ReadUpgradeBag(
    LPCWSTR pszPropName,
    VARIANT *pvari,
    IErrorLog *pErrorLog)
{
    if (!_EnsureUpgradeBag(STGM_READ, IID_IPropertyBag))
        return E_FAIL;

    return m_pUpgradeBag->Read(pszPropName, pvari, pErrorLog);
}

HRESULT
CViewStatePropertyBag::_ReadUserDefaultsBag(
    LPCWSTR pszPropName,
    VARIANT *pvari,
    IErrorLog *pErrorLog)
{
    if (!_EnsureUserDefaultsBag(STGM_READ, IID_IPropertyBag))
        return E_FAIL;

    return m_pUserDefaultsBag->Read(pszPropName, pvari, pErrorLog);
}

HRESULT
CViewStatePropertyBag::_ReadFolderDefaultsBag(
    LPCWSTR pszPropName,
    VARIANT *pvari,
    IErrorLog *pErrorLog)
{
    if (!_EnsureFolderDefaultsBag(STGM_READ, IID_IPropertyBag))
        return E_FAIL;

    return m_pFolderDefaultsBag->Read(pszPropName, pvari, pErrorLog);
}

HRESULT
CViewStatePropertyBag::_ReadGlobalDefaultsBag(
    LPCWSTR pszPropName,
    VARIANT *pvari,
    IErrorLog *pErrorLog)
{
    if (!_EnsureGlobalDefaultsBag(STGM_READ, IID_IPropertyBag))
        return E_FAIL;

    return m_pGlobalDefaultsBag->Read(pszPropName, pvari, pErrorLog);
}

STDMETHODIMP
CViewStatePropertyBag::Read(
    _In_z_ LPCWSTR pszPropName,
    _Inout_ VARIANT *pvari,
    _Inout_opt_ IErrorLog *pErrorLog)
{
    if ((m_dwVspbFlags & SHGVSPB_NOAUTODEFAULTS) || (m_dwVspbFlags & SHGVSPB_INHERIT))
    {
        if (!_EnsureReadBag(STGM_READ, IID_IPropertyBag))
            return E_FAIL;

        return m_pReadBag->Read(pszPropName, pvari, pErrorLog);
    }

    HRESULT hr = _ReadPidlBag(pszPropName, pvari, pErrorLog);
    if (SUCCEEDED(hr))
        return hr;

    hr = _ReadInheritBag(pszPropName, pvari, pErrorLog);
    if (SUCCEEDED(hr))
        return hr;

    hr = _ReadUpgradeBag(pszPropName, pvari, pErrorLog);
    if (SUCCEEDED(hr))
        return hr;

    hr = _ReadUserDefaultsBag(pszPropName, pvari, pErrorLog);
    if (SUCCEEDED(hr))
        return hr;

    hr = _ReadFolderDefaultsBag(pszPropName, pvari, pErrorLog);
    if (SUCCEEDED(hr))
        return hr;

    return _ReadGlobalDefaultsBag(pszPropName, pvari, pErrorLog);
}

void CViewStatePropertyBag::_PruneMRUTree()
{
    HKEY hKey = _GetHKey(SHGVSPB_INHERIT);
    if (!hKey)
        return;

    CComPtr<IMruPidlList> pMruList;
    HRESULT hr = ::CoCreateInstance(CLSID_MruPidlList, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IMruPidlList, (void**)&pMruList);
    if (SUCCEEDED(hr))
    {
        hr = pMruList->InitList(200, hKey, L"BagMRU");
        if (SUCCEEDED(hr))
            pMruList->PruneKids(m_pidl);
    }

    ::RegCloseKey(hKey);
}

BOOL CViewStatePropertyBag::_EnsureWriteBag(DWORD dwMode, REFIID riid)
{
    if (!m_pWriteBag && !m_bWriteBag)
    {
        m_bWriteBag = TRUE;
        _CreateBag(m_pidl, m_pszPath, m_dwVspbFlags, dwMode, riid, &m_pWriteBag);
        if (m_pWriteBag)
        {
            _ResetTryAgainFlag();
            if (m_dwVspbFlags & SHGVSPB_INHERIT)
                _PruneMRUTree();
        }
    }
    return (m_pWriteBag != NULL);
}

STDMETHODIMP CViewStatePropertyBag::Write(_In_z_ LPCWSTR pszPropName, _In_ VARIANT *pvari)
{
    if (!_EnsureWriteBag(STGM_WRITE, IID_IPropertyBag))
        return E_FAIL;

    return m_pWriteBag->Write(pszPropName, pvari);
}

static BOOL SHIsRemovableDrive(LPCITEMIDLIST pidl)
{
    STRRET strret;
    CComPtr<IShellFolder> psf;
    WCHAR szBuff[MAX_PATH];
    LPCITEMIDLIST ppidlLast;
    INT iDrive, nType;
    HRESULT hr;

    hr = SHBindToParent(pidl, IID_IShellFolder, (void **)&psf, &ppidlLast);
    if (FAILED(hr))
        return FALSE;

    hr = psf->GetDisplayNameOf(ppidlLast, SHGDN_FORPARSING, &strret);
    if (FAILED(hr))
        return FALSE;

    hr = StrRetToBufW(&strret, ppidlLast, szBuff, _countof(szBuff));
    if (FAILED(hr))
        return FALSE;

    iDrive = PathGetDriveNumberW(szBuff);
    if (iDrive < 0)
        return FALSE;

    nType = RealDriveType(iDrive, FALSE);
    return (nType == DRIVE_REMOVABLE || nType == DRIVE_CDROM);
}

/**************************************************************************
 *  SHGetViewStatePropertyBag (SHLWAPI.515)
 *
 * Retrieves a property bag in which the view state information of a folder
 * can be stored.
 *
 * @param pidl      PIDL of the folder requested
 * @param bag_name  Name of the property bag requested
 * @param flags     Optional SHGVSPB_... flags
 * @param riid      IID of requested property bag interface
 * @param ppv       Address to receive pointer to the new interface
 * @return          An HRESULT value. S_OK on success, non-zero on failure.
 * @see https://learn.microsoft.com/en-us/windows/win32/api/shlwapi/nf-shlwapi-shgetviewstatepropertybag
 */
EXTERN_C HRESULT WINAPI
SHGetViewStatePropertyBag(
    _In_opt_ PCIDLIST_ABSOLUTE pidl,
    _In_opt_ LPCWSTR bag_name,
    _In_ DWORD flags,
    _In_ REFIID riid,
    _Outptr_ void **ppv)
{
    HRESULT hr;

    TRACE("%p %s 0x%X %p %p\n", pidl, debugstr_w(bag_name), flags, &riid, ppv);

    *ppv = NULL;

    ::EnterCriticalSection(&g_csBagCacheLock);

    if (g_pCachedBag && g_pCachedBag->IsSameBag(pidl, bag_name, flags))
    {
        hr = g_pCachedBag->QueryInterface(riid, ppv);
        ::LeaveCriticalSection(&g_csBagCacheLock);
        return hr;
    }

    if (SHIsRemovableDrive(pidl))
    {
        TRACE("pidl %p is removable\n", pidl);
        ::LeaveCriticalSection(&g_csBagCacheLock);
        return E_FAIL;
    }

    CComPtr<CViewStatePropertyBag> pBag(new CViewStatePropertyBag());

    hr = pBag->Init(pidl, bag_name, flags);
    if (FAILED(hr))
    {
        ERR("0x%08X\n", hr);
        ::LeaveCriticalSection(&g_csBagCacheLock);
        return hr;
    }

    g_pCachedBag.Attach(pBag);

    hr = g_pCachedBag->QueryInterface(riid, ppv);

    ::LeaveCriticalSection(&g_csBagCacheLock);
    return hr;
}

EXTERN_C VOID FreeViewStatePropertyBagCache(VOID)
{
    ::EnterCriticalSection(&g_csBagCacheLock);
    g_pCachedBag.Release();
    ::LeaveCriticalSection(&g_csBagCacheLock);
}

/**************************************************************************
 *  SHGetPerScreenResName (SHLWAPI.533)
 *
 * @see https://www.geoffchappell.com/studies/windows/shell/shlwapi/api/propbag/getperscreenresname.htm
 */
EXTERN_C INT WINAPI
SHGetPerScreenResName(
    _Out_writes_(cchBuffer) LPWSTR pszBuffer,
    _In_ INT cchBuffer,
    _In_ DWORD dwReserved)
{
    if (dwReserved)
        return 0;

    HDC hDC = ::GetDC(NULL);
    INT cxWidth = ::GetDeviceCaps(hDC, HORZRES);
    INT cyHeight = ::GetDeviceCaps(hDC, VERTRES);
    INT cMonitors = ::GetSystemMetrics(SM_CMONITORS);
    ::ReleaseDC(NULL, hDC);

    StringCchPrintfW(pszBuffer, cchBuffer, L"%dx%d(%d)", cxWidth, cyHeight, cMonitors);
    return lstrlenW(pszBuffer);
}

/**************************************************************************
 *  IUnknown_QueryServicePropertyBag (SHLWAPI.536)
 *
 * @param punk      An IUnknown interface.
 * @param flags     The SHGVSPB_... flags of SHGetViewStatePropertyBag.
 * @param riid      IID of requested property bag interface.
 * @param ppvObj    Address to receive pointer to the new interface.
 * @return          An HRESULT value. S_OK on success, non-zero on failure.
 * @see https://geoffchappell.com/studies/windows/shell/shlwapi/api/util/iunknown/queryservicepropertybag.htm
 */
EXTERN_C HRESULT WINAPI
IUnknown_QueryServicePropertyBag(
    _In_ IUnknown *punk,
    _In_ long flags,
    _In_ REFIID riid,
    _Outptr_ void **ppvObj)
{
    TRACE("%p 0x%x %p %p\n", punk, flags, &riid, ppvObj);

    CComPtr<IShellBrowserService> pService;
    HRESULT hr = IUnknown_QueryService(punk, SID_STopLevelBrowser, IID_IShellBrowserService,
                                       (void **)&pService);
    if (FAILED(hr))
    {
        ERR("0x%X\n", hr);
        return hr;
    }

    return pService->GetPropertyBag(flags, riid, ppvObj);
}
