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

MtDefine(DiffuseMaterialResource, MILRender, "DiffuseMaterial Resource");

MtDefine(CMilDiffuseMaterialDuce, DiffuseMaterialResource, "CMilDiffuseMaterialDuce");

CMilDiffuseMaterialDuce::~CMilDiffuseMaterialDuce()
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
override bool CMilDiffuseMaterialDuce::ShouldRender()
{
    return m_data.m_pBrush != NULL;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      Realize
//
//  Synopsis:
//      Creates a DiffuseShader for rendering with the material's brush.
//
//      NOTE: It is legal for this method to return NULL if there if
//            the brush is empty.
//
//------------------------------------------------------------------------------

override HRESULT CMilDiffuseMaterialDuce::Realize(
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __in_ecount(1) CDrawingContext *pDrawingContext,
    __in_ecount(1) CContextState *pContextState,
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_out_ecount_opt(1) CMILShader **ppShader
    )
{
    HRESULT hr = S_OK;
    CBrushRealizer *pBrush = NULL;
    CMILShaderDiffuse *pShaderDiffuse = NULL;

    //
    // We should be executing this code only if we will be rendered.
    //
    Assert(ShouldRender());

    *ppShader = NULL;

    CMILLightData &lightData = pContextState->LightData;

    bool lightingValid = lightData.SetMaterialDiffuseColor(*(reinterpret_cast<MilColorF *>(&m_data.m_color)));
    lightingValid &= lightData.SetMaterialAmbientColor(*(reinterpret_cast<MilColorF *>(&m_data.m_ambientColor)));

    // If this diffuse or ambient color is different from the values that we
    // precomputed for, then we have to completely recompute diffuse (ouch)
    if (!lightingValid)
    {
        // The light data has already been set up correctly by the walker
        Assert(lightData.IsDiffuseEnabled());
        
        // 
        //
        // Ideally, we'd disable specular calculation here to only recompute
        // diffuse, but recomputing lights would overwrite our specular colors
        // with black.

        pMesh3D->InvalidateColorCache();
    }
    
    IFC(pDrawingContext->GetBrushRealizer(
        m_data.m_pBrush,
        pBrushContext,
        &pBrush
        ));

    IFC(CMILShaderDiffuse::Create(
        NULL,
        pBrush,
        &pShaderDiffuse
        ));

    // NOTE: We steal the pShaderDiffuse's ref and set pShaderDiffuse
    //       to null so it only get's released on error.
    (*ppShader) = pShaderDiffuse;
    pShaderDiffuse = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pShaderDiffuse);
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

override HRESULT CMilDiffuseMaterialDuce::Flatten(
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
    
    if (!(*pfDiffuseMaterialFound))
    {
        *pFirstAmbientColor = *(reinterpret_cast<MilColorF*>(&m_data.m_ambientColor));
        *pFirstDiffuseColor = *(reinterpret_cast<MilColorF*>(&m_data.m_color));
        *pfDiffuseMaterialFound = true;
    }

Cleanup:
    RRETURN(hr);
}


