// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*++



Module Name:

    renderdata.cpp

Abstract:

    Implementation of the render data (stream) resource.

Environment:

    User mode only.


--*/

#include "precomp.hpp"

#define RENDERSTREAM_INITIAL_SIZE  0x100

MtDefine(CMilSlaveRenderData, MILRender, "CMilSlaveRenderData");
MtDefine(CMilRenderData_arryHandles, MILRender, "CMilRenderData_arryHandles");
MtDefine(CMilRenderDataDrawFrame, CMilSlaveRenderData, "CMilRenderDataDrawFrame");

CMilSlaveRenderData::CMilSlaveRenderData(
    __in_ecount(1) CComposition* pComposition
    )
{
    m_pComposition = pComposition;
    m_pScheduleRecord = NULL;
}

CMilSlaveRenderData::~CMilSlaveRenderData()
{
    DestroyRenderData();
}

void CMilSlaveRenderData::DestroyRenderData()
{
    {
        UINT iCount = m_rgpResources.GetCount();

        for (UINT i = 0; i < iCount; i++)
        {
            UnRegisterNotifier(m_rgpResources[i]);
        }
        m_rgpResources.Reset(FALSE);
    }

    {
        UINT iCount = m_rgpGuidelineKits.GetCount();

        for (UINT i = 0; i < iCount; i++)
        {
            delete m_rgpGuidelineKits[i];
        }
        m_rgpGuidelineKits.Reset();
    }

    if (m_pScheduleRecord)
    {
        CMilScheduleManager* pScheduleManager = m_pComposition->GetScheduleManager();
        Assert(pScheduleManager);
        pScheduleManager->Unschedule(&m_pScheduleRecord);
    }

    m_instructions.Reset();
}


HRESULT CMilSlaveRenderData::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_RENDERDATA* prd,
    __in_bcount_opt(cbPayload) LPCVOID pPayload,
    UINT cbPayload
    )
{
    HRESULT hr = S_OK;

    //
    // Process packet
    //

    DestroyRenderData();

    if (prd->cbData > 0)
    {
        if (cbPayload != prd->cbData)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        //
        // Allocate a buffer to hold the render-data instructions.
        //

        m_instructions.Reset();
        IFC(m_instructions.AddBlockData(pPayload, cbPayload));

        //
        // Scan though the render data and read out resource handles into m_pbufHandle.
        // The first entry in the handle array is set to zero.
        //
        IFC(GetHandles(pHandleTable));

    }

    NotifyOnChanged(this);

Cleanup:
    if (FAILED(hr))
    {
        //
        // If we can't process the update correctly it means we have invalid
        // instructions, or references to invalid handles. In either case we
        // can't trust this stream to render it.
        //

        DestroyRenderData();
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Class:
//      CRenderDataDrawFrame
//
//  Synopsis:
//      Struct representing one logical frame of a renderdata iteration.
//      As the renderdata iterates the instruction stream, it passes commands on to
//      an IDrawingContext.  There are occasions when we wish to render a sub-portion
//      of the instruction stream into a nested IDrawingContext, and then use those
//      results to pass information into the original IDrawingContext.
//      One example of this is to accumulate bounds for a sub-portion of the renderdata.
//      We create a new bounds render target and associated CDrawingContext, store the
//      old state in a CRenderDataDrawFrame and render to the new target.  When we're
//      finished, we use the CRenderDataDrawFrame to restore the previous state.
//
//-----------------------------------------------------------------------------

class CRenderDataDrawFrame
{
public:
    static HRESULT Create(
        __in_ecount(1) CSwRenderTargetGetBounds *pBoundsRenderTarget,
        __in_ecount(1) CDrawingContext *pDrawingContext,
        __range(>=, 0) int iPrevFrameStackDepth,
        __ecount(1) CRectF<CoordinateSpace::LocalRendering> *prcBounds,
        __in_ecount_opt(1) CRenderDataDrawFrame *pPrev,
        __deref_out CRenderDataDrawFrame** ppNewFrame
        )
    {
        HRESULT hr = S_OK;
        CRenderDataDrawFrame* pNewFrame = NULL;

        Assert((iPrevFrameStackDepth >= 0) &&
               (iPrevFrameStackDepth <= INT_MAX) &&
               (ppNewFrame != NULL));

        IFCOOM(pNewFrame = new CRenderDataDrawFrame(pBoundsRenderTarget,
                                                    pDrawingContext,
                                                    iPrevFrameStackDepth,
                                                    prcBounds,
                                                    pPrev));

        pNewFrame->AddRef();

        *ppNewFrame = pNewFrame;

    Cleanup:
        RRETURN(hr);
    };

    ULONG AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }

    ULONG Release()
    {
        AssertConstMsgW(
            m_cRef != 0,
            L"Attempt to release an object with 0 references! Possible memory leak."
            );

        ULONG cRef = InterlockedDecrement(&m_cRef);

        if (0 == cRef)
        {
            delete this;
        }

        return cRef;
    }

    __ecount(1) CSwRenderTargetGetBounds* GetBoundsRenderTargetNoRef() const
    {
        return m_pBoundsRenderTarget;
    }

    __ecount(1) CDrawingContext* GetDrawingContextNoRef() const
    {
        return m_pDrawingContext;
    }

    __range(>=, 0) int GetPrevFrameStackDepth() const
    {
        return m_iPrevFrameStackDepth;
    }

    __ecount(1) CRectF<CoordinateSpace::LocalRendering>* GetBoundsPtr() const
    {
        return m_prcBounds;
    }

    __ecount_opt(1) CRenderDataDrawFrame* GetPreviousFrame() const
    {
        if (m_pPrev != NULL)
        {
            m_pPrev->AddRef();
        }

        return m_pPrev;
    }

private:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMilRenderDataDrawFrame));

    CRenderDataDrawFrame(
        __in_ecount(1) CSwRenderTargetGetBounds *pBoundsRenderTarget,
        __in_ecount(1) CDrawingContext *pDrawingContext,
        __range(>=, 0) int iPrevFrameStackDepth,
        __ecount(1) CRectF<CoordinateSpace::LocalRendering> *prcBounds,
        __in_ecount_opt(1) CRenderDataDrawFrame *pPrev
        ): m_pBoundsRenderTarget(pBoundsRenderTarget),
           m_pDrawingContext(pDrawingContext),
           m_iPrevFrameStackDepth(iPrevFrameStackDepth),
           m_prcBounds(prcBounds),
           m_pPrev(pPrev),
           m_cRef(0)
    {
        Assert((iPrevFrameStackDepth >= 0) && (iPrevFrameStackDepth <= INT_MAX));

        m_pBoundsRenderTarget->AddRef();
        m_pDrawingContext->AddRef();

        if (m_pPrev != NULL)
        {
            m_pPrev->AddRef();
        }
    };

    ~CRenderDataDrawFrame()
    {
        ReleaseInterfaceNoNULL(m_pBoundsRenderTarget);
        ReleaseInterfaceNoNULL(m_pDrawingContext);
        ReleaseInterfaceNoNULL(m_pPrev);
    }

    __field_ecount(1) CSwRenderTargetGetBounds * const m_pBoundsRenderTarget;
    __field_ecount(1) CDrawingContext * const m_pDrawingContext;
    __field_range(>=, 0) const int m_iPrevFrameStackDepth;
    __field_ecount(1) CRectF<CoordinateSpace::LocalRendering> * const m_prcBounds;
    __field_ecount_opt(1) CRenderDataDrawFrame * const m_pPrev;

    __field_range(>=, 0) LONG m_cRef;
};

//---------------------------------------------------------------------------------
// CMilSlaveRenderData::BeginNewBoundingFrame
//
//    This function begins a new bounding frame inside ::Draw, including calling
//    BeginFrame on the new CDrawingContext.
//    Upon success, it will update iCurrentFrameStackDepth, pCurrentFrame and pCurrentDC.
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT
CMilSlaveRenderData::BeginBoundingFrame(
    __deref_in_range(>=, 0) __deref_out_range(==,0) int *piCurrentFrameStackDepth,
    __ecount(1) CRectF<CoordinateSpace::LocalRendering> *prcBounds,
    __deref_inout_ecount_inopt(1)  CRenderDataDrawFrame **ppCurrentFrame,
    __deref_inout_ecount_inopt(1)  IDrawingContext **ppCurrentDC
    )
{
    HRESULT hr = S_OK;

    CSwRenderTargetGetBounds *pBoundsRenderTarget = NULL;
    CDrawingContext *pDrawingContext = NULL;
    CRenderDataDrawFrame *pNewFrame = NULL;
    CRenderDataDrawFrame *pOldFrame = NULL;

    Assert((piCurrentFrameStackDepth != NULL) &&
           (ppCurrentFrame != NULL) &&
           (ppCurrentDC != NULL));

    // Transfer the ref to pOldFrame
    ReplaceInterface(pOldFrame, *ppCurrentFrame);

    // Instantiate bounds render target & render context
    IFC(CSwRenderTargetGetBounds::Create(&pBoundsRenderTarget));
    IFC(CDrawingContext::Create(m_pComposition, &pDrawingContext));

    IFC(CRenderDataDrawFrame::Create(pBoundsRenderTarget,
                                     pDrawingContext,
                                     *piCurrentFrameStackDepth,
                                     prcBounds,
                                     pOldFrame,
                                     &pNewFrame));

    IFC(pDrawingContext->BeginFrame(pBoundsRenderTarget
                                    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::PageInPixels)
                                    ));

    // At this point we've succeeded, update out params
    *piCurrentFrameStackDepth = 0;

    // Transfer the ref to pCurrentFrame
    ReplaceInterface(*ppCurrentFrame, pNewFrame);

    // IDrawingContext cannot maintain a reference (IDrawingContext doesn't have AddRef/Release.
    // The construction of a new CRenderDataDrawFrame caused the new frame to take a reference,
    // which is what will keep this object alive.
    *ppCurrentDC = pDrawingContext;

Cleanup:

    ReleaseInterface(pDrawingContext);
    ReleaseInterface(pBoundsRenderTarget);
    ReleaseInterface(pOldFrame);
    ReleaseInterface(pNewFrame);

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CMilSlaveRenderData::EndBoundingFrame
//
//    This function complets a bounding frame inside ::Draw, including calling
//    EndFrame on the new CDrawingContext.
//    Upon success, it will update iCurrentFrameStackDepth, pCurrentFrame and pCurrentDC.
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT
CMilSlaveRenderData::EndBoundingFrame(
    __inout_ecount(1) int *piCurrentFrameStackDepth,
    __deref_inout_ecount_outopt(1) CRenderDataDrawFrame **ppCurrentFrame,
    __deref_out_ecount_opt(1) IDrawingContext **ppCurrentDC,
    __in_ecount(1) IDrawingContext *pOriginalDC
    )
{
    HRESULT hr = S_OK;

    Assert((piCurrentFrameStackDepth != NULL) &&
           (ppCurrentFrame != NULL) &&
           (ppCurrentDC != NULL));

    // Grab the bounder
    CSwRenderTargetGetBounds *pBounderNoRef = (*ppCurrentFrame)->GetBoundsRenderTargetNoRef();

    // Grab the current DC from the frame (this should be same as pCurrentDC,
    // but pCurrentDC is an IDrawingContext, and doesn't support EndFrame or
    // Release...)
    CDrawingContext *pCurrentCDCNoRef = (*ppCurrentFrame)->GetDrawingContextNoRef();

    Assert(*ppCurrentDC == static_cast<IDrawingContext*>(pCurrentCDCNoRef));

    // Grab the previous frame depth
    int iPrevFrameStackepth = (*ppCurrentFrame)->GetPrevFrameStackDepth();

    // And the previous Frame (if present)
    CRenderDataDrawFrame *pPrev = (*ppCurrentFrame)->GetPreviousFrame();

    // Grab the location for the bounding information
    CRectF<CoordinateSpace::LocalRendering> *prcBounds = (*ppCurrentFrame)->GetBoundsPtr();

    *prcBounds =
        CRectF<CoordinateSpace::LocalRendering>::ReinterpretNonSpaceTyped
        (pBounderNoRef->GetAccumulatedBounds());

    // In case of error, set empty
    if (!(prcBounds->IsWellOrdered()))
    {
        prcBounds->SetEmpty();
    }

    // End the current frame.
    pCurrentCDCNoRef->EndFrame();

    // We can now release the drawing context, bounder and frame
    ReleaseInterface(*ppCurrentFrame);

    // Reset local variables

    // Update pCurrentFrame to the previous frame, transferring the reference
    *ppCurrentFrame = pPrev;
    pPrev = NULL;

    // Reset pCurrentDC to tbe previous IDrawingContext.
    if (*ppCurrentFrame == NULL)
    {
        *ppCurrentDC = pOriginalDC;
    }
    else
    {
        *ppCurrentDC = (*ppCurrentFrame)->GetDrawingContextNoRef();
    }

    // Restore the previous frame stack depth
    *piCurrentFrameStackDepth = iPrevFrameStackepth;

    // Finally, we need to add the bounds we've just calculated to the parent DC (if present)
    if (*ppCurrentDC != NULL)
    {
        MilPointAndSizeD rectd;
        MilColorF white = {1.0, 1.0, 1.0, 1.0};
        __ecount_opt(1) CMilSolidColorBrushDuce *pTempBrush = NULL;

        MilPointAndSizeDFromMilRectF(reinterpret_cast<MilPointAndSizeD&>(rectd),  *prcBounds);

        MIL_THR(CMilSolidColorBrushDuce::CreateFromColor(&pTempBrush,
                                                         m_pComposition,
                                                         white));

        if (SUCCEEDED(hr))
        {
            MIL_THR((*ppCurrentDC)->DrawRectangle(rectd,
                                                  NULL,  // pen
                                                  pTempBrush,
                                                  NULL)); // rect animations
        }

        ReleaseInterface(pTempBrush);
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CMilSlaveRenderData::Draw
//
//    This function enumerates the drawing instructions into the given
//    IDrawingContext interface.
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT
CMilSlaveRenderData::Draw(
    __in_ecount(1) IDrawingContext *pIDC
    )
{
    HRESULT hr = S_OK;

    CMilSlaveResource **rgpResources = m_rgpResources.GetDataBuffer();
//
//[pfx_parse] -worcaround for PREfix parse problems
//
#if ((defined(_PREFIX_))||(defined(_PREFAST_)))&&(_MSC_VER<1400)
    UINT cResources = m_rgpResources.GetCount();
    UINT cGuidelinesKits = m_rgpGuidelineKits.GetCount();
#else //!_PREFIX_
#if DBG_ANALYSIS
    UINT cResources = m_rgpResources.GetCount();
    UINT cGuidelinesKits = m_rgpGuidelineKits.GetCount();
#endif
#endif //!_PREFIX_

    //
    // Set up the command enumeration.
    //

    CMilDataBlockReader cmdReader(m_instructions.FlushData());

    UINT nItemID;
    PVOID pItemData;
    UINT nItemDataSize;

    //
    // These pointers maintain the original DC and the current DC to support nesting of
    // DC frames.  Neither of these variables maintain reference counts (note that IDrawingContext
    // doesn't implement AddRef/Release) - nested DCs are maintained by the chain of
    // CRenderDataDrawFrame pointed to by pCurrentFrame.
    //

    IDrawingContext *pOriginalDC = pIDC;
    IDrawingContext *pCurrentDC = pOriginalDC;

    CRenderDataDrawFrame *pCurrentFrame = NULL;
    int iCurrentFrameStackDepth = 0;

    //
    // Now get the first item and start executing the render buffer.
    //

    IFC(cmdReader.GetFirstItemSafe(&nItemID, &pItemData, &nItemDataSize));

    //
    // Following is a trap to detect code pieces that break FPU state
    //

    CFloatFPU::AssertPrecisionAndRoundingMode();

    while (hr == S_OK)
    {
        //  Improve lazy evaluation of render state
        //   This way is simpler (and less error-prone, which is good for now).
        //   But it causes unnecessary work, e.g. between 2 repeated PopTransform operations.

        pCurrentDC->ApplyRenderState();

        //
        // Dispatch the current command to the appropriate handler routine.
        //

        //
        // Because the render data packets have been validated when received
        // and stored, we don't need to check that they are the correct size and that
        // the index ranges are valid for each walk of the data. However keep
        // these checks as asserts to guard this assumption.
        //

        if (SUCCEEDED(hr))
        {
            switch (nItemID)
            {
                default:
                    hr = THR(WGXERR_UCE_MALFORMEDPACKET);
                    break;

                case MilPushEffect:
                {
                    // Effectively do nothing, API has been disabled
                    IFC(pCurrentDC->PushOpacity(1.0, NULL));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    break;
                }
                case MilDrawLine:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_LINE));

                    const MILCMD_DRAW_LINE *pData =
                        reinterpret_cast<MILCMD_DRAW_LINE *>(pItemData);

                    Assert(pData->hPen < cResources);

                    IFC(pCurrentDC->DrawLine(
                        pData->point0,
                        pData->point1,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        NULL,
                        NULL
                        ));
                    break;
                }
                case MilDrawLineAnimate:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_LINE_ANIMATE));

                    const MILCMD_DRAW_LINE_ANIMATE *pData =
                        reinterpret_cast<MILCMD_DRAW_LINE_ANIMATE *>(pItemData);

                    Assert (   pData->hPen < cResources
                            && pData->hPoint0Animations < cResources
                            && pData->hPoint1Animations < cResources
                            );

                    IFC(pCurrentDC->DrawLine(
                        pData->point0,
                        pData->point1,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilSlavePoint, rgpResources[pData->hPoint0Animations]),
                        DYNCAST(CMilSlavePoint, rgpResources[pData->hPoint1Animations])
                        ));
                    break;
                }
                case MilDrawRectangle:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_RECTANGLE));

                    MILCMD_DRAW_RECTANGLE *pData =
                        reinterpret_cast<MILCMD_DRAW_RECTANGLE *>(pItemData);
                    Assert(   pData->hBrush < cResources
                           && pData->hPen < cResources);

                    IFC(pCurrentDC->DrawRectangle(
                        pData->rectangle,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hBrush]),
                        NULL
                        ));
                    break;
                }
                case MilDrawRectangleAnimate:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_RECTANGLE_ANIMATE));

                    MILCMD_DRAW_RECTANGLE_ANIMATE *pData =
                        reinterpret_cast<MILCMD_DRAW_RECTANGLE_ANIMATE *>(pItemData);
                    Assert(   pData->hBrush < cResources
                           && pData->hPen < cResources
                           && pData->hRectangleAnimations < cResources);

                    IFC(pCurrentDC->DrawRectangle(
                        pData->rectangle,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hBrush]),
                        DYNCAST(CMilSlaveRect, rgpResources[pData->hRectangleAnimations])
                        ));
                    break;
                }
                case MilDrawRoundedRectangle:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_ROUNDED_RECTANGLE));

                    MILCMD_DRAW_ROUNDED_RECTANGLE *pData =
                        reinterpret_cast<MILCMD_DRAW_ROUNDED_RECTANGLE *>(pItemData);
                    Assert(   pData->hBrush < cResources
                           && pData->hPen < cResources);

                    double radiusX = pData->radiusX;    // For alignment
                    double radiusY = pData->radiusY;    // For alignment

                    IFC(pCurrentDC->DrawRoundedRectangle(
                        pData->rectangle,
                        radiusX,
                        radiusY,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hBrush]),
                        NULL,
                        NULL,
                        NULL
                        ));
                    break;
                }
                case MilDrawRoundedRectangleAnimate:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_ROUNDED_RECTANGLE_ANIMATE));

                    MILCMD_DRAW_ROUNDED_RECTANGLE_ANIMATE *pData =
                        reinterpret_cast<MILCMD_DRAW_ROUNDED_RECTANGLE_ANIMATE *>(pItemData);
                    Assert(   pData->hBrush < cResources
                           && pData->hPen < cResources
                           && pData->hRectangleAnimations < cResources
                           && pData->hRadiusXAnimations < cResources
                           && pData->hRadiusYAnimations < cResources);

                    double radiusX = pData->radiusX;    // For alignment
                    double radiusY = pData->radiusY;    // For alignment

                    IFC(pCurrentDC->DrawRoundedRectangle(
                        pData->rectangle,
                        radiusX,
                        radiusY,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hBrush]),
                        DYNCAST(CMilSlaveRect, rgpResources[pData->hRectangleAnimations]),
                        DYNCAST(CMilSlaveDouble, rgpResources[pData->hRadiusXAnimations]),
                        DYNCAST(CMilSlaveDouble, rgpResources[pData->hRadiusYAnimations])
                        ));
                    break;
                }
                case MilDrawEllipse:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_ELLIPSE));

                    MILCMD_DRAW_ELLIPSE *pData =
                        reinterpret_cast<MILCMD_DRAW_ELLIPSE *>(pItemData);

                    Assert(   pData->hBrush < cResources
                           && pData->hPen < cResources);

                    double radiusX = pData->radiusX;    // For alignment
                    double radiusY = pData->radiusY;    // For alignment

                    // No animations, so NULL for last 3 parameters
                    IFC(pCurrentDC->DrawEllipse(
                        pData->center,
                        radiusX,
                        radiusY,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hBrush]),
                        NULL,
                        NULL,
                        NULL
                        ));
                    break;
                }
                case MilDrawEllipseAnimate:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_ELLIPSE_ANIMATE));

                    MILCMD_DRAW_ELLIPSE_ANIMATE *pData =
                        reinterpret_cast<MILCMD_DRAW_ELLIPSE_ANIMATE *>(pItemData);

                    Assert(   pData->hBrush < cResources
                           && pData->hPen < cResources
                           && pData->hCenterAnimations < cResources
                           && pData->hRadiusXAnimations < cResources
                           && pData->hRadiusYAnimations < cResources);

                    double radiusX = pData->radiusX;    // For alignment
                    double radiusY = pData->radiusY;    // For alignment

                    IFC(pCurrentDC->DrawEllipse(
                        pData->center,
                        radiusX,
                        radiusY,
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hBrush]),
                        DYNCAST(CMilSlavePoint, rgpResources[pData->hCenterAnimations]),
                        DYNCAST(CMilSlaveDouble, rgpResources[pData->hRadiusXAnimations]),
                        DYNCAST(CMilSlaveDouble, rgpResources[pData->hRadiusYAnimations])
                        ));
                    break;
                }
                case MilDrawGeometry:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_GEOMETRY));

                    MILCMD_DRAW_GEOMETRY *pData = reinterpret_cast<MILCMD_DRAW_GEOMETRY *>(pItemData);
                    Assert(   pData->hBrush < cResources
                           && pData->hPen < cResources
                           && pData->hGeometry < cResources);

                    IFC(pCurrentDC->DrawGeometry(
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hBrush]),
                        DYNCAST(CMilPenDuce, rgpResources[pData->hPen]),
                        DYNCAST(CMilGeometryDuce, rgpResources[pData->hGeometry])
                        ));
                    break;
                }
                case MilDrawImage:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_IMAGE));

                    MILCMD_DRAW_IMAGE *pData = reinterpret_cast<MILCMD_DRAW_IMAGE*>(pItemData);
                    Assert(pData->hImageSource < cResources);

                    IFC(pCurrentDC->DrawImage(
                        rgpResources[pData->hImageSource],
                        &pData->rectangle,
                        NULL
                        ));
                    break;
                }
                case MilDrawImageAnimate:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_IMAGE_ANIMATE));

                    MILCMD_DRAW_IMAGE_ANIMATE *pDataAnimate = reinterpret_cast<MILCMD_DRAW_IMAGE_ANIMATE*>(pItemData);
                    Assert(   pDataAnimate->hImageSource < cResources
                           && pDataAnimate->hRectangleAnimations < cResources);

                    IFC(pCurrentDC->DrawImage(
                        rgpResources[pDataAnimate->hImageSource],
                        &pDataAnimate->rectangle,
                        DYNCAST(CMilSlaveRect, rgpResources[pDataAnimate->hRectangleAnimations])
                        ));
                    break;
                }
                case MilDrawDrawing:
                {
                    CMilDrawingDuce *pDrawing;

                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_DRAWING));

                    MILCMD_DRAW_DRAWING *pData = reinterpret_cast<MILCMD_DRAW_DRAWING *>(pItemData);

                    Assert(pData->hDrawing < cResources);

                    if (rgpResources[pData->hDrawing])
                    {
                        pDrawing = DYNCAST(CMilDrawingDuce, rgpResources[pData->hDrawing]);
                        Assert(pDrawing);

                        IFC(pCurrentDC->DrawDrawing(pDrawing));
                    }
                    break;
                }
                case MilDrawVideo:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_VIDEO));

                    MILCMD_DRAW_VIDEO *pData = reinterpret_cast<MILCMD_DRAW_VIDEO*>(pItemData);

                    Assert(pData->hPlayer < cResources);

                    IFC(pCurrentDC->DrawVideo(
                        DYNCAST(CMilSlaveVideo, rgpResources[pData->hPlayer]),
                        &(pData->rectangle),
                        NULL
                        ));
                    break;
                }
                case MilDrawVideoAnimate:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_VIDEO_ANIMATE));

                    MILCMD_DRAW_VIDEO_ANIMATE *pDataAnimate = reinterpret_cast<MILCMD_DRAW_VIDEO_ANIMATE*>(pItemData);
                    Assert(   pDataAnimate->hPlayer < cResources
                           && pDataAnimate->hRectangleAnimations < cResources);

                    IFC(pCurrentDC->DrawVideo(
                        DYNCAST(CMilSlaveVideo, rgpResources[pDataAnimate->hPlayer]),
                        &(pDataAnimate->rectangle),
                        DYNCAST(CMilSlaveRect, rgpResources[pDataAnimate->hRectangleAnimations])
                        ));
                    break;
                }
                case MilDrawGlyphRun:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_DRAW_GLYPH_RUN));

                    MILCMD_DRAW_GLYPH_RUN *pData = reinterpret_cast<MILCMD_DRAW_GLYPH_RUN*>(pItemData);
                    Assert(   pData->hForegroundBrush < cResources
                           && pData->hGlyphRun < cResources);

                    IFC(pCurrentDC->DrawGlyphRun(
                        DYNCAST(CMilBrushDuce, rgpResources[pData->hForegroundBrush]),
                        DYNCAST(CGlyphRunResource, rgpResources[pData->hGlyphRun])
                        ));
                    break;
                }
                case MilPushOpacityMask:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_OPACITY_MASK));

                    MILCMD_PUSH_OPACITY_MASK *pData = reinterpret_cast<MILCMD_PUSH_OPACITY_MASK *>(pItemData);

                    Assert(pData->hOpacityMask < cResources);

                    // In the bounding pass, we populate the "bounds" field on pData
                    // by creating a new frame
                    if (pCurrentDC->IsBounding())
                    {
                        IFC(BeginBoundingFrame(
                            IN OUT &iCurrentFrameStackDepth,
                            ReinterpretNonSpaceTypeDUCERectAsLocalRenderingRect(&pData->boundingBoxCacheLocalSpace),
                            IN OUT &pCurrentFrame,
                            IN OUT &pCurrentDC));
                    }
                    // In the rendering pass, we use the "bounds" field on pData as the bounds
                    // passed to PushOpacityMask
                    else
                    {
                        IFC(pCurrentDC->PushOpacityMask(
                            DYNCAST(CMilBrushDuce, rgpResources[pData->hOpacityMask]),
                            ReinterpretNonSpaceTypeDUCERectAsLocalRenderingRect(&pData->boundingBoxCacheLocalSpace)
                            ));

                        // Increment the current frame depth
                        Assert(iCurrentFrameStackDepth < INT_MAX);
                        iCurrentFrameStackDepth++;
                    }

                    break;
                }
                case MilPushOpacity:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_OPACITY));

                    MILCMD_PUSH_OPACITY *pData = reinterpret_cast<MILCMD_PUSH_OPACITY*>(pItemData);

                    // Current opacity value

                    double opacity = pData->opacity;    // For alignment

                    IFC(pCurrentDC->PushOpacity(
                        opacity,
                        NULL
                        ));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    break;
                }
                case MilPushOpacityAnimate:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_OPACITY_ANIMATE));

                    MILCMD_PUSH_OPACITY_ANIMATE *pData = reinterpret_cast<MILCMD_PUSH_OPACITY_ANIMATE*>(pItemData);
                    Assert(pData->hOpacityAnimations < cResources);

                    double opacity = pData->opacity;    // For alignment

                    IFC(pCurrentDC->PushOpacity(
                        opacity,
                        DYNCAST(CMilSlaveDouble, rgpResources[pData->hOpacityAnimations])
                        ));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    break;
                }
                case MilPushTransform:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_TRANSFORM));

                    MILCMD_PUSH_TRANSFORM *pTransform = reinterpret_cast<MILCMD_PUSH_TRANSFORM*>(pItemData);
                    Assert(pTransform->hTransform < cResources);

                    IFC(pCurrentDC->PushTransform(
                        DYNCAST(CMilTransformDuce, rgpResources[pTransform->hTransform])
                        ));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    break;
                }
                case MilPushGuidelineSet:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_GUIDELINE_SET));

                    MILCMD_PUSH_GUIDELINE_SET *pData= reinterpret_cast<MILCMD_PUSH_GUIDELINE_SET*>(pItemData);
                    Assert(pData->hGuidelines < cResources);

                    IFC(pCurrentDC->PushGuidelineCollection(
                        DYNCAST(CMilGuidelineSetDuce, rgpResources[pData->hGuidelines])
                        ));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    break;
                }
                case MilPushGuidelineY1:
                {
                    bool fNeedMoreCycles = false;

                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_GUIDELINE_Y1));

                    MILCMD_PUSH_GUIDELINE_Y1 *pData= reinterpret_cast<MILCMD_PUSH_GUIDELINE_Y1*>(pItemData);
                    UINT index = *(reinterpret_cast<UINT *>(&pData->coordinate));

                    Assert(index < cGuidelinesKits);

                    CGuidelineCollection *pGuidelineCollection = GetGuidelineCollection(index);
                    // pGuidelineCollection == NULL is okay

                    IFC(pCurrentDC->PushGuidelineCollection(
                        pGuidelineCollection,
                        fNeedMoreCycles
                        ));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    if (fNeedMoreCycles)
                    {
                        IFC(ScheduleRender());
                    }
                    break;
                }
                case MilPushGuidelineY2:
                {
                    bool fNeedMoreCycles = false;

                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_GUIDELINE_Y2));

                    MILCMD_PUSH_GUIDELINE_Y2 *pData= reinterpret_cast<MILCMD_PUSH_GUIDELINE_Y2*>(pItemData);
                    UINT index = *(reinterpret_cast<UINT *>(&pData->leadingCoordinate));

                    Assert(index < cGuidelinesKits);

                    CGuidelineCollection *pGuidelineCollection = GetGuidelineCollection(index);
                    // pGuidelineCollection == NULL is okay

                    IFC(pCurrentDC->PushGuidelineCollection(
                        pGuidelineCollection,
                        fNeedMoreCycles
                        ));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    if (fNeedMoreCycles)
                    {
                        IFC(ScheduleRender());
                    }
                    break;
                }
                case MilPushClip:
                {
                    Assert(nItemDataSize == sizeof(MILCMD_PUSH_CLIP));

                    MILCMD_PUSH_CLIP *pClip = reinterpret_cast<MILCMD_PUSH_CLIP*>(pItemData);
                    Assert(pClip->hClipGeometry < cResources);

                    IFC(pCurrentDC->PushClip(
                        DYNCAST(CMilGeometryDuce, rgpResources[pClip->hClipGeometry])
                        ));

                    // Increment the current frame depth
                    Assert(iCurrentFrameStackDepth < INT_MAX);
                    iCurrentFrameStackDepth++;

                    break;
                }
                case MilPop:
                {
                    // Either our frame depth is at least 1 or there needs to be a current frame.
                    Assert((iCurrentFrameStackDepth > 0) || (pCurrentFrame != NULL));


                    // This check determines whether our current frame's stack depth is 0,
                    // in which case we need to pop a frame.
                    // If it's not 0, then we need to decrement.
                    if (iCurrentFrameStackDepth > 0)
                    {
                        iCurrentFrameStackDepth--;
                        IFC(pCurrentDC->Pop());
                    }
                    else
                    {
                        IFC(EndBoundingFrame(
                            IN OUT &iCurrentFrameStackDepth,
                            IN OUT &pCurrentFrame,
                            IN OUT &pCurrentDC,
                            IN pOriginalDC));
                    }

                    break;
                }
            }
        }

        // If a drawing method returns S_FALSE we interupt the execution of the render
        // data.
        if (hr == S_FALSE)
        {
            break;
        }

        //
        // Following is a trap to detect code pieces that break FPU state
        // (see  and others)
        //

        CFloatFPU::AssertPrecisionAndRoundingMode();

        IFC(cmdReader.GetNextItemSafe(
            &nItemID,
            &pItemData,
            &nItemDataSize
            ));
    }

    //
    // S_FALSE means that we reached the end of the stream. Hence we executed the stream
    // correctly and therefore we should return S_OK.
    //

    if (hr == S_FALSE)
    {
        hr = S_OK;
    }

Cleanup:

    AssertMsg((pCurrentFrame == NULL) || FAILED(hr), "We should only need to clean up nested frames in the failure case.");

    ReleaseInterface(pCurrentFrame);

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CMilSlaveRenderData::ScheduleRender
//
//  Synopsis:
//      Guideline helper
//
//-----------------------------------------------------------------------------

HRESULT
CMilSlaveRenderData::ScheduleRender()
{
    CMilScheduleManager* pScheduleManager = m_pComposition->GetScheduleManager();
    Assert(pScheduleManager);
    RRETURN(pScheduleManager->ScheduleRelative(
        this,
        &m_pScheduleRecord,
        CDynamicGuideline::sc_uTimeDelta
        ));
}




