/*
 * PROJECT:     shell32
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     [PROP]VARIANT functions
 * COPYRIGHT:   Copyright 2026 Whindmar Saksit <whindsaks@proton.me>
 */

#define SHELL32_PROP_IMPL
#include "precomp.h"
#include "prop.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell_prop);

static const struct SHELL32PKEYMAP
{
    PCWSTR pszLegacyName;
    const PROPERTYKEY *pkey;
} g_PkMap[] =
{
// learn.microsoft.com/en-us/windows/win32/api/propsys/nf-propsys-psgetpropertydescriptionbyname
#define MKSH32PKEYMAP(sysname, legacyname, pkey)  { (L##legacyname), &(pkey) },
MKSH32PKEYMAP(?,                                "Name",                PKEYSHELL32_Name)
MKSH32PKEYMAP("System.ItemTypeText",            "Type",                PKEY_ItemTypeText)
MKSH32PKEYMAP("System.Size",                    "Size",                PKEY_Size)
MKSH32PKEYMAP("System.FileAttributes",          "Attributes",          PKEY_FileAttributes)
MKSH32PKEYMAP("System.DateModified",            "Write",               PKEY_DateModified)
MKSH32PKEYMAP("System.Comment",                 "DocComments",         PKEYSHELL32_Comments)
};

static const PROPERTYKEY*
SHELL_GetPropertyKeyFromLegacyName(_In_ PCWSTR pszString)
{
    for (SIZE_T i = 0; i < _countof(g_PkMap); ++i)
    {
        if (!_wcsicmp(g_PkMap[i].pszLegacyName, pszString))
            return g_PkMap[i].pkey;
    }
    return NULL;
}

#if /*TODO*/0 && (NTDDI_VERSION >= NTDDI_VISTA)
#define SHELL_PropertyKeyFromPropertyKeyString PSPropertyKeyFromString
#else
static HRESULT
SHELL_PropertyKeyFromPropertyKeyString(_In_ PCWSTR pszString, _Out_ PROPERTYKEY *pkey)
{
    if (GUIDFromStringW(pszString, &pkey->fmtid))
    {
        for (pszString += 38; *pszString == ',' || *pszString == ' ';)
            ++pszString;
        pkey->pid = StrToInt(pszString);
        if (pkey->pid)
            return S_OK;
    }
    return E_INVALIDARG;
}
#endif

const PROPERTYKEY*
SHELL_GetPropertyKeyFromString(_In_ PCWSTR pszString, _Out_ PROPERTYKEY *pkey)
{
    if (SUCCEEDED(SHELL_PropertyKeyFromPropertyKeyString(pszString, pkey)))
        return pkey;
    return SHELL_GetPropertyKeyFromLegacyName(pszString);
}

HRESULT
VariantToIdlist(_In_ VARIANT *pV, _Out_ LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_FAIL;
    if (V_VT(pV) == VT_I4)
    {
        hr = SHGetSpecialFolderLocation(NULL, V_I4(pV), ppidl);
    }
    else if (V_VT(pV) == VT_BSTR)
    {
        hr = SHILCreateFromPathW(V_BSTR(pV), ppidl, NULL);
    }
    return hr;
}

HRESULT
VariantQueryInterface(_In_ VARIANT *pV, _In_ REFIID riid, _Out_ void **ppv)
{
    pV = VariantDerefVariant(pV);
    switch (V_VT(pV))
    {
        case VT_DISPATCH | VT_BYREF:
            return V_DISPATCHREF(pV) && *V_DISPATCHREF(pV) ? (*V_DISPATCHREF(pV))->QueryInterface(riid, ppv) : E_UNEXPECTED;
        case VT_DISPATCH:
        case VT_UNKNOWN:
            return V_UNKNOWN(pV) ? V_UNKNOWN(pV)->QueryInterface(riid, ppv) : E_UNEXPECTED;
    }
    return E_FAIL;
}
