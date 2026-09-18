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
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(DirectionalLightResource, MILRender, "DirectionalLight Resource");

MtDefine(CMilDirectionalLightDuce, DirectionalLightResource, "CMilDirectionalLightDuce");

CMilDirectionalLightDuce::~CMilDirectionalLightDuce()
{
    UnRegisterNotifiers();
}

__out_ecount_opt(1) CMilTransform3DDuce *CMilDirectionalLightDuce::GetTransform()
{
    return m_data.m_pTransform;
}

HRESULT CMilDirectionalLightDuce::PreRender(
    __in_ecount(1) CPrerenderWalker *pPrerenderer,
    __in_ecount(1) CMILMatrix *pTransform
    )
{
    HRESULT hr = S_OK;

    CMILLightDirectional *pDirectionalLightNoRef = NULL;

    // Get the light data
    IFC(GetRealization(&pDirectionalLightNoRef));

    Assert(pDirectionalLightNoRef);

    pDirectionalLightNoRef->Transform(CMILLight::TransformType_ViewSpace, pTransform, 1.0);
    
    pPrerenderer->AddLight(pDirectionalLightNoRef);

Cleanup:
    RRETURN(hr);
}

void CMilDirectionalLightDuce::ClearRealization()
{
}

HRESULT CMilDirectionalLightDuce::GetRealization(
    __deref_out_ecount_opt(1) CMILLightDirectional **ppRealizationNoRef
    )
{
    HRESULT hr = S_OK;
    auto direction = vector3::create(m_data.m_direction);

    *ppRealizationNoRef = NULL;

    IFC(SynchronizeAnimatedFields());

    

    m_directionalLightRealization.Set(
        reinterpret_cast<MilColorF *>(&m_data.m_color),
        &direction
        );

    *ppRealizationNoRef = &m_directionalLightRealization;

    // NOTE: Direction will be transformed by PreRender.

Cleanup:
    RRETURN(hr);
}


