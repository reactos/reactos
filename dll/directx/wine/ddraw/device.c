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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(refiid), obj);

    /* According to COM docs, if the QueryInterface fails, obj should be set to NULL */
    *obj = NULL;

    if(!refiid)
        return DDERR_INVALIDPARAMS;

    if ( IsEqualGUID( &IID_IUnknown, refiid ) )
    {
        *obj = iface;
    }

    /* Check DirectDraw Interfaces. */
    else if( IsEqualGUID( &IID_IDirectDraw7, refiid ) )
    {
        *obj = &This->ddraw->IDirectDraw7_iface;
        TRACE("(%p) Returning IDirectDraw7 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw4, refiid ) )
    {
        *obj = &This->ddraw->IDirectDraw4_iface;
        TRACE("(%p) Returning IDirectDraw4 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirectDraw2, refiid ) )
    {
        *obj = &This->ddraw->IDirectDraw2_iface;
        TRACE("(%p) Returning IDirectDraw2 interface at %p\n", This, *obj);
    }
    else if( IsEqualGUID( &IID_IDirectDraw, refiid ) )
    {
        *obj = &This->ddraw->IDirectDraw_iface;
        TRACE("(%p) Returning IDirectDraw interface at %p\n", This, *obj);
    }

    /* Direct3D */
    else if ( IsEqualGUID( &IID_IDirect3D  , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D_iface;
        TRACE("(%p) Returning IDirect3D interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D2 , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D2_iface;
        TRACE("(%p) Returning IDirect3D2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D3 , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D3_iface;
        TRACE("(%p) Returning IDirect3D3 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3D7 , refiid ) )
    {
        *obj = &This->ddraw->IDirect3D7_iface;
        TRACE("(%p) Returning IDirect3D7 interface at %p\n", This, *obj);
    }

    /* Direct3DDevice */
    else if ( IsEqualGUID( &IID_IDirect3DDevice  , refiid ) )
    {
        *obj = &This->IDirect3DDevice_iface;
        TRACE("(%p) Returning IDirect3DDevice interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice2  , refiid ) ) {
        *obj = &This->IDirect3DDevice2_iface;
        TRACE("(%p) Returning IDirect3DDevice2 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice3  , refiid ) ) {
        *obj = &This->IDirect3DDevice3_iface;
        TRACE("(%p) Returning IDirect3DDevice3 interface at %p\n", This, *obj);
    }
    else if ( IsEqualGUID( &IID_IDirect3DDevice7  , refiid ) ) {
        *obj = This;
        TRACE("(%p) Returning IDirect3DDevice7 interface at %p\n", This, *obj);
    }

    /* DirectDrawSurface */
    else if (IsEqualGUID(&IID_IDirectDrawSurface, refiid) && This->from_surface)
    {
        *obj = &This->target->IDirectDrawSurface_iface;
        TRACE("Returning IDirectDrawSurface interface %p.\n", *obj);
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

static HRESULT WINAPI IDirect3DDeviceImpl_3_QueryInterface(IDirect3DDevice3 *iface, REFIID riid,
        void **obj)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    return IDirect3DDevice7_QueryInterface(&This->IDirect3DDevice7_iface, riid, obj);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_QueryInterface(IDirect3DDevice2 *iface, REFIID riid,
        void **obj)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obj);

    return IDirect3DDevice7_QueryInterface(&This->IDirect3DDevice7_iface, riid, obj);
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_QueryInterface(IDirect3DDevice *iface, REFIID riid,
        void **obp)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), obp);

    return IDirect3DDevice7_QueryInterface(&This->IDirect3DDevice7_iface, riid, obp);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", This, ref);

    return ref;
}

static ULONG WINAPI IDirect3DDeviceImpl_3_AddRef(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_AddRef(&This->IDirect3DDevice7_iface);
}

static ULONG WINAPI IDirect3DDeviceImpl_2_AddRef(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_AddRef(&This->IDirect3DDevice7_iface);
}

static ULONG WINAPI IDirect3DDeviceImpl_1_AddRef(IDirect3DDevice *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_AddRef(&This->IDirect3DDevice7_iface);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", This, ref);

    /* This method doesn't destroy the WineD3DDevice, because it's still in use for
     * 2D rendering. IDirectDrawSurface7::Release will destroy the WineD3DDevice
     * when the render target is released
     */
    if (ref == 0)
    {
        DWORD i;

        wined3d_mutex_lock();

        /* There is no need to unset any resources here, wined3d will take
         * care of that on Uninit3D(). */

        /* Free the index buffer. */
        wined3d_buffer_decref(This->indexbuffer);

        /* Set the device up to render to the front buffer since the back
         * buffer will vanish soon. */
        wined3d_device_set_render_target(This->wined3d_device, 0,
                This->ddraw->wined3d_frontbuffer, TRUE);

        /* Release the WineD3DDevice. This won't destroy it. */
        if (!wined3d_device_decref(This->wined3d_device))
            ERR("The wined3d device (%p) was destroyed unexpectedly.\n", This->wined3d_device);

        /* The texture handles should be unset by now, but there might be some bits
         * missing in our reference counting(needs test). Do a sanity check. */
        for (i = 0; i < This->handle_table.entry_count; ++i)
        {
            struct ddraw_handle_entry *entry = &This->handle_table.entries[i];

            switch (entry->type)
            {
                case DDRAW_HANDLE_FREE:
                    break;

                case DDRAW_HANDLE_MATERIAL:
                {
                    IDirect3DMaterialImpl *m = entry->object;
                    FIXME("Material handle %#x (%p) not unset properly.\n", i + 1, m);
                    m->Handle = 0;
                    break;
                }

                case DDRAW_HANDLE_MATRIX:
                {
                    /* No FIXME here because this might happen because of sloppy applications. */
                    WARN("Leftover matrix handle %#x (%p), deleting.\n", i + 1, entry->object);
                    IDirect3DDevice_DeleteMatrix(&This->IDirect3DDevice_iface, i + 1);
                    break;
                }

                case DDRAW_HANDLE_STATEBLOCK:
                {
                    /* No FIXME here because this might happen because of sloppy applications. */
                    WARN("Leftover stateblock handle %#x (%p), deleting.\n", i + 1, entry->object);
                    IDirect3DDevice7_DeleteStateBlock(iface, i + 1);
                    break;
                }

                case DDRAW_HANDLE_SURFACE:
                {
                    IDirectDrawSurfaceImpl *surf = entry->object;
                    FIXME("Texture handle %#x (%p) not unset properly.\n", i + 1, surf);
                    surf->Handle = 0;
                    break;
                }

                default:
                    FIXME("Handle %#x (%p) has unknown type %#x.\n", i + 1, entry->object, entry->type);
                    break;
            }
        }

        ddraw_handle_table_destroy(&This->handle_table);

        TRACE("Releasing target %p.\n", This->target);
        /* Release the render target and the WineD3D render target
         * (See IDirect3D7::CreateDevice for more comments on this)
         */
        IDirectDrawSurface7_Release(&This->target->IDirectDrawSurface7_iface);
        TRACE("Target release done\n");

        This->ddraw->d3ddevice = NULL;

        /* Now free the structure */
        HeapFree(GetProcessHeap(), 0, This);
        wined3d_mutex_unlock();
    }

    TRACE("Done\n");
    return ref;
}

static ULONG WINAPI IDirect3DDeviceImpl_3_Release(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_Release(&This->IDirect3DDevice7_iface);
}

static ULONG WINAPI IDirect3DDeviceImpl_2_Release(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_Release(&This->IDirect3DDevice7_iface);
}

static ULONG WINAPI IDirect3DDeviceImpl_1_Release(IDirect3DDevice *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_Release(&This->IDirect3DDevice7_iface);
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
    /* It shouldn't be crucial, but print a FIXME, I'm interested if
     * any game calls it and when. */
    FIXME("iface %p, d3d %p, guid %s, device_desc %p nop!\n",
            iface, Direct3D, debugstr_guid(guid), Desc);

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    D3DDEVICEDESC OldDesc;

    TRACE("iface %p, device_desc %p.\n", iface, Desc);

    if (!Desc)
    {
        WARN("Desc is NULL, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }

    /* Call the same function used by IDirect3D, this saves code */
    return IDirect3DImpl_GetCaps(This->ddraw->wined3d, &OldDesc, Desc);
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

/* There are 3 versions of D3DDEVICEDESC. All 3 share the same name because
 * Microsoft just expanded the existing structure without naming them
 * D3DDEVICEDESC2 and D3DDEVICEDESC3. Which version is used have depends
 * on the version of the DirectX SDK. DirectX 6+ and Wine use the latest
 * one with 252 bytes.
 *
 * All 3 versions are allowed as parameters and only the specified amount of
 * bytes is written.
 *
 * Note that Direct3D7 and earlier are not available in native Win64
 * ddraw.dll builds, so possible size differences between 32 bit and
 * 64 bit are a non-issue.
 */
static inline BOOL check_d3ddevicedesc_size(DWORD size)
{
    if (size == FIELD_OFFSET(D3DDEVICEDESC, dwMinTextureWidth) /* 172 */
            || size == FIELD_OFFSET(D3DDEVICEDESC, dwMaxTextureRepeat) /* 204 */
            || size == sizeof(D3DDEVICEDESC) /* 252 */) return TRUE;
    return FALSE;
}

static HRESULT WINAPI
IDirect3DDeviceImpl_3_GetCaps(IDirect3DDevice3 *iface,
                              D3DDEVICEDESC *HWDesc,
                              D3DDEVICEDESC *HelDesc)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    D3DDEVICEDESC oldDesc;
    D3DDEVICEDESC7 newDesc;
    HRESULT hr;

    TRACE("iface %p, hw_desc %p, hel_desc %p.\n", iface, HWDesc, HelDesc);

    if (!HWDesc)
    {
        WARN("HWDesc is NULL, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }
    if (!check_d3ddevicedesc_size(HWDesc->dwSize))
    {
        WARN("HWDesc->dwSize is %u, returning DDERR_INVALIDPARAMS.\n", HWDesc->dwSize);
        return DDERR_INVALIDPARAMS;
    }
    if (!HelDesc)
    {
        WARN("HelDesc is NULL, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }
    if (!check_d3ddevicedesc_size(HelDesc->dwSize))
    {
        WARN("HelDesc->dwSize is %u, returning DDERR_INVALIDPARAMS.\n", HelDesc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    hr = IDirect3DImpl_GetCaps(This->ddraw->wined3d, &oldDesc, &newDesc);
    if(hr != D3D_OK) return hr;

    DD_STRUCT_COPY_BYSIZE(HWDesc, &oldDesc);
    DD_STRUCT_COPY_BYSIZE(HelDesc, &oldDesc);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetCaps(IDirect3DDevice2 *iface,
        D3DDEVICEDESC *D3DHWDevDesc, D3DDEVICEDESC *D3DHELDevDesc)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p, hw_desc %p, hel_desc %p.\n", iface, D3DHWDevDesc, D3DHELDevDesc);
    return IDirect3DDevice3_GetCaps(&This->IDirect3DDevice3_iface, D3DHWDevDesc, D3DHELDevDesc);
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_GetCaps(IDirect3DDevice *iface,
        D3DDEVICEDESC *D3DHWDevDesc, D3DDEVICEDESC *D3DHELDevDesc)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    TRACE("iface %p, hw_desc %p, hel_desc %p.\n", iface, D3DHWDevDesc, D3DHELDevDesc);
    return IDirect3DDevice3_GetCaps(&This->IDirect3DDevice3_iface, D3DHWDevDesc, D3DHELDevDesc);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    IDirectDrawSurfaceImpl *surf1 = unsafe_impl_from_IDirect3DTexture2(Tex1);
    IDirectDrawSurfaceImpl *surf2 = unsafe_impl_from_IDirect3DTexture2(Tex2);
    DWORD h1, h2;

    TRACE("iface %p, tex1 %p, tex2 %p.\n", iface, Tex1, Tex2);

    wined3d_mutex_lock();

    h1 = surf1->Handle - 1;
    h2 = surf2->Handle - 1;
    This->handle_table.entries[h1].object = surf2;
    This->handle_table.entries[h2].object = surf1;
    surf2->Handle = h1 + 1;
    surf1->Handle = h2 + 1;

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_SwapTextureHandles(IDirect3DDevice *iface,
        IDirect3DTexture *D3DTex1, IDirect3DTexture *D3DTex2)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    IDirectDrawSurfaceImpl *surf1 = unsafe_impl_from_IDirect3DTexture(D3DTex1);
    IDirectDrawSurfaceImpl *surf2 = unsafe_impl_from_IDirect3DTexture(D3DTex2);
    IDirect3DTexture2 *t1 = surf1 ? &surf1->IDirect3DTexture2_iface : NULL;
    IDirect3DTexture2 *t2 = surf2 ? &surf2->IDirect3DTexture2_iface : NULL;

    TRACE("iface %p, tex1 %p, tex2 %p.\n", iface, D3DTex1, D3DTex2);

    return IDirect3DDevice2_SwapTextureHandles(&This->IDirect3DDevice2_iface, t1, t2);
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
    FIXME("iface %p, stats %p stub!\n", iface, Stats);

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

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetStats(IDirect3DDevice2 *iface, D3DSTATS *Stats)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, stats %p.\n", iface, Stats);

    return IDirect3DDevice3_GetStats(&This->IDirect3DDevice3_iface, Stats);
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_GetStats(IDirect3DDevice *iface, D3DSTATS *Stats)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, stats %p.\n", iface, Stats);

    return IDirect3DDevice3_GetStats(&This->IDirect3DDevice3_iface, Stats);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    IDirect3DExecuteBufferImpl* object;
    HRESULT hr;

    TRACE("iface %p, buffer_desc %p, buffer %p, outer_unknown %p.\n",
            iface, Desc, ExecuteBuffer, UnkOuter);

    if(UnkOuter)
        return CLASS_E_NOAGGREGATION;

    /* Allocate the new Execute Buffer */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DExecuteBufferImpl));
    if(!object)
    {
        ERR("Out of memory when allocating a IDirect3DExecuteBufferImpl structure\n");
        return DDERR_OUTOFMEMORY;
    }

    hr = d3d_execute_buffer_init(object, This, Desc);
    if (FAILED(hr))
    {
        WARN("Failed to initialize execute buffer, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *ExecuteBuffer = &object->IDirect3DExecuteBuffer_iface;

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
static HRESULT WINAPI IDirect3DDeviceImpl_1_Execute(IDirect3DDevice *iface,
        IDirect3DExecuteBuffer *ExecuteBuffer, IDirect3DViewport *Viewport, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    IDirect3DExecuteBufferImpl *buffer = unsafe_impl_from_IDirect3DExecuteBuffer(ExecuteBuffer);
    IDirect3DViewportImpl *Direct3DViewportImpl = unsafe_impl_from_IDirect3DViewport(Viewport);
    HRESULT hr;

    TRACE("iface %p, buffer %p, viewport %p, flags %#x.\n", iface, ExecuteBuffer, Viewport, Flags);

    if(!buffer)
        return DDERR_INVALIDPARAMS;

    /* Execute... */
    wined3d_mutex_lock();
    hr = d3d_execute_buffer_execute(buffer, This, Direct3DViewportImpl);
    wined3d_mutex_unlock();

    return hr;
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport3(Viewport);

    TRACE("iface %p, viewport %p.\n", iface, Viewport);

    /* Sanity check */
    if(!vp)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    list_add_head(&This->viewport_list, &vp->entry);
    vp->active_device = This; /* Viewport must be usable for Clear() after AddViewport,
                                    so set active_device here. */
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_AddViewport(IDirect3DDevice2 *iface,
        IDirect3DViewport2 *Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport2(Direct3DViewport2);

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport2);

    return IDirect3DDevice3_AddViewport(&This->IDirect3DDevice3_iface, &vp->IDirect3DViewport3_iface);
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_AddViewport(IDirect3DDevice *iface,
        IDirect3DViewport *Direct3DViewport)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport(Direct3DViewport);

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport);

    return IDirect3DDevice3_AddViewport(&This->IDirect3DDevice3_iface, &vp->IDirect3DViewport3_iface);
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
static HRESULT WINAPI IDirect3DDeviceImpl_3_DeleteViewport(IDirect3DDevice3 *iface, IDirect3DViewport3 *viewport)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport3(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    wined3d_mutex_lock();

    if (vp->active_device != This)
    {
        WARN("Viewport %p active device is %p.\n", vp, vp->active_device);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    vp->active_device = NULL;
    list_remove(&vp->entry);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_DeleteViewport(IDirect3DDevice2 *iface,
        IDirect3DViewport2 *Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport2(Direct3DViewport2);

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport2);

    return IDirect3DDevice3_DeleteViewport(&This->IDirect3DDevice3_iface, &vp->IDirect3DViewport3_iface);
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_DeleteViewport(IDirect3DDevice *iface,
        IDirect3DViewport *Direct3DViewport)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport(Direct3DViewport);

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport);

    return IDirect3DDevice3_DeleteViewport(&This->IDirect3DDevice3_iface, &vp->IDirect3DViewport3_iface);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport3(Viewport3);
    IDirect3DViewportImpl *next;
    struct list *entry;

    TRACE("iface %p, viewport %p, next %p, flags %#x.\n",
            iface, Viewport3, lplpDirect3DViewport3, Flags);

    if(!vp)
    {
        *lplpDirect3DViewport3 = NULL;
        return DDERR_INVALIDPARAMS;
    }


    wined3d_mutex_lock();
    switch (Flags)
    {
        case D3DNEXT_NEXT:
            entry = list_next(&This->viewport_list, &vp->entry);
            break;

        case D3DNEXT_HEAD:
            entry = list_head(&This->viewport_list);
            break;

        case D3DNEXT_TAIL:
            entry = list_tail(&This->viewport_list);
            break;

        default:
            WARN("Invalid flags %#x.\n", Flags);
            *lplpDirect3DViewport3 = NULL;
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
    }

    if (entry)
    {
        next = LIST_ENTRY(entry, IDirect3DViewportImpl, entry);
        *lplpDirect3DViewport3 = &next->IDirect3DViewport3_iface;
    }
    else
        *lplpDirect3DViewport3 = NULL;

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_NextViewport(IDirect3DDevice2 *iface,
        IDirect3DViewport2 *Viewport2, IDirect3DViewport2 **lplpDirect3DViewport2, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport2(Viewport2);
    IDirect3DViewport3 *res;
    HRESULT hr;

    TRACE("iface %p, viewport %p, next %p, flags %#x.\n",
            iface, Viewport2, lplpDirect3DViewport2, Flags);

    hr = IDirect3DDevice3_NextViewport(&This->IDirect3DDevice3_iface,
            &vp->IDirect3DViewport3_iface, &res, Flags);
    *lplpDirect3DViewport2 = (IDirect3DViewport2 *)res;
    return hr;
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_NextViewport(IDirect3DDevice *iface,
        IDirect3DViewport *Viewport, IDirect3DViewport **lplpDirect3DViewport, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport(Viewport);
    IDirect3DViewport3 *res;
    HRESULT hr;

    TRACE("iface %p, viewport %p, next %p, flags %#x.\n",
            iface, Viewport, lplpDirect3DViewport, Flags);

    hr = IDirect3DDevice3_NextViewport(&This->IDirect3DDevice3_iface,
            &vp->IDirect3DViewport3_iface, &res, Flags);
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
    FIXME("iface %p, buffer %p, viewport %p, flags %#x, rect %s stub!\n",
            iface, ExecuteBuffer, Viewport, Flags, wine_dbgstr_rect((RECT *)Rect));

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
    FIXME("iface %p, count %p, records %p stub!\n", iface, Count, D3DPickRec);

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_display_mode mode;
    HRESULT hr;
    unsigned int i;

    static const enum wined3d_format_id FormatList[] =
    {
        /* 16 bit */
        WINED3DFMT_B5G5R5X1_UNORM,
        WINED3DFMT_B5G5R5A1_UNORM,
        WINED3DFMT_B4G4R4A4_UNORM,
        WINED3DFMT_B5G6R5_UNORM,
        /* 32 bit */
        WINED3DFMT_B8G8R8X8_UNORM,
        WINED3DFMT_B8G8R8A8_UNORM,
        /* 8 bit */
        WINED3DFMT_B2G3R3_UNORM,
        WINED3DFMT_P8_UINT,
        /* FOURCC codes */
        WINED3DFMT_DXT1,
        WINED3DFMT_DXT3,
        WINED3DFMT_DXT5,
    };

    static const enum wined3d_format_id BumpFormatList[] =
    {
        WINED3DFMT_R8G8_SNORM,
        WINED3DFMT_R5G5_SNORM_L6_UNORM,
        WINED3DFMT_R8G8_SNORM_L8X8_UNORM,
        WINED3DFMT_R16G16_SNORM,
        WINED3DFMT_R10G11B11_SNORM,
        WINED3DFMT_R10G10B10_SNORM_A2_UNORM
    };

    TRACE("iface %p, callback %p, context %p.\n", iface, Callback, Arg);

    if(!Callback)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    memset(&mode, 0, sizeof(mode));
    hr = wined3d_device_get_display_mode(This->ddraw->wined3d_device, 0, &mode);
    if (FAILED(hr))
    {
        wined3d_mutex_unlock();
        WARN("Cannot get the current adapter format\n");
        return hr;
    }

    for (i = 0; i < sizeof(FormatList) / sizeof(*FormatList); ++i)
    {
        hr = wined3d_check_device_format(This->ddraw->wined3d, WINED3DADAPTER_DEFAULT, WINED3D_DEVICE_TYPE_HAL,
                mode.format_id, 0, WINED3D_RTYPE_TEXTURE, FormatList[i], SURFACE_OPENGL);
        if (hr == D3D_OK)
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
                wined3d_mutex_unlock();
                return D3D_OK;
            }
        }
    }

    for (i = 0; i < sizeof(BumpFormatList) / sizeof(*BumpFormatList); ++i)
    {
        hr = wined3d_check_device_format(This->ddraw->wined3d, WINED3DADAPTER_DEFAULT,
                WINED3D_DEVICE_TYPE_HAL, mode.format_id, WINED3DUSAGE_QUERY_LEGACYBUMPMAP,
                WINED3D_RTYPE_TEXTURE, BumpFormatList[i], SURFACE_OPENGL);
        if (hr == D3D_OK)
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
                wined3d_mutex_unlock();
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration\n");
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_EnumTextureFormats(IDirect3DDevice3 *iface,
        LPD3DENUMPIXELFORMATSCALLBACK Callback, void *Arg)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, Callback, Arg);

    return IDirect3DDevice7_EnumTextureFormats(&This->IDirect3DDevice7_iface, Callback, Arg);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    struct wined3d_display_mode mode;
    HRESULT hr;
    unsigned int i;

    static const enum wined3d_format_id FormatList[] =
    {
        /* 16 bit */
        WINED3DFMT_B5G5R5X1_UNORM,
        WINED3DFMT_B5G5R5A1_UNORM,
        WINED3DFMT_B4G4R4A4_UNORM,
        WINED3DFMT_B5G6R5_UNORM,
        /* 32 bit */
        WINED3DFMT_B8G8R8X8_UNORM,
        WINED3DFMT_B8G8R8A8_UNORM,
        /* 8 bit */
        WINED3DFMT_B2G3R3_UNORM,
        WINED3DFMT_P8_UINT,
        /* FOURCC codes - Not in this version*/
    };

    TRACE("iface %p, callback %p, context %p.\n", iface, Callback, Arg);

    if(!Callback)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    memset(&mode, 0, sizeof(mode));
    hr = wined3d_device_get_display_mode(This->ddraw->wined3d_device, 0, &mode);
    if (FAILED(hr))
    {
        wined3d_mutex_unlock();
        WARN("Cannot get the current adapter format\n");
        return hr;
    }

    for (i = 0; i < sizeof(FormatList) / sizeof(*FormatList); ++i)
    {
        hr = wined3d_check_device_format(This->ddraw->wined3d, 0, WINED3D_DEVICE_TYPE_HAL,
                mode.format_id, 0, WINED3D_RTYPE_TEXTURE, FormatList[i], SURFACE_OPENGL);
        if (hr == D3D_OK)
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
                wined3d_mutex_unlock();
                return D3D_OK;
            }
        }
    }
    TRACE("End of enumeration\n");
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_EnumTextureFormats(IDirect3DDevice *iface,
        LPD3DENUMTEXTUREFORMATSCALLBACK Callback, void *Arg)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, Callback, Arg);

    return IDirect3DDevice2_EnumTextureFormats(&This->IDirect3DDevice2_iface, Callback, Arg);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    D3DMATRIX *Matrix;
    DWORD h;

    TRACE("iface %p, matrix_handle %p.\n", iface, D3DMatHandle);

    if(!D3DMatHandle)
        return DDERR_INVALIDPARAMS;

    Matrix = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3DMATRIX));
    if(!Matrix)
    {
        ERR("Out of memory when allocating a D3DMATRIX\n");
        return DDERR_OUTOFMEMORY;
    }

    wined3d_mutex_lock();

    h = ddraw_allocate_handle(&This->handle_table, Matrix, DDRAW_HANDLE_MATRIX);
    if (h == DDRAW_INVALID_HANDLE)
    {
        ERR("Failed to allocate a matrix handle.\n");
        HeapFree(GetProcessHeap(), 0, Matrix);
        wined3d_mutex_unlock();
        return DDERR_OUTOFMEMORY;
    }

    *D3DMatHandle = h + 1;

    TRACE(" returning matrix handle %d\n", *D3DMatHandle);

    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    D3DMATRIX *m;

    TRACE("iface %p, matrix_handle %#x, matrix %p.\n", iface, D3DMatHandle, D3DMatrix);

    if (!D3DMatrix) return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    m = ddraw_get_object(&This->handle_table, D3DMatHandle - 1, DDRAW_HANDLE_MATRIX);
    if (!m)
    {
        WARN("Invalid matrix handle.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (TRACE_ON(ddraw))
        dump_D3DMATRIX(D3DMatrix);

    *m = *D3DMatrix;

    if (D3DMatHandle == This->world)
        wined3d_device_set_transform(This->wined3d_device,
                WINED3D_TS_WORLD_MATRIX(0), (struct wined3d_matrix *)D3DMatrix);

    if (D3DMatHandle == This->view)
        wined3d_device_set_transform(This->wined3d_device,
                WINED3D_TS_VIEW, (struct wined3d_matrix *)D3DMatrix);

    if (D3DMatHandle == This->proj)
        wined3d_device_set_transform(This->wined3d_device,
                WINED3D_TS_PROJECTION, (struct wined3d_matrix *)D3DMatrix);

    wined3d_mutex_unlock();

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice::GetMatrix
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    D3DMATRIX *m;

    TRACE("iface %p, matrix_handle %#x, matrix %p.\n", iface, D3DMatHandle, D3DMatrix);

    if (!D3DMatrix) return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    m = ddraw_get_object(&This->handle_table, D3DMatHandle - 1, DDRAW_HANDLE_MATRIX);
    if (!m)
    {
        WARN("Invalid matrix handle.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    *D3DMatrix = *m;

    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    D3DMATRIX *m;

    TRACE("iface %p, matrix_handle %#x.\n", iface, D3DMatHandle);

    wined3d_mutex_lock();

    m = ddraw_free_handle(&This->handle_table, D3DMatHandle - 1, DDRAW_HANDLE_MATRIX);
    if (!m)
    {
        WARN("Invalid matrix handle.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_unlock();

    HeapFree(GetProcessHeap(), 0, m);

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_begin_scene(This->wined3d_device);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_BeginScene(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_BeginScene(&This->IDirect3DDevice7_iface);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_BeginScene(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_BeginScene(&This->IDirect3DDevice7_iface);
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_BeginScene(IDirect3DDevice *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_BeginScene(&This->IDirect3DDevice7_iface);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_end_scene(This->wined3d_device);
    wined3d_mutex_unlock();

    if(hr == WINED3D_OK) return D3D_OK;
    else return D3DERR_SCENE_NOT_IN_SCENE;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH
IDirect3DDeviceImpl_7_EndScene_FPUSetup(IDirect3DDevice7 *iface)
{
    return IDirect3DDeviceImpl_7_EndScene(iface);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH
IDirect3DDeviceImpl_7_EndScene_FPUPreserve(IDirect3DDevice7 *iface)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = IDirect3DDeviceImpl_7_EndScene(iface);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDeviceImpl_3_EndScene(IDirect3DDevice3 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_EndScene(&This->IDirect3DDevice7_iface);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDeviceImpl_2_EndScene(IDirect3DDevice2 *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_EndScene(&This->IDirect3DDevice7_iface);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDeviceImpl_1_EndScene(IDirect3DDevice *iface)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);
    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_EndScene(&This->IDirect3DDevice7_iface);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, d3d %p.\n", iface, Direct3D7);

    if(!Direct3D7)
        return DDERR_INVALIDPARAMS;

    *Direct3D7 = &This->ddraw->IDirect3D7_iface;
    IDirect3D7_AddRef(*Direct3D7);

    TRACE(" returning interface %p\n", *Direct3D7);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_3_GetDirect3D(IDirect3DDevice3 *iface,
        IDirect3D3 **Direct3D3)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, d3d %p.\n", iface, Direct3D3);

    if(!Direct3D3)
        return DDERR_INVALIDPARAMS;

    IDirect3D3_AddRef(&This->ddraw->IDirect3D3_iface);
    *Direct3D3 = &This->ddraw->IDirect3D3_iface;
    TRACE(" returning interface %p\n", *Direct3D3);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetDirect3D(IDirect3DDevice2 *iface,
        IDirect3D2 **Direct3D2)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, d3d %p.\n", iface, Direct3D2);

    if(!Direct3D2)
        return DDERR_INVALIDPARAMS;

    IDirect3D2_AddRef(&This->ddraw->IDirect3D2_iface);
    *Direct3D2 = &This->ddraw->IDirect3D2_iface;
    TRACE(" returning interface %p\n", *Direct3D2);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_1_GetDirect3D(IDirect3DDevice *iface,
        IDirect3D **Direct3D)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, d3d %p.\n", iface, Direct3D);

    if(!Direct3D)
        return DDERR_INVALIDPARAMS;

    IDirect3D_AddRef(&This->ddraw->IDirect3D_iface);
    *Direct3D = &This->ddraw->IDirect3D_iface;
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport3(Direct3DViewport3);

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport3);

    wined3d_mutex_lock();
    /* Do nothing if the specified viewport is the same as the current one */
    if (This->current_viewport == vp )
    {
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    if (vp->active_device != This)
    {
        WARN("Viewport %p active device is %p.\n", vp, vp->active_device);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    /* Release previous viewport and AddRef the new one */
    if (This->current_viewport)
    {
        TRACE("ViewportImpl is at %p, interface is at %p\n", This->current_viewport,
                &This->current_viewport->IDirect3DViewport3_iface);
        IDirect3DViewport3_Release(&This->current_viewport->IDirect3DViewport3_iface);
    }
    IDirect3DViewport3_AddRef(Direct3DViewport3);

    /* Set this viewport as the current viewport */
    This->current_viewport = vp;

    /* Activate this viewport */
    viewport_activate(This->current_viewport, FALSE);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_SetCurrentViewport(IDirect3DDevice2 *iface,
        IDirect3DViewport2 *Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    IDirect3DViewportImpl *vp = unsafe_impl_from_IDirect3DViewport2(Direct3DViewport2);

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport2);

    return IDirect3DDevice3_SetCurrentViewport(&This->IDirect3DDevice3_iface,
            &vp->IDirect3DViewport3_iface);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport3);

    if(!Direct3DViewport3)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    *Direct3DViewport3 = &This->current_viewport->IDirect3DViewport3_iface;

    /* AddRef the returned viewport */
    if(*Direct3DViewport3) IDirect3DViewport3_AddRef(*Direct3DViewport3);

    TRACE(" returning interface %p\n", *Direct3DViewport3);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetCurrentViewport(IDirect3DDevice2 *iface,
        IDirect3DViewport2 **Direct3DViewport2)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, Direct3DViewport2);

    hr = IDirect3DDevice3_GetCurrentViewport(&This->IDirect3DDevice3_iface,
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
static HRESULT d3d_device_set_render_target(IDirect3DDeviceImpl *This, IDirectDrawSurfaceImpl *Target)
{
    HRESULT hr;

    wined3d_mutex_lock();

    if(This->target == Target)
    {
        TRACE("No-op SetRenderTarget operation, not doing anything\n");
        wined3d_mutex_unlock();
        return D3D_OK;
    }
    This->target = Target;
    hr = wined3d_device_set_render_target(This->wined3d_device, 0,
            Target ? Target->wined3d_surface : NULL, FALSE);
    if(hr != D3D_OK)
    {
        wined3d_mutex_unlock();
        return hr;
    }
    IDirect3DDeviceImpl_UpdateDepthStencil(This);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT
IDirect3DDeviceImpl_7_SetRenderTarget(IDirect3DDevice7 *iface,
                                      IDirectDrawSurface7 *NewTarget,
                                      DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    IDirectDrawSurfaceImpl *Target = unsafe_impl_from_IDirectDrawSurface7(NewTarget);

    TRACE("iface %p, target %p, flags %#x.\n", iface, NewTarget, Flags);
    /* Flags: Not used */

    IDirectDrawSurface7_AddRef(NewTarget);
    IDirectDrawSurface7_Release(&This->target->IDirectDrawSurface7_iface);
    return d3d_device_set_render_target(This, Target);
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

static HRESULT WINAPI IDirect3DDeviceImpl_3_SetRenderTarget(IDirect3DDevice3 *iface,
        IDirectDrawSurface4 *NewRenderTarget, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirectDrawSurfaceImpl *Target = unsafe_impl_from_IDirectDrawSurface4(NewRenderTarget);

    TRACE("iface %p, target %p, flags %#x.\n", iface, NewRenderTarget, Flags);

    IDirectDrawSurface4_AddRef(NewRenderTarget);
    IDirectDrawSurface4_Release(&This->target->IDirectDrawSurface4_iface);
    return d3d_device_set_render_target(This, Target);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_SetRenderTarget(IDirect3DDevice2 *iface,
        IDirectDrawSurface *NewRenderTarget, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    IDirectDrawSurfaceImpl *Target = unsafe_impl_from_IDirectDrawSurface(NewRenderTarget);

    TRACE("iface %p, target %p, flags %#x.\n", iface, NewRenderTarget, Flags);

    IDirectDrawSurface_AddRef(NewRenderTarget);
    IDirectDrawSurface_Release(&This->target->IDirectDrawSurface_iface);
    return d3d_device_set_render_target(This, Target);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, target %p.\n", iface, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    *RenderTarget = &This->target->IDirectDrawSurface7_iface;
    IDirectDrawSurface7_AddRef(*RenderTarget);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_3_GetRenderTarget(IDirect3DDevice3 *iface,
        IDirectDrawSurface4 **RenderTarget)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirectDrawSurface7 *RenderTarget7;
    IDirectDrawSurfaceImpl *RenderTargetImpl;
    HRESULT hr;

    TRACE("iface %p, target %p.\n", iface, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    hr = IDirect3DDevice7_GetRenderTarget(&This->IDirect3DDevice7_iface, &RenderTarget7);
    if(hr != D3D_OK) return hr;
    RenderTargetImpl = impl_from_IDirectDrawSurface7(RenderTarget7);
    *RenderTarget = &RenderTargetImpl->IDirectDrawSurface4_iface;
    IDirectDrawSurface4_AddRef(*RenderTarget);
    IDirectDrawSurface7_Release(RenderTarget7);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetRenderTarget(IDirect3DDevice2 *iface,
        IDirectDrawSurface **RenderTarget)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    IDirectDrawSurface7 *RenderTarget7;
    IDirectDrawSurfaceImpl *RenderTargetImpl;
    HRESULT hr;

    TRACE("iface %p, target %p.\n", iface, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    hr = IDirect3DDevice7_GetRenderTarget(&This->IDirect3DDevice7_iface, &RenderTarget7);
    if(hr != D3D_OK) return hr;
    RenderTargetImpl = impl_from_IDirectDrawSurface7(RenderTarget7);
    *RenderTarget = &RenderTargetImpl->IDirectDrawSurface_iface;
    IDirectDrawSurface_AddRef(*RenderTarget);
    IDirectDrawSurface7_Release(RenderTarget7);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, FVF %#x, flags %#x.\n",
            iface, PrimitiveType, VertexTypeDesc, Flags);

    wined3d_mutex_lock();
    This->primitive_type = PrimitiveType;
    This->vertex_type = VertexTypeDesc;
    This->render_flags = Flags;
    This->vertex_size = get_flexible_vertex_size(This->vertex_type);
    This->nb_vertices = 0;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_Begin(IDirect3DDevice2 *iface, D3DPRIMITIVETYPE d3dpt,
        D3DVERTEXTYPE dwVertexTypeDesc, DWORD dwFlags)
{
    DWORD FVF;
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, flags %#x.\n",
            iface, d3dpt, dwVertexTypeDesc, dwFlags);

    switch(dwVertexTypeDesc)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", dwVertexTypeDesc);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    };

    return IDirect3DDevice3_Begin(&This->IDirect3DDevice3_iface, d3dpt, FVF, dwFlags);
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
    FIXME("iface %p, primitive_type %#x, FVF %#x, vertices %p, vertex_count %u, flags %#x stub!\n",
            iface, PrimitiveType, VertexType, Vertices, NumVertices, Flags);

    return D3D_OK;
}


static HRESULT WINAPI IDirect3DDeviceImpl_2_BeginIndexed(IDirect3DDevice2 *iface,
        D3DPRIMITIVETYPE d3dptPrimitiveType, D3DVERTEXTYPE d3dvtVertexType,
        void *lpvVertices, DWORD dwNumVertices, DWORD dwFlags)
{
    DWORD FVF;
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, vertices %p, vertex_count %u, flags %#x stub!\n",
            iface, d3dptPrimitiveType, d3dvtVertexType, lpvVertices, dwNumVertices, dwFlags);

    switch(d3dvtVertexType)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", d3dvtVertexType);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    };

    return IDirect3DDevice3_BeginIndexed(&This->IDirect3DDevice3_iface,
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, vertex %p.\n", iface, Vertex);

    if(!Vertex)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
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
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_Vertex(IDirect3DDevice2 *iface, void *lpVertexType)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, vertex %p.\n", iface, lpVertexType);

    return IDirect3DDevice3_Vertex(&This->IDirect3DDevice3_iface, lpVertexType);
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
    FIXME("iface %p, index %#x stub!\n", iface, VertexIndex);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_Index(IDirect3DDevice2 *iface, WORD wVertexIndex)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, index %#x.\n", iface, wVertexIndex);

    return IDirect3DDevice3_Index(&This->IDirect3DDevice3_iface, wVertexIndex);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, flags %#x.\n", iface, Flags);

    return IDirect3DDevice7_DrawPrimitive(&This->IDirect3DDevice7_iface, This->primitive_type,
            This->vertex_type, This->vertex_buffer, This->nb_vertices, This->render_flags);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_End(IDirect3DDevice2 *iface, DWORD dwFlags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, flags %#x.\n", iface, dwFlags);

    return IDirect3DDevice3_End(&This->IDirect3DDevice3_iface, dwFlags);
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
static HRESULT IDirect3DDeviceImpl_7_GetRenderState(IDirect3DDevice7 *iface,
        D3DRENDERSTATETYPE RenderStateType, DWORD *Value)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %p.\n", iface, RenderStateType, Value);

    if(!Value)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    switch(RenderStateType)
    {
        case D3DRENDERSTATE_TEXTUREMAG:
        {
            enum wined3d_texture_filter_type tex_mag;

            hr = wined3d_device_get_sampler_state(This->wined3d_device, 0, WINED3D_SAMP_MAG_FILTER, &tex_mag);

            switch (tex_mag)
            {
                case WINED3D_TEXF_POINT:
                    *Value = D3DFILTER_NEAREST;
                    break;
                case WINED3D_TEXF_LINEAR:
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
            enum wined3d_texture_filter_type tex_min;
            enum wined3d_texture_filter_type tex_mip;

            hr = wined3d_device_get_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_MIN_FILTER, &tex_min);
            if (FAILED(hr))
            {
                wined3d_mutex_unlock();
                return hr;
            }
            hr = wined3d_device_get_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_MIP_FILTER, &tex_mip);

            switch (tex_min)
            {
                case WINED3D_TEXF_POINT:
                    switch (tex_mip)
                    {
                        case WINED3D_TEXF_NONE:
                            *Value = D3DFILTER_NEAREST;
                            break;
                        case WINED3D_TEXF_POINT:
                            *Value = D3DFILTER_MIPNEAREST;
                            break;
                        case WINED3D_TEXF_LINEAR:
                            *Value = D3DFILTER_LINEARMIPNEAREST;
                            break;
                        default:
                            ERR("Unhandled mip filter %#x.\n", tex_mip);
                            *Value = D3DFILTER_NEAREST;
                            break;
                    }
                    break;
                case WINED3D_TEXF_LINEAR:
                    switch (tex_mip)
                    {
                        case WINED3D_TEXF_NONE:
                            *Value = D3DFILTER_LINEAR;
                            break;
                        case WINED3D_TEXF_POINT:
                            *Value = D3DFILTER_MIPLINEAR;
                            break;
                        case WINED3D_TEXF_LINEAR:
                            *Value = D3DFILTER_LINEARMIPLINEAR;
                            break;
                        default:
                            ERR("Unhandled mip filter %#x.\n", tex_mip);
                            *Value = D3DFILTER_LINEAR;
                            break;
                    }
                    break;
                default:
                    ERR("Unhandled texture min filter %#x.\n",tex_min);
                    *Value = D3DFILTER_NEAREST;
                    break;
            }
            break;
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            hr = wined3d_device_get_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_ADDRESS_U, Value);
            break;
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            hr = wined3d_device_get_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_ADDRESS_V, Value);
            break;

        case D3DRENDERSTATE_BORDERCOLOR:
            FIXME("Unhandled render state D3DRENDERSTATE_BORDERCOLOR.\n");
            hr = E_NOTIMPL;
            break;

        case D3DRENDERSTATE_TEXTUREHANDLE:
        case D3DRENDERSTATE_TEXTUREMAPBLEND:
            WARN("Render state %#x is invalid in d3d7.\n", RenderStateType);
            hr = DDERR_INVALIDPARAMS;
            break;

        case D3DRENDERSTATE_ZBIAS:
            hr = wined3d_device_get_render_state(This->wined3d_device, WINED3D_RS_DEPTHBIAS, Value);
            break;

        default:
            if (RenderStateType >= D3DRENDERSTATE_STIPPLEPATTERN00
                    && RenderStateType <= D3DRENDERSTATE_STIPPLEPATTERN31)
            {
                FIXME("Unhandled stipple pattern render state (%#x).\n",
                        RenderStateType);
                hr = E_NOTIMPL;
                break;
            }
            hr = wined3d_device_get_render_state(This->wined3d_device, RenderStateType, Value);
    }
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %p.\n", iface, dwRenderStateType, lpdwRenderState);

    switch(dwRenderStateType)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            /* This state is wrapped to SetTexture in SetRenderState, so
             * it has to be wrapped to GetTexture here. */
            struct wined3d_texture *tex = NULL;
            *lpdwRenderState = 0;

            wined3d_mutex_lock();
            hr = wined3d_device_get_texture(This->wined3d_device, 0, &tex);
            if (SUCCEEDED(hr) && tex)
            {
                /* The parent of the texture is the IDirectDrawSurface7
                 * interface of the ddraw surface. */
                IDirectDrawSurfaceImpl *parent = wined3d_texture_get_parent(tex);
                if (parent) *lpdwRenderState = parent->Handle;
                wined3d_texture_decref(tex);
            }
            wined3d_mutex_unlock();

            return hr;
        }

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
        {
            /* D3DRENDERSTATE_TEXTUREMAPBLEND is mapped to texture state stages in SetRenderState; reverse
               the mapping to get the value. */
            DWORD colorop, colorarg1, colorarg2;
            DWORD alphaop, alphaarg1, alphaarg2;

            wined3d_mutex_lock();

            This->legacyTextureBlending = TRUE;

            wined3d_device_get_texture_stage_state(This->wined3d_device, 0, WINED3D_TSS_COLOR_OP, &colorop);
            wined3d_device_get_texture_stage_state(This->wined3d_device, 0, WINED3D_TSS_COLOR_ARG1, &colorarg1);
            wined3d_device_get_texture_stage_state(This->wined3d_device, 0, WINED3D_TSS_COLOR_ARG2, &colorarg2);
            wined3d_device_get_texture_stage_state(This->wined3d_device, 0, WINED3D_TSS_ALPHA_OP, &alphaop);
            wined3d_device_get_texture_stage_state(This->wined3d_device, 0, WINED3D_TSS_ALPHA_ARG1, &alphaarg1);
            wined3d_device_get_texture_stage_state(This->wined3d_device, 0, WINED3D_TSS_ALPHA_ARG2, &alphaarg2);

            if (colorop == WINED3D_TOP_SELECT_ARG1 && colorarg1 == WINED3DTA_TEXTURE
                    && alphaop == WINED3D_TOP_SELECT_ARG1 && alphaarg1 == WINED3DTA_TEXTURE)
                *lpdwRenderState = D3DTBLEND_DECAL;
            else if (colorop == WINED3D_TOP_SELECT_ARG1 && colorarg1 == WINED3DTA_TEXTURE
                    && alphaop == WINED3D_TOP_MODULATE
                    && alphaarg1 == WINED3DTA_TEXTURE && alphaarg2 == WINED3DTA_CURRENT)
                *lpdwRenderState = D3DTBLEND_DECALALPHA;
            else if (colorop == WINED3D_TOP_MODULATE
                    && colorarg1 == WINED3DTA_TEXTURE && colorarg2 == WINED3DTA_CURRENT
                    && alphaop == WINED3D_TOP_MODULATE
                    && alphaarg1 == WINED3DTA_TEXTURE && alphaarg2 == WINED3DTA_CURRENT)
                *lpdwRenderState = D3DTBLEND_MODULATEALPHA;
            else
            {
                struct wined3d_texture *tex = NULL;
                HRESULT hr;
                BOOL tex_alpha = FALSE;
                DDPIXELFORMAT ddfmt;

                hr = wined3d_device_get_texture(This->wined3d_device, 0, &tex);

                if(hr == WINED3D_OK && tex)
                {
                    struct wined3d_resource *sub_resource;

                    if ((sub_resource = wined3d_texture_get_sub_resource(tex, 0)))
                    {
                        struct wined3d_resource_desc desc;

                        wined3d_resource_get_desc(sub_resource, &desc);
                        ddfmt.dwSize = sizeof(ddfmt);
                        PixelFormat_WineD3DtoDD(&ddfmt, desc.format);
                        if (ddfmt.u5.dwRGBAlphaBitMask) tex_alpha = TRUE;
                    }

                    wined3d_texture_decref(tex);
                }

                if (!(colorop == WINED3D_TOP_MODULATE
                        && colorarg1 == WINED3DTA_TEXTURE && colorarg2 == WINED3DTA_CURRENT
                        && alphaop == (tex_alpha ? WINED3D_TOP_SELECT_ARG1 : WINED3D_TOP_SELECT_ARG2)
                        && alphaarg1 == WINED3DTA_TEXTURE && alphaarg2 == WINED3DTA_CURRENT))
                    ERR("Unexpected texture stage state setup, returning D3DTBLEND_MODULATE - likely erroneous.\n");

                *lpdwRenderState = D3DTBLEND_MODULATE;
            }

            wined3d_mutex_unlock();

            return D3D_OK;
        }

        default:
            return IDirect3DDevice7_GetRenderState(&This->IDirect3DDevice7_iface, dwRenderStateType, lpdwRenderState);
    }
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetRenderState(IDirect3DDevice2 *iface,
        D3DRENDERSTATETYPE dwRenderStateType, DWORD *lpdwRenderState)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %p.\n", iface, dwRenderStateType, lpdwRenderState);

    return IDirect3DDevice3_GetRenderState(&This->IDirect3DDevice3_iface,
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %#x.\n", iface, RenderStateType, Value);

    wined3d_mutex_lock();
    /* Some render states need special care */
    switch(RenderStateType)
    {
        /*
         * The ddraw texture filter mapping works like this:
         *     D3DFILTER_NEAREST            Point min/mag, no mip
         *     D3DFILTER_MIPNEAREST         Point min/mag, point mip
         *     D3DFILTER_LINEARMIPNEAREST:  Point min/mag, linear mip
         *
         *     D3DFILTER_LINEAR             Linear min/mag, no mip
         *     D3DFILTER_MIPLINEAR          Linear min/mag, point mip
         *     D3DFILTER_LINEARMIPLINEAR    Linear min/mag, linear mip
         *
         * This is the opposite of the GL naming convention,
         * D3DFILTER_LINEARMIPNEAREST corresponds to GL_NEAREST_MIPMAP_LINEAR.
         */
        case D3DRENDERSTATE_TEXTUREMAG:
        {
            enum wined3d_texture_filter_type tex_mag;

            switch (Value)
            {
                case D3DFILTER_NEAREST:
                case D3DFILTER_MIPNEAREST:
                case D3DFILTER_LINEARMIPNEAREST:
                    tex_mag = WINED3D_TEXF_POINT;
                    break;
                case D3DFILTER_LINEAR:
                case D3DFILTER_MIPLINEAR:
                case D3DFILTER_LINEARMIPLINEAR:
                    tex_mag = WINED3D_TEXF_LINEAR;
                    break;
                default:
                    tex_mag = WINED3D_TEXF_POINT;
                    ERR("Unhandled texture mag %d !\n",Value);
                    break;
            }

            hr = wined3d_device_set_sampler_state(This->wined3d_device, 0, WINED3D_SAMP_MAG_FILTER, tex_mag);
            break;
        }

        case D3DRENDERSTATE_TEXTUREMIN:
        {
            enum wined3d_texture_filter_type tex_min;
            enum wined3d_texture_filter_type tex_mip;

            switch ((D3DTEXTUREFILTER)Value)
            {
                case D3DFILTER_NEAREST:
                    tex_min = WINED3D_TEXF_POINT;
                    tex_mip = WINED3D_TEXF_NONE;
                    break;
                case D3DFILTER_LINEAR:
                    tex_min = WINED3D_TEXF_LINEAR;
                    tex_mip = WINED3D_TEXF_NONE;
                    break;
                case D3DFILTER_MIPNEAREST:
                    tex_min = WINED3D_TEXF_POINT;
                    tex_mip = WINED3D_TEXF_POINT;
                    break;
                case D3DFILTER_MIPLINEAR:
                    tex_min = WINED3D_TEXF_LINEAR;
                    tex_mip = WINED3D_TEXF_POINT;
                    break;
                case D3DFILTER_LINEARMIPNEAREST:
                    tex_min = WINED3D_TEXF_POINT;
                    tex_mip = WINED3D_TEXF_LINEAR;
                    break;
                case D3DFILTER_LINEARMIPLINEAR:
                    tex_min = WINED3D_TEXF_LINEAR;
                    tex_mip = WINED3D_TEXF_LINEAR;
                    break;

                default:
                    ERR("Unhandled texture min %d !\n",Value);
                    tex_min = WINED3D_TEXF_POINT;
                    tex_mip = WINED3D_TEXF_NONE;
                    break;
            }

            wined3d_device_set_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_MIP_FILTER, tex_mip);
            hr = wined3d_device_set_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_MIN_FILTER, tex_min);
            break;
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
            wined3d_device_set_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_ADDRESS_V, Value);
            /* Drop through */
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            hr = wined3d_device_set_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_ADDRESS_U, Value);
            break;
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            hr = wined3d_device_set_sampler_state(This->wined3d_device,
                    0, WINED3D_SAMP_ADDRESS_V, Value);
            break;

        case D3DRENDERSTATE_BORDERCOLOR:
            /* This should probably just forward to the corresponding sampler
             * state. Needs tests. */
            FIXME("Unhandled render state D3DRENDERSTATE_BORDERCOLOR.\n");
            hr = E_NOTIMPL;
            break;

        case D3DRENDERSTATE_TEXTUREHANDLE:
        case D3DRENDERSTATE_TEXTUREMAPBLEND:
            WARN("Render state %#x is invalid in d3d7.\n", RenderStateType);
            hr = DDERR_INVALIDPARAMS;
            break;

        case D3DRENDERSTATE_ZBIAS:
            hr = wined3d_device_set_render_state(This->wined3d_device, WINED3D_RS_DEPTHBIAS, Value);
            break;

        default:
            if (RenderStateType >= D3DRENDERSTATE_STIPPLEPATTERN00
                    && RenderStateType <= D3DRENDERSTATE_STIPPLEPATTERN31)
            {
                FIXME("Unhandled stipple pattern render state (%#x).\n",
                        RenderStateType);
                hr = E_NOTIMPL;
                break;
            }

            hr = wined3d_device_set_render_state(This->wined3d_device, RenderStateType, Value);
            break;
    }
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, value %#x.\n", iface, RenderStateType, Value);

    wined3d_mutex_lock();

    switch(RenderStateType)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            IDirectDrawSurfaceImpl *surf;

            if(Value == 0)
            {
                hr = wined3d_device_set_texture(This->wined3d_device, 0, NULL);
                break;
            }

            surf = ddraw_get_object(&This->handle_table, Value - 1, DDRAW_HANDLE_SURFACE);
            if (!surf)
            {
                WARN("Invalid texture handle.\n");
                hr = DDERR_INVALIDPARAMS;
                break;
            }

            hr = IDirect3DDevice3_SetTexture(iface, 0, &surf->IDirect3DTexture2_iface);
            break;
        }

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
        {
            This->legacyTextureBlending = TRUE;

            switch ( (D3DTEXTUREBLEND) Value)
            {
                case D3DTBLEND_MODULATE:
                {
                    struct wined3d_texture *tex = NULL;
                    BOOL tex_alpha = FALSE;
                    DDPIXELFORMAT ddfmt;

                    hr = wined3d_device_get_texture(This->wined3d_device, 0, &tex);

                    if(hr == WINED3D_OK && tex)
                    {
                        struct wined3d_resource *sub_resource;

                        if ((sub_resource = wined3d_texture_get_sub_resource(tex, 0)))
                        {
                            struct wined3d_resource_desc desc;

                            wined3d_resource_get_desc(sub_resource, &desc);
                            ddfmt.dwSize = sizeof(ddfmt);
                            PixelFormat_WineD3DtoDD(&ddfmt, desc.format);
                            if (ddfmt.u5.dwRGBAlphaBitMask) tex_alpha = TRUE;
                        }

                        wined3d_texture_decref(tex);
                    }

                    if (tex_alpha)
                        wined3d_device_set_texture_stage_state(This->wined3d_device,
                                0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG1);
                    else
                        wined3d_device_set_texture_stage_state(This->wined3d_device,
                                0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG2);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_MODULATE);
                    break;
                }

                case D3DTBLEND_ADD:
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_ADD);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG2);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    break;

                case D3DTBLEND_MODULATEALPHA:
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_MODULATE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_MODULATE);
                    break;

                case D3DTBLEND_COPY:
                case D3DTBLEND_DECAL:
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_SELECT_ARG1);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG1);
                    break;

                case D3DTBLEND_DECALALPHA:
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_BLEND_TEXTURE_ALPHA);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG2);
                    wined3d_device_set_texture_stage_state(This->wined3d_device,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    break;

                default:
                    ERR("Unhandled texture environment %d !\n",Value);
            }

            hr = D3D_OK;
            break;
        }

        default:
            hr = IDirect3DDevice7_SetRenderState(&This->IDirect3DDevice7_iface, RenderStateType, Value);
            break;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_SetRenderState(IDirect3DDevice2 *iface,
        D3DRENDERSTATETYPE RenderStateType, DWORD Value)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %#x.\n", iface, RenderStateType, Value);

    return IDirect3DDevice3_SetRenderState(&This->IDirect3DDevice3_iface, RenderStateType, Value);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %#x.\n", iface, LightStateType, Value);

    if (!LightStateType || (LightStateType > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    if (LightStateType == D3DLIGHTSTATE_MATERIAL /* 1 */)
    {
        IDirect3DMaterialImpl *m = ddraw_get_object(&This->handle_table, Value - 1, DDRAW_HANDLE_MATERIAL);
        if (!m)
        {
            WARN("Invalid material handle.\n");
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
        }

        TRACE(" activating material %p.\n", m);
        material_activate(m);

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
                wined3d_mutex_unlock();
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
                wined3d_mutex_unlock();
                return DDERR_INVALIDPARAMS;
        }

        hr = IDirect3DDevice7_SetRenderState(&This->IDirect3DDevice7_iface, rs, Value);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_SetLightState(IDirect3DDevice2 *iface,
        D3DLIGHTSTATETYPE LightStateType, DWORD Value)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %#x.\n", iface, LightStateType, Value);

    return IDirect3DDevice3_SetLightState(&This->IDirect3DDevice3_iface, LightStateType, Value);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %p.\n", iface, LightStateType, Value);

    if (!LightStateType || (LightStateType > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    if(!Value)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
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
                wined3d_mutex_unlock();
                return DDERR_INVALIDPARAMS;
        }

        hr = IDirect3DDevice7_GetRenderState(&This->IDirect3DDevice7_iface, rs, Value);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetLightState(IDirect3DDevice2 *iface,
        D3DLIGHTSTATETYPE LightStateType, DWORD *Value)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %p.\n", iface, LightStateType, Value);

    return IDirect3DDevice3_GetLightState(&This->IDirect3DDevice3_iface, LightStateType, Value);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    D3DTRANSFORMSTATETYPE type;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, TransformStateType, Matrix);

    switch (TransformStateType)
    {
        case D3DTRANSFORMSTATE_WORLD:
            type = WINED3D_TS_WORLD_MATRIX(0);
            break;
        case D3DTRANSFORMSTATE_WORLD1:
            type = WINED3D_TS_WORLD_MATRIX(1);
            break;
        case D3DTRANSFORMSTATE_WORLD2:
            type = WINED3D_TS_WORLD_MATRIX(2);
            break;
        case D3DTRANSFORMSTATE_WORLD3:
            type = WINED3D_TS_WORLD_MATRIX(3);
            break;
        default:
            type = TransformStateType;
    }

    if (!Matrix)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    hr = wined3d_device_set_transform(This->wined3d_device, type, (struct wined3d_matrix *)Matrix);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_SetTransform(IDirect3DDevice3 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (!matrix)
        return DDERR_INVALIDPARAMS;

    if (state == D3DTRANSFORMSTATE_PROJECTION)
    {
        D3DMATRIX projection;
        HRESULT hr;

        wined3d_mutex_lock();
        multiply_matrix(&projection, &This->legacy_clipspace, matrix);
        hr = wined3d_device_set_transform(This->wined3d_device,
                WINED3D_TS_PROJECTION, (struct wined3d_matrix *)&projection);
        if (SUCCEEDED(hr))
            This->legacy_projection = *matrix;
        wined3d_mutex_unlock();

        return hr;
    }

    return IDirect3DDevice7_SetTransform(&This->IDirect3DDevice7_iface, state, matrix);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_SetTransform(IDirect3DDevice2 *iface,
        D3DTRANSFORMSTATETYPE TransformStateType, D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, TransformStateType, D3DMatrix);

    return IDirect3DDevice7_SetTransform(&This->IDirect3DDevice7_iface, TransformStateType, D3DMatrix);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    D3DTRANSFORMSTATETYPE type;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, TransformStateType, Matrix);

    switch(TransformStateType)
    {
        case D3DTRANSFORMSTATE_WORLD:
            type = WINED3D_TS_WORLD_MATRIX(0);
            break;
        case D3DTRANSFORMSTATE_WORLD1:
            type = WINED3D_TS_WORLD_MATRIX(1);
            break;
        case D3DTRANSFORMSTATE_WORLD2:
            type = WINED3D_TS_WORLD_MATRIX(2);
            break;
        case D3DTRANSFORMSTATE_WORLD3:
            type = WINED3D_TS_WORLD_MATRIX(3);
            break;
        default:
            type = TransformStateType;
    }

    if(!Matrix)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    hr = wined3d_device_get_transform(This->wined3d_device, type, (struct wined3d_matrix *)Matrix);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_GetTransform(IDirect3DDevice3 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (!matrix)
        return DDERR_INVALIDPARAMS;

    if (state == D3DTRANSFORMSTATE_PROJECTION)
    {
        wined3d_mutex_lock();
        *matrix = This->legacy_projection;
        wined3d_mutex_unlock();
        return DD_OK;
    }

    return IDirect3DDevice7_GetTransform(&This->IDirect3DDevice7_iface, state, matrix);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetTransform(IDirect3DDevice2 *iface,
        D3DTRANSFORMSTATETYPE TransformStateType, D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, TransformStateType, D3DMatrix);

    return IDirect3DDevice7_GetTransform(&This->IDirect3DDevice7_iface, TransformStateType, D3DMatrix);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;
    D3DTRANSFORMSTATETYPE type;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, TransformStateType, D3DMatrix);

    switch(TransformStateType)
    {
        case D3DTRANSFORMSTATE_WORLD:
            type = WINED3D_TS_WORLD_MATRIX(0);
            break;
        case D3DTRANSFORMSTATE_WORLD1:
            type = WINED3D_TS_WORLD_MATRIX(1);
            break;
        case D3DTRANSFORMSTATE_WORLD2:
            type = WINED3D_TS_WORLD_MATRIX(2);
            break;
        case D3DTRANSFORMSTATE_WORLD3:
            type = WINED3D_TS_WORLD_MATRIX(3);
            break;
        default:
            type = TransformStateType;
    }

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    hr = wined3d_device_multiply_transform(This->wined3d_device,
            type, (struct wined3d_matrix *)D3DMatrix);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_MultiplyTransform(IDirect3DDevice3 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (state == D3DTRANSFORMSTATE_PROJECTION)
    {
        D3DMATRIX projection, tmp;
        HRESULT hr;

        wined3d_mutex_lock();
        multiply_matrix(&tmp, &This->legacy_projection, matrix);
        multiply_matrix(&projection, &This->legacy_clipspace, &tmp);
        hr = wined3d_device_set_transform(This->wined3d_device,
                WINED3D_TS_PROJECTION, (struct wined3d_matrix *)&projection);
        if (SUCCEEDED(hr))
            This->legacy_projection = tmp;
        wined3d_mutex_unlock();

        return hr;
    }

    return IDirect3DDevice7_MultiplyTransform(&This->IDirect3DDevice7_iface, state, matrix);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_MultiplyTransform(IDirect3DDevice2 *iface,
        D3DTRANSFORMSTATETYPE TransformStateType, D3DMATRIX *D3DMatrix)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, TransformStateType, D3DMatrix);

    return IDirect3DDevice7_MultiplyTransform(&This->IDirect3DDevice7_iface, TransformStateType, D3DMatrix);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    UINT stride;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, FVF %#x, vertices %p, vertex_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, Vertices, VertexCount, Flags);

    if(!Vertices)
        return DDERR_INVALIDPARAMS;

    /* Get the stride */
    stride = get_flexible_vertex_size(VertexType);

    /* Set the FVF */
    wined3d_mutex_lock();
    hr = wined3d_device_set_vertex_declaration(This->wined3d_device, ddraw_find_decl(This->ddraw, VertexType));
    if(hr != D3D_OK)
    {
        wined3d_mutex_unlock();
        return hr;
    }

    /* This method translates to the user pointer draw of WineD3D */
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_primitive_up(This->wined3d_device, VertexCount, Vertices, stride);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_DrawPrimitive(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType, void *Vertices, DWORD VertexCount,
        DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p, primitive_type %#x, FVF %#x, vertices %p, vertex_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, Vertices, VertexCount, Flags);

    return IDirect3DDevice7_DrawPrimitive(&This->IDirect3DDevice7_iface,
            PrimitiveType, VertexType, Vertices, VertexCount, Flags);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_DrawPrimitive(IDirect3DDevice2 *iface,
        D3DPRIMITIVETYPE PrimitiveType, D3DVERTEXTYPE VertexType, void *Vertices,
        DWORD VertexCount, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    DWORD FVF;

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, vertices %p, vertex_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, Vertices, VertexCount, Flags);

    switch(VertexType)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", VertexType);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    }

    return IDirect3DDevice7_DrawPrimitive(&This->IDirect3DDevice7_iface,
            PrimitiveType, FVF, Vertices, VertexCount, Flags);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, FVF %#x, vertices %p, vertex_count %u, indices %p, index_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);

    /* Set the D3DDevice's FVF */
    wined3d_mutex_lock();
    hr = wined3d_device_set_vertex_declaration(This->wined3d_device, ddraw_find_decl(This->ddraw, VertexType));
    if(FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        wined3d_mutex_unlock();
        return hr;
    }

    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_indexed_primitive_up(This->wined3d_device, IndexCount, Indices,
            WINED3DFMT_R16_UINT, Vertices, get_flexible_vertex_size(VertexType));
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_DrawIndexedPrimitive(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType, void *Vertices, DWORD VertexCount,
        WORD *Indices, DWORD IndexCount, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p, primitive_type %#x, FVF %#x, vertices %p, vertex_count %u, indices %p, index_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);

    return IDirect3DDevice7_DrawIndexedPrimitive(&This->IDirect3DDevice7_iface,
            PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_DrawIndexedPrimitive(IDirect3DDevice2 *iface,
        D3DPRIMITIVETYPE PrimitiveType, D3DVERTEXTYPE VertexType, void *Vertices,
        DWORD VertexCount, WORD *Indices, DWORD IndexCount, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    DWORD FVF;

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, vertices %p, vertex_count %u, indices %p, index_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, Vertices, VertexCount, Indices, IndexCount, Flags);

    switch(VertexType)
    {
        case D3DVT_VERTEX: FVF = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: FVF = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: FVF = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %d\n", VertexType);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    }

    return IDirect3DDevice7_DrawIndexedPrimitive(&This->IDirect3DDevice7_iface,
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
    FIXME("iface %p, clip_status %p stub!\n", iface, ClipStatus);

    /* D3DCLIPSTATUS and WINED3DCLIPSTATUS are different. I don't know how to convert them
     * Perhaps this needs a new data type and an additional IWineD3DDevice method
     */
    /* return IWineD3DDevice_SetClipStatus(This->wineD3DDevice, ClipStatus);*/
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_3_SetClipStatus(IDirect3DDevice3 *iface,
        D3DCLIPSTATUS *ClipStatus)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p, clip_status %p.\n", iface, ClipStatus);

    return IDirect3DDevice7_SetClipStatus(&This->IDirect3DDevice7_iface, ClipStatus);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_SetClipStatus(IDirect3DDevice2 *iface,
        D3DCLIPSTATUS *ClipStatus)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p, clip_status %p.\n", iface, ClipStatus);

    return IDirect3DDevice7_SetClipStatus(&This->IDirect3DDevice7_iface, ClipStatus);
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
    FIXME("iface %p, clip_status %p stub!\n", iface, ClipStatus);

    /* D3DCLIPSTATUS and WINED3DCLIPSTATUS are different. I don't know how to convert them */
    /* return IWineD3DDevice_GetClipStatus(This->wineD3DDevice, ClipStatus);*/
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDeviceImpl_3_GetClipStatus(IDirect3DDevice3 *iface,
        D3DCLIPSTATUS *ClipStatus)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    TRACE("iface %p, clip_status %p.\n", iface, ClipStatus);

    return IDirect3DDevice7_GetClipStatus(&This->IDirect3DDevice7_iface, ClipStatus);
}

static HRESULT WINAPI IDirect3DDeviceImpl_2_GetClipStatus(IDirect3DDevice2 *iface,
        D3DCLIPSTATUS *ClipStatus)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice2(iface);
    TRACE("iface %p, clip_status %p.\n", iface, ClipStatus);

    return IDirect3DDevice7_GetClipStatus(&This->IDirect3DDevice7_iface, ClipStatus);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_strided_data wined3d_strided;
    DWORD i;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, FVF %#x, strided_data %p, vertex_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);

    memset(&wined3d_strided, 0, sizeof(wined3d_strided));
    /* Get the strided data right. the wined3d structure is a bit bigger
     * Watch out: The contents of the strided data are determined by the fvf,
     * not by the members set in D3DDrawPrimStrideData. So it's valid
     * to have diffuse.lpvData set to 0xdeadbeef if the diffuse flag is
     * not set in the fvf.
     */
    if(VertexType & D3DFVF_POSITION_MASK)
    {
        wined3d_strided.position.format = WINED3DFMT_R32G32B32_FLOAT;
        wined3d_strided.position.data = D3DDrawPrimStrideData->position.lpvData;
        wined3d_strided.position.stride = D3DDrawPrimStrideData->position.dwStride;
        if (VertexType & D3DFVF_XYZRHW)
        {
            wined3d_strided.position.format = WINED3DFMT_R32G32B32A32_FLOAT;
            wined3d_strided.position_transformed = TRUE;
        }
        else
        {
            wined3d_strided.position_transformed = FALSE;
        }
    }

    if (VertexType & D3DFVF_NORMAL)
    {
        wined3d_strided.normal.format = WINED3DFMT_R32G32B32_FLOAT;
        wined3d_strided.normal.data = D3DDrawPrimStrideData->normal.lpvData;
        wined3d_strided.normal.stride = D3DDrawPrimStrideData->normal.dwStride;
    }

    if (VertexType & D3DFVF_DIFFUSE)
    {
        wined3d_strided.diffuse.format = WINED3DFMT_B8G8R8A8_UNORM;
        wined3d_strided.diffuse.data = D3DDrawPrimStrideData->diffuse.lpvData;
        wined3d_strided.diffuse.stride = D3DDrawPrimStrideData->diffuse.dwStride;
    }

    if (VertexType & D3DFVF_SPECULAR)
    {
        wined3d_strided.specular.format = WINED3DFMT_B8G8R8A8_UNORM;
        wined3d_strided.specular.data = D3DDrawPrimStrideData->specular.lpvData;
        wined3d_strided.specular.stride = D3DDrawPrimStrideData->specular.dwStride;
    }

    for (i = 0; i < GET_TEXCOUNT_FROM_FVF(VertexType); ++i)
    {
        switch (GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i))
        {
            case 1: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32_FLOAT; break;
            case 2: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32G32_FLOAT; break;
            case 3: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32G32B32_FLOAT; break;
            case 4: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32G32B32A32_FLOAT; break;
            default: ERR("Unexpected texture coordinate size %d\n",
                         GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i));
        }
        wined3d_strided.tex_coords[i].data = D3DDrawPrimStrideData->textureCoords[i].lpvData;
        wined3d_strided.tex_coords[i].stride = D3DDrawPrimStrideData->textureCoords[i].dwStride;
    }

    /* WineD3D doesn't need the FVF here */
    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_primitive_strided(This->wined3d_device, VertexCount, &wined3d_strided);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_DrawPrimitiveStrided(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, FVF %#x, strided_data %p, vertex_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);

    return IDirect3DDevice7_DrawPrimitiveStrided(&This->IDirect3DDevice7_iface,
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_strided_data wined3d_strided;
    DWORD i;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, FVF %#x, strided_data %p, vertex_count %u, indices %p, index_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);

    memset(&wined3d_strided, 0, sizeof(wined3d_strided));
    /* Get the strided data right. the wined3d structure is a bit bigger
     * Watch out: The contents of the strided data are determined by the fvf,
     * not by the members set in D3DDrawPrimStrideData. So it's valid
     * to have diffuse.lpvData set to 0xdeadbeef if the diffuse flag is
     * not set in the fvf. */
    if (VertexType & D3DFVF_POSITION_MASK)
    {
        wined3d_strided.position.format = WINED3DFMT_R32G32B32_FLOAT;
        wined3d_strided.position.data = D3DDrawPrimStrideData->position.lpvData;
        wined3d_strided.position.stride = D3DDrawPrimStrideData->position.dwStride;
        if (VertexType & D3DFVF_XYZRHW)
        {
            wined3d_strided.position.format = WINED3DFMT_R32G32B32A32_FLOAT;
            wined3d_strided.position_transformed = TRUE;
        }
        else
        {
            wined3d_strided.position_transformed = FALSE;
        }
    }

    if (VertexType & D3DFVF_NORMAL)
    {
        wined3d_strided.normal.format = WINED3DFMT_R32G32B32_FLOAT;
        wined3d_strided.normal.data = D3DDrawPrimStrideData->normal.lpvData;
        wined3d_strided.normal.stride = D3DDrawPrimStrideData->normal.dwStride;
    }

    if (VertexType & D3DFVF_DIFFUSE)
    {
        wined3d_strided.diffuse.format = WINED3DFMT_B8G8R8A8_UNORM;
        wined3d_strided.diffuse.data = D3DDrawPrimStrideData->diffuse.lpvData;
        wined3d_strided.diffuse.stride = D3DDrawPrimStrideData->diffuse.dwStride;
    }

    if (VertexType & D3DFVF_SPECULAR)
    {
        wined3d_strided.specular.format = WINED3DFMT_B8G8R8A8_UNORM;
        wined3d_strided.specular.data = D3DDrawPrimStrideData->specular.lpvData;
        wined3d_strided.specular.stride = D3DDrawPrimStrideData->specular.dwStride;
    }

    for (i = 0; i < GET_TEXCOUNT_FROM_FVF(VertexType); ++i)
    {
        switch (GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i))
        {
            case 1: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32_FLOAT; break;
            case 2: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32G32_FLOAT; break;
            case 3: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32G32B32_FLOAT; break;
            case 4: wined3d_strided.tex_coords[i].format = WINED3DFMT_R32G32B32A32_FLOAT; break;
            default: ERR("Unexpected texture coordinate size %d\n",
                         GET_TEXCOORD_SIZE_FROM_FVF(VertexType, i));
        }
        wined3d_strided.tex_coords[i].data = D3DDrawPrimStrideData->textureCoords[i].lpvData;
        wined3d_strided.tex_coords[i].stride = D3DDrawPrimStrideData->textureCoords[i].dwStride;
    }

    /* WineD3D doesn't need the FVF here */
    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_indexed_primitive_strided(This->wined3d_device,
            IndexCount, &wined3d_strided, VertexCount, Indices, WINED3DFMT_R16_UINT);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount, WORD *Indices,
        DWORD IndexCount, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, FVF %#x, strided_data %p, vertex_count %u, indices %p, index_count %u, flags %#x.\n",
            iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);

    return IDirect3DDevice7_DrawIndexedPrimitiveStrided(&This->IDirect3DDevice7_iface,
            PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    IDirect3DVertexBufferImpl *vb = unsafe_impl_from_IDirect3DVertexBuffer7(D3DVertexBuf);
    HRESULT hr;
    DWORD stride;

    TRACE("iface %p, primitive_type %#x, vb %p, start_vertex %u, vertex_count %u, flags %#x.\n",
            iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);

    /* Sanity checks */
    if(!vb)
    {
        ERR("(%p) No Vertex buffer specified\n", This);
        return DDERR_INVALIDPARAMS;
    }
    stride = get_flexible_vertex_size(vb->fvf);

    wined3d_mutex_lock();
    hr = wined3d_device_set_vertex_declaration(This->wined3d_device, vb->wineD3DVertexDeclaration);
    if (FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        wined3d_mutex_unlock();
        return hr;
    }

    /* Set the vertex stream source */
    hr = wined3d_device_set_stream_source(This->wined3d_device, 0, vb->wineD3DVertexBuffer, 0, stride);
    if(hr != D3D_OK)
    {
        ERR("(%p) IDirect3DDevice::SetStreamSource failed with hr = %08x\n", This, hr);
        wined3d_mutex_unlock();
        return hr;
    }

    /* Now draw the primitives */
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_primitive(This->wined3d_device, StartVertex, NumVertices);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_DrawPrimitiveVB(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, IDirect3DVertexBuffer *D3DVertexBuf, DWORD StartVertex,
        DWORD NumVertices, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirect3DVertexBufferImpl *vb = unsafe_impl_from_IDirect3DVertexBuffer(D3DVertexBuf);

    TRACE("iface %p, primitive_type %#x, vb %p, start_vertex %u, vertex_count %u, flags %#x.\n",
            iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);

    return IDirect3DDevice7_DrawPrimitiveVB(&This->IDirect3DDevice7_iface,
            PrimitiveType, &vb->IDirect3DVertexBuffer7_iface, StartVertex, NumVertices, Flags);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    IDirect3DVertexBufferImpl *vb = unsafe_impl_from_IDirect3DVertexBuffer7(D3DVertexBuf);
    DWORD stride = get_flexible_vertex_size(vb->fvf);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_resource_desc desc;
    WORD *LockedIndices;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, vb %p, start_vertex %u, vertex_count %u, indices %p, index_count %u, flags %#x.\n",
            iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Indices, IndexCount, Flags);

    /* Steps:
     * 1) Upload the Indices to the index buffer
     * 2) Set the index source
     * 3) Set the Vertex Buffer as the Stream source
     * 4) Call IWineD3DDevice::DrawIndexedPrimitive
     */

    wined3d_mutex_lock();

    hr = wined3d_device_set_vertex_declaration(This->wined3d_device, vb->wineD3DVertexDeclaration);
    if (FAILED(hr))
    {
        ERR(" (%p) Setting the FVF failed, hr = %x!\n", This, hr);
        wined3d_mutex_unlock();
        return hr;
    }

    /* check that the buffer is large enough to hold the indices,
     * reallocate if necessary. */
    wined3d_resource = wined3d_buffer_get_resource(This->indexbuffer);
    wined3d_resource_get_desc(wined3d_resource, &desc);
    if (desc.size < IndexCount * sizeof(WORD))
    {
        UINT size = max(desc.size * 2, IndexCount * sizeof(WORD));
        struct wined3d_buffer *buffer;

        TRACE("Growing index buffer to %u bytes\n", size);

        hr = wined3d_buffer_create_ib(This->wined3d_device, size, WINED3DUSAGE_DYNAMIC /* Usage */,
                WINED3D_POOL_DEFAULT, NULL, &ddraw_null_wined3d_parent_ops, &buffer);
        if (FAILED(hr))
        {
            ERR("(%p) IWineD3DDevice::CreateIndexBuffer failed with hr = %08x\n", This, hr);
            wined3d_mutex_unlock();
            return hr;
        }

        wined3d_buffer_decref(This->indexbuffer);
        This->indexbuffer = buffer;
    }

    /* Copy the index stream into the index buffer. A new IWineD3DDevice
     * method could be created which takes an user pointer containing the
     * indices or a SetData-Method for the index buffer, which overrides the
     * index buffer data with our pointer. */
    hr = wined3d_buffer_map(This->indexbuffer, 0, IndexCount * sizeof(WORD),
            (BYTE **)&LockedIndices, 0);
    if (FAILED(hr))
    {
        ERR("Failed to map buffer, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }
    memcpy(LockedIndices, Indices, IndexCount * sizeof(WORD));
    wined3d_buffer_unmap(This->indexbuffer);

    /* Set the index stream */
    wined3d_device_set_base_vertex_index(This->wined3d_device, StartVertex);
    hr = wined3d_device_set_index_buffer(This->wined3d_device, This->indexbuffer, WINED3DFMT_R16_UINT);

    /* Set the vertex stream source */
    hr = wined3d_device_set_stream_source(This->wined3d_device, 0, vb->wineD3DVertexBuffer, 0, stride);
    if (FAILED(hr))
    {
        ERR("(%p) IDirect3DDevice::SetStreamSource failed with hr = %08x\n", This, hr);
        wined3d_mutex_unlock();
        return hr;
    }


    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_indexed_primitive(This->wined3d_device, 0, IndexCount);

    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, IDirect3DVertexBuffer *D3DVertexBuf, WORD *Indices,
        DWORD IndexCount, DWORD Flags)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirect3DVertexBufferImpl *vb = unsafe_impl_from_IDirect3DVertexBuffer(D3DVertexBuf);

    TRACE("iface %p, primitive_type %#x, vb %p, indices %p, index_count %u, flags %#x.\n",
            iface, PrimitiveType, D3DVertexBuf, Indices, IndexCount, Flags);

    return IDirect3DDevice7_DrawIndexedPrimitiveVB(&This->IDirect3DDevice7_iface,
            PrimitiveType, &vb->IDirect3DVertexBuffer7_iface, 0, IndexCount, Indices, IndexCount,
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
 *  D3D_OK
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

    TRACE("iface %p, centers %p, radii %p, sphere_count %u, flags %#x, return_values %p.\n",
            iface, Centers, Radii, NumSpheres, Flags, ReturnValues);

    hr = IDirect3DDeviceImpl_7_GetTransform(iface, D3DTRANSFORMSTATE_WORLD, &m);
    if ( hr != DD_OK ) return DDERR_INVALIDPARAMS;
    hr = IDirect3DDeviceImpl_7_GetTransform(iface, D3DTRANSFORMSTATE_VIEW, &temp);
    if ( hr != DD_OK ) return DDERR_INVALIDPARAMS;
    multiply_matrix(&m, &temp, &m);

    hr = IDirect3DDeviceImpl_7_GetTransform(iface, D3DTRANSFORMSTATE_PROJECTION, &temp);
    if ( hr != DD_OK ) return DDERR_INVALIDPARAMS;
    multiply_matrix(&m, &temp, &m);

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_ComputeSphereVisibility(IDirect3DDevice3 *iface,
        D3DVECTOR *Centers, D3DVALUE *Radii, DWORD NumSpheres, DWORD Flags, DWORD *ReturnValues)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, centers %p, radii %p, sphere_count %u, flags %#x, return_values %p.\n",
            iface, Centers, Radii, NumSpheres, Flags, ReturnValues);

    return IDirect3DDevice7_ComputeSphereVisibility(&This->IDirect3DDevice7_iface,
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_texture *wined3d_texture;
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, Texture);

    if(!Texture)
    {
        TRACE("Texture == NULL, failing with DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_get_texture(This->wined3d_device, Stage, &wined3d_texture);
    if (FAILED(hr) || !wined3d_texture)
    {
        *Texture = NULL;
        wined3d_mutex_unlock();
        return hr;
    }

    *Texture = wined3d_texture_get_parent(wined3d_texture);
    IDirectDrawSurface7_AddRef(*Texture);
    wined3d_texture_decref(wined3d_texture);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_GetTexture(IDirect3DDevice3 *iface, DWORD Stage,
        IDirect3DTexture2 **Texture2)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    HRESULT ret;
    IDirectDrawSurface7 *ret_val;
    IDirectDrawSurfaceImpl *ret_val_impl;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, Texture2);

    ret = IDirect3DDevice7_GetTexture(&This->IDirect3DDevice7_iface, Stage, &ret_val);

    ret_val_impl = unsafe_impl_from_IDirectDrawSurface7(ret_val);
    *Texture2 = ret_val_impl ? &ret_val_impl->IDirect3DTexture2_iface : NULL;

    TRACE("Returning texture %p.\n", *Texture2);

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    IDirectDrawSurfaceImpl *surf = unsafe_impl_from_IDirectDrawSurface7(Texture);
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, Texture);

    /* Texture may be NULL here */
    wined3d_mutex_lock();
    hr = wined3d_device_set_texture(This->wined3d_device,
            Stage, surf ? surf->wined3d_texture : NULL);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);
    IDirectDrawSurfaceImpl *tex = unsafe_impl_from_IDirect3DTexture2(Texture2);
    DWORD texmapblend;
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, Texture2);

    wined3d_mutex_lock();

    if (This->legacyTextureBlending)
        IDirect3DDevice3_GetRenderState(iface, D3DRENDERSTATE_TEXTUREMAPBLEND, &texmapblend);

    hr = IDirect3DDevice7_SetTexture(&This->IDirect3DDevice7_iface, Stage, &tex->IDirectDrawSurface7_iface);

    if (This->legacyTextureBlending && texmapblend == D3DTBLEND_MODULATE)
    {
        /* This fixup is required by the way D3DTBLEND_MODULATE maps to texture stage states.
           See IDirect3DDeviceImpl_3_SetRenderState for details. */
        struct wined3d_texture *tex = NULL;
        BOOL tex_alpha = FALSE;
        DDPIXELFORMAT ddfmt;
        HRESULT result;

        result = wined3d_device_get_texture(This->wined3d_device, 0, &tex);
        if (result == WINED3D_OK && tex)
        {
            struct wined3d_resource *sub_resource;

            if ((sub_resource = wined3d_texture_get_sub_resource(tex, 0)))
            {
                struct wined3d_resource_desc desc;

                wined3d_resource_get_desc(sub_resource, &desc);
                ddfmt.dwSize = sizeof(ddfmt);
                PixelFormat_WineD3DtoDD(&ddfmt, desc.format);
                if (ddfmt.u5.dwRGBAlphaBitMask) tex_alpha = TRUE;
            }

            wined3d_texture_decref(tex);
        }

        /* Arg 1/2 are already set to WINED3DTA_TEXTURE/WINED3DTA_CURRENT in case of D3DTBLEND_MODULATE */
        if (tex_alpha)
            wined3d_device_set_texture_stage_state(This->wined3d_device,
                    0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG1);
        else
            wined3d_device_set_texture_stage_state(This->wined3d_device,
                    0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG2);
    }

    wined3d_mutex_unlock();

    return hr;
}

static const struct tss_lookup
{
    BOOL sampler_state;
    enum wined3d_texture_stage_state state;
}
tss_lookup[] =
{
    {FALSE, WINED3D_TSS_INVALID},                   /*  0, unused */
    {FALSE, WINED3D_TSS_COLOR_OP},                  /*  1, D3DTSS_COLOROP */
    {FALSE, WINED3D_TSS_COLOR_ARG1},                /*  2, D3DTSS_COLORARG1 */
    {FALSE, WINED3D_TSS_COLOR_ARG2},                /*  3, D3DTSS_COLORARG2 */
    {FALSE, WINED3D_TSS_ALPHA_OP},                  /*  4, D3DTSS_ALPHAOP */
    {FALSE, WINED3D_TSS_ALPHA_ARG1},                /*  5, D3DTSS_ALPHAARG1 */
    {FALSE, WINED3D_TSS_ALPHA_ARG2},                /*  6, D3DTSS_ALPHAARG2 */
    {FALSE, WINED3D_TSS_BUMPENV_MAT00},             /*  7, D3DTSS_BUMPENVMAT00 */
    {FALSE, WINED3D_TSS_BUMPENV_MAT01},             /*  8, D3DTSS_BUMPENVMAT01 */
    {FALSE, WINED3D_TSS_BUMPENV_MAT10},             /*  9, D3DTSS_BUMPENVMAT10 */
    {FALSE, WINED3D_TSS_BUMPENV_MAT11},             /* 10, D3DTSS_BUMPENVMAT11 */
    {FALSE, WINED3D_TSS_TEXCOORD_INDEX},            /* 11, D3DTSS_TEXCOORDINDEX */
    {TRUE,  WINED3D_SAMP_ADDRESS_U},                /* 12, D3DTSS_ADDRESS */
    {TRUE,  WINED3D_SAMP_ADDRESS_U},                /* 13, D3DTSS_ADDRESSU */
    {TRUE,  WINED3D_SAMP_ADDRESS_V},                /* 14, D3DTSS_ADDRESSV */
    {TRUE,  WINED3D_SAMP_BORDER_COLOR},             /* 15, D3DTSS_BORDERCOLOR */
    {TRUE,  WINED3D_SAMP_MAG_FILTER},               /* 16, D3DTSS_MAGFILTER */
    {TRUE,  WINED3D_SAMP_MIN_FILTER},               /* 17, D3DTSS_MINFILTER */
    {TRUE,  WINED3D_SAMP_MIP_FILTER},               /* 18, D3DTSS_MIPFILTER */
    {TRUE,  WINED3D_SAMP_MIPMAP_LOD_BIAS},          /* 19, D3DTSS_MIPMAPLODBIAS */
    {TRUE,  WINED3D_SAMP_MAX_MIP_LEVEL},            /* 20, D3DTSS_MAXMIPLEVEL */
    {TRUE,  WINED3D_SAMP_MAX_ANISOTROPY},           /* 21, D3DTSS_MAXANISOTROPY */
    {FALSE, WINED3D_TSS_BUMPENV_LSCALE},            /* 22, D3DTSS_BUMPENVLSCALE */
    {FALSE, WINED3D_TSS_BUMPENV_LOFFSET},           /* 23, D3DTSS_BUMPENVLOFFSET */
    {FALSE, WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS},   /* 24, D3DTSS_TEXTURETRANSFORMFLAGS */
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;
    const struct tss_lookup *l;

    TRACE("iface %p, stage %u, state %#x, value %p.\n",
            iface, Stage, TexStageStateType, State);

    if(!State)
        return DDERR_INVALIDPARAMS;

    if (TexStageStateType > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        WARN("Invalid TexStageStateType %#x passed.\n", TexStageStateType);
        return DD_OK;
    }

    l = &tss_lookup[TexStageStateType];

    wined3d_mutex_lock();

    if (l->sampler_state)
    {
        hr = wined3d_device_get_sampler_state(This->wined3d_device, Stage, l->state, State);

        switch(TexStageStateType)
        {
            /* Mipfilter is a sampler state with different values */
            case D3DTSS_MIPFILTER:
            {
                switch(*State)
                {
                    case WINED3D_TEXF_NONE:
                        *State = D3DTFP_NONE;
                        break;
                    case WINED3D_TEXF_POINT:
                        *State = D3DTFP_POINT;
                        break;
                    case WINED3D_TEXF_LINEAR:
                        *State = D3DTFP_LINEAR;
                        break;
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
                    case WINED3D_TEXF_POINT:
                            *State = D3DTFG_POINT;
                            break;
                    case WINED3D_TEXF_LINEAR:
                            *State = D3DTFG_LINEAR;
                            break;
                    case WINED3D_TEXF_ANISOTROPIC:
                            *State = D3DTFG_ANISOTROPIC;
                            break;
                    case WINED3D_TEXF_FLAT_CUBIC:
                            *State = D3DTFG_FLATCUBIC;
                            break;
                    case WINED3D_TEXF_GAUSSIAN_CUBIC:
                            *State = D3DTFG_GAUSSIANCUBIC;
                            break;
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
        hr = wined3d_device_get_texture_stage_state(This->wined3d_device, Stage, l->state, State);
    }

    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_GetTextureStageState(IDirect3DDevice3 *iface,
        DWORD Stage, D3DTEXTURESTAGESTATETYPE TexStageStateType, DWORD *State)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, stage %u, state %#x, value %p.\n",
            iface, Stage, TexStageStateType, State);

    return IDirect3DDevice7_GetTextureStageState(&This->IDirect3DDevice7_iface,
            Stage, TexStageStateType, State);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    const struct tss_lookup *l;
    HRESULT hr;

    TRACE("iface %p, stage %u, state %#x, value %#x.\n",
            iface, Stage, TexStageStateType, State);

    if (TexStageStateType > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        WARN("Invalid TexStageStateType %#x passed.\n", TexStageStateType);
        return DD_OK;
    }

    l = &tss_lookup[TexStageStateType];

    wined3d_mutex_lock();

    if (l->sampler_state)
    {
        switch(TexStageStateType)
        {
            /* Mipfilter is a sampler state with different values */
            case D3DTSS_MIPFILTER:
            {
                switch(State)
                {
                    case D3DTFP_NONE:
                        State = WINED3D_TEXF_NONE;
                        break;
                    case D3DTFP_POINT:
                        State = WINED3D_TEXF_POINT;
                        break;
                    case 0: /* Unchecked */
                    case D3DTFP_LINEAR:
                        State = WINED3D_TEXF_LINEAR;
                        break;
                    default:
                        ERR("Unexpected mipfilter value %d\n", State);
                        State = WINED3D_TEXF_NONE;
                        break;
                }
                break;
            }

            /* Magfilter has slightly different values */
            case D3DTSS_MAGFILTER:
            {
                switch(State)
                {
                    case D3DTFG_POINT:
                        State = WINED3D_TEXF_POINT;
                        break;
                    case D3DTFG_LINEAR:
                        State = WINED3D_TEXF_LINEAR;
                        break;
                    case D3DTFG_FLATCUBIC:
                        State = WINED3D_TEXF_FLAT_CUBIC;
                        break;
                    case D3DTFG_GAUSSIANCUBIC:
                        State = WINED3D_TEXF_GAUSSIAN_CUBIC;
                        break;
                    case D3DTFG_ANISOTROPIC:
                        State = WINED3D_TEXF_ANISOTROPIC;
                        break;
                    default:
                        ERR("Unexpected d3d7 mag filter type %d\n", State);
                        State = WINED3D_TEXF_POINT;
                        break;
                }
                break;
            }

            case D3DTSS_ADDRESS:
                wined3d_device_set_sampler_state(This->wined3d_device, Stage, WINED3D_SAMP_ADDRESS_V, State);
                break;

            default:
                break;
        }

        hr = wined3d_device_set_sampler_state(This->wined3d_device, Stage, l->state, State);
    }
    else
    {
        hr = wined3d_device_set_texture_stage_state(This->wined3d_device, Stage, l->state, State);
    }

    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_SetTextureStageState(IDirect3DDevice3 *iface,
        DWORD Stage, D3DTEXTURESTAGESTATETYPE TexStageStateType, DWORD State)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, stage %u, state %#x, value %#x.\n",
            iface, Stage, TexStageStateType, State);

    return IDirect3DDevice7_SetTextureStageState(&This->IDirect3DDevice7_iface,
            Stage, TexStageStateType, State);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, pass_count %p.\n", iface, NumPasses);

    wined3d_mutex_lock();
    hr = wined3d_device_validate_device(This->wined3d_device, NumPasses);
    wined3d_mutex_unlock();

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

static HRESULT WINAPI IDirect3DDeviceImpl_3_ValidateDevice(IDirect3DDevice3 *iface, DWORD *Passes)
{
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, pass_count %p.\n", iface, Passes);

    return IDirect3DDevice7_ValidateDevice(&This->IDirect3DDevice7_iface, Passes);
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
static HRESULT IDirect3DDeviceImpl_7_Clear(IDirect3DDevice7 *iface, DWORD count,
        D3DRECT *rects, DWORD flags, D3DCOLOR color, D3DVALUE z, DWORD stencil)
{
    const struct wined3d_color c =
    {
        ((color >> 16) & 0xff) / 255.0f,
        ((color >>  8) & 0xff) / 255.0f,
        (color & 0xff) / 255.0f,
        ((color >> 24) & 0xff) / 255.0f,
    };
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, count %u, rects %p, flags %#x, color 0x%08x, z %.8e, stencil %#x.\n",
            iface, count, rects, flags, color, z, stencil);

    wined3d_mutex_lock();
    hr = wined3d_device_clear(This->wined3d_device, count, (RECT *)rects, flags, &c, z, stencil);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, Data);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DVIEWPORT7 is compatible with struct wined3d_viewport. */
    wined3d_mutex_lock();
    hr = wined3d_device_set_viewport(This->wined3d_device, (struct wined3d_viewport *)Data);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, Data);

    if(!Data)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DVIEWPORT7 is compatible with struct wined3d_viewport. */
    wined3d_mutex_lock();
    hr = wined3d_device_get_viewport(This->wined3d_device, (struct wined3d_viewport *)Data);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, Mat);

    if (!Mat) return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    /* Note: D3DMATERIAL7 is compatible with struct wined3d_material. */
    hr = wined3d_device_set_material(This->wined3d_device, (struct wined3d_material *)Mat);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, Mat);

    wined3d_mutex_lock();
    /* Note: D3DMATERIAL7 is compatible with struct wined3d_material. */
    hr = wined3d_device_get_material(This->wined3d_device, (struct wined3d_material *)Mat);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, light_idx %u, light %p.\n", iface, LightIndex, Light);

    wined3d_mutex_lock();
    /* Note: D3DLIGHT7 is compatible with struct wined3d_light. */
    hr = wined3d_device_set_light(This->wined3d_device, LightIndex, (struct wined3d_light *)Light);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT rc;

    TRACE("iface %p, light_idx %u, light %p.\n", iface, LightIndex, Light);

    wined3d_mutex_lock();
    /* Note: D3DLIGHT7 is compatible with struct wined3d_light. */
    rc =  wined3d_device_get_light(This->wined3d_device, LightIndex, (struct wined3d_light *)Light);
    wined3d_mutex_unlock();

    /* Translate the result. WineD3D returns other values than D3D7 */
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_begin_stateblock(This->wined3d_device);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    HRESULT hr;
    DWORD h;

    TRACE("iface %p, stateblock %p.\n", iface, BlockHandle);

    if(!BlockHandle)
    {
        WARN("BlockHandle == NULL, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();

    hr = wined3d_device_end_stateblock(This->wined3d_device, &wined3d_sb);
    if (FAILED(hr))
    {
        WARN("Failed to end stateblock, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        *BlockHandle = 0;
        return hr_ddraw_from_wined3d(hr);
    }

    h = ddraw_allocate_handle(&This->handle_table, wined3d_sb, DDRAW_HANDLE_STATEBLOCK);
    if (h == DDRAW_INVALID_HANDLE)
    {
        ERR("Failed to allocate a stateblock handle.\n");
        wined3d_stateblock_decref(wined3d_sb);
        wined3d_mutex_unlock();
        *BlockHandle = 0;
        return DDERR_OUTOFMEMORY;
    }

    wined3d_mutex_unlock();
    *BlockHandle = h + 1;

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
    IDirectDrawSurfaceImpl *surf = unsafe_impl_from_IDirectDrawSurface7(Texture);

    TRACE("iface %p, texture %p.\n", iface, Texture);

    if(!Texture)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    wined3d_surface_preload(surf->wined3d_surface);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    HRESULT hr;

    TRACE("iface %p, stateblock %#x.\n", iface, BlockHandle);

    wined3d_mutex_lock();
    wined3d_sb = ddraw_get_object(&This->handle_table, BlockHandle - 1, DDRAW_HANDLE_STATEBLOCK);
    if (!wined3d_sb)
    {
        WARN("Invalid stateblock handle.\n");
        wined3d_mutex_unlock();
        return D3DERR_INVALIDSTATEBLOCK;
    }

    hr = wined3d_stateblock_apply(wined3d_sb);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    HRESULT hr;

    TRACE("iface %p, stateblock %#x.\n", iface, BlockHandle);

    wined3d_mutex_lock();
    wined3d_sb = ddraw_get_object(&This->handle_table, BlockHandle - 1, DDRAW_HANDLE_STATEBLOCK);
    if (!wined3d_sb)
    {
        WARN("Invalid stateblock handle.\n");
        wined3d_mutex_unlock();
        return D3DERR_INVALIDSTATEBLOCK;
    }

    hr = wined3d_stateblock_capture(wined3d_sb);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    ULONG ref;

    TRACE("iface %p, stateblock %#x.\n", iface, BlockHandle);

    wined3d_mutex_lock();

    wined3d_sb = ddraw_free_handle(&This->handle_table, BlockHandle - 1, DDRAW_HANDLE_STATEBLOCK);
    if (!wined3d_sb)
    {
        WARN("Invalid stateblock handle.\n");
        wined3d_mutex_unlock();
        return D3DERR_INVALIDSTATEBLOCK;
    }

    if ((ref = wined3d_stateblock_decref(wined3d_sb)))
    {
        ERR("Something is still holding stateblock %p (refcount %u).\n", wined3d_sb, ref);
    }

    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    HRESULT hr;
    DWORD h;

    TRACE("iface %p, type %#x, stateblock %p.\n", iface, Type, BlockHandle);

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

    wined3d_mutex_lock();

    /* The D3DSTATEBLOCKTYPE enum is fine here. */
    hr = wined3d_stateblock_create(This->wined3d_device, Type, &wined3d_sb);
    if (FAILED(hr))
    {
        WARN("Failed to create stateblock, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        return hr_ddraw_from_wined3d(hr);
    }

    h = ddraw_allocate_handle(&This->handle_table, wined3d_sb, DDRAW_HANDLE_STATEBLOCK);
    if (h == DDRAW_INVALID_HANDLE)
    {
        ERR("Failed to allocate stateblock handle.\n");
        wined3d_stateblock_decref(wined3d_sb);
        wined3d_mutex_unlock();
        return DDERR_OUTOFMEMORY;
    }

    *BlockHandle = h + 1;
    wined3d_mutex_unlock();

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
            IDirectDrawSurface7_GetAttachedSurface(&dest_level->IDirectDrawSurface7_iface, &ddsd.ddsCaps, &temp);

            if (dest_level != dest) IDirectDrawSurface7_Release(&dest_level->IDirectDrawSurface7_iface);

            dest_level = unsafe_impl_from_IDirectDrawSurface7(temp);
        }

        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
        IDirectDrawSurface7_GetAttachedSurface(&src_level->IDirectDrawSurface7_iface, &ddsd.ddsCaps, &temp);

        if (src_level != src) IDirectDrawSurface7_Release(&src_level->IDirectDrawSurface7_iface);

        src_level = unsafe_impl_from_IDirectDrawSurface7(temp);
    }

    if (src_level && src_level != src) IDirectDrawSurface7_Release(&src_level->IDirectDrawSurface7_iface);
    if (dest_level && dest_level != dest) IDirectDrawSurface7_Release(&dest_level->IDirectDrawSurface7_iface);

    return !dest_level && levelFound;
}

/* Helper function for IDirect3DDeviceImpl_7_Load. */
static void copy_mipmap_chain(IDirect3DDeviceImpl *device,
                              IDirectDrawSurfaceImpl *dest,
                              IDirectDrawSurfaceImpl *src,
                              const POINT *DestPoint,
                              const RECT *SrcRect)
{
    IDirectDrawSurfaceImpl *src_level, *dest_level;
    IDirectDrawSurface7 *temp;
    DDSURFACEDESC2 ddsd;
    POINT point;
    RECT src_rect;
    HRESULT hr;
    IDirectDrawPalette *pal = NULL, *pal_src = NULL;
    DWORD ckeyflag;
    DDCOLORKEY ddckey;

    /* Copy palette, if possible. */
    IDirectDrawSurface7_GetPalette(&src->IDirectDrawSurface7_iface, &pal_src);
    IDirectDrawSurface7_GetPalette(&dest->IDirectDrawSurface7_iface, &pal);

    if (pal_src != NULL && pal != NULL)
    {
        PALETTEENTRY palent[256];

        IDirectDrawPalette_GetEntries(pal_src, 0, 0, 256, palent);
        IDirectDrawPalette_SetEntries(pal, 0, 0, 256, palent);
    }

    if (pal) IDirectDrawPalette_Release(pal);
    if (pal_src) IDirectDrawPalette_Release(pal_src);

    /* Copy colorkeys, if present. */
    for (ckeyflag = DDCKEY_DESTBLT; ckeyflag <= DDCKEY_SRCOVERLAY; ckeyflag <<= 1)
    {
        hr = IDirectDrawSurface7_GetColorKey(&src->IDirectDrawSurface7_iface, ckeyflag, &ddckey);

        if (SUCCEEDED(hr))
        {
            IDirectDrawSurface7_SetColorKey(&dest->IDirectDrawSurface7_iface, ckeyflag, &ddckey);
        }
    }

    src_level = src;
    dest_level = dest;

    point = *DestPoint;
    src_rect = *SrcRect;

    for (;src_level && dest_level;)
    {
        if (src_level->surface_desc.dwWidth == dest_level->surface_desc.dwWidth &&
            src_level->surface_desc.dwHeight == dest_level->surface_desc.dwHeight)
        {
            UINT src_w = src_rect.right - src_rect.left;
            UINT src_h = src_rect.bottom - src_rect.top;
            RECT dst_rect = {point.x, point.y, point.x + src_w, point.y + src_h};

            if (FAILED(hr = wined3d_surface_blt(dest_level->wined3d_surface, &dst_rect,
                    src_level->wined3d_surface, &src_rect, 0, NULL, WINED3D_TEXF_POINT)))
                ERR("Blit failed, hr %#x.\n", hr);

            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
            IDirectDrawSurface7_GetAttachedSurface(&dest_level->IDirectDrawSurface7_iface, &ddsd.ddsCaps, &temp);

            if (dest_level != dest) IDirectDrawSurface7_Release(&dest_level->IDirectDrawSurface7_iface);

            dest_level = unsafe_impl_from_IDirectDrawSurface7(temp);
        }

        ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
        ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
        IDirectDrawSurface7_GetAttachedSurface(&src_level->IDirectDrawSurface7_iface, &ddsd.ddsCaps, &temp);

        if (src_level != src) IDirectDrawSurface7_Release(&src_level->IDirectDrawSurface7_iface);

        src_level = unsafe_impl_from_IDirectDrawSurface7(temp);

        point.x /= 2;
        point.y /= 2;

        src_rect.top /= 2;
        src_rect.left /= 2;
        src_rect.right = (src_rect.right + 1) / 2;
        src_rect.bottom = (src_rect.bottom + 1) / 2;
    }

    if (src_level && src_level != src) IDirectDrawSurface7_Release(&src_level->IDirectDrawSurface7_iface);
    if (dest_level && dest_level != dest) IDirectDrawSurface7_Release(&dest_level->IDirectDrawSurface7_iface);
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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    IDirectDrawSurfaceImpl *dest = unsafe_impl_from_IDirectDrawSurface7(DestTex);
    IDirectDrawSurfaceImpl *src = unsafe_impl_from_IDirectDrawSurface7(SrcTex);
    POINT destpoint;
    RECT srcrect;

    TRACE("iface %p, dst_texture %p, dst_pos %s, src_texture %p, src_rect %s, flags %#x.\n",
            iface, DestTex, wine_dbgstr_point(DestPoint), SrcTex, wine_dbgstr_rect(SrcRect), Flags);

    if( (!src) || (!dest) )
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

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
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    /* Must be top level surfaces. */
    if (src->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL ||
        dest->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL)
    {
        wined3d_mutex_unlock();
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
            wined3d_mutex_unlock();
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
                            wined3d_mutex_unlock();
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
                        IDirectDrawSurface7_GetAttachedSurface(&src->IDirectDrawSurface7_iface, &ddsd.ddsCaps, &temp);

                        if (src_face != src) IDirectDrawSurface7_Release(&src_face->IDirectDrawSurface7_iface);

                        src_face = unsafe_impl_from_IDirectDrawSurface7(temp);
                    }
                    else
                    {
                        if (src_face != src) IDirectDrawSurface7_Release(&src_face->IDirectDrawSurface7_iface);

                        src_face = NULL;
                    }
                }

                if (dest_face_flag < DDSCAPS2_CUBEMAP_NEGATIVEZ)
                {
                    ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
                    ddsd.ddsCaps.dwCaps2 = DDSCAPS2_CUBEMAP | (dest_face_flag << 1);
                    IDirectDrawSurface7_GetAttachedSurface(&dest->IDirectDrawSurface7_iface, &ddsd.ddsCaps, &temp);

                    if (dest_face != dest) IDirectDrawSurface7_Release(&dest_face->IDirectDrawSurface7_iface);

                    dest_face = unsafe_impl_from_IDirectDrawSurface7(temp);
                }
                else
                {
                    if (dest_face != dest) IDirectDrawSurface7_Release(&dest_face->IDirectDrawSurface7_iface);

                    dest_face = NULL;
                }
            }

            if (i == 0)
            {
                /* Native returns error if src faces are not subset of dest faces. */
                if (src_face)
                {
                    wined3d_mutex_unlock();
                    return DDERR_INVALIDPARAMS;
                }
            }
        }

        wined3d_mutex_unlock();
        return D3D_OK;
    }
    else if (dest->surface_desc.ddsCaps.dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    /* Handle non cube map textures. */

    /* Destination mip levels must be subset of source mip levels. */
    if (!is_mip_level_subset(dest, src))
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    copy_mipmap_chain(This, dest, src, &destpoint, &srcrect);

    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, light_idx %u, enabled %#x.\n", iface, LightIndex, Enable);

    wined3d_mutex_lock();
    hr = wined3d_device_set_light_enable(This->wined3d_device, LightIndex, Enable);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, light_idx %u, enabled %p.\n", iface, LightIndex, Enable);

    if(!Enable)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = wined3d_device_get_light_enable(This->wined3d_device, LightIndex, Enable);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, idx %u, plane %p.\n", iface, Index, PlaneEquation);

    if(!PlaneEquation)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = wined3d_device_set_clip_plane(This->wined3d_device, Index, PlaneEquation);
    wined3d_mutex_unlock();

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
    IDirect3DDeviceImpl *This = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, idx %u, plane %p.\n", iface, Index, PlaneEquation);

    if(!PlaneEquation)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = wined3d_device_get_clip_plane(This->wined3d_device, Index, PlaneEquation);
    wined3d_mutex_unlock();

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
    TRACE("iface %p, info_id %#x, info %p, info_size %u.\n",
            iface, DevInfoID, DevInfoStruct, Size);

    if (TRACE_ON(ddraw))
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
static const struct IDirect3DDevice7Vtbl d3d_device7_fpu_setup_vtbl =
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

static const struct IDirect3DDevice7Vtbl d3d_device7_fpu_preserve_vtbl =
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

static const struct IDirect3DDevice3Vtbl d3d_device3_vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DDeviceImpl_3_QueryInterface,
    IDirect3DDeviceImpl_3_AddRef,
    IDirect3DDeviceImpl_3_Release,
    /*** IDirect3DDevice3 ***/
    IDirect3DDeviceImpl_3_GetCaps,
    IDirect3DDeviceImpl_3_GetStats,
    IDirect3DDeviceImpl_3_AddViewport,
    IDirect3DDeviceImpl_3_DeleteViewport,
    IDirect3DDeviceImpl_3_NextViewport,
    IDirect3DDeviceImpl_3_EnumTextureFormats,
    IDirect3DDeviceImpl_3_BeginScene,
    IDirect3DDeviceImpl_3_EndScene,
    IDirect3DDeviceImpl_3_GetDirect3D,
    IDirect3DDeviceImpl_3_SetCurrentViewport,
    IDirect3DDeviceImpl_3_GetCurrentViewport,
    IDirect3DDeviceImpl_3_SetRenderTarget,
    IDirect3DDeviceImpl_3_GetRenderTarget,
    IDirect3DDeviceImpl_3_Begin,
    IDirect3DDeviceImpl_3_BeginIndexed,
    IDirect3DDeviceImpl_3_Vertex,
    IDirect3DDeviceImpl_3_Index,
    IDirect3DDeviceImpl_3_End,
    IDirect3DDeviceImpl_3_GetRenderState,
    IDirect3DDeviceImpl_3_SetRenderState,
    IDirect3DDeviceImpl_3_GetLightState,
    IDirect3DDeviceImpl_3_SetLightState,
    IDirect3DDeviceImpl_3_SetTransform,
    IDirect3DDeviceImpl_3_GetTransform,
    IDirect3DDeviceImpl_3_MultiplyTransform,
    IDirect3DDeviceImpl_3_DrawPrimitive,
    IDirect3DDeviceImpl_3_DrawIndexedPrimitive,
    IDirect3DDeviceImpl_3_SetClipStatus,
    IDirect3DDeviceImpl_3_GetClipStatus,
    IDirect3DDeviceImpl_3_DrawPrimitiveStrided,
    IDirect3DDeviceImpl_3_DrawIndexedPrimitiveStrided,
    IDirect3DDeviceImpl_3_DrawPrimitiveVB,
    IDirect3DDeviceImpl_3_DrawIndexedPrimitiveVB,
    IDirect3DDeviceImpl_3_ComputeSphereVisibility,
    IDirect3DDeviceImpl_3_GetTexture,
    IDirect3DDeviceImpl_3_SetTexture,
    IDirect3DDeviceImpl_3_GetTextureStageState,
    IDirect3DDeviceImpl_3_SetTextureStageState,
    IDirect3DDeviceImpl_3_ValidateDevice
};

static const struct IDirect3DDevice2Vtbl d3d_device2_vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DDeviceImpl_2_QueryInterface,
    IDirect3DDeviceImpl_2_AddRef,
    IDirect3DDeviceImpl_2_Release,
    /*** IDirect3DDevice2 ***/
    IDirect3DDeviceImpl_2_GetCaps,
    IDirect3DDeviceImpl_2_SwapTextureHandles,
    IDirect3DDeviceImpl_2_GetStats,
    IDirect3DDeviceImpl_2_AddViewport,
    IDirect3DDeviceImpl_2_DeleteViewport,
    IDirect3DDeviceImpl_2_NextViewport,
    IDirect3DDeviceImpl_2_EnumTextureFormats,
    IDirect3DDeviceImpl_2_BeginScene,
    IDirect3DDeviceImpl_2_EndScene,
    IDirect3DDeviceImpl_2_GetDirect3D,
    IDirect3DDeviceImpl_2_SetCurrentViewport,
    IDirect3DDeviceImpl_2_GetCurrentViewport,
    IDirect3DDeviceImpl_2_SetRenderTarget,
    IDirect3DDeviceImpl_2_GetRenderTarget,
    IDirect3DDeviceImpl_2_Begin,
    IDirect3DDeviceImpl_2_BeginIndexed,
    IDirect3DDeviceImpl_2_Vertex,
    IDirect3DDeviceImpl_2_Index,
    IDirect3DDeviceImpl_2_End,
    IDirect3DDeviceImpl_2_GetRenderState,
    IDirect3DDeviceImpl_2_SetRenderState,
    IDirect3DDeviceImpl_2_GetLightState,
    IDirect3DDeviceImpl_2_SetLightState,
    IDirect3DDeviceImpl_2_SetTransform,
    IDirect3DDeviceImpl_2_GetTransform,
    IDirect3DDeviceImpl_2_MultiplyTransform,
    IDirect3DDeviceImpl_2_DrawPrimitive,
    IDirect3DDeviceImpl_2_DrawIndexedPrimitive,
    IDirect3DDeviceImpl_2_SetClipStatus,
    IDirect3DDeviceImpl_2_GetClipStatus
};

static const struct IDirect3DDeviceVtbl d3d_device1_vtbl =
{
    /*** IUnknown Methods ***/
    IDirect3DDeviceImpl_1_QueryInterface,
    IDirect3DDeviceImpl_1_AddRef,
    IDirect3DDeviceImpl_1_Release,
    /*** IDirect3DDevice1 ***/
    IDirect3DDeviceImpl_1_Initialize,
    IDirect3DDeviceImpl_1_GetCaps,
    IDirect3DDeviceImpl_1_SwapTextureHandles,
    IDirect3DDeviceImpl_1_CreateExecuteBuffer,
    IDirect3DDeviceImpl_1_GetStats,
    IDirect3DDeviceImpl_1_Execute,
    IDirect3DDeviceImpl_1_AddViewport,
    IDirect3DDeviceImpl_1_DeleteViewport,
    IDirect3DDeviceImpl_1_NextViewport,
    IDirect3DDeviceImpl_1_Pick,
    IDirect3DDeviceImpl_1_GetPickRecords,
    IDirect3DDeviceImpl_1_EnumTextureFormats,
    IDirect3DDeviceImpl_1_CreateMatrix,
    IDirect3DDeviceImpl_1_SetMatrix,
    IDirect3DDeviceImpl_1_GetMatrix,
    IDirect3DDeviceImpl_1_DeleteMatrix,
    IDirect3DDeviceImpl_1_BeginScene,
    IDirect3DDeviceImpl_1_EndScene,
    IDirect3DDeviceImpl_1_GetDirect3D
};

IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice7(IDirect3DDevice7 *iface)
{
    if (!iface) return NULL;
    assert((iface->lpVtbl == &d3d_device7_fpu_preserve_vtbl) || (iface->lpVtbl == &d3d_device7_fpu_setup_vtbl));
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice7_iface);
}

IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice3(IDirect3DDevice3 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_device3_vtbl);
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice3_iface);
}

IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice2(IDirect3DDevice2 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_device2_vtbl);
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice2_iface);
}

IDirect3DDeviceImpl *unsafe_impl_from_IDirect3DDevice(IDirect3DDevice *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_device1_vtbl);
    return CONTAINING_RECORD(iface, IDirect3DDeviceImpl, IDirect3DDevice_iface);
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
enum wined3d_depth_buffer_type IDirect3DDeviceImpl_UpdateDepthStencil(IDirect3DDeviceImpl *This)
{
    IDirectDrawSurface7 *depthStencil = NULL;
    IDirectDrawSurfaceImpl *dsi;
    static DDSCAPS2 depthcaps = { DDSCAPS_ZBUFFER, 0, 0, {0} };

    IDirectDrawSurface7_GetAttachedSurface(&This->target->IDirectDrawSurface7_iface, &depthcaps, &depthStencil);
    if(!depthStencil)
    {
        TRACE("Setting wined3d depth stencil to NULL\n");
        wined3d_device_set_depth_stencil(This->wined3d_device, NULL);
        return WINED3D_ZB_FALSE;
    }

    dsi = impl_from_IDirectDrawSurface7(depthStencil);
    TRACE("Setting wined3d depth stencil to %p (wined3d %p)\n", dsi, dsi->wined3d_surface);
    wined3d_device_set_depth_stencil(This->wined3d_device, dsi->wined3d_surface);

    IDirectDrawSurface7_Release(depthStencil);
    return WINED3D_ZB_TRUE;
}

HRESULT d3d_device_init(IDirect3DDeviceImpl *device, IDirectDrawImpl *ddraw, IDirectDrawSurfaceImpl *target)
{
    static const D3DMATRIX ident =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    HRESULT hr;

    if (ddraw->cooperative_level & DDSCL_FPUPRESERVE)
        device->IDirect3DDevice7_iface.lpVtbl = &d3d_device7_fpu_preserve_vtbl;
    else
        device->IDirect3DDevice7_iface.lpVtbl = &d3d_device7_fpu_setup_vtbl;

    device->IDirect3DDevice3_iface.lpVtbl = &d3d_device3_vtbl;
    device->IDirect3DDevice2_iface.lpVtbl = &d3d_device2_vtbl;
    device->IDirect3DDevice_iface.lpVtbl = &d3d_device1_vtbl;
    device->ref = 1;
    device->ddraw = ddraw;
    device->target = target;
    list_init(&device->viewport_list);

    if (!ddraw_handle_table_init(&device->handle_table, 64))
    {
        ERR("Failed to initialize handle table.\n");
        return DDERR_OUTOFMEMORY;
    }

    device->legacyTextureBlending = FALSE;
    device->legacy_projection = ident;
    device->legacy_clipspace = ident;

    /* Create an index buffer, it's needed for indexed drawing */
    hr = wined3d_buffer_create_ib(ddraw->wined3d_device, 0x40000 /* Length. Don't know how long it should be */,
            WINED3DUSAGE_DYNAMIC /* Usage */, WINED3D_POOL_DEFAULT, NULL,
            &ddraw_null_wined3d_parent_ops, &device->indexbuffer);
    if (FAILED(hr))
    {
        ERR("Failed to create an index buffer, hr %#x.\n", hr);
        ddraw_handle_table_destroy(&device->handle_table);
        return hr;
    }

    /* This is for convenience. */
    device->wined3d_device = ddraw->wined3d_device;
    wined3d_device_incref(ddraw->wined3d_device);

    /* Render to the back buffer */
    hr = wined3d_device_set_render_target(ddraw->wined3d_device, 0, target->wined3d_surface, TRUE);
    if (FAILED(hr))
    {
        ERR("Failed to set render target, hr %#x.\n", hr);
        wined3d_buffer_decref(device->indexbuffer);
        ddraw_handle_table_destroy(&device->handle_table);
        return hr;
    }

    /* FIXME: This is broken. The target AddRef() makes some sense, because
     * we store a pointer during initialization, but then that's also where
     * the AddRef() should be. We don't store ddraw->d3d_target anywhere. */
    /* AddRef the render target. Also AddRef the render target from ddraw,
     * because if it is released before the app releases the D3D device, the
     * D3D capabilities of wined3d will be uninitialized, which has bad effects.
     *
     * In most cases, those surfaces are the same anyway, but this will simply
     * add another ref which is released when the device is destroyed. */
    IDirectDrawSurface7_AddRef(&target->IDirectDrawSurface7_iface);

    ddraw->d3ddevice = device;

    wined3d_device_set_render_state(ddraw->wined3d_device, WINED3D_RS_ZENABLE,
            IDirect3DDeviceImpl_UpdateDepthStencil(device));

    return D3D_OK;
}
