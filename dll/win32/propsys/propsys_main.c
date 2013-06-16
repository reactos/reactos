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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#include <config.h>

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <rpcproxy.h>
#include <propsys.h>
#include <wine/debug.h>
#include <wine/unicode.h>

#include "propsys_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(propsys);

static HINSTANCE propsys_hInstance;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(0x%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
        case DLL_WINE_PREATTACH:
            return FALSE;    /* prefer native version */
        case DLL_PROCESS_ATTACH:
            propsys_hInstance = hinstDLL;
            DisableThreadLibraryCalls(hinstDLL);
            break;
        case DLL_PROCESS_DETACH:
            break;
        default:
            break;
    }

    return TRUE;
}

HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( propsys_hInstance );
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( propsys_hInstance );
}

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

HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
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

HRESULT WINAPI PSRefreshPropertySchema(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI PSStringFromPropertyKey(REFPROPERTYKEY pkey, LPWSTR psz, UINT cch)
{
    static const WCHAR guid_fmtW[] = {'{','%','0','8','X','-','%','0','4','X','-',
                                      '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
                                      '%','0','2','X','%','0','2','X','%','0','2','X',
                                      '%','0','2','X','%','0','2','X','%','0','2','X','}',0};
    static const WCHAR pid_fmtW[] = {'%','u',0};

    WCHAR pidW[PKEY_PIDSTR_MAX + 1];
    LPWSTR p = psz;
    int len;

    TRACE("(%p, %p, %u)\n", pkey, psz, cch);

    if (!psz)
        return E_POINTER;

    /* GUIDSTRING_MAX accounts for null terminator, +1 for space character. */
    if (cch <= GUIDSTRING_MAX + 1)
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    if (!pkey)
    {
        psz[0] = '\0';
        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    sprintfW(psz, guid_fmtW, pkey->fmtid.Data1, pkey->fmtid.Data2,
             pkey->fmtid.Data3, pkey->fmtid.Data4[0], pkey->fmtid.Data4[1],
             pkey->fmtid.Data4[2], pkey->fmtid.Data4[3], pkey->fmtid.Data4[4],
             pkey->fmtid.Data4[5], pkey->fmtid.Data4[6], pkey->fmtid.Data4[7]);

    /* Overwrite the null terminator with the space character. */
    p += GUIDSTRING_MAX - 1;
    *p++ = ' ';
    cch -= GUIDSTRING_MAX - 1 + 1;

    len = sprintfW(pidW, pid_fmtW, pkey->pid);

    if (cch >= len + 1)
    {
        strcpyW(p, pidW);
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

        return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
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
    int has_minus = 0, has_comma = 0;

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
                has_comma = 1;
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
            has_minus = 1;
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
            has_minus = 1;
            pszString++;
        }

        /* Skip any remaining spaces after minus sign. */
        while (*pszString == ' ')
            pszString++;
    }

    /* Overflow is not checked. */
    while (isdigitW(*pszString))
    {
        pkey->pid *= 10;
        pkey->pid += (*pszString - '0');
        pszString++;
    }

    if (has_minus)
        pkey->pid = ~pkey->pid + 1;

    return S_OK;
}
