// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains declarations for generic render utility routines.
//
//------------------------------------------------------------------------------

#pragma once

void
CalcHomogeneousClipTo2D(
    __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcViewport,
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> &matLocalTo,
    __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::Projection3D> &matProjection
    );

bool
ClipLine4D(
    const DWORD nClipPlanes,
    __in_ecount(1) dxlayer::vector4 * p0,
    __in_ecount(nClipPlanes) const double bc0[],
    const DWORD outCode0,
    __in_ecount(1) dxlayer::vector4 * p1,
    __in_ecount(nClipPlanes) const double bc1[],
    const DWORD outCode1);

HRESULT
ApplyProjectedMeshTo2DState(
    __in_ecount(1) const CContextState* pContextState,
    __in_ecount(1) CMILMesh3D *pMesh3D,
    __in_ecount(1) const CMILSurfaceRect &rcClip,
    __out_ecount(1) CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> *pmatBrushSpaceToSampleSpace,
    __out_ecount_opt(1) CMILSurfaceRect *prcRenderBoundsDeviceSpace,
    __out_ecount(1) bool *pfMeshVisible,
    __out_ecount(1) CRectF<CoordinateSpace::BaseSampling> *prcTextureCoordinateBoundsInBrushSpace
    );

void 
CombineContextState3DTransforms(
    __in_ecount(1) const CContextState* pContextState,
    __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::Local3D> *pCombined3DTransform
    );

void
Calc2DBoundsAndIdealSamplingEstimates(
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::Local3D> &matFullTransform3D,
    __in_ecount(1) const CMilPointAndSize3F* pMesh3DBox,
    __in_ecount(1) const CRectF<CoordinateSpace::BaseSampling>* prcBrushSampleBounds,
        // Texture coordinates are in world sampling space
    __out_ecount(1) CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> *pmatBrushSpaceToIdealSampleSpace,
    __out_ecount_opt(1) CMultiSpaceRectF<CoordinateSpace::PageInPixels,CoordinateSpace::Device> *prcMeshBoundsTargetSpace
    );

template <typename OutCoordSpace>
void
CalcProjectedBounds(
    __in_ecount(1) const CMatrix<CoordinateSpace::Local3D,OutCoordSpace> &matFullTransform3D,
    __in_ecount(1) const CMilPointAndSize3F *pboxBounds,
    __out_ecount(1) CRectF<OutCoordSpace> *prcTargetRect
    );

DWORD
ComputeOutCode(
    __in_ecount(nClipPlanes) const double bc[],
    const DWORD nClipPlanes);

bool
IsUniformNonZeroVec3(
    __in_ecount(1) const dxlayer::vector3 *vec3V
    );

bool
IsFiniteVec3(
    __in_ecount(1) const dxlayer::vector3 *vec3V
    );

float DegToRadF(double angleInDeg);


