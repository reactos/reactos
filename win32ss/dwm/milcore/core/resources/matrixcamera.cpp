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

MtDefine(MatrixCameraResource, MILRender, "MatrixCamera Resource");

MtDefine(CMilMatrixCameraDuce, MatrixCameraResource, "CMilMatrixCameraDuce");

CMilMatrixCameraDuce::~CMilMatrixCameraDuce()
{
    UnRegisterNotifiers();
}

/* override */ HRESULT CMilMatrixCameraDuce::ApplyToContextState(
    __inout_ecount(1) CContextState *pCtxState,     // Context state to modify
    const float flViewportWidth,
    const float flViewportHeight,
    const bool fUseComputedPlanes,
    const float flComputedNearPlane,
    const float flComputedFarPlane,
    bool &fRenderRequiredOut
    ) const
{
    HRESULT hr = S_OK;
    Assert(!fUseComputedPlanes);

    pCtxState->ProjectionTransform3D = m_data.m_projectionMatrix;

    // We use the accessor because we want both the Camera.Transform and
    // MatrixCamera.ViewTransform taken into account.
    IFC(GetViewTransform(
        /* out */ &(pCtxState->ViewTransform3D)
        ));
 
    fRenderRequiredOut = true;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilMatrixCameraDuce::GetViewTransform
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
//      camera's World-to-View transform here.  This is different
//      than the MatrixCamera.ViewMatrix property.
//
//------------------------------------------------------------------------------

override HRESULT CMilMatrixCameraDuce::GetViewTransform(
    __out_ecount(1) CMILMatrix *pViewMatrixOut) const
{
    HRESULT hr = S_OK;
    
    *pViewMatrixOut = m_data.m_viewMatrix;
    IFC(PrependInverseTransform(m_data.m_pTransform, pViewMatrixOut));

Cleanup:
    RRETURN(hr);
}

