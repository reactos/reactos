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
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(GeometryResource, MILRender, "Geometry Resource");
MtDefine(CMilGeometryDuce, GeometryResource, "CMilGeometryDuce");

/*++

Routine Description:

    CMilGeometryDuce::GetBounds

--*/

HRESULT CMilGeometryDuce::GetBounds(CMilRectF *pRect)
{
    HRESULT hr = S_OK;

    IShapeData *pShape = NULL;
    IFC(GetShapeData(&pShape));
    
    // Separate scope required for instantiating a class under IFC
    IFC(pShape->GetTightBounds(OUT *pRect));

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilGeometryDuce::GetBoundsSafe

    Returns infinite bounds upon encountering numerical error.

--*/

HRESULT CMilGeometryDuce::GetBoundsSafe(CMilRectF *pRect)
{
    HRESULT hr = S_OK;

    IShapeData *pShape = NULL;
    IFC(GetShapeData(&pShape));
    
    // Separate scope required for instantiating a class under IFC
    IFC(pShape->GetTightBounds(OUT *pRect));

Cleanup:
    if (hr == WGXERR_BADNUMBER)
    {
        *pRect = CMilRectF::sc_rcInfinite;
        hr = S_OK;
    }

    RRETURN(hr);
}


/*++

Routine Description:

    CMilGeometryDuce::GetShapeData

--*/
HRESULT 
CMilGeometryDuce::GetShapeData(
    OUT IShapeData **ppShapeData
    )
{    
    HRESULT hr = S_OK;

    // Initialize out-param to NULL
    *ppShapeData = NULL;

    // Update the cache if this geometry is Dirty
    if (IsDirty())
    {
        m_pCachedShapeData = NULL;
        IFC(GetShapeDataCore(&m_pCachedShapeData));
        SetDirty(FALSE);
    }

    // Set out-param to cached shape data
    *ppShapeData = m_pCachedShapeData;
    
Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    CMilGeometryDuce::GetPointAtLengthFraction

--*/

HRESULT 
CMilGeometryDuce::GetPointAtLengthFraction(
    DOUBLE rFraction,
    OUT MilPoint2D &pt,
    OUT MilPoint2D *pvecTangent
    )
{
    HRESULT hr = S_OK;

    IShapeData *pShape = NULL;
    IFC(GetShapeData(&pShape));

    MilPoint2F ptF;
    MilPoint2F vecTangentF;

    {
        CAnimationPath animationPath;
        IFC(animationPath.SetUp(*pShape));
        
        animationPath.GetPointAtLengthFraction(
            static_cast<REAL>(rFraction), 
            ptF, 
            &vecTangentF
            );
    }

    pt.X = static_cast<DOUBLE>(ptF.X);
    pt.Y = static_cast<DOUBLE>(ptF.Y);

    pvecTangent->X = static_cast<DOUBLE>(vecTangentF.X);
    pvecTangent->Y = static_cast<DOUBLE>(vecTangentF.Y);

Cleanup:
    RRETURN(hr);
}



