/*
 * Object Picker Dialog
 *
 * Copyright (C) 2002 John K. Hohm
 * Copyright (C) 2002 Robert Shearman
 * Copyright 2005 Thomas Weidenmueller <w3seek@reactos.com>
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
#include "objidl.h"
#include "objsel.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(objsel);

typedef struct
{
    IDsObjectPicker IDsObjectPicker_iface;
    LONG ref;
} IDsObjectPickerImpl;

/**********************************************************************
 * OBJSEL_IDsObjectPicker_Destroy (also IUnknown)
 */
static VOID OBJSEL_IDsObjectPicker_Destroy(IDsObjectPickerImpl *This)
{
    HeapFree(GetProcessHeap(),
             0,
             This);
}


static inline IDsObjectPickerImpl *impl_from_IDsObjectPicker(IDsObjectPicker *iface)
{
    return CONTAINING_RECORD(iface, IDsObjectPickerImpl, IDsObjectPicker_iface);
}

/**********************************************************************
 * OBJSEL_IDsObjectPicker_AddRef (also IUnknown)
 */
static ULONG WINAPI OBJSEL_IDsObjectPicker_AddRef(IDsObjectPicker * iface)
{
    IDsObjectPickerImpl *This = impl_from_IDsObjectPicker(iface);

    TRACE("\n");

    return InterlockedIncrement(&This->ref);
}


/**********************************************************************
 * OBJSEL_IDsObjectPicker_Release (also IUnknown)
 */
static ULONG WINAPI OBJSEL_IDsObjectPicker_Release(IDsObjectPicker * iface)
{
    IDsObjectPickerImpl *This = impl_from_IDsObjectPicker(iface);
    ULONG ref;

    TRACE("\n");

    ref = InterlockedDecrement(&This->ref);

    if (ref == 0)
        OBJSEL_IDsObjectPicker_Destroy(This);

    return ref;
}


/**********************************************************************
 * OBJSEL_IDsObjectPicker_QueryInterface (also IUnknown)
 */
static HRESULT WINAPI OBJSEL_IDsObjectPicker_QueryInterface(
    IDsObjectPicker * iface,
    REFIID riid,
    LPVOID *ppvObj)
{
    TRACE("\n\tIID:\t%s\n",debugstr_guid(riid));

    if (ppvObj == NULL) return E_POINTER;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
	IsEqualGUID(riid, &IID_IDsObjectPicker))
    {
        *ppvObj = iface;
	OBJSEL_IDsObjectPicker_AddRef(iface);
	return S_OK;
    }

    FIXME("- no interface IID: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}


/**********************************************************************
 * OBJSEL_IDsObjectPicker_Initialize
 */
static HRESULT WINAPI OBJSEL_IDsObjectPicker_Initialize(
    IDsObjectPicker * iface,
    PDSOP_INIT_INFO pInitInfo)
{
    FIXME("stub!\n");
    return S_OK;
}


/**********************************************************************
 * OBJSEL_IDsObjectPicker_InvokeDialog
 */
static HRESULT WINAPI OBJSEL_IDsObjectPicker_InvokeDialog(
    IDsObjectPicker * iface,
    HWND hwndParent,
    IDataObject** ppdoSelections)
{
    FIXME("stub!\n");
    return S_FALSE;
}


/**********************************************************************
 * IDsObjectPicker_Vtbl
 */
static IDsObjectPickerVtbl IDsObjectPicker_Vtbl =
{
    OBJSEL_IDsObjectPicker_QueryInterface,
    OBJSEL_IDsObjectPicker_AddRef,
    OBJSEL_IDsObjectPicker_Release,
    OBJSEL_IDsObjectPicker_Initialize,
    OBJSEL_IDsObjectPicker_InvokeDialog
};


static HRESULT object_picker_create(void **ppvObj)
{
    IDsObjectPickerImpl *Instance = HeapAlloc(GetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              sizeof(IDsObjectPickerImpl));
    if (Instance != NULL)
    {
        Instance->IDsObjectPicker_iface.lpVtbl = &IDsObjectPicker_Vtbl;
        OBJSEL_IDsObjectPicker_AddRef(&Instance->IDsObjectPicker_iface);

        *ppvObj = Instance;
        return S_OK;
    }
    else
        return E_OUTOFMEMORY;
}


struct class_factory
{
    IClassFactory IClassFactory_iface;
    LONG ref;
};


static struct class_factory *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, struct class_factory, IClassFactory_iface);
}


static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID iid, void **out)
{
    TRACE("iid %s, out %p.\n", debugstr_guid(iid), out);

    if (!out)
        return E_POINTER;

    if (IsEqualGUID(iid, &IID_IUnknown) || IsEqualGUID(iid, &IID_IClassFactory))
    {
        *out = iface;
        IClassFactory_AddRef(iface);
        return S_OK;
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}


static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("\n");

    return InterlockedIncrement(&factory->ref);
}


static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    struct class_factory *factory = impl_from_IClassFactory(iface);

    TRACE("\n");

    return InterlockedDecrement(&factory->ref);
}


static HRESULT WINAPI class_factory_CreateInstance(IClassFactory *iface,
        IUnknown *outer, REFIID iid, void **out)
{
    TRACE("outer %p, iid %s, out %p.\n", outer, debugstr_guid(iid), out);

    if (!out)
        return E_POINTER;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (IsEqualGUID(&IID_IDsObjectPicker, iid))
        return object_picker_create(out);

    return CLASS_E_CLASSNOTAVAILABLE;
}


static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("lock %d.\n", lock);

    if (lock)
        IClassFactory_AddRef(iface);
    else
        IClassFactory_Release(iface);
    return S_OK;
}


static IClassFactoryVtbl class_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    class_factory_CreateInstance,
    class_factory_LockServer
};


static struct class_factory class_factory = {{&class_factory_vtbl}, 0};

/***********************************************************************
 *		DllGetClassObject (OBJSEL.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void **out)
{
    TRACE("clsid %s, iid %s, out %p.\n", debugstr_guid(clsid), debugstr_guid(iid), out);

    *out = NULL;

    if (IsEqualGUID(clsid, &CLSID_DsObjectPicker))
        return IClassFactory_QueryInterface(&class_factory.IClassFactory_iface, iid, out);

    FIXME("%s not available, returning CLASS_E_CLASSNOTAVAILABLE.\n", debugstr_guid(clsid));
    return CLASS_E_CLASSNOTAVAILABLE;
}
