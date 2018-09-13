//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontainer.cxx
//
//  Contents:   Basic container node which introduces a new coordinate system
//              and clipping.
//
//  Classes:    CDispContainer
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_SAVEDISPCONTEXT_HXX_
#define X_SAVEDISPCONTEXT_HXX_
#include "savedispcontext.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

MtDefine(CDispContainer, DisplayTree, "CDispContainer")


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::CDispContainer
//
//  Synopsis:   Construct a container node with equivalent functionality to
//              the CDispItemPlus node passed as an argument.
//
//  Arguments:  pItemPlus       the prototype CDispItemPlus node
//
//  Notes:
//
//----------------------------------------------------------------------------


CDispContainer::CDispContainer(const CDispItemPlus* pItemPlus)
        : CDispContentNode(pItemPlus->GetDispClient())
{
    Assert(!pItemPlus->IsSet(CDispFlags::s_destruct));
    
    // copy size and position
    _rcVisBounds = pItemPlus->_rcVisBounds;
    _rcContainer = pItemPlus->_rcVisBounds;

    // copy relevant flags
    SetFlags(pItemPlus->_flags, CDispFlags::s_containerConstructMask);
    Assert(!IsBalanceNode());
    Assert(IsSet(CDispFlags::s_interiorNode));
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::SetSize
//
//  Synopsis:   Set size of this node.
//
//  Arguments:  size                new size
//              rcBorderWidths      size of borders + scroll bars
//              fInvalidateAll      TRUE to entirely invalidate this node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::SetSize(
        const SIZE& size,
        const RECT& rcBorderWidths,
        BOOL fInvalidateAll)
{
    // caller should call SetRootSize on CDispRoot
    Assert(GetNodeType() != DISPNODETYPE_ROOT);

    CSize sizeDelta(size.cx - _rcContainer.Width(),
                    size.cy - _rcContainer.Height());
    if (sizeDelta.IsZero())
        return;

    // calculate new bounds
    BOOL fRightToLeft =
        _pParentNode != NULL && _pParentNode->IsRightToLeft();
    CRect rcNew(_rcContainer);
    if (fRightToLeft)
    {
        rcNew.left -= sizeDelta.cx;
    }
    else
    {
        rcNew.right += sizeDelta.cx;
    }
    rcNew.bottom += sizeDelta.cy;

    // if changing right-to-left mode, child positions will shift
    if (IsRightToLeft() != fRightToLeft)
        fInvalidateAll = TRUE;

    // if the inval flag is set, we don't need to invalidate because the
    // current bounds might never have been rendered
    if (!IsSet(CDispFlags::s_inval))
    {
        // recalculate in-view flag of all children
        RequestRecalc();
        SetFlag(CDispFlags::s_recalcChildren);

        if (IsInView())
        {
            if (fInvalidateAll)
            {
                Invalidate(_rcContainer, COORDSYS_PARENT);  // inval old bounds
                SetFlag(CDispFlags::s_inval);               // inval new bounds
            }
            else
            {
                InvalidateEdges(_rcContainer, rcNew, rcBorderWidths, fRightToLeft);
            }
        }
        else
        {
            SetFlag(CDispFlags::s_inval);
        }
    }

    Assert(IsSet(CDispFlags::s_recalc));

    _rcContainer = rcNew;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::SetPosition
//
//  Synopsis:   Set the top left position of this container.
//
//  Arguments:  ptTopLeft       top left coordinate of this container
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::SetPosition(const POINT& ptTopLeft)
{
    if (_rcContainer.TopLeft() != ptTopLeft)
    {
        // if the inval flag is set, we don't need to invalidate because the
        // current bounds has never been rendered
        if (!IsSet(CDispFlags::s_inval))
        {
            if (IsVisible())
            {
                Invalidate(_rcVisBounds, COORDSYS_PARENT);
            }
            SetFlag(CDispFlags::s_invalAndRecalcChildren);
            RequestRecalc();
        }

        if (IsSet(CDispFlags::s_positionChange))
        {
            SetPositionHasChanged();
        }

        _rcContainer.MoveTo(ptTopLeft);
        // _rcVisBounds will be recomputed during recalc
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::SetPositionTopRight
//
//  Synopsis:   Set the top right position of this container.
//
//  Arguments:  ptTopLeft       top left coordinate of this container
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::SetPositionTopRight(const POINT& ptTopRight)
{
    CSize offset(ptTopRight.x - _rcContainer.right,
                 ptTopRight.y - _rcContainer.top);

    if (!offset.IsZero())
    {
        // if the inval flag is set, we don't need to invalidate because the
        // current bounds has never been rendered
        if (!IsSet(CDispFlags::s_inval))
        {
            if (IsVisible())
            {
                Invalidate(_rcVisBounds, COORDSYS_PARENT);
            }
            SetFlag(CDispFlags::s_invalAndRecalcChildren);
            RequestRecalc();

            if (IsSet(CDispFlags::s_positionChange))
            {
                SetPositionHasChanged();
            }
        }

        _rcContainer.OffsetRect(offset);
        // _rcVisBounds will be recomputed during recalc
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::GetClientRect
//
//  Synopsis:   Return rectangles for various interesting parts of a display
//              node.
//
//  Arguments:  prc         rect which is returned
//              type        type of rect wanted
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::GetClientRect(RECT* prc, CLIENTRECT type) const
{
    switch (type)
    {
    case CLIENTRECT_BACKGROUND:
    case CLIENTRECT_CONTENT:
    case CLIENTRECT_VISIBLECONTENT:
        {
            CDispInfo di(GetExtras());
            ((CRect*)prc)->SetRect(
                _rcContainer.Size()
                - di._prcBorderWidths->TopLeft().AsSize()
                - di._prcBorderWidths->BottomRight().AsSize());

            // BUGBUG -- Set prc->left = prc->right instead of to zero? (lylec)
            if (prc->left >= prc->right)
                prc->left = prc->right = 0;

            // BUGBUG -- same as above (lylec)
            if (prc->top >= prc->bottom)
                prc->top = prc->bottom = 0;

            if (IsRightToLeft() && type == CLIENTRECT_CONTENT)
            {
                ((CRect*)prc)->MirrorX();
            }
        }
        break;
    default:
        *prc = g_Zero.rc;
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::PreDraw
//
//  Synopsis:   Before drawing starts, PreDraw processes the redraw region,
//              subtracting areas that are blocked by opaque or buffered items.
//              PreDraw is finished when the redraw region becomes empty
//              (i.e., an opaque item completely obscures all content below it)
//
//  Arguments:  pContext    draw context
//
//  Returns:    TRUE if first opaque node to draw has been found
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispContainer::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(AllSet(CDispFlags::s_preDrawSelector));
    Assert(pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));

    // BUGBUG (donmarsh) - for now, we do not delve inside a filtered node.
    // Someday, this will be up to the filter to determine whether PreDraw
    // can safely look at its children and come up with the correct answers.
    if (IsFiltered())
        return FALSE;
    
    CDispExtras* pExtras = GetExtras();
    CDispInfo di(pExtras);

    // save current context information
    CDispContext saveContext(*pContext);

    // offset children
    CalcDispInfo(pContext->_rcClip, &di);
    pContext->_offset +=
        di._borderOffset + di._contentOffset + di._scrollOffset;

    // continue predraw traversal of children, top layers to bottom
    int lastLayer = (int)DISPNODELAYER_POSITIVEZ + 1;
    for (CDispNode* pChild = _pLastChildNode;
         pChild != NULL;
         pChild = pChild->_pPreviousSiblingNode)
    {
        // only children which meet our selection criteria
        if (pChild->AllSet(CDispFlags::s_preDrawSelector))
        {
            // switch clip rectangles and offsets between different layer types
            int childLayer = (int) pChild->GetLayerType();
            if (childLayer != lastLayer)
            {
                Assert(lastLayer > childLayer);
                switch (childLayer)
                {
                case DISPNODELAYER_NEGATIVEZ:
                    if (lastLayer == DISPNODELAYER_FLOW)
                    {
                        pContext->_offset -= *di._pInsetOffset;
                    }
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                case DISPNODELAYER_FLOW:
                    pContext->_offset += *di._pInsetOffset;
                    pContext->_rcClip = di._rcFlowClip;
                    break;
                default:
                    Assert(childLayer == DISPNODELAYER_POSITIVEZ);
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                }
                lastLayer = childLayer;
            }

            // if we found the first child to draw, stop further PreDraw calcs
            if (PreDrawChild(pChild, pContext, saveContext))
                return TRUE;
        }
    }

    // restore previous context
    *pContext = saveContext;
    
    // if this container is opaque, check to see if it needs to be subtracted
    // from the redraw region
    return IsSet(CDispFlags::s_opaqueNode) &&
        pContext->IntersectsRedrawRegion(_rcContainer) &&
        CDispNode::PreDraw(pContext);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawClientLayer
//
//  Synopsis:   Draw the indicated client layer in-between our normal layers.
//              
//  Arguments:  pContext        draw context
//              dwClientLayer   which client layer to draw
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispContainer::DrawClientLayer(
            CDispDrawContext* pContext,
            const CDispInfo& di,
            DWORD dwClientLayer)
{
    // BUGBUG (donmarsh) - we play a lot of offset and clipping games here,
    // because behaviors can't handle non-zero origins, and they don't know
    // about scrolling.  Behaviors always use physical clipping because
    // we aren't passing them the rect that they actually need to draw.
    // Yes, this has very nasty performance implications!

    CDispContext saveContext(*pContext);
    pContext->AddGlobalOffset(di._contentOffset);
    pContext->_rcClip.OffsetRect(-di._contentOffset);

    CRect rcClip(di._rcBackgroundClip);
    // BUGBUG (donmarsh) - remove scrolling offset (see above)
    rcClip.OffsetRect(di._scrollOffset);

    // BUGBUG (donmarsh) - we can't ask behaviors to render at negative coordinates
    if (IsRightToLeft())
    {
        rcClip.OffsetX(di._sizeBackground.cx);
        pContext->_rcClip.OffsetX(di._sizeBackground.cx);
        pContext->_offset.cx -= di._sizeBackground.cx;
    }

    // calculate intersection with redraw region
    pContext->IntersectRedrawRegion(&rcClip);

    if (!rcClip.IsEmpty())
    {
        CRect rc(di._sizeBackground);

        _pDispClient->DrawClientLayers(
            &rc,
            &rcClip,
            pContext->GetDispSurface(),
            this,
            0,
            pContext->GetClientData(),
            dwClientLayer);
    }

    *pContext = saveContext;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawSelf
//
//  Synopsis:   Draw this node's children, plus optional background.
//
//  Arguments:  pContext        draw context
//              pChild          start drawing at this child
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(AllSet(pContext->_drawSelector));
    Assert(IsSet(CDispFlags::s_savedRedrawRegion) ||
           pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));

    // calculate clip and position info
    CDispExtras* pExtras = GetExtras();
    CDispInfo di(pExtras);
    CalcDispInfo(pContext->GetClipRect(), &di);

    // save old clip rect and offset
    CDispContext saveContext(*pContext);
    pContext->AddGlobalOffset(di._borderOffset);
    pContext->SetClipRect(di._rcContainerClip);

    // did this node save the redraw region during PreDraw processing?
    pContext->PopRedrawRegionForKey((void*)this);

    // draw optional border
    if (HasBorder(pExtras))
    {
        DrawBorder(pContext, &di);
    }


    DWORD   dwClientLayers = _pDispClient->GetClientLayersInfo(this);

    if (dwClientLayers & CLIENTLAYERS_BEFOREBACKGROUND)
    {
        DrawClientLayer(pContext, di, CLIENTLAYERS_BEFOREBACKGROUND);
    }

    // draw optional background
    if (HasBackground() && pChild == NULL && !(dwClientLayers & CLIENTLAYERS_DISABLEBACKGROUND))
    {
        DrawBackground(pContext, &di);
    }

    if (dwClientLayers & CLIENTLAYERS_AFTERBACKGROUND)
    {
        DrawClientLayer(pContext, di, CLIENTLAYERS_AFTERBACKGROUND);
    }

    CSize scrollOffset = di._contentOffset + di._scrollOffset;
    
    // draw children, bottom layers to top
    if (pChild == NULL)
    {
        pChild = _pFirstChildNode;
    }
    
    // draw children in negative Z layer
    if (dwClientLayers & CLIENTLAYERS_DISABLENEGATIVEZ)
    {
        SkipLayer(DISPNODELAYER_NEGATIVEZ, &pChild);
    }
    else if (pChild != NULL && pChild->GetLayerType() == DISPNODELAYER_NEGATIVEZ)
    {
        DrawChildren(
            DISPNODELAYER_NEGATIVEZ,
            scrollOffset,
            di._rcPositionedClip,
            pContext,
            &pChild);
    }
    
    // draw children in flow layer
    if (dwClientLayers & CLIENTLAYERS_BEFORECONTENT)
    {
        DrawClientLayer(pContext, di, CLIENTLAYERS_BEFORECONTENT);
    }
    if (dwClientLayers & CLIENTLAYERS_DISABLECONTENT)
    {
        SkipLayer(DISPNODELAYER_FLOW, &pChild);
    }
    else if (pChild != NULL && pChild->GetLayerType() == DISPNODELAYER_FLOW)
    {
        DrawChildren(
            DISPNODELAYER_FLOW,
            scrollOffset + *di._pInsetOffset,
            di._rcFlowClip,
            pContext,
            &pChild);
    }
    if (dwClientLayers & CLIENTLAYERS_AFTERCONTENT)
    {
        DrawClientLayer(pContext, di, CLIENTLAYERS_AFTERCONTENT);
    }
    
    // draw children in negative Z layer
    if (dwClientLayers & CLIENTLAYERS_DISABLEPOSITIVEZ)
    {
        SkipLayer(DISPNODELAYER_POSITIVEZ, &pChild);
    }
    else if (pChild != NULL && pChild->GetLayerType() == DISPNODELAYER_POSITIVEZ)
    {
        DrawChildren(
            DISPNODELAYER_POSITIVEZ,
            scrollOffset,
            di._rcPositionedClip,
            pContext,
            &pChild);
    }
    if (dwClientLayers & CLIENTLAYERS_AFTERFOREGROUND)
    {
        DrawClientLayer(pContext, di, CLIENTLAYERS_AFTERFOREGROUND);
    }
    
    // restore context
    *pContext = saveContext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawBorderAndScrollbars
//
//  Synopsis:   Draw border and scroll bars if they intersect the redraw region.
//
//  Arguments:  pContext        display context
//              prcContent      returns the content rect inside the border
//
//  Notes:      This is an accelerator for the top-level container node only!
//
//----------------------------------------------------------------------------

void
CDispContainer::DrawBorderAndScrollbars(
    CDispDrawContext* pContext,
    CRect* prcContent)
{
    // draw optional border
    CDispExtras* pExtras = GetExtras();
    if (HasBorder(pExtras))
    {
        // calculate content rectangle
        CDispInfo di(pExtras);
        prcContent->SetRect(
            di._prcBorderWidths->TopLeft(),
            _rcContainer.Size()
            - di._prcBorderWidths->TopLeft().AsSize()
            - di._prcBorderWidths->BottomRight().AsSize());

        // does redraw region intersect the border?
        if (!pContext->GetRedrawRegion()->BoundsInside(*prcContent))
        {
            CRect rcBorder(_rcContainer.Size());
            CRect rcClip(pContext->GetClipRect());
            rcClip.IntersectRect(rcBorder);
            
            _pDispClient->DrawClientBorder(
                &rcBorder,
                &rcClip,
                pContext->GetDispSurface(),
                this,
                pContext->GetClientData(),
                0);
        }
    }

    // no border, so content rect is just the container bounds in global coords
    else
    {
        *prcContent = _rcContainer;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawBorder
//              
//  Synopsis:   Draw border (shared for filtered or unfiltered case).
//              
//  Arguments:  pContext    draw context
//              pDI         clipping and offset information for various layers
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispContainer::DrawBorder(CDispDrawContext* pContext, CDispInfo* pDI)
{
    Assert(HasBorder());
    
    if (IsVisible())
    {
        // does redraw region intersect the border?
        CRect rcInsideBorder(
            pDI->_prcBorderWidths->TopLeft(),
            _rcContainer.Size()
            - pDI->_prcBorderWidths->TopLeft().AsSize()
            - pDI->_prcBorderWidths->BottomRight().AsSize());
        pContext->Transform(&rcInsideBorder);
        if (!pContext->GetRedrawRegion()->BoundsInside(rcInsideBorder))
        {
            CRect rcBorder(_rcContainer.Size());
            _pDispClient->DrawClientBorder(
                &rcBorder,
                &pDI->_rcContainerClip,
                pContext->GetDispSurface(),
                this,
                pContext->GetClientData(),
                0);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawBackground
//              
//  Synopsis:   Draw background (shared for filtered or unfiltered case).
//              
//  Arguments:  pContext    draw context
//              pDI         clipping and offset information for various layers
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispContainer::DrawBackground(CDispDrawContext* pContext, CDispInfo* pDI)
{
    if (IsVisible())
    {
        CDispContext saveContext(*pContext);
        pContext->AddGlobalOffset(pDI->_contentOffset);
        pContext->SetClipRect(pDI->_rcBackgroundClip);
        
        // calculate intersection with redraw region
        CRect rcBackground;
        if (!IsRightToLeft())
        {
            rcBackground.SetRect(pDI->_sizeBackground);
        }
        else
        {
            rcBackground.SetRect(-pDI->_sizeBackground.cx, 0, 0, pDI->_sizeBackground.cy);
        }
        CRect rcBackgroundClip(pDI->_rcBackgroundClip);
        if (!IsSet(CDispFlags::s_fixedBackground))
        {
            pContext->AddGlobalOffset(pDI->_scrollOffset);
        }
        pContext->IntersectRedrawRegion(&rcBackgroundClip);
        if (!rcBackgroundClip.IsEmpty())
        {
            _pDispClient->DrawClientBackground(
                &rcBackground,
                &rcBackgroundClip,
                pContext->GetDispSurface(),
                this,
                pContext->GetClientData(),
                0);
        }
        
        *pContext = saveContext;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DrawChildren
//              
//  Synopsis:   Draw children for a particular layer, starting at the indicated
//              child.
//              
//  Arguments:  layer       only draw children in this layer
//              offset      add this to the current offset in the context
//              rcClip      set this as the clip rect in the context
//              pContext    draw context
//              ppChildNode [in] child to start drawing, if it is in the
//                          requested layer
//                          [out] child in next layer (may be NULL)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispContainer::DrawChildren(
        DISPNODELAYER layer,
        const CSize& offset,
        const CRect& rcClip,
        CDispDrawContext* pContext,
        CDispNode** ppChildNode)
{
    CDispNode* pChild = *ppChildNode;
    if (pChild == NULL || pChild->GetLayerType() != layer)
        return;
    
    CDispContext saveContext(*pContext);
    pContext->AddGlobalOffset(offset);
    pContext->SetClipRect(rcClip);
    
    for (;
         pChild != NULL && pChild->GetLayerType() == layer;
         pChild = pChild->_pNextSiblingNode)
    {
        // is this child visible and in view?
        if (pChild->AllSet(pContext->_drawSelector))
            pChild->Draw(pContext, NULL);
    }
    
    // remember new child pointer for subsequent layers
    *ppChildNode = pChild;
    
    // restore context
    *pContext = saveContext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::HitTestPoint
//
//  Synopsis:   Determine whether any of our children, OR THIS CONTAINER,
//              intersects the hit test point.
//
//  Arguments:  pContext        hit context
//
//  Returns:    TRUE if any child or this container intersects the hit test pt.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispContainer::HitTestPoint(CDispHitContext* pContext) const
{
    Assert(IsSet(CDispFlags::s_visibleBranch));
    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
    //
    
    Assert( pContext->FuzzyRectIsHit(_rcVisBounds, IsFatHitTest() ) );

    CPoint ptSave(pContext->_ptHitTest);

    // calculate clip and position info
    CDispExtras* pExtras = GetExtras();
    CDispInfo di(pExtras);
    CalcDispInfo(pContext->_rcClip, &di);

    {
        CSaveDispContext save(pContext);

        // translate hit point to scrolled coordinates
        pContext->_ptHitTest -=
            di._borderOffset + di._contentOffset + di._scrollOffset;

        // search for a hit from foreground layers to background
        int lastLayer = (int)DISPNODELAYER_POSITIVEZ + 1;
        for (CDispNode* pChild = _pLastChildNode;
             pChild != NULL;
             pChild = pChild->_pPreviousSiblingNode)
        {
            // if this branch has no visible children, skip it
            if (!pChild->IsSet(CDispFlags::s_visibleBranch))
                continue;

            // switch clip rectangles and offsets between different layer types
            int childLayer = (int) pChild->GetLayerType();
            if (childLayer != lastLayer)
            {
                Assert(lastLayer > childLayer);
                switch (childLayer)
                {
                case DISPNODELAYER_NEGATIVEZ:
                    if (lastLayer == DISPNODELAYER_FLOW)
                        pContext->_ptHitTest += *di._pInsetOffset;
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                case DISPNODELAYER_FLOW:
                    pContext->_ptHitTest -= *di._pInsetOffset;
                    pContext->_rcClip = di._rcFlowClip;
                    break;
                default:
                    Assert(childLayer == DISPNODELAYER_POSITIVEZ);
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                }
                lastLayer = childLayer;

                //
                // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
                // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
                // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
                //

                // can any child in this layer contain the hit point?
                
                if (!pContext->FuzzyRectIsHit(pContext->_rcClip, IsFatHitTest() ))
                {
                    // skip to previous layer
                    while (pChild->_pPreviousSiblingNode != NULL &&
                        (int)pChild->_pPreviousSiblingNode->GetLayerType() == childLayer)
                    {
                        pChild = pChild->_pPreviousSiblingNode;
                    }
                    continue;
                }
            }

            // restrict hits to inside user clip rect
            CRect rcChild;
            if (pChild->HasUserClip())
            {
                // BUGBUG (donmarsh) -- the bounds of the user clip will be
                // be used for the fuzzy hit test, which means we could hit
                // outside the user clip rect
                rcChild = pChild->GetUserClip();
                rcChild.OffsetRect(pChild->GetBounds().TopLeft().AsSize());
                rcChild.IntersectRect(pChild->_rcVisBounds);
            }
            else
            {
                rcChild = pChild->_rcVisBounds;
            }
            //
            // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
            // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
            // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
            //            
            if (pContext->FuzzyRectIsHit(rcChild, pChild->IsFatHitTest() ) &&
                pChild->HitTestPoint(pContext))
            {
                // NOTE: don't bother to restore _ptHitTest for speed
                return TRUE;
            }
        }
    }

    //
    // no children were hit, check to see if this container was hit
    //
    if (IsVisible())
    {
        pContext->_ptHitTest =
            ptSave - di._borderOffset - di._contentOffset - di._scrollOffset;
        if (pContext->RectIsHit(di._rcBackgroundClip))
        {
            if (_pDispClient->HitTestContent(
                    &pContext->_ptHitTest,
                    (CDispContainer*)this,
                    pContext->_pClientData))
            {
                // NOTE: don't bother to restore _ptHitTest for speed
                return TRUE;
            }
        }
    
        // check for border hit
        else if (HasBorder(pExtras))
        {
            pContext->_ptHitTest = ptSave - di._borderOffset;
            if (pContext->RectIsHit(di._rcContainerClip) &&
                (pContext->_ptHitTest.x < di._prcBorderWidths->left ||
                 pContext->_ptHitTest.y < di._prcBorderWidths->top ||
                 pContext->_ptHitTest.x >= _rcContainer.Width() - di._prcBorderWidths->right ||
                 pContext->_ptHitTest.y >= _rcContainer.Height() - di._prcBorderWidths->bottom))
            {
                if (_pDispClient->HitTestBorder(
                        &pContext->_ptHitTest,
                        (CDispContainer*)this,
                        pContext->_pClientData))
                {
                    // NOTE: don't bother to restore _ptHitTest for speed
                    return TRUE;
                }
            }
        }
    }
    
        
    // restore hit point
    pContext->_ptHitTest = ptSave;

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
    //
    
    // do fuzzy hit test if requested
    if (IsVisible() &&
        pContext->_cFuzzyHitTest &&
        !pContext->RectIsHit(_rcContainer) &&
        pContext->FuzzyRectIsHit(_rcContainer, IsFatHitTest() ) &&
        _pDispClient->HitTestFuzzy(
            &pContext->_ptHitTest,
            (CDispContainer*)this,
            pContext->_pClientData))
    {
        return TRUE;
    }
    
    return FALSE;
}


CDispScroller *
CDispContainer::HitScrollInset(CPoint *pptHit, DWORD *pdwScrollDir)
{
    CDispScroller * pDispScroller;
    CPoint ptSave(*pptHit);

    // calculate clip and position info
    CDispInfo di(GetExtras());
    CalcDispInfo(g_Zero.rc, &di);

    // translate hit point to scrolled coordinates
    *pptHit -= di._borderOffset + di._contentOffset + di._scrollOffset;

    pDispScroller = super::HitScrollInset(pptHit, pdwScrollDir);

    // restore hit point
    *pptHit = ptSave;

    return pDispScroller;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::CalcDispInfo
//
//  Synopsis:   Calculate clipping and positioning info for this node.
//
//  Arguments:  rcClip          current clip rect
//              pdi             display info structure
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::CalcDispInfo(
        const CRect& rcClip,
        CDispInfo* pdi) const
{
    //
    // NOTE: content coordinates == container coordinates, because there are no
    // borders
    //
    // user clip = current clip, in content coordinates
    //
    // border clip = current clip INTERSECT container bounds,
    //   in container coordinates
    //
    // flow clip = current clip INTERSECT container bounds,
    //   in content coordinates (same as border clip)
    //

    CDispInfo& di = *pdi;   // notational convenience

    // no scrolling or inset
    di._scrollOffset = g_Zero.size;
    di._pInsetOffset = &((CSize&)g_Zero.size);

    // content size
    _rcContainer.GetSize(&di._sizeContent);
    
    // background size
    di._sizeBackground = di._sizeContent;

    // offset to local coordinates
    _rcContainer.GetTopLeft(&(di._borderOffset.AsPoint()));

    // calc positioned clip
    di._rcPositionedClip = rcClip;
    di._rcPositionedClip.OffsetRect(-di._borderOffset);

    // container clip is clipped by container bounds
    ClipToParentCoords(&di._rcContainerClip, di._rcPositionedClip, di._sizeContent);

    // flow clip = container clip since there are no borders
    di._contentOffset = g_Zero.size;
    di._rcBackgroundClip = di._rcContainerClip;

    // adjust for right to left coordinate system
    if (IsRightToLeft())
    {
        LONG offset = _rcContainer.Width();
        di._contentOffset.cx = offset;
        di._rcPositionedClip.OffsetX(-offset);
        di._rcBackgroundClip.OffsetX(-offset);
    }

    di._rcFlowClip = di._rcBackgroundClip;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::GetNodeTransform
//
//  Synopsis:   Return an offset that can be added to points in the source
//              coordinate system, producing values for the destination
//              coordinate system.
//
//  Arguments:  pOffset         returns offset value
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::GetNodeTransform(
        CSize* pOffset,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    Assert(destination < source);
    BOOL fRightToLeft = IsRightToLeft();

    *pOffset = g_Zero.size;

    switch (source)
    {
    case COORDSYS_NONFLOWCONTENT:
    case COORDSYS_CONTENT:
        // COORDSYS_CONTENT --> COORDSYS_CONTAINER
        if(fRightToLeft)
            pOffset->cx += _rcContainer.Width();
        if (destination == COORDSYS_CONTAINER)
            break;
        // fall thru to continue transformation

    case COORDSYS_CONTAINER:
        // COORDSYS_CONTAINER --> COORDSYS_PARENT
        *pOffset += _rcContainer.TopLeft().AsSize();

        if (destination == COORDSYS_PARENT)
            break;
        // fall thru to continue transformation

    default:
        Assert(source != COORDSYS_GLOBAL);
        // COORDSYS_PARENT --> COORDSYS_GLOBAL
        if (_pParentNode != NULL)
        {
            CSize globalOffset;
            _pParentNode->GetNodeTransform(
                &globalOffset,
                GetContentCoordinateSystem(),
                COORDSYS_GLOBAL);
            *pOffset += globalOffset;
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::GetNodeTransform
//
//  Synopsis:   Return a context that can be used to transform values
//              in the source coordinate system, producing values for the
//              destination coordinate system.
//
//  Arguments:  pContext        returns context
//              source          source coordinate system
//              destination     destination coordinate system
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::GetNodeTransform(
        CDispContext* pContext,
        COORDINATE_SYSTEM source,
        COORDINATE_SYSTEM destination) const
{
    Assert(destination < source);
    BOOL fRightToLeft = IsRightToLeft();

    CSize& offset = pContext->_offset;
    CRect& rcClip = pContext->_rcClip;

    switch (source)
    {
    case COORDSYS_NONFLOWCONTENT:
    case COORDSYS_CONTENT:
        // COORDSYS_CONTENT --> COORDSYS_CONTAINER
        // flow children clip to container
        if (source == COORDSYS_CONTENT)
        {
            rcClip.SetRect(_rcContainer.Size());
            if (fRightToLeft)
            {
                rcClip.MirrorX();
            }
        }
        else
            pContext->SetNoClip();

        offset = g_Zero.size;

        if(fRightToLeft)
            offset.cx += _rcContainer.Width();
        if (destination == COORDSYS_CONTAINER)
            break;
        // fall thru to continue transformation

    case COORDSYS_CONTAINER:
        // COORDSYS_CONTAINER --> COORDSYS_PARENT
        if (source == COORDSYS_CONTAINER)
        {
            pContext->SetNoClip();
            offset = _rcContainer.TopLeft().AsSize();
        }
        else
        {
            offset += _rcContainer.TopLeft().AsSize();
        }
        if (destination == COORDSYS_PARENT)
            break;
        // fall thru to continue transformation

    default:
        Assert(source != COORDSYS_GLOBAL);
        // COORDSYS_PARENT --> COORDSYS_GLOBAL
        if (source == COORDSYS_PARENT)
        {
            if (_pParentNode != NULL)
            {
                _pParentNode->GetNodeTransform(
                    pContext,
                    GetContentCoordinateSystem(),
                    COORDSYS_GLOBAL);
            }
            else
                pContext->SetToIdentity();
        }
        else
        {
            if (_pParentNode != NULL)
            {
                CDispContext globalContext;
                _pParentNode->GetNodeTransform(
                    &globalContext,
                    GetContentCoordinateSystem(),
                    COORDSYS_GLOBAL);
                globalContext._rcClip.OffsetRect(-offset);  // to source coords.
                rcClip.IntersectRect(globalContext._rcClip);
                offset += globalContext._offset;
            }
        }
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::PushContext
//
//  Synopsis:   Get context information for the given child node.
//
//  Arguments:  pChild          the child node
//              pContextStack   context stack to save context changes in
//              pContext        display context
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::PushContext(
        const CDispNode* pChild,
        CDispContextStack* pContextStack,
        CDispContext* pContext) const
{
    super::PushContext(pChild, pContextStack, pContext);

    // modify context for child
    CDispContext childContext;
    GetNodeTransform(
        &childContext,
        pChild->GetContentCoordinateSystem(),
        COORDSYS_PARENT);
    pContext->_rcClip.OffsetRect(-childContext._offset);
    pContext->_rcClip.IntersectRect(childContext._rcClip);
    pContext->_offset += childContext._offset;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::ComputeVisibleBounds
//
//  Synopsis:   Compute visible bounds for an interior node, marking children
//              that determine the edges of these bounds
//
//  Arguments:  none
//
//  Returns:    TRUE if visible bounds changed.
//
//----------------------------------------------------------------------------

BOOL
CDispContainer::ComputeVisibleBounds()
{
    // visible bounds is always the size of the container, and may be extended
    // by items in Z layers that fall outside these bounds
    CRect rcBounds(_rcContainer);
    
    if (!IsFiltered())
    {
        CSize offset(_rcContainer.TopLeft().AsSize());
        if (IsRightToLeft())
        {
            offset.cx = _rcContainer.right;
        }
    
        for (CDispNode* pChild = _pFirstChildNode;
            pChild != NULL;
            pChild = pChild->_pNextSiblingNode)
        {
            if (IsPositionedLayer(pChild->GetLayerType()))
            {
                CRect rcChild(pChild->_rcVisBounds);
                if (!rcChild.IsEmpty())
                {
                    rcChild.OffsetRect(offset);
                    rcBounds.Union(rcChild);
                }
            }
        }
    }

    if (rcBounds != _rcVisBounds)
    {
        _rcVisBounds = rcBounds;
        return TRUE;
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::SubtractOpaqueChildren
//
//  Synopsis:   Subtract bounds of children specified by context selector from
//              the given region.
//
//  Arguments:  prgn        region to subtract from
//              pContext    display context
//
//  Returns:    TRUE if the resulting region is not empty
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispContainer::SubtractOpaqueChildren(
        CRegion* prgn,
        CDispContext* pContext)
{
    // opaque node should be handled by our caller
    Assert(!IsSet(CDispFlags::s_opaqueNode));

    BOOL fRegionEmpty = FALSE;

    CSaveDispContext save(pContext);

    // calculate clip and position info
    CDispInfo di(GetExtras());
    CalcDispInfo(pContext->_rcClip, &di);

    // offset children
    pContext->_offset +=
        di._borderOffset + di._contentOffset + di._scrollOffset;

    int lastLayer = (int)DISPNODELAYER_NEGATIVEZ - 1;
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        // select only opaque visible children
        if (pChild->AllSet(CDispFlags::s_subtractOpaqueSelector))
        {
            // switch clip rectangles and offsets between different layer types
            int childLayer = (int) pChild->GetLayerType();
            if (childLayer != lastLayer)
            {
                Assert(lastLayer < childLayer);
                switch (childLayer)
                {
                case DISPNODELAYER_NEGATIVEZ:
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                case DISPNODELAYER_FLOW:
                    pContext->_offset += *di._pInsetOffset;
                    pContext->_rcClip = di._rcFlowClip;
                    break;
                default:
                    Assert(childLayer == DISPNODELAYER_POSITIVEZ);
                    if (lastLayer == DISPNODELAYER_FLOW)
                    {
                        pContext->_offset -= *di._pInsetOffset;
                    }
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                }
                lastLayer = childLayer;
            }

            if (pChild->IsLeafNode() ||
                pChild->IsSet(CDispFlags::s_opaqueNode))
            {
                CRect rcOpaque;
                pChild->GetBounds(&rcOpaque);
                pContext->Transform(&rcOpaque);
                prgn->Subtract(rcOpaque);
                fRegionEmpty = prgn->IsEmpty();
                break;
            }
            else
            {
                CDispInteriorNode* pInterior =
                    DYNCAST(CDispInteriorNode, pChild);
                if (!pInterior->SubtractOpaqueChildren(prgn, pContext))
                {
                    fRegionEmpty = TRUE;
                    break;
                }
            }
        }
    }

    return !fRegionEmpty;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::CalculateInView
//
//  Synopsis:   Calculate whether this node and its children are in view or not.
//
//  Arguments:  pContext            display context
//              fPositionChanged    TRUE if position changed
//              fNoRedraw           TRUE to suppress redraw (after scrolling)
//
//  Returns:    TRUE if this node is in view
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispContainer::CalculateInView(
        CDispContext* pContext,
        BOOL fPositionChanged,
        BOOL fNoRedraw)
{
    // apply user clip
    CApplyUserClip applyUC(this, pContext);
    
    BOOL fInView = _rcVisBounds.Intersects(pContext->_rcClip);
    BOOL fWasInView = IsSet(CDispFlags::s_inView);

    // calculate in view status of children unless this node is not in view
    // and was not in view
    if (fInView || fWasInView)
    {
        // accelerated way to clear in view status of all children, unless
        // some child needs change notification
        if (!fInView && !IsSet(CDispFlags::s_inViewChange))
        {
            ClearSubtreeFlags(CDispFlags::s_inView);
            return FALSE;
        }

        CDispContext saveContext(*pContext);

        // calculate clip and position info
        CDispExtras* pExtras = GetExtras();
        CDispInfo di(pExtras);
        CalcDispInfo(pContext->_rcClip, &di);

        // set up for flow content
        pContext->_rcClip = di._rcFlowClip;
        pContext->_offset +=
            di._borderOffset + di._contentOffset + di._scrollOffset;

        int lastLayer = (int)DISPNODELAYER_NEGATIVEZ - 1;
        for (CDispNode* pChild = _pFirstChildNode;
             pChild != NULL;
             pChild = pChild->_pNextSiblingNode)
        {
            // switch clip rectangles and offsets between different layer types
            int childLayer = (int) pChild->GetLayerType();
            if (childLayer != lastLayer)
            {
                Assert(lastLayer < childLayer);
                switch (childLayer)
                {
                case DISPNODELAYER_NEGATIVEZ:
                    pContext->SetClipRect(di._rcPositionedClip);
                    break;
                case DISPNODELAYER_FLOW:
                    pContext->_offset += *di._pInsetOffset;
                    pContext->SetClipRect(di._rcFlowClip);
                    break;
                default:
                    Assert(childLayer == DISPNODELAYER_POSITIVEZ);
                    if (lastLayer == DISPNODELAYER_FLOW)
                    {
                        pContext->_offset -= *di._pInsetOffset;
                    }
                    pContext->SetClipRect(di._rcPositionedClip);
                    break;
                }
                lastLayer = childLayer;
            }

            pChild->CalculateInView(pContext, fPositionChanged, fNoRedraw);
        }

        *pContext = saveContext;
    }

    SetBoolean(CDispFlags::s_inView, fInView);
    return fInView;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::RecalcChildren
//
//  Synopsis:   Recalculate children.
//
//  Arguments:  fForceRecalc        TRUE to force recalc of this subtree
//              fSuppressInval      TRUE to suppress child invalidation
//              pContext            draw context
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispContainer::RecalcChildren(
        BOOL fForceRecalc,
        BOOL fSuppressInval,
        CDispDrawContext* pContext)
{
    CDispContext saveContext(*pContext);

    // calculate clip and position info
    CDispExtras* pExtras = GetExtras();
    CDispInfo di(pExtras);
    CalcDispInfo(pContext->_rcClip, &di);

    // offset children
    pContext->_offset +=
        di._borderOffset + di._contentOffset + di._scrollOffset;

    CDispFlags childrenFlags(0);

    int lastLayer = (int)DISPNODELAYER_NEGATIVEZ - 1;
    for (CDispNode* pChild = _pFirstChildNode;
         pChild != NULL;
         pChild = pChild->_pNextSiblingNode)
    {
        Assert(fForceRecalc ||
            pChild->IsSet(CDispFlags::s_recalc) ||
            !pChild->IsSet(CDispFlags::s_inval));

        if (fForceRecalc || pChild->IsSet(CDispFlags::s_recalc))
        {
            // switch clip rectangles and offsets between different layer types
            int childLayer = (int) pChild->GetLayerType();
            if (childLayer != lastLayer)
            {
                Assert(lastLayer < childLayer);
                switch (childLayer)
                {
                case DISPNODELAYER_NEGATIVEZ:
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                case DISPNODELAYER_FLOW:
                    pContext->_offset += *di._pInsetOffset;
                    pContext->_rcClip = di._rcFlowClip;
                    break;
                default:
                    Assert(childLayer == DISPNODELAYER_POSITIVEZ);
                    if (lastLayer == DISPNODELAYER_FLOW)
                    {
                        pContext->_offset -= *di._pInsetOffset;
                    }
                    pContext->_rcClip = di._rcPositionedClip;
                    break;
                }
                lastLayer = childLayer;
            }

            pChild->Recalc(fForceRecalc, fSuppressInval, pContext);
        }
        Assert(!pChild->IsSet(CDispFlags::s_inval));
        Assert(!pChild->IsSet(CDispFlags::s_recalc));
        Assert(!pChild->IsSet(CDispFlags::s_recalcChildren));
        childrenFlags.Set(pChild->GetFlags());
    }

    // ensure that we don't bother to invalidate anything during bounds calc.
    SetFlag(CDispFlags::s_recalc);
    ComputeVisibleBounds();

    // propagate flags from children, and clear recalc and inval flags
    SetFlags(childrenFlags, CDispFlags::s_propagatedAndRecalcAndInval);
    
    // restore context clip and offset values
    *pContext = saveContext;
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispContainer::DumpBounds
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//----------------------------------------------------------------------------

void
CDispContainer::DumpBounds(HANDLE hFile, long level, long childNumber)
{
    super::DumpBounds(hFile, level, childNumber);

    WriteString(hFile, _T("<i>rcContainer:</i>"));
    DumpRect(hFile, _rcContainer);
}
#endif


