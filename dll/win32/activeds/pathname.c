/*
 * Copyright 2020 Dmitry Timoshkov
 *
 * This file contains only stubs to get the printui.dll up and running
 * activeds.dll is much much more than this
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

#include "windef.h"
#include "winbase.h"
#include "iads.h"
#include "adserr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(activeds);

#include "initguid.h"
DEFINE_GUID(CLSID_Pathname,0x080d0d78,0xf421,0x11d0,0xa3,0x6e,0x00,0xc0,0x4f,0xb9,0x50,0xdc);

typedef struct
{
    IADsPathname IADsPathname_iface;
    LONG ref;
    BSTR provider, server, dn;
} Pathname;

static inline Pathname *impl_from_IADsPathname(IADsPathname *iface)
{
    return CONTAINING_RECORD(iface, Pathname, IADsPathname_iface);
}

static HRESULT WINAPI path_QueryInterface(IADsPathname *iface, REFIID riid, void **obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, &IID_IADsPathname))
    {
        IADsPathname_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI path_AddRef(IADsPathname *iface)
{
    Pathname *path = impl_from_IADsPathname(iface);
    return InterlockedIncrement(&path->ref);
}

static ULONG WINAPI path_Release(IADsPathname *iface)
{
    Pathname *path = impl_from_IADsPathname(iface);
    LONG ref = InterlockedDecrement(&path->ref);

    if (!ref)
    {
        TRACE("destroying %p\n", iface);
        SysFreeString(path->provider);
        SysFreeString(path->server);
        SysFreeString(path->dn);
        free(path);
    }

    return ref;
}

static HRESULT WINAPI path_GetTypeInfoCount(IADsPathname *iface, UINT *count)
{
    FIXME("%p,%p: stub\n", iface, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetTypeInfo(IADsPathname *iface, UINT index, LCID lcid, ITypeInfo **info)
{
    FIXME("%p,%u,%#lx,%p: stub\n", iface, index, lcid, info);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetIDsOfNames(IADsPathname *iface, REFIID riid, LPOLESTR *names,
                                         UINT count, LCID lcid, DISPID *dispid)
{
    FIXME("%p,%s,%p,%u,%lu,%p: stub\n", iface, debugstr_guid(riid), names, count, lcid, dispid);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_Invoke(IADsPathname *iface, DISPID dispid, REFIID riid, LCID lcid, WORD flags,
                                  DISPPARAMS *params, VARIANT *result, EXCEPINFO *excepinfo, UINT *argerr)
{
    FIXME("%p,%ld,%s,%04lx,%04x,%p,%p,%p,%p: stub\n", iface, dispid, debugstr_guid(riid), lcid, flags,
          params, result, excepinfo, argerr);
    return E_NOTIMPL;
}

static HRESULT parse_path(BSTR path, BSTR *provider, BSTR *server, BSTR *dn)
{
    WCHAR *p, *p_server;
    int server_len;

    *provider = NULL;
    *server = NULL;
    *dn = NULL;

    if (wcsnicmp(path, L"LDAP:", 5) != 0)
        return E_ADS_BAD_PATHNAME;

    *provider = SysAllocStringLen(path, 4);
    if (!*provider) return E_OUTOFMEMORY;

    p = path + 5;
    if (!*p) return S_OK;

    if (*p++ != '/' || *p++ != '/' || !*p)
    {
        SysFreeString(*provider);
        return E_ADS_BAD_PATHNAME;
    }

    p_server = p;
    server_len = 0;
    while (*p && *p != '/')
    {
        p++;
        server_len++;
    }
    if (server_len == 0)
    {
        SysFreeString(*provider);
        return E_ADS_BAD_PATHNAME;
    }

    *server = SysAllocStringLen(p_server, server_len);
    if (!*server)
    {
        SysFreeString(*provider);
        return E_OUTOFMEMORY;
    }

    if (!*p) return S_OK;

    if (*p++ != '/' || !*p)
    {
        SysFreeString(*provider);
        SysFreeString(*server);
        return E_ADS_BAD_PATHNAME;
    }

    *dn = SysAllocString(p);
    if (!*dn)
    {
        SysFreeString(*provider);
        SysFreeString(*server);
        return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT WINAPI path_Set(IADsPathname *iface, BSTR adspath, LONG type)
{
    Pathname *path = impl_from_IADsPathname(iface);
    HRESULT hr;
    BSTR provider, server, dn;

    TRACE("%p,%s,%ld\n", iface, debugstr_w(adspath), type);

    if (!adspath) return E_INVALIDARG;

    if (type == ADS_SETTYPE_PROVIDER)
    {
        SysFreeString(path->provider);
        path->provider = SysAllocString(adspath);
        return path->provider ? S_OK : E_OUTOFMEMORY;
    }

    if (type == ADS_SETTYPE_SERVER)
    {
        SysFreeString(path->server);
        path->server = SysAllocString(adspath);
        return path->server ? S_OK : E_OUTOFMEMORY;
    }

    if (type == ADS_SETTYPE_DN)
    {
        SysFreeString(path->dn);
        path->dn = SysAllocString(adspath);
        return path->dn ? S_OK : E_OUTOFMEMORY;
    }

    if (type != ADS_SETTYPE_FULL)
    {
        FIXME("type %ld not implemented\n", type);
        return E_INVALIDARG;
    }

    hr = parse_path(adspath, &provider, &server, &dn);
    if (hr == S_OK)
    {
        SysFreeString(path->provider);
        SysFreeString(path->server);
        SysFreeString(path->dn);

        path->provider = provider;
        path->server = server;
        path->dn = dn;
    }
    return hr;
}

static HRESULT WINAPI path_SetDisplayType(IADsPathname *iface, LONG type)
{
    FIXME("%p,%ld: stub\n", iface, type);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_Retrieve(IADsPathname *iface, LONG type, BSTR *adspath)
{
    Pathname *path = impl_from_IADsPathname(iface);
    int len;

    TRACE("%p,%ld,%p\n", iface, type, adspath);

    if (!adspath) return E_INVALIDARG;

    switch (type)
    {
    default:
        FIXME("type %ld not implemented\n", type);
        /* fall through */

    case ADS_FORMAT_X500:
        len = wcslen(path->provider) + 3;
        if (path->server) len += wcslen(path->server) + 1;
        if (path->dn) len += wcslen(path->dn);

        *adspath = SysAllocStringLen(NULL, len);
        if (!*adspath) break;

        wcscpy(*adspath, path->provider);
        wcscat(*adspath, L"://");
        if (path->server)
        {
            wcscat(*adspath, path->server);
            wcscat(*adspath, L"/");
        }
        if (path->dn) wcscat(*adspath, path->dn);
        break;

    case ADS_FORMAT_PROVIDER:
        *adspath = SysAllocString(path->provider);
        break;

    case ADS_FORMAT_SERVER:
        *adspath = path->provider ? SysAllocString(path->server) : SysAllocStringLen(NULL, 0);
        break;

    case ADS_FORMAT_X500_DN:
        *adspath = path->dn ? SysAllocString(path->dn) : SysAllocStringLen(NULL, 0);
        break;

    case ADS_FORMAT_LEAF:
        if (!path->dn)
            *adspath = SysAllocStringLen(NULL, 0);
        else
        {
            WCHAR *p = wcschr(path->dn, ',');
            *adspath = p ? SysAllocStringLen(path->dn, p - path->dn) : SysAllocString(path->dn);
        }
        break;
    }

    TRACE("=> %s\n", debugstr_w(*adspath));
    return *adspath ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI path_GetNumElements(IADsPathname *iface, LONG *count)
{
    Pathname *path = impl_from_IADsPathname(iface);
    WCHAR *p;

    TRACE("%p,%p\n", iface, count);

    if (!count) return E_INVALIDARG;

    *count = 0;

    p = path->dn;
    while (p)
    {
        *count += 1;
        p = wcschr(p, ',');
        if (p) p++;
    }

    return S_OK;
}

static HRESULT WINAPI path_GetElement(IADsPathname *iface, LONG index, BSTR *element)
{
    Pathname *path = impl_from_IADsPathname(iface);
    HRESULT hr;
    WCHAR *p, *end;
    LONG count;

    TRACE("%p,%ld,%p\n", iface, index, element);

    if (!element) return E_INVALIDARG;

    count = 0;
    hr = HRESULT_FROM_WIN32(ERROR_INVALID_INDEX);

    p = path->dn;
    while (p)
    {
        end = wcschr(p, ',');

        if (index == count)
        {
            *element = end ? SysAllocStringLen(p, end - p) : SysAllocString(p);
            hr = *element ? S_OK : E_OUTOFMEMORY;
            break;
        }

        p = end ? end + 1 : NULL;
        count++;
    }

    return hr;
}

static HRESULT WINAPI path_AddLeafElement(IADsPathname *iface, BSTR element)
{
    FIXME("%p,%s: stub\n", iface, debugstr_w(element));
    return E_NOTIMPL;
}

static HRESULT WINAPI path_RemoveLeafElement(IADsPathname *iface)
{
    FIXME("%p: stub\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_CopyPath(IADsPathname *iface, IDispatch **path)
{
    FIXME("%p,%p: stub\n", iface, path);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_GetEscapedElement(IADsPathname *iface, LONG reserved, BSTR element, BSTR *str)
{
    FIXME("%p,%ld,%s,%p: stub\n", iface, reserved, debugstr_w(element), str);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_get_EscapedMode(IADsPathname *iface, LONG *mode)
{
    FIXME("%p,%p: stub\n", iface, mode);
    return E_NOTIMPL;
}

static HRESULT WINAPI path_put_EscapedMode(IADsPathname *iface, LONG mode)
{
    FIXME("%p,%ld: stub\n", iface, mode);
    return E_NOTIMPL;
}

static const IADsPathnameVtbl IADsPathname_vtbl =
{
    path_QueryInterface,
    path_AddRef,
    path_Release,
    path_GetTypeInfoCount,
    path_GetTypeInfo,
    path_GetIDsOfNames,
    path_Invoke,
    path_Set,
    path_SetDisplayType,
    path_Retrieve,
    path_GetNumElements,
    path_GetElement,
    path_AddLeafElement,
    path_RemoveLeafElement,
    path_CopyPath,
    path_GetEscapedElement,
    path_get_EscapedMode,
    path_put_EscapedMode
};

static HRESULT Pathname_create(REFIID riid, void **obj)
{
    Pathname *path;
    HRESULT hr;

    path = malloc(sizeof(*path));
    if (!path) return E_OUTOFMEMORY;

    path->IADsPathname_iface.lpVtbl = &IADsPathname_vtbl;
    path->ref = 1;
    path->provider = SysAllocString(L"LDAP");
    path->server = NULL;
    path->dn = NULL;

    hr = IADsPathname_QueryInterface(&path->IADsPathname_iface, riid, obj);
    IADsPathname_Release(&path->IADsPathname_iface);

    return hr;
}

static const struct class_info
{
    const CLSID *clsid;
    HRESULT (*constructor)(REFIID, void **);
} class_info[] =
{
    { &CLSID_Pathname, Pathname_create }
};

typedef struct
{
    IClassFactory IClassFactory_iface;
    LONG ref;
    const struct class_info *info;
} class_factory;

static inline class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, class_factory, IClassFactory_iface);
}

static HRESULT WINAPI factory_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *obj)
{
    TRACE("%p,%s,%p\n", iface, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    if (IsEqualIID(riid, &IID_IUnknown) ||
        IsEqualIID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    FIXME("interface %s is not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI factory_AddRef(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&factory->ref);

    TRACE("(%p) ref %lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI factory_Release(IClassFactory *iface)
{
    class_factory *factory = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&factory->ref);

    TRACE("(%p) ref %lu\n", iface, ref);

    if (!ref)
        free(factory);

    return ref;
}

static HRESULT WINAPI factory_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **obj)
{
    class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("%p,%s,%p\n", outer, debugstr_guid(riid), obj);

    if (!riid || !obj) return E_INVALIDARG;

    *obj = NULL;
    if (outer) return CLASS_E_NOAGGREGATION;

    return factory->info->constructor(riid, obj);
}

static HRESULT WINAPI factory_LockServer(IClassFactory *iface, BOOL lock)
{
    FIXME("%p,%d: stub\n", iface, lock);
    return S_OK;
}

static const struct IClassFactoryVtbl factory_vtbl =
{
    factory_QueryInterface,
    factory_AddRef,
    factory_Release,
    factory_CreateInstance,
    factory_LockServer
};

static HRESULT factory_constructor(const struct class_info *info, REFIID riid, void **obj)
{
    class_factory *factory;
    HRESULT hr;

    factory = malloc(sizeof(*factory));
    if (!factory) return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &factory_vtbl;
    factory->ref = 1;
    factory->info = info;

    hr = IClassFactory_QueryInterface(&factory->IClassFactory_iface, riid, obj);
    IClassFactory_Release(&factory->IClassFactory_iface);

    return hr;
}

HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID *obj)
{
    int i;

    TRACE("%s,%s,%p\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    if (!clsid || !iid || !obj) return E_INVALIDARG;

    *obj = NULL;

    for (i = 0; i < ARRAY_SIZE(class_info); i++)
    {
        if (IsEqualCLSID(class_info[i].clsid, clsid))
            return factory_constructor(&class_info[i], iid, obj);
    }

    FIXME("class %s/%s is not implemented\n", debugstr_guid(clsid), debugstr_guid(iid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
