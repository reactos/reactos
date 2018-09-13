//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispitemplus.cxx
//
//  Contents:   A complex display item supporting background, border, and
//              user clip.
//
//  Classes:    CDispItemPlus
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_SAVEDISPCONTEXT_HXX_
#define X_SAVEDISPCONTEXT_HXX_
#include "savedispcontext.hxx"
#endif

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

MtDefine(CDispItemPlus, DisplayTree, "CDispItemPlus")


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::SetSize
//
//  Synopsis:   Set size for this node.
//
//  Arguments:  size            new size
//              fInvalidateAll  TRUE to invalidate entire contents of this node
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispItemPlus::SetSize(const SIZE& size, BOOL fInvalidateAll)
{
    CDispInfo di((CDispExtras*) _extra);
    super::SetSize(size, *di._prcBorderWidths, fInvalidateAll);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::ScrollRectIntoView
//
//  Synopsis:   Scroll the given rect in the indicated coordinate system into
//              view, with various pinning (alignment) options.
//
//  Arguments:  rc              rect to scroll into view
//              spVert          scroll pin request, vertical axis
//              spHorz          scroll pin request, horizontal axis
//              coordSystem     coordinate system for rc
//
//  Returns:    TRUE if any scrolling was necessary.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispItemPlus::ScrollRectIntoView(
        const CRect& rc,
        CRect::SCROLLPIN spVert,
        CRect::SCROLLPIN spHorz,
        COORDINATE_SYSTEM coordSystem,
        BOOL fScrollBits)
{
    Assert(coordSystem == COORDSYS_CONTENT);

    return super::ScrollRectIntoView(rc, spVert, spHorz, coordSystem, fScrollBits);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::CalculateInView
//
//  Synopsis:   Determine whether this leaf node is in view, and whether its
//              client must be notified.
//
//  Arguments:  pContext            display context
//              fPositionChanged    TRUE if position changed
//              fNoRedraw           TRUE to suppress redraw (after scrolling)
//
//  Returns:    TRUE if this node is in view.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispItemPlus::CalculateInView(
        CDispContext* pContext,
        BOOL fPositionChanged,
        BOOL fNoRedraw)
{
    // apply user clip
    CApplyUserClip applyUC(this, pContext);
    
    BOOL fInView = _rcVisBounds.Intersects(pContext->_rcClip);
    BOOL fWasInView = IsSet(CDispFlags::s_inView);

    // notify client if client requests it and view status changes
    if (AllSet(CDispFlags::s_inViewChangeAndVisibleBranch) &&
        (fInView || fWasInView))
    {
        if (fPositionChanged)
        {
            SetFlag(CDispFlags::s_positionHasChanged);
        }
        NotifyInViewChange(pContext, fInView, fWasInView, fNoRedraw);
    }

    SetBoolean(CDispFlags::s_inView, fInView);
    return fInView;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::DrawSelf
//
//  Synopsis:   Draw this item.
//
//  Arguments:  pContext        draw context
//              pChild          start drawing at this child
//
//----------------------------------------------------------------------------

void
CDispItemPlus::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild)
{
    // shouldn't be called unless this node was selected to draw
    Assert(AllSet(pContext->_drawSelector));
    Assert(IsSet(CDispFlags::s_savedRedrawRegion) ||
           pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(pChild == NULL);

    // BUGBUG (donmarsh) -- buffering not implemented yet
    Assert(!IsSet(CDispFlags::s_bufferInvalid));

    // calculate clip and position info
    CDispInfo di((CDispExtras*) _extra);
    CalcDispInfo(pContext->GetClipRect(), &di);
    
    // save old clip rect and offset
    CDispContext saveContext(*pContext);
    pContext->AddGlobalOffset(di._borderOffset);
    pContext->SetClipRect(di._rcContainerClip);
    
    // did this node save the redraw region during PreDraw processing?
    if (IsSet(CDispFlags::s_savedRedrawRegion))
    {
        pContext->PopRedrawRegionForKey((void*)this);
    }

    // draw optional border
    if (HasBorder((CDispExtras*) _extra))
    {
        DrawBorder(pContext, &di);
    }

    // offset content
    pContext->AddGlobalOffset(di._contentOffset);

    // set up for flow content
    CRect rcContent(di._sizeContent);
    CRect rcClip;

    // adjust for right to left coordinate system
    if (IsRightToLeft())
    {
        rcContent.MirrorX();
    }

    DWORD   dwClientLayers = _pDispClient->GetClientLayersInfo(this);

    // draw optional background
    if (HasBackground() ||
         (dwClientLayers & (CLIENTLAYERS_BEFOREBACKGROUND | CLIENTLAYERS_AFTERBACKGROUND)))
    {
        pContext->SetClipRect(di._rcBackgroundClip);
        rcClip = di._rcBackgroundClip;
        pContext->IntersectRedrawRegion(&rcClip);
        rcClip.IntersectRect(rcContent);
        if (!rcClip.IsEmpty())
        {
            if (dwClientLayers & CLIENTLAYERS_BEFOREBACKGROUND)
            {
                DrawClientLayer(
                    pContext, di, rcClip, CLIENTLAYERS_BEFOREBACKGROUND);
            }

            if (!(dwClientLayers & CLIENTLAYERS_DISABLEBACKGROUND))
            {
                // draw background
                _pDispClient->DrawClientBackground(
                    &rcContent, &rcClip,
                    pContext->GetDispSurface(),
                    this, 
                    pContext->GetClientData(),
                    0);
            }

            if (dwClientLayers & CLIENTLAYERS_AFTERBACKGROUND)
            {
                DrawClientLayer(
                    pContext, di, rcClip, CLIENTLAYERS_AFTERBACKGROUND);
            }
        }
    }

    pContext->AddGlobalOffset(*di._pInsetOffset + di._scrollOffset);
    pContext->SetClipRect(di._rcFlowClip);
    rcClip = di._rcFlowClip;
    pContext->IntersectRedrawRegion(&rcClip);
    rcClip.IntersectRect(rcContent);

    if (!rcClip.IsEmpty())
    {
        // draw content
        if(!IsRightToLeft())
            rcContent.right -= di._pInsetOffset->cx;
        else
            rcContent.left += di._pInsetOffset->cx;
        rcContent.bottom -= di._pInsetOffset->cy;

        if (!dwClientLayers)
        {
            _pDispClient->DrawClient(
                &rcContent, &rcClip,
                pContext->GetDispSurface(),
                this,
                0,
                pContext->GetClientData(),
                0);
        }
        
        else
        {
            if (dwClientLayers & CLIENTLAYERS_BEFORECONTENT)
            {
                DrawClientLayer(
                    pContext, di, rcClip, CLIENTLAYERS_BEFORECONTENT);
            }
        
            if (!(dwClientLayers & CLIENTLAYERS_DISABLECONTENT))
            {
                _pDispClient->DrawClient(
                    &rcContent, &rcClip,
                    pContext->GetDispSurface(),
                    this,
                    0,
                    pContext->GetClientData(),
                    0);
            }
            
            if (dwClientLayers & CLIENTLAYERS_AFTERCONTENT)
            {
                DrawClientLayer(
                    pContext, di, rcClip, CLIENTLAYERS_AFTERCONTENT);
            }
            if (dwClientLayers & CLIENTLAYERS_AFTERFOREGROUND)
            {
                DrawClientLayer(
                    pContext, di, rcClip, CLIENTLAYERS_AFTERFOREGROUND);
            }
        }
    }

    // restore context
    *pContext = saveContext;
    
#if DBG==1
    CDebugPaint::PausePaint(tagPaintWait);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::DrawBorder
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
CDispItemPlus::DrawBorder(CDispDrawContext* pContext, CDispInfo* pDI)
{
    // does redraw region intersect the border?
    CRect rcInsideBorder(
        pDI->_prcBorderWidths->left,
        pDI->_prcBorderWidths->top,
        _rcVisBounds.Width() - pDI->_prcBorderWidths->right,
        _rcVisBounds.Height() - pDI->_prcBorderWidths->bottom);
    pContext->Transform(&rcInsideBorder);
    if (!pContext->GetRedrawRegion()->BoundsInside(rcInsideBorder))
    {
        CRect rcBorder(_rcVisBounds.Size());
        CRect rcClip(pDI->_rcContainerClip);
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


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::DrawBackground
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
CDispItemPlus::DrawBackground(CDispDrawContext* pContext, CDispInfo* pDI)
{
    CDispContext saveContext(*pContext);
    pContext->AddGlobalOffset(pDI->_contentOffset);
    
    CRect rcContent(pDI->_sizeContent);
    // BUGBUG (donmarsh) - I think there should be an RTL test here!
    CRect rcClip(pDI->_rcBackgroundClip);
    pContext->SetClipRect(rcClip);
    pContext->IntersectRedrawRegion(&rcClip);
    rcClip.IntersectRect(rcContent);
    if (!rcClip.IsEmpty())
    {
        _pDispClient->DrawClientBackground(
            &rcContent,
            &rcClip,
            pContext->GetDispSurface(),
            this,
            pContext->GetClientData(),
            0);
    }
    
    *pContext = saveContext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::DrawContent
//              
//  Synopsis:   Draw content (shared for filtered or unfiltered case).
//              
//  Arguments:  pContext    draw context
//              pDI         clipping and offset information for various layers
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispItemPlus::DrawContent(CDispDrawContext* pContext, CDispInfo* pDI)
{
    CDispContext saveContext(*pContext);
    pContext->AddGlobalOffset(pDI->_contentOffset + *pDI->_pInsetOffset);
    
    CRect rcContent(pDI->_sizeContent);
    // BUGBUG (donmarsh) - I think there should be an RTL test here!
    CRect rcClip(pDI->_rcFlowClip);
    pContext->SetClipRect(rcClip);
    pContext->IntersectRedrawRegion(&rcClip);
    rcClip.IntersectRect(rcContent);
    if (!rcClip.IsEmpty())
    {
        rcContent.right -= pDI->_pInsetOffset->cx;
        rcContent.bottom -= pDI->_pInsetOffset->cy;
        _pDispClient->DrawClient(
            &rcContent,
            &rcClip,
            pContext->GetDispSurface(),
            this,
            0,
            pContext->GetClientData(),
            0);
    }

    *pContext = saveContext;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::DrawClientLayer
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
CDispItemPlus::DrawClientLayer(
            CDispDrawContext* pContext,
            const CDispInfo& di,
            const CRect& rcClip,
            DWORD dwClientLayer)
{
    CRect rc(di._sizeBackground);

    // BUGBUG (donmarsh) - we play a lot of offset and clipping games here,
    // because behaviors can't handle non-zero origins.
    // Behaviors always use physical clipping because
    // we aren't passing them the rect that they actually need to draw.
    // Yes, this has very nasty performance implications!

    // BUGBUG (donmarsh) - we can't ask behaviors to render at negative coordinates
    if (IsRightToLeft())
    {
        CDispContext saveContext(*pContext);
        CRect rcAdjustedClip(rcClip);

        long offset = _rcVisBounds.Width() - 
            di._prcBorderWidths->left -
            di._prcBorderWidths->right;
            
        rcAdjustedClip.OffsetX(offset);
        pContext->_rcClip.OffsetX(offset);
        pContext->_offset.cx -= offset;

        _pDispClient->DrawClientLayers(
            &rc,
            &rcAdjustedClip,
            pContext->GetDispSurface(),
            this,
            0,
            pContext->GetClientData(),
            dwClientLayer);

        *pContext = saveContext;
    }

    else
    {
        _pDispClient->DrawClientLayers(
            &rc,
            &rcClip,
            pContext->GetDispSurface(),
            this,
            0,
            pContext->GetClientData(),
            dwClientLayer);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::HitTestPoint
//
//  Synopsis:   Determine whether this item intersects the hit test point.
//
//  Arguments:  pContext        hit context
//
//  Returns:    TRUE if this item intersects the hit test point.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispItemPlus::HitTestPoint(CDispHitContext* pContext) const
{
    Assert(IsSet(CDispFlags::s_visibleBranch));
    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
    //    
    Assert(pContext->FuzzyRectIsHit(_rcVisBounds, IsFatHitTest() ));

    // calculate clip and position info
    CDispInfo di((CDispExtras*) _extra);
    CalcDispInfo(pContext->_rcClip, &di);

    // hit test border
    CPoint ptSave(pContext->_ptHitTest);
    pContext->_ptHitTest -= di._borderOffset;
    if (HasBorder((CDispExtras*) _extra) &&
        pContext->RectIsHit(di._rcContainerClip) &&
        (pContext->_ptHitTest.x < di._prcBorderWidths->left ||
         pContext->_ptHitTest.y < di._prcBorderWidths->top ||
         pContext->_ptHitTest.x >= _rcVisBounds.Width() - di._prcBorderWidths->right ||
         pContext->_ptHitTest.y >= _rcVisBounds.Height() - di._prcBorderWidths->bottom))
    {
        if (_pDispClient->HitTestBorder(
                &pContext->_ptHitTest,
                (CDispNode*)this,
                pContext->_pClientData))
        {
            // NOTE: don't bother to restore _ptHitTest for speed
            return TRUE;
        }
    }

    // hit test content
    else
    {
        pContext->_ptHitTest -=
            *di._pInsetOffset + di._contentOffset + di._scrollOffset;
        di._rcBackgroundClip.OffsetRect(-*di._pInsetOffset);
        if (pContext->RectIsHit(di._rcBackgroundClip))
        {
            // (paulnel) Adjust RTL case to be offset from beginning of
            //           the line.
            if(IsRightToLeft())
                pContext->_ptHitTest.x = ptSave.x - _rcVisBounds.right;
            if (_pDispClient->HitTestContent(
                    &(pContext->_ptHitTest),
                    (CDispLeafNode*)this,
                    pContext->_pClientData))
            {
                // NOTE: don't bother to restore _ptHitTest for speed
                return TRUE;
            }
        }
    }

    pContext->_ptHitTest = ptSave;

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
    //
    
    // do fuzzy hit test if requested
    if (pContext->_cFuzzyHitTest &&
        !pContext->RectIsHit(_rcVisBounds) &&
        pContext->FuzzyRectIsHit(_rcVisBounds, IsFatHitTest() ) &&
        _pDispClient->HitTestFuzzy(
            &(pContext->_ptHitTest),
            (CDispLeafNode*)this,
            pContext->_pClientData))
    {
        return TRUE;
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::CalcDispInfo
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
CDispItemPlus::CalcDispInfo(
        const CRect& rcClip,
        CDispInfo* pdi) const
{
    //
    // user clip = current clip INTERSECT optional user clip, in content coords
    //
    // border clip = current clip INTERSECT optional user clip, in container coords
    //
    // flow clip = current clip INTERSECT optional user clip, in content coords
    //

    CDispInfo& di = *pdi;   // notational convenience

    // no scrolling
    di._scrollOffset = g_Zero.size;

    // content size
    _rcVisBounds.GetSize(&di._sizeContent);

    // offset to local coordinates
    _rcVisBounds.GetTopLeft(&(di._borderOffset.AsPoint()));

    // calc positioned clip
    di._rcPositionedClip = rcClip;
    di._rcPositionedClip.OffsetRect(-di._borderOffset);
    di._rcContainerClip = di._rcPositionedClip;

    // inset user clip and flow clip by optional border
    di._sizeContent.cx -= di._prcBorderWidths->left + di._prcBorderWidths->right;
    di._sizeContent.cy -= di._prcBorderWidths->top + di._prcBorderWidths->bottom;
    di._sizeBackground = di._sizeContent;
    
    if(!IsRightToLeft())
    {
        di._contentOffset = di._prcBorderWidths->TopLeft().AsSize();
        di._rcPositionedClip.OffsetRect(-di._contentOffset);
        di._rcBackgroundClip = di._rcPositionedClip;
        di._rcFlowClip.left = max(di._rcBackgroundClip.left, di._pInsetOffset->cx);
        di._rcFlowClip.right = di._rcBackgroundClip.right;
    }
    else
    {
        di._contentOffset.cx = _rcVisBounds.Width() - di._prcBorderWidths->right;
        di._contentOffset.cy = di._prcBorderWidths->top;
        di._rcPositionedClip.OffsetRect(-di._contentOffset);
        di._rcBackgroundClip = di._rcPositionedClip;
        di._rcFlowClip.left = di._rcBackgroundClip.left;
        di._rcFlowClip.right = min(di._rcBackgroundClip.right, di._pInsetOffset->cx);
    }

    di._rcFlowClip.top = max(di._rcBackgroundClip.top, di._pInsetOffset->cy);
    di._rcFlowClip.bottom = di._rcBackgroundClip.bottom;
    di._rcFlowClip.OffsetRect(-*di._pInsetOffset);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::GetNodeTransform
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
CDispItemPlus::GetNodeTransform(
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
        {
            CDispInfo di((CDispExtras*) _extra);
            if (!fRightToLeft)
            {
                // add inset because content of CDispItemPlus is always flow content
                *pOffset = di._prcBorderWidths->TopLeft().AsSize() +
                    *di._pInsetOffset;
            }
            else
            {
                pOffset->cx = _rcVisBounds.Width() - di._prcBorderWidths->right - di._pInsetOffset->cx;
                pOffset->cy += di._prcBorderWidths->top + di._pInsetOffset->cy;
            }
        }
        if (destination == COORDSYS_CONTAINER)
            break;
        // fall thru to continue transformation

    case COORDSYS_CONTAINER:
        // COORDSYS_CONTAINER --> COORDSYS_PARENT
        *pOffset += _rcVisBounds.TopLeft().AsSize();

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
//  Member:     CDispItemPlus::GetNodeTransform
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
CDispItemPlus::GetNodeTransform(
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
        {
            CDispInfo di((CDispExtras*) _extra);
            const CSize& topLeftBorder = di._prcBorderWidths->TopLeft().AsSize();

            if(!fRightToLeft)
            {
                offset = topLeftBorder + *di._pInsetOffset;
            }
            else
            {
                offset.cx = _rcVisBounds.Width() - di._prcBorderWidths->right + di._pInsetOffset->cx;
                offset.cy = di._prcBorderWidths->top + di._pInsetOffset->cy;
            }

            // add inset and clip for flow child
            CSize sizeContained =
                _rcVisBounds.Size() - topLeftBorder -
                di._prcBorderWidths->BottomRight().AsSize();
            rcClip.SetRect(sizeContained);

            if (fRightToLeft)
            {
                rcClip.MirrorX();
            }
        }
        if (destination == COORDSYS_CONTAINER)
            break;
        // fall thru to continue transformation

    case COORDSYS_CONTAINER:
        // COORDSYS_CONTAINER --> COORDSYS_PARENT
        if (source == COORDSYS_CONTAINER)
        {
            rcClip.SetRect(_rcVisBounds.Size());
            if (HasUserClip())
            {
                rcClip.IntersectRect(GetUserClip((CDispExtras*) _extra));
            }
            offset = _rcVisBounds.TopLeft().AsSize();
        }
        else
        {
            if (HasUserClip())
            {
                CRect rcUserClip(GetUserClip((CDispExtras*) _extra));
                rcUserClip.OffsetRect(-offset);     // to source coords.
                rcClip.IntersectRect(rcUserClip);
            }
            offset += _rcVisBounds.TopLeft().AsSize();
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
                // everything clipped to this item's bounds
                rcClip.IntersectRect(_rcVisBounds);
            }
            else
            {
                offset = g_Zero.size;
                rcClip = _rcVisBounds;
            }
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
//  Member:     CDispItemPlus::GetClientRect
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
CDispItemPlus::GetClientRect(RECT* prc, CLIENTRECT type) const
{
    switch (type)
    {
    case CLIENTRECT_BACKGROUND:
    case CLIENTRECT_CONTENT:
    case CLIENTRECT_VISIBLECONTENT:
        {
            CDispInfo di((CDispExtras*) _extra);
            ((CRect*)prc)->SetRect(
                _rcVisBounds.Size()
                - di._prcBorderWidths->TopLeft().AsSize()
                - di._prcBorderWidths->BottomRight().AsSize());
            if (prc->left >= prc->right || prc->top >= prc->bottom)
                *prc = g_Zero.rc;
            else if (IsRightToLeft() && type == CLIENTRECT_CONTENT)
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
//  Member:     CDispItemPlus::GetZOrder
//
//  Synopsis:   Return the z order of this item.
//
//  Arguments:  none
//
//  Returns:    Z order of this item.
//
//  Notes:      This method shouldn't be called unless the item is
//              in the negative or positive Z layers.
//
//----------------------------------------------------------------------------

LONG
CDispItemPlus::GetZOrder() const
{
    Assert(GetLayerType() == DISPNODELAYER_NEGATIVEZ ||
           GetLayerType() == DISPNODELAYER_POSITIVEZ);

    return _pDispClient->GetZOrderForSelf();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::NotifyInViewChange
//
//  Synopsis:   Notify client when this item's in-view status or position
//              changes.
//
//  Arguments:  pContext            display context
//              fResolvedVisible    TRUE if this item is visible and in view
//              fWasResolvedVisible TRUE if this item was visible and in view
//              fNoRedraw           TRUE to suppress redraw
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispItemPlus::NotifyInViewChange(
        CDispContext* pContext,
        BOOL fResolvedVisible,
        BOOL fWasResolvedVisible,
        BOOL fNoRedraw)
{
    // calculate clip and position info
    CDispInfo di((CDispExtras*) _extra);
    CalcDispInfo(pContext->GetClipRect(), &di);

    CRect rcClient;
    CRect rcClip(di._rcFlowClip);

    // calculate client and clip rects in global coordinates
    if(!IsRightToLeft())
    {
        CRect rcTemp(_rcVisBounds.TopLeft() + di._contentOffset + di._scrollOffset
                     + *di._pInsetOffset + pContext->_offset,
                     di._sizeContent - *di._pInsetOffset);
        rcClient = rcTemp;
    }
    else
    {
        rcClient.left = _rcVisBounds.right - di._contentOffset.cx - di._scrollOffset.cx
                    - di._pInsetOffset->cx + pContext->_offset.cx;
        rcClient.top  = _rcVisBounds.top + di._contentOffset.cy + di._scrollOffset.cy
                    + di._pInsetOffset->cy + pContext->_offset.cy;
        rcClient.right = rcClient.left + di._sizeContent.cx - di._pInsetOffset->cx;
        rcClient.bottom = rcClient.top + di._sizeContent.cy - di._pInsetOffset->cy;
    }

    rcClip.OffsetRect(di._borderOffset + di._contentOffset + di._scrollOffset
        + pContext->_offset);
    rcClip.IntersectRect(rcClient);
    if (rcClip.IsEmpty())   // normalize empty rect for ignorant clients
        rcClip.SetRectEmpty();

    DWORD viewChangedFlags = 0;
    if (fResolvedVisible)
        viewChangedFlags = VCF_INVIEW;
    if (fResolvedVisible != fWasResolvedVisible)
        viewChangedFlags |= VCF_INVIEWCHANGED;
    if (IsSet(CDispFlags::s_positionHasChanged))
        viewChangedFlags |= VCF_POSITIONCHANGED;
    if (fNoRedraw)
        viewChangedFlags |= VCF_NOREDRAW;
    _pDispClient->HandleViewChange(
        viewChangedFlags,
        &rcClient,
        &rcClip,
        this);
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispItemPlus::DumpInfo
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispItemPlus::DumpInfo(HANDLE hFile, long level, long childNumber)
{
#if 0
    IDispClientDebug* pIDebug;
    if (SUCCEEDED(
        _pDispClient->QueryInterface(IID_IDispClientDebug,(void**)&pIDebug)))
    {
        pIDebug->DumpDebugInfo(hFile, level, childNumber, this, 0);
        pIDebug->Release();
    }
#else
    _pDispClient->DumpDebugInfo(hFile, level, childNumber, this, 0);
#endif
}
#endif

