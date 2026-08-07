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

MtDefine(PointLightResource, MILRender, "PointLight Resource");

MtDefine(CMilPointLightDuce, PointLightResource, "CMilPointLightDuce");

CMilPointLightDuce::~CMilPointLightDuce()
{
    UnRegisterNotifiers();
}

__out_ecount_opt(1) CMilTransform3DDuce *CMilPointLightDuce::GetTransform()
{
    return m_data.m_pTransform;
}

HRESULT CMilPointLightDuce::PreRender(
    __in_ecount(1) CPrerenderWalker *pPrerenderer,
    __in_ecount(1) CMILMatrix *pTransform
    )
{
    HRESULT hr = S_OK;

    CMILLightPoint *pPointLightNoRef = NULL;

    // Get the light data
    IFC(GetRealization(&pPointLightNoRef));

    Assert(pPointLightNoRef);

    float flScale =
        static_cast<float>(pow(fabs(pTransform->GetUpper3x3Determinant3D()), 1.0 / 3.0));
    
    if (flScale != 0.0)
    {
        pPointLightNoRef->Transform(CMILLight::TransformType_ViewSpace, pTransform, flScale);
        pPrerenderer->AddLight(pPointLightNoRef);
    }

Cleanup:
    RRETURN(hr);
}

void CMilPointLightDuce::ClearRealization()
{
}

HRESULT CMilPointLightDuce::GetRealization(
    __deref_out_ecount_opt(1) CMILLightPoint **ppRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    *ppRealizationNoRef = NULL;

    IFC(SynchronizeAnimatedFields());
    {
        auto position = vector3::create(m_data.m_position);

        m_pointLightRealization.Set(
            reinterpret_cast<MilColorF *>(&m_data.m_color),
            &position,
            static_cast<float>(m_data.m_range),
            static_cast<float>(m_data.m_constantAttenuation),
            static_cast<float>(m_data.m_linearAttenuation),
            static_cast<float>(m_data.m_quadraticAttenuation)
        );

        *ppRealizationNoRef = &m_pointLightRealization;
    }

Cleanup:
    RRETURN(hr);
}


