//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       displeaf.cxx
//
//  Contents:   Abstract base class for all leaf nodes in the display tree.
//
//  Classes:    CDispLeafNode
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPLEAF_HXX_
#define X_DISPLEAF_HXX_
#include "displeaf.hxx"
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

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif


/* ---------------------------------------------------------------------------

=== ISSUES TO SOLVE LATER ===

1. Share screen back buffers in such a way that multiple browser windows do not
each require a separate back buffer.

2. Determine when an item is very expensive to render, and buffer its graphic.
This is simple for opaque items.  It requires generation of a bit-mask for
transparent items that contain only completely transparent or completely opaque
pixels.  It requires per-pixel alpha blending in the general case.

----------------------------------------------------------------------------- */



//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::SetSize
//              
//  Synopsis:   Set size of this node.
//              
//  Arguments:  size                new size
//              rcBorderWidths      size of borders
//              fInvalidateAll      TRUE to entirely invalidate this node
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispLeafNode::SetSize(
        const SIZE& size,
        const RECT& rcBorderWidths,
        BOOL fInvalidateAll)
{
    CSize sizeDelta(size.cx - _rcVisBounds.Width(),
                    size.cy - _rcVisBounds.Height());
    if (sizeDelta.IsZero())
        return;
    
    // calculate new bounds
    BOOL fRightToLeft =
        _pParentNode != NULL && _pParentNode->IsRightToLeft();
    CRect rcNew(_rcVisBounds);
    if (fRightToLeft)
    {
        rcNew.left -= sizeDelta.cx;
    }
    else
    {
        rcNew.right += sizeDelta.cx;
    }
    rcNew.bottom += sizeDelta.cy;
    
    // BUGBUG (donmarsh) - this should really be s_viewHasChanged, when
    // we have such a flag.
    SetFlag(CDispFlags::s_positionHasChanged);
    
    // if the inval flag is set, we don't need to invalidate because the
    // current bounds has never been rendered
    if (!IsSet(CDispFlags::s_inval))
    {
        RequestRecalc();
        
        if (IsVisible())
        {
            if (IsInView())
            {
                if (fInvalidateAll)
                {
                    // inval old
                    Invalidate(_rcVisBounds, COORDSYS_PARENT);
                    SetFlag(CDispFlags::s_inval);   // inval new (deferred)
                }
                else
                {
                    InvalidateEdges(_rcVisBounds, rcNew, rcBorderWidths, fRightToLeft);
                }
            }
            else
            {
                SetFlag(CDispFlags::s_inval);
            }
        }
    }
    
    Assert(IsSet(CDispFlags::s_recalc));
    
    _rcVisBounds = rcNew;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::SetPosition
//              
//  Synopsis:   Set the position of this leaf node.
//              
//  Arguments:  ptTopLeft       new top left coordinates (in PARENT coordinates)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispLeafNode::SetPosition(const POINT& ptTopLeft)
{
    if (_rcVisBounds.TopLeft() != ptTopLeft) 
    {
        if (IsVisible() && !IsSet(CDispFlags::s_inval))
        {
            Invalidate(_rcVisBounds, COORDSYS_PARENT);
            SetFlag(CDispFlags::s_inval);
        }
        _rcVisBounds.MoveTo(ptTopLeft);
        SetFlag(CDispFlags::s_positionHasChanged);
        RequestRecalc();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLeafNode::SetPositionTopRight
//              
//  Synopsis:   Set the top right position of this leaf node.
//              
//  Arguments:  ptTopRight       new top right coordinates (in PARENT coordinates)
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispLeafNode::SetPositionTopRight(const POINT& ptTopRight)
{
    CPoint pt;
    _rcVisBounds.GetTopRight(&pt);

    if (pt != ptTopRight) 
    {
        if (IsVisible() && !IsSet(CDispFlags::s_inval))
        {
            Invalidate(_rcVisBounds, COORDSYS_PARENT);
            SetFlag(CDispFlags::s_inval);
        }
        _rcVisBounds.MoveTopRightTo(ptTopRight);
        SetFlag(CDispFlags::s_positionHasChanged);
        RequestRecalc();
    }
}


