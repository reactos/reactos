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

MtDefine(TransformGroupResource, MILRender, "TransformGroup Resource");

MtDefine(CMilTransformGroupDuce, TransformGroupResource, "CMilTransformGroupDuce");

CMilTransformGroupDuce::~CMilTransformGroupDuce()
{
    UnRegisterNotifiers();
}

/*++

Routine Description:

    CMilTransformGroupDuce::GetMatrixCore

--*/

HRESULT CMilTransformGroupDuce::GetMatrixCore(
    CMILMatrix *pmat
    )
{
    Assert(pmat != NULL);

    HRESULT hr = S_OK;

    pmat->SetToIdentity();

    if (!(EnterResource()))
    {
        // In case of a loop we set it to identity.
        goto Cleanup;
    }

    for (UINT32 i=0; i<m_data.m_cChildren; i++)
    {
        if (m_data.m_rgpChildren[i])
        {            
            const CMILMatrix *pmatNext;

            IFC(GetMatrixCurrentValue(m_data.m_rgpChildren[i], &pmatNext));

            pmat->Multiply(*pmatNext);                
        }
        else
        {
            IFC(E_INVALIDARG);
        }
    }

Cleanup:
    LeaveResource();

    RRETURN(hr);
}


