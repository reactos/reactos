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

static HRESULT
SHELL_VariantCopy(_Out_ VARIANT *pvd, _In_ const VARIANT *pvs)
{
    // VariantCopy does not copy VT_DATE?
    if (V_VT(pvs) <= VT_DATE || (VT_DECIMAL <= V_VT(pvs) && V_VT(pvs) <= VT_PTR))
    {
        CopyMemory(pvd, pvs, sizeof(*pvd));
        return S_OK;
    }
    return VariantCopy(pvd, pvs);
}

static HRESULT WINAPI
FallbackVariantToPropVariant(_In_ const VARIANT *pVar, _Out_ PROPVARIANT *pPropVar)
{
    if (!pVar || !pPropVar)
        return E_INVALIDARG;

    return SHELL_VariantCopy((VARIANT*)pPropVar, pVar);
}

HRESULT
SHELL_VariantToPropVariant(_In_ const VARIANT *pVar, _Out_ PROPVARIANT *pPropVar)
{
    static FARPROC func = NULL;
    if (!func)
    {
        if (HMODULE hPropSys = LoadLibraryW(L"PROPSYS"))
            func = GetProcAddress(hPropSys, "VariantToPropVariant");
        if (!func)
            func = (FARPROC)FallbackVariantToPropVariant;
    }

    HRESULT hr = ((HRESULT(WINAPI*)(const VARIANT*,void*))func)(pVar, pPropVar);
    if (FAILED(hr) && func != (FARPROC)FallbackVariantToPropVariant)
    {
        switch (V_VT(pVar))
        {
            case VT_DATE:
                return FallbackVariantToPropVariant(pVar, pPropVar);
        }
    }
    return hr;
}

static HRESULT WINAPI
FallbackPropVariantChangeType(_Out_ PROPVARIANT *ppvDest, _In_ REFPROPVARIANT pvSrc, _In_ int reserved, _In_ VARTYPE vt)
{
    if (!ppvDest || (SIZE_T)ppvDest == (SIZE_T)&pvSrc) // MSDN: The source and destination PROPVARIANT structures must be separate structures.
        return E_INVALIDARG;

    const VARTYPE finalvt = vt;
    switch (vt)
    {
        case VT_LPWSTR: vt = VT_BSTR; break;
        case VT_FILETIME: vt = VT_DATE; break;
    }

    VARIANT v;
    HRESULT hr;
    if (V_VT((VARIANT*)&pvSrc) == vt)
        hr = SHELL_VariantCopy(&v, (VARIANT*)&pvSrc);
    else
        hr = VariantChangeType(&v, (VARIANT*)&pvSrc, 0, vt);

    if (FAILED(hr))
    {
        if (pvSrc.vt == VT_UI4 && finalvt == VT_UI8)
        {
            ppvDest->vt = finalvt;
            ppvDest->uhVal.QuadPart = pvSrc.ulVal;
            return S_OK;
        }

        if (pvSrc.vt == VT_UI4 && finalvt == VT_I4 && pvSrc.ulVal <= INT_MAX)
            goto copy;

        if (pvSrc.vt == VT_UI4 && finalvt == VT_UI4)
        {
        copy:
            CopyMemory(ppvDest, &pvSrc, sizeof(*ppvDest));
            ppvDest->vt = finalvt;
            return S_OK;
        }
        return hr;
    }

    switch (finalvt)
    {
        case VT_LPWSTR:
            if (SUCCEEDED(hr = SHStrDupW(V_BSTR(&v), &ppvDest->pwszVal)))
                ppvDest->vt = finalvt;
            VariantClear(&v);
            return hr;
        case VT_FILETIME:
            if (VariantTimeToFileTime(V_DATE(&v), &ppvDest->filetime))
            {
                ppvDest->vt = finalvt;
                VariantClear(&v);
                return S_OK;
            }
            break;
    }

    hr = SHELL_VariantToPropVariant(&v, ppvDest);
    VariantClear(&v);
    return hr;
}

HRESULT
SHELL_PropVariantChangeType(_Out_ PROPVARIANT *ppvDest, _In_ REFPROPVARIANT pvSrc, _In_ VARTYPE vt)
{
    static FARPROC func = NULL;
    if (!func)
    {
        if (HMODULE hPropSys = LoadLibraryW(L"PROPSYS"))
            func = GetProcAddress(hPropSys, "PropVariantChangeType");
        if (!func)
            func = (FARPROC)FallbackPropVariantChangeType;
    }

    HRESULT hr = ((HRESULT(WINAPI*)(PROPVARIANT*,REFPROPVARIANT,int,VARTYPE))func)(ppvDest, pvSrc, 0, vt);
    if (FAILED(hr) && func != (FARPROC)FallbackPropVariantChangeType)
    {
        switch (vt)
        {
            case VT_FILETIME:
                return FallbackPropVariantChangeType(ppvDest, pvSrc, 0, vt);
        }
    }
    return hr;
}
