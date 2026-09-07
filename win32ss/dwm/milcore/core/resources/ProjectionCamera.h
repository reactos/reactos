// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_camera
//      $Keywords:
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CMilProjectionCameraDuce);

// Class: CMilProjectionCameraDuce
class CMilProjectionCameraDuce : public CMilCameraDuce
{
    friend class CResourceFactory;

protected:

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CMilProjectionCameraDuce));

    CMilProjectionCameraDuce(__in_ecount(1) CComposition* pComposition)
        : CMilCameraDuce(pComposition) {}

public:
    override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_PROJECTIONCAMERA || CMilCameraDuce::IsOfType(type);
    }

    override virtual bool ShouldComputeClipPlanes() const
    {
        // Future Consideration:  - Consider gating computation on Near/Far ratio
        //
        // Per spec, the near/far span specified by the users determines the
        // visibility limits, but we reserve the right to shrink this span if
        // we can do so without clipping additional content to improve
        // Z-Buffer precision.
        //
        // Our current implementation always shrinks if possible since we
        // expect the cost to be negligible, however we could gate this
        // behavior on the ratio of the near/far planes in the future.
        
        return true;
    }
    
    virtual HRESULT GetProjectionTransform(
        const double aspectRatio,
        const float flNearPlaneDistance,
        const float flFarPlaneDistance,
        __out_ecount(1) CMILMatrix *pProjectionMatrixOut
        ) const = 0;
    
protected:   
    virtual HRESULT EnsureClippingPlaneDistance(
        __inout_ecount(1) float &flNearPlane,
        __inout_ecount(1) float &flFarPlane
        ) const = 0;

    HRESULT ApplyToContextState(
        __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
        const float flViewportWidth,
        const float flViewportHeight,
        float flNearPlane,
        float flFarPlane,
        const bool fUseComputedPlanes,
        float flComputedNearPlane,
        float flComputedFarPlane,
        __out_ecount(1) bool &fRenderRequiredOut
        ) const;

    static double GetAspectRatio(double width, double height)
    { 
        return width/height;
    }
};


