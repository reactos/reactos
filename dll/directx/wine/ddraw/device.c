/*
 * Copyright (c) 1998-2004 Lionel Ulmer
 * Copyright (c) 2002-2005 Christian Costa
 * Copyright (c) 2006-2009, 2011-2013 Stefan Dösinger
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

#include "ddraw_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);
WINE_DECLARE_DEBUG_CHANNEL(winediag);

/* The device ID */
const GUID IID_D3DDEVICE_WineD3D = {
  0xaef72d43,
  0xb09a,
  0x4b7b,
  { 0xb7,0x98,0xc6,0x8a,0x77,0x2d,0x72,0x2a }
};

static inline void set_fpu_control_word(WORD fpucw)
{
#if defined(__i386__) && defined(_MSC_VER)
    __asm fldcw fpucw;
#elif defined(__i386__) || (defined(__x86_64__) && !defined(__arm64ec__) && (defined(__GNUC__) || defined(__clang__)))
    __asm__ volatile ("fldcw %0" : : "m" (fpucw));
#endif
}

static inline WORD d3d_fpu_setup(void)
{
    WORD oldcw;

#if defined(__i386__) && defined(_MSC_VER)
    __asm fnstcw oldcw;
#elif defined(__i386__) || (defined(__x86_64__) && !defined(__arm64ec__) && (defined(__GNUC__) || defined(__clang__)))
    __asm__ volatile ("fnstcw %0" : "=m" (oldcw));
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

static enum wined3d_render_state wined3d_render_state_from_ddraw(D3DRENDERSTATETYPE state)
{
    switch (state)
    {
        case D3DRENDERSTATE_ZBIAS:
            return WINED3D_RS_DEPTHBIAS;
        case D3DRENDERSTATE_EDGEANTIALIAS:
            return WINED3D_RS_ANTIALIASEDLINEENABLE;
        default:
            return (enum wined3d_render_state)state;
    }
}

static enum wined3d_transform_state wined3d_transform_state_from_ddraw(D3DTRANSFORMSTATETYPE state)
{
    switch (state)
    {
        case D3DTRANSFORMSTATE_WORLD:
            return WINED3D_TS_WORLD_MATRIX(0);
        case D3DTRANSFORMSTATE_WORLD1:
            return WINED3D_TS_WORLD_MATRIX(1);
        case D3DTRANSFORMSTATE_WORLD2:
            return WINED3D_TS_WORLD_MATRIX(2);
        case D3DTRANSFORMSTATE_WORLD3:
            return WINED3D_TS_WORLD_MATRIX(3);
        default:
            return (enum wined3d_transform_state)state;
    }
}

static enum wined3d_primitive_type wined3d_primitive_type_from_ddraw(D3DPRIMITIVETYPE type)
{
    return (enum wined3d_primitive_type)type;
}

static enum wined3d_stateblock_type wined3d_stateblock_type_from_ddraw(D3DSTATEBLOCKTYPE type)
{
    return (enum wined3d_stateblock_type)type;
}

static inline struct d3d_device *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct d3d_device, IUnknown_inner);
}

static HRESULT WINAPI d3d_device_inner_QueryInterface(IUnknown *iface, REFIID riid, void **out)
{
    struct d3d_device *device = impl_from_IUnknown(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    if (!riid)
    {
        *out = NULL;
        return DDERR_INVALIDPARAMS;
    }

    if (IsEqualGUID(&IID_IUnknown, riid))
    {
        IDirect3DDevice7_AddRef(&device->IDirect3DDevice7_iface);
        *out = &device->IDirect3DDevice7_iface;
        return S_OK;
    }

    if (device->version == 7)
    {
        if (IsEqualGUID(&IID_IDirect3DDevice7, riid))
        {
            IDirect3DDevice7_AddRef(&device->IDirect3DDevice7_iface);
            *out = &device->IDirect3DDevice7_iface;
            return S_OK;
        }
    }
    else
    {
        if (IsEqualGUID(&IID_IDirect3DDevice3, riid) && device->version == 3)
        {
            IDirect3DDevice3_AddRef(&device->IDirect3DDevice3_iface);
            *out = &device->IDirect3DDevice3_iface;
            return S_OK;
        }

        if (IsEqualGUID(&IID_IDirect3DDevice2, riid) && device->version >= 2)
        {
            IDirect3DDevice2_AddRef(&device->IDirect3DDevice2_iface);
            *out = &device->IDirect3DDevice2_iface;
            return S_OK;
        }

        if (IsEqualGUID(&IID_IDirect3DDevice, riid))
        {
            IDirect3DDevice_AddRef(&device->IDirect3DDevice_iface);
            *out = &device->IDirect3DDevice_iface;
            return S_OK;
        }
    }

    WARN("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(riid));

    *out = NULL;
    return E_NOINTERFACE;
}

static HRESULT WINAPI d3d_device7_QueryInterface(IDirect3DDevice7 *iface, REFIID riid, void **out)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IUnknown_QueryInterface(device->outer_unknown, riid, out);
}

static HRESULT WINAPI d3d_device3_QueryInterface(IDirect3DDevice3 *iface, REFIID riid, void **out)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IUnknown_QueryInterface(device->outer_unknown, riid, out);
}

static HRESULT WINAPI d3d_device2_QueryInterface(IDirect3DDevice2 *iface, REFIID riid, void **out)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IUnknown_QueryInterface(device->outer_unknown, riid, out);
}

static HRESULT WINAPI d3d_device1_QueryInterface(IDirect3DDevice *iface, REFIID riid, void **out)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, riid %s, out %p.\n", iface, debugstr_guid(riid), out);

    return IUnknown_QueryInterface(device->outer_unknown, riid, out);
}

static ULONG WINAPI d3d_device_inner_AddRef(IUnknown *iface)
{
    struct d3d_device *device = impl_from_IUnknown(iface);
    ULONG ref = InterlockedIncrement(&device->ref);

    TRACE("%p increasing refcount to %lu.\n", device, ref);

    return ref;
}

static ULONG WINAPI d3d_device7_AddRef(IDirect3DDevice7 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(device->outer_unknown);
}

static ULONG WINAPI d3d_device3_AddRef(IDirect3DDevice3 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(device->outer_unknown);
}

static ULONG WINAPI d3d_device2_AddRef(IDirect3DDevice2 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(device->outer_unknown);
}

static ULONG WINAPI d3d_device1_AddRef(IDirect3DDevice *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_AddRef(device->outer_unknown);
}

static ULONG WINAPI d3d_device_inner_Release(IUnknown *iface)
{
    struct d3d_device *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    IUnknown *rt_iface;

    TRACE("%p decreasing refcount to %lu.\n", This, ref);

    /* This method doesn't destroy the wined3d device, because it's still in
     * use for 2D rendering. IDirectDrawSurface7::Release will destroy the
     * wined3d device when the render target is released. */
    if (!ref)
    {
        static struct wined3d_rendertarget_view *const null_rtv;
        DWORD i;
        struct list *vp_entry, *vp_entry2;

        wined3d_mutex_lock();

        /* There is no need to unset any resources here, wined3d will take
         * care of that on uninit_3d(). */

        wined3d_streaming_buffer_cleanup(&This->index_buffer);
        wined3d_streaming_buffer_cleanup(&This->vertex_buffer);

        wined3d_device_context_set_rendertarget_views(This->immediate_context, 0, 1, &null_rtv, FALSE);

        /* Release the wined3d device. This won't destroy it. */
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

                case DDRAW_HANDLE_STATEBLOCK:
                {
                    /* No FIXME here because this might happen because of sloppy applications. */
                    WARN("Leftover stateblock handle %#lx (%p), deleting.\n", i + 1, entry->object);
                    IDirect3DDevice7_DeleteStateBlock(&This->IDirect3DDevice7_iface, i + 1);
                    break;
                }

                default:
                    FIXME("Handle %#lx (%p) has unknown type %#x.\n", i + 1, entry->object, entry->type);
                    break;
            }
        }

        ddraw_handle_table_destroy(&This->handle_table);

        LIST_FOR_EACH_SAFE(vp_entry, vp_entry2, &This->viewport_list)
        {
            struct d3d_viewport *vp = LIST_ENTRY(vp_entry, struct d3d_viewport, entry);
            IDirect3DDevice3_DeleteViewport(&This->IDirect3DDevice3_iface, &vp->IDirect3DViewport3_iface);
        }

        wined3d_stateblock_decref(This->state);
        if (This->recording)
            wined3d_stateblock_decref(This->recording);

        /* Releasing the render target below may release the last reference to the ddraw object. Detach
         * the device from it before so it doesn't try to save / restore state on the teared down device. */
        if (This->ddraw)
        {
            if (This->ddraw->device_last_applied_state == This)
                This->ddraw->device_last_applied_state = NULL;
            list_remove(&This->ddraw_entry);
            This->ddraw = NULL;
        }

        TRACE("Releasing render target %p.\n", This->rt_iface);
        rt_iface = This->rt_iface;
        This->rt_iface = NULL;
        This->target = NULL;
        This->target_ds = NULL;
        if (This->version != 1)
            IUnknown_Release(rt_iface);
        TRACE("Render target release done.\n");

        /* Now free the structure */
        free(This);
        wined3d_mutex_unlock();
    }

    TRACE("Done\n");
    return ref;
}

static ULONG WINAPI d3d_device7_Release(IDirect3DDevice7 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(device->outer_unknown);
}

static ULONG WINAPI d3d_device3_Release(IDirect3DDevice3 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(device->outer_unknown);
}

static ULONG WINAPI d3d_device2_Release(IDirect3DDevice2 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(device->outer_unknown);
}

static ULONG WINAPI d3d_device1_Release(IDirect3DDevice *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p.\n", iface);

    return IUnknown_Release(device->outer_unknown);
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
static HRESULT WINAPI d3d_device1_Initialize(IDirect3DDevice *iface,
        IDirect3D *d3d, GUID *guid, D3DDEVICEDESC *device_desc)
{
    /* It shouldn't be crucial, but print a FIXME, I'm interested if
     * any game calls it and when. */
    FIXME("iface %p, d3d %p, guid %s, device_desc %p nop!\n",
            iface, d3d, debugstr_guid(guid), device_desc);

    return D3D_OK;
}

static HRESULT d3d_device7_GetCaps(IDirect3DDevice7 *iface, D3DDEVICEDESC7 *device_desc)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, device_desc %p.\n", iface, device_desc);

    if (!device_desc)
    {
        WARN("device_desc is NULL, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }

    /* Call the same function used by IDirect3D, this saves code */
    return ddraw_get_d3dcaps(device->ddraw, device_desc);
}

static HRESULT WINAPI d3d_device7_GetCaps_FPUSetup(IDirect3DDevice7 *iface, D3DDEVICEDESC7 *desc)
{
    return d3d_device7_GetCaps(iface, desc);
}

static HRESULT WINAPI d3d_device7_GetCaps_FPUPreserve(IDirect3DDevice7 *iface, D3DDEVICEDESC7 *desc)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetCaps(iface, desc);
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

static HRESULT WINAPI d3d_device3_GetCaps(IDirect3DDevice3 *iface,
        D3DDEVICEDESC *HWDesc, D3DDEVICEDESC *HelDesc)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    D3DDEVICEDESC7 desc7;
    D3DDEVICEDESC desc1;
    HRESULT hr;

    TRACE("iface %p, hw_desc %p, hel_desc %p.\n", iface, HWDesc, HelDesc);

    if (!HWDesc)
    {
        WARN("HWDesc is NULL, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }
    if (!check_d3ddevicedesc_size(HWDesc->dwSize))
    {
        WARN("HWDesc->dwSize is %lu, returning DDERR_INVALIDPARAMS.\n", HWDesc->dwSize);
        return DDERR_INVALIDPARAMS;
    }
    if (!HelDesc)
    {
        WARN("HelDesc is NULL, returning DDERR_INVALIDPARAMS.\n");
        return DDERR_INVALIDPARAMS;
    }
    if (!check_d3ddevicedesc_size(HelDesc->dwSize))
    {
        WARN("HelDesc->dwSize is %lu, returning DDERR_INVALIDPARAMS.\n", HelDesc->dwSize);
        return DDERR_INVALIDPARAMS;
    }

    if (FAILED(hr = ddraw_get_d3dcaps(device->ddraw, &desc7)))
        return hr;

    ddraw_d3dcaps1_from_7(&desc1, &desc7);
    DD_STRUCT_COPY_BYSIZE(HWDesc, &desc1);
    DD_STRUCT_COPY_BYSIZE(HelDesc, &desc1);
    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_GetCaps(IDirect3DDevice2 *iface,
        D3DDEVICEDESC *hw_desc, D3DDEVICEDESC *hel_desc)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, hw_desc %p, hel_desc %p.\n", iface, hw_desc, hel_desc);

    return d3d_device3_GetCaps(&device->IDirect3DDevice3_iface, hw_desc, hel_desc);
}

static HRESULT WINAPI d3d_device1_GetCaps(IDirect3DDevice *iface,
        D3DDEVICEDESC *hw_desc, D3DDEVICEDESC *hel_desc)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, hw_desc %p, hel_desc %p.\n", iface, hw_desc, hel_desc);

    return d3d_device3_GetCaps(&device->IDirect3DDevice3_iface, hw_desc, hel_desc);
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
static HRESULT WINAPI d3d_device2_SwapTextureHandles(IDirect3DDevice2 *iface,
        IDirect3DTexture2 *tex1, IDirect3DTexture2 *tex2)
{
    struct ddraw_surface *surf1 = unsafe_impl_from_IDirect3DTexture2(tex1);
    struct ddraw_surface *surf2 = unsafe_impl_from_IDirect3DTexture2(tex2);
    DWORD h1, h2, h;
    HRESULT hr;

    TRACE("iface %p, tex1 %p, tex2 %p.\n", iface, tex1, tex2);

    if (!surf1->Handle || !surf2->Handle)
        return E_INVALIDARG;

    if (FAILED(hr = IDirect3DDevice2_GetRenderState(iface, D3DRENDERSTATE_TEXTUREHANDLE, &h)))
        return hr;

    wined3d_mutex_lock();

    h1 = surf1->Handle - 1;
    h2 = surf2->Handle - 1;
    global_handle_table.entries[h1].object = surf2;
    global_handle_table.entries[h2].object = surf1;
    surf2->Handle = h1 + 1;
    surf1->Handle = h2 + 1;

    wined3d_mutex_unlock();

    if ((h == surf1->Handle || h == surf2->Handle)
            && FAILED(hr = IDirect3DDevice2_SetRenderState(iface, D3DRENDERSTATE_TEXTUREHANDLE, h)))
        return hr;
    return D3D_OK;
}

static HRESULT WINAPI d3d_device1_SwapTextureHandles(IDirect3DDevice *iface,
        IDirect3DTexture *tex1, IDirect3DTexture *tex2)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);
    struct ddraw_surface *surf1 = unsafe_impl_from_IDirect3DTexture(tex1);
    struct ddraw_surface *surf2 = unsafe_impl_from_IDirect3DTexture(tex2);
    IDirect3DTexture2 *t1 = surf1 ? &surf1->IDirect3DTexture2_iface : NULL;
    IDirect3DTexture2 *t2 = surf2 ? &surf2->IDirect3DTexture2_iface : NULL;

    TRACE("iface %p, tex1 %p, tex2 %p.\n", iface, tex1, tex2);

    return d3d_device2_SwapTextureHandles(&device->IDirect3DDevice2_iface, t1, t2);
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
static HRESULT WINAPI d3d_device3_GetStats(IDirect3DDevice3 *iface, D3DSTATS *Stats)
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

static HRESULT WINAPI d3d_device2_GetStats(IDirect3DDevice2 *iface, D3DSTATS *stats)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, stats %p.\n", iface, stats);

    return d3d_device3_GetStats(&device->IDirect3DDevice3_iface, stats);
}

static HRESULT WINAPI d3d_device1_GetStats(IDirect3DDevice *iface, D3DSTATS *stats)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, stats %p.\n", iface, stats);

    return d3d_device3_GetStats(&device->IDirect3DDevice3_iface, stats);
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
static HRESULT WINAPI d3d_device1_CreateExecuteBuffer(IDirect3DDevice *iface,
        D3DEXECUTEBUFFERDESC *buffer_desc, IDirect3DExecuteBuffer **ExecuteBuffer, IUnknown *outer_unknown)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);
    struct d3d_execute_buffer *object;
    HRESULT hr;

    TRACE("iface %p, buffer_desc %p, buffer %p, outer_unknown %p.\n",
            iface, buffer_desc, ExecuteBuffer, outer_unknown);

    if (outer_unknown)
        return CLASS_E_NOAGGREGATION;

    /* Allocate the new Execute Buffer */
    if (!(object = calloc(1, sizeof(*object))))
    {
        ERR("Failed to allocate execute buffer memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    hr = d3d_execute_buffer_init(object, device, buffer_desc);
    if (FAILED(hr))
    {
        WARN("Failed to initialize execute buffer, hr %#lx.\n", hr);
        free(object);
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
static HRESULT WINAPI d3d_device1_Execute(IDirect3DDevice *iface,
        IDirect3DExecuteBuffer *ExecuteBuffer, IDirect3DViewport *viewport, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);
    struct d3d_execute_buffer *buffer = unsafe_impl_from_IDirect3DExecuteBuffer(ExecuteBuffer);
    struct d3d_viewport *viewport_impl = unsafe_impl_from_IDirect3DViewport(viewport);
    HRESULT hr;

    TRACE("iface %p, buffer %p, viewport %p, flags %#lx.\n", iface, ExecuteBuffer, viewport, flags);

    if(!buffer)
        return DDERR_INVALIDPARAMS;

    if (FAILED(hr = IDirect3DDevice3_SetCurrentViewport
            (&device->IDirect3DDevice3_iface, &viewport_impl->IDirect3DViewport3_iface)))
        return hr;

    /* Execute... */
    wined3d_mutex_lock();
    hr = d3d_execute_buffer_execute(buffer, device);
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
static HRESULT WINAPI d3d_device3_AddViewport(IDirect3DDevice3 *iface, IDirect3DViewport3 *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport3(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    /* Sanity check */
    if(!vp)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    IDirect3DViewport3_AddRef(viewport);
    list_add_head(&device->viewport_list, &vp->entry);
    /* Viewport must be usable for Clear() after AddViewport, so set active_device here. */
    vp->active_device = device;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_AddViewport(IDirect3DDevice2 *iface,
        IDirect3DViewport2 *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport2(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    return d3d_device3_AddViewport(&device->IDirect3DDevice3_iface, &vp->IDirect3DViewport3_iface);
}

static HRESULT WINAPI d3d_device1_AddViewport(IDirect3DDevice *iface, IDirect3DViewport *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    return d3d_device3_AddViewport(&device->IDirect3DDevice3_iface, &vp->IDirect3DViewport3_iface);
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
static HRESULT WINAPI d3d_device3_DeleteViewport(IDirect3DDevice3 *iface, IDirect3DViewport3 *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport3(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    if (!vp)
    {
        WARN("NULL viewport, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();

    if (vp->active_device != device)
    {
        WARN("Viewport %p active device is %p.\n", vp, vp->active_device);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (device->current_viewport == vp)
    {
        TRACE("Deleting current viewport, unsetting and releasing.\n");

        viewport_deactivate(vp);
        IDirect3DViewport3_Release(viewport);
        device->current_viewport = NULL;
    }

    vp->active_device = NULL;
    list_remove(&vp->entry);

    IDirect3DViewport3_Release(viewport);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_DeleteViewport(IDirect3DDevice2 *iface, IDirect3DViewport2 *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport2(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    return d3d_device3_DeleteViewport(&device->IDirect3DDevice3_iface,
            vp ? &vp->IDirect3DViewport3_iface : NULL);
}

static HRESULT WINAPI d3d_device1_DeleteViewport(IDirect3DDevice *iface, IDirect3DViewport *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    return d3d_device3_DeleteViewport(&device->IDirect3DDevice3_iface,
            vp ? &vp->IDirect3DViewport3_iface : NULL);
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
static HRESULT WINAPI d3d_device3_NextViewport(IDirect3DDevice3 *iface,
        IDirect3DViewport3 *Viewport3, IDirect3DViewport3 **lplpDirect3DViewport3, DWORD flags)
{
    struct d3d_device *This = impl_from_IDirect3DDevice3(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport3(Viewport3);
    struct d3d_viewport *next;
    struct list *entry;

    TRACE("iface %p, viewport %p, next %p, flags %#lx.\n",
            iface, Viewport3, lplpDirect3DViewport3, flags);

    if(!vp)
    {
        *lplpDirect3DViewport3 = NULL;
        return DDERR_INVALIDPARAMS;
    }


    wined3d_mutex_lock();
    switch (flags)
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
            WARN("Invalid flags %#lx.\n", flags);
            *lplpDirect3DViewport3 = NULL;
            wined3d_mutex_unlock();
            return DDERR_INVALIDPARAMS;
    }

    if (entry)
    {
        next = LIST_ENTRY(entry, struct d3d_viewport, entry);
        *lplpDirect3DViewport3 = &next->IDirect3DViewport3_iface;
    }
    else
        *lplpDirect3DViewport3 = NULL;

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_NextViewport(IDirect3DDevice2 *iface,
        IDirect3DViewport2 *viewport, IDirect3DViewport2 **next, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport2(viewport);
    IDirect3DViewport3 *res;
    HRESULT hr;

    TRACE("iface %p, viewport %p, next %p, flags %#lx.\n",
            iface, viewport, next, flags);

    hr = d3d_device3_NextViewport(&device->IDirect3DDevice3_iface,
            &vp->IDirect3DViewport3_iface, &res, flags);
    *next = (IDirect3DViewport2 *)res;
    return hr;
}

static HRESULT WINAPI d3d_device1_NextViewport(IDirect3DDevice *iface,
        IDirect3DViewport *viewport, IDirect3DViewport **next, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport(viewport);
    IDirect3DViewport3 *res;
    HRESULT hr;

    TRACE("iface %p, viewport %p, next %p, flags %#lx.\n",
            iface, viewport, next, flags);

    hr = d3d_device3_NextViewport(&device->IDirect3DDevice3_iface,
            &vp->IDirect3DViewport3_iface, &res, flags);
    *next = (IDirect3DViewport *)res;
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
static HRESULT WINAPI d3d_device1_Pick(IDirect3DDevice *iface, IDirect3DExecuteBuffer *buffer,
        IDirect3DViewport *viewport, DWORD flags, D3DRECT *rect)
{
    FIXME("iface %p, buffer %p, viewport %p, flags %#lx, rect %s stub!\n",
            iface, buffer, viewport, flags, wine_dbgstr_rect((RECT *)rect));

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
static HRESULT WINAPI d3d_device1_GetPickRecords(IDirect3DDevice *iface,
        DWORD *count, D3DPICKRECORD *records)
{
    FIXME("iface %p, count %p, records %p stub!\n", iface, count, records);

    return D3D_OK;
}

/*****************************************************************************
 * IDirect3DDevice7::EnumTextureformats
 *
 * Enumerates the supported texture formats. It checks against a list of all possible
 * formats to see if WineD3D supports it. If so, then it is passed to the app.
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
static HRESULT d3d_device7_EnumTextureFormats(IDirect3DDevice7 *iface,
        LPD3DENUMPIXELFORMATSCALLBACK callback, void *context)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
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
        WINED3DFMT_DXT2,
        WINED3DFMT_DXT3,
        WINED3DFMT_DXT4,
        WINED3DFMT_DXT5,
    };

    static const enum wined3d_format_id BumpFormatList[] =
    {
        WINED3DFMT_R8G8_SNORM,
        WINED3DFMT_R5G5_SNORM_L6_UNORM,
        WINED3DFMT_R8G8_SNORM_L8X8_UNORM,
        WINED3DFMT_R10G11B11_SNORM,
        WINED3DFMT_R10G10B10_SNORM_A2_UNORM
    };

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    if (!callback)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    memset(&mode, 0, sizeof(mode));
    if (FAILED(hr = wined3d_output_get_display_mode(device->ddraw->wined3d_output, &mode, NULL)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to get output display mode, hr %#lx.\n", hr);
        return hr;
    }

    for (i = 0; i < ARRAY_SIZE(FormatList); ++i)
    {
        if (wined3d_check_device_format(device->ddraw->wined3d, device->ddraw->wined3d_adapter,
                WINED3D_DEVICE_TYPE_HAL, mode.format_id, 0, WINED3D_BIND_SHADER_RESOURCE,
                WINED3D_RTYPE_TEXTURE_2D, FormatList[i]) == D3D_OK)
        {
            DDPIXELFORMAT pformat;

            memset(&pformat, 0, sizeof(pformat));
            pformat.dwSize = sizeof(pformat);
            ddrawformat_from_wined3dformat(&pformat, FormatList[i]);

            TRACE("Enumerating WineD3DFormat %d\n", FormatList[i]);
            hr = callback(&pformat, context);
            if(hr != DDENUMRET_OK)
            {
                TRACE("Format enumeration cancelled by application\n");
                wined3d_mutex_unlock();
                return D3D_OK;
            }
        }
    }

    for (i = 0; i < ARRAY_SIZE(BumpFormatList); ++i)
    {
        if (wined3d_check_device_format(device->ddraw->wined3d, device->ddraw->wined3d_adapter,
                WINED3D_DEVICE_TYPE_HAL, mode.format_id, WINED3DUSAGE_QUERY_LEGACYBUMPMAP,
                WINED3D_BIND_SHADER_RESOURCE, WINED3D_RTYPE_TEXTURE_2D, BumpFormatList[i]) == D3D_OK)
        {
            DDPIXELFORMAT pformat;

            memset(&pformat, 0, sizeof(pformat));
            pformat.dwSize = sizeof(pformat);
            ddrawformat_from_wined3dformat(&pformat, BumpFormatList[i]);

            TRACE("Enumerating WineD3DFormat %d\n", BumpFormatList[i]);
            hr = callback(&pformat, context);
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

static HRESULT WINAPI d3d_device7_EnumTextureFormats_FPUSetup(IDirect3DDevice7 *iface,
        LPD3DENUMPIXELFORMATSCALLBACK callback, void *context)
{
    return d3d_device7_EnumTextureFormats(iface, callback, context);
}

static HRESULT WINAPI d3d_device7_EnumTextureFormats_FPUPreserve(IDirect3DDevice7 *iface,
        LPD3DENUMPIXELFORMATSCALLBACK callback, void *context)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_EnumTextureFormats(iface, callback, context);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_EnumTextureFormats(IDirect3DDevice3 *iface,
        LPD3DENUMPIXELFORMATSCALLBACK callback, void *context)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    return IDirect3DDevice7_EnumTextureFormats(&device->IDirect3DDevice7_iface, callback, context);
}

/*****************************************************************************
 * IDirect3DDevice2::EnumTextureformats
 *
 * EnumTextureFormats for Version 1 and 2, see
 * IDirect3DDevice7::EnumTextureFormats for a more detailed description.
 *
 * This version has a different callback and does not enumerate FourCC
 * formats
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_device2_EnumTextureFormats(IDirect3DDevice2 *iface,
        LPD3DENUMTEXTUREFORMATSCALLBACK callback, void *context)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
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

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    if (!callback)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    memset(&mode, 0, sizeof(mode));
    if (FAILED(hr = wined3d_output_get_display_mode(device->ddraw->wined3d_output, &mode, NULL)))
    {
        wined3d_mutex_unlock();
        WARN("Failed to get output display mode, hr %#lx.\n", hr);
        return hr;
    }

    for (i = 0; i < ARRAY_SIZE(FormatList); ++i)
    {
        if (wined3d_check_device_format(device->ddraw->wined3d, device->ddraw->wined3d_adapter,
                WINED3D_DEVICE_TYPE_HAL, mode.format_id, 0, WINED3D_BIND_SHADER_RESOURCE,
                WINED3D_RTYPE_TEXTURE_2D, FormatList[i]) == D3D_OK)
        {
            DDSURFACEDESC sdesc;

            memset(&sdesc, 0, sizeof(sdesc));
            sdesc.dwSize = sizeof(sdesc);
            sdesc.dwFlags = DDSD_PIXELFORMAT | DDSD_CAPS;
            sdesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            sdesc.ddpfPixelFormat.dwSize = sizeof(sdesc.ddpfPixelFormat);
            ddrawformat_from_wined3dformat(&sdesc.ddpfPixelFormat, FormatList[i]);

            TRACE("Enumerating WineD3DFormat %d\n", FormatList[i]);
            hr = callback(&sdesc, context);
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

static HRESULT WINAPI d3d_device1_EnumTextureFormats(IDirect3DDevice *iface,
        LPD3DENUMTEXTUREFORMATSCALLBACK callback, void *context)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, callback %p, context %p.\n", iface, callback, context);

    return d3d_device2_EnumTextureFormats(&device->IDirect3DDevice2_iface, callback, context);
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
static HRESULT WINAPI d3d_device1_CreateMatrix(IDirect3DDevice *iface, D3DMATRIXHANDLE *D3DMatHandle)
{
    D3DMATRIX *matrix;
    DWORD h;

    TRACE("iface %p, matrix_handle %p.\n", iface, D3DMatHandle);

    if(!D3DMatHandle)
        return DDERR_INVALIDPARAMS;

    if (!(matrix = calloc(1, sizeof(*matrix))))
    {
        ERR("Out of memory when allocating a D3DMATRIX\n");
        return DDERR_OUTOFMEMORY;
    }

    wined3d_mutex_lock();

    h = ddraw_allocate_handle(NULL, matrix, DDRAW_HANDLE_MATRIX);
    if (h == DDRAW_INVALID_HANDLE)
    {
        ERR("Failed to allocate a matrix handle.\n");
        free(matrix);
        wined3d_mutex_unlock();
        return DDERR_OUTOFMEMORY;
    }

    *D3DMatHandle = h + 1;

    TRACE(" returning matrix handle %#lx\n", *D3DMatHandle);

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
static HRESULT WINAPI d3d_device1_SetMatrix(IDirect3DDevice *iface,
        D3DMATRIXHANDLE matrix_handle, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);
    D3DMATRIX *m;

    TRACE("iface %p, matrix_handle %#lx, matrix %p.\n", iface, matrix_handle, matrix);

    if (!matrix)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    m = ddraw_get_object(NULL, matrix_handle - 1, DDRAW_HANDLE_MATRIX);
    if (!m)
    {
        WARN("Invalid matrix handle.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (TRACE_ON(ddraw))
        dump_D3DMATRIX(matrix);

    *m = *matrix;

    if (matrix_handle == device->world)
        wined3d_stateblock_set_transform(device->state,
                WINED3D_TS_WORLD_MATRIX(0), (struct wined3d_matrix *)matrix);

    if (matrix_handle == device->view)
        wined3d_stateblock_set_transform(device->state,
                WINED3D_TS_VIEW, (struct wined3d_matrix *)matrix);

    if (matrix_handle == device->proj)
        wined3d_stateblock_set_transform(device->state,
                WINED3D_TS_PROJECTION, (struct wined3d_matrix *)matrix);

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
static HRESULT WINAPI d3d_device1_GetMatrix(IDirect3DDevice *iface,
        D3DMATRIXHANDLE D3DMatHandle, D3DMATRIX *D3DMatrix)
{
    D3DMATRIX *m;

    TRACE("iface %p, matrix_handle %#lx, matrix %p.\n", iface, D3DMatHandle, D3DMatrix);

    if (!D3DMatrix) return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    m = ddraw_get_object(NULL, D3DMatHandle - 1, DDRAW_HANDLE_MATRIX);
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
static HRESULT WINAPI d3d_device1_DeleteMatrix(IDirect3DDevice *iface, D3DMATRIXHANDLE D3DMatHandle)
{
    D3DMATRIX *m;

    TRACE("iface %p, matrix_handle %#lx.\n", iface, D3DMatHandle);

    wined3d_mutex_lock();

    m = ddraw_free_handle(NULL, D3DMatHandle - 1, DDRAW_HANDLE_MATRIX);
    if (!m)
    {
        WARN("Invalid matrix handle.\n");
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_unlock();

    free(m);

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
 *  D3D_OK on success,
 *  D3DERR_SCENE_IN_SCENE if WineD3D returns an error(Only in case of an already
 *  started scene).
 *
 *****************************************************************************/
static HRESULT d3d_device7_BeginScene(IDirect3DDevice7 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_begin_scene(device->wined3d_device);
    wined3d_mutex_unlock();

    if(hr == WINED3D_OK) return D3D_OK;
    else return D3DERR_SCENE_IN_SCENE; /* TODO: Other possible causes of failure */
}

static HRESULT WINAPI d3d_device7_BeginScene_FPUSetup(IDirect3DDevice7 *iface)
{
    return d3d_device7_BeginScene(iface);
}

static HRESULT WINAPI d3d_device7_BeginScene_FPUPreserve(IDirect3DDevice7 *iface)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_BeginScene(iface);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_BeginScene(IDirect3DDevice3 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_BeginScene(&device->IDirect3DDevice7_iface);
}

static HRESULT WINAPI d3d_device2_BeginScene(IDirect3DDevice2 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_BeginScene(&device->IDirect3DDevice7_iface);
}

static HRESULT WINAPI d3d_device1_BeginScene(IDirect3DDevice *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_BeginScene(&device->IDirect3DDevice7_iface);
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
 *  D3D_OK on success,
 *  D3DERR_SCENE_NOT_IN_SCENE is returned if WineD3D returns an error. It does
 *  that only if the scene was already ended.
 *
 *****************************************************************************/
static HRESULT d3d_device7_EndScene(IDirect3DDevice7 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_end_scene(device->wined3d_device);
    wined3d_mutex_unlock();

    if(hr == WINED3D_OK) return D3D_OK;
    else return D3DERR_SCENE_NOT_IN_SCENE;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d_device7_EndScene_FPUSetup(IDirect3DDevice7 *iface)
{
    return d3d_device7_EndScene(iface);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d_device7_EndScene_FPUPreserve(IDirect3DDevice7 *iface)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_EndScene(iface);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d_device3_EndScene(IDirect3DDevice3 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_EndScene(&device->IDirect3DDevice7_iface);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d_device2_EndScene(IDirect3DDevice2 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_EndScene(&device->IDirect3DDevice7_iface);
}

static HRESULT WINAPI DECLSPEC_HOTPATCH d3d_device1_EndScene(IDirect3DDevice *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p.\n", iface);

    return IDirect3DDevice7_EndScene(&device->IDirect3DDevice7_iface);
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
static HRESULT WINAPI d3d_device7_GetDirect3D(IDirect3DDevice7 *iface, IDirect3D7 **d3d)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, d3d %p.\n", iface, d3d);

    if (!d3d)
        return DDERR_INVALIDPARAMS;

    *d3d = &device->ddraw->IDirect3D7_iface;
    IDirect3D7_AddRef(*d3d);

    TRACE("Returning interface %p.\n", *d3d);
    return D3D_OK;
}

static HRESULT WINAPI d3d_device3_GetDirect3D(IDirect3DDevice3 *iface, IDirect3D3 **d3d)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, d3d %p.\n", iface, d3d);

    if (!d3d)
        return DDERR_INVALIDPARAMS;

    *d3d = &device->ddraw->IDirect3D3_iface;
    IDirect3D3_AddRef(*d3d);

    TRACE("Returning interface %p.\n", *d3d);
    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_GetDirect3D(IDirect3DDevice2 *iface, IDirect3D2 **d3d)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, d3d %p.\n", iface, d3d);

    if (!d3d)
        return DDERR_INVALIDPARAMS;

    *d3d = &device->ddraw->IDirect3D2_iface;
    IDirect3D2_AddRef(*d3d);

    TRACE("Returning interface %p.\n", *d3d);
    return D3D_OK;
}

static HRESULT WINAPI d3d_device1_GetDirect3D(IDirect3DDevice *iface, IDirect3D **d3d)
{
    struct d3d_device *device = impl_from_IDirect3DDevice(iface);

    TRACE("iface %p, d3d %p.\n", iface, d3d);

    if (!d3d)
        return DDERR_INVALIDPARAMS;

    *d3d = &device->ddraw->IDirect3D_iface;
    IDirect3D_AddRef(*d3d);

    TRACE("Returning interface %p.\n", *d3d);
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
static HRESULT WINAPI d3d_device3_SetCurrentViewport(IDirect3DDevice3 *iface, IDirect3DViewport3 *viewport)
{
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport3(viewport);
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, viewport %p, current_viewport %p.\n", iface, viewport, device->current_viewport);

    if (!vp)
    {
        WARN("Direct3DViewport3 is NULL.\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    /* Do nothing if the specified viewport is the same as the current one */
    if (device->current_viewport == vp)
    {
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    if (vp->active_device != device)
    {
        WARN("Viewport %p, active device %p.\n", vp, vp->active_device);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    IDirect3DViewport3_AddRef(viewport);
    if (device->current_viewport)
    {
        viewport_deactivate(device->current_viewport);
        IDirect3DViewport3_Release(&device->current_viewport->IDirect3DViewport3_iface);
    }
    device->current_viewport = vp;
    viewport_activate(device->current_viewport, FALSE);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_SetCurrentViewport(IDirect3DDevice2 *iface, IDirect3DViewport2 *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    struct d3d_viewport *vp = unsafe_impl_from_IDirect3DViewport2(viewport);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    return d3d_device3_SetCurrentViewport(&device->IDirect3DDevice3_iface,
            vp ? &vp->IDirect3DViewport3_iface : NULL);
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
static HRESULT WINAPI d3d_device3_GetCurrentViewport(IDirect3DDevice3 *iface, IDirect3DViewport3 **viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    wined3d_mutex_lock();
    if (!device->current_viewport)
    {
        wined3d_mutex_unlock();
        WARN("No current viewport, returning D3DERR_NOCURRENTVIEWPORT\n");
        return D3DERR_NOCURRENTVIEWPORT;
    }

    *viewport = &device->current_viewport->IDirect3DViewport3_iface;
    IDirect3DViewport3_AddRef(*viewport);

    TRACE("Returning interface %p.\n", *viewport);
    wined3d_mutex_unlock();
    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_GetCurrentViewport(IDirect3DDevice2 *iface, IDirect3DViewport2 **viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    return d3d_device3_GetCurrentViewport(&device->IDirect3DDevice3_iface,
            (IDirect3DViewport3 **)viewport);
}

static BOOL validate_surface_palette(struct ddraw_surface *surface)
{
    return !format_is_paletteindexed(&surface->surface_desc.ddpfPixelFormat)
            || surface->palette;
}

static HRESULT d3d_device_set_render_target(struct d3d_device *device,
        struct ddraw_surface *target, IUnknown *rt_iface)
{
    struct wined3d_rendertarget_view *rtv;
    HRESULT hr;

    if (device->rt_iface == rt_iface)
    {
        TRACE("No-op SetRenderTarget operation, not doing anything\n");
        return D3D_OK;
    }
    if (!target)
    {
        WARN("Trying to set render target to NULL.\n");
        return DDERR_INVALIDPARAMS;
    }

    rtv = ddraw_surface_get_rendertarget_view(target);
    if (FAILED(hr = wined3d_device_context_set_rendertarget_views(device->immediate_context, 0, 1, &rtv, FALSE)))
        return hr;

    IUnknown_AddRef(rt_iface);
    IUnknown_Release(device->rt_iface);
    device->rt_iface = rt_iface;
    device->target = target;
    d3d_device_update_depth_stencil(device);

    return D3D_OK;
}

static HRESULT d3d_device7_SetRenderTarget(IDirect3DDevice7 *iface,
        IDirectDrawSurface7 *target, DWORD flags)
{
    struct ddraw_surface *target_impl = unsafe_impl_from_IDirectDrawSurface7(target);
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, target %p, flags %#lx.\n", iface, target, flags);

    wined3d_mutex_lock();

    if (!validate_surface_palette(target_impl))
    {
        WARN("Surface %p has an indexed pixel format, but no palette.\n", target_impl);
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    if (!(target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE))
    {
        WARN("Surface %p is not a render target.\n", target_impl);
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    if (device->hardware_device && !(target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        WARN("Surface %p is not in video memory.\n", target_impl);
        wined3d_mutex_unlock();
        return DDERR_INVALIDPARAMS;
    }

    if (target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
    {
        WARN("Surface %p is a depth buffer.\n", target_impl);
        IDirectDrawSurface7_AddRef(target);
        IUnknown_Release(device->rt_iface);
        device->rt_iface = (IUnknown *)target;
        device->target = NULL;
        device->target_ds = NULL;
        wined3d_mutex_unlock();
        return DDERR_INVALIDPIXELFORMAT;
    }

    hr = d3d_device_set_render_target(device, target_impl, (IUnknown *)target);
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d_device7_SetRenderTarget_FPUSetup(IDirect3DDevice7 *iface,
        IDirectDrawSurface7 *NewTarget, DWORD flags)
{
    return d3d_device7_SetRenderTarget(iface, NewTarget, flags);
}

static HRESULT WINAPI d3d_device7_SetRenderTarget_FPUPreserve(IDirect3DDevice7 *iface,
        IDirectDrawSurface7 *NewTarget, DWORD flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetRenderTarget(iface, NewTarget, flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_SetRenderTarget(IDirect3DDevice3 *iface,
        IDirectDrawSurface4 *target, DWORD flags)
{
    struct ddraw_surface *target_impl = unsafe_impl_from_IDirectDrawSurface4(target);
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    HRESULT hr;

    TRACE("iface %p, target %p, flags %#lx.\n", iface, target, flags);

    wined3d_mutex_lock();

    if (!validate_surface_palette(target_impl))
    {
        WARN("Surface %p has an indexed pixel format, but no palette.\n", target_impl);
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    if (!(target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE))
    {
        WARN("Surface %p is not a render target.\n", target_impl);
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    if (target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
    {
        WARN("Surface %p is a depth buffer.\n", target_impl);
        IDirectDrawSurface4_AddRef(target);
        IUnknown_Release(device->rt_iface);
        device->rt_iface = (IUnknown *)target;
        device->target = NULL;
        device->target_ds = NULL;
        wined3d_mutex_unlock();
        return DDERR_INVALIDPIXELFORMAT;
    }

    if (device->hardware_device && !(target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        WARN("Surface %p is not in video memory.\n", target_impl);
        IDirectDrawSurface4_AddRef(target);
        IUnknown_Release(device->rt_iface);
        device->rt_iface = (IUnknown *)target;
        device->target = NULL;
        device->target_ds = NULL;
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    hr = d3d_device_set_render_target(device, target_impl, (IUnknown *)target);
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d_device2_SetRenderTarget(IDirect3DDevice2 *iface,
        IDirectDrawSurface *target, DWORD flags)
{
    struct ddraw_surface *target_impl = unsafe_impl_from_IDirectDrawSurface(target);
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    HRESULT hr;

    TRACE("iface %p, target %p, flags %#lx.\n", iface, target, flags);

    wined3d_mutex_lock();

    if (!validate_surface_palette(target_impl))
    {
        WARN("Surface %p has an indexed pixel format, but no palette.\n", target_impl);
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    if (!(target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE))
    {
        WARN("Surface %p is not a render target.\n", target_impl);
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }

    if (target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
    {
        WARN("Surface %p is a depth buffer.\n", target_impl);
        IUnknown_Release(device->rt_iface);
        device->rt_iface = (IUnknown *)target;
        device->target = NULL;
        device->target_ds = NULL;
        wined3d_mutex_unlock();
        return DDERR_INVALIDPIXELFORMAT;
    }

    if (device->hardware_device && !(target_impl->surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        WARN("Surface %p is not in video memory.\n", target_impl);
        IDirectDrawSurface_AddRef(target);
        IUnknown_Release(device->rt_iface);
        device->rt_iface = (IUnknown *)target;
        device->target = NULL;
        device->target_ds = NULL;
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    hr = d3d_device_set_render_target(device, target_impl, (IUnknown *)target);
    wined3d_mutex_unlock();
    return hr;
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
static HRESULT WINAPI d3d_device7_GetRenderTarget(IDirect3DDevice7 *iface, IDirectDrawSurface7 **RenderTarget)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, target %p.\n", iface, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = IUnknown_QueryInterface(device->rt_iface, &IID_IDirectDrawSurface7, (void **)RenderTarget);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device3_GetRenderTarget(IDirect3DDevice3 *iface, IDirectDrawSurface4 **RenderTarget)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    IDirectDrawSurface7 *RenderTarget7;
    struct ddraw_surface *RenderTargetImpl;
    HRESULT hr;

    TRACE("iface %p, target %p.\n", iface, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    hr = d3d_device7_GetRenderTarget(&device->IDirect3DDevice7_iface, &RenderTarget7);
    if(hr != D3D_OK) return hr;
    RenderTargetImpl = impl_from_IDirectDrawSurface7(RenderTarget7);
    *RenderTarget = &RenderTargetImpl->IDirectDrawSurface4_iface;
    IDirectDrawSurface4_AddRef(*RenderTarget);
    IDirectDrawSurface7_Release(RenderTarget7);
    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_GetRenderTarget(IDirect3DDevice2 *iface, IDirectDrawSurface **RenderTarget)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    IDirectDrawSurface7 *RenderTarget7;
    struct ddraw_surface *RenderTargetImpl;
    HRESULT hr;

    TRACE("iface %p, target %p.\n", iface, RenderTarget);

    if(!RenderTarget)
        return DDERR_INVALIDPARAMS;

    hr = d3d_device7_GetRenderTarget(&device->IDirect3DDevice7_iface, &RenderTarget7);
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
static HRESULT WINAPI d3d_device3_Begin(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, fvf %#lx, flags %#lx.\n",
            iface, primitive_type, fvf, flags);

    wined3d_mutex_lock();
    device->primitive_type = primitive_type;
    device->vertex_type = fvf;
    device->render_flags = flags;
    device->vertex_size = get_flexible_vertex_size(device->vertex_type);
    device->nb_vertices = 0;
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_Begin(IDirect3DDevice2 *iface,
        D3DPRIMITIVETYPE primitive_type, D3DVERTEXTYPE vertex_type, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    DWORD fvf;

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, flags %#lx.\n",
            iface, primitive_type, vertex_type, flags);

    switch (vertex_type)
    {
        case D3DVT_VERTEX: fvf = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: fvf = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: fvf = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %#x.\n", vertex_type);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    };

    return d3d_device3_Begin(&device->IDirect3DDevice3_iface, primitive_type, fvf, flags);
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
static HRESULT WINAPI d3d_device3_BeginIndexed(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf,
        void *vertices, DWORD vertex_count, DWORD flags)
{
    FIXME("iface %p, primitive_type %#x, fvf %#lx, vertices %p, vertex_count %lu, flags %#lx stub!\n",
            iface, primitive_type, fvf, vertices, vertex_count, flags);

    return D3D_OK;
}


static HRESULT WINAPI d3d_device2_BeginIndexed(IDirect3DDevice2 *iface,
        D3DPRIMITIVETYPE primitive_type, D3DVERTEXTYPE vertex_type,
        void *vertices, DWORD vertex_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    DWORD fvf;

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, vertices %p, vertex_count %lu, flags %#lx.\n",
            iface, primitive_type, vertex_type, vertices, vertex_count, flags);

    switch (vertex_type)
    {
        case D3DVT_VERTEX: fvf = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: fvf = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: fvf = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unexpected vertex type %#x.\n", vertex_type);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    };

    return d3d_device3_BeginIndexed(&device->IDirect3DDevice3_iface,
            primitive_type, fvf, vertices, vertex_count, flags);
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
static HRESULT WINAPI d3d_device3_Vertex(IDirect3DDevice3 *iface, void *vertex)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, vertex %p.\n", iface, vertex);

    if (!vertex)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if ((device->nb_vertices + 1) * device->vertex_size > device->buffer_size)
    {
        BYTE *old_buffer;

        device->buffer_size = device->buffer_size ? device->buffer_size * 2 : device->vertex_size * 3;
        old_buffer = device->sysmem_vertex_buffer;
        device->sysmem_vertex_buffer = malloc(device->buffer_size);
        if (old_buffer)
        {
            memcpy(device->sysmem_vertex_buffer, old_buffer, device->nb_vertices * device->vertex_size);
            free(old_buffer);
        }
    }

    memcpy(device->sysmem_vertex_buffer + device->nb_vertices++ * device->vertex_size, vertex, device->vertex_size);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_Vertex(IDirect3DDevice2 *iface, void *vertex)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, vertex %p.\n", iface, vertex);

    return d3d_device3_Vertex(&device->IDirect3DDevice3_iface, vertex);
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
static HRESULT WINAPI d3d_device3_Index(IDirect3DDevice3 *iface, WORD index)
{
    FIXME("iface %p, index %#x stub!\n", iface, index);

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_Index(IDirect3DDevice2 *iface, WORD index)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, index %#x.\n", iface, index);

    return d3d_device3_Index(&device->IDirect3DDevice3_iface, index);
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
 *  D3D_OK on success,
 *  DDERR_INVALIDPARAMS if Value == NULL
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetRenderState(IDirect3DDevice7 *iface,
        D3DRENDERSTATETYPE state, DWORD *value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    const struct wined3d_stateblock_state *device_state;
    HRESULT hr = D3D_OK;

    TRACE("iface %p, state %#x, value %p.\n", iface, state, value);

    if (!value)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    device_state = device->stateblock_state;
    switch (state)
    {
        case D3DRENDERSTATE_TEXTUREMAG:
        {
            enum wined3d_texture_filter_type tex_mag = device_state->sampler_states[0][WINED3D_SAMP_MAG_FILTER];

            switch (tex_mag)
            {
                case WINED3D_TEXF_POINT:
                    *value = D3DFILTER_NEAREST;
                    break;
                case WINED3D_TEXF_LINEAR:
                    *value = D3DFILTER_LINEAR;
                    break;
                default:
                    ERR("Unhandled texture mag %d !\n",tex_mag);
                    *value = 0;
            }
            break;
        }

        case D3DRENDERSTATE_TEXTUREMIN:
        {
            enum wined3d_texture_filter_type tex_min;
            enum wined3d_texture_filter_type tex_mip;

            tex_min = device_state->sampler_states[0][WINED3D_SAMP_MIN_FILTER];
            tex_mip = device_state->sampler_states[0][WINED3D_SAMP_MIP_FILTER];
            switch (tex_min)
            {
                case WINED3D_TEXF_POINT:
                    switch (tex_mip)
                    {
                        case WINED3D_TEXF_NONE:
                            *value = D3DFILTER_NEAREST;
                            break;
                        case WINED3D_TEXF_POINT:
                            *value = D3DFILTER_MIPNEAREST;
                            break;
                        case WINED3D_TEXF_LINEAR:
                            *value = D3DFILTER_LINEARMIPNEAREST;
                            break;
                        default:
                            ERR("Unhandled mip filter %#x.\n", tex_mip);
                            *value = D3DFILTER_NEAREST;
                            break;
                    }
                    break;
                case WINED3D_TEXF_LINEAR:
                    switch (tex_mip)
                    {
                        case WINED3D_TEXF_NONE:
                            *value = D3DFILTER_LINEAR;
                            break;
                        case WINED3D_TEXF_POINT:
                            *value = D3DFILTER_MIPLINEAR;
                            break;
                        case WINED3D_TEXF_LINEAR:
                            *value = D3DFILTER_LINEARMIPLINEAR;
                            break;
                        default:
                            ERR("Unhandled mip filter %#x.\n", tex_mip);
                            *value = D3DFILTER_LINEAR;
                            break;
                    }
                    break;
                default:
                    ERR("Unhandled texture min filter %#x.\n",tex_min);
                    *value = D3DFILTER_NEAREST;
                    break;
            }
            break;
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            *value = device_state->sampler_states[0][WINED3D_SAMP_ADDRESS_U];
            break;
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            *value = device_state->sampler_states[0][WINED3D_SAMP_ADDRESS_V];
            break;

        case D3DRENDERSTATE_BORDERCOLOR:
            FIXME("Unhandled render state D3DRENDERSTATE_BORDERCOLOR.\n");
            hr = E_NOTIMPL;
            break;

        case D3DRENDERSTATE_TEXTUREHANDLE:
        case D3DRENDERSTATE_TEXTUREMAPBLEND:
            WARN("Render state %#x is invalid in d3d7.\n", state);
            hr = DDERR_INVALIDPARAMS;
            break;

        default:
            if (state >= D3DRENDERSTATE_STIPPLEPATTERN00
                    && state <= D3DRENDERSTATE_STIPPLEPATTERN31)
            {
                FIXME("Unhandled stipple pattern render state (%#x).\n", state);
                hr = E_NOTIMPL;
                break;
            }
            *value = device_state->rs[wined3d_render_state_from_ddraw(state)];
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device7_GetRenderState_FPUSetup(IDirect3DDevice7 *iface,
        D3DRENDERSTATETYPE state, DWORD *value)
{
    return d3d_device7_GetRenderState(iface, state, value);
}

static HRESULT WINAPI d3d_device7_GetRenderState_FPUPreserve(IDirect3DDevice7 *iface,
        D3DRENDERSTATETYPE state, DWORD *value)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetRenderState(iface, state, value);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_GetRenderState(IDirect3DDevice3 *iface,
        D3DRENDERSTATETYPE state, DWORD *value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, value %p.\n", iface, state, value);

    switch (state)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            /* This state is wrapped to SetTexture in SetRenderState, so
             * it has to be wrapped to GetTexture here. */
            struct wined3d_texture *tex = NULL;
            *value = 0;

            wined3d_mutex_lock();
            if ((tex = device->stateblock_state->textures[0]))
            {
                /* The parent of the texture is the IDirectDrawSurface7
                 * interface of the ddraw surface. */
                struct ddraw_texture *parent = wined3d_texture_get_parent(tex);
                if (parent)
                    *value = parent->root->Handle;
            }
            wined3d_mutex_unlock();

            return D3D_OK;
        }

        case D3DRENDERSTATE_TEXTUREMAPBLEND:
        {
            *value = device->texture_map_blend;
            return D3D_OK;
        }

        case D3DRENDERSTATE_LIGHTING:
        case D3DRENDERSTATE_NORMALIZENORMALS:
        case D3DRENDERSTATE_LOCALVIEWER:
            *value = 0xffffffff;
            return D3D_OK;

        default:
            return IDirect3DDevice7_GetRenderState(&device->IDirect3DDevice7_iface, state, value);
    }
}

static HRESULT WINAPI d3d_device2_GetRenderState(IDirect3DDevice2 *iface,
        D3DRENDERSTATETYPE state, DWORD *value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %p.\n", iface, state, value);

    return IDirect3DDevice3_GetRenderState(&device->IDirect3DDevice3_iface, state, value);
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
 *****************************************************************************/
static HRESULT d3d_device7_SetRenderState(IDirect3DDevice7 *iface,
        D3DRENDERSTATETYPE state, DWORD value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr = D3D_OK;

    TRACE("iface %p, state %#x, value %#lx.\n", iface, state, value);

    wined3d_mutex_lock();
    /* Some render states need special care */
    switch (state)
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

            switch (value)
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
                    FIXME("Unhandled texture mag %#lx.\n", value);
                    break;
            }

            wined3d_stateblock_set_sampler_state(device->state, 0, WINED3D_SAMP_MAG_FILTER, tex_mag);
            break;
        }

        case D3DRENDERSTATE_TEXTUREMIN:
        {
            enum wined3d_texture_filter_type tex_min;
            enum wined3d_texture_filter_type tex_mip;

            switch (value)
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
                    FIXME("Unhandled texture min %#lx.\n",value);
                    tex_min = WINED3D_TEXF_POINT;
                    tex_mip = WINED3D_TEXF_NONE;
                    break;
            }

            wined3d_stateblock_set_sampler_state(device->state, 0, WINED3D_SAMP_MIP_FILTER, tex_mip);
            wined3d_stateblock_set_sampler_state(device->state, 0, WINED3D_SAMP_MIN_FILTER, tex_min);
            break;
        }

        case D3DRENDERSTATE_TEXTUREADDRESS:
            wined3d_stateblock_set_sampler_state(device->state, 0, WINED3D_SAMP_ADDRESS_V, value);
            /* Drop through */
        case D3DRENDERSTATE_TEXTUREADDRESSU:
            wined3d_stateblock_set_sampler_state(device->state, 0, WINED3D_SAMP_ADDRESS_U, value);
            break;
        case D3DRENDERSTATE_TEXTUREADDRESSV:
            wined3d_stateblock_set_sampler_state(device->state, 0, WINED3D_SAMP_ADDRESS_V, value);
            break;

        case D3DRENDERSTATE_BORDERCOLOR:
            /* This should probably just forward to the corresponding sampler
             * state. Needs tests. */
            FIXME("Unhandled render state D3DRENDERSTATE_BORDERCOLOR.\n");
            hr = E_NOTIMPL;
            break;

        case D3DRENDERSTATE_TEXTUREHANDLE:
        case D3DRENDERSTATE_TEXTUREMAPBLEND:
            WARN("Render state %#x is invalid in d3d7.\n", state);
            hr = DDERR_INVALIDPARAMS;
            break;

        default:
            if (state >= D3DRENDERSTATE_STIPPLEPATTERN00
                    && state <= D3DRENDERSTATE_STIPPLEPATTERN31)
            {
                FIXME("Unhandled stipple pattern render state (%#x).\n", state);
                hr = E_NOTIMPL;
                break;
            }

            wined3d_stateblock_set_render_state(device->update_state, wined3d_render_state_from_ddraw(state), value);
            break;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device7_SetRenderState_FPUSetup(IDirect3DDevice7 *iface,
        D3DRENDERSTATETYPE state, DWORD value)
{
    return d3d_device7_SetRenderState(iface, state, value);
}

static HRESULT WINAPI d3d_device7_SetRenderState_FPUPreserve(IDirect3DDevice7 *iface,
        D3DRENDERSTATETYPE state, DWORD value)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetRenderState(iface, state, value);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static void fixup_texture_alpha_op(struct d3d_device *device)
{
    /* This fixup is required by the way D3DTBLEND_MODULATE maps to texture stage states.
       See d3d_device3_SetRenderState() for details. */
    struct wined3d_texture *tex;
    BOOL tex_alpha = TRUE;
    DDPIXELFORMAT ddfmt;

    if (!(device->legacyTextureBlending && device->texture_map_blend == D3DTBLEND_MODULATE))
        return;

    if ((tex = device->stateblock_state->textures[0]))
    {
        struct wined3d_resource_desc desc;

        wined3d_resource_get_desc(wined3d_texture_get_resource(tex), &desc);
        ddfmt.dwSize = sizeof(ddfmt);
        ddrawformat_from_wined3dformat(&ddfmt, desc.format);
        if (!ddfmt.dwRGBAlphaBitMask)
            tex_alpha = FALSE;
    }

    /* Args 1 and 2 are already set to WINED3DTA_TEXTURE/WINED3DTA_CURRENT in case of D3DTBLEND_MODULATE */
    wined3d_stateblock_set_texture_stage_state(device->state,
            0, WINED3D_TSS_ALPHA_OP, tex_alpha ? WINED3D_TOP_SELECT_ARG1 : WINED3D_TOP_SELECT_ARG2);
}

static HRESULT WINAPI d3d_device3_SetRenderState(IDirect3DDevice3 *iface,
        D3DRENDERSTATETYPE state, DWORD value)
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
    GetTextureStageState and vice versa. */

    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %#lx.\n", iface, state, value);

    if (state >= D3DSTATE_OVERRIDE_BIAS)
    {
        WARN("Unhandled state %#x.\n", state);
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();

    switch (state)
    {
        case D3DRENDERSTATE_TEXTUREHANDLE:
        {
            struct ddraw_surface *surf;

            if (value == 0)
            {
                wined3d_stateblock_set_texture(device->state, 0, NULL);
                hr = D3D_OK;
                break;
            }

            surf = ddraw_get_object(NULL, value - 1, DDRAW_HANDLE_SURFACE);
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
            if (value == device->texture_map_blend)
            {
                TRACE("Application is setting the same value over, nothing to do.\n");

                hr = D3D_OK;
                break;
            }

            device->legacyTextureBlending = TRUE;
            device->texture_map_blend = value;

            switch (value)
            {
                case D3DTBLEND_MODULATE:
                {
                    fixup_texture_alpha_op(device);

                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_MODULATE);
                    break;
                }

                case D3DTBLEND_ADD:
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_ADD);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG2);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    break;

                case D3DTBLEND_MODULATEALPHA:
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_MODULATE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_MODULATE);
                    break;

                case D3DTBLEND_COPY:
                case D3DTBLEND_DECAL:
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_SELECT_ARG1);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG1);
                    break;

                case D3DTBLEND_DECALALPHA:
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_OP, WINED3D_TOP_BLEND_TEXTURE_ALPHA);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG1, WINED3DTA_TEXTURE);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_COLOR_ARG2, WINED3DTA_CURRENT);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_OP, WINED3D_TOP_SELECT_ARG2);
                    wined3d_stateblock_set_texture_stage_state(device->state,
                            0, WINED3D_TSS_ALPHA_ARG2, WINED3DTA_CURRENT);
                    break;

                default:
                    FIXME("Unhandled texture environment %#lx.\n", value);
            }
            hr = D3D_OK;
            break;
        }

        case D3DRENDERSTATE_LIGHTING:
        case D3DRENDERSTATE_NORMALIZENORMALS:
        case D3DRENDERSTATE_LOCALVIEWER:
            hr = D3D_OK;
            break;

        default:
            hr = IDirect3DDevice7_SetRenderState(&device->IDirect3DDevice7_iface, state, value);
            break;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device2_SetRenderState(IDirect3DDevice2 *iface,
        D3DRENDERSTATETYPE state, DWORD value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %#lx.\n", iface, state, value);

    return IDirect3DDevice3_SetRenderState(&device->IDirect3DDevice3_iface, state, value);
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
static HRESULT WINAPI d3d_device3_SetLightState(IDirect3DDevice3 *iface,
        D3DLIGHTSTATETYPE state, DWORD value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %#lx.\n", iface, state, value);

    if (!state || (state > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();
    if (state == D3DLIGHTSTATE_MATERIAL)
    {
        if (value)
        {
            struct d3d_material *m;

            if (!(m = ddraw_get_object(NULL, value - 1, DDRAW_HANDLE_MATERIAL)))
            {
                WARN("Invalid material handle.\n");
                wined3d_mutex_unlock();
                return DDERR_INVALIDPARAMS;
            }

            material_activate(m);
        }

        device->material = value;
    }
    else if (state == D3DLIGHTSTATE_COLORMODEL)
    {
        switch (value)
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
        switch (state)
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
                FIXME("Unhandled D3DLIGHTSTATETYPE %#x.\n", state);
                wined3d_mutex_unlock();
                return DDERR_INVALIDPARAMS;
        }

        hr = IDirect3DDevice7_SetRenderState(&device->IDirect3DDevice7_iface, rs, value);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_SetLightState(IDirect3DDevice2 *iface,
        D3DLIGHTSTATETYPE state, DWORD value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %#lx.\n", iface, state, value);

    return d3d_device3_SetLightState(&device->IDirect3DDevice3_iface, state, value);
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
static HRESULT WINAPI d3d_device3_GetLightState(IDirect3DDevice3 *iface,
        D3DLIGHTSTATETYPE state, DWORD *value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %p.\n", iface, state, value);

    if (!state || (state > D3DLIGHTSTATE_COLORVERTEX))
    {
        TRACE("Unexpected Light State Type\n");
        return DDERR_INVALIDPARAMS;
    }

    if (!value)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (state == D3DLIGHTSTATE_MATERIAL)
    {
        *value = device->material;
    }
    else if (state == D3DLIGHTSTATE_COLORMODEL)
    {
        *value = D3DCOLOR_RGB;
    }
    else
    {
        D3DRENDERSTATETYPE rs;
        switch (state)
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
                FIXME("Unhandled D3DLIGHTSTATETYPE %#x.\n", state);
                wined3d_mutex_unlock();
                return DDERR_INVALIDPARAMS;
        }

        hr = IDirect3DDevice7_GetRenderState(&device->IDirect3DDevice7_iface, rs, value);
        wined3d_mutex_unlock();
        return hr;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device2_GetLightState(IDirect3DDevice2 *iface,
        D3DLIGHTSTATETYPE state, DWORD *value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, value %p.\n", iface, state, value);

    return d3d_device3_GetLightState(&device->IDirect3DDevice3_iface, state, value);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_SetTransform(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (!matrix)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    wined3d_stateblock_set_transform(device->update_state,
            wined3d_transform_state_from_ddraw(state), (const struct wined3d_matrix *)matrix);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_SetTransform_FPUSetup(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    return d3d_device7_SetTransform(iface, state, matrix);
}

static HRESULT WINAPI d3d_device7_SetTransform_FPUPreserve(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetTransform(iface, state, matrix);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_SetTransform(IDirect3DDevice3 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (!matrix)
        return DDERR_INVALIDPARAMS;

    if (state == D3DTRANSFORMSTATE_PROJECTION)
    {
        struct wined3d_matrix projection;

        wined3d_mutex_lock();
        multiply_matrix(&projection, &device->legacy_clipspace, (struct wined3d_matrix *)matrix);
        wined3d_stateblock_set_transform(device->state, WINED3D_TS_PROJECTION, &projection);
        memcpy(&device->legacy_projection, matrix, sizeof(*matrix));
        wined3d_mutex_unlock();

        return D3D_OK;
    }

    return IDirect3DDevice7_SetTransform(&device->IDirect3DDevice7_iface, state, matrix);
}

static HRESULT WINAPI d3d_device2_SetTransform(IDirect3DDevice2 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    return IDirect3DDevice3_SetTransform(&device->IDirect3DDevice3_iface, state, matrix);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetTransform(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (!matrix)
        return DDERR_INVALIDPARAMS;

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    memcpy(matrix, &device->stateblock_state->transforms[wined3d_transform_state_from_ddraw(state)], sizeof(*matrix));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_GetTransform_FPUSetup(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    return d3d_device7_GetTransform(iface, state, matrix);
}

static HRESULT WINAPI d3d_device7_GetTransform_FPUPreserve(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetTransform(iface, state, matrix);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_GetTransform(IDirect3DDevice3 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (!matrix)
        return DDERR_INVALIDPARAMS;

    if (state == D3DTRANSFORMSTATE_PROJECTION)
    {
        wined3d_mutex_lock();
        memcpy(matrix, &device->legacy_projection, sizeof(*matrix));
        wined3d_mutex_unlock();
        return DD_OK;
    }

    return IDirect3DDevice7_GetTransform(&device->IDirect3DDevice7_iface, state, matrix);
}

static HRESULT WINAPI d3d_device2_GetTransform(IDirect3DDevice2 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    return IDirect3DDevice3_GetTransform(&device->IDirect3DDevice3_iface, state, matrix);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_MultiplyTransform(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    wined3d_stateblock_multiply_transform(device->state,
            wined3d_transform_state_from_ddraw(state), (struct wined3d_matrix *)matrix);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_MultiplyTransform_FPUSetup(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    return d3d_device7_MultiplyTransform(iface, state, matrix);
}

static HRESULT WINAPI d3d_device7_MultiplyTransform_FPUPreserve(IDirect3DDevice7 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_MultiplyTransform(iface, state, matrix);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_MultiplyTransform(IDirect3DDevice3 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    if (state == D3DTRANSFORMSTATE_PROJECTION)
    {
        struct wined3d_matrix projection, tmp;

        wined3d_mutex_lock();
        multiply_matrix(&tmp, &device->legacy_projection, (struct wined3d_matrix *)matrix);
        multiply_matrix(&projection, &device->legacy_clipspace, &tmp);
        wined3d_stateblock_set_transform(device->state, WINED3D_TS_PROJECTION, &projection);
        device->legacy_projection = tmp;
        wined3d_mutex_unlock();

        return D3D_OK;
    }

    return IDirect3DDevice7_MultiplyTransform(&device->IDirect3DDevice7_iface, state, matrix);
}

static HRESULT WINAPI d3d_device2_MultiplyTransform(IDirect3DDevice2 *iface,
        D3DTRANSFORMSTATETYPE state, D3DMATRIX *matrix)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, state %#x, matrix %p.\n", iface, state, matrix);

    return IDirect3DDevice3_MultiplyTransform(&device->IDirect3DDevice3_iface, state, matrix);
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
 *
 *****************************************************************************/
static void d3d_device_sync_rendertarget(struct d3d_device *device)
{
    struct wined3d_rendertarget_view *rtv, *dsv;

    rtv = device->target ? ddraw_surface_get_rendertarget_view(device->target) : NULL;
    if (rtv)
    {
        if (FAILED(wined3d_device_context_set_rendertarget_views(device->immediate_context, 0, 1, &rtv, FALSE)))
            ERR("wined3d_device_context_set_rendertarget_views failed.\n");
    }
    else if (!device->target)
    {
        /* NULL device->target may appear when the game was setting invalid render target which in some cases
         * still keeps the invalid render target in the device even while returning an error.
         *
         * TODO: make render go nowhere instead of lefover render target (like it seems to work on Windows on HW devices
         * while may just crash on software devices. */
        FIXME("Keeping leftover render target.\n");
    }

    dsv = device->target_ds ? ddraw_surface_get_rendertarget_view(device->target_ds) : NULL;
    if (FAILED(wined3d_device_context_set_depth_stencil_view(device->immediate_context, dsv)))
        ERR("wined3d_device_context_set_depth_stencil_view failed.\n");
    wined3d_stateblock_depth_buffer_changed(device->state);

    if (device->hardware_device)
        return;

    if (rtv)
        ddraw_surface_get_draw_texture(wined3d_rendertarget_view_get_parent(rtv), DDRAW_SURFACE_RW);

    if (dsv)
        ddraw_surface_get_draw_texture(wined3d_rendertarget_view_get_parent(dsv), DDRAW_SURFACE_RW);
}

void d3d_device_sync_surfaces(struct d3d_device *device)
{
    const struct wined3d_stateblock_state *state = device->stateblock_state;
    struct ddraw_surface *surface;
    unsigned int i, j;

    d3d_device_sync_rendertarget(device);

    if (!device->have_draw_textures)
        return;

    for (i = 0; i < ARRAY_SIZE(state->textures); ++i)
    {
        if (!state->textures[i])
            continue;

        j = 0;
        while ((surface = wined3d_texture_get_sub_resource_parent(state->textures[i], j)))
        {
            if (!surface->draw_texture)
                break;
            ddraw_surface_get_draw_texture(surface, DDRAW_SURFACE_READ);
            ++j;
        }
    }
}

void d3d_device_apply_state(struct d3d_device *device, BOOL clear_state)
{
    if (device->ddraw && device->ddraw->device_last_applied_state != device)
    {
        wined3d_stateblock_primary_dirtify_all_states(device->wined3d_device, device->state);
        device->ddraw->device_last_applied_state = device;
    }
    if (clear_state)
        wined3d_stateblock_apply_clear_state(device->state, device->wined3d_device);
    else
        wined3d_device_apply_stateblock(device->wined3d_device, device->state);
    d3d_device_sync_surfaces(device);
}

static HRESULT d3d_device7_DrawPrimitive(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices,
        DWORD vertex_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    UINT stride, vb_pos, size;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, fvf %#lx, vertices %p, vertex_count %lu, flags %#lx.\n",
            iface, primitive_type, fvf, vertices, vertex_count, flags);

    if (!vertex_count)
    {
        WARN("0 vertex count.\n");
        return D3D_OK;
    }

    /* Get the stride */
    stride = get_flexible_vertex_size(fvf);
    size = vertex_count * stride;

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device,
            &device->vertex_buffer, vertices, size, stride, &vb_pos)))
        goto done;

    hr = wined3d_stateblock_set_stream_source(device->state, 0, device->vertex_buffer.buffer, 0, stride);
    if (FAILED(hr))
        goto done;

    wined3d_stateblock_set_vertex_declaration(device->state, ddraw_find_decl(device->ddraw, fvf));
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_ddraw(primitive_type), 0);
    d3d_device_apply_state(device, FALSE);
    wined3d_device_context_draw(device->immediate_context, vb_pos / stride, vertex_count, 0, 0);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d_device7_DrawPrimitive_FPUSetup(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices,
        DWORD vertex_count, DWORD flags)
{
    return d3d_device7_DrawPrimitive(iface, primitive_type, fvf, vertices, vertex_count, flags);
}

static HRESULT WINAPI d3d_device7_DrawPrimitive_FPUPreserve(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices,
        DWORD vertex_count, DWORD flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_DrawPrimitive(iface, primitive_type, fvf, vertices, vertex_count, flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static void setup_lighting(const struct d3d_device *device, DWORD fvf, DWORD flags)
{
    BOOL enable = TRUE;

    /* Ignore the D3DFVF_XYZRHW case here, wined3d takes care of that */
    if (!device->material || !(fvf & D3DFVF_NORMAL) || (flags & D3DDP_DONOTLIGHT))
        enable = FALSE;

    wined3d_stateblock_set_render_state(device->state, WINED3D_RS_LIGHTING, enable);
}


static HRESULT WINAPI d3d_device3_DrawPrimitive(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices, DWORD vertex_count,
        DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, fvf %#lx, vertices %p, vertex_count %lu, flags %#lx.\n",
            iface, primitive_type, fvf, vertices, vertex_count, flags);

    setup_lighting(device, fvf, flags);

    return IDirect3DDevice7_DrawPrimitive(&device->IDirect3DDevice7_iface,
            primitive_type, fvf, vertices, vertex_count, flags);
}

static HRESULT WINAPI d3d_device2_DrawPrimitive(IDirect3DDevice2 *iface,
        D3DPRIMITIVETYPE primitive_type, D3DVERTEXTYPE vertex_type, void *vertices,
        DWORD vertex_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    DWORD fvf;

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, vertices %p, vertex_count %lu, flags %#lx.\n",
            iface, primitive_type, vertex_type, vertices, vertex_count, flags);

    switch (vertex_type)
    {
        case D3DVT_VERTEX: fvf = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: fvf = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: fvf = D3DFVF_TLVERTEX; break;
        default:
            FIXME("Unhandled vertex type %#x.\n", vertex_type);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    }

    return d3d_device3_DrawPrimitive(&device->IDirect3DDevice3_iface,
            primitive_type, fvf, vertices, vertex_count, flags);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_DrawIndexedPrimitive(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices, DWORD vertex_count,
        WORD *indices, DWORD index_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    unsigned int idx_size = index_count * sizeof(*indices);
    unsigned short min_index = USHRT_MAX, max_index = 0;
    unsigned int i;
    HRESULT hr;
    UINT stride = get_flexible_vertex_size(fvf);
    UINT vb_pos, ib_pos;

    TRACE("iface %p, primitive_type %#x, fvf %#lx, vertices %p, vertex_count %lu, "
            "indices %p, index_count %lu, flags %#lx.\n",
            iface, primitive_type, fvf, vertices, vertex_count, indices, index_count, flags);

    if (!vertex_count || !index_count)
    {
        WARN("0 vertex or index count.\n");
        return D3D_OK;
    }

    /* Prince of Persia 3D creates large vertex buffers but only actually uses
     * a few vertices from them. This improves performance dramatically. */
    for (i = 0; i < index_count; ++i)
    {
        min_index = min(min_index, indices[i]);
        max_index = max(max_index, indices[i]);
    }

    /* Set the D3DDevice's FVF */
    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device, &device->vertex_buffer,
            (char *)vertices + (min_index * stride), (max_index + 1 - min_index) * stride, stride, &vb_pos)))
        goto done;

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device,
            &device->index_buffer, indices, idx_size, sizeof(*indices), &ib_pos)))
        goto done;

    hr = wined3d_stateblock_set_stream_source(device->state, 0, device->vertex_buffer.buffer, 0, stride);
    if (FAILED(hr))
        goto done;
    wined3d_stateblock_set_index_buffer(device->state, device->index_buffer.buffer, WINED3DFMT_R16_UINT);

    wined3d_stateblock_set_vertex_declaration(device->state, ddraw_find_decl(device->ddraw, fvf));
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_ddraw(primitive_type), 0);
    d3d_device_apply_state(device, FALSE);
    wined3d_device_context_draw_indexed(device->immediate_context, (int)(vb_pos / stride) - min_index,
            ib_pos / sizeof(*indices), index_count, 0, 0);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d_device7_DrawIndexedPrimitive_FPUSetup(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices, DWORD vertex_count,
        WORD *indices, DWORD index_count, DWORD flags)
{
    return d3d_device7_DrawIndexedPrimitive(iface, primitive_type, fvf,
            vertices, vertex_count, indices, index_count, flags);
}

static HRESULT WINAPI d3d_device7_DrawIndexedPrimitive_FPUPreserve(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices, DWORD vertex_count,
        WORD *indices, DWORD index_count, DWORD flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_DrawIndexedPrimitive(iface, primitive_type, fvf,
            vertices, vertex_count, indices, index_count, flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_DrawIndexedPrimitive(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, void *vertices, DWORD vertex_count,
        WORD *indices, DWORD index_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, fvf %#lx, vertices %p, vertex_count %lu, "
            "indices %p, index_count %lu, flags %#lx.\n",
            iface, primitive_type, fvf, vertices, vertex_count, indices, index_count, flags);

    setup_lighting(device, fvf, flags);

    return IDirect3DDevice7_DrawIndexedPrimitive(&device->IDirect3DDevice7_iface,
            primitive_type, fvf, vertices, vertex_count, indices, index_count, flags);
}

static HRESULT WINAPI d3d_device2_DrawIndexedPrimitive(IDirect3DDevice2 *iface,
        D3DPRIMITIVETYPE primitive_type, D3DVERTEXTYPE vertex_type, void *vertices,
        DWORD vertex_count, WORD *indices, DWORD index_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);
    DWORD fvf;

    TRACE("iface %p, primitive_type %#x, vertex_type %#x, vertices %p, vertex_count %lu, "
            "indices %p, index_count %lu, flags %#lx.\n",
            iface, primitive_type, vertex_type, vertices, vertex_count, indices, index_count, flags);

    switch (vertex_type)
    {
        case D3DVT_VERTEX: fvf = D3DFVF_VERTEX; break;
        case D3DVT_LVERTEX: fvf = D3DFVF_LVERTEX; break;
        case D3DVT_TLVERTEX: fvf = D3DFVF_TLVERTEX; break;
        default:
            ERR("Unhandled vertex type %#x.\n", vertex_type);
            return DDERR_INVALIDPARAMS;  /* Should never happen */
    }

    return d3d_device3_DrawIndexedPrimitive(&device->IDirect3DDevice3_iface,
            primitive_type, fvf, vertices, vertex_count, indices, index_count, flags);
}

/*****************************************************************************
 * IDirect3DDevice3::End
 *
 * Ends a draw begun with IDirect3DDevice3::Begin or
 * IDirect3DDevice::BeginIndexed. The vertices specified with
 * IDirect3DDevice::Vertex or IDirect3DDevice::Index are drawn using
 * the IDirect3DDevice3::DrawPrimitive method. So far only
 * non-indexed mode is supported
 *
 * Version 2 and 3
 *
 * Params:
 *  Flags: Some flags, as usual. Don't know which are defined
 *
 * Returns:
 *  The return value of IDirect3DDevice3::DrawPrimitive
 *
 *****************************************************************************/
static HRESULT WINAPI d3d_device3_End(IDirect3DDevice3 *iface, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return d3d_device3_DrawPrimitive(&device->IDirect3DDevice3_iface, device->primitive_type,
            device->vertex_type, device->sysmem_vertex_buffer, device->nb_vertices, device->render_flags);
}

static HRESULT WINAPI d3d_device2_End(IDirect3DDevice2 *iface, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, flags %#lx.\n", iface, flags);

    return d3d_device3_End(&device->IDirect3DDevice3_iface, flags);
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
static HRESULT WINAPI d3d_device7_SetClipStatus(IDirect3DDevice7 *iface, D3DCLIPSTATUS *clip_status)
{
    FIXME("iface %p, clip_status %p stub!\n", iface, clip_status);

    return D3D_OK;
}

static HRESULT WINAPI d3d_device3_SetClipStatus(IDirect3DDevice3 *iface, D3DCLIPSTATUS *clip_status)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);

    return IDirect3DDevice7_SetClipStatus(&device->IDirect3DDevice7_iface, clip_status);
}

static HRESULT WINAPI d3d_device2_SetClipStatus(IDirect3DDevice2 *iface, D3DCLIPSTATUS *clip_status)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);

    return IDirect3DDevice7_SetClipStatus(&device->IDirect3DDevice7_iface, clip_status);
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
static HRESULT WINAPI d3d_device7_GetClipStatus(IDirect3DDevice7 *iface, D3DCLIPSTATUS *clip_status)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_viewport vp;

    FIXME("iface %p, clip_status %p stub.\n", iface, clip_status);

    vp = device->stateblock_state->viewport;
    clip_status->minx = vp.x;
    clip_status->maxx = vp.x + vp.width;
    clip_status->miny = vp.y;
    clip_status->maxy = vp.y + vp.height;
    clip_status->minz = 0.0f;
    clip_status->maxz = 0.0f;
    clip_status->dwFlags = D3DCLIPSTATUS_EXTENTS2;
    clip_status->dwStatus = 0;

    return D3D_OK;
}

static HRESULT WINAPI d3d_device3_GetClipStatus(IDirect3DDevice3 *iface, D3DCLIPSTATUS *clip_status)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);

    return IDirect3DDevice7_GetClipStatus(&device->IDirect3DDevice7_iface, clip_status);
}

static HRESULT WINAPI d3d_device2_GetClipStatus(IDirect3DDevice2 *iface, D3DCLIPSTATUS *clip_status)
{
    struct d3d_device *device = impl_from_IDirect3DDevice2(iface);

    TRACE("iface %p, clip_status %p.\n", iface, clip_status);

    return IDirect3DDevice7_GetClipStatus(&device->IDirect3DDevice7_iface, clip_status);
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
 *
 *****************************************************************************/
static void pack_strided_data(BYTE *dst, DWORD count, const D3DDRAWPRIMITIVESTRIDEDDATA *src, DWORD fvf)
{
    DWORD i, tex, offset;

    for (i = 0; i < count; i++)
    {
        /* The contents of the strided data are determined by the fvf,
         * not by the members set in src. So it's valid
         * to have diffuse.lpvData set to 0xdeadbeef if the diffuse flag is
         * not set in the fvf. */
        if (fvf & D3DFVF_POSITION_MASK)
        {
            offset = i * src->position.dwStride;
            if (fvf & D3DFVF_XYZRHW)
            {
                memcpy(dst, ((BYTE *)src->position.lpvData) + offset, 4 * sizeof(float));
                dst += 4 * sizeof(float);
            }
            else
            {
                memcpy(dst, ((BYTE *)src->position.lpvData) + offset, 3 * sizeof(float));
                dst += 3 * sizeof(float);
            }
        }

        if (fvf & D3DFVF_NORMAL)
        {
            offset = i * src->normal.dwStride;
            memcpy(dst, ((BYTE *)src->normal.lpvData) + offset, 3 * sizeof(float));
            dst += 3 * sizeof(float);
        }

        if (fvf & D3DFVF_DIFFUSE)
        {
            offset = i * src->diffuse.dwStride;
            memcpy(dst, ((BYTE *)src->diffuse.lpvData) + offset, sizeof(DWORD));
            dst += sizeof(DWORD);
        }

        if (fvf & D3DFVF_SPECULAR)
        {
            offset = i * src->specular.dwStride;
            memcpy(dst, ((BYTE *)src->specular.lpvData) + offset, sizeof(DWORD));
            dst += sizeof(DWORD);
        }

        for (tex = 0; tex < GET_TEXCOUNT_FROM_FVF(fvf); ++tex)
        {
            DWORD attrib_count = GET_TEXCOORD_SIZE_FROM_FVF(fvf, tex);
            offset = i * src->textureCoords[tex].dwStride;
            memcpy(dst, ((BYTE *)src->textureCoords[tex].lpvData) + offset, attrib_count * sizeof(float));
            dst += attrib_count * sizeof(float);
        }
    }
}

static HRESULT d3d_device7_DrawPrimitiveStrided(IDirect3DDevice7 *iface, D3DPRIMITIVETYPE primitive_type,
        DWORD fvf, D3DDRAWPRIMITIVESTRIDEDDATA *strided_data, DWORD vertex_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;
    UINT dst_stride = get_flexible_vertex_size(fvf);
    UINT dst_size = dst_stride * vertex_count;
    void *dst_data;
    UINT vb_pos;

    TRACE("iface %p, primitive_type %#x, fvf %#lx, strided_data %p, vertex_count %lu, flags %#lx.\n",
            iface, primitive_type, fvf, strided_data, vertex_count, flags);

    if (!vertex_count)
    {
        WARN("0 vertex count.\n");
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_streaming_buffer_map(device->wined3d_device,
            &device->vertex_buffer, dst_size, dst_stride, &vb_pos, &dst_data)))
        goto done;
    pack_strided_data(dst_data, vertex_count, strided_data, fvf);
    wined3d_streaming_buffer_unmap(&device->vertex_buffer);

    hr = wined3d_stateblock_set_stream_source(device->state, 0, device->vertex_buffer.buffer, 0, dst_stride);
    if (FAILED(hr))
        goto done;
    wined3d_stateblock_set_vertex_declaration(device->state, ddraw_find_decl(device->ddraw, fvf));

    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_ddraw(primitive_type), 0);
    d3d_device_apply_state(device, FALSE);
    wined3d_device_context_draw(device->immediate_context, vb_pos / dst_stride, vertex_count, 0, 0);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d_device7_DrawPrimitiveStrided_FPUSetup(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount, DWORD Flags)
{
    return d3d_device7_DrawPrimitiveStrided(iface, PrimitiveType,
            VertexType, D3DDrawPrimStrideData, VertexCount, Flags);
}

static HRESULT WINAPI d3d_device7_DrawPrimitiveStrided_FPUPreserve(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount, DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_DrawPrimitiveStrided(iface, PrimitiveType,
            VertexType, D3DDrawPrimStrideData, VertexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_DrawPrimitiveStrided(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount, DWORD Flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, FVF %#lx, strided_data %p, vertex_count %lu, flags %#lx.\n",
            iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Flags);

    setup_lighting(device, VertexType, Flags);

    return IDirect3DDevice7_DrawPrimitiveStrided(&device->IDirect3DDevice7_iface,
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_DrawIndexedPrimitiveStrided(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, DWORD fvf, D3DDRAWPRIMITIVESTRIDEDDATA *strided_data,
        DWORD vertex_count, WORD *indices, DWORD index_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    UINT vtx_dst_stride = get_flexible_vertex_size(fvf);
    UINT vtx_dst_size = vertex_count * vtx_dst_stride;
    UINT idx_size = index_count * sizeof(WORD);
    void *dst_data;
    UINT vb_pos;
    UINT ib_pos;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, fvf %#lx, strided_data %p, "
            "vertex_count %lu, indices %p, index_count %lu, flags %#lx.\n",
            iface, primitive_type, fvf, strided_data, vertex_count, indices, index_count, flags);

    if (!vertex_count || !index_count)
    {
        WARN("0 vertex or index count.\n");
        return D3D_OK;
    }

    wined3d_mutex_lock();

    if (FAILED(hr = wined3d_streaming_buffer_map(device->wined3d_device,
            &device->vertex_buffer, vtx_dst_size, vtx_dst_stride, &vb_pos, &dst_data)))
        goto done;
    pack_strided_data(dst_data, vertex_count, strided_data, fvf);
    wined3d_streaming_buffer_unmap(&device->vertex_buffer);

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device,
            &device->index_buffer, indices, idx_size, sizeof(WORD), &ib_pos)))
        goto done;

    hr = wined3d_stateblock_set_stream_source(device->state, 0, device->vertex_buffer.buffer, 0, vtx_dst_stride);
    if (FAILED(hr))
        goto done;
    wined3d_stateblock_set_index_buffer(device->state, device->index_buffer.buffer, WINED3DFMT_R16_UINT);

    wined3d_stateblock_set_vertex_declaration(device->state, ddraw_find_decl(device->ddraw, fvf));
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_ddraw(primitive_type), 0);
    d3d_device_apply_state(device, FALSE);
    wined3d_device_context_draw_indexed(device->immediate_context,
            vb_pos / vtx_dst_stride, ib_pos / sizeof(WORD), index_count, 0, 0);

done:
    wined3d_mutex_unlock();
    return hr;
}

static HRESULT WINAPI d3d_device7_DrawIndexedPrimitiveStrided_FPUSetup(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount,
        WORD *Indices, DWORD IndexCount, DWORD Flags)
{
    return d3d_device7_DrawIndexedPrimitiveStrided(iface, PrimitiveType, VertexType,
            D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
}

static HRESULT WINAPI d3d_device7_DrawIndexedPrimitiveStrided_FPUPreserve(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount,
        WORD *Indices, DWORD IndexCount, DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_DrawIndexedPrimitiveStrided(iface, PrimitiveType, VertexType,
            D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_DrawIndexedPrimitiveStrided(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE PrimitiveType, DWORD VertexType,
        D3DDRAWPRIMITIVESTRIDEDDATA *D3DDrawPrimStrideData, DWORD VertexCount, WORD *Indices,
        DWORD IndexCount, DWORD Flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, primitive_type %#x, FVF %#lx, strided_data %p, vertex_count %lu, indices %p, index_count %lu, flags %#lx.\n",
            iface, PrimitiveType, VertexType, D3DDrawPrimStrideData, VertexCount, Indices, IndexCount, Flags);

    setup_lighting(device, VertexType, Flags);

    return IDirect3DDevice7_DrawIndexedPrimitiveStrided(&device->IDirect3DDevice7_iface,
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
static HRESULT d3d_device7_DrawPrimitiveVB(IDirect3DDevice7 *iface, D3DPRIMITIVETYPE primitive_type,
        IDirect3DVertexBuffer7 *vb, DWORD start_vertex, DWORD vertex_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct d3d_vertex_buffer *vb_impl = unsafe_impl_from_IDirect3DVertexBuffer7(vb);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_map_desc wined3d_map_desc;
    struct wined3d_box wined3d_box = {0};
    DWORD stride;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, vb %p, start_vertex %lu, vertex_count %lu, flags %#lx.\n",
            iface, primitive_type, vb, start_vertex, vertex_count, flags);

    if (!vertex_count)
    {
        WARN("0 vertex count.\n");
        return D3D_OK;
    }

    vb_impl->discarded = false;

    stride = get_flexible_vertex_size(vb_impl->fvf);

    if (vb_impl->sysmem)
    {
        TRACE("Drawing from D3DVBCAPS_SYSTEMMEMORY vertex buffer, forwarding to DrawPrimitive().\n");
        wined3d_mutex_lock();
        wined3d_resource = wined3d_buffer_get_resource(vb_impl->wined3d_buffer);
        wined3d_box.left = start_vertex * stride;
        wined3d_box.right = wined3d_box.left + vertex_count * stride;
        if (FAILED(hr = wined3d_resource_map(wined3d_resource, 0, &wined3d_map_desc,
                &wined3d_box, WINED3D_MAP_READ)))
        {
            wined3d_mutex_unlock();
            return D3DERR_VERTEXBUFFERLOCKED;
        }
        hr = d3d_device7_DrawPrimitive(iface, primitive_type, vb_impl->fvf, wined3d_map_desc.data,
                vertex_count, flags);
        wined3d_resource_unmap(wined3d_resource, 0);
        wined3d_mutex_unlock();
        return hr;
    }

    wined3d_mutex_lock();
    wined3d_stateblock_set_vertex_declaration(device->state, vb_impl->wined3d_declaration);
    if (FAILED(hr = wined3d_stateblock_set_stream_source(device->state,
            0, vb_impl->wined3d_buffer, 0, stride)))
    {
        WARN("Failed to set stream source, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    /* Now draw the primitives */
    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_ddraw(primitive_type), 0);
    d3d_device_apply_state(device, FALSE);
    wined3d_device_context_draw(device->immediate_context, start_vertex, vertex_count, 0, 0);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device7_DrawPrimitiveVB_FPUSetup(IDirect3DDevice7 *iface, D3DPRIMITIVETYPE PrimitiveType,
        IDirect3DVertexBuffer7 *D3DVertexBuf, DWORD StartVertex, DWORD NumVertices, DWORD Flags)
{
    return d3d_device7_DrawPrimitiveVB(iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);
}

static HRESULT WINAPI d3d_device7_DrawPrimitiveVB_FPUPreserve(IDirect3DDevice7 *iface, D3DPRIMITIVETYPE PrimitiveType,
        IDirect3DVertexBuffer7 *D3DVertexBuf, DWORD StartVertex, DWORD NumVertices, DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_DrawPrimitiveVB(iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_DrawPrimitiveVB(IDirect3DDevice3 *iface, D3DPRIMITIVETYPE PrimitiveType,
        IDirect3DVertexBuffer *D3DVertexBuf, DWORD StartVertex, DWORD NumVertices, DWORD Flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    struct d3d_vertex_buffer *vb = unsafe_impl_from_IDirect3DVertexBuffer7((IDirect3DVertexBuffer7 *)D3DVertexBuf);

    TRACE("iface %p, primitive_type %#x, vb %p, start_vertex %lu, vertex_count %lu, flags %#lx.\n",
            iface, PrimitiveType, D3DVertexBuf, StartVertex, NumVertices, Flags);

    setup_lighting(device, vb->fvf, Flags);

    return IDirect3DDevice7_DrawPrimitiveVB(&device->IDirect3DDevice7_iface,
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
static HRESULT d3d_device7_DrawIndexedPrimitiveVB(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE primitive_type, IDirect3DVertexBuffer7 *vb,
        DWORD start_vertex, DWORD vertex_count, WORD *indices, DWORD index_count, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct d3d_vertex_buffer *vb_impl = unsafe_impl_from_IDirect3DVertexBuffer7(vb);
    DWORD stride = get_flexible_vertex_size(vb_impl->fvf);
    struct wined3d_resource *wined3d_resource;
    struct wined3d_map_desc wined3d_map_desc;
    struct wined3d_box wined3d_box = {0};
    HRESULT hr;
    UINT ib_pos;

    TRACE("iface %p, primitive_type %#x, vb %p, start_vertex %lu, "
            "vertex_count %lu, indices %p, index_count %lu, flags %#lx.\n",
            iface, primitive_type, vb, start_vertex, vertex_count, indices, index_count, flags);

    if (!vertex_count || !index_count)
    {
        WARN("0 vertex or index count.\n");
        return D3D_OK;
    }

    vb_impl->discarded = false;

    if (vb_impl->sysmem)
    {
        TRACE("Drawing from D3DVBCAPS_SYSTEMMEMORY vertex buffer, forwarding to DrawIndexedPrimitive().\n");
        wined3d_mutex_lock();
        wined3d_box.left = start_vertex * stride;
        wined3d_box.right = wined3d_box.left + vertex_count * stride;
        wined3d_resource = wined3d_buffer_get_resource(vb_impl->wined3d_buffer);
        if (FAILED(hr = wined3d_resource_map(wined3d_resource, 0, &wined3d_map_desc,
                &wined3d_box, WINED3D_MAP_READ)))
        {
            wined3d_mutex_unlock();
            return D3DERR_VERTEXBUFFERLOCKED;
        }
        hr = d3d_device7_DrawIndexedPrimitive(iface, primitive_type, vb_impl->fvf,
                wined3d_map_desc.data, vertex_count, indices, index_count, flags);
        wined3d_resource_unmap(wined3d_resource, 0);
        wined3d_mutex_unlock();
        return hr;
    }

    /* Steps:
     * 1) Upload the indices to the index buffer
     * 2) Set the index source
     * 3) Set the Vertex Buffer as the Stream source
     * 4) Call wined3d_device_context_draw_indexed()
     */

    wined3d_mutex_lock();

    wined3d_stateblock_set_vertex_declaration(device->state, vb_impl->wined3d_declaration);

    if (FAILED(hr = wined3d_streaming_buffer_upload(device->wined3d_device,
            &device->index_buffer, indices, index_count * sizeof(WORD), sizeof(WORD), &ib_pos)))
    {
        wined3d_mutex_unlock();
        return hr;
    }

    /* Set the index stream */
    wined3d_stateblock_set_index_buffer(device->state, device->index_buffer.buffer, WINED3DFMT_R16_UINT);

    /* Set the vertex stream source */
    if (FAILED(hr = wined3d_stateblock_set_stream_source(device->state,
            0, vb_impl->wined3d_buffer, 0, stride)))
    {
        ERR("Failed to set stream source for device %p, hr %#lx.\n", device, hr);
        wined3d_mutex_unlock();
        return hr;
    }

    wined3d_device_context_set_primitive_type(device->immediate_context,
            wined3d_primitive_type_from_ddraw(primitive_type), 0);
    d3d_device_apply_state(device, FALSE);
    wined3d_device_context_draw_indexed(device->immediate_context, start_vertex,
            ib_pos / sizeof(WORD), index_count, 0, 0);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device7_DrawIndexedPrimitiveVB_FPUSetup(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE PrimitiveType, IDirect3DVertexBuffer7 *D3DVertexBuf,
        DWORD StartVertex, DWORD NumVertices, WORD *Indices, DWORD IndexCount, DWORD Flags)
{
    return d3d_device7_DrawIndexedPrimitiveVB(iface, PrimitiveType,
            D3DVertexBuf, StartVertex, NumVertices, Indices, IndexCount, Flags);
}

static HRESULT WINAPI d3d_device7_DrawIndexedPrimitiveVB_FPUPreserve(IDirect3DDevice7 *iface,
        D3DPRIMITIVETYPE PrimitiveType, IDirect3DVertexBuffer7 *D3DVertexBuf,
        DWORD StartVertex, DWORD NumVertices, WORD *Indices, DWORD IndexCount, DWORD Flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_DrawIndexedPrimitiveVB(iface, PrimitiveType,
            D3DVertexBuf, StartVertex, NumVertices, Indices, IndexCount, Flags);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_DrawIndexedPrimitiveVB(IDirect3DDevice3 *iface,
        D3DPRIMITIVETYPE primitive_type, IDirect3DVertexBuffer *vertex_buffer,
        WORD *indices, DWORD index_count, DWORD flags)
{
    struct d3d_vertex_buffer *vb =
            unsafe_impl_from_IDirect3DVertexBuffer7((IDirect3DVertexBuffer7 *)vertex_buffer);
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    DWORD stride;

    TRACE("iface %p, primitive_type %#x, vb %p, indices %p, index_count %lu, flags %#lx.\n",
            iface, primitive_type, vertex_buffer, indices, index_count, flags);

    setup_lighting(device, vb->fvf, flags);

    if (!(stride = get_flexible_vertex_size(vb->fvf)))
        return D3D_OK;

    return IDirect3DDevice7_DrawIndexedPrimitiveVB(&device->IDirect3DDevice7_iface, primitive_type,
            &vb->IDirect3DVertexBuffer7_iface, 0, vb->size / stride, indices, index_count, flags);
}

/*****************************************************************************
 * IDirect3DDevice7::ComputeSphereVisibility
 *
 * Calculates the visibility of spheres in the current viewport. The spheres
 * are passed in the Centers and Radii arrays, the results are passed back
 * in the ReturnValues array. Return values are either completely visible,
 * partially visible or completely invisible.
 * The return value consists of a combination of D3DCLIP_* flags, or is
 * 0 if the sphere is completely visible (according to the SDK, not checked)
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

static DWORD in_plane(UINT idx, struct wined3d_vec4 p, D3DVECTOR center, D3DVALUE radius, BOOL equality)
{
    float distance, norm;

    norm = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
    distance = (p.x * center.x + p.y * center.y + p.z * center.z + p.w) / norm;

    if (equality)
    {
        if (fabs(distance) <= radius)
            return D3DSTATUS_CLIPUNIONLEFT << idx;
        if (distance <= -radius)
            return (D3DSTATUS_CLIPUNIONLEFT | D3DSTATUS_CLIPINTERSECTIONLEFT) << idx;
    }
    else
    {
        if (fabs(distance) < radius)
            return D3DSTATUS_CLIPUNIONLEFT << idx;
        if (distance < -radius)
            return (D3DSTATUS_CLIPUNIONLEFT | D3DSTATUS_CLIPINTERSECTIONLEFT) << idx;
    }
    return 0;
}

static void prepare_clip_space_planes(struct d3d_device *device, struct wined3d_vec4 *plane)
{
    const struct wined3d_stateblock_state *state;
    struct wined3d_matrix m;

    /* We want the wined3d matrices since those include the legacy viewport
     * transformation. */
    wined3d_mutex_lock();
    state = device->stateblock_state;
    multiply_matrix(&m, &state->transforms[WINED3D_TS_VIEW], &state->transforms[WINED3D_TS_WORLD]);
    multiply_matrix(&m, &state->transforms[WINED3D_TS_PROJECTION], &m);
    wined3d_mutex_unlock();

    /* Left plane. */
    plane[0].x = m._14 + m._11;
    plane[0].y = m._24 + m._21;
    plane[0].z = m._34 + m._31;
    plane[0].w = m._44 + m._41;

    /* Right plane. */
    plane[1].x = m._14 - m._11;
    plane[1].y = m._24 - m._21;
    plane[1].z = m._34 - m._31;
    plane[1].w = m._44 - m._41;

    /* Top plane. */
    plane[2].x = m._14 - m._12;
    plane[2].y = m._24 - m._22;
    plane[2].z = m._34 - m._32;
    plane[2].w = m._44 - m._42;

    /* Bottom plane. */
    plane[3].x = m._14 + m._12;
    plane[3].y = m._24 + m._22;
    plane[3].z = m._34 + m._32;
    plane[3].w = m._44 + m._42;

    /* Front plane. */
    plane[4].x = m._13;
    plane[4].y = m._23;
    plane[4].z = m._33;
    plane[4].w = m._43;

    /* Back plane. */
    plane[5].x = m._14 - m._13;
    plane[5].y = m._24 - m._23;
    plane[5].z = m._34 - m._33;
    plane[5].w = m._44 - m._43;
}

static void compute_sphere_visibility(const struct wined3d_vec4 *planes, DWORD enabled_planes, BOOL equality,
        const D3DVECTOR *centres, const D3DVALUE *radii, unsigned int sphere_count, DWORD *return_values)
{
    unsigned int mask, i, j;

    memset(return_values, 0, sphere_count * sizeof(*return_values));
    for (i = 0; i < sphere_count; ++i)
    {
        mask = enabled_planes;
        while (mask)
        {
            j = wined3d_bit_scan(&mask);
            return_values[i] |= in_plane(j, planes[j], centres[i], radii[i], equality);
        }
    }
}

static HRESULT WINAPI d3d_device7_ComputeSphereVisibility(IDirect3DDevice7 *iface,
        D3DVECTOR *centers, D3DVALUE *radii, DWORD sphere_count, DWORD flags, DWORD *return_values)
{
    struct wined3d_vec4 plane[12];
    DWORD enabled_planes = 0x3f;
    DWORD user_clip_planes;
    UINT j;

    TRACE("iface %p, centers %p, radii %p, sphere_count %lu, flags %#lx, return_values %p.\n",
            iface, centers, radii, sphere_count, flags, return_values);

    prepare_clip_space_planes(impl_from_IDirect3DDevice7(iface), plane);

    IDirect3DDevice7_GetRenderState(iface, D3DRENDERSTATE_CLIPPLANEENABLE, &user_clip_planes);
    enabled_planes |= user_clip_planes << 6;
    for (j = 6; j < 12; ++j)
        IDirect3DDevice7_GetClipPlane(iface, j - 6, (D3DVALUE *)&plane[j]);

    compute_sphere_visibility(plane, enabled_planes, FALSE, centers, radii, sphere_count, return_values);
    return D3D_OK;
}

static HRESULT WINAPI d3d_device3_ComputeSphereVisibility(IDirect3DDevice3 *iface,
        D3DVECTOR *centers, D3DVALUE *radii, DWORD sphere_count, DWORD flags, DWORD *return_values)
{
    static const DWORD enabled_planes = 0x3f;
    struct wined3d_vec4 plane[6];
    unsigned int i, j;

    TRACE("iface %p, centers %p, radii %p, sphere_count %lu, flags %#lx, return_values %p.\n",
            iface, centers, radii, sphere_count, flags, return_values);

    prepare_clip_space_planes(impl_from_IDirect3DDevice3(iface), plane);

    compute_sphere_visibility(plane, enabled_planes, TRUE, centers, radii, sphere_count, return_values);
    for (i = 0; i < sphere_count; ++i)
    {
        BOOL intersect_frustum = FALSE, outside_frustum = FALSE;
        DWORD d3d7_result = return_values[i];

        return_values[i] = 0;

        for (j = 0; j < 6; ++j)
        {
            DWORD clip = (d3d7_result >> j) & (D3DSTATUS_CLIPUNIONLEFT | D3DSTATUS_CLIPINTERSECTIONLEFT);

            if (clip == D3DSTATUS_CLIPUNIONLEFT)
            {
                return_values[i] |= D3DVIS_INTERSECT_LEFT << j * 2;
                intersect_frustum = TRUE;
            }
            else if (clip)
            {
                return_values[i] |= D3DVIS_OUTSIDE_LEFT << j * 2;
                outside_frustum = TRUE;
            }
        }
        if (outside_frustum)
            return_values[i] |= D3DVIS_OUTSIDE_FRUSTUM;
        else if (intersect_frustum)
            return_values[i] |= D3DVIS_INTERSECT_FRUSTUM;
    }
    return D3D_OK;
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetTexture(IDirect3DDevice7 *iface,
        DWORD stage, IDirectDrawSurface7 **texture)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_texture *wined3d_texture;
    struct ddraw_texture *ddraw_texture;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, texture);

    if (!texture)
        return DDERR_INVALIDPARAMS;

    if (stage >= DDRAW_MAX_TEXTURES)
    {
        WARN("Invalid stage %lu.\n", stage);
        *texture = NULL;
        return D3D_OK;
    }

    wined3d_mutex_lock();
    if (!(wined3d_texture = device->stateblock_state->textures[stage]))
    {
        *texture = NULL;
        wined3d_mutex_unlock();
        return D3D_OK;
    }

    ddraw_texture = wined3d_texture_get_parent(wined3d_texture);
    *texture = &ddraw_texture->root->IDirectDrawSurface7_iface;
    IDirectDrawSurface7_AddRef(*texture);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_GetTexture_FPUSetup(IDirect3DDevice7 *iface,
        DWORD stage, IDirectDrawSurface7 **Texture)
{
    return d3d_device7_GetTexture(iface, stage, Texture);
}

static HRESULT WINAPI d3d_device7_GetTexture_FPUPreserve(IDirect3DDevice7 *iface,
        DWORD stage, IDirectDrawSurface7 **Texture)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetTexture(iface, stage, Texture);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_GetTexture(IDirect3DDevice3 *iface, DWORD stage, IDirect3DTexture2 **Texture2)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    struct ddraw_surface *ret_val_impl;
    HRESULT ret;
    IDirectDrawSurface7 *ret_val;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, Texture2);

    ret = IDirect3DDevice7_GetTexture(&device->IDirect3DDevice7_iface, stage, &ret_val);

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
 *
 *****************************************************************************/
static HRESULT d3d_device7_SetTexture(IDirect3DDevice7 *iface,
        DWORD stage, IDirectDrawSurface7 *texture)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct ddraw_surface *surf = unsafe_impl_from_IDirectDrawSurface7(texture);
    struct wined3d_texture *wined3d_texture = NULL;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, texture);

    if (surf && (surf->surface_desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE))
    {
        if (surf->draw_texture)
        {
            wined3d_texture = surf->draw_texture;
            device->have_draw_textures = TRUE;
        }
        else
        {
            wined3d_texture = surf->wined3d_texture;
        }
    }

    wined3d_mutex_lock();
    wined3d_stateblock_set_texture(device->update_state, stage, wined3d_texture);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_SetTexture_FPUSetup(IDirect3DDevice7 *iface,
        DWORD stage, IDirectDrawSurface7 *texture)
{
    return d3d_device7_SetTexture(iface, stage, texture);
}

static HRESULT WINAPI d3d_device7_SetTexture_FPUPreserve(IDirect3DDevice7 *iface,
        DWORD stage, IDirectDrawSurface7 *texture)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetTexture(iface, stage, texture);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_SetTexture(IDirect3DDevice3 *iface,
        DWORD stage, IDirect3DTexture2 *texture)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    struct ddraw_surface *tex = unsafe_impl_from_IDirect3DTexture2(texture);
    struct wined3d_texture *wined3d_texture = NULL;

    TRACE("iface %p, stage %lu, texture %p.\n", iface, stage, texture);

    wined3d_mutex_lock();

    if (tex && ((tex->surface_desc.ddsCaps.dwCaps & DDSCAPS_TEXTURE) || !device->hardware_device))
    {
        if (tex->draw_texture)
        {
            wined3d_texture = tex->draw_texture;
            device->have_draw_textures = TRUE;
        }
        else
        {
            wined3d_texture = tex->wined3d_texture;
        }
    }

    wined3d_stateblock_set_texture(device->state, stage, wined3d_texture);
    fixup_texture_alpha_op(device);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static const struct tss_lookup
{
    BOOL sampler_state;
    union
    {
        enum wined3d_texture_stage_state texture_state;
        enum wined3d_sampler_state sampler_state;
    } u;
}
tss_lookup[] =
{
    {FALSE, .u.texture_state = WINED3D_TSS_INVALID},                   /*  0, unused */
    {FALSE, .u.texture_state = WINED3D_TSS_COLOR_OP},                  /*  1, D3DTSS_COLOROP */
    {FALSE, .u.texture_state = WINED3D_TSS_COLOR_ARG1},                /*  2, D3DTSS_COLORARG1 */
    {FALSE, .u.texture_state = WINED3D_TSS_COLOR_ARG2},                /*  3, D3DTSS_COLORARG2 */
    {FALSE, .u.texture_state = WINED3D_TSS_ALPHA_OP},                  /*  4, D3DTSS_ALPHAOP */
    {FALSE, .u.texture_state = WINED3D_TSS_ALPHA_ARG1},                /*  5, D3DTSS_ALPHAARG1 */
    {FALSE, .u.texture_state = WINED3D_TSS_ALPHA_ARG2},                /*  6, D3DTSS_ALPHAARG2 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT00},             /*  7, D3DTSS_BUMPENVMAT00 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT01},             /*  8, D3DTSS_BUMPENVMAT01 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT10},             /*  9, D3DTSS_BUMPENVMAT10 */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_MAT11},             /* 10, D3DTSS_BUMPENVMAT11 */
    {FALSE, .u.texture_state = WINED3D_TSS_TEXCOORD_INDEX},            /* 11, D3DTSS_TEXCOORDINDEX */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_ADDRESS_U},                /* 12, D3DTSS_ADDRESS */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_ADDRESS_U},                /* 13, D3DTSS_ADDRESSU */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_ADDRESS_V},                /* 14, D3DTSS_ADDRESSV */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_BORDER_COLOR},             /* 15, D3DTSS_BORDERCOLOR */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MAG_FILTER},               /* 16, D3DTSS_MAGFILTER */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MIN_FILTER},               /* 17, D3DTSS_MINFILTER */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MIP_FILTER},               /* 18, D3DTSS_MIPFILTER */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MIPMAP_LOD_BIAS},          /* 19, D3DTSS_MIPMAPLODBIAS */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MAX_MIP_LEVEL},            /* 20, D3DTSS_MAXMIPLEVEL */
    {TRUE,  .u.sampler_state = WINED3D_SAMP_MAX_ANISOTROPY},           /* 21, D3DTSS_MAXANISOTROPY */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_LSCALE},            /* 22, D3DTSS_BUMPENVLSCALE */
    {FALSE, .u.texture_state = WINED3D_TSS_BUMPENV_LOFFSET},           /* 23, D3DTSS_BUMPENVLOFFSET */
    {FALSE, .u.texture_state = WINED3D_TSS_TEXTURE_TRANSFORM_FLAGS},   /* 24, D3DTSS_TEXTURETRANSFORMFLAGS */
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetTextureStageState(IDirect3DDevice7 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD *value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    const struct wined3d_stateblock_state *device_state;
    const struct tss_lookup *l;

    TRACE("iface %p, stage %lu, state %#x, value %p.\n",
            iface, stage, state, value);

    if (!value)
        return DDERR_INVALIDPARAMS;

    if (state > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        WARN("Invalid state %#x passed.\n", state);
        return DD_OK;
    }

    if (stage >= DDRAW_MAX_TEXTURES)
    {
        WARN("Invalid stage %lu.\n", stage);
        *value = 0;
        return D3D_OK;
    }

    l = &tss_lookup[state];

    wined3d_mutex_lock();

    device_state = device->stateblock_state;

    if (l->sampler_state)
    {
        *value = device_state->sampler_states[stage][l->u.sampler_state];

        switch (state)
        {
            /* Mipfilter is a sampler state with different values */
            case D3DTSS_MIPFILTER:
            {
                switch (*value)
                {
                    case WINED3D_TEXF_NONE:
                        *value = D3DTFP_NONE;
                        break;
                    case WINED3D_TEXF_POINT:
                        *value = D3DTFP_POINT;
                        break;
                    case WINED3D_TEXF_LINEAR:
                        *value = D3DTFP_LINEAR;
                        break;
                    default:
                        ERR("Unexpected mipfilter value %#lx.\n", *value);
                        *value = D3DTFP_NONE;
                        break;
                }
                break;
            }

            /* Magfilter has slightly different values */
            case D3DTSS_MAGFILTER:
            {
                switch (*value)
                {
                    case WINED3D_TEXF_POINT:
                            *value = D3DTFG_POINT;
                            break;
                    case WINED3D_TEXF_LINEAR:
                            *value = D3DTFG_LINEAR;
                            break;
                    case WINED3D_TEXF_ANISOTROPIC:
                            *value = D3DTFG_ANISOTROPIC;
                            break;
                    case WINED3D_TEXF_FLAT_CUBIC:
                            *value = D3DTFG_FLATCUBIC;
                            break;
                    case WINED3D_TEXF_GAUSSIAN_CUBIC:
                            *value = D3DTFG_GAUSSIANCUBIC;
                            break;
                    default:
                        ERR("Unexpected wined3d mag filter value %#lx.\n", *value);
                        *value = D3DTFG_POINT;
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
        *value = device_state->texture_states[stage][l->u.texture_state];
    }

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_GetTextureStageState_FPUSetup(IDirect3DDevice7 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD *value)
{
    return d3d_device7_GetTextureStageState(iface, stage, state, value);
}

static HRESULT WINAPI d3d_device7_GetTextureStageState_FPUPreserve(IDirect3DDevice7 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD *value)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetTextureStageState(iface, stage, state, value);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_GetTextureStageState(IDirect3DDevice3 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD *value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, stage %lu, state %#x, value %p.\n",
            iface, stage, state, value);

    return IDirect3DDevice7_GetTextureStageState(&device->IDirect3DDevice7_iface, stage, state, value);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_SetTextureStageState(IDirect3DDevice7 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    const struct tss_lookup *l;

    TRACE("iface %p, stage %lu, state %#x, value %#lx.\n",
            iface, stage, state, value);

    if (state > D3DTSS_TEXTURETRANSFORMFLAGS)
    {
        WARN("Invalid state %#x passed.\n", state);
        return DD_OK;
    }

    l = &tss_lookup[state];

    wined3d_mutex_lock();

    if (l->sampler_state)
    {
        switch (state)
        {
            /* Mipfilter is a sampler state with different values */
            case D3DTSS_MIPFILTER:
            {
                switch (value)
                {
                    case D3DTFP_NONE:
                        value = WINED3D_TEXF_NONE;
                        break;
                    case D3DTFP_POINT:
                        value = WINED3D_TEXF_POINT;
                        break;
                    case 0: /* Unchecked */
                    case D3DTFP_LINEAR:
                        value = WINED3D_TEXF_LINEAR;
                        break;
                    default:
                        ERR("Unexpected mipfilter value %#lx.\n", value);
                        value = WINED3D_TEXF_NONE;
                        break;
                }
                break;
            }

            /* Magfilter has slightly different values */
            case D3DTSS_MAGFILTER:
            {
                switch (value)
                {
                    case D3DTFG_POINT:
                        value = WINED3D_TEXF_POINT;
                        break;
                    case D3DTFG_LINEAR:
                        value = WINED3D_TEXF_LINEAR;
                        break;
                    case D3DTFG_FLATCUBIC:
                        value = WINED3D_TEXF_FLAT_CUBIC;
                        break;
                    case D3DTFG_GAUSSIANCUBIC:
                        value = WINED3D_TEXF_GAUSSIAN_CUBIC;
                        break;
                    case D3DTFG_ANISOTROPIC:
                        value = WINED3D_TEXF_ANISOTROPIC;
                        break;
                    default:
                        ERR("Unexpected d3d7 mag filter value %#lx.\n", value);
                        value = WINED3D_TEXF_POINT;
                        break;
                }
                break;
            }

            case D3DTSS_ADDRESS:
                wined3d_stateblock_set_sampler_state(device->state, stage, WINED3D_SAMP_ADDRESS_V, value);
                break;

            default:
                break;
        }

        wined3d_stateblock_set_sampler_state(device->state, stage, l->u.sampler_state, value);
    }
    else
        wined3d_stateblock_set_texture_stage_state(device->update_state, stage, l->u.texture_state, value);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_SetTextureStageState_FPUSetup(IDirect3DDevice7 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
    return d3d_device7_SetTextureStageState(iface, stage, state, value);
}

static HRESULT WINAPI d3d_device7_SetTextureStageState_FPUPreserve(IDirect3DDevice7 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetTextureStageState(iface, stage, state, value);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_SetTextureStageState(IDirect3DDevice3 *iface,
        DWORD stage, D3DTEXTURESTAGESTATETYPE state, DWORD value)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);
    DWORD old_value;
    HRESULT hr;

    TRACE("iface %p, stage %lu, state %#x, value %#lx.\n",
            iface, stage, state, value);

    /* Tests show that legacy texture blending is not reset if the texture stage state
     * value is unchanged. */
    if (FAILED(hr = IDirect3DDevice7_GetTextureStageState(&device->IDirect3DDevice7_iface,
                stage, state, &old_value)))
        return hr;

    if (old_value == value)
    {
        TRACE("Application is setting the same value over, nothing to do.\n");
        return D3D_OK;
    }

    device->legacyTextureBlending = FALSE;

    return IDirect3DDevice7_SetTextureStageState(&device->IDirect3DDevice7_iface, stage, state, value);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_ValidateDevice(IDirect3DDevice7 *iface, DWORD *pass_count)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, pass_count %p.\n", iface, pass_count);

    wined3d_mutex_lock();
    hr = wined3d_device_validate_device(device->wined3d_device, device->stateblock_state, pass_count);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device7_ValidateDevice_FPUSetup(IDirect3DDevice7 *iface, DWORD *pass_count)
{
    return d3d_device7_ValidateDevice(iface, pass_count);
}

static HRESULT WINAPI d3d_device7_ValidateDevice_FPUPreserve(IDirect3DDevice7 *iface, DWORD *pass_count)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_ValidateDevice(iface, pass_count);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT WINAPI d3d_device3_ValidateDevice(IDirect3DDevice3 *iface, DWORD *pass_count)
{
    struct d3d_device *device = impl_from_IDirect3DDevice3(iface);

    TRACE("iface %p, pass_count %p.\n", iface, pass_count);

    return IDirect3DDevice7_ValidateDevice(&device->IDirect3DDevice7_iface, pass_count);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_Clear(IDirect3DDevice7 *iface, DWORD count,
        D3DRECT *rects, DWORD flags, D3DCOLOR color, D3DVALUE z, DWORD stencil)
{
    const struct wined3d_color c =
    {
        ((color >> 16) & 0xff) / 255.0f,
        ((color >>  8) & 0xff) / 255.0f,
        (color & 0xff) / 255.0f,
        ((color >> 24) & 0xff) / 255.0f,
    };
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, count %lu, rects %p, flags %#lx, color 0x%08lx, z %.8e, stencil %#lx.\n",
            iface, count, rects, flags, color, z, stencil);

    if (count && !rects)
    {
        WARN("count %lu with NULL rects.\n", count);
        count = 0;
    }

    wined3d_mutex_lock();
    d3d_device_apply_state(device, TRUE);
    hr = wined3d_device_clear(device->wined3d_device, count, (RECT *)rects, flags, &c, z, stencil);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device7_Clear_FPUSetup(IDirect3DDevice7 *iface, DWORD count,
        D3DRECT *rects, DWORD flags, D3DCOLOR color, D3DVALUE z, DWORD stencil)
{
    return d3d_device7_Clear(iface, count, rects, flags, color, z, stencil);
}

static HRESULT WINAPI d3d_device7_Clear_FPUPreserve(IDirect3DDevice7 *iface, DWORD count,
        D3DRECT *rects, DWORD flags, D3DCOLOR color, D3DVALUE z, DWORD stencil)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_Clear(iface, count, rects, flags, color, z, stencil);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT d3d_device7_SetViewport(IDirect3DDevice7 *iface, D3DVIEWPORT7 *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_sub_resource_desc rt_desc;
    struct wined3d_rendertarget_view *rtv;
    struct ddraw_surface *surface;
    struct wined3d_viewport vp;

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    if (!viewport)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    rtv = device->target ? ddraw_surface_get_rendertarget_view(device->target) : NULL;
    if (!rtv)
    {
        wined3d_mutex_unlock();
        return DDERR_INVALIDCAPS;
    }
    surface = wined3d_rendertarget_view_get_sub_resource_parent(rtv);
    wined3d_texture_get_sub_resource_desc(surface->wined3d_texture, surface->sub_resource_idx, &rt_desc);

    if (!wined3d_bound_range(viewport->dwX, viewport->dwWidth, rt_desc.width)
            || !wined3d_bound_range(viewport->dwY, viewport->dwHeight, rt_desc.height))
    {
        WARN("Invalid viewport, returning E_INVALIDARG.\n");
        wined3d_mutex_unlock();
        return E_INVALIDARG;
    }

    vp.x = viewport->dwX;
    vp.y = viewport->dwY;
    vp.width = viewport->dwWidth;
    vp.height = viewport->dwHeight;
    vp.min_z = viewport->dvMinZ;
    vp.max_z = viewport->dvMaxZ;

    wined3d_stateblock_set_viewport(device->update_state, &vp);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_SetViewport_FPUSetup(IDirect3DDevice7 *iface, D3DVIEWPORT7 *viewport)
{
    return d3d_device7_SetViewport(iface, viewport);
}

static HRESULT WINAPI d3d_device7_SetViewport_FPUPreserve(IDirect3DDevice7 *iface, D3DVIEWPORT7 *viewport)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetViewport(iface, viewport);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static HRESULT d3d_device7_GetViewport(IDirect3DDevice7 *iface, D3DVIEWPORT7 *viewport)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_viewport wined3d_viewport;

    TRACE("iface %p, viewport %p.\n", iface, viewport);

    if (!viewport)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    wined3d_viewport = device->stateblock_state->viewport;
    wined3d_mutex_unlock();

    viewport->dwX = wined3d_viewport.x;
    viewport->dwY = wined3d_viewport.y;
    viewport->dwWidth = wined3d_viewport.width;
    viewport->dwHeight = wined3d_viewport.height;
    viewport->dvMinZ = wined3d_viewport.min_z;
    viewport->dvMaxZ = wined3d_viewport.max_z;

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_GetViewport_FPUSetup(IDirect3DDevice7 *iface, D3DVIEWPORT7 *viewport)
{
    return d3d_device7_GetViewport(iface, viewport);
}

static HRESULT WINAPI d3d_device7_GetViewport_FPUPreserve(IDirect3DDevice7 *iface, D3DVIEWPORT7 *viewport)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetViewport(iface, viewport);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_SetMaterial(IDirect3DDevice7 *iface, D3DMATERIAL7 *material)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    if (!material)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    /* Note: D3DMATERIAL7 is compatible with struct wined3d_material. */
    wined3d_stateblock_set_material(device->update_state, (const struct wined3d_material *)material);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_SetMaterial_FPUSetup(IDirect3DDevice7 *iface, D3DMATERIAL7 *material)
{
    return d3d_device7_SetMaterial(iface, material);
}

static HRESULT WINAPI d3d_device7_SetMaterial_FPUPreserve(IDirect3DDevice7 *iface, D3DMATERIAL7 *material)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetMaterial(iface, material);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetMaterial(IDirect3DDevice7 *iface, D3DMATERIAL7 *material)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, material %p.\n", iface, material);

    wined3d_mutex_lock();
    /* Note: D3DMATERIAL7 is compatible with struct wined3d_material. */
    memcpy(material, &device->stateblock_state->material, sizeof(*material));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_GetMaterial_FPUSetup(IDirect3DDevice7 *iface, D3DMATERIAL7 *material)
{
    return d3d_device7_GetMaterial(iface, material);
}

static HRESULT WINAPI d3d_device7_GetMaterial_FPUPreserve(IDirect3DDevice7 *iface, D3DMATERIAL7 *material)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetMaterial(iface, material);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_SetLight(IDirect3DDevice7 *iface, DWORD light_idx, D3DLIGHT7 *light)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, light_idx %lu, light %p.\n", iface, light_idx, light);

    wined3d_mutex_lock();
    /* Note: D3DLIGHT7 is compatible with struct wined3d_light. */
    hr = wined3d_stateblock_set_light(device->update_state, light_idx, (const struct wined3d_light *)light);
    wined3d_mutex_unlock();

    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI d3d_device7_SetLight_FPUSetup(IDirect3DDevice7 *iface, DWORD light_idx, D3DLIGHT7 *light)
{
    return d3d_device7_SetLight(iface, light_idx, light);
}

static HRESULT WINAPI d3d_device7_SetLight_FPUPreserve(IDirect3DDevice7 *iface, DWORD light_idx, D3DLIGHT7 *light)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetLight(iface, light_idx, light);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetLight(IDirect3DDevice7 *iface, DWORD light_idx, D3DLIGHT7 *light)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    BOOL enabled;
    HRESULT hr;

    TRACE("iface %p, light_idx %lu, light %p.\n", iface, light_idx, light);

    wined3d_mutex_lock();
    /* Note: D3DLIGHT7 is compatible with struct wined3d_light. */
    hr = wined3d_stateblock_get_light(device->state, light_idx, (struct wined3d_light *)light, &enabled);
    wined3d_mutex_unlock();

    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI d3d_device7_GetLight_FPUSetup(IDirect3DDevice7 *iface, DWORD light_idx, D3DLIGHT7 *light)
{
    return d3d_device7_GetLight(iface, light_idx, light);
}

static HRESULT WINAPI d3d_device7_GetLight_FPUPreserve(IDirect3DDevice7 *iface, DWORD light_idx, D3DLIGHT7 *light)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetLight(iface, light_idx, light);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_BeginStateBlock(IDirect3DDevice7 *iface)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to begin a stateblock while recording, returning D3DERR_INBEGINSTATEBLOCK.\n");
        return D3DERR_INBEGINSTATEBLOCK;
    }
    if (SUCCEEDED(hr = wined3d_stateblock_create(device->wined3d_device, NULL, WINED3D_SBT_RECORDED, &stateblock)))
        device->update_state = device->recording = stateblock;
    wined3d_mutex_unlock();

    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI d3d_device7_BeginStateBlock_FPUSetup(IDirect3DDevice7 *iface)
{
    return d3d_device7_BeginStateBlock(iface);
}

static HRESULT WINAPI d3d_device7_BeginStateBlock_FPUPreserve(IDirect3DDevice7 *iface)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_BeginStateBlock(iface);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_EndStateBlock(IDirect3DDevice7 *iface, DWORD *stateblock)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    DWORD h;

    TRACE("iface %p, stateblock %p.\n", iface, stateblock);

    if (!stateblock)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (!device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to end a stateblock, but no stateblock is being recorded.\n");
        return D3DERR_NOTINBEGINSTATEBLOCK;
    }
    wined3d_sb = device->recording;
    wined3d_stateblock_init_contained_states(wined3d_sb);
    device->recording = NULL;
    device->update_state = device->state;

    h = ddraw_allocate_handle(&device->handle_table, wined3d_sb, DDRAW_HANDLE_STATEBLOCK);
    if (h == DDRAW_INVALID_HANDLE)
    {
        ERR("Failed to allocate a stateblock handle.\n");
        wined3d_stateblock_decref(wined3d_sb);
        wined3d_mutex_unlock();
        *stateblock = 0;
        return DDERR_OUTOFMEMORY;
    }

    wined3d_mutex_unlock();
    *stateblock = h + 1;

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_EndStateBlock_FPUSetup(IDirect3DDevice7 *iface, DWORD *stateblock)
{
    return d3d_device7_EndStateBlock(iface, stateblock);
}

static HRESULT WINAPI d3d_device7_EndStateBlock_FPUPreserve(IDirect3DDevice7 *iface, DWORD *stateblock)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_EndStateBlock(iface, stateblock);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_PreLoad(IDirect3DDevice7 *iface, IDirectDrawSurface7 *texture)
{
    struct ddraw_surface *surface = unsafe_impl_from_IDirectDrawSurface7(texture);

    TRACE("iface %p, texture %p.\n", iface, texture);

    if (!texture)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    wined3d_resource_preload(wined3d_texture_get_resource(surface->draw_texture ? surface->draw_texture
            : surface->wined3d_texture));
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_PreLoad_FPUSetup(IDirect3DDevice7 *iface, IDirectDrawSurface7 *texture)
{
    return d3d_device7_PreLoad(iface, texture);
}

static HRESULT WINAPI d3d_device7_PreLoad_FPUPreserve(IDirect3DDevice7 *iface, IDirectDrawSurface7 *texture)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_PreLoad(iface, texture);
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
static HRESULT d3d_device7_ApplyStateBlock(IDirect3DDevice7 *iface, DWORD stateblock)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;

    TRACE("iface %p, stateblock %#lx.\n", iface, stateblock);

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to apply a stateblock while recording, returning D3DERR_INBEGINSTATEBLOCK.\n");
        return D3DERR_INBEGINSTATEBLOCK;
    }
    wined3d_sb = ddraw_get_object(&device->handle_table, stateblock - 1, DDRAW_HANDLE_STATEBLOCK);
    if (!wined3d_sb)
    {
        WARN("Invalid stateblock handle.\n");
        wined3d_mutex_unlock();
        return D3DERR_INVALIDSTATEBLOCK;
    }

    wined3d_stateblock_apply(wined3d_sb, device->state);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_ApplyStateBlock_FPUSetup(IDirect3DDevice7 *iface, DWORD stateblock)
{
    return d3d_device7_ApplyStateBlock(iface, stateblock);
}

static HRESULT WINAPI d3d_device7_ApplyStateBlock_FPUPreserve(IDirect3DDevice7 *iface, DWORD stateblock)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_ApplyStateBlock(iface, stateblock);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_CaptureStateBlock(IDirect3DDevice7 *iface, DWORD stateblock)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;

    TRACE("iface %p, stateblock %#lx.\n", iface, stateblock);

    wined3d_mutex_lock();
    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to capture a stateblock while recording, returning D3DERR_INBEGINSTATEBLOCK.\n");
        return D3DERR_INBEGINSTATEBLOCK;
    }
    wined3d_sb = ddraw_get_object(&device->handle_table, stateblock - 1, DDRAW_HANDLE_STATEBLOCK);
    if (!wined3d_sb)
    {
        WARN("Invalid stateblock handle.\n");
        wined3d_mutex_unlock();
        return D3DERR_INVALIDSTATEBLOCK;
    }

    wined3d_stateblock_capture(wined3d_sb, device->state);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_CaptureStateBlock_FPUSetup(IDirect3DDevice7 *iface, DWORD stateblock)
{
    return d3d_device7_CaptureStateBlock(iface, stateblock);
}

static HRESULT WINAPI d3d_device7_CaptureStateBlock_FPUPreserve(IDirect3DDevice7 *iface, DWORD stateblock)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_CaptureStateBlock(iface, stateblock);
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
static HRESULT d3d_device7_DeleteStateBlock(IDirect3DDevice7 *iface, DWORD stateblock)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    ULONG ref;

    TRACE("iface %p, stateblock %#lx.\n", iface, stateblock);

    wined3d_mutex_lock();

    wined3d_sb = ddraw_free_handle(&device->handle_table, stateblock - 1, DDRAW_HANDLE_STATEBLOCK);
    if (!wined3d_sb)
    {
        WARN("Invalid stateblock handle.\n");
        wined3d_mutex_unlock();
        return D3DERR_INVALIDSTATEBLOCK;
    }

    if ((ref = wined3d_stateblock_decref(wined3d_sb)))
    {
        ERR("Something is still holding stateblock %p (refcount %lu).\n", wined3d_sb, ref);
    }

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_DeleteStateBlock_FPUSetup(IDirect3DDevice7 *iface, DWORD stateblock)
{
    return d3d_device7_DeleteStateBlock(iface, stateblock);
}

static HRESULT WINAPI d3d_device7_DeleteStateBlock_FPUPreserve(IDirect3DDevice7 *iface, DWORD stateblock)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_DeleteStateBlock(iface, stateblock);
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
static HRESULT d3d_device7_CreateStateBlock(IDirect3DDevice7 *iface,
        D3DSTATEBLOCKTYPE type, DWORD *stateblock)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_stateblock *wined3d_sb;
    HRESULT hr;
    DWORD h;

    TRACE("iface %p, type %#x, stateblock %p.\n", iface, type, stateblock);

    if (!stateblock)
        return DDERR_INVALIDPARAMS;

    if (type != D3DSBT_ALL
            && type != D3DSBT_PIXELSTATE
            && type != D3DSBT_VERTEXSTATE)
    {
        WARN("Unexpected stateblock type, returning DDERR_INVALIDPARAMS\n");
        return DDERR_INVALIDPARAMS;
    }

    wined3d_mutex_lock();

    if (device->recording)
    {
        wined3d_mutex_unlock();
        WARN("Trying to apply a stateblock while recording, returning D3DERR_INBEGINSTATEBLOCK.\n");
        return D3DERR_INBEGINSTATEBLOCK;
    }

    if (FAILED(hr = wined3d_stateblock_create(device->wined3d_device,
            device->state, wined3d_stateblock_type_from_ddraw(type), &wined3d_sb)))
    {
        WARN("Failed to create stateblock, hr %#lx.\n", hr);
        wined3d_mutex_unlock();
        return hr_ddraw_from_wined3d(hr);
    }

    h = ddraw_allocate_handle(&device->handle_table, wined3d_sb, DDRAW_HANDLE_STATEBLOCK);
    if (h == DDRAW_INVALID_HANDLE)
    {
        ERR("Failed to allocate stateblock handle.\n");
        wined3d_stateblock_decref(wined3d_sb);
        wined3d_mutex_unlock();
        return DDERR_OUTOFMEMORY;
    }

    *stateblock = h + 1;
    wined3d_mutex_unlock();

    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI d3d_device7_CreateStateBlock_FPUSetup(IDirect3DDevice7 *iface,
        D3DSTATEBLOCKTYPE type, DWORD *stateblock)
{
    return d3d_device7_CreateStateBlock(iface, type, stateblock);
}

static HRESULT WINAPI d3d_device7_CreateStateBlock_FPUPreserve(IDirect3DDevice7 *iface,
        D3DSTATEBLOCKTYPE type, DWORD *stateblock)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_CreateStateBlock(iface, type, stateblock);
    set_fpu_control_word(old_fpucw);

    return hr;
}

static BOOL is_mip_level_subset(struct ddraw_surface *dest, struct ddraw_surface *src)
{
    struct ddraw_surface *src_level, *dest_level;
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

static void copy_mipmap_chain(struct d3d_device *device, struct ddraw_surface *dst,
        struct ddraw_surface *src, const POINT *DestPoint, const RECT *SrcRect)
{
    struct ddraw_surface *dst_level, *src_level;
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
    IDirectDrawSurface7_GetPalette(&dst->IDirectDrawSurface7_iface, &pal);

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
            IDirectDrawSurface7_SetColorKey(&dst->IDirectDrawSurface7_iface, ckeyflag, &ddckey);
        }
    }

    src_level = src;
    dst_level = dst;

    point = *DestPoint;
    src_rect = *SrcRect;

    for (;src_level && dst_level;)
    {
        if (src_level->surface_desc.dwWidth == dst_level->surface_desc.dwWidth
                && src_level->surface_desc.dwHeight == dst_level->surface_desc.dwHeight)
        {
            UINT src_w = src_rect.right - src_rect.left;
            UINT src_h = src_rect.bottom - src_rect.top;
            RECT dst_rect = {point.x, point.y, point.x + src_w, point.y + src_h};

            if (FAILED(hr = wined3d_device_context_blt(device->immediate_context,
                    ddraw_surface_get_any_texture(dst_level, DDRAW_SURFACE_RW), dst_level->sub_resource_idx, &dst_rect,
                    ddraw_surface_get_any_texture(src_level, DDRAW_SURFACE_READ),
                    src_level->sub_resource_idx, &src_rect, 0, NULL, WINED3D_TEXF_POINT)))
                ERR("Blit failed, hr %#lx.\n", hr);

            ddsd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
            ddsd.ddsCaps.dwCaps2 = DDSCAPS2_MIPMAPSUBLEVEL;
            IDirectDrawSurface7_GetAttachedSurface(&dst_level->IDirectDrawSurface7_iface, &ddsd.ddsCaps, &temp);

            if (dst_level != dst)
                IDirectDrawSurface7_Release(&dst_level->IDirectDrawSurface7_iface);

            dst_level = unsafe_impl_from_IDirectDrawSurface7(temp);
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

    if (src_level && src_level != src)
        IDirectDrawSurface7_Release(&src_level->IDirectDrawSurface7_iface);
    if (dst_level && dst_level != dst)
        IDirectDrawSurface7_Release(&dst_level->IDirectDrawSurface7_iface);
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
 *  DDERR_INVALIDPARAMS if dst_texture or src_texture is NULL, broken coordinates or anything unexpected.
 *
 *
 *****************************************************************************/
static HRESULT d3d_device7_Load(IDirect3DDevice7 *iface, IDirectDrawSurface7 *dst_texture, POINT *dst_pos,
        IDirectDrawSurface7 *src_texture, RECT *src_rect, DWORD flags)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct ddraw_surface *dest = unsafe_impl_from_IDirectDrawSurface7(dst_texture);
    struct ddraw_surface *src = unsafe_impl_from_IDirectDrawSurface7(src_texture);
    POINT destpoint;
    RECT srcrect;

    TRACE("iface %p, dst_texture %p, dst_pos %s, src_texture %p, src_rect %s, flags %#lx.\n",
            iface, dst_texture, wine_dbgstr_point(dst_pos), src_texture, wine_dbgstr_rect(src_rect), flags);

    if( (!src) || (!dest) )
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();

    if (!src_rect)
        SetRect(&srcrect, 0, 0, src->surface_desc.dwWidth, src->surface_desc.dwHeight);
    else
        srcrect = *src_rect;

    if (!dst_pos)
        destpoint.x = destpoint.y = 0;
    else
        destpoint = *dst_pos;

    /* Check bad dimensions. dst_pos is validated against src, not dest, because
     * destination can be a subset of mip levels, in which case actual coordinates used
     * for it may be divided. If any dimension of dest is larger than source, it can't be
     * mip level subset, so an error can be returned early.
     */
    if (IsRectEmpty(&srcrect) || srcrect.right > src->surface_desc.dwWidth ||
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
        struct ddraw_surface *src_face, *dest_face;
        DWORD src_face_flag, dest_face_flag;
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
                    else if (flags & dest_face_flag)
                    {
                        copy_mipmap_chain(device, dest_face, src_face, &destpoint, &srcrect);
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

    copy_mipmap_chain(device, dest, src, &destpoint, &srcrect);

    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_Load_FPUSetup(IDirect3DDevice7 *iface, IDirectDrawSurface7 *dst_texture,
        POINT *dst_pos, IDirectDrawSurface7 *src_texture, RECT *src_rect, DWORD flags)
{
    return d3d_device7_Load(iface, dst_texture, dst_pos, src_texture, src_rect, flags);
}

static HRESULT WINAPI d3d_device7_Load_FPUPreserve(IDirect3DDevice7 *iface, IDirectDrawSurface7 *dst_texture,
        POINT *dst_pos, IDirectDrawSurface7 *src_texture, RECT *src_rect, DWORD flags)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_Load(iface, dst_texture, dst_pos, src_texture, src_rect, flags);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_LightEnable(IDirect3DDevice7 *iface, DWORD light_idx, BOOL enabled)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    HRESULT hr;

    TRACE("iface %p, light_idx %lu, enabled %#x.\n", iface, light_idx, enabled);

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_light_enable(device->update_state, light_idx, enabled);
    wined3d_mutex_unlock();

    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI d3d_device7_LightEnable_FPUSetup(IDirect3DDevice7 *iface, DWORD light_idx, BOOL enabled)
{
    return d3d_device7_LightEnable(iface, light_idx, enabled);
}

static HRESULT WINAPI d3d_device7_LightEnable_FPUPreserve(IDirect3DDevice7 *iface, DWORD light_idx, BOOL enabled)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_LightEnable(iface, light_idx, enabled);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetLightEnable(IDirect3DDevice7 *iface, DWORD light_idx, BOOL *enabled)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    struct wined3d_light light;
    HRESULT hr;

    TRACE("iface %p, light_idx %lu, enabled %p.\n", iface, light_idx, enabled);

    if (!enabled)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    hr = wined3d_stateblock_get_light(device->state, light_idx, &light, enabled);
    wined3d_mutex_unlock();

    return hr_ddraw_from_wined3d(hr);
}

static HRESULT WINAPI d3d_device7_GetLightEnable_FPUSetup(IDirect3DDevice7 *iface, DWORD light_idx, BOOL *enabled)
{
    return d3d_device7_GetLightEnable(iface, light_idx, enabled);
}

static HRESULT WINAPI d3d_device7_GetLightEnable_FPUPreserve(IDirect3DDevice7 *iface, DWORD light_idx, BOOL *enabled)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetLightEnable(iface, light_idx, enabled);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_SetClipPlane(IDirect3DDevice7 *iface, DWORD idx, D3DVALUE *plane)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);
    const struct wined3d_vec4 *wined3d_plane;
    HRESULT hr;

    TRACE("iface %p, idx %lu, plane %p.\n", iface, idx, plane);

    if (!plane)
        return DDERR_INVALIDPARAMS;

    wined3d_plane = (struct wined3d_vec4 *)plane;

    wined3d_mutex_lock();
    hr = wined3d_stateblock_set_clip_plane(device->update_state, idx, wined3d_plane);
    if (idx < ARRAY_SIZE(device->user_clip_planes))
    {
        device->user_clip_planes[idx] = *wined3d_plane;
        if (hr == WINED3DERR_INVALIDCALL)
        {
            WARN("Clip plane %lu is not supported.\n", idx);
            hr = D3D_OK;
        }
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI d3d_device7_SetClipPlane_FPUSetup(IDirect3DDevice7 *iface, DWORD idx, D3DVALUE *plane)
{
    return d3d_device7_SetClipPlane(iface, idx, plane);
}

static HRESULT WINAPI d3d_device7_SetClipPlane_FPUPreserve(IDirect3DDevice7 *iface, DWORD idx, D3DVALUE *plane)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_SetClipPlane(iface, idx, plane);
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
 *
 *****************************************************************************/
static HRESULT d3d_device7_GetClipPlane(IDirect3DDevice7 *iface, DWORD idx, D3DVALUE *plane)
{
    struct d3d_device *device = impl_from_IDirect3DDevice7(iface);

    TRACE("iface %p, idx %lu, plane %p.\n", iface, idx, plane);

    if (!plane)
        return DDERR_INVALIDPARAMS;

    wined3d_mutex_lock();
    if (idx < WINED3D_MAX_CLIP_DISTANCES)
        memcpy(plane, &device->stateblock_state->clip_planes[idx], sizeof(struct wined3d_vec4));
    else
    {
        WARN("Clip plane %lu is not supported.\n", idx);
        if (idx < ARRAY_SIZE(device->user_clip_planes))
            memcpy(plane, &device->user_clip_planes[idx], sizeof(struct wined3d_vec4));
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI d3d_device7_GetClipPlane_FPUSetup(IDirect3DDevice7 *iface, DWORD idx, D3DVALUE *plane)
{
    return d3d_device7_GetClipPlane(iface, idx, plane);
}

static HRESULT WINAPI d3d_device7_GetClipPlane_FPUPreserve(IDirect3DDevice7 *iface, DWORD idx, D3DVALUE *plane)
{
    HRESULT hr;
    WORD old_fpucw;

    old_fpucw = d3d_fpu_setup();
    hr = d3d_device7_GetClipPlane(iface, idx, plane);
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
static HRESULT WINAPI d3d_device7_GetInfo(IDirect3DDevice7 *iface, DWORD info_id, void *info, DWORD info_size)
{
    TRACE("iface %p, info_id %#lx, info %p, info_size %lu.\n",
            iface, info_id, info, info_size);

    if (TRACE_ON(ddraw))
    {
        TRACE(" info requested : ");
        switch (info_id)
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
    d3d_device7_QueryInterface,
    d3d_device7_AddRef,
    d3d_device7_Release,
    /*** IDirect3DDevice7 ***/
    d3d_device7_GetCaps_FPUSetup,
    d3d_device7_EnumTextureFormats_FPUSetup,
    d3d_device7_BeginScene_FPUSetup,
    d3d_device7_EndScene_FPUSetup,
    d3d_device7_GetDirect3D,
    d3d_device7_SetRenderTarget_FPUSetup,
    d3d_device7_GetRenderTarget,
    d3d_device7_Clear_FPUSetup,
    d3d_device7_SetTransform_FPUSetup,
    d3d_device7_GetTransform_FPUSetup,
    d3d_device7_SetViewport_FPUSetup,
    d3d_device7_MultiplyTransform_FPUSetup,
    d3d_device7_GetViewport_FPUSetup,
    d3d_device7_SetMaterial_FPUSetup,
    d3d_device7_GetMaterial_FPUSetup,
    d3d_device7_SetLight_FPUSetup,
    d3d_device7_GetLight_FPUSetup,
    d3d_device7_SetRenderState_FPUSetup,
    d3d_device7_GetRenderState_FPUSetup,
    d3d_device7_BeginStateBlock_FPUSetup,
    d3d_device7_EndStateBlock_FPUSetup,
    d3d_device7_PreLoad_FPUSetup,
    d3d_device7_DrawPrimitive_FPUSetup,
    d3d_device7_DrawIndexedPrimitive_FPUSetup,
    d3d_device7_SetClipStatus,
    d3d_device7_GetClipStatus,
    d3d_device7_DrawPrimitiveStrided_FPUSetup,
    d3d_device7_DrawIndexedPrimitiveStrided_FPUSetup,
    d3d_device7_DrawPrimitiveVB_FPUSetup,
    d3d_device7_DrawIndexedPrimitiveVB_FPUSetup,
    d3d_device7_ComputeSphereVisibility,
    d3d_device7_GetTexture_FPUSetup,
    d3d_device7_SetTexture_FPUSetup,
    d3d_device7_GetTextureStageState_FPUSetup,
    d3d_device7_SetTextureStageState_FPUSetup,
    d3d_device7_ValidateDevice_FPUSetup,
    d3d_device7_ApplyStateBlock_FPUSetup,
    d3d_device7_CaptureStateBlock_FPUSetup,
    d3d_device7_DeleteStateBlock_FPUSetup,
    d3d_device7_CreateStateBlock_FPUSetup,
    d3d_device7_Load_FPUSetup,
    d3d_device7_LightEnable_FPUSetup,
    d3d_device7_GetLightEnable_FPUSetup,
    d3d_device7_SetClipPlane_FPUSetup,
    d3d_device7_GetClipPlane_FPUSetup,
    d3d_device7_GetInfo
};

static const struct IDirect3DDevice7Vtbl d3d_device7_fpu_preserve_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_device7_QueryInterface,
    d3d_device7_AddRef,
    d3d_device7_Release,
    /*** IDirect3DDevice7 ***/
    d3d_device7_GetCaps_FPUPreserve,
    d3d_device7_EnumTextureFormats_FPUPreserve,
    d3d_device7_BeginScene_FPUPreserve,
    d3d_device7_EndScene_FPUPreserve,
    d3d_device7_GetDirect3D,
    d3d_device7_SetRenderTarget_FPUPreserve,
    d3d_device7_GetRenderTarget,
    d3d_device7_Clear_FPUPreserve,
    d3d_device7_SetTransform_FPUPreserve,
    d3d_device7_GetTransform_FPUPreserve,
    d3d_device7_SetViewport_FPUPreserve,
    d3d_device7_MultiplyTransform_FPUPreserve,
    d3d_device7_GetViewport_FPUPreserve,
    d3d_device7_SetMaterial_FPUPreserve,
    d3d_device7_GetMaterial_FPUPreserve,
    d3d_device7_SetLight_FPUPreserve,
    d3d_device7_GetLight_FPUPreserve,
    d3d_device7_SetRenderState_FPUPreserve,
    d3d_device7_GetRenderState_FPUPreserve,
    d3d_device7_BeginStateBlock_FPUPreserve,
    d3d_device7_EndStateBlock_FPUPreserve,
    d3d_device7_PreLoad_FPUPreserve,
    d3d_device7_DrawPrimitive_FPUPreserve,
    d3d_device7_DrawIndexedPrimitive_FPUPreserve,
    d3d_device7_SetClipStatus,
    d3d_device7_GetClipStatus,
    d3d_device7_DrawPrimitiveStrided_FPUPreserve,
    d3d_device7_DrawIndexedPrimitiveStrided_FPUPreserve,
    d3d_device7_DrawPrimitiveVB_FPUPreserve,
    d3d_device7_DrawIndexedPrimitiveVB_FPUPreserve,
    d3d_device7_ComputeSphereVisibility,
    d3d_device7_GetTexture_FPUPreserve,
    d3d_device7_SetTexture_FPUPreserve,
    d3d_device7_GetTextureStageState_FPUPreserve,
    d3d_device7_SetTextureStageState_FPUPreserve,
    d3d_device7_ValidateDevice_FPUPreserve,
    d3d_device7_ApplyStateBlock_FPUPreserve,
    d3d_device7_CaptureStateBlock_FPUPreserve,
    d3d_device7_DeleteStateBlock_FPUPreserve,
    d3d_device7_CreateStateBlock_FPUPreserve,
    d3d_device7_Load_FPUPreserve,
    d3d_device7_LightEnable_FPUPreserve,
    d3d_device7_GetLightEnable_FPUPreserve,
    d3d_device7_SetClipPlane_FPUPreserve,
    d3d_device7_GetClipPlane_FPUPreserve,
    d3d_device7_GetInfo
};

static const struct IDirect3DDevice3Vtbl d3d_device3_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_device3_QueryInterface,
    d3d_device3_AddRef,
    d3d_device3_Release,
    /*** IDirect3DDevice3 ***/
    d3d_device3_GetCaps,
    d3d_device3_GetStats,
    d3d_device3_AddViewport,
    d3d_device3_DeleteViewport,
    d3d_device3_NextViewport,
    d3d_device3_EnumTextureFormats,
    d3d_device3_BeginScene,
    d3d_device3_EndScene,
    d3d_device3_GetDirect3D,
    d3d_device3_SetCurrentViewport,
    d3d_device3_GetCurrentViewport,
    d3d_device3_SetRenderTarget,
    d3d_device3_GetRenderTarget,
    d3d_device3_Begin,
    d3d_device3_BeginIndexed,
    d3d_device3_Vertex,
    d3d_device3_Index,
    d3d_device3_End,
    d3d_device3_GetRenderState,
    d3d_device3_SetRenderState,
    d3d_device3_GetLightState,
    d3d_device3_SetLightState,
    d3d_device3_SetTransform,
    d3d_device3_GetTransform,
    d3d_device3_MultiplyTransform,
    d3d_device3_DrawPrimitive,
    d3d_device3_DrawIndexedPrimitive,
    d3d_device3_SetClipStatus,
    d3d_device3_GetClipStatus,
    d3d_device3_DrawPrimitiveStrided,
    d3d_device3_DrawIndexedPrimitiveStrided,
    d3d_device3_DrawPrimitiveVB,
    d3d_device3_DrawIndexedPrimitiveVB,
    d3d_device3_ComputeSphereVisibility,
    d3d_device3_GetTexture,
    d3d_device3_SetTexture,
    d3d_device3_GetTextureStageState,
    d3d_device3_SetTextureStageState,
    d3d_device3_ValidateDevice
};

static const struct IDirect3DDevice2Vtbl d3d_device2_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_device2_QueryInterface,
    d3d_device2_AddRef,
    d3d_device2_Release,
    /*** IDirect3DDevice2 ***/
    d3d_device2_GetCaps,
    d3d_device2_SwapTextureHandles,
    d3d_device2_GetStats,
    d3d_device2_AddViewport,
    d3d_device2_DeleteViewport,
    d3d_device2_NextViewport,
    d3d_device2_EnumTextureFormats,
    d3d_device2_BeginScene,
    d3d_device2_EndScene,
    d3d_device2_GetDirect3D,
    d3d_device2_SetCurrentViewport,
    d3d_device2_GetCurrentViewport,
    d3d_device2_SetRenderTarget,
    d3d_device2_GetRenderTarget,
    d3d_device2_Begin,
    d3d_device2_BeginIndexed,
    d3d_device2_Vertex,
    d3d_device2_Index,
    d3d_device2_End,
    d3d_device2_GetRenderState,
    d3d_device2_SetRenderState,
    d3d_device2_GetLightState,
    d3d_device2_SetLightState,
    d3d_device2_SetTransform,
    d3d_device2_GetTransform,
    d3d_device2_MultiplyTransform,
    d3d_device2_DrawPrimitive,
    d3d_device2_DrawIndexedPrimitive,
    d3d_device2_SetClipStatus,
    d3d_device2_GetClipStatus
};

static const struct IDirect3DDeviceVtbl d3d_device1_vtbl =
{
    /*** IUnknown Methods ***/
    d3d_device1_QueryInterface,
    d3d_device1_AddRef,
    d3d_device1_Release,
    /*** IDirect3DDevice1 ***/
    d3d_device1_Initialize,
    d3d_device1_GetCaps,
    d3d_device1_SwapTextureHandles,
    d3d_device1_CreateExecuteBuffer,
    d3d_device1_GetStats,
    d3d_device1_Execute,
    d3d_device1_AddViewport,
    d3d_device1_DeleteViewport,
    d3d_device1_NextViewport,
    d3d_device1_Pick,
    d3d_device1_GetPickRecords,
    d3d_device1_EnumTextureFormats,
    d3d_device1_CreateMatrix,
    d3d_device1_SetMatrix,
    d3d_device1_GetMatrix,
    d3d_device1_DeleteMatrix,
    d3d_device1_BeginScene,
    d3d_device1_EndScene,
    d3d_device1_GetDirect3D
};

static const struct IUnknownVtbl d3d_device_inner_vtbl =
{
    d3d_device_inner_QueryInterface,
    d3d_device_inner_AddRef,
    d3d_device_inner_Release,
};

struct d3d_device *unsafe_impl_from_IDirect3DDevice7(IDirect3DDevice7 *iface)
{
    if (!iface) return NULL;
    assert((iface->lpVtbl == &d3d_device7_fpu_preserve_vtbl) || (iface->lpVtbl == &d3d_device7_fpu_setup_vtbl));
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice7_iface);
}

struct d3d_device *unsafe_impl_from_IDirect3DDevice3(IDirect3DDevice3 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_device3_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice3_iface);
}

struct d3d_device *unsafe_impl_from_IDirect3DDevice2(IDirect3DDevice2 *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_device2_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice2_iface);
}

struct d3d_device *unsafe_impl_from_IDirect3DDevice(IDirect3DDevice *iface)
{
    if (!iface) return NULL;
    assert(iface->lpVtbl == &d3d_device1_vtbl);
    return CONTAINING_RECORD(iface, struct d3d_device, IDirect3DDevice_iface);
}

enum wined3d_depth_buffer_type d3d_device_update_depth_stencil(struct d3d_device *device)
{
    IDirectDrawSurface7 *depthStencil = NULL;
    IDirectDrawSurface7 *render_target;
    static DDSCAPS2 depthcaps = { DDSCAPS_ZBUFFER, 0, 0, {0} };

    if (device->rt_iface && SUCCEEDED(IUnknown_QueryInterface(device->rt_iface,
            &IID_IDirectDrawSurface7, (void **)&render_target)))
    {
        IDirectDrawSurface7_GetAttachedSurface(render_target, &depthcaps, &depthStencil);
        IDirectDrawSurface7_Release(render_target);
    }
    if (!depthStencil)
    {
        TRACE("Setting wined3d depth stencil to NULL\n");
        wined3d_device_context_set_depth_stencil_view(device->immediate_context, NULL);
        wined3d_stateblock_depth_buffer_changed(device->state);
        device->target_ds = NULL;
        return WINED3D_ZB_FALSE;
    }

    device->target_ds = impl_from_IDirectDrawSurface7(depthStencil);
    wined3d_device_context_set_depth_stencil_view(device->immediate_context,
            ddraw_surface_get_rendertarget_view(device->target_ds));
    wined3d_stateblock_depth_buffer_changed(device->state);

    IDirectDrawSurface7_Release(depthStencil);
    return WINED3D_ZB_TRUE;
}

static void device_reset_viewport_state(struct d3d_device *device)
{
    struct wined3d_viewport vp;
    RECT rect;

    wined3d_device_context_get_viewports(device->immediate_context, NULL, &vp);
    wined3d_stateblock_set_viewport(device->state, &vp);
    wined3d_device_context_get_scissor_rects(device->immediate_context, NULL, &rect);
    wined3d_stateblock_set_scissor_rect(device->state, &rect);
}

static HRESULT d3d_device_init(struct d3d_device *device, struct ddraw *ddraw, const GUID *guid,
        struct ddraw_surface *target, IUnknown *rt_iface, UINT version, IUnknown *outer_unknown)
{
    static const struct wined3d_matrix ident =
    {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    };
    struct wined3d_rendertarget_view *rtv;
    HRESULT hr;

    if (ddraw->cooperative_level & DDSCL_FPUPRESERVE)
        device->IDirect3DDevice7_iface.lpVtbl = &d3d_device7_fpu_preserve_vtbl;
    else
        device->IDirect3DDevice7_iface.lpVtbl = &d3d_device7_fpu_setup_vtbl;

    device->IDirect3DDevice3_iface.lpVtbl = &d3d_device3_vtbl;
    device->IDirect3DDevice2_iface.lpVtbl = &d3d_device2_vtbl;
    device->IDirect3DDevice_iface.lpVtbl = &d3d_device1_vtbl;
    device->IUnknown_inner.lpVtbl = &d3d_device_inner_vtbl;
    device->ref = 1;
    device->version = version;
    device->hardware_device = IsEqualGUID(&IID_IDirect3DTnLHalDevice, guid)
            || IsEqualGUID(&IID_IDirect3DHALDevice, guid);

    if (device->hardware_device && !(target->surface_desc.ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY))
    {
        WARN("Surface %p is not in video memory.\n", target);
        return D3DERR_SURFACENOTINVIDMEM;
    }

    if (outer_unknown)
        device->outer_unknown = outer_unknown;
    else
        device->outer_unknown = &device->IUnknown_inner;

    device->ddraw = ddraw;
    list_init(&device->viewport_list);

    if (!ddraw_handle_table_init(&device->handle_table, 64))
    {
        ERR("Failed to initialize handle table.\n");
        return DDERR_OUTOFMEMORY;
    }

    device->legacyTextureBlending = FALSE;
    device->legacy_projection = ident;
    device->legacy_clipspace = ident;

    if (FAILED(hr = wined3d_stateblock_create(ddraw->wined3d_device, NULL, WINED3D_SBT_PRIMARY, &device->state)))
    {
        ERR("Failed to create the primary stateblock, hr %#lx.\n", hr);
        ddraw_handle_table_destroy(&device->handle_table);
        return hr;
    }
    device->stateblock_state = wined3d_stateblock_get_state(device->state);
    device->update_state = device->state;

    /* This is for convenience. */
    device->wined3d_device = ddraw->wined3d_device;
    device->immediate_context = ddraw->immediate_context;
    wined3d_device_incref(ddraw->wined3d_device);

    wined3d_streaming_buffer_init(&device->vertex_buffer, WINED3D_BIND_VERTEX_BUFFER);
    wined3d_streaming_buffer_init(&device->index_buffer, WINED3D_BIND_INDEX_BUFFER);

    /* Render to the back buffer */
    rtv = ddraw_surface_get_rendertarget_view(target);
    if (FAILED(hr = wined3d_device_context_set_rendertarget_views(device->immediate_context, 0, 1, &rtv, TRUE)))
    {
        ERR("Failed to set render target, hr %#lx.\n", hr);
        wined3d_stateblock_decref(device->state);
        ddraw_handle_table_destroy(&device->handle_table);
        return hr;
    }

    device->rt_iface = rt_iface;
    device->target = target;
    if (version != 1)
        IUnknown_AddRef(device->rt_iface);

    list_add_head(&ddraw->d3ddevice_list, &device->ddraw_entry);

    wined3d_stateblock_set_render_state(device->state, WINED3D_RS_ZENABLE,
            d3d_device_update_depth_stencil(device));
    if (version == 1) /* Color keying is initially enabled for version 1 devices. */
        wined3d_stateblock_set_render_state(device->state, WINED3D_RS_COLORKEYENABLE, TRUE);
    else if (version == 2)
        wined3d_stateblock_set_render_state(device->state, WINED3D_RS_SPECULARENABLE, TRUE);
    if (version < 7)
    {
        wined3d_stateblock_set_render_state(device->state, WINED3D_RS_NORMALIZENORMALS, TRUE);
        IDirect3DDevice3_SetRenderState(&device->IDirect3DDevice3_iface,
                D3DRENDERSTATE_TEXTUREMAPBLEND, D3DTBLEND_MODULATE);
    }
    device_reset_viewport_state(device);
    return D3D_OK;
}

HRESULT d3d_device_create(struct ddraw *ddraw, const GUID *guid, struct ddraw_surface *target, IUnknown *rt_iface,
        UINT version, struct d3d_device **device, IUnknown *outer_unknown)
{
    struct d3d_device *object;
    HRESULT hr;

    TRACE("ddraw %p, guid %s, target %p, version %u, device %p, outer_unknown %p.\n",
            ddraw, debugstr_guid(guid), target, version, device, outer_unknown);

    if (!(target->surface_desc.ddsCaps.dwCaps & DDSCAPS_3DDEVICE)
            || (target->surface_desc.ddsCaps.dwCaps & DDSCAPS_ZBUFFER))
    {
        WARN("Surface %p is not a render target.\n", target);
        return DDERR_INVALIDCAPS;
    }

    if (!validate_surface_palette(target))
    {
        WARN("Surface %p has an indexed pixel format, but no palette.\n", target);
        return DDERR_NOPALETTEATTACHED;
    }

    if (ddraw->flags & DDRAW_NO3D)
    {
        ERR_(winediag)("The application wants to create a Direct3D device, "
                "but the current DirectDrawRenderer does not support this.\n");

        return DDERR_OUTOFMEMORY;
    }

    if (!(object = calloc(1, sizeof(*object))))
    {
        ERR("Failed to allocate device memory.\n");
        return DDERR_OUTOFMEMORY;
    }

    if (FAILED(hr = d3d_device_init(object, ddraw, guid, target, rt_iface, version, outer_unknown)))
    {
        WARN("Failed to initialize device, hr %#lx.\n", hr);
        free(object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = object;

    return D3D_OK;
}
