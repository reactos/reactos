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

MtDefine(MatrixTransform3DResource, MILRender, "MatrixTransform3D Resource");

MtDefine(CMilMatrixTransform3DDuce, MatrixTransform3DResource, "CMilMatrixTransform3DDuce");

CMilMatrixTransform3DDuce::~CMilMatrixTransform3DDuce()
{
    UnRegisterNotifiers();
}

void CMilMatrixTransform3DDuce::ClearRealization()
{
}

HRESULT CMilMatrixTransform3DDuce::GetRealization(
    __out_ecount(1) CMILMatrix *pRealization
    )
{
    *pRealization = m_data.m_matrix;

    RRETURN(S_OK);
}

HRESULT CMilMatrixTransform3DDuce::Append(
    __inout_ecount(1) CMILMatrix *pMat
    )
{
    pMat->Multiply(static_cast<CMILMatrix>(m_data.m_matrix));    

    RRETURN(S_OK);
}


