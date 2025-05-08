// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//       Contains functions which retrieve the current value of resources.
//

#include "precomp.hpp"

//+------------------------------------------------------------------------
//
//  Function:  
//      GetMatrixCurrentValue
//
//  Synopsis:  
//      Obtains the current value of a matrix resource.
//
//-------------------------------------------------------------------------
HRESULT 
GetMatrixCurrentValue(
    __in_ecount_opt(1) CMilTransformDuce *pResource,        
        // Transform resource to get current value from
    __deref_out_ecount_opt(1) CMILMatrix const **ppMatrix   
        // Current value of the transform resource
    )
{
    HRESULT hr = S_OK;

    // Initialize out-param to NULL
    *ppMatrix = NULL;

    // Obtain current matrix value if there is a resource
    if (pResource)
    {
        IFC(pResource->GetMatrix(ppMatrix));
    }

Cleanup:

    // ppMatrix must be non-NULL unless pResource was NULL or we failed
    Assert(*ppMatrix || !pResource || FAILED(hr));
        
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  
//      GetGeometryCurrentValue
//
//  Synopsis:  
//      Obtains the current value of a geometry resource.
//
//-------------------------------------------------------------------------
HRESULT 
GetGeometryCurrentValue(
    __in_ecount_opt(1) CMilGeometryDuce *pResource,        
        // Geometry resource to obtain current value for
    __deref_out_ecount_opt(1) IShapeData **ppShapeData  
        // Shape data representing the current value of the geometry
    )
{
    HRESULT hr = S_OK;

    // Initialize out-param to NULL
    *ppShapeData = NULL;
    
    // Obtain current geometry value if there is a resource
    if (pResource)
    {    
        IFC(pResource->GetShapeData(ppShapeData));
    }
    
Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  
//      GetBitmapCurrentValue
//
//  Synopsis:  
//      Obtains the current value of a bitmap resource.
//
//      Note: This will return NULL if pImageSource is NULL or if 
//      pImageSource is not a BitmapSource
//
//-------------------------------------------------------------------------
HRESULT 
GetBitmapCurrentValue(
    __in_ecount_opt(1) CMilImageSource *pImageSource,        
        // Bitmap resource to obtain current value for
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppBitmapSource  
        // Output current value of the bitmap
    )
{
    HRESULT hr = S_OK;

    // Initialize out-param to NULL
    *ppBitmapSource = NULL;
    
    // Obtain current bitmap value if there is a resource
    if (pImageSource)
    {    
        IFC(pImageSource->GetBitmapSource(ppBitmapSource));
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:
//      SetLineCurrentValue
//
//  Synopsis:
//      Obtains the current value of the line parameters, and sets them
//      on the output CLine.
//
//----------------------------------------------------------------------------
HRESULT
SetLineCurrentValue(
    __in_ecount(1) const MilPoint2D *pBasePoint0Value,
        // Base value of the first line point
    __in_ecount_opt(1) CMilSlaveResource *pPoint0Animations,
        // Optional animations that override the base Point0 value
    __in_ecount(1) const MilPoint2D *pBasePoint1Value,
        // Base value of the second line point    
    __in_ecount_opt(1) CMilSlaveResource *pPoint1Animations, 
        // Optional animations that override the base Point0 value    
    __inout_ecount(1) CLine *pLineCurrentValue
        // Shape to which the output current value is set.
    )
{
    HRESULT hr = S_OK;

    MilPoint2D point0CurrentValue;
    MilPoint2D point1CurrentValue;

    Assert(pLineCurrentValue);

    // Obtain the current value of the line points

    IFC(GetPointCurrentValue(
        pBasePoint0Value,
        pPoint0Animations,
        &point0CurrentValue
        ));

    IFC(GetPointCurrentValue(
        pBasePoint1Value,
        pPoint1Animations,
        &point1CurrentValue
        ));    

    // Set the line

    pLineCurrentValue->Set(
        static_cast<FLOAT>(point0CurrentValue.X),
        static_cast<FLOAT>(point0CurrentValue.Y),
        static_cast<FLOAT>(point1CurrentValue.X),
        static_cast<FLOAT>(point1CurrentValue.Y)
        );

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:
//      GetRectangleCurrentValue
//
//  Synopsis:
//      Obtains the current value of the (rounded) rectangle parameters
//
//----------------------------------------------------------------------------
HRESULT
GetRectangleCurrentValue(
    __in_ecount(1) const MilPointAndSizeD *pBaseRectangleValue,
        // Base value of the rectangle
    __in_ecount_opt(1) CMilSlaveResource *pRectangleAnimations,
        // Optional animations that override the base rectangle value    
    DOUBLE rRadiusXBaseValue, 
        // Base value of the x-radius    
    __in_ecount_opt(1) CMilSlaveResource *pRadiusXAnimations,
        // Optional animations that override the base x-radius 
    DOUBLE rRadiusYBaseValue, 
        // Base value of the y-radius        
    __in_ecount_opt(1) CMilSlaveResource *pRadiusYAnimations,   
        // Optional animations that override the base y-radius     
    __out_ecount(1) MilPointAndSizeF *pRectCurrentValue,
        // The rectangle's current value    
    __out_ecount(1) FLOAT *pRadiusXCurrentValue,
        // The RadiusX current value    
    __out_ecount(1) FLOAT *pRadiusYCurrentValue
        // The RadiusY current value    
    )
{
    HRESULT hr = S_OK;
    
    MilPointAndSizeD rectCurrentValueD;
    DOUBLE radiusXCurrentValueD, radiusYCurrentValueD;

    Assert(pRectCurrentValue);    
    Assert(pRadiusXCurrentValue);    
    Assert(pRadiusYCurrentValue);    

    // Obtain current value of the rectangle
    
    IFC(GetRectCurrentValue(
        pBaseRectangleValue,
        pRectangleAnimations,
        &rectCurrentValueD
        ));

    MilPointAndSizeFFromMilPointAndSizeD(pRectCurrentValue, &rectCurrentValueD);

    // Obtain current radii values

    IFC(GetDoubleCurrentValue(
        &rRadiusXBaseValue,
        pRadiusXAnimations,
        &radiusXCurrentValueD
        ));

    IFC(GetDoubleCurrentValue(
        &rRadiusYBaseValue,
        pRadiusYAnimations,
        &radiusYCurrentValueD
        ));

    *pRadiusXCurrentValue = static_cast<FLOAT>(radiusXCurrentValueD);
    *pRadiusYCurrentValue = static_cast<FLOAT>(radiusYCurrentValueD);

Cleanup:

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:
//      AddEllipseCurrentValueToShape
//
//  Synopsis:
//      Obtains the current value of the ellipse parameters, and adds them
//      to the output CShape.
//
//----------------------------------------------------------------------------
HRESULT
AddEllipseCurrentValueToShape(
    __in_ecount(1) const MilPoint2D *pCenterBaseValue,
        // Base value of the center    
    __in_ecount_opt(1) CMilSlaveResource *pCenterAnimations,
        // Optional animations that override the base center value     
    DOUBLE rRadiusXBaseValue, 
        // Base value of the x-radius    
    __in_ecount_opt(1) CMilSlaveResource *pRadiusXAnimations,
        // Optional animations that override the base x-radius value     
    DOUBLE rRadiusYBaseValue, 
        // Base value of the y-radius        
    __in_ecount_opt(1) CMilSlaveResource *pRadiusYAnimations,   
        // Optional animations that override the base y-radius value        
    __inout_ecount(1) CShape *pEllipseShapeCurrentValue
        // Shape to which the output current value is set.        
    )
{
    HRESULT hr = S_OK;

    MilPoint2D centerCurrentValue;
    DOUBLE  radiusXCurrentValue;
    DOUBLE  radiusYCurrentValue;

    Assert(pEllipseShapeCurrentValue);        

    // An empty shape with no figures is expected
    Assert(pEllipseShapeCurrentValue->GetFigureCount() == 0);

    // Obtain current value of the center point

    IFC(GetPointCurrentValue(
        pCenterBaseValue,
        pCenterAnimations,
        &centerCurrentValue
        ));

    // Obtain current radii values

    IFC(GetDoubleCurrentValue(
        &rRadiusXBaseValue,
        pRadiusXAnimations,
        &radiusXCurrentValue
        ));

    IFC(GetDoubleCurrentValue(
        &rRadiusYBaseValue,
        pRadiusYAnimations,
        &radiusYCurrentValue
        ));

    // Add the ellipse to the shape

    IFC(pEllipseShapeCurrentValue->AddEllipse(
        static_cast<REAL>(centerCurrentValue.X),
        static_cast<REAL>(centerCurrentValue.Y),        
        static_cast<REAL>(radiusXCurrentValue),
        static_cast<REAL>(radiusYCurrentValue),
        CR_Parameters
        ));               

Cleanup:
    
    RRETURN(hr);
}
    


