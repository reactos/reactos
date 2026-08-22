// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_lighting
//      $Keywords:
//
//  $Description:
//      Contains definitions for all fixed function shaders
//          - CHwFFShader (base class)
//          - CHwDiffuseShader
//          - CHwSpecularShader
//          - CHwEmissiveShader
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwFFShader,       MILRender, "CHwFFShader");
MtDefine(CHwDiffuseShader,  MILRender, "CHwDiffuseShader");
MtDefine(CHwSpecularShader, MILRender, "CHwSpecularShader");
MtDefine(CHwEmissiveShader, MILRender, "CHwEmissiveShader");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFShader::ctor
//
//  Synopsis:
//      NULL out the color source and set the Number of passes to 1
//
//------------------------------------------------------------------------------
CHwFFShader::CHwFFShader(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    )
    : CHwShader(pDevice)
{
    m_oShaderData.iNumPasses = 1;
    m_pSurfaceBrush = NULL;
    m_pEffectList = NULL;
    m_pEffectContextNoRef = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFShader::dtor
//
//  Synopsis:
//      Release the Color Source
//
//------------------------------------------------------------------------------
CHwFFShader::~CHwFFShader()
{
    ReleaseInterfaceNoNULL(m_pSurfaceBrush);
    ReleaseInterfaceNoNULL(m_pEffectList);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFShader::Init
//
//  Synopsis:
//      Add the pass data and set the color source
//
//------------------------------------------------------------------------------
HRESULT
CHwFFShader::Init(
    __in_ecount(1) CHwBrush *pHwBrush,
    __inout_ecount_opt(1) IMILEffectList *pEffectList,
    __in_ecount(1) CHwBrushContext const &oEffectContext
    )
{
    HRESULT hr = S_OK;
    MILShaderPassData singlePassData = {TRUE, TRUE};


    Assert(m_pSurfaceBrush == NULL);


    // Add Data to the base class shader for it's 1 pass

    IFC(m_rgPassData.Add(singlePassData));


    //
    // Set the Color Source
    //
    // We do not addref hw brushes, we only release them.
    //
    m_pSurfaceBrush = pHwBrush;

    SetInterface(m_pEffectList, pEffectList);

    // Retain a pointer to brush context for effects, but don't hold a reference.
    // Object is expected to survive until shader is no longer needed.
    m_pEffectContextNoRef = &oEffectContext;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFShader::CreateCompatibleVertexBufferBuilder
//
//  Synopsis:
//      Create a CHwVertexBuffer::Builder that can be used with this shader.
//
//------------------------------------------------------------------------------
HRESULT
CHwFFShader::CreateCompatibleVertexBufferBuilder(
    MilVertexFormat mvfGeometryOutput,
    MilVertexFormatAttribute mvfGeometryAALocation,
    __inout_ecount(1) CBufferDispenser *pBufferDispenser,
    __deref_out_ecount(1) CHwVertexBuffer::Builder **ppBufferBuilder
    )
{
    UNREFERENCED_PARAMETER(mvfGeometryOutput);
    UNREFERENCED_PARAMETER(mvfGeometryAALocation);
    UNREFERENCED_PARAMETER(pBufferDispenser);
    UNREFERENCED_PARAMETER(ppBufferBuilder);

    RIP("CHwFFShader is currently not compatible with CHwVertexBuffer's");
    RRETURN(THR(E_FAIL));
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwFFShader::Finish
//
//  Synopsis:
//      Called by base class - does nothing
//
//------------------------------------------------------------------------------
HRESULT
CHwFFShader::Finish()
{
    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDiffuseShader::Create
//
//  Synopsis:
//      Creates a new Diffuse Shader and initializes it.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CHwDiffuseShader::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CHwBrush         *pHwBrush,
    __inout_ecount_opt(1) IMILEffectList    *pEffectList,
    __in_ecount(1) CHwBrushContext const    &oEffectContext,
    __deref_out_ecount(1) CHwDiffuseShader **ppDiffuseShader
    )
{
    HRESULT hr = S_OK;
    CHwDiffuseShader *pNewShader = NULL;

    *ppDiffuseShader = NULL;

    pNewShader = new CHwDiffuseShader(pDevice);

    IFCOOM(pNewShader);

    pNewShader->AddRef();

    // Initialize with our Surface Color Source

    IFC(pNewShader->Init(
        pHwBrush,
        pEffectList,
        oEffectContext
        ));

    *ppDiffuseShader = pNewShader;

    pNewShader = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewShader);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDiffuseShader::SetupPassVirtual
//
//  Synopsis:
//      Called by base class - sets up state and textures
//
//------------------------------------------------------------------------------
HRESULT
CHwDiffuseShader::SetupPassVirtual(
    __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
    __inout_ecount(1) CHwPipeline *pHwPipeline,
    __range(0,INT_MAX) UINT uPassNum
    )
{
    return pHwPipeline->InitializeForRendering(
        MilCompositingMode::SourceOver,
        pGeometryGenerator,
        m_pSurfaceBrush,
        m_pEffectList,
        m_pEffectContextNoRef
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDiffuseShader::Begin
//
//  Synopsis:
//      Enable z write
//
//------------------------------------------------------------------------------
HRESULT
CHwDiffuseShader::Begin(
    __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
    __in_ecount(1) const CMilRectL &rcRenderingBounds,
    bool fZBufferEnabled
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pHwTargetSurface);
    UNREFERENCED_PARAMETER(rcRenderingBounds);

    if (fZBufferEnabled)
    {
        IFC(m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 1));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDiffuseShader::GetRequiredLightingValues
//
//  Synopsis:
//      Diffuse shader requires diffuse lighting
//
//------------------------------------------------------------------------------
CHwShader::LightingValues
CHwDiffuseShader::GetRequiredLightingValues() const
{
    return LV_Diffuse;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSpecularShader::Create
//
//  Synopsis:
//      Creates a new Specular Shader and initializes it.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CHwSpecularShader::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CHwBrush         *pHwBrush,
    __inout_ecount_opt(1) IMILEffectList    *pEffectList,
    __in_ecount(1) CHwBrushContext const    &oEffectContext,
    __deref_out_ecount(1) CHwSpecularShader **ppSpecularShader
    )
{
    HRESULT hr = S_OK;
    CHwSpecularShader *pNewShader = NULL;

    *ppSpecularShader = NULL;

    pNewShader = new CHwSpecularShader(pDevice);

    IFCOOM(pNewShader);

    pNewShader->AddRef();

    // Initialize with our Surface Color Source

    IFC(pNewShader->Init(
        pHwBrush,
        pEffectList,
        oEffectContext
        ));

    *ppSpecularShader = pNewShader;

    pNewShader = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewShader);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSpecularShader::SetupPassVirtual
//
//  Synopsis:
//      Called by base class - sets up state and textures
//
//------------------------------------------------------------------------------
HRESULT
CHwSpecularShader::SetupPassVirtual(
    __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
    __inout_ecount(1) CHwPipeline *pHwPipeline,
    __range(0,INT_MAX) UINT uPassNum
    )
{
    // Specular broken in SW
    //
    // Specular doesn't work in SW because RGBRast only interpolates the specular
    // vertex color if SPECULARENABLE is on. We use the diffuse vertex color and
    // we swap in the specular values before we render the specular material.
    // This way the hwpipeline will modulate the color against the diffuse color
    // in the vertex and it will work on rgbrast.

    return pHwPipeline->InitializeForRendering(
        MilCompositingMode::SourceAdd,
        pGeometryGenerator,
        m_pSurfaceBrush,
        m_pEffectList,
        m_pEffectContextNoRef
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSpecularShader::Begin
//
//  Synopsis:
//      Disable z write
//
//------------------------------------------------------------------------------
HRESULT
CHwSpecularShader::Begin(
    __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
    __in_ecount(1) const CMilRectL &rcRenderingBounds,
    bool fZBufferEnabled
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pHwTargetSurface);
    UNREFERENCED_PARAMETER(rcRenderingBounds);

    if (fZBufferEnabled)
    {
        IFC(m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSpecularShader::GetRequiredLightingValues
//
//  Synopsis:
//      Specular shader requires specular lighting values
//
//------------------------------------------------------------------------------
CHwShader::LightingValues
CHwSpecularShader::GetRequiredLightingValues() const
{
    return LV_Specular;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwEmissiveShader::Create
//
//  Synopsis:
//      Creates a new Emissive Shader and initializes it.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CHwEmissiveShader::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CHwBrush         *pHwBrush,
    __inout_ecount_opt(1) IMILEffectList    *pEffectList,
    __in_ecount(1) CHwBrushContext const    &oEffectContext,
    __deref_out_ecount(1) CHwEmissiveShader **ppEmissiveShader
    )
{
    HRESULT hr = S_OK;
    CHwEmissiveShader *pNewShader = NULL;

    *ppEmissiveShader = NULL;

    pNewShader = new CHwEmissiveShader(pDevice);

    IFCOOM(pNewShader);

    pNewShader->AddRef();

    // Initialize with our Surface Color Source

    IFC(pNewShader->Init(
        pHwBrush,
        pEffectList,
        oEffectContext
        ));

    *ppEmissiveShader = pNewShader;

    pNewShader = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewShader);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwEmissiveShader::SetupPassVirtual
//
//  Synopsis:
//      Called by base class - sets up state and textures
//
//------------------------------------------------------------------------------
HRESULT
CHwEmissiveShader::SetupPassVirtual(
    __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
    __inout_ecount(1) CHwPipeline *pHwPipeline,
    __range(0,INT_MAX) UINT uPassNum
    )
{
    return pHwPipeline->InitializeForRendering(
        MilCompositingMode::SourceAdd,
        pGeometryGenerator,
        m_pSurfaceBrush,
        m_pEffectList,
        m_pEffectContextNoRef
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwEmissiveShader::Begin
//
//  Synopsis:
//      Disable z write
//
//------------------------------------------------------------------------------
HRESULT
CHwEmissiveShader::Begin(
    __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
    __in_ecount(1) const CMilRectL &rcRenderingBounds,
    bool fZBufferEnabled
    )
{
    HRESULT hr = S_OK;

    UNREFERENCED_PARAMETER(pHwTargetSurface);
    UNREFERENCED_PARAMETER(rcRenderingBounds);

    if (fZBufferEnabled)
    {
        IFC(m_pDevice->SetRenderState(D3DRS_ZWRITEENABLE, 0));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwEmissiveShader::GetRequiredLightingValues
//
//  Synopsis:
//      Emissve shader currently requires white lighting
//
//------------------------------------------------------------------------------
CHwShader::LightingValues
CHwEmissiveShader::GetRequiredLightingValues() const
{
    return LV_Emissive;
}



