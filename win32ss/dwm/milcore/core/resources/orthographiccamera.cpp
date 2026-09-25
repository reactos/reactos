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
#include <cfenv>

using namespace dxlayer;

MtDefine(OrthographicCameraResource, MILRender, "OrthographicCamera Resource");

MtDefine(CMilOrthographicCameraDuce, OrthographicCameraResource, "CMilOrthographicCameraDuce");

CMilOrthographicCameraDuce::~CMilOrthographicCameraDuce()
{
    UnRegisterNotifiers();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilOrthographicCameraDuce::GetProjectionTransform
//
//  Synopsis:
//      Gets the projection matrix for this orthographic
//      camera.
//      NOTE: Uses near and far plane values given in arguments
//      rather than the ones stored in the camera data.
//
//  Returns:
//      Success if the matrix was retrieved successfully
//
//      NOTE assumes that the camera data structure is already synchronized with
//      any camera animations.
//
//------------------------------------------------------------------------------
/* override */ HRESULT CMilOrthographicCameraDuce::GetProjectionTransform(
    const double aspectRatio,
    const float flNearPlaneDistance,
    const float flFarPlaneDistance,
    __out_ecount(1) CMILMatrix *pProjectionMatrixOut) const
{
    HRESULT hr = S_OK;
    Assert(pProjectionMatrixOut);

    double height = m_data.m_width/aspectRatio;

    *pProjectionMatrixOut =
        matrix::get_ortho_rh(
            static_cast<float>(m_data.m_width), 
            static_cast<float>(height), 
            flNearPlaneDistance, 
            flFarPlaneDistance);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilOrthographicCameraDuce::GetViewTransform
//
//  Synopsis:
//      Gets the view matrix.
//
//  Returns:
//      Success if the matrix was retrieved successfully
//
//      NOTE: Assumes that the camera data structure is already
//      synchronized with any camera animations.
//
//      NOTE: We consider the Camera.Transform to be part of the
//      camera's World-to-View transform here.
//
//------------------------------------------------------------------------------
/* override */ HRESULT CMilOrthographicCameraDuce::GetViewTransform(
    __out_ecount(1) CMILMatrix *pViewMatrixOut) const
{
        HRESULT hr = S_OK;
        Assert(pViewMatrixOut);
    
        const vector3& position = *reinterpret_cast<const basetypes<dx_apiset>::vector3_base_t*>(&m_data.m_position);
        const vector3& look_direction = *reinterpret_cast<const basetypes<dx_apiset>::vector3_base_t*>(&m_data.m_lookDirection);
        const vector3& up = *reinterpret_cast<const basetypes<dx_apiset>::vector3_base_t*>(&m_data.m_upDirection);
        
        vector3 lookAtPoint = position + look_direction;
        *pViewMatrixOut = matrix::get_lookat_rh(position, lookAtPoint, up);

    
        IFC(PrependInverseTransform(m_data.m_pTransform, pViewMatrixOut));
            
    Cleanup:
        RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilOrthographicCameraDuce::EnsureClippingPlaneDistance
//
//  Synopsis:
//      This method widens the given near and far planes to ensure that geometry
//      right on the clipping planes still renders. It also enforces a
//      numerically stable minimal distance between the planes to handle edge
//      cases like the scene being entirely in a plane (i.e., nearPlane ==
//      farPlane)
//
//------------------------------------------------------------------------------
/* override */ HRESULT CMilOrthographicCameraDuce::EnsureClippingPlaneDistance(
    __inout_ecount(1) float &flNearPlane,
    __inout_ecount(1) float &flFarPlane
    ) const
{
    // If the near plane is farther than the far plane we consider the entire scene
    // to be clipped.  ApplyToContextState should have early exited.
    Assert(flNearPlane <= flFarPlane);
    
    // We need to do two adjustments to the scene depth
    // span before we can use it.  
    
    // 1. We need to widen it if it is too small (like the
    //    scene is at one depth.) Too small will cause
    //    the camera matrix to overflow and step 2 to
    //    fail.

    const float gamma = FLT_EPSILON * (fabs(flFarPlane) + fabs(flNearPlane));

    if (fabs(flFarPlane - flNearPlane) < 2 * gamma)
    {
        flNearPlane -= gamma;
        flFarPlane += gamma;
    }
    
    // 2. We need to widen it (regardless of size) so that
    //    geometry EXACTLY at the near and far renders.
    //    This step is different for PerspectiveCamera vs
    //    OrthographicCamera

    // First we compute the delta required to expand the planes so
    // they are at least FLT_EPSILON away from the geometry in
    // float percision on our CPU.  Because FLT_EPSILON is computed
    // at 1.0f we need to scale this by the magnitude of the near or
    // far plane, whichever is larger.  (Note that we want the
    // larger magnitude, not the magnitude of the larger value.)
    const float fpDelta = FLT_EPSILON * max(fabs(flNearPlane), fabs(flFarPlane));

    // Next we compute the delta required to expand the planes so
    // that geometry is projected to be at least FIXED_24_EPSILON
    // inside the 0..1 range in the 24-bit fixed point Z-Buffer.
    const float fixDelta = (flFarPlane - flNearPlane) * FIXED_24_EPSILON / (1 - 2 * FIXED_24_EPSILON);

    // We then use the larger of the deltas to extend our planes.
    //
    // NOTE: flNearPlane may end up slightly negative but that is fine in
    // an orthographic projection and it'll produce more predictable results
    // for items on the same plane as the camera position.

    const float delta = max(fpDelta, fixDelta);    
    flNearPlane -= delta;
    flFarPlane  += delta;

#if defined(DIRECTXMATH)
    // DirectXMath library requires that the distance between near and far 
    // planes be at least 0.00001f. 
    const float dxmath_epsilon = 0.00001f;
    if (std::abs(flFarPlane - flNearPlane) <= dxmath_epsilon)
    {
        // This is the value by which we'd want to advance the 'mid' point 
        // in either direction to ensure that the condition 
        // | flFarPlane - flNearPlane | <= 0.00001f is satisfied.
        const float dxmath_delta =
            0.000005f + std::numeric_limits<float>::epsilon();

        static_assert(std::numeric_limits<float>::is_iec559, 
            "floating point assumptions here depend on conformance with the IEC 559 standard");

        // Calculate the next representable floating point value in each direction by
        // calling into std::nextafter.
        //
        // From the 'next' value,  calculate the 'gap size'.This 'gap size' represents the 
        // minimum noticeable floating-point change that can be made in either direction. Trivially, 
        // std::numeric_limits<float>::epsilon() would be the 'gap size' for values in 
        // the range [1.0f, 2.0f), and this gap-size for a given range would grow (exponentially)
        // with the magnitude of the values bracketing that range.

        // First, ensure that the values are not +/- infinity
        // This will ensure that we do not have to deal with overflow/underflow
        // conditions
        flFarPlane = 
            ClampValue<float>(flFarPlane, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());
        flNearPlane =
            ClampValue<float>(flNearPlane, std::numeric_limits<float>::lowest(), std::numeric_limits<float>::max());

        float mid = (flFarPlane / 2.0f + flNearPlane / 2.0f);
        float next_value_after_mid = std::nextafter(mid, std::numeric_limits<float>::max());
        float prev_value_before_mid = std::nextafter(mid, std::numeric_limits<float>::lowest());

        // if the 'gap size' is larger than our preferred delta (dxmath_delta), then 
        // use the 'next' value obtained from std::nextafter to widen the distance between 
        // the near and the far planes.Otherwise, use dxmath_delta to widen that distance.
        //
        // IF (dxmath_delta <= 'gap size')
        //      * dxmath_delta is too small to be perceptible in add/subtract *
        //      * operations. use the nextafter value * 
        //      SET near/far plane = nextafter value
        // ELSE
        //      * dxmath_delta is sufficiently large to be perceptible in *
        //      * add/subtract operations *
        //      SET near/far plane = near/far plane +/- dxmath_delta
        // ENDIF
        //
        // This can be implemented in a simplified manner as follows:

        flFarPlane = std::max(mid + dxmath_delta, next_value_after_mid);
        flNearPlane = std::min(mid - dxmath_delta, prev_value_before_mid);

        assert(std::abs(flFarPlane - flNearPlane) > dxmath_epsilon);
    }
#endif

    RRETURN(S_OK);
}

/* override */ HRESULT CMilOrthographicCameraDuce::ApplyToContextState(
    __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
    const float flViewportWidth,
    const float flViewportHeight,
    const bool fUseComputedPlanes,
    const float flComputedNearPlane,
    const float flComputedFarPlane,
    __out_ecount(1) bool &fRenderRequiredOut
    ) const
{
    RRETURN(CMilProjectionCameraDuce::ApplyToContextState(
        pCtxState,
        flViewportWidth,
        flViewportHeight,
        static_cast<float>(m_data.m_nearPlaneDistance),
        static_cast<float>(m_data.m_farPlaneDistance),
        fUseComputedPlanes,
        flComputedNearPlane,
        flComputedFarPlane,
        /* out */ fRenderRequiredOut
        ));
}

