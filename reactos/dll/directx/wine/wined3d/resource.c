/*
 * IWineD3DResource Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
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

#include "config.h"
#include "wined3d_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d);
#define GLINFO_LOCATION ((IWineD3DImpl *)(((IWineD3DDeviceImpl *)This->resource.wineD3DDevice)->wineD3D))->gl_info

/* IWineD3DResource IUnknown parts follow: */
HRESULT WINAPI IWineD3DResourceImpl_QueryInterface(IWineD3DResource *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

ULONG WINAPI IWineD3DResourceImpl_AddRef(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %d\n", This, ref - 1);
    return ref;
}

ULONG WINAPI IWineD3DResourceImpl_Release(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {
        IWineD3DResourceImpl_CleanUp(iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* class static (not in vtable) */
void IWineD3DResourceImpl_CleanUp(IWineD3DResource *iface){
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    struct list *e1, *e2;
    PrivateData *data;
    HRESULT hr;

    TRACE("(%p) Cleaning up resource\n", This);
    if (This->resource.pool == WINED3DPOOL_DEFAULT) {
        TRACE("Decrementing device memory pool by %u\n", This->resource.size);
        globalChangeGlRam(-This->resource.size);
    }

    LIST_FOR_EACH_SAFE(e1, e2, &This->resource.privateData) {
        data = LIST_ENTRY(e1, PrivateData, entry);
        hr = IWineD3DResourceImpl_FreePrivateData(iface, &data->tag);
        if(hr != WINED3D_OK) {
            ERR("Failed to free private data when destroying resource %p, hr = %08x\n", This, hr);
        }
    }

    HeapFree(GetProcessHeap(), 0, This->resource.allocatedMemory);
    This->resource.allocatedMemory = 0;

    if (This->resource.wineD3DDevice != NULL) {
        IWineD3DDevice_ResourceReleased((IWineD3DDevice *)This->resource.wineD3DDevice, iface);
    }/* NOTE: this is not really an error for systemmem resoruces */
    return;
}

/* IWineD3DResource Interface follow: */
HRESULT WINAPI IWineD3DResourceImpl_GetDevice(IWineD3DResource *iface, IWineD3DDevice** ppDevice) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->resource.wineD3DDevice);
    *ppDevice = (IWineD3DDevice *) This->resource.wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);
    return WINED3D_OK;
}

static PrivateData* IWineD3DResourceImpl_FindPrivateData(IWineD3DResourceImpl *This,
                    REFGUID tag)
{
    PrivateData *data;
    struct list *entry;

    TRACE("Searching for private data %s\n", debugstr_guid(tag));
    LIST_FOR_EACH(entry, &This->resource.privateData)
    {
        data = LIST_ENTRY(entry, PrivateData, entry);
        if (IsEqualGUID(&data->tag, tag)) {
            TRACE("Found %p\n", data);
            return data;
        }
    }
    TRACE("Not found\n");
    return NULL;
}

HRESULT WINAPI IWineD3DResourceImpl_SetPrivateData(IWineD3DResource *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    PrivateData *data;

    TRACE("(%p) : %s %p %d %d\n", This, debugstr_guid(refguid), pData, SizeOfData, Flags);
    IWineD3DResourceImpl_FreePrivateData(iface, refguid);

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data));
    if (NULL == data) return E_OUTOFMEMORY;

    data->tag = *refguid;
    data->flags = Flags;
#if 0
        (*data)->uniquenessValue = This->uniquenessValue;
#endif
    if (Flags & WINED3DSPD_IUNKNOWN) {
        if(SizeOfData != sizeof(IUnknown *)) {
            WARN("IUnknown data with size %d, returning WINED3DERR_INVALIDCALL\n", SizeOfData);
            return WINED3DERR_INVALIDCALL;
        }
        data->ptr.object = (LPUNKNOWN)pData;
        data->size = sizeof(LPUNKNOWN);
        IUnknown_AddRef(data->ptr.object);
    }
    else
    {
        data->ptr.data = HeapAlloc(GetProcessHeap(), 0, SizeOfData);
        if (NULL == data->ptr.data) {
            HeapFree(GetProcessHeap(), 0, data);
            return E_OUTOFMEMORY;
        }
        data->size = SizeOfData;
        memcpy(data->ptr.data, pData, SizeOfData);
    }
    list_add_tail(&This->resource.privateData, &data->entry);

    return WINED3D_OK;
}

HRESULT WINAPI IWineD3DResourceImpl_GetPrivateData(IWineD3DResource *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    PrivateData *data;

    TRACE("(%p) : %p %p %p\n", This, refguid, pData, pSizeOfData);
    data = IWineD3DResourceImpl_FindPrivateData(This, refguid);
    if (data == NULL) return WINED3DERR_NOTFOUND;


#if 0 /* This may not be right. */
    if (((*data)->flags & WINED3DSPD_VOLATILE)
        && (*data)->uniquenessValue != This->uniquenessValue)
        return DDERR_EXPIRED;
#endif
    if (*pSizeOfData < data->size) {
        *pSizeOfData = data->size;
        return WINED3DERR_MOREDATA;
    }

    if (data->flags & WINED3DSPD_IUNKNOWN) {
        *(LPUNKNOWN *)pData = data->ptr.object;
        if(((IWineD3DImpl *) This->resource.wineD3DDevice->wineD3D)->dxVersion != 7) {
            /* D3D8 and D3D9 addref the private data, DDraw does not. This can't be handled in
             * ddraw because it doesn't know if the pointer returned is an IUnknown * or just a
             * Blob
             */
            IUnknown_AddRef(data->ptr.object);
        }
    }
    else {
        memcpy(pData, data->ptr.data, data->size);
    }

    return WINED3D_OK;
}
HRESULT WINAPI IWineD3DResourceImpl_FreePrivateData(IWineD3DResource *iface, REFGUID refguid) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    PrivateData *data;

    TRACE("(%p) : %s\n", This, debugstr_guid(refguid));
    data = IWineD3DResourceImpl_FindPrivateData(This, refguid);
    if (data == NULL) return WINED3DERR_NOTFOUND;

    if (data->flags & WINED3DSPD_IUNKNOWN)
    {
        if (data->ptr.object != NULL)
            IUnknown_Release(data->ptr.object);
    } else {
        HeapFree(GetProcessHeap(), 0, data->ptr.data);
    }
    list_remove(&data->entry);

    HeapFree(GetProcessHeap(), 0, data);

    return WINED3D_OK;
}

/* Priority support is not implemented yet */
DWORD    WINAPI        IWineD3DResourceImpl_SetPriority(IWineD3DResource *iface, DWORD PriorityNew) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}
DWORD    WINAPI        IWineD3DResourceImpl_GetPriority(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);
    return 0;
}

/* Preloading of resources is not supported yet */
void     WINAPI        IWineD3DResourceImpl_PreLoad(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    FIXME("(%p) : stub\n", This);
}

WINED3DRESOURCETYPE WINAPI IWineD3DResourceImpl_GetType(IWineD3DResource *iface) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->resource.resourceType);
    return This->resource.resourceType;
}

HRESULT WINAPI IWineD3DResourceImpl_GetParent(IWineD3DResource *iface, IUnknown **pParent) {
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    IUnknown_AddRef(This->resource.parent);
    *pParent = This->resource.parent;
    return WINED3D_OK;
}

void dumpResources(ResourceList *resources) {
    ResourceList *iterator = resources;

    while(iterator) {
        FIXME("Leftover resource %p with type %d,%s\n", iterator->resource, IWineD3DResource_GetType(iterator->resource), debug_d3dresourcetype(IWineD3DResource_GetType(iterator->resource)));
        iterator = iterator->next;
    }
}

static const IWineD3DResourceVtbl IWineD3DResource_Vtbl =
{
    /* IUnknown */
    IWineD3DResourceImpl_QueryInterface,
    IWineD3DResourceImpl_AddRef,
    IWineD3DResourceImpl_Release,
    /* IWineD3DResource */
    IWineD3DResourceImpl_GetParent,
    IWineD3DResourceImpl_GetDevice,
    IWineD3DResourceImpl_SetPrivateData,
    IWineD3DResourceImpl_GetPrivateData,
    IWineD3DResourceImpl_FreePrivateData,
    IWineD3DResourceImpl_SetPriority,
    IWineD3DResourceImpl_GetPriority,
    IWineD3DResourceImpl_PreLoad,
    IWineD3DResourceImpl_GetType
};
