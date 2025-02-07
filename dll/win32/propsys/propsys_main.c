/*
 * propsys main
 *
 * Copyright 1997, 2002 Alexandre Julliard
 * Copyright 2008 James Hawkins
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "propsys.h"
#include "wine/debug.h"

#include "propsys_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);

    return S_OK;
}

static HRESULT WINAPI InMemoryPropertyStoreFactory_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    return PropertyStore_CreateInstance(outer, riid, ppv);
}

static const IClassFactoryVtbl InMemoryPropertyStoreFactoryVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InMemoryPropertyStoreFactory_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory InMemoryPropertyStoreFactory = { &InMemoryPropertyStoreFactoryVtbl };

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    if(IsEqualGUID(&CLSID_InMemoryPropertyStore, rclsid)) {
        TRACE("(CLSID_InMemoryPropertyStore %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&InMemoryPropertyStoreFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

static HRESULT WINAPI propsys_QueryInterface(IPropertySystem *iface, REFIID riid, void **obj)
{
    *obj = NULL;

    if (IsEqualIID(riid, &IID_IPropertySystem) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IPropertySystem_AddRef(iface);
        return S_OK;
    }

    FIXME("%s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI propsys_AddRef(IPropertySystem *iface)
{
    return 2;
}

static ULONG WINAPI propsys_Release(IPropertySystem *iface)
{
    return 1;
}

static HRESULT WINAPI propsys_GetPropertyDescription(IPropertySystem *iface,
    REFPROPERTYKEY propkey, REFIID riid, void **ppv)
{
    return PSGetPropertyDescription(propkey, riid, ppv);
}

static HRESULT WINAPI propsys_GetPropertyDescriptionByName(IPropertySystem *iface,
    LPCWSTR canonical_name, REFIID riid, void **ppv)
{
    FIXME("%s %s %p: stub\n", debugstr_w(canonical_name), debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI propsys_GetPropertyDescriptionListFromString(IPropertySystem *iface,
    LPCWSTR proplist, REFIID riid, void **ppv)
{
    return PSGetPropertyDescriptionListFromString(proplist, riid, ppv);
}

static HRESULT WINAPI propsys_EnumeratePropertyDescriptions(IPropertySystem *iface,
    PROPDESC_ENUMFILTER filter, REFIID riid, void **ppv)
{
    FIXME("%d %s %p: stub\n", filter, debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

static HRESULT WINAPI propsys_FormatForDisplay(IPropertySystem *iface,
    REFPROPERTYKEY key, REFPROPVARIANT propvar, PROPDESC_FORMAT_FLAGS flags,
    LPWSTR dest, DWORD destlen)
{
    FIXME("%p %p %x %p %ld: stub\n", key, propvar, flags, dest, destlen);
    return E_NOTIMPL;
}

static HRESULT WINAPI propsys_FormatForDisplayAlloc(IPropertySystem *iface,
    REFPROPERTYKEY key, REFPROPVARIANT propvar, PROPDESC_FORMAT_FLAGS flags,
    LPWSTR *text)
{
    FIXME("%p %p %x %p: stub\n", key, propvar, flags, text);
    return E_NOTIMPL;
}

static HRESULT WINAPI propsys_RegisterPropertySchema(IPropertySystem *iface, LPCWSTR path)
{
    return PSRegisterPropertySchema(path);
}

static HRESULT WINAPI propsys_UnregisterPropertySchema(IPropertySystem *iface, LPCWSTR path)
{
    return PSUnregisterPropertySchema(path);
}

static HRESULT WINAPI propsys_RefreshPropertySchema(IPropertySystem *iface)
{
    return PSRefreshPropertySchema();
}

static const IPropertySystemVtbl propsysvtbl = {
    propsys_QueryInterface,
    propsys_AddRef,
    propsys_Release,
    propsys_GetPropertyDescription,
    propsys_GetPropertyDescriptionByName,
    propsys_GetPropertyDescriptionListFromString,
    propsys_EnumeratePropertyDescriptions,
    propsys_FormatForDisplay,
    propsys_FormatForDisplayAlloc,
    propsys_RegisterPropertySchema,
    propsys_UnregisterPropertySchema,
    propsys_RefreshPropertySchema
};

static IPropertySystem propsys = { &propsysvtbl };

HRESULT WINAPI PSGetPropertySystem(REFIID riid, void **obj)
{
    return IPropertySystem_QueryInterface(&propsys, riid, obj);
}

HRESULT WINAPI PSRegisterPropertySchema(PCWSTR path)
{
    FIXME("%s stub\n", debugstr_w(path));

    return S_OK;
}

HRESULT WINAPI PSUnregisterPropertySchema(PCWSTR path)
{
    FIXME("%s stub\n", debugstr_w(path));

    return E_NOTIMPL;
}

HRESULT WINAPI PSGetPropertyDescription(REFPROPERTYKEY propkey, REFIID riid, void **ppv)
{
    FIXME("%p, %p, %p\n", propkey, riid, ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI PSGetPropertyDescriptionListFromString(LPCWSTR proplist, REFIID riid, void **ppv)
{
    FIXME("%s, %p, %p\n", debugstr_w(proplist), riid, ppv);
    return E_NOTIMPL;
}

HRESULT WINAPI PSGetPropertyKeyFromName(PCWSTR name, PROPERTYKEY *key)
{
    FIXME("%s, %p\n", debugstr_w(name), key);
    return E_NOTIMPL;
}

HRESULT WINAPI PSRefreshPropertySchema(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI PSStringFromPropertyKey(REFPROPERTYKEY pkey, LPWSTR psz, UINT cch)
{
    WCHAR pidW[PKEY_PIDSTR_MAX + 1];
    LPWSTR p = psz;
    int len;

    TRACE("(%p, %p, %u)\n", pkey, psz, cch);

    if (!psz)
        return E_POINTER;

    /* GUIDSTRING_MAX accounts for null terminator, +1 for space character. */
    if (cch <= GUIDSTRING_MAX + 1)
        return E_NOT_SUFFICIENT_BUFFER;

    if (!pkey)
    {
        psz[0] = '\0';
        return E_NOT_SUFFICIENT_BUFFER;
    }

    swprintf(psz, cch, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", pkey->fmtid.Data1,
             pkey->fmtid.Data2, pkey->fmtid.Data3, pkey->fmtid.Data4[0], pkey->fmtid.Data4[1],
             pkey->fmtid.Data4[2], pkey->fmtid.Data4[3], pkey->fmtid.Data4[4],
             pkey->fmtid.Data4[5], pkey->fmtid.Data4[6], pkey->fmtid.Data4[7]);

    /* Overwrite the null terminator with the space character. */
    p += GUIDSTRING_MAX - 1;
    *p++ = ' ';
    cch -= GUIDSTRING_MAX - 1 + 1;

    len = swprintf(pidW, ARRAY_SIZE(pidW), L"%u", pkey->pid);

    if (cch >= len + 1)
    {
        lstrcpyW(p, pidW);
        return S_OK;
    }
    else
    {
        WCHAR *ptr = pidW + len - 1;

        psz[0] = '\0';
        *p++ = '\0';
        cch--;

        /* Replicate a quirk of the native implementation where the contents
         * of the property ID string are written backwards to the output
         * buffer, skipping the rightmost digit. */
        if (cch)
        {
            ptr--;
            while (cch--)
                *p++ = *ptr--;
        }

        return E_NOT_SUFFICIENT_BUFFER;
    }
}

static const BYTE hex2bin[] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x00 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x10 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x20 */
    0,1,2,3,4,5,6,7,8,9,0,0,0,0,0,0,        /* 0x30 */
    0,10,11,12,13,14,15,0,0,0,0,0,0,0,0,0,  /* 0x40 */
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,        /* 0x50 */
    0,10,11,12,13,14,15                     /* 0x60 */
};

static BOOL validate_indices(LPCWSTR s, int min, int max)
{
    int i;

    for (i = min; i <= max; i++)
    {
        if (!s[i])
            return FALSE;

        if (i == 0)
        {
            if (s[i] != '{')
                return FALSE;
        }
        else if (i == 9 || i == 14 || i == 19 || i == 24)
        {
            if (s[i] != '-')
                return FALSE;
        }
        else if (i == 37)
        {
            if (s[i] != '}')
                return FALSE;
        }
        else
        {
            if (s[i] > 'f' || (!hex2bin[s[i]] && s[i] != '0'))
                return FALSE;
        }
    }

    return TRUE;
}

/* Adapted from CLSIDFromString helper in dlls/ole32/compobj.c and
 * UuidFromString in dlls/rpcrt4/rpcrt4_main.c. */
static BOOL string_to_guid(LPCWSTR s, LPGUID id)
{
    /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

    if (!validate_indices(s, 0, 8)) return FALSE;
    id->Data1 = (hex2bin[s[1]] << 28 | hex2bin[s[2]] << 24 | hex2bin[s[3]] << 20 | hex2bin[s[4]] << 16 |
                 hex2bin[s[5]] << 12 | hex2bin[s[6]] << 8  | hex2bin[s[7]] << 4  | hex2bin[s[8]]);
    if (!validate_indices(s, 9, 14)) return FALSE;
    id->Data2 = hex2bin[s[10]] << 12 | hex2bin[s[11]] << 8 | hex2bin[s[12]] << 4 | hex2bin[s[13]];
    if (!validate_indices(s, 15, 19)) return FALSE;
    id->Data3 = hex2bin[s[15]] << 12 | hex2bin[s[16]] << 8 | hex2bin[s[17]] << 4 | hex2bin[s[18]];

    /* these are just sequential bytes */

    if (!validate_indices(s, 20, 21)) return FALSE;
    id->Data4[0] = hex2bin[s[20]] << 4 | hex2bin[s[21]];
    if (!validate_indices(s, 22, 24)) return FALSE;
    id->Data4[1] = hex2bin[s[22]] << 4 | hex2bin[s[23]];

    if (!validate_indices(s, 25, 26)) return FALSE;
    id->Data4[2] = hex2bin[s[25]] << 4 | hex2bin[s[26]];
    if (!validate_indices(s, 27, 28)) return FALSE;
    id->Data4[3] = hex2bin[s[27]] << 4 | hex2bin[s[28]];
    if (!validate_indices(s, 29, 30)) return FALSE;
    id->Data4[4] = hex2bin[s[29]] << 4 | hex2bin[s[30]];
    if (!validate_indices(s, 31, 32)) return FALSE;
    id->Data4[5] = hex2bin[s[31]] << 4 | hex2bin[s[32]];
    if (!validate_indices(s, 33, 34)) return FALSE;
    id->Data4[6] = hex2bin[s[33]] << 4 | hex2bin[s[34]];
    if (!validate_indices(s, 35, 37)) return FALSE;
    id->Data4[7] = hex2bin[s[35]] << 4 | hex2bin[s[36]];

    return TRUE;
}

HRESULT WINAPI PSPropertyKeyFromString(LPCWSTR pszString, PROPERTYKEY *pkey)
{
    BOOL has_minus = FALSE, has_comma = FALSE;

    TRACE("(%s, %p)\n", debugstr_w(pszString), pkey);

    if (!pszString || !pkey)
        return E_POINTER;

    memset(pkey, 0, sizeof(PROPERTYKEY));

    if (!string_to_guid(pszString, &pkey->fmtid))
        return E_INVALIDARG;

    pszString += GUIDSTRING_MAX - 1;

    if (!*pszString)
        return E_INVALIDARG;

    /* Only the space seems to be recognized as whitespace. The comma is only
     * recognized once and processing terminates if another comma is found. */
    while (*pszString == ' ' || *pszString == ',')
    {
        if (*pszString == ',')
        {
            if (has_comma)
                return S_OK;
            else
                has_comma = TRUE;
        }
        pszString++;
    }

    if (!*pszString)
        return E_INVALIDARG;

    /* Only two minus signs are recognized if no comma is detected. The first
     * sign is ignored, and the second is interpreted. If a comma is detected
     * before the minus sign, then only one minus sign counts, and property ID
     * interpretation begins with the next character. */
    if (has_comma)
    {
        if (*pszString == '-')
        {
            has_minus = TRUE;
            pszString++;
        }
    }
    else
    {
        if (*pszString == '-')
            pszString++;

        /* Skip any intermediate spaces after the first minus sign. */
        while (*pszString == ' ')
            pszString++;

        if (*pszString == '-')
        {
            has_minus = TRUE;
            pszString++;
        }

        /* Skip any remaining spaces after minus sign. */
        while (*pszString == ' ')
            pszString++;
    }

    /* Overflow is not checked. */
    while ('0' <= *pszString && *pszString <= '9')
    {
        pkey->pid *= 10;
        pkey->pid += (*pszString - '0');
        pszString++;
    }

    if (has_minus)
        pkey->pid = ~pkey->pid + 1;

    return S_OK;
}

HRESULT WINAPI PSCreateMemoryPropertyStore(REFIID riid, void **ppv)
{
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    return PropertyStore_CreateInstance(NULL, riid, ppv);
}

#ifdef __REACTOS__
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}
#endif
