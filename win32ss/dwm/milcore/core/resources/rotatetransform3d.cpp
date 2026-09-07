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

MtDefine(RotateTransform3DResource, MILRender, "RotateTransform3D Resource");

MtDefine(CMilRotateTransform3DDuce, RotateTransform3DResource, "CMilRotateTransform3DDuce");

CMilRotateTransform3DDuce::~CMilRotateTransform3DDuce()
{
    UnRegisterNotifiers();
}

void CMilRotateTransform3DDuce::ClearRealization()
{
}

HRESULT CMilRotateTransform3DDuce::GetRealization(
    __out_ecount(1) CMILMatrix *pRealization
    )
{
    HRESULT hr = S_OK;

    IFC(SynchronizeAnimatedFields());

    if (m_data.m_pRotation != NULL)
    {
        IFC(m_data.m_pRotation->GetRealization(pRealization));
        
        if (m_data.m_centerX != 0.0 || m_data.m_centerY != 0.0 || m_data.m_centerZ != 0.0)
        {
            float cx = static_cast<float>(m_data.m_centerX);
            float cy = static_cast<float>(m_data.m_centerY);
            float cz = static_cast<float>(m_data.m_centerZ);

            pRealization->_41 = cx - pRealization->_11 * cx - pRealization->_21 * cy - pRealization->_31 * cz;
            pRealization->_42 = cy - pRealization->_12 * cx - pRealization->_22 * cy - pRealization->_32 * cz;
            pRealization->_43 = cz - pRealization->_13 * cx - pRealization->_23 * cy - pRealization->_33 * cz;
        }
    }
    else
    {
        pRealization->reset_to_identity();
    }

Cleanup:
    RRETURN(hr);
}

HRESULT CMilRotateTransform3DDuce::Append(
    __inout_ecount(1) CMILMatrix *pMat
    )
{
    HRESULT hr = S_OK;
    
    CMILMatrix mTemp;
    IFC(GetRealization(&mTemp));

    pMat->Multiply(mTemp);

Cleanup:
    RRETURN(hr);
}


