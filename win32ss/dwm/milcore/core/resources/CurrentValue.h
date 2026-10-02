// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Abstract:
//       Contains functions which retrieve the current value of resources.
//
    
//+------------------------------------------------------------------------
//
//  Function:  
//      GetTypeSpecificResource
//
//  Synopsis:  
//      Downcasts from a CMilSlaveResource to the resource type
//      specified by the out-param.  Before downcasting, this 
//      method also checks the Type(), and returns E_INVALIDARG
//      if the type is invalid.
//
//-------------------------------------------------------------------------
template <typename TResourceType>
HRESULT
GetTypeSpecificResource(
    __in_ecount_opt(1) CMilSlaveResource *pResource,        
        // Resource to downcast
    IN MIL_RESOURCE_TYPE type,                          
        // Type() that pResource must be
    __deref_out_ecount_opt(1) TResourceType **ppSpecificResource    
        // Output type-specific pointer
    )
{
    HRESULT hr = S_OK;

    Assert(ppSpecificResource);

    // Initialize out-param to NULL
    *ppSpecificResource = NULL;
    
    if (pResource)
    {
        if (!pResource->IsOfType(type))
        {
            IFC(E_INVALIDARG);
        }
        *ppSpecificResource = DYNCAST(TResourceType, pResource);
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   
//      GetCurrentValue
//
//  Synopsis:   
//      Retrieves the current value of a resource given it's base
//      value, resource animations, and resource animations type.
//
//  Notes:      
//      The resource type, TResourceType, must implement a method
//      with the following signature to use this template method:
//    
//      TBaseType *GetValue();
//
//      GetValue is automatically implemented by all of our template 
//      TMilSlaveValue types.
//
//-------------------------------------------------------------------------
template <typename TBaseType, typename TResourceType>
HRESULT
GetCurrentValue(
    __in_ecount(1) const TBaseType *pBaseValue,         
        // Base value of type
    __in_ecount_opt(1) CMilSlaveResource *pAnimations,  
        // Optional animations
    MIL_RESOURCE_TYPE type,                             
        // Type that pAnimations must be
    __out_ecount(1) TBaseType *pCurrentValue            
        // Output current value
    )
{
    HRESULT hr = S_OK;    
    TResourceType *pAnimationsType;

    Assert(pBaseValue && pCurrentValue);

    // Initialize out-param to base value
    *pCurrentValue = *pBaseValue;    

    // Cast pAnimations to a specific resource type
    IFC(GetTypeSpecificResource<TResourceType>(
        pAnimations, 
        type,
        &pAnimationsType
        ));

    // Override the base value with the animations, if animations exist.
    if (NULL != pAnimationsType)
    {       
        // Obtain value from rectangle resource
        *pCurrentValue = *(pAnimationsType->GetValue());
    }
        
Cleanup:
    
    RRETURN(hr);
}


//
// Non-templated current value methods
// 

HRESULT 
GetMatrixCurrentValue(
    __in_ecount_opt(1) CMilTransformDuce *pResource, 
    __deref_out_ecount_opt(1) CMILMatrix const ** ppMatrix
    );

HRESULT 
GetGeometryCurrentValue(
    __in_ecount_opt(1) CMilGeometryDuce *pResource, 
    __deref_out_ecount_opt(1) IShapeData ** ppShapeData
    );

HRESULT 
GetBitmapCurrentValue(
    __in_ecount_opt(1) CMilImageSource *pImageSource,        
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppBitmapSource  
    );

HRESULT
SetLineCurrentValue(
    __in_ecount(1) const MilPoint2D *pBasePoint0Value,
    __in_ecount_opt(1) CMilSlaveResource *pPoint0Animations,
    __in_ecount(1) const MilPoint2D *pBasePoint1Value,
    __in_ecount_opt(1) CMilSlaveResource *pPoint1Animations, 
    __inout_ecount(1) CLine *pLineCurrentValue
    );

HRESULT
GetRectangleCurrentValue(
    __in_ecount(1) const MilPointAndSizeD *pBaseRectangleValue,
    __in_ecount_opt(1) CMilSlaveResource *pRectangleAnimations,
    DOUBLE rRadiusXBaseValue, 
    __in_ecount_opt(1) CMilSlaveResource *pRadiusXAnimations,
    DOUBLE rRadiusYBaseValue, 
    __in_ecount_opt(1) CMilSlaveResource *pRadiusYAnimations,   
    __out_ecount(1) MilPointAndSizeF *pRectCurrentValue,
    __out_ecount(1) FLOAT *pRadiusXCurrentValue,
    __out_ecount(1) FLOAT *pRadiusYCurrentValue
    );

HRESULT
AddEllipseCurrentValueToShape(
    __in_ecount(1) const MilPoint2D *pCenterBaseValue,
    __in_ecount_opt(1) CMilSlaveResource *pCenterAnimations,
    DOUBLE rRadiusXBaseValue, 
    __in_ecount_opt(1) CMilSlaveResource *pRadiusXAnimations,
    DOUBLE rRadiusYBaseValue, 
    __in_ecount_opt(1) CMilSlaveResource *pRadiusYAnimations,   
    __inout_ecount(1) CShape *pRectangleShapeCurrentValue
    );

//
// Type-specific inline current value wrappers
//

inline HRESULT 
GetDoubleCurrentValue(
    __in_ecount(1) const DOUBLE *prBaseValue, 
    __in_ecount_opt(1) CMilSlaveResource *pDoubleAnimations,
    __out_ecount(1) DOUBLE *prCurrentValue
    )
{
    // Unable to use INLINED_RRETURN here because we are calling a template function,
    // which results in compiler errors when used inside of a macro.    
    return GetCurrentValue<DOUBLE, CMilSlaveDouble>(
        prBaseValue,
        pDoubleAnimations,
        TYPE_DOUBLERESOURCE,
        prCurrentValue
        );
}

inline HRESULT 
GetPointCurrentValue(
    __in_ecount(1) const MilPoint2D *pBaseValue,
    __in_ecount_opt(1) CMilSlaveResource *pPointAnimations,    
    __out_ecount(1) MilPoint2D *pCurrentValue
    )
{
    // Unable to use INLINED_RRETURN here because we are calling a template function,
    // which results in compiler errors when used inside of a macro.    
    return GetCurrentValue<MilPoint2D, CMilSlavePoint>(
        pBaseValue,
        pPointAnimations,
        TYPE_POINTRESOURCE,
        pCurrentValue
        );
}
        
inline HRESULT 
GetRectCurrentValue(
    __in_ecount(1) const MilPointAndSizeD *pBaseRectangleValue,
    __in_ecount_opt(1) CMilSlaveResource *pRectangleAnimations,
    __out_ecount(1) MilPointAndSizeD *pCurrentRectangleValue
    )
{
    // Unable to use INLINED_RRETURN here because we are calling a template function,
    // which results in compiler errors when used inside of a macro.
    return GetCurrentValue<MilPointAndSizeD, CMilSlaveRect>(
        pBaseRectangleValue,
        pRectangleAnimations,
        TYPE_RECTRESOURCE,
        pCurrentRectangleValue
        );    
}

inline HRESULT 
GetColorCurrentValue(
    __in_ecount(1) const MilColorF *pBaseColorValue,
    __in_ecount_opt(1) CMilSlaveColor *pColorAnimations,
        // Color resource to obtain current value for
    __out_ecount_opt(1) MilColorF *pCurrentColorValue
        // Color data representing the current value of the color
    )
{
    // Unable to use INLINED_RRETURN here because we are calling a template function,
    // which results in compiler errors when used inside of a macro.
    return GetCurrentValue<MilColorF, CMilSlaveColor>(
        pBaseColorValue,
        pColorAnimations,
        TYPE_COLORRESOURCE,
        pCurrentColorValue
        );    
}

inline HRESULT 
GetSizeCurrentValue(
    __in_ecount(1) const MilSizeD *pBaseSizeValue,
    __in_ecount_opt(1) CMilSlaveResource *pSizeAnimations,
    __out_ecount(1) MilSizeD *pCurrentSizeValue
    )
{
    // Unable to use INLINED_RRETURN here because we are calling a template function,
    // which results in compiler errors when used inside of a macro.
    return GetCurrentValue<MilSizeD, CMilSlaveSize>(
        pBaseSizeValue,
        pSizeAnimations,
        TYPE_SIZERESOURCE,
        pCurrentSizeValue
        );    
}


