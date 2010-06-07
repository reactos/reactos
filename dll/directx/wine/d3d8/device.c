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

D3DFORMAT d3dformat_from_wined3dformat(WINED3DFORMAT format)
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
            FIXME("Unhandled WINED3DFORMAT %#x\n", format);
            return D3DFMT_UNKNOWN;
    }
}

WINED3DFORMAT wined3dformat_from_d3dformat(D3DFORMAT format)
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

static ULONG WINAPI D3D8CB_DestroySwapChain(IWineD3DSwapChain *swapchain)
{
    IUnknown *parent;

    TRACE("swapchain %p.\n", swapchain);

    IWineD3DSwapChain_GetParent(swapchain, &parent);
    IUnknown_Release(parent);
    return IUnknown_Release(parent);
}

/* IDirect3D IUnknown parts follow: */
static HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface,REFIID riid,LPVOID *ppobj)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("iface %p, riid %s, object %p.\n",
            iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DDevice8)) {
        IUnknown_AddRef(iface);
        *ppobj = This;
        return S_OK;
    }

    if (IsEqualGUID(riid, &IID_IWineD3DDeviceParent))
    {
        IUnknown_AddRef((IUnknown *)&This->device_parent_vtbl);
        *ppobj = &This->device_parent_vtbl;
        return S_OK;
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3DDevice8Impl_AddRef(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref;

    if (This->inDestruction) return 0;
    ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        unsigned i;

        TRACE("Releasing wined3d device %p\n", This->WineD3DDevice);

        wined3d_mutex_lock();

        This->inDestruction = TRUE;

        for(i = 0; i < This->numConvertedDecls; i++) {
            IDirect3DVertexDeclaration8_Release(This->decls[i].decl);
        }
        HeapFree(GetProcessHeap(), 0, This->decls);

        IWineD3DDevice_Uninit3D(This->WineD3DDevice, D3D8CB_DestroySwapChain);
        IWineD3DDevice_ReleaseFocusWindow(This->WineD3DDevice);
        IWineD3DDevice_Release(This->WineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This->handle_table.entries);
        HeapFree(GetProcessHeap(), 0, This);

        wined3d_mutex_unlock();
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
static HRESULT WINAPI IDirect3DDevice8Impl_TestCooperativeLevel(IDirect3DDevice8 *iface)
{
    TRACE("iface %p.\n", iface);

    return D3D_OK;
}

static UINT WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetAvailableTextureMem(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ResourceManagerDiscardBytes(LPDIRECT3DDEVICE8 iface, DWORD Bytes) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, byte_count %u.\n", iface, Bytes);
    FIXME("Byte count ignored.\n");

    wined3d_mutex_lock();
    hr = IWineD3DDevice_EvictManagedResources(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDirect3D(LPDIRECT3DDEVICE8 iface, IDirect3D8** ppD3D8) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3D* pWineD3D;

    TRACE("iface %p, d3d8 %p.\n", iface, ppD3D8);

    if (NULL == ppD3D8) {
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetDirect3D(This->WineD3DDevice, &pWineD3D);
    if (hr == D3D_OK && pWineD3D != NULL)
    {
        IWineD3D_GetParent(pWineD3D,(IUnknown **)ppD3D8);
        IWineD3D_Release(pWineD3D);
    } else {
        FIXME("Call to IWineD3DDevice_GetDirect3D failed\n");
        *ppD3D8 = NULL;
    }
    wined3d_mutex_unlock();

    TRACE("(%p) returning %p\n",This , *ppD3D8);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDeviceCaps(LPDIRECT3DDEVICE8 iface, D3DCAPS8* pCaps) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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
    hrc = IWineD3DDevice_GetDeviceCaps(This->WineD3DDevice, pWineCaps);
    wined3d_mutex_unlock();

    fixup_caps(pWineCaps);
    WINECAPSTOD3D8CAPS(pCaps, pWineCaps)
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    TRACE("Returning %p %p\n", This, pCaps);
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDisplayMode(LPDIRECT3DDEVICE8 iface, D3DDISPLAYMODE* pMode) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, mode %p.\n", iface, pMode);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetDisplayMode(This->WineD3DDevice, 0, (WINED3DDISPLAYMODE *) pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetCreationParameters(LPDIRECT3DDEVICE8 iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, parameters %p.\n", iface, pParameters);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetCreationParameters(This->WineD3DDevice, (WINED3DDEVICE_CREATION_PARAMETERS *) pParameters);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *pSurface = (IDirect3DSurface8Impl*)pCursorBitmap;
    HRESULT hr;

    TRACE("iface %p, hotspot_x %u, hotspot_y %u, bitmap %p.\n",
            iface, XHotSpot, YHotSpot, pCursorBitmap);

    if(!pCursorBitmap) {
        WARN("No cursor bitmap, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetCursorProperties(This->WineD3DDevice,XHotSpot,YHotSpot,pSurface->wineD3DSurface);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("iface %p, x %u, y %u, flags %#x.\n",
            iface, XScreenSpace, YScreenSpace, Flags);

    wined3d_mutex_lock();
    IWineD3DDevice_SetCursorPosition(This->WineD3DDevice, XScreenSpace, YScreenSpace, Flags);
    wined3d_mutex_unlock();
}

static BOOL WINAPI IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    BOOL ret;

    TRACE("iface %p, show %#x.\n", iface, bShow);

    wined3d_mutex_lock();
    ret = IWineD3DDevice_ShowCursor(This->WineD3DDevice, bShow);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateAdditionalSwapChain(IDirect3DDevice8 *iface,
        D3DPRESENT_PARAMETERS *present_parameters, IDirect3DSwapChain8 **swapchain)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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
    *swapchain = (IDirect3DSwapChain8 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Reset(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    WINED3DPRESENT_PARAMETERS localParameters;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p.\n", iface, pPresentationParameters);

    localParameters.BackBufferWidth                             = pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight                            = pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat                            = wined3dformat_from_d3dformat(pPresentationParameters->BackBufferFormat);
    localParameters.BackBufferCount                             = pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                             = pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality                          = 0; /* d3d9 only */
    localParameters.SwapEffect                                  = pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                               = pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                                    = pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil                      = pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat                      = wined3dformat_from_d3dformat(pPresentationParameters->AutoDepthStencilFormat);
    localParameters.Flags                                       = pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz                  = pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval                        = pPresentationParameters->FullScreen_PresentationInterval;
    localParameters.AutoRestoreDisplayMode                      = TRUE;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_Reset(This->WineD3DDevice, &localParameters);
    if(SUCCEEDED(hr)) {
        hr = IWineD3DDevice_SetRenderState(This->WineD3DDevice, WINED3DRS_POINTSIZE_MIN, 0);
    }
    wined3d_mutex_unlock();

    pPresentationParameters->BackBufferWidth                    = localParameters.BackBufferWidth;
    pPresentationParameters->BackBufferHeight                   = localParameters.BackBufferHeight;
    pPresentationParameters->BackBufferFormat                   = d3dformat_from_wined3dformat(localParameters.BackBufferFormat);
    pPresentationParameters->BackBufferCount                    = localParameters.BackBufferCount;
    pPresentationParameters->MultiSampleType                    = localParameters.MultiSampleType;
    pPresentationParameters->SwapEffect                         = localParameters.SwapEffect;
    pPresentationParameters->hDeviceWindow                      = localParameters.hDeviceWindow;
    pPresentationParameters->Windowed                           = localParameters.Windowed;
    pPresentationParameters->EnableAutoDepthStencil             = localParameters.EnableAutoDepthStencil;
    pPresentationParameters->AutoDepthStencilFormat             = d3dformat_from_wined3dformat(localParameters.AutoDepthStencilFormat);
    pPresentationParameters->Flags                              = localParameters.Flags;
    pPresentationParameters->FullScreen_RefreshRateInHz         = localParameters.FullScreen_RefreshRateInHz;
    pPresentationParameters->FullScreen_PresentationInterval    = localParameters.PresentationInterval;

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, src_rect %p, dst_rect %p, dst_window_override %p, dirty_region %p.\n",
            iface, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_Present(This->WineD3DDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DSurface *retSurface = NULL;
    HRESULT rc = D3D_OK;

    TRACE("iface %p, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, BackBuffer, Type, ppBackBuffer);

    wined3d_mutex_lock();
    rc = IWineD3DDevice_GetBackBuffer(This->WineD3DDevice, 0, BackBuffer, (WINED3DBACKBUFFER_TYPE) Type, &retSurface);
    if (rc == D3D_OK && NULL != retSurface && NULL != ppBackBuffer) {
        IWineD3DSurface_GetParent(retSurface, (IUnknown **)ppBackBuffer);
        IWineD3DSurface_Release(retSurface);
    }
    wined3d_mutex_unlock();

    return rc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetRasterStatus(LPDIRECT3DDEVICE8 iface, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, raster_status %p.\n", iface, pRasterStatus);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetRasterStatus(This->WineD3DDevice, 0, (WINED3DRASTER_STATUS *) pRasterStatus);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("iface %p, flags %#x, ramp %p.\n", iface, Flags, pRamp);

    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    wined3d_mutex_lock();
    IWineD3DDevice_SetGammaRamp(This->WineD3DDevice, 0, Flags, (CONST WINED3DGAMMARAMP *) pRamp);
    wined3d_mutex_unlock();
}

static void WINAPI IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("iface %p, ramp %p.\n", iface, pRamp);

    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    wined3d_mutex_lock();
    IWineD3DDevice_GetGammaRamp(This->WineD3DDevice, 0, (WINED3DGAMMARAMP *) pRamp);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateTexture(IDirect3DDevice8 *iface,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DTexture8 **texture)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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
    *texture = (IDirect3DTexture8 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVolumeTexture(IDirect3DDevice8 *iface,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DVolumeTexture8 **texture)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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
    *texture = (IDirect3DVolumeTexture8 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateCubeTexture(IDirect3DDevice8 *iface, UINT edge_length,
        UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool, IDirect3DCubeTexture8 **texture)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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
    *texture = (IDirect3DCubeTexture8 *)object;

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexBuffer(IDirect3DDevice8 *iface, UINT size, DWORD usage,
        DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer8 **buffer)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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
    *buffer = (IDirect3DVertexBuffer8 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateIndexBuffer(IDirect3DDevice8 *iface, UINT size, DWORD usage,
        D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer8 **buffer)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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
    *buffer = (IDirect3DIndexBuffer8 *)object;

    return D3D_OK;
}

static HRESULT IDirect3DDevice8Impl_CreateSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height,
        D3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IDirect3DSurface8 **ppSurface,
        UINT Usage, D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, lockable %#x, discard %#x, level %u, surface %p,\n"
            "\tusage %#x, pool %#x, multisample_type %#x, multisample_quality %u.\n",
            iface, Width, Height, Format, Lockable, Discard, Level, ppSurface,
            Usage, Pool, MultiSample, MultisampleQuality);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    if (!object)
    {
        FIXME("Failed to allocate surface memory.\n");
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = surface_init(object, This, Width, Height, Format, Lockable, Discard,
            Level, Usage, Pool, MultiSample, MultisampleQuality);
    if (FAILED(hr))
    {
        WARN("Failed to initialize surface, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created surface %p.\n", object);
    *ppSurface = (IDirect3DSurface8 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, lockable %#x, surface %p.\n",
            iface, Width, Height, Format, MultiSample, Lockable, ppSurface);

    hr = IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, Lockable, FALSE /* Discard */,
            0 /* Level */, ppSurface, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT, MultiSample, 0);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, surface %p.\n",
            iface, Width, Height, Format, MultiSample, ppSurface);

    /* TODO: Verify that Discard is false */
    hr = IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Lockable */, FALSE,
            0 /* Level */, ppSurface, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT, MultiSample, 0);

    return hr;
}

/*  IDirect3DDevice8Impl::CreateImageSurface returns surface with pool type SYSTEMMEM */
static HRESULT WINAPI IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, surface %p.\n",
            iface, Width, Height, Format, ppSurface);

    hr = IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Lockable */, FALSE /* Discard */,
            0 /* Level */, ppSurface, 0 /* Usage (undefined/none) */, D3DPOOL_SYSTEMMEM, D3DMULTISAMPLE_NONE,
            0 /* MultisampleQuality */);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8 *pSourceSurface, CONST RECT *pSourceRects, UINT cRects, IDirect3DSurface8 *pDestinationSurface, CONST POINT *pDestPoints) {
    IDirect3DSurface8Impl *Source = (IDirect3DSurface8Impl *) pSourceSurface;
    IDirect3DSurface8Impl *Dest = (IDirect3DSurface8Impl *) pDestinationSurface;

    HRESULT              hr = WINED3D_OK;
    WINED3DFORMAT        srcFormat, destFormat;
    WINED3DSURFACE_DESC  winedesc;

    TRACE("iface %p, src_surface %p, src_rects %p, rect_count %u, dst_surface %p, dst_points %p.\n",
            iface, pSourceSurface, pSourceRects, cRects, pDestinationSurface, pDestPoints);

    /* Check that the source texture is in WINED3DPOOL_SYSTEMMEM and the
     * destination texture is in WINED3DPOOL_DEFAULT. */

    wined3d_mutex_lock();
    IWineD3DSurface_GetDesc(Source->wineD3DSurface, &winedesc);
    srcFormat = winedesc.format;

    IWineD3DSurface_GetDesc(Dest->wineD3DSurface, &winedesc);
    destFormat = winedesc.format;

    /* Check that the source and destination formats match */
    if (srcFormat != destFormat && WINED3DFMT_UNKNOWN != destFormat) {
        WARN("(%p) source %p format must match the dest %p format, returning WINED3DERR_INVALIDCALL\n", iface, pSourceSurface, pDestinationSurface);
        wined3d_mutex_unlock();
        return WINED3DERR_INVALIDCALL;
    } else if (WINED3DFMT_UNKNOWN == destFormat) {
        TRACE("(%p) : Converting destination surface from WINED3DFMT_UNKNOWN to the source format\n", iface);
        IWineD3DSurface_SetFormat(Dest->wineD3DSurface, srcFormat);
    }

    /* Quick if complete copy ... */
    if (cRects == 0 && pSourceRects == NULL && pDestPoints == NULL) {
        IWineD3DSurface_BltFast(Dest->wineD3DSurface, 0, 0, Source->wineD3DSurface, NULL, WINEDDBLTFAST_NOCOLORKEY);
    } else {
        unsigned int i;
        /* Copy rect by rect */
        if (NULL != pSourceRects && NULL != pDestPoints) {
            for (i = 0; i < cRects; ++i) {
                IWineD3DSurface_BltFast(Dest->wineD3DSurface, pDestPoints[i].x, pDestPoints[i].y, Source->wineD3DSurface, &pSourceRects[i], WINEDDBLTFAST_NOCOLORKEY);
            }
        } else {
            for (i = 0; i < cRects; ++i) {
                IWineD3DSurface_BltFast(Dest->wineD3DSurface, 0, 0, Source->wineD3DSurface, &pSourceRects[i], WINEDDBLTFAST_NOCOLORKEY);
            }
        }
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, src_texture %p, dst_texture %p.\n", iface, pSourceTexture, pDestinationTexture);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_UpdateTexture(This->WineD3DDevice,  ((IDirect3DBaseTexture8Impl *)pSourceTexture)->wineD3DBaseTexture, ((IDirect3DBaseTexture8Impl *)pDestinationTexture)->wineD3DBaseTexture);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *destSurface = (IDirect3DSurface8Impl *)pDestSurface;
    HRESULT hr;

    TRACE("iface %p, dst_surface %p.\n", iface, pDestSurface);

    if (pDestSurface == NULL) {
        WARN("(%p) : Caller passed NULL as pDestSurface returning D3DERR_INVALIDCALL\n", This);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetFrontBufferData(This->WineD3DDevice, 0, destSurface->wineD3DSurface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *pSurface = (IDirect3DSurface8Impl *)pRenderTarget;
    IDirect3DSurface8Impl *pZSurface = (IDirect3DSurface8Impl *)pNewZStencil;
    IWineD3DSurface *original_ds = NULL;
    HRESULT hr;

    TRACE("iface %p, render_target %p, depth_stencil %p.\n", iface, pRenderTarget, pNewZStencil);

    wined3d_mutex_lock();

    hr = IWineD3DDevice_GetDepthStencilSurface(This->WineD3DDevice, &original_ds);
    if (hr == WINED3D_OK || hr == WINED3DERR_NOTFOUND)
    {
        hr = IWineD3DDevice_SetDepthStencilSurface(This->WineD3DDevice, pZSurface ? pZSurface->wineD3DSurface : NULL);
        if (SUCCEEDED(hr) && pSurface)
            hr = IWineD3DDevice_SetRenderTarget(This->WineD3DDevice, 0, pSurface->wineD3DSurface, TRUE);
        if (FAILED(hr)) IWineD3DDevice_SetDepthStencilSurface(This->WineD3DDevice, original_ds);
    }
    if (original_ds) IWineD3DSurface_Release(original_ds);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pRenderTarget;

    TRACE("iface %p, render_target %p.\n", iface, ppRenderTarget);

    if (ppRenderTarget == NULL) {
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetRenderTarget(This->WineD3DDevice, 0, &pRenderTarget);

    if (hr == D3D_OK && pRenderTarget != NULL) {
        IWineD3DSurface_GetParent(pRenderTarget,(IUnknown**)ppRenderTarget);
        IWineD3DSurface_Release(pRenderTarget);
    } else {
        FIXME("Call to IWineD3DDevice_GetRenderTarget failed\n");
        *ppRenderTarget = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pZStencilSurface;

    TRACE("iface %p, depth_stencil %p.\n", iface, ppZStencilSurface);

    if(ppZStencilSurface == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr=IWineD3DDevice_GetDepthStencilSurface(This->WineD3DDevice,&pZStencilSurface);
    if (hr == WINED3D_OK) {
        IWineD3DSurface_GetParent(pZStencilSurface,(IUnknown**)ppZStencilSurface);
        IWineD3DSurface_Release(pZStencilSurface);
    }else{
        if (hr != WINED3DERR_NOTFOUND)
                FIXME("Call to IWineD3DDevice_GetDepthStencilSurface failed with 0x%08x\n", hr);
        *ppZStencilSurface = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_BeginScene(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_EndScene(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, rect_count %u, rects %p, flags %#x, color 0x%08x, z %.8e, stencil %u.\n",
            iface, Count, pRects, Flags, Color, Z, Stencil);

    /* Note: D3DRECT is compatible with WINED3DRECT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_Clear(This->WineD3DDevice, Count, (CONST WINED3DRECT*) pRects, Flags, Color, Z, Stencil);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* lpMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, lpMatrix);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) lpMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, pMatrix);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetTransform(This->WineD3DDevice, State, (WINED3DMATRIX*) pMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_MultiplyTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, pMatrix);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_MultiplyTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) pMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetViewport(LPDIRECT3DDEVICE8 iface, CONST D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, pViewport);

    /* Note: D3DVIEWPORT8 is compatible with WINED3DVIEWPORT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetViewport(This->WineD3DDevice, (const WINED3DVIEWPORT *)pViewport);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, pViewport);

    /* Note: D3DVIEWPORT8 is compatible with WINED3DVIEWPORT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetViewport(This->WineD3DDevice, (WINED3DVIEWPORT *)pViewport);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetMaterial(LPDIRECT3DDEVICE8 iface, CONST D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, pMaterial);

    /* Note: D3DMATERIAL8 is compatible with WINED3DMATERIAL */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetMaterial(This->WineD3DDevice, (const WINED3DMATERIAL *)pMaterial);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetMaterial(LPDIRECT3DDEVICE8 iface, D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, pMaterial);

    /* Note: D3DMATERIAL8 is compatible with WINED3DMATERIAL */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetMaterial(This->WineD3DDevice, (WINED3DMATERIAL *)pMaterial);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index, CONST D3DLIGHT8* pLight) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, light %p.\n", iface, Index, pLight);

    /* Note: D3DLIGHT8 is compatible with WINED3DLIGHT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetLight(This->WineD3DDevice, Index, (const WINED3DLIGHT *)pLight);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,D3DLIGHT8* pLight) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, light %p.\n", iface, Index, pLight);

    /* Note: D3DLIGHT8 is compatible with WINED3DLIGHT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetLight(This->WineD3DDevice, Index, (WINED3DLIGHT *)pLight);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL Enable) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, enable %#x.\n", iface, Index, Enable);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetLightEnable(This->WineD3DDevice, Index, Enable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL* pEnable) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, enable %p.\n", iface, Index, pEnable);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetLightEnable(This->WineD3DDevice, Index, pEnable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, plane %p.\n", iface, Index, pPlane);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetClipPlane(This->WineD3DDevice, Index, pPlane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, plane %p.\n", iface, Index, pPlane);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetClipPlane(This->WineD3DDevice, Index, pPlane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD Value) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, value %#x.\n", iface, State, Value);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetRenderState(This->WineD3DDevice, State, Value);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, value %p.\n", iface, State, pValue);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetRenderState(This->WineD3DDevice, State, pValue);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_BeginStateBlock(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DStateBlock *stateblock;
    HRESULT hr;

    TRACE("iface %p, token %p.\n", iface, pToken);

    /* Tell wineD3D to endstateblock before anything else (in case we run out
     * of memory later and cause locking problems)
     */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_EndStateBlock(This->WineD3DDevice , &stateblock);
    if (hr != D3D_OK) {
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
        IWineD3DStateBlock_Release(stateblock);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    ++*pToken;

    TRACE("Returning %#x (%p).\n", *pToken, stateblock);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DDevice8Impl     *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DStateBlock *stateblock;
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
    hr = IWineD3DStateBlock_Apply(stateblock);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DStateBlock *stateblock;
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
    hr = IWineD3DStateBlock_Capture(stateblock);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DStateBlock *stateblock;

    TRACE("iface %p, token %#x.\n", iface, Token);

    wined3d_mutex_lock();
    stateblock = d3d8_free_handle(&This->handle_table, Token - 1, D3D8_HANDLE_SB);

    if (!stateblock)
    {
        WARN("Invalid handle (%#x) passed.\n", Token);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    if (IWineD3DStateBlock_Release((IUnknown *)stateblock))
    {
        ERR("Stateblock %p has references left, this shouldn't happen.\n", stateblock);
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateStateBlock(IDirect3DDevice8 *iface,
        D3DSTATEBLOCKTYPE Type, DWORD *handle)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DStateBlock *stateblock;
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
    hr = IWineD3DDevice_CreateStateBlock(This->WineD3DDevice, (WINED3DSTATEBLOCKTYPE)Type,
            &stateblock, NULL);
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
        IWineD3DStateBlock_Release(stateblock);
        wined3d_mutex_unlock();
        return E_FAIL;
    }
    ++*handle;

    TRACE("Returning %#x (%p).\n", *handle, stateblock);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetClipStatus(LPDIRECT3DDEVICE8 iface, CONST D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, pClipStatus);
/* FIXME: Verify that D3DCLIPSTATUS8 ~= WINED3DCLIPSTATUS */

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetClipStatus(This->WineD3DDevice, (const WINED3DCLIPSTATUS *)pClipStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetClipStatus(LPDIRECT3DDEVICE8 iface, D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, pClipStatus);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetClipStatus(This->WineD3DDevice, (WINED3DCLIPSTATUS *)pClipStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8** ppTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DBaseTexture *retTexture;
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, ppTexture);

    if(ppTexture == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetTexture(This->WineD3DDevice, Stage, &retTexture);
    if (FAILED(hr))
    {
        WARN("Failed to get texture for stage %u, hr %#x.\n", Stage, hr);
        wined3d_mutex_unlock();
        *ppTexture = NULL;
        return hr;
    }

    if (retTexture)
    {
        IWineD3DBaseTexture_GetParent(retTexture, (IUnknown **)ppTexture);
        IWineD3DBaseTexture_Release(retTexture);
    }
    else
    {
        *ppTexture = NULL;
    }
    wined3d_mutex_unlock();

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage, IDirect3DBaseTexture8* pTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, pTexture);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetTexture(This->WineD3DDevice, Stage,
                                   pTexture==NULL ? NULL : ((IDirect3DBaseTexture8Impl *)pTexture)->wineD3DBaseTexture);
    wined3d_mutex_unlock();

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
    {FALSE, WINED3DTSS_FORCE_DWORD},            /* 12, unused */
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
    {TRUE,  WINED3DSAMP_ADDRESSW},              /* 25, D3DTSS_ADDRESSW */
    {FALSE, WINED3DTSS_COLORARG0},              /* 26, D3DTSS_COLORARG0 */
    {FALSE, WINED3DTSS_ALPHAARG0},              /* 27, D3DTSS_ALPHAARG0 */
    {FALSE, WINED3DTSS_RESULTARG},              /* 28, D3DTSS_RESULTARG */
};

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    const struct tss_lookup *l = &tss_lookup[Type];
    HRESULT hr;

    TRACE("iface %p, stage %u, state %#x, value %p.\n", iface, Stage, Type, pValue);

    wined3d_mutex_lock();
    if (l->sampler_state) hr = IWineD3DDevice_GetSamplerState(This->WineD3DDevice, Stage, l->state, pValue);
    else hr = IWineD3DDevice_GetTextureStageState(This->WineD3DDevice, Stage, l->state, pValue);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    const struct tss_lookup *l = &tss_lookup[Type];
    HRESULT hr;

    TRACE("iface %p, stage %u, state %#x, value %#x.\n", iface, Stage, Type, Value);

    wined3d_mutex_lock();
    if (l->sampler_state) hr = IWineD3DDevice_SetSamplerState(This->WineD3DDevice, Stage, l->state, Value);
    else hr = IWineD3DDevice_SetTextureStageState(This->WineD3DDevice, Stage, l->state, Value);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, pass_count %p.\n", iface, pNumPasses);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_ValidateDevice(This->WineD3DDevice, pNumPasses);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetInfo(IDirect3DDevice8 *iface,
        DWORD info_id, void *info, DWORD info_size)
{
    FIXME("iface %p, info_id %#x, info %p, info_size %u stub!\n", iface, info_id, info, info_size);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %u, entries %p.\n", iface, PaletteNumber, pEntries);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %u, entries %p.\n", iface, PaletteNumber, pEntries);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %u.\n", iface, PaletteNumber);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %p.\n", iface, PaletteNumber);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitive(IDirect3DDevice8 *iface, D3DPRIMITIVETYPE PrimitiveType,
        UINT StartVertex, UINT PrimitiveCount)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, start_vertex %u, primitive_count %u.\n",
            iface, PrimitiveType, StartVertex, PrimitiveCount);

    wined3d_mutex_lock();
    IWineD3DDevice_SetPrimitiveType(This->WineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawPrimitive(This->WineD3DDevice, StartVertex,
            vertex_count_from_primitive_count(PrimitiveType, PrimitiveCount));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,
                                                           UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, min_vertex_idx %u, vertex_count %u, start_idx %u, primitive_count %u.\n",
            iface, PrimitiveType, MinVertexIndex, NumVertices, startIndex, primCount);

    wined3d_mutex_lock();
    IWineD3DDevice_SetPrimitiveType(This->WineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawIndexedPrimitive(This->WineD3DDevice, startIndex,
            vertex_count_from_primitive_count(PrimitiveType, primCount));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, primitive_count %u, data %p, stride %u.\n",
            iface, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);

    wined3d_mutex_lock();
    IWineD3DDevice_SetPrimitiveType(This->WineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawPrimitiveUP(This->WineD3DDevice,
            vertex_count_from_primitive_count(PrimitiveType, PrimitiveCount),
            pVertexStreamZeroData, VertexStreamZeroStride);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,
                                                             UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,
                                                             D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, min_vertex_idx %u, index_count %u, primitive_count %u,\n"
            "index_data %p, index_format %#x, vertex_data %p, vertex_stride %u.\n",
            iface, PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount,
            pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);

    wined3d_mutex_lock();
    IWineD3DDevice_SetPrimitiveType(This->WineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawIndexedPrimitiveUP(This->WineD3DDevice,
            vertex_count_from_primitive_count(PrimitiveType, PrimitiveCount), pIndexData,
            wined3dformat_from_d3dformat(IndexDataFormat), pVertexStreamZeroData, VertexStreamZeroStride);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ProcessVertices(LPDIRECT3DDEVICE8 iface, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer8* pDestBuffer,DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    IDirect3DVertexBuffer8Impl *dest = (IDirect3DVertexBuffer8Impl *) pDestBuffer;

    TRACE("iface %p, src_start_idx %u, dst_idx %u, vertex_count %u, dst_buffer %p, flags %#x.\n",
            iface, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_ProcessVertices(This->WineD3DDevice,SrcStartIndex, DestIndex, VertexCount, dest->wineD3DVertexBuffer, NULL, Flags, dest->fvf);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexShader(IDirect3DDevice8 *iface,
        const DWORD *declaration, const DWORD *byte_code, DWORD *shader, DWORD usage)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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

static HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DVertexShader8Impl *shader;
    HRESULT hr;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    if (VS_HIGHESTFIXEDFXF >= pShader) {
        TRACE("Setting FVF, %#x\n", pShader);

        wined3d_mutex_lock();
        IWineD3DDevice_SetVertexDeclaration(This->WineD3DDevice,
                IDirect3DDevice8Impl_FindDecl(This, pShader)->wined3d_vertex_declaration);
        IWineD3DDevice_SetVertexShader(This->WineD3DDevice, NULL);
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

    hr = IWineD3DDevice_SetVertexDeclaration(This->WineD3DDevice,
            ((IDirect3DVertexDeclaration8Impl *)shader->vertex_declaration)->wined3d_vertex_declaration);
    if (SUCCEEDED(hr)) hr = IWineD3DDevice_SetVertexShader(This->WineD3DDevice, shader->wineD3DVertexShader);
    wined3d_mutex_unlock();

    TRACE("Returning hr %#x\n", hr);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DVertexDeclaration *wined3d_declaration;
    IDirect3DVertexDeclaration8 *d3d8_declaration;
    HRESULT hrc;

    TRACE("iface %p, shader %p.\n", iface, ppShader);

    wined3d_mutex_lock();
    hrc = IWineD3DDevice_GetVertexDeclaration(This->WineD3DDevice, &wined3d_declaration);
    if (FAILED(hrc))
    {
        wined3d_mutex_unlock();
        WARN("(%p) : Call to IWineD3DDevice_GetVertexDeclaration failed %#x (device %p)\n",
                This, hrc, This->WineD3DDevice);
        return hrc;
    }

    if (!wined3d_declaration)
    {
        wined3d_mutex_unlock();
        *ppShader = 0;
        return D3D_OK;
    }

    hrc = IWineD3DVertexDeclaration_GetParent(wined3d_declaration, (IUnknown **)&d3d8_declaration);
    IWineD3DVertexDeclaration_Release(wined3d_declaration);
    wined3d_mutex_unlock();
    if (SUCCEEDED(hrc))
    {
        *ppShader = ((IDirect3DVertexDeclaration8Impl *)d3d8_declaration)->shader_handle;
        IDirect3DVertexDeclaration8_Release(d3d8_declaration);
    }

    TRACE("(%p) : returning %#x\n", This, *ppShader);

    return hrc;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteVertexShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DVertexShader8Impl *shader;
    IWineD3DVertexShader *cur = NULL;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    wined3d_mutex_lock();
    shader = d3d8_free_handle(&This->handle_table, pShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_VS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pShader);
        wined3d_mutex_unlock();

        return D3DERR_INVALIDCALL;
    }

    IWineD3DDevice_GetVertexShader(This->WineD3DDevice, &cur);

    if (cur)
    {
        if (cur == shader->wineD3DVertexShader) IDirect3DDevice8_SetVertexShader(iface, 0);
        IWineD3DVertexShader_Release(cur);
    }

    wined3d_mutex_unlock();

    if (IUnknown_Release((IUnknown *)shader))
    {
        ERR("Shader %p has references left, this shouldn't happen.\n", shader);
    }

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    if(Register + ConstantCount > D3D8_MAX_VERTEX_SHADER_CONSTANTF) {
        WARN("Trying to access %u constants, but d3d8 only supports %u\n",
             Register + ConstantCount, D3D8_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetVertexShaderConstantF(This->WineD3DDevice, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    if(Register + ConstantCount > D3D8_MAX_VERTEX_SHADER_CONSTANTF) {
        WARN("Trying to access %u constants, but d3d8 only supports %u\n",
             Register + ConstantCount, D3D8_MAX_VERTEX_SHADER_CONSTANTF);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetVertexShaderConstantF(This->WineD3DDevice, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderDeclaration(LPDIRECT3DDEVICE8 iface, DWORD pVertexShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD pVertexShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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

    if (!shader->wineD3DVertexShader)
    {
        wined3d_mutex_unlock();
        *pSizeOfData = 0;
        return D3D_OK;
    }

    hr = IWineD3DVertexShader_GetFunction(shader->wineD3DVertexShader, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8* pIndexData, UINT baseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    IDirect3DIndexBuffer8Impl *ib = (IDirect3DIndexBuffer8Impl *)pIndexData;

    TRACE("iface %p, buffer %p, base_vertex_idx %u.\n", iface, pIndexData, baseVertexIndex);

    /* WineD3D takes an INT(due to d3d9), but d3d8 uses UINTs. Do I have to add a check here that
     * the UINT doesn't cause an overflow in the INT? It seems rather unlikely because such large
     * vertex buffers can't be created to address them with an index that requires the 32nd bit
     * (4 Byte minimum vertex size * 2^31-1 -> 8 gb buffer. The index sign would be the least
     * problem)
     */
    wined3d_mutex_lock();
    IWineD3DDevice_SetBaseVertexIndex(This->WineD3DDevice, baseVertexIndex);
    hr = IWineD3DDevice_SetIndexBuffer(This->WineD3DDevice,
            ib ? ib->wineD3DIndexBuffer : NULL,
            ib ? ib->format : WINED3DFMT_UNKNOWN);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8** ppIndexData,UINT* pBaseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DBuffer *retIndexData = NULL;
    HRESULT rc = D3D_OK;

    TRACE("iface %p, buffer %p, base_vertex_index %p.\n", iface, ppIndexData, pBaseVertexIndex);

    if(ppIndexData == NULL){
        return D3DERR_INVALIDCALL;
    }

    /* The case from UINT to INT is safe because d3d8 will never set negative values */
    wined3d_mutex_lock();
    IWineD3DDevice_GetBaseVertexIndex(This->WineD3DDevice, (INT *) pBaseVertexIndex);
    rc = IWineD3DDevice_GetIndexBuffer(This->WineD3DDevice, &retIndexData);
    if (SUCCEEDED(rc) && retIndexData) {
        IWineD3DBuffer_GetParent(retIndexData, (IUnknown **)ppIndexData);
        IWineD3DBuffer_Release(retIndexData);
    } else {
        if (FAILED(rc)) FIXME("Call to GetIndices failed\n");
        *ppIndexData = NULL;
    }
    wined3d_mutex_unlock();

    return rc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreatePixelShader(IDirect3DDevice8 *iface,
        const DWORD *byte_code, DWORD *shader)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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

static HRESULT WINAPI IDirect3DDevice8Impl_SetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *shader;
    HRESULT hr;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    wined3d_mutex_lock();

    if (!pShader)
    {
        hr = IWineD3DDevice_SetPixelShader(This->WineD3DDevice, NULL);
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
    hr = IWineD3DDevice_SetPixelShader(This->WineD3DDevice, shader->wineD3DPixelShader);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DPixelShader *object;
    HRESULT hrc = D3D_OK;

    TRACE("iface %p, shader %p.\n", iface, ppShader);

    if (NULL == ppShader) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hrc = IWineD3DDevice_GetPixelShader(This->WineD3DDevice, &object);
    if (D3D_OK == hrc && NULL != object) {
        IDirect3DPixelShader8Impl *d3d8_shader;
        hrc = IWineD3DPixelShader_GetParent(object, (IUnknown **)&d3d8_shader);
        IWineD3DPixelShader_Release(object);
        *ppShader = d3d8_shader->handle;
        IDirect3DPixelShader8_Release((IDirect3DPixelShader8 *)d3d8_shader);
    } else {
        *ppShader = 0;
    }
    wined3d_mutex_unlock();

    TRACE("(%p) : returning %#x\n", This, *ppShader);

    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeletePixelShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *shader;
    IWineD3DPixelShader *cur = NULL;

    TRACE("iface %p, shader %#x.\n", iface, pShader);

    wined3d_mutex_lock();

    shader = d3d8_free_handle(&This->handle_table, pShader - (VS_HIGHESTFIXEDFXF + 1), D3D8_HANDLE_PS);
    if (!shader)
    {
        WARN("Invalid handle (%#x) passed.\n", pShader);
        wined3d_mutex_unlock();
        return D3DERR_INVALIDCALL;
    }

    IWineD3DDevice_GetPixelShader(This->WineD3DDevice, &cur);

    if (cur)
    {
        if (cur == shader->wineD3DPixelShader) IDirect3DDevice8_SetPixelShader(iface, 0);
        IWineD3DPixelShader_Release(cur);
    }

    wined3d_mutex_unlock();

    if (IUnknown_Release((IUnknown *)shader))
    {
        ERR("Shader %p has references left, this shouldn't happen.\n", shader);
    }

    return D3D_OK;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetPixelShaderConstantF(This->WineD3DDevice, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, register %u, data %p, count %u.\n",
            iface, Register, pConstantData, ConstantCount);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetPixelShaderConstantF(This->WineD3DDevice, Register, pConstantData, ConstantCount);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD pPixelShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
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

    hr = IWineD3DPixelShader_GetFunction(shader->wineD3DPixelShader, pData, pSizeOfData);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawRectPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, handle %#x, segment_count %p, patch_info %p.\n",
            iface, Handle, pNumSegs, pRectPatchInfo);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_DrawRectPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DRECTPATCH_INFO *)pRectPatchInfo);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawTriPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, handle %#x, segment_count %p, patch_info %p.\n",
            iface, Handle, pNumSegs, pTriPatchInfo);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_DrawTriPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DTRIPATCH_INFO *)pTriPatchInfo);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeletePatch(LPDIRECT3DDEVICE8 iface, UINT Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, handle %#x.\n", iface, Handle);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_DeletePatch(This->WineD3DDevice, Handle);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8* pStreamData,UINT Stride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, buffer %p, stride %u.\n",
            iface, StreamNumber, pStreamData, Stride);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetStreamSource(This->WineD3DDevice, StreamNumber,
                                        NULL == pStreamData ? NULL : ((IDirect3DVertexBuffer8Impl *)pStreamData)->wineD3DVertexBuffer,
                                        0/* Offset in bytes */, Stride);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8** pStream,UINT* pStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DBuffer *retStream = NULL;
    HRESULT rc = D3D_OK;

    TRACE("iface %p, stream_idx %u, buffer %p, stride %p.\n",
            iface, StreamNumber, pStream, pStride);

    if(pStream == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    rc = IWineD3DDevice_GetStreamSource(This->WineD3DDevice, StreamNumber, &retStream, 0 /* Offset in bytes */, pStride);
    if (rc == D3D_OK  && NULL != retStream) {
        IWineD3DBuffer_GetParent(retStream, (IUnknown **)pStream);
        IWineD3DBuffer_Release(retStream);
    }else{
        if (rc != D3D_OK){
            FIXME("Call to GetStreamSource failed %p\n",  pStride);
        }
        *pStream = NULL;
    }
    wined3d_mutex_unlock();

    return rc;
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

/* IWineD3DDeviceParent IUnknown methods */

static inline struct IDirect3DDevice8Impl *device_from_device_parent(IWineD3DDeviceParent *iface)
{
    return (struct IDirect3DDevice8Impl *)((char*)iface
            - FIELD_OFFSET(struct IDirect3DDevice8Impl, device_parent_vtbl));
}

static HRESULT STDMETHODCALLTYPE device_parent_QueryInterface(IWineD3DDeviceParent *iface, REFIID riid, void **object)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    return IDirect3DDevice8Impl_QueryInterface((IDirect3DDevice8 *)This, riid, object);
}

static ULONG STDMETHODCALLTYPE device_parent_AddRef(IWineD3DDeviceParent *iface)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    return IDirect3DDevice8Impl_AddRef((IDirect3DDevice8 *)This);
}

static ULONG STDMETHODCALLTYPE device_parent_Release(IWineD3DDeviceParent *iface)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    return IDirect3DDevice8Impl_Release((IDirect3DDevice8 *)This);
}

/* IWineD3DDeviceParent methods */

static void STDMETHODCALLTYPE device_parent_WineD3DDeviceCreated(IWineD3DDeviceParent *iface, IWineD3DDevice *device)
{
    TRACE("iface %p, device %p\n", iface, device);
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateSurface(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, DWORD usage,
        WINED3DPOOL pool, UINT level, WINED3DCUBEMAP_FACES face, IWineD3DSurface **surface)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    IDirect3DSurface8Impl *d3d_surface;
    BOOL lockable = TRUE;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, usage %#x,\n"
            "\tpool %#x, level %u, face %u, surface %p\n",
            iface, superior, width, height, format, usage, pool, level, face, surface);


    if (pool == WINED3DPOOL_DEFAULT && !(usage & WINED3DUSAGE_DYNAMIC)) lockable = FALSE;

    hr = IDirect3DDevice8Impl_CreateSurface((IDirect3DDevice8 *)This, width, height,
            d3dformat_from_wined3dformat(format), lockable, FALSE /* Discard */, level,
            (IDirect3DSurface8 **)&d3d_surface, usage, pool, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);
    if (FAILED(hr))
    {
        ERR("(%p) CreateSurface failed, returning %#x\n", iface, hr);
        return hr;
    }

    *surface = d3d_surface->wineD3DSurface;
    IWineD3DSurface_AddRef(*surface);

    d3d_surface->container = superior;
    IUnknown_Release(d3d_surface->parentDevice);
    d3d_surface->parentDevice = NULL;

    IDirect3DSurface8_Release((IDirect3DSurface8 *)d3d_surface);
    d3d_surface->forwardReference = superior;

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateRenderTarget(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, WINED3DMULTISAMPLE_TYPE multisample_type,
        DWORD multisample_quality, BOOL lockable, IWineD3DSurface **surface)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    IDirect3DSurface8Impl *d3d_surface;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, lockable %u, surface %p\n",
            iface, superior, width, height, format, multisample_type, multisample_quality, lockable, surface);

    hr = IDirect3DDevice8_CreateRenderTarget((IDirect3DDevice8 *)This, width, height,
            d3dformat_from_wined3dformat(format), multisample_type, lockable, (IDirect3DSurface8 **)&d3d_surface);
    if (FAILED(hr))
    {
        ERR("(%p) CreateRenderTarget failed, returning %#x\n", iface, hr);
        return hr;
    }

    *surface = d3d_surface->wineD3DSurface;
    IWineD3DSurface_AddRef(*surface);

    d3d_surface->container = (IUnknown *)This;
    /* Implicit surfaces are created with an refcount of 0 */
    IUnknown_Release((IUnknown *)d3d_surface);

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateDepthStencilSurface(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, WINED3DMULTISAMPLE_TYPE multisample_type,
        DWORD multisample_quality, BOOL discard, IWineD3DSurface **surface)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    IDirect3DSurface8Impl *d3d_surface;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, discard %u, surface %p\n",
            iface, superior, width, height, format, multisample_type, multisample_quality, discard, surface);

    hr = IDirect3DDevice8_CreateDepthStencilSurface((IDirect3DDevice8 *)This, width, height,
            d3dformat_from_wined3dformat(format), multisample_type, (IDirect3DSurface8 **)&d3d_surface);
    if (FAILED(hr))
    {
        ERR("(%p) CreateDepthStencilSurface failed, returning %#x\n", iface, hr);
        return hr;
    }

    *surface = d3d_surface->wineD3DSurface;
    IWineD3DSurface_AddRef(*surface);

    d3d_surface->container = (IUnknown *)This;
    /* Implicit surfaces are created with an refcount of 0 */
    IUnknown_Release((IUnknown *)d3d_surface);

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateVolume(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, UINT depth, WINED3DFORMAT format,
        WINED3DPOOL pool, DWORD usage, IWineD3DVolume **volume)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    IDirect3DVolume8Impl *object;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, depth %u, format %#x, pool %#x, usage %#x, volume %p\n",
            iface, superior, width, height, depth, format, pool, usage, volume);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        FIXME("Allocation of memory failed\n");
        *volume = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    hr = volume_init(object, This, width, height, depth, usage, format, pool);
    if (FAILED(hr))
    {
        WARN("Failed to initialize volume, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    *volume = object->wineD3DVolume;
    IWineD3DVolume_AddRef(*volume);
    IDirect3DVolume8_Release((IDirect3DVolume8 *)object);

    object->container = superior;
    object->forwardReference = superior;

    TRACE("(%p) Created volume %p\n", iface, object);

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateSwapChain(IWineD3DDeviceParent *iface,
        WINED3DPRESENT_PARAMETERS *present_parameters, IWineD3DSwapChain **swapchain)
{
    struct IDirect3DDevice8Impl *This = device_from_device_parent(iface);
    IDirect3DSwapChain8Impl *d3d_swapchain;
    D3DPRESENT_PARAMETERS local_parameters;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p, swapchain %p\n", iface, present_parameters, swapchain);

    /* Copy the presentation parameters */
    local_parameters.BackBufferWidth = present_parameters->BackBufferWidth;
    local_parameters.BackBufferHeight = present_parameters->BackBufferHeight;
    local_parameters.BackBufferFormat = d3dformat_from_wined3dformat(present_parameters->BackBufferFormat);
    local_parameters.BackBufferCount = present_parameters->BackBufferCount;
    local_parameters.MultiSampleType = present_parameters->MultiSampleType;
    local_parameters.SwapEffect = present_parameters->SwapEffect;
    local_parameters.hDeviceWindow = present_parameters->hDeviceWindow;
    local_parameters.Windowed = present_parameters->Windowed;
    local_parameters.EnableAutoDepthStencil = present_parameters->EnableAutoDepthStencil;
    local_parameters.AutoDepthStencilFormat = d3dformat_from_wined3dformat(present_parameters->AutoDepthStencilFormat);
    local_parameters.Flags = present_parameters->Flags;
    local_parameters.FullScreen_RefreshRateInHz = present_parameters->FullScreen_RefreshRateInHz;
    local_parameters.FullScreen_PresentationInterval = present_parameters->PresentationInterval;

    hr = IDirect3DDevice8_CreateAdditionalSwapChain((IDirect3DDevice8 *)This,
            &local_parameters, (IDirect3DSwapChain8 **)&d3d_swapchain);
    if (FAILED(hr))
    {
        ERR("(%p) CreateAdditionalSwapChain failed, returning %#x\n", iface, hr);
        *swapchain = NULL;
        return hr;
    }

    *swapchain = d3d_swapchain->wineD3DSwapChain;
    IUnknown_Release(d3d_swapchain->parentDevice);
    d3d_swapchain->parentDevice = NULL;

    /* Copy back the presentation parameters */
    present_parameters->BackBufferWidth = local_parameters.BackBufferWidth;
    present_parameters->BackBufferHeight = local_parameters.BackBufferHeight;
    present_parameters->BackBufferFormat = wined3dformat_from_d3dformat(local_parameters.BackBufferFormat);
    present_parameters->BackBufferCount = local_parameters.BackBufferCount;
    present_parameters->MultiSampleType = local_parameters.MultiSampleType;
    present_parameters->SwapEffect = local_parameters.SwapEffect;
    present_parameters->hDeviceWindow = local_parameters.hDeviceWindow;
    present_parameters->Windowed = local_parameters.Windowed;
    present_parameters->EnableAutoDepthStencil = local_parameters.EnableAutoDepthStencil;
    present_parameters->AutoDepthStencilFormat = wined3dformat_from_d3dformat(local_parameters.AutoDepthStencilFormat);
    present_parameters->Flags = local_parameters.Flags;
    present_parameters->FullScreen_RefreshRateInHz = local_parameters.FullScreen_RefreshRateInHz;
    present_parameters->PresentationInterval = local_parameters.FullScreen_PresentationInterval;

    return hr;
}

static const IWineD3DDeviceParentVtbl d3d8_wined3d_device_parent_vtbl =
{
    /* IUnknown methods */
    device_parent_QueryInterface,
    device_parent_AddRef,
    device_parent_Release,
    /* IWineD3DDeviceParent methods */
    device_parent_WineD3DDeviceCreated,
    device_parent_CreateSurface,
    device_parent_CreateRenderTarget,
    device_parent_CreateDepthStencilSurface,
    device_parent_CreateVolume,
    device_parent_CreateSwapChain,
};

static void setup_fpu(void)
{
    WORD cw;

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    __asm__ volatile ("fnstcw %0" : "=m" (cw));
    cw = (cw & ~0xf3f) | 0x3f;
    __asm__ volatile ("fldcw %0" : : "m" (cw));
#else
    FIXME("FPU setup not implemented for this platform.\n");
#endif
}

HRESULT device_init(IDirect3DDevice8Impl *device, IWineD3D *wined3d, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters)
{
    WINED3DPRESENT_PARAMETERS wined3d_parameters;
    HRESULT hr;

    device->lpVtbl = &Direct3DDevice8_Vtbl;
    device->device_parent_vtbl = &d3d8_wined3d_device_parent_vtbl;
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
    hr = IWineD3D_CreateDevice(wined3d, adapter, device_type, focus_window, flags, (IUnknown *)device,
            (IWineD3DDeviceParent *)&device->device_parent_vtbl, &device->WineD3DDevice);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d device, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
        return hr;
    }

    if (!parameters->Windowed)
    {
        if (!focus_window) focus_window = parameters->hDeviceWindow;
        if (FAILED(hr = IWineD3DDevice_AcquireFocusWindow(device->WineD3DDevice, focus_window)))
        {
            ERR("Failed to acquire focus window, hr %#x.\n", hr);
            IWineD3DDevice_Release(device->WineD3DDevice);
            wined3d_mutex_unlock();
            HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
            return hr;
        }
    }

    if (flags & D3DCREATE_MULTITHREADED) IWineD3DDevice_SetMultithreaded(device->WineD3DDevice);

    wined3d_parameters.BackBufferWidth = parameters->BackBufferWidth;
    wined3d_parameters.BackBufferHeight = parameters->BackBufferHeight;
    wined3d_parameters.BackBufferFormat = wined3dformat_from_d3dformat(parameters->BackBufferFormat);
    wined3d_parameters.BackBufferCount = parameters->BackBufferCount;
    wined3d_parameters.MultiSampleType = parameters->MultiSampleType;
    wined3d_parameters.MultiSampleQuality = 0; /* d3d9 only */
    wined3d_parameters.SwapEffect = parameters->SwapEffect;
    wined3d_parameters.hDeviceWindow = parameters->hDeviceWindow;
    wined3d_parameters.Windowed = parameters->Windowed;
    wined3d_parameters.EnableAutoDepthStencil = parameters->EnableAutoDepthStencil;
    wined3d_parameters.AutoDepthStencilFormat = wined3dformat_from_d3dformat(parameters->AutoDepthStencilFormat);
    wined3d_parameters.Flags = parameters->Flags;
    wined3d_parameters.FullScreen_RefreshRateInHz = parameters->FullScreen_RefreshRateInHz;
    wined3d_parameters.PresentationInterval = parameters->FullScreen_PresentationInterval;
    wined3d_parameters.AutoRestoreDisplayMode = TRUE;

    hr = IWineD3DDevice_Init3D(device->WineD3DDevice, &wined3d_parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize 3D, hr %#x.\n", hr);
        IWineD3DDevice_ReleaseFocusWindow(device->WineD3DDevice);
        IWineD3DDevice_Release(device->WineD3DDevice);
        wined3d_mutex_unlock();
        HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
        return hr;
    }

    hr = IWineD3DDevice_SetRenderState(device->WineD3DDevice, WINED3DRS_POINTSIZE_MIN, 0);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
        ERR("Failed to set minimum pointsize, hr %#x.\n", hr);
        goto err;
    }

    parameters->BackBufferWidth = wined3d_parameters.BackBufferWidth;
    parameters->BackBufferHeight = wined3d_parameters.BackBufferHeight;
    parameters->BackBufferFormat = d3dformat_from_wined3dformat(wined3d_parameters.BackBufferFormat);
    parameters->BackBufferCount = wined3d_parameters.BackBufferCount;
    parameters->MultiSampleType = wined3d_parameters.MultiSampleType;
    parameters->SwapEffect = wined3d_parameters.SwapEffect;
    parameters->hDeviceWindow = wined3d_parameters.hDeviceWindow;
    parameters->Windowed = wined3d_parameters.Windowed;
    parameters->EnableAutoDepthStencil = wined3d_parameters.EnableAutoDepthStencil;
    parameters->AutoDepthStencilFormat = d3dformat_from_wined3dformat(wined3d_parameters.AutoDepthStencilFormat);
    parameters->Flags = wined3d_parameters.Flags;
    parameters->FullScreen_RefreshRateInHz = wined3d_parameters.FullScreen_RefreshRateInHz;
    parameters->FullScreen_PresentationInterval = wined3d_parameters.PresentationInterval;

    device->declArraySize = 16;
    device->decls = HeapAlloc(GetProcessHeap(), 0, device->declArraySize * sizeof(*device->decls));
    if (!device->decls)
    {
        ERR("Failed to allocate FVF vertex delcaration map memory.\n");
        hr = E_OUTOFMEMORY;
        goto err;
    }

    return D3D_OK;

err:
    wined3d_mutex_lock();
    IWineD3DDevice_Uninit3D(device->WineD3DDevice, D3D8CB_DestroySwapChain);
    IWineD3DDevice_ReleaseFocusWindow(device->WineD3DDevice);
    IWineD3DDevice_Release(device->WineD3DDevice);
    wined3d_mutex_unlock();
    HeapFree(GetProcessHeap(), 0, device->handle_table.entries);
    return hr;
}
