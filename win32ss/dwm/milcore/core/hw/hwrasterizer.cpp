// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Trapezoidal anti-aliasing implementation
//
//  >>>> Note that some of this code is duplicated in sw\aarasterizer.cpp,
//  >>>> so changes to this file may need to propagate.
//
//   pursue reduced code duplication
//

#include "precomp.hpp"

MtDefine(CHwRasterizer, MILRender, "CHwRasterizer");

DeclareTag(tagDisableTrapezoids, "MIL-HW", "Disable trapezoids");

//
// Optimize for speed instead of size for these critical methods
//

#pragma optimize( "t", on )

//-------------------------------------------------------------------------
//
// Coordinate system encoding
//
// All points/coordinates are named as follows:
//
//    <HungarianType><CoordinateSystem>[X|Y][Left|Right|Top|Bottom]VariableName
//
//    Common hungarian types:
//        n - INT
//        u - UINT
//        r - FLOAT
//
//    Coordinate systems:
//        Pixel - Device pixel space assuming integer coordinates in the pixel top left corner.
//        Subpixel - Overscaled space.
//
//        To convert between Pixel to Subpixel, we have:
//            nSubpixelCoordinate = nPixelCoordinate << c_nShift;
//            nPixelCoordinate = nSubpixelCoordinate >> c_nShift;
//
//        Note that the conversion to nPixelCoordinate needs to also track
//        (nSubpixelCoordinate & c_nShiftMask) to maintain the full value.
//
//        Note that since trapezoidal only supports 8x8, c_nShiftSize is always equal to 8.  So,
//        (1, 2) in pixel space would become (8, 16) in subpixel space.
//
//    [X|Y]
//        Indicates which coordinate is being referred to.
//
//    [Left|Right|Top|Bottom]
//        When referring to trapezoids or rectangular regions, this
//        component indicates which edge is being referred to.
//
//    VariableName
//       Descriptive portion of the variable name
//
//-------------------------------------------------------------------------

MtDefine(HwRasterizerEdge, MILRawMemory, "MAARasterizerEdge");

//-------------------------------------------------------------------------
//
//  Function:   IsFractionGreaterThan
//
//  Synopsis:
//     Determine if nNumeratorA/nDenominatorA > nNumeratorB/nDenominatorB
//
//     Note that we assume all denominators are strictly greater than zero.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE BOOL
IsFractionGreaterThan(
    INT nNumeratorA,                    // Left hand side numerator
    __in_range(>=, 1) INT nDenominatorA, // Left hand side denominator
    INT nNumeratorB,                    // Right hand side numerator
    __in_range(>=, 1) INT nDenominatorB  // Right hand side denominator
    )
{
    //
    // nNumeratorA/nDenominatorA > nNumeratorB/nDenominatorB
    // iff nNumeratorA*nDenominatorB/nDenominatorA > nNumeratorB, since nDenominatorB > 0
    // iff nNumeratorA*nDenominatorB > nNumeratorB*nDenominatorA, since nDenominatorA > 0
    //
    // Now, all input parameters are 32-bit integers, so we need to use
    // a 64-bit result to compute the product.
    //

    LONGLONG lNumeratorAxDenominatorB = Int32x32To64(nNumeratorA, nDenominatorB);
    LONGLONG lNumeratorBxDenominatorA = Int32x32To64(nNumeratorB, nDenominatorA);

    return (lNumeratorAxDenominatorB > lNumeratorBxDenominatorA);
}

//-------------------------------------------------------------------------
//
//  Function:   IsFractionLessThan
//
//  Synopsis:
//     Determine if nNumeratorA/nDenominatorA < nNumeratorB/nDenominatorB
//
//     Note that we assume all denominators are strictly greater than zero.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE BOOL
IsFractionLessThan(
    INT nNumeratorA,                    // Left hand side numerator
    __in_range(>=, 1) INT nDenominatorA, // Left hand side denominator
    INT nNumeratorB,                    // Right hand side numerator
    __in_range(>=, 1) INT nDenominatorB  // Right hand side denominator
    )
{
    //
    // Same check as previous function with less than comparision instead of
    // a greater than comparison.
    //

    LONGLONG lNumeratorAxDenominatorB = Int32x32To64(nNumeratorA, nDenominatorB);
    LONGLONG lNumeratorBxDenominatorA = Int32x32To64(nNumeratorB, nDenominatorA);

    return (lNumeratorAxDenominatorB < lNumeratorBxDenominatorA);
}


//-------------------------------------------------------------------------
//
//  Function:   AdvanceDDAMultipleSteps
//
//  Synopsis:
//     Advance the DDA by multiple steps
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE void
AdvanceDDAMultipleSteps(
    __in_ecount(1) const CEdge *pEdgeLeft,         // Left edge from active edge list
    __in_ecount(1) const CEdge *pEdgeRight,        // Right edge from active edge list
    INT nSubpixelYAdvance,                    // Number of steps to advance the DDA
    __out_ecount(1) INT &nSubpixelXLeftBottom,     // Resulting left x position
    __out_ecount(1) INT &nSubpixelErrorLeftBottom, // Resulting left x position error
    __out_ecount(1) INT &nSubpixelXRightBottom,    // Resulting right x position
    __out_ecount(1) INT &nSubpixelErrorRightBottom // Resulting right x position error
    )
{
    //
    // In this method, we need to be careful of overflow.  Expected input ranges for values are:
    //
    //      edge points: x and y subpixel space coordinates are between [-2^26, 2^26]
    //                   since we start with 28.4 space (and are now in subpixel space,
    //                   i.e., no 16x scale) and assume 2 bits of working space.
    //
    //                   This assumption is ensured by TransformRasterizerPointsTo28_4.
    //
#if DBG
    INT nDbgPixelCoordinateMax = (1 << 26);
    INT nDbgPixelCoordinateMin = -nDbgPixelCoordinateMax;

    Assert(pEdgeLeft->X >= nDbgPixelCoordinateMin && pEdgeLeft->X <= nDbgPixelCoordinateMax);
    Assert(pEdgeLeft->EndY >= nDbgPixelCoordinateMin && pEdgeLeft->EndY <= nDbgPixelCoordinateMax);
    Assert(pEdgeRight->X >= nDbgPixelCoordinateMin && pEdgeRight->X <= nDbgPixelCoordinateMax);
    Assert(pEdgeRight->EndY >= nDbgPixelCoordinateMin && pEdgeRight->EndY <= nDbgPixelCoordinateMax);

    //
    //        errorDown: (0, 2^30)
    //                   Since errorDown is the edge delta y in 28.4 space (not subpixel space
    //                   like the end points), we have a larger range of (0, 2^32) for the positive
    //                   error down.  With 2 bits of work space (which TransformRasterizerPointsTo28_4
    //                   ensures), we know we are between (0, 2^30)
    //

    INT nDbgErrorDownMax = (1 << 30);
    Assert(pEdgeLeft->ErrorDown  > 0 && pEdgeLeft->ErrorDown  < nDbgErrorDownMax);
    Assert(pEdgeRight->ErrorDown > 0 && pEdgeRight->ErrorDown < nDbgErrorDownMax);

    //
    //          errorUp: [0, errorDown)
    //
    Assert(pEdgeLeft->ErrorUp  >= 0 && pEdgeLeft->ErrorUp  < pEdgeLeft->ErrorDown);
    Assert(pEdgeRight->ErrorUp >= 0 && pEdgeRight->ErrorUp < pEdgeRight->ErrorDown);
#endif

    //
    // Advance the left edge
    //

    // Since each point on the edge is withing 28.4 space, the following computation can't overflow.
    nSubpixelXLeftBottom = pEdgeLeft->X + nSubpixelYAdvance*pEdgeLeft->Dx;

    // Since the error values can be close to 2^30, we can get an overflow by multiplying with yAdvance.
    // So, we need to use a 64-bit temporary in this case.
    LONGLONG llSubpixelErrorBottom = pEdgeLeft->Error + Int32x32To64(nSubpixelYAdvance, pEdgeLeft->ErrorUp);
    if (llSubpixelErrorBottom >= 0)
    {
        LONGLONG llSubpixelXLeftDelta = llSubpixelErrorBottom / static_cast<LONGLONG>(pEdgeLeft->ErrorDown);

        // The delta should remain in range since it still represents a delta along the edge which
        // we know fits entirely in 28.4.  Note that we add one here since the error must end up
        // less than 0.
        Assert(llSubpixelXLeftDelta < INT_MAX);
        INT nSubpixelXLeftDelta = static_cast<INT>(llSubpixelXLeftDelta) + 1;

        nSubpixelXLeftBottom += nSubpixelXLeftDelta;
        llSubpixelErrorBottom -= Int32x32To64(pEdgeLeft->ErrorDown, nSubpixelXLeftDelta);
    }

    // At this point, the subtraction above should have generated an error that is within
    // (-pLeft->ErrorDown, 0)

    Assert((llSubpixelErrorBottom > -pEdgeLeft->ErrorDown) && (llSubpixelErrorBottom < 0));
    nSubpixelErrorLeftBottom = static_cast<INT>(llSubpixelErrorBottom);

    //
    // Advance the right edge
    //

    // Since each point on the edge is withing 28.4 space, the following computation can't overflow.
    nSubpixelXRightBottom = pEdgeRight->X + nSubpixelYAdvance*pEdgeRight->Dx;

    // Since the error values can be close to 2^30, we can get an overflow by multiplying with yAdvance.
    // So, we need to use a 64-bit temporary in this case.
    llSubpixelErrorBottom = pEdgeRight->Error + Int32x32To64(nSubpixelYAdvance, pEdgeRight->ErrorUp);
    if (llSubpixelErrorBottom >= 0)
    {
        LONGLONG llSubpixelXRightDelta = llSubpixelErrorBottom / static_cast<LONGLONG>(pEdgeRight->ErrorDown);

        // The delta should remain in range since it still represents a delta along the edge which
        // we know fits entirely in 28.4.  Note that we add one here since the error must end up
        // less than 0.
        Assert(llSubpixelXRightDelta < INT_MAX);
        INT nSubpixelXRightDelta = static_cast<INT>(llSubpixelXRightDelta) + 1;

        nSubpixelXRightBottom += nSubpixelXRightDelta;
        llSubpixelErrorBottom -= Int32x32To64(pEdgeRight->ErrorDown, nSubpixelXRightDelta);
    }

    // At this point, the subtraction above should have generated an error that is within
    // (-pRight->ErrorDown, 0)

    Assert((llSubpixelErrorBottom > -pEdgeRight->ErrorDown) && (llSubpixelErrorBottom < 0));
    nSubpixelErrorRightBottom = static_cast<INT>(llSubpixelErrorBottom);
}

//-------------------------------------------------------------------------
//
//  Function:   ComputeDeltaUpperBound
//
//  Synopsis:
//     Compute some value that is >= nSubpixelAdvanceY*|1/m| where m is the
//     slope defined by the edge below.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT
ComputeDeltaUpperBound(
    __in_ecount(1) const CEdge *pEdge,  // Edge containing 1/m value used for computation
    INT nSubpixelYAdvance          // Multiplier in synopsis expression
    )
{
    INT nSubpixelDeltaUpperBound;

    //
    // Compute the delta bound
    //

    if (pEdge->ErrorUp == 0)
    {
        //
        // No errorUp, so simply compute bound based on dx value
        //

        nSubpixelDeltaUpperBound = nSubpixelYAdvance*abs(pEdge->Dx);
    }
    else
    {
        INT nAbsDx, nAbsErrorUp;

        //
        // Compute abs of (dx, error)
        //
        // Here, we can assume errorUp > 0
        //

        Assert(pEdge->ErrorUp > 0);

        if (pEdge->Dx >= 0)
        {
            nAbsDx = pEdge->Dx;
            nAbsErrorUp = pEdge->ErrorUp;
        }
        else
        {
            //
            // Dx < 0, so negate (dx, errorUp)
            //
            // Note that since errorUp > 0, we know -errorUp < 0 and that
            // we need to add errorDown to get an errorUp >= 0 which
            // also means substracting one from dx.
            //

            nAbsDx = -pEdge->Dx - 1;
            nAbsErrorUp = -pEdge->ErrorUp + pEdge->ErrorDown;
        }

        //
        // Compute the bound of nSubpixelAdvanceY*|1/m|
        //
        // Note that the +1 below is included to bound any left over errorUp that we are dropping here.
        //

        nSubpixelDeltaUpperBound = nSubpixelYAdvance*nAbsDx + (nSubpixelYAdvance*nAbsErrorUp)/pEdge->ErrorDown + 1;
    }

    return nSubpixelDeltaUpperBound;
}

//-------------------------------------------------------------------------
//
//  Function:   ComputeDistanceLowerBound
//
//  Synopsis:
//     Compute some value that is <= distance between
//     (pEdgeLeft->X, pEdgeLeft->Error) and (pEdgeRight->X, pEdgeRight->Error)
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE INT
ComputeDistanceLowerBound(
    __in_ecount(1) const CEdge *pEdgeLeft, // Left edge containing the position for the distance computation
    __in_ecount(1) const CEdge *pEdgeRight // Right edge containing the position for the distance computation
    )
{
    //
    // Note: In these comments, error1 and error2 are theoretical. The actual Error members
    // are biased by -1.
    //
    // distance = (x2 + error2/errorDown2) - (x1 + error1/errorDown1)
    //          = x2 - x1 + error2/errorDown2 - error1/errorDown1
    //          >= x2 - x1 + error2/errorDown2   , since error1 < 0
    //          >= x2 - x1 - 1                   , since error2 < 0
    //          = pEdgeRight->X - pEdgeLeft->X - 1
    //
    // In the special case where error2/errorDown2 >= error1/errorDown1, we
    // can get a tigher bound of:
    //
    //          pEdgeRight->X - pEdgeLeft->X
    //
    // This case occurs often in thin strokes, so we check for it here.
    //

    Assert(pEdgeLeft->Error  < 0);
    Assert(pEdgeRight->Error < 0);
    Assert(pEdgeLeft->X <= pEdgeRight->X);

    INT nSubpixelXDistanceLowerBound = pEdgeRight->X - pEdgeLeft->X;

    //
    // If error2/errorDown2 < error1/errorDown1, we need to subtract one from the bound.
    // Note that error's are actually baised by -1, we so we have to add one before
    // we do the comparison.
    //

    if (IsFractionLessThan(
             pEdgeRight->Error+1,
             pEdgeRight->ErrorDown,
             pEdgeLeft->Error+1,
             pEdgeLeft->ErrorDown
        ))
    {
            // We can't use the tighter lower bound described above, so we need to subtract one to
            // ensure we have a lower bound.

            nSubpixelXDistanceLowerBound -= 1;
    }

    return nSubpixelXDistanceLowerBound;
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::CHwRasterizer
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CHwRasterizer::CHwRasterizer()
{
    m_pDeviceNoRef = NULL;

    // State is cleared on the Setup call
    m_matWorldToDevice.SetToIdentity();
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::ConvertSubpixelXToPixel
//
//  Synopsis:
//      Convert from our subpixel coordinate (x + error/errorDown)
//      to a floating point value.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE float
CHwRasterizer::ConvertSubpixelXToPixel(
    INT x,
    INT error,
    FLOAT rErrorDown
    )
{
    Assert(rErrorDown > FLT_EPSILON);
    return (static_cast<float>(x) + static_cast<float>(error)/rErrorDown)*c_rInvShiftSize;
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::ConvertSubpixelYToPixel
//
//  Synopsis:
//      Convert from our subpixel space to pixel space assuming no
//      error.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE float
CHwRasterizer::ConvertSubpixelYToPixel(
    int nSubpixel
    )
{
    return static_cast<float>(nSubpixel)*c_rInvShiftSize;
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::RasterizePath
//
//  Synopsis:
//      Internal rasterizer fill path.  Note that this method follows the
//      same basic structure as the software rasterizer in aarasterizer.cpp.
//
//      The general algorithm used for rasterization is a vertical sweep of
//      the shape that maintains an active edge list.  The sweep is done
//      at a sub-scanline resolution and results in either:
//          1. Sub-scanlines being combined in the coverage buffer and output
//             as "complex scans".
//          2. Simple trapezoids being recognized in the active edge list
//             and output using a faster simple trapezoid path.
//
//      This method consists of the setup to the main rasterization loop
//      which includes:
//
//          1. Setup of the clip rectangle
//          2. Calling FixedPointPathEnumerate to populate our inactive
//             edge list.
//          3. Delegating to RasterizePath to execute the main loop.
//
//-------------------------------------------------------------------------
HRESULT
CHwRasterizer::RasterizePath(
    __in_ecount(cPoints)   const MilPoint2F *rgpt,
    __in_ecount(cPoints)   const BYTE *rgTypes,
    const UINT cPoints,
    __in_ecount(1) const CMILMatrix *pmatWorldTransform,
    MilFillMode::Enum fillMode
    )
{
    HRESULT hr = S_OK;
    CInactiveEdge inactiveArrayStack[INACTIVE_LIST_NUMBER];
    CInactiveEdge *pInactiveArray;
    CInactiveEdge *pInactiveArrayAllocation = NULL;
    CEdge edgeHead;
    CEdge edgeTail;
    CEdge *pEdgeActiveList;
    CEdgeStore edgeStore;
    CInitializeEdgesContext edgeContext;

    edgeContext.ClipRect = NULL;

    edgeTail.X = INT_MAX;       // Terminator to active list
    edgeTail.StartY = INT_MAX;  // Terminator to inactive list

    edgeTail.EndY = INT_MIN;
    edgeHead.X = INT_MIN;       // Beginning of active list
    edgeContext.MaxY = INT_MIN;

    edgeHead.Next = &edgeTail;
    pEdgeActiveList = &edgeHead;
    edgeContext.Store = &edgeStore;

    edgeContext.AntiAliasMode = c_antiAliasMode;
    Assert(edgeContext.AntiAliasMode != MilAntiAliasMode::None);

    // If the path contains 0 or 1 points, we can ignore it.
    if (cPoints < 2)
    {
        return S_OK;
    }

    INT nPixelYClipBottom = m_rcClipBounds.Y + m_rcClipBounds.Height;

    // Scale the clip bounds rectangle by 16 to account for our
    // scaling to 28.4 coordinates:

    RECT clipBounds;
    clipBounds.left   = m_rcClipBounds.X * FIX4_ONE;
    clipBounds.top    = m_rcClipBounds.Y * FIX4_ONE;
    clipBounds.right  = (m_rcClipBounds.X + m_rcClipBounds.Width) * FIX4_ONE;
    clipBounds.bottom = (m_rcClipBounds.Y + m_rcClipBounds.Height) * FIX4_ONE;

    edgeContext.ClipRect = &clipBounds;

    //////////////////////////////////////////////////////////////////////////
    // Convert all our points to 28.4 fixed point:

    CMILMatrix matrix(*pmatWorldTransform);
    AppendScaleToMatrix(&matrix, TOREAL(16), TOREAL(16));

    // Enumerate the path and construct the edge table:

    MIL_THR(FixedPointPathEnumerate(
        rgpt,
        rgTypes,
        cPoints,
        &matrix,
        edgeContext.ClipRect,
        &edgeContext
        ));

    if (FAILED(hr))
    {
        if (hr == WGXERR_VALUEOVERFLOW)
        {
            // Draw nothing on value overflow and return
            hr = S_OK;
        }
        goto Cleanup;
    }

    UINT nTotalCount = edgeStore.StartEnumeration();
    if (nTotalCount == 0)
    {
        hr = S_OK;     // We're outta here (empty path or entirely clipped)
        goto Cleanup;
    }

    // At this point, there has to be at least two edges.  If there's only
    // one, it means that we didn't do the trivially rejection properly.

    Assert((nTotalCount >= 2) && (nTotalCount <= (UINT_MAX - 2)));

    pInactiveArray = &inactiveArrayStack[0];
    if (nTotalCount > (INACTIVE_LIST_NUMBER - 2))
    {
        IFC(HrMalloc(
            Mt(HwRasterizerEdge),
            sizeof(CInactiveEdge),
            nTotalCount + 2,
            (void **)&pInactiveArrayAllocation
            ));

        pInactiveArray = pInactiveArrayAllocation;
    }

    // Initialize and sort the inactive array:

    INT nSubpixelYCurrent = InitializeInactiveArray(
        &edgeStore,
        pInactiveArray,
        nTotalCount,
        &edgeTail
        );

    INT nSubpixelYBottom = edgeContext.MaxY;

    Assert(nSubpixelYBottom > 0);

    // Skip the head sentinel on the inactive array:

    pInactiveArray++;

    //
    // Rasterize the path
    //

    // 'nPixelYClipBottom' is in screen space and needs to be converted to the
    // format we use for antialiasing.

    nSubpixelYBottom = min(nSubpixelYBottom, nPixelYClipBottom << c_nShift);

    // 'nTotalCount' should have been zero if all the edges were
    // clipped out (RasterizeEdges assumes there's at least one edge
    // to be drawn):

    Assert(nSubpixelYBottom > nSubpixelYCurrent);

    IFC(RasterizeEdges(
        pEdgeActiveList,
        pInactiveArray,
        nSubpixelYCurrent,
        nSubpixelYBottom
        ));

Cleanup:
    // Free any objects and get outta here:
    GpFree(pInactiveArrayAllocation);

    // Free coverage buffer
    m_coverageBuffer.Destroy();

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::Setup
//
//  Synopsis:
//      1. Ensure clean state
//      2. Convert path to internal format
//
//-------------------------------------------------------------------------
HRESULT
CHwRasterizer::Setup(
    __in_ecount(1)       CD3DDeviceLevel1 *pD3DDevice,
    __in_ecount(1) const IShapeData       *pShape,
    __inout_ecount(1) DynArray<MilPoint2F> *prgPointsScratch,
    __inout_ecount(1) DynArray<BYTE>      *prgTypesScratch,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatWorldToDevice
    )
{
    HRESULT hr = S_OK;

    //
    // Setup scratch buffers
    //

    m_prgPoints = prgPointsScratch;
    m_prgTypes  = prgTypesScratch;

    //
    // Initialize our state
    //

    m_prgPoints->Reset(FALSE /* fReset */);
    m_prgTypes->Reset(FALSE /* fReset */);
    ZeroMemory(&m_rcClipBounds, sizeof(m_rcClipBounds));
    m_pIGeometrySink = NULL;

    // Initialize the coverage buffer
    m_coverageBuffer.Initialize();

    m_pDeviceNoRef = pD3DDevice;

    //
    // PS#856364-2003/07/01-ashrafm  Remove pixel center fixup
    //
    // Incoming coordinate space uses integers at upper-left of pixel (pixel
    // center are half integers) at device level.
    //
    // Rasterizer uses the coordinate space with integers at pixel center.
    //
    // To convert from center (1/2, 1/2) to center (0, 0) we need to subtract
    // 1/2 from each coordinate in device space.
    //
    // See InitializeEdges in aarasterizer.ccp to see how we unconvert for
    // antialiased rendering.
    //

    CMILMatrix matWorldHPCToDeviceIPC;
    if (pmatWorldToDevice)
    {
        matWorldHPCToDeviceIPC = *pmatWorldToDevice;
    }
    else
    {
        matWorldHPCToDeviceIPC.SetToIdentity();
    }
    matWorldHPCToDeviceIPC.SetDx(matWorldHPCToDeviceIPC.GetDx() - 0.5f);
    matWorldHPCToDeviceIPC.SetDy(matWorldHPCToDeviceIPC.GetDy() - 0.5f);

    //
    // Set local state.
    //

    pD3DDevice->GetClipRect(&m_rcClipBounds);

    IFC(pShape->ConvertToGpPath(*m_prgPoints, *m_prgTypes));

    m_matWorldToDevice = matWorldHPCToDeviceIPC;
    m_fillMode = pShape->GetFillMode();

    //  There's an opportunity for early clipping here
    //
    // However, since the rasterizer itself does a reasonable job of clipping some
    // cases, we don't early clip yet.

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::SendGeometry
//
//  Synopsis:
//     Tessellate and send geometry to the pipeline
//
//-------------------------------------------------------------------------
HRESULT
CHwRasterizer::SendGeometry(
    __inout_ecount(1) IGeometrySink *pIGeometrySink
    )
{
    HRESULT hr = S_OK;

    //
    // It's ok not to addref the geometry sink here since it
    // is never used outside the scope of this method.
    //

    m_pIGeometrySink = pIGeometrySink;

    //
    // Rasterize the path
    //

    IFC(RasterizePath(
        m_prgPoints->GetDataBuffer(),
        m_prgTypes->GetDataBuffer(),
        m_prgPoints->GetCount(),
        &m_matWorldToDevice,
        m_fillMode
        ));

    //
    // It's possible that we output no triangles.  For example, if we tried to fill a
    // line instead of stroke it.  Since we have no efficient way to detect all these cases
    // up front, we simply rasterize and see if we generated anything.
    //

    if (pIGeometrySink->IsEmpty())
    {
        hr = WGXHR_EMPTYFILL;
    }

Cleanup:
    m_pIGeometrySink = NULL;

    RRETURN1(hr, WGXHR_EMPTYFILL);
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::SendGeometryModifiers
//
//  Synopsis:   Send an AA color source to the pipeline.
//
//-------------------------------------------------------------------------
HRESULT
CHwRasterizer::SendGeometryModifiers(
    __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
    )
{
    HRESULT hr = S_OK;

    CHwColorComponentSource *pAntiAliasColorSource = NULL;

    m_pDeviceNoRef->GetColorComponentSource(
        CHwColorComponentSource::Diffuse,
        &pAntiAliasColorSource
        );

    IFC(pPipelineBuilder->Set_AAColorSource(
        pAntiAliasColorSource
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pAntiAliasColorSource);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::GenerateOutputAndClearCoverage
//
//  Synopsis:
//      Collapse output and generate span data
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT
CHwRasterizer::GenerateOutputAndClearCoverage(
    INT nSubpixelY
    )
{
    HRESULT hr = S_OK;
    INT nPixelY = nSubpixelY >> c_nShift;

    const CCoverageInterval *pIntervalSpanStart = m_coverageBuffer.m_pIntervalStart;

    IFC(m_pIGeometrySink->AddComplexScan(nPixelY, pIntervalSpanStart));

    m_coverageBuffer.Reset();

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::ComputeTrapezoidsEndScan
//
//  Synopsis:
//      This methods takes the current active edge list (and ycurrent)
//      and will determine:
//
//      1. Can we output some list of simple trapezoids for this active
//         edge list?  If the answer is no, then we simply return
//         nSubpixelYCurrent indicating this condition.
//
//      2. If we can output some set of trapezoids, then what is the
//         next ycurrent, i.e., how tall are our trapezoids.
//
//     Note that all trapezoids output for a particular active edge list
//     are all the same height.
//
//     To further understand the conditions for making this decision, it
//     is important to consider the simple trapezoid tessellation:
//
//           ___+_________________+___
//          /  +  /             \  +  \        '+' marks active edges
//         /  +  /               \  +  \
//        /  +  /                 \  +  \
//       /__+__/___________________\__+__\
//       1+1/m                         +
//
//      Note that 1+1/edge_slope is the required expand distance to ensure
//      that we cover all pixels required.
//
//      Now, we can fail to output any trapezoids under the following conditions:
//         1. The expand regions along the top edge of the trapezoid overlap.
//         2. The expand regions along the bottom edge of the trapezoid overlap
//            within the current scanline.  Note that if the bottom edges overlap
//            at some later point, we can shorten our trapezoid to remove the
//            overlapping.
//
//      The key to the algorithm at this point is to detect the above condition
//      in our active edge list and either update the returned end y position
//      or reject all together based on overlapping.
//
//-------------------------------------------------------------------------
INT
CHwRasterizer::ComputeTrapezoidsEndScan(
    __in_ecount(1) const CEdge *pEdgeCurrent,
    INT nSubpixelYCurrent,
    INT nSubpixelYNextInactive
    )
{
    INT nSubpixelYBottomTrapezoids = nSubpixelYNextInactive;
    const CEdge *pEdgeLeft;
    const CEdge *pEdgeRight;

    //
    // Trapezoids should always start at scanline boundaries
    //

    Assert((nSubpixelYCurrent & c_nShiftMask) == 0);

    //
    // If we are doing a winding mode fill, check that we can ignore mode and do an
    // alternating fill in OutputTrapezoids.  This condition occurs when winding is
    // equivalent to alternating which happens if the pairwise edges have different
    // winding directions.
    //

    if (m_fillMode == MilFillMode::Winding)
    {
        for (const CEdge *pEdge = pEdgeCurrent; pEdge->EndY != INT_MIN; pEdge = pEdge->Next->Next)
        {
            // The active edge list always has an even number of edges which we actually
            // assert in ASSERTACTIVELIST.

            Assert(pEdge->Next->EndY != INT_MIN);

            // If not alternating winding direction, we can't fill with alternate mode

            if (pEdge->WindingDirection == pEdge->Next->WindingDirection)
            {
                // Give up until we handle winding mode
                nSubpixelYBottomTrapezoids = nSubpixelYCurrent;
                goto Cleanup;
            }
        }
    }

    //
    // For each edge, we:
    //
    //    1. Set the new trapezoid bottom to the min of the current
    //       one and the edge EndY
    //
    //    2. Check if edges will intersect during trapezoidal shrink/expand
    //

    nSubpixelYBottomTrapezoids = nSubpixelYNextInactive;

    for (const CEdge *pEdge = pEdgeCurrent; pEdge->EndY != INT_MIN; pEdge = pEdge->Next)
    {
        //
        // Step 1
        //
        // Updated nSubpixelYBottomTrapezoids based on edge EndY.
        //
        // Since edges are clipped to the current clip rect y bounds, we also know
        // that pEdge->EndY <= nSubpixelYBottom so there is no need to check for that here.
        //

        nSubpixelYBottomTrapezoids = min(nSubpixelYBottomTrapezoids, pEdge->EndY);

        //
        // Step 2
        //
        // Check that edges will not overlap during trapezoid shrink/expand.
        //

        pEdgeLeft = pEdge;
        pEdgeRight = pEdge->Next;

        if (pEdgeRight->EndY != INT_MIN)
        {
            //
            //        __A__A'___________________B'_B__
            //        \  +  \                  /  +  /       '+' marks active edges
            //         \  +  \                /  +  /
            //          \  +  \              /  +  /
            //           \__+__\____________/__+__/
            //       1+1/m   C  C'         D' D
            //
            // We need to determine if position A' <= position B' and that position C' <= position D'
            // in the above diagram.  So, we need to ensure that both the distance between
            // A and B and the distance between C and D is greater than or equal to:
            //
            //    0.5 + |0.5/m1| + 0.5 + |0.5/m2|               (pixel space)
            //  = shiftsize + halfshiftsize*(|1/m1| + |1/m2|)   (subpixel space)
            //
            // So, we'll start by computing this distance.  Note that we can compute a distance
            // that is too large here since the self-intersection detection is simply used to
            // recognize trapezoid opportunities and isn't required for visual correctness.
            //

            INT nSubpixelExpandDistanceUpperBound =
                c_nShiftSize
                + ComputeDeltaUpperBound(pEdgeLeft, c_nHalfShiftSize)
                + ComputeDeltaUpperBound(pEdgeRight, c_nHalfShiftSize);

            //
            // Compute a top edge distance that is <= to the distance between A' and B' as follows:
            //   lowerbound(distance(A, B)) - nSubpixelExpandDistanceUpperBound
            //

            INT nSubpixelXTopDistanceLowerBound =
                ComputeDistanceLowerBound(pEdgeLeft, pEdgeRight) - nSubpixelExpandDistanceUpperBound;

            //
            // Check if the top edges cross
            //

            if (nSubpixelXTopDistanceLowerBound < 0)
            {
                // The top edges have crossed, so we are out of luck.  We can't
                // start a trapezoid on this scanline

                nSubpixelYBottomTrapezoids = nSubpixelYCurrent;
                goto Cleanup;
            }

            //
            // If the edges are converging, we need to check if they cross at
            // nSubpixelYBottomTrapezoids
            //
            //
            //  1) \       /    2) \    \       3)   /   /
            //      \     /          \   \          /  /
            //       \   /             \  \        / /
            //
            // The edges converge iff (dx1 > dx2 || (dx1 == dx2 && errorUp1/errorDown1 > errorUp2/errorDown2).
            //
            // Note that in the case where the edges do not converge, the code below will end up computing
            // the DDA at the end points and checking for intersection again.  This code doesn't rely on
            // the fact that the edges don't converge, so we can be too conservative here.
            //

            if (pEdgeLeft->Dx > pEdgeRight->Dx
                || ((pEdgeLeft->Dx == pEdgeRight->Dx)
                    && IsFractionGreaterThan(pEdgeLeft->ErrorUp, pEdgeLeft->ErrorDown, pEdgeRight->ErrorUp, pEdgeRight->ErrorDown)))
            {

                INT nSubpixelYAdvance =  nSubpixelYBottomTrapezoids - nSubpixelYCurrent;
                Assert(nSubpixelYAdvance > 0);

                //
                // Compute the edge position at nSubpixelYBottomTrapezoids
                //

                INT nSubpixelXLeftAdjustedBottom;
                INT nSubpixelErrorLeftBottom;
                INT nSubpixelXRightBottom;
                INT nSubpixelErrorRightBottom;

                AdvanceDDAMultipleSteps(
                    IN pEdgeLeft,
                    IN pEdgeRight,
                    IN nSubpixelYAdvance,
                    OUT nSubpixelXLeftAdjustedBottom,
                    OUT nSubpixelErrorLeftBottom,
                    OUT nSubpixelXRightBottom,
                    OUT nSubpixelErrorRightBottom
                    );

                //
                // Adjust the bottom left position by the expand distance for all the math
                // that follows.  Note that since we adjusted the top distance by that
                // same expand distance, this adjustment is equivalent to moving the edges
                // nSubpixelExpandDistanceUpperBound closer together.
                //

                nSubpixelXLeftAdjustedBottom += nSubpixelExpandDistanceUpperBound;

                //
                // Check if the bottom edge crosses.
                //
                // To avoid checking error1/errDown1 and error2/errDown2, we assume the
                // edges cross if nSubpixelXLeftAdjustedBottom == nSubpixelXRightBottom
                // and thus produce a result that is too conservative.
                //

                if (nSubpixelXLeftAdjustedBottom >= nSubpixelXRightBottom)
                {

                    //
                    // At this point, we have the following scenario
                    //
                    //            ____d1____
                    //            \        /   |   |
                    //              \    /     h1  |
                    //                \/       |   | nSubpixelYAdvance
                    //               /  \          |
                    //             /__d2__\        |
                    //
                    // We want to compute h1.  We know that:
                    //
                    //     h1 / nSubpixelYAdvance = d1 / (d1 + d2)
                    //     h1 = nSubpixelYAdvance * d1 / (d1 + d2)
                    //
                    // Now, if we approximate d1 with some d1' <= d1, we get
                    //
                    //     h1 = nSubpixelYAdvance * d1 / (d1 + d2)
                    //     h1 >= nSubpixelYAdvance * d1' / (d1' + d2)
                    //
                    // Similarly, if we approximate d2 with some d2' >= d2, we get
                    //
                    //     h1 >= nSubpixelYAdvance * d1' / (d1' + d2)
                    //        >= nSubpixelYAdvance * d1' / (d1' + d2')
                    //
                    // Since we are allowed to be too conservative with h1 (it can be
                    // less than the actual value), we'll construct such approximations
                    // for simplicity.
                    //
                    // Note that d1' = nSubpixelXTopDistanceLowerBound which we have already
                    // computed.
                    //
                    //      d2 = (x1 + error1/errorDown1) - (x2 + error2/errorDown2)
                    //         = x1 - x2 + error1/errorDown1 - error2/errorDown2
                    //         <= x1 - x2 - error2/errorDown2   , since error1 < 0
                    //         <= x1 - x2 + 1                   , since error2 < 0
                    //         = nSubpixelXLeftAdjustedBottom - nSubpixelXRightBottom + 1
                    //

                    INT nSubpixelXBottomDistanceUpperBound = nSubpixelXLeftAdjustedBottom - nSubpixelXRightBottom + 1;

                    Assert(nSubpixelXTopDistanceLowerBound >= 0);
                    Assert(nSubpixelXBottomDistanceUpperBound > 0);

#if DBG==1
                    INT nDbgPreviousSubpixelXBottomTrapezoids = nSubpixelYBottomTrapezoids;
#endif

                    nSubpixelYBottomTrapezoids =
                        nSubpixelYCurrent +
                        (nSubpixelYAdvance * nSubpixelXTopDistanceLowerBound) /
                        (nSubpixelXTopDistanceLowerBound + nSubpixelXBottomDistanceUpperBound);

#if DBG==1
                    Assert(nDbgPreviousSubpixelXBottomTrapezoids >= nSubpixelYBottomTrapezoids);
#endif

                    if (nSubpixelYBottomTrapezoids < nSubpixelYCurrent + c_nShiftSize)
                    {
                        // We no longer have a trapezoid that is at least one scanline high, so
                        // abort

                        nSubpixelYBottomTrapezoids = nSubpixelYCurrent;
                        goto Cleanup;
                    }
                }
            }
        }
    }

    //
    // Snap to pixel boundary
    //

    nSubpixelYBottomTrapezoids = nSubpixelYBottomTrapezoids & (~c_nShiftMask);

    //
    // Ensure that we are never less than nSubpixelYCurrent
    //

    Assert(nSubpixelYBottomTrapezoids >= nSubpixelYCurrent);

    //
    // Return trapezoid end scan
    //

Cleanup:
    return nSubpixelYBottomTrapezoids;
}


//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::OutputTrapezoids
//
//  Synopsis:
//      Given the current active edge list, output a list of
//      trapezoids.
//
//      _________________________
//     /     /             \     \
//    /     /               \     \
//   /     /                 \     \
//  /_____/___________________\_____\
//  1+1/m
//
// We output a trapezoid where the distance in X is 1+1/m slope on either edge.
// Note that we actually do a linear interpolation for coverage along the
// entire falloff region which comes within 12.5% error when compared to our
// 8x8 coverage output for complex scans.  What is happening here is
// that we are applying a linear approximation to the coverage function
// based on slope.  It is possible to get better linear interpolations
// by varying the expanded region, but it hasn't been necessary to apply
// these quality improvements yet.
//
//-------------------------------------------------------------------------
HRESULT
CHwRasterizer::OutputTrapezoids(
    __inout_ecount(1) CEdge *pEdgeCurrent,
    INT nSubpixelYCurrent, // inclusive
    INT nSubpixelYNext     // exclusive
    )
{
    HRESULT hr = S_OK;
    INT nSubpixelYAdvance;
    float rSubpixelLeftErrorDown;
    float rSubpixelRightErrorDown;
    float rPixelXLeft;
    float rPixelXRight;
    float rSubpixelLeftInvSlope;;
    float rSubpixelLeftAbsInvSlope;
    float rSubpixelRightInvSlope;
    float rSubpixelRightAbsInvSlope;
    float rPixelXLeftDelta;
    float rPixelXRightDelta;

    CEdge *pEdgeLeft = pEdgeCurrent;
    CEdge *pEdgeRight = pEdgeCurrent->Next;

    Assert((nSubpixelYCurrent & c_nShiftMask) == 0);
    Assert(pEdgeLeft->EndY != INT_MIN);
    Assert(pEdgeRight->EndY != INT_MIN);

    //
    // Compute the height our trapezoids
    //

    nSubpixelYAdvance = nSubpixelYNext - nSubpixelYCurrent;

    //
    // Output each trapezoid
    //

    for (;;)
    {
        //
        // Compute x/error for end of trapezoid
        //

        INT nSubpixelXLeftBottom;
        INT nSubpixelErrorLeftBottom;
        INT nSubpixelXRightBottom;
        INT nSubpixelErrorRightBottom;

        AdvanceDDAMultipleSteps(
            IN pEdgeLeft,
            IN pEdgeRight,
            IN nSubpixelYAdvance,
            OUT nSubpixelXLeftBottom,
            OUT nSubpixelErrorLeftBottom,
            OUT nSubpixelXRightBottom,
            OUT nSubpixelErrorRightBottom
            );

        // The above computation should ensure that we are a simple
        // trapezoid at this point

        Assert(nSubpixelXLeftBottom <= nSubpixelXRightBottom);

        // We know we have a simple trapezoid now.  Now, compute the end of our current trapezoid

        Assert(nSubpixelYAdvance > 0);

        //
        // Computation of edge data
        //

        rSubpixelLeftErrorDown  = static_cast<float>(pEdgeLeft->ErrorDown);
        rSubpixelRightErrorDown = static_cast<float>(pEdgeRight->ErrorDown);
        rPixelXLeft  = ConvertSubpixelXToPixel(pEdgeLeft->X, pEdgeLeft->Error, rSubpixelLeftErrorDown);
        rPixelXRight = ConvertSubpixelXToPixel(pEdgeRight->X, pEdgeRight->Error, rSubpixelRightErrorDown);

        rSubpixelLeftInvSlope     = static_cast<float>(pEdgeLeft->Dx) + static_cast<float>(pEdgeLeft->ErrorUp)/rSubpixelLeftErrorDown;
        rSubpixelLeftAbsInvSlope  = fabsf(rSubpixelLeftInvSlope);
        rSubpixelRightInvSlope    = static_cast<float>(pEdgeRight->Dx) + static_cast<float>(pEdgeRight->ErrorUp)/rSubpixelRightErrorDown;
        rSubpixelRightAbsInvSlope = fabsf(rSubpixelRightInvSlope);

        rPixelXLeftDelta  = 0.5f + 0.5f * rSubpixelLeftAbsInvSlope;
        rPixelXRightDelta = 0.5f + 0.5f * rSubpixelRightAbsInvSlope;

        float rPixelYTop         = ConvertSubpixelYToPixel(nSubpixelYCurrent);
        float rPixelYBottom      = ConvertSubpixelYToPixel(nSubpixelYNext);

        float rPixelXBottomLeft  = ConvertSubpixelXToPixel(
                                        nSubpixelXLeftBottom,
                                        nSubpixelErrorLeftBottom,
                                        static_cast<float>(pEdgeLeft->ErrorDown)
                                        );

        float rPixelXBottomRight = ConvertSubpixelXToPixel(
                                        nSubpixelXRightBottom,
                                        nSubpixelErrorRightBottom,
                                        static_cast<float>(pEdgeRight->ErrorDown)
                                        );

        //
        // Output the trapezoid
        //

        IFC(m_pIGeometrySink->AddTrapezoid(
            rPixelYTop,              // In: y coordinate of top of trapezoid
            rPixelXLeft,             // In: x coordinate for top left
            rPixelXRight,            // In: x coordinate for top right
            rPixelYBottom,           // In: y coordinate of bottom of trapezoid
            rPixelXBottomLeft,       // In: x coordinate for bottom left
            rPixelXBottomRight,      // In: x coordinate for bottom right
            rPixelXLeftDelta,        // In: trapezoid expand radius for left edge
            rPixelXRightDelta        // In: trapezoid expand radius for right edge
            ));

        //
        // Update the edge data
        //

        //  no need to do this if edges are stale

        pEdgeLeft->X      = nSubpixelXLeftBottom;
        pEdgeLeft->Error  = nSubpixelErrorLeftBottom;
        pEdgeRight->X     = nSubpixelXRightBottom;
        pEdgeRight->Error = nSubpixelErrorRightBottom;

        //
        // Check for termination
        //

        if (pEdgeRight->Next->EndY == INT_MIN)
        {
            break;
        }

        //
        // Advance edge data
        //

        pEdgeLeft  = pEdgeRight->Next;
        pEdgeRight = pEdgeLeft->Next;

    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CHwRasterizer::RasterizeEdges
//
//  Synopsis:
//      Rasterize using trapezoidal AA
//
//-------------------------------------------------------------------------
HRESULT
CHwRasterizer::RasterizeEdges(
    __inout_ecount(1) CEdge         *pEdgeActiveList,
    __inout_ecount(1) CInactiveEdge *pInactiveEdgeArray,
    INT nSubpixelYCurrent,
    INT nSubpixelYBottom
    )
{
    HRESULT hr = S_OK;
    CEdge *pEdgePrevious;
    CEdge *pEdgeCurrent;
    INT nSubpixelYNextInactive;
    INT nSubpixelYNext;

    InsertNewEdges(
        pEdgeActiveList,
        nSubpixelYCurrent,
        &pInactiveEdgeArray,
        &nSubpixelYNextInactive
        );

    while (nSubpixelYCurrent < nSubpixelYBottom)
    {
        ASSERTACTIVELIST(pEdgeActiveList, nSubpixelYCurrent);

        //
        // Detect trapezoidal case
        //

        pEdgePrevious = pEdgeActiveList;
        pEdgeCurrent = pEdgeActiveList->Next;

        nSubpixelYNext = nSubpixelYCurrent;

        if (!IsTagEnabled(tagDisableTrapezoids)
            && (nSubpixelYCurrent & c_nShiftMask) == 0
            && pEdgeCurrent->EndY != INT_MIN
            && nSubpixelYNextInactive >= nSubpixelYCurrent + c_nShiftSize
            )
        {
            // Edges are paired, so we can assert we have another one
            Assert(pEdgeCurrent->Next->EndY != INT_MIN);

            //
            // Given an active edge list, we compute the furthest we can go in the y direction
            // without creating self-intersection or going past the edge EndY.  Note that if we
            // can't even go one scanline, then nSubpixelYNext == nSubpixelYCurrent
            //

            nSubpixelYNext = ComputeTrapezoidsEndScan(pEdgeCurrent, nSubpixelYCurrent, nSubpixelYNextInactive);
            Assert(nSubpixelYNext >= nSubpixelYCurrent);

            //
            // Attempt to output a trapezoid.  If it turns out we don't have any
            // potential trapezoids, then nSubpixelYNext == nSubpixelYCurent
            // indicating that we need to fall back to complex scans.
            //

            if (nSubpixelYNext >= nSubpixelYCurrent + c_nShiftSize)
            {
                IFC(OutputTrapezoids(
                    pEdgeCurrent,
                    nSubpixelYCurrent,
                    nSubpixelYNext
                    ));
            }
        }

        //
        // Rasterize simple trapezoid or a complex scanline
        //

        if (nSubpixelYNext > nSubpixelYCurrent)
        {
            // If we advance, it must be by at least one scan line

            Assert(nSubpixelYNext - nSubpixelYCurrent >= c_nShiftSize);

            // Advance nSubpixelYCurrent

            nSubpixelYCurrent = nSubpixelYNext;

            // Remove stale edges.  Note that the DDA is incremented in OutputTrapezoids.

            while (pEdgeCurrent->EndY != INT_MIN)
            {
                if (pEdgeCurrent->EndY <= nSubpixelYCurrent)
                {
                    // Unlink and advance

                    pEdgeCurrent = pEdgeCurrent->Next;
                    pEdgePrevious->Next = pEdgeCurrent;
                }
                else
                {
                    // Advance

                    pEdgePrevious = pEdgeCurrent;
                    pEdgeCurrent = pEdgeCurrent->Next;
                }
            }
        }
        else
        {
            //
            // Trapezoid rasterization failed, so
            //   1) Handle case with no active edges, or
            //   2) fall back to scan rasterization
            //

            if (pEdgeCurrent->EndY == INT_MIN)
            {
                nSubpixelYNext = nSubpixelYNextInactive;
            }
            else
            {
                nSubpixelYNext = nSubpixelYCurrent + 1;
                if (m_fillMode == MilFillMode::Alternate)
                {
                    IFC(m_coverageBuffer.FillEdgesAlternating(pEdgeActiveList, nSubpixelYCurrent));
                }
                else
                {
                    IFC(m_coverageBuffer.FillEdgesWinding(pEdgeActiveList, nSubpixelYCurrent));
                }
            }

            // If the next scan is done, output what's there:
            if (nSubpixelYNext > (nSubpixelYCurrent | c_nShiftMask))
            {
                IFC(GenerateOutputAndClearCoverage(nSubpixelYCurrent));
            }

            // Advance nSubpixelYCurrent
            nSubpixelYCurrent = nSubpixelYNext;

            // Advance DDA and update edge list
            AdvanceDDAAndUpdateActiveEdgeList(nSubpixelYCurrent, pEdgeActiveList);
        }

        //
        // Update edge list
        //

        if (nSubpixelYCurrent == nSubpixelYNextInactive)
        {
            InsertNewEdges(
                pEdgeActiveList,
                nSubpixelYCurrent,
                &pInactiveEdgeArray,
                &nSubpixelYNextInactive
                );
        }
    }

    //
    // Output the last scanline that has partial coverage
    //

    if ((nSubpixelYCurrent & c_nShiftMask) != 0)
    {
        IFC(GenerateOutputAndClearCoverage(nSubpixelYCurrent));
    }

Cleanup:
    RRETURN(hr);
}



