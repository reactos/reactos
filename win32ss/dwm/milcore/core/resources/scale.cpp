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


MtDefine(ScaleTransformResource, MILRender, "ScaleTransform Resource");

MtDefine(CMilScaleTransformDuce, ScaleTransformResource, "CMilScaleTransformDuce");

CMilScaleTransformDuce::~CMilScaleTransformDuce()
{
    UnRegisterNotifiers();
}

/*++

Routine Description:

    CMilScaleTransformDuce::GetMatrixCore

--*/

HRESULT CMilScaleTransformDuce::GetMatrixCore(
    CMILMatrix *pmat
    )
{
    HRESULT hr = S_OK;
    Assert(pmat != NULL);

    IFC(SynchronizeAnimatedFields());

    DOUBLE scaleX, scaleY, centerX, centerY;

    scaleX = m_data.m_ScaleX;
    scaleY = m_data.m_ScaleY;
    centerX = m_data.m_CenterX;
    centerY = m_data.m_CenterY;

    {
        auto result =
            matrix::get_translation(
                static_cast<float>(-centerX),
                static_cast<float>(-centerY),
                0.0f
            );

        auto scale =
            matrix::get_scaling(
                static_cast<float>(scaleX),
                static_cast<float>(scaleY),
                1.0f
            );

        result = result * scale;

        auto translation =
            matrix::get_translation(
                static_cast<float>(centerX), 
                static_cast<float>(centerY),
                0.0f
            );

        *pmat = result * translation;
    }

Cleanup:
    RRETURN(hr);
}


