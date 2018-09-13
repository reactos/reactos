//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       dispcontext.cxx
//
//  Contents:   Context object passed throughout display tree.
//
//  Classes:    CDispContext, CDispDrawContext, CDispHitContext
//
//----------------------------------------------------------------------------

#include "headers.hxx"

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

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPSURFACE_HXX_
#define X_DISPSURFACE_HXX_
#include "dispsurface.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPINTERIOR_HXX_
#define X_DISPINTERIOR_HXX_
#include "dispinterior.hxx"
#endif

#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif

MtDefine(CDispContext, DisplayTree, "CDispContext")
MtDefine(CDispHitContext, DisplayTree, "CDispHitContext")
MtDefine(CDispDrawContext, DisplayTree, "CDispDrawContext")
MtDefine(CDispLayerContext, DisplayTree, "CDispLayerContext")


//
// BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
//
const       int             FAT_HIT_TEST      =        4;

//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::AddToRedrawRegionGlobal
//              
//  Synopsis:   Add the given rect (in global coords) to the current redraw
//              region.
//              
//  Arguments:  rcGlobal    rect to add to redraw region (in global coords)
//              
//  Notes:      Do not make this an in-line method, because dispcontext.hxx
//              cannot be dependent on disproot.hxx (circular dependency).
//              
//----------------------------------------------------------------------------

void
CDispDrawContext::AddToRedrawRegionGlobal(const CRect& rcGlobal)
{
    Assert(_pRootNode != NULL);
    if(_pRootNode != NULL)
        _pRootNode->InvalidateRoot(rcGlobal, FALSE, FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::PushRedrawRegion
//              
//  Synopsis:   Save the current redraw region, then subtract the given region
//              from the redraw region, and make it the new redraw region.
//              
//  Arguments:  rgn         region to subtract from the current redraw region
//              key         key used to decide when to pop the next region
//              
//  Returns:    TRUE if redraw region was successfully pushed, FALSE if the
//              redraw region became empty after subtraction or the stack is
//              full.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispDrawContext::PushRedrawRegion(const CRegion& rgn, void* key)
{
    Assert(!_pRedrawRegionStack->IsFull());
    
    // save the old redraw region
    CRect rcBounds;
    CRegion *pTemp;
    pTemp = new CRegion(*_prgnRedraw);
    if (pTemp != NULL)
    {
        rgn.GetBounds(&rcBounds);
        _pRedrawRegionStack->PushRegion(_prgnRedraw, key, rcBounds);
    
        // New region.
        _prgnRedraw = pTemp;

        // subtract given region from current redraw region
        _prgnRedraw->Subtract(rgn);

        // if new redraw region is empty, start drawing
        if (_prgnRedraw->IsEmpty())
            return FALSE;    

        // if the stack became full, we will have to render from the root node
        if (_pRedrawRegionStack->IsFull())
        {
            _pFirstDrawNode = (CDispNode*) _pRootNode;
            return FALSE;
        }
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::PopContext
//              
//  Synopsis:   Pop context information off the context stack.
//              
//  Arguments:  pDispNode       current node for which we should be getting
//                              context information
//              
//  Returns:    TRUE if the proper context information was found
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispDrawContext::PopContext(CDispNode* pDispNode)
{
    Assert(_pContextStack != NULL);
    return _pContextStack->PopContext(this, pDispNode);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::FillContextStack
//              
//  Synopsis:   Fill the context stack with information for display node
//              ancestors.
//              
//  Arguments:  pDispNode       current display node
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispDrawContext::FillContextStack(CDispNode* pDispNode)
{
    Assert(_pContextStack != NULL);
    _pContextStack->Init();
    Assert(pDispNode->_pParentNode != NULL);
    pDispNode->_pParentNode->PushContext(pDispNode, _pContextStack, this);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::PushContext
//              
//  Synopsis:   Push the given context on the context stack.
//              
//  Arguments:  pDispNode       node the context is associated with
//              context         context to push
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispDrawContext::SaveContext(
        const CDispNode *pDispNode,
        const CDispContext& context)
{
    // can't inline, because CDispDrawContext doesn't know what a context
    // stack is (circular dependency)
    _pContextStack->SaveContext(context, pDispNode);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::GetRawDC
//              
//  Synopsis:   Get a raw DC (no adjustments to offset or clipping).
//              
//  Arguments:  none
//              
//  Returns:    DC
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

HDC
CDispDrawContext::GetRawDC()
{
    return _pDispSurface->GetRawDC();
}


void
CDispDrawContext::SetSurfaceRegion()
{
    _pDispSurface->SetClipRgn(_prgnRedraw);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispDrawContext::GetDispSurface
//              
//  Synopsis:   Return the display surface, properly prepared for client
//              rendering.
//              
//  Arguments:  none
//              
//  Returns:    pointer to the display surface
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

CDispSurface*
CDispDrawContext::GetDispSurface()
{
    _pDispSurface->PrepareClientSurface(&_offset, &_rcClip);
    return _pDispSurface;
}


//+---------------------------------------------------------------------------
//
//  Member:     CApplyUserClip::CApplyUserClip
//              
//  Synopsis:   If the given display node has optional user clip, apply it to
//              the total clip in the given context.
//              
//  Arguments:  pNode       display node
//              pContext    display context
//              
//  Notes:      
//              
//----------------------------------------------------------------------------


CApplyUserClip::CApplyUserClip(CDispNode* pNode, CDispContext* pContext)
{
    Assert(pNode != NULL);
    Assert(pContext != NULL);
    
    _fHasUserClip = pNode->HasUserClip();
    
    if (_fHasUserClip)
    {
        _pContext = pContext;
        _rcClipSave = _pContext->_rcClip;
        
        _pContext->_rcClip = pNode->GetUserClip();
        _pContext->_rcClip.OffsetRect(pNode->GetBounds().TopLeft().AsSize());
        _pContext->_rcClip.IntersectRect(_rcClipSave);
    }
}

//+====================================================================================
//
// Method: FuzzyRectIsHit
//
// Synopsis: Do a fuzzy hit test.
//
//------------------------------------------------------------------------------------


BOOL            
CDispHitContext::FuzzyRectIsHit(const CRect& rc, BOOL fFatHitTest )
{
    if (rc.Contains(_ptHitTest))
        return TRUE;
    if (_cFuzzyHitTest == 0)
    {
        Assert( ! fFatHitTest ); // don't expect to do a fat hit test if not doing a fuzzy.
        return FALSE;
    }        
    CSize size;
    rc.GetSize(&size);
    if (size.cx <= 0 || size.cy <= 0)
        return FALSE;
    CRect rcFuzzy(rc);
    size.cx -= _cFuzzyHitTest;
    size.cy -= _cFuzzyHitTest;
    if (size.cx < 0)
    {
        rcFuzzy.left += size.cx;
        rcFuzzy.right -= size.cx; 
    }
    if (size.cy < 0)
    {
        rcFuzzy.top += size.cy;
        rcFuzzy.bottom -= size.cy; 
    }

    //
    // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
    // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
    //

    //
    // BUGBUG instead of just returning result of rcFuzzy.Contains - we now
    // also return the results of FatRectIsHit if fFatHitTest is true
    // this works as once _cFuzzyHitTest > 0 we are in design mode
    //
    // the code below should be changed to return just the result of rcFuzzy.Contains test
    // once the above BUGBUG is satisfied.
    //
    if ( rcFuzzy.Contains(_ptHitTest))
        return TRUE;
    else 
        return fFatHitTest && FatRectIsHit( rc );
}




//+====================================================================================
//
// Method: FatRectIsHit
//
// Synopsis: Check to see if the "fat rect is hit"
//
//------------------------------------------------------------------------------------

//
// BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
//

BOOL            
CDispHitContext::FatRectIsHit(const CRect& rc)
{
    if (rc.Contains(_ptHitTest))
        return TRUE;
        
    CRect rcFat(rc);

    rcFat.left -= FAT_HIT_TEST;
    rcFat.right += FAT_HIT_TEST; 
    rcFat.top -= FAT_HIT_TEST;
    rcFat.bottom += FAT_HIT_TEST; 

    return rcFat.Contains(_ptHitTest);
}

#ifdef NEVER
//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::GetDC
//              
//  Synopsis:   Get a raw DC (no adjustments to offset or clipping).
//              
//  Arguments:  pointer to HDC
//              
//  Returns:    
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT CDispLayerContext::GetDC(HDC *phdc)
{
    *phdc = _pDrawContext->GetRawDC();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::GetDDSurface
//              
//  Synopsis:   Get a DD surface
//              
//  Arguments:  pointer to IDirectDrawSurface*
//              
//  Returns:    
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT CDispLayerContext::GetDDSurface(IDirectDrawSurface **ppDDSurface)
{
    if (!_pDrawContext->_pDispSurface->_pDDSurface)
    {
        RRETURN(_pDrawContext->_pDispSurface->GetSurfaceFromDC(ppDDSurface));
    }
   *ppDDSurface = _pDrawContext->_pDispSurface->_pDDSurface;
    (*ppDDSurface)->AddRef();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::SetDC
//              
//  Synopsis:   Set the graphics surface
//              
//  Arguments:  HDC
//              
//  Returns:    This replaces the existing hdc or direct draw surface
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT CDispLayerContext::SetDC(HDC hdc)
{
    // Need to cook up a new surface to wrap the DC.
    _pDrawContext->_pDispSurface = new CDispSurface(hdc);

    if (!_pDrawContext->_pDispSurface)
        RRETURN(E_OUTOFMEMORY);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::SetDDSurface
//              
//  Synopsis:   Set the graphics surface
//              
//  Arguments:  pointer to IDirectDrawSurface*
//              
//  Returns:    This replaces the existing hdc or direct draw surface
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
HRESULT 
CDispLayerContext::SetDDSurface(IDirectDrawSurface *pDDSurface)
{
    // Need to cook up a new surface to wrap the DD surface
    _pDrawContext->_pDispSurface = new CDispSurface(pDDSurface);

    if (!_pDrawContext->_pDispSurface)
        RRETURN(E_OUTOFMEMORY);
    
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::GetClip
//              
//  Synopsis:   Returns the current clip
//              
//  Arguments:  LPRECT
//              
//  Returns:    
//              
//  Notes:      This will have to get beefed up to return more information
//              about the clip, at least the region.
//              
//----------------------------------------------------------------------------
HRESULT 
CDispLayerContext::GetClip(LPRECT prc) // Caller must provide a region, we stomp on it
{
    *prc = _pDrawContext->_rcClip;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::SetClip
//              
//  Synopsis:   Changes the clipping
//              
//  Arguments:  LPCRECT
//              
//  Returns:    
//              
//  Notes:      This will have to get beefed up to include more information
//              about the clip, at least the region.
//              
//----------------------------------------------------------------------------
HRESULT
CDispLayerContext::SetClip(LPCRECT prc) // Caller retains ownership
{
    _pDrawContext->_rcClip = *prc;
    
    // BUGBUG (donmarsh) - hack solution for now -- the surface needs its
    // clip region set to avoid problems later in CDispSurface::GetDC
    if (_pDrawContext->_pDispSurface)
    {
        _pDrawContext->_pDispSurface->SetClipRgn((CRegion*) prc);
    }

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::GetOffset
//              
//  Synopsis:   Returns the current viewport offset
//              
//  Arguments:  POINT *
//              
//  Returns:    
//              
//  Notes:      This will have to get beefed up to return more information
//              about the clip, at least the region.
//              
//----------------------------------------------------------------------------
HRESULT CDispLayerContext::GetOffset(POINT *ppt)
{
    ppt->x = _pDrawContext->_offset.cx;
    ppt->y = _pDrawContext->_offset.cy;

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::SetOffset
//              
//  Synopsis:   Changes the current viewport offset
//              
//  Arguments:  const POINT *
//              
//  Returns:    
//              
//  Notes:      This will have to get beefed up to return more information
//              about the clip, at least the region.
//              
//----------------------------------------------------------------------------
HRESULT CDispLayerContext::SetOffset(const POINT *ppt)
{
    _pDrawContext->_offset.cx = ppt->x;
    _pDrawContext->_offset.cy = ppt->y;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::DrawLayer
//              
//  Synopsis:   Draw the unfiltered version of the layer
//              
//  Arguments:  layer       which layer to draw
//              
//  Returns:    
//              
//  Notes:      Drawing layers out of sequence is not supported
//              
//----------------------------------------------------------------------------
void
CDispLayerContext::DrawLayer(DISPNODELAYER layer)
{
    Assert(layer == _layer);

    if (layer == _layer)
		_pDispNode->DrawLayer(layer, this);
}

void
CDispLayerContext::DrawLayer(HDC hdc, long x, long y, DISPNODELAYER layer)
{
    Assert(layer == _layer);

    if (layer == _layer)
    {
		CDispSurface *pSurface = new CDispSurface(hdc);

		if (!pSurface)
			return;

		SaveContextState();

        _pDrawContext->SetDispSurface(pSurface);

		CPoint pt = _pDispNode->GetBounds().TopLeft();

		pt.x = x - pt.x;
		pt.y = y - pt.y;

		SetOffset(&pt);

		// BUGBUG (michaelw) we really need to figure out clipping correctly
		CRect rcClip(-5000, -5000, 5000, 5000);
		SetClip(&rcClip);

		CRegion rgn(rcClip);

		_pDrawContext->SetRedrawRegion(&rgn);

        // get surface ready for rendering
        pSurface->PrepareClientSurface(&pt.AsSize(), &rcClip);
        pSurface->SetClipRgn(&rgn);

		_pDispNode->DrawLayer(layer, this);

		RestoreContextState();

		delete pSurface;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::SaveContextState
//              
//  Synopsis:   Grabs the current state from the context and saves it
//              
//  Arguments:  
//              
//  Returns:    
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
void
CDispLayerContext::SaveContextState()
{
    _pDispSurfaceSave = _pDrawContext->_pDispSurface;
    _saveContext = *_pDrawContext;
    _pregionSave = _pDrawContext->_pDispSurface->_prgnClip;
    _pCurrentChildNodeSave = _pCurrentChildNode;
	_prgnRedrawSave = _pDrawContext->_prgnRedraw;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispLayerContext::RestoreContextState
//              
//  Synopsis:   Restores the current state from the last saved copy
//              
//  Arguments:  
//              
//  Returns:    
//              
//  Notes:      This should be used after handing the CDispLayerContext
//              to someone who might change it
//              
//----------------------------------------------------------------------------
void 
CDispLayerContext::RestoreContextState()
{
    *_pDrawContext = _saveContext;
    _pDrawContext->_pDispSurface->_prgnClip = _pregionSave;

    if (_pDrawContext->_pDispSurface != _pDispSurfaceSave)
    {
        _pDrawContext->_pDispSurface = _pDispSurfaceSave;
    }

	if (_prgnRedrawSave != _pDrawContext->_prgnRedraw)
	{
		_pDrawContext->_prgnRedraw = _prgnRedrawSave;
	}

    _pCurrentChildNode = _pCurrentChildNodeSave;
}
#endif
