// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      Implementation of CMilGeometryGroupDuce
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(GeometryGroupResource, MILRender, "GeometryGroup Resource");

MtDefine(CMilGeometryGroupDuce, GeometryGroupResource, "CMilGeometryGroupDuce");

CMilGeometryGroupDuce::~CMilGeometryGroupDuce()
{
    UnRegisterNotifiers();
}

/*++

Routine Description:

    CMilGeometryGroupDuce::GetShapeDataCore

--*/

HRESULT CMilGeometryGroupDuce::GetShapeDataCore(
    IShapeData **ppShapeData
    )
{
    Assert(ppShapeData != NULL);

    const CMILMatrix *pMatrix = NULL;

    HRESULT hr = S_OK;

    *ppShapeData = NULL;
    m_shape.Reset(FALSE);

    Assert(!m_data.m_cChildren || m_data.m_rgpChildren);

    if (!(EnterResource()))
    {
        // In case of a loop we set ppShapeData to NULL.
        goto Cleanup;
    }

    // Get the current matrix value
    IFC(GetMatrixCurrentValue(m_data.m_pTransform, &pMatrix));
    
    for (UINT i = 0;  i < m_data.m_cChildren;  i++)
    {
        IShapeData *pShape;
        
        IFC(GetGeometryCurrentValue(
            m_data.m_rgpChildren[i],
            &pShape
            ));

        // Aggregate the current shape with m_shape, if this shape is non-null
        if (pShape)
        {
            IFC(m_shape.AddShapeData(*pShape, pMatrix));
        }
    }

    m_shape.SetFillMode(static_cast<MilFillMode::Enum>(m_data.m_FillRule));

    *ppShapeData = &m_shape;

Cleanup:
    LeaveResource();

    RRETURN(hr);
}



