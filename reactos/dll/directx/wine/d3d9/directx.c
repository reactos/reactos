/*
 * IDirect3D9 implementation
 *
 * Copyright 2002 Jason Edmeades
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

/* IDirect3D9 IUnknown parts follow: */
static HRESULT WINAPI IDirect3D9Impl_QueryInterface(LPDIRECT3D9EX iface, REFIID riid, LPVOID* ppobj)
{
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;

    if (IsEqualGUID(riid, &IID_IUnknown)
        || IsEqualGUID(riid, &IID_IDirect3D9)) {
        IDirect3D9Ex_AddRef(iface);
        *ppobj = This;
        TRACE("Returning IDirect3D9 interface at %p\n", *ppobj);
        return S_OK;
    } else if(IsEqualGUID(riid, &IID_IDirect3D9Ex)) {
        if(This->extended) {
            *ppobj = This;
            TRACE("Returning IDirect3D9Ex interface at %p\n", *ppobj);
            IDirect3D9Ex_AddRef((IDirect3D9Ex *)*ppobj);
        } else {
            WARN("Application asks for IDirect3D9Ex, but this instance wasn't created with Direct3DCreate9Ex\n");
            WARN("Returning E_NOINTERFACE\n");
            *ppobj = NULL;
            return E_NOINTERFACE;
        }
    }

    WARN("(%p)->(%s,%p),not found\n", This, debugstr_guid(riid), ppobj);
    *ppobj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI IDirect3D9Impl_AddRef(LPDIRECT3D9EX iface) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) : AddRef from %d\n", This, ref - 1);

    return ref;
}

static ULONG WINAPI IDirect3D9Impl_Release(LPDIRECT3D9EX iface) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) : ReleaseRef to %d\n", This, ref);

    if (ref == 0) {
        EnterCriticalSection(&d3d9_cs);
        IWineD3D_Release(This->WineD3D);
        LeaveCriticalSection(&d3d9_cs);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/* IDirect3D9 Interface follow: */
static HRESULT  WINAPI  IDirect3D9Impl_RegisterSoftwareDevice(LPDIRECT3D9EX iface, void* pInitializeFunction) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, pInitializeFunction);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_RegisterSoftwareDevice(This->WineD3D, pInitializeFunction);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static UINT     WINAPI  IDirect3D9Impl_GetAdapterCount(LPDIRECT3D9EX iface) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("%p\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_GetAdapterCount(This->WineD3D);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_GetAdapterIdentifier(LPDIRECT3D9EX iface, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    WINED3DADAPTER_IDENTIFIER adapter_id;
    HRESULT hr;

    /* dx8 and dx9 have different structures to be filled in, with incompatible 
       layouts so pass in pointers to the places to be filled via an internal 
       structure                                                                */
    adapter_id.Driver           = pIdentifier->Driver;          
    adapter_id.Description      = pIdentifier->Description;     
    adapter_id.DeviceName       = pIdentifier->DeviceName;      
    adapter_id.DriverVersion    = &pIdentifier->DriverVersion;   
    adapter_id.VendorId         = &pIdentifier->VendorId;        
    adapter_id.DeviceId         = &pIdentifier->DeviceId;        
    adapter_id.SubSysId         = &pIdentifier->SubSysId;        
    adapter_id.Revision         = &pIdentifier->Revision;        
    adapter_id.DeviceIdentifier = &pIdentifier->DeviceIdentifier;
    adapter_id.WHQLLevel        = &pIdentifier->WHQLLevel;       

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_GetAdapterIdentifier(This->WineD3D, Adapter, Flags, &adapter_id);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static UINT WINAPI IDirect3D9Impl_GetAdapterModeCount(LPDIRECT3D9EX iface, UINT Adapter, D3DFORMAT Format) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d, %d\n", This, Adapter, Format);

    /* Others than that not supported by d3d9, but reported by wined3d for ddraw. Filter them out */
    if(Format != D3DFMT_X8R8G8B8 && Format != D3DFMT_R5G6B5) {
        return 0;
    }

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_GetAdapterModeCount(This->WineD3D, Adapter, wined3dformat_from_d3dformat(Format));
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_EnumAdapterModes(LPDIRECT3D9EX iface, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d, %d, %d, %p)\n", This, Adapter, Format, Mode, pMode);
    /* We can't pass this to WineD3D, otherwise it'll think it came from D3D8 or DDraw.
       It's supposed to fail anyway, so no harm returning failure. */
    if(Format != D3DFMT_X8R8G8B8 && Format != D3DFMT_R5G6B5)
        return D3DERR_INVALIDCALL;

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_EnumAdapterModes(This->WineD3D, Adapter, wined3dformat_from_d3dformat(Format),
            Mode, (WINED3DDISPLAYMODE *) pMode);
    LeaveCriticalSection(&d3d9_cs);

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_GetAdapterDisplayMode(LPDIRECT3D9EX iface, UINT Adapter, D3DDISPLAYMODE* pMode) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_GetAdapterDisplayMode(This->WineD3D, Adapter, (WINED3DDISPLAYMODE *) pMode);
    LeaveCriticalSection(&d3d9_cs);

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceType(LPDIRECT3D9EX iface,
					      UINT Adapter, D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat,
					      D3DFORMAT BackBufferFormat, BOOL Windowed) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%d, %d, %d, %d, %s\n", This, Adapter, CheckType, DisplayFormat,
          BackBufferFormat, Windowed ? "true" : "false");

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_CheckDeviceType(This->WineD3D, Adapter, CheckType, wined3dformat_from_d3dformat(DisplayFormat),
            wined3dformat_from_d3dformat(BackBufferFormat), Windowed);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormat(LPDIRECT3D9EX iface,
						  UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
						  DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    WINED3DRESOURCETYPE WineD3DRType;
    TRACE("%p\n", This);

    switch(RType) {
        case D3DRTYPE_VERTEXBUFFER:
        case D3DRTYPE_INDEXBUFFER:
            WineD3DRType = WINED3DRTYPE_BUFFER;
            break;

        default:
            WineD3DRType = RType;
            break;
    }

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_CheckDeviceFormat(This->WineD3D, Adapter, DeviceType, wined3dformat_from_d3dformat(AdapterFormat),
            Usage, WineD3DRType, wined3dformat_from_d3dformat(CheckFormat), SURFACE_OPENGL);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceMultiSampleType(LPDIRECT3D9EX iface,
							   UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat,
							   BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("%p\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_CheckDeviceMultiSampleType(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(SurfaceFormat), Windowed, MultiSampleType, pQualityLevels);
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDepthStencilMatch(LPDIRECT3D9EX iface,
						       UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat,
						       D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("%p\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_CheckDepthStencilMatch(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(AdapterFormat), wined3dformat_from_d3dformat(RenderTargetFormat),
            wined3dformat_from_d3dformat(DepthStencilFormat));
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormatConversion(LPDIRECT3D9EX iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    TRACE("%p\n", This);

    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_CheckDeviceFormatConversion(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(SourceFormat), wined3dformat_from_d3dformat(TargetFormat));
    LeaveCriticalSection(&d3d9_cs);
    return hr;
}

void filter_caps(D3DCAPS9* pCaps)
{

    DWORD textureFilterCaps =
        D3DPTFILTERCAPS_MINFPOINT      | D3DPTFILTERCAPS_MINFLINEAR    | D3DPTFILTERCAPS_MINFANISOTROPIC |
        D3DPTFILTERCAPS_MINFPYRAMIDALQUAD                              | D3DPTFILTERCAPS_MINFGAUSSIANQUAD|
        D3DPTFILTERCAPS_MIPFPOINT      | D3DPTFILTERCAPS_MIPFLINEAR    | D3DPTFILTERCAPS_MAGFPOINT       |
        D3DPTFILTERCAPS_MAGFLINEAR     |D3DPTFILTERCAPS_MAGFANISOTROPIC|D3DPTFILTERCAPS_MAGFPYRAMIDALQUAD|
        D3DPTFILTERCAPS_MAGFGAUSSIANQUAD;
    pCaps->TextureFilterCaps &= textureFilterCaps;
    pCaps->CubeTextureFilterCaps &= textureFilterCaps;
    pCaps->VolumeTextureFilterCaps &= textureFilterCaps;

    pCaps->DevCaps &=
        D3DDEVCAPS_EXECUTESYSTEMMEMORY | D3DDEVCAPS_EXECUTEVIDEOMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TLVERTEXVIDEOMEMORY | D3DDEVCAPS_TEXTURESYSTEMMEMORY| D3DDEVCAPS_TEXTUREVIDEOMEMORY   |
        D3DDEVCAPS_DRAWPRIMTLVERTEX    | D3DDEVCAPS_CANRENDERAFTERFLIP | D3DDEVCAPS_TEXTURENONLOCALVIDMEM|
        D3DDEVCAPS_DRAWPRIMITIVES2     | D3DDEVCAPS_SEPARATETEXTUREMEMORIES                              |
        D3DDEVCAPS_DRAWPRIMITIVES2EX   | D3DDEVCAPS_HWTRANSFORMANDLIGHT| D3DDEVCAPS_CANBLTSYSTONONLOCAL  |
        D3DDEVCAPS_HWRASTERIZATION     | D3DDEVCAPS_PUREDEVICE         | D3DDEVCAPS_QUINTICRTPATCHES     |
        D3DDEVCAPS_RTPATCHES           | D3DDEVCAPS_RTPATCHHANDLEZERO  | D3DDEVCAPS_NPATCHES;

    pCaps->ShadeCaps &=
        D3DPSHADECAPS_COLORGOURAUDRGB  | D3DPSHADECAPS_SPECULARGOURAUDRGB |
        D3DPSHADECAPS_ALPHAGOURAUDBLEND | D3DPSHADECAPS_FOGGOURAUD;

    pCaps->RasterCaps &=
        D3DPRASTERCAPS_DITHER          | D3DPRASTERCAPS_ZTEST          | D3DPRASTERCAPS_FOGVERTEX        |
        D3DPRASTERCAPS_FOGTABLE        | D3DPRASTERCAPS_MIPMAPLODBIAS  | D3DPRASTERCAPS_ZBUFFERLESSHSR   |
        D3DPRASTERCAPS_FOGRANGE        | D3DPRASTERCAPS_ANISOTROPY     | D3DPRASTERCAPS_WBUFFER          |
        D3DPRASTERCAPS_WFOG            | D3DPRASTERCAPS_ZFOG           | D3DPRASTERCAPS_COLORPERSPECTIVE |
        D3DPRASTERCAPS_SCISSORTEST     | D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS                              |
        D3DPRASTERCAPS_DEPTHBIAS       | D3DPRASTERCAPS_MULTISAMPLE_TOGGLE;

    pCaps->DevCaps2 &=
        D3DDEVCAPS2_STREAMOFFSET       | D3DDEVCAPS2_DMAPNPATCH        | D3DDEVCAPS2_ADAPTIVETESSRTPATCH |
        D3DDEVCAPS2_ADAPTIVETESSNPATCH | D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES                       |
        D3DDEVCAPS2_PRESAMPLEDDMAPNPATCH| D3DDEVCAPS2_VERTEXELEMENTSCANSHARESTREAMOFFSET;

    pCaps->Caps2 &=
        D3DCAPS2_FULLSCREENGAMMA       | D3DCAPS2_CANCALIBRATEGAMMA    | D3DCAPS2_RESERVED               |
        D3DCAPS2_CANMANAGERESOURCE     | D3DCAPS2_DYNAMICTEXTURES      | D3DCAPS2_CANAUTOGENMIPMAP;

    pCaps->VertexProcessingCaps &=
        D3DVTXPCAPS_TEXGEN             | D3DVTXPCAPS_MATERIALSOURCE7   | D3DVTXPCAPS_DIRECTIONALLIGHTS   |
        D3DVTXPCAPS_POSITIONALLIGHTS   | D3DVTXPCAPS_LOCALVIEWER       | D3DVTXPCAPS_TWEENING            |
        D3DVTXPCAPS_TEXGEN_SPHEREMAP   | D3DVTXPCAPS_NO_TEXGEN_NONLOCALVIEWER;

    pCaps->TextureCaps &=
        D3DPTEXTURECAPS_PERSPECTIVE    | D3DPTEXTURECAPS_POW2          | D3DPTEXTURECAPS_ALPHA           |
        D3DPTEXTURECAPS_SQUAREONLY     | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE                        |
        D3DPTEXTURECAPS_ALPHAPALETTE   | D3DPTEXTURECAPS_NONPOW2CONDITIONAL                              |
        D3DPTEXTURECAPS_PROJECTED      | D3DPTEXTURECAPS_CUBEMAP       | D3DPTEXTURECAPS_VOLUMEMAP       |
        D3DPTEXTURECAPS_MIPMAP         | D3DPTEXTURECAPS_MIPVOLUMEMAP  | D3DPTEXTURECAPS_MIPCUBEMAP      |
        D3DPTEXTURECAPS_CUBEMAP_POW2   | D3DPTEXTURECAPS_VOLUMEMAP_POW2| D3DPTEXTURECAPS_NOPROJECTEDBUMPENV;
}

static HRESULT WINAPI IDirect3D9Impl_GetDeviceCaps(LPDIRECT3D9EX iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("(%p) Relay %d %u %p\n", This, Adapter, DeviceType, pCaps);

    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /*well this is what MSDN says to return*/
    }
    memset(pCaps, 0, sizeof(*pCaps));
    EnterCriticalSection(&d3d9_cs);
    hrc = IWineD3D_GetDeviceCaps(This->WineD3D, Adapter, DeviceType, pWineCaps);
    LeaveCriticalSection(&d3d9_cs);
    WINECAPSTOD3D9CAPS(pCaps, pWineCaps)
    HeapFree(GetProcessHeap(), 0, pWineCaps);

    /* Some functionality is implemented in d3d9.dll, not wined3d.dll. Add the needed caps */
    pCaps->DevCaps2 |= D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES;

    filter_caps(pCaps);

    TRACE("(%p) returning %p\n", This, pCaps);
    return hrc;
}

static HMONITOR WINAPI IDirect3D9Impl_GetAdapterMonitor(LPDIRECT3D9EX iface, UINT Adapter) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HMONITOR ret;
    TRACE("%p\n", This);

    EnterCriticalSection(&d3d9_cs);
    ret = IWineD3D_GetAdapterMonitor(This->WineD3D, Adapter);
    LeaveCriticalSection(&d3d9_cs);
    return ret;
}

ULONG WINAPI D3D9CB_DestroyRenderTarget(IWineD3DSurface *pSurface) {
    IDirect3DSurface9Impl* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    IWineD3DSurface_GetParent(pSurface, (IUnknown **) &surfaceParent);
    surfaceParent->isImplicit = FALSE;
    /* Surface had refcount of 0 GetParent addrefed to 1, so 1 Release is enough */
    return IDirect3DSurface9_Release((IDirect3DSurface9*) surfaceParent);
}

ULONG WINAPI D3D9CB_DestroySwapChain(IWineD3DSwapChain *pSwapChain) {
    IDirect3DSwapChain9Impl* swapChainParent;
    TRACE("(%p) call back\n", pSwapChain);

    IWineD3DSwapChain_GetParent(pSwapChain,(IUnknown **) &swapChainParent);
    swapChainParent->isImplicit = FALSE;
    /* Swap chain had refcount of 0 GetParent addrefed to 1, so 1 Release is enough */
    return IDirect3DSwapChain9_Release((IDirect3DSwapChain9*) swapChainParent);
}

ULONG WINAPI D3D9CB_DestroyDepthStencilSurface(IWineD3DSurface *pSurface) {
    IDirect3DSurface9Impl* surfaceParent;
    TRACE("(%p) call back\n", pSurface);

    IWineD3DSurface_GetParent(pSurface, (IUnknown **) &surfaceParent);
    surfaceParent->isImplicit = FALSE;
    /* Surface had refcount of 0 GetParent addrefed to 1, so 1 Release is enough */
    return IDirect3DSurface9_Release((IDirect3DSurface9*) surfaceParent);
}

static HRESULT WINAPI IDirect3D9Impl_CreateDevice(LPDIRECT3D9EX iface, UINT Adapter, D3DDEVTYPE DeviceType,
                                                  HWND hFocusWindow, DWORD BehaviourFlags,
                                                  D3DPRESENT_PARAMETERS* pPresentationParameters,
                                                  IDirect3DDevice9** ppReturnedDeviceInterface) {

    IDirect3D9Impl       *This   = (IDirect3D9Impl *)iface;
    IDirect3DDevice9Impl *object = NULL;
    WINED3DPRESENT_PARAMETERS *localParameters;
    UINT i, count = 1;
    HRESULT hr;
    TRACE("(%p) Relay\n", This);

    /* Check the validity range of the adapter parameter */
    if (Adapter >= IDirect3D9Impl_GetAdapterCount(iface)) {
        *ppReturnedDeviceInterface = NULL;
        return D3DERR_INVALIDCALL;
    }

    /* Allocate the storage for the device object */
    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirect3DDevice9Impl));
    if (NULL == object) {
        FIXME("Allocation of memory failed\n");
        *ppReturnedDeviceInterface = NULL;
        return D3DERR_OUTOFVIDEOMEMORY;
    }

    object->lpVtbl = &Direct3DDevice9_Vtbl;
    object->device_parent_vtbl = &d3d9_wined3d_device_parent_vtbl;
    object->ref = 1;
    *ppReturnedDeviceInterface = (IDirect3DDevice9 *)object;

    /* Allocate an associated WineD3DDevice object */
    EnterCriticalSection(&d3d9_cs);
    hr = IWineD3D_CreateDevice(This->WineD3D, Adapter, DeviceType, hFocusWindow, BehaviourFlags,
            (IUnknown *)object, (IWineD3DDeviceParent *)&object->device_parent_vtbl, &object->WineD3DDevice);
    if (hr != D3D_OK) {
        HeapFree(GetProcessHeap(), 0, object);
        *ppReturnedDeviceInterface = NULL;
        LeaveCriticalSection(&d3d9_cs);
        return hr;
    }

    TRACE("(%p) : Created Device %p\n", This, object);

    if (BehaviourFlags & D3DCREATE_ADAPTERGROUP_DEVICE)
    {
        WINED3DCAPS caps;

        IWineD3D_GetDeviceCaps(This->WineD3D, Adapter, DeviceType, &caps);
        count = caps.NumberOfAdaptersInGroup;
    }

    if(BehaviourFlags & D3DCREATE_MULTITHREADED) {
        IWineD3DDevice_SetMultithreaded(object->WineD3DDevice);
    }

    localParameters = HeapAlloc(GetProcessHeap(), 0, sizeof(*localParameters) * count);
    for (i = 0; i < count; ++i)
    {
        localParameters[i].BackBufferWidth = pPresentationParameters[i].BackBufferWidth;
        localParameters[i].BackBufferHeight = pPresentationParameters[i].BackBufferHeight;
        localParameters[i].BackBufferFormat = wined3dformat_from_d3dformat(pPresentationParameters[i].BackBufferFormat);
        localParameters[i].BackBufferCount = pPresentationParameters[i].BackBufferCount;
        localParameters[i].MultiSampleType = pPresentationParameters[i].MultiSampleType;
        localParameters[i].MultiSampleQuality = pPresentationParameters[i].MultiSampleQuality;
        localParameters[i].SwapEffect = pPresentationParameters[i].SwapEffect;
        localParameters[i].hDeviceWindow = pPresentationParameters[i].hDeviceWindow;
        localParameters[i].Windowed = pPresentationParameters[i].Windowed;
        localParameters[i].EnableAutoDepthStencil = pPresentationParameters[i].EnableAutoDepthStencil;
        localParameters[i].AutoDepthStencilFormat = wined3dformat_from_d3dformat(pPresentationParameters[i].AutoDepthStencilFormat);
        localParameters[i].Flags = pPresentationParameters[i].Flags;
        localParameters[i].FullScreen_RefreshRateInHz = pPresentationParameters[i].FullScreen_RefreshRateInHz;
        localParameters[i].PresentationInterval = pPresentationParameters[i].PresentationInterval;
        localParameters[i].AutoRestoreDisplayMode = TRUE;
    }

    hr = IWineD3DDevice_Init3D(object->WineD3DDevice, localParameters);
    if (hr != D3D_OK) {
        FIXME("(%p) D3D Initialization failed for WineD3DDevice %p\n", This, object->WineD3DDevice);
        HeapFree(GetProcessHeap(), 0, object);
        *ppReturnedDeviceInterface = NULL;
    }

    for (i = 0; i < count; ++i)
    {
        pPresentationParameters[i].BackBufferWidth = localParameters[i].BackBufferWidth;
        pPresentationParameters[i].BackBufferHeight = localParameters[i].BackBufferHeight;
        pPresentationParameters[i].BackBufferFormat = d3dformat_from_wined3dformat(localParameters[i].BackBufferFormat);
        pPresentationParameters[i].BackBufferCount = localParameters[i].BackBufferCount;
        pPresentationParameters[i].MultiSampleType = localParameters[i].MultiSampleType;
        pPresentationParameters[i].MultiSampleQuality = localParameters[i].MultiSampleQuality;
        pPresentationParameters[i].SwapEffect = localParameters[i].SwapEffect;
        pPresentationParameters[i].hDeviceWindow = localParameters[i].hDeviceWindow;
        pPresentationParameters[i].Windowed = localParameters[i].Windowed;
        pPresentationParameters[i].EnableAutoDepthStencil = localParameters[i].EnableAutoDepthStencil;
        pPresentationParameters[i].AutoDepthStencilFormat = d3dformat_from_wined3dformat(localParameters[i].AutoDepthStencilFormat);
        pPresentationParameters[i].Flags = localParameters[i].Flags;
        pPresentationParameters[i].FullScreen_RefreshRateInHz = localParameters[i].FullScreen_RefreshRateInHz;
        pPresentationParameters[i].PresentationInterval = localParameters[i].PresentationInterval;
    }
    HeapFree(GetProcessHeap(), 0, localParameters);

    /* Initialize the converted declaration array. This creates a valid pointer and when adding decls HeapReAlloc
     * can be used without further checking
     */
    object->convertedDecls = HeapAlloc(GetProcessHeap(), 0, 0);
    LeaveCriticalSection(&d3d9_cs);

    return hr;
}

static UINT WINAPI IDirect3D9ExImpl_GetAdapterModeCountEx(IDirect3D9Ex *iface, UINT Adapter, CONST D3DDISPLAYMODEFILTER *pFilter) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    FIXME("(%p)->(%d, %p): Stub!\n", This, Adapter, pFilter);
    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI IDirect3D9ExImpl_EnumAdapterModesEx(IDirect3D9Ex *iface, UINT Adapter, CONST D3DDISPLAYMODEFILTER *pFilter, UINT Mode, D3DDISPLAYMODEEX* pMode) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    FIXME("(%p)->(%d, %p, %p): Stub!\n", This, Adapter, pFilter, pMode);
    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI IDirect3D9ExImpl_GetAdapterDisplayModeEx(IDirect3D9Ex *iface, UINT Adapter, D3DDISPLAYMODEEX *pMode, D3DDISPLAYROTATION *pRotation) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    FIXME("(%p)->(%d, %p, %p): Stub!\n", This, Adapter, pMode, pRotation);
    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI IDirect3D9ExImpl_CreateDeviceEx(IDirect3D9Ex *iface, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode, struct IDirect3DDevice9Ex **ppReturnedDeviceInterface) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    FIXME("(%p)->(%d, %d, %p, 0x%08x, %p, %p, %p): Stub!\n", This, Adapter, DeviceType, hFocusWindow, BehaviorFlags, pPresentationParameters, pFullscreenDisplayMode, ppReturnedDeviceInterface);
    *ppReturnedDeviceInterface = NULL;
    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI IDirect3D9ExImpl_GetAdapterLUID(IDirect3D9Ex *iface, UINT Adapter, LUID *pLUID) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    FIXME("(%p)->(%d, %p)\n", This, Adapter, pLUID);
    return D3DERR_DRIVERINTERNALERROR;
}


const IDirect3D9ExVtbl Direct3D9_Vtbl =
{
    /* IUnknown */
    IDirect3D9Impl_QueryInterface,
    IDirect3D9Impl_AddRef,
    IDirect3D9Impl_Release,
    /* IDirect3D9 */
    IDirect3D9Impl_RegisterSoftwareDevice,
    IDirect3D9Impl_GetAdapterCount,
    IDirect3D9Impl_GetAdapterIdentifier,
    IDirect3D9Impl_GetAdapterModeCount,
    IDirect3D9Impl_EnumAdapterModes,
    IDirect3D9Impl_GetAdapterDisplayMode,
    IDirect3D9Impl_CheckDeviceType,
    IDirect3D9Impl_CheckDeviceFormat,
    IDirect3D9Impl_CheckDeviceMultiSampleType,
    IDirect3D9Impl_CheckDepthStencilMatch,
    IDirect3D9Impl_CheckDeviceFormatConversion,
    IDirect3D9Impl_GetDeviceCaps,
    IDirect3D9Impl_GetAdapterMonitor,
    IDirect3D9Impl_CreateDevice,
    /* IDirect3D9Ex */
    IDirect3D9ExImpl_GetAdapterModeCountEx,
    IDirect3D9ExImpl_EnumAdapterModesEx,
    IDirect3D9ExImpl_GetAdapterDisplayModeEx,
    IDirect3D9ExImpl_CreateDeviceEx,
    IDirect3D9ExImpl_GetAdapterLUID

};
