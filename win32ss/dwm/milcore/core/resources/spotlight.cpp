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

MtDefine(SpotLightResource, MILRender, "SpotLight Resource");
MtDefine(CMilSpotLightDuce, SpotLightResource, "CMilSpotLightDuce");

CMilSpotLightDuce::~CMilSpotLightDuce()
{
    UnRegisterNotifiers();
}

__out_ecount_opt(1) CMilTransform3DDuce *CMilSpotLightDuce::GetTransform()
{
    return m_data.m_pTransform;
}

HRESULT CMilSpotLightDuce::PreRender(
    __in_ecount(1) CPrerenderWalker *pPrerenderer,
    __in_ecount(1) CMILMatrix *pTransform
    )
{
    HRESULT hr = S_OK;

    CMILLightSpot *pSpotLightNoRef = NULL;

    // Get the light data
    IFC(GetRealization(&pSpotLightNoRef));

    Assert(pSpotLightNoRef);

    float flScale =
        static_cast<float>(pow(fabs(pTransform->GetUpper3x3Determinant3D()), 1.0 / 3.0));
    
    if (flScale != 0.0)
    {
        pSpotLightNoRef->Transform(CMILLight::TransformType_ViewSpace, pTransform, flScale);
        pPrerenderer->AddLight(pSpotLightNoRef);
    }

Cleanup:
    RRETURN(hr);
}

void CMilSpotLightDuce::ClearRealization()
{
}

HRESULT CMilSpotLightDuce::GetRealization(
    __deref_out_ecount_opt(1) CMILLightSpot **ppRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    *ppRealizationNoRef = NULL;

    IFC(SynchronizeAnimatedFields());
    {
        auto direction = vector3::create(m_data.m_direction);
        auto position = vector3::create(m_data.m_position);

        m_spotLightRealization.Set(
            reinterpret_cast<MilColorF *>(&m_data.m_color),
            &direction,
            &position,
            static_cast<float>(m_data.m_range),
            static_cast<float>(math_extensions::to_radian(static_cast<float>(m_data.m_innerConeAngle))),
            static_cast<float>(math_extensions::to_radian(static_cast<float>(m_data.m_outerConeAngle))),
            static_cast<float>(m_data.m_constantAttenuation),
            static_cast<float>(m_data.m_linearAttenuation),
            static_cast<float>(m_data.m_quadraticAttenuation)
        );

        *ppRealizationNoRef = &m_spotLightRealization;
    }
    // NOTE: Direction, Position and Range will be transformed by PreRender.

Cleanup:
    RRETURN(hr);
}


