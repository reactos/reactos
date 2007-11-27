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

/* Shader handle functions */
static shader_handle *alloc_shader_handle(IDirect3DDevice8Impl *This) {
    if (This->free_shader_handles) {
        /* Use a free handle */
        shader_handle *handle = This->free_shader_handles;
        This->free_shader_handles = *handle;
        return handle;
    }
    if (!(This->allocated_shader_handles < This->shader_handle_table_size)) {
        /* Grow the table */
        DWORD new_size = This->shader_handle_table_size + (This->shader_handle_table_size >> 1);
        shader_handle *new_handles = HeapReAlloc(GetProcessHeap(), 0, This->shader_handles, new_size * sizeof(shader_handle));
        if (!new_handles) return NULL;
        This->shader_handles = new_handles;
        This->shader_handle_table_size = new_size;
    }

    return &This->shader_handles[This->allocated_shader_handles++];
}

static void free_shader_handle(IDirect3DDevice8Impl *This, shader_handle *handle) {
    *handle = This->free_shader_handles;
    This->free_shader_handles = handle;
}

/* IDirect3D IUnknown parts follow: */
static HRESULT WINAPI IDirect3DDevice8Impl_QueryInterface(LPDIRECT3DDEVICE8 iface,REFIID riid,LPVOID *ppobj)
{
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

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

static ULONG WINAPI IDirect3DDevice8Impl_AddRef(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3DDevice8Impl_Release(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    ULONG ref;

    if (This->inDestruction) return 0;
    ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        int i;

        TRACE("Releasing wined3d device %p\n", This->WineD3DDevice);
        EnterCriticalSection(&d3d8_cs);
        This->inDestruction = TRUE;

        for(i = 0; i < This->numConvertedDecls; i++) {
            IWineD3DVertexDeclaration_Release(This->decls[i].decl);
        }
        HeapFree(GetProcessHeap(), 0, This->decls);

        IWineD3DDevice_Uninit3D(This->WineD3DDevice, D3D8CB_DestroyDepthStencilSurface, D3D8CB_DestroySwapChain);
        IWineD3DDevice_Release(This->WineD3DDevice);
        HeapFree(GetProcessHeap(), 0, This->shader_handles);
        HeapFree(GetProcessHeap(), 0, This);
        LeaveCriticalSection(&d3d8_cs);
    }
    return ref;
}

/* IDirect3DDevice Interface follow: */
static HRESULT WINAPI IDirect3DDevice8Impl_TestCooperativeLevel(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("(%p) : Relay\n", This);
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_TestCooperativeLevel(This->WineD3DDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static UINT WINAPI  IDirect3DDevice8Impl_GetAvailableTextureMem(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("(%p) Relay\n", This);
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetAvailableTextureMem(This->WineD3DDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ResourceManagerDiscardBytes(LPDIRECT3DDEVICE8 iface, DWORD Bytes) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("(%p) : Relay bytes(%d)\n", This, Bytes);
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_EvictManagedResources(This->WineD3DDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDirect3D(LPDIRECT3DDEVICE8 iface, IDirect3D8** ppD3D8) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3D* pWineD3D;

    TRACE("(%p) Relay\n", This);

    if (NULL == ppD3D8) {
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetDirect3D(This->WineD3DDevice, &pWineD3D);
    if (hr == D3D_OK && pWineD3D != NULL)
    {
        IWineD3D_GetParent(pWineD3D,(IUnknown **)ppD3D8);
        IWineD3D_Release(pWineD3D);
    } else {
        FIXME("Call to IWineD3DDevice_GetDirect3D failed\n");
        *ppD3D8 = NULL;
    }
    TRACE("(%p) returning %p\n",This , *ppD3D8);
    LeaveCriticalSection(&d3d8_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDeviceCaps(LPDIRECT3DDEVICE8 iface, D3DCAPS8* pCaps) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("(%p) : Relay pCaps %p\n", This, pCaps);
    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /* well this is what MSDN says to return */
    }

    D3D8CAPSTOWINECAPS(pCaps, pWineCaps)
    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_GetDeviceCaps(This->WineD3DDevice, pWineCaps);
    LeaveCriticalSection(&d3d8_cs);
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    /* D3D8 doesn't support SM 2.0 or higher, so clamp to 1.x */
    if(pCaps->PixelShaderVersion > D3DPS_VERSION(1,4)){
        pCaps->PixelShaderVersion = D3DPS_VERSION(1,4);
    }
    if(pCaps->VertexShaderVersion > D3DVS_VERSION(1,1)){
        pCaps->VertexShaderVersion = D3DVS_VERSION(1,1);
    }

    TRACE("Returning %p %p\n", This, pCaps);
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetDisplayMode(LPDIRECT3DDEVICE8 iface, D3DDISPLAYMODE* pMode) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetDisplayMode(This->WineD3DDevice, 0, (WINED3DDISPLAYMODE *) pMode);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetCreationParameters(LPDIRECT3DDEVICE8 iface, D3DDEVICE_CREATION_PARAMETERS *pParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetCreationParameters(This->WineD3DDevice, (WINED3DDEVICE_CREATION_PARAMETERS *) pParameters);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetCursorProperties(LPDIRECT3DDEVICE8 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface8* pCursorBitmap) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *pSurface = (IDirect3DSurface8Impl*)pCursorBitmap;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);
    if(!pCursorBitmap) {
        WARN("No cursor bitmap, returning WINED3DERR_INVALIDCALL\n");
        return WINED3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetCursorProperties(This->WineD3DDevice,XHotSpot,YHotSpot,(IWineD3DSurface*)pSurface->wineD3DSurface);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static void WINAPI IDirect3DDevice8Impl_SetCursorPosition(LPDIRECT3DDEVICE8 iface, UINT XScreenSpace, UINT YScreenSpace, DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    IWineD3DDevice_SetCursorPosition(This->WineD3DDevice, XScreenSpace, YScreenSpace, Flags);
    LeaveCriticalSection(&d3d8_cs);
}

static BOOL WINAPI IDirect3DDevice8Impl_ShowCursor(LPDIRECT3DDEVICE8 iface, BOOL bShow) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    BOOL ret;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    ret = IWineD3DDevice_ShowCursor(This->WineD3DDevice, bShow);
    LeaveCriticalSection(&d3d8_cs);
    return ret;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateAdditionalSwapChain(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain8** pSwapChain) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSwapChain8Impl* object;
    HRESULT hrc = D3D_OK;
    WINED3DPRESENT_PARAMETERS localParameters;

    TRACE("(%p) Relay\n", This);

    /* Fix the back buffer count */
    if(pPresentationParameters->BackBufferCount == 0) {
        pPresentationParameters->BackBufferCount = 1;
    }

    object = HeapAlloc(GetProcessHeap(),  HEAP_ZERO_MEMORY, sizeof(*object));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *pSwapChain = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }
    object->ref = 1;
    object->lpVtbl = &Direct3DSwapChain8_Vtbl;

    /* Allocate an associated WineD3DDevice object */
    localParameters.BackBufferWidth                             = pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight                            = pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat                            = pPresentationParameters->BackBufferFormat;
    localParameters.BackBufferCount                             = pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                             = pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality                          = 0; /* d3d9 only */
    localParameters.SwapEffect                                  = pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                               = pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                                    = pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil                      = pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat                      = pPresentationParameters->AutoDepthStencilFormat;
    localParameters.Flags                                       = pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz                  = pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval                        = pPresentationParameters->FullScreen_PresentationInterval;

    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_CreateAdditionalSwapChain(This->WineD3DDevice, &localParameters, &object->wineD3DSwapChain, (IUnknown*)object, D3D8CB_CreateRenderTarget, D3D8CB_CreateDepthStencilSurface);
    LeaveCriticalSection(&d3d8_cs);

    pPresentationParameters->BackBufferWidth                    = localParameters.BackBufferWidth;
    pPresentationParameters->BackBufferHeight                   = localParameters.BackBufferHeight;
    pPresentationParameters->BackBufferFormat                   = localParameters.BackBufferFormat;
    pPresentationParameters->BackBufferCount                    = localParameters.BackBufferCount;
    pPresentationParameters->MultiSampleType                    = localParameters.MultiSampleType;
    pPresentationParameters->SwapEffect                         = localParameters.SwapEffect;
    pPresentationParameters->hDeviceWindow                      = localParameters.hDeviceWindow;
    pPresentationParameters->Windowed                           = localParameters.Windowed;
    pPresentationParameters->EnableAutoDepthStencil             = localParameters.EnableAutoDepthStencil;
    pPresentationParameters->AutoDepthStencilFormat             = localParameters.AutoDepthStencilFormat;
    pPresentationParameters->Flags                              = localParameters.Flags;
    pPresentationParameters->FullScreen_RefreshRateInHz         = localParameters.FullScreen_RefreshRateInHz;
    pPresentationParameters->FullScreen_PresentationInterval    = localParameters.PresentationInterval;

    if (hrc != D3D_OK) {
        FIXME("(%p) call to IWineD3DDevice_CreateAdditionalSwapChain failed\n", This);
        HeapFree(GetProcessHeap(), 0 , object);
        *pSwapChain = NULL;
    }else{
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *pSwapChain = (IDirect3DSwapChain8 *)object;
    }
    TRACE("(%p) returning %p\n", This, *pSwapChain);
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Reset(LPDIRECT3DDEVICE8 iface, D3DPRESENT_PARAMETERS* pPresentationParameters) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    WINED3DPRESENT_PARAMETERS localParameters;
    HRESULT hr;

    TRACE("(%p) Relay pPresentationParameters(%p)\n", This, pPresentationParameters);

    localParameters.BackBufferWidth                             = pPresentationParameters->BackBufferWidth;
    localParameters.BackBufferHeight                            = pPresentationParameters->BackBufferHeight;
    localParameters.BackBufferFormat                            = pPresentationParameters->BackBufferFormat;
    localParameters.BackBufferCount                             = pPresentationParameters->BackBufferCount;
    localParameters.MultiSampleType                             = pPresentationParameters->MultiSampleType;
    localParameters.MultiSampleQuality                          = 0; /* d3d9 only */
    localParameters.SwapEffect                                  = pPresentationParameters->SwapEffect;
    localParameters.hDeviceWindow                               = pPresentationParameters->hDeviceWindow;
    localParameters.Windowed                                    = pPresentationParameters->Windowed;
    localParameters.EnableAutoDepthStencil                      = pPresentationParameters->EnableAutoDepthStencil;
    localParameters.AutoDepthStencilFormat                      = pPresentationParameters->AutoDepthStencilFormat;
    localParameters.Flags                                       = pPresentationParameters->Flags;
    localParameters.FullScreen_RefreshRateInHz                  = pPresentationParameters->FullScreen_RefreshRateInHz;
    localParameters.PresentationInterval                        = pPresentationParameters->FullScreen_PresentationInterval;

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_Reset(This->WineD3DDevice, &localParameters);
    LeaveCriticalSection(&d3d8_cs);

    pPresentationParameters->BackBufferWidth                    = localParameters.BackBufferWidth;
    pPresentationParameters->BackBufferHeight                   = localParameters.BackBufferHeight;
    pPresentationParameters->BackBufferFormat                   = localParameters.BackBufferFormat;
    pPresentationParameters->BackBufferCount                    = localParameters.BackBufferCount;
    pPresentationParameters->MultiSampleType                    = localParameters.MultiSampleType;
    pPresentationParameters->SwapEffect                         = localParameters.SwapEffect;
    pPresentationParameters->hDeviceWindow                      = localParameters.hDeviceWindow;
    pPresentationParameters->Windowed                           = localParameters.Windowed;
    pPresentationParameters->EnableAutoDepthStencil             = localParameters.EnableAutoDepthStencil;
    pPresentationParameters->AutoDepthStencilFormat             = localParameters.AutoDepthStencilFormat;
    pPresentationParameters->Flags                              = localParameters.Flags;
    pPresentationParameters->FullScreen_RefreshRateInHz         = localParameters.FullScreen_RefreshRateInHz;
    pPresentationParameters->FullScreen_PresentationInterval    = localParameters.PresentationInterval;

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Present(LPDIRECT3DDEVICE8 iface, CONST RECT* pSourceRect,CONST RECT* pDestRect,HWND hDestWindowOverride,CONST RGNDATA* pDirtyRegion) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_Present(This->WineD3DDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetBackBuffer(LPDIRECT3DDEVICE8 iface, UINT BackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface8** ppBackBuffer) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DSurface *retSurface = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    rc = IWineD3DDevice_GetBackBuffer(This->WineD3DDevice, 0, BackBuffer, (WINED3DBACKBUFFER_TYPE) Type, (IWineD3DSurface **)&retSurface);
    if (rc == D3D_OK && NULL != retSurface && NULL != ppBackBuffer) {
        IWineD3DSurface_GetParent(retSurface, (IUnknown **)ppBackBuffer);
        IWineD3DSurface_Release(retSurface);
    }
    LeaveCriticalSection(&d3d8_cs);
    return rc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetRasterStatus(LPDIRECT3DDEVICE8 iface, D3DRASTER_STATUS* pRasterStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetRasterStatus(This->WineD3DDevice, 0, (WINED3DRASTER_STATUS *) pRasterStatus);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static void WINAPI IDirect3DDevice8Impl_SetGammaRamp(LPDIRECT3DDEVICE8 iface, DWORD Flags, CONST D3DGAMMARAMP* pRamp) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    EnterCriticalSection(&d3d8_cs);
    IWineD3DDevice_SetGammaRamp(This->WineD3DDevice, 0, Flags, (CONST WINED3DGAMMARAMP *) pRamp);
    LeaveCriticalSection(&d3d8_cs);
}

static void WINAPI IDirect3DDevice8Impl_GetGammaRamp(LPDIRECT3DDEVICE8 iface, D3DGAMMARAMP* pRamp) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);

    /* Note: D3DGAMMARAMP is compatible with WINED3DGAMMARAMP */
    EnterCriticalSection(&d3d8_cs);
    IWineD3DDevice_GetGammaRamp(This->WineD3DDevice, 0, (WINED3DGAMMARAMP *) pRamp);
    LeaveCriticalSection(&d3d8_cs);
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateTexture(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage,
                                                    D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture8 **ppTexture) {
    IDirect3DTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : W(%d) H(%d), Lvl(%d) d(%d), Fmt(%u), Pool(%d)\n", This, Width, Height, Levels, Usage, Format,  Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DTexture8Impl));

    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
/*        *ppTexture = NULL; */
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DTexture8_Vtbl;
    object->ref = 1;
    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_CreateTexture(This->WineD3DDevice, Width, Height, Levels, Usage & WINED3DUSAGE_MASK,
                                 (WINED3DFORMAT)Format, (WINED3DPOOL) Pool, &object->wineD3DTexture, NULL, (IUnknown *)object, D3D8CB_CreateSurface);
    LeaveCriticalSection(&d3d8_cs);

    if (FAILED(hrc)) {
        /* free up object */ 
        FIXME("(%p) call to IWineD3DDevice_CreateTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
/*      *ppTexture = NULL; */
   } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *ppTexture = (LPDIRECT3DTEXTURE8) object;
   }

   TRACE("(%p) Created Texture %p, %p\n",This,object,object->wineD3DTexture);
   return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVolumeTexture(LPDIRECT3DDEVICE8 iface, 
                                                          UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, 
                                                          D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture8** ppVolumeTexture) {

    IDirect3DVolumeTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVolumeTexture8Impl));
    if (NULL == object) {
        FIXME("(%p) allocation of memory failed\n", This);
        *ppVolumeTexture = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVolumeTexture8_Vtbl;
    object->ref = 1;
    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_CreateVolumeTexture(This->WineD3DDevice, Width, Height, Depth, Levels, Usage & WINED3DUSAGE_MASK,
                                 (WINED3DFORMAT)Format, (WINED3DPOOL) Pool, &object->wineD3DVolumeTexture, NULL,
                                 (IUnknown *)object, D3D8CB_CreateVolume);
    LeaveCriticalSection(&d3d8_cs);

    if (hrc != D3D_OK) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateVolumeTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVolumeTexture = NULL;
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *ppVolumeTexture = (LPDIRECT3DVOLUMETEXTURE8) object;
    }
    TRACE("(%p)  returning %p\n", This , *ppVolumeTexture);
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateCubeTexture(LPDIRECT3DDEVICE8 iface, UINT EdgeLength, UINT Levels, DWORD Usage, 
                                                        D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture8** ppCubeTexture) {

    IDirect3DCubeTexture8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : ELen(%d) Lvl(%d) Usage(%d) fmt(%u), Pool(%d)\n" , This, EdgeLength, Levels, Usage, Format, Pool);

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));

    if (NULL == object) {
        FIXME("(%p) allocation of CubeTexture failed\n", This);
        *ppCubeTexture = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DCubeTexture8_Vtbl;
    object->ref = 1;
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_CreateCubeTexture(This->WineD3DDevice, EdgeLength, Levels, Usage & WINED3DUSAGE_MASK,
                                 (WINED3DFORMAT)Format, (WINED3DPOOL) Pool, &object->wineD3DCubeTexture, NULL, (IUnknown*)object,
                                 D3D8CB_CreateSurface);
    LeaveCriticalSection(&d3d8_cs);

    if (hr != D3D_OK){

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateCubeTexture failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppCubeTexture = NULL;
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *ppCubeTexture = (LPDIRECT3DCUBETEXTURE8) object;
    }

    TRACE("(%p) returning %p\n",This, *ppCubeTexture);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexBuffer(LPDIRECT3DDEVICE8 iface, UINT Size, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer8** ppVertexBuffer) {
    IDirect3DVertexBuffer8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);
    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DVertexBuffer8Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppVertexBuffer = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DVertexBuffer8_Vtbl;
    object->ref = 1;
    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_CreateVertexBuffer(This->WineD3DDevice, Size, Usage & WINED3DUSAGE_MASK, FVF, (WINED3DPOOL) Pool, &(object->wineD3DVertexBuffer), NULL, (IUnknown *)object);
    LeaveCriticalSection(&d3d8_cs);

    if (D3D_OK != hrc) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateVertexBuffer failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppVertexBuffer = NULL;
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *ppVertexBuffer = (LPDIRECT3DVERTEXBUFFER8) object;
    }
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateIndexBuffer(LPDIRECT3DDEVICE8 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer8** ppIndexBuffer) {
    IDirect3DIndexBuffer8Impl *object;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) Relay\n", This);
    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppIndexBuffer = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DIndexBuffer8_Vtbl;
    object->ref = 1;
    TRACE("Calling wined3d create index buffer\n");
    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_CreateIndexBuffer(This->WineD3DDevice, Length, Usage & WINED3DUSAGE_MASK, Format, (WINED3DPOOL) Pool, &object->wineD3DIndexBuffer, NULL, (IUnknown *)object);
    LeaveCriticalSection(&d3d8_cs);

    if (D3D_OK != hrc) {

        /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateIndexBuffer failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppIndexBuffer = NULL;
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *ppIndexBuffer = (LPDIRECT3DINDEXBUFFER8)object;
    }
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, BOOL Lockable, BOOL Discard, UINT Level, IDirect3DSurface8 **ppSurface,D3DRESOURCETYPE Type, UINT Usage,D3DPOOL Pool, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality)  {
    HRESULT hrc;
    IDirect3DSurface8Impl *object;
    IDirect3DDevice8Impl  *This = (IDirect3DDevice8Impl *)iface;
    TRACE("(%p) Relay\n", This);
    if(MultisampleQuality < 0) { 
        FIXME("MultisampleQuality out of range %d, substituting 0\n", MultisampleQuality);
        /*FIXME: Find out what windows does with a MultisampleQuality < 0 */
        MultisampleQuality=0;
    }

    if(MultisampleQuality > 0){
        FIXME("MultisampleQuality set to %d, substituting 0\n" , MultisampleQuality);
        /*
        MultisampleQuality
        [in] Quality level. The valid range is between zero and one less than the level returned by pQualityLevels used by IDirect3D8::CheckDeviceMultiSampleType. Passing a larger value returns the error D3DERR_INVALIDCALL. The MultisampleQuality values of paired render targets, depth stencil surfaces, and the MultiSample type must all match.
        */
        MultisampleQuality=0;
    }
    /*FIXME: Check MAX bounds of MultisampleQuality*/

    /* Allocate the storage for the device */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DSurface8Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppSurface = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DSurface8_Vtbl;
    object->ref = 1;

    TRACE("(%p) : w(%d) h(%d) fmt(%d) surf@%p\n", This, Width, Height, Format, *ppSurface);

    /* Not called from the VTable, no locking needed */
    hrc = IWineD3DDevice_CreateSurface(This->WineD3DDevice, Width, Height, Format, Lockable, Discard, Level,  &object->wineD3DSurface, Type, Usage & WINED3DUSAGE_MASK, (WINED3DPOOL) Pool,MultiSample,MultisampleQuality, NULL, SURFACE_OPENGL, (IUnknown *)object);
    if (hrc != D3D_OK || NULL == object->wineD3DSurface) {
       /* free up object */
        FIXME("(%p) call to IWineD3DDevice_CreateSurface failed\n", This);
        HeapFree(GetProcessHeap(), 0, object);
        *ppSurface = NULL;
    } else {
        IUnknown_AddRef(iface);
        object->parentDevice = iface;
        *ppSurface = (LPDIRECT3DSURFACE8) object;
    }
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateRenderTarget(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, BOOL Lockable, IDirect3DSurface8** ppSurface) {
    HRESULT hr;
    TRACE("Relay\n");

    EnterCriticalSection(&d3d8_cs);
    hr = IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, Lockable, FALSE /* Discard */, 0 /* Level */ , ppSurface, D3DRTYPE_SURFACE, D3DUSAGE_RENDERTARGET, D3DPOOL_DEFAULT, MultiSample, 0);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateDepthStencilSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, IDirect3DSurface8** ppSurface) {
    HRESULT hr;
    TRACE("Relay\n");

    /* TODO: Verify that Discard is false */
    EnterCriticalSection(&d3d8_cs);
    hr = IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Lockable */, FALSE, 0 /* Level */
                                               ,ppSurface, D3DRTYPE_SURFACE, D3DUSAGE_DEPTHSTENCIL,
                                                D3DPOOL_DEFAULT, MultiSample, 0);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateImageSurface(LPDIRECT3DDEVICE8 iface, UINT Width, UINT Height, D3DFORMAT Format, IDirect3DSurface8** ppSurface) {
    HRESULT hr;
    TRACE("Relay\n");

    EnterCriticalSection(&d3d8_cs);
    hr = IDirect3DDevice8Impl_CreateSurface(iface, Width, Height, Format, TRUE /* Loackable */ , FALSE /*Discard*/ , 0 /* Level */ , ppSurface, D3DRTYPE_SURFACE, 0 /* Usage (undefined/none) */ , D3DPOOL_SCRATCH, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CopyRects(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8 *pSourceSurface, CONST RECT *pSourceRects, UINT cRects, IDirect3DSurface8 *pDestinationSurface, CONST POINT *pDestPoints) {
    IDirect3DSurface8Impl *Source = (IDirect3DSurface8Impl *) pSourceSurface;
    IDirect3DSurface8Impl *Dest = (IDirect3DSurface8Impl *) pDestinationSurface;

    HRESULT              hr = WINED3D_OK;
    WINED3DFORMAT        srcFormat, destFormat;
    UINT                 srcWidth,  destWidth;
    UINT                 srcHeight, destHeight;
    UINT                 srcSize;
    WINED3DSURFACE_DESC  winedesc;

    TRACE("(%p) pSrcSur=%p, pSourceRects=%p, cRects=%d, pDstSur=%p, pDestPtsArr=%p\n", iface,
          pSourceSurface, pSourceRects, cRects, pDestinationSurface, pDestPoints);


    /* Check that the source texture is in WINED3DPOOL_SYSTEMMEM and the destination texture is in WINED3DPOOL_DEFAULT */
    memset(&winedesc, 0, sizeof(winedesc));

    winedesc.Format = &srcFormat;
    winedesc.Width  = &srcWidth;
    winedesc.Height = &srcHeight;
    winedesc.Size   = &srcSize;
    IWineD3DSurface_GetDesc(Source->wineD3DSurface, &winedesc);

    winedesc.Format = &destFormat;
    winedesc.Width  = &destWidth;
    winedesc.Height = &destHeight;
    winedesc.Size   = NULL;
    EnterCriticalSection(&d3d8_cs);
    IWineD3DSurface_GetDesc(Dest->wineD3DSurface, &winedesc);

    /* Check that the source and destination formats match */
    if (srcFormat != destFormat && WINED3DFMT_UNKNOWN != destFormat) {
        WARN("(%p) source %p format must match the dest %p format, returning WINED3DERR_INVALIDCALL\n", iface, pSourceSurface, pDestinationSurface);
        LeaveCriticalSection(&d3d8_cs);
        return WINED3DERR_INVALIDCALL;
    } else if (WINED3DFMT_UNKNOWN == destFormat) {
        TRACE("(%p) : Converting destination surface from WINED3DFMT_UNKNOWN to the source format\n", iface);
        IWineD3DSurface_SetFormat(Dest->wineD3DSurface, srcFormat);
        destFormat = srcFormat;
    }

    /* Quick if complete copy ... */
    if (cRects == 0 && pSourceRects == NULL && pDestPoints == NULL) {
        IWineD3DSurface_BltFast(Dest->wineD3DSurface, 0, 0, Source->wineD3DSurface, NULL, WINEDDBLTFAST_NOCOLORKEY);
    } else {
        unsigned int i;
        /* Copy rect by rect */
        if (NULL != pSourceRects && NULL != pDestPoints) {
            for (i = 0; i < cRects; ++i) {
                IWineD3DSurface_BltFast(Dest->wineD3DSurface, pDestPoints[i].x, pDestPoints[i].y, Source->wineD3DSurface, (RECT *) &pSourceRects[i], WINEDDBLTFAST_NOCOLORKEY);
            }
        } else {
            for (i = 0; i < cRects; ++i) {
                IWineD3DSurface_BltFast(Dest->wineD3DSurface, 0, 0, Source->wineD3DSurface, (RECT *) &pSourceRects[i], WINEDDBLTFAST_NOCOLORKEY);
            }
        }
    }
    LeaveCriticalSection(&d3d8_cs);

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_UpdateTexture(LPDIRECT3DDEVICE8 iface, IDirect3DBaseTexture8* pSourceTexture, IDirect3DBaseTexture8* pDestinationTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_UpdateTexture(This->WineD3DDevice,  ((IDirect3DBaseTexture8Impl *)pSourceTexture)->wineD3DBaseTexture, ((IDirect3DBaseTexture8Impl *)pDestinationTexture)->wineD3DBaseTexture);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetFrontBuffer(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pDestSurface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *destSurface = (IDirect3DSurface8Impl *)pDestSurface;
    HRESULT hr;

    TRACE("(%p) Relay\n" , This);

    if (pDestSurface == NULL) {
        WARN("(%p) : Caller passed NULL as pDestSurface returning D3DERR_INVALIDCALL\n", This);
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetFrontBufferData(This->WineD3DDevice, 0, destSurface->wineD3DSurface);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8* pRenderTarget, IDirect3DSurface8* pNewZStencil) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DSurface8Impl *pSurface = (IDirect3DSurface8Impl *)pRenderTarget;
    IDirect3DSurface8Impl *pZSurface = (IDirect3DSurface8Impl *)pNewZStencil;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    IWineD3DDevice_SetDepthStencilSurface(This->WineD3DDevice, NULL == pZSurface ? NULL : (IWineD3DSurface *)pZSurface->wineD3DSurface);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetRenderTarget(This->WineD3DDevice, 0, pSurface ? (IWineD3DSurface *)pSurface->wineD3DSurface : NULL);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetRenderTarget(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppRenderTarget) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pRenderTarget;

    TRACE("(%p) Relay\n" , This);

    if (ppRenderTarget == NULL) {
        return D3DERR_INVALIDCALL;
    }
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetRenderTarget(This->WineD3DDevice, 0, &pRenderTarget);

    if (hr == D3D_OK && pRenderTarget != NULL) {
        IWineD3DSurface_GetParent(pRenderTarget,(IUnknown**)ppRenderTarget);
        IWineD3DSurface_Release(pRenderTarget);
    } else {
        FIXME("Call to IWineD3DDevice_GetRenderTarget failed\n");
        *ppRenderTarget = NULL;
    }
    LeaveCriticalSection(&d3d8_cs);

    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetDepthStencilSurface(LPDIRECT3DDEVICE8 iface, IDirect3DSurface8** ppZStencilSurface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr = D3D_OK;
    IWineD3DSurface *pZStencilSurface;

    TRACE("(%p) Relay\n" , This);
    if(ppZStencilSurface == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    hr=IWineD3DDevice_GetDepthStencilSurface(This->WineD3DDevice,&pZStencilSurface);
    if(hr == D3D_OK && pZStencilSurface != NULL){
        IWineD3DSurface_GetParent(pZStencilSurface,(IUnknown**)ppZStencilSurface);
        IWineD3DSurface_Release(pZStencilSurface);
    }else{
        FIXME("Call to IWineD3DDevice_GetDepthStencilSurface failed\n");
        *ppZStencilSurface = NULL;
    }
    LeaveCriticalSection(&d3d8_cs);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_BeginScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_BeginScene(This->WineD3DDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_EndScene(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_EndScene(This->WineD3DDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_Clear(LPDIRECT3DDEVICE8 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DRECT is compatible with WINED3DRECT */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_Clear(This->WineD3DDevice, Count, (CONST WINED3DRECT*) pRects, Flags, Color, Z, Stencil);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* lpMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) lpMatrix);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State,D3DMATRIX* pMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetTransform(This->WineD3DDevice, State, (WINED3DMATRIX*) pMatrix);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_MultiplyTransform(LPDIRECT3DDEVICE8 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATRIX is compatible with WINED3DMATRIX */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_MultiplyTransform(This->WineD3DDevice, State, (CONST WINED3DMATRIX*) pMatrix);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetViewport(LPDIRECT3DDEVICE8 iface, CONST D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DVIEWPORT8 is compatible with WINED3DVIEWPORT */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetViewport(This->WineD3DDevice, (const WINED3DVIEWPORT *)pViewport);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetViewport(LPDIRECT3DDEVICE8 iface, D3DVIEWPORT8* pViewport) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DVIEWPORT8 is compatible with WINED3DVIEWPORT */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetViewport(This->WineD3DDevice, (WINED3DVIEWPORT *)pViewport);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetMaterial(LPDIRECT3DDEVICE8 iface, CONST D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATERIAL8 is compatible with WINED3DMATERIAL */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetMaterial(This->WineD3DDevice, (const WINED3DMATERIAL *)pMaterial);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetMaterial(LPDIRECT3DDEVICE8 iface, D3DMATERIAL8* pMaterial) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DMATERIAL8 is compatible with WINED3DMATERIAL */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetMaterial(This->WineD3DDevice, (WINED3DMATERIAL *)pMaterial);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetLight(LPDIRECT3DDEVICE8 iface, DWORD Index, CONST D3DLIGHT8* pLight) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);
 
    /* Note: D3DLIGHT8 is compatible with WINED3DLIGHT */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetLight(This->WineD3DDevice, Index, (const WINED3DLIGHT *)pLight);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetLight(LPDIRECT3DDEVICE8 iface, DWORD Index,D3DLIGHT8* pLight) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    /* Note: D3DLIGHT8 is compatible with WINED3DLIGHT */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetLight(This->WineD3DDevice, Index, (WINED3DLIGHT *)pLight);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_LightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL Enable) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetLightEnable(This->WineD3DDevice, Index, Enable);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetLightEnable(LPDIRECT3DDEVICE8 iface, DWORD Index,BOOL* pEnable) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetLightEnable(This->WineD3DDevice, Index, pEnable);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,CONST float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetClipPlane(This->WineD3DDevice, Index, pPlane);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetClipPlane(LPDIRECT3DDEVICE8 iface, DWORD Index,float* pPlane) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetClipPlane(This->WineD3DDevice, Index, pPlane);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD Value) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetRenderState(This->WineD3DDevice, State, Value);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetRenderState(LPDIRECT3DDEVICE8 iface, D3DRENDERSTATETYPE State,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetRenderState(This->WineD3DDevice, State, pValue);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_BeginStateBlock(LPDIRECT3DDEVICE8 iface) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p)\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_BeginStateBlock(This->WineD3DDevice);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_EndStateBlock(LPDIRECT3DDEVICE8 iface, DWORD* pToken) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    IWineD3DStateBlock* wineD3DStateBlock;
    IDirect3DStateBlock8Impl* object;

    TRACE("(%p) Relay\n", This);

    /* Tell wineD3D to endstatablock before anything else (in case we run out
     * of memory later and cause locking problems)
     */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_EndStateBlock(This->WineD3DDevice , &wineD3DStateBlock);
    if (hr != D3D_OK) {
        FIXME("IWineD3DDevice_EndStateBlock returned an error\n");
        LeaveCriticalSection(&d3d8_cs);
        return hr;
    }

    /* allocate a new IDirectD3DStateBlock */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY ,sizeof(IDirect3DStateBlock8Impl));
    object->ref = 1;
    object->lpVtbl = &Direct3DStateBlock8_Vtbl;

    object->wineD3DStateBlock = wineD3DStateBlock;

    *pToken = (DWORD)object;
    TRACE("(%p)Returning %p %p\n", This, object, wineD3DStateBlock);

    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ApplyStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DStateBlock8Impl *pSB  = (IDirect3DStateBlock8Impl*) Token;
    IDirect3DDevice8Impl     *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("(%p) %p Relay\n", This, pSB);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DStateBlock_Apply(pSB->wineD3DStateBlock);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CaptureStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DStateBlock8Impl* pSB = (IDirect3DStateBlock8Impl *)Token;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;

    TRACE("(%p) %p Relay\n", This, pSB);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DStateBlock_Capture(pSB->wineD3DStateBlock);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeleteStateBlock(LPDIRECT3DDEVICE8 iface, DWORD Token) {
    IDirect3DStateBlock8Impl* pSB = (IDirect3DStateBlock8Impl *)Token;
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    while(IUnknown_Release((IUnknown *)pSB));
    LeaveCriticalSection(&d3d8_cs);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateStateBlock(LPDIRECT3DDEVICE8 iface, D3DSTATEBLOCKTYPE Type, DWORD* pToken) {
   IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
   IDirect3DStateBlock8Impl *object;
   HRESULT hrc = D3D_OK;

   TRACE("(%p) Relay\n", This);

   if(Type != D3DSBT_ALL         && Type != D3DSBT_PIXELSTATE &&
      Type != D3DSBT_VERTEXSTATE                              ) {
       WARN("Unexpected stateblock type, returning D3DERR_INVALIDCALL\n");
       return D3DERR_INVALIDCALL;
   }

   object  = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DStateBlock8Impl));
   if (NULL == object) {
      *pToken = 0;
      return E_OUTOFMEMORY;
   }
   object->lpVtbl = &Direct3DStateBlock8_Vtbl;
   object->ref = 1;

   EnterCriticalSection(&d3d8_cs);
   hrc = IWineD3DDevice_CreateStateBlock(This->WineD3DDevice, (WINED3DSTATEBLOCKTYPE)Type, &object->wineD3DStateBlock, (IUnknown *)object);
   LeaveCriticalSection(&d3d8_cs);
   if(D3D_OK != hrc){
       FIXME("(%p) Call to IWineD3DDevice_CreateStateBlock failed.\n", This);
       HeapFree(GetProcessHeap(), 0, object);
       *pToken = 0;
   } else {
       *pToken = (DWORD)object;
   }
   TRACE("(%p) returning token (ptr to stateblock) of %p\n", This, object);

   return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetClipStatus(LPDIRECT3DDEVICE8 iface, CONST D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);
/* FIXME: Verify that D3DCLIPSTATUS8 ~= WINED3DCLIPSTATUS */
    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetClipStatus(This->WineD3DDevice, (const WINED3DCLIPSTATUS *)pClipStatus);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetClipStatus(LPDIRECT3DDEVICE8 iface, D3DCLIPSTATUS8* pClipStatus) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetClipStatus(This->WineD3DDevice, (WINED3DCLIPSTATUS *)pClipStatus);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage,IDirect3DBaseTexture8** ppTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DBaseTexture *retTexture = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n" , This);

    if(ppTexture == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    rc = IWineD3DDevice_GetTexture(This->WineD3DDevice, Stage, (IWineD3DBaseTexture **)&retTexture);
    if (rc == D3D_OK && NULL != retTexture) {
        IWineD3DBaseTexture_GetParent(retTexture, (IUnknown **)ppTexture);
        IWineD3DBaseTexture_Release(retTexture);
    } else {
        FIXME("Call to get texture  (%d) failed (%p)\n", Stage, retTexture);
        *ppTexture = NULL;
    }
    LeaveCriticalSection(&d3d8_cs);

    return rc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTexture(LPDIRECT3DDEVICE8 iface, DWORD Stage, IDirect3DBaseTexture8* pTexture) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay %d %p\n" , This, Stage, pTexture);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetTexture(This->WineD3DDevice, Stage,
                                   pTexture==NULL ? NULL : ((IDirect3DBaseTexture8Impl *)pTexture)->wineD3DBaseTexture);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage,D3DTEXTURESTAGESTATETYPE Type,DWORD* pValue) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    switch(Type) {
    case D3DTSS_ADDRESSU:
        Type = WINED3DSAMP_ADDRESSU;
        break;
    case D3DTSS_ADDRESSV:
        Type = WINED3DSAMP_ADDRESSV;
        break;
    case D3DTSS_ADDRESSW:
        Type = WINED3DSAMP_ADDRESSW;
        break;
    case D3DTSS_BORDERCOLOR:
        Type = WINED3DSAMP_BORDERCOLOR;
        break;
    case D3DTSS_MAGFILTER:
        Type = WINED3DSAMP_MAGFILTER;
        break;
    case D3DTSS_MAXANISOTROPY:
        Type = WINED3DSAMP_MAXANISOTROPY;
        break;
    case D3DTSS_MAXMIPLEVEL:
        Type = WINED3DSAMP_MAXMIPLEVEL;
        break;
    case D3DTSS_MINFILTER:
        Type = WINED3DSAMP_MINFILTER;
        break;
    case D3DTSS_MIPFILTER:
        Type = WINED3DSAMP_MIPFILTER;
        break;
    case D3DTSS_MIPMAPLODBIAS:
        Type = WINED3DSAMP_MIPMAPLODBIAS;
        break;
    default:
        EnterCriticalSection(&d3d8_cs);
        hr = IWineD3DDevice_GetTextureStageState(This->WineD3DDevice, Stage, Type, pValue);
        LeaveCriticalSection(&d3d8_cs);
        return hr;
    }

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetSamplerState(This->WineD3DDevice, Stage, Type, pValue);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetTextureStageState(LPDIRECT3DDEVICE8 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    switch(Type) {
    case D3DTSS_ADDRESSU:
        Type = WINED3DSAMP_ADDRESSU;
        break;
    case D3DTSS_ADDRESSV:
        Type = WINED3DSAMP_ADDRESSV;
        break;
    case D3DTSS_ADDRESSW:
        Type = WINED3DSAMP_ADDRESSW;
        break;
    case D3DTSS_BORDERCOLOR:
        Type = WINED3DSAMP_BORDERCOLOR;
        break;
    case D3DTSS_MAGFILTER:
        Type = WINED3DSAMP_MAGFILTER;
        break;
    case D3DTSS_MAXANISOTROPY:
        Type = WINED3DSAMP_MAXANISOTROPY;
        break;
    case D3DTSS_MAXMIPLEVEL:
        Type = WINED3DSAMP_MAXMIPLEVEL;
        break;
    case D3DTSS_MINFILTER:
        Type = WINED3DSAMP_MINFILTER;
        break;
    case D3DTSS_MIPFILTER:
        Type = WINED3DSAMP_MIPFILTER;
        break;
    case D3DTSS_MIPMAPLODBIAS:
        Type = WINED3DSAMP_MIPMAPLODBIAS;
        break;
    default:
        EnterCriticalSection(&d3d8_cs);
        hr = IWineD3DDevice_SetTextureStageState(This->WineD3DDevice, Stage, Type, Value);
        LeaveCriticalSection(&d3d8_cs);
        return hr;
    }

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetSamplerState(This->WineD3DDevice, Stage, Type, Value);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ValidateDevice(LPDIRECT3DDEVICE8 iface, DWORD* pNumPasses) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_ValidateDevice(This->WineD3DDevice, pNumPasses);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetInfo(LPDIRECT3DDEVICE8 iface, DWORD DevInfoID, void* pDevInfoStruct, DWORD DevInfoStructSize) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    FIXME("(%p) : stub\n", This);
    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPaletteEntries(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber, PALETTEENTRY* pEntries) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetPaletteEntries(This->WineD3DDevice, PaletteNumber, pEntries);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_GetCurrentTexturePalette(LPDIRECT3DDEVICE8 iface, UINT *PaletteNumber) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetCurrentTexturePalette(This->WineD3DDevice, PaletteNumber);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface; 
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_DrawPrimitive(This->WineD3DDevice, PrimitiveType, StartVertex, PrimitiveCount);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitive(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,
                                                           UINT MinVertexIndex,UINT NumVertices,UINT startIndex,UINT primCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_DrawIndexedPrimitive(This->WineD3DDevice, PrimitiveType, MinVertexIndex, NumVertices, startIndex, primCount);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT PrimitiveCount,CONST void* pVertexStreamZeroData,UINT VertexStreamZeroStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_DrawPrimitiveUP(This->WineD3DDevice, PrimitiveType, PrimitiveCount, pVertexStreamZeroData, VertexStreamZeroStride);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE8 iface, D3DPRIMITIVETYPE PrimitiveType,UINT MinVertexIndex,
                                                             UINT NumVertexIndices,UINT PrimitiveCount,CONST void* pIndexData,
                                                             D3DFORMAT IndexDataFormat,CONST void* pVertexStreamZeroData,
                                                             UINT VertexStreamZeroStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_DrawIndexedPrimitiveUP(This->WineD3DDevice, PrimitiveType, MinVertexIndex, NumVertexIndices, PrimitiveCount,
                                               pIndexData, IndexDataFormat, pVertexStreamZeroData, VertexStreamZeroStride);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_ProcessVertices(LPDIRECT3DDEVICE8 iface, UINT SrcStartIndex,UINT DestIndex,UINT VertexCount,IDirect3DVertexBuffer8* pDestBuffer,DWORD Flags) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_ProcessVertices(This->WineD3DDevice,SrcStartIndex, DestIndex, VertexCount, ((IDirect3DVertexBuffer8Impl *)pDestBuffer)->wineD3DVertexBuffer, NULL, Flags);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexDeclaration(IDirect3DDevice8 *iface, CONST DWORD *declaration, IDirect3DVertexDeclaration8 **decl_ptr) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DVertexDeclaration8Impl *object;
    WINED3DVERTEXELEMENT *wined3d_elements;
    size_t wined3d_element_count;
    HRESULT hr = D3D_OK;

    TRACE("(%p) : declaration %p\n", This, declaration);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object) {
        ERR("Memory allocation failed\n");
        *decl_ptr = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->ref_count = 1;
    object->lpVtbl = &Direct3DVertexDeclaration8_Vtbl;

    wined3d_element_count = convert_to_wined3d_declaration(declaration, &object->elements_size, &wined3d_elements);
    object->elements = HeapAlloc(GetProcessHeap(), 0, object->elements_size);
    if (!object->elements) {
        ERR("Memory allocation failed\n");
        HeapFree(GetProcessHeap(), 0, wined3d_elements);
        HeapFree(GetProcessHeap(), 0, object);
        *decl_ptr = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    CopyMemory(object->elements, declaration, object->elements_size);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_CreateVertexDeclaration(This->WineD3DDevice, &object->wined3d_vertex_declaration,
            (IUnknown *)object, wined3d_elements, wined3d_element_count);
    LeaveCriticalSection(&d3d8_cs);
    HeapFree(GetProcessHeap(), 0, wined3d_elements);

    if (FAILED(hr)) {
        ERR("(%p) : IWineD3DDevice_CreateVertexDeclaration call failed\n", This);
        HeapFree(GetProcessHeap(), 0, object->elements);
        HeapFree(GetProcessHeap(), 0, object);
    } else {
        *decl_ptr = (IDirect3DVertexDeclaration8 *)object;
        TRACE("(%p) : Created vertex declaration %p\n", This, object);
    }

    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_CreateVertexShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pDeclaration, CONST DWORD* pFunction, DWORD* ppShader, DWORD Usage) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;
    IDirect3DVertexShader8Impl *object;
    IWineD3DVertexDeclaration *wined3d_vertex_declaration;

    /* Setup a stub object for now */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    TRACE("(%p) : pFunction(%p), ppShader(%p)\n", This, pFunction, ppShader);
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppShader = 0;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->ref = 1;
    object->lpVtbl = &Direct3DVertexShader8_Vtbl;

    EnterCriticalSection(&d3d8_cs);
    hrc = IDirect3DDevice8Impl_CreateVertexDeclaration(iface, pDeclaration, &object->vertex_declaration);
    if (FAILED(hrc)) {
        ERR("(%p) : IDirect3DDeviceImpl_CreateVertexDeclaration call failed\n", This);
        LeaveCriticalSection(&d3d8_cs);
        HeapFree(GetProcessHeap(), 0, object);
        *ppShader = 0;
        return D3DERR_INVALIDCALL;
    }
    wined3d_vertex_declaration = ((IDirect3DVertexDeclaration8Impl *)object->vertex_declaration)->wined3d_vertex_declaration;

    /* Usage is missing ... Use SetRenderState to set the sw vp render state in SetVertexShader */
    hrc = IWineD3DDevice_CreateVertexShader(This->WineD3DDevice, wined3d_vertex_declaration, pFunction, &object->wineD3DVertexShader, (IUnknown *)object);

    if (FAILED(hrc)) {
        /* free up object */
        FIXME("Call to IWineD3DDevice_CreateVertexShader failed\n");
        HeapFree(GetProcessHeap(), 0, object);
        *ppShader = 0;
    } else {
        /* TODO: Store the VS declarations locally so that they can be derefferenced with a value higher than VS_HIGHESTFIXEDFXF */
        shader_handle *handle = alloc_shader_handle(This);
        if (!handle) {
            ERR("Failed to allocate shader handle\n");
            IDirect3DVertexShader8_Release((IUnknown *)object);
            hrc = E_OUTOFMEMORY;
        } else {
            object->handle = handle;
            *handle = object;
            *ppShader = (handle - This->shader_handles) + VS_HIGHESTFIXEDFXF + 1;

            load_local_constants(pDeclaration, object->wineD3DVertexShader);
        }
    }
    LeaveCriticalSection(&d3d8_cs);
    TRACE("(%p) : returning %p (handle %#x)\n", This, object, *ppShader);

    return hrc;
}

IWineD3DVertexDeclaration *IDirect3DDevice8Impl_FindDecl(IDirect3DDevice8Impl *This, DWORD fvf)
{
    HRESULT hr;
    IWineD3DVertexDeclaration* pDecl = NULL;
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
            return convertedDecls[p].decl;
        } else if(convertedDecls[p].fvf < fvf) {
            low = p + 1;
        } else {
            high = p - 1;
        }
    }
    TRACE("not found. Creating and inserting at position %d.\n", low);

    hr = IWineD3DDevice_CreateVertexDeclarationFromFVF(This->WineD3DDevice,
                                                       &pDecl,
                                                       (IUnknown *) This,
                                                       fvf);
    if (FAILED(hr)) return NULL;

    if(This->declArraySize == This->numConvertedDecls) {
        int grow = This->declArraySize / 2;
        convertedDecls = HeapReAlloc(GetProcessHeap(), 0, convertedDecls,
                                     sizeof(convertedDecls[0]) * (This->numConvertedDecls + grow));
        if(!convertedDecls) {
            /* This will destroy it */
            IWineD3DVertexDeclaration_Release(pDecl);
            return NULL;
        }
        This->decls = convertedDecls;
        This->declArraySize += grow;
    }

    memmove(convertedDecls + low + 1, convertedDecls + low, sizeof(convertedDecls[0]) * (This->numConvertedDecls - low));
    convertedDecls[low].decl = pDecl;
    convertedDecls[low].fvf = fvf;
    This->numConvertedDecls++;

    TRACE("Returning %p. %d decls in array\n", pDecl, This->numConvertedDecls);
    return pDecl;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : Relay\n", This);
    EnterCriticalSection(&d3d8_cs);
    if (VS_HIGHESTFIXEDFXF >= pShader) {
        TRACE("Setting FVF, %d %d\n", VS_HIGHESTFIXEDFXF, pShader);
        IWineD3DDevice_SetFVF(This->WineD3DDevice, pShader);
        IWineD3DDevice_SetVertexDeclaration(This->WineD3DDevice, IDirect3DDevice8Impl_FindDecl(This, pShader));
        IWineD3DDevice_SetVertexShader(This->WineD3DDevice, NULL);
    } else {
        TRACE("Setting shader\n");
        if (This->allocated_shader_handles <= pShader - (VS_HIGHESTFIXEDFXF + 1)) {
            FIXME("(%p) : Number of shaders exceeds the maximum number of possible shaders\n", This);
            hrc = D3DERR_INVALIDCALL;
        } else {
            IDirect3DVertexShader8Impl *shader = This->shader_handles[pShader - (VS_HIGHESTFIXEDFXF + 1)];
            IWineD3DDevice_SetVertexDeclaration(This->WineD3DDevice,
                    shader ? ((IDirect3DVertexDeclaration8Impl *)shader->vertex_declaration)->wined3d_vertex_declaration : NULL);
            hrc = IWineD3DDevice_SetVertexShader(This->WineD3DDevice, 0 == shader ? NULL : shader->wineD3DVertexShader);
        }
    }
    TRACE("(%p) : returning hr(%u)\n", This, hrc);
    LeaveCriticalSection(&d3d8_cs);

    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShader(LPDIRECT3DDEVICE8 iface, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DVertexShader *pShader;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : Relay  device@%p\n", This, This->WineD3DDevice);
    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_GetVertexShader(This->WineD3DDevice, &pShader);
    if (D3D_OK == hrc) {
        if(0 != pShader) {
            IDirect3DVertexShader8Impl *d3d8_shader;
            hrc = IWineD3DVertexShader_GetParent(pShader, (IUnknown **)&d3d8_shader);
            IWineD3DVertexShader_Release(pShader);
            *ppShader = (d3d8_shader->handle - This->shader_handles) + (VS_HIGHESTFIXEDFXF + 1);
        } else {
            *ppShader = 0;
            hrc = D3D_OK;
        }
    } else {
        WARN("(%p) : Call to IWineD3DDevice_GetVertexShader failed %u (device %p)\n", This, hrc, This->WineD3DDevice);
    }
    TRACE("(%p) : returning %#x\n", This, *ppShader);
    LeaveCriticalSection(&d3d8_cs);

    return hrc;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_DeleteVertexShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) : pShader %#x\n", This, pShader);

    EnterCriticalSection(&d3d8_cs);
    if (pShader <= VS_HIGHESTFIXEDFXF || This->allocated_shader_handles <= pShader - (VS_HIGHESTFIXEDFXF + 1)) {
        ERR("(%p) : Trying to delete an invalid handle\n", This);
        LeaveCriticalSection(&d3d8_cs);
        return D3DERR_INVALIDCALL;
    } else {
        IWineD3DVertexShader *cur = NULL;
        shader_handle *handle = &This->shader_handles[pShader - (VS_HIGHESTFIXEDFXF + 1)];
        IDirect3DVertexShader8Impl *shader = *handle;

        IWineD3DDevice_GetVertexShader(This->WineD3DDevice, &cur);
        if(cur) {
            if(cur == shader->wineD3DVertexShader) IDirect3DDevice8_SetVertexShader(iface, 0);
            IWineD3DVertexShader_Release(cur);
        }

        while(IUnknown_Release((IUnknown *)shader));
        free_shader_handle(This, handle);
    }
    LeaveCriticalSection(&d3d8_cs);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) : Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetVertexShaderConstantF(This->WineD3DDevice, Register, (CONST float *)pConstantData, ConstantCount);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) : Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetVertexShaderConstantF(This->WineD3DDevice, Register, (float *)pConstantData, ConstantCount);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderDeclaration(LPDIRECT3DDEVICE8 iface, DWORD pVertexShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DVertexDeclaration8Impl *declaration;
    IDirect3DVertexShader8Impl *shader = NULL;

    TRACE("(%p) : pVertexShader 0x%08x, pData %p, *pSizeOfData %u\n", This, pVertexShader, pData, *pSizeOfData);

    EnterCriticalSection(&d3d8_cs);
    if (pVertexShader <= VS_HIGHESTFIXEDFXF || This->allocated_shader_handles <= pVertexShader - (VS_HIGHESTFIXEDFXF + 1)) {
        ERR("Passed an invalid shader handle.\n");
        LeaveCriticalSection(&d3d8_cs);
        return D3DERR_INVALIDCALL;
    }

    shader = This->shader_handles[pVertexShader - (VS_HIGHESTFIXEDFXF + 1)];
    declaration = (IDirect3DVertexDeclaration8Impl *)shader->vertex_declaration;

    /* If pData is NULL, we just return the required size of the buffer. */
    if (!pData) {
        *pSizeOfData = declaration->elements_size;
        LeaveCriticalSection(&d3d8_cs);
        return D3D_OK;
    }

    /* MSDN claims that if *pSizeOfData is smaller than the required size
     * we should write the required size and return D3DERR_MOREDATA.
     * That's not actually true. */
    if (*pSizeOfData < declaration->elements_size) {
        LeaveCriticalSection(&d3d8_cs);
        return D3DERR_INVALIDCALL;
    }

    CopyMemory(pData, declaration->elements, declaration->elements_size);
    LeaveCriticalSection(&d3d8_cs);

    return D3D_OK;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetVertexShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD pVertexShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DVertexShader8Impl *shader = NULL;
    HRESULT hr;

    TRACE("(%p) : pVertexShader %#x, pData %p, pSizeOfData %p\n", This, pVertexShader, pData, pSizeOfData);

    EnterCriticalSection(&d3d8_cs);
    if (pVertexShader <= VS_HIGHESTFIXEDFXF || This->allocated_shader_handles <= pVertexShader - (VS_HIGHESTFIXEDFXF + 1)) {
        ERR("Passed an invalid shader handle.\n");
        LeaveCriticalSection(&d3d8_cs);
        return D3DERR_INVALIDCALL;
    }

    shader = This->shader_handles[pVertexShader - (VS_HIGHESTFIXEDFXF + 1)];
    hr = IWineD3DVertexShader_GetFunction(shader->wineD3DVertexShader, pData, (UINT *)pSizeOfData);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8* pIndexData, UINT baseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    /* WineD3D takes an INT(due to d3d9), but d3d8 uses UINTs. Do I have to add a check here that
     * the UINT doesn't cause an overflow in the INT? It seems rather unlikely because such large
     * vertex buffers can't be created to address them with an index that requires the 32nd bit
     * (4 Byte minimum vertex size * 2^31-1 -> 8 gb buffer. The index sign would be the least
     * problem)
     */
    IWineD3DDevice_SetBaseVertexIndex(This->WineD3DDevice, baseVertexIndex);
    hr = IWineD3DDevice_SetIndices(This->WineD3DDevice,
            pIndexData ? ((IDirect3DIndexBuffer8Impl *)pIndexData)->wineD3DIndexBuffer : NULL);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetIndices(LPDIRECT3DDEVICE8 iface, IDirect3DIndexBuffer8** ppIndexData,UINT* pBaseVertexIndex) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DIndexBuffer *retIndexData = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n", This);

    if(ppIndexData == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    /* The case from UINT to INT is safe because d3d8 will never set negative values */
    IWineD3DDevice_GetBaseVertexIndex(This->WineD3DDevice, (INT *) pBaseVertexIndex);
    rc = IWineD3DDevice_GetIndices(This->WineD3DDevice, &retIndexData);
    if (SUCCEEDED(rc) && retIndexData) {
        IWineD3DIndexBuffer_GetParent(retIndexData, (IUnknown **)ppIndexData);
        IWineD3DIndexBuffer_Release(retIndexData);
    } else {
        if (FAILED(rc)) FIXME("Call to GetIndices failed\n");
        *ppIndexData = NULL;
    }
    LeaveCriticalSection(&d3d8_cs);

    return rc;
}
static HRESULT WINAPI IDirect3DDevice8Impl_CreatePixelShader(LPDIRECT3DDEVICE8 iface, CONST DWORD* pFunction, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *object;
    HRESULT hrc = D3D_OK;

    TRACE("(%p) : pFunction(%p), ppShader(%p)\n", This, pFunction, ppShader);

    if (NULL == ppShader) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));

    if (NULL == object) {
        return E_OUTOFMEMORY;
    } else {
        EnterCriticalSection(&d3d8_cs);

        object->ref    = 1;
        object->lpVtbl = &Direct3DPixelShader8_Vtbl;
        hrc = IWineD3DDevice_CreatePixelShader(This->WineD3DDevice, pFunction, &object->wineD3DPixelShader , (IUnknown *)object);
        if (D3D_OK != hrc) {
            FIXME("(%p) call to IWineD3DDevice_CreatePixelShader failed\n", This);
            HeapFree(GetProcessHeap(), 0 , object);
            *ppShader = 0;
        } else {
            shader_handle *handle = alloc_shader_handle(This);
            if (!handle) {
                ERR("Failed to allocate shader handle\n");
                IDirect3DVertexShader8_Release((IUnknown *)object);
                hrc = E_OUTOFMEMORY;
            } else {
                object->handle = handle;
                *handle = object;
                *ppShader = (handle - This->shader_handles) + VS_HIGHESTFIXEDFXF + 1;
            }
        }
        LeaveCriticalSection(&d3d8_cs);
    }

    TRACE("(%p) : returning %p (handle %#x)\n", This, object, *ppShader);
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *shader = NULL;
    HRESULT hr;

    TRACE("(%p) : pShader %#x\n", This, pShader);

    EnterCriticalSection(&d3d8_cs);
    if (pShader > VS_HIGHESTFIXEDFXF && This->allocated_shader_handles > pShader - (VS_HIGHESTFIXEDFXF + 1)) {
        shader = This->shader_handles[pShader - (VS_HIGHESTFIXEDFXF + 1)];
    } else if (pShader) {
        ERR("Trying to set an invalid handle.\n");
    }

    TRACE("(%p) : Setting shader %p\n", This, shader);
    hr = IWineD3DDevice_SetPixelShader(This->WineD3DDevice, shader == NULL ? NULL :shader->wineD3DPixelShader);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShader(LPDIRECT3DDEVICE8 iface, DWORD* ppShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DPixelShader *object;

    HRESULT hrc = D3D_OK;
    TRACE("(%p) Relay\n", This);
    if (NULL == ppShader) {
        TRACE("(%p) Invalid call\n", This);
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    hrc = IWineD3DDevice_GetPixelShader(This->WineD3DDevice, &object);
    if (D3D_OK == hrc && NULL != object) {
        IDirect3DPixelShader8Impl *d3d8_shader;
        hrc = IWineD3DPixelShader_GetParent(object, (IUnknown **)&d3d8_shader);
        IWineD3DPixelShader_Release(object);
        *ppShader = (d3d8_shader->handle - This->shader_handles) + (VS_HIGHESTFIXEDFXF + 1);
    } else {
        *ppShader = (DWORD)NULL;
    }

    TRACE("(%p) : returning %#x\n", This, *ppShader);
    LeaveCriticalSection(&d3d8_cs);
    return hrc;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeletePixelShader(LPDIRECT3DDEVICE8 iface, DWORD pShader) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;

    TRACE("(%p) : pShader %#x\n", This, pShader);

    EnterCriticalSection(&d3d8_cs);
    if (pShader <= VS_HIGHESTFIXEDFXF || This->allocated_shader_handles <= pShader - (VS_HIGHESTFIXEDFXF + 1)) {
        ERR("(%p) : Trying to delete an invalid handle\n", This);
        LeaveCriticalSection(&d3d8_cs);
        return D3DERR_INVALIDCALL;
    } else {
        IWineD3DPixelShader *cur = NULL;
        shader_handle *handle = &This->shader_handles[pShader - (VS_HIGHESTFIXEDFXF + 1)];
        IDirect3DPixelShader8Impl *shader = *handle;

        IWineD3DDevice_GetPixelShader(This->WineD3DDevice, &cur);
        if(cur) {
            if(cur == shader->wineD3DPixelShader) IDirect3DDevice8_SetPixelShader(iface, 0);
            IWineD3DPixelShader_Release(cur);
        }

        while(IUnknown_Release((IUnknown *)shader));
        free_shader_handle(This, handle);
    }
    LeaveCriticalSection(&d3d8_cs);

    return D3D_OK;
}

static HRESULT  WINAPI  IDirect3DDevice8Impl_SetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, CONST void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetPixelShaderConstantF(This->WineD3DDevice, Register, (CONST float *)pConstantData, ConstantCount);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderConstant(LPDIRECT3DDEVICE8 iface, DWORD Register, void* pConstantData, DWORD ConstantCount) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_GetPixelShaderConstantF(This->WineD3DDevice, Register, (float *)pConstantData, ConstantCount);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetPixelShaderFunction(LPDIRECT3DDEVICE8 iface, DWORD pPixelShader, void* pData, DWORD* pSizeOfData) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IDirect3DPixelShader8Impl *shader = NULL;
    HRESULT hr;

    TRACE("(%p) : pPixelShader %#x, pData %p, pSizeOfData %p\n", This, pPixelShader, pData, pSizeOfData);

    EnterCriticalSection(&d3d8_cs);
    if (pPixelShader <= VS_HIGHESTFIXEDFXF || This->allocated_shader_handles <= pPixelShader - (VS_HIGHESTFIXEDFXF + 1)) {
        ERR("Passed an invalid shader handle.\n");
        LeaveCriticalSection(&d3d8_cs);
        return D3DERR_INVALIDCALL;
    }

    shader = This->shader_handles[pPixelShader - (VS_HIGHESTFIXEDFXF + 1)];
    hr = IWineD3DPixelShader_GetFunction(shader->wineD3DPixelShader, pData, (UINT *)pSizeOfData);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawRectPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DRECTPATCH_INFO* pRectPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_DrawRectPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DRECTPATCH_INFO *)pRectPatchInfo);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DrawTriPatch(LPDIRECT3DDEVICE8 iface, UINT Handle,CONST float* pNumSegs,CONST D3DTRIPATCH_INFO* pTriPatchInfo) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_DrawTriPatch(This->WineD3DDevice, Handle, pNumSegs, (CONST WINED3DTRIPATCH_INFO *)pTriPatchInfo);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_DeletePatch(LPDIRECT3DDEVICE8 iface, UINT Handle) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_DeletePatch(This->WineD3DDevice, Handle);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_SetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8* pStreamData,UINT Stride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    HRESULT hr;
    TRACE("(%p) Relay\n" , This);

    EnterCriticalSection(&d3d8_cs);
    hr = IWineD3DDevice_SetStreamSource(This->WineD3DDevice, StreamNumber,
                                        NULL == pStreamData ? NULL : ((IDirect3DVertexBuffer8Impl *)pStreamData)->wineD3DVertexBuffer,
                                        0/* Offset in bytes */, Stride);
    LeaveCriticalSection(&d3d8_cs);
    return hr;
}

static HRESULT WINAPI IDirect3DDevice8Impl_GetStreamSource(LPDIRECT3DDEVICE8 iface, UINT StreamNumber,IDirect3DVertexBuffer8** pStream,UINT* pStride) {
    IDirect3DDevice8Impl *This = (IDirect3DDevice8Impl *)iface;
    IWineD3DVertexBuffer *retStream = NULL;
    HRESULT rc = D3D_OK;

    TRACE("(%p) Relay\n" , This);

    if(pStream == NULL){
        return D3DERR_INVALIDCALL;
    }

    EnterCriticalSection(&d3d8_cs);
    rc = IWineD3DDevice_GetStreamSource(This->WineD3DDevice, StreamNumber, (IWineD3DVertexBuffer **)&retStream, 0 /* Offset in bytes */, pStride);
    if (rc == D3D_OK  && NULL != retStream) {
        IWineD3DVertexBuffer_GetParent(retStream, (IUnknown **)pStream);
        IWineD3DVertexBuffer_Release(retStream);
    }else{
        if (rc != D3D_OK){
            FIXME("Call to GetStreamSource failed %p\n",  pStride);
        }
        *pStream = NULL;
    }
    LeaveCriticalSection(&d3d8_cs);

    return rc;
}


const IDirect3DDevice8Vtbl Direct3DDevice8_Vtbl =
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

/* Internal function called back during the CreateDevice to create a render target  */
HRESULT WINAPI D3D8CB_CreateSurface(IUnknown *device, IUnknown *pSuperior, UINT Width, UINT Height,
                                         WINED3DFORMAT Format, DWORD Usage, WINED3DPOOL Pool, UINT Level,
                                         WINED3DCUBEMAP_FACES Face, IWineD3DSurface **ppSurface,
                                         HANDLE *pSharedHandle) {

    HRESULT res = D3D_OK;
    IDirect3DSurface8Impl *d3dSurface = NULL;
    BOOL Lockable = TRUE;

    if((WINED3DPOOL_DEFAULT == Pool && WINED3DUSAGE_DYNAMIC != Usage))
        Lockable = FALSE;

    TRACE("relay\n");
    res = IDirect3DDevice8Impl_CreateSurface((IDirect3DDevice8 *)device, Width, Height, (D3DFORMAT)Format, Lockable, FALSE/*Discard*/, Level,  (IDirect3DSurface8 **)&d3dSurface, D3DRTYPE_SURFACE, Usage, Pool, D3DMULTISAMPLE_NONE, 0 /* MultisampleQuality */);

    if (SUCCEEDED(res)) {
        *ppSurface = d3dSurface->wineD3DSurface;
        d3dSurface->container = pSuperior;
        IUnknown_Release(d3dSurface->parentDevice);
        d3dSurface->parentDevice = NULL;
        d3dSurface->forwardReference = pSuperior;
    } else {
        FIXME("(%p) IDirect3DDevice8_CreateSurface failed\n", device);
    }
    return res;
}

ULONG WINAPI D3D8CB_DestroySurface(IWineD3DSurface *pSurface) {
    IDirect3DSurface8Impl* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    IWineD3DSurface_GetParent(pSurface, (IUnknown **) &surfaceParent);
    /* GetParent's AddRef was forwarded to an object in destruction.
     * Releasing it here again would cause an endless recursion. */
    surfaceParent->forwardReference = NULL;
    return IDirect3DSurface8_Release((IDirect3DSurface8*) surfaceParent);
}
