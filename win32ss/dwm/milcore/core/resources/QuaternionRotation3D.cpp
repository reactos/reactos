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

MtDefine(QuaternionRotation3DResource, MILRender, "QuaternionRotation3D Resource");
MtDefine(CMilQuaternionRotation3DDuce, QuaternionRotation3DResource, "CMilQuaternionRotation3DDuce");

/* override */ CMilQuaternionRotation3DDuce::~CMilQuaternionRotation3DDuce()
{
    UnRegisterNotifiers();
}

/* override */ HRESULT CMilQuaternionRotation3DDuce::GetRealization(
    __out_ecount(1) CMILMatrix *pRealization
)
{
    HRESULT hr = S_OK;
    quaternion q;

    IFC(SynchronizeAnimatedFields());

    q = 
    { 
        m_data.m_quaternion.X,
        m_data.m_quaternion.Y,
        m_data.m_quaternion.Z,
        m_data.m_quaternion.W
    };

    *pRealization = matrix::make_rotation(q);

Cleanup:
    RRETURN(hr);
}

