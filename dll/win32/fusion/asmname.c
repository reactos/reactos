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

#include "fusionpriv.h"

#include <assert.h>
#include <winuser.h>

typedef struct {
    IAssemblyName IAssemblyName_iface;

    LPWSTR path;

    LPWSTR displayname;
    LPWSTR name;
    LPWSTR culture;
    LPWSTR procarch;

    WORD version[4];
    DWORD versize;

    BYTE pubkey[8];
    BOOL haspubkey;

    LONG ref;
} IAssemblyNameImpl;

static const WCHAR separator[] = {',',' ',0};
static const WCHAR version[] = {'V','e','r','s','i','o','n',0};
static const WCHAR culture[] = {'C','u','l','t','u','r','e',0};
static const WCHAR pubkey[] =
    {'P','u','b','l','i','c','K','e','y','T','o','k','e','n',0};
static const WCHAR procarch[] = {'p','r','o','c','e','s','s','o','r',
    'A','r','c','h','i','t','e','c','t','u','r','e',0};

#define CHARS_PER_PUBKEY 16

static inline IAssemblyNameImpl *impl_from_IAssemblyName(IAssemblyName *iface)
{
    return CONTAINING_RECORD(iface, IAssemblyNameImpl, IAssemblyName_iface);
}

static HRESULT WINAPI IAssemblyNameImpl_QueryInterface(IAssemblyName *iface,
                                                       REFIID riid, LPVOID *ppobj)
{
    IAssemblyNameImpl *This = impl_from_IAssemblyName(iface);

    TRACE("(%p, %s, %p)\n", This, debugstr_guid(riid), ppobj);

    *ppobj = NULL;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IAssemblyName))
    {
        IAssemblyName_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p, %s, %p): not found\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI IAssemblyNameImpl_AddRef(IAssemblyName *iface)
{
    IAssemblyNameImpl *This = impl_from_IAssemblyName(iface);
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IAssemblyNameImpl_Release(IAssemblyName *iface)
{
    IAssemblyNameImpl *This = impl_from_IAssemblyName(iface);
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(ref before = %u)\n", This, refCount + 1);

    if (!refCount)
    {
        HeapFree(GetProcessHeap(), 0, This->path);
        HeapFree(GetProcessHeap(), 0, This->displayname);
        HeapFree(GetProcessHeap(), 0, This->name);
        HeapFree(GetProcessHeap(), 0, This->culture);
        HeapFree(GetProcessHeap(), 0, This->procarch);
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
    IAssemblyNameImpl *name = impl_from_IAssemblyName(iface);

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
    IAssemblyNameImpl *name = impl_from_IAssemblyName(iface);
    WCHAR verstr[30];
    DWORD size;
    LPWSTR cultureval = 0;

    static const WCHAR equals[] = {'=',0};

    TRACE("(%p, %p, %p, %d)\n", iface, szDisplayName,
          pccDisplayName, dwDisplayFlags);

    if (dwDisplayFlags == 0)
    {
        if (!name->displayname || !*name->displayname)
            return FUSION_E_INVALID_NAME;

        size = min(*pccDisplayName, lstrlenW(name->displayname) + 1);

        lstrcpynW(szDisplayName, name->displayname, size);
        *pccDisplayName = size;

        return S_OK;
    }

    if (!name->name || !*name->name)
        return FUSION_E_INVALID_NAME;

    /* Verify buffer size is sufficient */
    size = lstrlenW(name->name) + 1;

    if ((dwDisplayFlags & ASM_DISPLAYF_VERSION) && (name->versize > 0))
    {
        static const WCHAR spec[] = {'%','d',0};
        static const WCHAR period[] = {'.',0};
        DWORD i;

        wsprintfW(verstr, spec, name->version[0]);

        for (i = 1; i < name->versize; i++)
        {
            WCHAR value[6];
            wsprintfW(value, spec, name->version[i]);

            lstrcatW(verstr, period);
            lstrcatW(verstr, value);
        }

        size += lstrlenW(separator) + lstrlenW(version) + lstrlenW(equals) + lstrlenW(verstr);
    }

    if ((dwDisplayFlags & ASM_DISPLAYF_CULTURE) && (name->culture))
    {
        static const WCHAR neutral[] = {'n','e','u','t','r','a','l', 0};

        cultureval = (lstrlenW(name->culture) == 2) ? name->culture : (LPWSTR) neutral;
        size += lstrlenW(separator) + lstrlenW(culture) + lstrlenW(equals) + lstrlenW(cultureval);
    }

    if ((dwDisplayFlags & ASM_DISPLAYF_PUBLIC_KEY_TOKEN) && (name->haspubkey))
        size += lstrlenW(separator) + lstrlenW(pubkey) + lstrlenW(equals) + CHARS_PER_PUBKEY;

    if ((dwDisplayFlags & ASM_DISPLAYF_PROCESSORARCHITECTURE) && (name->procarch))
        size += lstrlenW(separator) + lstrlenW(procarch) + lstrlenW(equals) + lstrlenW(name->procarch);

    if (size > *pccDisplayName)
        return S_FALSE;

    /* Construct the string */
    lstrcpyW(szDisplayName, name->name);

    if ((dwDisplayFlags & ASM_DISPLAYF_VERSION) && (name->versize > 0))
    {
        lstrcatW(szDisplayName, separator);

        lstrcatW(szDisplayName, version);
        lstrcatW(szDisplayName, equals);
        lstrcatW(szDisplayName, verstr);
    }

    if ((dwDisplayFlags & ASM_DISPLAYF_CULTURE) && (name->culture))
    {
        lstrcatW(szDisplayName, separator);

        lstrcatW(szDisplayName, culture);
        lstrcatW(szDisplayName, equals);
        lstrcatW(szDisplayName, cultureval);
    }

    if ((dwDisplayFlags & ASM_DISPLAYF_PUBLIC_KEY_TOKEN) && (name->haspubkey))
    {
        WCHAR pkt[CHARS_PER_PUBKEY + 1];
        static const WCHAR spec[] = {'%','0','x','%','0','x','%','0','x',
            '%','0','x','%','0','x','%','0','x','%','0','x','%','0','x',0};

        lstrcatW(szDisplayName, separator);

        lstrcatW(szDisplayName, pubkey);
        lstrcatW(szDisplayName, equals);

        wsprintfW(pkt, spec, name->pubkey[0], name->pubkey[1], name->pubkey[2],
            name->pubkey[3], name->pubkey[4], name->pubkey[5], name->pubkey[6],
            name->pubkey[7]);

        lstrcatW(szDisplayName, pkt);
    }

    if ((dwDisplayFlags & ASM_DISPLAYF_PROCESSORARCHITECTURE) && (name->procarch))
    {
        lstrcatW(szDisplayName, separator);

        lstrcatW(szDisplayName, procarch);
        lstrcatW(szDisplayName, equals);
        lstrcatW(szDisplayName, name->procarch);
    }

    *pccDisplayName = size;
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
    IAssemblyNameImpl *name = impl_from_IAssemblyName(iface);

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
    IAssemblyNameImpl *name = impl_from_IAssemblyName(iface);

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
                                                DWORD flags)
{
    IAssemblyNameImpl *name1 = impl_from_IAssemblyName(iface);
    IAssemblyNameImpl *name2 = impl_from_IAssemblyName(pName);

    TRACE("(%p, %p, 0x%08x)\n", iface, pName, flags);

    if (!pName) return S_FALSE;
    if (flags & ~ASM_CMPF_IL_ALL) FIXME("unsupported flags\n");

    if ((flags & ASM_CMPF_NAME) && strcmpW(name1->name, name2->name)) return S_FALSE;
    if (name1->versize && name2->versize)
    {
        if ((flags & ASM_CMPF_MAJOR_VERSION) &&
            name1->version[0] != name2->version[0]) return S_FALSE;
        if ((flags & ASM_CMPF_MINOR_VERSION) &&
            name1->version[1] != name2->version[1]) return S_FALSE;
        if ((flags & ASM_CMPF_BUILD_NUMBER) &&
            name1->version[2] != name2->version[2]) return S_FALSE;
        if ((flags & ASM_CMPF_REVISION_NUMBER) &&
            name1->version[3] != name2->version[3]) return S_FALSE;
    }
    if ((flags & ASM_CMPF_PUBLIC_KEY_TOKEN) &&
        name1->haspubkey && name2->haspubkey &&
        memcmp(name1->pubkey, name2->pubkey, sizeof(name1->pubkey))) return S_FALSE;

    if ((flags & ASM_CMPF_CULTURE) &&
        name1->culture && name2->culture &&
        strcmpW(name1->culture, name2->culture)) return S_FALSE;

    return S_OK;
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

/* Internal methods */
static inline IAssemblyNameImpl *unsafe_impl_from_IAssemblyName(IAssemblyName *iface)
{
    assert(iface->lpVtbl == &AssemblyNameVtbl);

    return impl_from_IAssemblyName(iface);
}

HRESULT IAssemblyName_SetPath(IAssemblyName *iface, LPCWSTR path)
{
    IAssemblyNameImpl *name = unsafe_impl_from_IAssemblyName(iface);

    name->path = strdupW(path);
    if (!name->path)
        return E_OUTOFMEMORY;

    return S_OK;
}

HRESULT IAssemblyName_GetPath(IAssemblyName *iface, LPWSTR buf, ULONG *len)
{
    ULONG buffer_size = *len;
    IAssemblyNameImpl *name = unsafe_impl_from_IAssemblyName(iface);

    if (!name->path)
        return S_OK;

    if (!buf)
        buffer_size = 0;

    *len = lstrlenW(name->path) + 1;

    if (*len <= buffer_size)
        lstrcpyW(buf, name->path);
    else
        return E_NOT_SUFFICIENT_BUFFER;

    return S_OK;
}

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

static HRESULT parse_culture(IAssemblyNameImpl *name, LPCWSTR culture)
{
    static const WCHAR empty[] = {0};

    if (lstrlenW(culture) == 2)
        name->culture = strdupW(culture);
    else
        name->culture = strdupW(empty);

    return S_OK;
}

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

static HRESULT parse_pubkey(IAssemblyNameImpl *name, LPCWSTR pubkey)
{
    int i;
    BYTE val;
    static const WCHAR nullstr[] = {'n','u','l','l',0};

    if(lstrcmpiW(pubkey, nullstr) == 0)
        return FUSION_E_PRIVATE_ASM_DISALLOWED;

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

static WCHAR *parse_value( const WCHAR *str, unsigned int len )
{
    WCHAR *ret;
    const WCHAR *p = str;
    BOOL quoted = FALSE;
    unsigned int i = 0;

    if (!(ret = HeapAlloc( GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR) ))) return NULL;
    if (*p == '\"')
    {
        quoted = TRUE;
        p++;
    }
    while (*p && *p != '\"') ret[i++] = *p++;
    if ((quoted && *p != '\"') || (!quoted && *p == '\"'))
    {
        HeapFree( GetProcessHeap(), 0, ret );
        return NULL;
    }
    ret[i] = 0;
    return ret;
}

static HRESULT parse_display_name(IAssemblyNameImpl *name, LPCWSTR szAssemblyName)
{
    LPWSTR str, save, ptr, ptr2, value;
    HRESULT hr = S_OK;
    BOOL done = FALSE;

    if (!szAssemblyName)
        return S_OK;

    name->displayname = strdupW(szAssemblyName);
    if (!name->displayname)
        return E_OUTOFMEMORY;

    str = strdupW(szAssemblyName);
    save = str;
    if (!str)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ptr = strchrW(str, ',');
    if (ptr) *ptr = '\0';

    /* no ',' but ' ' only */
    if( !ptr && strchrW(str, ' ') )
    {
        hr = FUSION_E_INVALID_NAME;
        goto done;
    }

    name->name = strdupW(str);
    if (!name->name)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    if (!ptr)
        goto done;

    str = ptr + 1;
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

        if (!(ptr2 = strchrW(ptr, ',')))
        {
            if (!(ptr2 = strchrW(ptr, '\0')))
            {
                hr = FUSION_E_INVALID_NAME;
                goto done;
            }

            done = TRUE;
        }

        *ptr2 = '\0';
        if (!(value = parse_value( ptr, ptr2 - ptr )))
        {
            hr = FUSION_E_INVALID_NAME;
            goto done;
        }
        while (*str == ' ') str++;

        if (!lstrcmpiW(str, version))
            hr = parse_version( name, value );
        else if (!lstrcmpiW(str, culture))
            hr = parse_culture( name, value );
        else if (!lstrcmpiW(str, pubkey))
            hr = parse_pubkey( name, value );
        else if (!lstrcmpiW(str, procarch))
        {
            name->procarch = value;
            value = NULL;
        }
        HeapFree( GetProcessHeap(), 0, value );

        if (FAILED(hr))
            goto done;

        str = ptr2 + 1;
    }

done:
    HeapFree(GetProcessHeap(), 0, save);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, name->displayname);
        HeapFree(GetProcessHeap(), 0, name->name);
        HeapFree(GetProcessHeap(), 0, name->culture);
        HeapFree(GetProcessHeap(), 0, name->procarch);
    }
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

    TRACE("(%p, %s, %08x, %p)\n", ppAssemblyNameObj,
          debugstr_w(szAssemblyName), dwFlags, pvReserved);

    if (!ppAssemblyNameObj)
        return E_INVALIDARG;

    if ((dwFlags & CANOF_PARSE_DISPLAY_NAME) &&
        (!szAssemblyName || !*szAssemblyName))
        return E_INVALIDARG;

    name = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IAssemblyNameImpl));
    if (!name)
        return E_OUTOFMEMORY;

    name->IAssemblyName_iface.lpVtbl = &AssemblyNameVtbl;
    name->ref = 1;

    hr = parse_display_name(name, szAssemblyName);
    if (FAILED(hr))
    {
        HeapFree(GetProcessHeap(), 0, name);
        return hr;
    }

    *ppAssemblyNameObj = &name->IAssemblyName_iface;

    return S_OK;
}
