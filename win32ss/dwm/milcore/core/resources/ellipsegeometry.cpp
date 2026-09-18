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
//      The EllipseGeometry CSlaveResource is responsible for maintaining the
//      current base values & animation resources for all EllipseGeometry
//      properties.  This class processes updates to those properties, and
//      obtains their current value when GetShapeDataCore is called.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(EllipseGeometryResource, MILRender, "EllipseGeometry Resource");
MtDefine(CMilEllipseGeometryDuce, EllipseGeometryResource, "CMilEllipseGeometryDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilEllipseGeometryDuce::~CMilEllipseGeometryDuce
//
//  Synopsis:
//      Class destructor.
//
//------------------------------------------------------------------------------
CMilEllipseGeometryDuce::~CMilEllipseGeometryDuce()
{
    UnRegisterNotifiers();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilEllipseGeometryDuce::GetShapeDataCore
//
//  Synopsis:
//      Obtains the current value of this EllipseGeometry from its properties'
//      base and animated values
//
//------------------------------------------------------------------------------
HRESULT 
CMilEllipseGeometryDuce::GetShapeDataCore(
    __deref_out_ecount(1) IShapeData **ppShapeData
        // Output current value of this EllipseGeometry
    )
{
    Assert(ppShapeData != NULL);

    HRESULT hr = S_OK;

    const CMILMatrix *pMatrix;    

    *ppShapeData = NULL;

    m_shape.Reset(FALSE);    

    // Obtain the current value of this geometry
    IFC(AddEllipseCurrentValueToShape(
        &m_data.m_Center,
        m_data.m_pCenterAnimation,
        m_data.m_RadiusX,
        m_data.m_pRadiusXAnimation,
        m_data.m_RadiusY,
        m_data.m_pRadiusYAnimation,
        &m_shape
        ));

    // Obtain the current value of the geometry transform
    IFC(GetMatrixCurrentValue(
        m_data.m_pTransform, 
        &pMatrix
        ));    

    // Transform the shape, if a transform exists    
    if (pMatrix)
    {
        m_shape.Transform(pMatrix);        
    }
   
    // Set the output value upon success
   *ppShapeData = &m_shape;

Cleanup:
    RRETURN(hr);
}



