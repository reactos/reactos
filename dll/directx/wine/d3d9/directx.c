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

    TRACE("iface %p, riid %s, object %p.\n", iface, debugstr_guid(riid), ppobj);

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

    TRACE("%p increasing refcount to %u.\n", iface, ref);

    return ref;
}

static ULONG WINAPI IDirect3D9Impl_Release(LPDIRECT3D9EX iface) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p decreasing refcount to %u.\n", iface, ref);

    if (ref == 0) {
        wined3d_mutex_lock();
        IWineD3D_Release(This->WineD3D);
        wined3d_mutex_unlock();

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

/* IDirect3D9 Interface follow: */
static HRESULT  WINAPI  IDirect3D9Impl_RegisterSoftwareDevice(LPDIRECT3D9EX iface, void* pInitializeFunction) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, init_function %p.\n", iface, pInitializeFunction);

    wined3d_mutex_lock();
    hr = IWineD3D_RegisterSoftwareDevice(This->WineD3D, pInitializeFunction);
    wined3d_mutex_unlock();

    return hr;
}

static UINT     WINAPI  IDirect3D9Impl_GetAdapterCount(LPDIRECT3D9EX iface) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p.\n", iface);

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterCount(This->WineD3D);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_GetAdapterIdentifier(LPDIRECT3D9EX iface, UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    WINED3DADAPTER_IDENTIFIER adapter_id;
    HRESULT hr;

    TRACE("iface %p, adapter %u, flags %#x, identifier %p.\n",
            iface, Adapter, Flags, pIdentifier);

    adapter_id.driver = pIdentifier->Driver;
    adapter_id.driver_size = sizeof(pIdentifier->Driver);
    adapter_id.description = pIdentifier->Description;
    adapter_id.description_size = sizeof(pIdentifier->Description);
    adapter_id.device_name = pIdentifier->DeviceName;
    adapter_id.device_name_size = sizeof(pIdentifier->DeviceName);

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterIdentifier(This->WineD3D, Adapter, Flags, &adapter_id);
    wined3d_mutex_unlock();

    pIdentifier->DriverVersion = adapter_id.driver_version;
    pIdentifier->VendorId = adapter_id.vendor_id;
    pIdentifier->DeviceId = adapter_id.device_id;
    pIdentifier->SubSysId = adapter_id.subsystem_id;
    pIdentifier->Revision = adapter_id.revision;
    memcpy(&pIdentifier->DeviceIdentifier, &adapter_id.device_identifier, sizeof(pIdentifier->DeviceIdentifier));
    pIdentifier->WHQLLevel = adapter_id.whql_level;

    return hr;
}

static UINT WINAPI IDirect3D9Impl_GetAdapterModeCount(LPDIRECT3D9EX iface, UINT Adapter, D3DFORMAT Format) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, adapter %u, format %#x.\n", iface, Adapter, Format);

    /* Others than that not supported by d3d9, but reported by wined3d for ddraw. Filter them out */
    if(Format != D3DFMT_X8R8G8B8 && Format != D3DFMT_R5G6B5) {
        return 0;
    }

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterModeCount(This->WineD3D, Adapter, wined3dformat_from_d3dformat(Format));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_EnumAdapterModes(LPDIRECT3D9EX iface, UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, adapter %u, format %#x, mode_idx %u, mode %p.\n",
            iface, Adapter, Format, Mode, pMode);

    /* We can't pass this to WineD3D, otherwise it'll think it came from D3D8 or DDraw.
       It's supposed to fail anyway, so no harm returning failure. */
    if(Format != D3DFMT_X8R8G8B8 && Format != D3DFMT_R5G6B5)
        return D3DERR_INVALIDCALL;

    wined3d_mutex_lock();
    hr = IWineD3D_EnumAdapterModes(This->WineD3D, Adapter, wined3dformat_from_d3dformat(Format),
            Mode, (WINED3DDISPLAYMODE *) pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_GetAdapterDisplayMode(LPDIRECT3D9EX iface, UINT Adapter, D3DDISPLAYMODE* pMode) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, adapter %u, mode %p.\n", iface, Adapter, pMode);

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterDisplayMode(This->WineD3D, Adapter, (WINED3DDISPLAYMODE *) pMode);
    wined3d_mutex_unlock();

    if (SUCCEEDED(hr)) pMode->Format = d3dformat_from_wined3dformat(pMode->Format);

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceType(IDirect3D9Ex *iface, UINT Adapter,
        D3DDEVTYPE CheckType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, BOOL Windowed)
{
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, display_format %#x, backbuffer_format %#x, windowed %#x.\n",
            iface, Adapter, CheckType, DisplayFormat, BackBufferFormat, Windowed);

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDeviceType(This->WineD3D, Adapter, CheckType, wined3dformat_from_d3dformat(DisplayFormat),
            wined3dformat_from_d3dformat(BackBufferFormat), Windowed);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormat(IDirect3D9Ex *iface, UINT Adapter, D3DDEVTYPE DeviceType,
        D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat)
{
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;
    WINED3DRESOURCETYPE WineD3DRType;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, usage %#x, resource_type %#x, format %#x.\n",
            iface, Adapter, DeviceType, AdapterFormat, Usage, RType, CheckFormat);

    /* This format is nothing special and it is supported perfectly.
     * However, ati and nvidia driver on windows do not mark this format as
     * supported (tested with the dxCapsViewer) and pretending to
     * support this format uncovers a bug in Battlefield 1942 (fonts are missing)
     * So do the same as Windows drivers and pretend not to support it on dx8 and 9
     */
    if(CheckFormat == D3DFMT_R8G8B8)
    {
        WARN("D3DFMT_R8G8B8 is not available on windows, returning D3DERR_NOTAVAILABLE\n");
        return D3DERR_NOTAVAILABLE;
    }

    switch(RType) {
        case D3DRTYPE_VERTEXBUFFER:
        case D3DRTYPE_INDEXBUFFER:
            WineD3DRType = WINED3DRTYPE_BUFFER;
            break;

        default:
            WineD3DRType = RType;
            break;
    }

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDeviceFormat(This->WineD3D, Adapter, DeviceType, wined3dformat_from_d3dformat(AdapterFormat),
            Usage, WineD3DRType, wined3dformat_from_d3dformat(CheckFormat), SURFACE_OPENGL);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceMultiSampleType(IDirect3D9Ex *iface, UINT Adapter,
        D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType,
        DWORD *pQualityLevels)
{
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, format %#x, windowed %#x, multisample_type %#x, levels %p.\n",
            iface, Adapter, DeviceType, SurfaceFormat, Windowed, MultiSampleType, pQualityLevels);

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDeviceMultiSampleType(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(SurfaceFormat), Windowed, MultiSampleType, pQualityLevels);
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDepthStencilMatch(IDirect3D9Ex *iface, UINT Adapter,
        D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat)
{
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, adapter_format %#x, rt_format %#x, ds_format %#x.\n",
            iface, Adapter, DeviceType, AdapterFormat, RenderTargetFormat, DepthStencilFormat);

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDepthStencilMatch(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(AdapterFormat), wined3dformat_from_d3dformat(RenderTargetFormat),
            wined3dformat_from_d3dformat(DepthStencilFormat));
    wined3d_mutex_unlock();

    return hr;
}

static HRESULT WINAPI IDirect3D9Impl_CheckDeviceFormatConversion(LPDIRECT3D9EX iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, src_format %#x, dst_format %#x.\n",
            iface, Adapter, DeviceType, SourceFormat, TargetFormat);

    wined3d_mutex_lock();
    hr = IWineD3D_CheckDeviceFormatConversion(This->WineD3D, Adapter, DeviceType,
            wined3dformat_from_d3dformat(SourceFormat), wined3dformat_from_d3dformat(TargetFormat));
    wined3d_mutex_unlock();

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

    pCaps->MaxVertexShaderConst = min(D3D9_MAX_VERTEX_SHADER_CONSTANTF, pCaps->MaxVertexShaderConst);
    pCaps->NumSimultaneousRTs = min(D3D9_MAX_SIMULTANEOUS_RENDERTARGETS, pCaps->NumSimultaneousRTs);
}

static HRESULT WINAPI IDirect3D9Impl_GetDeviceCaps(LPDIRECT3D9EX iface, UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) {
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    HRESULT hrc = D3D_OK;
    WINED3DCAPS *pWineCaps;

    TRACE("iface %p, adapter %u, device_type %#x, caps %p.\n", iface, Adapter, DeviceType, pCaps);

    if(NULL == pCaps){
        return D3DERR_INVALIDCALL;
    }
    pWineCaps = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINED3DCAPS));
    if(pWineCaps == NULL){
        return D3DERR_INVALIDCALL; /*well this is what MSDN says to return*/
    }
    memset(pCaps, 0, sizeof(*pCaps));

    wined3d_mutex_lock();
    hrc = IWineD3D_GetDeviceCaps(This->WineD3D, Adapter, DeviceType, pWineCaps);
    wined3d_mutex_unlock();

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

    TRACE("iface %p, adapter %u.\n", iface, Adapter);

    wined3d_mutex_lock();
    ret = IWineD3D_GetAdapterMonitor(This->WineD3D, Adapter);
    wined3d_mutex_unlock();

    return ret;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3D9Impl_CreateDevice(IDirect3D9Ex *iface, UINT adapter,
        D3DDEVTYPE device_type, HWND focus_window, DWORD flags, D3DPRESENT_PARAMETERS *parameters,
        IDirect3DDevice9 **device)
{
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    IDirect3DDevice9Impl *object;
    HRESULT hr;

    TRACE("iface %p, adapter %u, device_type %#x, focus_window %p, flags %#x, parameters %p, device %p.\n",
            iface, adapter, device_type, focus_window, flags, parameters, device);

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*object));
    if (!object)
    {
        ERR("Failed to allocate device memory.\n");
        return E_OUTOFMEMORY;
    }

    hr = device_init(object, This->WineD3D, adapter, device_type, focus_window, flags, parameters);
    if (FAILED(hr))
    {
        WARN("Failed to initialize device, hr %#x.\n", hr);
        HeapFree(GetProcessHeap(), 0, object);
        return hr;
    }

    TRACE("Created device %p.\n", object);
    *device = (IDirect3DDevice9 *)object;

    return D3D_OK;
}

static UINT WINAPI IDirect3D9ExImpl_GetAdapterModeCountEx(IDirect3D9Ex *iface,
        UINT adapter, const D3DDISPLAYMODEFILTER *filter)
{
    FIXME("iface %p, adapter %u, filter %p stub!\n", iface, adapter, filter);

    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI IDirect3D9ExImpl_EnumAdapterModesEx(IDirect3D9Ex *iface,
        UINT adapter, const D3DDISPLAYMODEFILTER *filter, UINT mode_idx, D3DDISPLAYMODEEX *mode)
{
    FIXME("iface %p, adapter %u, filter %p, mode_idx %u, mode %p stub!\n",
            iface, adapter, filter, mode_idx, mode);

    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI IDirect3D9ExImpl_GetAdapterDisplayModeEx(IDirect3D9Ex *iface,
        UINT adapter, D3DDISPLAYMODEEX *mode, D3DDISPLAYROTATION *rotation)
{
    FIXME("iface %p, adapter %u, mode %p, rotation %p stub!\n",
            iface, adapter, mode, rotation);

    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI DECLSPEC_HOTPATCH IDirect3D9ExImpl_CreateDeviceEx(IDirect3D9Ex *iface,
        UINT adapter, D3DDEVTYPE device_type, HWND focus_window, DWORD flags,
        D3DPRESENT_PARAMETERS *parameters, D3DDISPLAYMODEEX *mode, IDirect3DDevice9Ex **device)
{
    FIXME("iface %p, adapter %u, device_type %#x, focus_window %p, flags %#x,\n"
            "parameters %p, mode %p, device %p stub!\n",
            iface, adapter, device_type, focus_window, flags,
            parameters, mode, device);

    *device = NULL;

    return D3DERR_DRIVERINTERNALERROR;
}

static HRESULT WINAPI IDirect3D9ExImpl_GetAdapterLUID(IDirect3D9Ex *iface, UINT adapter, LUID *luid)
{
    IDirect3D9Impl *This = (IDirect3D9Impl *)iface;
    WINED3DADAPTER_IDENTIFIER adapter_id;
    HRESULT hr;

    TRACE("iface %p, adapter %u, luid %p.\n", iface, adapter, luid);

    adapter_id.driver_size = 0;
    adapter_id.description_size = 0;
    adapter_id.device_name_size = 0;

    wined3d_mutex_lock();
    hr = IWineD3D_GetAdapterIdentifier(This->WineD3D, adapter, 0, &adapter_id);
    wined3d_mutex_unlock();

    memcpy(luid, &adapter_id.adapter_luid, sizeof(*luid));

    return hr;
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
