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
//      The LineGeometry CSlaveResource is responsible for maintaining the
//      current base values & animation resources for all LineGeometry
//      properties.  This class processes updates to those properties, and
//      obtains their current value when GetShapeDataCore is called.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(LineGeometryResource, MILRender, "LineGeometry Resource");

MtDefine(CMilLineGeometryDuce, LineGeometryResource, "CMilLineGeometryDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilLineGeometryDuce::~CMilLineGeometryDuce
//
//  Synopsis:
//      Class destructor.
//
//------------------------------------------------------------------------------
CMilLineGeometryDuce::~CMilLineGeometryDuce()
{
    UnRegisterNotifiers();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilLineGeometryDuce::GetShapeDataCore
//
//  Synopsis:
//      Obtains the current value of this LineGeometry from its properties' base
//      and animated values
//
//------------------------------------------------------------------------------
HRESULT CMilLineGeometryDuce::GetShapeDataCore(
    __deref_out_ecount(1) IShapeData **ppShapeData
        // Output current value of this LineGeometry    
    )
{
    Assert(ppShapeData != NULL);

    HRESULT hr = S_OK;

    const CMILMatrix *pMatrix;    

    *ppShapeData = NULL;

    // Obtain the current value of this geometry
    IFC(SetLineCurrentValue(
        &m_data.m_StartPoint,
        m_data.m_pStartPointAnimation,
        &m_data.m_EndPoint,
        m_data.m_pEndPointAnimation,
        &m_line
        ));

    // Obtain the current value of the geometry transform
    IFC(GetMatrixCurrentValue(
        m_data.m_pTransform, 
        &pMatrix
        ));

    // Transform the shape, if a transform exists
    if (pMatrix)
    {
        m_line.Transform(pMatrix);        
    }

    // Set the output value upon success
    *ppShapeData = &m_line;

Cleanup:
    RRETURN(hr);
}



