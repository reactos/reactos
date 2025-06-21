/*
 * Device Enumeration
 *
 * Copyright (C) 2002 John K. Hohm
 * Copyright (C) 2002 Robert Shearman
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

#include "devenum_private.h"
#include "rpcproxy.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(devenum);

struct class_factory
{
    IClassFactory IClassFactory_iface;
    IUnknown *obj;
};

static inline struct class_factory *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD( iface, struct class_factory, IClassFactory_iface );
}

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID iid, void **obj)
{
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), obj);

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        IClassFactory_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;
    WARN("no interface for %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_CreateInstance(IClassFactory *iface,
    IUnknown *outer, REFIID iid, void **obj)
{
    struct class_factory *This = impl_from_IClassFactory( iface );

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(iid), obj);

    if (!obj) return E_POINTER;

    if (outer) return CLASS_E_NOAGGREGATION;

    return IUnknown_QueryInterface(This->obj, iid, obj);
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("iface %p, lock %d.\n", iface, lock);
    return S_OK;
}

static const IClassFactoryVtbl ClassFactory_vtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    ClassFactory_CreateInstance,
    ClassFactory_LockServer
};

static struct class_factory create_devenum_cf = { { &ClassFactory_vtbl }, (IUnknown *)&devenum_factory };
static struct class_factory device_moniker_cf = { { &ClassFactory_vtbl }, (IUnknown *)&devenum_parser };

/***********************************************************************
 *		DllGetClassObject (DEVENUM.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **obj)
{
    TRACE("(%s, %s, %p)\n", debugstr_guid(clsid), debugstr_guid(iid), obj);

    *obj = NULL;

    if (IsEqualGUID(clsid, &CLSID_SystemDeviceEnum))
        return IClassFactory_QueryInterface(&create_devenum_cf.IClassFactory_iface, iid, obj);
    else if (IsEqualGUID(clsid, &CLSID_CDeviceMoniker))
        return IClassFactory_QueryInterface(&device_moniker_cf.IClassFactory_iface, iid, obj);

    FIXME("class %s not available\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllRegisterServer (DEVENUM.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    HRESULT res;
    IFilterMapper2 * pMapper = NULL;
    LPVOID mapvptr;

    TRACE("\n");

    res = __wine_register_resources();
    if (FAILED(res))
        return res;

    res = CoCreateInstance(&CLSID_FilterMapper2, NULL, CLSCTX_INPROC,
                           &IID_IFilterMapper2,  &mapvptr);
    if (SUCCEEDED(res))
    {
        pMapper = mapvptr;

        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioCompressorCategory, MERIT_DO_NOT_USE, L"Audio Compressors");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioInputDeviceCategory, MERIT_DO_NOT_USE, L"Audio Capture Sources");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_AudioRendererCategory, MERIT_NORMAL, L"Audio Renderers");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_DeviceControlCategory, MERIT_DO_NOT_USE, L"Device Control Filters");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_LegacyAmFilterCategory, MERIT_NORMAL, L"DirectShow Filters");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_MidiRendererCategory, MERIT_NORMAL, L"Midi Renderers");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_TransmitCategory, MERIT_DO_NOT_USE, L"External Renderers");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_VideoInputDeviceCategory, MERIT_DO_NOT_USE, L"Video Capture Sources");
        IFilterMapper2_CreateCategory(pMapper, &CLSID_VideoCompressorCategory, MERIT_DO_NOT_USE, L"Video Compressors");

        IFilterMapper2_Release(pMapper);
    }

    return res;
}

/***********************************************************************
 *		DllUnregisterServer (DEVENUM.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("stub!\n");
    return __wine_unregister_resources();
}
