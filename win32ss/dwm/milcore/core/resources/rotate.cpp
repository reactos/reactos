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

MtDefine(RotateTransformResource, MILRender, "RotateTransform Resource");

MtDefine(CMilRotateTransformDuce, RotateTransformResource, "CMilRotateTransformDuce");

CMilRotateTransformDuce::~CMilRotateTransformDuce()
{
    UnRegisterNotifiers();
}

/*++

Routine Description:

    CMilRotateTransformDuce::GetMatrixCore

--*/

HRESULT CMilRotateTransformDuce::GetMatrixCore(
    CMILMatrix *pmat
    )
{
    HRESULT hr = S_OK;
    Assert(pmat != NULL);

    DOUBLE angle, centerX, centerY;

    IFC(SynchronizeAnimatedFields());

    angle = m_data.m_Angle;

    // Take angle value modulo 360 before casting to float to avoid
    // excessive loss of precision when going from double to float.
    // Otherwise even angles as small as 36000000 will be inaccurate
    angle = fmod(angle,360);

    centerX = m_data.m_CenterX;
    centerY = m_data.m_CenterY;

    {
        auto result = 
            matrix::get_translation(
                static_cast<float>(-centerX), 
                static_cast<float>(-centerY), 
                0.0f);

        auto rotate 
            = matrix::get_rotation_z(math_extensions::to_radian(static_cast<float>(angle)));

        result = result * rotate;

        auto translate = 
            matrix::get_translation(
                static_cast<float>(centerX), 
                static_cast<float>(centerY),
                0.0f);

        *pmat = result * translate;
    }

Cleanup:
    RRETURN(hr);
}


