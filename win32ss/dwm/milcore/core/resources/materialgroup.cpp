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

MtDefine(MaterialGroupResource, MILRender, "MaterialGroup Resource");

MtDefine(CMilMaterialGroupDuce, MaterialGroupResource, "CMilMaterialGroupDuce");

CMilMaterialGroupDuce::~CMilMaterialGroupDuce()
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
override bool CMilMaterialGroupDuce::ShouldRender()
{
    bool toReturn = false;
    
    if (!(EnterResource()))
    {
        // In case of a loop.
        goto Cleanup;
    }

    for (UINT32 i = 0; i < m_data.m_cChildren; i++)
    {
        if (m_data.m_rgpChildren[i]->ShouldRender())
        {
            toReturn = true;
            break;
        }
    }

Cleanup:
    LeaveResource();
    return toReturn;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      Realize
//
//  Synopsis:
//      Groups cannot be realized, but since materials are flattened before
//      being rendered we never expect this implementation to be invoked.
//
//------------------------------------------------------------------------------

override HRESULT CMilMaterialGroupDuce::Realize(
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __in_ecount(1) CDrawingContext *pDrawingContext,
    __in_ecount(1) CContextState *pContextState,
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_out_ecount_opt(1) CMILShader **ppShader
    )
{
    //
    // We expect to never run this implementation.
    //

    Assert(false);
    RRETURN(E_UNEXPECTED);
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

override HRESULT CMilMaterialGroupDuce::Flatten(
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

    //
    // We should always be able to enter because if there were a loop
    // then ShouldRender() would return false and we won't call Flatten()
    //
    Assert(CanEnterResource());

    EnterResource();

#if DBG
    //
    // If we reach this point, we should only be proceeding if there is a child
    // that should be rendered.
    //
    
    bool fDbg_RenderableChildFound = false;
#endif

    for (UINT32 i = 0; i < m_data.m_cChildren; i++)
    {
        if (m_data.m_rgpChildren[i]->ShouldRender())
        {
#if DBG
            fDbg_RenderableChildFound = true;
#endif
            
            IFC(m_data.m_rgpChildren[i]->Flatten(
                pMaterialList, 
                pfDiffuseMaterialFound, 
                pfSpecularMaterialFound,
                pflFirstSpecularPower,
                pFirstAmbientColor,
                pFirstDiffuseColor,
                pFirstSpecularColor
                ));
        }
    }

#if DBG
    Assert(fDbg_RenderableChildFound);
#endif

Cleanup:
    LeaveResource();
    RRETURN(hr);
}


