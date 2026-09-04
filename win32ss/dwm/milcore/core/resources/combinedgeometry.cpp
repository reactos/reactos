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
//      Implementation of CMilCombinedGeometryDuce
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CombinedGeometryResource, MILRender, "CombinedGeometry Resource");

MtDefine(CMilCombinedGeometryDuce, CombinedGeometryResource, "CMilCombinedGeometryDuce");

CMilCombinedGeometryDuce::~CMilCombinedGeometryDuce()
{
    UnRegisterNotifiers();
}

/*++

Routine Description:

    CMilCombinedGeometryDuce::GetShapeDataCore

--*/

HRESULT CMilCombinedGeometryDuce::GetShapeDataCore(
    __deref_out_ecount(1) IShapeData **ppShapeData
    )
{
    Assert(ppShapeData != NULL);

    HRESULT hr = S_OK;

    IShapeData *pShape1 = NULL;
    IShapeData *pShape2 = NULL;

    bool fAllocatedShape1 = false;
    bool fAllocatedShape2 = false;

    const CMILMatrix *pMatrix;
        
    *ppShapeData = NULL;

    if (!(EnterResource()))
    {
        // In case of a loop we set ppShapeData to NULL.
        goto Cleanup;
    }

    m_shape.Reset(FALSE);

    //
    // Get the current values of the geometries.
    // A null geometry pointer is interpreted as an empty geometry
    //

    IFC(GetGeometryCurrentValue(
        m_data.m_pGeometry1,
        &pShape1
        ));
    if (!pShape1)
    {
        IFCOOM(pShape1 = new CShape);
        fAllocatedShape1 = true;
    }


    IFC(GetGeometryCurrentValue(
        m_data.m_pGeometry2,
        &pShape2
        ));
    if (!pShape2)
    {
        IFCOOM(pShape2 = new CShape);
        fAllocatedShape2 = true;
    }
    
    //
    // Get the current matrix value
    // 

    IFC(GetMatrixCurrentValue(
        m_data.m_pTransform,
        &pMatrix
        ));

    //
    // Combine the shapes
    //
    
    IFC(CShapeBase::Combine(
        pShape1, 
        pShape2, 
        m_data.m_GeometryCombineMode, 
        true,  // ==> Do retrieve curves from the flattened result
        &m_shape,
        pMatrix,
        pMatrix
        ));

    *ppShapeData = &m_shape;        

Cleanup:
    if (fAllocatedShape1)
    {
        delete pShape1;
    }
    
    if (fAllocatedShape2)
    {
        delete pShape2;
    }

    LeaveResource();

    RRETURN(hr);
}



