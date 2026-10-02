// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//      Helpers for standalone executable to generate shader sources.
//
//------------------------------------------------------------------------

#include "precomp.h"

//+----------------------------------------------------------------------------
//
//  Member:    static CFakeDevice::Create
//
//  Synopsis:  Create an instance of CFakeDevice.
//
//-----------------------------------------------------------------------------
HRESULT
CFakeDevice::Create(
    __deref_out_ecount(1) IDirect3DDevice9 **ppDevice
    )
{
    HRESULT hr = S_OK;
    CFakeDevice *pDevice = new CFakeDevice;
    IFCOOM(pDevice);
    *ppDevice = pDevice;
    pDevice = NULL;
Cleanup:
    ReleaseInterface(pDevice);
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Member:    CFakeDevice::CFakeDevice
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------
CFakeDevice::CFakeDevice()
{
    m_cRef = 1;
}

//+----------------------------------------------------------------------------
//
//  Member:    CFakeDevice::~CFakeDevice
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------
CFakeDevice::~CFakeDevice()
{
}

//+----------------------------------------------------------------------------
//
//  Member:    CFakeDevice::AddRef
//
//  Synopsis:  Classical IUnknown::AddRef
//
//-----------------------------------------------------------------------------
ULONG
CFakeDevice::AddRef()
{
    return ++m_cRef;
}

//+----------------------------------------------------------------------------
//
//  Member:    CFakeDevice::Release
//
//  Synopsis:  Classical IUnknown::Release
//
//-----------------------------------------------------------------------------
ULONG
CFakeDevice::Release()
{
    ULONG cRef = --m_cRef;
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

//+----------------------------------------------------------------------------
//
//  Member:    CFakeDevice::CreatePixelShader
//
//  Synopsis:  Do nothing, return bad hr.
//             D3DX stuff will provide debug message but continue working.
//
//-----------------------------------------------------------------------------
HRESULT
CFakeDevice::CreatePixelShader(
    DWORD const *pFunction,
    IDirect3DPixelShader9 **ppShader
    )
{
    return D3DERR_INVALIDCALL;
}

//+----------------------------------------------------------------------------
//
//  Member:    CFakeDevice::CreateVertexShader
//
//  Synopsis:  Do nothing, return bad hr.
//             D3DX stuff will provide debug message but continue working.
//
//-----------------------------------------------------------------------------
HRESULT
CFakeDevice::CreateVertexShader(
    DWORD const *pFunction,
    IDirect3DVertexShader9** ppShader
    )
{
    return D3DERR_INVALIDCALL;
}

//+----------------------------------------------------------------------------
//
//  Fake Members:   CFakeDevice::*
//
//  Synopsis:
//      Just a stubs to make compiler happy.
//      These methods are not used.
//      To ensure, we call DebugBreak() on each member implementation.
//
//-----------------------------------------------------------------------------
    
#define BREAK DebugBreak();
#define STUB {BREAK; return E_FAIL;}


HRESULT CFakeDevice::QueryInterface(struct _GUID const &,void * *) STUB

HRESULT CFakeDevice::GetMaximumFrameLatency(unsigned int *) STUB
HRESULT CFakeDevice::SetMaximumFrameLatency(unsigned int) STUB
HRESULT CFakeDevice::WaitForVBlank(unsigned int) STUB
HRESULT CFakeDevice::SetGPUThreadPriority(unsigned int) STUB
HRESULT CFakeDevice::GetGPUThreadPriority(unsigned int *) STUB
HRESULT CFakeDevice::PresentEx(struct tagRECT const *,struct tagRECT const *,struct HWND__ *,struct _RGNDATA const *,unsigned long,struct IDirect3DSurface9 *) STUB
HRESULT CFakeDevice::ComposeRects(struct IDirect3DSurface9 *,struct IDirect3DSurface9 *,struct IDirect3DVertexBuffer9 *,unsigned int,struct IDirect3DVertexBuffer9 *,enum _D3DCOMPOSERECTSOP,int,int) STUB
HRESULT CFakeDevice::SetConvolutionMonoKernel(unsigned int,unsigned int,float *,float *) STUB
HRESULT CFakeDevice::CreateQuery(enum _D3DQUERYTYPE,struct IDirect3DQuery9 * *) STUB
HRESULT CFakeDevice::DeletePatch(unsigned int) STUB
HRESULT CFakeDevice::DrawTriPatch(unsigned int,float const *,struct _D3DTRIPATCH_INFO const *) STUB
HRESULT CFakeDevice::DrawRectPatch(unsigned int,float const *,struct _D3DRECTPATCH_INFO const *) STUB
HRESULT CFakeDevice::GetPixelShaderConstantB(unsigned int,int *,unsigned int) STUB
HRESULT CFakeDevice::SetPixelShaderConstantB(unsigned int,int const *,unsigned int) STUB
HRESULT CFakeDevice::GetPixelShaderConstantI(unsigned int,int *,unsigned int) STUB
HRESULT CFakeDevice::SetPixelShaderConstantI(unsigned int,int const *,unsigned int) STUB
HRESULT CFakeDevice::GetPixelShaderConstantF(unsigned int,float *,unsigned int) STUB
HRESULT CFakeDevice::SetPixelShaderConstantF(unsigned int,float const *,unsigned int) STUB
HRESULT CFakeDevice::GetPixelShader(struct IDirect3DPixelShader9 * *) STUB
HRESULT CFakeDevice::SetPixelShader(struct IDirect3DPixelShader9 *) STUB
HRESULT CFakeDevice::GetIndices(struct IDirect3DIndexBuffer9 * *) STUB
HRESULT CFakeDevice::SetIndices(struct IDirect3DIndexBuffer9 *) STUB
HRESULT CFakeDevice::GetStreamSourceFreq(unsigned int,unsigned int *) STUB
HRESULT CFakeDevice::SetStreamSourceFreq(unsigned int,unsigned int) STUB
HRESULT CFakeDevice::GetStreamSource(unsigned int,struct IDirect3DVertexBuffer9 * *,unsigned int *,unsigned int *) STUB
HRESULT CFakeDevice::SetStreamSource(unsigned int,struct IDirect3DVertexBuffer9 *,unsigned int,unsigned int) STUB
HRESULT CFakeDevice::GetVertexShaderConstantB(unsigned int,int *,unsigned int) STUB
HRESULT CFakeDevice::SetVertexShaderConstantB(unsigned int,int const *,unsigned int) STUB
HRESULT CFakeDevice::GetVertexShaderConstantI(unsigned int,int *,unsigned int) STUB
HRESULT CFakeDevice::SetVertexShaderConstantI(unsigned int,int const *,unsigned int) STUB
HRESULT CFakeDevice::GetVertexShaderConstantF(unsigned int,float *,unsigned int) STUB
HRESULT CFakeDevice::SetVertexShaderConstantF(unsigned int,float const *,unsigned int) STUB
HRESULT CFakeDevice::GetVertexShader(struct IDirect3DVertexShader9 * *) STUB
HRESULT CFakeDevice::SetVertexShader(struct IDirect3DVertexShader9 *) STUB
HRESULT CFakeDevice::GetFVF(unsigned long *) STUB
HRESULT CFakeDevice::SetFVF(unsigned long) STUB
HRESULT CFakeDevice::GetVertexDeclaration(struct IDirect3DVertexDeclaration9 * *) STUB
HRESULT CFakeDevice::SetVertexDeclaration(struct IDirect3DVertexDeclaration9 *) STUB
HRESULT CFakeDevice::CreateVertexDeclaration(struct _D3DVERTEXELEMENT9 const *,struct IDirect3DVertexDeclaration9 * *) STUB
HRESULT CFakeDevice::ProcessVertices(unsigned int,unsigned int,unsigned int,struct IDirect3DVertexBuffer9 *,struct IDirect3DVertexDeclaration9 *,unsigned long) STUB
HRESULT CFakeDevice::DrawIndexedPrimitiveUP(enum _D3DPRIMITIVETYPE,unsigned int,unsigned int,unsigned int,void const *,enum _D3DFORMAT,void const *,unsigned int) STUB
HRESULT CFakeDevice::DrawPrimitiveUP(enum _D3DPRIMITIVETYPE,unsigned int,void const *,unsigned int) STUB
HRESULT CFakeDevice::DrawIndexedPrimitive(enum _D3DPRIMITIVETYPE,int,unsigned int,unsigned int,unsigned int,unsigned int) STUB
HRESULT CFakeDevice::DrawPrimitive(enum _D3DPRIMITIVETYPE,unsigned int,unsigned int) STUB
float CFakeDevice::GetNPatchMode(void) {BREAK; return 0;}
HRESULT CFakeDevice::SetNPatchMode(float) STUB
int CFakeDevice::GetSoftwareVertexProcessing(void) {BREAK; return 0;}
HRESULT CFakeDevice::SetSoftwareVertexProcessing(int) STUB
HRESULT CFakeDevice::GetScissorRect(struct tagRECT *) STUB
HRESULT CFakeDevice::SetScissorRect(struct tagRECT const *) STUB
HRESULT CFakeDevice::GetCurrentTexturePalette(unsigned int *) STUB
HRESULT CFakeDevice::SetCurrentTexturePalette(unsigned int) STUB
HRESULT CFakeDevice::GetPaletteEntries(unsigned int,struct tagPALETTEENTRY *) STUB
HRESULT CFakeDevice::SetPaletteEntries(unsigned int,struct tagPALETTEENTRY const *) STUB
HRESULT CFakeDevice::ValidateDevice(unsigned long *) STUB
HRESULT CFakeDevice::SetSamplerState(unsigned long,enum _D3DSAMPLERSTATETYPE,unsigned long) STUB
HRESULT CFakeDevice::GetSamplerState(unsigned long,enum _D3DSAMPLERSTATETYPE,unsigned long *) STUB
HRESULT CFakeDevice::SetTextureStageState(unsigned long,enum _D3DTEXTURESTAGESTATETYPE,unsigned long) STUB
HRESULT CFakeDevice::GetTextureStageState(unsigned long,enum _D3DTEXTURESTAGESTATETYPE,unsigned long *) STUB
HRESULT CFakeDevice::SetTexture(unsigned long,struct IDirect3DBaseTexture9 *) STUB
HRESULT CFakeDevice::GetTexture(unsigned long,struct IDirect3DBaseTexture9 * *) STUB
HRESULT CFakeDevice::GetClipStatus(struct _D3DCLIPSTATUS9 *) STUB
HRESULT CFakeDevice::SetClipStatus(struct _D3DCLIPSTATUS9 const *) STUB
HRESULT CFakeDevice::EndStateBlock(struct IDirect3DStateBlock9 * *) STUB
HRESULT CFakeDevice::BeginStateBlock(void) STUB
HRESULT CFakeDevice::CreateStateBlock(enum _D3DSTATEBLOCKTYPE,struct IDirect3DStateBlock9 * *) STUB
HRESULT CFakeDevice::GetRenderState(enum _D3DRENDERSTATETYPE,unsigned long *) STUB
HRESULT CFakeDevice::SetRenderState(enum _D3DRENDERSTATETYPE,unsigned long) STUB
HRESULT CFakeDevice::GetClipPlane(unsigned long,float *) STUB
HRESULT CFakeDevice::SetClipPlane(unsigned long,float const *) STUB
HRESULT CFakeDevice::GetLightEnable(unsigned long,int *) STUB
HRESULT CFakeDevice::LightEnable(unsigned long,int) STUB
HRESULT CFakeDevice::GetLight(unsigned long,struct _D3DLIGHT9 *) STUB
HRESULT CFakeDevice::SetLight(unsigned long,struct _D3DLIGHT9 const *) STUB
HRESULT CFakeDevice::GetMaterial(struct _D3DMATERIAL9 *) STUB
HRESULT CFakeDevice::SetMaterial(struct _D3DMATERIAL9 const *) STUB
HRESULT CFakeDevice::GetViewport(struct _D3DVIEWPORT9 *) STUB
HRESULT CFakeDevice::SetViewport(struct _D3DVIEWPORT9 const *) STUB
HRESULT CFakeDevice::MultiplyTransform(enum _D3DTRANSFORMSTATETYPE,struct _D3DMATRIX const *) STUB
HRESULT CFakeDevice::GetTransform(enum _D3DTRANSFORMSTATETYPE,struct _D3DMATRIX *) STUB
HRESULT CFakeDevice::SetTransform(enum _D3DTRANSFORMSTATETYPE,struct _D3DMATRIX const *) STUB
HRESULT CFakeDevice::Clear(unsigned long,struct _D3DRECT const *,unsigned long,unsigned long,float,unsigned long) STUB
HRESULT CFakeDevice::EndScene(void) STUB
HRESULT CFakeDevice::BeginScene(void) STUB
HRESULT CFakeDevice::GetDepthStencilSurface(struct IDirect3DSurface9 * *) STUB
HRESULT CFakeDevice::SetDepthStencilSurface(struct IDirect3DSurface9 *) STUB
HRESULT CFakeDevice::GetRenderTarget(unsigned long,struct IDirect3DSurface9 * *) STUB
HRESULT CFakeDevice::SetRenderTarget(unsigned long,struct IDirect3DSurface9 *) STUB
HRESULT CFakeDevice::CreateOffscreenPlainSurface(unsigned int,unsigned int,enum _D3DFORMAT,enum _D3DPOOL,struct IDirect3DSurface9 * *,void * *) STUB
HRESULT CFakeDevice::ColorFill(struct IDirect3DSurface9 *,struct tagRECT const *,unsigned long) STUB
HRESULT CFakeDevice::StretchRect(struct IDirect3DSurface9 *,struct tagRECT const *,struct IDirect3DSurface9 *,struct tagRECT const *,enum _D3DTEXTUREFILTERTYPE) STUB
HRESULT CFakeDevice::GetFrontBufferData(unsigned int,struct IDirect3DSurface9 *) STUB
HRESULT CFakeDevice::GetRenderTargetData(struct IDirect3DSurface9 *,struct IDirect3DSurface9 *) STUB
HRESULT CFakeDevice::UpdateTexture(struct IDirect3DBaseTexture9 *,struct IDirect3DBaseTexture9 *) STUB
HRESULT CFakeDevice::UpdateSurface(struct IDirect3DSurface9 *,struct tagRECT const *,struct IDirect3DSurface9 *,struct tagPOINT const *) STUB
HRESULT CFakeDevice::CreateDepthStencilSurface(unsigned int,unsigned int,enum _D3DFORMAT,enum _D3DMULTISAMPLE_TYPE,unsigned long,int,struct IDirect3DSurface9 * *,void * *) STUB
HRESULT CFakeDevice::CreateRenderTarget(unsigned int,unsigned int,enum _D3DFORMAT,enum _D3DMULTISAMPLE_TYPE,unsigned long,int,struct IDirect3DSurface9 * *,void * *) STUB
HRESULT CFakeDevice::CreateIndexBuffer(unsigned int,unsigned long,enum _D3DFORMAT,enum _D3DPOOL,struct IDirect3DIndexBuffer9 * *,void * *) STUB
HRESULT CFakeDevice::CreateVertexBuffer(unsigned int,unsigned long,unsigned long,enum _D3DPOOL,struct IDirect3DVertexBuffer9 * *,void * *) STUB
HRESULT CFakeDevice::CreateCubeTexture(unsigned int,unsigned int,unsigned long,enum _D3DFORMAT,enum _D3DPOOL,struct IDirect3DCubeTexture9 * *,void * *) STUB
HRESULT CFakeDevice::CreateVolumeTexture(unsigned int,unsigned int,unsigned int,unsigned int,unsigned long,enum _D3DFORMAT,enum _D3DPOOL,struct IDirect3DVolumeTexture9 * *,void * *) STUB
HRESULT CFakeDevice::CreateTexture(unsigned int,unsigned int,unsigned int,unsigned long,enum _D3DFORMAT,enum _D3DPOOL,struct IDirect3DTexture9 * *,void * *) STUB
void CFakeDevice::GetGammaRamp(unsigned int,struct _D3DGAMMARAMP *) {BREAK;}
void CFakeDevice::SetGammaRamp(unsigned int,unsigned long,struct _D3DGAMMARAMP const *) {BREAK;}
HRESULT CFakeDevice::SetDialogBoxMode(int) STUB
HRESULT CFakeDevice::GetRasterStatus(unsigned int,struct _D3DRASTER_STATUS *) STUB
HRESULT CFakeDevice::GetBackBuffer(unsigned int,unsigned int,enum _D3DBACKBUFFER_TYPE,struct IDirect3DSurface9 * *) STUB
HRESULT CFakeDevice::Present(struct tagRECT const *,struct tagRECT const *,struct HWND__ *,struct _RGNDATA const *) STUB
HRESULT CFakeDevice::Reset(struct _D3DPRESENT_PARAMETERS_ *) STUB
unsigned int CFakeDevice::GetNumberOfSwapChains(void) {BREAK; return 0;}
HRESULT CFakeDevice::GetSwapChain(unsigned int,struct IDirect3DSwapChain9 * *) STUB
HRESULT CFakeDevice::CreateAdditionalSwapChain(struct _D3DPRESENT_PARAMETERS_ *,struct IDirect3DSwapChain9 * *) STUB
int CFakeDevice::ShowCursor(int) {BREAK; return 0;}
void CFakeDevice::SetCursorPosition(int,int,unsigned long) {BREAK;}
HRESULT CFakeDevice::SetCursorProperties(unsigned int,unsigned int,struct IDirect3DSurface9 *) STUB
HRESULT CFakeDevice::GetCreationParameters(struct _D3DDEVICE_CREATION_PARAMETERS *) STUB
HRESULT CFakeDevice::GetDisplayMode(unsigned int,struct _D3DDISPLAYMODE *) STUB
HRESULT CFakeDevice::GetDeviceCaps(struct _D3DCAPS9 *) STUB
HRESULT CFakeDevice::GetDirect3D(struct IDirect3D9 * *) STUB
HRESULT CFakeDevice::EvictManagedResources(void) STUB
unsigned int CFakeDevice::GetAvailableTextureMem(void) {BREAK; return 0;}
HRESULT CFakeDevice::TestCooperativeLevel(void) STUB

// LH only methods
HRESULT CFakeDevice::PresentEx(const RECT *,const RECT *,HWND,const RGNDATA *,DWORD) STUB
HRESULT CFakeDevice::GetGPUThreadPriority(INT *) STUB
HRESULT CFakeDevice::SetGPUThreadPriority(INT) STUB
HRESULT CFakeDevice::CheckDeviceState(HWND) STUB
HRESULT CFakeDevice::CreateRenderTargetEx(UINT, UINT, D3DFORMAT, D3DMULTISAMPLE_TYPE, DWORD, BOOL, IDirect3DSurface9 **, HANDLE *, DWORD) STUB
HRESULT CFakeDevice::CreateOffscreenPlainSurfaceEx(UINT, UINT, D3DFORMAT, D3DPOOL, IDirect3DSurface9 **, HANDLE *, DWORD) STUB
HRESULT CFakeDevice::CreateDepthStencilSurfaceEx(UINT,UINT,D3DFORMAT,D3DMULTISAMPLE_TYPE,DWORD,BOOL,IDirect3DSurface9 **,HANDLE *,DWORD) STUB


