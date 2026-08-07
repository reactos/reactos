// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_transform
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(Transform3DGroupResource, MILRender, "Transform3DGroup Resource");
MtDefine(CMilTransform3DGroupDuce, Transform3DGroupResource, "CMilTransform3DGroupDuce");

CMilTransform3DGroupDuce::~CMilTransform3DGroupDuce()
{
    UnRegisterNotifiers();
}

void CMilTransform3DGroupDuce::ClearRealization()
{
}

HRESULT CMilTransform3DGroupDuce::GetRealization(
    __out_ecount(1) CMILMatrix *pRealization
    )
{
    HRESULT hr = S_OK;

    *pRealization = matrix::get_identity();
    IFC(Append(pRealization));

Cleanup:
    RRETURN(hr);
}

HRESULT CMilTransform3DGroupDuce::Append(
    __inout_ecount(1) CMILMatrix *pMat
    )
{
    HRESULT hr = S_OK;
    
    if (!(EnterResource()))
    {
        // In case of a loop, we pretend we're the identity
        // which means no multiplication is necessary.
        goto Cleanup;
    }

    for (UINT i = 0; i < m_data.m_cChildren; i++)
    {
        IFC(m_data.m_rgpChildren[i]->Append(pMat));
    }

Cleanup:
    LeaveResource();

    RRETURN(hr);
}


