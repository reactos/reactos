// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_camera
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(CameraResource, MILRender, "Camera Resource");
MtDefine(CMilCameraDuce, CameraResource, "CMilCameraDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilCameraDuce::PrependInverseTransform
//
//  Synopsis:
//      Helper method to prepend the inverse of Camera.Transform to the the
//      given pViewMatrix.  This is used by the various GetViewMatrix()
//      implementations.
//
//      Transforming the camera is equivalent to applying the inverse transform
//      to the scene.  We invert the transform and prepend it to the result of
//      pViewMatrix:
//
//                                    -1
//      pViewMatrix = Camera.Transform   x pViewMatrix
//
//      If the matrix is not invertable we zero the pViewMatrix to prevent
//      rendering.  This is the correct behavior since the near and far planes
//      will have collapsed onto each other.
//
//------------------------------------------------------------------------------
/* static */ HRESULT CMilCameraDuce::PrependInverseTransform(
    __in_ecount_opt(1) CMilTransform3DDuce* pTransform,
    __inout_ecount(1) CMILMatrix *pViewMatrix
    )
{
    HRESULT hr = S_OK;

    if (pTransform)
    {
        CMILMatrix matrix;
        IFC(pTransform->GetRealization(&matrix));

        // CMILMatrix::Invert returns FALSE if the matrix is not invertible.
        if (matrix.Invert())
        {
            auto& view_matrix = *pViewMatrix;
            *pViewMatrix = matrix * view_matrix;
        }
        else
        {
            ZeroMemory(pViewMatrix, sizeof(*pViewMatrix));
        }
    }
    
Cleanup:
    RRETURN(hr);
}

