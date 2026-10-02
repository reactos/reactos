// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      CHwBrushContext implementation.
//      Contains data passed through the pipeline related to hw brush creation.
//

class CHwBrushContext
{
public:
    CHwBrushContext(
        __in_ecount(1) const CContextState* pContextState,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorld2DToSampleSpace,
        MilPixelFormat::Enum fmtTargetSurface,
        BOOL fCanFallback
        ) :
        m_pContextState(pContextState),
        m_fmtTargetSurface(fmtTargetSurface),
        m_matWorld2DToSampleSpace(matWorld2DToSampleSpace),
        m_fCanFallback(fCanFallback)
        DBG_ANALYSIS_COMMA_PARAM(m_fDbgRenderBoundSet(false))
    {
    }

private:
    CHwBrushContext(const CHwBrushContext& t); // Hide copy constructor
    CHwBrushContext& operator=(const CHwBrushContext& t); // Hide assignment operator

    // This object should only be allocated on the stack
    //
    // Illegal allocation operators
    //
    //   These are declared, but not defined such that any use that gets around
    //   the private protection will generate a link time error.
    //

    __allocator __bcount(cb) void * operator new(size_t cb);
    __allocator __bcount(cb) void * operator new[](size_t cb);
    __bcount(cb) void * operator new(size_t cb, __out_bcount(cb) void * pv);

public:
    void SetDeviceRenderingAndSamplingBounds(
        __in_ecount(1) const CMILSurfaceRect &rcRenderingBounds
        )
    {
        WHEN_DBG_ANALYSIS(m_fDbgRenderBoundSet = true);
        m_rcRenderingBounds = rcRenderingBounds;

        Assert(!m_rcRenderingBounds.IsEmpty());
        SetDeviceSamplingBounds(
            CRectF<CoordinateSpace::Device>
            (static_cast<FLOAT>(m_rcRenderingBounds.left),
             static_cast<FLOAT>(m_rcRenderingBounds.top),
             static_cast<FLOAT>(m_rcRenderingBounds.right),
             static_cast<FLOAT>(m_rcRenderingBounds.bottom),
             LTRB_Parameters
             ));
    }

    __returnro const CMILSurfaceRect &GetDeviceRenderingBounds() const
    {
        Assert(m_fDbgRenderBoundSet);
        return m_rcRenderingBounds;
    }

    
    void SetBaseSamplingBounds(
        __in_ecount(1) const CRectF<CoordinateSpace::BaseSampling> &rcBounds
        )
    {
        Assert(m_rcSamplingBounds.DbgCurrentCoordSpace() == CoordinateSpaceId::Invalid);
        m_rcSamplingBounds.BaseSampling() = rcBounds;
    }

    void SetDeviceSamplingBounds(
        __in_ecount(1) const CRectF<CoordinateSpace::Device> &rcRenderingBounds
        )
    {
        Assert(!rcRenderingBounds.IsEmpty());
        Assert(rcRenderingBounds.left + 1.0f <= rcRenderingBounds.right);
        Assert(rcRenderingBounds.top + 1.0f <= rcRenderingBounds.bottom);
        //
        // Set sampling bounds.  Device rendering and "device" sampling bounds
        // have the same scale, but samples are based on center of device
        // pixel.  Therefore deflate the integer-based inclusive-exclusive
        // rectangle by 1/2 pixel.  This will produce an inclusive-inclusive
        // sampling rectangle.  Do NOT use Deflate method because it assumes
        // inclusive-exclusive rectangles.
        //
        // NOTICE-2006/10/04-JasonHa  This assumes target is not multisampled
        // or that multisample rendering is currently disabled.  Otherwise
        // actual deflation would be less because sample points won't be based
        // at center of "the pixel".  For example a 4 sample target can have
        // samples every 0.5 device pixels and thus only have a deflation
        // factor of 0.25.
        //
        Assert(m_rcSamplingBounds.DbgCurrentCoordSpace() == CoordinateSpaceId::Invalid);
        m_rcSamplingBounds.Device().left   = rcRenderingBounds.left   + 0.5f;
        m_rcSamplingBounds.Device().top    = rcRenderingBounds.top    + 0.5f;
        m_rcSamplingBounds.Device().right  = rcRenderingBounds.right  - 0.5f;
        m_rcSamplingBounds.Device().bottom = rcRenderingBounds.bottom - 0.5f;
        Assert(m_rcSamplingBounds.DbgCurrentCoordSpace() == CoordinateSpaceId::Device);
    }


    const CContextState* GetContextStatePtr() const
    {
        return m_pContextState;
    }

    MilPixelFormat::Enum GetFormat() const
    {
        return m_fmtTargetSurface;
    }

    void GetRealizationBoundsAndTransforms(
        __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
        __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> &matBitmapToIdealRealization,
        __out_ecount(1) BitmapToXSpaceTransform &matRealizationToGivenSampleBoundsSpace,
        __out_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds
        ) const;

    void GetRealizationBoundsAndTransforms(
        __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &matRealizationToBaseSampling,
        __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> &matBitmapToIdealRealization,
        __out_ecount(1) BitmapToXSpaceTransform &matRealizationToGivenSampleBoundsSpace,
        __out_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds
        ) const;

    __returnro const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> & GetWorld2DToIdealSamplingSpace() const
    {
        return m_matWorld2DToSampleSpace;
    }

    BOOL CanFallback() const
    {
        return m_fCanFallback;
    }

private:
    const CContextState * const m_pContextState;
    const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &m_matWorld2DToSampleSpace;
    const MilPixelFormat::Enum m_fmtTargetSurface;
    const BOOL m_fCanFallback;
    CMILSurfaceRect m_rcRenderingBounds;
    CMultiSpaceRectF<CoordinateSpace::BaseSampling,CoordinateSpace::Device> m_rcSamplingBounds;

#if DBG_ANALYSIS
    bool m_fDbgRenderBoundSet;
#endif
};



