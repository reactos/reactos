/*        DirectDrawEx
 *
 * Copyright 2006 Ulrich Czekalla
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


#include "wine/debug.h"

#define COBJMACROS

#include "winbase.h"
#include "wingdi.h"
#include "objbase.h"
#include "rpcproxy.h"

#include "ddraw.h"

#include "initguid.h"
#include "ddrawex_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddrawex);

struct ddrawex_class_factory
{
    IClassFactory IClassFactory_iface;
    LONG ref;
    HRESULT (*pfnCreateInstance)(IUnknown *outer, REFIID iid, void **out);
};

static inline struct ddrawex_class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex_class_factory, IClassFactory_iface);
}

static HRESULT WINAPI ddrawex_class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IClassFactory)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IClassFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ddrawex_class_factory_AddRef(IClassFactory *iface)
{
    struct ddrawex_class_factory *factory = impl_from_IClassFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI ddrawex_class_factory_Release(IClassFactory *iface)
{
    struct ddrawex_class_factory *factory = impl_from_IClassFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(factory);

    return refcount;
}

static HRESULT WINAPI ddrawex_class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer_unknown, REFIID riid, void **out)
{
    struct ddrawex_class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("iface %p, outer_unknown %p, riid %s, out %p.\n",
            iface, outer_unknown, debugstr_guid(riid), out);

    return factory->pfnCreateInstance(outer_unknown, riid, out);
}

static HRESULT WINAPI ddrawex_class_factory_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("iface %p, dolock %#x stub!\n", iface, dolock);

    return S_OK;
}

static const IClassFactoryVtbl ddrawex_class_factory_vtbl =
{
    ddrawex_class_factory_QueryInterface,
    ddrawex_class_factory_AddRef,
    ddrawex_class_factory_Release,
    ddrawex_class_factory_CreateInstance,
    ddrawex_class_factory_LockServer,
};

struct ddrawex_factory
{
    IDirectDrawFactory IDirectDrawFactory_iface;
    LONG ref;
};

static inline struct ddrawex_factory *impl_from_IDirectDrawFactory(IDirectDrawFactory *iface)
{
    return CONTAINING_RECORD(iface, struct ddrawex_factory, IDirectDrawFactory_iface);
}

static HRESULT WINAPI ddrawex_factory_QueryInterface(IDirectDrawFactory *iface, REFIID riid, void **out)
{
    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (IsEqualGUID(riid, &IID_IDirectDrawFactory)
            || IsEqualGUID(riid, &IID_IUnknown))
    {
        IDirectDrawFactory_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI ddrawex_factory_AddRef(IDirectDrawFactory *iface)
{
    struct ddrawex_factory *factory = impl_from_IDirectDrawFactory(iface);
    ULONG refcount = InterlockedIncrement(&factory->ref);

    TRACE("%p increasing refcount to %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI ddrawex_factory_Release(IDirectDrawFactory *iface)
{
    struct ddrawex_factory *factory = impl_from_IDirectDrawFactory(iface);
    ULONG refcount = InterlockedDecrement(&factory->ref);

    TRACE("%p decreasing refcount to %lu.\n", iface, refcount);

    if (!refcount)
        free(factory);

    return refcount;
}

static HRESULT WINAPI ddrawex_factory_DirectDrawEnumerate(IDirectDrawFactory *iface,
        LPDDENUMCALLBACKW cb, void *ctx)
{
    FIXME("iface %p, cb %p, ctx %p stub!\n", iface, cb, ctx);

    return E_FAIL;
}

static const IDirectDrawFactoryVtbl ddrawex_factory_vtbl =
{
    ddrawex_factory_QueryInterface,
    ddrawex_factory_AddRef,
    ddrawex_factory_Release,
    ddrawex_factory_CreateDirectDraw,
    ddrawex_factory_DirectDrawEnumerate,
};

static HRESULT ddrawex_factory_create(IUnknown *outer_unknown, REFIID riid, void **out)
{
    struct ddrawex_factory *factory;
    HRESULT hr;

    TRACE("outer_unknown %p, riid %s, out %p.\n", outer_unknown, debugstr_guid(riid), out);

    if (outer_unknown)
        return CLASS_E_NOAGGREGATION;

    if (!(factory = calloc(1, sizeof(*factory))))
        return E_OUTOFMEMORY;

    factory->IDirectDrawFactory_iface.lpVtbl = &ddrawex_factory_vtbl;

    if (FAILED(hr = ddrawex_factory_QueryInterface(&factory->IDirectDrawFactory_iface, riid, out)))
        free(factory);

    return hr;
}

/*******************************************************************************
 * DllGetClassObject [DDRAWEX.@]
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **out)
{
    struct ddrawex_class_factory *factory;

    TRACE("rclsid %s, riid %s, out %p.\n", debugstr_guid(rclsid), debugstr_guid(riid), out);

    if (!IsEqualGUID( &IID_IClassFactory, riid)
        && !IsEqualGUID( &IID_IUnknown, riid))
        return E_NOINTERFACE;

    if (!IsEqualGUID(&CLSID_DirectDrawFactory, rclsid))
    {
        FIXME("%s: no class found.\n", debugstr_guid(rclsid));
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    if (!(factory = calloc(1, sizeof(*factory))))
        return E_OUTOFMEMORY;

    factory->IClassFactory_iface.lpVtbl = &ddrawex_class_factory_vtbl;
    factory->ref = 1;

    factory->pfnCreateInstance = ddrawex_factory_create;

    *out = factory;

    return S_OK;
}
