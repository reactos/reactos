/*
 * IAssemblyName implementation
 *
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

#include <stdarg.h>

#define COBJMACROS
#define INITGUID

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "guiddef.h"
#include "fusion.h"
#include "corerror.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "fusionpriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(fusion);

typedef struct {
    const IAssemblyNameVtbl *lpIAssemblyNameVtbl;

    LPWSTR displayname;
    LPWSTR name;
    LPWSTR culture;

    WORD version[4];
    DWORD versize;

    BYTE pubkey[8];
    BOOL haspubkey;

    LONG ref;
} IAssemblyNameImpl;

static HRESULT WINAPI IAssemblyNameImpl_QueryInterface(IAssemblyName *iface,
                                                       REFIID riid, LPVOID *ppobj)
{
    IAssemblyNameImpl *This = (IAssemblyNameImpl *)iface;

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyName))
    {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyNameImpl_AddRef(IAssemblyName *iface)
{
    IAssemblyNameImpl *This = (IAssemblyNameImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyNameImpl_Release(IAssemblyName *iface)
{
    IAssemblyNameImpl *This = (IAssemblyNameImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
    {
        HeapFree(GetProcessHeap(), 0, This->displayname);
        HeapFree(GetProcessHeap(), 0, This->name);
        HeapFree(GetProcessHeap(), 0, This->culture);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return refCount;
}

static HRESULT WINAPI IAssemblyNameImpl_SetProperty(IAssemblyName *iface,
                                                    DWORD PropertyId,
                                                    LPVOID pvProperty,
                                                    DWORD cbProperty)
{
    FIXME("(%p, %d, %p, %d) stub!\n", iface, PropertyId, pvProperty, cbProperty);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_GetProperty(IAssemblyName *iface,
                                                    DWORD PropertyId,
                                                    LPVOID pvProperty,
                                                    LPDWORD pcbProperty)
{
    IAssemblyNameImpl *name = (IAssemblyNameImpl *)iface;

    TRACE("(%p, %d, %p, %p)\n", iface, PropertyId, pvProperty, pcbProperty);

    *((LPWSTR)pvProperty) = '\0';

    switch (PropertyId)
    {
        case ASM_NAME_NULL_PUBLIC_KEY:
        case ASM_NAME_NULL_PUBLIC_KEY_TOKEN:
            if (name->haspubkey)
                return S_OK;
            return S_FALSE;

        case ASM_NAME_NULL_CUSTOM:
            return S_OK;

        case ASM_NAME_NAME:
            *pcbProperty = 0;
            if (name->name)
            {
                lstrcpyW(pvProperty, name->name);
                *pcbProperty = (lstrlenW(name->name) + 1) * 2;
            }
            break;

        case ASM_NAME_MAJOR_VERSION:
            *pcbProperty = 0;
            *((WORD *)pvProperty) = name->version[0];
            if (name->versize >= 1)
                *pcbProperty = sizeof(WORD);
            break;

        case ASM_NAME_MINOR_VERSION:
            *pcbProperty = 0;
            *((WORD *)pvProperty) = name->version[1];
            if (name->versize >= 2)
                *pcbProperty = sizeof(WORD);
            break;

        case ASM_NAME_BUILD_NUMBER:
            *pcbProperty = 0;
            *((WORD *)pvProperty) = name->version[2];
            if (name->versize >= 3)
                *pcbProperty = sizeof(WORD);
            break;

        case ASM_NAME_REVISION_NUMBER:
            *pcbProperty = 0;
            *((WORD *)pvProperty) = name->version[3];
            if (name->versize >= 4)
                *pcbProperty = sizeof(WORD);
            break;

        case ASM_NAME_CULTURE:
            *pcbProperty = 0;
            if (name->culture)
            {
                lstrcpyW(pvProperty, name->culture);
                *pcbProperty = (lstrlenW(name->culture) + 1) * 2;
            }
            break;

        case ASM_NAME_PUBLIC_KEY_TOKEN:
            *pcbProperty = 0;
            if (name->haspubkey)
            {
                memcpy(pvProperty, name->pubkey, sizeof(DWORD) * 2);
                *pcbProperty = sizeof(DWORD) * 2;
            }
            break;

        default:
            *pcbProperty = 0;
            break;
    }

    return S_OK;
}

static HRESULT WINAPI IAssemblyNameImpl_Finalize(IAssemblyName *iface)
{
    FIXME("(%p) stub!\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_GetDisplayName(IAssemblyName *iface,
                                                       LPOLESTR szDisplayName,
                                                       LPDWORD pccDisplayName,
                                                       DWORD dwDisplayFlags)
{
    IAssemblyNameImpl *name = (IAssemblyNameImpl *)iface;

    TRACE("(%p, %p, %p, %d)\n", iface, szDisplayName,
          pccDisplayName, dwDisplayFlags);

    if (!name->displayname || !*name->displayname)
        return FUSION_E_INVALID_NAME;

    lstrcpyW(szDisplayName, name->displayname);
    *pccDisplayName = lstrlenW(szDisplayName) + 1;

    return S_OK;
}

static HRESULT WINAPI IAssemblyNameImpl_Reserved(IAssemblyName *iface,
                                                 REFIID refIID,
                                                 IUnknown *pUnkReserved1,
                                                 IUnknown *pUnkReserved2,
                                                 LPCOLESTR szReserved,
                                                 LONGLONG llReserved,
                                                 LPVOID pvReserved,
                                                 DWORD cbReserved,
                                                 LPVOID *ppReserved)
{
    TRACE("(%p, %s, %p, %p, %s, %x%08x, %p, %d, %p)\n", iface,
          debugstr_guid(refIID), pUnkReserved1, pUnkReserved2,
          debugstr_w(szReserved), (DWORD)(llReserved >> 32), (DWORD)llReserved,
          pvReserved, cbReserved, ppReserved);

    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_GetName(IAssemblyName *iface,
                                                LPDWORD lpcwBuffer,
                                                WCHAR *pwzName)
{
    IAssemblyNameImpl *name = (IAssemblyNameImpl *)iface;

    TRACE("(%p, %p, %p)\n", iface, lpcwBuffer, pwzName);

    if (!name->name)
    {
        *pwzName = '\0';
        *lpcwBuffer = 0;
        return S_OK;
    }

    lstrcpyW(pwzName, name->name);
    *lpcwBuffer = lstrlenW(pwzName) + 1;

    return S_OK;
}

static HRESULT WINAPI IAssemblyNameImpl_GetVersion(IAssemblyName *iface,
                                                   LPDWORD pdwVersionHi,
                                                   LPDWORD pdwVersionLow)
{
    IAssemblyNameImpl *name = (IAssemblyNameImpl *)iface;

    TRACE("(%p, %p, %p)\n", iface, pdwVersionHi, pdwVersionLow);

    *pdwVersionHi = 0;
    *pdwVersionLow = 0;

    if (name->versize != 4)
        return FUSION_E_INVALID_NAME;

    *pdwVersionHi = (name->version[0] << 16) + name->version[1];
    *pdwVersionLow = (name->version[2] << 16) + name->version[3];

    return S_OK;
}

static HRESULT WINAPI IAssemblyNameImpl_IsEqual(IAssemblyName *iface,
                                                IAssemblyName *pName,
                                                DWORD dwCmpFlags)
{
    FIXME("(%p, %p, %d) stub!\n", iface, pName, dwCmpFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI IAssemblyNameImpl_Clone(IAssemblyName *iface,
                                              IAssemblyName **pName)
{
    FIXME("(%p, %p) stub!\n", iface, pName);
    return E_NOTIMPL;
}

static const IAssemblyNameVtbl AssemblyNameVtbl = {
    IAssemblyNameImpl_QueryInterface,
    IAssemblyNameImpl_AddRef,
    IAssemblyNameImpl_Release,
    IAssemblyNameImpl_SetProperty,
    IAssemblyNameImpl_GetProperty,
    IAssemblyNameImpl_Finalize,
    IAssemblyNameImpl_GetDisplayName,
    IAssemblyNameImpl_Reserved,
    IAssemblyNameImpl_GetName,
    IAssemblyNameImpl_GetVersion,
    IAssemblyNameImpl_IsEqual,
    IAssemblyNameImpl_Clone
};

static HRESULT parse_version(IAssemblyNameImpl *name, LPWSTR version)
{
    LPWSTR beg, end;
    int i;

    for (i = 0, beg = version; i < 4; i++)
    {
        if (!*beg)
            return S_OK;

        end = strchrW(beg, '.');

        if (end) *end = '\0';
        name->version[i] = atolW(beg);
        name->versize++;

        if (!end && i < 3)
            return S_OK;

        beg = end + 1;
    }

    return S_OK;
}

static HRESULT parse_culture(IAssemblyNameImpl *name, LPWSTR culture)
{
    static const WCHAR empty[] = {0};

    if (lstrlenW(culture) == 2)
        name->culture = strdupW(culture);
    else
        name->culture = strdupW(empty);

    return S_OK;
}

#define CHARS_PER_PUBKEY 16

static BOOL is_hex(WCHAR c)
{
    return ((c >= 'a' && c <= 'f') ||
            (c >= 'A' && c <= 'F') ||
            (c >= '0' && c <= '9'));
}

static BYTE hextobyte(WCHAR c)
{
    if(c >= '0' && c <= '9')
        return c - '0';
    if(c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if(c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    return 0;
}

static HRESULT parse_pubkey(IAssemblyNameImpl *name, LPWSTR pubkey)
{
    int i;
    BYTE val;

    if (lstrlenW(pubkey) < CHARS_PER_PUBKEY)
        return FUSION_E_INVALID_NAME;

    for (i = 0; i < CHARS_PER_PUBKEY; i++)
        if (!is_hex(pubkey[i]))
            return FUSION_E_INVALID_NAME;

    name->haspubkey = TRUE;

    for (i = 0; i < CHARS_PER_PUBKEY; i += 2)
    {
        val = (hextobyte(pubkey[i]) << 4) + hextobyte(pubkey[i + 1]);
        name->pubkey[i / 2] = val;
    }

    return S_OK;
}

static HRESULT parse_display_name(IAssemblyNameImpl *name, LPCWSTR szAssemblyName)
{
    LPWSTR str, save;
    LPWSTR ptr, ptr2;
    HRESULT hr = S_OK;
    BOOL done = FALSE;

    static const WCHAR separator[] = {',',' ',0};
    static const WCHAR version[] = {'V','e','r','s','i','o','n',0};
    static const WCHAR culture[] = {'C','u','l','t','u','r','e',0};
    static const WCHAR pubkey[] =
        {'P','u','b','l','i','c','K','e','y','T','o','k','e','n',0};

    if (!szAssemblyName)
        return S_OK;

    name->displayname = strdupW(szAssemblyName);
    if (!name->displayname)
        return E_OUTOFMEMORY;

    str = strdupW(szAssemblyName);
    save = str;
    if (!str)
        return E_OUTOFMEMORY;

    ptr = strstrW(str, separator);
    if (ptr) *ptr = '\0';
    name->name = strdupW(str);
    if (!name->name)
        return E_OUTOFMEMORY;

    if (!ptr)
        goto done;

    str = ptr + 2;
    while (!done)
    {
        ptr = strchrW(str, '=');
        if (!ptr)
        {
            hr = FUSION_E_INVALID_NAME;
            goto done;
        }

        *(ptr++) = '\0';
        if (!*ptr)
        {
            hr = FUSION_E_INVALID_NAME;
            goto done;
        }

        if (!(ptr2 = strstrW(ptr, separator)))
        {
            if (!(ptr2 = strchrW(ptr, '\0')))
            {
                hr = FUSION_E_INVALID_NAME;
                goto done;
            }

            done = TRUE;
        }

        *ptr2 = '\0';

        while (*str == ' ') str++;

        if (!lstrcmpW(str, version))
            hr = parse_version(name, ptr);
        else if (!lstrcmpW(str, culture))
            hr = parse_culture(name, ptr);
        else if (!lstrcmpW(str, pubkey))
            hr = parse_pubkey(name, ptr);

        if (FAILED(hr))
            goto done;

        str = ptr2 + 1;
    }

done:
    HeapFree(GetProcessHeap(), 0, save);
    return hr;
}

/******************************************************************
 *  CreateAssemblyNameObject   (FUSION.@)
 */
HRESULT WINAPI CreateAssemblyNameObject(LPASSEMBLYNAME *ppAssemblyNameObj,
                                        LPCWSTR szAssemblyName, DWORD dwFlags,
                                        LPVOID pvReserved)
{
    IAssemblyNameImpl *name;
    HRESULT hr;

    TRACE("(%p, %s, %08x, %p) stub!\n", ppAssemblyNameObj,
          debugstr_w(szAssemblyName), dwFlags, pvReserved);

    if (!ppAssemblyNameObj)
        return E_INVALIDARG;

    if ((dwFlags & CANOF_PARSE_DISPLAY_NAME) &&
        (!szAssemblyName || !*szAssemblyName))
        return E_INVALIDARG;

    name = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAssemblyNameImpl));
    if (!name)
        return E_OUTOFMEMORY;

    name->lpIAssemblyNameVtbl = &AssemblyNameVtbl;
    name->ref = 1;

    hr = parse_display_name(name, szAssemblyName);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, name);
        return hr;
    }

    *ppAssemblyNameObj = (IAssemblyName *)name;

    return S_OK;
}
