/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS ReactX
 * FILE:            dll/directx/d3d9/d3d9_puredevice.c
 * PURPOSE:         d3d9.dll internal device functions
 * PROGRAMERS:      Gregor Gullwi <gbrunmar (dot) ros (at) gmail (dot) com>
 */
#include "d3d9_puredevice.h"
#include <debug.h>

#define LOCK_D3DDEVICE9()     if (This->bLockDevice) EnterCriticalSection(&This->CriticalSection);
#define UNLOCK_D3DDEVICE9()   if (This->bLockDevice) LeaveCriticalSection(&This->CriticalSection);

/* Convert a IDirect3D9 pointer safely to the internal implementation struct */
/*static LPD3D9PUREDEVICE IDirect3DDevice9ToImpl(LPDIRECT3DDEVICE9 iface)
{
    if (NULL == iface)
        return NULL;

    return (LPD3D9PUREDEVICE)((ULONG_PTR)iface - FIELD_OFFSET(D3D9PUREDEVICE, BaseDevice.lpVtbl));
}*/

/* IDirect3DDevice9 public interface */
HRESULT WINAPI IDirect3DDevice9Pure_SetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetRenderTarget(LPDIRECT3DDEVICE9 iface, DWORD RenderTargetIndex,IDirect3DSurface9** ppRenderTarget)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9* pNewZStencil)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetDepthStencilSurface(LPDIRECT3DDEVICE9 iface, IDirect3DSurface9** ppZStencilSurface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_BeginScene(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_EndScene(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_Clear(LPDIRECT3DDEVICE9 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_MultiplyTransform(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetViewport(LPDIRECT3DDEVICE9 iface, CONST D3DVIEWPORT9* pViewport)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetViewport(LPDIRECT3DDEVICE9 iface, D3DVIEWPORT9* pViewport)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetMaterial(LPDIRECT3DDEVICE9 iface, CONST D3DMATERIAL9* pMaterial)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetMaterial(LPDIRECT3DDEVICE9 iface, D3DMATERIAL9* pMaterial)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST D3DLIGHT9* pLight)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetLight(LPDIRECT3DDEVICE9 iface, DWORD Index, D3DLIGHT9* pLight)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_LightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL Enable)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetLightEnable(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL* pEnable)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST float* pPlane)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetClipPlane(LPDIRECT3DDEVICE9 iface, DWORD Index, float* pPlane)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetRenderState(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_CreateStateBlock(LPDIRECT3DDEVICE9 iface, D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_BeginStateBlock(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_EndStateBlock(LPDIRECT3DDEVICE9 iface, IDirect3DStateBlock9** ppSB)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetClipStatus(LPDIRECT3DDEVICE9 iface, CONST D3DCLIPSTATUS9* pClipStatus)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetClipStatus(LPDIRECT3DDEVICE9 iface, D3DCLIPSTATUS9* pClipStatus)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9** ppTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetTexture(LPDIRECT3DDEVICE9 iface, DWORD Stage, IDirect3DBaseTexture9* pTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetTextureStageState(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetSamplerState(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_ValidateDevice(LPDIRECT3DDEVICE9 iface, DWORD* pNumPasses)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, CONST PALETTEENTRY* pEntries)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetPaletteEntries(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber, PALETTEENTRY* pEntries)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT PaletteNumber)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetCurrentTexturePalette(LPDIRECT3DDEVICE9 iface, UINT* pPaletteNumber)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetScissorRect(LPDIRECT3DDEVICE9 iface, CONST RECT* pRect)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetScissorRect(LPDIRECT3DDEVICE9 iface, RECT* pRect)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface, BOOL bSoftware)
{
    UNIMPLEMENTED

    return D3D_OK;
}

BOOL WINAPI IDirect3DDevice9Pure_GetSoftwareVertexProcessing(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return TRUE;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetNPatchMode(LPDIRECT3DDEVICE9 iface, float nSegments)
{
    UNIMPLEMENTED

    return D3D_OK;
}

float WINAPI IDirect3DDevice9Pure_GetNPatchMode(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return 0.0f;
}

HRESULT WINAPI IDirect3DDevice9Pure_DrawPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_DrawIndexedPrimitive(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_DrawPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_DrawIndexedPrimitiveUP(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT MinVertexIndex, UINT NumVertices, UINT PrimitiveCount, CONST void* pIndexData, D3DFORMAT IndexDataFormat, CONST void* pVertexStreamZeroData, UINT VertexStreamZeroStride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_ProcessVertices(LPDIRECT3DDEVICE9 iface, UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_CreateVertexDeclaration(LPDIRECT3DDEVICE9 iface, CONST D3DVERTEXELEMENT9* pVertexElements, IDirect3DVertexDeclaration9** ppDecl)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9* pDecl)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetVertexDeclaration(LPDIRECT3DDEVICE9 iface, IDirect3DVertexDeclaration9** ppDecl)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetFVF(LPDIRECT3DDEVICE9 iface, DWORD FVF)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetFVF(LPDIRECT3DDEVICE9 iface, DWORD* pFVF)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_CreateVertexShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DVertexShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetVertexShader(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetVertexShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetVertexShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetVertexShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetStreamSource(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* pOffsetInBytes, UINT* pStride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,UINT Setting)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetStreamSourceFreq(LPDIRECT3DDEVICE9 iface, UINT StreamNumber, UINT* pSetting)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9* pIndexData)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetIndices(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9** ppIndexData)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_CreatePixelShader(LPDIRECT3DDEVICE9 iface, CONST DWORD* pFunction, IDirect3DPixelShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetPixelShader(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9** ppShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetPixelShaderConstantF(LPDIRECT3DDEVICE9 iface, UINT StartRegister, float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetPixelShaderConstantI(LPDIRECT3DDEVICE9 iface, UINT StartRegister, int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_GetPixelShaderConstantB(LPDIRECT3DDEVICE9 iface, UINT StartRegister, BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_DrawRectPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DRECTPATCH_INFO* pRectPatchInfo)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_DrawTriPatch(LPDIRECT3DDEVICE9 iface, UINT Handle, CONST float* pNumSegs, CONST D3DTRIPATCH_INFO* pTriPatchInfo)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_DeletePatch(LPDIRECT3DDEVICE9 iface, UINT Handle)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_CreateQuery(LPDIRECT3DDEVICE9 iface, D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery)
{
    UNIMPLEMENTED

    return D3D_OK;
}

/* IDirect3DDevice9 private interface */
HRESULT WINAPI IDirect3DDevice9Pure_SetRenderStateWorker(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetTextureStageStateInt(LPDIRECT3DDEVICE9 iface, DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetSamplerStateInt(LPDIRECT3DDEVICE9 iface, DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetMaterialInt(LPDIRECT3DDEVICE9 iface, CONST D3DMATERIAL9* pMaterial)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShaderInt(LPDIRECT3DDEVICE9 iface, IDirect3DVertexShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShaderConstantFInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShaderConstantIInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetVertexShaderConstantBInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderInt(LPDIRECT3DDEVICE9 iface, IDirect3DPixelShader9* pShader)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantFInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST float* pConstantData,UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantIInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST int* pConstantData,UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantBInt(LPDIRECT3DDEVICE9 iface, UINT StartRegister,CONST BOOL* pConstantData,UINT  BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetFVFInt(LPDIRECT3DDEVICE9 iface, DWORD FVF)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetTextureInt(LPDIRECT3DDEVICE9 iface, DWORD Stage,IDirect3DBaseTexture9* pTexture)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetIndicesInt(LPDIRECT3DDEVICE9 iface, IDirect3DIndexBuffer9* pIndexData)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetStreamSourceInt(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,IDirect3DVertexBuffer9* pStreamData,UINT OffsetInBytes,UINT Stride)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetStreamSourceFreqInt(LPDIRECT3DDEVICE9 iface, UINT StreamNumber,UINT Setting)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9Pure_UpdateRenderState(LPDIRECT3DDEVICE9 iface, DWORD Unknown1, DWORD Unknown2)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9Pure_SetTransformInt(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State,CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_MultiplyTransformInt(LPDIRECT3DDEVICE9 iface, D3DTRANSFORMSTATETYPE State, CONST D3DMATRIX* pMatrix)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetClipPlaneInt(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST float* pPlane)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9Pure_UpdateDriverState(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9Pure_SetViewportInt(LPDIRECT3DDEVICE9 iface, CONST D3DVIEWPORT9* pViewport)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9Pure_SetStreamSourceWorker(LPDIRECT3DDEVICE9 iface, LPVOID UnknownStreamData)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantFWorker(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST float* pConstantData, UINT Vector4fCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantIWorker(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST int* pConstantData, UINT Vector4iCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetPixelShaderConstantBWorker(LPDIRECT3DDEVICE9 iface, UINT StartRegister, CONST BOOL* pConstantData, UINT BoolCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9Pure_DrawPrimitiveWorker(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9Pure_SetLightInt(LPDIRECT3DDEVICE9 iface, DWORD Index, CONST D3DLIGHT9* pLight)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_LightEnableInt(LPDIRECT3DDEVICE9 iface, DWORD Index, BOOL Enable)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_SetRenderStateInt(LPDIRECT3DDEVICE9 iface, D3DRENDERSTATETYPE State, DWORD Value)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_DrawPrimitiveUPInt(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT PrimitiveCount)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_ClearInt(LPDIRECT3DDEVICE9 iface, DWORD Count, CONST D3DRECT* pRects, DWORD Flags, D3DCOLOR Color, float Z, DWORD Stencil)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9Pure_DrawPrimitivesWorker(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

VOID WINAPI IDirect3DDevice9Pure_UpdateVertexShader(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

HRESULT WINAPI IDirect3DDevice9Pure_ValidateDrawCall(LPDIRECT3DDEVICE9 iface, D3DPRIMITIVETYPE PrimitiveType, UINT Unknown1, UINT Unknown2, UINT Unknown3, INT Unknown4, UINT Unknown5, INT Unknown6)
{
    UNIMPLEMENTED

    return D3D_OK;
}

HRESULT WINAPI IDirect3DDevice9Pure_Init(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED

    return D3D_OK;
}

VOID WINAPI IDirect3DDevice9Pure_InitState(LPDIRECT3DDEVICE9 iface, INT State)
{
    UNIMPLEMENTED
}

VOID WINAPI IDirect3DDevice9Pure_Destroy(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}

VOID WINAPI IDirect3DDevice9Pure_VirtualDestructor(LPDIRECT3DDEVICE9 iface)
{
    UNIMPLEMENTED
}
