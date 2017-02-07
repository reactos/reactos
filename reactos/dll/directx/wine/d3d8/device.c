/*
 * IDirect3DDevice8 implementation
 *
 * Copyright 2002-2004 Jason Edmeades
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

#include <math.h>
#include <stdarg.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wingdi.h"
#include "wine/debug.h"

#include "d3d8_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d8);

D3DFORMAT d3dformat_from_wined3dformat(enum wined3d_format_id format)
{
    BYTE *c = (BYTE *)&format;

    /* Don't translate FOURCC formats */
    if (isprint(c[0]) && isprint(c[1]) && isprint(c[2]) && isprint(c[3])) return format;

    switch(format)
    {
        case WINED3DFMT_UNKNOWN: return D3DFMT_UNKNOWN;
        case WINED3DFMT_B8G8R8_UNORM: return D3DFMT_R8G8B8;
        case WINED3DFMT_B8G8R8A8_UNORM: return D3DFMT_A8R8G8B8;
        case WINED3DFMT_B8G8R8X8_UNORM: return D3DFMT_X8R8G8B8;
        case WINED3DFMT_B5G6R5_UNORM: return D3DFMT_R5G6B5;
        case WINED3DFMT_B5G5R5X1_UNORM: return D3DFMT_X1R5G5B5;
        case WINED3DFMT_B5G5R5A1_UNORM: return D3DFMT_A1R5G5B5;
        case WINED3DFMT_B4G4R4A4_UNORM: return D3DFMT_A4R4G4B4;
        case WINED3DFMT_B2G3R3_UNORM: return D3DFMT_R3G3B2;
        case WINED3DFMT_A8_UNORM: return D3DFMT_A8;
        case WINED3DFMT_B2G3R3A8_UNORM: return D3DFMT_A8R3G3B2;
        case WINED3DFMT_B4G4R4X4_UNORM: return D3DFMT_X4R4G4B4;
        case WINED3DFMT_R10G10B10A2_UNORM: return D3DFMT_A2B10G10R10;
        case WINED3DFMT_R16G16_UNORM: return D3DFMT_G16R16;
        case WINED3DFMT_P8_UINT_A8_UNORM: return D3DFMT_A8P8;
        case WINED3DFMT_P8_UINT: return D3DFMT_P8;
        case WINED3DFMT_L8_UNORM: return D3DFMT_L8;
        case WINED3DFMT_L8A8_UNORM: return D3DFMT_A8L8;
        case WINED3DFMT_L4A4_UNORM: return D3DFMT_A4L4;
        case WINED3DFMT_R8G8_SNORM: return D3DFMT_V8U8;
        case WINED3DFMT_R5G5_SNORM_L6_UNORM: return D3DFMT_L6V5U5;
        case WINED3DFMT_R8G8_SNORM_L8X8_UNORM: return D3DFMT_X8L8V8U8;
        case WINED3DFMT_R8G8B8A8_SNORM: return D3DFMT_Q8W8V8U8;
        case WINED3DFMT_R16G16_SNORM: return D3DFMT_V16U16;
        case WINED3DFMT_R10G11B11_SNORM: return D3DFMT_W11V11U10;
        case WINED3DFMT_R10G10B10_SNORM_A2_UNORM: return D3DFMT_A2W10V10U10;
        case WINED3DFMT_D16_LOCKABLE: return D3DFMT_D16_LOCKABLE;
        case WINED3DFMT_D32_UNORM: return D3DFMT_D32;
        case WINED3DFMT_S1_UINT_D15_UNORM: return D3DFMT_D15S1;
        case WINED3DFMT_D24_UNORM_S8_UINT: return D3DFMT_D24S8;
        case WINED3DFMT_X8D24_UNORM: return D3DFMT_D24X8;
        case WINED3DFMT_S4X4_UINT_D24_UNORM: return D3DFMT_D24X4S4;
        case WINED3DFMT_D16_UNORM: return D3DFMT_D16;
        case WINED3DFMT_VERTEXDATA: return D3DFMT_VERTEXDATA;
        case WINED3DFMT_R16_UINT: return D3DFMT_INDEX16;
        case WINED3DFMT_R32_UINT: return D3DFMT_INDEX32;
        default:
            FIXME("Unhandled wined3d format %#x.\n", format);
            return D3DFMT_UNKNOWN;
    }
}

enum wined3d_format_id wined3dformat_from_d3dformat(D3DFORMAT format)
{
    BYTE *c = (BYTE *)&format;

    /* Don't translate FOURCC formats */
    if (isprint(c[0]) && isprint(c[1]) && isprint(c[2]) && isprint(c[3])) return format;

    switch(format)
    {
        case D3DFMT_UNKNOWN: return WINED3DFMT_UNKNOWN;
        case D3DFMT_R8G8B8: return WINED3DFMT_B8G8R8_UNORM;
        case D3DFMT_A8R8G8B8: return WINED3DFMT_B8G8R8A8_UNORM;
        case D3DFMT_X8R8G8B8: return WINED3DFMT_B8G8R8X8_UNORM;
        case D3DFMT_R5G6B5: return WINED3DFMT_B5G6R5_UNORM;
        case D3DFMT_X1R5G5B5: return WINED3DFMT_B5G5R5X1_UNORM;
        case D3DFMT_A1R5G5B5: return WINED3DFMT_B5G5R5A1_UNORM;
        case D3DFMT_A4R4G4B4: return WINED3DFMT_B4G4R4A4_UNORM;
        case D3DFMT_R3G3B2: return WINED3DFMT_B2G3R3_UNORM;
        case D3DFMT_A8: return WINED3DFMT_A8_UNORM;
        case D3DFMT_A8R3G3B2: return WINED3DFMT_B2G3R3A8_UNORM;
        case D3DFMT_X4R4G4B4: return WINED3DFMT_B4G4R4X4_UNORM;
        case D3DFMT_A2B10G10R10: return WINED3DFMT_R10G10B10A2_UNORM;
        case D3DFMT_G16R16: return WINED3DFMT_R16G16_UNORM;
        case D3DFMT_A8P8: return WINED3DFMT_P8_UINT_A8_UNORM;
        case D3DFMT_P8: return WINED3DFMT_P8_UINT;
        case D3DFMT_L8: return WINED3DFMT_L8_UNORM;
        case D3DFMT_A8L8: return WINED3DFMT_L8A8_UNORM;
        case D3DFMT_A4L4: return WINED3DFMT_L4A4_UNORM;
        case D3DFMT_V8U8: return WINED3DFMT_R8G8_SNORM;
        case D3DFMT_L6V5U5: return WINED3DFMT_R5G5_SNORM_L6_UNORM;
        case D3DFMT_X8L8V8U8: return WINED3DFMT_R8G8_SNORM_L8X8_UNORM;
        case D3DFMT_Q8W8V8U8: return WINED3DFMT_R8G8B8A8_SNORM;
        case D3DFMT_V16U16: return WINED3DFMT_R16G16_SNORM;
        case D3DFMT_W11V11U10: return WINED3DFMT_R10G11B11_SNORM;
        case D3DFMT_A2W10V10U10: return WINED3DFMT_R10G10B10_SNORM_A2_UNORM;
        case D3DFMT_D16_LOCKABLE: return WINED3DFMT_D16_LOCKABLE;
        case D3DFMT_D32: return WINED3DFMT_D32_UNORM;
        case D3DFMT_D15S1: return WINED3DFMT_S1_UINT_D15_UNORM;
        case D3DFMT_D24S8: return WINED3DFMT_D24_UNORM_S8_UINT;
        case D3DFMT_D24X8: return WINED3DFMT_X8D24_UNORM;
        case D3DFMT_D24X4S4: return WINED3DFMT_S4X4_UINT_D24_UNORM;
        case D3DFMT_D16: return WINED3DFMT_D16_UNORM;
        case D3DFMT_VERTEXDATA: return WINED3DFMT_VERTEXDATA;
        case D3DFMT_INDEX16: return WINED3DFMT_R16_UINT;
        case D3DFMT_INDEX32: return WINED3DFMT_R32_UINT;
        default:
            FIXME("Unhandled D3DFORMAT %#x\n", format);
            return WINED3DFMT_UNKNOWN;
    }
}

static UINT vertex_count_from_primitive_count(D3DPRIMITIVETYPE primitive_type, UINT primitive_count)
{
    switch(primitive_type)
    {
        case D3DPT_POINTLIST:
            return primitive_count;

        case D3DPT_LINELIST:
            return primitive_count * 2;

        case D3DPT_LINESTRIP:
            return primitive_count + 1;

        case D3DPT_TRIANGLELIST:
            return primitive_count * 3;

        case D3DPT_TRIANGLESTRIP:
        case D3DPT_TRIANGLEFAN:
            return primitive_count + 2;

        default:
            FIXME("Unhandled primitive type %#x\n", primitive_type);
            return 0;
    }
}

/* Handle table functions */
static DWORD d3d8_allocate_handle(struct d3d8_handle_table *t, void *object, enum d3d8_handle_type type)
{
    struct d3d8_handle_entry *entry;

    if (t->free_entries)
    {
        DWORD index = t->free_entries - t->entries;
        /* Use a free handle */
        entry = t->free_entries;
        if (entry->type != D3D8_HANDLE_FREE)
        {
            ERR("Handle %u(%p) is in the free list, but has type %#x.\n", index, entry, entry->type);
            return D3D8_INVALID_HANDLE;
        }
        t->free_entries = entry->object;
        entry->object = object;
        entry->type = type;

        return index;
    }

    if (!(t->entry_count < t->table_size))
    {
        /* Grow the table */
        UINT new_size = t->table_size + (t->table_size >> 1);
        struct d3d8_handle_entry *new_entries = HeapReAlloc(GetProcessHeap(),
                0, t->entries, new_size * sizeof(*t->entries));
        if (!new_entries)
        {
            ERR("Failed to grow the handle table.\n");
            return D3D8_INVALID_HANDLE;
        }
        t->entries = new_entries;
        t->table_size = new_size;
    }

    entry = &t->entries[t->entry_count];
    entry->object = object;
    entry->type = type;

    return t->entry_count++;
}

static void *d3d8_free_handle(struct d3d8_handle_table *t, DWORD handle, enum d3d8_handle_type type)
{
    struct d3d8_handle_entry *entry;
    void *object;

    if (handle == D3D8_INVALID_HANDLE || handle >= t->entry_count)
    {
        WARN("Invalid handle %u passed.\n", handle);
        return NULL;
    }

    entry = &t->entries[handle];
    if (entry->type != type)
    {
        WARN("Handle %u(%p) is not of type %#x.\n", handle, entry, type);
        return NULL;
    }

    object = entry->object;
    entry->object = t->free_entries;
    entry->type = D3D8_HANDLE_FREE;
    t->free_entries = entry;

    return object;
}

static void *d3d8_get_object(struct d3d8_handle_table *t, DWORD handle, enum d3d8_handle_type type)
{
    struct d3d8_handle_entry *entry;

    if (handle == D3D8_INVALID_HANDLE || handle >= t->entry_count)
    {
        WARN("Invalid handle %u passed.\n", handle);
        return NULL;
    }

    entry = &t->entries[handle];
    if (entry->type != type)
    {
        WARN("Handle %u(%p) is not of type %#x.\n", handle, entry, type);
        return NULL;
    }

    return entry->object;
}

static inline IDirect3DDevice8Impl *impl_from_IDirect3DDevice8(IDirect3DDevice8 *iface)
{
    return CONTAINING_RECORD(iface, IDirect3DDevice8Impl, IDirect3DDevice8_iface);
}

static HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(IDirect3DDevice8 *iface, REFIID riid,
        void **ppobj)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, riid %s, object %p.\n",
            iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DDevice8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DDevice8Impl_AddRef(IDirect3DDevice8 *iface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DDevice8Impl_Release(IDirect3DDevice8 *iface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    ULONG ref;

    if (This->inDestruction) return 0;
    ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        unsigned i;
        IDirect3D8 *parent = This->d3d_parent;

        TRACE("Releasing wined3d device %p.\n", This->wined3d_device);

        wined3d_mutex_lock();

        This->inDestruction = TRUE;

        for(i = 0; i < This->numConvertedDecls; i++) {
            IDirect3DVertexDeclaration8_Release(This->decls[i].decl);
        }
        HeapFree(GetProcessHeap(), 0, This->decls);

        wined3d_device_uninit_3d(This->wined3d_device);
        wined3d_device_release_focus_window(This->wined3d_device);
        wined3d_device_decref(This->wined3d_device);
        HeapFree(GetProcessHeap(), 0, This->handle_table.entries);
        HeapFree(GetProcessHeap(), 0, This);

        wined3d_mutex_unlock();

        IDirect3D8_Release(parent);
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
static HRESULT WINAPI IDirect3DDevice8Impl_TestCooperativeLevel(IDirect3DDevice8 *iface)
{
    IDirect3DDevice8Impl *device = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p.\n", iface);

    if (device->lost)
    {
        TRACE("Device is lost.\n");
        return D3DERR_DEVICENOTRESET;
    }

    return D3D_OK;
}

static UINT WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(IDirect3DDevice8 *iface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_get_available_texture_mem(This->wined3d_device);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ResourceManagerDiscardBytes(IDirect3DDevice8 *iface,
        DWORD Bytes)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, byte_count %u.\n", iface, Bytes);
    if (Bytes) FIXME("Byte count ignored.\n");

    wined3d_mutex_lock();
    wined3d_device_evict_managed_resources(This->wined3d_device);
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDirect3D(IDirect3DDevice8 *iface, IDirect3D8 **ppD3D8)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, d3d8 %p.\n", iface, ppD3D8);

    if (NULL == ppD3D8) {
        return D3DERR_INVALIDCALL;
    }

    return IDirect3D8_QueryInterface(This->d3d_parent, &IID_IDirect3D8, (void **)ppD3D8);
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDeviceCaps(IDirect3DDevice8 *iface, D3DCAPS8 *pCaps)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("iface %p, caps %p.\n", iface, pCaps);

    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /* well this is what MSDN says to return */
    }

    wined3d_mutex_lock();
    hrc = wined3d_device_get_device_caps(This->wined3d_device, pWineCaps);
    wined3d_mutex_unlock();

    fixup_caps(pWineCaps);
    WINECAPSTOD3D8CAPS(pCaps, pWineCaps)
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    TRACE("Returning %p %p\n", This, pCaps);
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDisplayMode(IDirect3DDevice8 *iface,
        D3DDISPLAYMODE *pMode)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, mode %p.\n", iface, pMode);

    wined3d_mutex_lock();
    hr = wined3d_device_get_display_mode(This->wined3d_device, 0, (struct wined3d_display_mode *)pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetCreationParameters(IDirect3DDevice8 *iface,
        D3DDEVICE_CREATION_PARAMETERS *pParameters)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, parameters %p.\n", iface, pParameters);

    wined3d_mutex_lock();
    hr = wined3d_device_get_creation_parameters(This->wined3d_device,
            (struct wined3d_device_creation_parameters *)pParameters);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetCursorProperties(IDirect3DDevice8 *iface,
        UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8 *pCursorBitmap)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DSurface8Impl *pSurface = unsafe_impl_from_IDirect3DSurface8(pCursorBitmap);
    HRESULT hr;

    TRACE("iface %p, hotspot_x %u, hotspot_y %u, bitmap %p.\n",
            iface, XHotSpot, YHotSpot, pCursorBitmap);

    if (!pCursorBitmap)
    {
        WARN("No cursor bitmap, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_set_cursor_properties(This->wined3d_device, XHotSpot, YHotSpot, pSurface->wined3d_surface);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DDevice8Impl_SetCursorPosition(IDirect3DDevice8 *iface,
        UINT XScreenSpace, UINT YScreenSpace, DWORD Flags)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, x %u, y %u, flags %#x.\n",
            iface, XScreenSpace, YScreenSpace, Flags);

    wined3d_mutex_lock();
    wined3d_device_set_cursor_position(This->wined3d_device, XScreenSpace, YScreenSpace, Flags);
    wined3d_mutex_unlock();
}

static BOOL WINAPI IDirect3DDevice8Impl_ShowCursor(IDirect3DDevice8 *iface, BOOL bShow)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    BOOL ret;

    TRACE("iface %p, show %#x.\n", iface, bShow);

    wined3d_mutex_lock();
    ret = wined3d_device_show_cursor(This->wined3d_device, bShow);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateAdditionalSwapChain(IDirect3DDevice8 *iface,
        D3DPRESENT_PARAMETERS *present_parameters, IDirect3DSwapChain8 **swapchain)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DSwapChain8Impl *object;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p, swapchain %p.\n",
            iface, present_parameters, swapchain);

    object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate swapchain memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = swapchain_init(object, This, present_parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize swapchain, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created swapchain %p.\n", object);
    *swapchain = &object->IDirect3DSwapChain8_iface;

    return D3D_OK;
}

static HRESULT CDECL reset_enum_callback(struct wined3d_resource *resource)
{
    struct wined3d_resource_desc desc;

    wined3d_resource_get_desc(resource, &desc);
    if (desc.pool == WINED3D_POOL_DEFAULT)
    {
        IDirect3DSurface8 *surface;

        if (desc.resource_type != WINED3D_RTYPE_SURFACE)
        {
            WARN("Resource %p in pool D3DPOOL_DEFAULT blocks the Reset call.\n", resource);
            return D3DERR_DEVICELOST;
        }

        surface = wined3d_resource_get_parent(resource);

        IDirect3DSurface8_AddRef(surface);
        if (IDirect3DSurface8_Release(surface))
        {
            WARN("Surface %p (resource %p) in pool D3DPOOL_DEFAULT blocks the Reset call.\n", surface, resource);
            return D3DERR_DEVICELOST;
        }

        WARN("Surface %p (resource %p) is an implicit resource with ref 0.\n", surface, resource);
    }

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Reset(IDirect3DDevice8 *iface,
        D3DPRESENT_PARAMETERS *pPresentationParameters)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_swapchain_desc swapchain_desc;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p.\n", iface, pPresentationParameters);

    wined3d_mutex_lock();

    swapchain_desc.backbuffer_width = pPresentationParameters->BackBufferWidth;
    swapchain_desc.backbuffer_height = pPresentationParameters->BackBufferHeight;
    swapchain_desc.backbuffer_format = wined3dformat_from_d3dformat(pPresentationParameters->BackBufferFormat);
    swapchain_desc.backbuffer_count = pPresentationParameters->BackBufferCount;
    swapchain_desc.multisample_type = pPresentationParameters->MultiSampleType;
    swapchain_desc.multisample_quality = 0; /* d3d9 only */
    swapchain_desc.swap_effect = pPresentationParameters->SwapEffect;
    swapchain_desc.device_window = pPresentationParameters->hDeviceWindow;
    swapchain_desc.windowed = pPresentationParameters->Windowed;
    swapchain_desc.enable_auto_depth_stencil = pPresentationParameters->EnableAutoDepthStencil;
    swapchain_desc.auto_depth_stencil_format = wined3dformat_from_d3dformat(pPresentationParameters->AutoDepthStencilFormat);
    swapchain_desc.flags = pPresentationParameters->Flags;
    swapchain_desc.refresh_rate = pPresentationParameters->FullScreen_RefreshRateInHz;
    swapchain_desc.swap_interval = pPresentationParameters->FullScreen_PresentationInterval;
    swapchain_desc.auto_restore_display_mode = TRUE;

    hr = wined3d_device_reset(This->wined3d_device, &swapchain_desc, reset_enum_callback);
    if (SUCCEEDED(hr))
    {
        hr = wined3d_device_set_render_state(This->wined3d_device, WINED3D_RS_POINTSIZE_MIN, 0);
        This->lost = FALSE;
    }
    else
    {
        This->lost = TRUE;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Present(IDirect3DDevice8 *iface, const RECT *pSourceRect,
        const RECT *pDestRect, HWND hDestWindowOverride, const RGNDATA *pDirtyRegion)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, src_rect %p, dst_rect %p, dst_window_override %p, dirty_region %p.\n",
            iface, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    wined3d_mutex_lock();
    hr = wined3d_device_present(This->wined3d_device, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetBackBuffer(IDirect3DDevice8 *iface,
        UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8 **ppBackBuffer)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_surface *wined3d_surface = NULL;
    HRESULT hr;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, BackBuffer, Type, ppBackBuffer);

    wined3d_mutex_lock();
    hr = wined3d_device_get_back_buffer(This->wined3d_device, 0,
            BackBuffer, (enum wined3d_backbuffer_type)Type, &wined3d_surface);
    if (SUCCEEDED(hr) && wined3d_surface && ppBackBuffer)
    {
        *ppBackBuffer = wined3d_surface_get_parent(wined3d_surface);
        IDirect3DSurface8_AddRef(*ppBackBuffer);
        wined3d_surface_decref(wined3d_surface);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetRasterStatus(IDirect3DDevice8 *iface,
        D3DRASTER_STATUS *pRasterStatus)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, raster_status %p.\n", iface, pRasterStatus);

    wined3d_mutex_lock();
    hr = wined3d_device_get_raster_status(This->wined3d_device, 0, (struct wined3d_raster_status *)pRasterStatus);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DDevice8Impl_SetGammaRamp(IDirect3DDevice8 *iface, DWORD Flags,
        const D3DGAMMARAMP *pRamp)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, flags %#x, ramp %p.\n", iface, Flags, pRamp);

    /* Note: D3DGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
    wined3d_mutex_lock();
    wined3d_device_set_gamma_ramp(This->wined3d_device, 0, Flags, (const struct wined3d_gamma_ramp *)pRamp);
    wined3d_mutex_unlock();
}

static void WINAPI IDirect3DDevice8Impl_GetGammaRamp(IDirect3DDevice8 *iface, D3DGAMMARAMP *pRamp)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);

    TRACE("iface %p, ramp %p.\n", iface, pRamp);

    /* Note: D3DGAMMARAMP is compatible with struct wined3d_gamma_ramp. */
    wined3d_mutex_lock();
    wined3d_device_get_gamma_ramp(This->wined3d_device, 0, (struct wined3d_gamma_ramp *)pRamp);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateTexture(IDirect3DDevice8 *iface,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DTexture8 **texture)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DTexture8Impl *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, levels %u, usage %#x, format %#x, pool %#x, texture %p.\n",
            iface, width, height, levels, usage, format, pool, texture);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate texture memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = texture_init(object, This, width, height, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize texture, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created texture %p.\n", object);
    *texture = &object->IDirect3DTexture8_iface;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVolumeTexture(IDirect3DDevice8 *iface,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DVolumeTexture8 **texture)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVolumeTexture8Impl *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, depth %u, levels %u, usage %#x, format %#x, pool %#x, texture %p.\n",
            iface, width, height, depth, levels, usage, format, pool, texture);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate volume texture memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = volumetexture_init(object, This, width, height, depth, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize volume texture, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created volume texture %p.\n", object);
    *texture = &object->IDirect3DVolumeTexture8_iface;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateCubeTexture(IDirect3DDevice8 *iface, UINT edge_length,
        UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture8 **texture)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DCubeTexture8Impl *object;
    HRESULT hr;

    TRACE("iface %p, edge_length %u, levels %u, usage %#x, format %#x, pool %#x, texture %p.\n",
            iface, edge_length, levels, usage, format, pool, texture);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate cube texture memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = cubetexture_init(object, This, edge_length, levels, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize cube texture, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created cube texture %p.\n", object);
    *texture = &object->IDirect3DCubeTexture8_iface;

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexBuffer(IDirect3DDevice8 *iface, UINT size,
        DWORD usage, DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer8 **buffer)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexBuffer8Impl *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#x, fvf %#x, pool %#x, buffer %p.\n",
            iface, size, usage, fvf, pool, buffer);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate buffer memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = vertexbuffer_init(object, This, size, usage, fvf, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex buffer, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex buffer %p.\n", object);
    *buffer = &object->IDirect3DVertexBuffer8_iface;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateIndexBuffer(IDirect3DDevice8 *iface, UINT size,
        DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer8 **buffer)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DIndexBuffer8Impl *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#x, format %#x, pool %#x, buffer %p.\n",
            iface, size, usage, format, pool, buffer);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate buffer memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = indexbuffer_init(object, This, size, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize index buffer, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created index buffer %p.\n", object);
    *buffer = &object->IDirect3DIndexBuffer8_iface;

    return D3D_OK;
}

static HRESULT IDirect3DDevice8Impl_CreateSurface(IDirect3DDevice8Impl *device, UINT Width,
        UINT Height, D3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level,
        IDirect3DSurface8 **ppSurface, UINT Usage, D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample,
        DWORD MultisampleQuality)
{
    IDirect3DSurface8Impl *object;
    HRESULT hr;

    TRACE("device %p, width %u, height %u, format %#x, lockable %#x, discard %#x, level %u, surface %p,\n"
            "\tusage %#x, pool %#x, multisample_type %#x, multisample_quality %u.\n",
            device, Width, Height, Format, Lockable, Discard, Level, ppSurface,
            Usage, Pool, MultiSample, MultisampleQuality);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    if (!object)
    {
        FIXME("Failed to allocate surface memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = surface_init(object, device, Width, Height, Format, Lockable, Discard, Level, Usage,
            Pool, MultiSample, MultisampleQuality);
    if (FAILED(hr))
    {
        WARN("Failed to initialize surface, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created surface %p.\n", object);
    *ppSurface = &object->IDirect3DSurface8_iface;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateRenderTarget(IDirect3DDevice8 *iface, UINT Width,
        UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable,
        IDirect3DSurface8 **ppSurface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, lockable %#x, surface %p.\n",
            iface, Width, Height, Format, MultiSample, Lockable, ppSurface);

    hr = IDirect3DDevice8Impl_CreateSurface(This, Width, Height, Format, Lockable,
            FALSE /* Discard */, 0 /* Level */, ppSurface, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT,
            MultiSample, 0);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateDepthStencilSurface(IDirect3DDevice8 *iface,
        UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
        IDirect3DSurface8 **ppSurface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, surface %p.\n",
            iface, Width, Height, Format, MultiSample, ppSurface);

    /* TODO: Verify that Discard is false */
    hr = IDirect3DDevice8Impl_CreateSurface(This, Width, Height, Format, TRUE /* Lockable */, FALSE,
            0 /* Level */, ppSurface, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT, MultiSample, 0);

    return hr;
}

/*  IDirect3DDevice8Impl::CreateImageSurface returns surface with pool type SYSTEMMEM */
static HRESULT WINAPI IDirect3DDevice8Impl_CreateImageSurface(IDirect3DDevice8 *iface, UINT Width,
        UINT Height, D3DFORMAT Format, IDirect3DSurface8 **ppSurface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, surface %p.\n",
            iface, Width, Height, Format, ppSurface);

    hr = IDirect3DDevice8Impl_CreateSurface(This, Width, Height, Format, TRUE /* Lockable */,
            FALSE /* Discard */, 0 /* Level */, ppSurface, 0 /* Usage (undefined/none) */,
            D3DPOOL_SYSTEMMEM, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CopyRects(IDirect3DDevice8 *iface,
        IDirect3DSurface8 *pSourceSurface, const RECT *pSourceRects, UINT cRects,
        IDirect3DSurface8 *pDestinationSurface, const POINT *pDestPoints)
{
    IDirect3DSurface8Impl *Source = unsafe_impl_from_IDirect3DSurface8(pSourceSurface);
    IDirect3DSurface8Impl *Dest = unsafe_impl_from_IDirect3DSurface8(pDestinationSurface);
    enum wined3d_format_id srcFormat, destFormat;
    struct wined3d_resource_desc wined3d_desc;
    struct wined3d_resource *wined3d_resource;
    UINT src_w, src_h;
    HRESULT hr;

    TRACE("iface %p, src_surface %p, src_rects %p, rect_count %u, dst_surface %p, dst_points %p.\n",
            iface, pSourceSurface, pSourceRects, cRects, pDestinationSurface, pDestPoints);

    /* Check that the source texture is in WINED3D_POOL_SYSTEM_MEM and the
     * destination texture is in WINED3D_POOL_DEFAULT. */

    wined3d_mutex_lock();
    wined3d_resource = wined3d_surface_get_resource(Source->wined3d_surface);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    srcFormat = wined3d_desc.format;
    src_w = wined3d_desc.width;
    src_h = wined3d_desc.height;

    wined3d_resource = wined3d_surface_get_resource(Dest->wined3d_surface);
    wined3d_resource_get_desc(wined3d_resource, &wined3d_desc);
    destFormat = wined3d_desc.format;

    /* Check that the source and destination formats match */
    if (srcFormat != destFormat && WINED3DFMT_UNKNOWN != destFormat)
    {
        WARN("Source %p format must match the dest %p format, returning D3DERR_INVALIDCALL.\n",
                pSourceSurface, pDestinationSurface);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }
    else if (WINED3DFMT_UNKNOWN == destFormat)
    {
        TRACE("(%p) : Converting destination surface from WINED3DFMT_UNKNOWN to the source format\n", iface);
        if (FAILED(hr = wined3d_surface_update_desc(Dest->wined3d_surface, wined3d_desc.width, wined3d_desc.height,
                srcFormat, wined3d_desc.multisample_type, wined3d_desc.multisample_quality)))
        {
            WARN("Failed to update surface desc, hr %#x.\n", hr);
            wined3d_mutex_unlock();
            return hr;
        }
    }

    /* Quick if complete copy ... */
    if (!cRects && !pSourceRects && !pDestPoints)
    {
        RECT rect = {0, 0, src_w, src_h};
        wined3d_surface_blt(Dest->wined3d_surface, &rect,
                Source->wined3d_surface, &rect, 0, NULL, WINED3D_TEXF_POINT);
    }
    else
    {
        unsigned int i;
        /* Copy rect by rect */
        if (pSourceRects && pDestPoints)
        {
            for (i = 0; i < cRects; ++i)
            {
                UINT w = pSourceRects[i].right - pSourceRects[i].left;
                UINT h = pSourceRects[i].bottom - pSourceRects[i].top;
                RECT dst_rect = {pDestPoints[i].x, pDestPoints[i].y,
                        pDestPoints[i].x + w, pDestPoints[i].y + h};

                wined3d_surface_blt(Dest->wined3d_surface, &dst_rect,
                        Source->wined3d_surface, &pSourceRects[i], 0, NULL, WINED3D_TEXF_POINT);
            }
        }
        else
        {
            for (i = 0; i < cRects; ++i)
            {
                UINT w = pSourceRects[i].right - pSourceRects[i].left;
                UINT h = pSourceRects[i].bottom - pSourceRects[i].top;
                RECT dst_rect = {0, 0, w, h};

                wined3d_surface_blt(Dest->wined3d_surface, &dst_rect,
                        Source->wined3d_surface, &pSourceRects[i], 0, NULL, WINED3D_TEXF_POINT);
            }
        }
    }
    wined3d_mutex_unlock();

    return WINED3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_UpdateTexture(IDirect3DDevice8 *iface,
        IDirect3DBaseTexture8 *src_texture, IDirect3DBaseTexture8 *dst_texture)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, src_texture %p, dst_texture %p.\n", iface, src_texture, dst_texture);

    wined3d_mutex_lock();
    hr = wined3d_device_update_texture(This->wined3d_device,
            ((IDirect3DBaseTexture8Impl *)src_texture)->wined3d_texture,
            ((IDirect3DBaseTexture8Impl *)dst_texture)->wined3d_texture);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetFrontBuffer(IDirect3DDevice8 *iface,
        IDirect3DSurface8 *pDestSurface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DSurface8Impl *destSurface = unsafe_impl_from_IDirect3DSurface8(pDestSurface);
    HRESULT hr;

    TRACE("iface %p, dst_surface %p.\n", iface, pDestSurface);

    if (pDestSurface == NULL) {
        WARN("(%p) : Caller passed NULL as pDestSurface returning D3DERR_INVALIDCALL\n", This);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_get_front_buffer_data(This->wined3d_device, 0, destSurface->wined3d_surface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetRenderTarget(IDirect3DDevice8 *iface,
        IDirect3DSurface8 *pRenderTarget, IDirect3DSurface8 *pNewZStencil)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DSurface8Impl *pSurface = unsafe_impl_from_IDirect3DSurface8(pRenderTarget);
    IDirect3DSurface8Impl *pZSurface = unsafe_impl_from_IDirect3DSurface8(pNewZStencil);
    struct wined3d_surface *original_ds = NULL;
    HRESULT hr;

    TRACE("iface %p, render_target %p, depth_stencil %p.\n", iface, pRenderTarget, pNewZStencil);

    wined3d_mutex_lock();

    if (pZSurface)
    {
        struct wined3d_resource_desc ds_desc, rt_desc;
        struct wined3d_resource *wined3d_resource;
        struct wined3d_surface *original_rt = NULL;

        /* If no render target is passed in check the size against the current RT */
        if (!pRenderTarget)
        {
            hr = wined3d_device_get_render_target(This->wined3d_device, 0, &original_rt);
            if (FAILED(hr) || !original_rt)
            {
                wined3d_mutex_unlock();
                return hr;
            }
            wined3d_resource = wined3d_surface_get_resource(original_rt);
            wined3d_surface_decref(original_rt);
        }
        else
            wined3d_resource = wined3d_surface_get_resource(pSurface->wined3d_surface);
        wined3d_resource_get_desc(wined3d_resource, &rt_desc);

        wined3d_resource = wined3d_surface_get_resource(pZSurface->wined3d_surface);
        wined3d_resource_get_desc(wined3d_resource, &ds_desc);

        if (ds_desc.width < rt_desc.width || ds_desc.height < rt_desc.height)
        {
            WARN("Depth stencil is smaller than the render target, returning D3DERR_INVALIDCALL\n");
            wined3d_mutex_unlock();
            return D3DERR_INVALIDCALL;
        }
    }

    hr = wined3d_device_get_depth_stencil(This->wined3d_device, &original_ds);
    if (hr == WINED3D_OK || hr == WINED3DERR_NOTFOUND)
    {
        hr = wined3d_device_set_depth_stencil(This->wined3d_device, pZSurface ? pZSurface->wined3d_surface : NULL);
        if (SUCCEEDED(hr) && pRenderTarget)
        {
            hr = wined3d_device_set_render_target(This->wined3d_device, 0, pSurface->wined3d_surface, TRUE);
            if (FAILED(hr))
                wined3d_device_set_depth_stencil(This->wined3d_device, original_ds);
        }
    }
    if (original_ds)
        wined3d_surface_decref(original_ds);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetRenderTarget(IDirect3DDevice8 *iface,
        IDirect3DSurface8 **ppRenderTarget)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_surface *wined3d_surface;
    HRESULT hr;

    TRACE("iface %p, render_target %p.\n", iface, ppRenderTarget);

    if (ppRenderTarget == NULL) {
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_get_render_target(This->wined3d_device, 0, &wined3d_surface);
    if (SUCCEEDED(hr) && wined3d_surface)
    {
        *ppRenderTarget = wined3d_surface_get_parent(wined3d_surface);
        IDirect3DSurface8_AddRef(*ppRenderTarget);
        wined3d_surface_decref(wined3d_surface);
    }
    else
    {
        FIXME("Call to IWineD3DDevice_GetRenderTarget failed\n");
        *ppRenderTarget = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDepthStencilSurface(IDirect3DDevice8 *iface,
        IDirect3DSurface8 **ppZStencilSurface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_surface *wined3d_surface;
    HRESULT hr;

    TRACE("iface %p, depth_stencil %p.\n", iface, ppZStencilSurface);

    if(ppZStencilSurface == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_get_depth_stencil(This->wined3d_device, &wined3d_surface);
    if (SUCCEEDED(hr))
    {
        *ppZStencilSurface = wined3d_surface_get_parent(wined3d_surface);
        IDirect3DSurface8_AddRef(*ppZStencilSurface);
        wined3d_surface_decref(wined3d_surface);
    }
    else
    {
        if (hr != WINED3DERR_NOTFOUND)
                FIXME("Call to IWineD3DDevice_GetDepthStencilSurface failed with 0x%08x\n", hr);
        *ppZStencilSurface = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_BeginScene(IDirect3DDevice8 *iface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_begin_scene(This->wined3d_device);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDevice8Impl_EndScene(IDirect3DDevice8 *iface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_end_scene(This->wined3d_device);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Clear(IDirect3DDevice8 *iface, DWORD rect_count,
        const D3DRECT *rects, DWORD flags, D3DCOLOR color, float z, DWORD stencil)
{
    const struct wined3d_color c =
    {
        ((color >> 16) & 0xff) / 255.0f,
        ((color >>  8) & 0xff) / 255.0f,
        (color & 0xff) / 255.0f,
        ((color >> 24) & 0xff) / 255.0f,
    };
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, rect_count %u, rects %p, flags %#x, color 0x%08x, z %.8e, stencil %u.\n",
            iface, rect_count, rects, flags, color, z, stencil);

    wined3d_mutex_lock();
    hr = wined3d_device_clear(This->wined3d_device, rect_count, (const RECT *)rects, flags, &c, z, stencil);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTransform(IDirect3DDevice8 *iface,
        D3DTRANSFORMSTATETYPE State, const D3DMATRIX *lpMatrix)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, lpMatrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    hr = wined3d_device_set_transform(This->wined3d_device, State, (const struct wined3d_matrix *)lpMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetTransform(IDirect3DDevice8 *iface,
        D3DTRANSFORMSTATETYPE State, D3DMATRIX *pMatrix)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, pMatrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    hr = wined3d_device_get_transform(This->wined3d_device, State, (struct wined3d_matrix *)pMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_MultiplyTransform(IDirect3DDevice8 *iface,
        D3DTRANSFORMSTATETYPE State, const D3DMATRIX *pMatrix)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, pMatrix);

    /* Note: D3DMATRIX is compatible with struct wined3d_matrix. */
    wined3d_mutex_lock();
    hr = wined3d_device_multiply_transform(This->wined3d_device, State, (const struct wined3d_matrix *)pMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetViewport(IDirect3DDevice8 *iface,
        const D3DVIEWPORT8 *pViewport)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, pViewport);

    /* Note: D3DVIEWPORT8 is compatible with struct wined3d_viewport. */
    wined3d_mutex_lock();
    hr = wined3d_device_set_viewport(This->wined3d_device, (const struct wined3d_viewport *)pViewport);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetViewport(IDirect3DDevice8 *iface,
        D3DVIEWPORT8 *pViewport)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, pViewport);

    /* Note: D3DVIEWPORT8 is compatible with struct wined3d_viewport. */
    wined3d_mutex_lock();
    hr = wined3d_device_get_viewport(This->wined3d_device, (struct wined3d_viewport *)pViewport);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetMaterial(IDirect3DDevice8 *iface,
        const D3DMATERIAL8 *pMaterial)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, pMaterial);

    /* Note: D3DMATERIAL8 is compatible with struct wined3d_material. */
    wined3d_mutex_lock();
    hr = wined3d_device_set_material(This->wined3d_device, (const struct wined3d_material *)pMaterial);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetMaterial(IDirect3DDevice8 *iface,
        D3DMATERIAL8 *pMaterial)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, pMaterial);

    /* Note: D3DMATERIAL8 is compatible with struct wined3d_material. */
    wined3d_mutex_lock();
    hr = wined3d_device_get_material(This->wined3d_device, (struct wined3d_material *)pMaterial);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetLight(IDirect3DDevice8 *iface, DWORD Index,
        const D3DLIGHT8 *pLight)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %u, light %p.\n", iface, Index, pLight);

    /* Note: D3DLIGHT8 is compatible with struct wined3d_light. */
    wined3d_mutex_lock();
    hr = wined3d_device_set_light(This->wined3d_device, Index, (const struct wined3d_light *)pLight);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetLight(IDirect3DDevice8 *iface, DWORD Index,
        D3DLIGHT8 *pLight)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %u, light %p.\n", iface, Index, pLight);

    /* Note: D3DLIGHT8 is compatible with struct wined3d_light. */
    wined3d_mutex_lock();
    hr = wined3d_device_get_light(This->wined3d_device, Index, (struct wined3d_light *)pLight);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_LightEnable(IDirect3DDevice8 *iface, DWORD Index,
        BOOL Enable)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %u, enable %#x.\n", iface, Index, Enable);

    wined3d_mutex_lock();
    hr = wined3d_device_set_light_enable(This->wined3d_device, Index, Enable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetLightEnable(IDirect3DDevice8 *iface, DWORD Index,
        BOOL *pEnable)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %u, enable %p.\n", iface, Index, pEnable);

    wined3d_mutex_lock();
    hr = wined3d_device_get_light_enable(This->wined3d_device, Index, pEnable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetClipPlane(IDirect3DDevice8 *iface, DWORD Index,
        const float *pPlane)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %u, plane %p.\n", iface, Index, pPlane);

    wined3d_mutex_lock();
    hr = wined3d_device_set_clip_plane(This->wined3d_device, Index, pPlane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetClipPlane(IDirect3DDevice8 *iface, DWORD Index,
        float *pPlane)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, index %u, plane %p.\n", iface, Index, pPlane);

    wined3d_mutex_lock();
    hr = wined3d_device_get_clip_plane(This->wined3d_device, Index, pPlane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetRenderState(IDirect3DDevice8 *iface,
        D3DRENDERSTATETYPE State, DWORD Value)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %#x.\n", iface, State, Value);

    wined3d_mutex_lock();
    switch (State)
    {
        case D3DRS_ZBIAS:
            hr = wined3d_device_set_render_state(This->wined3d_device, WINED3D_RS_DEPTHBIAS, Value);
            break;

        default:
            hr = wined3d_device_set_render_state(This->wined3d_device, State, Value);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetRenderState(IDirect3DDevice8 *iface,
        D3DRENDERSTATETYPE State, DWORD *pValue)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, state %#x, value %p.\n", iface, State, pValue);

    wined3d_mutex_lock();
    switch (State)
    {
        case D3DRS_ZBIAS:
            hr = wined3d_device_get_render_state(This->wined3d_device, WINED3D_RS_DEPTHBIAS, pValue);
            break;

        default:
            hr = wined3d_device_get_render_state(This->wined3d_device, State, pValue);
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_BeginStateBlock(IDirect3DDevice8 *iface)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = wined3d_device_begin_stateblock(This->wined3d_device);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_EndStateBlock(IDirect3DDevice8 *iface, DWORD *pToken)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("iface %p, token %p.\n", iface, pToken);

    /* Tell wineD3D to endstateblock before anything else (in case we run out
     * of memory later and cause locking problems)
     */
    wined3d_mutex_lock();
    hr = wined3d_device_end_stateblock(This->wined3d_device, &stateblock);
    if (FAILED(hr))
    {
        WARN("IWineD3DDevice_EndStateBlock returned an error\n");
        wined3d_mutex_unlock();
        return hr;
    }

    *pToken = d3d8_allocate_handle(&This->handle_table, stateblock, D3D8_HANDLE_SB);
    wined3d_mutex_unlock();

    if (*pToken == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to create a handle\n");
        wined3d_mutex_lock();
        wined3d_stateblock_decref(stateblock);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    ++*pToken;

    TRACE("Returning %#x (%p).\n", *pToken, stateblock);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ApplyStateBlock(IDirect3DDevice8 *iface, DWORD Token)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("iface %p, token %#x.\n", iface, Token);

    if (!Token) return D3D_OK;

    wined3d_mutex_lock();
    stateblock = d3d8_get_object(&This->handle_table, Token - 1, D3D8_HANDLE_SB);
    if (!stateblock)
    {
        WARN("Invalid handle (%#x) passed.\n", Token);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }
    hr = wined3d_stateblock_apply(stateblock);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CaptureStateBlock(IDirect3DDevice8 *iface, DWORD Token)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("iface %p, token %#x.\n", iface, Token);

    wined3d_mutex_lock();
    stateblock = d3d8_get_object(&This->handle_table, Token - 1, D3D8_HANDLE_SB);
    if (!stateblock)
    {
        WARN("Invalid handle (%#x) passed.\n", Token);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }
    hr = wined3d_stateblock_capture(stateblock);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeleteStateBlock(IDirect3DDevice8 *iface, DWORD Token)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;

    TRACE("iface %p, token %#x.\n", iface, Token);

    wined3d_mutex_lock();
    stateblock = d3d8_free_handle(&This->handle_table, Token - 1, D3D8_HANDLE_SB);

    if (!stateblock)
    {
        WARN("Invalid handle (%#x) passed.\n", Token);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    if (wined3d_stateblock_decref(stateblock))
    {
        ERR("Stateblock %p has references left, this shouldn't happen.\n", stateblock);
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateStateBlock(IDirect3DDevice8 *iface,
        D3DSTATEBLOCKTYPE Type, DWORD *handle)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_stateblock *stateblock;
    HRESULT hr;

    TRACE("iface %p, type %#x, handle %p.\n", iface, Type, handle);

    if (Type != D3DSBT_ALL
            && Type != D3DSBT_PIXELSTATE
            && Type != D3DSBT_VERTEXSTATE)
    {
        WARN("Unexpected stateblock type, returning D3DERR_INVALIDCALL\n");
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_stateblock_create(This->wined3d_device, (enum wined3d_stateblock_type)Type, &stateblock);
    if (FAILED(hr))
    {
        wined3d_mutex_unlock();
        ERR("IWineD3DDevice_CreateStateBlock failed, hr %#x\n", hr);
        return hr;
    }

    *handle = d3d8_allocate_handle(&This->handle_table, stateblock, D3D8_HANDLE_SB);
    wined3d_mutex_unlock();

    if (*handle == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to allocate a handle.\n");
        wined3d_mutex_lock();
        wined3d_stateblock_decref(stateblock);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    ++*handle;

    TRACE("Returning %#x (%p).\n", *handle, stateblock);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetClipStatus(IDirect3DDevice8 *iface,
        const D3DCLIPSTATUS8 *pClipStatus)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, pClipStatus);
    /* FIXME: Verify that D3DCLIPSTATUS8 ~= struct wined3d_clip_status. */

    wined3d_mutex_lock();
    hr = wined3d_device_set_clip_status(This->wined3d_device, (const struct wined3d_clip_status *)pClipStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetClipStatus(IDirect3DDevice8 *iface,
        D3DCLIPSTATUS8 *pClipStatus)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, pClipStatus);

    wined3d_mutex_lock();
    hr = wined3d_device_get_clip_status(This->wined3d_device, (struct wined3d_clip_status *)pClipStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetTexture(IDirect3DDevice8 *iface,
        DWORD Stage, IDirect3DBaseTexture8 **ppTexture)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_texture *wined3d_texture;
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, ppTexture);

    if(ppTexture == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_get_texture(This->wined3d_device, Stage, &wined3d_texture);
    if (FAILED(hr))
    {
        WARN("Failed to get texture for stage %u, hr %#x.\n", Stage, hr);
        wined3d_mutex_unlock();
        *ppTexture = NULL;
        return hr;
    }

    if (wined3d_texture)
    {
        *ppTexture = wined3d_texture_get_parent(wined3d_texture);
        IDirect3DBaseTexture8_AddRef(*ppTexture);
        wined3d_texture_decref(wined3d_texture);
    }
    else
    {
        *ppTexture = NULL;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTexture(IDirect3DDevice8 *iface, DWORD Stage,
        IDirect3DBaseTexture8 *pTexture)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, pTexture);

    wined3d_mutex_lock();
    hr = wined3d_device_set_texture(This->wined3d_device, Stage,
            pTexture ? ((IDirect3DBaseTexture8Impl *)pTexture)->wined3d_texture : NULL);
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
    {FALSE, WINED3D_TSS_INVALID},                   /* 12, unused */
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
    {TRUE,  WINED3D_SAMP_ADDRESS_W},                /* 25, D3DTSS_ADDRESSW */
    {FALSE, WINED3D_TSS_COLOR_ARG0},                /* 26, D3DTSS_COLORARG0 */
    {FALSE, WINED3D_TSS_ALPHA_ARG0},                /* 27, D3DTSS_ALPHAARG0 */
    {FALSE, WINED3D_TSS_RESULT_ARG},                /* 28, D3DTSS_RESULTARG */
};

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(IDirect3DDevice8 *iface,
        DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD *pValue)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    const struct tss_lookup *l;
    HRESULT hr;

    TRACE("iface %p, stage %u, state %#x, value %p.\n", iface, Stage, Type, pValue);

    if (Type >= sizeof(tss_lookup) / sizeof(*tss_lookup))
    {
        WARN("Invalid Type %#x passed.\n", Type);
        return D3D_OK;
    }

    l = &tss_lookup[Type];

    wined3d_mutex_lock();
    if (l->sampler_state)
        hr = wined3d_device_get_sampler_state(This->wined3d_device, Stage, l->state, pValue);
    else
        hr = wined3d_device_get_texture_stage_state(This->wined3d_device, Stage, l->state, pValue);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTextureStageState(IDirect3DDevice8 *iface,
        DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    const struct tss_lookup *l;
    HRESULT hr;

    TRACE("iface %p, stage %u, state %#x, value %#x.\n", iface, Stage, Type, Value);

    if (Type >= sizeof(tss_lookup) / sizeof(*tss_lookup))
    {
        WARN("Invalid Type %#x passed.\n", Type);
        return D3D_OK;
    }

    l = &tss_lookup[Type];

    wined3d_mutex_lock();
    if (l->sampler_state)
        hr = wined3d_device_set_sampler_state(This->wined3d_device, Stage, l->state, Value);
    else
        hr = wined3d_device_set_texture_stage_state(This->wined3d_device, Stage, l->state, Value);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ValidateDevice(IDirect3DDevice8 *iface,
        DWORD *pNumPasses)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, pass_count %p.\n", iface, pNumPasses);

    wined3d_mutex_lock();
    hr = wined3d_device_validate_device(This->wined3d_device, pNumPasses);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetInfo(IDirect3DDevice8 *iface,
        DWORD info_id, void *info, DWORD info_size)
{
    FIXME("iface %p, info_id %#x, info %p, info_size %u stub!\n", iface, info_id, info, info_size);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetPaletteEntries(IDirect3DDevice8 *iface,
        UINT PaletteNumber, const PALETTEENTRY *pEntries)
{
    FIXME("iface %p, palette_idx %u, entries %p unimplemented\n", iface, PaletteNumber, pEntries);

    /* GPUs stopped supporting palettized textures with the Shader Model 1 generation. Wined3d
     * does not have a d3d8/9-style palette API */

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPaletteEntries(IDirect3DDevice8 *iface,
        UINT PaletteNumber, PALETTEENTRY *pEntries)
{
    FIXME("iface %p, palette_idx %u, entries %p unimplemented.\n", iface, PaletteNumber, pEntries);

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetCurrentTexturePalette(IDirect3DDevice8 *iface,
        UINT PaletteNumber)
{
    FIXME("iface %p, palette_idx %u unimplemented.\n", iface, PaletteNumber);

    return D3DERR_INVALIDCALL;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(IDirect3DDevice8 *iface,
        UINT *PaletteNumber)
{
    FIXME("iface %p, palette_idx %p unimplemented.\n", iface, PaletteNumber);

    return D3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitive(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, start_vertex %u, primitive_count %u.\n",
            iface, PrimitiveType, StartVertex, PrimitiveCount);

    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_primitive(This->wined3d_device, StartVertex,
            vertex_count_from_primitive_count(PrimitiveType, PrimitiveCount));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitive(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT startIndex,
        UINT primCount)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, min_vertex_idx %u, vertex_count %u, start_idx %u, primitive_count %u.\n",
            iface, PrimitiveType, MinVertexIndex, NumVertices, startIndex, primCount);

    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_indexed_primitive(This->wined3d_device, startIndex,
            vertex_count_from_primitive_count(PrimitiveType, primCount));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitiveUP(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, const void *pVertexStreamZeroData,
        UINT VertexStreamZeroStride)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, primitive_count %u, data %p, stride %u.\n",
            iface, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_primitive_up(This->wined3d_device,
            vertex_count_from_primitive_count(PrimitiveType, PrimitiveCount),
            pVertexStreamZeroData, VertexStreamZeroStride);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(IDirect3DDevice8 *iface,
        D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertexIndices,
        UINT PrimitiveCount, const void *pIndexData, D3DFORMAT IndexDataFormat,
        const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, min_vertex_idx %u, index_count %u, primitive_count %u,\n"
            "index_data %p, index_format %#x, vertex_data %p, vertex_stride %u.\n",
            iface, PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount,
            pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    wined3d_mutex_lock();
    wined3d_device_set_primitive_type(This->wined3d_device, PrimitiveType);
    hr = wined3d_device_draw_indexed_primitive_up(This->wined3d_device,
            vertex_count_from_primitive_count(PrimitiveType, PrimitiveCount), pIndexData,
            wined3dformat_from_d3dformat(IndexDataFormat), pVertexStreamZeroData, VertexStreamZeroStride);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ProcessVertices(IDirect3DDevice8 *iface,
        UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer8 *pDestBuffer,
        DWORD Flags)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexBuffer8Impl *dest = unsafe_impl_from_IDirect3DVertexBuffer8(pDestBuffer);
    HRESULT hr;

    TRACE("iface %p, src_start_idx %u, dst_idx %u, vertex_count %u, dst_buffer %p, flags %#x.\n",
            iface, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, Flags);

    wined3d_mutex_lock();
    hr = wined3d_device_process_vertices(This->wined3d_device, SrcStartIndex, DestIndex,
            VertexCount, dest->wineD3DVertexBuffer, NULL, Flags, dest->fvf);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexShader(IDirect3DDevice8 *iface,
        const DWORD *declaration, const DWORD *byte_code, DWORD *shader, DWORD usage)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexShader8Impl *object;
    DWORD shader_handle;
    DWORD handle;
    HRESULT hr;

    TRACE("iface %p, declaration %p, byte_code %p, shader %p, usage %#x.\n",
            iface, declaration, byte_code, shader, usage);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate vertex shader memory.\n");
        *shader = 0;
        return E_OUTOFMEMORY;
    }

    wined3d_mutex_lock();
    handle = d3d8_allocate_handle(&This->handle_table, object, D3D8_HANDLE_VS);
    wined3d_mutex_unlock();
    if (handle == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to allocate vertex shader handle.\n");
        HeapFree(GetProcessHeap(), 0, object);
        *shader = 0;
        return E_OUTOFMEMORY;
    }

    shader_handle = handle + VS_HIGHESTFIXEDFXF + 1;

    hr = vertexshader_init(object, This, declaration, byte_code, shader_handle, usage);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex shader, hr %#x.\n", hr);
        wined3d_mutex_lock();
        d3d8_free_handle(&This->handle_table, handle, D3D8_HANDLE_VS);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, object);
        *shader = 0;
        return hr;
    }

    TRACE("Created vertex shader %p (handle %#x).\n", object, shader_handle);
    *shader = shader_handle;

    return D3D_OK;
}

static IDirect3DVertexDeclaration8Impl *IDirect3DDevice8Impl_FindDecl(IDirect3DDevice8Impl *This, DWORD fvf)
{
    IDirect3DVertexDeclaration8Impl *d3d8_declaration;
    HRESULT hr;
    int p, low, high; /* deliberately signed */
    struct FvfToDecl *convertedDecls = This->decls;

    TRACE("Searching for declaration for fvf %08x... ", fvf);

    low = 0;
    high = This->numConvertedDecls - 1;
    while(low <= high) {
        p = (low + high) >> 1;
        TRACE("%d ", p);
        if(convertedDecls[p].fvf == fvf) {
            TRACE("found %p\n", convertedDecls[p].decl);
            return (IDirect3DVertexDeclaration8Impl *)convertedDecls[p].decl;
        } else if(convertedDecls[p].fvf < fvf) {
            low = p + 1;
        } else {
            high = p - 1;
        }
    }
    TRACE("not found. Creating and inserting at position %d.\n", low);

    d3d8_declaration = HeapAlloc(GetProcessHeap(), 0, sizeof(*d3d8_declaration));
    if (!d3d8_declaration)
    {
        ERR("Memory allocation failed.\n");
        return NULL;
    }

    hr = vertexdeclaration_init_fvf(d3d8_declaration, This, fvf);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex declaration, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, d3d8_declaration);
        return NULL;
    }

    if(This->declArraySize == This->numConvertedDecls) {
        int grow = This->declArraySize / 2;
        convertedDecls = HeapReAlloc(GetProcessHeap(), 0, convertedDecls,
                                     sizeof(convertedDecls[0]) * (This->numConvertedDecls + grow));
        if(!convertedDecls) {
            /* This will destroy it */
            IDirect3DVertexDeclaration8_Release((IDirect3DVertexDeclaration8 *)d3d8_declaration);
            return NULL;
        }
        This->decls = convertedDecls;
        This->declArraySize += grow;
    }

    memmove(convertedDecls + low + 1, convertedDecls + low, sizeof(convertedDecls[0]) * (This->numConvertedDecls - low));
    convertedDecls[low].decl = (IDirect3DVertexDeclaration8 *)d3d8_declaration;
    convertedDecls[low].fvf = fvf;
    This->numConvertedDecls++;

    TRACE("Returning %p. %u decls in array\n", d3d8_declaration, This->numConvertedDecls);
    return d3d8_declaration;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShader(IDirect3DDevice8 *iface, DWORD pShader)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexShader8Impl *shader;
    HRESULT hr;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    if (VS_HIGHESTFIXEDFXF >= pShader) {
        TRACE("Setting FVF, %#x\n", pShader);

        wined3d_mutex_lock();
        wined3d_device_set_vertex_declaration(This->wined3d_device,
                IDirect3DDevice8Impl_FindDecl(This, pShader)->wined3d_vertex_declaration);
        wined3d_device_set_vertex_shader(This->wined3d_device, NULL);
        wined3d_mutex_unlock();

        return D3D_OK;
    }

    TRACE("Setting shader\n");

    wined3d_mutex_lock();
    shader = d3d8_get_object(&This->handle_table, pShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pShader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    hr = wined3d_device_set_vertex_declaration(This->wined3d_device,
            ((IDirect3DVertexDeclaration8Impl *)shader->vertex_declaration)->wined3d_vertex_declaration);
    if (SUCCEEDED(hr))
        hr = wined3d_device_set_vertex_shader(This->wined3d_device, shader->wined3d_shader);
    wined3d_mutex_unlock();

    TRACE("Returning hr %#x\n", hr);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShader(IDirect3DDevice8 *iface, DWORD *ppShader)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_vertex_declaration *wined3d_declaration;
    IDirect3DVertexDeclaration8 *d3d8_declaration;
    HRESULT hr;

    TRACE("iface %p, shader %p.\n", iface, ppShader);

    wined3d_mutex_lock();
    hr = wined3d_device_get_vertex_declaration(This->wined3d_device, &wined3d_declaration);
    if (FAILED(hr))
    {
        wined3d_mutex_unlock();
        WARN("(%p) : Call to IWineD3DDevice_GetVertexDeclaration failed %#x (device %p)\n",
                This, hr, This->wined3d_device);
        return hr;
    }

    if (!wined3d_declaration)
    {
        wined3d_mutex_unlock();
        *ppShader = 0;
        return D3D_OK;
    }

    d3d8_declaration = wined3d_vertex_declaration_get_parent(wined3d_declaration);
    wined3d_vertex_declaration_decref(wined3d_declaration);
    wined3d_mutex_unlock();
    *ppShader = ((IDirect3DVertexDeclaration8Impl *)d3d8_declaration)->shader_handle;

    TRACE("(%p) : returning %#x\n", This, *ppShader);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeleteVertexShader(IDirect3DDevice8 *iface, DWORD pShader)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexShader8Impl *shader;
    struct wined3d_shader *cur;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    wined3d_mutex_lock();
    shader = d3d8_free_handle(&This->handle_table, pShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pShader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    cur = wined3d_device_get_vertex_shader(This->wined3d_device);
    if (cur)
    {
        if (cur == shader->wined3d_shader)
            IDirect3DDevice8_SetVertexShader(iface, 0);
        wined3d_shader_decref(cur);
    }

    wined3d_mutex_unlock();

    if (IDirect3DVertexShader8_Release(&shader->IDirect3DVertexShader8_iface))
    {
        ERR("Shader %p has references left, this shouldn't happen.\n", shader);
    }

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShaderConstant(IDirect3DDevice8 *iface,
        DWORD Register, const void *pConstantData, DWORD ConstantCount)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    if(Register + ConstantCount > D3D8_MAX_VERTEX_SHADER_CONSTANTF) {
        WARN("Trying to access %u constants, but d3d8 only supports %u\n",
             Register + ConstantCount, D3D8_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_set_vs_consts_f(This->wined3d_device, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderConstant(IDirect3DDevice8 *iface,
        DWORD Register, void *pConstantData, DWORD ConstantCount)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    if(Register + ConstantCount > D3D8_MAX_VERTEX_SHADER_CONSTANTF) {
        WARN("Trying to access %u constants, but d3d8 only supports %u\n",
             Register + ConstantCount, D3D8_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_get_vs_consts_f(This->wined3d_device, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderDeclaration(IDirect3DDevice8 *iface,
        DWORD pVertexShader, void *pData, DWORD *pSizeOfData)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexDeclaration8Impl *declaration;
    IDirect3DVertexShader8Impl *shader;

    TRACE("iface %p, shader %#x, data %p, data_size %p.\n",
            iface, pVertexShader, pData, pSizeOfData);

    wined3d_mutex_lock();
    shader = d3d8_get_object(&This->handle_table, pVertexShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS);
    wined3d_mutex_unlock();

    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pVertexShader);
        return D3DERR_INVALIDCALL;
    }
    declaration = (IDirect3DVertexDeclaration8Impl *)shader->vertex_declaration;

    /* If pData is NULL, we just return the required size of the buffer. */
    if (!pData) {
        *pSizeOfData = declaration->elements_size;
        return D3D_OK;
    }

    /* MSDN claims that if *pSizeOfData is smaller than the required size
     * we should write the required size and return D3DERR_MOREDATA.
     * That's not actually true. */
    if (*pSizeOfData < declaration->elements_size) {
        return D3DERR_INVALIDCALL;
    }

    CopyMemory(pData, declaration->elements, declaration->elements_size);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderFunction(IDirect3DDevice8 *iface,
        DWORD pVertexShader, void *pData, DWORD *pSizeOfData)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexShader8Impl *shader = NULL;
    HRESULT hr;

    TRACE("iface %p, shader %#x, data %p, data_size %p.\n",
            iface, pVertexShader, pData, pSizeOfData);

    wined3d_mutex_lock();
    shader = d3d8_get_object(&This->handle_table, pVertexShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pVertexShader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    if (!shader->wined3d_shader)
    {
        wined3d_mutex_unlock();
        *pSizeOfData = 0;
        return D3D_OK;
    }

    hr = wined3d_shader_get_byte_code(shader->wined3d_shader, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetIndices(IDirect3DDevice8 *iface,
        IDirect3DIndexBuffer8 *pIndexData, UINT baseVertexIndex)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DIndexBuffer8Impl *ib = unsafe_impl_from_IDirect3DIndexBuffer8(pIndexData);
    HRESULT hr;

    TRACE("iface %p, buffer %p, base_vertex_idx %u.\n", iface, pIndexData, baseVertexIndex);

    /* WineD3D takes an INT(due to d3d9), but d3d8 uses UINTs. Do I have to add a check here that
     * the UINT doesn't cause an overflow in the INT? It seems rather unlikely because such large
     * vertex buffers can't be created to address them with an index that requires the 32nd bit
     * (4 Byte minimum vertex size * 2^31-1 -> 8 gb buffer. The index sign would be the least
     * problem)
     */
    wined3d_mutex_lock();
    wined3d_device_set_base_vertex_index(This->wined3d_device, baseVertexIndex);
    hr = wined3d_device_set_index_buffer(This->wined3d_device,
            ib ? ib->wineD3DIndexBuffer : NULL,
            ib ? ib->format : WINED3DFMT_UNKNOWN);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetIndices(IDirect3DDevice8 *iface,
        IDirect3DIndexBuffer8 **ppIndexData, UINT *pBaseVertexIndex)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_buffer *retIndexData = NULL;
    HRESULT hr;

    TRACE("iface %p, buffer %p, base_vertex_index %p.\n", iface, ppIndexData, pBaseVertexIndex);

    if(ppIndexData == NULL){
        return D3DERR_INVALIDCALL;
    }

    /* The case from UINT to INT is safe because d3d8 will never set negative values */
    wined3d_mutex_lock();
    *pBaseVertexIndex = wined3d_device_get_base_vertex_index(This->wined3d_device);
    hr = wined3d_device_get_index_buffer(This->wined3d_device, &retIndexData);
    if (SUCCEEDED(hr) && retIndexData)
    {
        *ppIndexData = wined3d_buffer_get_parent(retIndexData);
        IDirect3DIndexBuffer8_AddRef(*ppIndexData);
        wined3d_buffer_decref(retIndexData);
    } else {
        if (FAILED(hr)) FIXME("Call to GetIndices failed\n");
        *ppIndexData = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreatePixelShader(IDirect3DDevice8 *iface,
        const DWORD *byte_code, DWORD *shader)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DPixelShader8Impl *object;
    DWORD shader_handle;
    DWORD handle;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, shader %p.\n", iface, byte_code, shader);

    if (!shader)
    {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate pixel shader memmory.\n");
        return E_OUTOFMEMORY;
    }

    wined3d_mutex_lock();
    handle = d3d8_allocate_handle(&This->handle_table, object, D3D8_HANDLE_PS);
    wined3d_mutex_unlock();
    if (handle == D3D8_INVALID_HANDLE)
    {
        ERR("Failed to allocate pixel shader handle.\n");
        HeapFree(GetProcessHeap(), 0, object);
        return E_OUTOFMEMORY;
    }

    shader_handle = handle + VS_HIGHESTFIXEDFXF + 1;

    hr = pixelshader_init(object, This, byte_code, shader_handle);
    if (FAILED(hr))
    {
        WARN("Failed to initialize pixel shader, hr %#x.\n", hr);
        wined3d_mutex_lock();
        d3d8_free_handle(&This->handle_table, handle, D3D8_HANDLE_PS);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, object);
        *shader = 0;
        return hr;
    }

    TRACE("Created pixel shader %p (handle %#x).\n", object, shader_handle);
    *shader = shader_handle;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetPixelShader(IDirect3DDevice8 *iface, DWORD pShader)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DPixelShader8Impl *shader;
    HRESULT hr;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    wined3d_mutex_lock();

    if (!pShader)
    {
        hr = wined3d_device_set_pixel_shader(This->wined3d_device, NULL);
        wined3d_mutex_unlock();
        return hr;
    }

    shader = d3d8_get_object(&This->handle_table, pShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_PS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pShader);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    TRACE("(%p) : Setting shader %p\n", This, shader);
    hr = wined3d_device_set_pixel_shader(This->wined3d_device, shader->wined3d_shader);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShader(IDirect3DDevice8 *iface, DWORD *ppShader)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_shader *object;

    TRACE("iface %p, shader %p.\n", iface, ppShader);

    if (NULL == ppShader) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    object = wined3d_device_get_pixel_shader(This->wined3d_device);
    if (object)
    {
        IDirect3DPixelShader8Impl *d3d8_shader;
        d3d8_shader = wined3d_shader_get_parent(object);
        wined3d_shader_decref(object);
        *ppShader = d3d8_shader->handle;
    }
    else
    {
        *ppShader = 0;
    }
    wined3d_mutex_unlock();

    TRACE("(%p) : returning %#x\n", This, *ppShader);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeletePixelShader(IDirect3DDevice8 *iface, DWORD pShader)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DPixelShader8Impl *shader;
    struct wined3d_shader *cur;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    wined3d_mutex_lock();

    shader = d3d8_free_handle(&This->handle_table, pShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_PS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pShader);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    cur = wined3d_device_get_pixel_shader(This->wined3d_device);
    if (cur)
    {
        if (cur == shader->wined3d_shader)
            IDirect3DDevice8_SetPixelShader(iface, 0);
        wined3d_shader_decref(cur);
    }

    wined3d_mutex_unlock();

    if (IDirect3DPixelShader8_Release(&shader->IDirect3DPixelShader8_iface))
    {
        ERR("Shader %p has references left, this shouldn't happen.\n", shader);
    }

    return D3D_OK;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(IDirect3DDevice8 *iface,
        DWORD Register, const void *pConstantData, DWORD ConstantCount)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    wined3d_mutex_lock();
    hr = wined3d_device_set_ps_consts_f(This->wined3d_device, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderConstant(IDirect3DDevice8 *iface,
        DWORD Register, void *pConstantData, DWORD ConstantCount)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    wined3d_mutex_lock();
    hr = wined3d_device_get_ps_consts_f(This->wined3d_device, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderFunction(IDirect3DDevice8 *iface,
        DWORD pPixelShader, void *pData, DWORD *pSizeOfData)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DPixelShader8Impl *shader = NULL;
    HRESULT hr;

    TRACE("iface %p, shader %#x, data %p, data_size %p.\n",
            iface, pPixelShader, pData, pSizeOfData);

    wined3d_mutex_lock();
    shader = d3d8_get_object(&This->handle_table, pPixelShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_PS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pPixelShader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    hr = wined3d_shader_get_byte_code(shader->wined3d_shader, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawRectPatch(IDirect3DDevice8 *iface, UINT Handle,
        const float *pNumSegs, const D3DRECTPATCH_INFO *pRectPatchInfo)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, handle %#x, segment_count %p, patch_info %p.\n",
            iface, Handle, pNumSegs, pRectPatchInfo);

    wined3d_mutex_lock();
    hr = wined3d_device_draw_rect_patch(This->wined3d_device, Handle,
            pNumSegs, (const struct wined3d_rect_patch_info *)pRectPatchInfo);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawTriPatch(IDirect3DDevice8 *iface, UINT Handle,
        const float *pNumSegs, const D3DTRIPATCH_INFO *pTriPatchInfo)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, handle %#x, segment_count %p, patch_info %p.\n",
            iface, Handle, pNumSegs, pTriPatchInfo);

    wined3d_mutex_lock();
    hr = wined3d_device_draw_tri_patch(This->wined3d_device, Handle,
            pNumSegs, (const struct wined3d_tri_patch_info *)pTriPatchInfo);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeletePatch(IDirect3DDevice8 *iface, UINT Handle)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    HRESULT hr;

    TRACE("iface %p, handle %#x.\n", iface, Handle);

    wined3d_mutex_lock();
    hr = wined3d_device_delete_patch(This->wined3d_device, Handle);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetStreamSource(IDirect3DDevice8 *iface,
        UINT StreamNumber, IDirect3DVertexBuffer8 *pStreamData, UINT Stride)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    IDirect3DVertexBuffer8Impl *streamdata = unsafe_impl_from_IDirect3DVertexBuffer8(pStreamData);
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, buffer %p, stride %u.\n",
            iface, StreamNumber, pStreamData, Stride);

    wined3d_mutex_lock();
    hr = wined3d_device_set_stream_source(This->wined3d_device, StreamNumber,
            streamdata ? streamdata->wineD3DVertexBuffer : NULL, 0/* Offset in bytes */, Stride);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetStreamSource(IDirect3DDevice8 *iface,
        UINT StreamNumber, IDirect3DVertexBuffer8 **pStream, UINT *pStride)
{
    IDirect3DDevice8Impl *This = impl_from_IDirect3DDevice8(iface);
    struct wined3d_buffer *retStream = NULL;
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, buffer %p, stride %p.\n",
            iface, StreamNumber, pStream, pStride);

    if(pStream == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = wined3d_device_get_stream_source(This->wined3d_device, StreamNumber,
            &retStream, 0 /* Offset in bytes */, pStride);
    if (SUCCEEDED(hr) && retStream)
    {
        *pStream = wined3d_buffer_get_parent(retStream);
        IDirect3DVertexBuffer8_AddRef(*pStream);
        wined3d_buffer_decref(retStream);
    }
    else
    {
        if (FAILED(hr)) FIXME("Call to GetStreamSource failed, hr %#x.\n", hr);
        *pStream = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static const IDirect3DDevice8Vtbl Direct3DDevice8_Vtbl =
{
    IDirect3DDevice8Impl_QueryInterface,
    IDirect3DDevice8Impl_AddRef,
    IDirect3DDevice8Impl_Release,
    IDirect3DDevice8Impl_TestCooperativeLevel,
    IDirect3DDevice8Impl_GetAvailableTextureMem,
    IDirect3DDevice8Impl_ResourceManagerDiscardBytes,
    IDirect3DDevice8Impl_GetDirect3D,
    IDirect3DDevice8Impl_GetDeviceCaps,
    IDirect3DDevice8Impl_GetDisplayMode,
    IDirect3DDevice8Impl_GetCreationParameters,
    IDirect3DDevice8Impl_SetCursorProperties,
    IDirect3DDevice8Impl_SetCursorPosition,
    IDirect3DDevice8Impl_ShowCursor,
    IDirect3DDevice8Impl_CreateAdditionalSwapChain,
    IDirect3DDevice8Impl_Reset,
    IDirect3DDevice8Impl_Present,
    IDirect3DDevice8Impl_GetBackBuffer,
    IDirect3DDevice8Impl_GetRasterStatus,
    IDirect3DDevice8Impl_SetGammaRamp,
    IDirect3DDevice8Impl_GetGammaRamp,
    IDirect3DDevice8Impl_CreateTexture,
    IDirect3DDevice8Impl_CreateVolumeTexture,
    IDirect3DDevice8Impl_CreateCubeTexture,
    IDirect3DDevice8Impl_CreateVertexBuffer,
    IDirect3DDevice8Impl_CreateIndexBuffer,
    IDirect3DDevice8Impl_CreateRenderTarget,
    IDirect3DDevice8Impl_CreateDepthStencilSurface,
    IDirect3DDevice8Impl_CreateImageSurface,
    IDirect3DDevice8Impl_CopyRects,
    IDirect3DDevice8Impl_UpdateTexture,
    IDirect3DDevice8Impl_GetFrontBuffer,
    IDirect3DDevice8Impl_SetRenderTarget,
    IDirect3DDevice8Impl_GetRenderTarget,
    IDirect3DDevice8Impl_GetDepthStencilSurface,
    IDirect3DDevice8Impl_BeginScene,
    IDirect3DDevice8Impl_EndScene,
    IDirect3DDevice8Impl_Clear,
    IDirect3DDevice8Impl_SetTransform,
    IDirect3DDevice8Impl_GetTransform,
    IDirect3DDevice8Impl_MultiplyTransform,
    IDirect3DDevice8Impl_SetViewport,
    IDirect3DDevice8Impl_GetViewport,
    IDirect3DDevice8Impl_SetMaterial,
    IDirect3DDevice8Impl_GetMaterial,
    IDirect3DDevice8Impl_SetLight,
    IDirect3DDevice8Impl_GetLight,
    IDirect3DDevice8Impl_LightEnable,
    IDirect3DDevice8Impl_GetLightEnable,
    IDirect3DDevice8Impl_SetClipPlane,
    IDirect3DDevice8Impl_GetClipPlane,
    IDirect3DDevice8Impl_SetRenderState,
    IDirect3DDevice8Impl_GetRenderState,
    IDirect3DDevice8Impl_BeginStateBlock,
    IDirect3DDevice8Impl_EndStateBlock,
    IDirect3DDevice8Impl_ApplyStateBlock,
    IDirect3DDevice8Impl_CaptureStateBlock,
    IDirect3DDevice8Impl_DeleteStateBlock,
    IDirect3DDevice8Impl_CreateStateBlock,
    IDirect3DDevice8Impl_SetClipStatus,
    IDirect3DDevice8Impl_GetClipStatus,
    IDirect3DDevice8Impl_GetTexture,
    IDirect3DDevice8Impl_SetTexture,
    IDirect3DDevice8Impl_GetTextureStageState,
    IDirect3DDevice8Impl_SetTextureStageState,
    IDirect3DDevice8Impl_ValidateDevice,
    IDirect3DDevice8Impl_GetInfo,
    IDirect3DDevice8Impl_SetPaletteEntries,
    IDirect3DDevice8Impl_GetPaletteEntries,
    IDirect3DDevice8Impl_SetCurrentTexturePalette,
    IDirect3DDevice8Impl_GetCurrentTexturePalette,
    IDirect3DDevice8Impl_DrawPrimitive,
    IDirect3DDevice8Impl_DrawIndexedPrimitive,
    IDirect3DDevice8Impl_DrawPrimitiveUP,
    IDirect3DDevice8Impl_DrawIndexedPrimitiveUP,
    IDirect3DDevice8Impl_ProcessVertices,
    IDirect3DDevice8Impl_CreateVertexShader,
    IDirect3DDevice8Impl_SetVertexShader,
    IDirect3DDevice8Impl_GetVertexShader,
    IDirect3DDevice8Impl_DeleteVertexShader,
    IDirect3DDevice8Impl_SetVertexShaderConstant,
    IDirect3DDevice8Impl_GetVertexShaderConstant,
    IDirect3DDevice8Impl_GetVertexShaderDeclaration,
    IDirect3DDevice8Impl_GetVertexShaderFunction,
    IDirect3DDevice8Impl_SetStreamSource,
    IDirect3DDevice8Impl_GetStreamSource,
    IDirect3DDevice8Impl_SetIndices,
    IDirect3DDevice8Impl_GetIndices,
    IDirect3DDevice8Impl_CreatePixelShader,
    IDirect3DDevice8Impl_SetPixelShader,
    IDirect3DDevice8Impl_GetPixelShader,
    IDirect3DDevice8Impl_DeletePixelShader,
    IDirect3DDevice8Impl_SetPixelShaderConstant,
    IDirect3DDevice8Impl_GetPixelShaderConstant,
    IDirect3DDevice8Impl_GetPixelShaderFunction,
    IDirect3DDevice8Impl_DrawRectPatch,
    IDirect3DDevice8Impl_DrawTriPatch,
    IDirect3DDevice8Impl_DeletePatch
};

static inline IDirect3DDevice8Impl *device_from_device_parent(struct wined3d_device_parent *device_parent)
{
    return CONTAINING_RECORD(device_parent, IDirect3DDevice8Impl, device_parent);
}

static void CDECL device_parent_wined3d_device_created(struct wined3d_device_parent *device_parent,
        struct wined3d_device *device)
{
    TRACE("device_parent %p, device %p\n", device_parent, device);
}

static void CDECL device_parent_mode_changed(struct wined3d_device_parent *device_parent)
{
    TRACE("device_parent %p.\n", device_parent);
}

static HRESULT CDECL device_parent_create_surface(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, enum wined3d_format_id format, DWORD usage,
        enum wined3d_pool pool, UINT level, enum wined3d_cubemap_face face, struct wined3d_surface **surface)
{
    IDirect3DDevice8Impl *device = device_from_device_parent(device_parent);
    IDirect3DSurface8Impl *d3d_surface;
    BOOL lockable = TRUE;
    HRESULT hr;

    TRACE("device_parent %p, container_parent %p, width %u, height %u, format %#x, usage %#x,\n"
            "\tpool %#x, level %u, face %u, surface %p.\n",
            device_parent, container_parent, width, height, format, usage, pool, level, face, surface);


    if (pool == WINED3D_POOL_DEFAULT && !(usage & WINED3DUSAGE_DYNAMIC))
        lockable = FALSE;

    hr = IDirect3DDevice8Impl_CreateSurface(device, width, height,
            d3dformat_from_wined3dformat(format), lockable, FALSE /* Discard */, level,
            (IDirect3DSurface8 **)&d3d_surface, usage, pool, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);
    if (FAILED(hr))
    {
        WARN("Failed to create surface, hr %#x.\n", hr);
        return hr;
    }

    *surface = d3d_surface->wined3d_surface;
    wined3d_surface_incref(*surface);

    d3d_surface->container = container_parent;
    IUnknown_Release(d3d_surface->parentDevice);
    d3d_surface->parentDevice = NULL;

    IDirect3DSurface8_Release(&d3d_surface->IDirect3DSurface8_iface);
    d3d_surface->forwardReference = container_parent;

    return hr;
}

static HRESULT CDECL device_parent_create_rendertarget(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, enum wined3d_format_id format,
        enum wined3d_multisample_type multisample_type, DWORD multisample_quality, BOOL lockable,
        struct wined3d_surface **surface)
{
    IDirect3DDevice8Impl *device = device_from_device_parent(device_parent);
    IDirect3DSurface8Impl *d3d_surface;
    HRESULT hr;

    TRACE("device_parent %p, container_parent %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, lockable %u, surface %p.\n",
            device_parent, container_parent, width, height, format,
            multisample_type, multisample_quality, lockable, surface);

    hr = IDirect3DDevice8_CreateRenderTarget(&device->IDirect3DDevice8_iface, width, height,
            d3dformat_from_wined3dformat(format), multisample_type, lockable, (IDirect3DSurface8 **)&d3d_surface);
    if (FAILED(hr))
    {
        WARN("Failed to create rendertarget, hr %#x.\n", hr);
        return hr;
    }

    *surface = d3d_surface->wined3d_surface;
    wined3d_surface_incref(*surface);

    d3d_surface->container = (IUnknown *)&device->IDirect3DDevice8_iface;
    /* Implicit surfaces are created with an refcount of 0 */
    IDirect3DSurface8_Release(&d3d_surface->IDirect3DSurface8_iface);

    return hr;
}

static HRESULT CDECL device_parent_create_depth_stencil(struct wined3d_device_parent *device_parent,
        UINT width, UINT height, enum wined3d_format_id format, enum wined3d_multisample_type multisample_type,
        DWORD multisample_quality, BOOL discard, struct wined3d_surface **surface)
{
    IDirect3DDevice8Impl *device = device_from_device_parent(device_parent);
    IDirect3DSurface8Impl *d3d_surface;
    HRESULT hr;

    TRACE("device_parent %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, discard %u, surface %p.\n",
            device_parent, width, height, format, multisample_type, multisample_quality, discard, surface);

    hr = IDirect3DDevice8_CreateDepthStencilSurface(&device->IDirect3DDevice8_iface, width, height,
            d3dformat_from_wined3dformat(format), multisample_type, (IDirect3DSurface8 **)&d3d_surface);
    if (FAILED(hr))
    {
        WARN("Failed to create depth/stencil surface, hr %#x.\n", hr);
        return hr;
    }

    *surface = d3d_surface->wined3d_surface;
    wined3d_surface_incref(*surface);

    d3d_surface->container = (IUnknown *)&device->IDirect3DDevice8_iface;
    /* Implicit surfaces are created with an refcount of 0 */
    IDirect3DSurface8_Release(&d3d_surface->IDirect3DSurface8_iface);

    return hr;
}

static HRESULT CDECL device_parent_create_volume(struct wined3d_device_parent *device_parent,
        void *container_parent, UINT width, UINT height, UINT depth, enum wined3d_format_id format,
        enum wined3d_pool pool, DWORD usage, struct wined3d_volume **volume)
{
    IDirect3DDevice8Impl *device = device_from_device_parent(device_parent);
    IDirect3DVolume8Impl *object;
    HRESULT hr;

    TRACE("device_parent %p, container_parent %p, width %u, height %u, depth %u, "
            "format %#x, pool %#x, usage %#x, volume %p.\n",
            device_parent, container_parent, width, height, depth,
            format, pool, usage, volume);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        FIXME("Allocation of memory failed\n");
        *volume = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = volume_init(object, device, width, height, depth, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize volume, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *volume = object->wined3d_volume;
    wined3d_volume_incref(*volume);
    IDirect3DVolume8_Release(&object->IDirect3DVolume8_iface);

    object->container = container_parent;
    object->forwardReference = container_parent;

    TRACE("Created volume %p.\n", object);

    return hr;
}

static HRESULT CDECL device_parent_create_swapchain(struct wined3d_device_parent *device_parent,
        struct wined3d_swapchain_desc *desc, struct wined3d_swapchain **swapchain)
{
    IDirect3DDevice8Impl *device = device_from_device_parent(device_parent);
    D3DPRESENT_PARAMETERS local_parameters;
    IDirect3DSwapChain8 *d3d_swapchain;
    HRESULT hr;

    TRACE("device_parent %p, desc %p, swapchain %p.\n", device_parent, desc, swapchain);

    /* Copy the presentation parameters */
    local_parameters.BackBufferWidth = desc->backbuffer_width;
    local_parameters.BackBufferHeight = desc->backbuffer_height;
    local_parameters.BackBufferFormat = d3dformat_from_wined3dformat(desc->backbuffer_format);
    local_parameters.BackBufferCount = desc->backbuffer_count;
    local_parameters.MultiSampleType = desc->multisample_type;
    local_parameters.SwapEffect = desc->swap_effect;
    local_parameters.hDeviceWindow = desc->device_window;
    local_parameters.Windowed = desc->windowed;
    local_parameters.EnableAutoDepthStencil = desc->enable_auto_depth_stencil;
    local_parameters.AutoDepthStencilFormat = d3dformat_from_wined3dformat(desc->auto_depth_stencil_format);
    local_parameters.Flags = desc->flags;
    local_parameters.FullScreen_RefreshRateInHz = desc->refresh_rate;
    local_parameters.FullScreen_PresentationInterval = desc->swap_interval;

    hr = IDirect3DDevice8_CreateAdditionalSwapChain(&device->IDirect3DDevice8_iface,
            &local_parameters, &d3d_swapchain);
    if (FAILED(hr))
    {
        WARN("Failed to create swapchain, hr %#x.\n", hr);
        *swapchain = NULL;
        return hr;
    }

    *swapchain = ((IDirect3DSwapChain8Impl *)d3d_swapchain)->wined3d_swapchain;
    wined3d_swapchain_incref(*swapchain);
    IDirect3DSwapChain8_Release(d3d_swapchain);

    /* Copy back the presentation parameters */
    desc->backbuffer_width = local_parameters.BackBufferWidth;
    desc->backbuffer_height = local_parameters.BackBufferHeight;
    desc->backbuffer_format = wined3dformat_from_d3dformat(local_parameters.BackBufferFormat);
    desc->backbuffer_count = local_parameters.BackBufferCount;
    desc->multisample_type = local_parameters.MultiSampleType;
    desc->swap_effect = local_parameters.SwapEffect;
    desc->device_window = local_parameters.hDeviceWindow;
    desc->windowed = local_parameters.Windowed;
    desc->enable_auto_depth_stencil = local_parameters.EnableAutoDepthStencil;
    desc->auto_depth_stencil_format = wined3dformat_from_d3dformat(local_parameters.AutoDepthStencilFormat);
    desc->flags = local_parameters.Flags;
    desc->refresh_rate = local_parameters.FullScreen_RefreshRateInHz;
    desc->swap_interval = local_parameters.FullScreen_PresentationInterval;

    return hr;
}

static const struct wined3d_device_parent_ops d3d8_wined3d_device_parent_ops =
{
    device_parent_wined3d_device_created,
    device_parent_mode_changed,
    device_parent_create_surface,
    device_parent_create_rendertarget,
    device_parent_create_depth_stencil,
    device_parent_create_volume,
    device_parent_create_swapchain,
};

static void setup_fpu(void)
{
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    WORD cw;
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
    cw = (cw & ~0xf3f) | 0x3f;
    __asm__ volatile ("fldcw %0" : : "m" (cw));
#elif defined(__i386__) && defined(_MSC_VER)
    WORD cw;
    __asm fnstcw cw;
    cw = (cw & ~0xf3f) | 0x3f;
    __asm fldcw cw;
#else
    FIXME("FPU setup not implemented for this platform.\n");
#endif
}

HRESULT device_init(IDirect3DDevice8Impl *device, IDirect3D8Impl *parent, struct wined3d *wined3d, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters)
{
    struct wined3d_swapchain_desc swapchain_desc;
    HRESULT hr;

    device->IDirect3DDevice8_iface.lpVtbl = &Direct3DDevice8_Vtbl;
    device->device_parent.ops = &d3d8_wined3d_device_parent_ops;
    device->ref = 1;
    device->handle_table.entries = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
            D3D8_INITIAL_HANDLE_TABLE_SIZE * sizeof(*device->handle_table.entries));
    if (!device->handle_table.entries)
    {
        ERR("Failed to allocate handle table memory.\n");
        return E_OUTOFMEMORY;
    }
    device->handle_table.table_size = D3D8_INITIAL_HANDLE_TABLE_SIZE;

    if (!(flags & D3DCREATE_FPU_PRESERVE)) setup_fpu();

    wined3d_mutex_lock();
    hr = wined3d_device_create(wined3d, adapter, device_type, focus_window, flags, 4,
            &device->device_parent, &device->wined3d_device);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d device, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
        return hr;
    }

    if (!parameters->Windowed)
    {
        HWND device_window = parameters->hDeviceWindow;

        if (!focus_window)
            focus_window = device_window;
        if (FAILED(hr = wined3d_device_acquire_focus_window(device->wined3d_device, focus_window)))
        {
            ERR("Failed to acquire focus window, hr %#x.\n", hr);
            wined3d_device_decref(device->wined3d_device);
            wined3d_mutex_unlock();
            HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
            return hr;
        }

        if (!device_window)
            device_window = focus_window;
        wined3d_device_setup_fullscreen_window(device->wined3d_device, device_window,
                parameters->BackBufferWidth,
                parameters->BackBufferHeight);
    }

    if (flags & D3DCREATE_MULTITHREADED)
        wined3d_device_set_multithreaded(device->wined3d_device);

    swapchain_desc.backbuffer_width = parameters->BackBufferWidth;
    swapchain_desc.backbuffer_height = parameters->BackBufferHeight;
    swapchain_desc.backbuffer_format = wined3dformat_from_d3dformat(parameters->BackBufferFormat);
    swapchain_desc.backbuffer_count = parameters->BackBufferCount;
    swapchain_desc.multisample_type = parameters->MultiSampleType;
    swapchain_desc.multisample_quality = 0; /* d3d9 only */
    swapchain_desc.swap_effect = parameters->SwapEffect;
    swapchain_desc.device_window = parameters->hDeviceWindow;
    swapchain_desc.windowed = parameters->Windowed;
    swapchain_desc.enable_auto_depth_stencil = parameters->EnableAutoDepthStencil;
    swapchain_desc.auto_depth_stencil_format = wined3dformat_from_d3dformat(parameters->AutoDepthStencilFormat);
    swapchain_desc.flags = parameters->Flags;
    swapchain_desc.refresh_rate = parameters->FullScreen_RefreshRateInHz;
    swapchain_desc.swap_interval = parameters->FullScreen_PresentationInterval;
    swapchain_desc.auto_restore_display_mode = TRUE;

    hr = wined3d_device_init_3d(device->wined3d_device, &swapchain_desc);
    if (FAILED(hr))
    {
        WARN("Failed to initialize 3D, hr %#x.\n", hr);
        wined3d_device_release_focus_window(device->wined3d_device);
        wined3d_device_decref(device->wined3d_device);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
        return hr;
    }

    hr = wined3d_device_set_render_state(device->wined3d_device, WINED3D_RS_POINTSIZE_MIN, 0);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        ERR("Failed to set minimum pointsize, hr %#x.\n", hr);
        goto err;
    }

    parameters->BackBufferWidth = swapchain_desc.backbuffer_width;
    parameters->BackBufferHeight = swapchain_desc.backbuffer_height;
    parameters->BackBufferFormat = d3dformat_from_wined3dformat(swapchain_desc.backbuffer_format);
    parameters->BackBufferCount = swapchain_desc.backbuffer_count;
    parameters->MultiSampleType = swapchain_desc.multisample_type;
    parameters->SwapEffect = swapchain_desc.swap_effect;
    parameters->hDeviceWindow = swapchain_desc.device_window;
    parameters->Windowed = swapchain_desc.windowed;
    parameters->EnableAutoDepthStencil = swapchain_desc.enable_auto_depth_stencil;
    parameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(swapchain_desc.auto_depth_stencil_format);
    parameters->Flags = swapchain_desc.flags;
    parameters->FullScreen_RefreshRateInHz = swapchain_desc.refresh_rate;
    parameters->FullScreen_PresentationInterval = swapchain_desc.swap_interval;

    device->declArraySize = 16;
    device->decls = HeapAlloc(GetProcessHeap(), 0, device->declArraySize * sizeof(*device->decls));
    if (!device->decls)
    {
        ERR("Failed to allocate FVF vertex declaration map memory.\n");
        hr = E_OUTOFMEMORY;
        goto err;
    }

    device->d3d_parent = &parent->IDirect3D8_iface;
    IDirect3D8_AddRef(device->d3d_parent);

    return D3D_OK;

err:
    wined3d_mutex_lock();
    wined3d_device_uninit_3d(device->wined3d_device);
    wined3d_device_release_focus_window(device->wined3d_device);
    wined3d_device_decref(device->wined3d_device);
    wined3d_mutex_unlock();
    HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
    return hr;
}
