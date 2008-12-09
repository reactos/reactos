/*
 * IWineD3DIndexBuffer Implementation
 *
 * Copyright 2002-2004 Jason Edmeades
 * Copyright 2003-2004 Raphael Junqueira
 * Copyright 2004 Christian Costa
 * Copyright 2005 Oliver Stieber
 * Copyright 2007 Stefan Dösinger for CodeWeavers
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
#define GLINFO_LOCATION This->resource.wineD3DDevice->adapter->gl_info

/* *******************************************
   IWineD3DIndexBuffer IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DIndexBufferImpl_QueryInterface(IWineD3DIndexBuffer *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DIndexBuffer)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DIndexBufferImpl_AddRef(IWineD3DIndexBuffer *iface) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IWineD3DIndexBufferImpl_Release(IWineD3DIndexBuffer *iface) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {
        if(This->vbo) {
            IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            /* No need to manually unset the buffer. glDeleteBuffers unsets it for the current context,
             * but not for other contexts. However, because the d3d buffer is destroyed the app has to
             * unset it before doing the next draw, thus dirtifying the index buffer state and forcing
             * binding a new buffer
             */
            GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
        }

        resource_cleanup((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DIndexBuffer IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DIndexBufferImpl_GetDevice(IWineD3DIndexBuffer *iface, IWineD3DDevice** ppDevice) {
    return resource_get_device((IWineD3DResource *)iface, ppDevice);
}

static HRESULT WINAPI IWineD3DIndexBufferImpl_SetPrivateData(IWineD3DIndexBuffer *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return resource_set_private_data((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DIndexBufferImpl_GetPrivateData(IWineD3DIndexBuffer *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return resource_get_private_data((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DIndexBufferImpl_FreePrivateData(IWineD3DIndexBuffer *iface, REFGUID refguid) {
    return resource_free_private_data((IWineD3DResource *)iface, refguid);
}

static DWORD WINAPI IWineD3DIndexBufferImpl_SetPriority(IWineD3DIndexBuffer *iface, DWORD PriorityNew) {
    return resource_set_priority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD WINAPI IWineD3DIndexBufferImpl_GetPriority(IWineD3DIndexBuffer *iface) {
    return resource_get_priority((IWineD3DResource *)iface);
}

static void WINAPI IWineD3DIndexBufferImpl_PreLoad(IWineD3DIndexBuffer *iface) {
    FIXME("iface %p stub!\n", iface);
}

static void WINAPI IWineD3DIndexBufferImpl_UnLoad(IWineD3DIndexBuffer *iface) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *) iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    TRACE("(%p)\n", This);

    /* This is easy: The whole content is shadowed in This->resource.allocatedMemory,
     * so we only have to destroy the vbo. Only do it if we have a vbo, which implies
     * that vbos are supported.
     * (TODO: Make a IWineD3DBuffer base class for index and vertex buffers and share
     * this code. Also needed for D3D10)
     */
    if(This->vbo) {
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        ENTER_GL();
        GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
        checkGLcall("glDeleteBuffersARB");
        LEAVE_GL();
        This->vbo = 0;
    }
}

static WINED3DRESOURCETYPE WINAPI IWineD3DIndexBufferImpl_GetType(IWineD3DIndexBuffer *iface) {
    return resource_get_type((IWineD3DResource *)iface);
}

static HRESULT WINAPI IWineD3DIndexBufferImpl_GetParent(IWineD3DIndexBuffer *iface, IUnknown **pParent) {
    return resource_get_parent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DIndexBuffer IWineD3DIndexBuffer parts follow
   ****************************************************** */
static HRESULT WINAPI IWineD3DIndexBufferImpl_Lock(IWineD3DIndexBuffer *iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    TRACE("(%p) : offset %d, size %d, Flags=%x\n", This, OffsetToLock, SizeToLock, Flags);

    InterlockedIncrement(&This->lockcount);
    *ppbData = This->resource.allocatedMemory + OffsetToLock;

    if(Flags & (WINED3DLOCK_READONLY | WINED3DLOCK_NO_DIRTY_UPDATE) || This->vbo == 0) {
        return WINED3D_OK;
    }

    if(This->dirtystart != This->dirtyend) {
        if(This->dirtystart > OffsetToLock) This->dirtystart = OffsetToLock;
        if(SizeToLock) {
            if(This->dirtyend < OffsetToLock + SizeToLock) This->dirtyend = OffsetToLock + SizeToLock;
        } else {
            This->dirtyend = This->resource.size;
        }
    } else {
        This->dirtystart = OffsetToLock;
        if(SizeToLock)
            This->dirtyend = OffsetToLock + SizeToLock;
        else
            This->dirtyend = This->resource.size;
    }

    return WINED3D_OK;
}
static HRESULT WINAPI IWineD3DIndexBufferImpl_Unlock(IWineD3DIndexBuffer *iface) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;
    unsigned long locks = InterlockedDecrement(&This->lockcount);
    TRACE("(%p)\n", This);

    /* For now load in unlock */
    if(locks == 0 && This->vbo && (This->dirtyend - This->dirtystart) > 0) {
        IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);

        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,
                This->dirtystart, This->dirtyend - This->dirtystart, This->resource.allocatedMemory + This->dirtystart));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
        This->dirtystart = 0;
        This->dirtyend = 0;
        /* TODO: Move loading into preload when the buffer is used, that avoids dirtifying the state */
        IWineD3DDeviceImpl_MarkStateDirty(device, STATE_INDEXBUFFER);
    }
    return WINED3D_OK;
}
static HRESULT WINAPI IWineD3DIndexBufferImpl_GetDesc(IWineD3DIndexBuffer *iface, WINED3DINDEXBUFFER_DESC *pDesc) {
    IWineD3DIndexBufferImpl *This = (IWineD3DIndexBufferImpl *)iface;

    TRACE("(%p)\n", This);
    pDesc->Format = This->resource.format;
    pDesc->Type   = This->resource.resourceType;
    pDesc->Usage  = This->resource.usage;
    pDesc->Pool   = This->resource.pool;
    pDesc->Size   = This->resource.size;
    return WINED3D_OK;
}

const IWineD3DIndexBufferVtbl IWineD3DIndexBuffer_Vtbl =
{
    /* IUnknown */
    IWineD3DIndexBufferImpl_QueryInterface,
    IWineD3DIndexBufferImpl_AddRef,
    IWineD3DIndexBufferImpl_Release,
    /* IWineD3DResource */
    IWineD3DIndexBufferImpl_GetParent,
    IWineD3DIndexBufferImpl_GetDevice,
    IWineD3DIndexBufferImpl_SetPrivateData,
    IWineD3DIndexBufferImpl_GetPrivateData,
    IWineD3DIndexBufferImpl_FreePrivateData,
    IWineD3DIndexBufferImpl_SetPriority,
    IWineD3DIndexBufferImpl_GetPriority,
    IWineD3DIndexBufferImpl_PreLoad,
    IWineD3DIndexBufferImpl_UnLoad,
    IWineD3DIndexBufferImpl_GetType,
    /* IWineD3DIndexBuffer */
    IWineD3DIndexBufferImpl_Lock,
    IWineD3DIndexBufferImpl_Unlock,
    IWineD3DIndexBufferImpl_GetDesc
};
