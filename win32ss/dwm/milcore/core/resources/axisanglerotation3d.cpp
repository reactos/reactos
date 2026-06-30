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

MtDefine(AxisAngleRotation3DResource, MILRender, "AxisAngleRotation3D Resource");
MtDefine(CMilAxisAngleRotation3DDuce, AxisAngleRotation3DResource, "CMilAxisAngleRotation3DDuce");

CMilAxisAngleRotation3DDuce::~CMilAxisAngleRotation3DDuce()
{
    UnRegisterNotifiers();
}

/* override */ HRESULT CMilAxisAngleRotation3DDuce::GetRealization(
    __out_ecount(1) CMILMatrix *pRealization
    )
{
    HRESULT hr = S_OK;
    vector3 axis;

    IFC(SynchronizeAnimatedFields());

    axis = { m_data.m_axis.X, m_data.m_axis.Y, m_data.m_axis.Z };
    C_ASSERT(sizeof(axis) == sizeof(m_data.m_axis));

    // 
    // 
    // D3DXMatrixRotationAxis (which is part of the deprecated DirectX 9 Extension API's) that 
    // this code was originally written to depend on) has the behavior that if the length^2 of 
    // axis <= FLT_MIN, then the returned matrix will be a uniform scale of cos(angle).
    //
    // This threshold needs to match the one we used in D3DXVec3Normalize (d3dxmath9.cpp)
    // and in managed code.  See also AxisAngleRotation3D.cs.
    if (axis.length_sq() > FLT_MIN)
    {    
        *pRealization = matrix::rotation_axis(axis, DegToRadF(m_data.m_angle));
    }
    else
    {
        // If we have a zero-length axis we set pRealization to identity (i.e.,
        // we consider this to be no rotation.)
        pRealization->SetToIdentity();
    }

Cleanup:
    RRETURN(hr);
}

