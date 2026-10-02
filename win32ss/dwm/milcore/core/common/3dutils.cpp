// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains generic render utility routines.
//

#include "precomp.hpp"
#include <vector>
using namespace dxlayer;

//+------------------------------------------------------------------------
//
//  Function:  CalcHomogeneousClipTo2D
//
//  Synopsis:  Calculates the transform to from a projected homogeneous clip
//          space to 2D Device Space given a viewport and a WorldToDevice
//          for THAT viewport.  The use for this is to calculate a transform
//          that can be applied to a 3D Projection Transform to put the scene
//          into a viewport affected by the WorldToDevice transform.
//
//-------------------------------------------------------------------------
void CalcHomogeneousClipTo2D(
    __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcViewport,
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> &matLocalTo,
    __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::Projection3D> &matProjection
    )
{
    const CRectF<CoordinateSpace::HomogeneousClipping> rcHomogeneousClip(-1.0f, 1.0f, 1.0f, -1.0f, LTRB_Parameters);

    CMatrix<CoordinateSpace::HomogeneousClipping,
            CoordinateSpace::LocalRendering> matHomogeneousClipToLocalViewport;

    //
    // We need to go from homogeneous clipping space to World Space of the rectangle
    // passed to us.  From there we need to apply the WorldToDevice transform to take
    // us into device space.
    //
    // The homogeneous clipping space that we have from the ProjectionTransform is this:
    //
    //                 ^ y = 1.0
    //                 |
    //                 |
    //                 |
    // x = -1.0 <------O------> x = 1.0
    //                 |
    //                 |
    //                 |
    //                 v y = -1.0
    //
    //
    // We need to take this to local space of the rectangle, which has the y-axis inverted...
    //
    //
    //                 ^ y-
    //                 |
    //           x- <--o--> x+
    //                 |
    //                 v y+
    //

    matHomogeneousClipToLocalViewport.InferAffineMatrix(
        rcHomogeneousClip,
        rcViewport
        );

    matProjection.SetToMultiplyResult(
        matHomogeneousClipToLocalViewport,
        matLocalTo
        );

    // We assume that this transform which is used for ViewportProjectionModifier3D
    // will leave Z and W unchanged (i.e., the 3rd and 4th columns are identity or
    // NaN in degenerate cases like an empty viewport which will not render.)
    // 
    // This assumption allows CalcProjectedBounds to clip to the camera near and far
    // planes in device space rather then going through an intermediate clip space.
    
    Assert(!(matProjection._13 < 0 || matProjection._13 > 0));  // _13 == 0 or NaN
    Assert(!(matProjection._23 < 0 || matProjection._23 > 0));  // _23 == 0 or NaN
    Assert(!(matProjection._33 < 1 || matProjection._33 > 1));  // _33 == 1 or NaN
    Assert(!(matProjection._43 < 0 || matProjection._43 > 0));  // _43 == 0 or NaN
    Assert(!(matProjection._14 < 0 || matProjection._14 > 0));  // _14 == 0 or NaN
    Assert(!(matProjection._24 < 0 || matProjection._24 > 0));  // _24 == 0 or NaN
    Assert(!(matProjection._34 < 0 || matProjection._34 > 0));  // _34 == 0 or NaN
    Assert(!(matProjection._44 < 1 || matProjection._44 > 1));  // _44 == 1 or NaN
}


//+------------------------------------------------------------------------
//
//  Function:  ComputeOutCode
//
//  Synopsis:  Utility function to compute the outCodes required by
//             ClipLine4D.  The outCode is a bitarray represented as a
//             DWORD where the ith bit is set if bc[i] is < 0 (which we
//             standardize to mean that the point is on the non-visibile
//             side of clipping plane[i]).
//
//             See also ClipLine4D.
//
//-------------------------------------------------------------------------

inline DWORD ComputeOutCode(
    __in_ecount(nClipPlanes) const double bc[],
    const DWORD nClipPlanes)
{
    Assert(bc);
    Assert(nClipPlanes <= (sizeof(DWORD)*8));

    DWORD outCode = 0;
    DWORD mask = 1;

    for (DWORD i = 0; i < nClipPlanes; i++)
    {
        if (bc[i] < 0)
        {
            outCode |= mask;
        }

        mask <<= 1;
    }

    return outCode;
}

//+------------------------------------------------------------------------
//
//  Function:  ClipLine4D
//
//  Synopsis:  Clips the end points of the line defined by p0 and p1
//             against any number of arbitrary planes.  Rather than pass
//             an array of planes and having ClipLine4D do the dot
//             product to determine visibility the user instead supplies
//             an array of doubles which are the dot product between the
//             point and the oriented plane such that a positive value
//             means the point is on the visibile side of the plane.
//             Blinn refers to these as the "Boundary Coordinates".
//
//             The advantages to having the user supply the boundary
//             coordinates rather than the planes are:
//
//             1) The math for the dot product for common clipping
//                planes in homogeneous coordinates is simpler than the
//                general dot product.  (i.e., a single add/sub)
//
//             2) If the same point is shared between line segments
//                there is no reason to recompute the dot product or
//                outCode (discussed below.)
//
//             Common choices for clipping planes in homogeneous
//             coordinates:
//
//                  Plane        Boundary Coordinate
//                  --------     ---------------------
//                  X = -1       w + x
//                  X =  1       w - x
//                  Y = -1       w + y
//                  Y =  1       w - y
//                  Z =  0         z
//                  Z =  1       w - z
//
//             If you are clipping against the near and far Z planes
//             you would pass the following BC arrays:
//
//                  bc0 = { p0.z, p0.w - p0.z };
//                  bc1 = { p1.z, p1.w - p0.z };
//
//             The user also supplies an outCode for p0 and p1.  The
//             outCode is just a bitarray where bit 1 is set if the point
//             is on the non-visible side of the 1st clip plane, the
//             2nd bit set if it is on the non-visible side of the
//             2nd clip plane, and so on.  The ComputeOutCode() helper
//             function computes these outCodes from the BC arrays for
//             you.
//
//             Ref: Jim Blinn's Corner: Line Clipping,
//                  IEEE Computer Graphics & Applications, 1991, Jan.
//                  p.98 - 105
//
//                  Clipping Using Homogeneous Coordinates,
//                  James F. Blinn and Martin E. Newell
//
//-------------------------------------------------------------------------

bool ClipLine4D(
    const DWORD nClipPlanes,
    __in_ecount(1) vector4 * p0,
    __in_ecount(nClipPlanes) const double bc0[],
    const DWORD outCode0,
    __in_ecount(1) vector4 * p1,
    __in_ecount(nClipPlanes) const double bc1[],
    const DWORD outCode1)
{
    // Caller is required to put us into double precision mode prior to calling
    // ClipLine4D.
    CDoubleFPU::AssertMode();

    Assert(nClipPlanes > 0);
    Assert(p0);
    Assert(bc0);
    Assert(p1);
    Assert(bc1);

    // ANDing the outCodes returns a clipCode where any bit set indicates
    // that the end points are both on the non-visibile side of the given
    // clipping plane.  If both end points are on the non-visible side, the
    // line is not visible.  (trivial reject case)
    if ((outCode0 & outCode1) != 0) return false;

    // ORing the outCodes returns a clipCode where any bit set indicates
    // that the end points of the line are straddling the given clipping plane.
    // (We need to clip one or both ends of the line in the given plane.)
    DWORD clipCode = outCode0 | outCode1;

    // If no bit is set then both end points are on the visibile side of all
    // clipping planes and the line is entirely visible.  (trivial accept case)
    if (clipCode == 0) return true;

    DWORD mask = 1;
    double alpha0 = 0;      // Time at which the line enters the visibile space
    double alpha1 = 1;      // Time at which the line exits the visible space

    for (DWORD i = 0; i < nClipPlanes; i++)
    {
        if ((clipCode & mask) != 0)
        {
            // Compute the time at which line intersects this plane (alpha).
            double alpha = bc0[i] / (bc0[i] - bc1[i]);

            // I think Blinn does this outCode check before comparing alphas
            // to avoid a float operation -- I left it in because the outCodes
            // we use are computed using only one floating point operation
            // and theoretically have less error than the hit time calculation.
            //
            if ((outCode0 & mask) != 0)
            {
                if (alpha > alpha0)
                {
                    alpha0 = alpha;
                }
            }
            else
            {
                if (alpha < alpha1)
                {
                    alpha1 = alpha;
                }
            }

            // Non-trivial reject case
            if (alpha1 < alpha0) return false;
        }

        mask <<= 1;
    }

    // Sanity check that the ends of the line that the outCodes said
    // needed to be clipped in fact will be at least within rounding
    // error.  The comparison to -FLT_EPSILON is over-generous.
    // Case 1: outCode != 0 --> point was outside at least one halfspace and
    //         should have been clipped
    Assert((outCode0 == 0) || (alpha0 > -FLT_EPSILON));
    Assert((outCode1 == 0) || (alpha1 < 1+FLT_EPSILON+FLT_EPSILON));
    // Case 2: outCode == 0 --> point was inside all half spaces and
    //         shouldn't have been clipped
    Assert((outCode0 != 0) || (alpha0 == 0));
    Assert((outCode1 != 0) || (alpha1 == 1));

    // We need a local copy of both end points in the event that
    // both ends need to be trimmed.
    double x0 = p0->x;
    double y0 = p0->y;
    double z0 = p0->z;
    double w0 = p0->w;

    double x1 = p1->x;
    double y1 = p1->y;
    double z1 = p1->z;
    double w1 = p1->w;

    // Use the hit times (alpha) computed above to trim the
    // ends of the line as needed.
    //
    // We depart from Blinn's implementation by using the affine
    // combinations to reduce floating point error.

    if (outCode0 != 0)
    {
        p0->x = (float) ((1 - alpha0)*x0 + alpha0*x1);
        p0->y = (float) ((1 - alpha0)*y0 + alpha0*y1);
        p0->z = (float) ((1 - alpha0)*z0 + alpha0*z1);
        p0->w = (float) ((1 - alpha0)*w0 + alpha0*w1);
    }

    if (outCode1 != 0)
    {
        p1->x = (float) ((1 - alpha1)*x0 + alpha1*x1);
        p1->y = (float) ((1 - alpha1)*y0 + alpha1*y1);
        p1->z = (float) ((1 - alpha1)*z0 + alpha1*z1);
        p1->w = (float) ((1 - alpha1)*w0 + alpha1*w1);
    }

    return true;
}

//+------------------------------------------------------------------------
//
//  Function:  BoundPointHelper
//
//  Synopsis:  This is a helper for CalcProjectedBounds.  This is
//             essentially the inner loop of D3DXCalculateBoundingBox with
//             some extra coercing to deal with D3DXVECTOR4 and the fact
//             that we do not have a known "first" point.  This method
//             does the following:
//
//             1.  Check our point for a W of zero in which case we return
//                 maximum bounds.
//
//             2.  Project the 4D point into affine space.
//
//             3.  If fFirstPoint is true, initialize vecMin and vecMax
//                 with the first point, change the fFirstPoint flag to
//                 false, and exit with S_OK.
//
//             4.  Otherwise update vecMin/vecMax as appropriate
//                 and return with S_OK.
//-------------------------------------------------------------------------

inline static void
BoundPointHelper(
    __in_ecount(1) const vector4& vec4Point,
    __inout_ecount(1) vector3* pvecMin,
    __inout_ecount(1) vector3 * pvecMax,
    __inout_ecount(1) bool * pfFirstPoint)
{
    Assert(pvecMin);
    Assert(pvecMax);
    Assert(pfFirstPoint);

    Assert(*pfFirstPoint || pvecMin->x <= pvecMax->x);
    Assert(*pfFirstPoint || pvecMin->y <= pvecMax->y);
    Assert(*pfFirstPoint || pvecMin->z <= pvecMax->z);

    if (vec4Point.w == 0)
    {
        // Use half FLT_MAX so that dimensions of box fit into FLT_MAX
        // Boxes are stored as minimum and size and we don't want the
        // size to overflow.
        pvecMin->x = -FLT_MAX/2;
        pvecMin->y = -FLT_MAX/2;
        pvecMin->z = -FLT_MAX/2;
        pvecMax->x = +FLT_MAX/2;
        pvecMax->y = +FLT_MAX/2;
        pvecMax->z = +FLT_MAX/2;
        *pfFirstPoint = false;
    }
    else
    {
        auto vecCur  = vector3(
            vec4Point.x / vec4Point.w,
            vec4Point.y / vec4Point.w,
            vec4Point.z / vec4Point.w
            );

        if (*pfFirstPoint)
        {
            *pvecMin = *pvecMax = vecCur;
            *pfFirstPoint = false;
        }
        else
        {
            if (vecCur.x < pvecMin->x)
            {
                pvecMin->x = vecCur.x;
            }
            else if (vecCur.x > pvecMax->x)
            {
                pvecMax->x = vecCur.x;
            }

            if (vecCur.y < pvecMin->y)
            {
                pvecMin->y = vecCur.y;
            }
            else if (vecCur.y > pvecMax->y)
            {
                pvecMax->y = vecCur.y;
            }

            if (vecCur.z < pvecMin->z)
            {
                pvecMin->z = vecCur.z;
            }
            else if (vecCur.z > pvecMax->z)
            {
                pvecMax->z = vecCur.z;
            }
        }

        Assert(!(*pfFirstPoint));
        Assert(pvecMin->x <= vecCur.x && vecCur.x <= pvecMax->x);
        Assert(pvecMin->y <= vecCur.y && vecCur.y <= pvecMax->y);
        Assert(pvecMin->z <= vecCur.z && vecCur.z <= pvecMax->z);
    }
}

//+------------------------------------------------------------------------
//
//  Function:  ApplyProjectedMeshTo2DState
//
//  Synopsis:  Projects a 3D mesh's bounds into 2D, then uses them
//             to reduce the current clipping region and calculate
//             a scale transform for realizing 2D brush content.
//
//-------------------------------------------------------------------------
HRESULT
ApplyProjectedMeshTo2DState(
    __in_ecount(1) const CContextState* pContextState,
        // Context state containing 3D transforms
    __in_ecount(1) CMILMesh3D *pMesh3D,
        // Mesh to project & apply to 2D state
    __in_ecount(1) const CMILSurfaceRect &rcClip,
        // Clip to intersect with the projected mesh bounds
    __out_ecount(1) CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> *pmatBrushSpaceToIdealSampleSpace,
        // Brush->sample space transform
    __out_ecount_opt(1) CMILSurfaceRect *prcRenderBoundsDeviceSpace,
        // Projected & clipped mesh bounds
    __out_ecount(1) bool *pfMeshVisible,
        // Whether or not the mesh is visible.  If 'false', it lies outside of rcClip.
    __out_ecount(1) CRectF<CoordinateSpace::BaseSampling> *prcBrushSamplingBounds
        // Brush sampling bounds of pMesh3D
    )
    
{
    HRESULT hr = S_OK;

    CMultiSpaceRectF<CoordinateSpace::PageInPixels,CoordinateSpace::Device> rcMeshBoundsTargetSpace;
    CMilPointAndSize3F meshBoundingBox3D;
    CMultiOutSpaceMatrix<CoordinateSpace::Local3D> full3DTransform;
    CMILSurfaceRect rcRenderBoundsPageSpace;

    //
    // Obtain the combined 3D transform, 3D mesh bounds, & texture coordinate bounds
    //

    CombineContextState3DTransforms(
        pContextState,
        &full3DTransform
        );

    IFC(pMesh3D->GetBounds(&meshBoundingBox3D));

    // Mesh texture coordinates are brush coordinates
    IFC(pMesh3D->GetTextureCoordinateBounds(
        prcBrushSamplingBounds  // Delegate setting of out-param
        ));

    //
    // Compute the 3D brush transform & bounds from the trasformed mesh bounds
    //
    
    Calc2DBoundsAndIdealSamplingEstimates(
        IN full3DTransform,
        IN &meshBoundingBox3D,
        IN prcBrushSamplingBounds,
        OUT pmatBrushSpaceToIdealSampleSpace, // Delegate setting of out-param
        OUT &rcMeshBoundsTargetSpace
        );

    //
    // Intersect the Mesh bounds with the clip rectangle
    //

    *pfMeshVisible = IntersectBoundsRectFWithSurfaceRect(
        MilAntiAliasMode::EightByEight, // Since anti-aliased is a superset of aliased, we can use it for both
        prcRenderBoundsDeviceSpace ? rcMeshBoundsTargetSpace.Device() : ReinterpretPageInPixelsAsDevice(rcMeshBoundsTargetSpace.PageInPixels()),
        rcClip,
        prcRenderBoundsDeviceSpace ? prcRenderBoundsDeviceSpace : &rcRenderBoundsPageSpace // Delegate setting of out-param
        );

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CombineContextState3DTransforms
//
//  Synopsis:  Computes the full model to page 3D transform for the 
//             specified context state, i.e.
// 
//             result =  WorldTransform3D 
//                     * ViewTransform3D 
//                     * ProjectionTransform3D 
//                     * ViewportProjectionModifier3D
//-------------------------------------------------------------------------

void 
CombineContextState3DTransforms(
    __in_ecount(1) const CContextState *pContextState,
    __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::Local3D> *pCombined3DTransform
    )
{
    typedef CMatrix<CoordinateSpace::Local3D,CoordinateSpace::World3D> LocalToWorld3D_t;
    typedef CMatrix<CoordinateSpace::World3D,CoordinateSpace::View3D> WorldToView_t;
    typedef CMatrix<CoordinateSpace::View3D,CoordinateSpace::Projection3D> ViewToProjection_t;

    pCombined3DTransform->SetToMultiplyResult(
        reinterpret_cast<const LocalToWorld3D_t &>(pContextState->WorldTransform3D),
        reinterpret_cast<const WorldToView_t &>(pContextState->ViewTransform3D)
        );

    pCombined3DTransform->AppendMultiply(
        reinterpret_cast<const ViewToProjection_t &>(pContextState->ProjectionTransform3D)
        );

    pCombined3DTransform->AppendMultiply(
        pContextState->ViewportProjectionModifier3D
        );
}


//+------------------------------------------------------------------------
//
//  Function:  CalcProjectedBounds
//
//  Synopsis:  Computes the 2D screen bounds of the CMilPointAndSize3F after
//             projecting with the current 3D world, view, and projection
//             transforms and clipping to the camera's Near and Far
//             planes.
//
//-------------------------------------------------------------------------
void
CalcProjectedBounds(
    __in_ecount(1) const CBaseMatrix &matFullTransform3D,
    __in_ecount(1) const CMilPointAndSize3F *pboxBounds,
    __out_ecount(1) CMilRectF *prcTargetRect
    )
{
    // If we fail before computing the bounds, return infinite rect.
    prcTargetRect->SetInfinite();

    //
    //  Get the 8 points at the corners of the box
    //

    // The following line could be written simply as:
    //
    //    vector4 rvecBoxVertices[8]; 
    //
    // There is a compiler bug in VS2013 that forces us to use the 
    // array initializer form seen here.
    vector4 rvecBoxVertices[8]{ {}, {}, {}, {}, {}, {}, {}, {} };

    pboxBounds->ToVector4Array(rvecBoxVertices);

    //
    //  Transform the 8 points by the full world to device space transform.
    //
    auto vertices =
        math_extensions::transform_array(
            sizeof(rvecBoxVertices[0]),                                                   // out_stride
            std::vector<vector4>(std::begin(rvecBoxVertices), std::end(rvecBoxVertices)), // in 
            sizeof(rvecBoxVertices[0]),                                                   // in_stride
            matFullTransform3D,                                                           // transformation
            ARRAY_SIZE(rvecBoxVertices));

    std::copy(vertices.begin(), vertices.end(), rvecBoxVertices);

    // If after transform all of the values aren't in
    // (-FLT_MAX,FLT_MAX) we quit early with an infinite bounding box.
    for (int i = 0 ; i < ARRAY_SIZE(rvecBoxVertices); ++i)
        for (int j = 0; j < 4; ++j )
    {
        float f = rvecBoxVertices[i][j];
        if ( !(f > -FLT_MAX && f < FLT_MAX) )
            return;
    }

    //
    //  Clip the 12 line segments of the box to the camera's near
    //  and far clipping planes.  (Windows OS# 933994)
    //
    {
        const DWORD nClipPlanes = 2;
        double rBCs[ARRAY_SIZE(rvecBoxVertices)][nClipPlanes];
        DWORD rdwOutCodes[ARRAY_SIZE(rvecBoxVertices)];
        bool lookingForFirstPoint = true;

        // We initialize these to zero so that in the case the that all 12
        // lines are clipped we return an empty bounding box.
        auto vecMin = vector3(0.0f, 0.0f, 0.0f);
        auto vecMax = vector3(0.0f, 0.0f, 0.0f);

        CDoubleFPU fpu;     // Setting floating point state to double precision

        // Compute the boundary coordinates and outcodes for the vertices.
        // See ClipLine4D for more information.
        //
        // NOTE: If we kept a running AND and OR of the outCodes as we compute
        //       them in this loop we could find the trivial accept and reject
        //       cases for the entire bounding box without calling ClipLine4D.
        //       (Possible perf win).  (danlehen 4/22/04)
        //
        for(int i = 0; i < ARRAY_SIZE(rvecBoxVertices); i++)
        {
            const auto& p = rvecBoxVertices[i];

            // Move near and far clipping planes out so bounds computation is conservative relative
            // to the hw clipping.  Z here is linearly mapped into depth buffer values, so to
            // account for floating point error we should expand on the order of 1/(2^bits) if bits
            // is the depth buffer bits.  We can use 0.0001 (> 1/2^16 = 0.000015) because
            // conservatism here will cause no significant problems.
            const double eps = 0.0001;
            rBCs[i][0] =     eps*p.w + p.z; // BC for Z = -eps plane
            rBCs[i][1] = (1+eps)*p.w - p.z; // BC for Z = 1+eps plane

            rdwOutCodes[i] = ComputeOutCode(rBCs[i], nClipPlanes);
        }

        // Run through the 12 edges of the box trimming the line segments against
        // the Z=0 and Z=1 planes.
        //
        // NOTE: I am not terribly fond of using the lookingForFirstPoint flag.
        //       Another idea I had was to have two loops.  The first from 0..12
        //       which breaks after the first line is found and the second picks
        //       up from i..12, but I didn't like that the duplicate code that was
        //       producing, so I guess the flag is not terrible.  (danlehen 4/30/04)
        //
        for (DWORD i = 0; i < 12; i++)
        {
            DWORD index0 = CMilPointAndSize3F::sc_rdwEdgeList[i][0];
            auto p0 = rvecBoxVertices[index0];

            DWORD index1 = CMilPointAndSize3F::sc_rdwEdgeList[i][1];
            auto p1 = rvecBoxVertices[index1];

            bool includeLine = ClipLine4D(nClipPlanes, &p0, rBCs[index0], rdwOutCodes[index0], &p1, rBCs[index1], rdwOutCodes[index1]);

            if (includeLine)
            {
                // Sanity check our clip results.  We expect W > 0 and 0 <= Z <= 1 (approximately).

#if NEVER
                // Clipping has very large worse case error of DBL_EPSILON * FLT_MAX,
                // which is as large as 7e22, so these asserts don't work.  Perhaps after the
                // clipping bug is fixed these asserts can be resurrected.
                Assert(p0.w > -500.0f * FLT_EPSILON && (p0.w*-500.0f * FLT_EPSILON) <= p0.z && (p0.z <= p0.w*(1.0 + 500.0f * FLT_EPSILON)));
                Assert(p1.w > -500.0f * FLT_EPSILON && (p1.w*-500.0f * FLT_EPSILON) <= p1.z && (p1.z <= p1.w*(1.0 + 500.0f * FLT_EPSILON)));
#endif

                // Add p0 to the clipped bounds.
                BoundPointHelper(p0, &vecMin, &vecMax, &lookingForFirstPoint);

                Assert(!lookingForFirstPoint);

                // Add p1 to the clipped bounds.
                BoundPointHelper(p1, &vecMin, &vecMax, &lookingForFirstPoint);
            }
        }

        *prcTargetRect = CMilRectF(
            vecMin.x,
            vecMin.y,
            vecMax.x,
            vecMax.y,
            LTRB_Parameters
            );
    } // end of scope for CDoubleFPU
}

template <typename OutCoordSpace>
void
CalcProjectedBounds(
    __in_ecount(1) const CMatrix<CoordinateSpace::Local3D,OutCoordSpace> &matFullTransform3D,
    __in_ecount(1) const CMilPointAndSize3F *pboxBounds,
    __out_ecount(1) CRectF<OutCoordSpace> *prcTargetRect
    )
{
    CalcProjectedBounds(
        static_cast<const CBaseMatrix &>(matFullTransform3D),
        pboxBounds,
        prcTargetRect
        );
}


//+----------------------------------------------------------------------------
//
//  Function:  Calc2DBoundsAndIdealSamplingEstimates
//
//  Synopsis:  Returns an approximated transform from brush space to ideal
//             (resolution quality optimized) sample space.  Optionally the
//             device bounds of the Mesh3D and/or the estimated ddx/ddy are
//             also returned.
//
//-----------------------------------------------------------------------------
void
Calc2DBoundsAndIdealSamplingEstimates(
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::Local3D> &matFullTransform3D,
    __in_ecount(1) const CMilPointAndSize3F* pMesh3DBox,
    __in_ecount(1) const CRectF<CoordinateSpace::BaseSampling>* prcBrushSampleBounds,
        // Texture coordinates are in world sampling space
    __out_ecount(1) CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> *pmatBrushSpaceToIdealSampleSpace,
    __out_ecount_opt(1) CMultiSpaceRectF<CoordinateSpace::PageInPixels,CoordinateSpace::Device> *prcMeshBoundsTargetSpace 
    )
{
    CMultiSpaceRectF<CoordinateSpace::PageInPixels,CoordinateSpace::Device> rcMeshBoundsTargetSpace;
       
    float flBrushSampleWidthSpan = prcBrushSampleBounds->Width();
    bool fBrushSpanHasWidth = !IsCloseReal(flBrushSampleWidthSpan, 0.0f);

    float flBrushSampleHeightSpan = prcBrushSampleBounds->Height();
    bool fBrushSpanHasHeight = !IsCloseReal(flBrushSampleHeightSpan, 0.0f);

    if (   fBrushSpanHasWidth
        || fBrushSpanHasHeight
        || prcMeshBoundsTargetSpace
       )
    {
        if (!prcMeshBoundsTargetSpace)
        {
            prcMeshBoundsTargetSpace = &rcMeshBoundsTargetSpace;
        }

#if DBG_ANALYSIS
        if (matFullTransform3D.DbgCurrentCoordinateSpaceId() == CoordinateSpace::Device::Id)
        {
            CalcProjectedBounds(
                static_cast<const CMatrix<CoordinateSpace::Local3D,CoordinateSpace::Device> &>(matFullTransform3D),
                pMesh3DBox,
                OUT &prcMeshBoundsTargetSpace->Device()// Delegate setting of out-param
                );
        }
        else
#else
        // When not debug, it is fine to just pick one space and work with that.
#endif
        {
            CalcProjectedBounds(
                static_cast<const CMatrix<CoordinateSpace::Local3D,CoordinateSpace::PageInPixels> &>(matFullTransform3D),
                pMesh3DBox,
                OUT &prcMeshBoundsTargetSpace->PageInPixels()  // Delegate setting of out-param
                );
        }
    }

    //
    // We need to approximate the size of the brush realization required to
    // render on the 3d object.  Currently we take the diagonal of the screen
    // bounding box for the scale.
    //

    float flIdealBrushSampleSpan = 0;

    // If either brush span is non-zero, compute the approximate
    if (   fBrushSpanHasWidth
        || fBrushSpanHasHeight
       )
    {
        //
        // We need to map the bounds of brush sampling of the mesh to ideal
        // sample space for the brush.  Since we don't know the orientation of
        // the brush on the screen we can't know whether the brush will be
        // scaled in any direction more than others.  Therefore we scale
        // uniformly in both x and y.
        //
        // The ideal brush realization size is currently calculated as the
        // length of the longest line that could be drawn on the screen based
        // on the bounds of the object.  This is the diagonal of the bounds. 
        // This ideal brush realization size is subject to change.
        //

        // Rect should be ordered, but may have Width or Height beyond float
        // range; so, Width() and Height() properties can't be used without
        // double precision.  In float math the large width and/or height will
        // become infinity and that is okay.  Thus use UnorderedWidth/Height().
        Assert(prcMeshBoundsTargetSpace->AnySpace().IsWellOrdered());
        float flWidth = prcMeshBoundsTargetSpace->AnySpace().UnorderedWidth<float>();
        float flHeight = prcMeshBoundsTargetSpace->AnySpace().UnorderedHeight<float>();

        flIdealBrushSampleSpan = sqrtf(
            flWidth*flWidth + 
            flHeight*flHeight
            );

        flIdealBrushSampleSpan = CFloatFPU::FloorFFast(flIdealBrushSampleSpan);
    }

    //
    // We divide the ideal realization brush size (of Ideal Sample Space =
    // Device Space) by the brush sampling bounds (of Brush Space) to
    // create the approximate mapping from Brush Space to Ideal Sample
    // Space.
    //
    //   Should 0 be default brush->ideal sample scale?
    //

    auto sx = fBrushSpanHasWidth ? (flIdealBrushSampleSpan / flBrushSampleWidthSpan) : 1.0f;
    auto sy = fBrushSpanHasHeight ? (flIdealBrushSampleSpan / flBrushSampleHeightSpan) : 1.0f;
    const auto sz = 1.0f; // Should be unused
    *pmatBrushSpaceToIdealSampleSpace = matrix::get_scaling(sx, sy, sz);
}

//+------------------------------------------------------------------------
//
//  Function:  IsUniformNonZeroVec3
//
//  Synopsis:  Returns "true" if x, y, and z are the same and non-zero
//             
//-------------------------------------------------------------------------
bool 
IsUniformNonZeroVec3(
    __in_ecount(1) const vector3 *vec3V
    )
{
    float flAvg = (vec3V->x + vec3V->y + vec3V->z) / 3.0f;
    // if it isn't a zero vector...
    if (fabs(flAvg) > FLT_EPSILON) 
    {
        // ...and the vector is uniform
        if (fabs(vec3V->x - flAvg) <= FLT_EPSILON &&
            fabs(vec3V->y - flAvg) <= FLT_EPSILON &&
            fabs(vec3V->z - flAvg) <= FLT_EPSILON)
        {
            return true;
        }
    }

    return false;
}

//+------------------------------------------------------------------------
//
//  Function:  IsFiniteVec3
//
//  Synopsis:  Returns "true" if x, y, and z are all finite and non-NaN
//             
//-------------------------------------------------------------------------
bool 
IsFiniteVec3(
    __in_ecount(1) const vector3 *vec3V
    )
{
    return (_finite(vec3V->x) && _finite(vec3V->y) && _finite(vec3V->z));
}


//+------------------------------------------------------------------------
//
//  Function:  DegToRadF
//
//  Synopsis:  Converts an angle from degrees to radians. If the angle is
//             greater than or less than 360, we mod it with 360.
//             
//-------------------------------------------------------------------------

float
DegToRadF(double angleInDeg)
{
    // Take angle value modulo 360 before casting to float to avoid
    // excessive loss of precision when going from double to float.
    // Otherwise even angles as small as 36000000 will be inaccurate
    if (angleInDeg > 360 || angleInDeg < -360)
    {
        angleInDeg = fmod(angleInDeg, 360);
    }

    auto angleInRadians = math_extensions::to_radian(static_cast<float>(angleInDeg));
    return static_cast<float>(angleInRadians);
}






