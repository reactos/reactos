//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontainerplus.cxx
//
//  Contents:   A container node with optional border and user clip.
//
//  Classes:    CDispContainerPlus
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPCONTAINERPLUS_HXX_
#define X_DISPCONTAINERPLUS_HXX_
#include "dispcontainerplus.hxx"
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

MtDefine(CDispContainerPlus, DisplayTree, "CDispContainerPlus")


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainerPlus::CDispContainerPlus
//              
//  Synopsis:   Construct a container node with equivalent functionality to
//              the CDispItemPlus node passed as an argument.
//              
//  Arguments:  pItemPlus       the prototype CDispItemPlus node
//              
//  Notes:      
//              
//----------------------------------------------------------------------------


CDispContainerPlus::CDispContainerPlus(const CDispItemPlus* pItemPlus)
        : CDispContainer(pItemPlus)
{
    CDispExtras* pExtras = (CDispExtras*) pItemPlus->_extra;
    Assert(pExtras->HasExtraCookie() ||
           pExtras->HasUserClip() ||
           pExtras->HasInset() ||
           pExtras->GetBorderType() != DISPNODEBORDER_NONE);
    
    *((CDispExtras*)&_extra) = *pExtras;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainerPlus::SetSize
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
CDispContainerPlus::SetSize(const SIZE& size, BOOL fInvalidateAll)
{
    CDispInfo di((CDispExtras*) _extra);
    super::SetSize(size, *di._prcBorderWidths, fInvalidateAll);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainerPlus::CalcDispInfo
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
CDispContainerPlus::CalcDispInfo(
        const CRect& rcClip,
        CDispInfo* pdi) const
{
    //
    // user clip = current clip INTERSECT optional user clip, in content
    //   coordinates
    //   
    // border clip = current clip INTERSECT optional user clip INTERSECT
    //   container bounds, in container coordinates
    //   
    // flow clip = current clip INTERSECT optional user clip INTERSECT
    //   container bounds, in content coordinates
    //   
    
    CDispInfo& di = *pdi;   // notational convenience
    
    // no scrolling
    di._scrollOffset = g_Zero.size;
    
    // content size
    _rcContainer.GetSize(&di._sizeContent);
    
    // offset to local coordinates
    _rcContainer.GetTopLeft(&(di._borderOffset.AsPoint()));
    
    // calc user clip
    di._rcPositionedClip = rcClip;
    di._rcPositionedClip.OffsetRect(-di._borderOffset);
    
    // container clip is clipped by container bounds
    ClipToParentCoords(&di._rcContainerClip, di._rcPositionedClip, di._sizeContent);
    
    // inset user clip and flow clip by optional border
    di._contentOffset = di._prcBorderWidths->TopLeft().AsSize();
    di._sizeContent.cx -= di._prcBorderWidths->left + di._prcBorderWidths->right;
    di._sizeContent.cy -= di._prcBorderWidths->top + di._prcBorderWidths->bottom;
    di._sizeBackground = di._sizeContent;
    di._rcPositionedClip.OffsetRect(-di._contentOffset);
    di._rcBackgroundClip.top = max(0L, di._rcPositionedClip.top);
    di._rcBackgroundClip.bottom = min(di._sizeContent.cy,
                                      di._rcPositionedClip.bottom);

    // adjust for right to left coordinate system
    if (!IsRightToLeft())
    {
        di._rcBackgroundClip.left = max(0L, di._rcPositionedClip.left);
        di._rcBackgroundClip.right = min(di._sizeContent.cx,
                                         di._rcPositionedClip.right);
        di._rcFlowClip.left = max(di._rcBackgroundClip.left, di._pInsetOffset->cx);
        di._rcFlowClip.right = di._rcBackgroundClip.right;
    }
    else
    {
        LONG offset = _rcContainer.Width() - di._prcBorderWidths->right;
        di._contentOffset.cx = offset;
        di._rcPositionedClip.OffsetX(-offset + di._prcBorderWidths->right);
        
        di._rcBackgroundClip.left = max(-di._sizeContent.cx,
                                        di._rcPositionedClip.left);
        di._rcBackgroundClip.right = min(0L, di._rcPositionedClip.right);
        di._rcFlowClip.left = di._rcBackgroundClip.left;
        di._rcFlowClip.right = min(di._rcBackgroundClip.right, di._pInsetOffset->cx);
    }

    di._rcFlowClip.top = max(di._rcBackgroundClip.top, di._pInsetOffset->cy);
    di._rcFlowClip.bottom = di._rcBackgroundClip.bottom;
    di._rcFlowClip.OffsetRect(-*di._pInsetOffset);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainerPlus::GetNodeTransform
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
CDispContainerPlus::GetNodeTransform(
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
                *pOffset = di._prcBorderWidths->TopLeft().AsSize();
            
                // add inset for flow child
                if (source == COORDSYS_CONTENT)
                {
                    *pOffset += *di._pInsetOffset;
                }
            }
            else
            {
            
                pOffset->cx = _rcContainer.Width() - di._prcBorderWidths->right;
                pOffset->cy = di._prcBorderWidths->top;

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
//  Member:     CDispContainerPlus::GetNodeTransform
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
CDispContainerPlus::GetNodeTransform(
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
                offset = topLeftBorder;
            else
            {
                offset.cx = _rcContainer.Width() - di._prcBorderWidths->right;
                offset.cy = di._prcBorderWidths->top;
            }
            
            CSize sizeContained =
                _rcContainer.Size() - topLeftBorder -
                di._prcBorderWidths->BottomRight().AsSize();
            
            // add inset and clip for flow child
            if (source == COORDSYS_CONTENT)
            {
                rcClip.SetRect(sizeContained);
                if(!fRightToLeft)
                    offset += *di._pInsetOffset;
                else
                {
                    rcClip.MirrorX();
                    offset.cx -= di._pInsetOffset->cx;
                    offset.cy += di._pInsetOffset->cy;
                }
            }
            else
            {
                pContext->SetNoClip();
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
                if(fRightToLeft)
                    rcClip.MirrorX();
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

                if(fRightToLeft)
                    rcClip.MirrorX();
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


//+---------------------------------------------------------------------------
//
//  Member:     CDispContainerPlus::ComputeVisibleBounds
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
CDispContainerPlus::ComputeVisibleBounds()
{
    BOOL fRightToLeft = IsRightToLeft();
    // visible bounds is always the size of the container, and may be extended
    // by items in Z layers that fall outside these bounds
    CRect rcBounds(_rcContainer);
    
    if (!IsFiltered())
    {
        // offset position of Z layers by border width
        CSize borderOffset(_rcContainer.TopLeft().AsSize());
        if (HasBorder((CDispExtras*) _extra))
        {
            CRect rcBorderWidths;
            ((CDispExtras*)_extra)->GetBorderWidths(&rcBorderWidths);
            borderOffset += rcBorderWidths.TopLeft().AsSize();
            if (fRightToLeft)
            {
                borderOffset.cx = _rcContainer.right - rcBorderWidths.right;
            }
        }
        else if (fRightToLeft)
        {
            borderOffset.cx = _rcContainer.right;
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
                    rcChild.OffsetRect(borderOffset);
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



