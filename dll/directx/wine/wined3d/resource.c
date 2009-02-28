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

HRESULT resource_init(struct IWineD3DResourceClass *resource, WINED3DRESOURCETYPE resource_type,
        IWineD3DDeviceImpl *device, UINT size, DWORD usage, WINED3DFORMAT format, WINED3DPOOL pool, IUnknown *parent)
{
    resource->wineD3DDevice = device;
    resource->parent = parent;
    resource->resourceType = resource_type;
    resource->ref = 1;
    resource->pool = pool;
    resource->format = format;
    resource->usage = usage;
    resource->size = size;
    resource->priority = 0;
    list_init(&resource->privateData);

    if (size)
    {
        resource->heapMemory = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size + RESOURCE_ALIGNMENT);
        if (!resource->heapMemory)
        {
            ERR("Out of memory!\n");
            return WINED3DERR_OUTOFVIDEOMEMORY;
        }
    }
    else
    {
        resource->heapMemory = NULL;
    }
    resource->allocatedMemory = (BYTE *)(((ULONG_PTR)resource->heapMemory + (RESOURCE_ALIGNMENT - 1)) & ~(RESOURCE_ALIGNMENT - 1));

    /* Check that we have enough video ram left */
    if (pool == WINED3DPOOL_DEFAULT)
    {
        if (size > IWineD3DDevice_GetAvailableTextureMem((IWineD3DDevice *)device))
        {
            ERR("Out of adapter memory\n");
            HeapFree(GetProcessHeap(), 0, resource->heapMemory);
            return WINED3DERR_OUTOFVIDEOMEMORY;
        }
        WineD3DAdapterChangeGLRam(device, size);
    }

    return WINED3D_OK;
}

void resource_cleanup(IWineD3DResource *iface)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    struct list *e1, *e2;
    PrivateData *data;
    HRESULT hr;

    TRACE("(%p) Cleaning up resource\n", This);
    if (This->resource.pool == WINED3DPOOL_DEFAULT) {
        TRACE("Decrementing device memory pool by %u\n", This->resource.size);
        WineD3DAdapterChangeGLRam(This->resource.wineD3DDevice, -This->resource.size);
    }

    LIST_FOR_EACH_SAFE(e1, e2, &This->resource.privateData) {
        data = LIST_ENTRY(e1, PrivateData, entry);
        hr = resource_free_private_data(iface, &data->tag);
        if(hr != WINED3D_OK) {
            ERR("Failed to free private data when destroying resource %p, hr = %08x\n", This, hr);
        }
    }

    HeapFree(GetProcessHeap(), 0, This->resource.heapMemory);
    This->resource.allocatedMemory = 0;
    This->resource.heapMemory = 0;

    if (This->resource.wineD3DDevice != NULL) {
        IWineD3DDevice_ResourceReleased((IWineD3DDevice *)This->resource.wineD3DDevice, iface);
    }/* NOTE: this is not really an error for system memory resources */
    return;
}

HRESULT resource_get_device(IWineD3DResource *iface, IWineD3DDevice** ppDevice)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) : returning %p\n", This, This->resource.wineD3DDevice);
    *ppDevice = (IWineD3DDevice *) This->resource.wineD3DDevice;
    IWineD3DDevice_AddRef(*ppDevice);
    return WINED3D_OK;
}

static PrivateData* resource_find_private_data(IWineD3DResourceImpl *This, REFGUID tag)
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

HRESULT resource_set_private_data(IWineD3DResource *iface, REFGUID refguid,
        const void *pData, DWORD SizeOfData, DWORD Flags)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    PrivateData *data;

    TRACE("(%p) : %s %p %d %d\n", This, debugstr_guid(refguid), pData, SizeOfData, Flags);
    resource_free_private_data(iface, refguid);

    data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data));
    if (NULL == data) return E_OUTOFMEMORY;

    data->tag = *refguid;
    data->flags = Flags;

    if (Flags & WINED3DSPD_IUNKNOWN) {
        if(SizeOfData != sizeof(IUnknown *)) {
            WARN("IUnknown data with size %d, returning WINED3DERR_INVALIDCALL\n", SizeOfData);
            HeapFree(GetProcessHeap(), 0, data);
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

HRESULT resource_get_private_data(IWineD3DResource *iface, REFGUID refguid, void *pData, DWORD *pSizeOfData)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    PrivateData *data;

    TRACE("(%p) : %p %p %p\n", This, refguid, pData, pSizeOfData);
    data = resource_find_private_data(This, refguid);
    if (data == NULL) return WINED3DERR_NOTFOUND;

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
HRESULT resource_free_private_data(IWineD3DResource *iface, REFGUID refguid)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    PrivateData *data;

    TRACE("(%p) : %s\n", This, debugstr_guid(refguid));
    data = resource_find_private_data(This, refguid);
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

DWORD resource_set_priority(IWineD3DResource *iface, DWORD PriorityNew)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    DWORD PriorityOld = This->resource.priority;
    This->resource.priority = PriorityNew;
    TRACE("(%p) : new priority %d, returning old priority %d\n", This, PriorityNew, PriorityOld );
    return PriorityOld;
}

DWORD resource_get_priority(IWineD3DResource *iface)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->resource.priority );
    return This->resource.priority;
}

WINED3DRESOURCETYPE resource_get_type(IWineD3DResource *iface)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    TRACE("(%p) : returning %d\n", This, This->resource.resourceType);
    return This->resource.resourceType;
}

HRESULT resource_get_parent(IWineD3DResource *iface, IUnknown **pParent)
{
    IWineD3DResourceImpl *This = (IWineD3DResourceImpl *)iface;
    IUnknown_AddRef(This->resource.parent);
    *pParent = This->resource.parent;
    return WINED3D_OK;
}

void dumpResources(struct list *list) {
    IWineD3DResourceImpl *resource;

    LIST_FOR_EACH_ENTRY(resource, list, IWineD3DResourceImpl, resource.resource_list_entry) {
        FIXME("Leftover resource %p with type %d,%s\n", resource, IWineD3DResource_GetType((IWineD3DResource *) resource), debug_d3dresourcetype(IWineD3DResource_GetType((IWineD3DResource *) resource)));
    }
}
