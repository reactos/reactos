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

MtDefine(AmbientLightResource, MILRender, "AmbientLight Resource");

MtDefine(CMilAmbientLightDuce, AmbientLightResource, "CMilAmbientLightDuce");

CMilAmbientLightDuce::~CMilAmbientLightDuce()
{
    UnRegisterNotifiers();
}

__out_ecount_opt(1) CMilTransform3DDuce *CMilAmbientLightDuce::GetTransform()
{
    return m_data.m_pTransform;
}

HRESULT CMilAmbientLightDuce::PreRender(
    __in_ecount(1) CPrerenderWalker *pPrerenderer,
    __in_ecount(1) CMILMatrix *pTransform
    )
{
    HRESULT hr = S_OK;

    CMILLightAmbient *pAmbientLightNoRef = NULL;

    IFC(GetRealization(&pAmbientLightNoRef));

    Assert(pAmbientLightNoRef);

    pPrerenderer->AddLight(pAmbientLightNoRef);

Cleanup:
    RRETURN(hr);
}

void CMilAmbientLightDuce::ClearRealization()
{

}

HRESULT CMilAmbientLightDuce::GetRealization(
    __deref_out_ecount_opt(1) CMILLightAmbient **ppRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    *ppRealizationNoRef = NULL;

    IFC(SynchronizeAnimatedFields());

    m_ambientLightRealization.Set(
        reinterpret_cast<MilColorF *>(&m_data.m_color)
        );

    *ppRealizationNoRef = &m_ambientLightRealization;
    
Cleanup:
    RRETURN(hr);
}


