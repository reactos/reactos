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

MtDefine(MatrixTransformResource, MILRender, "MatrixTransform Resource");

MtDefine(CMilMatrixTransformDuce, MatrixTransformResource, "CMilMatrixTransformDuce");

CMilMatrixTransformDuce::~CMilMatrixTransformDuce()
{
    UnRegisterNotifiers();
}


/*++

Routine Description:

    CMilMatrixTransformDuce::GetMatrixCore

--*/

HRESULT CMilMatrixTransformDuce::GetMatrixCore(
    CMILMatrix *pmat
    )
{
    Assert(pmat != NULL);

    HRESULT hr = S_OK;

    const MilMatrix3x2D *pMatD = NULL;
    MilMatrix3x2D tempMat;

    if (m_data.m_pMatrixAnimation)
    {
        IFC(m_data.m_pMatrixAnimation->GetValue(&tempMat));
        pMatD = &tempMat;
    }
    else
    {
        pMatD = &m_data.m_Matrix;
    }

    pmat->_11 = static_cast<FLOAT> (pMatD->S_11);
    pmat->_12 = static_cast<FLOAT> (pMatD->S_12);
    pmat->_13 = 0.0f;
    pmat->_14 = 0.0f;

    pmat->_21 = static_cast<FLOAT> (pMatD->S_21);
    pmat->_22 = static_cast<FLOAT> (pMatD->S_22);
    pmat->_23 = 0.0f;
    pmat->_24 = 0.0f;

    pmat->_31 = 0.0f;
    pmat->_32 = 0.0f;
    pmat->_33 = 1.0f;
    pmat->_34 = 0.0f;

    pmat->_41 = static_cast<FLOAT> (pMatD->DX);
    pmat->_42 = static_cast<FLOAT> (pMatD->DY);
    pmat->_43 = 0.0f;
    pmat->_44 = 1.0f;

Cleanup:
    RRETURN(hr);
}


