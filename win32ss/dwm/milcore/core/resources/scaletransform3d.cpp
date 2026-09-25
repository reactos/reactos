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

MtDefine(ScaleTransform3DResource, MILRender, "ScaleTransform3D Resource");

MtDefine(CMilScaleTransform3DDuce, ScaleTransform3DResource, "CMilScaleTransform3DDuce");

CMilScaleTransform3DDuce::~CMilScaleTransform3DDuce()
{
    UnRegisterNotifiers();
}

void CMilScaleTransform3DDuce::ClearRealization()
{

}

HRESULT CMilScaleTransform3DDuce::GetRealization(
    __out_ecount(1) CMILMatrix *pRealization
    )
{
    HRESULT hr = S_OK;

    IFC(SynchronizeAnimatedFields());

    float sx = static_cast<float>(m_data.m_scaleX);
    float sy = static_cast<float>(m_data.m_scaleY);
    float sz = static_cast<float>(m_data.m_scaleZ);

    pRealization->_12 = pRealization->_13 = pRealization->_14 =
    pRealization->_21 = pRealization->_23 = pRealization->_24 =
    pRealization->_31 = pRealization->_32 = pRealization->_34 = 0.0f;

    pRealization->_11 = sx;
    pRealization->_22 = sy;
    pRealization->_33 = sz;
    pRealization->_44 = 1.0f;

    if (m_data.m_centerX == 0.0 && m_data.m_centerY == 0.0 && m_data.m_centerZ == 0.0)
    {
        pRealization->_41 = 0.0f;
        pRealization->_42 = 0.0f;
        pRealization->_43 = 0.0f;
    }
    else
    {
        float cx = static_cast<float>(m_data.m_centerX);
        float cy = static_cast<float>(m_data.m_centerY);
        float cz = static_cast<float>(m_data.m_centerZ);

        pRealization->_41 = cx - sx * cx;
        pRealization->_42 = cy - sy * cy;
        pRealization->_43 = cz - sz * cz;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT CMilScaleTransform3DDuce::Append(
    __inout_ecount(1) CMILMatrix *pMat
    )
{ 
    HRESULT hr = S_OK;

    IFC(SynchronizeAnimatedFields());
    
    float sx = static_cast<float>(m_data.m_scaleX);
    float sy = static_cast<float>(m_data.m_scaleY);
    float sz = static_cast<float>(m_data.m_scaleZ);

    if (m_data.m_centerX == 0.0 && m_data.m_centerY == 0.0 && m_data.m_centerZ == 0.0)
    {
        pMat->_11 *= sx; pMat->_12 *= sy; pMat->_13 *= sz;
        pMat->_21 *= sx; pMat->_22 *= sy; pMat->_23 *= sz;
        pMat->_31 *= sx; pMat->_32 *= sy; pMat->_33 *= sz;
        pMat->_41 *= sx; pMat->_42 *= sy; pMat->_43 *= sz;
    }
    else
    {
        float cx = static_cast<float>(m_data.m_centerX);
        float cy = static_cast<float>(m_data.m_centerY);
        float cz = static_cast<float>(m_data.m_centerZ);   

        float tmp = pMat->_14 * cx;
        pMat->_11 = tmp + sx * (pMat->_11 - tmp);
        tmp = pMat->_14 * cy;
        pMat->_12 = tmp + sy * (pMat->_12 - tmp);
        tmp = pMat->_14 * cz;
        pMat->_13 = tmp + sz * (pMat->_13 - tmp);

        tmp = pMat->_24 * cx;
        pMat->_21 = tmp + sx * (pMat->_21 - tmp);
        tmp = pMat->_24 * cy;
        pMat->_22 = tmp + sy * (pMat->_22 - tmp);
        tmp = pMat->_24 * cz;
        pMat->_23 = tmp + sz * (pMat->_23 - tmp);

        tmp = pMat->_34 * cx;
        pMat->_31 = tmp + sx * (pMat->_31 - tmp);
        tmp = pMat->_34 * cy;
        pMat->_32 = tmp + sy * (pMat->_32 - tmp);
        tmp = pMat->_34 * cz;
        pMat->_33 = tmp + sz * (pMat->_33 - tmp);

        tmp = pMat->_44 * cx;
        pMat->_41 = tmp + sx * (pMat->_41 - tmp);
        tmp = pMat->_44 * cy;
        pMat->_42 = tmp + sy * (pMat->_42 - tmp);
        tmp = pMat->_44 * cz;
        pMat->_43 = tmp + sz * (pMat->_43 - tmp);
    }

Cleanup:
    RRETURN(hr);
}


