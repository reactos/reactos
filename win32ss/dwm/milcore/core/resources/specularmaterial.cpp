// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_material
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(SpecularMaterialResource, MILRender, "SpecularMaterial Resource");

MtDefine(CMilSpecularMaterialDuce, SpecularMaterialResource, "CMilSpecularMaterialDuce");

CMilSpecularMaterialDuce::~CMilSpecularMaterialDuce()
{
    UnRegisterNotifiers();
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ShouldRender
//
//  Synopsis:
//      Returns true if the Material is valid for rendering, false otherwise.
//
//------------------------------------------------------------------------------
override bool CMilSpecularMaterialDuce::ShouldRender()
{
    return m_data.m_pBrush != NULL;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      Realize
//
//  Synopsis:
//      Creates a SpecularShader for rendering with the material's brush.
//
//      NOTE: It is legal for this method to return NULL if there if
//            the brush is empty.
//
//------------------------------------------------------------------------------

override HRESULT CMilSpecularMaterialDuce::Realize(
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __in_ecount(1) CDrawingContext *pDrawingContext,
    __in_ecount(1) CContextState *pContextState,
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_out_ecount_opt(1) CMILShader **ppShader
    )
{
    HRESULT hr = S_OK;
    CBrushRealizer *pBrush = NULL;
    CMILShaderSpecular *pShaderSpecular = NULL;

    Assert(pContextState->CullMode3D == D3DCULL_CW || pContextState->CullMode3D == D3DCULL_CCW);

    //
    // We should be executing this code only if we will be rendered.
    //
    Assert(ShouldRender());

    *ppShader = NULL;

    CMILLightData &lightData = pContextState->LightData;

    bool lightingValid = lightData.SetMaterialSpecularColor(*(reinterpret_cast<MilColorF *>(&m_data.m_color)));
    lightingValid &= lightData.SetMaterialSpecularPower(static_cast<float>(m_data.m_specularPower));

    // If this specular power or color is different from the values that we
    // precomputed for, then we have to completely recompute specular (ouch)
    if (!lightingValid)
    {
        // The light data has already been set up correctly by the walker
        Assert(lightData.IsSpecularEnabled());
        
        // 
        //
        // Ideally, we'd disable diffuse calculation here to only recompute
        // specular. However, doing that overwrites the (possibly) existing 
        // diffuse values with black so we have to redo diffuse.
        //
        // A MaterialGroup with multiple SpecularMaterials with different
        // powers is an extremely rare scenario and the cost of calculating
        // specular is much worse than the cost of calculating diffuse so
        // fixing this isn't really worth it. However, it's bothersome.
       
        // Later on in the shader, if software lighting is needed, this 
        // invalidation will force a recomputation of the lighting
        pMesh3D->InvalidateColorCache();
    }

    IFC(pDrawingContext->GetBrushRealizer(
        m_data.m_pBrush,
        pBrushContext,
        &pBrush
        ));

    IFC(CMILShaderSpecular::Create(
        NULL,
        pBrush,
        m_data.m_specularPower,
        &pShaderSpecular
        ));

    // NOTE: We steal the pShaderSpecular's ref and set pShaderSpecular
    //       to null so it only get's released on error.
    (*ppShader) = pShaderSpecular;
    pShaderSpecular = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pShaderSpecular);
    ReleaseInterfaceNoNULL(pBrush);
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      Flatten
//
//  Synopsis:
//      Flattens children, if any, and identifies material properties.
//
//------------------------------------------------------------------------------

override HRESULT CMilSpecularMaterialDuce::Flatten(
    __inout_ecount(1) DynArray<CMilMaterialDuce *> *pMaterialList,
    __inout_ecount(1) bool *pfDiffuseMaterialFound,
    __inout_ecount(1) bool *pfSpecularMaterialFound,
    __out_ecount(1) float *pflFirstSpecularPower,
    __out_ecount(1) MilColorF *pFirstAmbientColor,
    __out_ecount(1) MilColorF *pFirstDiffuseColor,
    __out_ecount(1) MilColorF *pFirstSpecularColor
    )
{
    HRESULT hr = S_OK;

    Assert(ShouldRender());

    IFC(pMaterialList->Add(this));

    if (!(*pfSpecularMaterialFound))
    {
        *pflFirstSpecularPower = static_cast<float>(m_data.m_specularPower);
        *pFirstSpecularColor = *(reinterpret_cast<MilColorF*>(&m_data.m_color));
        *pfSpecularMaterialFound = true;
    }

Cleanup:
    RRETURN(hr);
}


