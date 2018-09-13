//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispscrollerplus.cxx
//
//  Contents:   Scrolling container with optional border and user clip.
//
//  Classes:    CDispScrollerPlus
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPSCROLLERPLUS_HXX_
#define X_DISPSCROLLERPLUS_HXX_
#include "dispscrollerplus.hxx"
#endif

#ifndef X_SAVEDISPCONTEXT_HXX_
#define X_SAVEDISPCONTEXT_HXX_
#include "savedispcontext.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

MtDefine(CDispScrollerPlus, DisplayTree, "CDispScrollerPlus")


// handy inline methods
inline void
CDispScrollerPlus::GetInsideBorderRect(CRect* prc, const CDispInfo& di) const
{
    prc->SetRect(
        _rcContainer.left + di._prcBorderWidths->left,
        _rcContainer.top + di._prcBorderWidths->top,
        _rcContainer.right - di._prcBorderWidths->right,
        _rcContainer.bottom - di._prcBorderWidths->bottom);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScrollerPlus::CalcDispInfo
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
CDispScrollerPlus::CalcDispInfo(
        const CRect& rcClip,
        CDispInfo* pdi) const
{
    //
    // user clip = current clip INTERSECT optional user clip INTERSECT
    //   container bounds, in content coordinates with scrolling
    //   
    // border clip = current clip INTERSECT optional user clip INTERSECT
    //   container bounds, in container coordinates (no scrolling)
    //   
    // flow clip = current clip INTERSECT optional user clip INTERSECT
    //   container bounds, in content coordinates with scrolling
    //   
    CDispInfo& di = *pdi;   // notational convenience
    
    // set scrolling offset
    di._scrollOffset = _sizeScrollOffset;
    
    // content size
    di._sizeContent = _sizeContent;
    
    // offset to local coordinates
    _rcContainer.GetTopLeft(&(di._borderOffset.AsPoint()));
    
    // calc container clip intersect with container bounds
    di._rcContainerClip = rcClip;
    di._rcContainerClip.IntersectRect(_rcContainer);
    di._rcContainerClip.OffsetRect(-di._borderOffset);
    
    // calc rect inside border and scroll bars
    di._sizeBackground =
        _rcContainer.Size()
        - di._prcBorderWidths->TopLeft().AsSize()
        - di._prcBorderWidths->BottomRight().AsSize();
    if (_fHasVScrollbar)
    {
        di._sizeBackground.cx -= _sizeScrollbars.cx;
    }
    if (_fHasHScrollbar)
    {
        di._sizeBackground.cy -= _sizeScrollbars.cy;
    }
    di._rcPositionedClip.SetRect(
        di._prcBorderWidths->TopLeft(),
        di._sizeBackground);
    di._contentOffset = di._prcBorderWidths->TopLeft().AsSize();
    di._rcPositionedClip.IntersectRect(di._rcContainerClip);
    di._rcPositionedClip.OffsetRect(-di._contentOffset);

    // contents scroll
    if (!IsSet(CDispFlags::s_fixedBackground))
    {
        di._rcPositionedClip.OffsetRect(-_sizeScrollOffset);
        di._rcBackgroundClip = di._rcPositionedClip;
    }
    else
    {
        di._rcBackgroundClip = di._rcPositionedClip;
        di._rcPositionedClip.OffsetRect(-_sizeScrollOffset);
    }
    
    // adjust for right to left coordinate system
    if (!IsRightToLeft())
    {
        di._rcFlowClip.left = max(di._rcPositionedClip.left, di._pInsetOffset->cx);
        di._rcFlowClip.right = di._rcPositionedClip.right;
    }
    else
    {
        di._contentOffset.cx = di._sizeBackground.cx + di._prcBorderWidths->left;
        if(_fHasVScrollbar)
        {
            di._contentOffset.cx += _sizeScrollbars.cx;
        }
        di._rcPositionedClip.OffsetX(-di._sizeBackground.cx);
        di._rcBackgroundClip.OffsetX(-di._sizeBackground.cx);
        
        di._rcFlowClip.left = di._rcPositionedClip.left;
        di._rcFlowClip.right = min(di._rcPositionedClip.right, di._pInsetOffset->cx);
    }

    di._rcFlowClip.top = max(di._rcPositionedClip.top, di._pInsetOffset->cy);
    di._rcFlowClip.bottom = di._rcPositionedClip.bottom;
    di._rcFlowClip.OffsetRect(-*di._pInsetOffset);
    
    // size of background is big enough to fill background and content
    di._sizeBackground.Max(di._sizeContent);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispScrollerPlus::GetNodeTransform
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
CDispScrollerPlus::GetNodeTransform(
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
                *pOffset = di._prcBorderWidths->TopLeft().AsSize() + _sizeScrollOffset;
            
                // add inset for flow child
                if (source == COORDSYS_CONTENT)
                {
                    *pOffset += *di._pInsetOffset;
                }
            }
            else
            {
            
                pOffset->cx = _rcContainer.Width() - di._prcBorderWidths->right + _sizeScrollOffset.cx;
                pOffset->cy = di._prcBorderWidths->top + _sizeScrollOffset.cy;

                // add inset for flow child
                if (source == COORDSYS_CONTENT)
                {
                    pOffset->cx -= di._pInsetOffset->cx;
                    pOffset->cy += di._pInsetOffset->cy;
                }
            }
        }
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
//  Member:     CDispScrollerPlus::GetNodeTransform
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
CDispScrollerPlus::GetNodeTransform(
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
                offset = topLeftBorder + _sizeScrollOffset;
            
                // add inset for flow child
                if (source == COORDSYS_CONTENT)
                {
                    offset += *di._pInsetOffset;
                }
            }
            else
            {
                offset.cx = _rcContainer.Width() - di._prcBorderWidths->right + _sizeScrollOffset.cx;
                offset.cy = di._prcBorderWidths->top + _sizeScrollOffset.cy;

            }
            
            // all layers clip inside border regardless of layer type
            CSize sizeContained =
                _rcContainer.Size() - topLeftBorder -
                di._prcBorderWidths->BottomRight().AsSize();
            // clip to scroll bars
            if (_fHasVScrollbar)
            {
                sizeContained.cx -= _sizeScrollbars.cx;
            }
            if (_fHasHScrollbar)
            {
                sizeContained.cy -= _sizeScrollbars.cy;
            }
            

            if (!fRightToLeft)
            {
                rcClip.SetRect(-_sizeScrollOffset.AsPoint(), sizeContained);
            }
            else
            {
                CPoint pt(_sizeScrollOffset.cx, -_sizeScrollOffset.cy);
                rcClip.SetRect(pt, sizeContained);
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
            if (HasUserClip())
            {
                rcClip = GetUserClip((CDispExtras*) _extra);
            }
            else
                pContext->SetNoClip();
            offset = _rcContainer.TopLeft().AsSize();
        }
        else
        {
            if (HasUserClip())
            {
                CRect rcUserClip(GetUserClip((CDispExtras*) _extra));
                rcUserClip.OffsetRect(-offset);     // to source coords.
                rcClip.IntersectRect(rcUserClip);
            }
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


