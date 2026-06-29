/*
 * PROJECT:     ReactOS CTF
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     ITfCategoryMgr implementation
 * COPYRIGHT:   Copyright 2009 Aric Stewart, CodeWeavers
 *              Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(msctf);

////////////////////////////////////////////////////////////////////////////

class CCategoryMgr
    : public ITfCategoryMgr
{
public:
    CCategoryMgr();
    virtual ~CCategoryMgr();

    // ** IUnknown methods **
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // ** ITfCategoryMgr methods **
    STDMETHODIMP RegisterCategory(
        _In_ REFCLSID rclsid,
        _In_ REFGUID rcatid,
        _In_ REFGUID rguid) override;
    STDMETHODIMP UnregisterCategory(
        _In_ REFCLSID rclsid,
        _In_ REFGUID rcatid,
        _In_ REFGUID rguid) override;
    STDMETHODIMP EnumCategoriesInItem(
        _In_ REFGUID rguid,
        _Out_ IEnumGUID **ppEnum) override;
    STDMETHODIMP EnumItemsInCategory(
        _In_ REFGUID rcatid,
        _Out_ IEnumGUID **ppEnum) override;
    STDMETHODIMP FindClosestCategory(
        _In_ REFGUID rguid,
        _Out_ GUID *pcatid,
        _In_ const GUID **ppcatidList,
        _In_ ULONG ulCount) override;
    STDMETHODIMP RegisterGUIDDescription(
        _In_ REFCLSID rclsid,
        _In_ REFGUID rguid,
        _In_ const WCHAR *pchDesc,
        _In_ ULONG cch) override;
    STDMETHODIMP UnregisterGUIDDescription(
        _In_ REFCLSID rclsid,
        _In_ REFGUID rguid) override;
    STDMETHODIMP GetGUIDDescription(
        _In_ REFGUID rguid,
        _Out_ BSTR *pbstrDesc) override;
    STDMETHODIMP RegisterGUIDDWORD(
        _In_ REFCLSID rclsid,
        _In_ REFGUID rguid,
        _In_ DWORD dw) override;
    STDMETHODIMP UnregisterGUIDDWORD(
        _In_ REFCLSID rclsid,
        _In_ REFGUID rguid) override;
    STDMETHODIMP GetGUIDDWORD(
        _In_ REFGUID rguid,
        _Out_ DWORD *pdw) override;
    STDMETHODIMP RegisterGUID(
        _In_ REFGUID rguid,
        _Out_ TfGuidAtom *pguidatom) override;
    STDMETHODIMP GetGUID(
        _In_ TfGuidAtom guidatom,
        _Out_ GUID *pguid) override;
    STDMETHODIMP IsEqualTfGuidAtom(
        _In_ TfGuidAtom guidatom,
        _In_ REFGUID rguid,
        _Out_ BOOL *pfEqual) override;

protected:
    LONG m_cRefs;
};

////////////////////////////////////////////////////////////////////////////

CCategoryMgr::CCategoryMgr()
    : m_cRefs(1)
{
}

CCategoryMgr::~CCategoryMgr()
{
    TRACE("destroying %p\n", this);
}

STDMETHODIMP CCategoryMgr::QueryInterface(REFIID riid, void **ppvObj)
{
    if (!ppvObj)
        return E_INVALIDARG;

    *ppvObj = NULL;

    if (riid == IID_IUnknown || riid == IID_ITfCategoryMgr)
        *ppvObj = this;

    if (*ppvObj)
    {
        AddRef();
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(&riid));
    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) CCategoryMgr::AddRef()
{
    return ::InterlockedIncrement(&m_cRefs);
}

STDMETHODIMP_(ULONG) CCategoryMgr::Release()
{
    ULONG ret = ::InterlockedDecrement(&m_cRefs);
    if (!ret)
        delete this;
    return ret;
}

STDMETHODIMP CCategoryMgr::RegisterCategory(
    _In_ REFCLSID rclsid,
    _In_ REFGUID rcatid,
    _In_ REFGUID rguid)
{
    WCHAR szFullKey[110], szClsid[39], szCatid[39], szGuid[39];
    HKEY hTipKey = NULL, hCatKey = NULL, hItemKey = NULL;
    LSTATUS error;
    HRESULT hr = E_FAIL;

    TRACE("%p -> (%s, %s, %s)\n", this, debugstr_guid(&rclsid), debugstr_guid(&rcatid),
          debugstr_guid(&rguid));

    StringFromGUID2(rclsid, szClsid, _countof(szClsid));
    StringFromGUID2(rcatid, szCatid, _countof(szCatid));
    StringFromGUID2(rguid, szGuid, _countof(szGuid));

    StringCchPrintfW(szFullKey, _countof(szFullKey), L"%s\\%s", szwSystemTIPKey, szClsid);
    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ | KEY_WRITE, &hTipKey);
    if (error != ERROR_SUCCESS)
        return E_FAIL;

    StringCchPrintfW(szFullKey, _countof(szFullKey), L"Category\\Category\\%s\\%s", szCatid, szGuid);
    error = RegCreateKeyExW(hTipKey, szFullKey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                            &hCatKey, NULL);
    if (error == ERROR_SUCCESS)
    {
        RegCloseKey(hCatKey);

        StringCchPrintfW(szFullKey, _countof(szFullKey), L"Category\\Item\\%s\\%s", szGuid, szCatid);
        error = RegCreateKeyExW(hTipKey, szFullKey, 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                                &hItemKey, NULL);
        if (error == ERROR_SUCCESS)
        {
            RegCloseKey(hItemKey);
            hr = S_OK;
        }
    }

    RegCloseKey(hTipKey);
    return hr;
}

STDMETHODIMP CCategoryMgr::UnregisterCategory(
    _In_ REFCLSID rclsid,
    _In_ REFGUID rcatid,
    _In_ REFGUID rguid)
{
    WCHAR szFullKey[110], szClsid[39], szCatid[39], szGuid[39];
    HKEY hTipKey = NULL;
    LSTATUS error;

    TRACE("%p -> (%s %s %s)\n", this, debugstr_guid(&rclsid), debugstr_guid(&rcatid),
          debugstr_guid(&rguid));

    StringFromGUID2(rclsid, szClsid, _countof(szClsid));
    StringFromGUID2(rcatid, szCatid, _countof(szCatid));
    StringFromGUID2(rguid, szGuid, _countof(szGuid));

    StringCchPrintfW(szFullKey, _countof(szFullKey), L"%s\\%s", szwSystemTIPKey, szClsid);
    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ | KEY_WRITE, &hTipKey);
    if (error != ERROR_SUCCESS)
        return E_FAIL;

    StringCchPrintfW(szFullKey, _countof(szFullKey), L"Category\\Category\\%s\\%s", szCatid, szGuid);
    RegDeleteTreeW(hTipKey, szFullKey);

    StringCchPrintfW(szFullKey, _countof(szFullKey), L"Category\\Item\\%s\\%s", szGuid, szCatid);
    RegDeleteTreeW(hTipKey, szFullKey);

    RegCloseKey(hTipKey);
    return S_OK;
}

STDMETHODIMP CCategoryMgr::EnumCategoriesInItem(
    _In_ REFGUID rguid,
    _Out_ IEnumGUID **ppEnum)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::EnumItemsInCategory(
    _In_ REFGUID rcatid,
    _Out_ IEnumGUID **ppEnum)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::FindClosestCategory(
    _In_ REFGUID rguid,
    _Out_ GUID *pcatid,
    _In_ const GUID **ppcatidList,
    _In_ ULONG ulCount)
{
    WCHAR szFullKey[120], szGuid[39];
    HKEY hKey = NULL;
    HRESULT hr = S_FALSE;
    DWORD dwIndex = 0;
    LSTATUS error;

    TRACE("(%p)\n", this);

    if (!pcatid || (ulCount && !ppcatidList))
        return E_INVALIDARG;

    StringFromGUID2(rguid, szGuid, _countof(szGuid));
    StringCchPrintfW(szFullKey, _countof(szFullKey), L"%s\\%s\\Category\\Item\\%s", szwSystemTIPKey, szGuid, szGuid);
    *pcatid = GUID_NULL;

    error = RegOpenKeyExW(HKEY_LOCAL_MACHINE, szFullKey, 0, KEY_READ, &hKey);
    if (error != ERROR_SUCCESS)
        return S_FALSE;

    // Enumerate subkeys to find the closest category
    while (TRUE)
    {
        WCHAR szCatidName[39];
        DWORD cchCatidName = _countof(szCatidName);
        GUID currentCatid;

        error = RegEnumKeyExW(hKey, dwIndex, szCatidName, &cchCatidName, NULL, NULL, NULL, NULL);
        if (error != ERROR_SUCCESS || error == ERROR_NO_MORE_ITEMS)
            break;

        dwIndex++;

        HRESULT hr2 = CLSIDFromString(szCatidName, &currentCatid);
        if (FAILED(hr2))
            continue; // Skip invalid GUID strings

        if (ulCount <= 0)
        {
            *pcatid = currentCatid;
            hr = S_OK; // Found a category
            break;
        }

        // If a list of categories is provided, check if the current one is in the list
        BOOL bFound = FALSE;
        for (ULONG j = 0; j < ulCount; j++)
        {
            if (currentCatid == *ppcatidList[j])
            {
                bFound = TRUE;
                *pcatid = currentCatid;
                hr = S_OK; // Found a matching category
                break;
            }
        }
        if (bFound)
            break; // Found and matched, so stop searching
    }

    RegCloseKey(hKey);
    return hr;
}

STDMETHODIMP CCategoryMgr::RegisterGUIDDescription(
    _In_ REFCLSID rclsid,
    _In_ REFGUID rguid,
    _In_ const WCHAR *pchDesc,
    _In_ ULONG cch)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::UnregisterGUIDDescription(
    _In_ REFCLSID rclsid,
    _In_ REFGUID rguid)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::GetGUIDDescription(
    _In_ REFGUID rguid,
    _Out_ BSTR *pbstrDesc)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::RegisterGUIDDWORD(
    _In_ REFCLSID rclsid,
    _In_ REFGUID rguid,
    _In_ DWORD dw)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::UnregisterGUIDDWORD(
    _In_ REFCLSID rclsid,
    _In_ REFGUID rguid)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::GetGUIDDWORD(
    _In_ REFGUID rguid,
    _Out_ DWORD *pdw)
{
    FIXME("STUB:(%p)\n", this);
    return E_NOTIMPL;
}

STDMETHODIMP CCategoryMgr::RegisterGUID(
    _In_ REFGUID rguid,
    _Out_ TfGuidAtom *pguidatom)
{
    TRACE("%p -> (%s, %p)\n", this, debugstr_guid(&rguid), pguidatom);

    if (!pguidatom)
        return E_INVALIDARG;

    DWORD dwCookieId = 0, dwEnumIndex = 0;
    do
    {
        dwCookieId = enumerate_Cookie(COOKIE_MAGIC_GUIDATOM, &dwEnumIndex);
        if (dwCookieId && rguid == *(const GUID *)get_Cookie_data(dwCookieId))
        {
            *pguidatom = dwCookieId;
            return S_OK;
        }
    } while (dwCookieId != 0);

    GUID *pNewGuid = (GUID *)cicMemAlloc(sizeof(GUID));
    if (!pNewGuid)
        return E_OUTOFMEMORY;

    *pNewGuid = rguid;

    dwCookieId = generate_Cookie(COOKIE_MAGIC_GUIDATOM, pNewGuid);
    if (dwCookieId == 0)
    {
        cicMemFree(pNewGuid);
        return E_FAIL;
    }

    *pguidatom = dwCookieId;
    return S_OK;
}

STDMETHODIMP CCategoryMgr::GetGUID(
    _In_ TfGuidAtom guidatom,
    _Out_ GUID *pguid)
{
    TRACE("%p -> (%d, %p)\n", this, guidatom, pguid);

    if (!pguid)
        return E_INVALIDARG;

    *pguid = GUID_NULL;

    if (get_Cookie_magic(guidatom) == COOKIE_MAGIC_GUIDATOM)
        *pguid = *(const GUID *)get_Cookie_data(guidatom);

    return S_OK;
}

STDMETHODIMP CCategoryMgr::IsEqualTfGuidAtom(
    _In_ TfGuidAtom guidatom,
    _In_ REFGUID rguid,
    _Out_ BOOL *pfEqual)
{
    TRACE("%p -> (%d %s %p)\n", this, guidatom, debugstr_guid(&rguid), pfEqual);

    if (!pfEqual)
        return E_INVALIDARG;

    *pfEqual = FALSE;
    if (get_Cookie_magic(guidatom) == COOKIE_MAGIC_GUIDATOM)
    {
        if (rguid == *(const GUID *)get_Cookie_data(guidatom))
            *pfEqual = TRUE;
    }

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////

EXTERN_C
HRESULT CategoryMgr_Constructor(IUnknown *pUnkOuter, IUnknown **ppOut)
{
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    CCategoryMgr *This = new(cicNoThrow) CCategoryMgr();
    if (!This)
        return E_OUTOFMEMORY;

    *ppOut = static_cast<ITfCategoryMgr *>(This);
    TRACE("returning %p\n", *ppOut);
    return S_OK;
}
