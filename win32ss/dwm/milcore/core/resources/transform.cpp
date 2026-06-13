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

MtDefine(TransformResource, MILRender, "Transform Resource");

MtDefine(CMilTransformDuce, TransformResource, "CMilTransformDuce");

/*++

Routine Description:

    CMilTransformDuceExtra::GetMatrix

--*/

HRESULT 
CMilTransformDuce::GetMatrix(
    OUT CMILMatrix const **ppMatrix
    )
{
    HRESULT hr = S_OK;

    *ppMatrix = NULL;

    if (IsDirty())
    {
        IFC(GetMatrixCore(&m_matrix));

        SetDirty(FALSE);
    }

    *ppMatrix = &m_matrix;

Cleanup:

    // ppMatrix should be non-NULL unless this operation failed
    Assert (*ppMatrix || FAILED(hr));
    
    RRETURN(hr);
}


