/*
 * Copyright (c) 1998-2004 Lionel Ulmer
 * Copyright (c) 2002-2005 Christian Costa
 * Copyright (c) 2006 Stefan Dösinger
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
 *
 * IDirect3DDevice implementation, version 1, 2, 3 and 7. Rendering is relayed
 * to WineD3D, some minimal DirectDraw specific management is handled here.
 * The Direct3DDevice is NOT the parent of the WineD3DDevice, because d3d
 * is initialized when DirectDraw creates the primary surface.
 * Some type management is necessary, because some D3D types changed between
 * D3D7 and D3D9.
 *
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define COBJMACROS

//#ifndef WINE_NATIVEWIN32
# include "windef.h"
# include "winbase.h"
# include "winnls.h"
# include "winerror.h"
# include "wingdi.h"
//#endif


#include "ddraw.h"
#include "d3d.h"

#include "ddraw_private.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d7);
WINE_DECLARE_DEBUG_CHANNEL(ddraw_thunk);

/* The device ID */
const GUID IID_D3DDEVICE_WineD3D = {
  0xaef72d43,
  0xb09a,
  0x4b7b,
  { 0xb7,0x98,0xc6,0x8a,0x77,0x2d,0x72,0x2a }
};

/*****************************************************************************
 * IUnknown Methods. Common for Version 1, 2, 3 and 7 
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DDevice7::QueryInterface
 *
 * Used to query other interfaces from a Direct3DDevice interface.
 * It can return interface pointers to all Direct3DDevice versions as well
 * as IDirectDraw and IDirect3D. For a link to QueryInterface
 * rules see ddraw.c, IDirectDraw7::QueryInterface
 *
 * Exists in Version 1, 2, 3 and 7
 *
 * Params:
 *  refiid: Interface ID queried for
 *  obj: Used to return the interface pointer
 *
 * Returns:
 *  D3D_OK or E_NOINTERFACE
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_QueryInterface(IDirect3DDevice7 *iface,
                                     REFIID refiid,
                                     void **obj)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(refiid), obj);

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;

    if(!refiid)
        return DDERR_INVALIDPARAMS;

    if ( IsEqualGUID( &IID_IUnknown, refiid ) )
    {
        *obj = ICOM_INTERFACE(This, IDirect3DDevice7);
    }

    /* Check DirectDraw Interfacs */
    else if( IsEqualGUID( &IID_IDirectDraw7, refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirectDraw7);
        TRACE("(%p) Returning IDirectDraw7 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw4, refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirectDraw4);
        TRACE("(%p) Returning IDirectDraw4 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirectDraw2);
        TRACE("(%p) Returning IDirectDraw2 interface at %p\n", This, *obj);
    }
    else if( IsEqualGUID( &IID_IDirectDraw, refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirectDraw);
        TRACE("(%p) Returning IDirectDraw interface at %p\n", This, *obj);
    }

    /* Direct3D */
    else if ( IsEqualGUID( &IID_IDirect3D  , refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirect3D);
        TRACE("(%p) Returning IDirect3D interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D2 , refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirect3D2);
        TRACE("(%p) Returning IDirect3D2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D3 , refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirect3D3);
        TRACE("(%p) Returning IDirect3D3 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D7 , refiid ) )
    {
        *obj = ICOM_INTERFACE(This->ddraw, IDirect3D7);
        TRACE("(%p) Returning IDirect3D7 interface at %p\n", This, *obj);
    }

    /* Direct3DDevice */
    else if ( IsEqualGUID( &IID_IDirect3DDevice  , refiid ) )
    {
        *obj = ICOM_INTERFACE(This, IDirect3DDevice);
        TRACE("(%p) Returning IDirect3DDevice interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice2  , refiid ) ) {
        *obj = ICOM_INTERFACE(This, IDirect3DDevice2);
        TRACE("(%p) Returning IDirect3DDevice2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice3  , refiid ) ) {
        *obj = ICOM_INTERFACE(This, IDirect3DDevice3);
        TRACE("(%p) Returning IDirect3DDevice3 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice7  , refiid ) ) {
        *obj = ICOM_INTERFACE(This, IDirect3DDevice7);
        TRACE("(%p) Returning IDirect3DDevice7 interface at %p\n", This, *obj);
    }

    /* Unknown interface */
    else
    {
        ERR("(%p)->(%s, %p): No interface found\n", This, debugstr_guid(refiid), obj);
        return E_NOINTERFACE;
    }

    /* AddRef the returned interface */
    IUnknown_AddRef( (IUnknown *) *obj);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_QueryInterface(IDirect3DDevice3 *iface,
                                           REFIID riid,
                                           void **obj)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", This, debugstr_guid(riid), obj);
    return IDirect3DDevice7_QueryInterface(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           riid,
                                           obj);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_QueryInterface(IDirect3DDevice2 *iface,
                                           REFIID riid,
                                           void **obj)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", This, debugstr_guid(riid), obj);
    return IDirect3DDevice7_QueryInterface(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           riid,
                                           obj);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_QueryInterface(IDirect3DDevice *iface,
                                           REFIID riid,
                                           void **obp)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", This, debugstr_guid(riid), obp);
    return IDirect3DDevice7_QueryInterface(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           riid,
                                           obp);
}

/*****************************************************************************
 * IDirect3DDevice7::AddRef
 *
 * Increases the refcount....
 * The most exciting Method, definitely
 *
 * Exists in Version 1, 2, 3 and 7
 *
 * Returns:
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DDeviceImpl_7_AddRef(IDirect3DDevice7 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : incrementing from %u.\n", This, ref -1);

    return ref;
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_AddRef(IDirect3DDevice3 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_AddRef(IDirect3DDevice2 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_AddRef(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_AddRef(IDirect3DDevice *iface)
{
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_AddRef(COM_INTERFACE_CAST(IDirect3DDeviceImpl, IDirect3DDevice, IDirect3DDevice7, iface));
}

/*****************************************************************************
 * IDirect3DDevice7::Release
 *
 * Decreases the refcount of the interface
 * When the refcount is reduced to 0, the object is destroyed.
 *
 * Exists in Version 1, 2, 3 and 7
 *
 * Returns:d
 *  The new refcount
 *
 *****************************************************************************/
static ULONG WINAPI
IDirect3DDeviceImpl_7_Release(IDirect3DDevice7 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref +1);

    /* This method doesn't destroy the WineD3DDevice, because it's still in use for
     * 2D rendering. IDirectDrawSurface7::Release will destroy the WineD3DDevice
     * when the render target is released
     */
    if (ref == 0)
    {
        IParent *IndexBufferParent;
        DWORD i;

        /* Free the index buffer. */
        IWineD3DDevice_SetIndices(This->wineD3DDevice,
                                  NULL,
                                  0);
        IWineD3DIndexBuffer_GetParent(This->indexbuffer,
                                      (IUnknown **) &IndexBufferParent);
        IParent_Release(IndexBufferParent); /* Once for the getParent */
        if( IParent_Release(IndexBufferParent) != 0)  /* And now to destroy it */
        {
            ERR(" (%p) Something is still holding the index buffer parent %p\n", This, IndexBufferParent);
        }

        /* There is no need to unset the vertex buffer here, IWineD3DDevice_Uninit3D will do that when
         * destroying the primary stateblock. If a vertex buffer is destroyed while it is bound
         * IDirect3DVertexBuffer::Release will unset it.
         */

        /* Restore the render targets */
        if(This->OffScreenTarget)
        {
            /* Set the device up to render to the front buffer since the back buffer will
             * vanish soon.
             */
            IWineD3DDevice_SetRenderTarget(This->wineD3DDevice, 0,
                                           This->ddraw->d3d_target->WineD3DSurface);
            /* This->target is the offscreen target.
             * This->ddraw->d3d_target is the target used by DDraw
             */
            TRACE("(%p) Release: Using %p as front buffer, %p as back buffer\n", This, This->ddraw->d3d_target, NULL);
            IWineD3DDevice_SetFrontBackBuffers(This->wineD3DDevice,
                                               This->ddraw->d3d_target->WineD3DSurface,
                                               NULL);
        }

        /* Release the WineD3DDevice. This won't destroy it */
        if(IWineD3DDevice_Release(This->wineD3DDevice) <= 0)
        {
            ERR(" (%p) The wineD3D device %p was destroyed unexpectadely. Prepare for trouble\n", This, This->wineD3DDevice);
        }

        /* The texture handles should be unset by now, but there might be some bits
         * missing in our reference counting(needs test). Do a sanity check
         */
        for(i = 0; i < This->numHandles; i++)
        {
            if(This->Handles[i].ptr)
            {
                switch(This->Handles[i].type)
                {
                    case DDrawHandle_Texture:
                    {
                        IDirectDrawSurfaceImpl *surf = (IDirectDrawSurfaceImpl *) This->Handles[i].ptr;
                        FIXME("Texture Handle %d not unset properly\n", i + 1);
                        surf->Handle = 0;
                    }
                    break;

                    case DDrawHandle_Material:
                    {
                        IDirect3DMaterialImpl *mat = (IDirect3DMaterialImpl *) This->Handles[i].ptr;
                        FIXME("Material handle %d not unset properly\n", i + 1);
                        mat->Handle = 0;
                    }
                    break;

                    case DDrawHandle_Matrix:
                    {
                        /* No fixme here because this might happen because of sloppy apps */
                        WARN("Leftover matrix handle %d, deleting\n", i + 1);
                        IDirect3DDevice_DeleteMatrix(ICOM_INTERFACE(This, IDirect3DDevice),
                                                     i + 1);
                    }
                    break;

                    case DDrawHandle_StateBlock:
                    {
                        /* No fixme here because this might happen because of sloppy apps */
                        WARN("Leftover stateblock handle %d, deleting\n", i + 1);
                        IDirect3DDevice7_DeleteStateBlock(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                          i + 1);
                    }
                    break;

                    default:
                        FIXME("Unknown handle %d not unset properly\n", i + 1);
                }
            }
        }

        HeapFree(GetProcessHeap(), 0, This->Handles);

        /* Release the render target and the WineD3D render target
         * (See IDirect3D7::CreateDevice for more comments on this)
         */
        IDirectDrawSurface7_Release(ICOM_INTERFACE(This->target, IDirectDrawSurface7));
        IDirectDrawSurface7_Release(ICOM_INTERFACE(This->ddraw->d3d_target,IDirectDrawSurface7));

        This->ddraw->d3ddevice = NULL;

        /* Now free the structure */
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_Release(IDirect3DDevice3 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_Release(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_Release(IDirect3DDevice2 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_Release(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_Release(IDirect3DDevice *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_Release(ICOM_INTERFACE(This, IDirect3DDevice7));
}

/*****************************************************************************
 * IDirect3DDevice Methods
 *****************************************************************************/

/*****************************************************************************
 * IDirect3DDevice::Initialize
 *
 * Initializes a Direct3DDevice. This implementation is a no-op, as all
 * initialization is done at create time.
 *
 * Exists in Version 1
 *
 * Parameters:
 *  No idea what they mean, as the MSDN page is gone
 *
 * Returns: DD_OK
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_Initialize(IDirect3DDevice *iface,
                                 IDirect3D *Direct3D, GUID *guid,
                                 D3DDEVICEDESC *Desc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);

    /* It shouldn't be crucial, but print a FIXME, I'm interested if
     * any game calls it and when
     */
    FIXME("(%p)->(%p,%p,%p): No-op!\n", This, Direct3D, guid, Desc);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::GetCaps
 *
 * Retrieves the device's capabilities
 *
 * This implementation is used for Version 7 only, the older versions have
 * their own implementation.
 *
 * Parameters:
 *  Desc: Pointer to a D3DDEVICEDESC7 structure to fill
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_* if a problem occurs. See WineD3D
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetCaps(IDirect3DDevice7 *iface,
                              D3DDEVICEDESC7 *Desc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    D3DDEVICEDESC OldDesc;
    TRACE("(%p)->(%p)\n", This, Desc);

    /* Call the same function used by IDirect3D, this saves code */
    return IDirect3DImpl_GetCaps(This->ddraw->wineD3D, &OldDesc, Desc);
}

/*****************************************************************************
 * IDirect3DDevice3::GetCaps
 *
 * Retrieves the capabilities of the hardware device and the emulation
 * device. For Wine, hardware and emulation are the same (it's all HW).
 *
 * This implementation is used for Version 1, 2, and 3. Version 7 has its own
 *
 * Parameters:
 *  HWDesc: Structure to fill with the HW caps
 *  HelDesc: Structure to fill with the hardare emulation caps
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_* if a problem occurs. See WineD3D
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_GetCaps(IDirect3DDevice3 *iface,
                              D3DDEVICEDESC *HWDesc,
                              D3DDEVICEDESC *HelDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    D3DDEVICEDESC7 newDesc;
    HRESULT hr;
    TRACE("(%p)->(%p,%p)\n", iface, HWDesc, HelDesc);

    hr = IDirect3DImpl_GetCaps(This->ddraw->wineD3D, HWDesc, &newDesc);
    if(hr != D3D_OK) return hr;

    *HelDesc = *HWDesc;
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCaps(IDirect3DDevice2 *iface,
                                    D3DDEVICEDESC *D3DHWDevDesc,
                                    D3DDEVICEDESC *D3DHELDevDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice3 interface.\n", This, D3DHWDevDesc, D3DHELDevDesc);
    return IDirect3DDevice3_GetCaps(ICOM_INTERFACE(This, IDirect3DDevice3),
                                    D3DHWDevDesc,
                                    D3DHELDevDesc);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetCaps(IDirect3DDevice *iface,
                                    D3DDEVICEDESC *D3DHWDevDesc,
                                    D3DDEVICEDESC *D3DHELDevDesc)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice3 interface.\n", This, D3DHWDevDesc, D3DHELDevDesc);
    return IDirect3DDevice3_GetCaps(ICOM_INTERFACE(This, IDirect3DDevice3),
                                    D3DHWDevDesc,
                                    D3DHELDevDesc);
}

/*****************************************************************************
 * IDirect3DDevice2::SwapTextureHandles
 *
 * Swaps the texture handles of 2 Texture interfaces. Version 1 and 2
 *
 * Parameters:
 *  Tex1, Tex2: The 2 Textures to swap
 *
 * Returns:
 *  D3D_OK
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_2_SwapTextureHandles(IDirect3DDevice2 *iface,
                                         IDirect3DTexture2 *Tex1,
                                         IDirect3DTexture2 *Tex2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    DWORD swap;
    IDirectDrawSurfaceImpl *surf1 = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirect3DTexture2, Tex1);
    IDirectDrawSurfaceImpl *surf2 = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirect3DTexture2, Tex2);
    TRACE("(%p)->(%p,%p)\n", This, surf1, surf2);

    This->Handles[surf1->Handle - 1].ptr = surf2;
    This->Handles[surf2->Handle - 1].ptr = surf1;

    swap = surf2->Handle;
    surf2->Handle = surf1->Handle;
    surf1->Handle = swap;

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_SwapTextureHandles(IDirect3DDevice *iface,
                                               IDirect3DTexture *D3DTex1,
                                               IDirect3DTexture *D3DTex2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    IDirectDrawSurfaceImpl *surf1 = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirect3DTexture, D3DTex1);
    IDirectDrawSurfaceImpl *surf2 = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirect3DTexture, D3DTex2);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice2 interface.\n", This, surf1, surf2);
    return IDirect3DDevice2_SwapTextureHandles(ICOM_INTERFACE(This, IDirect3DDevice2),
                                               ICOM_INTERFACE(surf1, IDirect3DTexture2),
                                               ICOM_INTERFACE(surf2, IDirect3DTexture2));
}

/*****************************************************************************
 * IDirect3DDevice3::GetStats
 *
 * This method seems to retrieve some stats from the device.
 * The MSDN documentation doesn't exist any more, but the D3DSTATS
 * structure suggests that the amout of drawn primitives and processed
 * vertices is returned.
 *
 * Exists in Version 1, 2 and 3
 *
 * Parameters:
 *  Stats: Pointer to a D3DSTATS structure to be filled
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Stats == NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_GetStats(IDirect3DDevice3 *iface,
                               D3DSTATS *Stats)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p)->(%p): Stub!\n", This, Stats);

    if(!Stats)
        return DDERR_INVALIDPARAMS;

    /* Fill the Stats with 0 */
    Stats->dwTrianglesDrawn = 0;
    Stats->dwLinesDrawn = 0;
    Stats->dwPointsDrawn = 0;
    Stats->dwSpansDrawn = 0;
    Stats->dwVerticesProcessed = 0;

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetStats(IDirect3DDevice2 *iface,
                                     D3DSTATS *Stats)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, Stats);
    return IDirect3DDevice3_GetStats(ICOM_INTERFACE(This, IDirect3DDevice3),
                                     Stats);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetStats(IDirect3DDevice *iface,
                                     D3DSTATS *Stats)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, Stats);
    return IDirect3DDevice3_GetStats(ICOM_INTERFACE(This, IDirect3DDevice3),
                                     Stats);
}

/*****************************************************************************
 * IDirect3DDevice::CreateExecuteBuffer
 *
 * Creates an IDirect3DExecuteBuffer, used for rendering with a
 * Direct3DDevice.
 *
 * Version 1 only.
 *
 * Params:
 *  Desc: Buffer description
 *  ExecuteBuffer: Address to return the Interface pointer at
 *  UnkOuter: Must be NULL. Basically for aggregation, which ddraw doesn't
 *            support
 *
 * Returns:
 *  CLASS_E_NOAGGREGATION if UnkOuter != NULL
 *  DDERR_OUTOFMEMORY if we ran out of memory
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_CreateExecuteBuffer(IDirect3DDevice *iface,
                                          D3DEXECUTEBUFFERDESC *Desc,
                                          IDirect3DExecuteBuffer **ExecuteBuffer,
                                          IUnknown *UnkOuter)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DExecuteBufferImpl* object;
    TRACE("(%p)->(%p,%p,%p)!\n", This, Desc, ExecuteBuffer, UnkOuter);

    if(UnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Allocate the new Execute Buffer */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DExecuteBufferImpl));
    if(!object)
    {
        ERR("Out of memory when allocating a IDirect3DExecuteBufferImpl structure\n");
        return DDERR_OUTOFMEMORY;
    }

    ICOM_INIT_INTERFACE(object, IDirect3DExecuteBuffer, IDirect3DExecuteBuffer_Vtbl);

    object->ref = 1;
    object->d3ddev = This;

    /* Initializes memory */
    memcpy(&object->desc, Desc, Desc->dwSize);

    /* No buffer given */
    if ((object->desc.dwFlags & D3DDEB_LPDATA) == 0)
        object->desc.lpData = NULL;

    /* No buffer size given */
    if ((object->desc.dwFlags & D3DDEB_BUFSIZE) == 0)
        object->desc.dwBufferSize = 0;

    /* Create buffer if asked */
    if ((object->desc.lpData == NULL) && (object->desc.dwBufferSize > 0))
    {
        object->need_free = TRUE;
        object->desc.lpData = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,object->desc.dwBufferSize);
        if(!object->desc.lpData)
        {
            ERR("Out of memory when allocating the execute buffer data\n");
            HeapFree(GetProcessHeap(), 0, object);
            return DDERR_OUTOFMEMORY;
        }
    }
    else
    {
        object->need_free = FALSE;
    }

    /* No vertices for the moment */
    object->vertex_data = NULL;

    object->desc.dwFlags |= D3DDEB_LPDATA;

    object->indices = NULL;
    object->nb_indices = 0;

    *ExecuteBuffer = ICOM_INTERFACE(object, IDirect3DExecuteBuffer);

    TRACE(" Returning IDirect3DExecuteBuffer at %p, implementation is at %p\n", *ExecuteBuffer, object);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice::Execute
 *
 * Executes all the stuff in an execute buffer.
 *
 * Params:
 *  ExecuteBuffer: The buffer to execute
 *  Viewport: The viewport used for rendering
 *  Flags: Some flags
 *
 * Returns:
 *  DDERR_INVALIDPARAMS if ExecuteBuffer == NULL
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_Execute(IDirect3DDevice *iface,
                              IDirect3DExecuteBuffer *ExecuteBuffer,
                              IDirect3DViewport *Viewport,
                              DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DExecuteBufferImpl *Direct3DExecuteBufferImpl = ICOM_OBJECT(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, ExecuteBuffer);
    IDirect3DViewportImpl *Direct3DViewportImpl = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Viewport);

    TRACE("(%p)->(%p,%p,%08x)\n", This, Direct3DExecuteBufferImpl, Direct3DViewportImpl, Flags);

    if(!Direct3DExecuteBufferImpl)
        return DDERR_INVALIDPARAMS;

    /* Execute... */
    IDirect3DExecuteBufferImpl_Execute(Direct3DExecuteBufferImpl, This, Direct3DViewportImpl);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice3::AddViewport
 *
 * Add a Direct3DViewport to the device's viewport list. These viewports
 * are wrapped to IDirect3DDevice7 viewports in viewport.c
 *
 * Exists in Version 1, 2 and 3. Note that IDirect3DViewport 1, 2 and 3
 * are the same interfaces.
 *
 * Params:
 *  Viewport: The viewport to add
 *
 * Returns:
 *  DDERR_INVALIDPARAMS if Viewport == NULL
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_AddViewport(IDirect3DDevice3 *iface,
                                  IDirect3DViewport3 *Viewport)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Viewport);

    TRACE("(%p)->(%p)\n", This, vp);

    /* Sanity check */
    if(!vp)
        return DDERR_INVALIDPARAMS;

    vp->next = This->viewport_list;
    This->viewport_list = vp;

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_AddViewport(IDirect3DDevice2 *iface,
                                        IDirect3DViewport2 *Direct3DViewport2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Direct3DViewport2);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_AddViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                        ICOM_INTERFACE(vp, IDirect3DViewport3));
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_AddViewport(IDirect3DDevice *iface,
                                        IDirect3DViewport *Direct3DViewport)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Direct3DViewport);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_AddViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                        ICOM_INTERFACE(vp, IDirect3DViewport3));
}

/*****************************************************************************
 * IDirect3DDevice3::DeleteViewport
 *
 * Deletes a Direct3DViewport from the device's viewport list.
 *
 * Exists in Version 1, 2 and 3. Note that all Viewport interface versions
 * are equal.
 *
 * Params:
 *  Viewport: The viewport to delete
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if the viewport wasn't found in the list
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_DeleteViewport(IDirect3DDevice3 *iface,
                                     IDirect3DViewport3 *Viewport)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *) Viewport;
    IDirect3DViewportImpl *cur_viewport, *prev_viewport = NULL;

    TRACE("(%p)->(%p)\n", This, vp);

    cur_viewport = This->viewport_list;
    while (cur_viewport != NULL)
    {
        if (cur_viewport == vp)
        {
            if (prev_viewport == NULL) This->viewport_list = cur_viewport->next;
            else prev_viewport->next = cur_viewport->next;
            /* TODO : add desactivate of the viewport and all associated lights... */
            return D3D_OK;
        }
        prev_viewport = cur_viewport;
        cur_viewport = cur_viewport->next;
    }

    return DDERR_INVALIDPARAMS;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DeleteViewport(IDirect3DDevice2 *iface,
                                           IDirect3DViewport2 *Direct3DViewport2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Direct3DViewport2);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_DeleteViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                           ICOM_INTERFACE(vp, IDirect3DViewport3));
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_DeleteViewport(IDirect3DDevice *iface,
                                           IDirect3DViewport *Direct3DViewport)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Direct3DViewport);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_DeleteViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                           ICOM_INTERFACE(vp, IDirect3DViewport3));
}

/*****************************************************************************
 * IDirect3DDevice3::NextViewport
 *
 * Returns a viewport from the viewport list, depending on the
 * passed viewport and the flags.
 *
 * Exists in Version 1, 2 and 3. Note that all Viewport interface versions
 * are equal.
 *
 * Params:
 *  Viewport: Viewport to use for beginning the search
 *  Flags: D3DNEXT_NEXT, D3DNEXT_HEAD or D3DNEXT_TAIL
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if the flags were wrong, or Viewport was NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_NextViewport(IDirect3DDevice3 *iface,
                                   IDirect3DViewport3 *Viewport3,
                                   IDirect3DViewport3 **lplpDirect3DViewport3,
                                   DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Viewport3);
    IDirect3DViewportImpl *res = NULL;

    TRACE("(%p)->(%p,%p,%08x)\n", This, vp, lplpDirect3DViewport3, Flags);

    if(!vp)
    {
        *lplpDirect3DViewport3 = NULL;
        return DDERR_INVALIDPARAMS;
    }


    switch (Flags)
    {
        case D3DNEXT_NEXT:
        {
            res = vp->next;
        }
        break;
        case D3DNEXT_HEAD:
        {
            res = This->viewport_list;
        }
        break;
        case D3DNEXT_TAIL:
        {
            IDirect3DViewportImpl *cur_viewport = This->viewport_list;
            if (cur_viewport != NULL)
            {
                while (cur_viewport->next != NULL) cur_viewport = cur_viewport->next;
            }
            res = cur_viewport;
        }
        break;
        default:
            *lplpDirect3DViewport3 = NULL;
            return DDERR_INVALIDPARAMS;
    }

    *lplpDirect3DViewport3 = ICOM_INTERFACE(res, IDirect3DViewport3);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_NextViewport(IDirect3DDevice2 *iface,
                                         IDirect3DViewport2 *Viewport2,
                                         IDirect3DViewport2 **lplpDirect3DViewport2,
                                         DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Viewport2);
    IDirect3DViewport3 *res;
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p,%p,%08x) thunking to IDirect3DDevice3 interface.\n", This, vp, lplpDirect3DViewport2, Flags);
    hr = IDirect3DDevice3_NextViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                       ICOM_INTERFACE(vp, IDirect3DViewport3),
                                       &res,
                                       Flags);
    *lplpDirect3DViewport2 = (IDirect3DViewport2 *) COM_INTERFACE_CAST(IDirect3DViewportImpl, IDirect3DViewport3, IDirect3DViewport3, res);
    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_NextViewport(IDirect3DDevice *iface,
                                         IDirect3DViewport *Viewport,
                                         IDirect3DViewport **lplpDirect3DViewport,
                                         DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Viewport);
    IDirect3DViewport3 *res;
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p,%p,%08x) thunking to IDirect3DDevice3 interface.\n", This, vp, lplpDirect3DViewport, Flags);
    hr = IDirect3DDevice3_NextViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                       ICOM_INTERFACE(vp, IDirect3DViewport3),
                                       &res,
                                       Flags);
    *lplpDirect3DViewport = (IDirect3DViewport *) COM_INTERFACE_CAST(IDirect3DViewportImpl, IDirect3DViewport3, IDirect3DViewport3, res);
    return hr;
}

/*****************************************************************************
 * IDirect3DDevice::Pick
 *
 * Executes an execute buffer without performing rendering. Instead, a
 * list of primitives that intersect with (x1,y1) of the passed rectangle
 * is created. IDirect3DDevice::GetPickRecords can be used to retrieve
 * this list.
 *
 * Version 1 only
 *
 * Params:
 *  ExecuteBuffer: Buffer to execute
 *  Viewport: Viewport to use for execution
 *  Flags: None are defined, according to the SDK
 *  Rect: Specifies the coordinates to be picked. Only x1 and y2 are used,
 *        x2 and y2 are ignored.
 *
 * Returns:
 *  D3D_OK because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_Pick(IDirect3DDevice *iface,
                           IDirect3DExecuteBuffer *ExecuteBuffer,
                           IDirect3DViewport *Viewport,
                           DWORD Flags,
                           D3DRECT *Rect)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    IDirect3DExecuteBufferImpl *execbuf = ICOM_OBJECT(IDirect3DExecuteBufferImpl, IDirect3DExecuteBuffer, ExecuteBuffer);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Viewport);
    FIXME("(%p)->(%p,%p,%08x,%p): stub!\n", This, execbuf, vp, Flags, Rect);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice::GetPickRecords
 *
 * Retrieves the pick records generated by IDirect3DDevice::GetPickRecords
 *
 * Version 1 only
 *
 * Params:
 *  Count: Pointer to a DWORD containing the numbers of pick records to
 *         retrieve
 *  D3DPickRec: Address to store the resulting D3DPICKRECORD arry.
 *
 * Returns:
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_GetPickRecords(IDirect3DDevice *iface,
                                     DWORD *Count,
                                     D3DPICKRECORD *D3DPickRec)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    FIXME("(%p)->(%p,%p): stub!\n", This, Count, D3DPickRec);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::EnumTextureformats
 *
 * Enumerates the supported texture formats. It has a list of all possible
 * formats and calls IWineD3D::CheckDeviceFormat for each format to see if
 * WineD3D supports it. If so, then it is passed to the app.
 *
 * This is for Version 7 and 3, older versions have a different
 * callback function and their own implementation
 *
 * Params:
 *  Callback: Callback to call for each enumerated format
 *  Arg: Argument to pass to the callback
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Callback == NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_EnumTextureFormats(IDirect3DDevice7 *iface,
                                         LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                         void *Arg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    int i;

    WINED3DFORMAT FormatList[] = {
        /* 32 bit */
        WINED3DFMT_A8R8G8B8,
        WINED3DFMT_X8R8G8B8,
        /* 24 bit */
        WINED3DFMT_R8G8B8,
        /* 16 Bit */
        WINED3DFMT_A1R5G5B5,
        WINED3DFMT_A4R4G4B4,
        WINED3DFMT_R5G6B5,
        WINED3DFMT_X1R5G5B5,
        /* 8 Bit */
        WINED3DFMT_R3G3B2,
        WINED3DFMT_P8,
        /* FOURCC codes */
        WINED3DFMT_DXT1,
        WINED3DFMT_DXT3,
        WINED3DFMT_DXT5,
    };

    TRACE("(%p)->(%p,%p): Relay\n", This, Callback, Arg);

    if(!Callback)
        return DDERR_INVALIDPARAMS;

    for(i = 0; i < sizeof(FormatList) / sizeof(WINED3DFORMAT); i++)
    {
        hr = IWineD3D_CheckDeviceFormat(This->ddraw->wineD3D,
                                        0 /* Adapter */,
                                        0 /* DeviceType */,
                                        0 /* AdapterFormat */,
                                        0 /* Usage */,
                                        0 /* ResourceType */,
                                        FormatList[i]);
        if(hr == D3D_OK)
        {
            DDPIXELFORMAT pformat;

            memset(&pformat, 0, sizeof(pformat));
            pformat.dwSize = sizeof(pformat);
            PixelFormat_WineD3DtoDD(&pformat, FormatList[i]);

            TRACE("Enumerating WineD3DFormat %d\n", FormatList[i]);
            hr = Callback(&pformat, Arg);
            if(hr != DDENUMRET_OK)
            {
                TRACE("Format enumeration cancelled by application\n");
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration\n");
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EnumTextureFormats(IDirect3DDevice3 *iface,
                                               LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                               void *Arg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice7 interface.\n", This, Callback, Arg);
    return IDirect3DDevice7_EnumTextureFormats(ICOM_INTERFACE(This, IDirect3DDevice7),
                                               Callback,
                                               Arg);
}

/*****************************************************************************
 * IDirect3DDevice2::EnumTextureformats
 *
 * EnumTextureFormats for Version 1 and 2, see
 * IDirect3DDevice7::EnumTexureFormats for a more detailed description.
 *
 * This version has a different callback and does not enumerate FourCC
 * formats
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_2_EnumTextureFormats(IDirect3DDevice2 *iface,
                                         LPD3DENUMTEXTUREFORMATSCALLBACK Callback,
                                         void *Arg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    HRESULT hr;
    int i;

    WINED3DFORMAT FormatList[] = {
        /* 32 bit */
        WINED3DFMT_A8R8G8B8,
        WINED3DFMT_X8R8G8B8,
        /* 24 bit */
        WINED3DFMT_R8G8B8,
        /* 16 Bit */
        WINED3DFMT_A1R5G5B5,
        WINED3DFMT_A4R4G4B4,
        WINED3DFMT_R5G6B5,
        WINED3DFMT_X1R5G5B5,
        /* 8 Bit */
        WINED3DFMT_R3G3B2,
        WINED3DFMT_P8,
        /* FOURCC codes - Not in this version*/
    };

    TRACE("(%p)->(%p,%p): Relay\n", This, Callback, Arg);

    if(!Callback)
        return DDERR_INVALIDPARAMS;

    for(i = 0; i < sizeof(FormatList) / sizeof(WINED3DFORMAT); i++)
    {
        hr = IWineD3D_CheckDeviceFormat(This->ddraw->wineD3D,
                                        0 /* Adapter */,
                                        0 /* DeviceType */,
                                        0 /* AdapterFormat */,
                                        0 /* Usage */,
                                        0 /* ResourceType */,
                                        FormatList[i]);
        if(hr == D3D_OK)
        {
            DDSURFACEDESC sdesc;

            memset(&sdesc, 0, sizeof(sdesc));
            sdesc.dwSize = sizeof(sdesc);
            sdesc.dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            sdesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            sdesc.ddpfPixelFormat.dwSize = sizeof(sdesc.ddpfPixelFormat);
            PixelFormat_WineD3DtoDD(&sdesc.ddpfPixelFormat, FormatList[i]);

            TRACE("Enumerating WineD3DFormat %d\n", FormatList[i]);
            hr = Callback(&sdesc, Arg);
            if(hr != DDENUMRET_OK)
            {
                TRACE("Format enumeration cancelled by application\n");
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration\n");
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EnumTextureFormats(IDirect3DDevice *iface,
                                               LPD3DENUMTEXTUREFORMATSCALLBACK Callback,
                                               void *Arg)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice2 interface.\n", This, Callback, Arg);
    return IDirect3DDevice2_EnumTextureFormats(ICOM_INTERFACE(This, IDirect3DDevice2),
                                               Callback,
                                               Arg);
}

/*****************************************************************************
 * IDirect3DDevice::CreateMatrix
 *
 * Creates a matrix handle. A handle is created and memory for a D3DMATRIX is
 * allocated for the handle.
 *
 * Version 1 only
 *
 * Params
 *  D3DMatHandle: Address to return the handle at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if D3DMatHandle = NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_CreateMatrix(IDirect3DDevice *iface, D3DMATRIXHANDLE *D3DMatHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    D3DMATRIX *Matrix;
    TRACE("(%p)->(%p)\n", This, D3DMatHandle);

    if(!D3DMatHandle)
        return DDERR_INVALIDPARAMS;

    Matrix = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3DMATRIX));
    if(!Matrix)
    {
        ERR("Out of memory when allocating a D3DMATRIX\n");
        return DDERR_OUTOFMEMORY;
    }
    *D3DMatHandle = IDirect3DDeviceImpl_CreateHandle(This);
    if(!(*D3DMatHandle))
    {
        ERR("Failed to create a matrix handle\n");
        HeapFree(GetProcessHeap(), 0, Matrix);
        return DDERR_OUTOFMEMORY;
    }
    This->Handles[(DWORD) *D3DMatHandle - 1].ptr = Matrix;
    This->Handles[(DWORD) *D3DMatHandle - 1].type = DDrawHandle_Matrix;
    TRACE(" returning matrix handle %d\n", *D3DMatHandle);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice::SetMatrix
 *
 * Sets a matrix for a matrix handle. The matrix is copied into the memory
 * allocated for the handle
 *
 * Version 1 only
 *
 * Params:
 *  D3DMatHandle: Handle to set the matrix to
 *  D3DMatrix: Matrix to set
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if the handle of the matrix is invalid or the matrix
 *   to set is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_SetMatrix(IDirect3DDevice *iface,
                                D3DMATRIXHANDLE D3DMatHandle,
                                D3DMATRIX * const D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE("(%p)->(%08x,%p)\n", This, (DWORD) D3DMatHandle, D3DMatrix);

    if( (!D3DMatHandle) || (!D3DMatrix) )
        return DDERR_INVALIDPARAMS;

    if(D3DMatHandle > This->numHandles)
    {
        ERR("Handle %d out of range\n", D3DMatHandle);
        return DDERR_INVALIDPARAMS;
    }
    else if(This->Handles[D3DMatHandle - 1].type != DDrawHandle_Matrix)
    {
        ERR("Handle %d is not a matrix handle\n", D3DMatHandle);
        return DDERR_INVALIDPARAMS;
    }

    if (TRACE_ON(d3d7))
        dump_D3DMATRIX(D3DMatrix);

    *((D3DMATRIX *) This->Handles[D3DMatHandle - 1].ptr) = *D3DMatrix;

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice::SetMatrix
 *
 * Returns the content of a D3DMATRIX handle
 *
 * Version 1 only
 *
 * Params:
 *  D3DMatHandle: Matrix handle to read the content from
 *  D3DMatrix: Address to store the content at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if D3DMatHandle is invalid or D3DMatrix is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_GetMatrix(IDirect3DDevice *iface,
                                D3DMATRIXHANDLE D3DMatHandle,
                                D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE("(%p)->(%08x,%p)\n", This, (DWORD) D3DMatHandle, D3DMatrix);

    if(!D3DMatrix)
        return DDERR_INVALIDPARAMS;
    if(!D3DMatHandle)
        return DDERR_INVALIDPARAMS;

    if(D3DMatHandle > This->numHandles)
    {
        ERR("Handle %d out of range\n", D3DMatHandle);
        return DDERR_INVALIDPARAMS;
    }
    else if(This->Handles[D3DMatHandle - 1].type != DDrawHandle_Matrix)
    {
        ERR("Handle %d is not a matrix handle\n", D3DMatHandle);
        return DDERR_INVALIDPARAMS;
    }

    /* The handle is simply a pointer to a D3DMATRIX structure */
    *D3DMatrix = *((D3DMATRIX *) This->Handles[D3DMatHandle - 1].ptr);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice::DeleteMatrix
 *
 * Destroys a Matrix handle. Frees the memory and unsets the handle data
 *
 * Version 1 only
 *
 * Params:
 *  D3DMatHandle: Handle to destroy
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if D3DMatHandle is invalid
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_1_DeleteMatrix(IDirect3DDevice *iface,
                                   D3DMATRIXHANDLE D3DMatHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE("(%p)->(%08x)\n", This, (DWORD) D3DMatHandle);

    if(!D3DMatHandle)
        return DDERR_INVALIDPARAMS;

    if(D3DMatHandle > This->numHandles)
    {
        ERR("Handle %d out of range\n", D3DMatHandle);
        return DDERR_INVALIDPARAMS;
    }
    else if(This->Handles[D3DMatHandle - 1].type != DDrawHandle_Matrix)
    {
        ERR("Handle %d is not a matrix handle\n", D3DMatHandle);
        return DDERR_INVALIDPARAMS;
    }

    HeapFree(GetProcessHeap(), 0, This->Handles[D3DMatHandle - 1].ptr);
    This->Handles[D3DMatHandle - 1].ptr = NULL;
    This->Handles[D3DMatHandle - 1].type = DDrawHandle_Unknown;

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::BeginScene
 *
 * This method must be called before any rendering is performed.
 * IDirect3DDevice::EndScene has to be called after the scene is complete
 *
 * Version 1, 2, 3 and 7
 *
 * Returns:
 *  D3D_OK on success, for details see IWineD3DDevice::BeginScene
 *  D3DERR_SCENE_IN_SCENE if WineD3D returns an error(Only in case of an already
 *  started scene).
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_BeginScene(IDirect3DDevice7 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p): Relay\n", This);

    hr = IWineD3DDevice_BeginScene(This->wineD3DDevice);
    if(hr == WINED3D_OK) return D3D_OK;
    else return D3DERR_SCENE_IN_SCENE; /* TODO: Other possible causes of failure */
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_BeginScene(IDirect3DDevice3 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_BeginScene(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_BeginScene(IDirect3DDevice2 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_BeginScene(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_BeginScene(IDirect3DDevice *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_BeginScene(ICOM_INTERFACE(This, IDirect3DDevice7));
}

/*****************************************************************************
 * IDirect3DDevice7::EndScene
 *
 * Ends a scene that has been begun with IDirect3DDevice7::BeginScene.
 * This method must be called after rendering is finished.
 *
 * Version 1, 2, 3 and 7
 *
 * Returns:
 *  D3D_OK on success, for details see IWineD3DDevice::EndScene
 *  D3DERR_SCENE_NOT_IN_SCENE is returned if WineD3D returns an error. It does
 *  that only if the scene was already ended.
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_EndScene(IDirect3DDevice7 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p): Relay\n", This);

    hr = IWineD3DDevice_EndScene(This->wineD3DDevice);
    if(hr == WINED3D_OK) return D3D_OK;
    else return D3DERR_SCENE_NOT_IN_SCENE;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EndScene(IDirect3DDevice3 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_EndScene(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_EndScene(IDirect3DDevice2 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_EndScene(ICOM_INTERFACE(This, IDirect3DDevice7));
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EndScene(IDirect3DDevice *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_EndScene(ICOM_INTERFACE(This, IDirect3DDevice7));
}

/*****************************************************************************
 * IDirect3DDevice7::GetDirect3D
 *
 * Returns the IDirect3D(= interface to the DirectDraw object) used to create
 * this device.
 *
 * Params:
 *  Direct3D7: Address to store the interface pointer at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Direct3D7 == NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetDirect3D(IDirect3DDevice7 *iface,
                                  IDirect3D7 **Direct3D7)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%p)\n", This, Direct3D7);

    if(!Direct3D7)
        return DDERR_INVALIDPARAMS;

    *Direct3D7 = ICOM_INTERFACE(This->ddraw, IDirect3D7);
    IDirect3D7_AddRef(*Direct3D7);

    TRACE(" returning interface %p\n", *Direct3D7);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetDirect3D(IDirect3DDevice3 *iface,
                                        IDirect3D3 **Direct3D3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    HRESULT ret;
    IDirect3D7 *ret_ptr;

    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Direct3D3);
    ret = IDirect3DDevice7_GetDirect3D(ICOM_INTERFACE(This, IDirect3DDevice7),
                                       &ret_ptr);
    if(ret != D3D_OK)
        return ret;
    *Direct3D3 = COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirect3D3, ret_ptr);
    TRACE(" returning interface %p\n", *Direct3D3);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetDirect3D(IDirect3DDevice2 *iface,
                                        IDirect3D2 **Direct3D2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    HRESULT ret;
    IDirect3D7 *ret_ptr;

    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Direct3D2);
    ret = IDirect3DDevice7_GetDirect3D(ICOM_INTERFACE(This, IDirect3DDevice7),
                                       &ret_ptr);
    if(ret != D3D_OK)
        return ret;
    *Direct3D2 = COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirect3D2, ret_ptr);
    TRACE(" returning interface %p\n", *Direct3D2);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetDirect3D(IDirect3DDevice *iface,
                                        IDirect3D **Direct3D)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice, iface);
    HRESULT ret;
    IDirect3D7 *ret_ptr;

    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Direct3D);
    ret = IDirect3DDevice7_GetDirect3D(ICOM_INTERFACE(This, IDirect3DDevice7),
                                       &ret_ptr);
    if(ret != D3D_OK)
        return ret;
    *Direct3D = COM_INTERFACE_CAST(IDirectDrawImpl, IDirect3D7, IDirect3D, ret_ptr);
    TRACE(" returning interface %p\n", *Direct3D);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice3::SetCurrentViewport
 *
 * Sets a Direct3DViewport as the current viewport.
 * For the thunks note that all viewport interface versions are equal
 *
 * Params:
 *  Direct3DViewport3: The viewport to set
 *
 * Version 2 and 3
 *
 * Returns:
 *  D3D_OK on success
 *  (Is a NULL viewport valid?)
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_SetCurrentViewport(IDirect3DDevice3 *iface,
                                         IDirect3DViewport3 *Direct3DViewport3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Direct3DViewport3);
    TRACE("(%p)->(%p)\n", This, Direct3DViewport3);

    /* Do nothing if the specified viewport is the same as the current one */
    if (This->current_viewport == vp )
      return D3D_OK;

    /* Should check if the viewport was added or not */

    /* Release previous viewport and AddRef the new one */
    if (This->current_viewport)
    {
        TRACE("ViewportImpl is at %p, interface is at %p\n", This->current_viewport, ICOM_INTERFACE(This->current_viewport, IDirect3DViewport3));
        IDirect3DViewport3_Release( ICOM_INTERFACE(This->current_viewport, IDirect3DViewport3) );
    }
    IDirect3DViewport3_AddRef(Direct3DViewport3);

    /* Set this viewport as the current viewport */
    This->current_viewport = vp;

    /* Activate this viewport */
    This->current_viewport->active_device = This;
    This->current_viewport->activate(This->current_viewport);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetCurrentViewport(IDirect3DDevice2 *iface,
                                               IDirect3DViewport2 *Direct3DViewport2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    IDirect3DViewportImpl *vp = ICOM_OBJECT(IDirect3DViewportImpl, IDirect3DViewport3, Direct3DViewport2);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_SetCurrentViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                               ICOM_INTERFACE(vp, IDirect3DViewport3));
}

/*****************************************************************************
 * IDirect3DDevice3::GetCurrentViewport
 *
 * Returns the currently active viewport.
 *
 * Version 2 and 3
 *
 * Params:
 *  Direct3DViewport3: Address to return the interface pointer at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Direct3DViewport == NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_GetCurrentViewport(IDirect3DDevice3 *iface,
                                         IDirect3DViewport3 **Direct3DViewport3)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p)->(%p)\n", This, Direct3DViewport3);

    if(!Direct3DViewport3)
        return DDERR_INVALIDPARAMS;

    *Direct3DViewport3 = ICOM_INTERFACE(This->current_viewport, IDirect3DViewport3);

    /* AddRef the returned viewport */
    if(*Direct3DViewport3) IDirect3DViewport3_AddRef(*Direct3DViewport3);

    TRACE(" returning interface %p\n", *Direct3DViewport3);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCurrentViewport(IDirect3DDevice2 *iface,
                                               IDirect3DViewport2 **Direct3DViewport2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, Direct3DViewport2);
    hr = IDirect3DDevice3_GetCurrentViewport(ICOM_INTERFACE(This, IDirect3DDevice3),
                                            (IDirect3DViewport3 **) Direct3DViewport2);
    if(hr != D3D_OK) return hr;
    *Direct3DViewport2 = (IDirect3DViewport2 *) COM_INTERFACE_CAST(IDirect3DViewportImpl, IDirect3DViewport3, IDirect3DViewport3, *Direct3DViewport2);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::SetRenderTarget
 *
 * Sets the render target for the Direct3DDevice.
 * For the thunks note that IDirectDrawSurface7 == IDirectDrawSurface4 and
 * IDirectDrawSurface3 == IDirectDrawSurface
 *
 * Version 2, 3 and 7
 *
 * Params:
 *  NewTarget: Pointer to an IDirectDrawSurface7 interface to set as the new
 *             render target
 *  Flags: Some flags
 *
 * Returns:
 *  D3D_OK on success, for details see IWineD3DDevice::SetRenderTarget
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetRenderTarget(IDirect3DDevice7 *iface,
                                      IDirectDrawSurface7 *NewTarget,
                                      DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirectDrawSurfaceImpl *Target = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, NewTarget);
    TRACE("(%p)->(%p,%08x): Relay\n", This, NewTarget, Flags);

    /* Flags: Not used */

    return IWineD3DDevice_SetRenderTarget(This->wineD3DDevice,
                                          0,
                                          Target ? Target->WineD3DSurface : NULL);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderTarget(IDirect3DDevice3 *iface,
                                            IDirectDrawSurface4 *NewRenderTarget,
                                            DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirectDrawSurfaceImpl *Target = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, NewRenderTarget);
    TRACE_(ddraw_thunk)("(%p)->(%p,%08x) thunking to IDirect3DDevice7 interface.\n", This, Target, Flags);
    return IDirect3DDevice7_SetRenderTarget(ICOM_INTERFACE(This, IDirect3DDevice7),
                                            ICOM_INTERFACE(Target, IDirectDrawSurface7),
                                            Flags);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderTarget(IDirect3DDevice2 *iface,
                                            IDirectDrawSurface *NewRenderTarget,
                                            DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    IDirectDrawSurfaceImpl *Target = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface3, NewRenderTarget);
    TRACE_(ddraw_thunk)("(%p)->(%p,%08x) thunking to IDirect3DDevice7 interface.\n", This, Target, Flags);
    return IDirect3DDevice7_SetRenderTarget(ICOM_INTERFACE(This, IDirect3DDevice7),
                                            ICOM_INTERFACE(Target, IDirectDrawSurface7),
                                            Flags);
}

/*****************************************************************************
 * IDirect3DDevice7::GetRenderTarget
 *
 * Returns the current render target.
 * This is handled locally, because the WineD3D render target's parent
 * is an IParent
 *
 * Version 2, 3 and 7
 *
 * Params:
 *  RenderTarget: Address to store the surface interface pointer
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if RenderTarget == NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetRenderTarget(IDirect3DDevice7 *iface,
                                      IDirectDrawSurface7 **RenderTarget)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%p): Relay\n", This, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    *RenderTarget = ICOM_INTERFACE(This->target, IDirectDrawSurface7);
    IDirectDrawSurface7_AddRef(*RenderTarget);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderTarget(IDirect3DDevice3 *iface,
                                            IDirectDrawSurface4 **RenderTarget)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, RenderTarget);
    hr = IDirect3DDevice7_GetRenderTarget(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          (IDirectDrawSurface7 **) RenderTarget);
    if(hr != D3D_OK) return hr;
    *RenderTarget = (IDirectDrawSurface4 *) COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface7, IDirectDrawSurface7, *RenderTarget);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderTarget(IDirect3DDevice2 *iface,
                                            IDirectDrawSurface **RenderTarget)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, RenderTarget);
    hr = IDirect3DDevice7_GetRenderTarget(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          (IDirectDrawSurface7 **) RenderTarget);
    if(hr != D3D_OK) return hr;
    *RenderTarget = (IDirectDrawSurface *) COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface7, IDirectDrawSurface3, *RenderTarget);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice3::Begin
 *
 * Begins a description block of vertices. This is similar to glBegin()
 * and glEnd(). After a call to IDirect3DDevice3::End, the vertices
 * described with IDirect3DDevice::Vertex are drawn.
 *
 * Version 2 and 3
 *
 * Params:
 *  PrimitiveType: The type of primitives to draw
 *  VertexTypeDesc: A flexible vertex format description of the vertices
 *  Flags: Some flags..
 *
 * Returns:
 *  D3D_OK on success
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_Begin(IDirect3DDevice3 *iface,
                            D3DPRIMITIVETYPE PrimitiveType,
                            DWORD VertexTypeDesc,
                            DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p)->(%d,%d,%08x)\n", This, PrimitiveType, VertexTypeDesc, Flags);

    This->primitive_type = PrimitiveType;
    This->vertex_type = VertexTypeDesc;
    This->render_flags = Flags;
    This->vertex_size = get_flexible_vertex_size(This->vertex_type);
    This->nb_vertices = 0;

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Begin(IDirect3DDevice2 *iface,
                                  D3DPRIMITIVETYPE d3dpt,
                                  D3DVERTEXTYPE dwVertexTypeDesc,
                                  DWORD dwFlags)
{
    DWORD FVF;
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p/%p)->(%08x,%08x,%08x): Thunking to IDirect3DDevice3\n", This, iface, d3dpt, dwVertexTypeDesc, dwFlags);

    switch(dwVertexTypeDesc)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", dwVertexTypeDesc);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    };

    return IDirect3DDevice3_Begin(ICOM_INTERFACE(This, IDirect3DDevice3),
                                  d3dpt,
                                  FVF,
                                  dwFlags);
}

/*****************************************************************************
 * IDirect3DDevice3::BeginIndexed
 *
 * Draws primitives based on vertices in a vertex array which are specified
 * by indices.
 *
 * Version 2 and 3
 *
 * Params:
 *  PrimitiveType: Primitive type to draw
 *  VertexType: A FVF description of the vertex format
 *  Vertices: pointer to an array containing the vertices
 *  NumVertices: The number of vertices in the vertex array
 *  Flags: Some flags ...
 *
 * Returns:
 *  D3D_OK, because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_BeginIndexed(IDirect3DDevice3 *iface,
                                   D3DPRIMITIVETYPE PrimitiveType,
                                   DWORD VertexType,
                                   void *Vertices,
                                   DWORD NumVertices,
                                   DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p)->(%08x,%08x,%p,%08x,%08x): stub!\n", This, PrimitiveType, VertexType, Vertices, NumVertices, Flags);
    return D3D_OK;
}


static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_BeginIndexed(IDirect3DDevice2 *iface,
                                         D3DPRIMITIVETYPE d3dptPrimitiveType,
                                         D3DVERTEXTYPE d3dvtVertexType,
                                         void *lpvVertices,
                                         DWORD dwNumVertices,
                                         DWORD dwFlags)
{
    DWORD FVF;
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p/%p)->(%08x,%08x,%p,%08x,%08x): Thunking to IDirect3DDevice3\n", This, iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwNumVertices, dwFlags);

    switch(d3dvtVertexType)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", d3dvtVertexType);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    };

    return IDirect3DDevice3_BeginIndexed(ICOM_INTERFACE(This,IDirect3DDevice3),
                                         d3dptPrimitiveType,
                                         FVF,
                                         lpvVertices,
                                         dwNumVertices,
                                         dwFlags);
}

/*****************************************************************************
 * IDirect3DDevice3::Vertex
 *
 * Draws a vertex as described by IDirect3DDevice3::Begin. It places all
 * drawn vertices in a vertex buffer. If the buffer is too small, its
 * size is increased.
 *
 * Version 2 and 3
 *
 * Params:
 *  Vertex: Pointer to the vertex
 *
 * Returns:
 *  D3D_OK, on success
 *  DDERR_INVALIDPARAMS if Vertex is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_Vertex(IDirect3DDevice3 *iface,
                             void *Vertex)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p)->(%p)\n", This, Vertex);

    if(!Vertex)
        return DDERR_INVALIDPARAMS;

    if ((This->nb_vertices+1)*This->vertex_size > This->buffer_size)
    {
        BYTE *old_buffer;
        This->buffer_size = This->buffer_size ? This->buffer_size * 2 : This->vertex_size * 3;
        old_buffer = This->vertex_buffer;
        This->vertex_buffer = HeapAlloc(GetProcessHeap(), 0, This->buffer_size);
        if (old_buffer)
        {
            CopyMemory(This->vertex_buffer, old_buffer, This->nb_vertices * This->vertex_size);
            HeapFree(GetProcessHeap(), 0, old_buffer);
        }
    }

    CopyMemory(This->vertex_buffer + This->nb_vertices++ * This->vertex_size, Vertex, This->vertex_size);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Vertex(IDirect3DDevice2 *iface,
                                   void *lpVertexType)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, lpVertexType);
    return IDirect3DDevice3_Vertex(ICOM_INTERFACE(This, IDirect3DDevice3),
                                                  lpVertexType);
}

/*****************************************************************************
 * IDirect3DDevice3::Index
 *
 * Specifies an index to a vertex to be drawn. The vertex array has to
 * be specified with BeginIndexed first.
 *
 * Parameters:
 *  VertexIndex: The index of the vertex to draw
 *
 * Returns:
 *  D3D_OK because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_Index(IDirect3DDevice3 *iface,
                            WORD VertexIndex)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    FIXME("(%p)->(%04x): stub!\n", This, VertexIndex);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Index(IDirect3DDevice2 *iface,
                                  WORD wVertexIndex)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%04x) thunking to IDirect3DDevice3 interface.\n", This, wVertexIndex);
    return IDirect3DDevice3_Index(ICOM_INTERFACE(This, IDirect3DDevice3),
                                  wVertexIndex);
}

/*****************************************************************************
 * IDirect3DDevice3::End
 *
 * Ends a draw begun with IDirect3DDevice3::Begin or
 * IDirect3DDevice::BeginIndexed. The vertices specified with
 * IDirect3DDevice::Vertex or IDirect3DDevice::Index are drawn using
 * the IDirect3DDevice7::DrawPrimitive method. So far only
 * non-indexed mode is supported
 *
 * Version 2 and 3
 *
 * Params:
 *  Flags: Some flags, as usual. Don't know which are defined
 *
 * Returns:
 *  The return value of IDirect3DDevice7::DrawPrimitive
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_End(IDirect3DDevice3 *iface,
                          DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE("(%p)->(%08x)\n", This, Flags);

    return IDirect3DDevice7_DrawPrimitive(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          This->primitive_type, This->vertex_type,
                                          This->vertex_buffer, This->nb_vertices,
                                          This->render_flags);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_End(IDirect3DDevice2 *iface,
                                DWORD dwFlags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x) thunking to IDirect3DDevice3 interface.\n", This, dwFlags);
    return IDirect3DDevice3_End(ICOM_INTERFACE(This, IDirect3DDevice3),
                                dwFlags);
}

/*****************************************************************************
 * IDirect3DDevice7::GetRenderState
 *
 * Returns the value of a render state. The possible render states are
 * defined in include/d3dtypes.h
 *
 * Version 2, 3 and 7
 *
 * Params:
 *  RenderStateType: Render state to return the current setting of
 *  Value: Address to store the value at
 *
 * Returns:
 *  D3D_OK on success, for details see IWineD3DDevice::GetRenderState
 *  DDERR_INVALIDPARAMS if Value == NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetRenderState(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD *Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay\n", This, RenderStateType, Value);

    if(!Value)
        return DDERR_INVALIDPARAMS;

    switch(RenderStateType)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            /* This state is wrapped to SetTexture in SetRenderState, so
             * it has to be wrapped to GetTexture here
             */
            IWineD3DBaseTexture *tex = NULL;
            *Value = 0;

            hr = IWineD3DDevice_GetTexture(This->wineD3DDevice,
                                           0,
                                           &tex);

            if(hr == WINED3D_OK && tex)
            {
                IDirectDrawSurface7 *parent = NULL;
                hr = IWineD3DBaseTexture_GetParent(tex,
                                                   (IUnknown **) &parent);
                if(parent)
                {
                    /* The parent of the texture is the IDirectDrawSurface7 interface
                     * of the ddraw surface
                     */
                    IDirectDrawSurfaceImpl *texImpl = ICOM_OBJECT(IDirectDrawSurfaceImpl,
                                                                  IDirectDrawSurface7,
                                                                  parent);
                    *Value = texImpl->Handle;
                    IDirectDrawSurface7_Release(parent);
                }
                IWineD3DBaseTexture_Release(tex);
            }
            return hr;
        }

        case D3DRENDERSTATE_TEXTUREMAG:
        {
            WINED3DTEXTUREFILTERTYPE tex_mag;

            hr = IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_MAGFILTER,
                                                &tex_mag);

            switch (tex_mag)
            {
                case WINED3DTEXF_POINT:
                    *Value = D3DFILTER_NEAREST;
                    break;
                case WINED3DTEXF_LINEAR:
                    *Value = D3DFILTER_LINEAR;
                    break;
                default:
                    ERR("Unhandled texture mag %d !\n",tex_mag);
                    *Value = 0;
            }
            return hr;
        }

        case D3DRENDERSTATE_TEXTUREMIN:
        {
            WINED3DTEXTUREFILTERTYPE tex_min;

            hr = IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_MINFILTER,
                                                &tex_min);

            switch (tex_min)
            {
                case WINED3DTEXF_POINT:
                    *Value = D3DFILTER_NEAREST;
                    break;
                case WINED3DTEXF_LINEAR:
                    *Value = D3DFILTER_LINEAR;
                    break;
                default:
                    ERR("Unhandled texture mag %d !\n",tex_min);
                    *Value = 0;
            }
            return hr;
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            return IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_ADDRESSU,
                                                  Value);
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            return IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_ADDRESSV,
                                                  Value);

        default:
            /* FIXME: Unhandled: D3DRENDERSTATE_STIPPLEPATTERN00 - 31 */
            return IWineD3DDevice_GetRenderState(This->wineD3DDevice,
                                                 RenderStateType,
                                                 Value);
    }
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderState(IDirect3DDevice3 *iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD *lpdwRenderState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, dwRenderStateType, lpdwRenderState);
    return IDirect3DDevice7_GetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           dwRenderStateType,
                                           lpdwRenderState);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderState(IDirect3DDevice2 *iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD *lpdwRenderState)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, dwRenderStateType, lpdwRenderState);
    return IDirect3DDevice7_GetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           dwRenderStateType,
                                           lpdwRenderState);
}

/*****************************************************************************
 * IDirect3DDevice7::SetRenderState
 *
 * Sets a render state. The possible render states are defined in
 * include/d3dtypes.h
 *
 * Version 2, 3 and 7
 *
 * Params:
 *  RenderStateType: State to set
 *  Value: Value to assign to that state
 *
 * Returns:
 *  D3D_OK on success,
 *  for details see IWineD3DDevice::SetRenderState
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetRenderState(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%08x,%d): Relay\n", This, RenderStateType, Value);

    /* Some render states need special care */
    switch(RenderStateType)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            if(Value == 0)
            {
                    return IWineD3DDevice_SetTexture(This->wineD3DDevice,
                                                     0,
                                                     NULL);
            }

            if(Value > This->numHandles)
            {
                FIXME("Specified handle %d out of range\n", Value);
                return DDERR_INVALIDPARAMS;
            }
            if(This->Handles[Value - 1].type != DDrawHandle_Texture)
            {
                FIXME("Handle %d isn't a texture handle\n", Value);
                return DDERR_INVALIDPARAMS;
            }
            else
            {
                IDirectDrawSurfaceImpl *surf = (IDirectDrawSurfaceImpl *) This->Handles[Value - 1].ptr;
                return IWineD3DDevice_SetTexture(This->wineD3DDevice,
                                                 0,
                                                 (IWineD3DBaseTexture *) surf->wineD3DTexture);
            }
        }

        case D3DRENDERSTATE_TEXTUREMAG:
        {
            WINED3DTEXTUREFILTERTYPE tex_mag = WINED3DTEXF_NONE;

            switch ((D3DTEXTUREFILTER) Value)
            {
                case D3DFILTER_NEAREST:
                case D3DFILTER_LINEARMIPNEAREST:
                    tex_mag = WINED3DTEXF_POINT;
                    break;
                case D3DFILTER_LINEAR:
                case D3DFILTER_LINEARMIPLINEAR:
                    tex_mag = WINED3DTEXF_LINEAR;
                    break;
                default:
                    ERR("Unhandled texture mag %d !\n",Value);
            }

            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_MAGFILTER,
                                                  tex_mag);
        }

        case D3DRENDERSTATE_TEXTUREMIN:
        {
            WINED3DTEXTUREFILTERTYPE tex_min = WINED3DTEXF_NONE;
            WINED3DTEXTUREFILTERTYPE tex_mip = WINED3DTEXF_NONE;

            switch ((D3DTEXTUREFILTER) Value)
            {
                case D3DFILTER_NEAREST:
                    tex_min = WINED3DTEXF_POINT;
                    break;
                case D3DFILTER_LINEAR:
                    tex_min = WINED3DTEXF_LINEAR;
                    break;
                case D3DFILTER_MIPNEAREST:
                    tex_min = WINED3DTEXF_NONE;
                    tex_mip = WINED3DTEXF_POINT;
                    break;
                case D3DFILTER_MIPLINEAR:
                    tex_min = WINED3DTEXF_NONE;
                    tex_mip = WINED3DTEXF_LINEAR;
                    break;
                case D3DFILTER_LINEARMIPNEAREST:
                    tex_min = WINED3DTEXF_POINT;
                    tex_mip = WINED3DTEXF_LINEAR;
                    break;
                case D3DFILTER_LINEARMIPLINEAR:
                    tex_min = WINED3DTEXF_LINEAR;
                    tex_mip = WINED3DTEXF_LINEAR;
                    break;

                default:
                    ERR("Unhandled texture min %d !\n",Value);
            }

                   IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_MIPFILTER,
                                                  tex_mip);
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_MINFILTER,
                                                  tex_min);
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
                   IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_ADDRESSV,
                                                  Value);
            /* Drop through */
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_ADDRESSU,
                                                  Value);
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_ADDRESSV,
                                                  Value);

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
        {
            /* Old texture combine setup style, superseded by texture stage states
             * in D3D7. It is safe for us to wrap it to texture stage states.
             */
            switch ( (D3DTEXTUREBLEND) Value)
            {
                case D3DTBLEND_MODULATE:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_MODULATE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_SELECTARG1);
                    break;

                case D3DTBLEND_MODULATEALPHA:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_MODULATE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_MODULATE);
                    break;

                case D3DTBLEND_DECAL:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_SELECTARG1);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_SELECTARG1);
                    break;

                case D3DTBLEND_DECALALPHA:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_SELECTARG1);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_MODULATE);
                    break;

                default:
                    ERR("Unhandled texture environment %d !\n",Value);
                }
                return D3D_OK;
            break;
        }

        default:

            /* FIXME: Unhandled: D3DRENDERSTATE_STIPPLEPATTERN00 - 31 */

            return IWineD3DDevice_SetRenderState(This->wineD3DDevice,
                                                 RenderStateType,
                                                 Value);
    }
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderState(IDirect3DDevice3 *iface,
                                           D3DRENDERSTATETYPE RenderStateType,
                                           DWORD Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, RenderStateType, Value);
    return IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           RenderStateType,
                                           Value);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderState(IDirect3DDevice2 *iface,
                                           D3DRENDERSTATETYPE RenderStateType,
                                           DWORD Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, RenderStateType, Value);
    return IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           RenderStateType,
                                           Value);
}

/*****************************************************************************
 * Direct3DDevice3::SetLightState
 *
 * Sets a light state for Direct3DDevice3 and Direct3DDevice2. The
 * light states are forwarded to Direct3DDevice7 render states
 *
 * Version 2 and 3
 *
 * Params:
 *  LightStateType: The light state to change
 *  Value: The value to assign to that light state
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if the parameters were incorrect
 *  Also check IDirect3DDevice7::SetRenderState
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_SetLightState(IDirect3DDevice3 *iface,
                                    D3DLIGHTSTATETYPE LightStateType,
                                    DWORD Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);

    TRACE("(%p)->(%08x,%08x)\n", This, LightStateType, Value);

    if (!LightStateType && (LightStateType > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    if (LightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */)
    {
        IDirect3DMaterialImpl *mat;

        if(Value == 0) mat = NULL;
        else if(Value > This->numHandles)
        {
            ERR("Material handle out of range(%d)\n", Value);
            return DDERR_INVALIDPARAMS;
        }
        else if(This->Handles[Value - 1].type != DDrawHandle_Material)
        {
            ERR("Invalid handle %d\n", Value);
            return DDERR_INVALIDPARAMS;
        }
        else
        {
            mat = (IDirect3DMaterialImpl *) This->Handles[Value - 1].ptr;
        }

        if (mat != NULL)
        {
            TRACE(" activating material %p.\n", mat);
            mat->activate(mat);
        }
        else
        {
            FIXME(" D3DLIGHTSTATE_MATERIAL called with NULL material !!!\n");
        }
        This->material = Value;
    }
    else if (LightStateType == D3DLIGHTSTATE_COLORMODEL /* 3 */)
    {
        switch (Value)
        {
            case D3DCOLOR_MONO:
                ERR("DDCOLOR_MONO should not happen!\n");
                break;
            case D3DCOLOR_RGB:
                /* We are already in this mode */
                TRACE("Setting color model to RGB (no-op).\n");
                break;
            default:
                ERR("Unknown color model!\n");
                return DDERR_INVALIDPARAMS;
        }
    }
    else
    {
        D3DRENDERSTATETYPE rs;
        switch (LightStateType)
        {
            case D3DLIGHTSTATE_AMBIENT:       /* 2 */
                rs = D3DRENDERSTATE_AMBIENT;
                break;		
            case D3DLIGHTSTATE_FOGMODE:       /* 4 */
                rs = D3DRENDERSTATE_FOGVERTEXMODE;
                break;
            case D3DLIGHTSTATE_FOGSTART:      /* 5 */
                rs = D3DRENDERSTATE_FOGSTART;
                break;
            case D3DLIGHTSTATE_FOGEND:        /* 6 */
                rs = D3DRENDERSTATE_FOGEND;
                break;
            case D3DLIGHTSTATE_FOGDENSITY:    /* 7 */
                rs = D3DRENDERSTATE_FOGDENSITY;
                break;
            case D3DLIGHTSTATE_COLORVERTEX:   /* 8 */
                rs = D3DRENDERSTATE_COLORVERTEX;
                break;
            default:
                ERR("Unknown D3DLIGHTSTATETYPE %d.\n", LightStateType);
                return DDERR_INVALIDPARAMS;
        }

        return IDirect3DDevice7_SetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7), 
                                               rs,
                                               Value);
    }

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetLightState(IDirect3DDevice2 *iface,
                                          D3DLIGHTSTATETYPE LightStateType,
                                          DWORD Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x) thunking to IDirect3DDevice3 interface.\n", This, LightStateType, Value);
    return IDirect3DDevice3_SetLightState(ICOM_INTERFACE(This, IDirect3DDevice3),
                                          LightStateType,
                                          Value);
}

/*****************************************************************************
 * IDirect3DDevice3::GetLightState
 *
 * Returns the current setting of a light state. The state is read from
 * the Direct3DDevice7 render state.
 *
 * Version 2 and 3
 *
 * Params:
 *  LightStateType: The light state to return
 *  Value: The address to store the light state setting at
 *
 * Returns:
 *  D3D_OK on success
 *  DDDERR_INVALIDPARAMS if the parameters were incorrect
 *  Also see IDirect3DDevice7::GetRenderState
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_3_GetLightState(IDirect3DDevice3 *iface,
                                    D3DLIGHTSTATETYPE LightStateType,
                                    DWORD *Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);

    TRACE("(%p)->(%08x,%p)\n", This, LightStateType, Value);

    if (!LightStateType && (LightStateType > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    if(!Value)
        return DDERR_INVALIDPARAMS;

    if (LightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */)
    {
        *Value = This->material;
    }
    else if (LightStateType == D3DLIGHTSTATE_COLORMODEL /* 3 */)
    {
        *Value = D3DCOLOR_RGB;
    }
    else
    {
        D3DRENDERSTATETYPE rs;
        switch (LightStateType)
        {
            case D3DLIGHTSTATE_AMBIENT:       /* 2 */
                rs = D3DRENDERSTATE_AMBIENT;
                break;		
            case D3DLIGHTSTATE_FOGMODE:       /* 4 */
                rs = D3DRENDERSTATE_FOGVERTEXMODE;
                break;
            case D3DLIGHTSTATE_FOGSTART:      /* 5 */
                rs = D3DRENDERSTATE_FOGSTART;
                break;
            case D3DLIGHTSTATE_FOGEND:        /* 6 */
                rs = D3DRENDERSTATE_FOGEND;
                break;
            case D3DLIGHTSTATE_FOGDENSITY:    /* 7 */
                rs = D3DRENDERSTATE_FOGDENSITY;
                break;
            case D3DLIGHTSTATE_COLORVERTEX:   /* 8 */
                rs = D3DRENDERSTATE_COLORVERTEX;
                break;
            default:
                ERR("Unknown D3DLIGHTSTATETYPE %d.\n", LightStateType);
                return DDERR_INVALIDPARAMS;
        }

        return IDirect3DDevice7_GetRenderState(ICOM_INTERFACE(This, IDirect3DDevice7),
                                               rs,
                                               Value);
    }

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetLightState(IDirect3DDevice2 *iface,
                                          D3DLIGHTSTATETYPE LightStateType,
                                          DWORD *Value)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice3 interface.\n", This, LightStateType, Value);
    return IDirect3DDevice3_GetLightState(ICOM_INTERFACE(This, IDirect3DDevice3),
                                          LightStateType,
                                          Value);
}

/*****************************************************************************
 * IDirect3DDevice7::SetTransform
 *
 * Assigns a D3DMATRIX to a transform type. The transform types are defined
 * in include/d3dtypes.h.
 * The D3DTRANSFORMSTATE_WORLD (=1) is translated to D3DTS_WORLDMATRIX(0)
 * (=255) for wined3d, because the 1 transform state was removed in d3d8
 * and WineD3D already understands the replacement D3DTS_WORLDMATRIX(0)
 *
 * Version 2, 3 and 7
 *
 * Params:
 *  TransformStateType: transform state to set
 *  Matrix: Matrix to assign to the state
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Matrix == NULL
 *  For details see IWineD3DDevice::SetTransform
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTransform(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    D3DTRANSFORMSTATETYPE type = TransformStateType;
    TRACE("(%p)->(%08x,%p): Relay\n", This, TransformStateType, Matrix);

    if(!Matrix)
        return DDERR_INVALIDPARAMS;

    /* D3DTRANSFORMSTATE_WORLD doesn't exist in WineD3D,
     * use D3DTS_WORLDMATRIX(0) instead
     * D3DTS_WORLDMATRIX(index) is (D3DTRANSFORMSTATETYPE)(index + 256)
     */
    if(TransformStateType == D3DTRANSFORMSTATE_WORLD)
        type = (D3DTRANSFORMSTATETYPE)(0 + 256);

    /* FIXME:
       Unhandled: D3DTRANSFORMSTATE_WORLD1
       Unhandled: D3DTRANSFORMSTATE_WORLD2
       Unhandled: D3DTRANSFORMSTATE_WORLD3
     */

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    return IWineD3DDevice_SetTransform(This->wineD3DDevice,
                                       type,
                                       (WINED3DMATRIX*) Matrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTransform(IDirect3DDevice3 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_SetTransform(ICOM_INTERFACE(This, IDirect3DDevice7),
                                         TransformStateType,
                                         D3DMatrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetTransform(IDirect3DDevice2 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_SetTransform(ICOM_INTERFACE(This, IDirect3DDevice7),
                                         TransformStateType,
                                         D3DMatrix);
}

/*****************************************************************************
 * IDirect3DDevice7::GetTransform
 *
 * Returns the matrix assigned to a transform state
 * D3DTRANSFORMSTATE_WORLD is translated to D3DTS_WORLDMATRIX(0), see
 * SetTransform
 *
 * Params:
 *  TransformStateType: State to read the matrix from
 *  Matrix: Address to store the matrix at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Matrix == NULL
 *  For details, see IWineD3DDevice::GetTransform
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTransform(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    D3DTRANSFORMSTATETYPE type = TransformStateType;
    TRACE("(%p)->(%08x,%p): Relay\n", This, TransformStateType, Matrix);

    if(!Matrix)
        return DDERR_INVALIDPARAMS;

    /* D3DTRANSFORMSTATE_WORLD doesn't exist in WineD3D,
     * use D3DTS_WORLDMATRIX(0) instead
     * D3DTS_WORLDMATRIX(index) is (D3DTRANSFORMSTATETYPE)(index + 256)
     */
    if(TransformStateType == D3DTRANSFORMSTATE_WORLD)
        type = (D3DTRANSFORMSTATETYPE)(0 + 256);

    /* FIXME:
       Unhandled: D3DTRANSFORMSTATE_WORLD1
       Unhandled: D3DTRANSFORMSTATE_WORLD2
       Unhandled: D3DTRANSFORMSTATE_WORLD3
     */

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    return IWineD3DDevice_GetTransform(This->wineD3DDevice, type, (WINED3DMATRIX*) Matrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTransform(IDirect3DDevice3 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_GetTransform(ICOM_INTERFACE(This, IDirect3DDevice7),
                                         TransformStateType,
                                         D3DMatrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetTransform(IDirect3DDevice2 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_GetTransform(ICOM_INTERFACE(This, IDirect3DDevice7),
                                         TransformStateType,
                                         D3DMatrix);
}

/*****************************************************************************
 * IDirect3DDevice7::MultiplyTransform
 *
 * Multiplies the already-set transform matrix of a transform state
 * with another matrix. For the world matrix, see SetTransform
 *
 * Version 2, 3 and 7
 *
 * Params:
 *  TransformStateType: Transform state to multiply
 *  D3DMatrix Matrix to multiply with.
 *
 * Returns
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if D3DMatrix is NULL
 *  For details, see IWineD3DDevice::MultiplyTransform
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_MultiplyTransform(IDirect3DDevice7 *iface,
                                        D3DTRANSFORMSTATETYPE TransformStateType,
                                        D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%08x,%p): Relay\n", This, TransformStateType, D3DMatrix);

    /* D3DTRANSFORMSTATE_WORLD doesn't exist in WineD3D,
     * use D3DTS_WORLDMATRIX(0) instead
     * D3DTS_WORLDMATRIX(index) is (D3DTRANSFORMSTATETYPE)(index + 256)
     */
    if(TransformStateType == D3DTRANSFORMSTATE_WORLD)
        TransformStateType = (D3DTRANSFORMSTATETYPE)(0 + 256);

    /* FIXME:
       Unhandled: D3DTRANSFORMSTATE_WORLD1
       Unhandled: D3DTRANSFORMSTATE_WORLD2
       Unhandled: D3DTRANSFORMSTATE_WORLD3
     */

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    return IWineD3DDevice_MultiplyTransform(This->wineD3DDevice,
                                            TransformStateType,
                                            (WINED3DMATRIX*) D3DMatrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_MultiplyTransform(IDirect3DDevice3 *iface,
                                              D3DTRANSFORMSTATETYPE TransformStateType,
                                              D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_MultiplyTransform(ICOM_INTERFACE(This, IDirect3DDevice7),
                                              TransformStateType,
                                              D3DMatrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_MultiplyTransform(IDirect3DDevice2 *iface,
                                              D3DTRANSFORMSTATETYPE TransformStateType,
                                              D3DMATRIX *D3DMatrix)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_MultiplyTransform(ICOM_INTERFACE(This, IDirect3DDevice7),
                                              TransformStateType,
                                              D3DMatrix);
}

/*****************************************************************************
 * IDirect3DDevice7::DrawPrimitive
 *
 * Draws primitives based on vertices in an application-provided pointer
 *
 * Version 2, 3 and 7. The IDirect3DDevice2 thunk converts the fixed vertex type into
 * an FVF format for D3D7
 *
 * Params:
 *  PrimitiveType: The type of the primitives to draw
 *  Vertex type: Flexible vertex format vertex description
 *  Vertices: Pointer to the vertex array
 *  VertexCount: The number of vertices to draw
 *  Flags: As usual a few flags
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Vertices is NULL
 *  For details, see IWineD3DDevice::DrawPrimitiveUP
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitive(IDirect3DDevice7 *iface,
                                    D3DPRIMITIVETYPE PrimitiveType,
                                    DWORD VertexType,
                                    void *Vertices,
                                    DWORD VertexCount,
                                    DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    UINT PrimitiveCount, stride;
    HRESULT hr;
    TRACE("(%p)->(%08x,%08x,%p,%08x,%08x): Relay!\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Flags);

    if(!Vertices)
        return DDERR_INVALIDPARAMS;

    /* Get the vertex count */
    switch(PrimitiveType)
    {
      case D3DPT_POINTLIST: 
        PrimitiveCount = VertexCount;
        break;

      case D3DPT_LINELIST: 
        PrimitiveCount = VertexCount / 2;
        break;

      case D3DPT_LINESTRIP:
        PrimitiveCount = VertexCount - 1;
        break;

      case D3DPT_TRIANGLELIST:
        PrimitiveCount = VertexCount / 3;
        break;

      case D3DPT_TRIANGLESTRIP:
        PrimitiveCount = VertexCount - 2;
        break;

      case D3DPT_TRIANGLEFAN:
        PrimitiveCount = VertexCount - 2;
        break;

      default: return DDERR_INVALIDPARAMS;
    }

    /* Get the stride */
    stride = get_flexible_vertex_size(VertexType);

    /* Set the FVF */
    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             IDirectDrawImpl_FindDecl(This->ddraw, VertexType));
    if(hr != D3D_OK) return hr;

    /* This method translates to the user pointer draw of WineD3D */
    return IWineD3DDevice_DrawPrimitiveUP(This->wineD3DDevice,
                                          PrimitiveType,
                                          PrimitiveCount,
                                          Vertices,
                                          stride);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitive(IDirect3DDevice3 *iface,
                                          D3DPRIMITIVETYPE PrimitiveType,
                                          DWORD VertexType,
                                          void *Vertices,
                                          DWORD VertexCount,
                                          DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Flags);
    return IDirect3DDevice7_DrawPrimitive(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          PrimitiveType,
                                          VertexType,
                                          Vertices,
                                          VertexCount,
                                          Flags);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DrawPrimitive(IDirect3DDevice2 *iface,
                                          D3DPRIMITIVETYPE PrimitiveType,
                                          D3DVERTEXTYPE VertexType,
                                          void *Vertices,
                                          DWORD VertexCount,
                                          DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    DWORD FVF;
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Flags);

    switch(VertexType)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", VertexType);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    }

    return IDirect3DDevice7_DrawPrimitive(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          PrimitiveType,
                                          FVF,
                                          Vertices,
                                          VertexCount,
                                          Flags);
}

/*****************************************************************************
 * IDirect3DDevice7::DrawIndexedPrimitive
 *
 * Draws vertices from an application-provided pointer, based on the index
 * numbers in a WORD array.
 *
 * Version 2, 3 and 7. The version 7 thunk translates the vertex type into
 * an FVF format for D3D7
 *
 * Params:
 *  PrimitiveType: The primitive type to draw
 *  VertexType: The FVF vertex description
 *  Vertices: Pointer to the vertex array
 *  VertexCount: ?
 *  Indices: Pointer to the index array
 *  IndexCount: Number of indices = Number of vertices to draw
 *  Flags: As usual, some flags
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Vertices or Indices is NULL
 *  For details, see IWineD3DDevice::DrawIndexedPrimitiveUP
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitive(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           void *Vertices,
                                           DWORD VertexCount,
                                           WORD *Indices,
                                           DWORD IndexCount,
                                           DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    UINT PrimitiveCount = 0;
    HRESULT hr;
    TRACE("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x): Relay!\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);
    /* Get the primitive number */
    switch(PrimitiveType)
    {
      case D3DPT_POINTLIST: 
        PrimitiveCount = IndexCount;
        break;

      case D3DPT_LINELIST: 
        PrimitiveCount = IndexCount / 2;
        break;

      case D3DPT_LINESTRIP:
        PrimitiveCount = IndexCount - 1;
        break;

      case D3DPT_TRIANGLELIST:
        PrimitiveCount = IndexCount / 3;
        break;

      case D3DPT_TRIANGLESTRIP:
        PrimitiveCount = IndexCount - 2;
        break;

      case D3DPT_TRIANGLEFAN:
        PrimitiveCount = IndexCount - 2;
        break;

      default: return DDERR_INVALIDPARAMS;
    }

    /* Set the D3DDevice's FVF */
    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             IDirectDrawImpl_FindDecl(This->ddraw, VertexType));
    if(FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        return hr;
    }

    return IWineD3DDevice_DrawIndexedPrimitiveUP(This->wineD3DDevice,
                                                 PrimitiveType,
                                                 0 /* MinVertexIndex */,
                                                 VertexCount /* UINT NumVertexIndex */,
                                                 PrimitiveCount,
                                                 Indices,
                                                 WINED3DFMT_INDEX16,
                                                 Vertices,
                                                 get_flexible_vertex_size(VertexType));
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitive(IDirect3DDevice3 *iface,
                                                 D3DPRIMITIVETYPE PrimitiveType,
                                                 DWORD VertexType,
                                                 void *Vertices,
                                                 DWORD VertexCount,
                                                 WORD *Indices,
                                                 DWORD IndexCount,
                                                 DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);
    return IDirect3DDevice7_DrawIndexedPrimitive(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                 PrimitiveType,
                                                 VertexType,
                                                 Vertices,
                                                 VertexCount,
                                                 Indices,
                                                 IndexCount,
                                                 Flags);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DrawIndexedPrimitive(IDirect3DDevice2 *iface,
                                                 D3DPRIMITIVETYPE PrimitiveType,
                                                 D3DVERTEXTYPE VertexType,
                                                 void *Vertices,
                                                 DWORD VertexCount,
                                                 WORD *Indices,
                                                 DWORD IndexCount,
                                                 DWORD Flags)
{
    DWORD FVF;
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);

    switch(VertexType)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", VertexType);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    }

    return IDirect3DDevice7_DrawIndexedPrimitive(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                 PrimitiveType,
                                                 FVF,
                                                 Vertices,
                                                 VertexCount,
                                                 Indices,
                                                 IndexCount,
                                                 Flags);
}

/*****************************************************************************
 * IDirect3DDevice7::SetClipStatus
 *
 * Sets the clip status. This defines things as clipping conditions and
 * the extents of the clipping region.
 *
 * Version 2, 3 and 7
 *
 * Params:
 *  ClipStatus:
 *
 * Returns:
 *  D3D_OK because it's a stub
 *  (DDERR_INVALIDPARAMS if ClipStatus == NULL)
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetClipStatus(IDirect3DDevice7 *iface,
                                    D3DCLIPSTATUS *ClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p)->(%p): Stub!\n", This, ClipStatus);

    /* D3DCLIPSTATUS and WINED3DCLIPSTATUS are different. I don't know how to convert them
     * Perhaps this needs a new data type and an additional IWineD3DDevice method
     */
    /* return IWineD3DDevice_SetClipStatus(This->wineD3DDevice, ClipStatus);*/
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetClipStatus(IDirect3DDevice3 *iface,
                                          D3DCLIPSTATUS *ClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_SetClipStatus(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          ClipStatus);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetClipStatus(IDirect3DDevice2 *iface,
                                          D3DCLIPSTATUS *ClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_SetClipStatus(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          ClipStatus);
}

/*****************************************************************************
 * IDirect3DDevice7::GetClipStatus
 *
 * Returns the clip status
 *
 * Params:
 *  ClipStatus: Address to write the clip status to
 *
 * Returns:
 *  D3D_OK because it's a stub
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetClipStatus(IDirect3DDevice7 *iface,
                                    D3DCLIPSTATUS *ClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p)->(%p): Stub!\n", This, ClipStatus);

    /* D3DCLIPSTATUS and WINED3DCLIPSTATUS are different. I don't know how to convert them */
    /* return IWineD3DDevice_GetClipStatus(This->wineD3DDevice, ClipStatus);*/
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetClipStatus(IDirect3DDevice3 *iface,
                                          D3DCLIPSTATUS *ClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_GetClipStatus(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          ClipStatus);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetClipStatus(IDirect3DDevice2 *iface,
                                          D3DCLIPSTATUS *ClipStatus)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice2, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_GetClipStatus(ICOM_INTERFACE(This, IDirect3DDevice7),
                                          ClipStatus);
}

/*****************************************************************************
 * IDirect3DDevice::DrawPrimitiveStrided
 *
 * Draws vertices described by a D3DDRAWPRIMITIVESTRIDEDDATA structure.
 *
 * Version 3 and 7
 *
 * Params:
 *  PrimitiveType: The primitive type to draw
 *  VertexType: The FVF description of the vertices to draw (for the stride??)
 *  D3DDrawPrimStrideData: A D3DDRAWPRIMITIVESTRIDEDDATA structure describing
 *                         the vertex data locations
 *  VertexCount: The number of vertices to draw
 *  Flags: Some flags
 *
 * Returns:
 *  D3D_OK, because it's a stub
 *  (DDERR_INVALIDPARAMS if D3DDrawPrimStrideData is NULL)
 *  (For details, see IWineD3DDevice::DrawPrimitiveStrided)
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitiveStrided(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                           DWORD VertexCount,
                                           DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    WineDirect3DVertexStridedData WineD3DStrided;
    int i;
    UINT PrimitiveCount;

    TRACE("(%p)->(%08x,%08x,%p,%08x,%08x): stub!\n", This, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);

    memset(&WineD3DStrided, 0, sizeof(WineD3DStrided));
    /* Get the strided data right. the wined3d structure is a bit bigger
     * Watch out: The contents of the strided data are determined by the fvf,
     * not by the members set in D3DDrawPrimStrideData. So it's valid
     * to have diffuse.lpvData set to 0xdeadbeef if the diffuse flag is
     * not set in the fvf.
     */
    if(VertexType & D3DFVF_POSITION_MASK)
    {
        WineD3DStrided.u.s.position.lpData = D3DDrawPrimStrideData->position.lpvData;
        WineD3DStrided.u.s.position.dwStride = D3DDrawPrimStrideData->position.dwStride;
        WineD3DStrided.u.s.position.dwType = WINED3DDECLTYPE_FLOAT3;
        if (VertexType & D3DFVF_XYZRHW)
        {
            WineD3DStrided.u.s.position.dwType = WINED3DDECLTYPE_FLOAT4;
            WineD3DStrided.u.s.position_transformed = TRUE;
        } else
            WineD3DStrided.u.s.position_transformed = FALSE;
    }

    if(VertexType & D3DFVF_NORMAL)
    {
        WineD3DStrided.u.s.normal.lpData = D3DDrawPrimStrideData->normal.lpvData;
        WineD3DStrided.u.s.normal.dwStride = D3DDrawPrimStrideData->normal.dwStride;
        WineD3DStrided.u.s.normal.dwType = WINED3DDECLTYPE_FLOAT3;
    }

    if(VertexType & D3DFVF_DIFFUSE)
    {
        WineD3DStrided.u.s.diffuse.lpData = D3DDrawPrimStrideData->diffuse.lpvData;
        WineD3DStrided.u.s.diffuse.dwStride = D3DDrawPrimStrideData->diffuse.dwStride;
        WineD3DStrided.u.s.diffuse.dwType = WINED3DDECLTYPE_SHORT4;
    }

    if(VertexType & D3DFVF_SPECULAR)
    {
        WineD3DStrided.u.s.specular.lpData = D3DDrawPrimStrideData->specular.lpvData;
        WineD3DStrided.u.s.specular.dwStride = D3DDrawPrimStrideData->specular.dwStride;
        WineD3DStrided.u.s.specular.dwType = WINED3DDECLTYPE_SHORT4;
    }

    for( i = 0; i < GET_TEXCOUNT_FROM_FVF(VertexType); i++)
    {
        WineD3DStrided.u.s.texCoords[i].lpData = D3DDrawPrimStrideData->textureCoords[i].lpvData;
        WineD3DStrided.u.s.texCoords[i].dwStride = D3DDrawPrimStrideData->textureCoords[i].dwStride;
        switch(GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i))
        {
            case 1: WineD3DStrided.u.s.texCoords[i].dwType = WINED3DDECLTYPE_FLOAT1; break;
            case 2: WineD3DStrided.u.s.texCoords[i].dwType = WINED3DDECLTYPE_FLOAT2; break;
            case 3: WineD3DStrided.u.s.texCoords[i].dwType = WINED3DDECLTYPE_FLOAT3; break;
            case 4: WineD3DStrided.u.s.texCoords[i].dwType = WINED3DDECLTYPE_FLOAT4; break;
            default: ERR("Unexpected texture coordinate size %d\n",
                         GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i));
        }
    }

    /* Get the primitive count */
    switch(PrimitiveType)
    {
        case D3DPT_POINTLIST: 
          PrimitiveCount = VertexCount;
          break;

        case D3DPT_LINELIST: 
          PrimitiveCount = VertexCount / 2;
          break;

        case D3DPT_LINESTRIP:
          PrimitiveCount = VertexCount - 1;
          break;

        case D3DPT_TRIANGLELIST:
          PrimitiveCount = VertexCount / 3;
          break;

        case D3DPT_TRIANGLESTRIP:
          PrimitiveCount = VertexCount - 2;
          break;

        case D3DPT_TRIANGLEFAN:
          PrimitiveCount = VertexCount - 2;
          break;

        default: return DDERR_INVALIDPARAMS;
    }

    /* WineD3D doesn't need the FVF here */
    return IWineD3DDevice_DrawPrimitiveStrided(This->wineD3DDevice,
                                               PrimitiveType,
                                               PrimitiveCount,
                                               &WineD3DStrided);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveStrided(IDirect3DDevice3 *iface,
                                                 D3DPRIMITIVETYPE PrimitiveType,
                                                 DWORD VertexType,
                                                 D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                                 DWORD VertexCount,
                                                 DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);
    return IDirect3DDevice7_DrawPrimitiveStrided(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                 PrimitiveType,
                                                 VertexType,
                                                 D3DDrawPrimStrideData,
                                                 VertexCount,
                                                 Flags);
}

/*****************************************************************************
 * IDirect3DDevice7::DrawIndexedPrimitiveStrided
 *
 * Draws primitives specified by strided data locations based on indices
 *
 * Version 3 and 7
 *
 * Params:
 *  PrimitiveType:
 *
 * Returns:
 *  D3D_OK, because it's a stub
 *  (DDERR_INVALIDPARAMS if D3DDrawPrimStrideData is NULL)
 *  (DDERR_INVALIDPARAMS if Indices is NULL)
 *  (For more details, see IWineD3DDevice::DrawIndexedPrimitiveStrided)
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided(IDirect3DDevice7 *iface,
                                                  D3DPRIMITIVETYPE PrimitiveType,
                                                  DWORD VertexType,
                                                  D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                                  DWORD VertexCount,
                                                  WORD *Indices,
                                                  DWORD IndexCount,
                                                  DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x): stub!\n", This, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);

    /* I'll implement it as soon as I find a app to test it.
     * This needs an additional method in IWineD3DDevice.
     */
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided(IDirect3DDevice3 *iface,
                                                        D3DPRIMITIVETYPE PrimitiveType,
                                                        DWORD VertexType,
                                                        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                                        DWORD VertexCount,
                                                        WORD *Indices,
                                                        DWORD IndexCount,
                                                        DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
    return IDirect3DDevice7_DrawIndexedPrimitiveStrided(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                        PrimitiveType,
                                                        VertexType,
                                                        D3DDrawPrimStrideData,
                                                        VertexCount,
                                                        Indices,
                                                        IndexCount,
                                                        Flags);
}

/*****************************************************************************
 * IDirect3DDevice7::DrawPrimitiveVB
 *
 * Draws primitives from a vertex buffer to the screen.
 *
 * Version 3 and 7
 *
 * Params:
 *  PrimitiveType: Type of primitive to be rendered.
 *  D3DVertexBuf: Source Vertex Buffer
 *  StartVertex: Index of the first vertex from the buffer to be rendered
 *  NumVertices: Number of vertices to be rendered
 *  Flags: Can be D3DDP_WAIT to wait until rendering has finished
 *
 * Return values
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if D3DVertexBuf is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitiveVB(IDirect3DDevice7 *iface,
                                      D3DPRIMITIVETYPE PrimitiveType,
                                      IDirect3DVertexBuffer7 *D3DVertexBuf,
                                      DWORD StartVertex,
                                      DWORD NumVertices,
                                      DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DVertexBufferImpl *vb = ICOM_OBJECT(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, D3DVertexBuf);
    UINT PrimitiveCount;
    HRESULT hr;
    DWORD stride;
    WINED3DVERTEXBUFFER_DESC Desc;

    TRACE("(%p)->(%08x,%p,%08x,%08x,%08x)\n", This, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);

    /* Sanity checks */
    if(!vb)
    {
        ERR("(%p) No Vertex buffer specified\n", This);
        return DDERR_INVALIDPARAMS;
    }

    /* Get the primitive count */
    switch(PrimitiveType)
    {
        case D3DPT_POINTLIST: 
          PrimitiveCount = NumVertices;
          break;

        case D3DPT_LINELIST: 
          PrimitiveCount = NumVertices / 2;
          break;

        case D3DPT_LINESTRIP:
          PrimitiveCount = NumVertices - 1;
          break;

        case D3DPT_TRIANGLELIST:
          PrimitiveCount = NumVertices / 3;
          break;

        case D3DPT_TRIANGLESTRIP:
          PrimitiveCount = NumVertices - 2;
          break;

        case D3DPT_TRIANGLEFAN:
          PrimitiveCount = NumVertices - 2;
          break;

        default: return DDERR_INVALIDPARAMS;
    }

    /* Get the FVF of the vertex buffer, and its stride */
    hr = IWineD3DVertexBuffer_GetDesc(vb->wineD3DVertexBuffer,
                                      &Desc);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DVertexBuffer::GetDesc failed with hr = %08x\n", This, hr);
        return hr;
    }
    stride = get_flexible_vertex_size(Desc.FVF);

    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             vb->wineD3DVertexDeclaration);
    if(FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        return hr;
    }

    /* Set the vertex stream source */
    hr = IWineD3DDevice_SetStreamSource(This->wineD3DDevice,
                                        0 /* StreamNumber */,
                                        vb->wineD3DVertexBuffer,
                                        0 /* StartVertex - we pass this to DrawPrimitive */,
                                        stride);
    if(hr != D3D_OK)
    {
        ERR("(%p) IDirect3DDevice::SetStreamSource failed with hr = %08x\n", This, hr);
        return hr;
    }

    /* Now draw the primitives */
    return IWineD3DDevice_DrawPrimitive(This->wineD3DDevice,
                                        PrimitiveType,
                                        StartVertex,
                                        PrimitiveCount);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveVB(IDirect3DDevice3 *iface,
                                            D3DPRIMITIVETYPE PrimitiveType,
                                            IDirect3DVertexBuffer *D3DVertexBuf,
                                            DWORD StartVertex,
                                            DWORD NumVertices,
                                            DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DVertexBufferImpl *vb = ICOM_OBJECT(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, D3DVertexBuf);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p,%08x,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This,  PrimitiveType, vb, StartVertex, NumVertices, Flags);
    return IDirect3DDevice7_DrawPrimitiveVB(ICOM_INTERFACE(This, IDirect3DDevice7),
                                            PrimitiveType,
                                            ICOM_INTERFACE(vb, IDirect3DVertexBuffer7),
                                            StartVertex,
                                            NumVertices,
                                            Flags);
}


/*****************************************************************************
 * IDirect3DDevice7::DrawIndexedPrimitiveVB
 *
 * Draws primitives from a vertex buffer to the screen
 *
 * Params:
 *  PrimitiveType: Type of primitive to be rendered.
 *  D3DVertexBuf: Source Vertex Buffer
 *  StartVertex: Index of the first vertex from the buffer to be rendered
 *  NumVertices: Number of vertices to be rendered
 *  Indices: Array of DWORDs used to index into the Vertices
 *  IndexCount: Number of indices in Indices
 *  Flags: Can be D3DDP_WAIT to wait until rendering has finished
 *
 * Return values
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB(IDirect3DDevice7 *iface,
                                             D3DPRIMITIVETYPE PrimitiveType,
                                             IDirect3DVertexBuffer7 *D3DVertexBuf,
                                             DWORD StartVertex,
                                             DWORD NumVertices,
                                             WORD *Indices,
                                             DWORD IndexCount,
                                             DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirect3DVertexBufferImpl *vb = ICOM_OBJECT(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer7, D3DVertexBuf);
    DWORD stride;
    UINT PrimitiveCount;
    WORD *LockedIndices;
    HRESULT hr;
    WINED3DVERTEXBUFFER_DESC Desc;

    TRACE("(%p)->(%08x,%p,%d,%d,%p,%d,%08x)\n", This, PrimitiveType, vb, StartVertex, NumVertices, Indices, IndexCount, Flags);

    /* Steps:
     * 1) Calculate some things: Vertex count -> Primitive count, stride, ...
     * 2) Upload the Indices to the index buffer
     * 3) Set the index source
     * 4) Set the Vertex Buffer as the Stream source
     * 5) Call IWineD3DDevice::DrawIndexedPrimitive
     */

    /* Get the primitive count */
    switch(PrimitiveType)
    {
        case D3DPT_POINTLIST: 
          PrimitiveCount = IndexCount;
          break;

        case D3DPT_LINELIST: 
          PrimitiveCount = IndexCount / 2;
          break;

        case D3DPT_LINESTRIP:
          PrimitiveCount = IndexCount - 1;
          break;

        case D3DPT_TRIANGLELIST:
          PrimitiveCount = IndexCount / 3;
          break;

        case D3DPT_TRIANGLESTRIP:
          PrimitiveCount = IndexCount - 2;
          break;

        case D3DPT_TRIANGLEFAN:
          PrimitiveCount = IndexCount - 2;
          break;

        default: return DDERR_INVALIDPARAMS;
    }

    /* Get the FVF of the vertex buffer, and its stride */
    hr = IWineD3DVertexBuffer_GetDesc(vb->wineD3DVertexBuffer,
                                      &Desc);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DVertexBuffer::GetDesc failed with hr = %08x\n", This, hr);
        return hr;
    }
    stride = get_flexible_vertex_size(Desc.FVF);
    TRACE("Vertex buffer FVF = %08x, stride=%d\n", Desc.FVF, stride);

    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             vb->wineD3DVertexDeclaration);
    if(FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        return hr;
    }

    /* copy the index stream into the index buffer.
     * A new IWineD3DDevice method could be created
     * which takes an user pointer containing the indices
     * or a SetData-Method for the index buffer, which
     * overrides the index buffer data with our pointer.
     */
    hr = IWineD3DIndexBuffer_Lock(This->indexbuffer,
                                  0 /* OffSetToLock */,
                                  IndexCount * sizeof(WORD),
                                  (BYTE **) &LockedIndices,
                                  0 /* Flags */);
    assert(IndexCount < 0x100000);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DIndexBuffer::Lock failed with hr = %08x\n", This, hr);
        return hr;
    }
    memcpy(LockedIndices, Indices, IndexCount * sizeof(WORD));
    hr = IWineD3DIndexBuffer_Unlock(This->indexbuffer);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DIndexBuffer::Unlock failed with hr = %08x\n", This, hr);
        return hr;
    }

    /* Set the index stream */
    hr = IWineD3DDevice_SetIndices(This->wineD3DDevice,
                                   This->indexbuffer,
                                   StartVertex);

    /* Set the vertex stream source */
    hr = IWineD3DDevice_SetStreamSource(This->wineD3DDevice,
                                        0 /* StreamNumber */,
                                        vb->wineD3DVertexBuffer,
                                        0 /* offset, we pass this to DrawIndexedPrimitive */,
                                        stride);
    if(hr != D3D_OK)
    {
        ERR("(%p) IDirect3DDevice::SetStreamSource failed with hr = %08x\n", This, hr);
        return hr;
    }


    hr = IWineD3DDevice_DrawIndexedPrimitive(This->wineD3DDevice,
                                             PrimitiveType,
                                             0 /* minIndex */,
                                             NumVertices,
                                             0 /* StartIndex */,
                                             PrimitiveCount);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB(IDirect3DDevice3 *iface,
                                                   D3DPRIMITIVETYPE PrimitiveType,
                                                   IDirect3DVertexBuffer *D3DVertexBuf,
                                                   WORD *Indices,
                                                   DWORD IndexCount,
                                                   DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirect3DVertexBufferImpl *VB = ICOM_OBJECT(IDirect3DVertexBufferImpl, IDirect3DVertexBuffer, D3DVertexBuf);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VB, Indices, IndexCount, Flags);

    return IDirect3DDevice7_DrawIndexedPrimitiveVB(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                   PrimitiveType,
                                                   ICOM_INTERFACE(VB, IDirect3DVertexBuffer7),
                                                   0,
                                                   IndexCount,
                                                   Indices,
                                                   IndexCount,
                                                   Flags);
}

/*****************************************************************************
 * IDirect3DDevice7::ComputeSphereVisibility
 *
 * Calculates the visibility of spheres in the current viewport. The spheres
 * are passed in the Centers and Radii arrays, the results are passed back
 * in the ReturnValues array. Return values are either completely visible,
 * partially visible or completely invisible.
 * The return value consist of a combination of D3DCLIP_* flags, or it's
 * 0 if the sphere is completely visible(according to the SDK, not checked)
 *
 * Sounds like an overdose of math ;)
 *
 * Version 3 and 7
 *
 * Params:
 *  Centers: Array containing the sphere centers
 *  Radii: Array containing the sphere radii
 *  NumSpheres: The number of centers and radii in the arrays
 *  Flags: Some flags
 *  ReturnValues: Array to write the results to
 *
 * Returns:
 *  D3D_OK because it's a stub
 *  (DDERR_INVALIDPARAMS if Centers, Radii or ReturnValues are NULL)
 *  (D3DERR_INVALIDMATRIX if the combined world, view and proj matrix
 *  is singular)
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_ComputeSphereVisibility(IDirect3DDevice7 *iface,
                                              D3DVECTOR *Centers,
                                              D3DVALUE *Radii,
                                              DWORD NumSpheres,
                                              DWORD Flags,
                                              DWORD *ReturnValues)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    FIXME("(%p)->(%p,%p,%08x,%08x,%p): stub!\n", This, Centers, Radii, NumSpheres, Flags, ReturnValues);

    /* the DirectX 7 sdk says that the visibility is computed by
     * back-transforming the viewing frustum to model space
     * using the inverse of the combined world, view and projection
     * matrix. If the matrix can't be reversed, D3DERR_INVALIDMATRIX
     * is returned.
     *
     * Basic implementation idea:
     * 1) Check if the center is in the viewing frustum
     * 2) Cut the sphere with the planes of the viewing
     *    frustum
     *
     * ->Center inside the frustum, no intersections:
     *    Fully visible
     * ->Center outside the frustum, no intersections:
     *    Not visible
     * ->Some intersections: Partially visible
     *
     * Implement this call in WineD3D. Either implement the
     * matrix and vector stuff in WineD3D, or use some external
     * math library.
     */

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ComputeSphereVisibility(IDirect3DDevice3 *iface,
                                                    D3DVECTOR *Centers,
                                                    D3DVALUE *Radii,
                                                    DWORD NumSpheres,
                                                    DWORD Flags,
                                                    DWORD *ReturnValues)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p,%08x,%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, Centers, Radii, NumSpheres, Flags, ReturnValues);
    return IDirect3DDevice7_ComputeSphereVisibility(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                    Centers,
                                                    Radii,
                                                    NumSpheres,
                                                    Flags,
                                                    ReturnValues);
}

/*****************************************************************************
 * IDirect3DDevice7::GetTexture
 *
 * Returns the texture interface handle assigned to a texture stage.
 * The returned texture is AddRefed. This is taken from old ddraw,
 * not checked in Windows.
 *
 * Version 3 and 7
 *
 * Params:
 *  Stage: Texture stage to read the texture from
 *  Texture: Address to store the interface pointer at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Texture is NULL
 *  For details, see IWineD3DDevice::GetTexture
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTexture(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 **Texture)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IWineD3DBaseTexture *Surf;
    HRESULT hr;
    TRACE("(%p)->(%d,%p): Relay\n", This, Stage, Texture);

    if(!Texture)
    {
        TRACE("Texture == NULL, failing with DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    hr = IWineD3DDevice_GetTexture(This->wineD3DDevice, Stage, (IWineD3DBaseTexture **) &Surf);
    if( (hr != D3D_OK) || (!Surf) ) 
    {
        *Texture = NULL;
        return hr;
    }

    /* GetParent AddRef()s, which is perfectly OK.
     * We have passed the IDirectDrawSurface7 interface to WineD3D, so that's OK too.
     */
    return IWineD3DBaseTexture_GetParent(Surf,
                                         (IUnknown **) Texture);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTexture(IDirect3DDevice3 *iface,
                                       DWORD Stage,
                                       IDirect3DTexture2 **Texture2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    HRESULT ret;
    IDirectDrawSurface7 *ret_val;

    TRACE_(ddraw_thunk)("(%p)->(%d,%p) thunking to IDirect3DDevice7 interface.\n", This, Stage, Texture2);
    ret = IDirect3DDevice7_GetTexture(ICOM_INTERFACE(This, IDirect3DDevice7),
                                      Stage,
                                      &ret_val);

    *Texture2 = COM_INTERFACE_CAST(IDirectDrawSurfaceImpl, IDirectDrawSurface7, IDirect3DTexture2, ret_val);

    TRACE_(ddraw_thunk)(" returning interface %p.\n", *Texture2);

    return ret;
}

/*****************************************************************************
 * IDirect3DDevice7::SetTexture
 *
 * Assigns a texture to a texture stage. Is the texture AddRef-ed?
 *
 * Version 3 and 7
 *
 * Params:
 *  Stage: The stage to assign the texture to
 *  Texture: Interface pointer to the texture surface
 *
 * Returns
 * D3D_OK on success
 * For details, see IWineD3DDevice::SetTexture
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTexture(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 *Texture)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirectDrawSurfaceImpl *surf = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, Texture);
    TRACE("(%p)->(%08x,%p): Relay!\n", This, Stage, surf);

    /* Texture may be NULL here */
    return IWineD3DDevice_SetTexture(This->wineD3DDevice,
                                     Stage,
                                     surf ? (IWineD3DBaseTexture * ) surf->wineD3DTexture : NULL);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTexture(IDirect3DDevice3 *iface,
                                       DWORD Stage,
                                       IDirect3DTexture2 *Texture2)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    IDirectDrawSurfaceImpl *tex = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirect3DTexture2, Texture2);
    TRACE_(ddraw_thunk)("(%p)->(%d,%p) thunking to IDirect3DDevice7 interface.\n", This, Stage, tex);
    return IDirect3DDevice7_SetTexture(ICOM_INTERFACE(This, IDirect3DDevice7),
                                       Stage,
                                       ICOM_INTERFACE(tex, IDirectDrawSurface7));
}

/*****************************************************************************
 * IDirect3DDevice7::GetTextureStageState
 *
 * Retrieves a state from a texture stage.
 *
 * Version 3 and 7
 *
 * Params:
 *  Stage: The stage to retrieve the state from
 *  TexStageStateType: The state type to retrieve
 *  State: Address to store the state's value at
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if State is NULL
 *  For details, see IWineD3DDevice::GetTextureStageState
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTextureStageState(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD *State)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%08x,%08x,%p): Relay!\n", This, Stage, TexStageStateType, State);

    if(!State)
        return DDERR_INVALIDPARAMS;

    switch(TexStageStateType)
    {
        /* Mipfilter is a sampler state with different values */
        case D3DTSS_MIPFILTER:
        {
            HRESULT hr;
            WINED3DTEXTUREFILTERTYPE value;

            hr = IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                Stage,
                                                WINED3DSAMP_MIPFILTER,
                                                &value);
            switch(value)
            {
                case WINED3DTEXF_NONE: *State = D3DTFP_NONE; break;
                case WINED3DTEXF_POINT: *State = D3DTFP_POINT; break;
                case WINED3DTEXF_LINEAR: *State = D3DTFP_LINEAR; break;
                default:
                    ERR("Unexpected mipfilter value %d\n", value);
                    *State = D3DTFP_NONE;
            }
            return hr;
        }

        /* Minfilter is a sampler state too, equal values */
        case D3DTSS_MINFILTER:
            return IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_MINFILTER,
                                                  State);
        /* Same for MAGFILTER */
        case D3DTSS_MAGFILTER:
            return IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_MAGFILTER,
                                                  State);

        case D3DTSS_ADDRESS:
        case D3DTSS_ADDRESSU:
            return IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_ADDRESSU,
                                                  State);
        case D3DTSS_ADDRESSV:
            return IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_ADDRESSV,
                                                  State);
        default:
            return IWineD3DDevice_GetTextureStageState(This->wineD3DDevice,
                                                       Stage,
                                                       TexStageStateType,
                                                       State);
    }
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTextureStageState(IDirect3DDevice3 *iface,
                                                 DWORD Stage,
                                                 D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                                 DWORD *State)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, Stage, TexStageStateType, State);
    return IDirect3DDevice7_GetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                 Stage,
                                                 TexStageStateType,
                                                 State);
}

/*****************************************************************************
 * IDirect3DDevice7::SetTextureStageState
 *
 * Sets a texture stage state. Some stage types need to be handled specially,
 * because they do not exist in WineD3D and were moved to another place
 *
 * Version 3 and 7
 *
 * Params:
 *  Stage: The stage to modify
 *  TexStageStateType: The state to change
 *  State: The new value for the state
 *
 * Returns:
 *  D3D_OK on success
 *  For details, see IWineD3DDevice::SetTextureStageState
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTextureStageState(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD State)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%08x,%08x,%08x): Relay!\n", This, Stage, TexStageStateType, State);
    switch(TexStageStateType)
    {
        /* Mipfilter is a sampler state with different values */
        case D3DTSS_MIPFILTER:
        {
            WINED3DTEXTUREFILTERTYPE value;
            switch(State)
            {
                case D3DTFP_NONE: value = WINED3DTEXF_NONE; break;
                case D3DTFP_POINT: value = WINED3DTEXF_POINT; break;
                case 0: /* Unchecked */
                case D3DTFP_LINEAR: value = WINED3DTEXF_LINEAR; break;
                default:
                    ERR("Unexpected mipfilter value %d\n", State);
                    value = WINED3DTEXF_NONE;
            }
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_MIPFILTER,
                                                  value);
        }

        /* Minfilter is a sampler state too, equal values */
        case D3DTSS_MINFILTER:
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_MINFILTER,
                                                  State);
        /* Same for MAGFILTER */
        case D3DTSS_MAGFILTER:
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_MAGFILTER,
                                                  State);

        case D3DTSS_ADDRESS:
                   IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_ADDRESSV,
                                                  State);
            /* Drop through */
        case D3DTSS_ADDRESSU:
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_ADDRESSU,
                                                  State);
        case D3DTSS_ADDRESSV:
            return IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  Stage,
                                                  WINED3DSAMP_ADDRESSV,
                                                  State);
        default:

            return IWineD3DDevice_SetTextureStageState(This->wineD3DDevice,
                                                       Stage,
                                                       TexStageStateType,
                                                       State);
    }
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTextureStageState(IDirect3DDevice3 *iface,
                                                 DWORD Stage,
                                                 D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                                 DWORD State)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, Stage, TexStageStateType, State);
    return IDirect3DDevice7_SetTextureStageState(ICOM_INTERFACE(This, IDirect3DDevice7),
                                                 Stage,
                                                 TexStageStateType,
                                                 State);
}

/*****************************************************************************
 * IDirect3DDevice7::ValidateDevice
 *
 * SDK: "Reports the device's ability to render the currently set
 * texture-blending operations in a single pass". Whatever that means
 * exactly...
 *
 * Version 3 and 7
 *
 * Params:
 *  NumPasses: Address to write the number of necessary passes for the
 *             desired effect to.
 *
 * Returns:
 *  D3D_OK on success
 *  See IWineD3DDevice::ValidateDevice for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_ValidateDevice(IDirect3DDevice7 *iface,
                                     DWORD *NumPasses)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%p): Relay\n", This, NumPasses);

    return IWineD3DDevice_ValidateDevice(This->wineD3DDevice, NumPasses);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ValidateDevice(IDirect3DDevice3 *iface,
                                           DWORD *Passes)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice3, iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Passes);
    return IDirect3DDevice7_ValidateDevice(ICOM_INTERFACE(This, IDirect3DDevice7),
                                           Passes);
}

/*****************************************************************************
 * IDirect3DDevice7::Clear
 *
 * Fills the render target, the z buffer and the stencil buffer with a
 * clear color / value
 *
 * Version 7 only
 *
 * Params:
 *  Count: Number of rectangles in Rects must be 0 if Rects is NULL
 *  Rects: Rectangles to clear. If NULL, the whole surface is cleared
 *  Flags: Some flags, as usual
 *  Color: Clear color for the render target
 *  Z: Clear value for the Z buffer
 *  Stencil: Clear value to store in each stencil buffer entry
 *
 * Returns:
 *  D3D_OK on success
 *  For details, see IWineD3DDevice::Clear
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_Clear(IDirect3DDevice7 *iface,
                            DWORD Count,
                            D3DRECT *Rects,
                            DWORD Flags,
                            D3DCOLOR Color,
                            D3DVALUE Z,
                            DWORD Stencil)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%08x,%p,%08x,%08x,%f,%08x): Relay\n", This, Count, Rects, Flags, (DWORD) Color, Z, Stencil);

    /* Note; D3DRECT is compatible with WINED3DRECT */
    return IWineD3DDevice_Clear(This->wineD3DDevice, Count, (WINED3DRECT*) Rects, Flags, Color, Z, Stencil);
}

/*****************************************************************************
 * IDirect3DDevice7::SetViewport
 *
 * Sets the current viewport.
 *
 * Version 7 only, but IDirect3DViewport uses this call for older
 * versions
 *
 * Params:
 *  Data: The new viewport to set
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Data is NULL
 *  For more details, see IWineDDDevice::SetViewport
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetViewport(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%p) Relay!\n", This, Data);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DVIEWPORT7 is compatible with WINED3DVIEWPORT */
    return IWineD3DDevice_SetViewport(This->wineD3DDevice,
                                      (WINED3DVIEWPORT*) Data);
}

/*****************************************************************************
 * IDirect3DDevice::GetViewport
 *
 * Returns the current viewport
 *
 * Version 7
 *
 * Params:
 *  Data: D3D7Viewport structure to write the viewport information to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Data is NULL
 *  For more details, see IWineD3DDevice::GetViewport
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetViewport(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%p) Relay!\n", This, Data);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DVIEWPORT7 is compatible with WINED3DVIEWPORT */
    hr = IWineD3DDevice_GetViewport(This->wineD3DDevice,
                                    (WINED3DVIEWPORT*) Data);

    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::SetMaterial
 *
 * Sets the Material
 *
 * Version 7
 *
 * Params:
 *  Mat: The material to set
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Mat is NULL.
 *  For more details, see IWineD3DDevice::SetMaterial
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetMaterial(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%p): Relay!\n", This, Mat);

    /* Note: D3DMATERIAL7 is compatible with WINED3DMATERIAL */
    hr = IWineD3DDevice_SetMaterial(This->wineD3DDevice,
                                    (WINED3DMATERIAL*) Mat);

    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::GetMaterial
 *
 * Returns the current material
 *
 * Version 7
 *
 * Params:
 *  Mat: D3DMATERIAL7 structure to write the material parameters to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Mat is NULL
 *  For more details, see IWineD3DDevice::GetMaterial
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetMaterial(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%p): Relay!\n", This, Mat);

    /* Note: D3DMATERIAL7 is compatible with WINED3DMATERIAL */ 
    hr = IWineD3DDevice_GetMaterial(This->wineD3DDevice,
                                    (WINED3DMATERIAL*) Mat);

    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::SetLight
 *
 * Assigns a light to a light index, but doesn't activate it yet.
 *
 * Version 7, IDirect3DLight uses this method for older versions
 *
 * Params:
 *  LightIndex: The index of the new light
 *  Light: A D3DLIGHT7 structure describing the light
 *
 * Returns:
 *  D3D_OK on success
 *  For more details, see IWineD3DDevice::SetLight
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetLight(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay!\n", This, LightIndex, Light);

    /* Note: D3DLIGHT7 is compatible with WINED3DLIGHT */
    hr = IWineD3DDevice_SetLight(This->wineD3DDevice,
                                 LightIndex,
                                 (WINED3DLIGHT*) Light);

    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::GetLight
 *
 * Returns the light assigned to a light index
 *
 * Params:
 *  Light: Structure to write the light information to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Light is NULL
 *  For details, see IWineD3DDevice::GetLight
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetLight(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT rc;
    TRACE("(%p)->(%08x,%p): Relay!\n", This, LightIndex, Light);

    /* Note: D3DLIGHT7 is compatible with WINED3DLIGHT */
    rc =  IWineD3DDevice_GetLight(This->wineD3DDevice,
                                  LightIndex,
                                  (WINED3DLIGHT*) Light);

    /* Translate the result. WineD3D returns other values than D3D7 */
    return hr_ddraw_from_wined3d(rc);
}

/*****************************************************************************
 * IDirect3DDevice7::BeginStateBlock
 *
 * Begins recording to a stateblock
 *
 * Version 7
 *
 * Returns:
 *  D3D_OK on success
 *  For details see IWineD3DDevice::BeginStateBlock
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_BeginStateBlock(IDirect3DDevice7 *iface)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(): Relay!\n", This);

    hr = IWineD3DDevice_BeginStateBlock(This->wineD3DDevice);
    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::EndStateBlock
 *
 * Stops recording to a state block and returns the created stateblock
 * handle.
 *
 * Version 7
 *
 * Params:
 *  BlockHandle: Address to store the stateblock's handle to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if BlockHandle is NULL
 *  See IWineD3DDevice::EndStateBlock for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_EndStateBlock(IDirect3DDevice7 *iface,
                                    DWORD *BlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%p): Relay!\n", This, BlockHandle);

    if(!BlockHandle)
    {
        WARN("BlockHandle == NULL, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    *BlockHandle = IDirect3DDeviceImpl_CreateHandle(This);
    if(!*BlockHandle)
    {
        ERR("Cannot get a handle number for the stateblock\n");
        return DDERR_OUTOFMEMORY;
    }
    This->Handles[*BlockHandle - 1].type = DDrawHandle_StateBlock;
    hr = IWineD3DDevice_EndStateBlock(This->wineD3DDevice,
                                      (IWineD3DStateBlock **) &This->Handles[*BlockHandle - 1].ptr);
    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::PreLoad
 *
 * Allows the app to signal that a texture will be used soon, to allow
 * the Direct3DDevice to load it to the video card in the meantime.
 *
 * Version 7
 *
 * Params:
 *  Texture: The texture to preload
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Texture is NULL
 *  See IWineD3DSurface::PreLoad for details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_PreLoad(IDirect3DDevice7 *iface,
                              IDirectDrawSurface7 *Texture)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirectDrawSurfaceImpl *surf = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, Texture);

    TRACE("(%p)->(%p): Relay!\n", This, surf);

    if(!Texture)
        return DDERR_INVALIDPARAMS;

    IWineD3DSurface_PreLoad(surf->WineD3DSurface);
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::ApplyStateBlock
 *
 * Activates the state stored in a state block handle.
 *
 * Params:
 *  BlockHandle: The stateblock handle to activate
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_INVALIDSTATEBLOCK if BlockHandle is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_ApplyStateBlock(IDirect3DDevice7 *iface,
                                      DWORD BlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%08x): Relay!\n", This, BlockHandle);

    if(!BlockHandle || BlockHandle > This->numHandles)
    {
        WARN("Out of range handle %d, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        return D3DERR_INVALIDSTATEBLOCK;
    }
    if(This->Handles[BlockHandle - 1].type != DDrawHandle_StateBlock)
    {
        WARN("Handle %d is not a stateblock, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        return D3DERR_INVALIDSTATEBLOCK;
    }

    hr = IWineD3DStateBlock_Apply((IWineD3DStateBlock *) This->Handles[BlockHandle - 1].ptr);
    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::CaptureStateBlock
 *
 * Updates a stateblock's values to the values currently set for the device
 *
 * Version 7
 *
 * Params:
 *  BlockHandle: Stateblock to update
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_INVALIDSTATEBLOCK if BlockHandle is NULL
 *  See IWineD3DDevice::CaptureStateBlock for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_CaptureStateBlock(IDirect3DDevice7 *iface,
                                        DWORD BlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%08x): Relay!\n", This, BlockHandle);

    if(BlockHandle == 0 || BlockHandle > This->numHandles)
    {
        WARN("Out of range handle %d, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        return D3DERR_INVALIDSTATEBLOCK;
    }
    if(This->Handles[BlockHandle - 1].type != DDrawHandle_StateBlock)
    {
        WARN("Handle %d is not a stateblock, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        return D3DERR_INVALIDSTATEBLOCK;
    }

    hr = IWineD3DStateBlock_Capture((IWineD3DStateBlock *) This->Handles[BlockHandle - 1].ptr);
    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::DeleteStateBlock
 *
 * Deletes a stateblock handle. This means releasing the WineD3DStateBlock
 *
 * Version 7
 *
 * Params:
 *  BlockHandle: Stateblock handle to delete
 *
 * Returns:
 *  D3D_OK on success
 *  D3DERR_INVALIDSTATEBLOCK if BlockHandle is 0
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_DeleteStateBlock(IDirect3DDevice7 *iface,
                                       DWORD BlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    ULONG ref;
    TRACE("(%p)->(%08x): Relay!\n", This, BlockHandle);

    if(BlockHandle == 0 || BlockHandle > This->numHandles)
    {
        WARN("Out of range handle %d, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        return D3DERR_INVALIDSTATEBLOCK;
    }
    if(This->Handles[BlockHandle - 1].type != DDrawHandle_StateBlock)
    {
        WARN("Handle %d is not a stateblock, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        return D3DERR_INVALIDSTATEBLOCK;
    }

    ref = IWineD3DStateBlock_Release((IWineD3DStateBlock *) This->Handles[BlockHandle - 1].ptr);
    if(ref)
    {
        ERR("Something is still holding the stateblock %p(Handle %d). Ref = %d\n", This->Handles[BlockHandle - 1].ptr, BlockHandle, ref);
    }
    This->Handles[BlockHandle - 1].ptr = NULL;
    This->Handles[BlockHandle - 1].type = DDrawHandle_Unknown;

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::CreateStateBlock
 *
 * Creates a new state block handle.
 *
 * Version 7
 *
 * Params:
 *  Type: The state block type
 *  BlockHandle: Address to write the created handle to
 *
 * Returns:
 *   D3D_OK on success
 *   DDERR_INVALIDPARAMS if BlockHandle is NULL
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_CreateStateBlock(IDirect3DDevice7 *iface,
                                       D3DSTATEBLOCKTYPE Type,
                                       DWORD *BlockHandle)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%08x,%p)!\n", This, Type, BlockHandle);

    if(!BlockHandle)
    {
        WARN("BlockHandle == NULL, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    *BlockHandle = IDirect3DDeviceImpl_CreateHandle(This);
    if(!*BlockHandle)
    {
        ERR("Cannot get a handle number for the stateblock\n");
        return DDERR_OUTOFMEMORY;
    }
    This->Handles[*BlockHandle - 1].type = DDrawHandle_StateBlock;

    /* The D3DSTATEBLOCKTYPE enum is fine here */
    hr = IWineD3DDevice_CreateStateBlock(This->wineD3DDevice,
                                         Type,
                                         (IWineD3DStateBlock **) &This->Handles[*BlockHandle - 1].ptr,
                                         NULL /* Parent, hope that works */);
    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::Load
 *
 * Loads a rectangular area from the source into the destination texture.
 * It can also copy the source to the faces of a cubic environment map
 *
 * Version 7
 *
 * Params:
 *  DestTex: Destination texture
 *  DestPoint: Point in the destination where the source image should be
 *             written to
 *  SrcTex: Source texture
 *  SrcRect: Source rectangle
 *  Flags: Some flags
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if DestTex or SrcTex are NULL
 *  See IDirect3DTexture2::Load for details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_Load(IDirect3DDevice7 *iface,
                           IDirectDrawSurface7 *DestTex,
                           POINT *DestPoint,
                           IDirectDrawSurface7 *SrcTex,
                           RECT *SrcRect,
                           DWORD Flags)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    IDirectDrawSurfaceImpl *dest = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, DestTex);
    IDirectDrawSurfaceImpl *src = ICOM_OBJECT(IDirectDrawSurfaceImpl, IDirectDrawSurface7, SrcTex);
    FIXME("(%p)->(%p,%p,%p,%p,%08x): Partially Implemented!\n", This, dest, DestPoint, src, SrcRect, Flags);

    if( (!src) || (!dest) )
        return DDERR_INVALIDPARAMS;

    IDirect3DTexture2_Load(ICOM_INTERFACE(dest, IDirect3DTexture2),
                           ICOM_INTERFACE(src, IDirect3DTexture2));
    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::LightEnable
 *
 * Enables or disables a light
 *
 * Version 7, IDirect3DLight uses this method too.
 *
 * Params:
 *  LightIndex: The index of the light to enable / disable
 *  Enable: Enable or disable the light
 *
 * Returns:
 *  D3D_OK on success
 *  For more details, see IWineD3DDevice::SetLightEnable
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_LightEnable(IDirect3DDevice7 *iface,
                                  DWORD LightIndex,
                                  BOOL Enable)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%08x,%d): Relay!\n", This, LightIndex, Enable);

    hr = IWineD3DDevice_SetLightEnable(This->wineD3DDevice, LightIndex, Enable);
    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::GetLightEnable
 *
 * Retrieves if the light with the given index is enabled or not
 *
 * Version 7
 *
 * Params:
 *  LightIndex: Index of desired light
 *  Enable: Pointer to a BOOL which contains the result
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if Enable is NULL
 *  See IWineD3DDevice::GetLightEnable for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetLightEnable(IDirect3DDevice7 *iface,
                                     DWORD LightIndex,
                                     BOOL* Enable)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay\n", This, LightIndex, Enable);

    if(!Enable)
        return DDERR_INVALIDPARAMS;

    hr = IWineD3DDevice_GetLightEnable(This->wineD3DDevice, LightIndex, Enable);
    return hr_ddraw_from_wined3d(hr);
}

/*****************************************************************************
 * IDirect3DDevice7::SetClipPlane
 *
 * Sets custom clipping plane
 *
 * Version 7
 *
 * Params:
 *  Index: The index of the clipping plane
 *  PlaneEquation: An equation defining the clipping plane
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if PlaneEquation is NULL
 *  See IWineD3DDevice::SetClipPlane for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetClipPlane(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%08x,%p): Relay!\n", This, Index, PlaneEquation);

    if(!PlaneEquation)
        return DDERR_INVALIDPARAMS;

    return IWineD3DDevice_SetClipPlane(This->wineD3DDevice, Index, PlaneEquation);
}

/*****************************************************************************
 * IDirect3DDevice7::GetClipPlane
 *
 * Returns the clipping plane with a specific index
 *
 * Params:
 *  Index: The index of the desired plane
 *  PlaneEquation: Address to store the plane equation to
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if PlaneEquation is NULL
 *  See IWineD3DDevice::GetClipPlane for more details
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetClipPlane(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%d,%p): Relay!\n", This, Index, PlaneEquation);

    if(!PlaneEquation)
        return DDERR_INVALIDPARAMS;

    return IWineD3DDevice_GetClipPlane(This->wineD3DDevice, Index, PlaneEquation);
}

/*****************************************************************************
 * IDirect3DDevice7::GetInfo
 *
 * Retrieves some information about the device. The DirectX sdk says that
 * this version returns S_FALSE for all retail builds of DirectX, that's what
 * this implementation does.
 *
 * Params:
 *  DevInfoID: Information type requested
 *  DevInfoStruct: Pointer to a structure to store the info to
 *  Size: Size of the structure
 *
 * Returns:
 *  S_FALSE, because it's a non-debug driver
 *
 *****************************************************************************/
static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetInfo(IDirect3DDevice7 *iface,
                              DWORD DevInfoID,
                              void *DevInfoStruct,
                              DWORD Size)
{
    ICOM_THIS_FROM(IDirect3DDeviceImpl, IDirect3DDevice7, iface);
    TRACE("(%p)->(%08x,%p,%08x)\n", This, DevInfoID, DevInfoStruct, Size);

    if (TRACE_ON(d3d7))
    {
        TRACE(" info requested : ");
        switch (DevInfoID)
        {
            case D3DDEVINFOID_TEXTUREMANAGER: TRACE("D3DDEVINFOID_TEXTUREMANAGER\n"); break;
            case D3DDEVINFOID_D3DTEXTUREMANAGER: TRACE("D3DDEVINFOID_D3DTEXTUREMANAGER\n"); break;
            case D3DDEVINFOID_TEXTURING: TRACE("D3DDEVINFOID_TEXTURING\n"); break;
            default: ERR(" invalid flag !!!\n"); return DDERR_INVALIDPARAMS;
        }
    }

    return S_FALSE; /* According to MSDN, this is valid for a non-debug driver */
}

const IDirect3DDevice7Vtbl IDirect3DDevice7_Vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DDeviceImpl_7_QueryInterface,
    IDirect3DDeviceImpl_7_AddRef,
    IDirect3DDeviceImpl_7_Release,
    /*** IDirect3DDevice7 ***/
    IDirect3DDeviceImpl_7_GetCaps,
    IDirect3DDeviceImpl_7_EnumTextureFormats,
    IDirect3DDeviceImpl_7_BeginScene,
    IDirect3DDeviceImpl_7_EndScene,
    IDirect3DDeviceImpl_7_GetDirect3D,
    IDirect3DDeviceImpl_7_SetRenderTarget,
    IDirect3DDeviceImpl_7_GetRenderTarget,
    IDirect3DDeviceImpl_7_Clear,
    IDirect3DDeviceImpl_7_SetTransform,
    IDirect3DDeviceImpl_7_GetTransform,
    IDirect3DDeviceImpl_7_SetViewport,
    IDirect3DDeviceImpl_7_MultiplyTransform,
    IDirect3DDeviceImpl_7_GetViewport,
    IDirect3DDeviceImpl_7_SetMaterial,
    IDirect3DDeviceImpl_7_GetMaterial,
    IDirect3DDeviceImpl_7_SetLight,
    IDirect3DDeviceImpl_7_GetLight,
    IDirect3DDeviceImpl_7_SetRenderState,
    IDirect3DDeviceImpl_7_GetRenderState,
    IDirect3DDeviceImpl_7_BeginStateBlock,
    IDirect3DDeviceImpl_7_EndStateBlock,
    IDirect3DDeviceImpl_7_PreLoad,
    IDirect3DDeviceImpl_7_DrawPrimitive,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitive,
    IDirect3DDeviceImpl_7_SetClipStatus,
    IDirect3DDeviceImpl_7_GetClipStatus,
    IDirect3DDeviceImpl_7_DrawPrimitiveStrided,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided,
    IDirect3DDeviceImpl_7_DrawPrimitiveVB,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB,
    IDirect3DDeviceImpl_7_ComputeSphereVisibility,
    IDirect3DDeviceImpl_7_GetTexture,
    IDirect3DDeviceImpl_7_SetTexture,
    IDirect3DDeviceImpl_7_GetTextureStageState,
    IDirect3DDeviceImpl_7_SetTextureStageState,
    IDirect3DDeviceImpl_7_ValidateDevice,
    IDirect3DDeviceImpl_7_ApplyStateBlock,
    IDirect3DDeviceImpl_7_CaptureStateBlock,
    IDirect3DDeviceImpl_7_DeleteStateBlock,
    IDirect3DDeviceImpl_7_CreateStateBlock,
    IDirect3DDeviceImpl_7_Load,
    IDirect3DDeviceImpl_7_LightEnable,
    IDirect3DDeviceImpl_7_GetLightEnable,
    IDirect3DDeviceImpl_7_SetClipPlane,
    IDirect3DDeviceImpl_7_GetClipPlane,
    IDirect3DDeviceImpl_7_GetInfo
};

const IDirect3DDevice3Vtbl IDirect3DDevice3_Vtbl =
{
    /*** IUnknown Methods ***/
    Thunk_IDirect3DDeviceImpl_3_QueryInterface,
    Thunk_IDirect3DDeviceImpl_3_AddRef,
    Thunk_IDirect3DDeviceImpl_3_Release,
    /*** IDirect3DDevice3 ***/
    IDirect3DDeviceImpl_3_GetCaps,
    IDirect3DDeviceImpl_3_GetStats,
    IDirect3DDeviceImpl_3_AddViewport,
    IDirect3DDeviceImpl_3_DeleteViewport,
    IDirect3DDeviceImpl_3_NextViewport,
    Thunk_IDirect3DDeviceImpl_3_EnumTextureFormats,
    Thunk_IDirect3DDeviceImpl_3_BeginScene,
    Thunk_IDirect3DDeviceImpl_3_EndScene,
    Thunk_IDirect3DDeviceImpl_3_GetDirect3D,
    IDirect3DDeviceImpl_3_SetCurrentViewport,
    IDirect3DDeviceImpl_3_GetCurrentViewport,
    Thunk_IDirect3DDeviceImpl_3_SetRenderTarget,
    Thunk_IDirect3DDeviceImpl_3_GetRenderTarget,
    IDirect3DDeviceImpl_3_Begin,
    IDirect3DDeviceImpl_3_BeginIndexed,
    IDirect3DDeviceImpl_3_Vertex,
    IDirect3DDeviceImpl_3_Index,
    IDirect3DDeviceImpl_3_End,
    Thunk_IDirect3DDeviceImpl_3_GetRenderState,
    Thunk_IDirect3DDeviceImpl_3_SetRenderState,
    IDirect3DDeviceImpl_3_GetLightState,
    IDirect3DDeviceImpl_3_SetLightState,
    Thunk_IDirect3DDeviceImpl_3_SetTransform,
    Thunk_IDirect3DDeviceImpl_3_GetTransform,
    Thunk_IDirect3DDeviceImpl_3_MultiplyTransform,
    Thunk_IDirect3DDeviceImpl_3_DrawPrimitive,
    Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitive,
    Thunk_IDirect3DDeviceImpl_3_SetClipStatus,
    Thunk_IDirect3DDeviceImpl_3_GetClipStatus,
    Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveStrided,
    Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided,
    Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveVB,
    Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB,
    Thunk_IDirect3DDeviceImpl_3_ComputeSphereVisibility,
    Thunk_IDirect3DDeviceImpl_3_GetTexture,
    Thunk_IDirect3DDeviceImpl_3_SetTexture,
    Thunk_IDirect3DDeviceImpl_3_GetTextureStageState,
    Thunk_IDirect3DDeviceImpl_3_SetTextureStageState,
    Thunk_IDirect3DDeviceImpl_3_ValidateDevice
};

const IDirect3DDevice2Vtbl IDirect3DDevice2_Vtbl =
{
    /*** IUnknown Methods ***/
    Thunk_IDirect3DDeviceImpl_2_QueryInterface,
    Thunk_IDirect3DDeviceImpl_2_AddRef,
    Thunk_IDirect3DDeviceImpl_2_Release,
    /*** IDirect3DDevice2 ***/
    Thunk_IDirect3DDeviceImpl_2_GetCaps,
    IDirect3DDeviceImpl_2_SwapTextureHandles,
    Thunk_IDirect3DDeviceImpl_2_GetStats,
    Thunk_IDirect3DDeviceImpl_2_AddViewport,
    Thunk_IDirect3DDeviceImpl_2_DeleteViewport,
    Thunk_IDirect3DDeviceImpl_2_NextViewport,
    IDirect3DDeviceImpl_2_EnumTextureFormats,
    Thunk_IDirect3DDeviceImpl_2_BeginScene,
    Thunk_IDirect3DDeviceImpl_2_EndScene,
    Thunk_IDirect3DDeviceImpl_2_GetDirect3D,
    Thunk_IDirect3DDeviceImpl_2_SetCurrentViewport,
    Thunk_IDirect3DDeviceImpl_2_GetCurrentViewport,
    Thunk_IDirect3DDeviceImpl_2_SetRenderTarget,
    Thunk_IDirect3DDeviceImpl_2_GetRenderTarget,
    Thunk_IDirect3DDeviceImpl_2_Begin,
    Thunk_IDirect3DDeviceImpl_2_BeginIndexed,
    Thunk_IDirect3DDeviceImpl_2_Vertex,
    Thunk_IDirect3DDeviceImpl_2_Index,
    Thunk_IDirect3DDeviceImpl_2_End,
    Thunk_IDirect3DDeviceImpl_2_GetRenderState,
    Thunk_IDirect3DDeviceImpl_2_SetRenderState,
    Thunk_IDirect3DDeviceImpl_2_GetLightState,
    Thunk_IDirect3DDeviceImpl_2_SetLightState,
    Thunk_IDirect3DDeviceImpl_2_SetTransform,
    Thunk_IDirect3DDeviceImpl_2_GetTransform,
    Thunk_IDirect3DDeviceImpl_2_MultiplyTransform,
    Thunk_IDirect3DDeviceImpl_2_DrawPrimitive,
    Thunk_IDirect3DDeviceImpl_2_DrawIndexedPrimitive,
    Thunk_IDirect3DDeviceImpl_2_SetClipStatus,
    Thunk_IDirect3DDeviceImpl_2_GetClipStatus
};

const IDirect3DDeviceVtbl IDirect3DDevice1_Vtbl =
{
    /*** IUnknown Methods ***/
    Thunk_IDirect3DDeviceImpl_1_QueryInterface,
    Thunk_IDirect3DDeviceImpl_1_AddRef,
    Thunk_IDirect3DDeviceImpl_1_Release,
    /*** IDirect3DDevice1 ***/
    IDirect3DDeviceImpl_1_Initialize,
    Thunk_IDirect3DDeviceImpl_1_GetCaps,
    Thunk_IDirect3DDeviceImpl_1_SwapTextureHandles,
    IDirect3DDeviceImpl_1_CreateExecuteBuffer,
    Thunk_IDirect3DDeviceImpl_1_GetStats,
    IDirect3DDeviceImpl_1_Execute,
    Thunk_IDirect3DDeviceImpl_1_AddViewport,
    Thunk_IDirect3DDeviceImpl_1_DeleteViewport,
    Thunk_IDirect3DDeviceImpl_1_NextViewport,
    IDirect3DDeviceImpl_1_Pick,
    IDirect3DDeviceImpl_1_GetPickRecords,
    Thunk_IDirect3DDeviceImpl_1_EnumTextureFormats,
    IDirect3DDeviceImpl_1_CreateMatrix,
    IDirect3DDeviceImpl_1_SetMatrix,
    IDirect3DDeviceImpl_1_GetMatrix,
    IDirect3DDeviceImpl_1_DeleteMatrix,
    Thunk_IDirect3DDeviceImpl_1_EndScene,
    Thunk_IDirect3DDeviceImpl_1_BeginScene,
    Thunk_IDirect3DDeviceImpl_1_GetDirect3D
};

/*****************************************************************************
 * IDirect3DDeviceImpl_CreateHandle
 *
 * Not called from the VTable
 *
 * Some older interface versions operate with handles, which are basically
 * DWORDs which identify an interface, for example
 * IDirect3DDevice::SetRenderState with DIRECT3DRENDERSTATE_TEXTUREHANDLE
 *
 * Those handle could be just casts to the interface pointers or vice versa,
 * but that is not 64 bit safe and would mean blindly derefering a DWORD
 * passed by the app. Instead there is a dynamic array in the device which
 * keeps a DWORD to pointer information and a type for the handle.
 *
 * Basically this array only grows, when a handle is freed its pointer is
 * just set to NULL. There will be much more reads from the array than
 * insertion operations, so a dynamic array is fine.
 *
 * Params:
 *  This: D3DDevice implementation for which this handle should be created
 *
 * Returns:
 *  A free handle on success
 *  0 on failure
 *
 *****************************************************************************/
DWORD
IDirect3DDeviceImpl_CreateHandle(IDirect3DDeviceImpl *This)
{
    DWORD i;
    struct HandleEntry *oldHandles = This->Handles;

    TRACE("(%p)\n", This);

    for(i = 0; i < This->numHandles; i++)
    {
        if(This->Handles[i].ptr == NULL &&
           This->Handles[i].type == DDrawHandle_Unknown)
        {
            TRACE("Reusing freed handle %d\n", i + 1);
            return i + 1;
        }
    }

    TRACE("Growing the handle array\n");

    This->numHandles++;
    This->Handles = HeapAlloc(GetProcessHeap(), 0, sizeof(struct HandleEntry) * This->numHandles);
    if(!This->Handles)
    {
        ERR("Out of memory\n");
        This->Handles = oldHandles;
        This->numHandles--;
        return 0;
    }
    if(oldHandles)
    {
        memcpy(This->Handles, oldHandles, (This->numHandles - 1) * sizeof(struct HandleEntry));
        HeapFree(GetProcessHeap(), 0, oldHandles);
    }

    TRACE("Returning %d\n", This->numHandles);
    return This->numHandles;
}
