/* Direct3D Vertex Buffer
 * Copyright (c) 2002 Lionel ULMER
 * Copyright (c) 2006 Stefan DÖSINGER
 *
 * This file contains the implementation of Direct3DVertexBuffer COM object
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
#include "wine/port.h"
#include "wine/debug.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"

#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d7);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_thunk);

/*****************************************************************************
 * IUnknown Methods
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DVertexBuffer7::QueryInterface
 *
 * The QueryInterface Method for Vertex Buffers
 * For a link to QueryInterface rules, see IDirectDraw7::QueryInterface
 *
 * Params
 *  riid: Queryied Interface id
 *  obj: Address to return the interface pointer
 *
 * Returns:
 *  S_OK on success
 *  E_NOINTERFACE if the interface wasn't found
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DVertexBufferImpl_QueryInterface(IDirect3DVertexBuffer7 *iface,
                                         REFIID riid,
                                         void  **obj)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(riid), obj);

    /* By default, set the object pointer to NULL */
    *obj = NULL;

    if ( IsEqualGUID( &IID_IUnknown,  riid ) )
    {
        IUnknown_AddRef(iface);
        *obj = iface;
        TRACE("  Creating IUnknown interface at %p.\n", *obj);
        return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DVertexBuffer, riid ) )
    {
        IUnknown_AddRef(iface);
        *obj = &This->IDirect3DVertexBuffer_vtbl;
        TRACE("  Creating IDirect3DVertexBuffer interface %p\n", *obj);
        return S_OK;
    }
    if ( IsEqualGUID( &IID_IDirect3DVertexBuffer7, riid ) )
    {
        IUnknown_AddRef(iface);
        *obj = iface;
        TRACE("  Creating IDirect3DVertexBuffer7 interface %p\n", *obj);
        return S_OK;
    }
    FIXME("(%p): interface for IID %s NOT found!\n", This, debugstr_guid(riid));
    return E_NOINTERFACE;
}

static HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_QueryInterface(IDirect3DVertexBuffer *iface,
                                                 REFIID riid,
                                                 void **obj)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DVertexBuffer7 interface.\n", This, debugstr_guid(riid), obj);

    return IDirect3DVertexBuffer7_QueryInterface((IDirect3DVertexBuffer7 *)This, riid, obj);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::AddRef
 *
 * AddRef for Vertex Buffers
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DVertexBufferImpl_AddRef(IDirect3DVertexBuffer7 *iface)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p/%p)->() incrementing from %u.\n", This, iface, ref - 1);

    return ref;
}

static ULONG WINAPI
Thunk_IDirect3DVertexBufferImpl_1_AddRef(IDirect3DVertexBuffer *iface)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DVertexBuffer7 interface.\n", This);

    return IDirect3DVertexBuffer7_AddRef((IDirect3DVertexBuffer7 *)This);
}


/*****************************************************************************
 * IDirect3DVertexBuffer7::Release
 *
 * Release for Vertex Buffers
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DVertexBufferImpl_Release(IDirect3DVertexBuffer7 *iface)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref + 1);

    if (ref == 0)
    {
        IWineD3DBuffer *curVB = NULL;
        UINT offset, stride;

        EnterCriticalSection(&ddraw_cs);
        /* D3D7 Vertex buffers don't stay bound in the device, they are passed as a parameter
         * to drawPrimitiveVB. DrawPrimitiveVB sets them as the stream source in wined3d,
         * and they should get unset there before they are destroyed
         */
        IWineD3DDevice_GetStreamSource(This->ddraw->wineD3DDevice,
                                       0 /* Stream number */,
                                       &curVB,
                                       &offset,
                                       &stride);
        if(curVB == This->wineD3DVertexBuffer)
        {
            IWineD3DDevice_SetStreamSource(This->ddraw->wineD3DDevice,
                                        0 /* Steam number */,
                                        NULL /* stream data */,
                                        0 /* Offset */,
                                        0 /* stride */);
        }
        if(curVB)
        {
            IWineD3DBuffer_Release(curVB); /* For the GetStreamSource */
        }

        IWineD3DVertexDeclaration_Release(This->wineD3DVertexDeclaration);
        IWineD3DBuffer_Release(This->wineD3DVertexBuffer);
        LeaveCriticalSection(&ddraw_cs);
        HeapFree(GetProcessHeap(), 0, This);

        return 0;
    }
    return ref;
}

static ULONG WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Release(IDirect3DVertexBuffer *iface)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DVertexBuffer7 interface.\n", This);

    return IDirect3DVertexBuffer7_Release((IDirect3DVertexBuffer7 *)This);
}

/*****************************************************************************
 * IDirect3DVertexBuffer Methods
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DVertexBuffer7::Lock
 *
 * Locks the vertex buffer and returns a pointer to the vertex data
 * Locking vertex buffers is similar to locking surfaces, because Windows
 * uses surfaces to store vertex data internally (According to the DX sdk)
 *
 * Params:
 *  Flags: Locking flags. Relevant here are DDLOCK_READONLY, DDLOCK_WRITEONLY,
 *         DDLOCK_DISCARDCONTENTS and DDLOCK_NOOVERWRITE.
 *  Data:  Returns a pointer to the vertex data
 *  Size:  Returns the size of the buffer if not NULL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Data is NULL
 *  D3DERR_VERTEXBUFFEROPTIMIZED if called on an optimized buffer(WineD3D)
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DVertexBufferImpl_Lock(IDirect3DVertexBuffer7 *iface,
                               DWORD Flags,
                               void **Data,
                               DWORD *Size)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    WINED3DBUFFER_DESC Desc;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p,%p)\n", This, Flags, Data, Size);

    EnterCriticalSection(&ddraw_cs);
    if(Size)
    {
        /* Get the size, for returning it, and for locking */
        hr = IWineD3DBuffer_GetDesc(This->wineD3DVertexBuffer, &Desc);
        if(hr != D3D_OK)
        {
            ERR("(%p) IWineD3DBuffer::GetDesc failed with hr=%08x\n", This, hr);
            LeaveCriticalSection(&ddraw_cs);
            return hr;
        }
        *Size = Desc.Size;
    }

    hr = IWineD3DBuffer_Map(This->wineD3DVertexBuffer, 0 /* OffsetToLock */,
            0 /* SizeToLock, 0 == Full lock */, (BYTE **)Data, Flags);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Lock(IDirect3DVertexBuffer *iface,
                                       DWORD Flags,
                                       void **Data,
                                       DWORD *Size)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p,%p) thunking to IDirect3DVertexBuffer7 interface.\n", This, Flags, Data, Size);

    return IDirect3DVertexBuffer7_Lock((IDirect3DVertexBuffer7 *)This, Flags, Data, Size);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::Unlock
 *
 * Unlocks a vertex Buffer
 *
 * Returns:
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DVertexBufferImpl_Unlock(IDirect3DVertexBuffer7 *iface)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->()\n", This);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DBuffer_Unmap(This->wineD3DVertexBuffer);
    LeaveCriticalSection(&ddraw_cs);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Unlock(IDirect3DVertexBuffer *iface)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DVertexBuffer7 interface.\n", This);

    return IDirect3DVertexBuffer7_Unlock((IDirect3DVertexBuffer7 *)This);
}


/*****************************************************************************
 * IDirect3DVertexBuffer7::ProcessVertices
 *
 * Processes untransformed Vertices into a transformed or optimized vertex
 * buffer. It can also perform other operations, such as lighting or clipping
 *
 * Params
 *  VertexOp: Operation(s) to perform: D3DVOP_CLIP, _EXTENTS, _LIGHT, _TRANSFORM
 *  DestIndex: Index in the destination buffer(This), where the vertices are
 *             placed
 *  Count: Number of Vertices in the Source buffer to process
 *  SrcBuffer: Source vertex buffer
 *  SrcIndex: Index of the first vertex in the src buffer to process
 *  D3DDevice: Device to use for transformation
 *  Flags: 0 for default, D3DPV_DONOTCOPYDATA to prevent copying
 *         unchaned vertices
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS If D3DVOP_TRANSFORM wasn't passed
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DVertexBufferImpl_ProcessVertices(IDirect3DVertexBuffer7 *iface,
                                          DWORD VertexOp,
                                          DWORD DestIndex,
                                          DWORD Count,
                                          IDirect3DVertexBuffer7 *SrcBuffer,
                                          DWORD SrcIndex,
                                          IDirect3DDevice7 *D3DDevice,
                                          DWORD Flags)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    IDirect3DVertexBufferImpl *Src = (IDirect3DVertexBufferImpl *)SrcBuffer;
    IDirect3DDeviceImpl *D3D = (IDirect3DDeviceImpl *)D3DDevice;
    BOOL oldClip, doClip;
    HRESULT hr;

    TRACE("(%p)->(%08x,%d,%d,%p,%d,%p,%08x)\n", This, VertexOp, DestIndex, Count, Src, SrcIndex, D3D, Flags);

    /* Vertex operations:
     * D3DVOP_CLIP: Clips vertices outside the viewing frustrum. Needs clipping information
     * in the vertex buffer (Buffer may not be created with D3DVBCAPS_DONOTCLIP)
     * D3DVOP_EXTENTS: Causes the screen extents to be updated when rendering the vertices
     * D3DVOP_LIGHT: Lights the vertices
     * D3DVOP_TRANSFORM: Transform the vertices. This flag is necessary
     *
     * WineD3D only transforms and clips the vertices by now, so EXTENTS and LIGHT
     * are not implemented. Clipping is disabled ATM, because of unsure conditions.
     */
    if( !(VertexOp & D3DVOP_TRANSFORM) ) return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    /* WineD3D doesn't know d3d7 vertex operation, it uses
     * render states instead. Set the render states according to
     * the vertex ops
     */
    doClip = VertexOp & D3DVOP_CLIP ? TRUE : FALSE;
    IWineD3DDevice_GetRenderState(D3D->wineD3DDevice,
                                  WINED3DRS_CLIPPING,
                                  (DWORD *) &oldClip);
    if(doClip != oldClip)
    {
        IWineD3DDevice_SetRenderState(D3D->wineD3DDevice,
                                      WINED3DRS_CLIPPING,
                                      doClip);
    }

    IWineD3DDevice_SetStreamSource(D3D->wineD3DDevice,
                                   0, /* Stream No */
                                   Src->wineD3DVertexBuffer,
                                   0, /* Offset */
                                   get_flexible_vertex_size(Src->fvf));
    IWineD3DDevice_SetVertexDeclaration(D3D->wineD3DDevice,
                                        Src->wineD3DVertexDeclaration);
    hr = IWineD3DDevice_ProcessVertices(D3D->wineD3DDevice,
                                        SrcIndex,
                                        DestIndex,
                                        Count,
                                        This->wineD3DVertexBuffer,
                                        NULL /* Output vdecl */,
                                        Flags,
                                        This->fvf);

    /* Restore the states if needed */
    if(doClip != oldClip)
        IWineD3DDevice_SetRenderState(D3D->wineD3DDevice,
                                      WINED3DRS_CLIPPING,
                                      oldClip);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_ProcessVertices(IDirect3DVertexBuffer *iface,
                                                  DWORD VertexOp,
                                                  DWORD DestIndex,
                                                  DWORD Count,
                                                  IDirect3DVertexBuffer *SrcBuffer,
                                                  DWORD SrcIndex,
                                                  IDirect3DDevice3 *D3DDevice,
                                                  DWORD Flags)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    IDirect3DVertexBufferImpl *Src = SrcBuffer ? vb_from_vb1(SrcBuffer) : NULL;
    IDirect3DDeviceImpl *D3D = D3DDevice ? device_from_device3(D3DDevice) : NULL;

    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%08x,%p,%08x,%p,%08x) thunking to IDirect3DVertexBuffer7 interface.\n", This, VertexOp, DestIndex, Count, Src, SrcIndex, D3D, Flags);

    return IDirect3DVertexBuffer7_ProcessVertices((IDirect3DVertexBuffer7 *)This, VertexOp, DestIndex,
            Count, (IDirect3DVertexBuffer7 *)Src, SrcIndex, (IDirect3DDevice7 *)D3D, Flags);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::GetVertexBufferDesc
 *
 * Returns the description of a vertex buffer
 *
 * Params:
 *  Desc: Address to write the description to
 *
 * Returns
 *  DDERR_INVALIDPARAMS if Desc is NULL
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DVertexBufferImpl_GetVertexBufferDesc(IDirect3DVertexBuffer7 *iface,
                                              D3DVERTEXBUFFERDESC *Desc)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    WINED3DBUFFER_DESC WDesc;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, Desc);

    if(!Desc) return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DBuffer_GetDesc(This->wineD3DVertexBuffer, &WDesc);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DBuffer::GetDesc failed with hr=%08x\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* Now fill the Desc structure */
    Desc->dwCaps = This->Caps;
    Desc->dwFVF = This->fvf;
    Desc->dwNumVertices = WDesc.Size / get_flexible_vertex_size(This->fvf);
    LeaveCriticalSection(&ddraw_cs);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_GetVertexBufferDesc(IDirect3DVertexBuffer *iface,
                                                      D3DVERTEXBUFFERDESC *Desc)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DVertexBuffer7 interface.\n", This, Desc);

    return IDirect3DVertexBuffer7_GetVertexBufferDesc((IDirect3DVertexBuffer7 *)This, Desc);
}


/*****************************************************************************
 * IDirect3DVertexBuffer7::Optimize
 *
 * Converts an unoptimized vertex buffer into an optimized buffer
 *
 * Params:
 *  D3DDevice: Device for which this buffer is optimized
 *  Flags: Not used, should be set to 0
 *
 * Returns
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DVertexBufferImpl_Optimize(IDirect3DVertexBuffer7 *iface,
                                   IDirect3DDevice7 *D3DDevice,
                                   DWORD Flags)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    IDirect3DDeviceImpl *D3D = (IDirect3DDeviceImpl *)D3DDevice;
    static BOOL hide = FALSE;

    if (!hide)
    {
        FIXME("(%p)->(%p,%08x): stub!\n", This, D3D, Flags);
        hide = TRUE;
    }

    /* We could forward this call to WineD3D and take advantage
     * of it once we use OpenGL vertex buffers
     */
    EnterCriticalSection(&ddraw_cs);
    This->Caps |= D3DVBCAPS_OPTIMIZED;
    LeaveCriticalSection(&ddraw_cs);

    return DD_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DVertexBufferImpl_1_Optimize(IDirect3DVertexBuffer *iface,
                                           IDirect3DDevice3 *D3DDevice,
                                           DWORD Flags)
{
    IDirect3DVertexBufferImpl *This = vb_from_vb1(iface);
    IDirect3DDeviceImpl *D3D = D3DDevice ? device_from_device3(D3DDevice) : NULL;
    TRACE_(ddraw_thunk)("(%p)->(%p,%08x) thunking to IDirect3DVertexBuffer7 interface.\n", This, D3D, Flags);

    return IDirect3DVertexBuffer7_Optimize((IDirect3DVertexBuffer7 *)This, (IDirect3DDevice7 *)D3D, Flags);
}

/*****************************************************************************
 * IDirect3DVertexBuffer7::ProcessVerticesStrided
 *
 * This method processes untransformed strided vertices into a processed
 * or optimized vertex buffer.
 *
 * For more details on the parameters, see
 * IDirect3DVertexBuffer7::ProcessVertices
 *
 * Params:
 *  VertexOp: Operations to perform
 *  DestIndex: Destination index to write the vertices to
 *  Count: Number of input vertices
 *  StrideData: Array containing the input vertices
 *  VertexTypeDesc: Vertex Description or source index?????????
 *  D3DDevice: IDirect3DDevice7 to use for processing
 *  Flags: Can be D3DPV_DONOTCOPYDATA to avoid copying unmodified vertices
 *
 * Returns
 *  D3D_OK on success, or DDERR_*
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DVertexBufferImpl_ProcessVerticesStrided(IDirect3DVertexBuffer7 *iface,
                                                 DWORD VertexOp,
                                                 DWORD DestIndex,
                                                 DWORD Count,
                                                 D3DDRAWPRIMITIVESTRIDEDDATA *StrideData,
                                                 DWORD VertexTypeDesc,
                                                 IDirect3DDevice7 *D3DDevice,
                                                 DWORD Flags)
{
    IDirect3DVertexBufferImpl *This = (IDirect3DVertexBufferImpl *)iface;
    IDirect3DDeviceImpl *D3D = (IDirect3DDeviceImpl *)D3DDevice;
    FIXME("(%p)->(%08x,%08x,%08x,%p,%08x,%p,%08x): stub!\n", This, VertexOp, DestIndex, Count, StrideData, VertexTypeDesc, D3D, Flags);
    return DD_OK;
}

/*****************************************************************************
 * The VTables
 *****************************************************************************/

const IDirect3DVertexBuffer7Vtbl IDirect3DVertexBuffer7_Vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DVertexBufferImpl_QueryInterface,
    IDirect3DVertexBufferImpl_AddRef,
    IDirect3DVertexBufferImpl_Release,
    /*** IDirect3DVertexBuffer Methods ***/
    IDirect3DVertexBufferImpl_Lock,
    IDirect3DVertexBufferImpl_Unlock,
    IDirect3DVertexBufferImpl_ProcessVertices,
    IDirect3DVertexBufferImpl_GetVertexBufferDesc,
    IDirect3DVertexBufferImpl_Optimize,
    /*** IDirect3DVertexBuffer7 Methods ***/
    IDirect3DVertexBufferImpl_ProcessVerticesStrided
};

const IDirect3DVertexBufferVtbl IDirect3DVertexBuffer1_Vtbl =
{
    /*** IUnknown Methods ***/
    Thunk_IDirect3DVertexBufferImpl_1_QueryInterface,
    Thunk_IDirect3DVertexBufferImpl_1_AddRef,
    Thunk_IDirect3DVertexBufferImpl_1_Release,
    /*** IDirect3DVertexBuffer Methods ***/
    Thunk_IDirect3DVertexBufferImpl_1_Lock,
    Thunk_IDirect3DVertexBufferImpl_1_Unlock,
    Thunk_IDirect3DVertexBufferImpl_1_ProcessVertices,
    Thunk_IDirect3DVertexBufferImpl_1_GetVertexBufferDesc,
    Thunk_IDirect3DVertexBufferImpl_1_Optimize
};
