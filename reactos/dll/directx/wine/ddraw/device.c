/*
 * Copyright (c) 1998-2004 Lionel Ulmer
 * Copyright (c) 2002-2005 Christian Costa
 * Copyright (c) 2006 Stefan Dösinger
 * Copyright (c) 2008 Alexander Dorofeyev
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wingdi.h"
#include "wine/exception.h"

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

static inline void set_fpu_control_word(WORD fpucw)
{
#if defined(__i386__) && defined(__GNUC__)
    __asm__ volatile ("fldcw %0" : : "m" (fpucw));
#elif defined(__i386__) && defined(_MSC_VER)
    __asm fldcw fpucw;
#endif
}

static inline WORD d3d_fpu_setup(void)
{
    WORD oldcw;

#if defined(__i386__) && defined(__GNUC__)
    __asm__ volatile ("fnstcw %0" : "=m" (oldcw));
#elif defined(__i386__) && defined(_MSC_VER)
    __asm fnstcw oldcw;
#else
    static BOOL warned = FALSE;
    if(!warned)
    {
        FIXME("FPUPRESERVE not implemented for this platform / compiler\n");
        warned = TRUE;
    }
    return 0;
#endif

    set_fpu_control_word(0x37f);

    return oldcw;
}

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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    TRACE("(%p)->(%s,%p)\n", This, debugstr_guid(refiid), obj);

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;

    if(!refiid)
        return DDERR_INVALIDPARAMS;

    if ( IsEqualGUID( &IID_IUnknown, refiid ) )
    {
        *obj = iface;
    }

    /* Check DirectDraw Interfacs */
    else if( IsEqualGUID( &IID_IDirectDraw7, refiid ) )
    {
        *obj = This->ddraw;
        TRACE("(%p) Returning IDirectDraw7 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw4, refiid ) )
    {
        *obj = &This->ddraw->IDirectDraw4_vtbl;
        TRACE("(%p) Returning IDirectDraw4 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) )
    {
        *obj = &This->ddraw->IDirectDraw2_vtbl;
        TRACE("(%p) Returning IDirectDraw2 interface at %p\n", This, *obj);
    }
    else if( IsEqualGUID( &IID_IDirectDraw, refiid ) )
    {
        *obj = &This->ddraw->IDirectDraw_vtbl;
        TRACE("(%p) Returning IDirectDraw interface at %p\n", This, *obj);
    }

    /* Direct3D */
    else if ( IsEqualGUID( &IID_IDirect3D  , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D_vtbl;
        TRACE("(%p) Returning IDirect3D interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D2 , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D2_vtbl;
        TRACE("(%p) Returning IDirect3D2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D3 , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D3_vtbl;
        TRACE("(%p) Returning IDirect3D3 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D7 , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D7_vtbl;
        TRACE("(%p) Returning IDirect3D7 interface at %p\n", This, *obj);
    }

    /* Direct3DDevice */
    else if ( IsEqualGUID( &IID_IDirect3DDevice  , refiid ) )
    {
        *obj = &This->IDirect3DDevice_vtbl;
        TRACE("(%p) Returning IDirect3DDevice interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice2  , refiid ) ) {
        *obj = &This->IDirect3DDevice2_vtbl;
        TRACE("(%p) Returning IDirect3DDevice2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice3  , refiid ) ) {
        *obj = &This->IDirect3DDevice3_vtbl;
        TRACE("(%p) Returning IDirect3DDevice3 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice7  , refiid ) ) {
        *obj = This;
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", This, debugstr_guid(riid), obj);
    return IDirect3DDevice7_QueryInterface((IDirect3DDevice7 *)This, riid, obj);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_QueryInterface(IDirect3DDevice2 *iface,
                                           REFIID riid,
                                           void **obj)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", This, debugstr_guid(riid), obj);
    return IDirect3DDevice7_QueryInterface((IDirect3DDevice7 *)This, riid, obj);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_QueryInterface(IDirect3DDevice *iface,
                                           REFIID riid,
                                           void **obp)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE_(ddraw_thunk)("(%p)->(%s,%p) thunking to IDirect3DDevice7 interface.\n", This, debugstr_guid(riid), obp);
    return IDirect3DDevice7_QueryInterface((IDirect3DDevice7 *)This, riid, obp);
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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : incrementing from %u.\n", This, ref -1);

    return ref;
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_AddRef(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_AddRef((IDirect3DDevice7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_AddRef(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_AddRef((IDirect3DDevice7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_AddRef(IDirect3DDevice *iface)
{
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", iface);
    return IDirect3DDevice7_AddRef((IDirect3DDevice7 *)device_from_device1(iface));
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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
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

        EnterCriticalSection(&ddraw_cs);
        /* Free the index buffer. */
        IWineD3DDevice_SetIndices(This->wineD3DDevice, NULL, WINED3DFMT_UNKNOWN);
        IWineD3DBuffer_GetParent(This->indexbuffer,
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
            WINED3DVIEWPORT vp;

            vp.X = 0;
            vp.Y = 0;
            vp.Width = This->ddraw->d3d_target->surface_desc.dwWidth;
            vp.Height = This->ddraw->d3d_target->surface_desc.dwHeight;
            vp.MinZ = 0.0;
            vp.MaxZ = 1.0;
            IWineD3DDevice_SetViewport(This->wineD3DDevice,
                                       &vp);

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
            ERR(" (%p) The wineD3D device %p was destroyed unexpectedly. Prepare for trouble\n", This, This->wineD3DDevice);
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
                        IDirectDrawSurfaceImpl *surf = This->Handles[i].ptr;
                        FIXME("Texture Handle %d not unset properly\n", i + 1);
                        surf->Handle = 0;
                    }
                    break;

                    case DDrawHandle_Material:
                    {
                        IDirect3DMaterialImpl *mat = This->Handles[i].ptr;
                        FIXME("Material handle %d not unset properly\n", i + 1);
                        mat->Handle = 0;
                    }
                    break;

                    case DDrawHandle_Matrix:
                    {
                        /* No fixme here because this might happen because of sloppy apps */
                        WARN("Leftover matrix handle %d, deleting\n", i + 1);
                        IDirect3DDevice_DeleteMatrix((IDirect3DDevice *)&This->IDirect3DDevice_vtbl, i + 1);
                    }
                    break;

                    case DDrawHandle_StateBlock:
                    {
                        /* No fixme here because this might happen because of sloppy apps */
                        WARN("Leftover stateblock handle %d, deleting\n", i + 1);
                        IDirect3DDevice7_DeleteStateBlock((IDirect3DDevice7 *)This, i + 1);
                    }
                    break;

                    default:
                        FIXME("Unknown handle %d not unset properly\n", i + 1);
                }
            }
        }

        HeapFree(GetProcessHeap(), 0, This->Handles);

        TRACE("Releasing target %p %p\n", This->target, This->ddraw->d3d_target);
        /* Release the render target and the WineD3D render target
         * (See IDirect3D7::CreateDevice for more comments on this)
         */
        IDirectDrawSurface7_Release((IDirectDrawSurface7 *)This->target);
        IDirectDrawSurface7_Release((IDirectDrawSurface7 *)This->ddraw->d3d_target);
        TRACE("Target release done\n");

        This->ddraw->d3ddevice = NULL;

        /* Now free the structure */
        HeapFree(GetProcessHeap(), 0, This);
        LeaveCriticalSection(&ddraw_cs);
    }

    TRACE("Done\n");
    return ref;
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_3_Release(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_Release((IDirect3DDevice7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_2_Release(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_Release((IDirect3DDevice7 *)This);
}

static ULONG WINAPI
Thunk_IDirect3DDeviceImpl_1_Release(IDirect3DDevice *iface)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_Release((IDirect3DDevice7 *)This);
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
    IDirect3DDeviceImpl *This = device_from_device1(iface);

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
static HRESULT
IDirect3DDeviceImpl_7_GetCaps(IDirect3DDevice7 *iface,
                              D3DDEVICEDESC7 *Desc)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    D3DDEVICEDESC OldDesc;
    TRACE("(%p)->(%p)\n", This, Desc);

    /* Call the same function used by IDirect3D, this saves code */
    return IDirect3DImpl_GetCaps(This->ddraw->wineD3D, &OldDesc, Desc);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetCaps_FPUSetup(IDirect3DDevice7 *iface,
                              D3DDEVICEDESC7 *Desc)
{
    return IDirect3DDeviceImpl_7_GetCaps(iface, Desc);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetCaps_FPUPreserve(IDirect3DDevice7 *iface,
                              D3DDEVICEDESC7 *Desc)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetCaps(iface, Desc);
    set_fpu_control_word(old_fpucw);

    return hr;
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
 *  HelDesc: Structure to fill with the hardware emulation caps
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
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
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice3 interface.\n", This, D3DHWDevDesc, D3DHELDevDesc);
    return IDirect3DDevice3_GetCaps((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, D3DHWDevDesc, D3DHELDevDesc);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetCaps(IDirect3DDevice *iface,
                                    D3DDEVICEDESC *D3DHWDevDesc,
                                    D3DDEVICEDESC *D3DHELDevDesc)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice3 interface.\n", This, D3DHWDevDesc, D3DHELDevDesc);
    return IDirect3DDevice3_GetCaps((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, D3DHWDevDesc, D3DHELDevDesc);
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
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    DWORD swap;
    IDirectDrawSurfaceImpl *surf1 = surface_from_texture2(Tex1);
    IDirectDrawSurfaceImpl *surf2 = surface_from_texture2(Tex2);
    TRACE("(%p)->(%p,%p)\n", This, surf1, surf2);

    EnterCriticalSection(&ddraw_cs);
    This->Handles[surf1->Handle - 1].ptr = surf2;
    This->Handles[surf2->Handle - 1].ptr = surf1;

    swap = surf2->Handle;
    surf2->Handle = surf1->Handle;
    surf1->Handle = swap;
    LeaveCriticalSection(&ddraw_cs);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_SwapTextureHandles(IDirect3DDevice *iface,
                                               IDirect3DTexture *D3DTex1,
                                               IDirect3DTexture *D3DTex2)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    IDirectDrawSurfaceImpl *surf1 = surface_from_texture1(D3DTex1);
    IDirectDrawSurfaceImpl *surf2 = surface_from_texture1(D3DTex2);
    IDirect3DTexture2 *t1 = surf1 ? (IDirect3DTexture2 *)&surf1->IDirect3DTexture2_vtbl : NULL;
    IDirect3DTexture2 *t2 = surf2 ? (IDirect3DTexture2 *)&surf2->IDirect3DTexture2_vtbl : NULL;
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice2 interface.\n", This, surf1, surf2);
    return IDirect3DDevice2_SwapTextureHandles((IDirect3DDevice2 *)&This->IDirect3DDevice2_vtbl, t1, t2);
}

/*****************************************************************************
 * IDirect3DDevice3::GetStats
 *
 * This method seems to retrieve some stats from the device.
 * The MSDN documentation doesn't exist any more, but the D3DSTATS
 * structure suggests that the amount of drawn primitives and processed
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
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
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, Stats);
    return IDirect3DDevice3_GetStats((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, Stats);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetStats(IDirect3DDevice *iface,
                                     D3DSTATS *Stats)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, Stats);
    return IDirect3DDevice3_GetStats((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, Stats);
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
    IDirect3DDeviceImpl *This = device_from_device1(iface);
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

    object->lpVtbl = &IDirect3DExecuteBuffer_Vtbl;
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

    *ExecuteBuffer = (IDirect3DExecuteBuffer *)object;

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
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    IDirect3DExecuteBufferImpl *Direct3DExecuteBufferImpl = (IDirect3DExecuteBufferImpl *)ExecuteBuffer;
    IDirect3DViewportImpl *Direct3DViewportImpl = (IDirect3DViewportImpl *)Viewport;

    TRACE("(%p)->(%p,%p,%08x)\n", This, Direct3DExecuteBufferImpl, Direct3DViewportImpl, Flags);

    if(!Direct3DExecuteBufferImpl)
        return DDERR_INVALIDPARAMS;

    /* Execute... */
    EnterCriticalSection(&ddraw_cs);
    IDirect3DExecuteBufferImpl_Execute(Direct3DExecuteBufferImpl, This, Direct3DViewportImpl);
    LeaveCriticalSection(&ddraw_cs);

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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Viewport;

    TRACE("(%p)->(%p)\n", This, vp);

    /* Sanity check */
    if(!vp)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    vp->next = This->viewport_list;
    This->viewport_list = vp;
    vp->active_device = This; /* Viewport must be usable for Clear() after AddViewport,
                                    so set active_device here. */
    LeaveCriticalSection(&ddraw_cs);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_AddViewport(IDirect3DDevice2 *iface,
                                        IDirect3DViewport2 *Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Direct3DViewport2;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_AddViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, (IDirect3DViewport3 *)vp);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_AddViewport(IDirect3DDevice *iface,
                                        IDirect3DViewport *Direct3DViewport)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Direct3DViewport;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_AddViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, (IDirect3DViewport3 *)vp);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *) Viewport;
    IDirect3DViewportImpl *cur_viewport, *prev_viewport = NULL;

    TRACE("(%p)->(%p)\n", This, vp);

    EnterCriticalSection(&ddraw_cs);
    cur_viewport = This->viewport_list;
    while (cur_viewport != NULL)
    {
        if (cur_viewport == vp)
        {
            if (prev_viewport == NULL) This->viewport_list = cur_viewport->next;
            else prev_viewport->next = cur_viewport->next;
            /* TODO : add desactivate of the viewport and all associated lights... */
            LeaveCriticalSection(&ddraw_cs);
            return D3D_OK;
        }
        prev_viewport = cur_viewport;
        cur_viewport = cur_viewport->next;
    }

    LeaveCriticalSection(&ddraw_cs);
    return DDERR_INVALIDPARAMS;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DeleteViewport(IDirect3DDevice2 *iface,
                                           IDirect3DViewport2 *Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Direct3DViewport2;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_DeleteViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, (IDirect3DViewport3 *)vp);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_DeleteViewport(IDirect3DDevice *iface,
                                           IDirect3DViewport *Direct3DViewport)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Direct3DViewport;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_DeleteViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, (IDirect3DViewport3 *)vp);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Viewport3;
    IDirect3DViewportImpl *res = NULL;

    TRACE("(%p)->(%p,%p,%08x)\n", This, vp, lplpDirect3DViewport3, Flags);

    if(!vp)
    {
        *lplpDirect3DViewport3 = NULL;
        return DDERR_INVALIDPARAMS;
    }


    EnterCriticalSection(&ddraw_cs);
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
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
    }

    *lplpDirect3DViewport3 = (IDirect3DViewport3 *)res;
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_NextViewport(IDirect3DDevice2 *iface,
                                         IDirect3DViewport2 *Viewport2,
                                         IDirect3DViewport2 **lplpDirect3DViewport2,
                                         DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Viewport2;
    IDirect3DViewport3 *res;
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p,%p,%08x) thunking to IDirect3DDevice3 interface.\n", This, vp, lplpDirect3DViewport2, Flags);
    hr = IDirect3DDevice3_NextViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl,
            (IDirect3DViewport3 *)vp, &res, Flags);
    *lplpDirect3DViewport2 = (IDirect3DViewport2 *)res;
    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_NextViewport(IDirect3DDevice *iface,
                                         IDirect3DViewport *Viewport,
                                         IDirect3DViewport **lplpDirect3DViewport,
                                         DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Viewport;
    IDirect3DViewport3 *res;
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p,%p,%08x) thunking to IDirect3DDevice3 interface.\n", This, vp, lplpDirect3DViewport, Flags);
    hr = IDirect3DDevice3_NextViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl,
            (IDirect3DViewport3 *)vp, &res, Flags);
    *lplpDirect3DViewport = (IDirect3DViewport *)res;
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
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    IDirect3DExecuteBufferImpl *execbuf = (IDirect3DExecuteBufferImpl *)ExecuteBuffer;
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Viewport;
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
 *  D3DPickRec: Address to store the resulting D3DPICKRECORD array.
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
    IDirect3DDeviceImpl *This = device_from_device1(iface);
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
static HRESULT
IDirect3DDeviceImpl_7_EnumTextureFormats(IDirect3DDevice7 *iface,
                                         LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                         void *Arg)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    WINED3DDISPLAYMODE mode;
    unsigned int i;

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

    WINED3DFORMAT BumpFormatList[] = {
        WINED3DFMT_R8G8_SNORM,
        WINED3DFMT_L6V5U5,
        WINED3DFMT_X8L8V8U8,
        WINED3DFMT_R8G8B8A8_SNORM,
        WINED3DFMT_R16G16_SNORM,
        WINED3DFMT_W11V11U10,
        WINED3DFMT_A2W10V10U10
    };

    TRACE("(%p)->(%p,%p): Relay\n", This, Callback, Arg);

    if(!Callback)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);

    memset(&mode, 0, sizeof(mode));
    hr = IWineD3DDevice_GetDisplayMode(This->ddraw->wineD3DDevice,
                                       0,
                                       &mode);
    if(FAILED(hr)) {
        LeaveCriticalSection(&ddraw_cs);
        WARN("Cannot get the current adapter format\n");
        return hr;
    }

    for(i = 0; i < sizeof(FormatList) / sizeof(WINED3DFORMAT); i++)
    {
        hr = IWineD3D_CheckDeviceFormat(This->ddraw->wineD3D,
                                        WINED3DADAPTER_DEFAULT,
                                        WINED3DDEVTYPE_HAL,
                                        mode.Format,
                                        0 /* Usage */,
                                        WINED3DRTYPE_TEXTURE,
                                        FormatList[i],
                                        SURFACE_OPENGL);
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
                LeaveCriticalSection(&ddraw_cs);
                return D3D_OK;
            }
        }
    }

    for(i = 0; i < sizeof(BumpFormatList) / sizeof(WINED3DFORMAT); i++)
    {
        hr = IWineD3D_CheckDeviceFormat(This->ddraw->wineD3D,
                                        WINED3DADAPTER_DEFAULT,
                                        WINED3DDEVTYPE_HAL,
                                        mode.Format,
                                        WINED3DUSAGE_QUERY_LEGACYBUMPMAP,
                                        WINED3DRTYPE_TEXTURE,
                                        BumpFormatList[i],
                                        SURFACE_OPENGL);
        if(hr == D3D_OK)
        {
            DDPIXELFORMAT pformat;

            memset(&pformat, 0, sizeof(pformat));
            pformat.dwSize = sizeof(pformat);
            PixelFormat_WineD3DtoDD(&pformat, BumpFormatList[i]);

            TRACE("Enumerating WineD3DFormat %d\n", BumpFormatList[i]);
            hr = Callback(&pformat, Arg);
            if(hr != DDENUMRET_OK)
            {
                TRACE("Format enumeration cancelled by application\n");
                LeaveCriticalSection(&ddraw_cs);
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration\n");
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_EnumTextureFormats_FPUSetup(IDirect3DDevice7 *iface,
                                         LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                         void *Arg)
{
    return IDirect3DDeviceImpl_7_EnumTextureFormats(iface, Callback, Arg);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_EnumTextureFormats_FPUPreserve(IDirect3DDevice7 *iface,
                                         LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                         void *Arg)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_EnumTextureFormats(iface, Callback, Arg);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EnumTextureFormats(IDirect3DDevice3 *iface,
                                               LPD3DENUMPIXELFORMATSCALLBACK Callback,
                                               void *Arg)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice7 interface.\n", This, Callback, Arg);
    return IDirect3DDevice7_EnumTextureFormats((IDirect3DDevice7 *)This, Callback, Arg);
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
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    HRESULT hr;
    unsigned int i;
    WINED3DDISPLAYMODE mode;

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

    EnterCriticalSection(&ddraw_cs);

    memset(&mode, 0, sizeof(mode));
    hr = IWineD3DDevice_GetDisplayMode(This->ddraw->wineD3DDevice,
                                       0,
                                       &mode);
    if(FAILED(hr)) {
        LeaveCriticalSection(&ddraw_cs);
        WARN("Cannot get the current adapter format\n");
        return hr;
    }

    for(i = 0; i < sizeof(FormatList) / sizeof(WINED3DFORMAT); i++)
    {
        hr = IWineD3D_CheckDeviceFormat(This->ddraw->wineD3D,
                                        0 /* Adapter */,
                                        WINED3DDEVTYPE_HAL,
                                        mode.Format,
                                        0 /* Usage */,
                                        WINED3DRTYPE_TEXTURE,
                                        FormatList[i],
                                        SURFACE_OPENGL);
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
                LeaveCriticalSection(&ddraw_cs);
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration\n");
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EnumTextureFormats(IDirect3DDevice *iface,
                                               LPD3DENUMTEXTUREFORMATSCALLBACK Callback,
                                               void *Arg)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p) thunking to IDirect3DDevice2 interface.\n", This, Callback, Arg);
    return IDirect3DDevice2_EnumTextureFormats((IDirect3DDevice2 *)&This->IDirect3DDevice2_vtbl, Callback, Arg);
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
    IDirect3DDeviceImpl *This = device_from_device1(iface);
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

    EnterCriticalSection(&ddraw_cs);
    *D3DMatHandle = IDirect3DDeviceImpl_CreateHandle(This);
    if(!(*D3DMatHandle))
    {
        ERR("Failed to create a matrix handle\n");
        HeapFree(GetProcessHeap(), 0, Matrix);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_OUTOFMEMORY;
    }
    This->Handles[*D3DMatHandle - 1].ptr = Matrix;
    This->Handles[*D3DMatHandle - 1].type = DDrawHandle_Matrix;
    TRACE(" returning matrix handle %d\n", *D3DMatHandle);

    LeaveCriticalSection(&ddraw_cs);
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
                                D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE("(%p)->(%08x,%p)\n", This, D3DMatHandle, D3DMatrix);

    if( (!D3DMatHandle) || (!D3DMatrix) )
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    if(D3DMatHandle > This->numHandles)
    {
        ERR("Handle %d out of range\n", D3DMatHandle);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }
    else if(This->Handles[D3DMatHandle - 1].type != DDrawHandle_Matrix)
    {
        ERR("Handle %d is not a matrix handle\n", D3DMatHandle);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    if (TRACE_ON(d3d7))
        dump_D3DMATRIX(D3DMatrix);

    *((D3DMATRIX *) This->Handles[D3DMatHandle - 1].ptr) = *D3DMatrix;

    if(This->world == D3DMatHandle)
    {
        IWineD3DDevice_SetTransform(This->wineD3DDevice,
                                    WINED3DTS_WORLDMATRIX(0),
                                    (WINED3DMATRIX *) D3DMatrix);
    }
    if(This->view == D3DMatHandle)
    {
        IWineD3DDevice_SetTransform(This->wineD3DDevice,
                                    WINED3DTS_VIEW,
                                    (WINED3DMATRIX *) D3DMatrix);
    }
    if(This->proj == D3DMatHandle)
    {
        IWineD3DDevice_SetTransform(This->wineD3DDevice,
                                    WINED3DTS_PROJECTION,
                                    (WINED3DMATRIX *) D3DMatrix);
    }

    LeaveCriticalSection(&ddraw_cs);
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
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE("(%p)->(%08x,%p)\n", This, D3DMatHandle, D3DMatrix);

    if(!D3DMatrix)
        return DDERR_INVALIDPARAMS;
    if(!D3DMatHandle)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    if(D3DMatHandle > This->numHandles)
    {
        ERR("Handle %d out of range\n", D3DMatHandle);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }
    else if(This->Handles[D3DMatHandle - 1].type != DDrawHandle_Matrix)
    {
        ERR("Handle %d is not a matrix handle\n", D3DMatHandle);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* The handle is simply a pointer to a D3DMATRIX structure */
    *D3DMatrix = *((D3DMATRIX *) This->Handles[D3DMatHandle - 1].ptr);

    LeaveCriticalSection(&ddraw_cs);
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
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE("(%p)->(%08x)\n", This, D3DMatHandle);

    if(!D3DMatHandle)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    if(D3DMatHandle > This->numHandles)
    {
        ERR("Handle %d out of range\n", D3DMatHandle);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }
    else if(This->Handles[D3DMatHandle - 1].type != DDrawHandle_Matrix)
    {
        ERR("Handle %d is not a matrix handle\n", D3DMatHandle);
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    HeapFree(GetProcessHeap(), 0, This->Handles[D3DMatHandle - 1].ptr);
    This->Handles[D3DMatHandle - 1].ptr = NULL;
    This->Handles[D3DMatHandle - 1].type = DDrawHandle_Unknown;

    LeaveCriticalSection(&ddraw_cs);
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
static HRESULT
IDirect3DDeviceImpl_7_BeginScene(IDirect3DDevice7 *iface)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p): Relay\n", This);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_BeginScene(This->wineD3DDevice);
    LeaveCriticalSection(&ddraw_cs);
    if(hr == WINED3D_OK) return D3D_OK;
    else return D3DERR_SCENE_IN_SCENE; /* TODO: Other possible causes of failure */
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_BeginScene_FPUSetup(IDirect3DDevice7 *iface)
{
    return IDirect3DDeviceImpl_7_BeginScene(iface);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_BeginScene_FPUPreserve(IDirect3DDevice7 *iface)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_BeginScene(iface);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_BeginScene(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_BeginScene((IDirect3DDevice7 *)This);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_BeginScene(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_BeginScene((IDirect3DDevice7 *)This);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_BeginScene(IDirect3DDevice *iface)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_BeginScene((IDirect3DDevice7 *)This);
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
static HRESULT
IDirect3DDeviceImpl_7_EndScene(IDirect3DDevice7 *iface)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p): Relay\n", This);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_EndScene(This->wineD3DDevice);
    LeaveCriticalSection(&ddraw_cs);
    if(hr == WINED3D_OK) return D3D_OK;
    else return D3DERR_SCENE_NOT_IN_SCENE;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_EndScene_FPUSetup(IDirect3DDevice7 *iface)
{
    return IDirect3DDeviceImpl_7_EndScene(iface);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_EndScene_FPUPreserve(IDirect3DDevice7 *iface)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_EndScene(iface);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_EndScene(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_EndScene((IDirect3DDevice7 *)This);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_EndScene(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_EndScene((IDirect3DDevice7 *)This);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_EndScene(IDirect3DDevice *iface)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    TRACE_(ddraw_thunk)("(%p)->() thunking to IDirect3DDevice7 interface.\n", This);
    return IDirect3DDevice7_EndScene((IDirect3DDevice7 *)This);
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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    TRACE("(%p)->(%p)\n", This, Direct3D7);

    if(!Direct3D7)
        return DDERR_INVALIDPARAMS;

    *Direct3D7 = (IDirect3D7 *)&This->ddraw->IDirect3D7_vtbl;
    IDirect3D7_AddRef(*Direct3D7);

    TRACE(" returning interface %p\n", *Direct3D7);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetDirect3D(IDirect3DDevice3 *iface,
                                        IDirect3D3 **Direct3D3)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    HRESULT ret;
    IDirect3D7 *ret_ptr;

    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Direct3D3);
    ret = IDirect3DDevice7_GetDirect3D((IDirect3DDevice7 *)This, &ret_ptr);
    if(ret != D3D_OK)
        return ret;
    *Direct3D3 = ret_ptr ? (IDirect3D3 *)&ddraw_from_d3d7(ret_ptr)->IDirect3D3_vtbl : NULL;
    TRACE(" returning interface %p\n", *Direct3D3);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetDirect3D(IDirect3DDevice2 *iface,
                                        IDirect3D2 **Direct3D2)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    HRESULT ret;
    IDirect3D7 *ret_ptr;

    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Direct3D2);
    ret = IDirect3DDevice7_GetDirect3D((IDirect3DDevice7 *)This, &ret_ptr);
    if(ret != D3D_OK)
        return ret;
    *Direct3D2 = ret_ptr ? (IDirect3D2 *)&ddraw_from_d3d7(ret_ptr)->IDirect3D2_vtbl : NULL;
    TRACE(" returning interface %p\n", *Direct3D2);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_1_GetDirect3D(IDirect3DDevice *iface,
                                        IDirect3D **Direct3D)
{
    IDirect3DDeviceImpl *This = device_from_device1(iface);
    HRESULT ret;
    IDirect3D7 *ret_ptr;

    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Direct3D);
    ret = IDirect3DDevice7_GetDirect3D((IDirect3DDevice7 *)This, &ret_ptr);
    if(ret != D3D_OK)
        return ret;
    *Direct3D = ret_ptr ? (IDirect3D *)&ddraw_from_d3d7(ret_ptr)->IDirect3D_vtbl : NULL;
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Direct3DViewport3;
    TRACE("(%p)->(%p)\n", This, Direct3DViewport3);

    EnterCriticalSection(&ddraw_cs);
    /* Do nothing if the specified viewport is the same as the current one */
    if (This->current_viewport == vp )
    {
        LeaveCriticalSection(&ddraw_cs);
        return D3D_OK;
    }

    /* Should check if the viewport was added or not */

    /* Release previous viewport and AddRef the new one */
    if (This->current_viewport)
    {
        TRACE("ViewportImpl is at %p, interface is at %p\n", This->current_viewport,
                (IDirect3DViewport3 *)This->current_viewport);
        IDirect3DViewport3_Release((IDirect3DViewport3 *)This->current_viewport);
    }
    IDirect3DViewport3_AddRef(Direct3DViewport3);

    /* Set this viewport as the current viewport */
    This->current_viewport = vp;

    /* Activate this viewport */
    This->current_viewport->active_device = This;
    This->current_viewport->activate(This->current_viewport, FALSE);

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetCurrentViewport(IDirect3DDevice2 *iface,
                                               IDirect3DViewport2 *Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    IDirect3DViewportImpl *vp = (IDirect3DViewportImpl *)Direct3DViewport2;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, vp);
    return IDirect3DDevice3_SetCurrentViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl,
            (IDirect3DViewport3 *)vp);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE("(%p)->(%p)\n", This, Direct3DViewport3);

    if(!Direct3DViewport3)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    *Direct3DViewport3 = (IDirect3DViewport3 *)This->current_viewport;

    /* AddRef the returned viewport */
    if(*Direct3DViewport3) IDirect3DViewport3_AddRef(*Direct3DViewport3);

    TRACE(" returning interface %p\n", *Direct3DViewport3);

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetCurrentViewport(IDirect3DDevice2 *iface,
                                               IDirect3DViewport2 **Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, Direct3DViewport2);
    hr = IDirect3DDevice3_GetCurrentViewport((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl,
            (IDirect3DViewport3 **)Direct3DViewport2);
    if(hr != D3D_OK) return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_SetRenderTarget(IDirect3DDevice7 *iface,
                                      IDirectDrawSurface7 *NewTarget,
                                      DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    IDirectDrawSurfaceImpl *Target = (IDirectDrawSurfaceImpl *)NewTarget;
    HRESULT hr;
    TRACE("(%p)->(%p,%08x): Relay\n", This, NewTarget, Flags);

    EnterCriticalSection(&ddraw_cs);
    /* Flags: Not used */

    if(This->target == Target)
    {
        TRACE("No-op SetRenderTarget operation, not doing anything\n");
        LeaveCriticalSection(&ddraw_cs);
        return D3D_OK;
    }

    hr = IWineD3DDevice_SetRenderTarget(This->wineD3DDevice,
                                        0,
                                        Target ? Target->WineD3DSurface : NULL);
    if(hr != D3D_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }
    IDirectDrawSurface7_AddRef(NewTarget);
    IDirectDrawSurface7_Release((IDirectDrawSurface7 *)This->target);
    This->target = Target;
    IDirect3DDeviceImpl_UpdateDepthStencil(This);
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetRenderTarget_FPUSetup(IDirect3DDevice7 *iface,
                                      IDirectDrawSurface7 *NewTarget,
                                      DWORD Flags)
{
    return IDirect3DDeviceImpl_7_SetRenderTarget(iface, NewTarget, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetRenderTarget_FPUPreserve(IDirect3DDevice7 *iface,
                                      IDirectDrawSurface7 *NewTarget,
                                      DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetRenderTarget(iface, NewTarget, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetRenderTarget(IDirect3DDevice3 *iface,
                                            IDirectDrawSurface4 *NewRenderTarget,
                                            DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirectDrawSurfaceImpl *Target = (IDirectDrawSurfaceImpl *)NewRenderTarget;
    TRACE_(ddraw_thunk)("(%p)->(%p,%08x) thunking to IDirect3DDevice7 interface.\n", This, Target, Flags);
    return IDirect3DDevice7_SetRenderTarget((IDirect3DDevice7 *)This, (IDirectDrawSurface7 *)Target, Flags);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderTarget(IDirect3DDevice2 *iface,
                                            IDirectDrawSurface *NewRenderTarget,
                                            DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    IDirectDrawSurfaceImpl *Target = (IDirectDrawSurfaceImpl *)NewRenderTarget;
    TRACE_(ddraw_thunk)("(%p)->(%p,%08x) thunking to IDirect3DDevice7 interface.\n", This, Target, Flags);
    return IDirect3DDevice7_SetRenderTarget((IDirect3DDevice7 *)This, (IDirectDrawSurface7 *)Target, Flags);
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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    TRACE("(%p)->(%p): Relay\n", This, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    *RenderTarget = (IDirectDrawSurface7 *)This->target;
    IDirectDrawSurface7_AddRef(*RenderTarget);

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetRenderTarget(IDirect3DDevice3 *iface,
                                            IDirectDrawSurface4 **RenderTarget)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, RenderTarget);
    hr = IDirect3DDevice7_GetRenderTarget((IDirect3DDevice7 *)This, (IDirectDrawSurface7 **)RenderTarget);
    if(hr != D3D_OK) return hr;
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderTarget(IDirect3DDevice2 *iface,
                                            IDirectDrawSurface **RenderTarget)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    HRESULT hr;
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, RenderTarget);
    hr = IDirect3DDevice7_GetRenderTarget((IDirect3DDevice7 *)This, (IDirectDrawSurface7 **)RenderTarget);
    if(hr != D3D_OK) return hr;
    *RenderTarget = *RenderTarget ?
            (IDirectDrawSurface *)&((IDirectDrawSurfaceImpl *)*RenderTarget)->IDirectDrawSurface3_vtbl : NULL;
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE("(%p)->(%d,%d,%08x)\n", This, PrimitiveType, VertexTypeDesc, Flags);

    EnterCriticalSection(&ddraw_cs);
    This->primitive_type = PrimitiveType;
    This->vertex_type = VertexTypeDesc;
    This->render_flags = Flags;
    This->vertex_size = get_flexible_vertex_size(This->vertex_type);
    This->nb_vertices = 0;
    LeaveCriticalSection(&ddraw_cs);

    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Begin(IDirect3DDevice2 *iface,
                                  D3DPRIMITIVETYPE d3dpt,
                                  D3DVERTEXTYPE dwVertexTypeDesc,
                                  DWORD dwFlags)
{
    DWORD FVF;
    IDirect3DDeviceImpl *This = device_from_device2(iface);
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

    return IDirect3DDevice3_Begin((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, d3dpt, FVF, dwFlags);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
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
    IDirect3DDeviceImpl *This = device_from_device2(iface);
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

    return IDirect3DDevice3_BeginIndexed((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl,
            d3dptPrimitiveType, FVF, lpvVertices, dwNumVertices, dwFlags);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE("(%p)->(%p)\n", This, Vertex);

    if(!Vertex)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
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

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Vertex(IDirect3DDevice2 *iface,
                                   void *lpVertexType)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice3 interface.\n", This, lpVertexType);
    return IDirect3DDevice3_Vertex((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, lpVertexType);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    FIXME("(%p)->(%04x): stub!\n", This, VertexIndex);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_Index(IDirect3DDevice2 *iface,
                                  WORD wVertexIndex)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%04x) thunking to IDirect3DDevice3 interface.\n", This, wVertexIndex);
    return IDirect3DDevice3_Index((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, wVertexIndex);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE("(%p)->(%08x)\n", This, Flags);

    return IDirect3DDevice7_DrawPrimitive((IDirect3DDevice7 *)This, This->primitive_type,
            This->vertex_type, This->vertex_buffer, This->nb_vertices, This->render_flags);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_End(IDirect3DDevice2 *iface,
                                DWORD dwFlags)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x) thunking to IDirect3DDevice3 interface.\n", This, dwFlags);
    return IDirect3DDevice3_End((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, dwFlags);
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
static HRESULT
IDirect3DDeviceImpl_7_GetRenderState(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD *Value)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay\n", This, RenderStateType, Value);

    if(!Value)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    switch(RenderStateType)
    {
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
            break;
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
            break;
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            hr = IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_ADDRESSU,
                                                Value);
            break;
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            hr = IWineD3DDevice_GetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_ADDRESSV,
                                                Value);
            break;

        default:
            /* FIXME: Unhandled: D3DRENDERSTATE_STIPPLEPATTERN00 - 31 */
            hr = IWineD3DDevice_GetRenderState(This->wineD3DDevice,
                                               RenderStateType,
                                               Value);
    }
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetRenderState_FPUSetup(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD *Value)
{
    return IDirect3DDeviceImpl_7_GetRenderState(iface, RenderStateType, Value);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetRenderState_FPUPreserve(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD *Value)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetRenderState(iface, RenderStateType, Value);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_3_GetRenderState(IDirect3DDevice3 *iface,
                                     D3DRENDERSTATETYPE dwRenderStateType,
                                     DWORD *lpdwRenderState)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    HRESULT hr;
    TRACE("(%p)->(%08x,%p)\n", This, dwRenderStateType, lpdwRenderState);

    switch(dwRenderStateType)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            /* This state is wrapped to SetTexture in SetRenderState, so
             * it has to be wrapped to GetTexture here
             */
            IWineD3DBaseTexture *tex = NULL;
            *lpdwRenderState = 0;

            EnterCriticalSection(&ddraw_cs);

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
                    IDirectDrawSurfaceImpl *texImpl = (IDirectDrawSurfaceImpl *)parent;
                    *lpdwRenderState = texImpl->Handle;
                    IDirectDrawSurface7_Release(parent);
                }
                IWineD3DBaseTexture_Release(tex);
            }

            LeaveCriticalSection(&ddraw_cs);

            return hr;
        }

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
        {
            /* D3DRENDERSTATE_TEXTUREMAPBLEND is mapped to texture state stages in SetRenderState; reverse
               the mapping to get the value. */
            DWORD colorop, colorarg1, colorarg2;
            DWORD alphaop, alphaarg1, alphaarg2;

            EnterCriticalSection(&ddraw_cs);

            This->legacyTextureBlending = TRUE;

            IWineD3DDevice_GetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, &colorop);
            IWineD3DDevice_GetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, &colorarg1);
            IWineD3DDevice_GetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG2, &colorarg2);
            IWineD3DDevice_GetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, &alphaop);
            IWineD3DDevice_GetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, &alphaarg1);
            IWineD3DDevice_GetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG2, &alphaarg2);

            if (colorop == WINED3DTOP_SELECTARG1 && colorarg1 == WINED3DTA_TEXTURE &&
                alphaop == WINED3DTOP_SELECTARG1 && alphaarg1 == WINED3DTA_TEXTURE)
            {
                *lpdwRenderState = D3DTBLEND_DECAL;
            }
            else if (colorop == WINED3DTOP_SELECTARG1 && colorarg1 == WINED3DTA_TEXTURE &&
                alphaop == WINED3DTOP_MODULATE && alphaarg1 == WINED3DTA_TEXTURE && alphaarg2 == WINED3DTA_CURRENT)
            {
                *lpdwRenderState = D3DTBLEND_DECALALPHA;
            }
            else if (colorop == WINED3DTOP_MODULATE && colorarg1 == WINED3DTA_TEXTURE && colorarg2 == WINED3DTA_CURRENT &&
                alphaop == WINED3DTOP_MODULATE && alphaarg1 == WINED3DTA_TEXTURE && alphaarg2 == WINED3DTA_CURRENT)
            {
                *lpdwRenderState = D3DTBLEND_MODULATEALPHA;
            }
            else
            {
                HRESULT hr;
                BOOL tex_alpha = FALSE;
                IWineD3DBaseTexture *tex = NULL;
                WINED3DSURFACE_DESC desc;
                WINED3DFORMAT fmt;
                DDPIXELFORMAT ddfmt;

                hr = IWineD3DDevice_GetTexture(This->wineD3DDevice,
                                            0,
                                            &tex);

                if(hr == WINED3D_OK && tex)
                {
                    memset(&desc, 0, sizeof(desc));
                    desc.Format = &fmt;
                    hr = IWineD3DTexture_GetLevelDesc((IWineD3DTexture*) tex, 0, &desc);
                    if (SUCCEEDED(hr))
                    {
                        ddfmt.dwSize = sizeof(ddfmt);
                        PixelFormat_WineD3DtoDD(&ddfmt, fmt);
                        if (ddfmt.u5.dwRGBAlphaBitMask) tex_alpha = TRUE;
                    }

                    IWineD3DBaseTexture_Release(tex);
                }

                if (!(colorop == WINED3DTOP_MODULATE && colorarg1 == WINED3DTA_TEXTURE && colorarg2 == WINED3DTA_CURRENT &&
                      alphaop == WINED3DTOP_SELECTARG1 && alphaarg1 == (tex_alpha ? WINED3DTA_TEXTURE : WINED3DTA_CURRENT)))
                {
                    ERR("Unexpected texture stage state setup, returning D3DTBLEND_MODULATE - likely erroneous\n");
                }

                *lpdwRenderState = D3DTBLEND_MODULATE;
            }

            LeaveCriticalSection(&ddraw_cs);

            return D3D_OK;
        }

        default:
            return IDirect3DDevice7_GetRenderState((IDirect3DDevice7 *)This, dwRenderStateType, lpdwRenderState);
    }
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetRenderState(IDirect3DDevice2 *iface,
                                           D3DRENDERSTATETYPE dwRenderStateType,
                                           DWORD *lpdwRenderState)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice3 interface.\n", This, dwRenderStateType, lpdwRenderState);
    return IDirect3DDevice3_GetRenderState((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl,
            dwRenderStateType, lpdwRenderState);
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
static HRESULT
IDirect3DDeviceImpl_7_SetRenderState(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD Value)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%d): Relay\n", This, RenderStateType, Value);

    EnterCriticalSection(&ddraw_cs);
    /* Some render states need special care */
    switch(RenderStateType)
    {
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

            hr = IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_MAGFILTER,
                                                tex_mag);
            break;
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
            hr = IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_MINFILTER,
                                                tex_min);
            break;
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
                   IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                  0, WINED3DSAMP_ADDRESSV,
                                                  Value);
            /* Drop through */
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            hr = IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_ADDRESSU,
                                                Value);
            break;
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            hr = IWineD3DDevice_SetSamplerState(This->wineD3DDevice,
                                                0, WINED3DSAMP_ADDRESSV,
                                                Value);
            break;

        default:

            /* FIXME: Unhandled: D3DRENDERSTATE_STIPPLEPATTERN00 - 31 */

            hr = IWineD3DDevice_SetRenderState(This->wineD3DDevice,
                                               RenderStateType,
                                               Value);
            break;
    }
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetRenderState_FPUSetup(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD Value)
{
    return IDirect3DDeviceImpl_7_SetRenderState(iface, RenderStateType, Value);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetRenderState_FPUPreserve(IDirect3DDevice7 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD Value)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetRenderState(iface, RenderStateType, Value);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_3_SetRenderState(IDirect3DDevice3 *iface,
                                     D3DRENDERSTATETYPE RenderStateType,
                                     DWORD Value)
{
    /* Note about D3DRENDERSTATE_TEXTUREMAPBLEND implementation: most of values
    for this state can be directly mapped to texture stage colorop and alphaop, but
    D3DTBLEND_MODULATE is tricky: it uses alpha from texture when available and alpha
    from diffuse otherwise. So changing the texture must be monitored in SetTexture to modify
    alphaarg when needed.

    Aliens vs Predator 1 depends on accurate D3DTBLEND_MODULATE emulation

    Legacy texture blending (TEXTUREMAPBLEND) and texture stage states: directx6 docs state that
    TEXTUREMAPBLEND is deprecated, yet can still be used. Games must not use both or results
    are undefined. D3DTBLEND_MODULATE mode in particular is dependent on texture pixel format and
    requires fixup of stage 0 texture states when texture changes, but this fixup can interfere
    with games not using this deprecated state. So a flag 'legacyTextureBlending' has to be kept
    in device - TRUE if the app is using TEXTUREMAPBLEND.

    Tests show that setting TEXTUREMAPBLEND on native doesn't seem to change values returned by
    GetTextureStageState and vice versa. Not so on Wine, but it is 'undefined' anyway so, probably, ok,
    unless some broken game will be found that cares. */

    HRESULT hr;
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE("(%p)->(%08x,%d)\n", This, RenderStateType, Value);

    EnterCriticalSection(&ddraw_cs);

    switch(RenderStateType)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            if(Value == 0)
            {
                hr = IWineD3DDevice_SetTexture(This->wineD3DDevice,
                                               0,
                                               NULL);
                break;
            }

            if(Value > This->numHandles)
            {
                FIXME("Specified handle %d out of range\n", Value);
                hr = DDERR_INVALIDPARAMS;
                break;
            }
            if(This->Handles[Value - 1].type != DDrawHandle_Texture)
            {
                FIXME("Handle %d isn't a texture handle\n", Value);
                hr = DDERR_INVALIDPARAMS;
                break;
            }
            else
            {
                IDirectDrawSurfaceImpl *surf = This->Handles[Value - 1].ptr;
                IDirect3DTexture2 *tex = surf ? (IDirect3DTexture2 *)&surf->IDirect3DTexture2_vtbl : NULL;
                hr = IDirect3DDevice3_SetTexture(iface, 0, tex);
                break;
            }
        }

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
        {
            This->legacyTextureBlending = TRUE;

            switch ( (D3DTEXTUREBLEND) Value)
            {
                case D3DTBLEND_MODULATE:
                {
                    BOOL tex_alpha = FALSE;
                    IWineD3DBaseTexture *tex = NULL;
                    WINED3DSURFACE_DESC desc;
                    WINED3DFORMAT fmt;
                    DDPIXELFORMAT ddfmt;

                    hr = IWineD3DDevice_GetTexture(This->wineD3DDevice,
                                                0,
                                                &tex);

                    if(hr == WINED3D_OK && tex)
                    {
                        memset(&desc, 0, sizeof(desc));
                        desc.Format = &fmt;
                        hr = IWineD3DTexture_GetLevelDesc((IWineD3DTexture*) tex, 0, &desc);
                        if (SUCCEEDED(hr))
                        {
                            ddfmt.dwSize = sizeof(ddfmt);
                            PixelFormat_WineD3DtoDD(&ddfmt, fmt);
                            if (ddfmt.u5.dwRGBAlphaBitMask) tex_alpha = TRUE;
                        }

                        IWineD3DBaseTexture_Release(tex);
                    }

                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_SELECTARG1);
                    if (tex_alpha)
                    {
                        IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
                    }
                    else
                    {
                        IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_CURRENT);
                    }

                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_MODULATE);

                    break;
                }

                case D3DTBLEND_ADD:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_ADD);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_SELECTARG2);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG2, WINED3DTA_CURRENT);
                    break;

                case D3DTBLEND_MODULATEALPHA:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_MODULATE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_MODULATE);
                    break;

                case D3DTBLEND_COPY:
                case D3DTBLEND_DECAL:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_SELECTARG1);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_SELECTARG1);
                    break;

                case D3DTBLEND_DECALALPHA:
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLOROP, WINED3DTOP_BLENDTEXTUREALPHA);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG1, WINED3DTA_TEXTURE);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_COLORARG2, WINED3DTA_CURRENT);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAOP, WINED3DTOP_SELECTARG2);
                    IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG2, WINED3DTA_CURRENT);
                    break;

                default:
                    ERR("Unhandled texture environment %d !\n",Value);
            }

            hr = D3D_OK;
            break;
        }

        default:
            hr = IDirect3DDevice7_SetRenderState((IDirect3DDevice7 *)This, RenderStateType, Value);
            break;
    }

    LeaveCriticalSection(&ddraw_cs);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetRenderState(IDirect3DDevice2 *iface,
                                           D3DRENDERSTATETYPE RenderStateType,
                                           DWORD Value)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%d) thunking to IDirect3DDevice3 interface.\n", This, RenderStateType, Value);
    return IDirect3DDevice3_SetRenderState((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, RenderStateType, Value);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    HRESULT hr;

    TRACE("(%p)->(%08x,%08x)\n", This, LightStateType, Value);

    if (!LightStateType && (LightStateType > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    if (LightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */)
    {
        IDirect3DMaterialImpl *mat;

        if(Value == 0) mat = NULL;
        else if(Value > This->numHandles)
        {
            ERR("Material handle out of range(%d)\n", Value);
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
        else if(This->Handles[Value - 1].type != DDrawHandle_Material)
        {
            ERR("Invalid handle %d\n", Value);
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }
        else
        {
            mat = This->Handles[Value - 1].ptr;
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
                LeaveCriticalSection(&ddraw_cs);
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
                LeaveCriticalSection(&ddraw_cs);
                return DDERR_INVALIDPARAMS;
        }

        hr = IDirect3DDevice7_SetRenderState((IDirect3DDevice7 *)This, rs, Value);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetLightState(IDirect3DDevice2 *iface,
                                          D3DLIGHTSTATETYPE LightStateType,
                                          DWORD Value)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x) thunking to IDirect3DDevice3 interface.\n", This, LightStateType, Value);
    return IDirect3DDevice3_SetLightState((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, LightStateType, Value);
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    HRESULT hr;

    TRACE("(%p)->(%08x,%p)\n", This, LightStateType, Value);

    if (!LightStateType && (LightStateType > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    if(!Value)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
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
                LeaveCriticalSection(&ddraw_cs);
                return DDERR_INVALIDPARAMS;
        }

        hr = IDirect3DDevice7_GetRenderState((IDirect3DDevice7 *)This, rs, Value);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetLightState(IDirect3DDevice2 *iface,
                                          D3DLIGHTSTATETYPE LightStateType,
                                          DWORD *Value)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice3 interface.\n", This, LightStateType, Value);
    return IDirect3DDevice3_GetLightState((IDirect3DDevice3 *)&This->IDirect3DDevice3_vtbl, LightStateType, Value);
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
static HRESULT
IDirect3DDeviceImpl_7_SetTransform(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    D3DTRANSFORMSTATETYPE type;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay\n", This, TransformStateType, Matrix);

    switch(TransformStateType)
    {
        case D3DTRANSFORMSTATE_WORLD :  type = WINED3DTS_WORLDMATRIX(0); break;
        case D3DTRANSFORMSTATE_WORLD1:  type = WINED3DTS_WORLDMATRIX(1); break;
        case D3DTRANSFORMSTATE_WORLD2:  type = WINED3DTS_WORLDMATRIX(2); break;
        case D3DTRANSFORMSTATE_WORLD3:  type = WINED3DTS_WORLDMATRIX(3); break;
        default:                        type = TransformStateType;
    }

    if(!Matrix)
       return DDERR_INVALIDPARAMS;

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetTransform(This->wineD3DDevice,
                                     type,
                                     (WINED3DMATRIX*) Matrix);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTransform_FPUSetup(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    return IDirect3DDeviceImpl_7_SetTransform(iface, TransformStateType, Matrix);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTransform_FPUPreserve(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetTransform(iface, TransformStateType, Matrix);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTransform(IDirect3DDevice3 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_SetTransform((IDirect3DDevice7 *)This, TransformStateType, D3DMatrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetTransform(IDirect3DDevice2 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_SetTransform((IDirect3DDevice7 *)This, TransformStateType, D3DMatrix);
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
static HRESULT
IDirect3DDeviceImpl_7_GetTransform(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    D3DTRANSFORMSTATETYPE type;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay\n", This, TransformStateType, Matrix);

    switch(TransformStateType)
    {
        case D3DTRANSFORMSTATE_WORLD :  type = WINED3DTS_WORLDMATRIX(0); break;
        case D3DTRANSFORMSTATE_WORLD1:  type = WINED3DTS_WORLDMATRIX(1); break;
        case D3DTRANSFORMSTATE_WORLD2:  type = WINED3DTS_WORLDMATRIX(2); break;
        case D3DTRANSFORMSTATE_WORLD3:  type = WINED3DTS_WORLDMATRIX(3); break;
        default:                        type = TransformStateType;
    }

    if(!Matrix)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_GetTransform(This->wineD3DDevice, type, (WINED3DMATRIX*) Matrix);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTransform_FPUSetup(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    return IDirect3DDeviceImpl_7_GetTransform(iface, TransformStateType, Matrix);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTransform_FPUPreserve(IDirect3DDevice7 *iface,
                                   D3DTRANSFORMSTATETYPE TransformStateType,
                                   D3DMATRIX *Matrix)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetTransform(iface, TransformStateType, Matrix);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTransform(IDirect3DDevice3 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_GetTransform((IDirect3DDevice7 *)This, TransformStateType, D3DMatrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetTransform(IDirect3DDevice2 *iface,
                                         D3DTRANSFORMSTATETYPE TransformStateType,
                                         D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_GetTransform((IDirect3DDevice7 *)This, TransformStateType, D3DMatrix);
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
static HRESULT
IDirect3DDeviceImpl_7_MultiplyTransform(IDirect3DDevice7 *iface,
                                        D3DTRANSFORMSTATETYPE TransformStateType,
                                        D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    D3DTRANSFORMSTATETYPE type;
    TRACE("(%p)->(%08x,%p): Relay\n", This, TransformStateType, D3DMatrix);

    switch(TransformStateType)
    {
        case D3DTRANSFORMSTATE_WORLD :  type = WINED3DTS_WORLDMATRIX(0); break;
        case D3DTRANSFORMSTATE_WORLD1:  type = WINED3DTS_WORLDMATRIX(1); break;
        case D3DTRANSFORMSTATE_WORLD2:  type = WINED3DTS_WORLDMATRIX(2); break;
        case D3DTRANSFORMSTATE_WORLD3:  type = WINED3DTS_WORLDMATRIX(3); break;
        default:                        type = TransformStateType;
    }

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_MultiplyTransform(This->wineD3DDevice,
                                          type,
                                          (WINED3DMATRIX*) D3DMatrix);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_MultiplyTransform_FPUSetup(IDirect3DDevice7 *iface,
                                        D3DTRANSFORMSTATETYPE TransformStateType,
                                        D3DMATRIX *D3DMatrix)
{
    return IDirect3DDeviceImpl_7_MultiplyTransform(iface, TransformStateType, D3DMatrix);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_MultiplyTransform_FPUPreserve(IDirect3DDevice7 *iface,
                                        D3DTRANSFORMSTATETYPE TransformStateType,
                                        D3DMATRIX *D3DMatrix)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_MultiplyTransform(iface, TransformStateType, D3DMatrix);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_MultiplyTransform(IDirect3DDevice3 *iface,
                                              D3DTRANSFORMSTATETYPE TransformStateType,
                                              D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_MultiplyTransform((IDirect3DDevice7 *)This, TransformStateType, D3DMatrix);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_MultiplyTransform(IDirect3DDevice2 *iface,
                                              D3DTRANSFORMSTATETYPE TransformStateType,
                                              D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, TransformStateType, D3DMatrix);
    return IDirect3DDevice7_MultiplyTransform((IDirect3DDevice7 *)This, TransformStateType, D3DMatrix);
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
static HRESULT
IDirect3DDeviceImpl_7_DrawPrimitive(IDirect3DDevice7 *iface,
                                    D3DPRIMITIVETYPE PrimitiveType,
                                    DWORD VertexType,
                                    void *Vertices,
                                    DWORD VertexCount,
                                    DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    UINT stride;
    HRESULT hr;
    TRACE("(%p)->(%08x,%08x,%p,%08x,%08x): Relay!\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Flags);

    if(!Vertices)
        return DDERR_INVALIDPARAMS;

    /* Get the stride */
    stride = get_flexible_vertex_size(VertexType);

    /* Set the FVF */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             IDirectDrawImpl_FindDecl(This->ddraw, VertexType));
    if(hr != D3D_OK)
    {
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* This method translates to the user pointer draw of WineD3D */
    IWineD3DDevice_SetPrimitiveType(This->wineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawPrimitiveUP(This->wineD3DDevice, VertexCount, Vertices, stride);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitive_FPUSetup(IDirect3DDevice7 *iface,
                                    D3DPRIMITIVETYPE PrimitiveType,
                                    DWORD VertexType,
                                    void *Vertices,
                                    DWORD VertexCount,
                                    DWORD Flags)
{
    return IDirect3DDeviceImpl_7_DrawPrimitive(iface, PrimitiveType, VertexType, Vertices, VertexCount, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitive_FPUPreserve(IDirect3DDevice7 *iface,
                                    D3DPRIMITIVETYPE PrimitiveType,
                                    DWORD VertexType,
                                    void *Vertices,
                                    DWORD VertexCount,
                                    DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_DrawPrimitive(iface, PrimitiveType, VertexType, Vertices, VertexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitive(IDirect3DDevice3 *iface,
                                          D3DPRIMITIVETYPE PrimitiveType,
                                          DWORD VertexType,
                                          void *Vertices,
                                          DWORD VertexCount,
                                          DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Flags);
    return IDirect3DDevice7_DrawPrimitive((IDirect3DDevice7 *)This,
            PrimitiveType, VertexType, Vertices, VertexCount, Flags);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_DrawPrimitive(IDirect3DDevice2 *iface,
                                          D3DPRIMITIVETYPE PrimitiveType,
                                          D3DVERTEXTYPE VertexType,
                                          void *Vertices,
                                          DWORD VertexCount,
                                          DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
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

    return IDirect3DDevice7_DrawPrimitive((IDirect3DDevice7 *)This, PrimitiveType, FVF, Vertices, VertexCount, Flags);
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
static HRESULT
IDirect3DDeviceImpl_7_DrawIndexedPrimitive(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           void *Vertices,
                                           DWORD VertexCount,
                                           WORD *Indices,
                                           DWORD IndexCount,
                                           DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x): Relay!\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);

    /* Set the D3DDevice's FVF */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             IDirectDrawImpl_FindDecl(This->ddraw, VertexType));
    if(FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    IWineD3DDevice_SetPrimitiveType(This->wineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawIndexedPrimitiveUP(This->wineD3DDevice, 0 /* MinVertexIndex */,
            VertexCount /* UINT NumVertexIndex */, IndexCount, Indices, WINED3DFMT_R16_UINT,
            Vertices, get_flexible_vertex_size(VertexType));
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitive_FPUSetup(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           void *Vertices,
                                           DWORD VertexCount,
                                           WORD *Indices,
                                           DWORD IndexCount,
                                           DWORD Flags)
{
    return IDirect3DDeviceImpl_7_DrawIndexedPrimitive(iface, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitive_FPUPreserve(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           void *Vertices,
                                           DWORD VertexCount,
                                           WORD *Indices,
                                           DWORD IndexCount,
                                           DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_DrawIndexedPrimitive(iface, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);
    return IDirect3DDevice7_DrawIndexedPrimitive((IDirect3DDevice7 *)This,
            PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);
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
    IDirect3DDeviceImpl *This = device_from_device2(iface);
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

    return IDirect3DDevice7_DrawIndexedPrimitive((IDirect3DDevice7 *)This,
            PrimitiveType, FVF, Vertices, VertexCount, Indices, IndexCount, Flags);
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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_SetClipStatus((IDirect3DDevice7 *)This, ClipStatus);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_SetClipStatus(IDirect3DDevice2 *iface,
                                          D3DCLIPSTATUS *ClipStatus)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_SetClipStatus((IDirect3DDevice7 *)This, ClipStatus);
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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    FIXME("(%p)->(%p): Stub!\n", This, ClipStatus);

    /* D3DCLIPSTATUS and WINED3DCLIPSTATUS are different. I don't know how to convert them */
    /* return IWineD3DDevice_GetClipStatus(This->wineD3DDevice, ClipStatus);*/
    return D3D_OK;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetClipStatus(IDirect3DDevice3 *iface,
                                          D3DCLIPSTATUS *ClipStatus)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_GetClipStatus((IDirect3DDevice7 *)This, ClipStatus);
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_2_GetClipStatus(IDirect3DDevice2 *iface,
                                          D3DCLIPSTATUS *ClipStatus)
{
    IDirect3DDeviceImpl *This = device_from_device2(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, ClipStatus);
    return IDirect3DDevice7_GetClipStatus((IDirect3DDevice7 *)This, ClipStatus);
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
static HRESULT
IDirect3DDeviceImpl_7_DrawPrimitiveStrided(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                           DWORD VertexCount,
                                           DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    WineDirect3DVertexStridedData WineD3DStrided;
    DWORD i;
    HRESULT hr;

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
        WineD3DStrided.position.format = WINED3DFMT_R32G32B32_FLOAT;
        WineD3DStrided.position.lpData = D3DDrawPrimStrideData->position.lpvData;
        WineD3DStrided.position.dwStride = D3DDrawPrimStrideData->position.dwStride;
        if (VertexType & D3DFVF_XYZRHW)
        {
            WineD3DStrided.position.format = WINED3DFMT_R32G32B32A32_FLOAT;
            WineD3DStrided.position_transformed = TRUE;
        } else
            WineD3DStrided.position_transformed = FALSE;
    }

    if(VertexType & D3DFVF_NORMAL)
    {
        WineD3DStrided.normal.format = WINED3DFMT_R32G32B32_FLOAT;
        WineD3DStrided.normal.lpData = D3DDrawPrimStrideData->normal.lpvData;
        WineD3DStrided.normal.dwStride = D3DDrawPrimStrideData->normal.dwStride;
    }

    if(VertexType & D3DFVF_DIFFUSE)
    {
        WineD3DStrided.diffuse.format = WINED3DFMT_A8R8G8B8;
        WineD3DStrided.diffuse.lpData = D3DDrawPrimStrideData->diffuse.lpvData;
        WineD3DStrided.diffuse.dwStride = D3DDrawPrimStrideData->diffuse.dwStride;
    }

    if(VertexType & D3DFVF_SPECULAR)
    {
        WineD3DStrided.specular.format = WINED3DFMT_A8R8G8B8;
        WineD3DStrided.specular.lpData = D3DDrawPrimStrideData->specular.lpvData;
        WineD3DStrided.specular.dwStride = D3DDrawPrimStrideData->specular.dwStride;
    }

    for( i = 0; i < GET_TEXCOUNT_FROM_FVF(VertexType); i++)
    {
        switch(GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i))
        {
            case 1: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32_FLOAT; break;
            case 2: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32G32_FLOAT; break;
            case 3: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32G32B32_FLOAT; break;
            case 4: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32G32B32A32_FLOAT; break;
            default: ERR("Unexpected texture coordinate size %d\n",
                         GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i));
        }
        WineD3DStrided.texCoords[i].lpData = D3DDrawPrimStrideData->textureCoords[i].lpvData;
        WineD3DStrided.texCoords[i].dwStride = D3DDrawPrimStrideData->textureCoords[i].dwStride;
    }

    /* WineD3D doesn't need the FVF here */
    EnterCriticalSection(&ddraw_cs);
    IWineD3DDevice_SetPrimitiveType(This->wineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawPrimitiveStrided(This->wineD3DDevice, VertexCount, &WineD3DStrided);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitiveStrided_FPUSetup(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                           DWORD VertexCount,
                                           DWORD Flags)
{
    return IDirect3DDeviceImpl_7_DrawPrimitiveStrided(iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitiveStrided_FPUPreserve(IDirect3DDevice7 *iface,
                                           D3DPRIMITIVETYPE PrimitiveType,
                                           DWORD VertexType,
                                           D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                           DWORD VertexCount,
                                           DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_DrawPrimitiveStrided(iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveStrided(IDirect3DDevice3 *iface,
                                                 D3DPRIMITIVETYPE PrimitiveType,
                                                 DWORD VertexType,
                                                 D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                                 DWORD VertexCount,
                                                 DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);
    return IDirect3DDevice7_DrawPrimitiveStrided((IDirect3DDevice7 *)This,
            PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);
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
static HRESULT
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided(IDirect3DDevice7 *iface,
                                                  D3DPRIMITIVETYPE PrimitiveType,
                                                  DWORD VertexType,
                                                  D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                                  DWORD VertexCount,
                                                  WORD *Indices,
                                                  DWORD IndexCount,
                                                  DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    WineDirect3DVertexStridedData WineD3DStrided;
    DWORD i;
    HRESULT hr;

    TRACE("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x)\n", This, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);

    memset(&WineD3DStrided, 0, sizeof(WineD3DStrided));
    /* Get the strided data right. the wined3d structure is a bit bigger
     * Watch out: The contents of the strided data are determined by the fvf,
     * not by the members set in D3DDrawPrimStrideData. So it's valid
     * to have diffuse.lpvData set to 0xdeadbeef if the diffuse flag is
     * not set in the fvf.
     */
    if(VertexType & D3DFVF_POSITION_MASK)
    {
        WineD3DStrided.position.format = WINED3DFMT_R32G32B32_FLOAT;
        WineD3DStrided.position.lpData = D3DDrawPrimStrideData->position.lpvData;
        WineD3DStrided.position.dwStride = D3DDrawPrimStrideData->position.dwStride;
        if (VertexType & D3DFVF_XYZRHW)
        {
            WineD3DStrided.position.format = WINED3DFMT_R32G32B32A32_FLOAT;
            WineD3DStrided.position_transformed = TRUE;
        } else
            WineD3DStrided.position_transformed = FALSE;
    }

    if(VertexType & D3DFVF_NORMAL)
    {
        WineD3DStrided.normal.format = WINED3DFMT_R32G32B32_FLOAT;
        WineD3DStrided.normal.lpData = D3DDrawPrimStrideData->normal.lpvData;
        WineD3DStrided.normal.dwStride = D3DDrawPrimStrideData->normal.dwStride;
    }

    if(VertexType & D3DFVF_DIFFUSE)
    {
        WineD3DStrided.diffuse.format = WINED3DFMT_A8R8G8B8;
        WineD3DStrided.diffuse.lpData = D3DDrawPrimStrideData->diffuse.lpvData;
        WineD3DStrided.diffuse.dwStride = D3DDrawPrimStrideData->diffuse.dwStride;
    }

    if(VertexType & D3DFVF_SPECULAR)
    {
        WineD3DStrided.specular.format = WINED3DFMT_A8R8G8B8;
        WineD3DStrided.specular.lpData = D3DDrawPrimStrideData->specular.lpvData;
        WineD3DStrided.specular.dwStride = D3DDrawPrimStrideData->specular.dwStride;
    }

    for( i = 0; i < GET_TEXCOUNT_FROM_FVF(VertexType); i++)
    {
        switch(GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i))
        {
            case 1: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32_FLOAT; break;
            case 2: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32G32_FLOAT; break;
            case 3: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32G32B32_FLOAT; break;
            case 4: WineD3DStrided.texCoords[i].format = WINED3DFMT_R32G32B32A32_FLOAT; break;
            default: ERR("Unexpected texture coordinate size %d\n",
                         GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i));
        }
        WineD3DStrided.texCoords[i].lpData = D3DDrawPrimStrideData->textureCoords[i].lpvData;
        WineD3DStrided.texCoords[i].dwStride = D3DDrawPrimStrideData->textureCoords[i].dwStride;
    }

    /* WineD3D doesn't need the FVF here */
    EnterCriticalSection(&ddraw_cs);
    IWineD3DDevice_SetPrimitiveType(This->wineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawIndexedPrimitiveStrided(This->wineD3DDevice,
            IndexCount, &WineD3DStrided, VertexCount, Indices, WINED3DFMT_R16_UINT);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided_FPUSetup(IDirect3DDevice7 *iface,
                                                  D3DPRIMITIVETYPE PrimitiveType,
                                                  DWORD VertexType,
                                                  D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                                  DWORD VertexCount,
                                                  WORD *Indices,
                                                  DWORD IndexCount,
                                                  DWORD Flags)
{
    return IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided(iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided_FPUPreserve(IDirect3DDevice7 *iface,
                                                  D3DPRIMITIVETYPE PrimitiveType,
                                                  DWORD VertexType,
                                                  D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData,
                                                  DWORD VertexCount,
                                                  WORD *Indices,
                                                  DWORD IndexCount,
                                                  DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided(iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p,%08x,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
    return IDirect3DDevice7_DrawIndexedPrimitiveStrided((IDirect3DDevice7 *)This, PrimitiveType,
            VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
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
static HRESULT
IDirect3DDeviceImpl_7_DrawPrimitiveVB(IDirect3DDevice7 *iface,
                                      D3DPRIMITIVETYPE PrimitiveType,
                                      IDirect3DVertexBuffer7 *D3DVertexBuf,
                                      DWORD StartVertex,
                                      DWORD NumVertices,
                                      DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    IDirect3DVertexBufferImpl *vb = (IDirect3DVertexBufferImpl *)D3DVertexBuf;
    HRESULT hr;
    DWORD stride;

    TRACE("(%p)->(%08x,%p,%08x,%08x,%08x)\n", This, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);

    /* Sanity checks */
    if(!vb)
    {
        ERR("(%p) No Vertex buffer specified\n", This);
        return DDERR_INVALIDPARAMS;
    }
    stride = get_flexible_vertex_size(vb->fvf);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             vb->wineD3DVertexDeclaration);
    if(FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
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
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* Now draw the primitives */
    IWineD3DDevice_SetPrimitiveType(This->wineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawPrimitive(This->wineD3DDevice, StartVertex, NumVertices);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitiveVB_FPUSetup(IDirect3DDevice7 *iface,
                                      D3DPRIMITIVETYPE PrimitiveType,
                                      IDirect3DVertexBuffer7 *D3DVertexBuf,
                                      DWORD StartVertex,
                                      DWORD NumVertices,
                                      DWORD Flags)
{
    return IDirect3DDeviceImpl_7_DrawPrimitiveVB(iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawPrimitiveVB_FPUPreserve(IDirect3DDevice7 *iface,
                                      D3DPRIMITIVETYPE PrimitiveType,
                                      IDirect3DVertexBuffer7 *D3DVertexBuf,
                                      DWORD StartVertex,
                                      DWORD NumVertices,
                                      DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_DrawPrimitiveVB(iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawPrimitiveVB(IDirect3DDevice3 *iface,
                                            D3DPRIMITIVETYPE PrimitiveType,
                                            IDirect3DVertexBuffer *D3DVertexBuf,
                                            DWORD StartVertex,
                                            DWORD NumVertices,
                                            DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirect3DVertexBufferImpl *vb = D3DVertexBuf ? vb_from_vb1(D3DVertexBuf) : NULL;
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p,%08x,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This,  PrimitiveType, vb, StartVertex, NumVertices, Flags);
    return IDirect3DDevice7_DrawPrimitiveVB((IDirect3DDevice7 *)This, PrimitiveType,
            (IDirect3DVertexBuffer7 *)vb, StartVertex, NumVertices, Flags);
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
static HRESULT
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB(IDirect3DDevice7 *iface,
                                             D3DPRIMITIVETYPE PrimitiveType,
                                             IDirect3DVertexBuffer7 *D3DVertexBuf,
                                             DWORD StartVertex,
                                             DWORD NumVertices,
                                             WORD *Indices,
                                             DWORD IndexCount,
                                             DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    IDirect3DVertexBufferImpl *vb = (IDirect3DVertexBufferImpl *)D3DVertexBuf;
    DWORD stride = get_flexible_vertex_size(vb->fvf);
    WORD *LockedIndices;
    HRESULT hr;

    TRACE("(%p)->(%08x,%p,%d,%d,%p,%d,%08x)\n", This, PrimitiveType, vb, StartVertex, NumVertices, Indices, IndexCount, Flags);

    /* Steps:
     * 1) Upload the Indices to the index buffer
     * 2) Set the index source
     * 3) Set the Vertex Buffer as the Stream source
     * 4) Call IWineD3DDevice::DrawIndexedPrimitive
     */

    EnterCriticalSection(&ddraw_cs);

    hr = IWineD3DDevice_SetVertexDeclaration(This->wineD3DDevice,
                                             vb->wineD3DVertexDeclaration);
    if(FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* copy the index stream into the index buffer.
     * A new IWineD3DDevice method could be created
     * which takes an user pointer containing the indices
     * or a SetData-Method for the index buffer, which
     * overrides the index buffer data with our pointer.
     */
    hr = IWineD3DBuffer_Map(This->indexbuffer,
                            0 /* OffSetToLock */,
                            IndexCount * sizeof(WORD),
                            (BYTE **) &LockedIndices,
                            0 /* Flags */);
    assert(IndexCount < 0x100000);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DBuffer::Map failed with hr = %08x\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }
    memcpy(LockedIndices, Indices, IndexCount * sizeof(WORD));
    hr = IWineD3DBuffer_Unmap(This->indexbuffer);
    if(hr != D3D_OK)
    {
        ERR("(%p) IWineD3DBuffer::Unmap failed with hr = %08x\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* Set the index stream */
    IWineD3DDevice_SetBaseVertexIndex(This->wineD3DDevice, StartVertex);
    hr = IWineD3DDevice_SetIndices(This->wineD3DDevice, This->indexbuffer,
                                   WINED3DFMT_R16_UINT);

    /* Set the vertex stream source */
    hr = IWineD3DDevice_SetStreamSource(This->wineD3DDevice,
                                        0 /* StreamNumber */,
                                        vb->wineD3DVertexBuffer,
                                        0 /* offset, we pass this to DrawIndexedPrimitive */,
                                        stride);
    if(hr != D3D_OK)
    {
        ERR("(%p) IDirect3DDevice::SetStreamSource failed with hr = %08x\n", This, hr);
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }


    IWineD3DDevice_SetPrimitiveType(This->wineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawIndexedPrimitive(This->wineD3DDevice,
            0 /* minIndex */, NumVertices, 0 /* StartIndex */, IndexCount);

    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB_FPUSetup(IDirect3DDevice7 *iface,
                                             D3DPRIMITIVETYPE PrimitiveType,
                                             IDirect3DVertexBuffer7 *D3DVertexBuf,
                                             DWORD StartVertex,
                                             DWORD NumVertices,
                                             WORD *Indices,
                                             DWORD IndexCount,
                                             DWORD Flags)
{
    return IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB(iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Indices, IndexCount, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB_FPUPreserve(IDirect3DDevice7 *iface,
                                             D3DPRIMITIVETYPE PrimitiveType,
                                             IDirect3DVertexBuffer7 *D3DVertexBuf,
                                             DWORD StartVertex,
                                             DWORD NumVertices,
                                             WORD *Indices,
                                             DWORD IndexCount,
                                             DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB(iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Indices, IndexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB(IDirect3DDevice3 *iface,
                                                   D3DPRIMITIVETYPE PrimitiveType,
                                                   IDirect3DVertexBuffer *D3DVertexBuf,
                                                   WORD *Indices,
                                                   DWORD IndexCount,
                                                   DWORD Flags)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirect3DVertexBufferImpl *VB = vb_from_vb1(D3DVertexBuf);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%p,%p,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, PrimitiveType, VB, Indices, IndexCount, Flags);

    return IDirect3DDevice7_DrawIndexedPrimitiveVB((IDirect3DDevice7 *)This, PrimitiveType,
            (IDirect3DVertexBuffer7 *)VB, 0, IndexCount, Indices, IndexCount, Flags);
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

static DWORD in_plane(UINT plane, D3DVECTOR normal, D3DVALUE origin_plane, D3DVECTOR center, D3DVALUE radius)
{
    float distance, norm;

    norm = sqrt( normal.u1.x * normal.u1.x + normal.u2.y * normal.u2.y + normal.u3.z * normal.u3.z );
    distance = ( origin_plane + normal.u1.x * center.u1.x + normal.u2.y * center.u2.y + normal.u3.z * center.u3.z ) / norm;

    if ( fabs( distance ) < radius ) return D3DSTATUS_CLIPUNIONLEFT << plane;
    if ( distance < -radius ) return (D3DSTATUS_CLIPUNIONLEFT  | D3DSTATUS_CLIPINTERSECTIONLEFT) << plane;
    return 0;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_ComputeSphereVisibility(IDirect3DDevice7 *iface,
                                              D3DVECTOR *Centers,
                                              D3DVALUE *Radii,
                                              DWORD NumSpheres,
                                              DWORD Flags,
                                              DWORD *ReturnValues)
{
    D3DMATRIX m, temp;
    D3DVALUE origin_plane[6];
    D3DVECTOR vec[6];
    HRESULT hr;
    UINT i, j;

    TRACE("(%p)->(%p,%p,%08x,%08x,%p)\n", iface, Centers, Radii, NumSpheres, Flags, ReturnValues);

    hr = IDirect3DDeviceImpl_7_GetTransform(iface, D3DTRANSFORMSTATE_WORLD, &m);
    if ( hr != DD_OK ) return DDERR_INVALIDPARAMS;
    hr = IDirect3DDeviceImpl_7_GetTransform(iface, D3DTRANSFORMSTATE_VIEW, &temp);
    if ( hr != DD_OK ) return DDERR_INVALIDPARAMS;
    multiply_matrix_D3D_way(&m, &m, &temp);

    hr = IDirect3DDeviceImpl_7_GetTransform(iface, D3DTRANSFORMSTATE_PROJECTION, &temp);
    if ( hr != DD_OK ) return DDERR_INVALIDPARAMS;
    multiply_matrix_D3D_way(&m, &m, &temp);

/* Left plane */
    vec[0].u1.x = m._14 + m._11;
    vec[0].u2.y = m._24 + m._21;
    vec[0].u3.z = m._34 + m._31;
    origin_plane[0] = m._44 + m._41;

/* Right plane */
    vec[1].u1.x = m._14 - m._11;
    vec[1].u2.y = m._24 - m._21;
    vec[1].u3.z = m._34 - m._31;
    origin_plane[1] = m._44 - m._41;

/* Top plane */
    vec[2].u1.x = m._14 - m._12;
    vec[2].u2.y = m._24 - m._22;
    vec[2].u3.z = m._34 - m._32;
    origin_plane[2] = m._44 - m._42;

/* Bottom plane */
    vec[3].u1.x = m._14 + m._12;
    vec[3].u2.y = m._24 + m._22;
    vec[3].u3.z = m._34 + m._32;
    origin_plane[3] = m._44 + m._42;

/* Front plane */
    vec[4].u1.x = m._13;
    vec[4].u2.y = m._23;
    vec[4].u3.z = m._33;
    origin_plane[4] = m._43;

/* Back plane*/
    vec[5].u1.x = m._14 - m._13;
    vec[5].u2.y = m._24 - m._23;
    vec[5].u3.z = m._34 - m._33;
    origin_plane[5] = m._44 - m._43;

    for(i=0; i<NumSpheres; i++)
    {
        ReturnValues[i] = 0;
        for(j=0; j<6; j++) ReturnValues[i] |= in_plane(j, vec[j], origin_plane[j], Centers[i], Radii[i]);
    }

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
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p,%p,%08x,%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, Centers, Radii, NumSpheres, Flags, ReturnValues);
    return IDirect3DDevice7_ComputeSphereVisibility((IDirect3DDevice7 *)This,
            Centers, Radii, NumSpheres, Flags, ReturnValues);
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
static HRESULT
IDirect3DDeviceImpl_7_GetTexture(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 **Texture)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    IWineD3DBaseTexture *Surf;
    HRESULT hr;
    TRACE("(%p)->(%d,%p): Relay\n", This, Stage, Texture);

    if(!Texture)
    {
        TRACE("Texture == NULL, failing with DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_GetTexture(This->wineD3DDevice, Stage, &Surf);
    if( (hr != D3D_OK) || (!Surf) ) 
    {
        *Texture = NULL;
        LeaveCriticalSection(&ddraw_cs);
        return hr;
    }

    /* GetParent AddRef()s, which is perfectly OK.
     * We have passed the IDirectDrawSurface7 interface to WineD3D, so that's OK too.
     */
    hr = IWineD3DBaseTexture_GetParent(Surf,
                                       (IUnknown **) Texture);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTexture_FPUSetup(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 **Texture)
{
    return IDirect3DDeviceImpl_7_GetTexture(iface, Stage, Texture);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTexture_FPUPreserve(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 **Texture)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetTexture(iface, Stage, Texture);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTexture(IDirect3DDevice3 *iface,
                                       DWORD Stage,
                                       IDirect3DTexture2 **Texture2)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    HRESULT ret;
    IDirectDrawSurface7 *ret_val;

    TRACE_(ddraw_thunk)("(%p)->(%d,%p) thunking to IDirect3DDevice7 interface.\n", This, Stage, Texture2);
    ret = IDirect3DDevice7_GetTexture((IDirect3DDevice7 *)This, Stage, &ret_val);

    *Texture2 = ret_val ? (IDirect3DTexture2 *)&((IDirectDrawSurfaceImpl *)ret_val)->IDirect3DTexture2_vtbl : NULL;

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
static HRESULT
IDirect3DDeviceImpl_7_SetTexture(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 *Texture)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    IDirectDrawSurfaceImpl *surf = (IDirectDrawSurfaceImpl *)Texture;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay!\n", This, Stage, surf);

    /* Texture may be NULL here */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetTexture(This->wineD3DDevice,
                                   Stage,
                                   surf ? surf->wineD3DTexture : NULL);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTexture_FPUSetup(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 *Texture)
{
    return IDirect3DDeviceImpl_7_SetTexture(iface, Stage, Texture);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTexture_FPUPreserve(IDirect3DDevice7 *iface,
                                 DWORD Stage,
                                 IDirectDrawSurface7 *Texture)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetTexture(iface, Stage, Texture);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_3_SetTexture(IDirect3DDevice3 *iface,
                                 DWORD Stage,
                                 IDirect3DTexture2 *Texture2)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    IDirectDrawSurfaceImpl *tex = Texture2 ? surface_from_texture2(Texture2) : NULL;
    DWORD texmapblend;
    HRESULT hr;
    TRACE("(%p)->(%d,%p)\n", This, Stage, tex);

    EnterCriticalSection(&ddraw_cs);

    if (This->legacyTextureBlending)
        IDirect3DDevice3_GetRenderState(iface, D3DRENDERSTATE_TEXTUREMAPBLEND, &texmapblend);

    hr = IDirect3DDevice7_SetTexture((IDirect3DDevice7 *)This, Stage, (IDirectDrawSurface7 *)tex);

    if (This->legacyTextureBlending && texmapblend == D3DTBLEND_MODULATE)
    {
        /* This fixup is required by the way D3DTBLEND_MODULATE maps to texture stage states.
           See IDirect3DDeviceImpl_3_SetRenderState for details. */
        BOOL tex_alpha = FALSE;
        IWineD3DBaseTexture *tex = NULL;
        WINED3DSURFACE_DESC desc;
        WINED3DFORMAT fmt;
        DDPIXELFORMAT ddfmt;
        HRESULT result;

        result = IWineD3DDevice_GetTexture(This->wineD3DDevice,
                                    0,
                                    &tex);

        if(result == WINED3D_OK && tex)
        {
            memset(&desc, 0, sizeof(desc));
            desc.Format = &fmt;
            result = IWineD3DTexture_GetLevelDesc((IWineD3DTexture*) tex, 0, &desc);
            if (SUCCEEDED(result))
            {
                ddfmt.dwSize = sizeof(ddfmt);
                PixelFormat_WineD3DtoDD(&ddfmt, fmt);
                if (ddfmt.u5.dwRGBAlphaBitMask) tex_alpha = TRUE;
            }

            IWineD3DBaseTexture_Release(tex);
        }

        /* alphaop is WINED3DTOP_SELECTARG1 if it's D3DTBLEND_MODULATE, so only modify alphaarg1 */
        if (tex_alpha)
        {
            IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_TEXTURE);
        }
        else
        {
            IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, 0, WINED3DTSS_ALPHAARG1, WINED3DTA_CURRENT);
        }
    }

    LeaveCriticalSection(&ddraw_cs);

    return hr;
}

static const struct tss_lookup
{
    BOOL sampler_state;
    DWORD state;
}
tss_lookup[] =
{
    {FALSE, WINED3DTSS_FORCE_DWORD},            /*  0, unused */
    {FALSE, WINED3DTSS_COLOROP},                /*  1, D3DTSS_COLOROP */
    {FALSE, WINED3DTSS_COLORARG1},              /*  2, D3DTSS_COLORARG1 */
    {FALSE, WINED3DTSS_COLORARG2},              /*  3, D3DTSS_COLORARG2 */
    {FALSE, WINED3DTSS_ALPHAOP},                /*  4, D3DTSS_ALPHAOP */
    {FALSE, WINED3DTSS_ALPHAARG1},              /*  5, D3DTSS_ALPHAARG1 */
    {FALSE, WINED3DTSS_ALPHAARG2},              /*  6, D3DTSS_ALPHAARG2 */
    {FALSE, WINED3DTSS_BUMPENVMAT00},           /*  7, D3DTSS_BUMPENVMAT00 */
    {FALSE, WINED3DTSS_BUMPENVMAT01},           /*  8, D3DTSS_BUMPENVMAT01 */
    {FALSE, WINED3DTSS_BUMPENVMAT10},           /*  9, D3DTSS_BUMPENVMAT10 */
    {FALSE, WINED3DTSS_BUMPENVMAT11},           /* 10, D3DTSS_BUMPENVMAT11 */
    {FALSE, WINED3DTSS_TEXCOORDINDEX},          /* 11, D3DTSS_TEXCOORDINDEX */
    {TRUE,  WINED3DSAMP_ADDRESSU},              /* 12, D3DTSS_ADDRESS */
    {TRUE,  WINED3DSAMP_ADDRESSU},              /* 13, D3DTSS_ADDRESSU */
    {TRUE,  WINED3DSAMP_ADDRESSV},              /* 14, D3DTSS_ADDRESSV */
    {TRUE,  WINED3DSAMP_BORDERCOLOR},           /* 15, D3DTSS_BORDERCOLOR */
    {TRUE,  WINED3DSAMP_MAGFILTER},             /* 16, D3DTSS_MAGFILTER */
    {TRUE,  WINED3DSAMP_MINFILTER},             /* 17, D3DTSS_MINFILTER */
    {TRUE,  WINED3DSAMP_MIPFILTER},             /* 18, D3DTSS_MIPFILTER */
    {TRUE,  WINED3DSAMP_MIPMAPLODBIAS},         /* 19, D3DTSS_MIPMAPLODBIAS */
    {TRUE,  WINED3DSAMP_MAXMIPLEVEL},           /* 20, D3DTSS_MAXMIPLEVEL */
    {TRUE,  WINED3DSAMP_MAXANISOTROPY},         /* 21, D3DTSS_MAXANISOTROPY */
    {FALSE, WINED3DTSS_BUMPENVLSCALE},          /* 22, D3DTSS_BUMPENVLSCALE */
    {FALSE, WINED3DTSS_BUMPENVLOFFSET},         /* 23, D3DTSS_BUMPENVLOFFSET */
    {FALSE, WINED3DTSS_TEXTURETRANSFORMFLAGS},  /* 24, D3DTSS_TEXTURETRANSFORMFLAGS */
};

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
static HRESULT
IDirect3DDeviceImpl_7_GetTextureStageState(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD *State)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    const struct tss_lookup *l = &tss_lookup[TexStageStateType];
    TRACE("(%p)->(%08x,%08x,%p): Relay!\n", This, Stage, TexStageStateType, State);

    if(!State)
        return DDERR_INVALIDPARAMS;

    if (TexStageStateType > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        WARN("Invalid TexStageStateType %#x passed.\n", TexStageStateType);
        *State = 0;
        return DD_OK;
    }

    EnterCriticalSection(&ddraw_cs);

    if (l->sampler_state)
    {
        hr = IWineD3DDevice_GetSamplerState(This->wineD3DDevice, Stage, l->state, State);

        switch(TexStageStateType)
        {
            /* Mipfilter is a sampler state with different values */
            case D3DTSS_MIPFILTER:
            {
                switch(*State)
                {
                    case WINED3DTEXF_NONE: *State = D3DTFP_NONE; break;
                    case WINED3DTEXF_POINT: *State = D3DTFP_POINT; break;
                    case WINED3DTEXF_LINEAR: *State = D3DTFP_LINEAR; break;
                    default:
                        ERR("Unexpected mipfilter value %#x\n", *State);
                        *State = D3DTFP_NONE;
                        break;
                }
                break;
            }

            /* Magfilter has slightly different values */
            case D3DTSS_MAGFILTER:
            {
                switch(*State)
                {
                    case WINED3DTEXF_POINT: *State = D3DTFG_POINT; break;
                    case WINED3DTEXF_LINEAR: *State = D3DTFG_LINEAR; break;
                    case WINED3DTEXF_ANISOTROPIC: *State = D3DTFG_ANISOTROPIC; break;
                    case WINED3DTEXF_FLATCUBIC: *State = D3DTFG_FLATCUBIC; break;
                    case WINED3DTEXF_GAUSSIANCUBIC: *State = D3DTFG_GAUSSIANCUBIC; break;
                    default:
                        ERR("Unexpected wined3d mag filter value %#x\n", *State);
                        *State = D3DTFG_POINT;
                        break;
                }
                break;
            }

            default:
                break;
        }
    }
    else
    {
        hr = IWineD3DDevice_GetTextureStageState(This->wineD3DDevice, Stage, l->state, State);
    }

    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTextureStageState_FPUSetup(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD *State)
{
    return IDirect3DDeviceImpl_7_GetTextureStageState(iface, Stage, TexStageStateType, State);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetTextureStageState_FPUPreserve(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD *State)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetTextureStageState(iface, Stage, TexStageStateType, State);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_GetTextureStageState(IDirect3DDevice3 *iface,
                                                 DWORD Stage,
                                                 D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                                 DWORD *State)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%p) thunking to IDirect3DDevice7 interface.\n", This, Stage, TexStageStateType, State);
    return IDirect3DDevice7_GetTextureStageState((IDirect3DDevice7 *)This, Stage, TexStageStateType, State);
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
static HRESULT
IDirect3DDeviceImpl_7_SetTextureStageState(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD State)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    const struct tss_lookup *l = &tss_lookup[TexStageStateType];
    HRESULT hr;
    TRACE("(%p)->(%08x,%08x,%08x): Relay!\n", This, Stage, TexStageStateType, State);

    if (TexStageStateType > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        WARN("Invalid TexStageStateType %#x passed.\n", TexStageStateType);
        return DD_OK;
    }

    EnterCriticalSection(&ddraw_cs);

    if (l->sampler_state)
    {
        switch(TexStageStateType)
        {
            /* Mipfilter is a sampler state with different values */
            case D3DTSS_MIPFILTER:
            {
                switch(State)
                {
                    case D3DTFP_NONE: State = WINED3DTEXF_NONE; break;
                    case D3DTFP_POINT: State = WINED3DTEXF_POINT; break;
                    case 0: /* Unchecked */
                    case D3DTFP_LINEAR: State = WINED3DTEXF_LINEAR; break;
                    default:
                        ERR("Unexpected mipfilter value %d\n", State);
                        State = WINED3DTEXF_NONE;
                        break;
                }
                break;
            }

            /* Magfilter has slightly different values */
            case D3DTSS_MAGFILTER:
            {
                switch(State)
                {
                    case D3DTFG_POINT: State = WINED3DTEXF_POINT; break;
                    case D3DTFG_LINEAR: State = WINED3DTEXF_LINEAR; break;
                    case D3DTFG_FLATCUBIC: State = WINED3DTEXF_FLATCUBIC; break;
                    case D3DTFG_GAUSSIANCUBIC: State = WINED3DTEXF_GAUSSIANCUBIC; break;
                    case D3DTFG_ANISOTROPIC: State = WINED3DTEXF_ANISOTROPIC; break;
                    default:
                        ERR("Unexpected d3d7 mag filter type %d\n", State);
                        State = WINED3DTEXF_POINT;
                        break;
                }
                break;
            }

            case D3DTSS_ADDRESS:
                IWineD3DDevice_SetSamplerState(This->wineD3DDevice, Stage, WINED3DSAMP_ADDRESSV, State);
                break;

            default:
                break;
        }

        hr = IWineD3DDevice_SetSamplerState(This->wineD3DDevice, Stage, l->state, State);
    }
    else
    {
        hr = IWineD3DDevice_SetTextureStageState(This->wineD3DDevice, Stage, l->state, State);
    }

    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTextureStageState_FPUSetup(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD State)
{
    return IDirect3DDeviceImpl_7_SetTextureStageState(iface, Stage, TexStageStateType, State);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetTextureStageState_FPUPreserve(IDirect3DDevice7 *iface,
                                           DWORD Stage,
                                           D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                           DWORD State)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetTextureStageState(iface, Stage, TexStageStateType, State);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_SetTextureStageState(IDirect3DDevice3 *iface,
                                                 DWORD Stage,
                                                 D3DTEXTURESTAGESTATETYPE TexStageStateType,
                                                 DWORD State)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%08x,%08x,%08x) thunking to IDirect3DDevice7 interface.\n", This, Stage, TexStageStateType, State);
    return IDirect3DDevice7_SetTextureStageState((IDirect3DDevice7 *)This, Stage, TexStageStateType, State);
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
static HRESULT
IDirect3DDeviceImpl_7_ValidateDevice(IDirect3DDevice7 *iface,
                                     DWORD *NumPasses)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay\n", This, NumPasses);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_ValidateDevice(This->wineD3DDevice, NumPasses);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_ValidateDevice_FPUSetup(IDirect3DDevice7 *iface,
                                     DWORD *NumPasses)
{
    return IDirect3DDeviceImpl_7_ValidateDevice(iface, NumPasses);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_ValidateDevice_FPUPreserve(IDirect3DDevice7 *iface,
                                     DWORD *NumPasses)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_ValidateDevice(iface, NumPasses);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI
Thunk_IDirect3DDeviceImpl_3_ValidateDevice(IDirect3DDevice3 *iface,
                                           DWORD *Passes)
{
    IDirect3DDeviceImpl *This = device_from_device3(iface);
    TRACE_(ddraw_thunk)("(%p)->(%p) thunking to IDirect3DDevice7 interface.\n", This, Passes);
    return IDirect3DDevice7_ValidateDevice((IDirect3DDevice7 *)This, Passes);
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
static HRESULT
IDirect3DDeviceImpl_7_Clear(IDirect3DDevice7 *iface,
                            DWORD Count,
                            D3DRECT *Rects,
                            DWORD Flags,
                            D3DCOLOR Color,
                            D3DVALUE Z,
                            DWORD Stencil)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p,%08x,%08x,%f,%08x): Relay\n", This, Count, Rects, Flags, Color, Z, Stencil);

    /* Note; D3DRECT is compatible with WINED3DRECT */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_Clear(This->wineD3DDevice, Count, (WINED3DRECT*) Rects, Flags, Color, Z, Stencil);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_Clear_FPUSetup(IDirect3DDevice7 *iface,
                            DWORD Count,
                            D3DRECT *Rects,
                            DWORD Flags,
                            D3DCOLOR Color,
                            D3DVALUE Z,
                            DWORD Stencil)
{
    return IDirect3DDeviceImpl_7_Clear(iface, Count, Rects, Flags, Color, Z, Stencil);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_Clear_FPUPreserve(IDirect3DDevice7 *iface,
                            DWORD Count,
                            D3DRECT *Rects,
                            DWORD Flags,
                            D3DCOLOR Color,
                            D3DVALUE Z,
                            DWORD Stencil)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_Clear(iface, Count, Rects, Flags, Color, Z, Stencil);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_SetViewport(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p) Relay!\n", This, Data);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DVIEWPORT7 is compatible with WINED3DVIEWPORT */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetViewport(This->wineD3DDevice,
                                    (WINED3DVIEWPORT*) Data);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetViewport_FPUSetup(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    return IDirect3DDeviceImpl_7_SetViewport(iface, Data);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetViewport_FPUPreserve(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetViewport(iface, Data);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_GetViewport(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p) Relay!\n", This, Data);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DVIEWPORT7 is compatible with WINED3DVIEWPORT */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_GetViewport(This->wineD3DDevice,
                                    (WINED3DVIEWPORT*) Data);

    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetViewport_FPUSetup(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    return IDirect3DDeviceImpl_7_GetViewport(iface, Data);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetViewport_FPUPreserve(IDirect3DDevice7 *iface,
                                  D3DVIEWPORT7 *Data)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetViewport(iface, Data);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_SetMaterial(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay!\n", This, Mat);

    if (!Mat) return DDERR_INVALIDPARAMS;
    /* Note: D3DMATERIAL7 is compatible with WINED3DMATERIAL */
    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetMaterial(This->wineD3DDevice,
                                    (WINED3DMATERIAL*) Mat);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetMaterial_FPUSetup(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    return IDirect3DDeviceImpl_7_SetMaterial(iface, Mat);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetMaterial_FPUPreserve(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetMaterial(iface, Mat);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_GetMaterial(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay!\n", This, Mat);

    EnterCriticalSection(&ddraw_cs);
    /* Note: D3DMATERIAL7 is compatible with WINED3DMATERIAL */ 
    hr = IWineD3DDevice_GetMaterial(This->wineD3DDevice,
                                    (WINED3DMATERIAL*) Mat);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetMaterial_FPUSetup(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    return IDirect3DDeviceImpl_7_GetMaterial(iface, Mat);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetMaterial_FPUPreserve(IDirect3DDevice7 *iface,
                                  D3DMATERIAL7 *Mat)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetMaterial(iface, Mat);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_SetLight(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay!\n", This, LightIndex, Light);

    EnterCriticalSection(&ddraw_cs);
    /* Note: D3DLIGHT7 is compatible with WINED3DLIGHT */
    hr = IWineD3DDevice_SetLight(This->wineD3DDevice,
                                 LightIndex,
                                 (WINED3DLIGHT*) Light);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetLight_FPUSetup(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    return IDirect3DDeviceImpl_7_SetLight(iface, LightIndex, Light);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetLight_FPUPreserve(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetLight(iface, LightIndex, Light);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_GetLight(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT rc;
    TRACE("(%p)->(%08x,%p): Relay!\n", This, LightIndex, Light);

    EnterCriticalSection(&ddraw_cs);
    /* Note: D3DLIGHT7 is compatible with WINED3DLIGHT */
    rc =  IWineD3DDevice_GetLight(This->wineD3DDevice,
                                  LightIndex,
                                  (WINED3DLIGHT*) Light);

    /* Translate the result. WineD3D returns other values than D3D7 */
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(rc);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetLight_FPUSetup(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    return IDirect3DDeviceImpl_7_GetLight(iface, LightIndex, Light);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetLight_FPUPreserve(IDirect3DDevice7 *iface,
                               DWORD LightIndex,
                               D3DLIGHT7 *Light)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetLight(iface, LightIndex, Light);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_BeginStateBlock(IDirect3DDevice7 *iface)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(): Relay!\n", This);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_BeginStateBlock(This->wineD3DDevice);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_BeginStateBlock_FPUSetup(IDirect3DDevice7 *iface)
{
    return IDirect3DDeviceImpl_7_BeginStateBlock(iface);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_BeginStateBlock_FPUPreserve(IDirect3DDevice7 *iface)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_BeginStateBlock(iface);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_EndStateBlock(IDirect3DDevice7 *iface,
                                    DWORD *BlockHandle)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p): Relay!\n", This, BlockHandle);

    if(!BlockHandle)
    {
        WARN("BlockHandle == NULL, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    *BlockHandle = IDirect3DDeviceImpl_CreateHandle(This);
    if(!*BlockHandle)
    {
        ERR("Cannot get a handle number for the stateblock\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_OUTOFMEMORY;
    }
    This->Handles[*BlockHandle - 1].type = DDrawHandle_StateBlock;
    hr = IWineD3DDevice_EndStateBlock(This->wineD3DDevice,
                                      (IWineD3DStateBlock **) &This->Handles[*BlockHandle - 1].ptr);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_EndStateBlock_FPUSetup(IDirect3DDevice7 *iface,
                                    DWORD *BlockHandle)
{
    return IDirect3DDeviceImpl_7_EndStateBlock(iface, BlockHandle);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_EndStateBlock_FPUPreserve(IDirect3DDevice7 *iface,
                                    DWORD *BlockHandle)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_EndStateBlock(iface, BlockHandle);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_PreLoad(IDirect3DDevice7 *iface,
                              IDirectDrawSurface7 *Texture)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    IDirectDrawSurfaceImpl *surf = (IDirectDrawSurfaceImpl *)Texture;

    TRACE("(%p)->(%p): Relay!\n", This, surf);

    if(!Texture)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    IWineD3DSurface_PreLoad(surf->WineD3DSurface);
    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_PreLoad_FPUSetup(IDirect3DDevice7 *iface,
                              IDirectDrawSurface7 *Texture)
{
    return IDirect3DDeviceImpl_7_PreLoad(iface, Texture);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_PreLoad_FPUPreserve(IDirect3DDevice7 *iface,
                              IDirectDrawSurface7 *Texture)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_PreLoad(iface, Texture);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_ApplyStateBlock(IDirect3DDevice7 *iface,
                                      DWORD BlockHandle)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x): Relay!\n", This, BlockHandle);

    EnterCriticalSection(&ddraw_cs);
    if(!BlockHandle || BlockHandle > This->numHandles)
    {
        WARN("Out of range handle %d, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        LeaveCriticalSection(&ddraw_cs);
        return D3DERR_INVALIDSTATEBLOCK;
    }
    if(This->Handles[BlockHandle - 1].type != DDrawHandle_StateBlock)
    {
        WARN("Handle %d is not a stateblock, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        LeaveCriticalSection(&ddraw_cs);
        return D3DERR_INVALIDSTATEBLOCK;
    }

    hr = IWineD3DStateBlock_Apply((IWineD3DStateBlock *) This->Handles[BlockHandle - 1].ptr);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_ApplyStateBlock_FPUSetup(IDirect3DDevice7 *iface,
                                      DWORD BlockHandle)
{
    return IDirect3DDeviceImpl_7_ApplyStateBlock(iface, BlockHandle);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_ApplyStateBlock_FPUPreserve(IDirect3DDevice7 *iface,
                                      DWORD BlockHandle)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_ApplyStateBlock(iface, BlockHandle);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_CaptureStateBlock(IDirect3DDevice7 *iface,
                                        DWORD BlockHandle)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x): Relay!\n", This, BlockHandle);

    EnterCriticalSection(&ddraw_cs);
    if(BlockHandle == 0 || BlockHandle > This->numHandles)
    {
        WARN("Out of range handle %d, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        LeaveCriticalSection(&ddraw_cs);
        return D3DERR_INVALIDSTATEBLOCK;
    }
    if(This->Handles[BlockHandle - 1].type != DDrawHandle_StateBlock)
    {
        WARN("Handle %d is not a stateblock, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        LeaveCriticalSection(&ddraw_cs);
        return D3DERR_INVALIDSTATEBLOCK;
    }

    hr = IWineD3DStateBlock_Capture((IWineD3DStateBlock *) This->Handles[BlockHandle - 1].ptr);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_CaptureStateBlock_FPUSetup(IDirect3DDevice7 *iface,
                                        DWORD BlockHandle)
{
    return IDirect3DDeviceImpl_7_CaptureStateBlock(iface, BlockHandle);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_CaptureStateBlock_FPUPreserve(IDirect3DDevice7 *iface,
                                        DWORD BlockHandle)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_CaptureStateBlock(iface, BlockHandle);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_DeleteStateBlock(IDirect3DDevice7 *iface,
                                       DWORD BlockHandle)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    ULONG ref;
    TRACE("(%p)->(%08x): Relay!\n", This, BlockHandle);

    EnterCriticalSection(&ddraw_cs);
    if(BlockHandle == 0 || BlockHandle > This->numHandles)
    {
        WARN("Out of range handle %d, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        LeaveCriticalSection(&ddraw_cs);
        return D3DERR_INVALIDSTATEBLOCK;
    }
    if(This->Handles[BlockHandle - 1].type != DDrawHandle_StateBlock)
    {
        WARN("Handle %d is not a stateblock, returning D3DERR_INVALIDSTATEBLOCK\n", BlockHandle);
        LeaveCriticalSection(&ddraw_cs);
        return D3DERR_INVALIDSTATEBLOCK;
    }

    ref = IWineD3DStateBlock_Release((IWineD3DStateBlock *) This->Handles[BlockHandle - 1].ptr);
    if(ref)
    {
        ERR("Something is still holding the stateblock %p(Handle %d). Ref = %d\n", This->Handles[BlockHandle - 1].ptr, BlockHandle, ref);
    }
    This->Handles[BlockHandle - 1].ptr = NULL;
    This->Handles[BlockHandle - 1].type = DDrawHandle_Unknown;

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DeleteStateBlock_FPUSetup(IDirect3DDevice7 *iface,
                                       DWORD BlockHandle)
{
    return IDirect3DDeviceImpl_7_DeleteStateBlock(iface, BlockHandle);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_DeleteStateBlock_FPUPreserve(IDirect3DDevice7 *iface,
                                       DWORD BlockHandle)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_DeleteStateBlock(iface, BlockHandle);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_CreateStateBlock(IDirect3DDevice7 *iface,
                                       D3DSTATEBLOCKTYPE Type,
                                       DWORD *BlockHandle)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p)!\n", This, Type, BlockHandle);

    if(!BlockHandle)
    {
        WARN("BlockHandle == NULL, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }
    if(Type != D3DSBT_ALL         && Type != D3DSBT_PIXELSTATE &&
       Type != D3DSBT_VERTEXSTATE                              ) {
        WARN("Unexpected stateblock type, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    EnterCriticalSection(&ddraw_cs);
    *BlockHandle = IDirect3DDeviceImpl_CreateHandle(This);
    if(!*BlockHandle)
    {
        ERR("Cannot get a handle number for the stateblock\n");
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_OUTOFMEMORY;
    }
    This->Handles[*BlockHandle - 1].type = DDrawHandle_StateBlock;

    /* The D3DSTATEBLOCKTYPE enum is fine here */
    hr = IWineD3DDevice_CreateStateBlock(This->wineD3DDevice,
                                         Type,
                                         (IWineD3DStateBlock **) &This->Handles[*BlockHandle - 1].ptr,
                                         NULL /* Parent, hope that works */);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_CreateStateBlock_FPUSetup(IDirect3DDevice7 *iface,
                                       D3DSTATEBLOCKTYPE Type,
                                       DWORD *BlockHandle)
{
    return IDirect3DDeviceImpl_7_CreateStateBlock(iface, Type, BlockHandle);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_CreateStateBlock_FPUPreserve(IDirect3DDevice7 *iface,
                                       D3DSTATEBLOCKTYPE Type,
                                       DWORD *BlockHandle)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr =IDirect3DDeviceImpl_7_CreateStateBlock(iface, Type, BlockHandle);
    set_fpu_control_word(old_fpucw);

    return hr;
}

/* Helper function for IDirect3DDeviceImpl_7_Load. */
static BOOL is_mip_level_subset(IDirectDrawSurfaceImpl *dest,
                                IDirectDrawSurfaceImpl *src)
{
    IDirectDrawSurfaceImpl *src_level, *dest_level;
    IDirectDrawSurface7 *temp;
    DDSURFACEDESC2 ddsd;
    BOOL levelFound; /* at least one suitable sublevel in dest found */

    /* To satisfy "destination is mip level subset of source" criteria (regular texture counts as 1 level),
     * 1) there must be at least one mip level in destination that matched dimensions of some mip level in source and
     * 2) there must be no destination levels that don't match any levels in source. Otherwise it's INVALIDPARAMS.
     */
    levelFound = FALSE;

    src_level = src;
    dest_level = dest;

    for (;src_level && dest_level;)
    {
        if (src_level->surface_desc.dwWidth == dest_level->surface_desc.dwWidth &&
            src_level->surface_desc.dwHeight == dest_level->surface_desc.dwHeight)
        {
            levelFound = TRUE;

            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
            IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)dest_level, &ddsd.ddsCaps, &temp);

            if (dest_level != dest) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)dest_level);

            dest_level = (IDirectDrawSurfaceImpl *)temp;
        }

        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
        IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)src_level, &ddsd.ddsCaps, &temp);

        if (src_level != src) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)src_level);

        src_level = (IDirectDrawSurfaceImpl *)temp;
    }

    if (src_level && src_level != src) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)src_level);
    if (dest_level && dest_level != dest) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)dest_level);

    return !dest_level && levelFound;
}

/* Helper function for IDirect3DDeviceImpl_7_Load. */
static void copy_mipmap_chain(IDirect3DDeviceImpl *device,
                              IDirectDrawSurfaceImpl *dest,
                              IDirectDrawSurfaceImpl *src,
                              POINT *DestPoint,
                              RECT *SrcRect)
{
    IDirectDrawSurfaceImpl *src_level, *dest_level;
    IDirectDrawSurface7 *temp;
    DDSURFACEDESC2 ddsd;
    POINT point;
    RECT rect;
    HRESULT hr;
    IDirectDrawPalette *pal = NULL, *pal_src = NULL;
    DWORD ckeyflag;
    DDCOLORKEY ddckey;
    BOOL palette_missing = FALSE;

    /* Copy palette, if possible. */
    IDirectDrawSurface7_GetPalette((IDirectDrawSurface7 *)src, &pal_src);
    IDirectDrawSurface7_GetPalette((IDirectDrawSurface7 *)dest, &pal);

    if (pal_src != NULL && pal != NULL)
    {
        PALETTEENTRY palent[256];

        IDirectDrawPalette_GetEntries(pal_src, 0, 0, 256, palent);
        IDirectDrawPalette_SetEntries(pal, 0, 0, 256, palent);
    }

    if (dest->surface_desc.u4.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 |
            DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXEDTO8) && !pal)
    {
        palette_missing = TRUE;
    }

    if (pal) IDirectDrawPalette_Release(pal);
    if (pal_src) IDirectDrawPalette_Release(pal_src);

    /* Copy colorkeys, if present. */
    for (ckeyflag = DDCKEY_DESTBLT; ckeyflag <= DDCKEY_SRCOVERLAY; ckeyflag <<= 1)
    {
        hr = IDirectDrawSurface7_GetColorKey((IDirectDrawSurface7 *)src, ckeyflag, &ddckey);

        if (SUCCEEDED(hr))
        {
            IDirectDrawSurface7_SetColorKey((IDirectDrawSurface7 *)dest, ckeyflag, &ddckey);
        }
    }

    src_level = src;
    dest_level = dest;

    point = *DestPoint;
    rect = *SrcRect;

    for (;src_level && dest_level;)
    {
        if (src_level->surface_desc.dwWidth == dest_level->surface_desc.dwWidth &&
            src_level->surface_desc.dwHeight == dest_level->surface_desc.dwHeight)
        {
            /* Try UpdateSurface that may perform a more direct opengl loading. But skip this if destination is paletted texture and has no palette.
             * Some games like Sacrifice set palette after Load, and it is a waste of effort to try to load texture without palette and generates
             * warnings in wined3d. */
	    if (!palette_missing)
                hr = IWineD3DDevice_UpdateSurface(device->wineD3DDevice, src_level->WineD3DSurface, &rect, dest_level->WineD3DSurface,
                                &point);

            if (palette_missing || FAILED(hr))
            {
                /* UpdateSurface may fail e.g. if dest is in system memory. Fall back to BltFast that is less strict. */
                IWineD3DSurface_BltFast(dest_level->WineD3DSurface,
                                        point.x, point.y,
                                        src_level->WineD3DSurface, &rect, 0);
            }

            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
            IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)dest_level, &ddsd.ddsCaps, &temp);

            if (dest_level != dest) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)dest_level);

            dest_level = (IDirectDrawSurfaceImpl *)temp;
        }

        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
        IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)src_level, &ddsd.ddsCaps, &temp);

        if (src_level != src) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)src_level);

        src_level = (IDirectDrawSurfaceImpl *)temp;

        point.x /= 2;
        point.y /= 2;

        rect.top /= 2;
        rect.left /= 2;
        rect.right = (rect.right + 1) / 2;
        rect.bottom = (rect.bottom + 1) / 2;
    }

    if (src_level && src_level != src) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)src_level);
    if (dest_level && dest_level != dest) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)dest_level);
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
 *  Flags: Cubemap faces to load (DDSCAPS2_CUBEMAP_ALLFACES, DDSCAPS2_CUBEMAP_POSITIVEX,
 *          DDSCAPS2_CUBEMAP_NEGATIVEX, DDSCAPS2_CUBEMAP_POSITIVEY, DDSCAPS2_CUBEMAP_NEGATIVEY,
 *          DDSCAPS2_CUBEMAP_POSITIVEZ, DDSCAPS2_CUBEMAP_NEGATIVEZ)
 *
 * Returns:
 *  D3D_OK on success
 *  DDERR_INVALIDPARAMS if DestTex or SrcTex are NULL, broken coordinates or anything unexpected.
 *
 *
 *****************************************************************************/

static HRESULT
IDirect3DDeviceImpl_7_Load(IDirect3DDevice7 *iface,
                           IDirectDrawSurface7 *DestTex,
                           POINT *DestPoint,
                           IDirectDrawSurface7 *SrcTex,
                           RECT *SrcRect,
                           DWORD Flags)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    IDirectDrawSurfaceImpl *dest = (IDirectDrawSurfaceImpl *)DestTex;
    IDirectDrawSurfaceImpl *src = (IDirectDrawSurfaceImpl *)SrcTex;
    POINT destpoint;
    RECT srcrect;
    TRACE("(%p)->(%p,%p,%p,%p,%08x)\n", This, dest, DestPoint, src, SrcRect, Flags);

    if( (!src) || (!dest) )
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);

    if (SrcRect) srcrect = *SrcRect;
    else
    {
        srcrect.left = srcrect.top = 0;
        srcrect.right = src->surface_desc.dwWidth;
        srcrect.bottom = src->surface_desc.dwHeight;
    }

    if (DestPoint) destpoint = *DestPoint;
    else
    {
        destpoint.x = destpoint.y = 0;
    }
    /* Check bad dimensions. DestPoint is validated against src, not dest, because
     * destination can be a subset of mip levels, in which case actual coordinates used
     * for it may be divided. If any dimension of dest is larger than source, it can't be
     * mip level subset, so an error can be returned early.
     */
    if (srcrect.left >= srcrect.right || srcrect.top >= srcrect.bottom ||
        srcrect.right > src->surface_desc.dwWidth ||
        srcrect.bottom > src->surface_desc.dwHeight ||
        destpoint.x + srcrect.right - srcrect.left > src->surface_desc.dwWidth ||
        destpoint.y + srcrect.bottom - srcrect.top > src->surface_desc.dwHeight ||
        dest->surface_desc.dwWidth > src->surface_desc.dwWidth ||
        dest->surface_desc.dwHeight > src->surface_desc.dwHeight)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* Must be top level surfaces. */
    if (src->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL ||
        dest->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    if (src->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        DWORD src_face_flag, dest_face_flag;
        IDirectDrawSurfaceImpl *src_face, *dest_face;
        IDirectDrawSurface7 *temp;
        DDSURFACEDESC2 ddsd;
        int i;

        if (!(dest->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP))
        {
            LeaveCriticalSection(&ddraw_cs);
            return DDERR_INVALIDPARAMS;
        }

        /* Iterate through cube faces 2 times. First time is just to check INVALIDPARAMS conditions, second
         * time it's actual surface loading. */
        for (i = 0; i < 2; i++)
        {
            dest_face = dest;
            src_face = src;

            for (;dest_face && src_face;)
            {
                src_face_flag = src_face->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES;
                dest_face_flag = dest_face->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES;

                if (src_face_flag == dest_face_flag)
                {
                    if (i == 0)
                    {
                        /* Destination mip levels must be subset of source mip levels. */
                        if (!is_mip_level_subset(dest_face, src_face))
                        {
                            LeaveCriticalSection(&ddraw_cs);
                            return DDERR_INVALIDPARAMS;
                        }
                    }
                    else if (Flags & dest_face_flag)
                    {
                        copy_mipmap_chain(This, dest_face, src_face, &destpoint, &srcrect);
                    }

                    if (src_face_flag < DDSCAPS2_CUBEMAP_NEGATIVEZ)
                    {
                        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
                        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | (src_face_flag << 1);
                        IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)src, &ddsd.ddsCaps, &temp);

                        if (src_face != src) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)src_face);

                        src_face = (IDirectDrawSurfaceImpl *)temp;
                    }
                    else
                    {
                        if (src_face != src) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)src_face);

                        src_face = NULL;
                    }
                }

                if (dest_face_flag < DDSCAPS2_CUBEMAP_NEGATIVEZ)
                {
                    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
                    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | (dest_face_flag << 1);
                    IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)dest, &ddsd.ddsCaps, &temp);

                    if (dest_face != dest) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)dest_face);

                    dest_face = (IDirectDrawSurfaceImpl *)temp;
                }
                else
                {
                    if (dest_face != dest) IDirectDrawSurface7_Release((IDirectDrawSurface7 *)dest_face);

                    dest_face = NULL;
                }
            }

            if (i == 0)
            {
                /* Native returns error if src faces are not subset of dest faces. */
                if (src_face)
                {
                    LeaveCriticalSection(&ddraw_cs);
                    return DDERR_INVALIDPARAMS;
                }
            }
        }

        LeaveCriticalSection(&ddraw_cs);
        return D3D_OK;
    }
    else if (dest->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    /* Handle non cube map textures. */

    /* Destination mip levels must be subset of source mip levels. */
    if (!is_mip_level_subset(dest, src))
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_INVALIDPARAMS;
    }

    copy_mipmap_chain(This, dest, src, &destpoint, &srcrect);

    LeaveCriticalSection(&ddraw_cs);
    return D3D_OK;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_Load_FPUSetup(IDirect3DDevice7 *iface,
                           IDirectDrawSurface7 *DestTex,
                           POINT *DestPoint,
                           IDirectDrawSurface7 *SrcTex,
                           RECT *SrcRect,
                           DWORD Flags)
{
    return IDirect3DDeviceImpl_7_Load(iface, DestTex, DestPoint, SrcTex, SrcRect, Flags);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_Load_FPUPreserve(IDirect3DDevice7 *iface,
                           IDirectDrawSurface7 *DestTex,
                           POINT *DestPoint,
                           IDirectDrawSurface7 *SrcTex,
                           RECT *SrcRect,
                           DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_Load(iface, DestTex, DestPoint, SrcTex, SrcRect, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_LightEnable(IDirect3DDevice7 *iface,
                                  DWORD LightIndex,
                                  BOOL Enable)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%d): Relay!\n", This, LightIndex, Enable);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetLightEnable(This->wineD3DDevice, LightIndex, Enable);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_LightEnable_FPUSetup(IDirect3DDevice7 *iface,
                                  DWORD LightIndex,
                                  BOOL Enable)
{
    return IDirect3DDeviceImpl_7_LightEnable(iface, LightIndex, Enable);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_LightEnable_FPUPreserve(IDirect3DDevice7 *iface,
                                  DWORD LightIndex,
                                  BOOL Enable)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_LightEnable(iface, LightIndex, Enable);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_GetLightEnable(IDirect3DDevice7 *iface,
                                     DWORD LightIndex,
                                     BOOL* Enable)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay\n", This, LightIndex, Enable);

    if(!Enable)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_GetLightEnable(This->wineD3DDevice, LightIndex, Enable);
    LeaveCriticalSection(&ddraw_cs);
    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetLightEnable_FPUSetup(IDirect3DDevice7 *iface,
                                     DWORD LightIndex,
                                     BOOL* Enable)
{
    return IDirect3DDeviceImpl_7_GetLightEnable(iface, LightIndex, Enable);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetLightEnable_FPUPreserve(IDirect3DDevice7 *iface,
                                     DWORD LightIndex,
                                     BOOL* Enable)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetLightEnable(iface, LightIndex, Enable);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_SetClipPlane(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p): Relay!\n", This, Index, PlaneEquation);

    if(!PlaneEquation)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_SetClipPlane(This->wineD3DDevice, Index, PlaneEquation);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetClipPlane_FPUSetup(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    return IDirect3DDeviceImpl_7_SetClipPlane(iface, Index, PlaneEquation);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_SetClipPlane_FPUPreserve(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_SetClipPlane(iface, Index, PlaneEquation);
    set_fpu_control_word(old_fpucw);

    return hr;
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
static HRESULT
IDirect3DDeviceImpl_7_GetClipPlane(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d,%p): Relay!\n", This, Index, PlaneEquation);

    if(!PlaneEquation)
        return DDERR_INVALIDPARAMS;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DDevice_GetClipPlane(This->wineD3DDevice, Index, PlaneEquation);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetClipPlane_FPUSetup(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    return IDirect3DDeviceImpl_7_GetClipPlane(iface, Index, PlaneEquation);
}

static HRESULT WINAPI
IDirect3DDeviceImpl_7_GetClipPlane_FPUPreserve(IDirect3DDevice7 *iface,
                                   DWORD Index,
                                   D3DVALUE* PlaneEquation)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_GetClipPlane(iface, Index, PlaneEquation);
    set_fpu_control_word(old_fpucw);

    return hr;
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
    IDirect3DDeviceImpl *This = (IDirect3DDeviceImpl *)iface;
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

/* For performance optimization, devices created in FPUSETUP and FPUPRESERVE modes
 * have separate vtables. Simple functions where this doesn't matter like GetDirect3D
 * are not duplicated.

 * Device created with DDSCL_FPUSETUP (d3d7 default) - device methods assume that FPU
 * has already been setup for optimal d3d operation.

 * Device created with DDSCL_FPUPRESERVE - resets and restores FPU mode when necessary in
 * d3d calls (FPU may be in a mode non-suitable for d3d when the app calls d3d). Required
 * by Sacrifice (game). */
const IDirect3DDevice7Vtbl IDirect3DDevice7_FPUSetup_Vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DDeviceImpl_7_QueryInterface,
    IDirect3DDeviceImpl_7_AddRef,
    IDirect3DDeviceImpl_7_Release,
    /*** IDirect3DDevice7 ***/
    IDirect3DDeviceImpl_7_GetCaps_FPUSetup,
    IDirect3DDeviceImpl_7_EnumTextureFormats_FPUSetup,
    IDirect3DDeviceImpl_7_BeginScene_FPUSetup,
    IDirect3DDeviceImpl_7_EndScene_FPUSetup,
    IDirect3DDeviceImpl_7_GetDirect3D,
    IDirect3DDeviceImpl_7_SetRenderTarget_FPUSetup,
    IDirect3DDeviceImpl_7_GetRenderTarget,
    IDirect3DDeviceImpl_7_Clear_FPUSetup,
    IDirect3DDeviceImpl_7_SetTransform_FPUSetup,
    IDirect3DDeviceImpl_7_GetTransform_FPUSetup,
    IDirect3DDeviceImpl_7_SetViewport_FPUSetup,
    IDirect3DDeviceImpl_7_MultiplyTransform_FPUSetup,
    IDirect3DDeviceImpl_7_GetViewport_FPUSetup,
    IDirect3DDeviceImpl_7_SetMaterial_FPUSetup,
    IDirect3DDeviceImpl_7_GetMaterial_FPUSetup,
    IDirect3DDeviceImpl_7_SetLight_FPUSetup,
    IDirect3DDeviceImpl_7_GetLight_FPUSetup,
    IDirect3DDeviceImpl_7_SetRenderState_FPUSetup,
    IDirect3DDeviceImpl_7_GetRenderState_FPUSetup,
    IDirect3DDeviceImpl_7_BeginStateBlock_FPUSetup,
    IDirect3DDeviceImpl_7_EndStateBlock_FPUSetup,
    IDirect3DDeviceImpl_7_PreLoad_FPUSetup,
    IDirect3DDeviceImpl_7_DrawPrimitive_FPUSetup,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitive_FPUSetup,
    IDirect3DDeviceImpl_7_SetClipStatus,
    IDirect3DDeviceImpl_7_GetClipStatus,
    IDirect3DDeviceImpl_7_DrawPrimitiveStrided_FPUSetup,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided_FPUSetup,
    IDirect3DDeviceImpl_7_DrawPrimitiveVB_FPUSetup,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB_FPUSetup,
    IDirect3DDeviceImpl_7_ComputeSphereVisibility,
    IDirect3DDeviceImpl_7_GetTexture_FPUSetup,
    IDirect3DDeviceImpl_7_SetTexture_FPUSetup,
    IDirect3DDeviceImpl_7_GetTextureStageState_FPUSetup,
    IDirect3DDeviceImpl_7_SetTextureStageState_FPUSetup,
    IDirect3DDeviceImpl_7_ValidateDevice_FPUSetup,
    IDirect3DDeviceImpl_7_ApplyStateBlock_FPUSetup,
    IDirect3DDeviceImpl_7_CaptureStateBlock_FPUSetup,
    IDirect3DDeviceImpl_7_DeleteStateBlock_FPUSetup,
    IDirect3DDeviceImpl_7_CreateStateBlock_FPUSetup,
    IDirect3DDeviceImpl_7_Load_FPUSetup,
    IDirect3DDeviceImpl_7_LightEnable_FPUSetup,
    IDirect3DDeviceImpl_7_GetLightEnable_FPUSetup,
    IDirect3DDeviceImpl_7_SetClipPlane_FPUSetup,
    IDirect3DDeviceImpl_7_GetClipPlane_FPUSetup,
    IDirect3DDeviceImpl_7_GetInfo
};

const IDirect3DDevice7Vtbl IDirect3DDevice7_FPUPreserve_Vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DDeviceImpl_7_QueryInterface,
    IDirect3DDeviceImpl_7_AddRef,
    IDirect3DDeviceImpl_7_Release,
    /*** IDirect3DDevice7 ***/
    IDirect3DDeviceImpl_7_GetCaps_FPUPreserve,
    IDirect3DDeviceImpl_7_EnumTextureFormats_FPUPreserve,
    IDirect3DDeviceImpl_7_BeginScene_FPUPreserve,
    IDirect3DDeviceImpl_7_EndScene_FPUPreserve,
    IDirect3DDeviceImpl_7_GetDirect3D,
    IDirect3DDeviceImpl_7_SetRenderTarget_FPUPreserve,
    IDirect3DDeviceImpl_7_GetRenderTarget,
    IDirect3DDeviceImpl_7_Clear_FPUPreserve,
    IDirect3DDeviceImpl_7_SetTransform_FPUPreserve,
    IDirect3DDeviceImpl_7_GetTransform_FPUPreserve,
    IDirect3DDeviceImpl_7_SetViewport_FPUPreserve,
    IDirect3DDeviceImpl_7_MultiplyTransform_FPUPreserve,
    IDirect3DDeviceImpl_7_GetViewport_FPUPreserve,
    IDirect3DDeviceImpl_7_SetMaterial_FPUPreserve,
    IDirect3DDeviceImpl_7_GetMaterial_FPUPreserve,
    IDirect3DDeviceImpl_7_SetLight_FPUPreserve,
    IDirect3DDeviceImpl_7_GetLight_FPUPreserve,
    IDirect3DDeviceImpl_7_SetRenderState_FPUPreserve,
    IDirect3DDeviceImpl_7_GetRenderState_FPUPreserve,
    IDirect3DDeviceImpl_7_BeginStateBlock_FPUPreserve,
    IDirect3DDeviceImpl_7_EndStateBlock_FPUPreserve,
    IDirect3DDeviceImpl_7_PreLoad_FPUPreserve,
    IDirect3DDeviceImpl_7_DrawPrimitive_FPUPreserve,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitive_FPUPreserve,
    IDirect3DDeviceImpl_7_SetClipStatus,
    IDirect3DDeviceImpl_7_GetClipStatus,
    IDirect3DDeviceImpl_7_DrawPrimitiveStrided_FPUPreserve,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitiveStrided_FPUPreserve,
    IDirect3DDeviceImpl_7_DrawPrimitiveVB_FPUPreserve,
    IDirect3DDeviceImpl_7_DrawIndexedPrimitiveVB_FPUPreserve,
    IDirect3DDeviceImpl_7_ComputeSphereVisibility,
    IDirect3DDeviceImpl_7_GetTexture_FPUPreserve,
    IDirect3DDeviceImpl_7_SetTexture_FPUPreserve,
    IDirect3DDeviceImpl_7_GetTextureStageState_FPUPreserve,
    IDirect3DDeviceImpl_7_SetTextureStageState_FPUPreserve,
    IDirect3DDeviceImpl_7_ValidateDevice_FPUPreserve,
    IDirect3DDeviceImpl_7_ApplyStateBlock_FPUPreserve,
    IDirect3DDeviceImpl_7_CaptureStateBlock_FPUPreserve,
    IDirect3DDeviceImpl_7_DeleteStateBlock_FPUPreserve,
    IDirect3DDeviceImpl_7_CreateStateBlock_FPUPreserve,
    IDirect3DDeviceImpl_7_Load_FPUPreserve,
    IDirect3DDeviceImpl_7_LightEnable_FPUPreserve,
    IDirect3DDeviceImpl_7_GetLightEnable_FPUPreserve,
    IDirect3DDeviceImpl_7_SetClipPlane_FPUPreserve,
    IDirect3DDeviceImpl_7_GetClipPlane_FPUPreserve,
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
    IDirect3DDeviceImpl_3_GetRenderState,
    IDirect3DDeviceImpl_3_SetRenderState,
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
    IDirect3DDeviceImpl_3_SetTexture,
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
    Thunk_IDirect3DDeviceImpl_1_BeginScene,
    Thunk_IDirect3DDeviceImpl_1_EndScene,
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

/*****************************************************************************
 * IDirect3DDeviceImpl_UpdateDepthStencil
 *
 * Checks the current render target for attached depth stencils and sets the
 * WineD3D depth stencil accordingly.
 *
 * Returns:
 *  The depth stencil state to set if creating the device
 *
 *****************************************************************************/
WINED3DZBUFFERTYPE
IDirect3DDeviceImpl_UpdateDepthStencil(IDirect3DDeviceImpl *This)
{
    IDirectDrawSurface7 *depthStencil = NULL;
    IDirectDrawSurfaceImpl *dsi;
    static DDSCAPS2 depthcaps = { DDSCAPS_ZBUFFER, 0, 0, {0} };

    IDirectDrawSurface7_GetAttachedSurface((IDirectDrawSurface7 *)This->target, &depthcaps, &depthStencil);
    if(!depthStencil)
    {
        TRACE("Setting wined3d depth stencil to NULL\n");
        IWineD3DDevice_SetDepthStencilSurface(This->wineD3DDevice,
                                              NULL);
        return WINED3DZB_FALSE;
    }

    dsi = (IDirectDrawSurfaceImpl *)depthStencil;
    TRACE("Setting wined3d depth stencil to %p (wined3d %p)\n", dsi, dsi->WineD3DSurface);
    IWineD3DDevice_SetDepthStencilSurface(This->wineD3DDevice,
                                          dsi->WineD3DSurface);

    IDirectDrawSurface7_Release(depthStencil);
    return WINED3DZB_TRUE;
}
