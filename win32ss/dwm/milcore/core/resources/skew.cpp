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

MtDefine(SkewTransformResource, MILRender, "SkewTransform Resource");

MtDefine(CMilSkewTransformDuce, SkewTransformResource, "CMilSkewTransformDuce");

CMilSkewTransformDuce::~CMilSkewTransformDuce()
{
    UnRegisterNotifiers();
}

/*++

Routine Description:

    CMilSkewTransformDuce::GetMatrixCore

--*/

HRESULT CMilSkewTransformDuce::GetMatrixCore(
    CMILMatrix *pmat
    )
{
    HRESULT hr = S_OK;
    Assert(pmat != NULL);

    DOUBLE angleX, angleY, centerX, centerY;

    IFC(SynchronizeAnimatedFields());
    
    angleX = m_data.m_AngleX;
    angleY = m_data.m_AngleY;
    centerX = m_data.m_CenterX;
    centerY = m_data.m_CenterY;

    {
        auto result = matrix::get_translation(
            static_cast<float>(-centerX),
            static_cast<float>(-centerY),
            0.0f
        );

        // Take angle value modulo 360 before converting to radians
        // to get extra precision for large angles (greater than 360.)
        angleX = fmod(angleX,360);
        angleY = fmod(angleY,360);

        CMILMatrix skew(
            1.0f, static_cast<FLOAT> (tan(math_extensions::to_radian(static_cast<float>(angleY)))), 0.0f, 0.0f,
            static_cast<FLOAT> (tan(math_extensions::to_radian(static_cast<float>(angleX)))), 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f);

        result = result * skew;

        auto translate = matrix::get_translation(
            static_cast<float>(centerX),
            static_cast<float>(centerY),
            0.0f
        );

        *pmat = result * translate;
    }

Cleanup:
    RRETURN(hr);
}


