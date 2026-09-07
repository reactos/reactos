// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
// File name:
//      node.cpp
//
// Abstract: 
//      Visual resource.
//
//---------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMilVisual, MILRender, "CMilVisual");

//---------------------------------------------------------------------------------
//
// class CMilVisual
//
//---------------------------------------------------------------------------------

//---------------------------------------------------------------------------------
// CMilVisual::dtor
//---------------------------------------------------------------------------------

CMilVisual::~CMilVisual()
{
    RemoveAllChildren();

    UnRegisterNotifier(m_pContent);
    UnRegisterNotifier(m_pTransform);
    UnRegisterNotifier(m_pEffect);
    UnRegisterNotifier(m_pClip);

    CMilBrushDuce *pAlphaMask = GetAlphaMask();
    UnRegisterNotifier(pAlphaMask);

    delete m_pGuidelineCollection;
    delete m_rgpAdditionalDirtyRects;
    delete m_pAlphaMaskWrapper;

    UnRegisterNotifier(m_pCaches);

    WPFFree(ProcessHeap, m_pScrollBag);

    if (m_pScheduleRecord)
    {
        CMilScheduleManager* pScheduleManager = m_pComposition->GetScheduleManager();
        Assert(pScheduleManager);
        pScheduleManager->Unschedule(&m_pScheduleRecord);
    }
}

//---------------------------------------------------------------------------------
// CMilVisual::GetParent
//
//    Returns the parent of this node or NULL if the node doesn't have a parent.
//---------------------------------------------------------------------------------

CMilVisual*
CMilVisual::GetParent()
{
    return m_pParent;
}

//---------------------------------------------------------------------------------
// CMilVisual::ScheduleRender
//
//    Guidelines helper
//---------------------------------------------------------------------------------

HRESULT
CMilVisual::ScheduleRender()
{
    CMilScheduleManager* pScheduleManager = m_pComposition->GetScheduleManager();
    Assert(pScheduleManager);
    return pScheduleManager->ScheduleRelative(
        this,
        &m_pScheduleRecord,
        CDynamicGuideline::sc_uTimeDelta
        );
}

//---------------------------------------------------------------------------------
// CMilVisual::PropagateFlags (static)
//
//      Ensures that the nodes from this node up the parent chain
//      are marked with the specified flags.
//---------------------------------------------------------------------------------

void
CMilVisual::PropagateFlags(
    __in CMilVisual* pNode,
    BOOL fNeedsBoundingBoxUpdate,
    BOOL fDirtyForRender,
    BOOL fAdditionalDirtyRegion,  // Default value FALSE.
    BOOL fUpdatePreventingScroll  // Default value TRUE.
    )
{
    Assert(pNode);
    AssertMsg(fNeedsBoundingBoxUpdate || fDirtyForRender || fAdditionalDirtyRegion,
        "We shouldn't call this function in the first place if there is nothing to propagate.");

    pNode->NotifyVisualTreeListeners();

    CMilVisual* pParent = pNode->GetParent();

    //
    // We can exit the following while loop if the following condition is true:
    //
    //   (fNeedsBBoxUpdate => pParent->m_fNeedsBBoxUpdate) && (fDirtyForRender => pParent->m_fIsDirtyForRenderInSubgraph)
    //
    // Hence we need to keep iterating iff
    //   !(fNeedsBBoxUpdate => pParent->m_fNeedsBBoxUpdate) && (fDirtyForRender => pParent->m_fIsDirtyForRenderInSubgraph)
    // which is equivalent to
    //   !(!fNeedsBBoxUpdate || pParent->m_fNeedsBBoxUpdate) && (!fDirtyForRender || pParent->m_fIsDirtyForRenderInSubgraph))
    // iff
    //   (fNeedsBBoxUpdate && !pParent->m_fNeedsBBoxUpdate) || (fDirtyForRender && !pParent->m_fIsDirtyForRenderInSubgraph)

    BOOL setIsDirtyForRenderInSubgraph = fAdditionalDirtyRegion || fDirtyForRender;
    
    while (pParent &&
        ((fNeedsBoundingBoxUpdate && !pParent->m_fNeedsBoundingBoxUpdate) ||
         (setIsDirtyForRenderInSubgraph && !pParent->m_fIsDirtyForRenderInSubgraph)))
    {
        pParent->m_fNeedsBoundingBoxUpdate = pParent->m_fNeedsBoundingBoxUpdate || fNeedsBoundingBoxUpdate;
        pParent->m_fIsDirtyForRenderInSubgraph = pParent->m_fIsDirtyForRenderInSubgraph || setIsDirtyForRenderInSubgraph;

        pParent->NotifyVisualTreeListeners();

        pParent = pParent->GetParent();
    }

    pNode->m_fNeedsBoundingBoxUpdate = pNode->m_fNeedsBoundingBoxUpdate || fNeedsBoundingBoxUpdate;
    pNode->m_fIsDirtyForRender = pNode->m_fIsDirtyForRender || fDirtyForRender;
    pNode->m_fHasAdditionalDirtyRegion = pNode->m_fHasAdditionalDirtyRegion || fAdditionalDirtyRegion;

    pNode->m_fHasStateOtherThanOffsetChanged |= !!fUpdatePreventingScroll;

    //
    // When a node becomes dirty for render, the fact that it had 
    // additional dirty rects from removed children becomes unimportant.
    // Remove them now if that is the case.
    //
    // If this is a scrollable node and we are doing a scroll, this is no longer true.
    // We may not actually add this node's entire bounding box to the dirty region.
    // Unfortunately at this point we don't necessarily know whether we're going to 
    // accelerate the scroll or not. So stay on the safe side and keep the additional 
    // regions if this is a scrollable node.
    // See comment on CPreComputeContext::ScrollableAreaHandling()    
    //
    if (pNode->m_fIsDirtyForRender && !pNode->HasScrollableArea())
    {
        SAFE_DELETE(pNode->m_rgpAdditionalDirtyRects);
        pNode->m_fAdditionalDirtyRectsExceeded = FALSE;
    }
}

//---------------------------------------------------------------------------------
// CMilVisual::MarkDirtyForPrecompute
//  Used for device lost cache invalidation.  Marks this node dirty so that it is
//  visited by the next precompute pass.
//---------------------------------------------------------------------------------

void
CMilVisual::MarkDirtyForPrecompute()
{
    CMilVisual::PropagateFlags(
        this, 
        FALSE, // fNeedsBoundingBoxUpdate
        TRUE // fDirtyForRender
        );

    // Ensure the cache is completely regenerated.
    m_fHasContentChanged = TRUE;
}

//---------------------------------------------------------------------------------
// CMilVisual::OnChanged (virtual)
//---------------------------------------------------------------------------------

BOOL
CMilVisual::OnChanged(
    CMilSlaveResource *pSender,
    NotificationEventArgs::Flags e
    )
{
    if (pSender == m_pContent)
    {
        m_fHasContentChanged = TRUE;
    }
    
    CMilVisual::PropagateFlags(
        this,
        TRUE  /* Needs bbox update. */ ,
        TRUE /* Needs to be added to dirty region. */);

    return FALSE;
}

//---------------------------------------------------------------------------------
// CMilVisual::NotifyVisualTreeListeners
//
// Synopsis: Parents of this node will get dirtied in PropagateFlags, but non-parent
//           notifiers will not.  Notify them here.
//
//---------------------------------------------------------------------------------
void
CMilVisual::NotifyVisualTreeListeners()
{
    CMilSlaveResource* pListener = NULL;
    CPtrMultiset<CMilSlaveResource>::Enumerator elt = m_rgpListener.GetEnumerator();
    while (NULL != (pListener = elt.MoveNext()))
    {
        //
        // Notify everyone other than our parent, who will get a more specific notification
        // in PropagateFlags.  
        // NOTE: m_pParent is not necessarily in the listeners list, but check anyway just in case.
        //
        if (pListener != static_cast<CMilSlaveResource*>(m_pParent))
        {
            pListener->NotifyOnChanged(this);
        }
    }
}

//---------------------------------------------------------------------------------
// CMilVisual::SetContent
//---------------------------------------------------------------------------------

HRESULT
CMilVisual::SetContent(
    __in_ecount_opt(1) CMilSlaveResource* pContent
    )
{
    HRESULT hr = S_OK;

    if (pContent != m_pContent)
    {
        // Replace the old resource with the new one
        IFC(RegisterNotifier(pContent));
        UnRegisterNotifier(m_pContent);

        m_pContent = pContent;

        // Mark the node's content as changed.
        m_fHasContentChanged = TRUE;
        
        // Mark the node as dirty and propagate flags
        CMilVisual::PropagateFlags(this, TRUE, TRUE);
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------------
// CMilVisual::SetParent
//---------------------------------------------------------------------------------

void
CMilVisual::SetParent(
    __in_opt CMilVisual *pParentNode
    )
{  
    // We expect that a node is first disconnected before it is connected to another
    // node.
    Assert(m_pParent == NULL || pParentNode == NULL);
    m_pParent = pParentNode;

    // Note that the parent is not add-refed to avoid circular references.
    // The child is kept alive by the parent node and therefore addref'd by the parent. 
}

//---------------------------------------------------------------------------------
// CMilVisual::InsertChildAt
//---------------------------------------------------------------------------------

HRESULT
CMilVisual::InsertChildAt(
    __in CMilVisual *pNewChild, 
    UINT iPosition
    )
{
    HRESULT hr = S_OK;

    // This prevents loops from entering in the visual children chain.
    if (pNewChild->m_pParent != NULL)
    {
        RIP("Attempted to re-parent a visual without disconnecting first.");
        IFC(E_INVALIDARG);
    }

    IFC(m_rgpChildren.InsertAt(pNewChild, iPosition));
    
    pNewChild->AddRef();
    pNewChild->SetParent(this);

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CMilVisual::RemoveChild
//---------------------------------------------------------------------------------

HRESULT
CMilVisual::RemoveChild(
    __in CMilVisual *pChild
    )
{
    HRESULT hr = S_OK;

    if (!m_rgpChildren.Remove(pChild))
    {
        IFC(E_INVALIDARG);
    }

    pChild->SetParent(NULL);
    pChild->Release();

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CMilVisual::RemoveAllChildren
//---------------------------------------------------------------------------------

VOID 
CMilVisual::RemoveAllChildren()
{
    UINT count = static_cast<UINT>(m_rgpChildren.GetCount());
    for (UINT i = 0; i < count; i++)
    {
        CMilVisual *pChild = m_rgpChildren[i];
        if (pChild)
        {
            pChild->SetParent(NULL);
            pChild->Release();
        }
    }
    m_rgpChildren.Clear(); 
}

//---------------------------------------------------------------------------------
// IGraphNode Interface
//      CMilVisual::GetChildAt
//---------------------------------------------------------------------------------

override IGraphNode* CMilVisual::GetChildAt(UINT index)
{
    if (m_rgpChildren.GetCount() <= index)
    {
        return NULL;
    }
    else
    {
        return m_rgpChildren[index];
    }
}

//+------------------------------------------------------------------------
//  IGraphNode Interface:  
//      CMilVisual::EnterNode
//
//  Synopsis:  
//      This is used for cycle detection. Currently we ignore cycles.
//      A count is maintained. The count can only go upto 2 as when the 
//      node tries to enter the second time (loop!!!) it should not 
//      be able to enter and LeaveNode() should be called.
//      Each call to this function should match a call to LeaveNode()
//      It calls the base functions defined in Resslave.h
//
//  Example Usage:
//      To implement this check for cycles, these functions are used as follows:-
//      if (EnterNode())
//      {
//          ...
//      }
//      
//      LeaveNode();
//-------------------------------------------------------------------------

override bool CMilVisual::EnterNode()
{
    return EnterResource();
}

override void CMilVisual::LeaveNode()
{
    LeaveResource();
}

override bool CMilVisual::CanEnterNode() const
{
    return CanEnterResource();
}

// ----------------------------------------------------------------------------
//
//   Command handlers
//
// ----------------------------------------------------------------------------


HRESULT
CMilVisual::ProcessCreate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_CREATE* pCmd
    )
{
    RIP("Unexpected MILCMD_VISUAL_CREATE.");
    RRETURN(E_UNEXPECTED);
}


HRESULT
CMilVisual::ProcessSetOffset(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETOFFSET* pCmd
    )
{
    // The packet contains doubles. Should they be floats? Why are we using doubles in managed
    // but run the compositor in floats?
    float offsetX = (float)pCmd->offsetX;
    float offsetY = (float)pCmd->offsetY;

    SetOffset(offsetX, offsetY);         

    return S_OK;
}

void
CMilVisual::SetOffset(
    float offsetX,
    float offsetY
    )
{
    if ((m_offsetX != offsetX) ||
        (m_offsetY != offsetY))    // It might be worth considering fuzzy comparisons here.
    {
        // Note: The state of CanBeScrolled() is not invariant throughout the PreCompute
        // pass, so it may be that after this SetOffset call occurs, something else happens
        // that means it can't be scrolled any more, and these properties will already have
        // been set. This is OK, because later in PreCompute, when the result of CanBeScrolled
        // is invariant, we will check the CanBeScrolled result again and ignore the parameters 
        // in that case
        if (CanBeScrolled() && !m_pScrollBag->scrollOccurred)
        {
            m_pScrollBag->scrollOccurred = true;
            m_pScrollBag->oldOffsetX = m_offsetX;
            m_pScrollBag->oldOffsetY = m_offsetY;
        }
        
        m_offsetX = offsetX;
        m_offsetY = offsetY;
        CMilVisual::PropagateFlags(this, TRUE, TRUE, FALSE, FALSE);
    }            
}

HRESULT 
CMilVisual::ProcessSetTransform(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETTRANSFORM* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the resource
    CMilTransformDuce *pTransform = NULL;
    if (pCmd->hTransform != HMIL_RESOURCE_NULL)
    {
        pTransform = DYNCAST(CMilTransformDuce, pHandleTable->GetResource(pCmd->hTransform, TYPE_TRANSFORM));
        if (pTransform == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    if (pTransform != m_pTransform)
    {
        // Replace the old resource with the new one
        IFC(RegisterNotifier(pTransform));
        UnRegisterNotifier(m_pTransform);
        m_pTransform = pTransform;

        // Mark the node as dirty and propagate flags
        CMilVisual::PropagateFlags(this, TRUE, TRUE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual::ProcessSetEffect(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETEFFECT* pCmd
    )
{
     HRESULT hr = S_OK;

    // Get the resource
    CMilEffectDuce *pEffect = NULL;
    if (pCmd->hEffect != HMIL_RESOURCE_NULL)
    {
        pEffect = DYNCAST(CMilEffectDuce, pHandleTable->GetResource(pCmd->hEffect, TYPE_EFFECT));
        if (pEffect == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    if (pEffect != m_pEffect)
    {
        // Replace the old resource with the new one
        IFC(RegisterNotifier(pEffect));
        UnRegisterNotifier(m_pEffect);
        m_pEffect = pEffect;

        // Mark the node as dirty and propagate flags
        CMilVisual::PropagateFlags(this, TRUE, TRUE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual::ProcessSetCacheMode(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETCACHEMODE* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilVisualCacheSet *pCaches = NULL;
    
    // Get the resource
    CMilCacheModeDuce *pCacheMode = NULL;
    if (pCmd->hCacheMode != HMIL_RESOURCE_NULL)
    {
        pCacheMode = DYNCAST(CMilCacheModeDuce, pHandleTable->GetResource(pCmd->hCacheMode, TYPE_CACHEMODE));
        if (pCacheMode == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    if (!m_pCaches)
    {
        IFCSUB1(CMilVisualCacheSet::Create(m_pComposition, this, &pCaches));
        m_pCaches = pCaches;
        IFCSUB1(RegisterNotifier(m_pCaches));
    }

    const CMilCacheModeDuce *pOldCacheMode = m_pCaches->GetNodeCacheMode();
    if (pCacheMode != pOldCacheMode)
    {
        IFCSUB1(m_pCaches->SetNodeCacheMode(pCacheMode));

        // Mark the node as dirty for precompute to ensure the cache is updated.
        MarkDirtyForPrecompute();
    }
    
SubCleanup1:
    // Release the add-ref from creation; we're still holding a ref from RegisterNotifier
    // in m_pCaches.
    ReleaseInterface(pCaches);
    
    // Release the wrapper upon failure or if our wrapper no longer holds onto any caches.
    if (FAILED(hr) || m_pCaches->GetCount() == 0)
    {
        UnRegisterNotifier(m_pCaches);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual::ProcessSetScrollableAreaClip(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETSCROLLABLEAREACLIP* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the resource    

    CRectF<CoordinateSpace::LocalRendering> rect;

    const MilPointAndSizeD *pPacketRect = &(pCmd->Clip);
    rect.left = static_cast<float>(pPacketRect->X);
    rect.top = static_cast<float>(pPacketRect->Y);
    rect.right = static_cast<float>(pPacketRect->X + pPacketRect->Width);
    rect.bottom = static_cast<float>(pPacketRect->Y + pPacketRect->Height);

    if (!pCmd->IsEnabled)
    {
        if (m_pScrollBag != NULL)
        {
            WPFFree(ProcessHeap, m_pScrollBag);            
            m_pScrollBag = NULL;

            CMilVisual::PropagateFlags(this, TRUE, TRUE);
        }
    }
    else if (!rect.IsInfinite() && rect.IsWellOrdered())
    {
        if (m_pScrollBag == NULL)
        {
            m_pScrollBag = (ScrollableAreaPropertyBag *)WPFAlloc(ProcessHeap, 
                                                                 Mt(CMilVisual),
                                                                 sizeof(ScrollableAreaPropertyBag)
                                                                 );
            IFCOOM(m_pScrollBag);
            memset(m_pScrollBag, 0, sizeof(ScrollableAreaPropertyBag));
        }

        m_pScrollBag->clipRect = rect;
           
        CMilVisual::PropagateFlags(this, TRUE, TRUE);
    }
    
Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual::ProcessSetClip(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETCLIP* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the resource
    CMilGeometryDuce *pClip = NULL;
    if (pCmd->hClip != HMIL_RESOURCE_NULL)
    {
        pClip = DYNCAST(CMilGeometryDuce, pHandleTable->GetResource(pCmd->hClip, TYPE_GEOMETRY));
        if (pClip == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }
    
    IFC(SetClip(pClip));
    
Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual::SetClip(
    __in_ecount_opt(1) CMilGeometryDuce *pClip
    )
{
    HRESULT hr = S_OK;

    if (pClip != m_pClip)
    {
        // Replace the old resource with the new one
        IFC(RegisterNotifier(pClip));
        UnRegisterNotifier(m_pClip);
        m_pClip = pClip;

        // Mark the node as dirty and propagate flags
        CMilVisual::PropagateFlags(this, TRUE, TRUE);
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual::ProcessSetAlpha(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETALPHA* pCmd
    )
{
    m_alpha = pCmd->alpha;

    CMilVisual::PropagateFlags(this, FALSE, TRUE);

    return S_OK;
}

HRESULT 
CMilVisual::ProcessSetRenderOptions(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETRENDEROPTIONS* pCmd
    )
{
    m_renderOptionsFlags = pCmd->renderOptions.Flags;
    // Assert that no options are being lost.  C_ASSERT in node.h should ensure that.
    Assert(m_renderOptionsFlags == static_cast<DWORD>(pCmd->renderOptions.Flags));
    m_edgeMode = pCmd->renderOptions.EdgeMode;
    m_compositingMode = pCmd->renderOptions.CompositingMode;
    m_bitmapScalingMode = pCmd->renderOptions.BitmapScalingMode;
    m_clearTypeHint = pCmd->renderOptions.ClearTypeHint;
    m_textRenderingMode = pCmd->renderOptions.TextRenderingMode;
    m_textHintingMode = pCmd->renderOptions.TextHintingMode;
    
    CMilVisual::PropagateFlags(this, FALSE, TRUE);

    return S_OK;
}

HRESULT 
CMilVisual::ProcessSetContent(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETCONTENT* pCmd
    )
{
    HRESULT hr = S_OK;

    //
    // Get the content resource -- this could be either 
    // a drawing or render data.
    //

    CMilSlaveResource *pContent = NULL;

    if (pCmd->hContent != HMIL_RESOURCE_NULL)
    {
        pContent = pHandleTable->GetResource(pCmd->hContent, TYPE_RENDERDATA);

        if (pContent == NULL)
        {
            pContent = pHandleTable->GetResource(pCmd->hContent, TYPE_DRAWING);

            if (pContent == NULL) 
            {
                IFC(WGXERR_UCE_MALFORMEDPACKET);
            }
        }
    }

    IFC(SetContent(pContent));

Cleanup:
    RRETURN(hr);

}


HRESULT 
CMilVisual::ProcessSetAlphaMask(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETALPHAMASK* pCmd
    )
{
    HRESULT hr = S_OK;

    // Get the new resource
    CMilBrushDuce *pAlphaMask = NULL;
    if (pCmd->hAlphaMask != HMIL_RESOURCE_NULL)
    {
        pAlphaMask = DYNCAST(CMilBrushDuce, pHandleTable->GetResource(pCmd->hAlphaMask, TYPE_BRUSH));
        if (pAlphaMask == NULL)
        {
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }
    }

    if (!m_pAlphaMaskWrapper)
    {
        IFCSUB1(CMilAlphaMaskWrapper::Create(&m_pAlphaMaskWrapper));
    }

    CMilBrushDuce *pOldAlphaMask = m_pAlphaMaskWrapper->GetAlphaMask();
    if (pAlphaMask != pOldAlphaMask)
    {
        // Replace the old resource with the new one
        IFCSUB1(RegisterNotifier(pAlphaMask));
        UnRegisterNotifier(pOldAlphaMask);
        m_pAlphaMaskWrapper->SetAlphaMask(pAlphaMask);

        // Mark the node as dirty and propagate flags
        CMilVisual::PropagateFlags(this, FALSE, TRUE);
    }

SubCleanup1:
    // Release the wrapper upon failure or if there is no alpha mask.
    if (FAILED(hr) || pAlphaMask == NULL)
    {
        delete m_pAlphaMaskWrapper;
        m_pAlphaMaskWrapper = NULL;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CMilVisual::ProcessRemoveAllChildren(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_REMOVEALLCHILDREN* pCmd
    )
{
    RemoveAllChildren();
    CMilVisual::PropagateFlags(this, TRUE, TRUE);

    return S_OK;
}

HRESULT 
CMilVisual::ProcessRemoveChild(    
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_REMOVECHILD* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilVisual* pChild =
        static_cast<CMilVisual*>(pHandleTable->GetResource(
            pCmd->hChild, 
            TYPE_VISUAL
            ));

    if (pChild == NULL) 
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    //
    // If we're not already dirty for render, add this child's
    // bounding box as an additional dirty region on this node.
    //
    if (!m_fIsDirtyForRender)
    {
        IFC(AddAdditionalDirtyRects(&(pChild->m_Bounds)));
    }

    IFC(RemoveChild(pChild));

    CMilVisual::PropagateFlags(this, 
                               TRUE /* fNeedsBoundingBoxUpdate */, 
                               FALSE /* fDirtyForRender */, 
                               FALSE /* fAdditionalDirtyRegion */,
                               FALSE /* fUpdatePreventingScroll */
                               );

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------------
// CMilVisual::AddAdditionalDirtyRects
//
// Synopsis:
//     Adds an additional dirty region on the node. The dirty region that is added
//     must be a sub-region of the nodes old or new bounding box
//
//---------------------------------------------------------------------------------

HRESULT
CMilVisual::AddAdditionalDirtyRects(
    __in_ecount(1) MilRectF const *pRegion
    )
{
    C_ASSERT(c_maxAdditionalDirtyRects > 1);
    
    HRESULT hr = S_OK;
    
    if (m_rgpAdditionalDirtyRects == NULL)
    {
        m_rgpAdditionalDirtyRects = new DynArray<CRectF<CoordinateSpace::LocalRendering>>();
        IFCOOM(m_rgpAdditionalDirtyRects);
    }

    if (m_rgpAdditionalDirtyRects->GetCount() >= c_maxAdditionalDirtyRects)
    {                
        // Union all the rects into unionedRect, then delete them. Start at the back and
        // remove elements one by one
        UINT count = m_rgpAdditionalDirtyRects->GetCount();
        for (UINT i = count - 1; i > 0; i--)
        {
            (*m_rgpAdditionalDirtyRects)[0].Union((*m_rgpAdditionalDirtyRects)[i]);
            m_rgpAdditionalDirtyRects->RemoveAt(i);
        }        

        m_fAdditionalDirtyRectsExceeded = TRUE;
    }
    
    if (m_fAdditionalDirtyRectsExceeded)
    {
        // Union new region
         (*m_rgpAdditionalDirtyRects)[0].Union(*CRectF<CoordinateSpace::LocalRendering>::ReinterpretNonSpaceTyped(pRegion));
        // No need to propagate - that will already have happened if we're here.
    }
    else
    {
        IFC(m_rgpAdditionalDirtyRects->Add(*CRectF<CoordinateSpace::LocalRendering>::ReinterpretNonSpaceTyped(pRegion)));

        // Having additional dirty rects does not prevent accelerated scroll, because we adjust them by offset in CollectAdditionalDirtyRegions()
        // to make sure they're in the right place.
        PropagateFlags(this, FALSE /* fNeedsBoundingBoxUpdate */, FALSE /* fDirtyForRender */, TRUE /* fAdditionalDirtyRegion */, FALSE /* fUpdatePreventingScroll */);    
    }
 
Cleanup:
    RRETURN(hr);    
}

HRESULT 
CMilVisual::ProcessInsertChildAt(
    __in_ecount(1) const CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_INSERTCHILDAT* pCmd
    )
{
    HRESULT hr = S_OK;

    CMilVisual* pChild =
        static_cast<CMilVisual*>(pHandleTable->GetResource(
            pCmd->hChild, 
            TYPE_VISUAL
            ));

    if (pChild == NULL) 
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    IFC(InsertChildAt(pChild, pCmd->index));

    CMilVisual::PropagateFlags(this, TRUE, FALSE, FALSE, FALSE);
    CMilVisual::PropagateFlags(pChild, FALSE, TRUE);

Cleanup:
    RRETURN(hr);
}


HRESULT 
CMilVisual::ProcessSetGuidelineCollection(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_VISUAL_SETGUIDELINECOLLECTION* pCmd,
    __in_bcount(cbPayload) LPCVOID pcvPayload,
    UINT cbPayload
    )
{
    HRESULT hr = S_OK;

    UINT uCountX = pCmd->countX; // implicitly convert from UINT16
    UINT uCountY = pCmd->countY;
    UINT uCount = uCountX + uCountY;

    if (cbPayload != sizeof(float) * uCount)
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    delete m_pGuidelineCollection;
    m_pGuidelineCollection = NULL;

    if (uCount != 0)
    {
        MIL_THR(CGuidelineCollection::Create(
            pCmd->countX,
            pCmd->countY,
            reinterpret_cast<const float*>(pcvPayload),
            false, // fDynamic
            &m_pGuidelineCollection
            ));

        if (hr == WGXERR_MALFORMED_GUIDELINE_DATA)
        {
            hr = WGXERR_UCE_MALFORMEDPACKET;
        }

        if (FAILED(hr))
            goto Cleanup;
    }

    // Mark the node as dirty and propagate flags
    CMilVisual::PropagateFlags(this, TRUE, TRUE);

Cleanup:
    RRETURN(hr);
}


HRESULT 
CMilVisual::CollectAdditionalDirtyRegion(
    __in_ecount(1) CDirtyRegion2 *pDirtyRegion,
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
    int scrollX,
    int scrollY,
    CRectF<CoordinateSpace::PageInPixels> clipRect,
    __in_ecount_opt(1) const CRectF<CoordinateSpace::PageInPixels> *pWorldClip
    )
{
    HRESULT hr = S_OK;
    
    if (m_rgpAdditionalDirtyRects)
    {
        for (UINT i = 0; i < m_rgpAdditionalDirtyRects->GetCount(); i++)
        {   
            // Transform the dirty rect into world space.
            CRectF<CoordinateSpace::PageInPixels> rcDirtyRectWorld;
            pWorldTransform->Transform2DBoundsConservative(
                (*m_rgpAdditionalDirtyRects)[i],
                OUT rcDirtyRectWorld
                );
            
            if (pWorldClip)
            {
                rcDirtyRectWorld.Intersect(*pWorldClip);
            }

            //  could optimize this so we don't add the whole "old" rect if
            // we're inside the scrolled area
            IFC(pDirtyRegion->Add(&rcDirtyRectWorld));           
            
            //
            // Handle special case where child content that is overlapping the scroll region is 
            // removed from the tree. It will be scroll blitted before we get here,
            // so we need to check if the offset dirty region is visible in the clipped
            // area, and if it is, add another dirty region for it.
            // See comment on CPreComputeContext::ScrollableAreaHandling()
            //
            if ((scrollX != 0) || (scrollY != 0))
            {
                rcDirtyRectWorld.Offset(static_cast<float>(scrollX), static_cast<float>(scrollY));

                if (rcDirtyRectWorld.Intersect(clipRect))
                {
                    IFC(pDirtyRegion->Add(&rcDirtyRectWorld));
                }
            }
         }
    }

Cleanup:
    SAFE_DELETE(m_rgpAdditionalDirtyRects);
     m_fAdditionalDirtyRectsExceeded = FALSE;    

    RRETURN(hr);
}

// Returns TRUE if Effects need to be handled.
// This is used in PreSubgraph and PostSubgraph, and is factored to ensure that identical
// logic is used to determine if setup/cleanup code needs to occur.
// If the alpha, alphaMask, etc are modified between PreSubgraph and PostSubgraph, all
// bets are off (i.e DON'T MODIFY EFFECTS ON A NODE betwee PreSubgraph and PostSubgraph)
BOOL
CMilVisual::HasEffects()
{
    // If we have an opacity...
    float flAlphaValue = (float)m_alpha;

    // ... and test for < 1.0f.  Ideally, the caller will early out 0 or < 0,
    // but if they *don't*, then we should make sure that they apply this effect
    // (for correctness).  Thus, less than 0 *does* count as having an effect.
    if ((flAlphaValue < 1.0f) && !IsCloseReal(flAlphaValue, 1.0f))
    {
        return TRUE;
    }

    // If we have an alpha mask...
    if (GetAlphaMask())
    {
        return TRUE;
    }

    // If we have a geometric mask...
    if (m_pClip)
    {
        return TRUE;
    }

    // If we have a bitmap effect...
    if (m_pEffect != NULL)
    {
        return TRUE;
    }

    return FALSE;
}


//=============================================================================

void
CMilVisual::TransformAndSnapScrollableRect(
    __in CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pTransform,
    __in_opt CMilRectF *pClip,
    __in CRectF<CoordinateSpace::LocalRendering> *pRectIn,
    __out CRectF<CoordinateSpace::PageInPixels> *pRectOut
    )
{
    pTransform->Transform2DBoundsConservative(*pRectIn, *pRectOut);

    if (pClip)
    {
        pRectOut->Intersect(*CRectF<CoordinateSpace::PageInPixels>::ReinterpretNonSpaceTyped(pClip));
    }

    // Round clip rectangle "in" in world space
    pRectOut->left      = CFloatFPU::CeilingF(pRectOut->left);
    pRectOut->right     = CFloatFPU::FloorF(pRectOut->right);
    pRectOut->top       = CFloatFPU::CeilingF(pRectOut->top);
    pRectOut->bottom    = CFloatFPU::FloorF(pRectOut->bottom);

    if (!pRectOut->IsWellOrdered())
    {
        // Snapping may have made this rect incorrect (if original rect was empty, we could have snapped it
        // to have negative size). In this case, just set it empty
        pRectOut->SetEmpty();
    }
}

//=============================================================================

HRESULT
CMilVisual::TransformAndSnapOffset(
    __in CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pTransform,
    __inout MilPoint2F *pOffset,
    bool fReturnToLocalSpace        
    )
{
    HRESULT hr = S_OK;
    
    pTransform->Transform(pOffset, pOffset);

    // Round offset "in" in world space
    pOffset->X      = CFloatFPU::FloorF(pOffset->X);
    pOffset->Y      = CFloatFPU::FloorF(pOffset->Y);

    // Transform back to local space
    if (fReturnToLocalSpace)
    {
        CMatrix<CoordinateSpace::PageInPixels, CoordinateSpace::LocalRendering> invertedTransform;
        bool isInvertible = !!invertedTransform.Invert(*pTransform);

        if (isInvertible)
        {
            invertedTransform.Transform(pOffset, pOffset);
        }
        else
        {
            IFC(E_UNEXPECTED);
        }
    }
        
Cleanup:
    RRETURN(hr);
}


//+--------------------------------------------------------------------------------
// CMilVisual::GetContentBounds
//
// Synopsis:
//     Compute the bounds of the content rendered by this node.
//
//---------------------------------------------------------------------------------

HRESULT
CMilVisual::GetContentBounds(
    __in_ecount(1) CContentBounder *pContentBounder, 
    __out_ecount(1) CMilRectF *prcBounds
    )
{
    RRETURN(pContentBounder->GetContentBounds(
        m_pContent, 
        prcBounds
        ));
}

//+--------------------------------------------------------------------------------
// CMilVisual::RenderContent
//
// Synopsis:
//     Render the contents of this node.  
//
//---------------------------------------------------------------------------------

HRESULT
CMilVisual::RenderContent(
    __in_ecount(1) CDrawingContext* pDrawingContext
    )
{
    HRESULT hr = S_OK;

    if (m_pContent)
    {
        if (m_pContent->IsOfType(TYPE_RENDERDATA))
        {
            CMilSlaveRenderData *pRenderData = DYNCAST(CMilSlaveRenderData, m_pContent);
            Assert(pRenderData);

            IFC(pRenderData->Draw(pDrawingContext));
        }
        else if (m_pContent->IsOfType(TYPE_DRAWING))
        {
            CMilDrawingDuce *pDrawing = DYNCAST(CMilDrawingDuce, m_pContent);
            Assert(pDrawing);

            IFC(pDrawing->Draw(pDrawingContext));
        }
        else
        {
            // Unknown visual content type
            Assert(FALSE);
        }
    }

Cleanup:
    RRETURN(hr);
}

//+--------------------------------------------------------------------------------
//
// CMilVisual::GetAlphaMask
//
// Synopsis:
//     Get the alpha mask for this node
//
//---------------------------------------------------------------------------------

__out_ecount_opt(1) CMilBrushDuce* 
CMilVisual::GetAlphaMask()
{
    return m_pAlphaMaskWrapper ? m_pAlphaMaskWrapper->GetAlphaMask() : NULL;
}

//+--------------------------------------------------------------------------------
//
// CMilVisual::GetCacheSet
//
// Synopsis:
//     Returns the cache set object for this node, or null if it doesn't exist.
//
//---------------------------------------------------------------------------------

__out_ecount_opt(1) CMilVisualCacheSet*
CMilVisual::GetCacheSet()
{
    return m_pCaches;
}

//+--------------------------------------------------------------------------------
//
// CMilVisual::RegisterCache
//
// Synopsis:
//     Adds a brush cache to this node.
//
//---------------------------------------------------------------------------------
HRESULT
CMilVisual::RegisterCache(
    __in_opt CMilBitmapCacheDuce *pCacheMode
    )
{
    HRESULT hr = S_OK;

    CMilVisualCacheSet *pCaches = NULL;

    // Lazily create our cache set.
    if (!m_pCaches)
    {
        IFC(CMilVisualCacheSet::Create(m_pComposition, this, &pCaches));
        m_pCaches = pCaches;
        IFC(RegisterNotifier(m_pCaches));
    }

    // Add the cache to the cache set.
    IFC(m_pCaches->AddCache(pCacheMode));
    
Cleanup:
    // Release the add-ref from creation; we're still holding a ref from RegisterNotifier
    // in m_pCaches.
    ReleaseInterface(pCaches);
    
    RRETURN(hr);
}

//+--------------------------------------------------------------------------------
//
// CMilVisual::UnRegisterCache
//
// Synopsis:
//     Removes a brush cache from this node.
//
//---------------------------------------------------------------------------------
void
CMilVisual::UnRegisterCache(
    __in_opt CMilBitmapCacheDuce *pCacheMode
    )
{
    // Cache set must have been created since RegisterCache must have been called first.
    Assert(m_pCaches);

    // Remove the cache from the cache set.
    #if DBG
    bool removed = 
    #endif
    m_pCaches->RemoveCache(pCacheMode);
    #if DBG
    Assert(removed);
    #endif

    // If our cache set no longer holds any caches, delete it.
    if (m_pCaches->GetCount() == 0)
    {
        UnRegisterNotifier(m_pCaches);
    }
}



