/*
 * IDirect3DDevice9 implementation
 *
 * Copyright 2002-2005 Jason Edmeades
 * Copyright 2002-2005 Raphael Junqueira
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
#include "d3d9_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(d3d9);

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
        case WINED3DFMT_R8G8B8A8_UNORM: return D3DFMT_A8B8G8R8;
        case WINED3DFMT_R8G8B8X8_UNORM: return D3DFMT_X8B8G8R8;
        case WINED3DFMT_R16G16_UNORM: return D3DFMT_G16R16;
        case WINED3DFMT_B10G10R10A2_UNORM: return D3DFMT_A2R10G10B10;
        case WINED3DFMT_R16G16B16A16_UNORM: return D3DFMT_A16B16G16R16;
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
        case WINED3DFMT_R10G10B10_SNORM_A2_UNORM: return D3DFMT_A2W10V10U10;
        case WINED3DFMT_D16_LOCKABLE: return D3DFMT_D16_LOCKABLE;
        case WINED3DFMT_D32_UNORM: return D3DFMT_D32;
        case WINED3DFMT_S1_UINT_D15_UNORM: return D3DFMT_D15S1;
        case WINED3DFMT_D24_UNORM_S8_UINT: return D3DFMT_D24S8;
        case WINED3DFMT_X8D24_UNORM: return D3DFMT_D24X8;
        case WINED3DFMT_S4X4_UINT_D24_UNORM: return D3DFMT_D24X4S4;
        case WINED3DFMT_D16_UNORM: return D3DFMT_D16;
        case WINED3DFMT_L16_UNORM: return D3DFMT_L16;
        case WINED3DFMT_D32_FLOAT: return D3DFMT_D32F_LOCKABLE;
        case WINED3DFMT_S8_UINT_D24_FLOAT: return D3DFMT_D24FS8;
        case WINED3DFMT_VERTEXDATA: return D3DFMT_VERTEXDATA;
        case WINED3DFMT_R16_UINT: return D3DFMT_INDEX16;
        case WINED3DFMT_R32_UINT: return D3DFMT_INDEX32;
        case WINED3DFMT_R16G16B16A16_SNORM: return D3DFMT_Q16W16V16U16;
        case WINED3DFMT_R16_FLOAT: return D3DFMT_R16F;
        case WINED3DFMT_R16G16_FLOAT: return D3DFMT_G16R16F;
        case WINED3DFMT_R16G16B16A16_FLOAT: return D3DFMT_A16B16G16R16F;
        case WINED3DFMT_R32_FLOAT: return D3DFMT_R32F;
        case WINED3DFMT_R32G32_FLOAT: return D3DFMT_G32R32F;
        case WINED3DFMT_R32G32B32A32_FLOAT: return D3DFMT_A32B32G32R32F;
        case WINED3DFMT_R8G8_SNORM_Cx: return D3DFMT_CxV8U8;
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
        case D3DFMT_A8B8G8R8: return WINED3DFMT_R8G8B8A8_UNORM;
        case D3DFMT_X8B8G8R8: return WINED3DFMT_R8G8B8X8_UNORM;
        case D3DFMT_G16R16: return WINED3DFMT_R16G16_UNORM;
        case D3DFMT_A2R10G10B10: return WINED3DFMT_B10G10R10A2_UNORM;
        case D3DFMT_A16B16G16R16: return WINED3DFMT_R16G16B16A16_UNORM;
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
        case D3DFMT_A2W10V10U10: return WINED3DFMT_R10G10B10_SNORM_A2_UNORM;
        case D3DFMT_D16_LOCKABLE: return WINED3DFMT_D16_LOCKABLE;
        case D3DFMT_D32: return WINED3DFMT_D32_UNORM;
        case D3DFMT_D15S1: return WINED3DFMT_S1_UINT_D15_UNORM;
        case D3DFMT_D24S8: return WINED3DFMT_D24_UNORM_S8_UINT;
        case D3DFMT_D24X8: return WINED3DFMT_X8D24_UNORM;
        case D3DFMT_D24X4S4: return WINED3DFMT_S4X4_UINT_D24_UNORM;
        case D3DFMT_D16: return WINED3DFMT_D16_UNORM;
        case D3DFMT_L16: return WINED3DFMT_L16_UNORM;
        case D3DFMT_D32F_LOCKABLE: return WINED3DFMT_D32_FLOAT;
        case D3DFMT_D24FS8: return WINED3DFMT_S8_UINT_D24_FLOAT;
        case D3DFMT_VERTEXDATA: return WINED3DFMT_VERTEXDATA;
        case D3DFMT_INDEX16: return WINED3DFMT_R16_UINT;
        case D3DFMT_INDEX32: return WINED3DFMT_R32_UINT;
        case D3DFMT_Q16W16V16U16: return WINED3DFMT_R16G16B16A16_SNORM;
        case D3DFMT_R16F: return WINED3DFMT_R16_FLOAT;
        case D3DFMT_G16R16F: return WINED3DFMT_R16G16_FLOAT;
        case D3DFMT_A16B16G16R16F: return WINED3DFMT_R16G16B16A16_FLOAT;
        case D3DFMT_R32F: return WINED3DFMT_R32_FLOAT;
        case D3DFMT_G32R32F: return WINED3DFMT_R32G32_FLOAT;
        case D3DFMT_A32B32G32R32F: return WINED3DFMT_R32G32B32A32_FLOAT;
        case D3DFMT_CxV8U8: return WINED3DFMT_R8G8_SNORM_Cx;
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

static ULONG WINAPI D3D9CB_DestroySwapChain(IWineD3DSwapChain *swapchain)
{
    IDirect3DSwapChain9Impl *parent;

    TRACE("swapchain %p.\n", swapchain);

    IWineD3DSwapChain_GetParent(swapchain, (IUnknown **)&parent);
    parent->isImplicit = FALSE;
    return IDirect3DSwapChain9_Release((IDirect3DSwapChain9 *)parent);
}

/* IDirect3D IUnknown parts follow: */
static HRESULT WINAPI IDirect3DDevice9Impl_QueryInterface(LPDIRECT3DDEVICE9EX iface, REFIID riid, LPVOID* ppobj) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3D9 *d3d;
    IDirect3D9Impl *d3dimpl;

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3DDevice9)) {
        IDirect3DDevice9Ex_AddRef(iface);
        *ppobj = This;
        TRACE("Returning IDirect3DDevice9 interface at %p\n", *ppobj);
        return S_OK;
    } else if(IsEqualGUID(riid, &IID_IDirect3DDevice9Ex)) {
        /* Find out if the creating d3d9 interface was created with Direct3DCreate9Ex.
         * It doesn't matter with which function the device was created.
         */
        IDirect3DDevice9_GetDirect3D(iface, &d3d);
        d3dimpl = (IDirect3D9Impl *) d3d;

        if(d3dimpl->extended) {
            *ppobj = iface;
            IDirect3DDevice9Ex_AddRef((IDirect3DDevice9Ex *) *ppobj);
            IDirect3D9_Release(d3d);
            TRACE("Returning IDirect3DDevice9Ex interface at %p\n", *ppobj);
            return S_OK;
        } else {
            WARN("IDirect3D9 instance wasn't created with CreateDirect3D9Ex, returning E_NOINTERFACE\n");
            IDirect3D9_Release(d3d);
            *ppobj = NULL;
            return E_NOINTERFACE;
        }
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

static ULONG WINAPI IDirect3DDevice9Impl_AddRef(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI DECLSPEC_HOTPATCH IDirect3DDevice9Impl_Release(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    ULONG ref;

    if (This->inDestruction) return 0;
    ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
      unsigned i;
      This->inDestruction = TRUE;

      wined3d_mutex_lock();
      for(i = 0; i < This->numConvertedDecls; i++) {
          /* Unless Wine is buggy or the app has a bug the refcount will be 0, because decls hold a reference to the
           * device
           */
          IDirect3DVertexDeclaration9Impl_Destroy(This->convertedDecls[i]);
      }
      HeapFree(GetProcessHeap(), 0, This->convertedDecls);

      IWineD3DDevice_Uninit3D(This->WineD3DDevice, D3D9CB_DestroySwapChain);
      IWineD3DDevice_Release(This->WineD3DDevice);
      wined3d_mutex_unlock();

      HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
static HRESULT WINAPI IDirect3DDevice9Impl_TestCooperativeLevel(IDirect3DDevice9Ex *iface)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;

    TRACE("iface %p.\n", iface);

    if (This->notreset)
    {
        TRACE("D3D9 device is marked not reset.\n");
        return D3DERR_DEVICENOTRESET;
    }

    return D3D_OK;
}

static UINT     WINAPI  IDirect3DDevice9Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetAvailableTextureMem(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_EvictManagedResources(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_EvictManagedResources(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetDirect3D(LPDIRECT3DDEVICE9EX iface, IDirect3D9** ppD3D9) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3D* pWineD3D;

    TRACE("iface %p, d3d9 %p.\n", iface, ppD3D9);

    if (NULL == ppD3D9) {
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetDirect3D(This->WineD3DDevice, &pWineD3D);
    if (hr == D3D_OK && pWineD3D != NULL)
    {
        IWineD3D_GetParent(pWineD3D,(IUnknown **)ppD3D9);
        IWineD3D_Release(pWineD3D);
    } else {
        FIXME("Call to IWineD3DDevice_GetDirect3D failed\n");
        *ppD3D9 = NULL;
    }
    TRACE("(%p) returning %p\n", This, *ppD3D9);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetDeviceCaps(LPDIRECT3DDEVICE9EX iface, D3DCAPS9* pCaps) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
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

    memset(pCaps, 0, sizeof(*pCaps));

    wined3d_mutex_lock();
    hrc = IWineD3DDevice_GetDeviceCaps(This->WineD3DDevice, pWineCaps);
    wined3d_mutex_unlock();

    WINECAPSTOD3D9CAPS(pCaps, pWineCaps)
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    /* Some functionality is implemented in d3d9.dll, not wined3d.dll. Add the needed caps */
    pCaps->DevCaps2 |= D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;

    filter_caps(pCaps);

    TRACE("Returning %p %p\n", This, pCaps);
    return hrc;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetDisplayMode(LPDIRECT3DDEVICE9EX iface, UINT iSwapChain, D3DDISPLAYMODE* pMode) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, swapchain %u, mode %p.\n", iface, iSwapChain, pMode);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetDisplayMode(This->WineD3DDevice, iSwapChain, (WINED3DDISPLAYMODE *) pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetCreationParameters(LPDIRECT3DDEVICE9EX iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, parameters %p.\n", iface, pParameters);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetCreationParameters(This->WineD3DDevice, (WINED3DDEVICE_CREATION_PARAMETERS *) pParameters);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetCursorProperties(LPDIRECT3DDEVICE9EX iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *pSurface = (IDirect3DSurface9Impl*)pCursorBitmap;
    HRESULT hr;

    TRACE("iface %p, hotspot_x %u, hotspot_y %u, bitmap %p.\n",
            iface, XHotSpot, YHotSpot, pCursorBitmap);

    if(!pCursorBitmap) {
        WARN("No cursor bitmap, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetCursorProperties(This->WineD3DDevice, XHotSpot, YHotSpot, pSurface->wineD3DSurface);
    wined3d_mutex_unlock();

    return hr;
}

static void     WINAPI  IDirect3DDevice9Impl_SetCursorPosition(LPDIRECT3DDEVICE9EX iface, int XScreenSpace, int YScreenSpace, DWORD Flags) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;

    TRACE("iface %p, x %u, y %u, flags %#x.\n", iface, XScreenSpace, YScreenSpace, Flags);

    wined3d_mutex_lock();
    IWineD3DDevice_SetCursorPosition(This->WineD3DDevice, XScreenSpace, YScreenSpace, Flags);
    wined3d_mutex_unlock();
}

static BOOL     WINAPI  IDirect3DDevice9Impl_ShowCursor(LPDIRECT3DDEVICE9EX iface, BOOL bShow) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    BOOL ret;

    TRACE("iface %p, show %#x.\n", iface, bShow);

    wined3d_mutex_lock();
    ret = IWineD3DDevice_ShowCursor(This->WineD3DDevice, bShow);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDevice9Impl_CreateAdditionalSwapChain(IDirect3DDevice9Ex *iface,
        D3DPRESENT_PARAMETERS *present_parameters, IDirect3DSwapChain9 **swapchain)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSwapChain9Impl *object;
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
    *swapchain = (IDirect3DSwapChain9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI reset_enum_callback(IWineD3DResource *resource, void *data) {
    BOOL *resources_ok = data;
    D3DRESOURCETYPE type;
    HRESULT ret = S_OK;
    WINED3DSURFACE_DESC surface_desc;
    WINED3DVOLUME_DESC volume_desc;
    D3DINDEXBUFFER_DESC index_desc;
    D3DVERTEXBUFFER_DESC vertex_desc;
    WINED3DPOOL pool;
    IDirect3DResource9 *parent;

    IWineD3DResource_GetParent(resource, (IUnknown **) &parent);
    type = IDirect3DResource9_GetType(parent);
    switch(type) {
        case D3DRTYPE_SURFACE:
            IWineD3DSurface_GetDesc((IWineD3DSurface *) resource, &surface_desc);
            pool = surface_desc.pool;
            break;

        case D3DRTYPE_VOLUME:
            IWineD3DVolume_GetDesc((IWineD3DVolume *) resource, &volume_desc);
            pool = volume_desc.Pool;
            break;

        case D3DRTYPE_INDEXBUFFER:
            IDirect3DIndexBuffer9_GetDesc((IDirect3DIndexBuffer9 *) parent, &index_desc);
            pool = index_desc.Pool;
            break;

        case D3DRTYPE_VERTEXBUFFER:
            IDirect3DVertexBuffer9_GetDesc((IDirect3DVertexBuffer9 *)parent, &vertex_desc);
            pool = vertex_desc.Pool;
            break;

        /* No need to check for textures. If there is a D3DPOOL_DEFAULT texture, there
         * is a D3DPOOL_DEFAULT surface or volume as well
         */
        default:
            pool = WINED3DPOOL_SCRATCH; /* a harmless pool */
            break;
    }

    if(pool == WINED3DPOOL_DEFAULT) {
        if(IUnknown_Release(parent) == 0) {
            TRACE("Parent %p is an implicit resource with ref 0\n", parent);
        } else {
            WARN("Resource %p(wineD3D %p) with pool D3DPOOL_DEFAULT blocks the Reset call\n", parent, resource);
            ret = S_FALSE;
            *resources_ok = FALSE;
        }
    } else {
        IUnknown_Release(parent);
    }
    IWineD3DResource_Release(resource);

    return ret;
}

static HRESULT  WINAPI DECLSPEC_HOTPATCH IDirect3DDevice9Impl_Reset(LPDIRECT3DDEVICE9EX iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    WINED3DPRESENT_PARAMETERS localParameters;
    HRESULT hr;
    BOOL resources_ok = TRUE;
    UINT i;

    TRACE("iface %p, present_parameters %p.\n", iface, pPresentationParameters);

    /* Reset states that hold a COM object. WineD3D holds an internal reference to set objects, because
     * such objects can still be used for rendering after their external d3d9 object has been destroyed.
     * These objects must not be enumerated. Unsetting them tells WineD3D that the application will not
     * make use of the hidden reference and destroys the objects.
     *
     * Unsetting them is no problem, because the states are supposed to be reset anyway. If the validation
     * below fails, the device is considered "lost", and _Reset and _Release are the only allowed calls
     */
    wined3d_mutex_lock();
    IWineD3DDevice_SetIndexBuffer(This->WineD3DDevice, NULL, WINED3DFMT_UNKNOWN);
    for(i = 0; i < 16; i++) {
        IWineD3DDevice_SetStreamSource(This->WineD3DDevice, i, NULL, 0, 0);
    }
    for(i = 0; i < 16; i++) {
        IWineD3DDevice_SetTexture(This->WineD3DDevice, i, NULL);
    }

    IWineD3DDevice_EnumResources(This->WineD3DDevice, reset_enum_callback, &resources_ok);
    if(!resources_ok) {
        WARN("The application is holding D3DPOOL_DEFAULT resources, rejecting reset\n");
        This->notreset = TRUE;
        wined3d_mutex_unlock();

        return WINED3DERR_INVALIDCALL;
    }

    localParameters.BackBufferWidth                     = pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight                    = pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat                    = wined3dformat_from_d3dformat(pPresentationParameters->BackBufferFormat);
    localParameters.BackBufferCount                     = pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                     = pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality                  = pPresentationParameters->MultiSampleQuality;
    localParameters.SwapEffect                          = pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                       = pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                            = pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil              = pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat              = wined3dformat_from_d3dformat(pPresentationParameters->AutoDepthStencilFormat);
    localParameters.Flags                               = pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz          = pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval                = pPresentationParameters->PresentationInterval;
    localParameters.AutoRestoreDisplayMode              = TRUE;

    hr = IWineD3DDevice_Reset(This->WineD3DDevice, &localParameters);
    if(FAILED(hr)) {
        This->notreset = TRUE;

        pPresentationParameters->BackBufferWidth            = localParameters.BackBufferWidth;
        pPresentationParameters->BackBufferHeight           = localParameters.BackBufferHeight;
        pPresentationParameters->BackBufferFormat           = d3dformat_from_wined3dformat(localParameters.BackBufferFormat);
        pPresentationParameters->BackBufferCount            = localParameters.BackBufferCount;
        pPresentationParameters->MultiSampleType            = localParameters.MultiSampleType;
        pPresentationParameters->MultiSampleQuality         = localParameters.MultiSampleQuality;
        pPresentationParameters->SwapEffect                 = localParameters.SwapEffect;
        pPresentationParameters->hDeviceWindow              = localParameters.hDeviceWindow;
        pPresentationParameters->Windowed                   = localParameters.Windowed;
        pPresentationParameters->EnableAutoDepthStencil     = localParameters.EnableAutoDepthStencil;
        pPresentationParameters->AutoDepthStencilFormat     = d3dformat_from_wined3dformat(localParameters.AutoDepthStencilFormat);
        pPresentationParameters->Flags                      = localParameters.Flags;
        pPresentationParameters->FullScreen_RefreshRateInHz = localParameters.FullScreen_RefreshRateInHz;
        pPresentationParameters->PresentationInterval       = localParameters.PresentationInterval;
    } else {
        This->notreset = FALSE;
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI DECLSPEC_HOTPATCH IDirect3DDevice9Impl_Present(LPDIRECT3DDEVICE9EX iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA*
 pDirtyRegion) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, src_rect %p, dst_rect %p, dst_window_override %p, dirty_region %p.\n",
            iface, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_Present(This->WineD3DDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    wined3d_mutex_unlock();

    return hr;
 }

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetBackBuffer(LPDIRECT3DDEVICE9EX iface, UINT iSwapChain, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9 ** ppBackBuffer) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DSurface *retSurface = NULL;
    HRESULT rc = D3D_OK;

    TRACE("iface %p, swapchain %u, backbuffer_idx %u, backbuffer_type %#x, backbuffer %p.\n",
            iface, iSwapChain, BackBuffer, Type, ppBackBuffer);

    wined3d_mutex_lock();
    rc = IWineD3DDevice_GetBackBuffer(This->WineD3DDevice, iSwapChain, BackBuffer, (WINED3DBACKBUFFER_TYPE) Type, &retSurface);
    if (rc == D3D_OK && NULL != retSurface && NULL != ppBackBuffer) {
        IWineD3DSurface_GetParent(retSurface, (IUnknown **)ppBackBuffer);
        IWineD3DSurface_Release(retSurface);
    }
    wined3d_mutex_unlock();

    return rc;
}
static HRESULT  WINAPI  IDirect3DDevice9Impl_GetRasterStatus(LPDIRECT3DDEVICE9EX iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, swapchain %u, raster_status %p.\n", iface, iSwapChain, pRasterStatus);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetRasterStatus(This->WineD3DDevice, iSwapChain, (WINED3DRASTER_STATUS *) pRasterStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetDialogBoxMode(LPDIRECT3DDEVICE9EX iface, BOOL bEnableDialogs) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, enable %#x.\n", iface, bEnableDialogs);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetDialogBoxMode(This->WineD3DDevice, bEnableDialogs);
    wined3d_mutex_unlock();

    return hr;
}

static void WINAPI IDirect3DDevice9Impl_SetGammaRamp(IDirect3DDevice9Ex *iface, UINT iSwapChain,
        DWORD Flags, const D3DGAMMARAMP *pRamp)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;

    TRACE("iface %p, swapchain %u, flags %#x, ramp %p.\n", iface, iSwapChain, Flags, pRamp);

    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    wined3d_mutex_lock();
    IWineD3DDevice_SetGammaRamp(This->WineD3DDevice, iSwapChain, Flags, (CONST WINED3DGAMMARAMP *)pRamp);
    wined3d_mutex_unlock();
}

static void WINAPI IDirect3DDevice9Impl_GetGammaRamp(LPDIRECT3DDEVICE9EX iface, UINT iSwapChain, D3DGAMMARAMP* pRamp) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;

    TRACE("iface %p, swapchain %u, ramp %p.\n", iface, iSwapChain, pRamp);

    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    wined3d_mutex_lock();
    IWineD3DDevice_GetGammaRamp(This->WineD3DDevice, iSwapChain, (WINED3DGAMMARAMP *) pRamp);
    wined3d_mutex_unlock();
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateTexture(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DTexture9 **texture, HANDLE *shared_handle)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DTexture9Impl *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, levels %u, usage %#x, format %#x, pool %#x, texture %p, shared_handle %p.\n",
            iface, width, height, levels, usage, format, pool, texture, shared_handle);

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
    *texture = (IDirect3DTexture9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVolumeTexture(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, UINT depth, UINT levels, DWORD usage, D3DFORMAT format,
        D3DPOOL pool, IDirect3DVolumeTexture9 **texture, HANDLE *shared_handle)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVolumeTexture9Impl *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, depth %u, levels %u\n",
            iface, width, height, depth, levels);
    TRACE("usage %#x, format %#x, pool %#x, texture %p, shared_handle %p.\n",
            usage, format, pool, texture, shared_handle);

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
    *texture = (IDirect3DVolumeTexture9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateCubeTexture(IDirect3DDevice9Ex *iface,
        UINT edge_length, UINT levels, DWORD usage, D3DFORMAT format, D3DPOOL pool,
        IDirect3DCubeTexture9 **texture, HANDLE *shared_handle)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DCubeTexture9Impl *object;
    HRESULT hr;

    TRACE("iface %p, edge_length %u, levels %u, usage %#x, format %#x, pool %#x, texture %p, shared_handle %p.\n",
            iface, edge_length, levels, usage, format, pool, texture, shared_handle);

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
    *texture = (IDirect3DCubeTexture9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexBuffer(IDirect3DDevice9Ex *iface, UINT size, DWORD usage,
        DWORD fvf, D3DPOOL pool, IDirect3DVertexBuffer9 **buffer, HANDLE *shared_handle)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexBuffer9Impl *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#x, fvf %#x, pool %#x, buffer %p, shared_handle %p.\n",
            iface, size, usage, fvf, pool, buffer, shared_handle);

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
    *buffer = (IDirect3DVertexBuffer9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateIndexBuffer(IDirect3DDevice9Ex *iface, UINT size, DWORD usage,
        D3DFORMAT format, D3DPOOL pool, IDirect3DIndexBuffer9 **buffer, HANDLE *shared_handle)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DIndexBuffer9Impl *object;
    HRESULT hr;

    TRACE("iface %p, size %u, usage %#x, format %#x, pool %#x, buffer %p, shared_handle %p.\n",
            iface, size, usage, format, pool, buffer, shared_handle);

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
    *buffer = (IDirect3DIndexBuffer9 *)object;

    return D3D_OK;
}

static HRESULT IDirect3DDevice9Impl_CreateSurface(LPDIRECT3DDEVICE9EX iface, UINT Width, UINT Height,
        D3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IDirect3DSurface9 **ppSurface,
        UINT Usage, D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *object;
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, lockable %#x, discard %#x, level %u, surface %p.\n"
            "usage %#x, pool %#x, multisample_type %#x, multisample_quality %u.\n",
            iface, Width, Height, Format, Lockable, Discard, Level, ppSurface,
            Usage, Pool, MultiSample, MultisampleQuality);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface9Impl));
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
    *ppSurface = (IDirect3DSurface9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateRenderTarget(IDirect3DDevice9Ex *iface, UINT Width, UINT Height,
        D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable,
        IDirect3DSurface9 **ppSurface, HANDLE *pSharedHandle)
{
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %u.\n"
            "lockable %#x, surface %p, shared_handle %p.\n",
            iface, Width, Height, Format, MultiSample, MultisampleQuality,
            Lockable, ppSurface, pSharedHandle);

    hr = IDirect3DDevice9Impl_CreateSurface(iface, Width, Height, Format, Lockable, FALSE /* Discard */,
            0 /* Level */, ppSurface, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT, MultiSample, MultisampleQuality);

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE9EX iface, UINT Width, UINT Height,
                                                                D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample,
                                                                DWORD MultisampleQuality, BOOL Discard,
                                                                IDirect3DSurface9 **ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %u.\n"
            "discard %#x, surface %p, shared_handle %p.\n",
            iface, Width, Height, Format, MultiSample, MultisampleQuality,
            Discard, ppSurface, pSharedHandle);

    hr = IDirect3DDevice9Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Lockable */, Discard,
            0 /* Level */, ppSurface, D3DUSAGE_DEPTHSTENCIL, D3DPOOL_DEFAULT, MultiSample, MultisampleQuality);

    return hr;
}


static HRESULT  WINAPI  IDirect3DDevice9Impl_UpdateSurface(LPDIRECT3DDEVICE9EX iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, src_surface %p, src_rect %p, dst_surface %p, dst_point %p.\n",
            iface, pSourceSurface, pSourceRect, pDestinationSurface, pDestPoint);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_UpdateSurface(This->WineD3DDevice, ((IDirect3DSurface9Impl *)pSourceSurface)->wineD3DSurface, pSourceRect, ((IDirect3DSurface9Impl *)pDestinationSurface)->wineD3DSurface, pDestPoint);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_UpdateTexture(LPDIRECT3DDEVICE9EX iface, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, src_texture %p, dst_texture %p.\n", iface, pSourceTexture, pDestinationTexture);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_UpdateTexture(This->WineD3DDevice,  ((IDirect3DBaseTexture9Impl *)pSourceTexture)->wineD3DBaseTexture, ((IDirect3DBaseTexture9Impl *)pDestinationTexture)->wineD3DBaseTexture);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetRenderTargetData(IDirect3DDevice9Ex *iface,
        IDirect3DSurface9 *pRenderTarget, IDirect3DSurface9 *pDestSurface)
{
    IDirect3DSurface9Impl *renderTarget = (IDirect3DSurface9Impl *)pRenderTarget;
    IDirect3DSurface9Impl *destSurface = (IDirect3DSurface9Impl *)pDestSurface;
    HRESULT hr;

    TRACE("iface %p, render_target %p, dst_surface %p.\n", iface, pRenderTarget, pDestSurface);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_BltFast(destSurface->wineD3DSurface, 0, 0, renderTarget->wineD3DSurface, NULL, WINEDDBLTFAST_NOCOLORKEY);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetFrontBufferData(LPDIRECT3DDEVICE9EX iface, UINT iSwapChain, IDirect3DSurface9* pDestSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *destSurface = (IDirect3DSurface9Impl *)pDestSurface;
    HRESULT hr;

    TRACE("iface %p, swapchain %u, dst_surface %p.\n", iface, iSwapChain, pDestSurface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetFrontBufferData(This->WineD3DDevice, iSwapChain, destSurface->wineD3DSurface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_StretchRect(IDirect3DDevice9Ex *iface, IDirect3DSurface9 *pSourceSurface,
        const RECT *pSourceRect, IDirect3DSurface9 *pDestSurface, const RECT *pDestRect, D3DTEXTUREFILTERTYPE Filter)
{
    IDirect3DSurface9Impl *src = (IDirect3DSurface9Impl *) pSourceSurface;
    IDirect3DSurface9Impl *dst = (IDirect3DSurface9Impl *) pDestSurface;
    HRESULT hr;

    TRACE("iface %p, src_surface %p, src_rect %p, dst_surface %p, dst_rect %p, filter %#x.\n",
            iface, pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter);

    wined3d_mutex_lock();
    hr = IWineD3DSurface_Blt(dst->wineD3DSurface, pDestRect, src->wineD3DSurface, pSourceRect, 0, NULL, Filter);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_ColorFill(LPDIRECT3DDEVICE9EX iface, IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *surface = (IDirect3DSurface9Impl *)pSurface;
    WINED3DPOOL pool;
    WINED3DRESOURCETYPE restype;
    DWORD usage;
    WINED3DSURFACE_DESC desc;
    HRESULT hr;

    TRACE("iface %p, surface %p, rect %p, color 0x%08x.\n", iface, pSurface, pRect, color);

    wined3d_mutex_lock();

    IWineD3DSurface_GetDesc(surface->wineD3DSurface, &desc);
    usage = desc.usage;
    pool = desc.pool;
    restype = desc.resource_type;

    /* This method is only allowed with surfaces that are render targets, or offscreen plain surfaces
     * in D3DPOOL_DEFAULT
     */
    if(!(usage & WINED3DUSAGE_RENDERTARGET) && (pool != WINED3DPOOL_DEFAULT || restype != WINED3DRTYPE_SURFACE)) {
        wined3d_mutex_unlock();
        WARN("Surface is not a render target, or not a stand-alone D3DPOOL_DEFAULT surface\n");
        return D3DERR_INVALIDCALL;
    }

    /* Colorfill can only be used on rendertarget surfaces, or offscreen plain surfaces in D3DPOOL_DEFAULT */
    /* Note: D3DRECT is compatible with WINED3DRECT */
    hr = IWineD3DDevice_ColorFill(This->WineD3DDevice, surface->wineD3DSurface, (CONST WINED3DRECT*)pRect, color);

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_CreateOffscreenPlainSurface(LPDIRECT3DDEVICE9EX iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9 **ppSurface, HANDLE* pSharedHandle) {
    HRESULT hr;

    TRACE("iface %p, width %u, height %u, format %#x, pool %#x, surface %p, shared_handle %p.\n",
            iface, Width, Height, Format, Pool, ppSurface, pSharedHandle);

    if(Pool == D3DPOOL_MANAGED ){
        FIXME("Attempting to create a managed offscreen plain surface\n");
        return D3DERR_INVALIDCALL;
    }
        /*
        'Off-screen plain surfaces are always lockable, regardless of their pool types.'
        but then...
        D3DPOOL_DEFAULT is the appropriate pool for use with the IDirect3DDevice9::StretchRect and IDirect3DDevice9::ColorFill.
        Why, their always lockable?
        should I change the usage to dynamic?
        */
    hr = IDirect3DDevice9Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Lockable */, FALSE /* Discard */,
            0 /* Level */, ppSurface, 0 /* Usage (undefined/none) */, (WINED3DPOOL)Pool, D3DMULTISAMPLE_NONE,
            0 /* MultisampleQuality */);

    return hr;
}

/* TODO: move to wineD3D */
static HRESULT  WINAPI  IDirect3DDevice9Impl_SetRenderTarget(LPDIRECT3DDEVICE9EX iface, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *pSurface = (IDirect3DSurface9Impl*)pRenderTarget;
    HRESULT hr;

    TRACE("iface %p, idx %u, surface %p.\n", iface, RenderTargetIndex, pRenderTarget);

    if (RenderTargetIndex >= D3D9_MAX_SIMULTANEOUS_RENDERTARGETS)
    {
        WARN("Invalid index %u specified.\n", RenderTargetIndex);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetRenderTarget(This->WineD3DDevice, RenderTargetIndex, pSurface ? pSurface->wineD3DSurface : NULL, TRUE);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderTarget(LPDIRECT3DDEVICE9EX iface, DWORD RenderTargetIndex, IDirect3DSurface9 **ppRenderTarget) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DSurface *pRenderTarget;
    HRESULT hr;

    TRACE("iface %p, idx %u, surface %p.\n", iface, RenderTargetIndex, ppRenderTarget);

    if (ppRenderTarget == NULL) {
        return D3DERR_INVALIDCALL;
    }

    if (RenderTargetIndex >= D3D9_MAX_SIMULTANEOUS_RENDERTARGETS)
    {
        WARN("Invalid index %u specified.\n", RenderTargetIndex);
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();

    hr=IWineD3DDevice_GetRenderTarget(This->WineD3DDevice,RenderTargetIndex,&pRenderTarget);

    if (FAILED(hr))
    {
        FIXME("Call to IWineD3DDevice_GetRenderTarget failed, hr %#x\n", hr);
    }
    else if (!pRenderTarget)
    {
        *ppRenderTarget = NULL;
    }
    else
    {
        IWineD3DSurface_GetParent(pRenderTarget, (IUnknown **)ppRenderTarget);
        IWineD3DSurface_Release(pRenderTarget);
    }

    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetDepthStencilSurface(LPDIRECT3DDEVICE9EX iface, IDirect3DSurface9* pZStencilSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DSurface9Impl *pSurface;
    HRESULT hr;

    TRACE("iface %p, depth_stencil %p.\n", iface, pZStencilSurface);

    pSurface = (IDirect3DSurface9Impl*)pZStencilSurface;

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetDepthStencilSurface(This->WineD3DDevice, NULL==pSurface ? NULL : pSurface->wineD3DSurface);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE9EX iface, IDirect3DSurface9 **ppZStencilSurface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pZStencilSurface;

    TRACE("iface %p, depth_stencil %p.\n", iface, ppZStencilSurface);

    if(ppZStencilSurface == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetDepthStencilSurface(This->WineD3DDevice,&pZStencilSurface);
    if (hr == WINED3D_OK) {
        IWineD3DSurface_GetParent(pZStencilSurface,(IUnknown**)ppZStencilSurface);
        IWineD3DSurface_Release(pZStencilSurface);
    } else {
        if (hr != WINED3DERR_NOTFOUND)
                WARN("Call to IWineD3DDevice_GetDepthStencilSurface failed with 0x%08x\n", hr);
        *ppZStencilSurface = NULL;
    }
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_BeginScene(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_BeginScene(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDevice9Impl_EndScene(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_EndScene(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_Clear(LPDIRECT3DDEVICE9EX iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, rect_count %u, rects %p, flags %#x, color 0x%08x, z %.8e, stencil %u.\n",
            iface, Count, pRects, Flags, Color, Z, Stencil);

    /* Note: D3DRECT is compatible with WINED3DRECT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_Clear(This->WineD3DDevice, Count, (CONST WINED3DRECT*) pRects, Flags, Color, Z, Stencil);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetTransform(LPDIRECT3DDEVICE9EX iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* lpMatrix) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, lpMatrix);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) lpMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetTransform(LPDIRECT3DDEVICE9EX iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, pMatrix);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetTransform(This->WineD3DDevice, State, (WINED3DMATRIX*) pMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_MultiplyTransform(LPDIRECT3DDEVICE9EX iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, matrix %p.\n", iface, State, pMatrix);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_MultiplyTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) pMatrix);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetViewport(LPDIRECT3DDEVICE9EX iface, CONST D3DVIEWPORT9* pViewport) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, pViewport);

    /* Note: D3DVIEWPORT9 is compatible with WINED3DVIEWPORT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetViewport(This->WineD3DDevice, (const WINED3DVIEWPORT *)pViewport);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetViewport(LPDIRECT3DDEVICE9EX iface, D3DVIEWPORT9* pViewport) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, viewport %p.\n", iface, pViewport);

    /* Note: D3DVIEWPORT9 is compatible with WINED3DVIEWPORT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetViewport(This->WineD3DDevice, (WINED3DVIEWPORT *)pViewport);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetMaterial(LPDIRECT3DDEVICE9EX iface, CONST D3DMATERIAL9* pMaterial) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, pMaterial);

    /* Note: D3DMATERIAL9 is compatible with WINED3DMATERIAL */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetMaterial(This->WineD3DDevice, (const WINED3DMATERIAL *)pMaterial);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetMaterial(LPDIRECT3DDEVICE9EX iface, D3DMATERIAL9* pMaterial) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, material %p.\n", iface, pMaterial);

    /* Note: D3DMATERIAL9 is compatible with WINED3DMATERIAL */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetMaterial(This->WineD3DDevice, (WINED3DMATERIAL *)pMaterial);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetLight(LPDIRECT3DDEVICE9EX iface, DWORD Index, CONST D3DLIGHT9* pLight) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, light %p.\n", iface, Index, pLight);

    /* Note: D3DLIGHT9 is compatible with WINED3DLIGHT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetLight(This->WineD3DDevice, Index, (const WINED3DLIGHT *)pLight);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetLight(LPDIRECT3DDEVICE9EX iface, DWORD Index, D3DLIGHT9* pLight) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, light %p.\n", iface, Index, pLight);

    /* Note: D3DLIGHT9 is compatible with WINED3DLIGHT */
    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetLight(This->WineD3DDevice, Index, (WINED3DLIGHT *)pLight);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_LightEnable(LPDIRECT3DDEVICE9EX iface, DWORD Index, BOOL Enable) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, enable %#x.\n", iface, Index, Enable);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetLightEnable(This->WineD3DDevice, Index, Enable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetLightEnable(LPDIRECT3DDEVICE9EX iface, DWORD Index, BOOL* pEnable) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, enable %p.\n", iface, Index, pEnable);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetLightEnable(This->WineD3DDevice, Index, pEnable);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetClipPlane(LPDIRECT3DDEVICE9EX iface, DWORD Index, CONST float* pPlane) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, plane %p.\n", iface, Index, pPlane);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetClipPlane(This->WineD3DDevice, Index, pPlane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetClipPlane(LPDIRECT3DDEVICE9EX iface, DWORD Index, float* pPlane) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, index %u, plane %p.\n", iface, Index, pPlane);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetClipPlane(This->WineD3DDevice, Index, pPlane);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI DECLSPEC_HOTPATCH IDirect3DDevice9Impl_SetRenderState(LPDIRECT3DDEVICE9EX iface, D3DRENDERSTATETYPE State, DWORD Value) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, value %#x.\n", iface, State, Value);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetRenderState(This->WineD3DDevice, State, Value);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetRenderState(LPDIRECT3DDEVICE9EX iface, D3DRENDERSTATETYPE State, DWORD* pValue) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, state %#x, value %p.\n", iface, State, pValue);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetRenderState(This->WineD3DDevice, State, pValue);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateStateBlock(IDirect3DDevice9Ex *iface,
        D3DSTATEBLOCKTYPE type, IDirect3DStateBlock9 **stateblock)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DStateBlock9Impl *object;
    HRESULT hr;

    TRACE("iface %p, type %#x, stateblock %p.\n", iface, type, stateblock);

    if (type != D3DSBT_ALL && type != D3DSBT_PIXELSTATE && type != D3DSBT_VERTEXSTATE)
    {
        WARN("Unexpected stateblock type, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate stateblock memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = stateblock_init(object, This, type, NULL);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stateblock, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created stateblock %p.\n", object);
    *stateblock = (IDirect3DStateBlock9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_BeginStateBlock(IDirect3DDevice9Ex *iface)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_BeginStateBlock(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_EndStateBlock(IDirect3DDevice9Ex *iface, IDirect3DStateBlock9 **stateblock)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DStateBlock *wined3d_stateblock;
    IDirect3DStateBlock9Impl *object;
    HRESULT hr;

    TRACE("iface %p, stateblock %p.\n", iface, stateblock);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_EndStateBlock(This->WineD3DDevice, &wined3d_stateblock);
    wined3d_mutex_unlock();
    if (FAILED(hr))
    {
       WARN("IWineD3DDevice_EndStateBlock() failed, hr %#x.\n", hr);
       return hr;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate stateblock memory.\n");
        IWineD3DStateBlock_Release(wined3d_stateblock);
        return E_OUTOFMEMORY;
    }

    hr = stateblock_init(object, This, 0, wined3d_stateblock);
    if (FAILED(hr))
    {
        WARN("Failed to initialize stateblock, hr %#x.\n", hr);
        IWineD3DStateBlock_Release(wined3d_stateblock);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created stateblock %p.\n", object);
    *stateblock = (IDirect3DStateBlock9 *)object;

    return D3D_OK;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetClipStatus(LPDIRECT3DDEVICE9EX iface, CONST D3DCLIPSTATUS9* pClipStatus) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, pClipStatus);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetClipStatus(This->WineD3DDevice, (const WINED3DCLIPSTATUS *)pClipStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetClipStatus(LPDIRECT3DDEVICE9EX iface, D3DCLIPSTATUS9* pClipStatus) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, clip_status %p.\n", iface, pClipStatus);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetClipStatus(This->WineD3DDevice, (WINED3DCLIPSTATUS *)pClipStatus);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetTexture(LPDIRECT3DDEVICE9EX iface, DWORD Stage, IDirect3DBaseTexture9 **ppTexture) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DBaseTexture *retTexture = NULL;
    HRESULT rc = D3D_OK;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, ppTexture);

    if(ppTexture == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    rc = IWineD3DDevice_GetTexture(This->WineD3DDevice, Stage, &retTexture);
    if (SUCCEEDED(rc) && NULL != retTexture) {
        IWineD3DBaseTexture_GetParent(retTexture, (IUnknown **)ppTexture);
        IWineD3DBaseTexture_Release(retTexture);
    } else {
        if(FAILED(rc)) {
            WARN("Call to get texture  (%d) failed (%p)\n", Stage, retTexture);
        }
        *ppTexture = NULL;
    }
    wined3d_mutex_unlock();

    return rc;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetTexture(LPDIRECT3DDEVICE9EX iface, DWORD Stage, IDirect3DBaseTexture9* pTexture) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stage %u, texture %p.\n", iface, Stage, pTexture);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetTexture(This->WineD3DDevice, Stage,
                                   pTexture==NULL ? NULL:((IDirect3DBaseTexture9Impl *)pTexture)->wineD3DBaseTexture);
    wined3d_mutex_unlock();

    return hr;
}

static const WINED3DTEXTURESTAGESTATETYPE tss_lookup[] =
{
    WINED3DTSS_FORCE_DWORD,             /*  0, unused */
    WINED3DTSS_COLOROP,                 /*  1, D3DTSS_COLOROP */
    WINED3DTSS_COLORARG1,               /*  2, D3DTSS_COLORARG1 */
    WINED3DTSS_COLORARG2,               /*  3, D3DTSS_COLORARG2 */
    WINED3DTSS_ALPHAOP,                 /*  4, D3DTSS_ALPHAOP */
    WINED3DTSS_ALPHAARG1,               /*  5, D3DTSS_ALPHAARG1 */
    WINED3DTSS_ALPHAARG2,               /*  6, D3DTSS_ALPHAARG2 */
    WINED3DTSS_BUMPENVMAT00,            /*  7, D3DTSS_BUMPENVMAT00 */
    WINED3DTSS_BUMPENVMAT01,            /*  8, D3DTSS_BUMPENVMAT01 */
    WINED3DTSS_BUMPENVMAT10,            /*  9, D3DTSS_BUMPENVMAT10 */
    WINED3DTSS_BUMPENVMAT11,            /* 10, D3DTSS_BUMPENVMAT11 */
    WINED3DTSS_TEXCOORDINDEX,           /* 11, D3DTSS_TEXCOORDINDEX */
    WINED3DTSS_FORCE_DWORD,             /* 12, unused */
    WINED3DTSS_FORCE_DWORD,             /* 13, unused */
    WINED3DTSS_FORCE_DWORD,             /* 14, unused */
    WINED3DTSS_FORCE_DWORD,             /* 15, unused */
    WINED3DTSS_FORCE_DWORD,             /* 16, unused */
    WINED3DTSS_FORCE_DWORD,             /* 17, unused */
    WINED3DTSS_FORCE_DWORD,             /* 18, unused */
    WINED3DTSS_FORCE_DWORD,             /* 19, unused */
    WINED3DTSS_FORCE_DWORD,             /* 20, unused */
    WINED3DTSS_FORCE_DWORD,             /* 21, unused */
    WINED3DTSS_BUMPENVLSCALE,           /* 22, D3DTSS_BUMPENVLSCALE */
    WINED3DTSS_BUMPENVLOFFSET,          /* 23, D3DTSS_BUMPENVLOFFSET */
    WINED3DTSS_TEXTURETRANSFORMFLAGS,   /* 24, D3DTSS_TEXTURETRANSFORMFLAGS */
    WINED3DTSS_FORCE_DWORD,             /* 25, unused */
    WINED3DTSS_COLORARG0,               /* 26, D3DTSS_COLORARG0 */
    WINED3DTSS_ALPHAARG0,               /* 27, D3DTSS_ALPHAARG0 */
    WINED3DTSS_RESULTARG,               /* 28, D3DTSS_RESULTARG */
    WINED3DTSS_FORCE_DWORD,             /* 29, unused */
    WINED3DTSS_FORCE_DWORD,             /* 30, unused */
    WINED3DTSS_FORCE_DWORD,             /* 31, unused */
    WINED3DTSS_CONSTANT,                /* 32, D3DTSS_CONSTANT */
};

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetTextureStageState(LPDIRECT3DDEVICE9EX iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stage %u, state %#x, value %p.\n", iface, Stage, Type, pValue);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetTextureStageState(This->WineD3DDevice, Stage, tss_lookup[Type], pValue);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetTextureStageState(LPDIRECT3DDEVICE9EX iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stage %u, state %#x, value %#x.\n", iface, Stage, Type, Value);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetTextureStageState(This->WineD3DDevice, Stage, tss_lookup[Type], Value);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetSamplerState(IDirect3DDevice9Ex *iface, DWORD Sampler,
        D3DSAMPLERSTATETYPE Type, DWORD *pValue)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, sampler %u, state %#x, value %p.\n", iface, Sampler, Type, pValue);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetSamplerState(This->WineD3DDevice, Sampler, Type, pValue);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3DDevice9Impl_SetSamplerState(LPDIRECT3DDEVICE9EX iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, sampler %u, state %#x, value %#x.\n", iface, Sampler, Type, Value);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetSamplerState(This->WineD3DDevice, Sampler, Type, Value);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_ValidateDevice(LPDIRECT3DDEVICE9EX iface, DWORD* pNumPasses) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, pass_count %p.\n", iface, pNumPasses);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_ValidateDevice(This->WineD3DDevice, pNumPasses);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetPaletteEntries(IDirect3DDevice9Ex *iface, UINT PaletteNumber,
        const PALETTEENTRY *pEntries)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %u, entries %p.\n", iface, PaletteNumber, pEntries);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetPaletteEntries(LPDIRECT3DDEVICE9EX iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %u, entries %p.\n", iface, PaletteNumber, pEntries);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE9EX iface, UINT PaletteNumber) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %u.\n", iface, PaletteNumber);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE9EX iface, UINT* PaletteNumber) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, palette_idx %p.\n", iface, PaletteNumber);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetScissorRect(LPDIRECT3DDEVICE9EX iface, CONST RECT* pRect) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, rect %p.\n", iface, pRect);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetScissorRect(This->WineD3DDevice, pRect);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetScissorRect(LPDIRECT3DDEVICE9EX iface, RECT* pRect) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, rect %p.\n", iface, pRect);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetScissorRect(This->WineD3DDevice, pRect);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetSoftwareVertexProcessing(LPDIRECT3DDEVICE9EX iface, BOOL bSoftware) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, software %#x.\n", iface, bSoftware);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetSoftwareVertexProcessing(This->WineD3DDevice, bSoftware);
    wined3d_mutex_unlock();

    return hr;
}

static BOOL     WINAPI  IDirect3DDevice9Impl_GetSoftwareVertexProcessing(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    BOOL ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DDevice_GetSoftwareVertexProcessing(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetNPatchMode(LPDIRECT3DDEVICE9EX iface, float nSegments) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, segment_count %.8e.\n", iface, nSegments);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetNPatchMode(This->WineD3DDevice, nSegments);
    wined3d_mutex_unlock();

    return hr;
}

static float    WINAPI  IDirect3DDevice9Impl_GetNPatchMode(LPDIRECT3DDEVICE9EX iface) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    float ret;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    ret = IWineD3DDevice_GetNPatchMode(This->WineD3DDevice);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawPrimitive(IDirect3DDevice9Ex *iface, D3DPRIMITIVETYPE PrimitiveType,
        UINT StartVertex, UINT PrimitiveCount)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
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

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE9EX iface, D3DPRIMITIVETYPE PrimitiveType,
                                                           INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, primitive_type %#x, base_vertex_idx %u, min_vertex_idx %u,\n"
            "vertex_count %u, start_idx %u, primitive_count %u.\n",
            iface, PrimitiveType, BaseVertexIndex, MinVertexIndex,
            NumVertices, startIndex, primCount);

    wined3d_mutex_lock();
    IWineD3DDevice_SetBaseVertexIndex(This->WineD3DDevice, BaseVertexIndex);
    IWineD3DDevice_SetPrimitiveType(This->WineD3DDevice, PrimitiveType);
    hr = IWineD3DDevice_DrawIndexedPrimitive(This->WineD3DDevice, startIndex,
            vertex_count_from_primitive_count(PrimitiveType, primCount));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_DrawPrimitiveUP(IDirect3DDevice9Ex *iface, D3DPRIMITIVETYPE PrimitiveType,
        UINT PrimitiveCount, const void *pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
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

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9EX iface, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex,
                                                             UINT NumVertexIndices, UINT PrimitiveCount, CONST void* pIndexData,
                                                             D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
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

static HRESULT  WINAPI  IDirect3DDevice9Impl_ProcessVertices(LPDIRECT3DDEVICE9EX iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *Decl = (IDirect3DVertexDeclaration9Impl *) pVertexDecl;
    HRESULT hr;
    IDirect3DVertexBuffer9Impl *dest = (IDirect3DVertexBuffer9Impl *) pDestBuffer;

    TRACE("iface %p, src_start_idx %u, dst_idx %u, vertex_count %u, dst_buffer %p, declaration %p, flags %#x.\n",
            iface, SrcStartIndex, DestIndex, VertexCount, pDestBuffer, pVertexDecl, Flags);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_ProcessVertices(This->WineD3DDevice,SrcStartIndex, DestIndex, VertexCount, dest->wineD3DVertexBuffer, Decl ? Decl->wineD3DVertexDeclaration : NULL, Flags, dest->fvf);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexDeclaration(IDirect3DDevice9Ex *iface,
        const D3DVERTEXELEMENT9 *elements, IDirect3DVertexDeclaration9 **declaration)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9Impl *object;
    HRESULT hr;

    TRACE("iface %p, elements %p, declaration %p.\n", iface, elements, declaration);

    if (!declaration)
    {
        WARN("Caller passed a NULL declaration, returning D3DERR_INVALIDCALL.\n");
        return D3DERR_INVALIDCALL;
    }

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate vertex declaration memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = vertexdeclaration_init(object, This, elements);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex declaration, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex declaration %p.\n", object);
    *declaration = (IDirect3DVertexDeclaration9 *)object;

    return D3D_OK;
}

static IDirect3DVertexDeclaration9 *getConvertedDecl(IDirect3DDevice9Impl *This, DWORD fvf) {
    HRESULT hr;
    D3DVERTEXELEMENT9* elements = NULL;
    IDirect3DVertexDeclaration9* pDecl = NULL;
    int p, low, high; /* deliberately signed */
    IDirect3DVertexDeclaration9  **convertedDecls = This->convertedDecls;

    TRACE("Searching for declaration for fvf %08x... ", fvf);

    low = 0;
    high = This->numConvertedDecls - 1;
    while(low <= high) {
        p = (low + high) >> 1;
        TRACE("%d ", p);
        if(((IDirect3DVertexDeclaration9Impl *) convertedDecls[p])->convFVF == fvf) {
            TRACE("found %p\n", convertedDecls[p]);
            return convertedDecls[p];
        } else if(((IDirect3DVertexDeclaration9Impl *) convertedDecls[p])->convFVF < fvf) {
            low = p + 1;
        } else {
            high = p - 1;
        }
    }
    TRACE("not found. Creating and inserting at position %d.\n", low);

    hr = vdecl_convert_fvf(fvf, &elements);
    if (hr != S_OK) return NULL;

    hr = IDirect3DDevice9Impl_CreateVertexDeclaration((IDirect3DDevice9Ex *) This, elements, &pDecl);
    HeapFree(GetProcessHeap(), 0, elements); /* CreateVertexDeclaration makes a copy */
    if (hr != S_OK) return NULL;

    if(This->declArraySize == This->numConvertedDecls) {
        int grow = max(This->declArraySize / 2, 8);
        convertedDecls = HeapReAlloc(GetProcessHeap(), 0, convertedDecls,
                                     sizeof(convertedDecls[0]) * (This->numConvertedDecls + grow));
        if(!convertedDecls) {
            /* This will destroy it */
            IDirect3DVertexDeclaration9_Release(pDecl);
            return NULL;
        }
        This->convertedDecls = convertedDecls;
        This->declArraySize += grow;
    }

    memmove(convertedDecls + low + 1, convertedDecls + low, sizeof(IDirect3DVertexDeclaration9Impl *) * (This->numConvertedDecls - low));
    convertedDecls[low] = pDecl;
    This->numConvertedDecls++;

    /* Will prevent the decl from being destroyed */
    ((IDirect3DVertexDeclaration9Impl *) pDecl)->convFVF = fvf;
    IDirect3DVertexDeclaration9_Release(pDecl); /* Does not destroy now */

    TRACE("Returning %p. %d decls in array\n", pDecl, This->numConvertedDecls);
    return pDecl;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetFVF(LPDIRECT3DDEVICE9EX iface, DWORD FVF) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexDeclaration9 *decl;
    HRESULT hr;

    TRACE("iface %p, fvf %#x.\n", iface, FVF);

    if (!FVF)
    {
        WARN("%#x is not a valid FVF\n", FVF);
        return D3D_OK;
    }

    wined3d_mutex_lock();
    decl = getConvertedDecl(This, FVF);
    wined3d_mutex_unlock();

    if (!decl)
    {
         /* Any situation when this should happen, except out of memory? */
         ERR("Failed to create a converted vertex declaration\n");
         return D3DERR_DRIVERINTERNALERROR;
    }

    hr = IDirect3DDevice9Impl_SetVertexDeclaration(iface, decl);
    if (FAILED(hr)) ERR("Failed to set vertex declaration\n");

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetFVF(IDirect3DDevice9Ex *iface, DWORD *pFVF)
{
    IDirect3DVertexDeclaration9 *decl;
    HRESULT hr;

    TRACE("iface %p, fvf %p.\n", iface, pFVF);

    hr = IDirect3DDevice9_GetVertexDeclaration(iface, &decl);
    if (FAILED(hr))
    {
        WARN("Failed to get vertex declaration, %#x\n", hr);
        *pFVF = 0;
        return hr;
    }

    if (decl)
    {
        *pFVF = ((IDirect3DVertexDeclaration9Impl *)decl)->convFVF;
        IDirect3DVertexDeclaration9_Release(decl);
    }
    else
    {
        *pFVF = 0;
    }

    TRACE("Returning FVF %#x\n", *pFVF);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateVertexShader(IDirect3DDevice9Ex *iface,
        const DWORD *byte_code, IDirect3DVertexShader9 **shader)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DVertexShader9Impl *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, shader %p.\n", iface, byte_code, shader);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate vertex shader memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = vertexshader_init(object, This, byte_code);
    if (FAILED(hr))
    {
        WARN("Failed to initialize vertex shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created vertex shader %p.\n", object);
    *shader = (IDirect3DVertexShader9 *)object;

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetStreamSource(LPDIRECT3DDEVICE9EX iface, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, buffer %p, offset %u, stride %u.\n",
            iface, StreamNumber, pStreamData, OffsetInBytes, Stride);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetStreamSource(This->WineD3DDevice, StreamNumber,
            pStreamData ? ((IDirect3DVertexBuffer9Impl *)pStreamData)->wineD3DVertexBuffer : NULL,
            OffsetInBytes, Stride);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_GetStreamSource(LPDIRECT3DDEVICE9EX iface, UINT StreamNumber, IDirect3DVertexBuffer9 **pStream, UINT* OffsetInBytes, UINT* pStride) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DBuffer *retStream = NULL;
    HRESULT rc = D3D_OK;

    TRACE("iface %p, stream_idx %u, buffer %p, offset %p, stride %p.\n",
            iface, StreamNumber, pStream, OffsetInBytes, pStride);

    if(pStream == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
    rc = IWineD3DDevice_GetStreamSource(This->WineD3DDevice, StreamNumber, &retStream, OffsetInBytes, pStride);
    if (rc == D3D_OK  && NULL != retStream) {
        IWineD3DBuffer_GetParent(retStream, (IUnknown **)pStream);
        IWineD3DBuffer_Release(retStream);
    }else{
        if (rc != D3D_OK){
            FIXME("Call to GetStreamSource failed %p %p\n", OffsetInBytes, pStride);
        }
        *pStream = NULL;
    }
    wined3d_mutex_unlock();

    return rc;
}

static HRESULT WINAPI IDirect3DDevice9Impl_SetStreamSourceFreq(IDirect3DDevice9Ex *iface, UINT StreamNumber,
        UINT Divider)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, freq %u.\n", iface, StreamNumber, Divider);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetStreamSourceFreq(This->WineD3DDevice, StreamNumber, Divider);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetStreamSourceFreq(LPDIRECT3DDEVICE9EX iface, UINT StreamNumber, UINT* Divider) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, stream_idx %u, freq %p.\n", iface, StreamNumber, Divider);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_GetStreamSourceFreq(This->WineD3DDevice, StreamNumber, Divider);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_SetIndices(LPDIRECT3DDEVICE9EX iface, IDirect3DIndexBuffer9* pIndexData) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;
    IDirect3DIndexBuffer9Impl *ib = (IDirect3DIndexBuffer9Impl *) pIndexData;

    TRACE("iface %p, buffer %p.\n", iface, pIndexData);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_SetIndexBuffer(This->WineD3DDevice,
            ib ? ib->wineD3DIndexBuffer : NULL,
            ib ? ib->format : WINED3DFMT_UNKNOWN);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_GetIndices(LPDIRECT3DDEVICE9EX iface, IDirect3DIndexBuffer9 **ppIndexData) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IWineD3DBuffer *retIndexData = NULL;
    HRESULT rc = D3D_OK;

    TRACE("iface %p, buffer %p.\n", iface, ppIndexData);

    if(ppIndexData == NULL){
        return D3DERR_INVALIDCALL;
    }

    wined3d_mutex_lock();
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

static HRESULT WINAPI IDirect3DDevice9Impl_CreatePixelShader(IDirect3DDevice9Ex *iface,
        const DWORD *byte_code, IDirect3DPixelShader9 **shader)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DPixelShader9Impl *object;
    HRESULT hr;

    TRACE("iface %p, byte_code %p, shader %p.\n", iface, byte_code, shader);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        FIXME("Failed to allocate pixel shader memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = pixelshader_init(object, This, byte_code);
    if (FAILED(hr))
    {
        WARN("Failed to initialize pixel shader, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created pixel shader %p.\n", object);
    *shader = (IDirect3DPixelShader9 *)object;

    return D3D_OK;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawRectPatch(LPDIRECT3DDEVICE9EX iface, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, handle %#x, segment_count %p, patch_info %p.\n",
            iface, Handle, pNumSegs, pRectPatchInfo);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_DrawRectPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DRECTPATCH_INFO *)pRectPatchInfo);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DrawTriPatch(LPDIRECT3DDEVICE9EX iface, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, handle %#x, segment_count %p, patch_info %p.\n",
            iface, Handle, pNumSegs, pTriPatchInfo);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_DrawTriPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DTRIPATCH_INFO *)pTriPatchInfo);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice9Impl_DeletePatch(LPDIRECT3DDEVICE9EX iface, UINT Handle) {
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, handle %#x.\n", iface, Handle);

    wined3d_mutex_lock();
    hr = IWineD3DDevice_DeletePatch(This->WineD3DDevice, Handle);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3DDevice9Impl_CreateQuery(IDirect3DDevice9Ex *iface,
        D3DQUERYTYPE type, IDirect3DQuery9 **query)
{
    IDirect3DDevice9Impl *This = (IDirect3DDevice9Impl *)iface;
    IDirect3DQuery9Impl *object;
    HRESULT hr;

    TRACE("iface %p, type %#x, query %p.\n", iface, type, query);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate query memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = query_init(object, This, type);
    if (FAILED(hr))
    {
        WARN("Failed to initialize query, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created query %p.\n", object);
    if (query) *query = (IDirect3DQuery9 *)object;
    else IDirect3DQuery9_Release((IDirect3DQuery9 *)object);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_SetConvolutionMonoKernel(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, float *rows, float *columns)
{
    FIXME("iface %p, width %u, height %u, rows %p, columns %p stub!\n",
            iface, width, height, rows, columns);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_ComposeRects(IDirect3DDevice9Ex *iface,
        IDirect3DSurface9 *src_surface, IDirect3DSurface9 *dst_surface, IDirect3DVertexBuffer9 *src_descs,
        UINT rect_count, IDirect3DVertexBuffer9 *dst_descs, D3DCOMPOSERECTSOP operation, INT offset_x, INT offset_y)
{
    FIXME("iface %p, src_surface %p, dst_surface %p, src_descs %p, rect_count %u,\n"
            "dst_descs %p, operation %#x, offset_x %u, offset_y %u stub!\n",
            iface, src_surface, dst_surface, src_descs, rect_count,
            dst_descs, operation, offset_x, offset_y);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_PresentEx(IDirect3DDevice9Ex *iface,
        const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
        const RGNDATA *dirty_region, DWORD flags)
{
    FIXME("iface %p, src_rect %p, dst_rect %p, dst_window_override %p, dirty_region %p, flags %#x stub!\n",
            iface, src_rect, dst_rect, dst_window_override, dirty_region, flags);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_GetGPUThreadPriority(IDirect3DDevice9Ex *iface, INT *priority)
{
    FIXME("iface %p, priority %p stub!\n", iface, priority);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_SetGPUThreadPriority(IDirect3DDevice9Ex *iface, INT priority)
{
    FIXME("iface %p, priority %d stub!\n", iface, priority);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_WaitForVBlank(IDirect3DDevice9Ex *iface, UINT swapchain_idx)
{
    FIXME("iface %p, swapchain_idx %u stub!\n", iface, swapchain_idx);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_CheckResourceResidency(IDirect3DDevice9Ex *iface,
        IDirect3DResource9 **resources, UINT32 resource_count)
{
    FIXME("iface %p, resources %p, resource_count %u stub!\n",
            iface, resources, resource_count);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_SetMaximumFrameLatency(IDirect3DDevice9Ex *iface, UINT max_latency)
{
    FIXME("iface %p, max_latency %u stub!\n", iface, max_latency);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_GetMaximumFrameLatency(IDirect3DDevice9Ex *iface, UINT *max_latency)
{
    FIXME("iface %p, max_latency %p stub!\n", iface, max_latency);

    *max_latency = 2;

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_CheckDeviceState(IDirect3DDevice9Ex *iface, HWND dst_window)
{
    FIXME("iface %p, dst_window %p stub!\n", iface, dst_window);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_CreateRenderTargetEx(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality,
        BOOL lockable, IDirect3DSurface9 **surface, HANDLE *shared_handle, DWORD usage)
{
    FIXME("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %u,\n"
            "lockable %#x, surface %p, shared_handle %p, usage %#x stub!\n",
            iface, width, height, format, multisample_type, multisample_quality,
            lockable, surface, shared_handle, usage);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_CreateOffscreenPlainSurfaceEx(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, D3DFORMAT format, D3DPOOL pool, IDirect3DSurface9 **surface,
        HANDLE *shared_handle, DWORD usage)
{
    FIXME("iface %p, width %u, height %u, format %#x, pool %#x, surface %p, shared_handle %p, usage %#x stub!\n",
            iface, width, height, format, pool, surface, shared_handle, usage);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_CreateDepthStencilSurfaceEx(IDirect3DDevice9Ex *iface,
        UINT width, UINT height, D3DFORMAT format, D3DMULTISAMPLE_TYPE multisample_type, DWORD multisample_quality,
        BOOL discard, IDirect3DSurface9 **surface, HANDLE *shared_handle, DWORD usage)
{
    FIXME("iface %p, width %u, height %u, format %#x, multisample_type %#x, multisample_quality %u,\n"
            "discard %#x, surface %p, shared_handle %p, usage %#x stub!\n",
            iface, width, height, format, multisample_type, multisample_quality,
            discard, surface, shared_handle, usage);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT WINAPI IDirect3DDevice9ExImpl_ResetEx(IDirect3DDevice9Ex *iface,
        D3DPRESENT_PARAMETERS *present_parameters, D3DDISPLAYMODEEX *mode)
{
    FIXME("iface %p, present_parameters %p, mode %p stub!\n", iface, present_parameters, mode);

    return WINED3DERR_INVALIDCALL;
}

static HRESULT  WINAPI  IDirect3DDevice9ExImpl_GetDisplayModeEx(IDirect3DDevice9Ex *iface,
        UINT swapchain_idx, D3DDISPLAYMODEEX *mode, D3DDISPLAYROTATION *rotation)
{
    FIXME("iface %p, swapchain_idx %u, mode %p, rotation %p stub!\n", iface, swapchain_idx, mode, rotation);

    return WINED3DERR_INVALIDCALL;
}

static const IDirect3DDevice9ExVtbl Direct3DDevice9_Vtbl =
{
    /* IUnknown */
    IDirect3DDevice9Impl_QueryInterface,
    IDirect3DDevice9Impl_AddRef,
    IDirect3DDevice9Impl_Release,
    /* IDirect3DDevice9 */
    IDirect3DDevice9Impl_TestCooperativeLevel,
    IDirect3DDevice9Impl_GetAvailableTextureMem,
    IDirect3DDevice9Impl_EvictManagedResources,
    IDirect3DDevice9Impl_GetDirect3D,
    IDirect3DDevice9Impl_GetDeviceCaps,
    IDirect3DDevice9Impl_GetDisplayMode,
    IDirect3DDevice9Impl_GetCreationParameters,
    IDirect3DDevice9Impl_SetCursorProperties,
    IDirect3DDevice9Impl_SetCursorPosition,
    IDirect3DDevice9Impl_ShowCursor,
    IDirect3DDevice9Impl_CreateAdditionalSwapChain,
    IDirect3DDevice9Impl_GetSwapChain,
    IDirect3DDevice9Impl_GetNumberOfSwapChains,
    IDirect3DDevice9Impl_Reset,
    IDirect3DDevice9Impl_Present,
    IDirect3DDevice9Impl_GetBackBuffer,
    IDirect3DDevice9Impl_GetRasterStatus,
    IDirect3DDevice9Impl_SetDialogBoxMode,
    IDirect3DDevice9Impl_SetGammaRamp,
    IDirect3DDevice9Impl_GetGammaRamp,
    IDirect3DDevice9Impl_CreateTexture,
    IDirect3DDevice9Impl_CreateVolumeTexture,
    IDirect3DDevice9Impl_CreateCubeTexture,
    IDirect3DDevice9Impl_CreateVertexBuffer,
    IDirect3DDevice9Impl_CreateIndexBuffer,
    IDirect3DDevice9Impl_CreateRenderTarget,
    IDirect3DDevice9Impl_CreateDepthStencilSurface,
    IDirect3DDevice9Impl_UpdateSurface,
    IDirect3DDevice9Impl_UpdateTexture,
    IDirect3DDevice9Impl_GetRenderTargetData,
    IDirect3DDevice9Impl_GetFrontBufferData,
    IDirect3DDevice9Impl_StretchRect,
    IDirect3DDevice9Impl_ColorFill,
    IDirect3DDevice9Impl_CreateOffscreenPlainSurface,
    IDirect3DDevice9Impl_SetRenderTarget,
    IDirect3DDevice9Impl_GetRenderTarget,
    IDirect3DDevice9Impl_SetDepthStencilSurface,
    IDirect3DDevice9Impl_GetDepthStencilSurface,
    IDirect3DDevice9Impl_BeginScene,
    IDirect3DDevice9Impl_EndScene,
    IDirect3DDevice9Impl_Clear,
    IDirect3DDevice9Impl_SetTransform,
    IDirect3DDevice9Impl_GetTransform,
    IDirect3DDevice9Impl_MultiplyTransform,
    IDirect3DDevice9Impl_SetViewport,
    IDirect3DDevice9Impl_GetViewport,
    IDirect3DDevice9Impl_SetMaterial,
    IDirect3DDevice9Impl_GetMaterial,
    IDirect3DDevice9Impl_SetLight,
    IDirect3DDevice9Impl_GetLight,
    IDirect3DDevice9Impl_LightEnable,
    IDirect3DDevice9Impl_GetLightEnable,
    IDirect3DDevice9Impl_SetClipPlane,
    IDirect3DDevice9Impl_GetClipPlane,
    IDirect3DDevice9Impl_SetRenderState,
    IDirect3DDevice9Impl_GetRenderState,
    IDirect3DDevice9Impl_CreateStateBlock,
    IDirect3DDevice9Impl_BeginStateBlock,
    IDirect3DDevice9Impl_EndStateBlock,
    IDirect3DDevice9Impl_SetClipStatus,
    IDirect3DDevice9Impl_GetClipStatus,
    IDirect3DDevice9Impl_GetTexture,
    IDirect3DDevice9Impl_SetTexture,
    IDirect3DDevice9Impl_GetTextureStageState,
    IDirect3DDevice9Impl_SetTextureStageState,
    IDirect3DDevice9Impl_GetSamplerState,
    IDirect3DDevice9Impl_SetSamplerState,
    IDirect3DDevice9Impl_ValidateDevice,
    IDirect3DDevice9Impl_SetPaletteEntries,
    IDirect3DDevice9Impl_GetPaletteEntries,
    IDirect3DDevice9Impl_SetCurrentTexturePalette,
    IDirect3DDevice9Impl_GetCurrentTexturePalette,
    IDirect3DDevice9Impl_SetScissorRect,
    IDirect3DDevice9Impl_GetScissorRect,
    IDirect3DDevice9Impl_SetSoftwareVertexProcessing,
    IDirect3DDevice9Impl_GetSoftwareVertexProcessing,
    IDirect3DDevice9Impl_SetNPatchMode,
    IDirect3DDevice9Impl_GetNPatchMode,
    IDirect3DDevice9Impl_DrawPrimitive,
    IDirect3DDevice9Impl_DrawIndexedPrimitive,
    IDirect3DDevice9Impl_DrawPrimitiveUP,
    IDirect3DDevice9Impl_DrawIndexedPrimitiveUP,
    IDirect3DDevice9Impl_ProcessVertices,
    IDirect3DDevice9Impl_CreateVertexDeclaration,
    IDirect3DDevice9Impl_SetVertexDeclaration,
    IDirect3DDevice9Impl_GetVertexDeclaration,
    IDirect3DDevice9Impl_SetFVF,
    IDirect3DDevice9Impl_GetFVF,
    IDirect3DDevice9Impl_CreateVertexShader,
    IDirect3DDevice9Impl_SetVertexShader,
    IDirect3DDevice9Impl_GetVertexShader,
    IDirect3DDevice9Impl_SetVertexShaderConstantF,
    IDirect3DDevice9Impl_GetVertexShaderConstantF,
    IDirect3DDevice9Impl_SetVertexShaderConstantI,
    IDirect3DDevice9Impl_GetVertexShaderConstantI,
    IDirect3DDevice9Impl_SetVertexShaderConstantB,
    IDirect3DDevice9Impl_GetVertexShaderConstantB,
    IDirect3DDevice9Impl_SetStreamSource,
    IDirect3DDevice9Impl_GetStreamSource,
    IDirect3DDevice9Impl_SetStreamSourceFreq,
    IDirect3DDevice9Impl_GetStreamSourceFreq,
    IDirect3DDevice9Impl_SetIndices,
    IDirect3DDevice9Impl_GetIndices,
    IDirect3DDevice9Impl_CreatePixelShader,
    IDirect3DDevice9Impl_SetPixelShader,
    IDirect3DDevice9Impl_GetPixelShader,
    IDirect3DDevice9Impl_SetPixelShaderConstantF,
    IDirect3DDevice9Impl_GetPixelShaderConstantF,
    IDirect3DDevice9Impl_SetPixelShaderConstantI,
    IDirect3DDevice9Impl_GetPixelShaderConstantI,
    IDirect3DDevice9Impl_SetPixelShaderConstantB,
    IDirect3DDevice9Impl_GetPixelShaderConstantB,
    IDirect3DDevice9Impl_DrawRectPatch,
    IDirect3DDevice9Impl_DrawTriPatch,
    IDirect3DDevice9Impl_DeletePatch,
    IDirect3DDevice9Impl_CreateQuery,
    /* IDirect3DDevice9Ex */
    IDirect3DDevice9ExImpl_SetConvolutionMonoKernel,
    IDirect3DDevice9ExImpl_ComposeRects,
    IDirect3DDevice9ExImpl_PresentEx,
    IDirect3DDevice9ExImpl_GetGPUThreadPriority,
    IDirect3DDevice9ExImpl_SetGPUThreadPriority,
    IDirect3DDevice9ExImpl_WaitForVBlank,
    IDirect3DDevice9ExImpl_CheckResourceResidency,
    IDirect3DDevice9ExImpl_SetMaximumFrameLatency,
    IDirect3DDevice9ExImpl_GetMaximumFrameLatency,
    IDirect3DDevice9ExImpl_CheckDeviceState,
    IDirect3DDevice9ExImpl_CreateRenderTargetEx,
    IDirect3DDevice9ExImpl_CreateOffscreenPlainSurfaceEx,
    IDirect3DDevice9ExImpl_CreateDepthStencilSurfaceEx,
    IDirect3DDevice9ExImpl_ResetEx,
    IDirect3DDevice9ExImpl_GetDisplayModeEx
};

/* IWineD3DDeviceParent IUnknown methods */

static inline struct IDirect3DDevice9Impl *device_from_device_parent(IWineD3DDeviceParent *iface)
{
    return (struct IDirect3DDevice9Impl *)((char*)iface
            - FIELD_OFFSET(struct IDirect3DDevice9Impl, device_parent_vtbl));
}

static HRESULT STDMETHODCALLTYPE device_parent_QueryInterface(IWineD3DDeviceParent *iface, REFIID riid, void **object)
{
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    return IDirect3DDevice9Impl_QueryInterface((IDirect3DDevice9Ex *)This, riid, object);
}

static ULONG STDMETHODCALLTYPE device_parent_AddRef(IWineD3DDeviceParent *iface)
{
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    return IDirect3DDevice9Impl_AddRef((IDirect3DDevice9Ex *)This);
}

static ULONG STDMETHODCALLTYPE device_parent_Release(IWineD3DDeviceParent *iface)
{
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    return IDirect3DDevice9Impl_Release((IDirect3DDevice9Ex *)This);
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
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    IDirect3DSurface9Impl *d3d_surface;
    BOOL lockable = TRUE;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, usage %#x,\n"
            "\tpool %#x, level %u, face %u, surface %p\n",
            iface, superior, width, height, format, usage, pool, level, face, surface);

    if (pool == WINED3DPOOL_DEFAULT && !(usage & D3DUSAGE_DYNAMIC))
        lockable = FALSE;

    hr = IDirect3DDevice9Impl_CreateSurface((IDirect3DDevice9Ex *)This, width, height,
            d3dformat_from_wined3dformat(format), lockable, FALSE /* Discard */, level,
            (IDirect3DSurface9 **)&d3d_surface, usage, pool, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);
    if (FAILED(hr))
    {
        ERR("(%p) CreateSurface failed, returning %#x\n", iface, hr);
        return hr;
    }

    *surface = d3d_surface->wineD3DSurface;
    IWineD3DSurface_AddRef(*surface);

    d3d_surface->container = superior;
    IDirect3DDevice9Ex_Release(d3d_surface->parentDevice);
    d3d_surface->parentDevice = NULL;

    IDirect3DSurface9_Release((IDirect3DSurface9 *)d3d_surface);
    d3d_surface->forwardReference = superior;

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateRenderTarget(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, WINED3DMULTISAMPLE_TYPE multisample_type,
        DWORD multisample_quality, BOOL lockable, IWineD3DSurface **surface)
{
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    IDirect3DSurface9Impl *d3d_surface;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, lockable %u, surface %p\n",
            iface, superior, width, height, format, multisample_type, multisample_quality, lockable, surface);

    hr = IDirect3DDevice9Impl_CreateRenderTarget((IDirect3DDevice9Ex *)This, width, height,
            d3dformat_from_wined3dformat(format), multisample_type, multisample_quality, lockable,
            (IDirect3DSurface9 **)&d3d_surface, NULL);
    if (FAILED(hr))
    {
        ERR("(%p) CreateRenderTarget failed, returning %#x\n", iface, hr);
        return hr;
    }

    *surface = d3d_surface->wineD3DSurface;
    IWineD3DSurface_AddRef(*surface);

    d3d_surface->container = superior;
    /* Implicit surfaces are created with an refcount of 0 */
    IDirect3DSurface9_Release((IDirect3DSurface9 *)d3d_surface);

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateDepthStencilSurface(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, WINED3DFORMAT format, WINED3DMULTISAMPLE_TYPE multisample_type,
        DWORD multisample_quality, BOOL discard, IWineD3DSurface **surface)
{
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    IDirect3DSurface9Impl *d3d_surface;
    HRESULT hr;

    TRACE("iface %p, superior %p, width %u, height %u, format %#x, multisample_type %#x,\n"
            "\tmultisample_quality %u, discard %u, surface %p\n",
            iface, superior, width, height, format, multisample_type, multisample_quality, discard, surface);

    hr = IDirect3DDevice9Impl_CreateDepthStencilSurface((IDirect3DDevice9Ex *)This, width, height,
            d3dformat_from_wined3dformat(format), multisample_type, multisample_quality, discard,
            (IDirect3DSurface9 **)&d3d_surface, NULL);
    if (FAILED(hr))
    {
        ERR("(%p) CreateDepthStencilSurface failed, returning %#x\n", iface, hr);
        return hr;
    }

    *surface = d3d_surface->wineD3DSurface;
    IWineD3DSurface_AddRef(*surface);
    d3d_surface->container = (IUnknown *)This;
    /* Implicit surfaces are created with an refcount of 0 */
    IDirect3DSurface9_Release((IDirect3DSurface9 *)d3d_surface);

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateVolume(IWineD3DDeviceParent *iface,
        IUnknown *superior, UINT width, UINT height, UINT depth, WINED3DFORMAT format,
        WINED3DPOOL pool, DWORD usage, IWineD3DVolume **volume)
{
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    IDirect3DVolume9Impl *object;
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
    IDirect3DVolume9_Release((IDirect3DVolume9 *)object);

    object->container = superior;
    object->forwardReference = superior;

    TRACE("(%p) Created volume %p\n", iface, object);

    return hr;
}

static HRESULT STDMETHODCALLTYPE device_parent_CreateSwapChain(IWineD3DDeviceParent *iface,
        WINED3DPRESENT_PARAMETERS *present_parameters, IWineD3DSwapChain **swapchain)
{
    struct IDirect3DDevice9Impl *This = device_from_device_parent(iface);
    IDirect3DSwapChain9Impl *d3d_swapchain;
    D3DPRESENT_PARAMETERS local_parameters;
    HRESULT hr;

    TRACE("iface %p, present_parameters %p, swapchain %p\n", iface, present_parameters, swapchain);

    /* Copy the presentation parameters */
    local_parameters.BackBufferWidth = present_parameters->BackBufferWidth;
    local_parameters.BackBufferHeight = present_parameters->BackBufferHeight;
    local_parameters.BackBufferFormat = d3dformat_from_wined3dformat(present_parameters->BackBufferFormat);
    local_parameters.BackBufferCount = present_parameters->BackBufferCount;
    local_parameters.MultiSampleType = present_parameters->MultiSampleType;
    local_parameters.MultiSampleQuality = present_parameters->MultiSampleQuality;
    local_parameters.SwapEffect = present_parameters->SwapEffect;
    local_parameters.hDeviceWindow = present_parameters->hDeviceWindow;
    local_parameters.Windowed = present_parameters->Windowed;
    local_parameters.EnableAutoDepthStencil = present_parameters->EnableAutoDepthStencil;
    local_parameters.AutoDepthStencilFormat = d3dformat_from_wined3dformat(present_parameters->AutoDepthStencilFormat);
    local_parameters.Flags = present_parameters->Flags;
    local_parameters.FullScreen_RefreshRateInHz = present_parameters->FullScreen_RefreshRateInHz;
    local_parameters.PresentationInterval = present_parameters->PresentationInterval;

    hr = IDirect3DDevice9Impl_CreateAdditionalSwapChain((IDirect3DDevice9Ex *)This,
            &local_parameters, (IDirect3DSwapChain9 **)&d3d_swapchain);
    if (FAILED(hr))
    {
        ERR("(%p) CreateAdditionalSwapChain failed, returning %#x\n", iface, hr);
        *swapchain = NULL;
        return hr;
    }

    *swapchain = d3d_swapchain->wineD3DSwapChain;
    d3d_swapchain->isImplicit = TRUE;
    /* Implicit swap chains are created with an refcount of 0 */
    IDirect3DSwapChain9_Release((IDirect3DSwapChain9 *)d3d_swapchain);

    /* Copy back the presentation parameters */
    present_parameters->BackBufferWidth = local_parameters.BackBufferWidth;
    present_parameters->BackBufferHeight = local_parameters.BackBufferHeight;
    present_parameters->BackBufferFormat = wined3dformat_from_d3dformat(local_parameters.BackBufferFormat);
    present_parameters->BackBufferCount = local_parameters.BackBufferCount;
    present_parameters->MultiSampleType = local_parameters.MultiSampleType;
    present_parameters->MultiSampleQuality = local_parameters.MultiSampleQuality;
    present_parameters->SwapEffect = local_parameters.SwapEffect;
    present_parameters->hDeviceWindow = local_parameters.hDeviceWindow;
    present_parameters->Windowed = local_parameters.Windowed;
    present_parameters->EnableAutoDepthStencil = local_parameters.EnableAutoDepthStencil;
    present_parameters->AutoDepthStencilFormat = wined3dformat_from_d3dformat(local_parameters.AutoDepthStencilFormat);
    present_parameters->Flags = local_parameters.Flags;
    present_parameters->FullScreen_RefreshRateInHz = local_parameters.FullScreen_RefreshRateInHz;
    present_parameters->PresentationInterval = local_parameters.PresentationInterval;

    return hr;
}

static const IWineD3DDeviceParentVtbl d3d9_wined3d_device_parent_vtbl =
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

HRESULT device_init(IDirect3DDevice9Impl *device, IWineD3D *wined3d, UINT adapter, D3DDEVTYPE device_type,
        HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters)
{
    WINED3DPRESENT_PARAMETERS *wined3d_parameters;
    UINT i, count = 1;
    HRESULT hr;

    device->lpVtbl = &Direct3DDevice9_Vtbl;
    device->device_parent_vtbl = &d3d9_wined3d_device_parent_vtbl;
    device->ref = 1;

    wined3d_mutex_lock();
    hr = IWineD3D_CreateDevice(wined3d, adapter, device_type, focus_window, flags, (IUnknown *)device,
            (IWineD3DDeviceParent *)&device->device_parent_vtbl, &device->WineD3DDevice);
    if (FAILED(hr))
    {
        WARN("Failed to create wined3d device, hr %#x.\n", hr);
        wined3d_mutex_unlock();
        return hr;
    }

    if (flags & D3DCREATE_ADAPTERGROUP_DEVICE)
    {
        WINED3DCAPS caps;

        IWineD3D_GetDeviceCaps(wined3d, adapter, device_type, &caps);
        count = caps.NumberOfAdaptersInGroup;
    }

    if (flags & D3DCREATE_MULTITHREADED) IWineD3DDevice_SetMultithreaded(device->WineD3DDevice);

    wined3d_parameters = HeapAlloc(GetProcessHeap(), 0, sizeof(*wined3d_parameters) * count);
    if (!wined3d_parameters)
    {
        ERR("Failed to allocate wined3d parameters.\n");
        IWineD3DDevice_Release(device->WineD3DDevice);
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < count; ++i)
    {
        wined3d_parameters[i].BackBufferWidth = parameters[i].BackBufferWidth;
        wined3d_parameters[i].BackBufferHeight = parameters[i].BackBufferHeight;
        wined3d_parameters[i].BackBufferFormat = wined3dformat_from_d3dformat(parameters[i].BackBufferFormat);
        wined3d_parameters[i].BackBufferCount = parameters[i].BackBufferCount;
        wined3d_parameters[i].MultiSampleType = parameters[i].MultiSampleType;
        wined3d_parameters[i].MultiSampleQuality = parameters[i].MultiSampleQuality;
        wined3d_parameters[i].SwapEffect = parameters[i].SwapEffect;
        wined3d_parameters[i].hDeviceWindow = parameters[i].hDeviceWindow;
        wined3d_parameters[i].Windowed = parameters[i].Windowed;
        wined3d_parameters[i].EnableAutoDepthStencil = parameters[i].EnableAutoDepthStencil;
        wined3d_parameters[i].AutoDepthStencilFormat =
                wined3dformat_from_d3dformat(parameters[i].AutoDepthStencilFormat);
        wined3d_parameters[i].Flags = parameters[i].Flags;
        wined3d_parameters[i].FullScreen_RefreshRateInHz = parameters[i].FullScreen_RefreshRateInHz;
        wined3d_parameters[i].PresentationInterval = parameters[i].PresentationInterval;
        wined3d_parameters[i].AutoRestoreDisplayMode = TRUE;
    }

    hr = IWineD3DDevice_Init3D(device->WineD3DDevice, wined3d_parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize 3D, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, wined3d_parameters);
        IWineD3DDevice_Release(device->WineD3DDevice);
        wined3d_mutex_unlock();
        return hr;
    }

    wined3d_mutex_unlock();

    for (i = 0; i < count; ++i)
    {
        parameters[i].BackBufferWidth = wined3d_parameters[i].BackBufferWidth;
        parameters[i].BackBufferHeight = wined3d_parameters[i].BackBufferHeight;
        parameters[i].BackBufferFormat = d3dformat_from_wined3dformat(wined3d_parameters[i].BackBufferFormat);
        parameters[i].BackBufferCount = wined3d_parameters[i].BackBufferCount;
        parameters[i].MultiSampleType = wined3d_parameters[i].MultiSampleType;
        parameters[i].MultiSampleQuality = wined3d_parameters[i].MultiSampleQuality;
        parameters[i].SwapEffect = wined3d_parameters[i].SwapEffect;
        parameters[i].hDeviceWindow = wined3d_parameters[i].hDeviceWindow;
        parameters[i].Windowed = wined3d_parameters[i].Windowed;
        parameters[i].EnableAutoDepthStencil = wined3d_parameters[i].EnableAutoDepthStencil;
        parameters[i].AutoDepthStencilFormat =
                d3dformat_from_wined3dformat(wined3d_parameters[i].AutoDepthStencilFormat);
        parameters[i].Flags = wined3d_parameters[i].Flags;
        parameters[i].FullScreen_RefreshRateInHz = wined3d_parameters[i].FullScreen_RefreshRateInHz;
        parameters[i].PresentationInterval = wined3d_parameters[i].PresentationInterval;
    }
    HeapFree(GetProcessHeap(), 0, wined3d_parameters);

    /* Initialize the converted declaration array. This creates a valid pointer
     * and when adding decls HeapReAlloc() can be used without further checking. */
    device->convertedDecls = HeapAlloc(GetProcessHeap(), 0, 0);
    if (!device->convertedDecls)
    {
        ERR("Failed to allocate FVF vertex declaration map memory.\n");
        wined3d_mutex_lock();
        IWineD3DDevice_Uninit3D(device->WineD3DDevice, D3D9CB_DestroySwapChain);
        IWineD3DDevice_Release(device->WineD3DDevice);
        wined3d_mutex_unlock();
        return E_OUTOFMEMORY;
    }

    return D3D_OK;
}
