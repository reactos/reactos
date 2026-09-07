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
using namespace dxlayer;

MtDefine(PerspectiveCameraResource, MILRender, "PerspectiveCamera Resource");

MtDefine(CMilPerspectiveCameraDuce, PerspectiveCameraResource, "CMilPerspectiveCameraDuce");

CMilPerspectiveCameraDuce::~CMilPerspectiveCameraDuce()
{
    UnRegisterNotifiers();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilPerspectiveCameraDuce::GetProjectionTransform
//
//  Synopsis:
//      Gets the projection matrix for this perspective camera.
//      NOTE: Uses near and far plane values given in arguments
//      rather than the ones stored in the camera data.
//
//  Returns:
//      Success if the matrix was successfully retrieved.
//
//      NOTE assumes that the camera data structure is already synchronized with
//      any camera animations.
//
//------------------------------------------------------------------------------
/* override */ HRESULT CMilPerspectiveCameraDuce::GetProjectionTransform(
    const double aspectRatio,
    const float flNearPlaneDistance,
    const float flFarPlaneDistance,
    __out_ecount(1) CMILMatrix *pProjectionMatrixOut) const
{
    HRESULT hr = S_OK;
    Assert(pProjectionMatrixOut);

    // We set up the matrix ourselves rather than use D3DMatrixPerspectiveFovRH
    // because our FoV is horizontal rather than vertical and there are some
    // simplifications we can take advantage of.

    double hFovRad = (m_data.m_fieldOfView / 180.0f)* math_extensions::get_pi();
    double halfWidthDepthRatio = tan(hFovRad/2);

    float m11 = static_cast<float>(1/halfWidthDepthRatio);
    float m22 = static_cast<float>(aspectRatio/halfWidthDepthRatio);
    float m33 = flFarPlaneDistance/(flNearPlaneDistance - flFarPlaneDistance);

    pProjectionMatrixOut->_11 = m11;
    pProjectionMatrixOut->_12 = 0.0f;
    pProjectionMatrixOut->_13 = 0.0f;
    pProjectionMatrixOut->_14 = 0.0f;

    pProjectionMatrixOut->_21 = 0.0f;
    pProjectionMatrixOut->_22 = m22;
    pProjectionMatrixOut->_23 = 0.0f;
    pProjectionMatrixOut->_24 = 0.0f;

    pProjectionMatrixOut->_31 = 0.0f;
    pProjectionMatrixOut->_32 = 0.0f;
    pProjectionMatrixOut->_33 = m33;
    pProjectionMatrixOut->_34 = -1.0f;

    pProjectionMatrixOut->_41 = 0.0f;
    pProjectionMatrixOut->_42 = 0.0f;
    pProjectionMatrixOut->_43 = m33*flNearPlaneDistance;
    pProjectionMatrixOut->_44 = 0.0f;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilPerspectiveCameraDuce::GetViewTransform
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

/* override */ HRESULT CMilPerspectiveCameraDuce::GetViewTransform(
    __out_ecount(1) CMILMatrix *pViewMatrixOut) const
{
    HRESULT hr = S_OK;
    Assert(pViewMatrixOut);

    const vector3& eye = *reinterpret_cast<const basetypes<dx_apiset>::vector3_base_t*>(&m_data.m_position);
    const vector3& look_direction = *reinterpret_cast<const basetypes<dx_apiset>::vector3_base_t*>(&m_data.m_lookDirection);
    const vector3& up = *reinterpret_cast<const basetypes<dx_apiset>::vector3_base_t*>(&m_data.m_upDirection);
    
    const vector3& at = eye + look_direction;
    *pViewMatrixOut = matrix::get_lookat_rh(eye, at, up);

    IFC(PrependInverseTransform(m_data.m_pTransform, pViewMatrixOut));
        
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilPerspectiveCameraDuce::EnsureClippingPlaneDistance
//
//  Synopsis:
//      This method widens the given near and far planes to ensure that geometry
//      right on the clipping planes still renders. It also enforces a
//      numerically stable minimal distance between the planes to handle edge
//      cases like the scene being entirely in a plane (i.e., nearPlane ==
//      farPlane)
//
//------------------------------------------------------------------------------

/* override */ HRESULT CMilPerspectiveCameraDuce::EnsureClippingPlaneDistance(
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
    //    fail.  Ensuring that the far plane is at least
    //    4x the near plane is fine, there's no reason for
    //    them to be closer.
    EnsureMinIntervalRatio(flNearPlane, flFarPlane, 2);

    // 2. We need to widen it (regardless of size) so that
    //    geometry EXACTLY at the near and far renders.
    //    This step is different for PerspectiveCamera vs
    //    OrthographicCamera

    // Steve Hollasch derives the right near and far planes
    // to make the scene near and far planes map to epsilon
    // and 1-epsilon in the z buffer in "Setting Z-Buffer
    // Bounds Automatically"
    //
    // http://research.microsoft.com/~hollasch/cgindex/render/zbound.html

    // Since we have 24 bits of z buffer we use FIXED_24_EPSILON
    // (4 / 2^24 = 2^-22) which gives us 3 slop values on either
    // side of where the the scene values should end up using
    // infinite precision.

    float numerator = flNearPlane * flFarPlane * (2 * FIXED_24_EPSILON - 1);
    float k = FIXED_24_EPSILON * (flNearPlane + flFarPlane);

    float flOriginalNearPlane = flNearPlane;
    float flOriginalFarPlane = flFarPlane;

    flNearPlane = numerator / (k - flFarPlane);
    flFarPlane  = numerator / (k - flOriginalNearPlane);

    // Union the old and the new to make sure that we have in fact expanded
    // the planes. For an example of contraction, if flNearPlane is 0 then
    // flFarPlane becomes 0 too. The funny boolean logic is so that we
    // overwrite flNear/FarPlane if they are NaNs.

    if (!(flOriginalNearPlane >= flNearPlane))
    {
        flNearPlane = flOriginalNearPlane;
    }

    if (!(flOriginalFarPlane <= flFarPlane))
    {
        flFarPlane = flOriginalFarPlane;
    }

    RRETURN(S_OK);
}

/* override */ HRESULT CMilPerspectiveCameraDuce::ApplyToContextState(
    __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
    const float flViewportWidth,
    const float flViewportHeight,
    const bool fUseComputedPlanes,
    const float flComputedNearPlane,
    const float flComputedFarPlane,
    __out_ecount(1) bool &fRenderRequiredOut
    ) const
{
    HRESULT hr = S_OK;
    float nearPlane = static_cast<float>(m_data.m_nearPlaneDistance);

    if (nearPlane < 0)
    {
        // We do not render perspective cameras with negative near planes.
        fRenderRequiredOut = false;
        goto Cleanup;
    }
    
    IFC(CMilProjectionCameraDuce::ApplyToContextState(
        pCtxState,
        flViewportWidth,
        flViewportHeight,
        nearPlane,
        static_cast<float>(m_data.m_farPlaneDistance),
        fUseComputedPlanes,
        flComputedNearPlane,
        flComputedFarPlane,
        /* out */ fRenderRequiredOut
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      EnsureMinIntervalRatio
//
//  Synopsis:
//      Ensures that the ratio between the start and end of a non-negative
//      interval meets a specified minimum.  If necessary, the interval is
//      expanded on both sides.
//
//------------------------------------------------------------------------------

void CMilPerspectiveCameraDuce::EnsureMinIntervalRatio(
    __inout_ecount(1) float &flStart,
    __inout_ecount(1) float &flEnd,
    float ratioRoot // Square root of ratio to ensure between top and bottom of the interval.
                    // i.e. flEnd > flStart * ratioRoot * ratioRoot
    )
{
    Assert(0 <= flStart && flStart <= flEnd);
    Assert(ratioRoot > 1);

    if (flEnd == 0)
    {
        flEnd = 1;            // Special case for interval [0,0]
    }
    else if (flEnd < flStart * ratioRoot * ratioRoot)
    {
        // Calculate geometric mean
        float gmean = static_cast<float>(sqrt(flStart * flEnd));
        flStart = gmean / ratioRoot;
        flEnd   = gmean * ratioRoot;
    }

    Assert(0 <= flStart && flStart < flEnd);
}

