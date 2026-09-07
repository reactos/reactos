// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Abstract:
//      Contains implementation for walking the visual tree for
//      precompute walk. The bounding boxes are updated here and
//      the dirty regions are also collected
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CPreComputeContext, Mem, "CPreComputeContext");

//=============================================================================

HRESULT
CPreComputeContext::Create(
    __in_ecount(1) CComposition *pDevice,
    __deref_out_ecount(1) CPreComputeContext** ppPreComputeContext
    )
{
    HRESULT hr = S_OK;

    // Instantiate PreComputeContext
    CPreComputeContext* pCtx = NULL;
    pCtx = new CPreComputeContext();
    IFCOOM(pCtx);

    // Instantiate the graph iterator
    pCtx->m_pGraphIterator = new CGraphIterator();
    IFCOOM(pCtx->m_pGraphIterator);

    // Create the render data bounder   
    IFC(CContentBounder::Create(pDevice, &(pCtx->m_pContentBounder)));

    *ppPreComputeContext = pCtx;
    pCtx = NULL;

Cleanup:
    delete pCtx;

    RRETURN(hr);
}

//=============================================================================

CPreComputeContext::CPreComputeContext()
{

}

//=============================================================================

CPreComputeContext::~CPreComputeContext()
{
    delete m_pGraphIterator;
    delete m_pContentBounder;
}

//-----------------------------------------------------------------------------
// CPreComputeContext::PreCompute
//
//   Starts the precompute walk from the specified root
//-----------------------------------------------------------------------------

HRESULT
CPreComputeContext::PreCompute(
    __in_ecount(1) CMilVisual *pRoot,
    __in_ecount_opt(1) const CMilRectF *prcSurfaceBounds,
    UINT uNumInvalidTargetRegions,
    __in_ecount_opt(uNumInvalidTargetRegions) MilRectF const *rgInvalidTargetRegions,
    float allowedDirtyRegionOverhead,
    MilBitmapInterpolationMode::Enum defaultInterpolationMode,
    __in_opt ScrollArea *pScrollArea,
    BOOL fDisableDirtyRegionOptimization
    )
{
    HRESULT hr = S_OK;

    if ((prcSurfaceBounds == NULL) && 
        !fDisableDirtyRegionOptimization)
    {
        // Dirty region computation is only supported if there are 
        // surface bounds specified.
        IFC(E_FAIL);
    }

    m_allowedDirtyRegionOverhead = allowedDirtyRegionOverhead;

    // Initialize our dirty region accumulator stack.
    m_rootDirtyRegion.Initialize(prcSurfaceBounds, allowedDirtyRegionOverhead);
    m_dirtyRegionStack.Push(&m_rootDirtyRegion);

    m_pScrollAreaParameters = pScrollArea;

    // Can't do scroll optimization in cases where dirty regions are turned off.
    // Can't have invalid regions and scroll
    if (pScrollArea && (fDisableDirtyRegionOptimization || uNumInvalidTargetRegions > 0))
    {
        IFC(E_INVALIDARG);
    }

    m_surfaceBounds = *prcSurfaceBounds;

    if (   !fDisableDirtyRegionOptimization
        && rgInvalidTargetRegions)
    {
        //
        // Add known invalid target regions to dirty tracking
        //
        for (UINT i = 0; i < uNumInvalidTargetRegions; i++)
        {
            IFC(m_rootDirtyRegion.Add(&(rgInvalidTargetRegions[i])));
        }
    }

    if (fDisableDirtyRegionOptimization)
    {
        m_rootDirtyRegion.Disable();
    }

    Assert(m_fScrollHasBegun == false);
    m_effectCount = 0;

    //
    // Start the walk from the root.
    //

    IFC(m_pGraphIterator->Walk(pRoot, this));

    if (fDisableDirtyRegionOptimization)
    {
        m_rootDirtyRegion.Enable();
    }

    Assert(m_effectCount == 0);
    Assert(m_dirtyRegionStack.GetSize() == 1);
    m_dirtyRegionStack.Clear();
    m_dirtyRegionStack.Optimize();

    // Can't have scrolls occurring if accelerated scrolling isn't enabled
    Assert(IsAcceleratedScrollEnabled() || !ScrollHasCompleted());

    m_fScrollHasCompleted = false;
    m_fScrollHasBegun = false;

Cleanup:
    // (SUCCEEDED(hr) => (m_transformStack.IsEmpty())
    Assert(!SUCCEEDED(hr) || (m_transformStack.IsEmpty()));

    // Here we need to clean up our state. This includes that we need to dump
    // the content in our state stacks.
    // (Note that the graph iterator cleans itself up if it fails).
    m_transformStack.Clear();

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CPreComputeContext::PreSubgraph (IGraphIteratorSink interface)
//
//   Method called by the graph iterator before visiting the node's subgraph
//-----------------------------------------------------------------------------

HRESULT
CPreComputeContext::PreSubgraph(
    __out_ecount(1) BOOL *pfVisitChildren
    )
{
    HRESULT hr = S_OK;
    
    CMilVisual* pNode = static_cast<CMilVisual*>(m_pGraphIterator->CurrentNode());
    Assert(pNode);

    if (pNode->HasEffects())
    {
        PushEffect();
    }

    Assert(pNode->m_fNodeWasScrolled == false);

    // Visit our children if we need to update bounding boxes or if something in the sub-graph
    // is dirty for render.
    //
    // Also visit children if this is a ScrollableArea node.
    //
    *pfVisitChildren = (pNode->m_fIsDirtyForRenderInSubgraph || pNode->m_fNeedsBoundingBoxUpdate);

    CDirtyRegion2 *pDirtyRegion;
    IFC(m_dirtyRegionStack.Top(&pDirtyRegion));
    
    #if DBG==1
    pNode->m_dwDirtyRegionEnableCount = pDirtyRegion->GetEnabledNestingCount();
    #endif
        
    // If we need to render this node we add his bbox to the dirty region.  We do not support dirty 
    // sub-regions for effects, since an effect can apply a non-affine transform to a dirty 
    // rect and there is no support for general transforms in native code at this time.  Instead we render the
    // entire node the effect is applied to.
    if (   pNode->m_fIsDirtyForRender 
        || (pNode->m_fIsDirtyForRenderInSubgraph && pNode->m_pEffect != NULL) 
       )
    {
        //
        // If this is a scrollable node, we want to calculate and set some special properties for the 
        // scroll, and add only newly exposed areas as dirty regions. If ScrollHandlingRequired is true, 
        // ScrollableAreaHandline handles all of this. If it returns false, we're not doing anything special 
        // and handle this in the regular way.
        //
        bool fScrollOccurred = false;
        
        if (ScrollHandlingRequired(pNode))
        {            
            ScrollableAreaHandling(pNode, pDirtyRegion, &fScrollOccurred);
        }

        if (!fScrollOccurred)
        {
            //
            // We only need to add the bounding box again if it
            // actually changed.
            //
            if (pNode->m_fNeedsBoundingBoxUpdate && !(pDirtyRegion->IsDisabled()))
            {
                // We add this nodes bbox to the dirty region. Alternatively we could walk the sub-graph and add the
                // bbox of each node's content to the dirty region. Note that this is much harder to do because if the
                // transform changes we don't know anymore the old transform. We would have to use to a two phased dirty
                // region algorithm.
                //
                IFC(AddToDirtyRegion(pDirtyRegion, &pNode->m_Bounds));
            }
            
            // If we added a node in the parent chain to the bbox we don't need to add anything below this node
            // to the dirty region.
            pDirtyRegion->Disable();
        }
    }

    //
    // This block caters for the case when there is content overlapping on screen an area which is using scrolling
    // acceleration (see comment on CPreComputeContext::ScrollableAreaHandling), but which is not a descendant of
    // the visual which initiated the scroll.
    //
    // In this case we need to take the old bounding box of this subgraph, translate it by the scroll offset (where
    // it will have been moved to by the ScrollBlt), and add a dirty region at that location to redraw the contents
    // correctly. We will also need to add a dirty region for the new bounds at the location where the content
    // exists, so that the content which was incorrectly offset there can be redrawn too.
    //
    if (   ScrollHasCompleted()
        && !pDirtyRegion->IsDisabled())
    {
        // Convert old bounds to world space, intersect with clip
        CRectF<CoordinateSpace::PageInPixels> bboxWorld;
        TransformBoundsToWorldAndClip(&pNode->m_Bounds, &bboxWorld);
        CRectF<CoordinateSpace::PageInPixels> bboxWorldClipped = bboxWorld;

        // If the bounds of this node are intersecting the previously scrolled area, 
        if (bboxWorldClipped.Intersect(m_scrolledClipArea))
        {
            // Take old bounds add to dirty region (so we can disable the children of this node from getting walked
            // and checked by this logic)
            IFC(pDirtyRegion->Add(&bboxWorld));            
        
            // Take old bounds, scroll offset, then interset with scroll clip, add to dirty region
            CRectF<CoordinateSpace::PageInPixels> offsetBounds = bboxWorldClipped;
            offsetBounds.Offset(static_cast<float>(m_pScrollAreaParameters->scrollX), static_cast<float>(m_pScrollAreaParameters->scrollY));

            IFC(pDirtyRegion->Add(&offsetBounds));

            pNode->m_fHasBoundingBoxAdded = TRUE;
            pDirtyRegion->Disable();
        }
    }

    // If a node in the sub-graph of this node is dirty for render and we haven't collected the bbox of one of pNode's
    // ascendants as dirty region, then we need to maintain the transform and clip stack so that we have a world transform
    // when we need to collect the bbox of the descendant node that is dirty for render.  If something has changed
    // in the contents or subgraph, we need to update the cache on this node.
    if (pNode->m_fIsDirtyForRenderInSubgraph || pNode->m_fHasAdditionalDirtyRegion || pNode->m_fHasContentChanged)
    {                
        // Dirty regions will be enabled if we haven't collected an ancestor's bbox or if they were re-enabled
        // by an ancestor's cache.
        if (!pDirtyRegion->IsDisabled())
        {
            IFC(PushBoundsAffectingProperties(pNode));
        }

        // If we have a cache on this node we need to invalidate it.
        if (pNode->m_pCaches != NULL)
        {
            // Note that pushing a cache may affect the m_dirtyRegion stack.  If we needed to use pDirtyRegion
            // after this point we should grab it from the Top() again.
            IFC(PushCache(pNode));
            #if DBG
            // Ensure we don't incorrectly use pDirtyRegion without updating it.
            pDirtyRegion = NULL;
            #endif
        }

    }

    //
    // Update content bounds
    //

    if (pNode->m_fNeedsBoundingBoxUpdate)
    {

        // This node's bbox needs to be updated. We start out by setting his bbox to the bbox of its content. All its
        // children will union their bbox into their parent's bbox. PostSubgraph will clip the bbox and transform it
        // to outer space.
        IFC(pNode->GetContentBounds(
            m_pContentBounder,
            OUT &(pNode->m_Bounds)
            ));
    }
 
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CPreComputeContext::PostSubgraph (IGraphIteratorSink interface)
//
//   Method called by the graph iterator after visiting the node's subgraph
//-----------------------------------------------------------------------------

HRESULT
CPreComputeContext::PostSubgraph()
{
    HRESULT hr = S_OK;
    
    CMilVisual* pParent = static_cast<CMilVisual*>(m_pGraphIterator->CurrentParent());
    CMilVisual* pNode = static_cast<CMilVisual*>(m_pGraphIterator->CurrentNode());
    Assert(pNode);

    // Store the inner bounds since we might need them for comparison later on.
    CMilRectF currentInnerBounds = pNode->m_Bounds;
    
    CDirtyRegion2 *pDirtyRegion;
    IFC(m_dirtyRegionStack.Top(&pDirtyRegion));

    if (pNode->m_fNeedsBoundingBoxUpdate)
    {
        //
        // If pNode's bbox got recomputed it is at this point still in inner
        // space. We need to apply the clip and transform.
        //
        IFC(ConvertInnerToOuterBounds(pNode));
    }

    //
    // Update state on the parent node if we have a parent.

    if (pParent)
    {
        // Update the bounding box on the parent.

        if (pParent->m_fNeedsBoundingBoxUpdate)
        {
            pParent->m_Bounds.Union(pNode->m_Bounds);
        }
    }
    

    // 
    // If there are additional dirty regions, pick them up. (Additional dirty regions are
    // specified before the tranform, i.e. in inner space, hence we have to pick them
    // up before we pop the transform from the transform stack.
    //
    if (pNode->m_fHasAdditionalDirtyRegion)
    {
        // We need to add the bbox of this node to the dirty region.
        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> top;
        m_transformStack.Top(&top);
        const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pTop = &top;

        // check for clip
        CRectF<CoordinateSpace::PageInPixels> clip;
        const CRectF<CoordinateSpace::PageInPixels> *pClip = NULL;
        if (!(m_clipStack.IsEmpty()))
        {
            // get the top clip  
            m_clipStack.Top(&clip); 
            pClip = &clip;
        }

        IFC(pNode->CollectAdditionalDirtyRegion(pDirtyRegion, 
                                                pTop, 
                                                ScrollHasCompleted() ? m_pScrollAreaParameters->scrollX : 0,
                                                ScrollHasCompleted() ? m_pScrollAreaParameters->scrollY : 0,
                                                m_scrolledClipArea,
                                                pClip
                                                ));
    }

    // If we pushed transforms here, we need to pop them again.  If we're handling a cache we need
    // to finish handling it here as well.
    if (pNode->m_fIsDirtyForRenderInSubgraph || pNode->m_fHasAdditionalDirtyRegion || pNode->m_fHasContentChanged)
    {           
        // If we have a cache to update on this node, update it.
        if (pNode->m_pCaches != NULL)
        {
            IFC(PopCache(pNode));
            // The cache may have changed the dirty region stack, so grab the top CDirtyRegion2.
            IFC(m_dirtyRegionStack.Top(&pDirtyRegion));
        }

        // Dirty regions will be enabled if we haven't pushed an ancestor's bbox, or if this node has a cache and it
        // is only dirty for render in its subgraph.
        if (!pDirtyRegion->IsDisabled())
        {
            PopBoundsAffectingProperties(pNode);
        }
    }

    if (pNode->m_fHasBoundingBoxAdded == TRUE)
    {
        IFC(AddToDirtyRegion(pDirtyRegion, &pNode->m_Bounds));
        pDirtyRegion->Enable();
        pNode->m_fHasBoundingBoxAdded = FALSE;        
    }
    
    // If this node is dirty we need to add this node's bounding box to the dirty region set.
    // We need to render any node with a bitmap effect and a dirty sub-region because legacy
    // bitmap effects can apply a non-affine transform to a dirty rect, and there is no support
    // for general transforms in native code at this time.
    
    if (pNode->m_fIsDirtyForRender
        || (pNode->m_fIsDirtyForRenderInSubgraph && pNode->m_pEffect != NULL)
       )
    {
        if (pNode->m_fNodeWasScrolled)
        {
            m_fScrollHasCompleted = true;

            // Save the area which was scrolled, so that we can detect overlapping content 
            // that is a "peer" (ie not in the child chain of this node) in the rest of the
            // precompute walk and treat it appropriately for dirtiness.
            m_scrolledClipArea = m_pScrollAreaParameters->clipRect;
        }
        else
        {                
            pDirtyRegion->Enable();

            // We need to add the bbox of this node to the dirty region.
            IFC(AddToDirtyRegion(pDirtyRegion, &pNode->m_Bounds));
        }
    }

    //
    // If this node has an alpha mask an we caused its inner bounds to change
    // then treat the node as if m_fIsDirtyForRender was set.
    //
    if (pNode->m_pAlphaMaskWrapper && pNode->m_fNeedsBoundingBoxUpdate)
    {
        IFC(CollectAlphaMaskDirtyRegions(pDirtyRegion, pNode, &currentInnerBounds));
    }

    if (pNode->m_fNodeWasScrolled)
    {
        Assert(IsAcceleratedScrollEnabled());
        Assert(pNode->m_pScrollBag);

        // Scroll has completed. Mark this in the precompute context
        m_fScrollHasCompleted = true;
    }

    if (pNode->m_pScrollBag)
    {
        // Reset the scroll property bag on the node, so that we don't try to 
        // perform this scroll again on the next precompute pass.
        pNode->m_pScrollBag->scrollOccurred = false;
    }

    if (pNode->HasEffects())
    {
        PopEffect();
    }

    pNode->m_fIsDirtyForRender = FALSE;
    pNode->m_fIsDirtyForRenderInSubgraph = FALSE;
    pNode->m_fNeedsBoundingBoxUpdate = FALSE;
    pNode->m_fHasAdditionalDirtyRegion = FALSE;
    pNode->m_fHasContentChanged = FALSE;
    pNode->m_fNodeWasScrolled = FALSE;
    pNode->m_fHasStateOtherThanOffsetChanged = FALSE;
    pNode->m_fAdditionalDirtyRectsExceeded = FALSE;

#if DBG==1
    AssertMsg(
        pNode->m_dwDirtyRegionEnableCount == pDirtyRegion->GetEnabledNestingCount(),
        "Mismatched dirtyRegion.Enabled and dirtyRegion.Disabled calls.");
#endif

Cleanup:
    RRETURN(hr);
}

//=============================================================================

HRESULT
CPreComputeContext::AddToDirtyRegion(
    __in_ecount(1) CDirtyRegion2 *pDirtyRegion,
    __in_ecount(1) const CMilRectF *pBoundsLocalSpace
    )
{
    HRESULT hr = S_OK;
    
    CRectF<CoordinateSpace::PageInPixels> bboxWorld;

    TransformBoundsToWorldAndClip(pBoundsLocalSpace, &bboxWorld);

    IFC(pDirtyRegion->Add(&bboxWorld));

Cleanup:
    RRETURN(hr);
}

//=============================================================================

void
CPreComputeContext::TransformBoundsToWorldAndClip(
    __in_ecount(1) const CMilRectF *pBoundsLocalSpace,
    __inout_ecount(1) CRectF<CoordinateSpace::PageInPixels> *pBboxWorld
    )
{
    CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> matTop;
    m_transformStack.Top(&matTop);

    matTop.Transform2DBoundsConservative(
        *CRectF<CoordinateSpace::LocalRendering>::ReinterpretNonSpaceTyped(pBoundsLocalSpace),
        *pBboxWorld
        );

    // clip the bounding box against the clip
    if (!(m_clipStack.IsEmpty()))
    {
        // get the top clip  
        CRectF<CoordinateSpace::PageInPixels> topClip;
        m_clipStack.Top(&topClip);          
        
        // top clip is in world space so we can directly intersect bboxworld with it
        pBboxWorld->Intersect(topClip);
    }
}

//=============================================================================

HRESULT
CPreComputeContext::ConvertInnerToOuterBounds(
    __in_ecount(1) CMilVisual *pNode
    )
{
    HRESULT hr = S_OK;
    CMilRectF *pNodeBounds = &(pNode->m_Bounds);

    // Image effects can transform the bounds, so we call into the
    // effect code to properly resize them before we apply the clip.
    if (pNode->m_pEffect != NULL)
    {
        IFC(pNode->m_pEffect->TransformBoundsForInflation(pNodeBounds));
    }
    
    if (pNode->m_pClip != NULL)
    {
        CMilRectF bounds;
        IFC(pNode->m_pClip->GetBoundsSafe(&bounds));

        pNodeBounds->Intersect(bounds);
    }

    // Apply transform if we have one.
    if (pNode->m_pTransform != NULL)
    {

        // Instead of having to look up this transform twice we could store this
        // transform on the graph iterator stack. Moving forward we will expose a service
        // that allows us to have a per node temporary storage on the graph iterator.

        const CMILMatrix* pMatrix;
        IFC(pNode->m_pTransform->GetMatrix(&pMatrix));

        // Now apply the transform.
        pMatrix->Transform2DBounds(*pNodeBounds, *pNodeBounds);
    }

    // Apply the offset.
    pNodeBounds->OffsetNoCheck(pNode->m_offsetX, pNode->m_offsetY);

    // set to infinite bounds if bounding box has NaN 
    if (!(pNodeBounds->IsWellOrdered()))
    {
        *pNodeBounds = CMilRectF::sc_rcInfinite;
    }

Cleanup:
    RRETURN(hr);  
}


//-----------------------------------------------------------------------------
// CPreComputeContext::CollectAlphaMaskDirtyRegions
//
//   If a node has an alpha mask and its inner bounds have changed (even if
//   the outer bounds remained the same), then we want to treat this node as 
//   if it had the flag m_fIsDirtyForRender set.
//   So we dirty its previous bounds and also dirty its current bounds.
//   We have to compare the inner bounds due to the fact that the opacity mask 
//   is applied below the clip.
//   We need this because alpha mask (like radialgradientbrush) use these bounds
//   and if bounds change, they re-create the realization. So we need to make 
//   the whole node dirty such that the new realization is displayed.
//-----------------------------------------------------------------------------
HRESULT
CPreComputeContext::CollectAlphaMaskDirtyRegions(
    __in_ecount(1) CDirtyRegion2 *pDirtyRegion,
    __in_ecount(1) CMilVisual *pNode,
    __in_ecount(1) const CMilRectF *pNodeInnerBounds
    )
{
    HRESULT hr = S_OK;

    Assert(pNode->m_pAlphaMaskWrapper);
    Assert(pNode->m_fNeedsBoundingBoxUpdate);

    CMilRectF nodePreviousInnerBounds;
    pNode->m_pAlphaMaskWrapper->GetVisualPreviousInnerBounds(&nodePreviousInnerBounds);

    if (!(pDirtyRegion->IsDisabled())       // then we already collected ancestors bounds                
        && !(pNode->m_fIsDirtyForRender)    // then we already collected our bounds
        && !(nodePreviousInnerBounds.IsEquivalentTo(*pNodeInnerBounds)))
    {
        // Since our bounds have changed and the above also holds, so the following has to be true.
        Assert((pNode->m_fIsDirtyForRenderInSubgraph) || (pNode->m_fHasAdditionalDirtyRegion));

        //
        // Add its current bounds to the dirty region list.
        //

        IFC(AddToDirtyRegion(pDirtyRegion, &pNode->m_Bounds));

        //
        // Add the previous bounds in outer space also since the new bounds 
        // might be smaller than the previous one.
        //

        CMilRectF nodePreviousOuterBounds;
        pNode->m_pAlphaMaskWrapper->GetVisualPreviousOuterBounds(&nodePreviousOuterBounds);
        IFC(AddToDirtyRegion(pDirtyRegion, &nodePreviousOuterBounds));
    }


    // Store the current inner bounds for tracking changes in bounds later.
    pNode->m_pAlphaMaskWrapper->SetVisualPreviousInnerBounds(*pNodeInnerBounds);

    // Store the current outer bounds for adding to dirty region later.
    pNode->m_pAlphaMaskWrapper->SetVisualPreviousOuterBounds(pNode->m_Bounds);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CPreComputeContext::PushBoundsAffectingProperties
// Pushes the offset, transform, and clip.
//-----------------------------------------------------------------------------
HRESULT
CPreComputeContext::PushBoundsAffectingProperties(
    __in CMilVisual *pNode
    )
{
    HRESULT hr = S_OK;

    //    
    // Special TS clip goes above all other modifiers
    // Note that we have to apply this clip even if we aren't actually able to accelerate
    // the scroll (eg if we're in hardware) to ensure consistent look between hardware
    // and software
    //
    if (pNode->m_pScrollBag)
    {
        CRectF<CoordinateSpace::LocalRendering> localClip = pNode->m_pScrollBag->clipRect;
        CRectF<CoordinateSpace::PageInPixels> worldSnappedClip;

        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> transform;
        m_transformStack.Top(&transform);

        CMilVisual::TransformAndSnapScrollableRect(&transform, NULL, &localClip, &worldSnappedClip);
        
        IFC(m_clipStack.Push(worldSnappedClip));
    }

    // If there's a scroll bag we may need to offset this node even if its offset is 0,0
    // because that may not be a 0 offset when transformed and snapped in world space. Fun!
    if (pNode->m_pScrollBag)
    {
        // Must round offset to integer size
        CMilPoint2F offset(pNode->m_offsetX, pNode->m_offsetY);

        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> transform;
        m_transformStack.Top(&transform);
        
        IFC(CMilVisual::TransformAndSnapOffset(&transform, &offset, true));

        IFC(m_transformStack.PushOffset(offset.X, offset.Y));
    }
    else
    {
        if ((pNode->m_offsetX != 0.0f) || (pNode->m_offsetY != 0.0f))
        {
            IFC(m_transformStack.PushOffset(pNode->m_offsetX, pNode->m_offsetY));
        }
    }

    if (pNode->m_pTransform != NULL) 
    {
        const CMILMatrix *pMatrix;
        IFC(pNode->m_pTransform->GetMatrix(&pMatrix));
        IFC(m_transformStack.Push(pMatrix));
    }

    if (pNode->m_pClip != NULL) 
    {
        CRectF<CoordinateSpace::LocalRendering> clipBounds;
        IFC(pNode->m_pClip->GetBoundsSafe(&clipBounds));  
        CRectF<CoordinateSpace::PageInPixels> clipWorld;

        // now convert this clip bound to world space. (Clip stack always remains at world space)    
        // get the transform, then transform clip to world space
        // Future Consideration:   Find accurate coordinate space name
        //  for CPreComputeContext's "world" space.
        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> top;
        m_transformStack.Top(&top);            
        top.Transform2DBounds(clipBounds, OUT clipWorld);

        // push the clip. Pushing it intersects it with the previous Clip            
        IFC(m_clipStack.Push(clipWorld));
    }
    
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CPreComputeContext::PopBoundsAffectingProperties
// Pops the offset, transform, and clip.
//-----------------------------------------------------------------------------
void
CPreComputeContext::PopBoundsAffectingProperties(
    __in CMilVisual const *pNode
    )
{
    if (pNode->m_pTransform != NULL) 
    {
        m_transformStack.Pop();
    }

    if (pNode->m_pClip != NULL) 
    {
        m_clipStack.Pop();
    }

    if (   (pNode->m_offsetX != 0) 
        || (pNode->m_offsetY != 0)
        || (pNode->m_pScrollBag != NULL))
    {
        m_transformStack.Pop();
    }

    // Pop special TS clip if we have one.
    if (pNode->m_pScrollBag)
    {
        m_clipStack.Pop();
    }            

}

//-----------------------------------------------------------------------------
// CPreComputeContext::PushCache
// If the contents have changed we'll invalidate the whole cache in PopCache.
// Otherwise only the subtree has changed so we'll collect dirty regions.
// Even if we've disabled dirty regions in an ancestor we effectively  re-enable them by pushing a new CDirtyRegion here.
// NOTE: Affects m_dirtyRegion stack!
//-----------------------------------------------------------------------------
HRESULT
CPreComputeContext::PushCache(
    __in CMilVisual *pNode
    )
{
    HRESULT hr = S_OK;
    
    if(!pNode->m_fHasContentChanged)
    {
        CDirtyRegion2 *pDirtyRegionNoRef;
        pNode->m_pCaches->BeginPartialInvalidate(m_allowedDirtyRegionOverhead, &pDirtyRegionNoRef);
        m_dirtyRegionStack.Push(pDirtyRegionNoRef);
        
        // We want to collect dirty regions relative to our node's bbox, so push a non-multiplicative transform
        // and an empty clip on top of the stack.
        CMILMatrix matIdentity(true);
        IFC(m_transformStack.Push(&matIdentity, false /*do not multiply*/));
        CRectF<CoordinateSpace::PageInPixels> noClip = CRectF<CoordinateSpace::PageInPixels>::ReinterpretNonSpaceTyped(CMilRectF::sc_rcInfinite);
        IFC(m_clipStack.PushExact(noClip));
    }
    
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
// CPreComputeContext::PopCache
// If the contents have changed we'll invalidate the whole cache.
// Otherwise only the subtree has changed so we'll invalidate the collected dirty regions.
// NOTE: Affects m_dirtyRegion stack!
//-----------------------------------------------------------------------------
HRESULT 
CPreComputeContext::PopCache(
    __in CMilVisual *pNode
    )
{
    HRESULT hr = S_OK;
    
    // We need to save the potentially-recalculated bounds to the cache, since we might have to create a 
    // differently sized intermediate.
    // Since the cache is applied below all the other properites on the node (including transform and offset)
    // we want to pass the cached local inner space bounds.
    CMilRectF rcLocalBounds;
    IFC(m_pContentBounder->GetVisualInnerBounds(pNode, &rcLocalBounds));

    //
    // Caches are invalidated and added to the update list in post-subgraph order.
    // This ensures that nested caches will render correctly (i.e. children first).
    //

    // If the visual's contents changed, we'll need to re-realize the entire cache so we didn't bother
    // collecting dirty regions.  Since m_fHasContentChanged implies m_fIsDirtyForRender, this node's
    // entire old and new bounds will be redrawn.
    if (pNode->m_fHasContentChanged)
    {
        IFC(pNode->m_pCaches->FullInvalidate(&rcLocalBounds));
    }
    // If only something in the cache's subtree was dirty, we'll handle dirty region accumulation.
    else
    {
        // We pop the dirty regions we've accumulated for this subtree cache.
        CDirtyRegion2 *pCacheDirtyRegion = NULL;
        m_dirtyRegionStack.Pop(&pCacheDirtyRegion);

        // Set the new top of the dirty region stack, the parent of cacheDirtyRegion.
        CDirtyRegion2 *pDirtyRegion;
        IFC(m_dirtyRegionStack.Top(&pDirtyRegion));

        // Pop off the transform and clip we pushed to ensure dirty rects were collected in local space.
        m_transformStack.Pop();
        m_clipStack.Pop();

        if (!pDirtyRegion->IsDisabled())
        {
            // We need to add any accumulated regions to the parent dirty region accumulator to
            // ensure that the updated cache is rendered in the Render pass.
            // These dirty regions must be transformed from the cache's local to the node's world space
            // using the accumulated transform on the stack.
            CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> matWorldTransform;
            m_transformStack.Top(&matWorldTransform);
            const CMILMatrix *pmatWorldTransform = ReinterpretLocalRenderingAsMILMatrix(&matWorldTransform);
            
            const MilRectF *pDirtyRects = pCacheDirtyRegion->GetUninflatedDirtyRegions();
            UINT cDirtyRect = pCacheDirtyRegion->GetRegionCount();

            if (cDirtyRect > 0)
            {
                // Calculate the inflated world-space bounds of this node.
                CRectF<CoordinateSpace::PageInPixels> bboxWorld;
                TransformBoundsToWorldAndClip(&rcLocalBounds, &bboxWorld);
                if (!bboxWorld.IsEmpty())
                {
                    InflateRectF_InPlace(&bboxWorld);

                    // After we scale each dirty rect to world space, we need to inflate it more to account for cache
                    // scaling.  When a cache is rendered at 1/10th scale, the 1-pixel AA inflation we do in that space is
                    // effectively 10 pixels in world space, so we need to account for that extra inflation or we get
                    // dirty rect artifacts.
                    float inflation = pNode->m_pCaches->GetNodeCacheScaleInflation();

                    for (UINT i = 0; i < cDirtyRect; i += 1)
                    {
                        const CMilRectF localDirtyRect(pDirtyRects[i]);
                        CMilRectF worldDirtyRect;
                        pmatWorldTransform->Transform2DBounds(localDirtyRect, worldDirtyRect);

                        if (!worldDirtyRect.IsEmpty())
                        {
                            // Inflate dirty rect to account for cache scaling.
                            // NOTE: Rects completely contained within the cache node's bboxWorld could be clipped against 
                            //       it to minimize overdraw caused by scaling rects up to account for a small RenderAtScale.  
                            //       However, since some dirty rects collected could represent the old bounds of the cache 
                            //       if it has moved or gotten smaller, clipping ALL rects against the current bounds is 
                            //       incorrect - it would leave stale content on the screen.
                            InflateRectF_InPlace(&worldDirtyRect, inflation);
                            
                            IFC(pDirtyRegion->Add(&worldDirtyRect));
                        }
                    }
                }
            }
        }
        
        IFC(pNode->m_pCaches->EndPartialInvalidate(&rcLocalBounds));
    }
    
Cleanup:
    RRETURN(hr);
}

//=============================================================================
//
// This is where the calculation of the area which we can do an "accelerated scroll" for.
// There are many prerequisites that must be satisfied before we can get to this point,
// and there are some post requisites too.
//
// This comment is intended to be a catch all comment describe the workings of the accelerated
// scrolling infrastructure. 
//
// Background: 
//      Currently WPF only has support for bitmap remoting. This change is designed to provide limited support
//      for using scrollblt accelerate some common LOB scenarios.
//
// Approach:
//      GDI has fairly extensive native remoting capabilities, and supports all OSs to sometime well
//      before WPFs support begins (ie we can assume GDI remoting is available on all platforms which WPF is)
//      While WPF does not use GDI for rendering since we have our own software rasterizer, WPF does
//      use GDI for presenting when rendering in software (which is the case in a TS session).
//      The most useful GDI feature we can make use of is "ScrollBlit", which basically means the use
//      of the ::BitBlt function with the same source and destination DC, and the same sized source
//      and destination rectangle. Since this command is remoted, it will instruct the remote client machine
//      to "move" a rectangle from one area of the app's window front buffer surface to another area, while
//      transferring only the data required to specify the command and parameters. Once this has occurred, we
//      can then re-render and present the "newly exposed" area only, instead of the entire area.
//
//      Consider this "scrollable area":
//
//      |-------------------------------|
//      |                               |
//      |  Some editor text goes here   |
//      |  Some more here               |
//      |                               |
//      |                               |
//      |                               |
//      |  ...                          |
//      |                               |
//      |                               |
//      |                               |
//      |  ...                          |
//      |                               |
//      |  More stuff here              |
//      |-------------------------------|
//
//      Now imagine the users cursor is positioned on the last line, and the user presses the down arrow key.
//      By performing the "ScrollBlt" of (all content except the top line) to the new postion such that the line
//      that was previously second is now first, we have:
//
//      |-------------------------------|
//      |  Some editor text goes here   |
//      |  Some more here               |
//      |                               |
//      |                               |
//      |                               |
//      |  ...                          |
//      |                               |
//      |                               |
//      |                               |
//      |  ...                          |
//      |                               |
//      |  More stuff here              |
//      |  More stuff here              |
//      |-------------------------------|
//
//      Note that "More stuff here" is duplicated, because we haven't over written over this area. We can render
//      this single line as a bitmap and present only that area, and now we have achieved a data transmission saving
//      of roughly 13:1 for this example (previously we sent 13 lines worth of bitmap data, now we send only 1 + the 
//      parameters for the scroll blit).
//
// Details:
//      As with many things, with this optimization the devil is in the details. The WPF UCE composition system was
//      not designed to easily enable this scenario, so this required extensive modification to the precompute and
//      dirty region collection logic.
//
//      The basic logic flow of this within WPF is:
//
//      1. Application uses a UIElement API ("ScrollableAreaClip") to mark a special clip area
//         on a particular Visual node. This special clip is a simple rectangle, and is actually clipped in world
//         space (to ensure pixel alignment, which is a requirement to use ScrollBlt since it works in pixels).
//         The local space rect which the client sets is saved in pNode->m_pScrollBag.clipRect. Because of the snapping
//         transformation and clipping to world space, this is likely not the final "screen space" rect.
//         Currently the application must guarantee that this Visual or one in its child tree will draw an opaque
//         background over the entire clipRect area. If this does not occur there will be artifacts.
//      2. When the offset is changed on this Visual, the UCE logic which responds to offset changes recognizes this
//         as a "special" node, and saves the previous offset as well as the new offset, and specially marks the node
//         as having a "potentially acceleratable scroll" (pNode->m_pScrollBag.scrollOccurred).
//      3. A number of checks are performed in CSlaveHWndRenderTarget::Render and CDrawingContext::Render before precompute
//         occurs, to determine if various system parameters and window configurations allow us to accelerate scroll. These
//         checks include:
//           - Determining if we're rendering in software
//           - We're not doing a full window render
//           - We're not presenting to a layered window (per pixel transparency can't be used with scrolling).
//           - There are no invalid regions on the render target (due to resize etc)
//           - The window is only on 1 display, ie it is not "straddling" multiple monitors in a multi mon scenario
//      4. At this point, the Precompute walk will begin, and if all the precompute checks above have passed, then precompute
//         will do additional checks as it walks through the tree to determine further eligibility. Things that can 
//         disable the ability to accelerate scroll in the precompute walk are:
//           - Presence of effects/clips on the scroll node or anywhere in its parent chain, because these use intermediates 
//             which can't be accelerated
//           - Presence of other intermediates - DB/VB do not end up passing the enabling arguments to PreCompute, and are
//             thus automatically excluded
//           - Presence of a rotation transform anywhere above the visual
//           - No previous accelerated scrolls have occurred for this frame (multiple "scroll areas" can be set simultaneously,
//             just not scrolled on the same frame)
//      5. If all preconditions are met when precompute arrives at the Visual on which the scroll has occurred, this function
//         gets called! (ScrollableAreaHandling)
//           - This function will calculate the area of the clip, then use the pixel snapped offset (calculate in world space by 
//             subtracting the new and old world space offsets of the Visual) to determine the source and destination scroll 
//             rectangles.
//           - This function will also calculate the "newly exposed area" that still must be added as a dirty region, and add
//             it to te dirty region collector.
//           - It also stores the scroll parameters it calculated on CPreComputeContext::m_pScrollAreaParameters for later use
//      6. After this function returns, it notifies PreSubgraph via the pScrollOccurred out argument, whether an accelerated
//         scroll can occur. If it can, PreSubgraph makes a number of behavior modifications based on that information:
//           - It doesn't add the bounding box of the visual to the dirty region 
//           - It instructs the CGraphIterator to still visit the children of the visual to collect their dirty regions
//                  - Children that have changed can have their regular bounds (with offset) added to the dirty region as normal.
//                    This means that we can still accelerate a scroll even if a line of text is removed or added in the 
//                    editor case, for example. 
//      7. Once the visual node that contains the scroll is exited in PostSubgraph, some properties are set indicating that
//         a scroll has occurred.
//      8. After this time as we continue our tree traversal, we may encounter nodes with content which overlaps the "scrollable 
//         area" that has changed. This overlapping content will always be on top due to our back to front tree traversal order.
//           - The problem here is that the content that is overlapping the scroll region will get scrolled with everything else
//             and will thus be out of place, when it actually should not have moved
//           - To correct for this, we add dirty regions for both the new and old bounds of the node as usual, and additionally
//             a third region which is the old bounds offset by the scroll vector, which will account for stale content that
//             was moved by ScrollBlt before we started rendering.
//           - We also must account for the case where this overlapping content has actually changed size/position in the same
//             frame as the scroll occurs, so we use the "old" bounds to add the offset old dirty region, and the new 
//             bounds for the new location.
//           - We also equip CMilVisual::CollectAdditionalDirtyRegions to be able to offset additional dirty regions if necessary
//             because overlapping content may have been completely removed on the same frame.
//      9. From here, things thankfully get simpler. Once the precompute walk is complete we have the information required
//         to perform the accelerated scroll, and a complete set of additional dirty regions that need to be redrawn after the
//         scroll occurs
//      A. Before render, we issue the ScrollBlt to the software render target. The render target will scroll all associated buffers
//         (front buffer, back buffer, and any color conversion buffers) so that they are all synchronized (which is necessary in case
//         we present again in future without rendering in response to a WM_PAINT, etc. It may defer the scroll to the front buffer until
//         after rendering so that all the GDI operations on the FB get batched together and there is less chance of tearing.
//      B. The dirty regions are rendered and presented. And boom. Accelerated scrolling.
//
//=============================================================================

HRESULT CPreComputeContext::ScrollableAreaHandling(
    __in CMilVisual *pNode,
    __in CDirtyRegion2 *pDirtyRegion,
    __out bool *pScrollOccurred
)
{
    HRESULT hr = S_OK;
    
    Assert(pNode);
    Assert(ScrollHandlingRequired(pNode));
    Assert(m_fScrollHasBegun == false);
    Assert(pNode->m_fNodeWasScrolled == false);
    Assert(m_pScrollAreaParameters->fDoScroll == false);            

    bool fScrollOccurred = false;

    //
    // Transform and clip the scroll area.
    // Check if we can scroll this area. We may not be able to due to
    // transforms above it making it non rectilinear.
    // Also round the transformed/clipped scroll area in to device pixels.
    //

    CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> matTop;
    m_transformStack.Top(&matTop);

    //
    // Don't allow rotate transforms since BitBlt only works for rectilinear source and destination
    //
    if (matTop.Is2DAxisAlignedPreservingNonNegativeScale())
    {
        const ScrollableAreaPropertyBag *pScrollBag = pNode->m_pScrollBag;

        // Determine snapped clip area

        // NOTE: Logic here should match logic in CDrawingContext::TransformAndSnapScrollableRect, except
        // that here we also need to clip the rect.
        CRectF<CoordinateSpace::LocalRendering> scrollClipRect = pScrollBag->clipRect;
        CRectF<CoordinateSpace::PageInPixels> scrollClipRectTransformedAndClipped;

        // get the top clip  
        CRectF<CoordinateSpace::PageInPixels> topClip;
        m_clipStack.Top(&topClip);          

        // Intersect it with the surface bounds, in case there's no clip (in which case topClip
        // will be an infinite rect
        CRectF<CoordinateSpace::PageInPixels> surfaceBounds = *CRectF<CoordinateSpace::PageInPixels>::ReinterpretNonSpaceTyped(&m_surfaceBounds);
        topClip.Intersect(surfaceBounds);

        // Get transform
        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> transform;
        m_transformStack.Top(&transform);

        CRectF<CoordinateSpace::PageInPixels> clippedAndSnappedF;
        CMilVisual::TransformAndSnapScrollableRect(&transform, &topClip, &scrollClipRect, &clippedAndSnappedF);

        CMilRectL scrollClipRectFinal;
        scrollClipRectFinal.left    = CFloatFPU::Round(clippedAndSnappedF.left);
        scrollClipRectFinal.top     = CFloatFPU::Round(clippedAndSnappedF.top);
        scrollClipRectFinal.right   = CFloatFPU::Round(clippedAndSnappedF.right);
        scrollClipRectFinal.bottom  = CFloatFPU::Round(clippedAndSnappedF.bottom);

        // Now we need to determine the offset change, in pixels
        // We already snap the offset of this node to whole pixels in world space, so we just need
        // to calculate the before and after offsets snapped in world space, and diff them.

        CMilPoint2F oldOffset(pScrollBag->oldOffsetX, pScrollBag->oldOffsetY);
        IFC(CMilVisual::TransformAndSnapOffset(&transform, &oldOffset, false));

        CMilPoint2F newOffset(pNode->m_offsetX, pNode->m_offsetY);
        IFC(CMilVisual::TransformAndSnapOffset(&transform, &newOffset, false));
        
        CMilPoint2F snappedOffset(newOffset);
        snappedOffset -= oldOffset;

        //TODO: Check for overflow
        int offsetX = CFloatFPU::Round(snappedOffset.X);
        int offsetY = CFloatFPU::Round(snappedOffset.Y);       

        //
        // We need to determine out area that is "exposed" by the scroll, since we're not adding the
        // whole bounds to the dirty region.
        // The "exposed" area is the scroll clip (which is above the offset and must be rectilinear) 
        // with itself offset by (offsetX, offsetY) geometrically subtracted. If one of offsetX or 
        // offsetY is 0, this will be a single rectangle. If they are both nonzero, it will be two.
        //

        // Take the vertical scroll case first, this is the most common in the targeted scenarios
        CMilRectL verticalScrollRect = CMilRectL::sc_rcEmpty;
        if (offsetY != 0)
        {
            verticalScrollRect = scrollClipRectFinal;
            if (offsetY > 0)
            {
                // scrolling up. Strip is along the top of the clip area
                verticalScrollRect.bottom =  verticalScrollRect.top + offsetY;
            }
            else
            {
                // scrolling down
                verticalScrollRect.top = verticalScrollRect.bottom + offsetY;
            }
        }

        CMilRectL horizontalScrollRect = CMilRectL::sc_rcEmpty;
        if (offsetX != 0)
        {                
            horizontalScrollRect = scrollClipRectFinal;
            if (offsetX > 0)
            {
                // scrolling left. Strip is along the left of the clip area
                horizontalScrollRect.right =  horizontalScrollRect.left + offsetX;
            }
            else
            {
                // content scrolling down
                horizontalScrollRect.left = horizontalScrollRect.right + offsetX;
            }
            // trim horizontalScrollRect so it doesn't overlap verticalScrollRect
            if (offsetY != 0)
            {
                if (offsetY > 0)
                {
                    // scrolling up. Strip is along the top of the clip area
                    horizontalScrollRect.top += offsetY;
                }
                else
                {
                    // scrolling down
                    horizontalScrollRect.bottom += offsetY;
                }
                
            }
        }

        Assert(verticalScrollRect.IsWellOrdered());
        Assert(horizontalScrollRect.IsWellOrdered());
        Assert(!horizontalScrollRect.DoesIntersect(verticalScrollRect));

        CMilRectF verticalScrollRectF   = MilRectLToMilRectF(verticalScrollRect);
        CMilRectF horizontalScrollRectF = MilRectLToMilRectF(horizontalScrollRect);
        
        CMILSurfaceRect dest = scrollClipRectFinal;
        dest.Offset(offsetX, offsetY);

        if (!dest.Intersect(scrollClipRectFinal))
        {
            //
            // It is legal to set an empty clip area. Even if it doesn't make sense.
            //
            //AssertMsg(false, "Empty destination, not expected?");
        }

        CMILSurfaceRect source = dest;
        source.Offset(-offsetX, -offsetY);

        CRectF<CoordinateSpace::PageInPixels> scrollClipRectFinalF;
        scrollClipRectFinalF.left   = static_cast<float>(static_cast<int>(scrollClipRectFinal.left));
        scrollClipRectFinalF.right = static_cast<float>(static_cast<int>(scrollClipRectFinal.right));
        scrollClipRectFinalF.top = static_cast<float>(static_cast<int>(scrollClipRectFinal.top));
        scrollClipRectFinalF.bottom = static_cast<float>(static_cast<int>(scrollClipRectFinal.bottom));

        m_pScrollAreaParameters->destination = dest;
        m_pScrollAreaParameters->source = source;
        m_pScrollAreaParameters->fDoScroll = true;
        m_pScrollAreaParameters->clipRect = scrollClipRectFinalF;
        m_pScrollAreaParameters->scrollX = offsetX;
        m_pScrollAreaParameters->scrollY = offsetY;

        m_fScrollHasBegun = true;
        pNode->m_fNodeWasScrolled = true;

        CRectF<CoordinateSpace::PageInPixels> bboxWorld = CRectF<CoordinateSpace::PageInPixels>::ReinterpretNonSpaceTyped(verticalScrollRectF);
        IFC(pDirtyRegion->Add(&bboxWorld));

        bboxWorld = CRectF<CoordinateSpace::PageInPixels>::ReinterpretNonSpaceTyped(horizontalScrollRectF);
        IFC(pDirtyRegion->Add(&bboxWorld));

        fScrollOccurred = true;
    }

    *pScrollOccurred = fScrollOccurred;
    
Cleanup:
    RRETURN(hr);
}

//=============================================================================

bool 
CPreComputeContext::ScrollHandlingRequired(
        __in CMilVisual const *pNode
        )
{
    return (   pNode->CanBeScrolled() 
            && (m_pScrollAreaParameters != NULL) 
            && (pNode->m_pScrollBag->scrollOccurred)
            && !EffectsInParentChain()
            && (m_fScrollHasBegun == false)
            );
}




