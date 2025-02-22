/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_haldevice.c
 * PURPOSE:         d3d9.dll internal HAL device functions
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_haldevice.h"
#include <debug.h>

#define LOCK_D3DDEVICE9()     if (This->bLockDevice) EnterCriticalSection(&This->CriticalSection);
#define UNLOCK_D3DDEVICE9()   if (This->bLockDevice) LeaveCriticalSection(&This->CriticalSection);

/* Convert a IDirect3D9 pointer safely to the internal implementation struct */
/*static LPD3D9HALDEVICE IDirect3DDevice9ToImpl(LPDIRECT3DDEVICE9 iface)
{
    if (NULL == iface)
        return NULL;

    return (LPD3D9HALDEVICE)((ULONG_PTR)iface - FIELD_OFFSET(D3D9HALDEVICE, PureDevice.BaseDevice.lpVtbl));
}*/

/* IDirect3DDevice9 public interface */
HRESULT WINAPI IDirect3DDevice9HAL_GetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetMaterial(LPDIRECT3DDEVICE9 iface, D3DMATERIAL9* pMaterial)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, D3DLIGHT9* pLight)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetLightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL* pEnable)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, float* pPlane)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetClipStatus(LPDIRECT3DDEVICE9 iface, CONST D3DCLIPSTATUS9* pClipStatus)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetClipStatus(LPDIRECT3DDEVICE9 iface, D3DCLIPSTATUS9* pClipStatus)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_ValidateDevice(LPDIRECT3DDEVICE9 iface, DWORD* pNumPasses)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface, BOOL bSoftware)
{
    UNIMPLEMENTED

    return D3D_OK;
}

BOOL WINAPI IDirect3DDevice9HAL_GetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return TRUE;
}

HRESULT WINAPI IDirect3DDevice9HAL_ProcessVertices(LPDIRECT3DDEVICE9 iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_GetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/* IDirect3DDevice9 private interface */
HRESULT WINAPI IDirect3DDevice9HAL_SetRenderStateWorker(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetTextureStageStateInt(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetSamplerStateInt(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetMaterialInt(LPDIRECT3DDEVICE9 iface, CONST D3DMATERIAL9* pMaterial)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetVertexShaderInt(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetVertexShaderConstantFInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetVertexShaderConstantIInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetVertexShaderConstantBInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetPixelShaderInt(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetPixelShaderConstantFInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetPixelShaderConstantIInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetPixelShaderConstantBInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetFVFInt(LPDIRECT3DDEVICE9 iface, DWORD FVF)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetTextureInt(LPDIRECT3DDEVICE9 iface, DWORD Stage,IDirect3DBaseTexture9* pTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetIndicesInt(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9* pIndexData)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetStreamSourceInt(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetStreamSourceFreqInt(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,UINT Setting)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9HAL_UpdateRenderState(LPDIRECT3DDEVICE9 iface, DWORD Unknown1, DWORD Unknown2)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9HAL_SetTransformInt(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_MultiplyTransformInt(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetClipPlaneInt(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST float* pPlane)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9HAL_UpdateDriverState(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9HAL_SetViewportInt(LPDIRECT3DDEVICE9 iface, CONST D3DVIEWPORT9* pViewport)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9HAL_SetStreamSourceWorker(LPDIRECT3DDEVICE9 iface, LPVOID UnknownStreamData)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9HAL_SetPixelShaderConstantFWorker(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetPixelShaderConstantIWorker(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetPixelShaderConstantBWorker(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9HAL_DrawPrimitiveWorker(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9HAL_SetLightInt(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST D3DLIGHT9* pLight)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_LightEnableInt(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL Enable)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_SetRenderStateInt(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_DrawPrimitiveUPInt(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_ClearInt(LPDIRECT3DDEVICE9 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9HAL_DrawPrimitivesWorker(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

VOID WINAPI IDirect3DDevice9HAL_UpdateVertexShader(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9HAL_ValidateDrawCall(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT Unknown1, UINT Unknown2, UINT Unknown3, INT Unknown4, UINT Unknown5, INT Unknown6)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9HAL_Init(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9HAL_InitState(LPDIRECT3DDEVICE9 iface, INT State)
{
    UNIMPLEMENTED
}

VOID WINAPI IDirect3DDevice9HAL_Destroy(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

VOID WINAPI IDirect3DDevice9HAL_VirtualDestructor(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

IDirect3DDevice9Vtbl_INT Direct3DDevice9HAL_Vtbl =
{
    {
    /* IUnknown */
    IDirect3DDevice9Base_QueryInterface,
    IDirect3DDevice9Base_AddRef,
    IDirect3DDevice9Base_Release,

    /* IDirect3DDevice9 public */
    IDirect3DDevice9Base_TestCooperativeLevel,
    IDirect3DDevice9Base_GetAvailableTextureMem,
    IDirect3DDevice9Base_EvictManagedResources,
    IDirect3DDevice9Base_GetDirect3D,
    IDirect3DDevice9Base_GetDeviceCaps,
    IDirect3DDevice9Base_GetDisplayMode,
    IDirect3DDevice9Base_GetCreationParameters,
    IDirect3DDevice9Base_SetCursorProperties,
    IDirect3DDevice9Base_SetCursorPosition,
    IDirect3DDevice9Base_ShowCursor,
    IDirect3DDevice9Base_CreateAdditionalSwapChain,
    IDirect3DDevice9Base_GetSwapChain,
    IDirect3DDevice9Base_GetNumberOfSwapChains,
    IDirect3DDevice9Base_Reset,
    IDirect3DDevice9Base_Present,
    IDirect3DDevice9Base_GetBackBuffer,
    IDirect3DDevice9Base_GetRasterStatus,
    IDirect3DDevice9Base_SetDialogBoxMode,
    IDirect3DDevice9Base_SetGammaRamp,
    IDirect3DDevice9Base_GetGammaRamp,
    IDirect3DDevice9Base_CreateTexture,
    IDirect3DDevice9Base_CreateVolumeTexture,
    IDirect3DDevice9Base_CreateCubeTexture,
    IDirect3DDevice9Base_CreateVertexBuffer,
    IDirect3DDevice9Base_CreateIndexBuffer,
    IDirect3DDevice9Base_CreateRenderTarget,
    IDirect3DDevice9Base_CreateDepthStencilSurface,
    IDirect3DDevice9Base_UpdateSurface,
    IDirect3DDevice9Base_UpdateTexture,
    IDirect3DDevice9Base_GetRenderTargetData,
    IDirect3DDevice9Base_GetFrontBufferData,
    IDirect3DDevice9Base_StretchRect,
    IDirect3DDevice9Base_ColorFill,
    IDirect3DDevice9Base_CreateOffscreenPlainSurface,
    IDirect3DDevice9Pure_SetRenderTarget,
    IDirect3DDevice9Pure_GetRenderTarget,
    IDirect3DDevice9Pure_SetDepthStencilSurface,
    IDirect3DDevice9Pure_GetDepthStencilSurface,
    IDirect3DDevice9Pure_BeginScene,
    IDirect3DDevice9Pure_EndScene,
    IDirect3DDevice9Pure_Clear,
    IDirect3DDevice9Pure_SetTransform,
    IDirect3DDevice9HAL_GetTransform,
    IDirect3DDevice9Pure_MultiplyTransform,
    IDirect3DDevice9Pure_SetViewport,
    IDirect3DDevice9Pure_GetViewport,
    IDirect3DDevice9Pure_SetMaterial,
    IDirect3DDevice9HAL_GetMaterial,
    IDirect3DDevice9Pure_SetLight,
    IDirect3DDevice9HAL_GetLight,
    IDirect3DDevice9Pure_LightEnable,
    IDirect3DDevice9HAL_GetLightEnable,
    IDirect3DDevice9Pure_SetClipPlane,
    IDirect3DDevice9HAL_GetClipPlane,
    IDirect3DDevice9HAL_SetRenderState,
    IDirect3DDevice9HAL_GetRenderState,
    IDirect3DDevice9Pure_CreateStateBlock,
    IDirect3DDevice9Pure_BeginStateBlock,
    IDirect3DDevice9Pure_EndStateBlock,
    IDirect3DDevice9HAL_SetClipStatus,
    IDirect3DDevice9HAL_GetClipStatus,
    IDirect3DDevice9Pure_GetTexture,
    IDirect3DDevice9Pure_SetTexture,
    IDirect3DDevice9HAL_GetTextureStageState,
    IDirect3DDevice9Pure_SetTextureStageState,
    IDirect3DDevice9HAL_GetSamplerState,
    IDirect3DDevice9Pure_SetSamplerState,
    IDirect3DDevice9HAL_ValidateDevice,
    IDirect3DDevice9Pure_SetPaletteEntries,
    IDirect3DDevice9Pure_GetPaletteEntries,
    IDirect3DDevice9Pure_SetCurrentTexturePalette,
    IDirect3DDevice9Pure_GetCurrentTexturePalette,
    IDirect3DDevice9Pure_SetScissorRect,
    IDirect3DDevice9Pure_GetScissorRect,
    IDirect3DDevice9HAL_SetSoftwareVertexProcessing,
    IDirect3DDevice9HAL_GetSoftwareVertexProcessing,
    IDirect3DDevice9Pure_SetNPatchMode,
    IDirect3DDevice9Pure_GetNPatchMode,
    IDirect3DDevice9Pure_DrawPrimitive,
    IDirect3DDevice9Pure_DrawIndexedPrimitive,
    IDirect3DDevice9Pure_DrawPrimitiveUP,
    IDirect3DDevice9Pure_DrawIndexedPrimitiveUP,
    IDirect3DDevice9HAL_ProcessVertices,
    IDirect3DDevice9Pure_CreateVertexDeclaration,
    IDirect3DDevice9Pure_SetVertexDeclaration,
    IDirect3DDevice9Pure_GetVertexDeclaration,
    IDirect3DDevice9Pure_SetFVF,
    IDirect3DDevice9Pure_GetFVF,
    IDirect3DDevice9Pure_CreateVertexShader,
    IDirect3DDevice9Pure_SetVertexShader,
    IDirect3DDevice9HAL_GetVertexShader,
    IDirect3DDevice9Pure_SetVertexShaderConstantF,
    IDirect3DDevice9Pure_GetVertexShaderConstantF,
    IDirect3DDevice9Pure_SetVertexShaderConstantI,
    IDirect3DDevice9Pure_GetVertexShaderConstantI,
    IDirect3DDevice9Pure_SetVertexShaderConstantB,
    IDirect3DDevice9Pure_GetVertexShaderConstantB,
    IDirect3DDevice9Pure_SetStreamSource,
    IDirect3DDevice9Pure_GetStreamSource,
    IDirect3DDevice9Pure_SetStreamSourceFreq,
    IDirect3DDevice9Pure_GetStreamSourceFreq,
    IDirect3DDevice9Pure_SetIndices,
    IDirect3DDevice9Pure_GetIndices,
    IDirect3DDevice9Pure_CreatePixelShader,
    IDirect3DDevice9Pure_SetPixelShader,
    IDirect3DDevice9HAL_GetPixelShader,
    IDirect3DDevice9Pure_SetPixelShaderConstantF,
    IDirect3DDevice9HAL_GetPixelShaderConstantF,
    IDirect3DDevice9Pure_SetPixelShaderConstantI,
    IDirect3DDevice9HAL_GetPixelShaderConstantI,
    IDirect3DDevice9Pure_SetPixelShaderConstantB,
    IDirect3DDevice9HAL_GetPixelShaderConstantB,
    IDirect3DDevice9Pure_DrawRectPatch,
    IDirect3DDevice9Pure_DrawTriPatch,
    IDirect3DDevice9Pure_DeletePatch,
    IDirect3DDevice9Pure_CreateQuery,
    },

    /* IDirect3DDevice9 private */
    IDirect3DDevice9HAL_SetRenderStateWorker,
    IDirect3DDevice9HAL_SetTextureStageStateInt,
    IDirect3DDevice9HAL_SetSamplerStateInt,
    IDirect3DDevice9HAL_SetMaterialInt,
    IDirect3DDevice9HAL_SetVertexShaderInt,
    IDirect3DDevice9HAL_SetVertexShaderConstantFInt,
    IDirect3DDevice9HAL_SetVertexShaderConstantIInt,
    IDirect3DDevice9HAL_SetVertexShaderConstantBInt,
    IDirect3DDevice9HAL_SetPixelShaderInt,
    IDirect3DDevice9HAL_SetPixelShaderConstantFInt,
    IDirect3DDevice9HAL_SetPixelShaderConstantIInt,
    IDirect3DDevice9HAL_SetPixelShaderConstantBInt,
    IDirect3DDevice9HAL_SetFVFInt,
    IDirect3DDevice9HAL_SetTextureInt,
    IDirect3DDevice9HAL_SetIndicesInt,
    IDirect3DDevice9HAL_SetStreamSourceInt,
    IDirect3DDevice9HAL_SetStreamSourceFreqInt,
    IDirect3DDevice9HAL_UpdateRenderState,
    IDirect3DDevice9HAL_SetTransformInt,
    IDirect3DDevice9HAL_MultiplyTransformInt,
    IDirect3DDevice9HAL_SetClipPlaneInt,
    IDirect3DDevice9HAL_UpdateDriverState,
    IDirect3DDevice9HAL_SetViewportInt,
    IDirect3DDevice9HAL_SetStreamSourceWorker,
    IDirect3DDevice9HAL_SetPixelShaderConstantFWorker,
    IDirect3DDevice9HAL_SetPixelShaderConstantIWorker,
    IDirect3DDevice9HAL_SetPixelShaderConstantBWorker,
    IDirect3DDevice9HAL_DrawPrimitiveWorker,
    IDirect3DDevice9HAL_SetLightInt,
    IDirect3DDevice9HAL_LightEnableInt,
    IDirect3DDevice9HAL_SetRenderStateInt,
    IDirect3DDevice9HAL_DrawPrimitiveUPInt,
    IDirect3DDevice9HAL_ClearInt,
    IDirect3DDevice9HAL_DrawPrimitivesWorker,
    IDirect3DDevice9HAL_UpdateVertexShader,
    IDirect3DDevice9HAL_ValidateDrawCall,
    IDirect3DDevice9HAL_Init,
    IDirect3DDevice9HAL_InitState,
    IDirect3DDevice9HAL_Destroy,
    IDirect3DDevice9HAL_VirtualDestructor,
};

