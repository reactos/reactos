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

#include "precomp.hpp"

MtDefine(CMilProjectionCameraDuce, CameraResource, "CMilProjectionCameraDuce");

//+-----------------------------------------------------------------------------
//
//  Function:
//      IntersectPlaneInterval
//
//  Synopsis:
//      Returns the intersection of [flNear1, flFar1] and [flNear2, flFar2] in
//      [flNearOut, flFarOut].
//
//      If the two intervals do not intersect, the "out" interval will be
//      flipped
//
//------------------------------------------------------------------------------

void IntersectPlaneInterval(
    float flNear1,
    float flFar1,
    float flNear2,
    float flFar2,
    __out_ecount(1) float *flNearOut,
    __out_ecount(1) float *flFarOut
    )
{   
    // The funny boolean logic is for NaNs
    if (!(flNear1 >= flNear2))
    {
        *flNearOut = flNear2;
    }
    else
    {
        *flNearOut = flNear1;
    }
    
    if (!(flFar1 <= flFar2))
    {
        *flFarOut = flFar2;
    }    
    else
    {
        *flFarOut = flFar1;
    }
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ApplyToContextState
//
//  Synopsis:
//      Shared implementation of ApplyToContextState used by both
//      OrthographicCamera and PerspectiveCamera.
//
//      If fRenderRequiredOut is false, pCtxState will NOT be modified
//
//------------------------------------------------------------------------------

HRESULT CMilProjectionCameraDuce::ApplyToContextState(
    __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
    const float flViewportWidth,
    const float flViewportHeight,
    float flNearPlane,
    float flFarPlane,
    const bool fUseComputedPlanes,
    float flComputedNearPlane,
    float flComputedFarPlane,
    __out_ecount(1) bool &fRenderRequiredOut
    ) const
{
    HRESULT hr = S_OK;
    fRenderRequiredOut = true;

    // We do not render the scene if:
    //
    //   1. Near > Far (the entire scene is clipped)
    //   2. Near == NaN || Far == NaN
    //
    // The logic below will handle both cases.
    if (!(flNearPlane <= flFarPlane))
    {
        fRenderRequiredOut = false;
        goto Cleanup;
    }

    // If we have computed planes use them to shrink the user's specified
    // span as much as possible without clipping any geometry that otherwise
    // would have been visible.
    //
    // NOTE: Before applying the computed planes we need to verify that the
    //       computed span is non-empty (near <= far) and not NaN.  This can
    //       happen if the scene is empty or degenerate.  If we fail this
    //       check we render with the user's specified planes.
    //
    if (fUseComputedPlanes && flComputedNearPlane <= flComputedFarPlane)
    { 
        float flAdjustedNearPlane, flAdjustedFarPlane;

        // Intersect the user's span with the computed span.  This shrinks
        // the user's span to to exclude empty space.  We do this before
        // EnsureClippingPlaneDistance so that we are expanding the
        // smallest possible interval.
        IntersectPlaneInterval(
            flNearPlane,
            flFarPlane,
            flComputedNearPlane,
            flComputedFarPlane,
            /* out */ &flAdjustedNearPlane,
            /* out */ &flAdjustedFarPlane
            );

        // It's possible that the intersection is empty (near > far) in
        // which case we early exit.  (Methods below expect non-empty spans.)
        if (flAdjustedNearPlane > flAdjustedFarPlane)
        {
            fRenderRequiredOut = false;
            goto Cleanup;
        }

        // In order to ensure we didn't inadvertently clip any geometry we
        // expand the adjusted span slighly to account for FP precision
        // differences with the Z-buffer.
        IFC(EnsureClippingPlaneDistance(
            /* ref */ flAdjustedNearPlane,
            /* ref */ flAdjustedFarPlane
            ));

        // Intersect again to make sure the expanded span did not go outside
        // of the user's specified near or far plane
        IntersectPlaneInterval(
            flNearPlane,
            flFarPlane,
            flAdjustedNearPlane,
            flAdjustedFarPlane,
            /* out */ &flAdjustedNearPlane,
            /* out */ &flAdjustedFarPlane
            );

        flNearPlane = flAdjustedNearPlane;
        flFarPlane = flAdjustedFarPlane;
    }
    
    Assert(flNearPlane <= flFarPlane);

    IFC(GetViewTransform(
        &(pCtxState->ViewTransform3D)
        ));
    
    IFC(GetProjectionTransform(
        GetAspectRatio(flViewportWidth, flViewportHeight),
        flNearPlane,
        flFarPlane,
        &(pCtxState->ProjectionTransform3D)
        ));

Cleanup:
    RRETURN(hr);
}

