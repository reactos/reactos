/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_device.h
 * PURPOSE:         d3d9.dll internal device structures
 * PROGRAMERS:      Gregor Brunmar <gregor (dot) brunmar (at) home (dot) se>
 */
#ifndef _D3D9_DEVICE_H_
#define _D3D9_DEVICE_H_

#include "d3d9_common.h"
#include <d3d9.h>
#include <d3d9types.h>
#include "d3d9_private.h"
#include "d3d9_swapchain.h"
#include "d3d9_surface.h"

#if !defined(__cplusplus) || defined(CINTERFACE)
typedef struct _IDirect3DDevice9Vtbl_INT
{
    struct IDirect3DDevice9Vtbl PublicInterface;

    HRESULT (WINAPI *SetRenderStateWorker)(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value); // Value = D3DDEGREETYPE Degree );
    HRESULT (WINAPI *SetTextureStageStateI)(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value);
    HRESULT (WINAPI *SetSamplerStateI)(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value);
    HRESULT (WINAPI *SetMaterial)(LPDIRECT3DDEVICE9 iface, CONST D3DMATERIAL9* pMaterial);
    HRESULT (WINAPI *SetVertexShader)(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9* pShader);
    HRESULT (WINAPI *SetVertexShaderConstantF)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    HRESULT (WINAPI *SetVertexShaderConstantI)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
    HRESULT (WINAPI *SetVertexShaderConstantB)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    HRESULT (WINAPI *SetPixelShader)(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader);
    HRESULT (WINAPI *SetPixelShaderConstantF)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    HRESULT (WINAPI *SetPixelShaderConstantI)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
    HRESULT (WINAPI *SetPixelShaderConstantB)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    HRESULT (WINAPI *SetFVF)(LPDIRECT3DDEVICE9 iface, DWORD FVF);
    HRESULT (WINAPI *SetTexture)(LPDIRECT3DDEVICE9 iface, DWORD Stage,IDirect3DBaseTexture9* pTexture);
    HRESULT (WINAPI *SetIndices)(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9* pIndexData);
    HRESULT (WINAPI *SetStreamSource)(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride);
    HRESULT (WINAPI *SetStreamSourceFreq)(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,UINT Setting);
    VOID (WINAPI *UpdateRenderState)(LPDIRECT3DDEVICE9 iface, DWORD Unknown1, DWORD Unknown2);
    HRESULT (WINAPI *SetTransform)(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix);
    HRESULT (WINAPI *MultiplyTransform)(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE,CONST D3DMATRIX*);
    HRESULT (WINAPI *SetClipPlane)(LPDIRECT3DDEVICE9 iface, DWORD Index,CONST float* pPlane);
    VOID (WINAPI *UpdateDriverState)(LPDIRECT3DDEVICE9 iface);
    HRESULT (WINAPI *SetViewport)(LPDIRECT3DDEVICE9 iface, CONST D3DVIEWPORT9* pViewport);
    VOID (WINAPI *SetStreamSourceInt)(LPDIRECT3DDEVICE9 iface, LPVOID UnknownStreamData);
    HRESULT (WINAPI *SetPixelShaderConstantFWorker)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount);
    HRESULT (WINAPI *SetPixelShaderConstantIWorker)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount);
    HRESULT (WINAPI *SetPixelShaderConstantBWorker)(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount);
    VOID (WINAPI *DrawPrimitiveWorker)(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType,UINT StartVertex,UINT PrimitiveCount);
    HRESULT (WINAPI *SetLight)(LPDIRECT3DDEVICE9 iface, DWORD Index,CONST D3DLIGHT9*);
    HRESULT (WINAPI *LightEnable)(LPDIRECT3DDEVICE9 iface, DWORD Index,BOOL Enable);
    HRESULT (WINAPI *SetRenderStateInt)(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value ); // Value = D3DDEGREETYPE Degree );
    HRESULT (WINAPI *DrawPrimitiveUPInt)(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount);
    HRESULT (WINAPI *Clear)(LPDIRECT3DDEVICE9 iface, DWORD Count,CONST D3DRECT* pRects,DWORD Flags,D3DCOLOR Color,float Z,DWORD Stencil);
    VOID (WINAPI *DrawPrimitivesWorker)(LPDIRECT3DDEVICE9 iface);
    VOID (WINAPI *UpdateVertexShader)(LPDIRECT3DDEVICE9 iface);
    HRESULT (WINAPI *ValidateDrawCall)(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT Unknown1, UINT Unknown2, UINT Unknown3, INT Unknown4, UINT Unknown5, INT Unknown6);
    HRESULT (WINAPI *Init)(LPDIRECT3DDEVICE9 iface);
    VOID (WINAPI *InitState)(LPDIRECT3DDEVICE9 iface, INT State);
    VOID (WINAPI *Destroy)(LPDIRECT3DDEVICE9 iface);
    VOID (WINAPI *VirtualDestructor)(LPDIRECT3DDEVICE9 iface);
} IDirect3DDevice9Vtbl_INT;
#endif

typedef struct _D3D9HeapTexture
{
/* 0x0000 */    DWORD dwUnknown00;
/* 0x0004 */    DWORD dwUnknown04;  // 0x400
/* 0x0008 */    LPDWORD pUnknown08; // malloc(dwUnknown04 * 2)
} D3D9HeapTexture;

typedef struct _D3D9ResourceManager
{
#ifdef D3D_DEBUG_INFO
/* N/A    - 0x0000 */   DDSURFACEDESC SurfaceDesc[8];
#endif
/* 0x0000 - 0x0160 */   struct _Direct3DDevice9_INT* pBaseDevice;
/* 0x0004 - 0x0164 */   DWORD dwUnknown0004;
/* 0x0008 - 0x0168 */   DWORD dwUnknown0008;
/* 0x000c - 0x016c */   DWORD MaxSimultaneousTextures;
/* 0x0010 - 0x0170 */   DWORD dwUnknown0010;
/* 0x0014 - 0x0174 */   D3D9HeapTexture* pTextureHeap;
} D3D9ResourceManager;

typedef struct _Direct3DDevice9_INT
{
/* 0x0000 */    struct _IDirect3DDevice9Vtbl_INT* lpVtbl;
/* 0x0004 */    CRITICAL_SECTION CriticalSection;
#ifdef D3D_DEBUG_INFO
/* N/A    - 0x001c */   DWORD dbg0004;
/* N/A    - 0x0020 */   DWORD dbg0008;
/* N/A    - 0x0024 */   DWORD dbg000c;
/* N/A    - 0x0028 */   DWORD dbg0010;
/* N/A    - 0x002c */   DWORD dbg0014;
/* N/A    - 0x0030 */   DWORD dbg0018;
/* N/A    - 0x0034 */   DWORD dbg001c;
/* N/A    - 0x0038 */   DWORD dbg0020;
/* N/A    - 0x003c */   DWORD dbg0024;
/* N/A    - 0x0040 */   DWORD dbg0028;
/* N/A    - 0x0044 */   DWORD dbg002c;
/* N/A    - 0x0048 */   DWORD dbg0030;
/* N/A    - 0x004c */   DWORD dbg0034;
/* N/A    - 0x0050 */   DWORD dbg0038;
#endif
/* 0x001c - 0x0054 */   BOOL bLockDevice;
/* 0x0020 - 0x0058 */   DWORD dwProcessId;
/* 0x0024 - 0x005c */   IUnknown* pUnknown;
/* 0x0028 - 0x0060 */   DWORD dwDXVersion;
/* 0x002c - 0x0064 */   DWORD unknown000011;
/* 0x0030 - 0x0068 */   LONG lRefCnt;
/* 0x0034 - 0x006c */   DWORD unknown000013;
/* 0x0038 - 0x0070 */   D3D9ResourceManager* pResourceManager;
/* 0x003c - 0x0074 */   HWND hWnd;
/* 0x0040 - 0x0078 */   DWORD AdjustedBehaviourFlags;
/* 0x0044 - 0x007c */   DWORD BehaviourFlags;
/* 0x0048 - 0x0080 */   D3D9BaseSurface* pUnknown0010;
/* 0x004c - 0x0084 */   DWORD NumAdaptersInDevice;
/* 0x0050 - 0x0088 */   D3DDISPLAYMODE CurrentDisplayMode[D3D9_INT_MAX_NUM_ADAPTERS];
/* 0x0110 - 0x0148 */   DWORD AdapterIndexInGroup[D3D9_INT_MAX_NUM_ADAPTERS];
/* 0x0140 - 0x0178 */   D3D9_DEVICEDATA DeviceData[D3D9_INT_MAX_NUM_ADAPTERS];
/* 0x1df0 - 0x1e28 */   LPDIRECT3DSWAPCHAIN9_INT pSwapChains[D3D9_INT_MAX_NUM_ADAPTERS];
/* 0x1e20 - 0x1e58 */   LPDIRECT3DSWAPCHAIN9_INT pSwapChains2[D3D9_INT_MAX_NUM_ADAPTERS];
/* 0x1e50 */    D3D9BaseSurface* pRenderTargetList;
/* 0x1e54 */    DWORD unknown001941;
/* 0x1e58 */    DWORD unknown001942;
/* 0x1e5c */    DWORD unknown001943;
/* 0x1e60 */    D3D9BaseSurface* pUnknown001944;
/* 0x1e64 */    D3DDEVTYPE DeviceType;
/* 0x1e68 */    LPDIRECT3D9_INT pDirect3D9;
/* 0x1e6c */    D3D9DriverSurface* pDriverSurfaceList;
/* 0x1e70 */    DWORD unknown001948;
/* 0x1e74 */    HANDLE hDX10UMDriver;
/* 0x1e78 */    HANDLE hDX10UMDriverInst;
/* 0x1e7c */    DWORD unknown001951;
/* 0x1e80 */    DWORD unknown001952;
/* 0x1e84 */    DWORD unknown001953;
/* 0x1e88 */    DWORD unknown001954;
/* 0x1e8c */    DWORD unknown001955;
/* 0x1e90 */    DWORD unknown001956;
/* 0x1e94 */    DWORD unknown001957;
/* 0x1e98 */    DWORD unknown001958;
/* 0x1e9c */    DWORD unknown001959;
/* 0x1ea0 */    DWORD unknown001960;
/* 0x1ea4 */    DWORD unknown001961;
/* 0x1ea8 */    DWORD unknown001962;
/* 0x1eac */    DWORD unknown001963;
/* 0x1eb0 */    DWORD unknown001964;
/* 0x1eb4 */    DWORD unknown001965;
/* 0x1eb8 */    DWORD unknown001966;
/* 0x1ebc */    DWORD unknown001967;
/* 0x1ec0 */    DWORD unknown001968;
/* 0x1ec4 */    DWORD unknown001969;
/* 0x1ec8 */    DWORD unknown001970;
/* 0x1ecc */    DWORD unknown001971;
/* 0x1ed0 */    DWORD unknown001972;
/* 0x1ed4 */    DWORD unknown001973;
/* 0x1ed8 */    DWORD unknown001974;
/* 0x1edc */    DWORD unknown001975;
/* 0x1ee0 */    DWORD unknown001976;
/* 0x1ee4 */    DWORD unknown001977;
/* 0x1ee8 */    DWORD unknown001978;
/* 0x1eec */    DWORD unknown001979;
/* 0x1ef0 */    DWORD unknown001980;
/* 0x1ef4 */    DWORD unknown001981;
/* 0x1ef8 */    DWORD unknown001982;
/* 0x1efc */    DWORD unknown001983;
/* 0x1f00 */    DWORD unknown001984;
/* 0x1f04 */    DWORD unknown001985;
/* 0x1f08 */    DWORD unknown001986;
/* 0x1f0c */    DWORD unknown001987;
/* 0x1f10 */    DWORD unknown001988;
/* 0x1f14 */    DWORD unknown001989;
/* 0x1f18 */    DWORD unknown001990;
/* 0x1f1c */    DWORD unknown001991;
/* 0x1f20 */    DWORD unknown001992;
/* 0x1f24 */    DWORD unknown001993;
/* 0x1f28 */    DWORD unknown001994;
/* 0x1f2c */    DWORD unknown001995;
/* 0x1f30 */    DWORD unknown001996;
/* 0x1f34 */    DWORD unknown001997;
/* 0x1f38 */    DWORD unknown001998;
/* 0x1f3c */    DWORD unknown001999;
/* 0x1f40 */    DWORD unknown002000;
/* 0x1f44 */    DWORD unknown002001;
} DIRECT3DDEVICE9_INT, FAR* LPDIRECT3DDEVICE9_INT;

/* Helper functions */
LPDIRECT3DDEVICE9_INT IDirect3DDevice9ToImpl(LPDIRECT3DDEVICE9 iface);

/* IUnknown interface */
HRESULT WINAPI IDirect3DDevice9Base_QueryInterface(LPDIRECT3DDEVICE9 iface, REFIID riid, void** ppvObject);
ULONG WINAPI IDirect3DDevice9Base_AddRef(LPDIRECT3DDEVICE9 iface);
ULONG WINAPI IDirect3DDevice9Base_Release(LPDIRECT3DDEVICE9 iface);

/* IDirect3DDevice9 public interface */
HRESULT WINAPI IDirect3DDevice9Base_TestCooperativeLevel(LPDIRECT3DDEVICE9 iface);
UINT WINAPI IDirect3DDevice9Base_GetAvailableTextureMem(LPDIRECT3DDEVICE9 iface);
HRESULT WINAPI IDirect3DDevice9Base_EvictManagedResources(LPDIRECT3DDEVICE9 iface);
HRESULT WINAPI IDirect3DDevice9Base_GetDirect3D(LPDIRECT3DDEVICE9 iface, IDirect3D9** ppD3D9);
HRESULT WINAPI IDirect3DDevice9Base_GetDeviceCaps(LPDIRECT3DDEVICE9 iface, D3DCAPS9* pCaps);
HRESULT WINAPI IDirect3DDevice9Base_GetDisplayMode(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DDISPLAYMODE* pMode);
HRESULT WINAPI IDirect3DDevice9Base_GetCreationParameters(LPDIRECT3DDEVICE9 iface, D3DDEVICE_CREATION_PARAMETERS* pParameters);
HRESULT WINAPI IDirect3DDevice9Base_SetCursorProperties(LPDIRECT3DDEVICE9 iface, UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap);
VOID WINAPI IDirect3DDevice9Base_SetCursorPosition(LPDIRECT3DDEVICE9 iface, int X, int Y, DWORD Flags);
BOOL WINAPI IDirect3DDevice9Base_ShowCursor(LPDIRECT3DDEVICE9 iface, BOOL bShow);
HRESULT WINAPI IDirect3DDevice9Base_CreateAdditionalSwapChain(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** ppSwapChain);
HRESULT WINAPI IDirect3DDevice9Base_GetSwapChain(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSwapChain9** ppSwapChain);
UINT WINAPI IDirect3DDevice9Base_GetNumberOfSwapChains(LPDIRECT3DDEVICE9 iface);
HRESULT WINAPI IDirect3DDevice9Base_Reset(LPDIRECT3DDEVICE9 iface, D3DPRESENT_PARAMETERS* pPresentationParameters);
HRESULT WINAPI IDirect3DDevice9Base_Present(LPDIRECT3DDEVICE9 iface, CONST RECT* pSourceRect, CONST RECT* pDestRect, HWND hDestWindowOverride, CONST RGNDATA* pDirtyRegion);
HRESULT WINAPI IDirect3DDevice9Base_GetBackBuffer(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer);
HRESULT WINAPI IDirect3DDevice9Base_GetRasterStatus(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus);
HRESULT WINAPI IDirect3DDevice9Base_SetDialogBoxMode(LPDIRECT3DDEVICE9 iface, BOOL bEnableDialogs);
VOID WINAPI IDirect3DDevice9Base_SetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, DWORD Flags, CONST D3DGAMMARAMP* pRamp);
VOID WINAPI IDirect3DDevice9Base_GetGammaRamp(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, D3DGAMMARAMP* pRamp);
HRESULT WINAPI IDirect3DDevice9Base_CreateTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle);
HRESULT WINAPI IDirect3DDevice9Base_CreateVolumeTexture(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle);
HRESULT WINAPI IDirect3DDevice9Base_CreateCubeTexture(LPDIRECT3DDEVICE9 iface, UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle);
HRESULT WINAPI IDirect3DDevice9Base_CreateVertexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle);
HRESULT WINAPI IDirect3DDevice9Base_CreateIndexBuffer(LPDIRECT3DDEVICE9 iface, UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle);
HRESULT WINAPI IDirect3DDevice9Base_CreateRenderTarget(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
HRESULT WINAPI IDirect3DDevice9Base_CreateDepthStencilSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, BOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);
HRESULT WINAPI IDirect3DDevice9Base_UpdateSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestinationSurface, CONST POINT* pDestPoint);
HRESULT WINAPI IDirect3DDevice9Base_UpdateTexture(LPDIRECT3DDEVICE9 iface, IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture);
HRESULT WINAPI IDirect3DDevice9Base_GetRenderTargetData(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface);
HRESULT WINAPI IDirect3DDevice9Base_GetFrontBufferData(LPDIRECT3DDEVICE9 iface, UINT iSwapChain, IDirect3DSurface9* pDestSurface);
HRESULT WINAPI IDirect3DDevice9Base_StretchRect(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSourceSurface, CONST RECT* pSourceRect, IDirect3DSurface9* pDestSurface, CONST RECT* pDestRect, D3DTEXTUREFILTERTYPE Filter);
HRESULT WINAPI IDirect3DDevice9Base_ColorFill(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pSurface, CONST RECT* pRect, D3DCOLOR color);
HRESULT WINAPI IDirect3DDevice9Base_CreateOffscreenPlainSurface(LPDIRECT3DDEVICE9 iface, UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle);

/* IDirect3DDevice9 private interface */
VOID WINAPI IDirect3DDevice9Base_Destroy(LPDIRECT3DDEVICE9 iface);
VOID WINAPI IDirect3DDevice9Base_VirtualDestructor(LPDIRECT3DDEVICE9 iface);

#endif /* _D3D9_DEVICE_H_ */
