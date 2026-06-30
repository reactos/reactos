// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilGradientBrushDuce);

// Class: CMilGradientBrushDuce
class CMilGradientBrushDuce : public CMilBrushDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilGradientBrushDuce));

    CMilGradientBrushDuce(__in_ecount(1) CComposition* pComposition)
        : CMilBrushDuce(pComposition)
    {
    }

public:

    override bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_GRADIENTBRUSH || CMilBrushDuce::IsOfType(type);
    }

#if DBG

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      RealizationMayNeedNonPow2Tiling
    //
    //  Synopsis:
    //      Returns whether the brush needs non-pow2 tiling. Now-pow2 tiling is
    //      not implemented in hardware text rendering, so text uses this query
    //      to determine if software should be used instead.
    //
    //  Notes:
    //      This DBG method is entirely unecessary. It exists so that I could
    //      comment why it doesn't need to exist.
    //
    //--------------------------------------------------------------------------

    override bool RealizationMayNeedNonPow2Tiling(
        __in_ecount(1) const BrushContext *pBrushContext
        ) const
    {
        //
        // Gradients are realized in two different ways.
        //  1. They are realized as an intermediate texture, in which case all
        //     "tiling" happens within the intermediate. Rendering the
        //     intermediate realization will not require tiling.
        //  2. They are realized to a Pow2 linear gradient texture
        // Either way, we don't need non-pow2 tiling.
        //

        bool fReturn = CMilBrushDuce::RealizationMayNeedNonPow2Tiling(
            pBrushContext
            );
        Assert(fReturn == false);

        return fReturn;
    }
#endif

protected:

    template <class T>
    static HRESULT GetGradientColorData(
        __in_ecount(1) T *pThis,
        __out_ecount(1) CGradientColorData *pColorData      // Realized gradient stops
        );

    template <class T>
    static bool IsConstantOpaqueInternal(
        __in_ecount(1) T *pThis
        );

    static HRESULT GetSolidColorRealization(
        __in_ecount(1) CGradientColorData *pGradientStops,
        __inout_ecount(1) CMILBrushSolid *pBrushRealization
        );
};

//+------------------------------------------------------------------------
//
//  Member:    
//      IsConstantOpaqueInternal
//
//  Synopsis:  
//      Returns true if this gradient brush is entirely opaque
//
//  Note:
//      Will be correct even if brush is dirty. This is necessary
//      because this is called before the render pass (in which the brush is
//      updated if it is dirty).
//
//-------------------------------------------------------------------------

template <class T>
bool 
CMilGradientBrushDuce::IsConstantOpaqueInternal(
    __in_ecount(1) T *pThis
    )
{
    HRESULT hr = S_OK;
    
    FLOAT rOpacity;

    MilGradientStop *pStop = pThis->m_data.m_pGradientStopsData;

    bool fIsConstantOpaque = false;

    IFC((GetOpacity(
        pThis->m_data.m_Opacity,
        pThis->m_data.m_pOpacityAnimation,
        &rOpacity
        )));

    // Check brush opacity
    if (rOpacity < 1.0f)
    {
        goto Cleanup;
    }

    UINT count = pThis->m_data.m_cbGradientStopsSize / sizeof(pStop[0]);

    for (UINT i = 0; i < count; i++)
    {
        // Check this stop for opacity
        if (pStop->Color.a < 1.0f)
        {
            goto Cleanup;
        }

        // Advance to next stop
        pStop++;
    }

    // This is the only path in which all opacity values were 1.0.
    fIsConstantOpaque = true;
    
Cleanup:
    // Deliberately ignore HRESULT failure. fIsConstantOpaque should
    // still be false in that case.
    return fIsConstantOpaque;
}

//+------------------------------------------------------------------------
//
//  Member:
//      GetGradientColorData
//
//  Synopsis:
//      Returns newly realized gradient color data, premultiplied with the
//      realized opacity.
//
//  Note:
//      Template parameter allows access to common gradient properties.
//

template <class T>
HRESULT 
CMilGradientBrushDuce::GetGradientColorData(
    __in_ecount(1) T *pThis,
    __out_ecount(1) CGradientColorData *pColorData      // Realized gradient stops
    )
{
    HRESULT hr = S_OK;

    FLOAT rOpacity;

    MilGradientStop *pStop = pThis->m_data.m_pGradientStopsData;

    pColorData->Clear();
 
    IFC(GetOpacity(
        pThis->m_data.m_Opacity,
        pThis->m_data.m_pOpacityAnimation,
        &rOpacity
        ));

     //
    // If processing the update packet or registering notifiers failed,
    // this count will be zero -- guaranteed by the marshaling code.
    //

    UINT count = pThis->m_data.m_cbGradientStopsSize / sizeof(pStop[0]);

    for(UINT i = 0; i < count; i++)
    {
        // Add stop        
        IFC(pColorData->AddColorWithPosition(
            reinterpret_cast<MilColorF *>(&pStop->Color),
            static_cast<FLOAT>(pStop->Position)
            ));
 
        // Advance to next stop
        pStop++;
    }

    // Apply opacity to all gradient stops
    IFC(pColorData->ApplyOpacity(rOpacity));
 
 Cleanup:

    RRETURN(hr);
}




