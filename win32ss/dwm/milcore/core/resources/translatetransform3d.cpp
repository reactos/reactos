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

MtDefine(TranslateTransform3DResource, MILRender, "TranslateTransform3D Resource");
MtDefine(CMilTranslateTransform3DDuce, TranslateTransform3DResource, "CMilTranslateTransform3DDuce");

CMilTranslateTransform3DDuce::~CMilTranslateTransform3DDuce()
{
    UnRegisterNotifiers();
}

void CMilTranslateTransform3DDuce::ClearRealization()
{
}

HRESULT CMilTranslateTransform3DDuce::GetRealization(
    __out_ecount(1) CMILMatrix *pRealization
    )
{
    HRESULT hr = S_OK;

    IFC(SynchronizeAnimatedFields());

                        pRealization->_12 = pRealization->_13 = pRealization->_14 =
    pRealization->_21 =                     pRealization->_23 = pRealization->_24 =
    pRealization->_31 = pRealization->_32 =                     pRealization->_34 = 0.0f;
    
    pRealization->_11 = pRealization->_22 = pRealization->_33 = pRealization->_44 = 1.0f;
    
    pRealization->_41 = static_cast<float>(m_data.m_offsetX);
    pRealization->_42 = static_cast<float>(m_data.m_offsetY);
    pRealization->_43 = static_cast<float>(m_data.m_offsetZ);

Cleanup:
    RRETURN(hr);
}

HRESULT CMilTranslateTransform3DDuce::Append(
    __inout_ecount(1) CMILMatrix *pMat
    )
{
    HRESULT hr = S_OK;

    IFC(SynchronizeAnimatedFields());

    float tx = static_cast<float>(m_data.m_offsetX);
    float ty = static_cast<float>(m_data.m_offsetY);
    float tz = static_cast<float>(m_data.m_offsetZ);

    pMat->_11 += pMat->_14 * tx; pMat->_12 += pMat->_14 * ty; pMat->_13 += pMat->_14 * tz;
    pMat->_21 += pMat->_24 * tx; pMat->_22 += pMat->_24 * ty; pMat->_23 += pMat->_24 * tz;
    pMat->_31 += pMat->_34 * tx; pMat->_32 += pMat->_34 * ty; pMat->_33 += pMat->_34 * tz;
    pMat->_41 += pMat->_44 * tx; pMat->_42 += pMat->_44 * ty; pMat->_43 += pMat->_44 * tz;

Cleanup:
    RRETURN(hr);
}


