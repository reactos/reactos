// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Contains the HwShader implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"
using namespace dxlayer;

MtDefine(HwShaderData, MILRender, "HwShaderData");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShader::CHwShader
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwShader::CHwShader(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    m_pDevice = pDevice;

    m_oShaderData.iNumPasses = 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShader::DrawMesh3D
//
//  Synopsis:
//      Takes a device, a mesh, shader, & context.  Sets up the shader, renders
//      every pass as many times as necessary, and then finally finishes the
//      shader.
//
//------------------------------------------------------------------------------
HRESULT
CHwShader::DrawMesh3D(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __in_ecount(1) const CMilRectL &rcRenderBoundsDeviceSpace,
    __inout_ecount(1) CContextState *pContextState,
    bool fZBufferEnabled
    )
{
    HRESULT hr = S_OK;

    bool fInShaderBegin = false;

    //
    // We need to initialize our state
    //
    IFC(Begin(
        pHwTargetSurface,
        rcRenderBoundsDeviceSpace,
        fZBufferEnabled
        ));
    
    fInShaderBegin = true;

    // Future Consideration: 
    // It would be nice if we could do this check at the meta rt level for
    // multi-mon scenarios, i.e. one card supports shader path and one 
    // doesn't. However, the meta rt doesn't know which rts will be 
    // rendered to.

    if (CHwShaderPipeline::CanRunWithDevice(pD3DDevice))
    {
        hr = ShaderDrawMesh3D(pD3DDevice, pMesh3D, pContextState);

        if (FAILED(hr))
        {
            IFC(FixedFunctionDrawMesh3D(pD3DDevice, pMesh3D, pContextState));
        }
    }
    else
    {
        IFC(FixedFunctionDrawMesh3D(pD3DDevice, pMesh3D, pContextState));
    }

Cleanup:

    if (fInShaderBegin)
    {
        HRESULT hrFinish = THR(Finish());

        if (SUCCEEDED(hr))
        {
            hr = hrFinish;
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShader::ShaderDrawMesh3D
//
//  Synopsis:
//      Draws the mesh using the shader pipeline
//
//------------------------------------------------------------------------------
HRESULT
CHwShader::ShaderDrawMesh3D(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __in_ecount(1) const CMILMesh3D *pMesh3D,
    __inout_ecount(1) CContextState *pContextState
    )
{
    HRESULT hr = S_OK;
    CHwShaderPipeline Pipeline(/* f2D = */ false, pD3DDevice);

    //
    // Our shaders can support multiple passes
    //
    UINT uNumPasses = GetNumPasses();

    const vector3 *pvec3Normals = NULL;
    size_t cbNormals = 0;
    // this does not perform a copy
    pMesh3D->GetNormals(pvec3Normals, cbNormals);

    pContextState->LightData.SetLightingPass(GetRequiredLightingValues());

    for (UINT i = 0; i < uNumPasses; i++)
    {
        // Future Consideration:   mesh perf 
        //
        // This means we copy the mesh once per Material. We should be 
        // able to avoid it except for a MaterialGroup with more than one 
        // unique specular power. MaterialGroups may be more common now that
        // we don't automatically write to Z with Emissive and Spec
        //
        // I'll be able to address this a little in the video mem mesh task
        // (bug #1412607)
        CHw3DGeometryRenderer<vector3> renderer(&pContextState->LightData, pD3DDevice);

        IFC(SetupPassVirtual(
            &renderer,
            &Pipeline,
            i
            ));

        //   This Execute call actually just sets up.
        // The CHw3DGeometryRenderer implementation of IGeometryGenerator produces no geometry but
        // does set the per vertex type.  Because it doesn't return WGXHR_EMPTYFILL this Execute
        // call realizes the color sources and sends all states w/o sending any geometry.
        IFC(Pipeline.Execute());

        IFC(renderer.Render(
            pMesh3D,
            pvec3Normals,
            cbNormals,
            /* defaultNormal = */ vector3(1.0f, 0.0f, 0.0f),  // Unspecified normals are generated -- This value should have no effect.
            pD3DDevice
            ));

        Pipeline.ReleaseExpensiveResources();    
    }

Cleanup:
    Pipeline.ReleaseExpensiveResources(); 
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShader::FixedFunctionDrawMesh3D
//
//  Synopsis:
//      Draws the mesh using the fixed function pipeline
//
//------------------------------------------------------------------------------

HRESULT
CHwShader::FixedFunctionDrawMesh3D(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount(1) CContextState *pContextState
    )
{    
    HRESULT hr = S_OK;
    CHwFFPipeline Pipeline(pD3DDevice);

    //
    // Our shaders can support multiple passes
    //
    UINT uNumPasses = GetNumPasses();

    //
    // This will actually calculate the lighting only if the mesh's
    // lighting cache is invalid. The LightData was properly initialized
    // above at the render walker level.
    //
    IFC(pMesh3D->PrecomputeLighting(
        &pContextState->WorldTransform3D,
        &pContextState->ViewTransform3D, 
        &pContextState->LightData
        ));

    //
    // Get a reference to the color stream required by the shader.
    // Note: no copy is done here (pdwColors is const)
    //
    const DWORD *pdwColors;
    size_t cbColors = 0;

    // For Diffuse/Specular vertex colors should be precomputed by lighting.
    // For Emissive we initialize this to the Emissive material color.
    // This default value should never affect rendering.
    DWORD dwDefaultColor = 0xFFFFFFFF;
    
    switch(GetRequiredLightingValues())
    {
        case LV_Specular:
            pMesh3D->GetSpecularColors(pdwColors, cbColors);
            break;

        case LV_Diffuse:
            pMesh3D->GetDiffuseColors(pdwColors, cbColors);
            break;

        case LV_Emissive:
        {
            MilColorF emissiveColor = pContextState->LightData.GetMaterialEmissiveColor();
            dwDefaultColor = Convert_MilColorF_sRGB_To_D3DCOLOR_ZeroAlpha(&emissiveColor);
            
            __fallthrough; // continue; -- We intentionally fall through to the default case.
        }
        
        default:
            pdwColors = NULL;
            cbColors = 0;
            break;
    }

    for (UINT i = 0; i < uNumPasses; i++)
    {
        // Future Consideration:   mesh -> card perf
        // See comment on ShaderDrawMesh3D
        CHw3DGeometryRenderer<DWORD> renderer(&pContextState->LightData);

        IFC(SetupPassVirtual(
            &renderer,
            &Pipeline,
            i
            ));

        // 
        // See comment on ShaderDrawMesh3D
        IFC(Pipeline.Execute());

        IFC(renderer.Render(
            pMesh3D,
            pdwColors,
            cbColors,
            dwDefaultColor,
            pD3DDevice
            ));

        Pipeline.ReleaseExpensiveResources();    
    }

Cleanup:
    Pipeline.ReleaseExpensiveResources(); 
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShader::DrawHwVertexBuffer
//
//  Synopsis:
//      Takes a device, a vertex buffer, shader, & context.  Sets up the shader,
//      renders every pass as many times as necessary, and then finally finishes
//      the shader.
//
//------------------------------------------------------------------------------
HRESULT
CHwShader::DrawHwVertexBuffer(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
    __inout_ecount(1) IGeometryGenerator *pIGeometryGenerator,
    __inout_ecount(1) CBufferDispenser *pBufferDispenser,
    __in_ecount(1) const CMilRectL &rcRenderBoundsDeviceSpace,
    BOOL fUse3DTransforms,
    bool fZBufferEnabled
    )
{
    CHwVertexBuffer::Builder *pShaderShapeBuilder = NULL;
    CHwVertexBuffer *pVertexBuffer = NULL;
    HRESULT hr = S_OK;
    BOOL fInShaderBegin = FALSE;

    if (!fUse3DTransforms)
    {
        IFC(pD3DDevice->Set2DTransformForFixedFunction());
    }

    UINT uNumPasses;

    //
    // We need to initialize our state
    //
    IFC(Begin(
        pHwTargetSurface,
        rcRenderBoundsDeviceSpace,
        fZBufferEnabled
        ));
    
    fInShaderBegin = TRUE;

    {
        MilVertexFormat mvfGeometryOutput;
        
        pIGeometryGenerator->GetPerVertexDataType(
            mvfGeometryOutput
            );

        IFC(CreateCompatibleVertexBufferBuilder(
            mvfGeometryOutput,
            HWPIPELINE_ANTIALIAS_LOCATION,
            pBufferDispenser,
            &pShaderShapeBuilder
            ));
    }

    // Let vertex builder know that is the end of the vertex mappings
    IFC(pShaderShapeBuilder->FinalizeMappings());

    IFC(pShaderShapeBuilder->BeginBuilding());

    IFC(pIGeometryGenerator->SendGeometry(pShaderShapeBuilder));
    if (hr == WGXHR_EMPTYFILL)
    {
        // Note that WGXHR_EMPTYFILL is a success code, so it will survive
        // the IFC above.
        goto Cleanup;
    }

    // Mark end of geometry data and retrieve realized vertex buffer
    IFC(pShaderShapeBuilder->EndBuilding(&pVertexBuffer));

    IFC(pVertexBuffer->SendVertexFormat(pD3DDevice));

    //
    // Our shaders can support multiple passes
    //
    uNumPasses = GetNumPasses();

    for (UINT i = 0; i < uNumPasses; i++)
    {
        IFC(SetupPassVirtual(
            NULL,
            NULL,
            i
            ));

        IFC(pVertexBuffer->DrawPrimitive(m_pDevice));
    }

Cleanup:
    if (fInShaderBegin)
    {
        HRESULT hrFinish = THR(Finish());

        if (SUCCEEDED(hr))
        {
            hr = hrFinish;
        }
    }

    delete pShaderShapeBuilder;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwShader::GetNumPasses
//
//  Synopsis:
//      Returns the number of passes in the shader
//
//------------------------------------------------------------------------------
__range(1, INT_MAX) UINT
CHwShader::GetNumPasses() const
{
    return static_cast<UINT>(m_oShaderData.iNumPasses);
}


