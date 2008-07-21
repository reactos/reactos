/*
 * IWineD3DVertexBuffer Implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 *                     Raphael Junqueira
 * Copyright 2004 Christian Costa
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

#define VB_MAXDECLCHANGES     100     /* After that number we stop converting */
#define VB_RESETDECLCHANGE    1000    /* Reset the changecount after that number of draws */

/* *******************************************
   IWineD3DVertexBuffer IUnknown parts follow
   ******************************************* */
static HRESULT WINAPI IWineD3DVertexBufferImpl_QueryInterface(IWineD3DVertexBuffer *iface, REFIID riid, LPVOID *ppobj)
{
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    TRACE("(%p)->(%s,%p)\n",This,debugstr_guid(riid),ppobj);
    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IWineD3DBase)
        || IsEqualGUID(riid, &IID_IWineD3DResource)
        || IsEqualGUID(riid, &IID_IWineD3DVertexBuffer)){
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IWineD3DVertexBufferImpl_AddRef(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->resource.ref);
    TRACE("(%p) : AddRef increasing from %d\n", This, ref - 1);
    return ref;
}

static ULONG WINAPI IWineD3DVertexBufferImpl_Release(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->resource.ref);
    TRACE("(%p) : Releasing from %d\n", This, ref + 1);
    if (ref == 0) {

        if(This->vbo) {
            IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
        }

        IWineD3DResourceImpl_CleanUp((IWineD3DResource *)iface);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* ****************************************************
   IWineD3DVertexBuffer IWineD3DResource parts follow
   **************************************************** */
static HRESULT WINAPI IWineD3DVertexBufferImpl_GetDevice(IWineD3DVertexBuffer *iface, IWineD3DDevice** ppDevice) {
    return IWineD3DResourceImpl_GetDevice((IWineD3DResource *)iface, ppDevice);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_SetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, CONST void* pData, DWORD SizeOfData, DWORD Flags) {
    return IWineD3DResourceImpl_SetPrivateData((IWineD3DResource *)iface, refguid, pData, SizeOfData, Flags);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_GetPrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid, void* pData, DWORD* pSizeOfData) {
    return IWineD3DResourceImpl_GetPrivateData((IWineD3DResource *)iface, refguid, pData, pSizeOfData);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_FreePrivateData(IWineD3DVertexBuffer *iface, REFGUID refguid) {
    return IWineD3DResourceImpl_FreePrivateData((IWineD3DResource *)iface, refguid);
}

static DWORD    WINAPI IWineD3DVertexBufferImpl_SetPriority(IWineD3DVertexBuffer *iface, DWORD PriorityNew) {
    return IWineD3DResourceImpl_SetPriority((IWineD3DResource *)iface, PriorityNew);
}

static DWORD    WINAPI IWineD3DVertexBufferImpl_GetPriority(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetPriority((IWineD3DResource *)iface);
}

static void fixup_vertices(
	BYTE *src, BYTE *dst,
	int stride,
	int num,
	int pos, BOOL haspos,
	int diffuse, BOOL hasdiffuse,
	int specular, BOOL hasspecular
) {
    int i;
    float x, y, z, w;

    for(i = num - 1; i >= 0; i--) {
        if(haspos) {
            float *p = (float *) ((src + pos) + i * stride);

            /* rhw conversion like in drawStridedSlow */
            if(p[3] == 1.0 || ((p[3] < eps) && (p[3] > -eps))) {
                x = p[0];
                y = p[1];
                z = p[2];
                w = 1.0;
            } else {
                w = 1.0 / p[3];
                x = p[0] * w;
                y = p[1] * w;
                z = p[2] * w;
            }
            p = (float *) (dst + i * stride + pos);
            p[0] = x;
            p[1] = y;
            p[2] = z;
            p[3] = w;
        }
        if(hasdiffuse) {
            DWORD srcColor, *dstColor = (DWORD *) (dst + i * stride + diffuse);
            srcColor = * (DWORD *) ( (src + diffuse) + i * stride);

            /* Color conversion like in drawStridedSlow. watch out for little endianity
            * If we want that stuff to work on big endian machines too we have to consider more things
            *
            * 0xff000000: Alpha mask
            * 0x00ff0000: Blue mask
            * 0x0000ff00: Green mask
            * 0x000000ff: Red mask
            */

            *dstColor = 0;
            *dstColor |= (srcColor & 0xff00ff00)      ;   /* Alpha Green */
            *dstColor |= (srcColor & 0x00ff0000) >> 16;   /* Red */
            *dstColor |= (srcColor & 0x000000ff) << 16;   /* Blue */
        }
        if(hasspecular) {
            DWORD srcColor, *dstColor = (DWORD *) (dst + i * stride + specular);
            srcColor = * (DWORD *) ( (src + specular) + i * stride);

            /* Similar to diffuse
             * TODO: Write the alpha value out for fog coords
             */
            *dstColor = 0;
            *dstColor |= (srcColor & 0xff00ff00)      ;   /* Alpha Green */
            *dstColor |= (srcColor & 0x00ff0000) >> 16;   /* Red */
            *dstColor |= (srcColor & 0x000000ff) << 16;   /* Blue */
        }
    }
}

inline BOOL WINAPI IWineD3DVertexBufferImpl_FindDecl(IWineD3DVertexBufferImpl *This)
{
    WineDirect3DVertexStridedData strided;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;

    /* In d3d7 the vertex buffer declaration NEVER changes because it is stored in the d3d7 vertex buffer.
     * Once we have our declaration there is no need to look it up again.
     */
    if(((IWineD3DImpl *)device->wineD3D)->dxVersion == 7 && This->Flags & VBFLAG_HASDESC) {
        return FALSE;
    }

    /* There are certain vertex data types that need to be fixed up. The Vertex Buffers FVF doesn't
     * help finding them, only the vertex declaration or the device FVF can determine that at drawPrim
     * time. Rules are as follows:
     *
     * -> No modification when Vertex Shaders are used
     * -> Fix up position1 and position 2 if they are XYZRHW
     * -> Fix up diffuse color
     * -> Fix up specular color
     *
     * The Declaration is only known at drawing time, and it can change from draw to draw. If any converted values
     * are changed, the whole buffer has to be reconverted and reloaded. (Converting is SLOW, so if this happens too
     * often PreLoad stops converting entirely and falls back to drawStridedSlow).
     *
     * Reconvert if:
     * -> New semantics that have to be converted appear
     * -> The position of semantics that have to be converted changes
     * -> The stride of the vertex changed AND there is stuff that needs conversion
     * -> (If a vertex shader is bound and in use assume that nothing that needs conversion is there)
     *
     * Return values:
     *  TRUE: Reload is needed
     *  FALSE: otherwise
     */

    if (use_vs(device)) {
        /* Assume no conversion */
        memset(&strided, 0, sizeof(strided));
    } else {
        /* we need a copy because we modify some params */
        memcpy(&strided, &device->strided_streams, sizeof(strided));

        /* Filter out data that does not come from this VBO */
        if(strided.u.s.position.VBO != This->vbo)    memset(&strided.u.s.position, 0, sizeof(strided.u.s.position));
        if(strided.u.s.diffuse.VBO != This->vbo)     memset(&strided.u.s.diffuse, 0, sizeof(strided.u.s.diffuse));
        if(strided.u.s.specular.VBO != This->vbo)    memset(&strided.u.s.specular, 0, sizeof(strided.u.s.specular));
        if(strided.u.s.position2.VBO != This->vbo)   memset(&strided.u.s.position2, 0, sizeof(strided.u.s.position2));
    }

    /* We have a declaration now in the buffer */
    This->Flags |= VBFLAG_HASDESC;

    /* Find out if reload is needed
     * Position of the semantic in the vertex and the stride must be equal to the stored type. Don't mind if only unconverted stuff changed.
     *
     * If some stuff does not exist in the buffer, then lpData, dwStride and dwType are memsetted to 0. So if the semantic didn't exist before
     * and does not exist now all 3 values will be equal(=0).
     *
     * Checking the lpData field alone is not enough, because data may appear at offset 0 in the buffer. This is the same location as nonexistent
     * data uses, so we have to check the type and stride too. Colors can be at offset 0 too, because it is perfectly fine to render from 2 or more
     * buffers at the same time and get the position from one and the color from the other buffer.
     */
    if( /* Position transformed vs untransformed */
        ((This->strided.u.s.position_transformed || strided.u.s.position_transformed) &&
          (This->strided.u.s.position.lpData != strided.u.s.position.lpData ||
           This->strided.u.s.position.dwType != strided.u.s.position.dwType)) ||
        /* Diffuse position and data type */
        This->strided.u.s.diffuse.lpData != strided.u.s.diffuse.lpData || This->strided.u.s.diffuse.dwStride != strided.u.s.diffuse.dwStride ||
         This->strided.u.s.diffuse.dwType != strided.u.s.diffuse.dwType ||
        /* Specular position and data type */
        This->strided.u.s.specular.lpData != strided.u.s.specular.lpData || This->strided.u.s.specular.dwStride != strided.u.s.specular.dwStride ||
         This->strided.u.s.specular.dwType != strided.u.s.specular.dwType) {

        TRACE("Declaration changed, reloading buffer\n");
        /* Set the new description */
        memcpy(&This->strided, &strided, sizeof(strided));
        return TRUE;
    }

    return FALSE;
}

static void     WINAPI IWineD3DVertexBufferImpl_PreLoad(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    IWineD3DDeviceImpl *device = This->resource.wineD3DDevice;
    BYTE *data;
    UINT start = 0, end = 0, stride = 0;
    BOOL declChanged = FALSE;
    TRACE("(%p)->()\n", This);

    if(This->Flags & VBFLAG_LOAD) {
        return; /* Already doing that stuff */
    }

    if(!This->vbo) {
        /* TODO: Make converting independent from VBOs */
        return; /* Not doing any conversion */
    }

    /* Reading the declaration makes only sense if the stateblock is finalized and the buffer bound to a stream */
    if(device->isInDraw && This->bindCount > 0) {
        declChanged = IWineD3DVertexBufferImpl_FindDecl(This);
    } else if(This->Flags & VBFLAG_HASDESC) {
        /* Reuse the declaration stored in the buffer. It will most likely not change, and if it does
         * the stream source state handler will call PreLoad again and the change will be cought
         */
    } else {
        /* Cannot get a declaration, and no declaration is stored in the buffer. It is pointless to preload
         * now. When the buffer is used, PreLoad will be called by the stream source state handler and a valid
         * declaration for the buffer can be found
         */
        return;
    }

    /* If applications change the declaration over and over, reconverting all the time is a huge
     * performance hit. So count the declaration changes and release the VBO if there are too much
     * of them(and thus stop converting)
     */
    if(declChanged) {
        This->declChanges++;
        This->draws = 0;

        if(This->declChanges > VB_MAXDECLCHANGES) {
            FIXME("Too much declaration changes, stopping converting\n");
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
            ENTER_GL();
            GL_EXTCALL(glDeleteBuffersARB(1, &This->vbo));
            checkGLcall("glDeleteBuffersARB");
            LEAVE_GL();
            This->vbo = 0;

            /* The stream source state handler might have read the memory of the vertex buffer already
             * and got the memory in the vbo which is not valid any longer. Dirtify the stream source
             * to force a reload. This happens only once per changed vertexbuffer and should occur rather
             * rarely
             */
            IWineD3DDeviceImpl_MarkStateDirty(device, STATE_STREAMSRC);

            return;
        }
    } else {
        /* However, it is perfectly fine to change the declaration every now and then. We don't want a game that
         * changes it every minute drop the VBO after VB_MAX_DECL_CHANGES minutes. So count draws without
         * decl changes and reset the decl change count after a specific number of them
         */
        This->draws++;
        if(This->draws > VB_RESETDECLCHANGE) This->declChanges = 0;
    }

    if(declChanged) {
        /* The declaration changed, reload the whole buffer */
        WARN("Reloading buffer because of decl change\n");
        start = 0;
        end = This->resource.size;
    } else if(This->Flags & VBFLAG_DIRTY) {
        /* No decl change, but dirty data, reload the changed stuff */
        start = This->dirtystart;
        end = This->dirtyend;
    } else {
        /* Desc not changed, buffer not dirty, nothing to do :-) */
        return;
    }

    /* Mark the buffer clean */
    This->Flags &= ~VBFLAG_DIRTY;
    This->dirtystart = 0;
    This->dirtyend = 0;

    if     (This->strided.u.s.position.dwStride) stride = This->strided.u.s.position.dwStride;
    else if(This->strided.u.s.specular.dwStride) stride = This->strided.u.s.specular.dwStride;
    else if(This->strided.u.s.diffuse.dwStride)  stride = This->strided.u.s.diffuse.dwStride;
    else {
        /* That means that there is nothing to fixup. Just upload from This->resource.allocatedMemory
         * directly into the vbo. Do not free the system memory copy because drawPrimitive may need it if
         * the stride is 0, for instancing emulation, vertex blending emulation or shader emulation.
         */
        TRACE("No conversion needed, locking directly into the VBO in future\n");

        if(!device->isInDraw) {
            ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
        }
        ENTER_GL();
        GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
        checkGLcall("glBindBufferARB");
        GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start, end-start, This->resource.allocatedMemory + start));
        checkGLcall("glBufferSubDataARB");
        LEAVE_GL();
        return;
    }

    /* OK, we have the original data from the app, the description of the buffer and the dirty area.
     * so convert the stuff
     */
    data = HeapAlloc(GetProcessHeap(), 0, end-start);
    if(!data) {
        ERR("Out of memory\n");
        return;
    }
    memcpy(data, This->resource.allocatedMemory + start, end - start);

    fixup_vertices(data, data, stride, ( end - start) / stride,
                   /* Position */
                   (int)This->strided.u.s.position.lpData, /* Data location */
                   This->strided.u.s.position_transformed, /* Do convert? */
                   /* Diffuse color */
                   (int)This->strided.u.s.diffuse.lpData, /* Location */
                   This->strided.u.s.diffuse.dwType == WINED3DDECLTYPE_SHORT4 || This->strided.u.s.diffuse.dwType == WINED3DDECLTYPE_D3DCOLOR, /* Convert? */
                   /* specular color */
                   (int)This->strided.u.s.specular.lpData, /* location */
                   This->strided.u.s.specular.dwType == WINED3DDECLTYPE_SHORT4 || This->strided.u.s.specular.dwType == WINED3DDECLTYPE_D3DCOLOR);

    if(!device->isInDraw) {
        ActivateContext(device, device->lastActiveRenderTarget, CTXUSAGE_RESOURCELOAD);
    }
    ENTER_GL();
    GL_EXTCALL(glBindBufferARB(GL_ARRAY_BUFFER_ARB, This->vbo));
    checkGLcall("glBindBufferARB");
    GL_EXTCALL(glBufferSubDataARB(GL_ARRAY_BUFFER_ARB, start, end - start, data));
    checkGLcall("glBufferSubDataARB");
    LEAVE_GL();

    HeapFree(GetProcessHeap(), 0, data);
}

static WINED3DRESOURCETYPE WINAPI IWineD3DVertexBufferImpl_GetType(IWineD3DVertexBuffer *iface) {
    return IWineD3DResourceImpl_GetType((IWineD3DResource *)iface);
}

static HRESULT WINAPI IWineD3DVertexBufferImpl_GetParent(IWineD3DVertexBuffer *iface, IUnknown **pParent) {
    return IWineD3DResourceImpl_GetParent((IWineD3DResource *)iface, pParent);
}

/* ******************************************************
   IWineD3DVertexBuffer IWineD3DVertexBuffer parts follow
   ****************************************************** */
static HRESULT  WINAPI IWineD3DVertexBufferImpl_Lock(IWineD3DVertexBuffer *iface, UINT OffsetToLock, UINT SizeToLock, BYTE** ppbData, DWORD Flags) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;
    BYTE *data;
    TRACE("(%p)->%d, %d, %p, %08x\n", This, OffsetToLock, SizeToLock, ppbData, Flags);

    InterlockedIncrement(&This->lockcount);

    if(This->Flags & VBFLAG_DIRTY) {
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

    data = This->resource.allocatedMemory;
    This->Flags |= VBFLAG_DIRTY;
    *ppbData = data + OffsetToLock;

    TRACE("(%p) : returning memory of %p (base:%p,offset:%u)\n", This, data + OffsetToLock, data, OffsetToLock);
    /* TODO: check Flags compatibility with This->currentDesc.Usage (see MSDN) */
    return WINED3D_OK;
}
HRESULT  WINAPI IWineD3DVertexBufferImpl_Unlock(IWineD3DVertexBuffer *iface) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *) iface;
    LONG lockcount;
    TRACE("(%p)\n", This);

    lockcount = InterlockedDecrement(&This->lockcount);
    if(lockcount > 0) {
        /* Delay loading the buffer until everything is unlocked */
        TRACE("Ignoring the unlock\n");
        return WINED3D_OK;
    }

    if(This->Flags & VBFLAG_HASDESC) {
        IWineD3DVertexBufferImpl_PreLoad(iface);
    }
    return WINED3D_OK;
}
static HRESULT  WINAPI IWineD3DVertexBufferImpl_GetDesc(IWineD3DVertexBuffer *iface, WINED3DVERTEXBUFFER_DESC *pDesc) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    TRACE("(%p)\n", This);
    pDesc->Format = This->resource.format;
    pDesc->Type   = This->resource.resourceType;
    pDesc->Usage  = This->resource.usage;
    pDesc->Pool   = This->resource.pool;
    pDesc->Size   = This->resource.size;
    pDesc->FVF    = This->fvf;
    return WINED3D_OK;
}

const IWineD3DVertexBufferVtbl IWineD3DVertexBuffer_Vtbl =
{
    /* IUnknown */
    IWineD3DVertexBufferImpl_QueryInterface,
    IWineD3DVertexBufferImpl_AddRef,
    IWineD3DVertexBufferImpl_Release,
    /* IWineD3DResource */
    IWineD3DVertexBufferImpl_GetParent,
    IWineD3DVertexBufferImpl_GetDevice,
    IWineD3DVertexBufferImpl_SetPrivateData,
    IWineD3DVertexBufferImpl_GetPrivateData,
    IWineD3DVertexBufferImpl_FreePrivateData,
    IWineD3DVertexBufferImpl_SetPriority,
    IWineD3DVertexBufferImpl_GetPriority,
    IWineD3DVertexBufferImpl_PreLoad,
    IWineD3DVertexBufferImpl_GetType,
    /* IWineD3DVertexBuffer */
    IWineD3DVertexBufferImpl_Lock,
    IWineD3DVertexBufferImpl_Unlock,
    IWineD3DVertexBufferImpl_GetDesc
};

BYTE* WINAPI IWineD3DVertexBufferImpl_GetMemory(IWineD3DVertexBuffer* iface, DWORD iOffset, GLint *vbo) {
    IWineD3DVertexBufferImpl *This = (IWineD3DVertexBufferImpl *)iface;

    *vbo = This->vbo;
    if(This->vbo == 0) {
        return This->resource.allocatedMemory + iOffset;
    } else {
        return (BYTE *) iOffset;
    }
}

HRESULT WINAPI IWineD3DVertexBufferImpl_ReleaseMemory(IWineD3DVertexBuffer* iface) {
  return WINED3D_OK;
}
