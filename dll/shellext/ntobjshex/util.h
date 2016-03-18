#pragma once

#ifndef INLINE
#define INLINE inline
#endif

HRESULT INLINE MakeStrRetFromString(LPCWSTR string, DWORD cbLength, STRRET * str)
{
    str->uType = STRRET_WSTR;

    DWORD blen = cbLength + sizeof(WCHAR);
    str->pOleStr = (LPWSTR) CoTaskMemAlloc(blen);
    return StringCbCopyNW(str->pOleStr, blen, string, cbLength);
}

HRESULT INLINE MakeStrRetFromString(LPCWSTR string, STRRET * str)
{
    DWORD stringLength = wcslen(string) * sizeof(WCHAR);
    return MakeStrRetFromString(string, stringLength, str);
}

HRESULT INLINE MakeVariantString(VARIANT * pv, PCWSTR string)
{
    V_VT(pv) = VT_BSTR;
    V_BSTR(pv) = SysAllocString(string);
    return S_OK;
}

HRESULT INLINE GetFullName(PCIDLIST_ABSOLUTE pidl, DWORD uFlags, PWSTR strName, DWORD cchName)
{
    CComPtr<IShellFolder> psfDesktop;
    STRRET str;
    HRESULT hr;

    hr = SHGetDesktopFolder(&psfDesktop);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    hr = psfDesktop->GetDisplayNameOf(pidl, uFlags, &str);
    if (FAILED_UNEXPECTEDLY(hr))
        return hr;

    return StrRetToBufW(&str, pidl, strName, cchName);
}
