// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//
//      Contains implementation of CSnappingFrame class
//

#include "precomp.hpp"

MtDefine(CGuidelineCollection, MILRender, "CGuidelineCollection");
MtDefine(CSnappingFrame, MILRender, "CSnappingFrame");


//=========================================================================
//
//                          class CGuidelineCollection
//
//=========================================================================

//+------------------------------------------------------------------------
//
//  Member:
//      static CGuidelineCollection::Create
//
//  Synopsis:
//      Create the instance of CStaticGuidelineCollection or
//      CDynamicGuidelineCollection, depending on "fDynamic" argument.
//
//-------------------------------------------------------------------------
HRESULT CGuidelineCollection::Create(
    UINT16 uCountX,
    UINT16 uCountY,
    __in_ecount(uCountX + uCountY) const float *prgData,
    bool fDynamic,
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    return fDynamic
        ?   CDynamicGuidelineCollection::Create(
                uCountX,
                uCountY,
                prgData,
                ppGuidelineCollection
                )
        :   CStaticGuidelineCollection::Create(
                uCountX,
                uCountY,
                prgData,
                ppGuidelineCollection
                )
        ;
}

//+------------------------------------------------------------------------
//
//  Member:
//      static CGuidelineCollection::CreateFromDoubles
//
//  Synopsis:
//      Create the instance of CStaticGuidelineCollection or
//      CDynamicGuidelineCollection, depending on "fDynamic" argument.
//
//-------------------------------------------------------------------------
HRESULT CGuidelineCollection::CreateFromDoubles(
    UINT16 uCountX,
    UINT16 uCountY,
    __in_ecount_opt(uCountX) const double *prgDataX,
    __in_ecount_opt(uCountY) const double *prgDataY,
    BOOL fDynamic,
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    return fDynamic
        ?   CDynamicGuidelineCollection::CreateFromDoubles(
            uCountX,
            uCountY,
            prgDataX,
            prgDataY,
            ppGuidelineCollection
                )
        :   CStaticGuidelineCollection::CreateFromDoubles(
            uCountX,
            uCountY,
            prgDataX,
            prgDataY,
            ppGuidelineCollection
                )
        ;
}

//=========================================================================
//
//                          class CStaticGuidelineCollection
//
//=========================================================================

//+------------------------------------------------------------------------
//
//  Member:
//      static CStaticGuidelineCollection::Create
//
//  Synopsis:
//      Create the instance of CStaticGuidelineCollection, filled with given data.
//      Ensure that coordinate arrays are given in increasing order.
//      If not so, reject with WGXERR_MALFORMED_GUIDELINE_DATA.
//
//-------------------------------------------------------------------------
HRESULT
CStaticGuidelineCollection::Create(
    UINT16 uCountX,
    UINT16 uCountY,
    __in_ecount(uCountX + uCountY) const float *prgData,
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    HRESULT hr = S_OK;

    CStaticGuidelineCollection *pThis = NULL;
    UINT32 uCount = static_cast<UINT32>(uCountX) + static_cast<UINT32>(uCountY);

    Assert(uCount); // Should not create empty collections

    // Create the instance with data memory attached
    {
        void *pMem = WPFAlloc(
            ProcessHeap,
            Mt(CGuidelineCollection),
            sizeof(CStaticGuidelineCollection) + sizeof(float) * uCount
            );
        IFCOOM(pMem);

        pThis = new(pMem) CStaticGuidelineCollection(uCountX, uCountY);
    }

    if (uCountX)
    {
        IFC(pThis->StoreRange(0, uCountX, prgData));
    }

    if (uCountY)
    {
        IFC(pThis->StoreRange(uCountX, uCountY, prgData + uCountX));
    }

    *ppGuidelineCollection = pThis;
    pThis = NULL;

Cleanup:
    delete pThis;
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:
//      static CStaticGuidelineCollection::CreateFromDoubles
//
//  Synopsis:
//      Create the instance of CStaticGuidelineCollection, filled with given data.
//      Ensure that coordinate arrays are given in increasing order.
//      If not so, reject with WGXERR_MALFORMED_GUIDELINE_DATA.
//
//-------------------------------------------------------------------------
HRESULT
CStaticGuidelineCollection::CreateFromDoubles(
    UINT16 uCountX,
    UINT16 uCountY,
    __in_ecount_opt(uCountX) const double *prgDataX,
    __in_ecount_opt(uCountY) const double *prgDataY,
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    HRESULT hr = S_OK;

    CStaticGuidelineCollection *pThis = NULL;
    UINT32 uCount = static_cast<UINT32>(uCountX) + static_cast<UINT32>(uCountY);

    Assert(uCount > 0); // Should not create empty collections

    // Create the instance with data memory attached
    {
        void *pMem = WPFAlloc(
            ProcessHeap,
            Mt(CGuidelineCollection),
            sizeof(CStaticGuidelineCollection) + sizeof(float) * uCount
            );
        IFCOOM(pMem);

        pThis = new(pMem) CStaticGuidelineCollection(uCountX, uCountY);
    }

    if (uCountX)
    {
        IFC(pThis->StoreRangeFromDoubles(0, uCountX, prgDataX));
    }

    if (uCountY)
    {
        IFC(pThis->StoreRangeFromDoubles(uCountX, uCountY, prgDataY));
    }

    *ppGuidelineCollection = pThis;
    pThis = NULL;

Cleanup:
    delete pThis;
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CStaticGuidelineCollection::StoreRange
//
//  Synopsis:
//      Private helper for CStaticGuidelineCollection::Create().
//      Handles the range of guideline coordinate array, either X or Y.
//
//-------------------------------------------------------------------------
HRESULT
CStaticGuidelineCollection::StoreRange(
    UINT uIndex,
    UINT uCount,
    __in_ecount(uCount) const float *prgSrc
    )
{
    Assert(uCount > 0);

    HRESULT hr = S_OK;
    float *prgDst = Data() + uIndex;

    // check prgSrc[0] for NaN
    if (prgSrc[0] != prgSrc[0])
    {
        IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
    }
    prgDst[0] = prgSrc[0];

    for (UINT i = 1; i < uCount; i++)
    {
        // Check for increasing order and NaN.
        if (!(prgSrc[i-1] <= prgSrc[i]))
        {
            IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
        }
        prgDst[i] = prgSrc[i];
    }

Cleanup:
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:
//      CStaticGuidelineCollection::StoreRangeFromDoubles
//
//  Synopsis:
//      Private helper for CStaticGuidelineCollection::CreateFromDoubles().
//      Handles the range of guideline coordinate array, either X or Y.
//
//-------------------------------------------------------------------------
HRESULT
CStaticGuidelineCollection::StoreRangeFromDoubles(
    UINT uIndex,
    UINT uCount,
    __in_ecount(uCount) const double *prgSrc
    )
{
    Assert(uCount > 0);

    HRESULT hr = S_OK;
    float *prgDst = Data() + uIndex;

    // check prgSrc[0] for NaN
    if (prgSrc[0] != prgSrc[0])
    {
        IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
    }
    prgDst[0] = static_cast<float>(prgSrc[0]);

    for (UINT i = 1; i < uCount; i++)
    {
        // Check for increasing order and NaN.
        if (!(prgSrc[i-1] <= prgSrc[i]))
        {
            IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
        }
        prgDst[i] = static_cast<float>(prgSrc[i]);
    }

Cleanup:
    return hr;
}


//=========================================================================
//
//                          class CDynamicGuidelineCollection
//
//=========================================================================

//+------------------------------------------------------------------------
//
//  Member:
//      static CDynamicGuidelineCollection::Create
//
//  Synopsis:
//      Create the instance of CDynamicGuidelineCollection, filled with given data.
//      Ensure that coordinate arrays are given in increasing order.
//      If not so, reject with WGXERR_MALFORMED_GUIDELINE_DATA.
//
//-------------------------------------------------------------------------
HRESULT CDynamicGuidelineCollection::Create(
    UINT16 uCountX,
    UINT16 uCountY,
    __in_ecount(uCountX + uCountY) const float *prgData,
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    HRESULT hr = S_OK;

    CDynamicGuidelineCollection *pThis = NULL;

    UINT16 uCountXPairs = uCountX >> 1;
    UINT16 uCountYPairs = uCountY >> 1;
    UINT32 uCount = static_cast<UINT32>(uCountXPairs) + static_cast<UINT32>(uCountYPairs);

    Assert(uCount); // Should not create empty collections

    // Create the instance with data memory attached
    {
        void *pMem = WPFAlloc(
            ProcessHeap,
            Mt(CGuidelineCollection),
            sizeof(CDynamicGuidelineCollection) + sizeof(CDynamicGuideline) * uCount
            );
        IFCOOM(pMem);

        pThis = new(pMem) CDynamicGuidelineCollection(uCountXPairs, uCountYPairs);
    }

    if (uCountXPairs)
    {
        IFC(pThis->StoreRange(0, uCountXPairs, prgData));
    }

    if (uCountYPairs)
    {
        IFC(pThis->StoreRange(uCountXPairs, uCountYPairs, prgData + uCountX));
    }

    *ppGuidelineCollection = pThis;
    pThis = NULL;

Cleanup:
    delete pThis;
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:
//      static CDynamicGuidelineCollection::CreateFromDoubles
//
//  Synopsis:
//      Create the instance of CDynamicGuidelineCollection, filled with given data.
//      Ensure that coordinate arrays are given in increasing order.
//
//-------------------------------------------------------------------------
HRESULT CDynamicGuidelineCollection::CreateFromDoubles(
    UINT16 uCountX,
    UINT16 uCountY,
    __in_ecount_opt(uCountX) const double *prgDataX,
    __in_ecount_opt(uCountY) const double *prgDataY,
    __deref_out_ecount(1) CGuidelineCollection **ppGuidelineCollection
    )
{
    HRESULT hr = S_OK;

    CDynamicGuidelineCollection *pThis = NULL;

    UINT16 uCountXPairs = uCountX >> 1;
    UINT16 uCountYPairs = uCountY >> 1;
    UINT32 uCount = static_cast<UINT32>(uCountXPairs) + static_cast<UINT32>(uCountYPairs);

    Assert(uCount); // Should not create empty collections

    // Create the instance with data memory attached
    {
        void *pMem = WPFAlloc(
            ProcessHeap,
            Mt(CGuidelineCollection),
            sizeof(CDynamicGuidelineCollection) + sizeof(CDynamicGuideline) * uCount
            );
        IFCOOM(pMem);

        pThis = new(pMem) CDynamicGuidelineCollection(uCountXPairs, uCountYPairs);
    }

    if (uCountXPairs)
    {
        IFC(pThis->StoreRangeFromDoubles(0, uCountXPairs, prgDataX));
    }

    if (uCountYPairs)
    {
        IFC(pThis->StoreRangeFromDoubles(uCountXPairs, uCountYPairs, prgDataY));
    }

    *ppGuidelineCollection = pThis;
    pThis = NULL;

Cleanup:
    delete pThis;
    return hr;
}


//+------------------------------------------------------------------------
//
//  Member:
//      CDynamicGuidelineCollection::StoreRange
//
//  Synopsis:
//      Private helper for CDynamicGuidelineCollection::Create().
//      Handles the range of guideline coordinate array, either X or Y.
//
//-------------------------------------------------------------------------
HRESULT
CDynamicGuidelineCollection::StoreRange(
    UINT16 uIndex,
    UINT16 uCount,
    __in_ecount(2*uCount) const float *prgSrc
    )
{
    Assert(uCount > 0);

    HRESULT hr = S_OK;
    CDynamicGuideline *prgDst = Data() + uIndex;

    // check prgSrc[0] && prgSrc[1] for NaNs
    if (prgSrc[0] != prgSrc[0] ||
        prgSrc[1] != prgSrc[1])
    {
        IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
    }
    new (&prgDst[0]) CDynamicGuideline(prgSrc[0], prgSrc[1]);

    for (UINT16 k = 1; k < uCount; k++)
    {
        UINT i = 2*k;
        // Check for increasing order and NaN.
        if (!(prgSrc[i-2] + prgSrc[i-1] <= prgSrc[i] + prgSrc[i]))
        {
            IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
        }
        new (&prgDst[k]) CDynamicGuideline(prgSrc[i], prgSrc[i+1]);
    }

Cleanup:
    return hr;
}



//+------------------------------------------------------------------------
//
//  Member:
//      CDynamicGuidelineCollection::StoreRangeFromDoubles
//
//  Synopsis:
//      Private helper for CDynamicGuidelineCollection::CreateFromDoubles().
//      Handles the range of guideline coordinate array, either X or Y.
//
//-------------------------------------------------------------------------
HRESULT
CDynamicGuidelineCollection::StoreRangeFromDoubles(
    UINT16 uIndex,
    UINT16 uCount,
    __in_ecount(2*uCount) const double *prgSrc
    )
{
    Assert(uCount > 0);

    HRESULT hr = S_OK;
    CDynamicGuideline *prgDst = Data() + uIndex;

    // check prgSrc[0] and prgSrc[1] for NaNs
    if (prgSrc[0] != prgSrc[0] ||
        prgSrc[1] != prgSrc[1])
    {
        IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
    }
    new (&prgDst[0]) CDynamicGuideline(
        static_cast<float>(prgSrc[0]),
        static_cast<float>(prgSrc[1])
        );

    for (UINT16 k = 1; k < uCount; k++)
    {
        UINT i = 2*k;
        // Check for increasing order and NaN.
        if (!(prgSrc[i-2] + prgSrc[i-1] <= prgSrc[i] + prgSrc[i]))
        {
            IFC(WGXERR_MALFORMED_GUIDELINE_DATA);
        }
        new (&prgDst[k]) CDynamicGuideline(
            static_cast<float>(prgSrc[i]),
            static_cast<float>(prgSrc[i+1])
            );
    }

Cleanup:
    return hr;
}



//=========================================================================
//
//                          class CSnappingFrame
//
//=========================================================================




//+------------------------------------------------------------------------
//
//  Member:
//      static CSnappingFrame::PushFrame
//
//  Synopsis:
//      Create an instance of one of CSnappingFrame derivatives and attach
//      it to frame stack.
//  
//      When given pGuidelineCollection == NULL then empty frame is created.
//
//      Given transformation is checked for being only scale and translate.
//      If not so, we also create empty frame.
//
//      Otherwise, the size of arrays in pGuidelineCollection are inspected.
//      When possible, optimized transform-based snapping frame is used.
//
// *****
//
// The argument "fNeedMoreCycles" makes sense for dynamic guidelines only.
// Dynamic guideline collection might happen to be in progress of moving
// from animated (non-snapped) position to stabilized (snapped) one.
// To complete the transition, this routine should be called, maybe several
// times, even if nothing changes in the scene. Caller is reponsible for
// scheduling additional rendering pass after getting fNeedMoreCycles = true.
//
//-------------------------------------------------------------------------
HRESULT
CSnappingFrame::PushFrame(
    __inout_ecount_opt(1) CGuidelineCollection *pGuidelineCollection,
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat,
    UINT uCurrentTime,
    __out_ecount(1) bool & fNeedMoreCycles,
    bool fSuppressAnimation,
    __deref_inout_ecount(1) CSnappingFrame **ppSnappingStack
    )
{
    Assert(ppSnappingStack);

    HRESULT hr = S_OK;

    UINT16 uCountX = 0;
    UINT16 uCountY = 0;

    CStaticGuidelineCollection *pStaticGuidelineCollection = NULL;
    CDynamicGuidelineCollection *pDynamicGuidelineCollection = NULL;

    fNeedMoreCycles = false;

    if (pGuidelineCollection != NULL)
    {
        pStaticGuidelineCollection = pGuidelineCollection->CastToStatic();
        pDynamicGuidelineCollection = pGuidelineCollection->CastToDynamic();
    }

    // Check whether we need snapping.
    BOOL fTransformIsScaleAndTranslate = mat.IsTranslateOrScale();

    //
    // When fSuppressAnimation is given "true" when guideline set
    // can be involved in several scene paths. For now we can't
    // handle multiple path scenario with animation detection, because
    // it requires storing per-path data (m_uTimeAndPhase, m_rLastGivenCoord
    // and m_rLastOffset in CDynamicGuideline) while we have no room
    // to keep these data (i.e. we dont have a way to identify current
    // path and associate it with some storage that persists from frame to
    // frame). Consequent calls to SubpixelAnimationCorrection()
    // with different transformation matrices might be accepted as animation
    // and cause infinite scene re-rendering.
    //
    // We should consider ways to support multiple path scenario.
    // For now, we use fSuppressAnimation to prohibit subpixel animation
    // correction, as a workaround.
    //

    if (pDynamicGuidelineCollection && !fSuppressAnimation)
    {
        //
        // When current transformation is not not only scale and translate
        // (i.e. involves rotation or skew), pixel snapping does not make sense.
        // We should not call SubpixelAnimationCorrection() but we need
        // to notify guideline collection that this case has been happened
        // so that it'll remember the history proper way.
        
        if (fTransformIsScaleAndTranslate)
        {
            pDynamicGuidelineCollection->SubpixelAnimationCorrection(mat, uCurrentTime, fNeedMoreCycles);
        }
        else
        {
            pDynamicGuidelineCollection->NotifyNonSnappableState(uCurrentTime);
        }
    }

    if (pGuidelineCollection && fTransformIsScaleAndTranslate)
    {
        uCountX = pGuidelineCollection->CountX();
        uCountY = pGuidelineCollection->CountY();
    }

    if (uCountX == 0 && uCountY == 0)
    {
        // handle empty frame push
        IFC( CSnappingFrameEmpty::PushFrame(ppSnappingStack) );
    }
    else
    {
        // Allocate memory slot for the instance of CSnappingFrame, followed by:
        // float GuidesX[uCountX];
        // float SnapsX[uCountX];
        // float GuidesY[uCountY];
        // float SnapsY[uCountY];

        UINT32 uCount = static_cast<UINT32>(uCountX) + static_cast<UINT32>(uCountY);
        void *pMem = WPFAlloc(
            ProcessHeap,
            Mt(CSnappingFrame),
            sizeof(CSnappingFrame) + sizeof(float) * uCount * 2
            );
        IFCOOM(pMem);

        {
            // Create the instance
            CSnappingFrame *pThis =
                new(pMem) CSnappingFrame(uCountX, uCountY);

            // Hook up the instance to the stack list
            pThis->HookupToStack(ppSnappingStack);


            // Populate instance with data
            if (pStaticGuidelineCollection)
            {
                pThis->PushFrameStatic(
                    pStaticGuidelineCollection,
                    mat
                    );
            }
            else
            {
                // We should not get here with NULLs in both
                // pStaticGuidelineCollection and pDynamicGuidelineCollection.
                Assert(pDynamicGuidelineCollection);

                pThis->PushFrameDynamic(
                    pDynamicGuidelineCollection,
                    fSuppressAnimation,
                    mat
                    );
            }
#if DBG
            {
                // snapping offsets should never exceed 1 pixel
                float const *prgOffsetsX = pThis->GuidesX() + pThis->m_uCountX;
                for (UINT i = 0; i < pThis->m_uCountX; i++)
                {
                    Assert(fabs(prgOffsetsX[i]) <= 1.f);
                }

                float const *prgOffsetsY = pThis->GuidesY() + pThis->m_uCountY;
                for (UINT i = 0; i < pThis->m_uCountY; i++)
                {
                    Assert(fabs(prgOffsetsY[i]) <= 1.f);
                }
            }

#endif
        }
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:
//      static CSnappingFrame::PopFrame
//
//  Synopsis:
//      Undo PushFrame().
//
//-------------------------------------------------------------------------
void
CSnappingFrame::PopFrame(
    __deref_inout_ecount(1) CSnappingFrame **ppSnappingStack
    )
{
    Assert(ppSnappingStack);
    CSnappingFrame *pCurrentFrame = *ppSnappingStack;
    if (pCurrentFrame)
    {
        if (pCurrentFrame->IsEmpty())
        {
            CSnappingFrameEmpty *pFrameEmpty = pCurrentFrame->CastToEmpty();

            if (--pFrameEmpty->m_uIdlePushCount)
                return;
        }

        pCurrentFrame->UnhookFromStack(ppSnappingStack);
        delete pCurrentFrame;
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      static CSnappingFrame::StoreRangeStatic
//
//  Synopsis:
//      Helper for CSnappingFrame::PushFrame(), see comments there.
//
//-------------------------------------------------------------------------
void
CSnappingFrame::StoreRangeStatic(
    __out_ecount(2*uCount) float *prgDst,
    __in_ecount(uCount) const float *prgSrc,
    __range(>=, 1) UINT16 uCount,
    float rScale,
    float rOffset
    )
{
    // Take care of increasing order:
    // reverse the array if given rScale is negative.
    int iStep = 1;
    const float *pSrc = prgSrc;
    if (rScale < 0)
    {
        iStep = -1;
        pSrc += (uCount - 1);
    }

    for (UINT16 i = 0; i < uCount; i++, pSrc += iStep)
    {
        float rCoordLocal = *pSrc;
        float rCoordDevice = rScale*rCoordLocal + rOffset;
        float rSnap = CFloatFPU::OffsetToRounded(rCoordDevice);

        prgDst[         i] = rCoordDevice;
        prgDst[i + uCount] = rSnap;
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      static CSnappingFrame::StoreRangeDynamic
//
//  Synopsis:
//      Helper for CSnappingFrame::PushFrame(), see comments there.
//
//-------------------------------------------------------------------------
void
CSnappingFrame::StoreRangeDynamic(
    __out_ecount(2*uCount) float *prgDst,
    __range(>=, 1) UINT16 uCount,
    __in_ecount(uCount) const CDynamicGuideline *prgSrc,
    bool fSuppressAnimation,
    float rScale,
    float rOffset
    )
{
    // Take care of increasing order:
    // reverse the array if given rScale is negative.
    int iStep = 1;
    const CDynamicGuideline *pSrc = prgSrc;
    if (rScale < 0)
    {
        iStep = -1;
        pSrc += (uCount - 1);
    }

    for (UINT i = 0; i < uCount; i++, pSrc += iStep)
    {
        const CDynamicGuideline &guideline = *pSrc;

        float rLeading, rLeadingSnappingOffset;
        if (fSuppressAnimation)
        {
            //
            // Use untransformed coordinate in local space that's
            // same for all the usages in multiple path scenario.
            // Generate snapping offset right here as we do
            // for static guidelines.
            //
            rLeading = guideline.GetLocalCoordinate() * rScale + rOffset;
            rLeadingSnappingOffset = CFloatFPU::OffsetToRounded(rLeading);
        }
        else
        {
            //
            // Use coordinate that's transformed to device space, and
            // snapping offset that's possibly affected with subpixel
            // animation. This works for only one path.
            //
            rLeading = guideline.GetGivenCoordinate();
            rLeadingSnappingOffset = guideline.GetSnappingOffset();
        }

        float rShift = guideline.GetShift() * rScale;
        float rShiftSnappingOffset = CFloatFPU::OffsetToRounded(rShift);

        //
        // Calculate guideline location in device space.
        // It is composed of the coordinate of leading guideline
        // and the shift from leading to driven.
        //
        float rDriven = rLeading + rShift;

        //
        // Calculate snapping offset that will affect all the points in the area
        // surrounding the guideline. It is composed of the snapping offset for leading
        // guideline and the offset for rShift.
        //
        float rDrivenSnappingOffset = rLeadingSnappingOffset + rShiftSnappingOffset;

        // When there is no animation,
        //      rLeading + rLeadingSnappingOffset = integer
        // and
        //      rShift + rShiftSnappingOffset = integer
        // so that
        //      rDriven + rDrivenSnappingOffset = integer.
        //
        // Note that 
        //      rLeading + rLeadingSnappingOffset = Round(rLeading)
        // and
        //      rShift + rShiftSnappingOffset = Round(rShift)
        // but
        //      rDriven + rDrivenSnappingOffset != Round(rDriven).
        //
        // The value of rDrivenSnappingOffset can reach 1 pixel, negative or positive.
        // This is the cost that we pay for desired "gap stabilization".
        //
        //  Example: text + decorator (say, underline).
        //
        //  When rendering the text, we use the guideline with leading coordinate
        //  on text baseline and zero shift. So leading and driven coincide, and
        //  text baseline goes to the pixel boundary.
        //
        //  When rendering the decorator, we use another guideline that has same
        //  leading coordinate as for text, but with some nonzero shift that's
        //  the desired gap between text baseline and decorator's edge.
        //
        //  We need the edge of the decorator to be on pixel boundary also.
        //  The calculation below guarantee that
        //      actualGap = Round(givenGap)
        //  regardless of fractional parts of non-snapped positions for text and
        //  decorator.
        //
        //  Animation will affect rLeadingSnappingOffset so that both text baseline
        //  and decorator's edge will be blurred; however the gap will not be affected.

        //
        // Pack guideline location and snapping offset that will be used
        // in CSnappingFrame::SnapPoint for snapping points of 2d primitives.
        //
        prgDst[         i] = rDriven;
        prgDst[uCount + i] = rDrivenSnappingOffset;
    }
}


//+------------------------------------------------------------------------
//
//  Member:
//      CSnappingFrame::SnapPoint
//
//  Synopsis:
//      Do pixel snapping for given point.
//
//      We look for the pair of vertical and horizontal guidelines that are
//      closest to the point, and use the offsets corresponding to these
//      guidelines to adjust the point.
//
//-------------------------------------------------------------------------
void
CSnappingFrame::SnapPoint(
    __inout_ecount(1) MilPoint2F &point
    )
{
    SnapCoordinate(point.X, m_uCountX, GuidesX());
    SnapCoordinate(point.Y, m_uCountY, GuidesY());
}

//+------------------------------------------------------------------------
//
//  Member:
//      static CSnappingFrame::SnapCoordinate
//
//  Synopsis:
//      Helper for CSnappingFrame::SnapPoint().
//      Handles either X or Y coordinate represented
//      by argument 'z'.
//
//      This function executes the binary search algorithm
//      to detect guideline which coordinate is closest to
//      given coordinate 'z'. Then it changes 'z' by adding
//      the offset value precalculated for this guideline.
//
//      Guideline coordinates and offsets are packed into
//      single array prgData[2*uCount]. First uCount values
//      are guideline coordinates that follow in increasing order:
//          prgData[i] <= prgData[j] when i < j.
//  
//      Remaining uCount values are guideline offsets.
//      Offsets follows in the same order as coordinales, so that
//      for guideline indexed by 'i' coordinate lays in prgData[i]
//      and offset resides in prgData[uCount+i].
//
//-------------------------------------------------------------------------
void
CSnappingFrame::SnapCoordinate(
    __inout_ecount(1) float &z,
    UINT uCount,
    __in_ecount(2*uCount) const float* prgData
    )
{
    if (uCount == 0) return;

    //
    // Following code uses indices 'a' and 'b' to specify
    // the range in given data array. At any moment 'z'
    // lays in this range:
    //   prgData[a] <= z <= prgData[b].
    // The search procedure splits the range (a,b) into ranges
    // (a,c) and (c,b) then chooses one of these parts that contains z.
    // Variables va, vb and vc are the values fetched from data array,
    // so that vi = prgData[i].
    // 

    UINT a = 0;
    float va = prgData[0];

    // Check input coordinate value and array size.
    // If the value is less than prgData[0] then guideline #0 is
    // closest, as far as given data are sorted and prgData[0] is
    // minimum of prgData[i].
    // We also need not search when there is only one guideline in array.
    if (uCount > 1 && z > va)
    {
        UINT b = uCount-1;
        float vb = prgData[b];

        // Check opposite end of range. If given 'z' is greater
        // than maximum guideline coordinate prgData[uCount-1] then use
        // the offset of this guideline.
        if (z > vb)
        {
            a = b;
        }
        else
        {
            // Apply binary search algorithm.
            // It will move indices 'a' and 'b' toward one another
            // so eventually the condition below will stop looping.
            while (b - a > 1)
            {
                // Split the range (a,b) by new index 'c'.
                UINT c = (a + b) >> 1;
                float vc = prgData[c];

                // Look if 'z' lays in (a,c) or (c,b).
                if (z > vc)
                {
                    // 'z' belongs to the range (c,b);
                    // We are interested in this range only;
                    // replace (a,b) with (c,b) and repeat.
                    a = c;
                    va = vc;
                }
                else
                {
                    // 'z' belongs to the range (a,c);
                    // We are interested in this range only;
                    // replace (a,b) with (a,c) and repeat.
                    b = c;
                    vb = vc;
                }
            }

            // We've reached the minimal range (a,b) represented
            // by neighboring guidelines, i.e. b == a+1.
            // Now we need to choose which one is closer to given 'z'.
            if (vb - z < z - va)
            {
                // The guideline 'b' is closer; replace 'a' with 'b'
                // and allow code below to execute snapping.
                a = b;
            }
        }
    }

    // Apply the offset corresponding to guideline indexed by 'a'.
    float offset = prgData[a + uCount];
    z += offset;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CSnappingFrame::SnapTransform
//
//  Synopsis:
//      Performance optimization for simple guideline collection
//      that has not more than one vertical and one horizontal
//      guideline. Instead of handling separate points, we
//      are correcting the matrix transform that used for all the points.
//
//-------------------------------------------------------------------------
void
CSnappingFrame::SnapTransform(
    __inout_ecount(1) CBaseMatrix &mat
    )
{
    Assert(IsSimple());

    if (m_uCountX)
    {
        mat._41 += GuidesX()[m_uCountX];
    }

    if (m_uCountY)
    {
        mat._42 += GuidesY()[m_uCountY];
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      static CSnappingFrameEmpty::PushFrame
//
//  Synopsis:
//      Helper for CSnappingFrame::PushFrame().
//
//-------------------------------------------------------------------------
HRESULT
CSnappingFrameEmpty::PushFrame(
    __deref_opt_inout_ecount(1) CSnappingFrame **ppSnappingStack
    )
{
    HRESULT hr = S_OK;

    CSnappingFrame *pCurrentFrame = *ppSnappingStack;
    if (pCurrentFrame == NULL)
    {
        // Stack is empty, do nothing.
        // We don't care about counting the amount of
        // idle pushes.
    }
    else if (pCurrentFrame->IsEmpty())
    {
        // We already have an instance of CSnappingFrameEmpty
        // on the top of the stack.
        // Don't allocate new one, just increase counter.

        CSnappingFrameEmpty *pEmptyFrame = pCurrentFrame->CastToEmpty();
        pEmptyFrame->m_uIdlePushCount++;
    }
    else
    {
        pCurrentFrame = new CSnappingFrameEmpty;
        IFCOOM(pCurrentFrame);

        // hook up this entry to the stack list
        pCurrentFrame->HookupToStack(ppSnappingStack);
    }

Cleanup:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CSnappingFrame::PushFrameStatic
//
//  Synopsis:
//      Helper for CSnappingFrame::PushFrame().
//      Populates frame with data taken from static guideline collection.
//
//-------------------------------------------------------------------------
void
CSnappingFrame::PushFrameStatic(
    __inout_ecount(1) CStaticGuidelineCollection *pGuidelineCollection,
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat
    )
{
    Assert(pGuidelineCollection);
    Assert(m_uCountX == pGuidelineCollection->CountX());
    Assert(m_uCountY == pGuidelineCollection->CountY());

    if (m_uCountX)
    {
        StoreRangeStatic(GuidesX(), pGuidelineCollection->GuidesX(), m_uCountX, mat._11, mat._41);
    }

    if (m_uCountY)
    {
        StoreRangeStatic(GuidesY(), pGuidelineCollection->GuidesY(), m_uCountY, mat._22, mat._42);
    }
}


//+------------------------------------------------------------------------
//
//  Member:
//      CSnappingFrame::PushFrameDynamic
//
//  Synopsis:
//      Helper for CSnappingFrame::PushFrame().
//      Populates frame with data taken from dynamic guideline collection.
// 
//-------------------------------------------------------------------------
void
CSnappingFrame::PushFrameDynamic(
    __inout_ecount(1) CDynamicGuidelineCollection *pGuidelineCollection,
    bool fSuppressAnimation,
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat
    )
{
    Assert(pGuidelineCollection);
    Assert(m_uCountX == pGuidelineCollection->CountX());
    Assert(m_uCountY == pGuidelineCollection->CountY());

    if (m_uCountX)
    {
        StoreRangeDynamic(
            GuidesX(),
            m_uCountX,
            pGuidelineCollection->GuidesX(),
            fSuppressAnimation,
            mat._11,
            mat._41
            );
    }

    if (m_uCountY)
    {
        StoreRangeDynamic(
            GuidesY(),
            m_uCountY,
            pGuidelineCollection->GuidesY(),
            fSuppressAnimation,
            mat._22,
            mat._42
            );
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDynamicGuidelineCollection::SubpixelAnimationCorrection
//
//  Synopsis:
//      Execute subpixel animation algorithm for every guideline in arrays.
// 
//-------------------------------------------------------------------------
void
CDynamicGuidelineCollection::SubpixelAnimationCorrection(
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &mat,
    UINT uCurrentTime,
    __out_ecount(1) bool & fNeedMoreCycles
    )
{
    CDynamicGuideline* pGuidelinesX = Data();

    for (UINT i = 0, n = CountX(); i < n; i++)
    {
        pGuidelinesX[i].SubpixelAnimationCorrection(mat._11, mat._41, uCurrentTime, fNeedMoreCycles);
    }

    CDynamicGuideline* pGuidelinesY = pGuidelinesX + CountX();
    for (UINT i = 0, n = CountY(); i < n; i++)
    {
        pGuidelinesY[i].SubpixelAnimationCorrection(mat._22, mat._42, uCurrentTime, fNeedMoreCycles);
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDynamicGuidelineCollection::NotifyNonSnappableState
//
//  Synopsis:
//      Notify every guideline that the rendering is happening with a translation
//      that's not scale-and-translation-only.
// 
//-------------------------------------------------------------------------
void
CDynamicGuidelineCollection::NotifyNonSnappableState(
    UINT uCurrentTime
    )
{
    CDynamicGuideline* pGuidelines = Data();
    UINT uCountX = CountX(); // implicit convertion from UINT16
    UINT uCountY = CountY(); // implicit convertion from UINT16

    for (UINT i = 0, n = uCountX + uCountY; i < n; i++)
    {
        pGuidelines[i].NotifyNonSnappableState(uCurrentTime);
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      CDynamicGuideline::SubpixelAnimationCorrection
//
//  Synopsis:
//      Detect animation state and correct guideline position correspondingly.
//      Glyph run animation states are distinguished by animation phases:
//
//      APH_Start: "Start" phase. This is initial state when no history is available.
//         After the very first rendering pass the history becames known,
//         and next, "quiet" phase is established. 
//
//      APH_Quiet: "Quiet" phase. As long as guideline stays at the same place on the
//         screen (possibly being re-rendered many times), we conclude it is 
//         immoveable and so needs to be as crisp as possible. To get that,
//         Y-coordinate of anchor point is snapped to pixel grid. This offset
//         never exceeds +- 1/2 of pixel size. If the location is changed,
//         we are leaving this phase and switch to "animation" one. However
//         the detection is lazy: seldom jumps don't switch. We only consider
//         animation started if two consecutive displacements happened during
//         short time.
//
//      APH_Animation: "Animation" phase. As long as position is changing frequently,
//         we consider animation phase is on. We don't snap to pixel grid
//         during animation phase. If we do, then we'll never get an
//         impression of smooth moving. Animation phase is getting finished
//         when we'll figure out that position has not been changed during
//         some critical time. At this moment we switch to "Landing" phase.
//
//      APH_Landing: "Landing" phase. The purpose of it is smooth transition from
//         animation to quiet. We don't switch at once from origin guideline
//         location to the one that's snapped to pixel grid. If we'll
//         do so, then we'll obtain a jerk that is pretty noticeable and
//         typically is accepted as a bug. Instead, we are making several
//         smaller steps toward snapped position. This takes several frame
//         re-rendering passes during a second or so. Each step is practically
//         not noticeable by human perception, and eventually the guideline
//         is getting settled onto pixel snapped position and we switch back
//         "quiet" phase.
//
//      APH_Flight: "In complicated flight" phase. Current transformation is not
//         scale_and_translation-only, so it is impossible to calculate
//         coordinate in device space. Both m_rLastGivenCoord and m_rLastOffset
//         are unknown so far.
//
//-------------------------------------------------------------------------
void
CDynamicGuideline::SubpixelAnimationCorrection(
    float rScale,
    float rOffset,
    UINT32 uCurrentTime,
    __out_ecount(1) bool & fNeedMoreCycles
    )
{
    static const float sc_rAllowedStep = 0.05f; // pixel
    static const float sc_rBigJumpThreshold = 3.0f; // pixels

    // convert coordinate to device space
    float rNewCoord = m_rCoord * rScale + rOffset;

    switch (GetAnimationPhase())
    {
    case APH_Start:
        {   // "Start" phase: the very first rendering pass.

            // store recent coordinate as given
            m_rLastGivenCoord = rNewCoord;

            // set actual coordinate snapped to pixel grid
            m_rLastOffset = CFloatFPU::OffsetToRounded(rNewCoord);

            SetBumpTime(uCurrentTime);

            // Go to "quiet" phase. We'll never return to "Start" phase.
            SetAnimationPhase(APH_Quiet);
        }
        break;

    case APH_Quiet:
        {   // "Quiet" phase: look what's going on and possibly switch
            // to "Animation" phase.

            // look how far ago we've got previous bump
            bool fBumpedRecently = BumpedRecently(uCurrentTime);

            // look if requested location has been changed
            float rOldCoord = m_rLastGivenCoord;
            bool fBumpedNow = (rOldCoord != rNewCoord);

            if (fBumpedNow)
            {
                // look how far the guideline has been moved
                bool fBigJump = fabs(rNewCoord - rOldCoord) >= sc_rBigJumpThreshold;
                if (fBigJump)
                {
                    // don't animate big jump
                    fBumpedNow = false;

                    // set m_uLastBumpTime so that fBumpedRecently will be false on next frame
                    SetBumpTime(uCurrentTime - sc_uCriticalTime);
                }
                else
                {
                    // remember that we've been bumped
                    SetBumpTime(uCurrentTime);
                }

                m_rLastGivenCoord = rNewCoord;
            }

            // if second bump received during little time, infer animation started
            bool fInAnimation = fBumpedNow && fBumpedRecently;

            if (fInAnimation)
            {
                // goto "animation" phase
                SetAnimationPhase(APH_Animation);

                // don't snap to pixel grid since we're in animation

                // There is a trouble here. Suppose given coordinate stayed at 0.45
                // during long time, then animation started and we've received
                // following sequence: 0.55, 0.65, 0.75, etc.
                // First bump to 0.55 would not be considered as animation so
                // we'll render at 0.00, 1.00, 0.65, 0.75, etc. Thus, with this
                // bad luck, we'll get unpleasant virtual jump from 0.00 to 1.00
                // then back to 0.65. I have no idea how to suppress it. At the
                // moment when 0.55 is received we may suppose that it is just
                // single jump and guideline is going to stay here for a long time.
                // If we'll goto 0.55 immediately on the bump, we'll distort
                // single-jump scenario that will get virtual blur with following
                // landing. I suppose it would be worse than forth-and-back jerk
                // on the beginning of animation.
       
                m_rLastOffset = 0;

                // need more cycles to detect animation finish
                fNeedMoreCycles = true;
            }
            else
            {
                // stay in "quiet" phase

                // snap to pixel grid
                m_rLastOffset = CFloatFPU::OffsetToRounded(rNewCoord);
            }
        }
        break;

    case APH_Animation:
        {   // "Animation" phase: look what's going on, possibly switch
            // to "landing" phase.

            // Look how far ago we've got previous bump.
            bool fBumpedRecently = BumpedRecently(uCurrentTime);

            // look if requested location has been changed
            float rOldCoord = m_rLastGivenCoord;
            bool fBumpedNow = (rOldCoord != rNewCoord);

            if (fBumpedNow)
            {
                // remember that we've been bumped
                SetBumpTime(uCurrentTime);
                m_rLastGivenCoord = rNewCoord;
            }

            // if we've received bump right now or recently, stay animated
            bool fInAnimation = fBumpedNow || fBumpedRecently;

            if (!fInAnimation)
            {
                // goto "landing" phase
                SetAnimationPhase(APH_Landing);
            }


            // don't snap to pixel grid since we're in animation
            m_rLastOffset = 0;

            // need more cycles to detect animation finish
            fNeedMoreCycles = true;
        }
        break;

    case APH_Landing:
        {   // "Landing" phase: smooth transition from "animation" to "quiet".

            // look if requered location has been changed
            float rOldCoord = m_rLastGivenCoord;
            bool fBumpedNow = (rOldCoord != rNewCoord);

            if (fBumpedNow)
            {
                // remember that we've been bumped
                SetBumpTime(uCurrentTime);
                m_rLastGivenCoord = rNewCoord;
            }

            // if we've received bump during "landing", go back to "animation" phase
            bool fInAnimation = fBumpedNow;

            if (fInAnimation)
            {
                // goto "animation" phase
                SetAnimationPhase(APH_Animation);

                // don't snap to pixel grid since we're in animation
                m_rLastOffset = 0;

                // need more cycles to detect animation finish
                fNeedMoreCycles = true;
            }
            else
            {
                // make a step toward snapped position

                // Note 2005/02/01 mikhaill:
                // Previous version used to calculate the step taking
                // into account frame rate and maximal allowed speed.
                // This became irrelevant due to changes in threading
                // model that throttles frame rate so that it is never
                // more 64 frames per second.
                // Now we only have the step value limit.

                float rFinalOffset = CFloatFPU::OffsetToRounded(rNewCoord);

                float rDistance = rFinalOffset - m_rLastOffset;

                if (fabs(rDistance) > sc_rAllowedStep)
                {
                    // make a step
                    m_rLastOffset += static_cast<float>(_copysign(sc_rAllowedStep, rDistance));

                    // stay in "landing" phase
                    fNeedMoreCycles = true;
                }
                else
                {
                    // we've arrived already to snapped position
                    m_rLastOffset = rFinalOffset;

                    // switch to "quiet" phase
                    SetAnimationPhase(APH_Quiet);
                }
            }
        }
        break;

    case APH_Flight:
        {   // "In complicated flight" phase.
            // Both m_rLastGivenCoord and m_rLastOffset are unknown.
            // We do know recent bump time, but it's uncertain how we
            // can make a sense of it.
            // So - do the same things as on APH_Start phase.

            // store recent coordinate as given
            m_rLastGivenCoord = rNewCoord;

            // set actual coordinate snapped to pixel grid
            m_rLastOffset = CFloatFPU::OffsetToRounded(rNewCoord);

            SetBumpTime(uCurrentTime);

            // Go to "quiet" phase. We'll never return to "Start" phase.
            SetAnimationPhase(APH_Quiet);
        }
        break;

    default:
        NO_DEFAULT("Wrong subpixel animation phase");
    }
}


//+------------------------------------------------------------------------
//
//  Member:
//      CDynamicGuideline::NotifyNonSnappableState
//
//  Synopsis:
//      Register the fact that rendering is happening with a translation
//      that's not scale-and-translation-only.
// 
//-------------------------------------------------------------------------
void
CDynamicGuideline::NotifyNonSnappableState(
    UINT uCurrentTime
    )
{
    SetAnimationPhase(APH_Flight);
    SetBumpTime(uCurrentTime);
}





