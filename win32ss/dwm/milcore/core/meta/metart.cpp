// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_meta
//      $Keywords:
//
//  $Description:
//      CMetaRenderTarget - This is a multiple or meta RenderTarget for
//      rendering on multiple devices. It handles enumerating the devices and
//      managing an array of sub-targets. If necessary it is able to hardware
//      accelerate and fall back to software RTs as appropriate.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

//  It is currently acceptable for our callers to call GetDeviceTransform
//  when no RTs are active.  This is useless and should be fixed.
#define SUPPORT_NO_ACTIVE_TARGETS   1

DeclareTag(tagMILRenderDrawCalls, "MILRender", "Trace MILRender drawing calls");

DWORD g_dwCallNo = 0;

void AssertEffectListHasNoAlphaMask(
    __in_ecount_opt(1) const IMILEffectList *pIEffect
    )
{
#if DBG
    HRESULT hr = S_OK;

    if (pIEffect)
    {
        UINT cResources;
        IFC(pIEffect->GetTotalResourceCount(&cResources));
        Assert(cResources == 0);

    }
Cleanup:
    Assert(SUCCEEDED(hr));
#else
    UNREFERENCED_PARAMETER(pIEffect);
#endif
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      CMetaRenderTarget QI helper routine
//
//  Arguments:
//      riid - input IID.
//      ppvObject - output interface pointer.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP
CMetaRenderTarget::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILRenderTarget)
        {
            *ppvObject = static_cast<IMILRenderTarget*>(this);

            hr = S_OK;
        }
        else if (riid == IID_IRenderTargetInternal)
        {
            *ppvObject = static_cast<IRenderTargetInternal*>(this);

            hr = S_OK;
        }
        else
        {
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CMetaRenderTarget constructor.
//
//  Arguments:
//
//------------------------------------------------------------------------------

CMetaRenderTarget::CMetaRenderTarget(
    __out_ecount(cMaxRTs) MetaData *pMetaData,
    UINT cMaxRTs,
    __inout_ecount(1) CDisplaySet const *pDisplaySet
    ) :
    m_cRT(cMaxRTs),
    m_rgMetaData(pMetaData),
    m_pDisplaySet(pDisplaySet)
{
    Assert(cMaxRTs <= pDisplaySet->GetDisplayCount());
    m_fUseRTOffset = FALSE;
    m_fAccumulateValidBounds = false;
    m_pDisplaySet->AddRef();
    ZeroMemory(m_rgMetaData, m_cRT*sizeof(m_rgMetaData[0]));
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CMetaRenderTarget destructor.
//
//------------------------------------------------------------------------------

CMetaRenderTarget::~CMetaRenderTarget()
{
    for (UINT i = 0; i < m_cRT; i++)
    {
        ReleaseInterfaceNoNULL(m_rgMetaData[i].pInternalRT);
    }
    ReleaseInterfaceNoNULL(m_pDisplaySet);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::UpdateValidContentBounds
//
//  Synopsis:
//      If aliased clip touches valid area outside of render bounds then that
//      changed area is removed from valid area.
//
//------------------------------------------------------------------------------

void
CMetaRenderTarget::UpdateValidContentBounds(
    __inout_ecount(1) MetaData &oDevData,
    __in_ecount(1) CAliasedClip const &aliasedDeviceClip
    )
{
    // The valid bounds should always encompass the required Present bounds.
    Assert(oDevData.rcLocalDeviceValidContentBounds.DoesContain(
               oDevData.rcLocalDevicePresentBounds));

    if (aliasedDeviceClip.IsNullClip())
    {
        // All content is valid in this device's render bounds, but nothing
        // more.
        oDevData.rcLocalDeviceValidContentBounds = oDevData.rcLocalDeviceRenderBounds;
    }
    else
    {
        CMilRectF rcClip;

        aliasedDeviceClip.GetAsCMilRectF(&rcClip);

        // Convenience references to avoid large names through out routine
        CMILSurfaceRect const &rcRender = oDevData.rcLocalDeviceRenderBounds;
        CMILSurfaceRect &rcValid = oDevData.rcLocalDeviceValidContentBounds;

        //
        // Determine if any part of current valid area is affected by
        // change.  If so valid area may need reduced.
        //

        CMILSurfaceRect rcValidAreaChanged;

        if (IntersectAliasedBoundsRectFWithSurfaceRect(
                IN  rcClip,
                IN  rcValid,
                OUT &rcValidAreaChanged
                ))
        {
            //
            // Reduce valid content bounds by subtracting change regions that
            // do not intersect render bounds.
            //
            // Subtraction is done by inspecting horizontal/vertical bands
            // affected by change and reducing valid edge to render edge if
            // change band goes beyond render edge.  If change or valid edge is
            // with in render bounds then nothing has to be reduced.
            //

            //
            // Consider the organization of valid (V, - border), change (C, \
            // border), and render (R, * border) rectangles as shown in the
            // diagram below.
            //
            // Also in the diagram is the remaining valid rectangle represented
            // by the region shaded by o's and the critically invalid region
            // represented by shading with -'s.

            //
            //                                           Never valid
            //
            // - - - - - +-------------------+ - - - - - - - - - Valid top
            //           | Valid             |
            //           |                   |           Still valid,
            //           |                   |           but detached
            //           |       Change      |
            // - - - - - | - - / / / / / / / / / / - - - - - - - Change top
            //           |     /- - - - - - -|   /  
            //           |     / - - - - - - |   /       Critically invalid
            //           |     /- - - - - - -|   /  
            // - - - * * | * * * * * * * * * | * * * - - - - - - Render top
            //       *   |o o o/o o o o o o o|   / *
            //       *   | o o / o o o o o o |   / *     Remains valid
            //       *   |o o o/o o o o o o o|   / *
            // - - - *   | o o /o/o/o/o/o/o/o/ / / * - - - - Change bottom
            //       *   |o o o o o o o o o o|     *
            //       *   | o o o o o o o o o |     *     Remains valid
            //       *   |o o o o o o o o o o|     *
            // - - - * - +-------------------+ - - * - - - - Valid bottom
            //       *                             *
            //       *                             *     Never valid
            //       *            Render           *
            // - - - * * * * * * * * * * * * * * * * - - - - Render bottom
            //
            //                                           Never valid
            //

            // With the left, top, right, and bottom edges classified into
            // ordering of most extreme to least extreme we can examine various
            // cases.  (More extreme for right is greater value while more
            // extreme for left is lesser value.)
            //
            // The top bands would be classified as V-C-R, and while two wholly
            // valid bands remain after such an change only the most important
            // band within rendering bounds is kept.  The partial valid band is
            // treated as wholly invalid which allows keeping of a simple valid
            // rectangle instead of a complex region. Thus V'.top = R.top.
            //
            // The bottom bands would be classified as R-V-C.  Since all
            // changes are contained within render bounds there is no change in
            // validity.  V'.bottom = V.bottom.
            //
            // There are only four other possible orderings of edges that have
            // interesting overlaps.  They are diagrammed below.

            //
            //                C-V-R    V'=R
            //
            //        / / / / / / / /
            //        / Change      /
            //        /             /    Valid
            //        /      +------/---------+
            //        /      |- - - /         |
            //        /      | - - -/         |
            //        /   * *|*o*o*o*o*o*o*ooo|
            // C-R-V  /   *  |oooooo/ooooo*ooo|  V-R-C
            //        /   *  |oooooo/ooooo*ooo|
            // V'=V   /   *  |oooooo/ooooo*ooo|  V'=V
            //        /   *  +------/-----*---+
            //        /   *         /     *
            //        /   *         /     *
            //        / / / / / / / /     *
            //            *               *
            //            *        Render *
            //            * * * * * * * * *
            //
            //                R-C-V   V'=V

            // The six cases and result for valid edge are:

            //  C-R-V   V
            //  C-V-R   R
            //  R-C-V   V
            //  R-V-C   V
            //  V-C-R   R
            //  V-R-C   V

            // Which simplifies to R for result when R is the least extreme of
            // edges, independent of C and V ordering.  This can further be   
            // reduced to checking changed-valid edge vs. R edge since
            // changed-valid edges are the least extreme of change and valid
            // edges.  
            //
            //
            // The remaining cases are when render and change do not intersect.
            // In these cases an arbitrary portion of the remaining valid area
            // could be chosen, but for simplicity the entire valid rectangle
            // outside of render bounds is abandonned.  (We know that valid
            // area within render bounds will remain valid because change does
            // not intersect render bounds for these cases.)  The cases
            // expressed in one dimension with result using catergorization
            // result rules from above are:

            //  Key:
            //      <  left edge
            //      >  right edge
            //         (space) no rectangle coverage
            //      -  one rectangle coverage
            //      =  two rectangle coverage (intersection)
            //
            //                       Categorization From ...         Valid/
            //   Range Expression   Left    <V'   Right   V>'  V'    Empty?
            //  -----------------   -----   --    -----   --  -----  ------
            //
            //  <R-R> <C-<V=C>-V>   R-C-V   <V    V-C-R   R>  R><V   empty
            //  <R-R> <C-<V=V>-C>   R-C-V   <V    C-V-R   R>  R><V   empty
            //  <R-R> <V-<C=C>-V>   R-V-C   <V    V-C-R   R>  R><V   empty
            //  <R-R> <V-<C=V>-C>   R-V-C   <V    C-V-R   R>  R><V   empty
            //
            //  <R-<V=R>-<C=C>-V>   R-V-C   <V    V-C-R   R>  <V-R>  valid
            //  <R-<V=R>-<C=V>-C>   R-V-C   <V    C-V-R   R>  <V-R>  valid
            //
            //  <V-<C=C>-<R=V>-R>   V-C-R   <R    R-V-C   V>  <R-V>  valid
            //  <C-<V=C>-<R=V>-R>   C-V-R   <R    R-V-C   V>  <R-V>  valid
            //
            //  <C-<V=C>-V> <R-R>   C-V-R   <R    R-V-C   V>  V><R   empty
            //  <C-<V=V>-C> <R-R>   C-V-R   <R    R-C-V   V>  V><R   empty
            //  <V-<C=C>-V> <R-R>   V-C-R   <R    R-V-C   V>  V><R   empty
            //  <V-<C=V>-C> <R-R>   V-C-R   <R    R-C-V   V>  V><R   empty

            // All of the above detail may be described as:
            //
            //  If changed-valid extends beyond render then limit valid area to
            //  render area.  This may make the valid area empty.  Check for
            //  all edges.
            //

            if (rcValidAreaChanged.left < rcRender.left)
            {
                rcValid.left = rcRender.left;
            }

            if (rcValidAreaChanged.top < rcRender.top)
            {
                rcValid.top = rcRender.top;
            }

            if (rcValidAreaChanged.right > rcRender.right)
            {
                rcValid.right = rcRender.right;
            }

            if (rcValidAreaChanged.bottom > rcRender.bottom)
            {
                rcValid.bottom = rcRender.bottom;
            }
        }
    }

    // Started with valid content bounds encompassing the required
    // Present bounds and they should still be contained.
    Assert(oDevData.rcLocalDeviceValidContentBounds.DoesContain(
               oDevData.rcLocalDevicePresentBounds));
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      Clear - Fills the surface with the given color.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP
CMetaRenderTarget::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,   // background fill color
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    API_ENTRY_NOFPU("CMetaRenderTarget::Clear");

    HRESULT hr = S_OK;

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CAliasedClip aliasedClipAdjusted(NULL);
        if (pAliasedClip)
        {
            aliasedClipAdjusted = *pAliasedClip;
        }

        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,          // pDisplaySet
            &aliasedClipAdjusted,   // pAliasedClip,
            NULL,                   // ppBoundsToAdjust,
            NULL,                   // pTransform,
            NULL,                   // pContextState,
            NULL                    // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->Clear(
                pColor,
                &aliasedClipAdjusted
                ));

            if (m_fAccumulateValidBounds)
            {
                //
                // Update valid content bounds for this sub-RT
                //

                UpdateValidContentBounds(
                    m_rgMetaData[metaIterator.CurrentRT()],
                    aliasedClipAdjusted
                    );
            }

        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    API_CHECK(hr);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::Begin3D
//
//  Synopsis:
//      Prepare for 3D scene within bounds given and clear Z to given value
//

STDMETHODIMP
CMetaRenderTarget::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. Begin 3D\n", ++g_dwCallNo));

    BOOL fSuccessfulStart = FALSE;
    UINT idxLastRTSuccessfullyStarted = 0;

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMilRectF const *prcBounds = CMilRectF::ReinterpretBaseType(&rcBounds);

        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,   // pDisplaySet
            NULL,            // pAliasedClip,
            &prcBounds,      // ppBoundsToAdjust,
            NULL,            // pTransform,
            NULL,            // pContextState,
            NULL             // ppIBitmapSource
            );
        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->Begin3D(
                *prcBounds,
                AntiAliasMode,
                fUseZBuffer,
                rZ
                ));

            // Keep track of success in case a later RT fails and we need to
            // unwind.
            fSuccessfulStart = TRUE;
            idxLastRTSuccessfullyStarted = metaIterator.CurrentRT();
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:

    //
    // Unwind any successes on any failure
    //

    if (FAILED(hr) && fSuccessfulStart)
    {
        for (UINT idx = idxFirstEnabledRT; idx <= idxLastRTSuccessfullyStarted; idx++)
        {
            if (m_rgMetaData[idx].fEnable)
            {
                IGNORE_HR(m_rgMetaData[idx].pInternalRT->End3D());
            }
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::End3D
//
//  Synopsis:
//      End section of 3D rendering.  Rendering accumulated outside true render
//      targets should be composited now.
//

STDMETHODIMP
CMetaRenderTarget::End3D(
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. End 3D\n", ++g_dwCallNo));

    for (UINT i = 0; i < m_cRT; i++)
    {
        // Don't End3D if this RT is not enabled.

        if (m_rgMetaData[i].fEnable)
        {
            MIL_THR_SECONDARY(m_rgMetaData[i].pInternalRT->End3D());
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      GetDeviceTransform - Compute the transform to device space. Note this is
//      assumed to be the device transform for the first sub-RT.
//
//  Return Value:
//      CMILMatrix
//
//------------------------------------------------------------------------------

STDMETHODIMP_(__outro_ecount(1) const CMILMatrix*)
CMetaRenderTarget::GetDeviceTransform() const
{
    Assert(m_cRT > 0);

    UINT i = 0;

    while (!m_rgMetaData[i].fEnable)
    {
        i++;
#if SUPPORT_NO_ACTIVE_TARGETS
        if (i >= m_cRT)
        {
            i = 0;
            while (!m_rgMetaData[i].pInternalRT)
            {
                i++;
                Assert(i < m_cRT);
            }
            break;
        }
#else
        Assert(i < m_cRT);
#endif
    }

    Assert(m_rgMetaData[i].pInternalRT);
    return m_rgMetaData[i].pInternalRT->GetDeviceTransform();
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DrawBitmap - Render a bitmap to the back-buffer according to the
//      transform stack contained in the pContextState.
//
//  Arguments:
//      pContextState - various drawing parameters; matrices, blend mode, etc.
//      pIBitmap      - interface to the actual bitmap.
//      pIEffect      - list of effect objects to apply.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP
CMetaRenderTarget::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    // Assert that an effect has no intermediates. We are not adjusting any
    // meta bitmap render targets that may be stored in an effect list here.
    // Note that we only need to worry about alpha masks since we can't have
    // intermediates any other way
    AssertEffectListHasNoAlphaMask(pIEffect);

    TraceTag((tagMILRenderDrawCalls, "%d. Draw Bitmap\n", ++g_dwCallNo));

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,   // pDisplaySet           
            NULL,            // pAliasedClip,
            NULL,            // ppBoundsToAdjust,
            NULL,            // pTransform,
            pContextState,   // pContextState,
            &pIBitmap        // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->DrawBitmap(
                pContextState,
                pIBitmap,
                pIEffect
                ));
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DrawMesh3D - Render a mesh to the back-buffer. Position coordinates are
//      transformed according to the transform stack contained in the
//      pContextState. Texture coordinates are transformed using the matrix
//      supplied. These transformed texture coordinates should be in the range
//      of 0 to 1. Anything outside this range will be clamped.
//
//  Arguments:
//      pContextState     - various drawing parameters; matrices, blend mode, etc.
//      pMesh3D           - mesh to render
//      pIEffect          - list of effect objects to apply.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP CMetaRenderTarget::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D *pMesh3D,
    __inout_ecount_opt(1) CMILShader *pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    Assert(pShader);            // pShader is opt only for bounds RTs
    
    // Assert that an effect has no intermediates. We are not adjusting any
    // meta bitmap render targets that may be stored in an effect list here.
    // Note that we only need to worry about alpha masks since we can't have
    // intermediates any other way
    AssertEffectListHasNoAlphaMask(pIEffect);

    TraceTag((tagMILRenderDrawCalls, "%d. Draw Mesh3D\n", ++g_dwCallNo));

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,   // pDisplaySet                       
            NULL,            // pAliasedClip,
            NULL,            // ppBoundsToAdjust,
            NULL,            // pTransform,
            pContextState,   // pContextState,
            NULL             // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->DrawMesh3D(
                pContextState,
                pBrushContext,
                pMesh3D,
                pShader,
                pIEffect
                ));
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    pShader->RestoreMetaIntermediates();

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DrawPath - Stroke and/or fill a path to the back-buffer. If a pen is
//      present, the stroke brush is used to stroke the outline of the path. If
//      the fill brush is present, the interior of the path is filled according
//      to the winding mode in the pContextState.
//
//  Arguments:
//      pContextState - various drawing parameters; matrices, blend mode, etc.
//      CShape        - geometry data of the path.
//      pPen          - pen for computing the stroke.
//      pStrokeBrush  - the brush for filling the widened stroke.
//      pFillBrush    - the brush for filling the interior of the path.
//      pIEffect      - list of effect objects to apply.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP
CMetaRenderTarget::DrawPath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) IShapeData *pShape,
    __inout_ecount_opt(1) CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. Draw Path\n", ++g_dwCallNo));

    //
    // Draw the path on each render target
    //

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,  // pDisplaySet           
            NULL,           // pAliasedClip,
            NULL,           // ppBoundsToAdjust,
            NULL,           // pTransform,
            pContextState,  // pContextState,
            NULL            // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->DrawPath(
                pContextState,
                pBrushContext,
                pShape,
                pPen,
                pStrokeBrush,
                pFillBrush
                ));
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    if (pStrokeBrush)
    {
        pStrokeBrush->RestoreMetaIntermediates();
    }
    if (pFillBrush)
    {
        pFillBrush->RestoreMetaIntermediates();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DrawInfinitePath
//
//  Arguments:
//      pContextState - various drawing parameters; matrices, blend mode, etc.
//      CShape        - geometry data of the path.
//      pPen          - pen for computing the stroke.
//      pStrokeBrush  - the brush for filling the widened stroke.
//      pFillBrush    - the brush for filling the interior of the path.
//      pIEffect      - list of effect objects to apply.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP CMetaRenderTarget::DrawInfinitePath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) BrushContext *pBrushContext,
    __inout_ecount(1) CBrushRealizer *pFillBrush
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. Draw All\n", ++g_dwCallNo));

    //
    // Draw on each render target
    //

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,  // pDisplaySet           
            NULL,           // pAliasedClip,
            NULL,           // ppBoundsToAdjust,
            NULL,           // pTransform,
            pContextState,  // pContextState,
            NULL            // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->DrawInfinitePath(
                pContextState,
                pBrushContext,
                pFillBrush
                ));
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    if (pFillBrush)
    {
        pFillBrush->RestoreMetaIntermediates();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      ComposeEffects - Composes the specified effects.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP 
CMetaRenderTarget::ComposeEffect(
    __inout_ecount(1) CContextState *pContextState,
    __in_ecount(1) CMILMatrix *pScaleTransform,
    __inout_ecount(1) CMilEffectDuce* pEffect,
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt IMILRenderTargetBitmap* pImplicitInput
    ) 
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. ComposeEffect\n", ++g_dwCallNo));

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,  // pDisplaySet           
            NULL,           // pAliasedClip,
            NULL,           // ppBoundsToAdjust,
            NULL,           // pTransform,
            pContextState,  // pContextState,
            NULL            // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->ComposeEffect(
                pContextState,
                pScaleTransform,
                pEffect,
                uIntermediateWidth,
                uIntermediateHeight,
                pImplicitInput
                ));
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Function:
//      DrawGlyphs - Render a glyph-run to the back-buffer
//
//  Arguments:
//      pContextState - various drawing parameters; matrices, blend mode, etc.
//      pGlyphRun     - glyphrun object.
//      pBrush        - brush for filling the glyph outlines.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP CMetaRenderTarget::DrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. Draw Glyphs\n", ++g_dwCallNo));

    CRectF<CoordinateSpace::PageInPixels> const rcBoundsOrig(pars.rcBounds.PageInPixels());
    CMilRectF const *prcBounds = &rcBoundsOrig;

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,      // pDisplaySet           
            NULL,               // pAliasedClip,
            &prcBounds,         // ppBoundsToAdjust,
            NULL,               // pTransform,
            pars.pContextState, // pContextState,
            NULL                // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));

            pars.rcBounds.Device() =
                *CRectF<CoordinateSpace::Device>::ReinterpretNonSpaceTyped(prcBounds);

            IFC(pRTInternalNoAddRef->DrawGlyphs(pars));

        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    pars.pBrushRealizer->RestoreMetaIntermediates();

    pars.rcBounds.PageInPixels() = rcBoundsOrig;
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      CreateRenderTargetBitmap - Create a meta bitmap render target
//
//  Arguments:
//      width - width of the desired bitmap
//      height - height of the desired bitmap.
//      ppIRenderTargetBitmap - interface to be returned.
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------

STDMETHODIMP
CMetaRenderTarget::CreateRenderTargetBitmap(
    UINT uWidth,
    UINT uHeight,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
    __in_opt DynArray<bool> const *pActiveDisplays
    )
{
    HRESULT hr = S_OK;

    *ppIRenderTargetBitmap = NULL;

    //
    // Create a meta bitmap render target
    //

    CMetaBitmapRenderTarget *pRT;

    // If the caller specified displays to enable in the new meta RTB,
    // copy the current metadata and enable those specific displays
    // for the new object.  Otherwise, just enable the same displays
    // that this parent object has enabled.
    MetaData *pTempMetaDataCopy = NULL;
    
    if (pActiveDisplays != NULL)
    {
        // sizeof(MetaData) * m_cRT is safe because we already have an array of that size of MetaData.
        pTempMetaDataCopy = reinterpret_cast<MetaData*>WPFAlloc(ProcessHeap, Mt(CMetaBitmapRenderTarget), sizeof(MetaData) * m_cRT);
        IFCOOM(pTempMetaDataCopy);

        for (UINT i = 0; i < m_cRT; i++)
        {
            pTempMetaDataCopy[i] = m_rgMetaData[i];
            pTempMetaDataCopy[i].fEnable = (*pActiveDisplays)[i];
            Assert(pTempMetaDataCopy[i].pInternalRT != NULL);
        }
    }
    
    IFC(CMetaBitmapRenderTarget::Create(
        uWidth,
        uHeight,
        m_cRT,
        (pActiveDisplays != NULL) ? pTempMetaDataCopy : m_rgMetaData,
        m_pDisplaySet,
        usageInfo,
        dwFlags,
        &pRT
        ));

    // Assign return value and steal reference
    *ppIRenderTargetBitmap = pRT;

Cleanup:
    WPFFree(ProcessHeap, pTempMetaDataCopy);

    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::BeginLayer
//
//  Synopsis:
//      Begin accumulation of rendering into a layer.  Modifications to layer,
//      as specified in arguments, are handled and result is applied to render
//      target when the matching EndLayer call is made.
//
//      Calls to BeginLayer may be nested, but other calls that depend on the
//      current contents, such as GetBits or Present, are not allowed until all
//      layers have been resolved with EndLayer.
//

STDMETHODIMP
CMetaRenderTarget::BeginLayer(
    __in_ecount(1) MilRectF const &LayerBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    __in_ecount_opt(1) IShapeData const *pGeometricMask,
    __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
    FLOAT flAlphaScale,
    __in_ecount_opt(1) CBrushRealizer *pAlphaMask
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. Begin Layer\n", ++g_dwCallNo));

    BOOL fSuccessfulStart = FALSE;
    UINT idxLastRTSuccessfullyStarted = 0;

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMilRectF const *pLayerBounds = CMilRectF::ReinterpretBaseType(&LayerBounds);

        //
        // Prepare transform matrix when there is mask and offsetting is
        // needed.
        //
        //  CAdjustTransform should accept const
        //  Convert CAdjustTransform to CAdjustBounds pattern which can
        //  reduce copying and simplify setup.
        //

        CMultiOutSpaceMatrix<CoordinateSpace::Shape> matMaskToTarget;
        CMultiOutSpaceMatrix<CoordinateSpace::Shape> *pmatMaskToTarget = NULL;

        if (pGeometricMask && m_fUseRTOffset)
        {
            if (pGeometricMaskToTarget)
            {
                matMaskToTarget =
                    *CMatrix<CoordinateSpace::Shape,CoordinateSpace::PageInPixels>::ReinterpretBase
                    (CBaseMatrix::ReinterpretBase
                     (pGeometricMaskToTarget));
            }
            else
            {
                matMaskToTarget.SetToIdentity();
                matMaskToTarget.DbgChangeToSpace<CoordinateSpace::Shape,CoordinateSpace::PageInPixels>();
            }

            pmatMaskToTarget = &matMaskToTarget;

            pGeometricMaskToTarget =
                CMILMatrix::ReinterpretBase(pmatMaskToTarget);
        }

        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,    // pDisplaySet           
            NULL,             // pAliasedClip,
            &pLayerBounds,    // ppBoundsToAdjust,
            pmatMaskToTarget, // pTransform,
            NULL,             // pContextState,
            NULL              // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->BeginLayer(
                *pLayerBounds,
                AntiAliasMode,
                pGeometricMask,
                pGeometricMaskToTarget,
                flAlphaScale,
                pAlphaMask
                ));

            // Keep track of success in case a later RT fails and we need to
            // unwind.
            fSuccessfulStart = TRUE;
            idxLastRTSuccessfullyStarted = metaIterator.CurrentRT();
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    if (pAlphaMask)
    {
        pAlphaMask->RestoreMetaIntermediates();
    }

    //
    // Unwind any successes on any failure
    //

    if (FAILED(hr) && fSuccessfulStart)
    {
        for (UINT idx = idxFirstEnabledRT; idx <= idxLastRTSuccessfullyStarted; idx++)
        {
            if (m_rgMetaData[idx].fEnable)
            {
                IGNORE_HR(m_rgMetaData[idx].pInternalRT->EndLayer());
            }
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::EndLayer
//
//  Synopsis:
//      End accumulation of rendering into current layer.  Modifications to
//      layer, as specified in BeginLayer arguments, are handled and result is
//      applied to render target.
//

STDMETHODIMP
CMetaRenderTarget::EndLayer(
    )
{
    HRESULT hr = S_OK;

    TraceTag((tagMILRenderDrawCalls, "%d. End Layer\n", ++g_dwCallNo));

    for (UINT i = 0; i < m_cRT; i++)
    {
        // Don't EndLayer if this RT is not enabled.

        if (m_rgMetaData[i].fEnable)
        {
            MIL_THR_SECONDARY(m_rgMetaData[i].pInternalRT->EndLayer());
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::EndAndIgnoreAllLayers
//
//  Synopsis:
//      End accumulation of rendering into all layers.  Mdifications to layers,
//      as specified in BeginLayer arguments, are ignored.
//

STDMETHODIMP_(void)
CMetaRenderTarget::EndAndIgnoreAllLayers(
    )
{
    for (UINT i = 0; i < m_cRT; i++)
    {
        // EndAndIgnoreAllLayers is safe even if RT is not enabled.

        if (m_rgMetaData[i].pInternalRT)
        {
            m_rgMetaData[i].pInternalRT->EndAndIgnoreAllLayers();
        }
    }

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::ReadEnabledDisplays
//
//  Synopsis:
//      Return true for each display that is enabled.
//
//------------------------------------------------------------------------------

STDMETHODIMP
CMetaRenderTarget::ReadEnabledDisplays (
    __inout DynArray<bool> *pEnabledDisplays
    )
{
    Assert(m_cRT == pEnabledDisplays->GetCount());
    for (UINT i = 0; i < pEnabledDisplays->GetCount(); i++)
    {
        (*pEnabledDisplays)[i] = !!m_rgMetaData[i].fEnable;
    }

    RRETURN(S_OK);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::GetType
//
//  Synopsis:
//      This method is used to determine if the render target is being used to
//      render, or if it's merely being used for bounds accumulation, hit test,
//      etc. If, going forward, we have more types of "non-rendering
//      rendertargets" (say, for gathering other statistics like "all uses of
//      IRT, etc), we may wish to expand this to return flags instead of just a
//      BOOL.
//
//------------------------------------------------------------------------------

STDMETHODIMP
CMetaRenderTarget::GetType(__out DWORD *pRenderTargetType)
{
    HRESULT hr = S_OK;
    
    DWORD rtType = SWRasterRenderTarget;
    
    TraceTag((tagMILRenderDrawCalls, "%d. GetType\n", ++g_dwCallNo));

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,  // pDisplaySet           
            NULL,           // pAliasedClip,
            NULL,           // ppBoundsToAdjust,
            NULL,           // pTransform,
            NULL,           // pContextState,
            NULL            // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            
            DWORD internalType = 0;
            IFC(pRTInternalNoAddRef->GetType(&internalType));
            Assert(internalType == SWRasterRenderTarget || 
                   internalType == HWRasterRenderTarget ||
                   internalType == DummyRenderTarget);
            // If any of the display RTs are hardware, we'll return hardware, 
            // otherwise we'll return software.
            if (internalType == HWRasterRenderTarget)
            {
                rtType = HWRasterRenderTarget;
            }
        } while (metaIterator.MoreIterationsNeeded());
    }

    *pRenderTargetType = rtType;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::SetClearTypeHint
//
//  Synopsis:
//      This method is used to allow a developer to force ClearType use in
//      intermediate render targets with alpha channels.
//
//------------------------------------------------------------------------------
STDMETHODIMP 
CMetaRenderTarget::SetClearTypeHint(
    __in bool forceClearType
    )
{
    HRESULT hr = S_OK;
 
    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet,  // pDisplaySet           
            NULL,           // pAliasedClip,
            NULL,           // ppBoundsToAdjust,
            NULL,           // pTransform,
            NULL,           // pContextState,
            NULL            // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            
            IFC(pRTInternalNoAddRef->SetClearTypeHint(forceClearType));
        } while (metaIterator.MoreIterationsNeeded());
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::GetRealizationCacheIndex
//
//  Synopsis:
//      Currently unused.
//

UINT
CMetaRenderTarget::GetRealizationCacheIndex()
{
    RIP("Currently unused.");

    return CMILResourceCache::InvalidToken;
}

//+-----------------------------------------------------------------------------
//
//  Function:
//      DrawVideo - Draw a video
//
//  Arguments:
//      pMedia - Video obj
//
//  Return Value:
//      HRESULT
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaRenderTarget::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    HRESULT hr = S_OK;

    //
    // Either the surface renderer isn't NULL or the bitmap source isn't NULL.
    //
    Assert(pSurfaceRenderer != NULL || pBitmapSource != NULL);

    // Assert that an effect has no intermediates. We are not adjusting any
    // meta bitmap render targets that may be stored in an effect list here.
    // Note that we only need to worry about alpha masks since we can't have
    // intermediates any other way
    AssertEffectListHasNoAlphaMask(pIEffect);

    BOOL bSetSrcRect = FALSE;

    if (!(pContextState->RenderState->Options.SourceRectValid))
    {
        MilPointAndSizeL &rect = reinterpret_cast<MilPointAndSizeL &>(pContextState->RenderState->SourceRect);

        //
        // We can have a NULL surface renderer if we have a bitmap source.
        //
        if (NULL != pSurfaceRenderer)
        {
            pContextState->RenderState->Options.SourceRectValid = TRUE;
            IFC(pSurfaceRenderer->GetContentRect(&rect));
            bSetSrcRect = TRUE;
        }
        else
        {
            UINT    width = 0;
            UINT    height = 0;

            IFC(pBitmapSource->GetSize(&width, &height));

            rect.X = rect.Y = 0;
            rect.Width = width;
            rect.Height = height;
        }
    }

    // Disable prefiltering for video.
    bool fSavePrefilterEnable = pContextState->RenderState->PrefilterEnable;
    pContextState->RenderState->PrefilterEnable = false;

    UINT idxFirstEnabledRT;
    if (FindFirstEnabledRT(&idxFirstEnabledRT))
    {
        CMetaIterator metaIterator(
            m_rgMetaData,
            m_cRT,
            idxFirstEnabledRT,
            m_fUseRTOffset,
            m_pDisplaySet, // pDisplaySet           
            NULL,          // pAliasedClip,
            NULL,          // ppBoundsToAdjust,
            NULL,          // pTransform,
            pContextState, // pContextState,
            NULL           // ppIBitmapSource
            );

        IFC(metaIterator.PrepareForIteration());

        do
        {
            IRenderTargetInternal *pRTInternalNoAddRef = NULL;
            IFC(metaIterator.SetupForNextInternalRT(&pRTInternalNoAddRef));
            IFC(pRTInternalNoAddRef->DrawVideo(
                pContextState,
                pSurfaceRenderer,
                pBitmapSource,
                pIEffect));

        } while (metaIterator.MoreIterationsNeeded());
    }

    pContextState->RenderState->PrefilterEnable = fSavePrefilterEnable;

Cleanup:

    if (bSetSrcRect)
    {
        pContextState->RenderState->Options.SourceRectValid = FALSE;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::GetNumQueuedPresents
//
//  Synopsis:
//      Test each rendertarget for it's number of queued presents, and return
//      the maximum value.
//
//------------------------------------------------------------------------------
HRESULT
CMetaRenderTarget::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    HRESULT hr = S_OK;
    UINT uMaxQueuedPresents = 0;

    *puNumQueuedPresents = 0;

    for( UINT idx = 0; (idx < m_cRT); idx++)
    {
        if (m_rgMetaData[idx].fEnable)
        {
            UINT uNumQueuedPresents = 0;

            IFC(m_rgMetaData[idx].pInternalRT->GetNumQueuedPresents(
                &uNumQueuedPresents
                ));

            uMaxQueuedPresents = max(uMaxQueuedPresents, uNumQueuedPresents);
        }
    }

    *puNumQueuedPresents = uMaxQueuedPresents;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaRenderTarget::HasEnabledDeviceIndex
//
//  Synopsis:
//      returns whether the given HW device is enabled in this desktop render
//      target.
//

bool
CMetaRenderTarget::HasEnabledDeviceIndex(
    IMILResourceCache::ValidIndex cacheIndex
    )
{
    bool fFoundMatch = false;

    for (UINT j = 0; j < m_cRT; j++)
    {
        if (   m_rgMetaData[j].fEnable
            && m_rgMetaData[j].pInternalRT->GetRealizationCacheIndex() == cacheIndex)
        {
            fFoundMatch = true;
            break;
        }
    }

    return fFoundMatch;
}







