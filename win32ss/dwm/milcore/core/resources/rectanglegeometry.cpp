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
//      The RectangleGeometry CSlaveResource is responsible for maintaining the
//      current base values & animation resources for all RectangleGeometry
//      properties.  This class processes updates to those properties, and
//      obtains their current value when GetShapeDataCore is called.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(RectangleGeometryResource, MILRender, "RectangleGeometry Resource");

MtDefine(CMilRectangleGeometryDuce, RectangleGeometryResource, "CMilRectangleGeometryDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRectangleGeometryDuce::~CMilRectangleGeometryDuce
//
//  Synopsis:
//      Class destructor.
//
//------------------------------------------------------------------------------
CMilRectangleGeometryDuce::~CMilRectangleGeometryDuce()
{
    UnRegisterNotifiers();
    delete m_pShape;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRectangleGeometryDuce::GetShapeDataCore
//
//  Synopsis:
//      Obtains the current value of this RectangleGeometry from its properties'
//      base and animated values
//
//------------------------------------------------------------------------------
HRESULT 
CMilRectangleGeometryDuce::GetShapeDataCore(
    __deref_out_ecount(1) IShapeData **ppShapeData
        // Output current value of this RectangleGeometry        
    )
{
    Assert(ppShapeData != NULL);

    HRESULT hr = S_OK;

    const CMILMatrix *pMatrix;    

    MilPointAndSizeF rectCurrentValue;
    FLOAT radiusXCurrentValue, radiusYCurrentValue;
    IShapeData *pShapeData = NULL;
    
    *ppShapeData = NULL;

    // Obtain the current value of this geometry
    IFC(GetRectangleCurrentValue(
        &m_data.m_Rect,
        m_data.m_pRectAnimation,
        m_data.m_RadiusX,
        m_data.m_pRadiusXAnimation,
        m_data.m_RadiusY,
        m_data.m_pRadiusYAnimation,
        &rectCurrentValue, 
        &radiusXCurrentValue, 
        &radiusYCurrentValue
        ));

    // Obtain the current value of the geometry transform
    IFC(GetMatrixCurrentValue(
        m_data.m_pTransform, 
        &pMatrix
        ));

    if (IsRectEmptyOrInvalid(&rectCurrentValue))
    {
        // The rectangle is empty, so construct an empty CShape
        IFCOOM(pShapeData = new CShape);
    }
    else if (radiusXCurrentValue == radiusYCurrentValue &&
        (!pMatrix || pMatrix->IsIdentity()))
    {
        // It's a regular rectangle, so construct it as a CRectangle
        CRectangle *pShape = new CRectangle();
        IFCOOM(pShapeData = pShape);

        IFC(pShape->Set(
            rectCurrentValue,
            radiusXCurrentValue));
    }
    else 
    {
        // It's a rounded rectangle, so construct it as CShape
        CShape *pShape = new CShape;
        IFCOOM(pShapeData = pShape);

        IFC(pShape->AddRoundedRectangle(
            rectCurrentValue, 
            radiusXCurrentValue, 
            radiusYCurrentValue
            ));

        if (pMatrix  &&  !pMatrix->IsIdentity())
        {
            pShape->Transform(pMatrix);
        }
    }

    // Note that we need to invoke Transform on the explicit implementation class,
    // because IShapeData is a read-only interface that does not support Transform.

    // Construction succeeded, hand over the result.
    delete m_pShape;
    *ppShapeData = m_pShape = pShapeData;

    // m_pShape now owns the allocated shape, and is responsible for deleting it.
    pShapeData = NULL;

Cleanup:

    delete pShapeData;
    RRETURN(hr);
}



