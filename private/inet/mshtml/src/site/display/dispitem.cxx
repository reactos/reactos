//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispitem.cxx
//
//  Contents:   A leaf node in the display tree which invokes client drawing.
//
//  Classes:    CDispItem
//
//----------------------------------------------------------------------------

#ifdef NEVER
//
// NOTE: This file is obsolete!  CDispItemPlus is now the only leaf node class
// used by the Display Tree.  This change was necessary due to coordinate
// system adjustments that were needed to accommodate GDI's 16-bit coordinate
// precision on Win 9x.
// 

#include "headers.hxx"

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPITEM_HXX_
#define X_DISPITEM_HXX_
#include "dispitem.hxx"
#endif

#ifndef X_DISPCONTEXT_HXX_
#define X_DISPCONTEXT_HXX_
#include "dispcontext.hxx"
#endif

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif

#ifndef X_DISPCONTENT_HXX_
#define X_DISPCONTENT_HXX_
#include "dispcontent.hxx"
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


MtDefine(CDispItem, DisplayTree, "CDispItem")


//+---------------------------------------------------------------------------
//
//  Member:     CDispItem::CalculateInView
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
CDispItem::CalculateInView(
        CDispContext* pContext,
        BOOL fPositionChanged,
        BOOL fNoRedraw)
{
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
//  Member:     CDispItem::Draw
//
//  Synopsis:   Draw this item.
//
//  Arguments:  pContext        draw context
//              pChild          start drawing at this child
//              
//----------------------------------------------------------------------------

void
CDispItem::Draw(CDispDrawContext* pContext, CDispNode* pChild)
{
#if DBG==1
CRegionRects debugRedraw(*pContext->GetRedrawRegion());
#endif

    // shouldn't be called unless this node was selected to draw
    Assert(AllSet(CDispFlags::s_drawSelector));
    Assert(IsSet(CDispFlags::s_savedRedrawRegion) ||
           pContext->IntersectsRedrawRegion(_rcVisBounds));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(pChild == NULL);

    // no filtering of CDispItem allowed, at least for now
    Assert(!IsFiltered());
    
    // BUGBUG (donmarsh) -- buffering not implemented yet
    Assert(!IsSet(CDispFlags::s_bufferInvalid));

    // did this item save the redraw region during PreDraw processing?
    if (IsSet(CDispFlags::s_savedRedrawRegion))
    {
        pContext->PopRedrawRegionForKey((void*)this);
    }

    CDispClient* pDispClient = pContext->GetDispClient();

#if 0
    // if item is buffered, we may have to copy background pixels
    if (IsSet(CDispFlags::s_bufferInvalid))
    {
        CBackBuffer* pBackBuffer = pContext->_GetBackBuffer(pItem);
        CRegion rgnBufferInvalid(pBackBuffer->_rgnBufferInvalid);
        pContext->LocalToGlobalRegion(&rgnBufferInvalid);
        // *** copy pixels to pBackBuffer through rgnBufferInvalid
        // *** subtract rgnBufferInvalid from pBackBuffer->_rgnBufferInvalid
        // *** clear s_bufferInvalid only if rgnBufferInvalid is empty
    }
#endif

    // calculate redraw bounds
    // BUGBUG (donmarsh) -- shouldn't we already set the clip to the
    // intersection with the redraw region in CDispInteriorNode for example?
    CRect rcRedraw(_rcVisBounds);
    rcRedraw.IntersectRect(pContext->GetClipRect());
    pContext->IntersectRedrawRegion(&rcRedraw);
    Assert(!rcRedraw.IsEmpty());

    // draw client content
    pDispClient->DrawClient(
        &_rcVisBounds,
        &rcRedraw,
        pContext->GetDispSurface(),
        this,
        _cookie,
        pContext->GetClientData(),
        0);

#if DBG==1
    CDebugPaint::PausePaint(tagPaintWait);
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItem::HitTestPoint
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
CDispItem::HitTestPoint(CDispHitContext* pContext) const
{
    Assert(IsSet(CDispFlags::s_visibleBranch));
    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    // BUGBUG revert sig. of FuzzyRectIsHit to not take the extra param
    //    
    Assert(pContext->FuzzyRectIsHit(_rcVisBounds, IsFatHitTest() ));

    // CDispItem does not allow fuzzy hits
    if (pContext->RectIsHit(_rcVisBounds))
    {
        CDispClient* pDispClient = GetDispClient();
        Assert(pDispClient != NULL);
        // written like this instead of simple return statement for debugging
        if (pDispClient->HitTestContent(
                &(pContext->_ptHitTest),
                (CDispLeafNode*)this,
                pContext->_pClientData))
        {
            return TRUE;
        }
    }

    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItem::GetClientRect
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
CDispItem::GetClientRect(RECT* prc, CLIENTRECT type) const
{
    switch (type)
    {
    case CLIENTRECT_BACKGROUND:
    case CLIENTRECT_CONTENT:
    case CLIENTRECT_VISIBLECONTENT:
        *prc = _rcVisBounds;
        if (IsRightToLeft() && type == CLIENTRECT_CONTENT)
        {
            ((CRect*)prc)->MirrorX();
        }
        break;
    default:
        *prc = g_Zero.rc;
        break;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItem::GetNodeTransform
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
CDispItem::GetNodeTransform(
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
        if (destination == COORDSYS_CONTAINER)
            break;
        // fall thru to continue transformation

    case COORDSYS_CONTAINER:
        // COORDSYS_CONTAINER --> COORDSYS_PARENT
        if(!fRightToLeft)
            *pOffset += _rcVisBounds.TopLeft().AsSize();
        else
        {
            pOffset->cx += _rcVisBounds.right;
            pOffset->cy += _rcVisBounds.top;
        }
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
//  Member:     CDispItem::GetNodeTransform
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
CDispItem::GetNodeTransform(
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
            CSize sizeContained = _rcVisBounds.Size();
            rcClip.SetRect(sizeContained);
            offset = g_Zero.size;

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
            if(!fRightToLeft)
            {
                offset = _rcVisBounds.TopLeft().AsSize();
            }
            else
            {
                rcClip.MirrorX();

                offset.cx = _rcVisBounds.right;
                offset.cy = _rcVisBounds.top;
            }
        }
        else
        {
            if(!fRightToLeft)
                offset += _rcVisBounds.TopLeft().AsSize();
            else
            {
                offset.cx += _rcVisBounds.right;
                offset.cy += _rcVisBounds.top;
            }
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
//  Member:     CDispItem::GetZOrder
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
CDispItem::GetZOrder() const
{
    Assert(GetLayerType() == DISPNODELAYER_NEGATIVEZ ||
           GetLayerType() == DISPNODELAYER_POSITIVEZ);

    CDispClient* pDispClient = GetDispClient();
    return pDispClient->GetZOrderForChild(_cookie);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispItem::NotifyInViewChange
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
CDispItem::NotifyInViewChange(
        CDispContext* pContext,
        BOOL fResolvedVisible,
        BOOL fWasResolvedVisible,
        BOOL fNoRedraw)
{
    CDispClient* pDispClient = GetDispClient();
    if (pDispClient != NULL)
    {
        // calculate client and clip rects in global coordinates
        CRect rcClient(_rcVisBounds);
        rcClient.OffsetRect(pContext->_offset);
        CRect rcClip;
        pContext->GetTransformedClipRect(&rcClip);
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
        pDispClient->HandleViewChange(
            viewChangedFlags,
            &rcClient,
            &rcClip,
            this);
    }
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     CDispItem::DumpInfo
//
//  Synopsis:   Dump custom information for this node.
//
//  Arguments:  hFile       file handle to dump to
//              level       tree depth at this node
//              childNumber number of this child in parent list
//
//  Notes:      Nodes with extra information to display override this method.
//
//----------------------------------------------------------------------------

void
CDispItem::DumpInfo(HANDLE hFile, long level, long childNumber)
{
    WriteHelp(hFile, _T("  <<i>cookie:<</i> <0d>"), _cookie);

    CDispClient* pDispClient = GetDispClient();
#if 0
    IDispClientDebug* pIDebug;
    if (SUCCEEDED(
        pDispClient->QueryInterface(IID_IDispClientDebug,(void**)&pIDebug)))
    {
        pIDebug->DumpDebugInfo(hFile, level, childNumber, this, _cookie);
        pIDebug->Release();
    }
#else
    pDispClient->DumpDebugInfo(hFile, level, childNumber, this, _cookie);
#endif
}
#endif

#endif

