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

MtDefine(EmissiveMaterialResource, MILRender, "EmissiveMaterial Resource");

MtDefine(CMilEmissiveMaterialDuce, EmissiveMaterialResource, "CMilEmissiveMaterialDuce");

CMilEmissiveMaterialDuce::~CMilEmissiveMaterialDuce()
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
override bool CMilEmissiveMaterialDuce::ShouldRender()
{
    return m_data.m_pBrush != NULL;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      Realize
//
//  Synopsis:
//      Creates a EmissiveShader for rendering with the material's brush.
//
//      NOTE: It is legal for this method to return NULL if there if
//            the brush is empty.
//
//------------------------------------------------------------------------------

HRESULT CMilEmissiveMaterialDuce::Realize(
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __in_ecount(1) CDrawingContext *pDrawingContext,
    __in_ecount(1) CContextState *pContextState,
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_out_ecount_opt(1) CMILShader **ppShader
    )
{
    HRESULT hr = S_OK;
    CBrushRealizer *pBrush = NULL;
    CMILShaderEmissive *pShaderEmissive = NULL;

    //
    // We should be executing this code only if we will be rendered.
    //
    Assert(ShouldRender());

    *ppShader = NULL;

    pContextState->LightData.SetMaterialEmissiveColor(*(reinterpret_cast<MilColorF *>(&m_data.m_color)));
    
    IFC(pDrawingContext->GetBrushRealizer(
        m_data.m_pBrush,
        pBrushContext,
        &pBrush
        ));

    IFC(CMILShaderEmissive::Create(
        NULL,
        pBrush,
        &pShaderEmissive
        ));

    // NOTE: We steal the pShaderEmissive's ref and set pShaderEmissive
    //       to null so it only get's released on error.
    (*ppShader) = pShaderEmissive;
    pShaderEmissive = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pShaderEmissive);
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

HRESULT CMilEmissiveMaterialDuce::Flatten(
    __inout_ecount(1) DynArray<CMilMaterialDuce *> *pMaterialList,
    __inout_ecount(1) bool *pfDiffuseMaterialFound,
    __inout_ecount(1) bool *pfSpecularMaterialFound,
    __out_ecount(1) float *pflFirstSpecularPower,
    __out_ecount(1) MilColorF *pFirstAmbientColor,
    __out_ecount(1) MilColorF *pFirstDiffuseColor,
    __out_ecount(1) MilColorF *pFirstSpecularColor
    )
{
    Assert(ShouldRender());

    RRETURN(pMaterialList->Add(this));
}


