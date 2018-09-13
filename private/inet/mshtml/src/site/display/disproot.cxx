//+---------------------------------------------------------------------------
//
//  Microsoft Internet Explorer
//  Copyright (C) Microsoft Corporation, 1997-1998
//
//  File:       disproot.cxx
//
//  Contents:   Interior node at the root of a display tree.
//
//  Classes:    CDispRoot
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_DISPGRP_HXX_
#define X_DISPGRP_HXX_
#include "dispgrp.hxx"
#endif

#ifndef X_DISPCONTAINER_HXX_
#define X_DISPCONTAINER_HXX_
#include "dispcontainer.hxx"
#endif

#ifndef X_DISPCONTAINERPLUS_HXX_
#define X_DISPCONTAINERPLUS_HXX_
#include "dispcontainerplus.hxx"
#endif

#ifndef X_DISPSCROLLER_HXX_
#define X_DISPSCROLLER_HXX_
#include "dispscroller.hxx"
#endif

#ifndef X_DISPSCROLLERPLUS_HXX_
#define X_DISPSCROLLERPLUS_HXX_
#include "dispscrollerplus.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_DISPCLIENT_HXX_
#define X_DISPCLIENT_HXX_
#include "dispclient.hxx"
#endif

#ifndef X_DISPINFO_HXX_
#define X_DISPINFO_HXX_
#include "dispinfo.hxx"
#endif

// BUGBUG: Only here for the use of ScrollRect (brendand)
#ifndef X_VIEW_HXX_
#define X_VIEW_HXX_
#include "view.hxx"
#endif

MtDefine(CDispRoot, DisplayTree, "CDispRoot")

DeclareTag(tagDisplayTreeOpen,   "Display: TreeOpen stack",   "Stack trace for each OpenDisplayTree")
DeclareTag(tagOscForceDDBanding, "Display: Force offscreen",  "Force banding when using DirectDraw")

const long  s_cBUFFERLINES = 150;

#if DBG==1
void
CDispRoot::OpenDisplayTree()
{
#ifndef VSTUDIO7
    CheckReenter();
#endif //VSTUDIO7
    _cOpen++;

    TraceTag((tagDisplayTreeOpen, "\n***** OpenDisplayTree call stack:"));
    TraceCallers(tagDisplayTreeOpen, 0, 10);

    if (_cOpen == 1)
    {
        // on first open, none of these flags should be set
        VerifyFlags(CDispFlags::s_generalFlagsNotSetInDraw, CDispFlags::s_none,
                    TRUE);
    }
}

void
CDispRoot::CloseDisplayTree()
{
#ifndef VSTUDIO7
    CheckReenter();
#endif //VSTUDIO7
    Assert(_cOpen > 0);
    if (_cOpen == 1)
    {
        RecalcRoot();
        
        // after recalc, no recalc flags should be set
        VerifyFlags(CDispFlags::s_generalFlagsNotSetInDraw, CDispFlags::s_none,
                    TRUE);
    }
    _cOpen--;
}
#endif

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::CDispRoot
//
//  Synopsis:   Constructor.
//
//  Arguments:  pObserver       display tree observer
//              pDispClient     display client
//              fHasBackground  TRUE if root node has a background
//              fBackgroundIsOpaque TRUE if background is opaque
//
//  Notes:
//
//----------------------------------------------------------------------------


CDispRoot::CDispRoot(
        IDispObserver* pObserver,
        CDispClient* pDispClient)
    : CDispContainer(pDispClient)
{
    _drawContext._pRootNode = this;
    SetFlag(CDispFlags::s_preDrawSelector);
    SetFlag(CDispFlags::s_opaqueNode);
    SetFlag(CDispFlags::s_visibleNode);
    _pDispObserver = pObserver;
    
#if DBG==1
    // check integrity of flag definitions
    CDispFlags::CheckFlagIntegrity();
#endif
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::~CDispRoot
//
//  Synopsis:   Destructor
//
//----------------------------------------------------------------------------


CDispRoot::~CDispRoot()
{
    AssertSz(!_fDrawLock, "Illegal call to CDispRoot inside Draw()");
    ReleaseRenderSurface();
    ReleaseOffscreenBuffer();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DestroyTreeWithPrejudice
//
//  Synopsis:   Destroy the entire tree in an optimal fashion without executing
//              any destructors.
//
//  Arguments:  none
//
//  Notes:      WARNING!  You must be sure that nothing holds a pointer to
//              any node in the tree before calling this method.
//
//----------------------------------------------------------------------------

void
CDispRoot::DestroyTreeWithPrejudice()
{
    AssertSz(!_fDrawLock, "Illegal call to CDispRoot inside Draw()");
    
    // from CDispRoot destructor:
    ReleaseRenderSurface();
    ReleaseOffscreenBuffer();

    // traverse entire tree, blowing away everything without running any
    // destructors or calling any virtual methods
    CDispNode* pChild = this;
    CDispInteriorNode* pInterior = NULL;

    while (pChild != NULL)
    {
        // find left-most descendant of this interior node
        while (pChild->IsInteriorNode())
        {
            pInterior = DYNCAST(CDispInteriorNode, pChild);
            pChild = pInterior->_pFirstChildNode;
            if (pChild == NULL)
                break;
            pInterior->_pFirstChildNode = pChild->_pNextSiblingNode;
        }

        // blow away left-most leaf nodes at this level
        while (pChild != NULL && pChild->IsLeafNode())
        {
            CDispNode* pDeadChild = pChild;
            pChild = pChild->_pNextSiblingNode;
            MemFree(pDeadChild);
        }

        // descend into tree if we found an interior node
        if (pChild != NULL)
        {
            pInterior->_pFirstChildNode = pChild->_pNextSiblingNode;
            pInterior = DYNCAST(CDispInteriorNode, pChild);
        }

        // go up a level in the tree if we ran out of nodes at this level
        else
        {
            Assert(pInterior != NULL);
            pChild = pInterior->_pParentNode;
            MemFree(pInterior);
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawRoot
//
//  Synopsis:   Draw this display tree.
//
//  Arguments:  pContext        draw context
//              pClientData     client data used by clients in DrawClient
//              hrgnDraw        region to draw in destination coordinates
//              prcDraw         rect to draw in destination coordinates
//
//  Notes:      if hrgnDraw and prcDraw are both NULL, the bounding rect of
//              this root node is used
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawRoot(
        CDispDrawContext* pContext,
        void* pClientData,
        HRGN hrgnDraw,
        const RECT* prcDraw)
{
    // BUGBUG (donmarsh) - for now we must use this root's own context, due
    // to the context stack used for drawing.  Eventually, we should eliminate
    // the context pointer argument to DrawRoot.
    Assert(pContext == &_drawContext);
    AssertSz(!_fDrawLock, "Illegal call to DrawRoot inside Draw()");
    AssertSz(_cOpen == 0, "DrawRoot called while Display Tree is open");
    Assert(_pRenderSurface != NULL);
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));

#if DBG==1
    // shouldn't be here with a tree needing recalc
    VerifyFlags(CDispFlags::s_generalFlagsNotSetInDraw, CDispFlags::s_none, TRUE);
#endif

    // set redraw region (will become a rect if possible)
    CRegion rgnRedraw;
    if (hrgnDraw != NULL)
    {
        rgnRedraw = hrgnDraw;
    }
    else if (prcDraw != NULL)
    {
        rgnRedraw = *prcDraw;
    }
    else
    {
        rgnRedraw = _rcContainer;
    }
    
#if DBG==1
    CRegionRects debugHrgn(hrgnDraw);
    CRegionRects debugRedraw(rgnRedraw);

    // show invalid area for debugging
    if (rgnRedraw.IsRegion())
    {
        CDebugPaint::ShowPaint(
            NULL, rgnRedraw.GetRegionAlias(), pContext->GetRawDC(),
            tagPaintShow, tagPaintPause, tagPaintWait, TRUE);
    }
    else
    {
        CDebugPaint::ShowPaint(
            &rgnRedraw.AsRect(), NULL, pContext->GetRawDC(),
            tagPaintShow, tagPaintPause, tagPaintWait, TRUE);
    }
#endif

    // check for early exit conditions
    if (!rgnRedraw.Intersects(_rcContainer))
        return;
    
    // set initial context values
    pContext->SetClientData(pClientData);
    pContext->SetNoClip();
    pContext->_offset = _rcContainer.TopLeft().AsSize();
    pContext->_pFirstDrawNode = NULL;
    pContext->SetRedrawRegion(&rgnRedraw);

    // BUGBUG (donmarsh) -- this is a little ugly, but our the _rcVisBounds
    // for CDispRoot must be zero-based, because it is transformed by the
    // offset in CDispNode::Draw, and if _rcVisBounds == _rcContainer like
    // one would expect, _rcVisBounds gets transformed twice.
    _rcVisBounds.MoveToOrigin();

    if (_pFirstChildNode == NULL)
    {
        pContext->SetDispSurface(_pRenderSurface);
        _pRenderSurface->SetBandOffset(g_Zero.size);
        CRect rcClip;
        rgnRedraw.GetBounds(&rcClip);
        CRegion rgnClip(rcClip);
        _pRenderSurface->SetClipRgn(&rgnClip);

        DrawSelf(pContext, NULL);

        _pRenderSurface->SetClipRgn(NULL);
        ::SelectClipRgn(_pRenderSurface->GetRawDC(), NULL);
        goto Cleanup;
    }
    
    // DrawRoot does not allow recursion
    _fDrawLock = TRUE;

    // speed optimization: draw border and scroll bars for first node
    // without buffering or banding, then subtract them from the redraw
    // region.
    if (_pFirstChildNode == _pLastChildNode &&
        _pFirstChildNode->IsContainer() &&
        _pFirstChildNode->IsVisible())
    {
        CDispContainer* pContainer = DYNCAST(CDispContainer, _pFirstChildNode);
        pContext->SetDispSurface(_pRenderSurface);
        _pRenderSurface->SetBandOffset(g_Zero.size);
        CRect rcClip;
        rgnRedraw.GetBounds(&rcClip);
        CRegion rgnClip(rcClip);
        _pRenderSurface->SetClipRgn(&rgnClip);

        pContainer->DrawBorderAndScrollbars(pContext, &_rcContent);
        _rcContent.OffsetRect(_rcContainer.TopLeft().AsSize());
        _pRenderSurface->SetClipRgn(NULL);

        // restore clipping on destination surface to redraw region
        // (this is important when we're using filters and we have a
        // direct draw surface)
        if (rgnRedraw.IsComplex())
        {
            ::SelectClipRgn(_pRenderSurface->GetRawDC(), rgnRedraw.GetRegionAlias());
        }
        else
        {
            HRGN hrgnClip = ::CreateRectRgnIndirect(&rgnRedraw.AsRect());
            ::SelectClipRgn(_pRenderSurface->GetRawDC(), hrgnClip);
            ::DeleteObject(hrgnClip);
        }
        
        rgnRedraw.Intersect(_rcContent);
        // early exit if all we needed to draw was the border and scroll bars
        if (rgnRedraw.IsEmpty())
            goto Cleanup;
    }
    else
    {
        _rcContent = _rcContainer;
    }

    // early exit if all we needed to draw was the border and scroll bars
    if (!_rcContent.IsEmpty())
    {
        // try to allocate offscreen buffer
        BOOL fOffscreen = SetupOffscreenBuffer(pContext);
        Assert(!fOffscreen || _pOffscreenBuffer);

        // allocate stacks for redraw regions and context values
        CRegionStack redrawRegionStack;
        pContext->SetRedrawRegionStack(&redrawRegionStack);
        CDispContextStack contextStack;
        pContext->SetContextStack(&contextStack);

        // PreDraw pass processes the tree from highest layer to lowest,
        // culling layers beneath opaque layers, and identifying the lowest
        // opaque layer which needs to be rendered during the Draw pass
        PreDraw(pContext);
        
        // if we didn't consume the redraw region, we'll have to start rendering
        // from the root
        if (!pContext->GetRedrawRegion()->IsEmpty())
        {
            pContext->_pFirstDrawNode = this;
            redrawRegionStack.PushRegionForRoot(pContext->GetRedrawRegion(), this, _rcContainer);
            contextStack.Init();
        }
        else
        {
            Assert(pContext->_pFirstDrawNode != NULL);
            AssertSz(redrawRegionStack.MoreToPop(), "Mysterious redraw region stack bug!");
            delete pContext->GetRedrawRegion();
            contextStack.Restore();
        }

    
#if DBG==1
        // we shouldn't reference this redraw region again
        pContext->SetRedrawRegion(NULL);
#endif

        if (!fOffscreen)
        {
            _pRenderSurface->SetBandOffset(g_Zero.size);
            DrawEntire(pContext);
        }
        else if (_pOffscreenBuffer->Height() >= _rcContent.Height())
        {
#if DBG == 1
            {
                HDC hdc = _pOffscreenBuffer->GetRawDC();
                HBRUSH hbr = CreateSolidBrush(RGB(0, 255, 0));
                CRect rc(-10000, -10000, 20000, 20000);
                FillRect(hdc, &rc, hbr);
                DeleteObject(hbr);
            }
#endif

            _pOffscreenBuffer->SetBandOffset(_rcContent.TopLeft().AsSize());
            DrawEntire(pContext);
            _pOffscreenBuffer->Draw(_pRenderSurface, _rcContent);
        }
        else
        {
            DrawBands(pContext, &rgnRedraw, redrawRegionStack);
        }
    
        pContext->SetClientData(NULL);

        // clear redraw region trails
        redrawRegionStack.Restore();
        for (;;)
        {
            CDispNode* pOpaqueNode = (CDispNode*) redrawRegionStack.PopKey();
            if (pOpaqueNode == NULL)
                break;
            pOpaqueNode->ClearFlagToRoot(CDispFlags::s_savedRedrawRegion);
        }
    
        // delete all regions except the first
        redrawRegionStack.DeleteStack(&rgnRedraw);
    
#if DBG==1
        // make sure we didn't lose any nodes that were marked with
        // savedRedrawRegion
        VerifyFlags(CDispFlags::s_savedRedrawRegion, CDispFlags::s_none, TRUE);
#endif
    }

Cleanup:
    // BUGBUG (donmarsh) -- restore _rcVisBounds to coincide with _rcContainer
    _rcVisBounds = _rcContainer;

    _fDrawLock = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawEntire
//
//  Synopsis:   Draw the display tree in one pass, starting at the top node
//              in the saved redraw region array.
//
//  Arguments:  pContext        draw context
//
//  Notes:      DrawEntire assumes that the display context is currently
//              set correctly to start drawing from the starting opaque node
//              (or the root).
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawEntire(CDispDrawContext* pContext)
{
    // pop first node and redraw region
    CDispNode* pDrawNode = (CDispNode*) pContext->PopFirstRedrawRegion();
    
    Assert(pDrawNode == pContext->_pFirstDrawNode);

    // we better have the correct context ready for the parent node
    Assert(pDrawNode != NULL &&
           (pDrawNode->_pParentNode == NULL ||
            pContext->GetContextStack()->GetTopNode() == pDrawNode->_pParentNode));
    
    while (pDrawNode != NULL)
    {
        // get context (clip rect and offset) for parent node
        CDispInteriorNode* pParent = pDrawNode->_pParentNode;
        
        if (pParent == NULL)
        {
            pParent = this;
            pDrawNode = NULL;
        }
        else if (!pContext->PopContext(pParent))
        {
            pContext->FillContextStack(pDrawNode);
            Verify(pContext->PopContext(pParent));
        }

        // draw children of this parent node, starting with this child
        pParent->Draw(pContext, pDrawNode);

        // find next node to the right of the parent node
        for (;;)
        {
            pDrawNode = pParent->_pNextSiblingNode;
            if (pDrawNode != NULL)
                break;
            pParent = pParent->_pParentNode;
            if (pParent == NULL)
                break;
            
            // this parent node should not have saved context information
            Assert(pContext->GetContextStack()->GetTopNode() != pParent);
        }
    }

    // stacks should now be empty
    Assert(!pContext->GetContextStack()->MoreToPop());
    Assert(!pContext->GetRedrawRegionStack()->MoreToPop());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawBand
//
//  Synopsis:   Draw one band using the display tree, starting at the top node
//              in the saved redraw region array.
//
//  Arguments:  pContext        draw context
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawBands(
        CDispDrawContext* pContext,
        CRegion* prgnRedraw,
        const CRegionStack& redrawRegionStack)
{
    CDispContextStack* pSaveContextStack = pContext->GetContextStack();

    Assert(_pOffscreenBuffer);

    long height = min(_rcContent.Height(), _pOffscreenBuffer->Height());
    while (!prgnRedraw->IsEmpty())
    {
        // compute next banding rectangle
        CRect rcBand;
        prgnRedraw->GetBounds(&rcBand);
        rcBand.left = _rcContent.left;
        rcBand.right = _rcContent.right;
        rcBand.bottom = rcBand.top + height;
        if (rcBand.bottom > _rcContent.bottom)
        {
            rcBand.bottom = _rcContent.bottom;
        }
        
        // BUGBUG (donmarsh) -- for some reason, we're getting here occasionally
        // with an empty rcBand.  At one time, this could be caused by
        // _pOffscreenBuffer->Height() returning zero.  I don't believe that
        // is possible any more.  However, something is happening, and we have
        // to check for it, or we will go into an infinite loop.
        Assert(height > 0 && _pOffscreenBuffer->Height() > 0);
        Assert(!rcBand.IsEmpty());
        Assert(prgnRedraw->Intersects(rcBand));
        if (rcBand.bottom <= rcBand.top || !prgnRedraw->Intersects(rcBand))
            break;
        
        _pOffscreenBuffer->SetBandOffset(rcBand.TopLeft() - _rcContainer.TopLeft());
        
#if DBG==1
        // show invalid area for debugging
        CRect rcDebug(0,0,_rcContainer.Width(), height);
        CDebugPaint::ShowPaint(
            &rcDebug, NULL,
            pContext->GetRawDC(),
            tagPaintShow, tagPaintPause, tagPaintWait, TRUE);
#endif

        // clip regions in redraw region stack to this band
        CRegionStack clippedRedrawRegionStack(redrawRegionStack, rcBand);
        
        if (clippedRedrawRegionStack.MoreToPop())
        {
            pContext->SetRedrawRegionStack(&clippedRedrawRegionStack);
    
            // restore initial context stack
            pContext->SetContextStack(pSaveContextStack);
            pSaveContextStack->Restore();
            
#if DBG == 1
            {
                HDC hdc = _pOffscreenBuffer->GetRawDC();
                HBRUSH hbr = CreateSolidBrush(RGB(0, 255, 0));
                CRect rc(-10000, -10000, 20000, 20000);
                FillRect(hdc, &rc, hbr);
                DeleteObject(hbr);
            }
#endif
            
            // draw contents of this band
            DrawBand(pContext);
    
            // draw offscreen buffer to destination
            _pOffscreenBuffer->Draw(_pRenderSurface, rcBand);
            
            // discard clipped regions
            clippedRedrawRegionStack.DeleteStack();
        }
        
        // remove band from redraw region
        prgnRedraw->Subtract(rcBand);
    }
}


void
CDispRoot::DrawBand(CDispDrawContext* pContext)
{
    // pop first node and redraw region
    CDispNode* pDrawNode = (CDispNode*) pContext->PopFirstRedrawRegion();

    // use an alternate context stack if drawing will start from a different
    // node in this band than in other bands
    CDispContextStack alternateContextStack;

    // create an alternate context stack so the initial context stack can be
    // left intact for subsequent invocations of DrawBand
    if (pDrawNode != pContext->_pFirstDrawNode)
    {
        pContext->SetContextStack(&alternateContextStack);
    }

    while (pDrawNode != NULL)
    {
        // get context (clip rect and offset) for parent node
        CDispInteriorNode* pParent = pDrawNode->_pParentNode;
        if (pParent == NULL)
        {
            pParent = this;
            pDrawNode = NULL;
        }
        else if (!pContext->PopContext(pParent))
        {
            pContext->SetContextStack(&alternateContextStack);
            pContext->FillContextStack(pDrawNode);
            Verify(pContext->PopContext(pParent));
        }

        // draw children of this parent node, starting with this child
        pParent->Draw(pContext, pDrawNode);

        // find next node to the right of the parent node
        for (;;)
        {
            pDrawNode = pParent->_pNextSiblingNode;
            if (pDrawNode != NULL)
                break;
            pParent = pParent->_pParentNode;
            if (pParent == NULL)
                break;

            // this parent node should not have saved context information
            Assert(pContext->GetContextStack()->GetTopNode() != pParent);
        }
    }

    // stacks should now be empty
    Assert(!pContext->GetContextStack()->MoreToPop());
    Assert(!pContext->GetRedrawRegionStack()->MoreToPop());
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawNode
//
//  Synopsis:   Draw the display tree rooted at the given display node.
//
//  Arguments:  pNode           node to draw
//              pSurface        display surface on which to draw
//              ptOrg           where to draw this node
//              rgnClip         clip region in surface coordinates
//              pClientData     client-specific rendering data
//
//  Notes:      
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawNode(
        CDispNode* pNode,
        CDispSurface* pSurface,
        const POINT& ptOrg,
        HRGN rgnClip,
        void* pClientData)
{
    // DrawRoot does not allow recursion
    AssertSz(!_fDrawLock, "Illegal call to DrawNodeLayer inside Draw()");
    AssertSz(_cOpen == 0, "DrawNodeLayer called while Display Tree is open");
    Assert(pSurface != NULL);

#if DBG==1
    // shouldn't be here with a tree needing recalc
    VerifyFlags(CDispFlags::s_generalFlagsNotSetInDraw, CDispFlags::s_none, TRUE);
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));
#endif

    // punt if this branch isn't visible or inview
    if (!pNode->AllSet(CDispFlags::s_drawSelector))
        return;

    _fDrawLock = TRUE;

    // BUGBUG (donmarsh) -- for ultimate performance, we should set things up
    // like DrawRoot in order to do a PreDraw pass on the children belonging to
    // the indicated layer.  However, this is complicated by the fact that we
    // may begin drawing at an arbitrary node deep in the tree below this node,
    // and we have to be sure not to draw any nodes above this node.
    // DrawEntire, which we would like to
    // use to accomplish this, does not stop drawing until it has drawn all
    // layers in all nodes all the way to the root.  For now, we do the simple
    // thing, and just draw all of our children,
    // ignoring the opaque optimizations of PreDraw.
    
    // set initial context values
    CDispDrawContext context;

    context._rcClip = pNode->GetBounds();
    context._offset = (const CPoint&) ptOrg - pNode->GetBounds().TopLeft();
    context.SetClientData(pClientData);
    context._pFirstDrawNode = NULL;
    
    // set redraw region (will become a rect if possible)

    CRegion rgnRedraw;

    if (rgnClip)
    {
        rgnRedraw = rgnClip;
    }
    else
    {
        rgnRedraw = context._rcClip;
        rgnRedraw.Offset(context._offset);
    }

    context.SetRedrawRegion(&rgnRedraw);

    // allocate stacks for redraw regions and context values
    CRegionStack redrawRegionStack;
    context.SetRedrawRegionStack(&redrawRegionStack);
    CDispContextStack contextStack;
    context.SetContextStack(&contextStack);

    context.SetDispSurface(pSurface);

    context._fBypassFilter = TRUE;
    pNode->Draw(&context, NULL);
    
    _fDrawLock = FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::PushContext
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
CDispRoot::PushContext(
        const CDispNode* pChild,
        CDispContextStack* pContextStack,
        CDispContext* pContext) const
{
    Assert(_pParentNode == NULL);

    pContext->_rcClip.SetRect(_rcContainer.Size());
    pContext->_offset = _rcContainer.TopLeft().AsSize();

    // context needs to be saved only if this child is not our last, or this
    // will be the first entry in the context stack
    if (pChild != _pLastChildNode || pContextStack->IsEmpty())
    {
        pContextStack->ReserveSlot(this);
        pContextStack->PushContext(*pContext, this);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::RecalcChildren
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
CDispRoot::RecalcChildren(
        BOOL fForceRecalc,
        BOOL fSuppressInval,
        CDispDrawContext* pContext)
{
    super::RecalcChildren(fForceRecalc, fSuppressInval, pContext);

    // the root is always a visible, opaque, in-view branch
    SetFlag(CDispFlags::s_preDrawSelector);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetRootSize
//
//  Synopsis:   Set the size of the root container.
//
//  Arguments:  size                new size
//              fInvalidateAll      TRUE if entire area should be invalidated
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispRoot::SetRootSize(const SIZE& size, BOOL fInvalidateAll)
{
    AssertSz(!_fDrawLock, "Illegal call to SetRootSize inside Draw()");
    
    if (fInvalidateAll)
    {
        _rcContainer.SetSize(size);
        _rcVisBounds.SetSize(size);
        InvalidateRoot(_rcVisBounds, FALSE, FALSE);
        SetFlag(CDispFlags::s_invalAndRecalcChildren);
        RequestRecalc();
        return;
    }

    if (_rcContainer.Size() == size)
        return;
    
    // invalidate uncovered area
    CRect rcNew(_rcContainer.TopLeft(), size);
    if (rcNew.right != _rcContainer.right)
    {
        InvalidateRoot(
            CRect(min(rcNew.right, _rcContainer.right),
              _rcContainer.top,
              max(rcNew.right, _rcContainer.right),
              max(rcNew.bottom, _rcContainer.bottom)),
            FALSE, FALSE);
    }
    if (rcNew.bottom != _rcContainer.bottom)
    {
        InvalidateRoot(
            CRect(_rcContainer.left,
                  min(rcNew.bottom, _rcContainer.bottom),
                  max(rcNew.right, _rcContainer.right),
                  max(rcNew.bottom, _rcContainer.bottom)),
            FALSE, FALSE);
    }

    // set new container size
    _rcContainer.SetSize(size);
    _rcVisBounds.SetSize(size);
    SetFlag(CDispFlags::s_recalcChildren);
    RequestRecalc();
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetContentOffset
//              
//  Synopsis:   Set an offset that shifts displayed content (used by printing
//              to effectively scroll content between pages).
//              
//  Arguments:  sizeOffset      offset amount, where positive values display
//                              content farther to the right and bottom
//                              
//  Returns:    TRUE if the content offset amount was successfully set.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispRoot::SetContentOffset(const SIZE& sizeOffset)
{
    AssertSz(_cOpen == 1, "Display Tree: Unexpected call to SetContentOffset");
    
    if (_pFirstChildNode != NULL)
    {
        if (_pFirstChildNode->IsScroller())
        {
            CDispScroller* pScroller = DYNCAST(CDispScroller, _pFirstChildNode);
            pScroller->ForceScrollOffset(sizeOffset);
            return TRUE;
        }
    }
    
    return FALSE;
}


void
CDispRoot::RecalcRoot()
{
    AssertSz(_cOpen == 1, "Display Tree: Unexpected call to RecalcRoot");
#ifndef VSTUDIO7
    AssertSz(!_fDrawLock, "Illegal call to RecalcRoot inside Draw()");
#endif //VSTUDIO7

    if (IsSet(CDispFlags::s_recalc))
    {
        // set initial clip
        _drawContext._rcClip = _rcContainer;
        _drawContext._offset = _rcContainer.TopLeft().AsSize();

        BOOL fSizeChanged = IsSet(CDispFlags::s_inval);
        _fRecalcLock = TRUE;
        Recalc(fSizeChanged, fSizeChanged, &_drawContext);
        _fRecalcLock = FALSE;
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetDestination
//
//  Synopsis:   Set destination rendering surface.
//
//  Arguments:  hdc         DC destination
//              pSurface    IDirectDrawSurface
//              fOnscreen   TRUE if rendering surface is onscreen
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispRoot::SetDestination(
        HDC hdc,
        IDirectDrawSurface *pDDSurface)
{
    AssertSz(!_fDrawLock, "Illegal call to SetDestination inside Draw()");
    
    CDispSurface *pSurface = NULL;

    if (hdc)
        pSurface = new CDispSurface(hdc);
    else if (pDDSurface)
        pSurface = new CDispSurface(pDDSurface);

    if (pSurface)
    {
        if (_pRenderSurface)
            delete _pRenderSurface;

        _pRenderSurface = pSurface;
        _drawContext.SetDispSurface(_pRenderSurface);
    }
}


CDispItemPlus*
CDispRoot::CreateDispItemPlus(
        CDispClient* pDispClient,
        BOOL fHasExtraCookie,
        BOOL fHasUserClip,
        BOOL fHasInset,
        DISPNODEBORDER borderType,
        BOOL fRightToLeft)
{
    size_t extrasSize =
        CDispExtras::GetExtrasSize(fHasExtraCookie, fHasUserClip, fHasInset, borderType);
    CDispItemPlus* pItem =
        new (extrasSize) CDispItemPlus(pDispClient);
    if (pItem != NULL)
    {
        CDispExtras* pExtras = pItem->GetExtras();
        pExtras->SetHasExtraCookie(fHasExtraCookie);
        if (fHasUserClip)
        {
            pExtras->SetHasUserClip(TRUE);
            pItem->SetFlag(CDispFlags::s_hasUserClip);
        }
        pExtras->SetHasInset(fHasInset);
        pExtras->SetBorderType(borderType);
        pItem->SetRightToLeft(fRightToLeft);
    }
    return pItem;
}

CDispGroup*
CDispRoot::CreateDispGroup(CDispClient* pDispClient)
{
    return new CDispGroup(pDispClient);
}

CDispContainer*
CDispRoot::CreateDispContainer(
        CDispClient* pDispClient,
        BOOL fHasExtraCookie,
        BOOL fHasUserClip,
        BOOL fHasInset,
        DISPNODEBORDER borderType,
        BOOL fRightToLeft)
{
    CDispContainer* pContainer;
    
    if (fHasExtraCookie || fHasUserClip ||
        fHasInset || borderType != DISPNODEBORDER_NONE)
    {
        size_t extrasSize =
            CDispExtras::GetExtrasSize(fHasExtraCookie, fHasUserClip, fHasInset, borderType);
        pContainer = new (extrasSize) CDispContainerPlus(pDispClient);
        if (pContainer != NULL)
        {
            CDispExtras* pExtras = pContainer->GetExtras();
            pExtras->SetHasExtraCookie(fHasExtraCookie);
            if (fHasUserClip)
            {
                pExtras->SetHasUserClip(TRUE);
                pContainer->SetFlag(CDispFlags::s_hasUserClip);
            }
            pExtras->SetHasInset(fHasInset);
            pExtras->SetBorderType(borderType);
        }
    }
    else
        pContainer = new CDispContainer(pDispClient);
    
    if (pContainer != NULL)
        pContainer->SetRightToLeft(fRightToLeft);
    
    return pContainer;
}

CDispContainer*
CDispRoot::CreateDispContainer(const CDispItemPlus* pItemPlus)
{
    Assert(pItemPlus != NULL);
    CDispExtras* pExtras = (CDispExtras*) pItemPlus->_extra;
    BOOL fHasExtraCookie = pExtras->HasExtraCookie();
    BOOL fHasUserClip = pExtras->HasUserClip();
    BOOL fHasInset = pExtras->HasInset();
    DISPNODEBORDER borderType = pExtras->GetBorderType();

    if (fHasExtraCookie || fHasUserClip || fHasInset || borderType != DISPNODEBORDER_NONE)
    {
        size_t extrasSize =
            CDispExtras::GetExtrasSize(fHasExtraCookie, fHasUserClip, fHasInset, borderType);
        return new (extrasSize) CDispContainerPlus(pItemPlus);
    }

    return new CDispContainer(pItemPlus);
}

CDispScroller*
CDispRoot::CreateDispScroller(
        CDispClient* pDispClient,
        BOOL fHasExtraCookie,
        BOOL fHasUserClip,
        BOOL fHasInset,
        DISPNODEBORDER borderType,
        BOOL fRightToLeft)
{
    CDispScroller* pScroller;
    
    if (fHasExtraCookie || fHasUserClip ||
        fHasInset || borderType != DISPNODEBORDER_NONE)
    {
        size_t extrasSize =
            CDispExtras::GetExtrasSize(fHasExtraCookie, fHasUserClip, fHasInset, borderType);
        pScroller = new (extrasSize) CDispScrollerPlus(pDispClient);
        if (pScroller != NULL)
        {
            CDispExtras* pExtras = pScroller->GetExtras();
            pExtras->SetHasExtraCookie(fHasExtraCookie);
            if (fHasUserClip)
            {
                pExtras->SetHasUserClip(TRUE);
                pScroller->SetFlag(CDispFlags::s_hasUserClip);
            }
            pExtras->SetHasInset(fHasInset);
            pExtras->SetBorderType(borderType);
        }
    }
    else
        pScroller = new CDispScroller(pDispClient);
    
    if (pScroller != NULL)
        pScroller->SetRightToLeft(fRightToLeft);
    
    return pScroller;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ReleaseOffscreenBuffer
//
//  Synopsis:   Release the buffer used to perform offscreen rendering.
//
//  Arguments:  none
//
//  Notes:      It's useful to have this as a non-inline function for easy
//              debugging.
//
//----------------------------------------------------------------------------

void
CDispRoot::ReleaseOffscreenBuffer()
{
    if (_pOffscreenBuffer != NULL)
    {
        delete _pOffscreenBuffer;
        _pOffscreenBuffer = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispNode::ScrollRectIntoView
//
//  Synopsis:   Scroll the given rect in CONTENT coordinates into
//              view, with various pinning (alignment) options.
//
//  Arguments:  rc              rect to scroll into view
//              spVert          scroll pin request, vertical axis
//              spHorz          scroll pin request, horizontal axis
//              coordSystem     coordinate system for rc (COORDSYS_CONTENT
//                              or COORDSYS_NONFLOWCONTENT only)
//
//  Returns:    TRUE if any scrolling was necessary.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispRoot::ScrollRectIntoView(
        const CRect& rc,
        CRect::SCROLLPIN spVert,
        CRect::SCROLLPIN spHorz,
        COORDINATE_SYSTEM coordSystem,
        BOOL fScrollBits)
{
    BOOL fRet = super::ScrollRectIntoView(
                                    rc,
                                    spVert,
                                    spHorz,
                                    coordSystem,
                                    fScrollBits);

    // Need to pass the scroll request up the parent view chain

    CDispClient* pClient = GetDispClient();

    if (pClient != NULL)
    {
        // BUGBUG (MohanB) Need to get rid of this ugly cast in IE6 by
        // adding the method on IDispObserver.
        return DYNCAST(CViewDispClient, pClient)->ScrollRectIntoParentView(
                                                        rc,
                                                        spVert,
                                                        spHorz,
                                                        fScrollBits);
    }
    return fRet;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::ScrollRect
//
//  Synopsis:   Smoothly scroll the content in the given rect by the indicated
//              delta, and draw the newly-exposed content.
//
//  Arguments:  rcScroll        rect to scroll
//              scrollDelta     direction to scroll (one axis only!)
//              pDispScroller   which scroller node is calling
//              rgnInvalid      additional invalid area (used to invalidate
//                              scroll bars)
//              fMayScrollDC    TRUE if it would be okay to use ScrollDC
//
//  Returns:    TRUE if we used ScrollDC to do it.
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL
CDispRoot::ScrollRect(
        const CRect& rcScroll,
        const CSize& scrollDelta,
        CDispScroller* pScroller,
        const CRegion& rgnInvalid,
        BOOL fMayScrollDC)
{
    // shouldn't be here with recalc flags set
    WHEN_DBG(VerifyFlags(CDispFlags::s_generalFlagsNotSetInDraw, CDispFlags::s_none, TRUE));

    Assert(!scrollDelta.IsZero());
    AssertSz(!_fDrawLock, "CView is not preventing recursive draw calls into CDispRoot. :)\nCDispRoot::ScrollRect called inside Draw.");

    if (_fDrawLock &&
        _pDispObserver == NULL)
        return FALSE;

    BOOL fTemporaryRenderSurface = FALSE;
    HDC hdcSave = NULL;
    if (_pRenderSurface == NULL)
    {
        _pRenderSurface = new CDispSurface();
        if (_pRenderSurface == NULL)
            return FALSE;
        fTemporaryRenderSurface = TRUE;
    }
    else
    {
        // swap current DC
        hdcSave = _pRenderSurface->GetRawDC();
#ifndef _MAC
        _pRenderSurface->SetRawDC(NULL);
#endif
    }

    // for efficiency, we draw top-level scroll bars specially (no banding,
    // no invalidation)
    if (pScroller == _pFirstChildNode &&  _fCanScrollDC)
    {
    #ifdef _MAC
        if (  _pRenderSurface->GetRawDC() == nil )
    #endif
        _pRenderSurface->SetRawDC(_pDispObserver->GetClientDC(&_rcContainer));

        _drawContext.SetDispSurface(_pRenderSurface);
        _drawContext._prgnRedraw = (CRegion*) &rgnInvalid;
        _drawContext._offset = _rcContainer.TopLeft().AsSize();
        _drawContext._rcClip.SetRect(_rcContainer.Size());
        _drawContext._pClientData = NULL;
        _drawContext._pRedrawRegionStack = NULL;
        _pRenderSurface->SetBandOffset(_rcContainer.TopLeft().AsSize());
        _pRenderSurface->SetClipRgn(_drawContext._prgnRedraw);
        pScroller->DrawScrollbars(&_drawContext, DISPSCROLLBARHINT_NOBUTTONDRAW);
    }
    else
    {
        // add scroll bar invalid region to accumulated invalid region
        InvalidateRoot(rgnInvalid, FALSE, FALSE);
    }

    // invalidate entire scrolled area synchronously if we must
    // BUGBUG: Once ScrollRect is moved into CView, the check for LAYOUT_FORCE should be merged...
    if (!_fCanScrollDC || !fMayScrollDC || (((CView *)_pDispObserver)->GetLayoutFlags() & LAYOUT_FORCE))
    {
SyncInvalidate:
        if (_pRenderSurface->GetRawDC() != NULL)
        {
#ifdef _MAC
            if ( _pRenderSurface->GetRawDC() != hdcSave )
#endif
            _pDispObserver->ReleaseClientDC(_pRenderSurface->GetRawDC());
        }
        if (fTemporaryRenderSurface)
        {
            delete _pRenderSurface;
            _pRenderSurface = NULL;
        }
        else
            _pRenderSurface->SetRawDC(hdcSave);
    
        InvalidateRoot(rcScroll, TRUE, TRUE);

        return FALSE;
    }

    // notify the client of the scroll event
    _pDispClient->NotifyScrollEvent((RECT *)&rcScroll, (SIZE *)&scrollDelta);

    //
    // scroll synchronously
    //

    // get a DC if we didn't do it above
    if (_pRenderSurface->GetRawDC() == NULL)
    {
        _pRenderSurface->SetRawDC(_pDispObserver->GetClientDC(&_rcContainer));
    }

    // make sure we don't have any clip region, or ScrollDC won't report
    // the correct invalid region
    ::SelectClipRgn(_pRenderSurface->GetRawDC(), NULL);

    HRGN hrgn = ::CreateRectRgnIndirect(&g_Zero.rc);
    if (!hrgn)
        goto SyncInvalidate;

    Verify(::ScrollDC(_pRenderSurface->GetRawDC(), scrollDelta.cx, scrollDelta.cy, &rcScroll, &rcScroll, hrgn, NULL));

    InvalidateRoot(hrgn, FALSE, TRUE);
    ::DeleteObject(hrgn);

    // release the surface/HDC
#ifdef  _MAC
    if (_pRenderSurface->GetRawDC() != hdcSave)
#endif
        _pDispObserver->ReleaseClientDC(_pRenderSurface->GetRawDC());
    
    if (fTemporaryRenderSurface)
    {
        delete _pRenderSurface;
        _pRenderSurface = NULL;
    }
    else
        _pRenderSurface->SetRawDC(hdcSave);

    // synchronously redraw revealed content
    _pDispObserver->DrawSynchronous(NULL, NULL, NULL);

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::PreDraw
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
CDispRoot::PreDraw(CDispDrawContext* pContext)
{
    // Interesting nodes are visible, in-view, opaque
    Assert(AllSet(CDispFlags::s_preDrawSelector));
    Assert(!IsSet(CDispFlags::s_generalFlagsNotSetInDraw));
    Assert(!IsSet(CDispFlags::s_interiorFlagsNotSetInDraw));

    // we don't expect filter on the root node
    Assert(!IsFiltered());
    
    // root must have only one or zero children
    CDispNode* pChild = _pFirstChildNode;
    Assert(_pFirstChildNode == _pLastChildNode && _cChildren <= 1);

    if (pChild != NULL)
    {
        // only children which meet our selection criteria
        if (pChild->AllSet(CDispFlags::s_preDrawSelector))
        {
            // if we found the first child to draw, stop further PreDraw calcs
            if (PreDrawChild(pChild, pContext, *pContext))
                return TRUE;
        }
    }

    // root is always opaque
    pContext->_pFirstDrawNode = this;
    Verify(!pContext->PushRedrawRegion(*(pContext->GetRedrawRegion()),this));
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::DrawSelf
//
//  Synopsis:   Draw this node's children and background.
//
//  Arguments:  pContext        draw context
//              pChild          start drawing at this child
//
//  Notes:
//
//----------------------------------------------------------------------------

void
CDispRoot::DrawSelf(CDispDrawContext* pContext, CDispNode* pChild)
{
    // draw optional background
    if (HasBackground() && (pChild == NULL || !_pFirstChildNode->IsVisible()))
    {
        // calculate intersection with redraw region
        CRect rcBackground(_rcContainer.Size());
        CRect rcBackgroundClip(rcBackground);
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
    }

    // draw children, bottom layers to top
    if (pChild == NULL)
    {
        pChild = _pFirstChildNode;
    }

    // only children which meet our visibility and inview criteria
    if (pChild != NULL && pChild->AllSet(CDispFlags::s_drawSelector))
    {
        pChild->Draw(pContext, NULL);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::HitTest
//              
//  Synopsis:   Determine whether and what the given point intersects.
//
//  Arguments:  pptHit              [in] the point to test
//                                  [out] if something was hit, the point is
//                                  returned in container coordinates for the
//                                  thing that was hit
//              coordinateSystem    the initial coordinate system for pptHit
//              pClientData         client data
//              fHitContent         if TRUE, hit test the content regardless
//                                  of whether it is clipped or not. If FALSE,
//                                  take current clipping into account,
//                                  and clip to the bounds of this container.
//              cFuzzyHitTest       Number of pixels to extend hit testing
//                                  rectangles in order to do "fuzzy" hit
//                                  testing
//
//  Returns:    TRUE if the point hit something.
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

BOOL
CDispRoot::HitTest(
        CPoint* pptHit,
        COORDINATE_SYSTEM coordinateSystem,
        void* pClientData,
        BOOL fHitContent,
        long cFuzzyHitTest)
{
    if (!fHitContent)
    {
        return super::HitTest(pptHit, coordinateSystem, pClientData, fHitContent, cFuzzyHitTest);
    }
    
    // when trying to hit content, CDispRoot can't determine the size of
    // the content that we're trying to hit.  Therefore, we have to ask our
    // child node, a CDispScroller.
    if (_pFirstChildNode != NULL)
    {
        CPoint ptHit(*pptHit);
        if (coordinateSystem == COORDSYS_GLOBAL ||
            coordinateSystem == COORDSYS_PARENT)
        {
            ptHit -= _rcContainer.TopLeft().AsSize();
        }
        
        // BUGBUG (donmarsh) -- not taking user clip into account on first child
        // of root.
        BOOL fHit =
            _pFirstChildNode->HitTest(&ptHit, COORDSYS_PARENT, pClientData,
                                      fHitContent, cFuzzyHitTest);
        if (fHit)
            *pptHit = ptHit;
        
        return fHit;
    }
    
    return FALSE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::EraseBackground
//              
//  Synopsis:   Quickly draw border and background.
//              
//  Arguments:  pContext        draw context
//              pClientData     client data used by clients in DrawClient
//              hrgnDraw        region to draw in destination coordinates
//              prcDraw         rect to draw in destination coordinates
//
//  Notes:      if hrgnDraw and prcDraw are both NULL, the bounding rect of
//              this root node is used
//              
//----------------------------------------------------------------------------

void
CDispRoot::EraseBackground(
        CDispDrawContext* pContext,
        void* pClientData,
        HRGN hrgnDraw,
        const RECT* prcDraw)
{
    // BUGBUG (donmarsh) - EraseBackground can be called while we are recalcing
    // the display tree!  For example, CSelectLayout::HandleViewChange changes
    // the clip region of child windows, which causes an immediate
    // call to EraseBackground.  This is messy, because we could stomp on
    // values in pContext that are in use by the recalc code.  Therefore, we
    // ignore these calls.  If the following Assert fires, you should look
    // at the stack and protect the operation that called it with
    // CServer::CLock lock(Doc(), SERVERLOCK_IGNOREERASEBKGND);
    AssertSz(!_fDrawLock, "EraseBackground called during drawing");
// BUGBUG (donmarsh) -- temporarily disable this assert to help 5.0 stress numbers
//    AssertSz(!_fRecalcLock, "EraseBackground called during recalc");
    if (_fDrawLock || _fRecalcLock)
        return;
    
    // BUGBUG (donmarsh) - for now we must use this root's own context, due
    // to the context stack used for drawing.  Eventually, we should eliminate
    // the context pointer argument to DrawRoot.
    Assert(pContext == &_drawContext);
    AssertSz(!_fDrawLock, "Illegal call to EraseBackground inside Draw()");
    Assert(_pRenderSurface != NULL);

    // set redraw region (will become a rect if possible)
    CRegion rgnRedraw;
    if (hrgnDraw != NULL)
    {
        rgnRedraw = hrgnDraw;
    }
    else if (prcDraw != NULL)
    {
        rgnRedraw = *prcDraw;
    }
    else
    {
        rgnRedraw = _rcContainer;
    }
    
    // check for early exit conditions
    if (!rgnRedraw.Intersects(_rcContainer))
        return;
    
    // set initial context values
    pContext->SetClientData(pClientData);
    pContext->_offset = _rcContainer.TopLeft().AsSize();
    pContext->_pFirstDrawNode = NULL;
    pContext->SetRedrawRegion(&rgnRedraw);
    pContext->SetDispSurface(_pRenderSurface);
    _pRenderSurface->SetBandOffset(g_Zero.size);

    // BUGBUG (donmarsh) - To address bug 62008 (erase background for HTML Help
    // control), we need to disable clipping in CDispSurface::GetDC.  Ideally,
    // CDispSurface should allow us to have a NULL _prgnClip, but that is
    // currently not the case.
    CRegion rgnClip(-15000,-15000,15000,15000);

    _pRenderSurface->SetClipRgn(&rgnClip);
    
    if (_pFirstChildNode == NULL)
    {
        DrawSelf(pContext, NULL);
    }
    
    else if (_pFirstChildNode->IsScroller())
    {
        CDispScroller* pScroller = DYNCAST(CDispScroller, _pFirstChildNode);
        CDispExtras* pExtras = pScroller->GetExtras();
        CDispInfo di(pExtras);
        pScroller->CalcDispInfo(_rcContainer, &di);
        
        // draw root background if scroller isn't opaque or doesn't have a background
        if (!pScroller->IsOpaque() || !pScroller->HasBackground())
        {
            Assert(HasBackground());
            CRect rcBackground(_rcContainer.Size());
            CRect rcBackgroundClip(rcBackground);
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
        }
        else
        {
            pScroller->DrawBackground(pContext, &di);
        }
        
        // draw scroller's border if it has one
        if (pScroller->HasBorder(pExtras))
        {
            pScroller->DrawBorder(pContext, &di);
        }
    }
    
    _pRenderSurface->SetClipRgn(NULL);
    ::SelectClipRgn(_pRenderSurface->GetRawDC(), NULL);
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::InvalidateRoot
//              
//  Synopsis:   Add the given rect to the accumulated invalid area.
//              
//  Arguments:  rcInval     invalid rect
//              
//  Notes:      Slot 0 in the invalid rect array is special.  It is used to
//              hold the new invalid rect while figuring out which rects to
//              merge.
//              
//----------------------------------------------------------------------------

void
CDispRoot::InvalidateRoot(const CRect& rcInval, BOOL fSynchronousRedraw, BOOL fInvalChildWindows)
{
    // Unfortunately, we can't assert this, because certain OLE controls
    // invalidate when they are asked to draw.  This isn't really harmful,
    // just not optimal.
    //AssertSz(!_fDrawLock, "Illegal call to CDispRoot inside Draw()");

    if (_pDispObserver != NULL)
    {
        // BUGBUG (donmarsh) - HACK flags until we can clean up IDispObserver
        DWORD flags = 0;
        if (fSynchronousRedraw) flags  = 0x01;
        if (fInvalChildWindows) flags |= 0x02;

        _pDispObserver->Invalidate(&rcInval, NULL, flags);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::InvalidateRoot
//              
//  Synopsis:   Add the given invalid region to the accumulated invalid area.
//              
//  Arguments:  rgn         region to invalidate
//              
//  Notes:      
//              
//----------------------------------------------------------------------------

void
CDispRoot::InvalidateRoot(const CRegion& rgn, BOOL fSynchronousRedraw, BOOL fInvalChildWindows)
{
    AssertSz(!_fDrawLock, "Illegal call to CDispRoot inside Draw()");

    if (rgn.IsRegion())
    {
        if (_pDispObserver != NULL)
        {
            // BUGBUG (donmarsh) - HACK flags until we can clean up IDispObserver
            DWORD flags = 0;
            if (fSynchronousRedraw) flags  = 0x01;
            if (fInvalChildWindows) flags |= 0x02;

            _pDispObserver->Invalidate(NULL, rgn.GetRegionAlias(), flags);
        }
    }
    else
    {
        InvalidateRoot(rgn.AsRect(), fSynchronousRedraw, fInvalChildWindows);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetOffscreenBufferInfo
//              
//  Synopsis:   Remember stuff about how we will need to buffer
//              
//  Arguments:  see CreateBuffer above
//              
//  Notes:      fWantOffscreen indicates that the document (and host) want offscreen buffering
//              fAllowOffscreen indicates that the document (and host) will allow offscreen buffering
//              
//----------------------------------------------------------------------------
void
CDispRoot::SetOffscreenBufferInfo(HPALETTE hpal, short bufferDepth, BOOL fDirectDraw, BOOL fTexture, BOOL fWantOffscreen, BOOL fAllowOffscreen)
{
    //
    // All we do now is set things up.  Later on we'll either re-use or release the 
    // current buffer if it isn't compat
    //
    _hpal = hpal;
    _cBufferDepth = bufferDepth;
    _fDirectDraw = fDirectDraw;
    _fTexture = fTexture;
    _fAllowOffscreen = fAllowOffscreen;
    _fWantOffscreen = fWantOffscreen;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDispRoot::SetupOffscreenBuffer
//              
//  Synopsis:   The primary place of policy regarding our offscreen buffering
//              The only other policy is in the CDispSurface->IsCompat that
//              determines if a cached surface is acceptable.
//              
//  Arguments:  pContext
//              
//  Notes:      
//              
//----------------------------------------------------------------------------
BOOL
CDispRoot::SetupOffscreenBuffer(CDispDrawContext *pContext)
{
    Assert(pContext->GetDispSurface() == _pRenderSurface);
    WHEN_DBG(pContext->SetDispSurface(_pRenderSurface));
    PerfDbgExtern(tagOscUseDD);
    PerfDbgExtern(tagNoTile);
    PerfDbgExtern(tagOscFullsize);
    PerfDbgExtern(tagOscTinysize);
    PerfDbgExtern(tagNoOffScr);

    // If the host won't allow offscreen buffering then we won't do it
    if (!_fAllowOffscreen)
        return FALSE;


    short bufferDepthDestination = GetDeviceCaps(pContext->GetRawDC(), PLANES) * GetDeviceCaps(pContext->GetRawDC(), BITSPIXEL);
    short bufferDepth;

    if (_cBufferDepth == 0 || _cBufferDepth == -1)
        bufferDepth = bufferDepthDestination;
    else
        bufferDepth = _cBufferDepth;

    BOOL fDirectDraw = _fDirectDraw || (_cBufferDepth == -1);
#if DBG == 1
    if (IsTagEnabled(tagOscUseDD))
        fDirectDraw = TRUE;
#endif

    BOOL fWantOffscreen;
    if (bufferDepth != bufferDepthDestination)
    {
        fDirectDraw = TRUE;     // GDI can't buffer at a bit depth different from the destination
        fWantOffscreen = TRUE;
    }
    else
    {
        fWantOffscreen = _fWantOffscreen || (fDirectDraw && !_pRenderSurface->VerifyGetSurfaceFromDC());
    }

    // If we can avoid buffering, we should

    if (!fWantOffscreen)
        return FALSE;

    // We are definitely going to buffer offscreen
    // Now we determine the size of the buffer

    long height;

    //
    // BUGBUG (michaelw)
    //
    // If we're using DD then we don't tile.  This is mostly because DA and filters don't handle
    // banding very well.  We really need a new status bit that indicates poor tiling support
    //

#ifdef MAC
    height = _rcContent.Height();
#else
    if (fDirectDraw)
        height = _rcContent.Height();
    else
        height = s_cBUFFERLINES;
#endif

#if DBG == 1
    if (IsTagEnabled(tagOscFullsize))
    {
        height = _rcContent.Height();
    }
    else if (IsPerfDbgEnabled(tagOscTinysize))
    {
        height = 8;
    }
    else if (fDirectDraw && IsTagEnabled(tagOscForceDDBanding))
    {
        height = s_cBUFFERLINES;
    }
#endif

    Assert(height > 0);
    Assert(_rcContent.Width() > 0);

    if (!_pOffscreenBuffer || !_pOffscreenBuffer->IsCompat(_rcContent.Width(), height, bufferDepth, _hpal, fDirectDraw, _fTexture))
    {
        ReleaseOffscreenBuffer();

        _pOffscreenBuffer = _pRenderSurface->CreateBuffer(
                        _rcContent.Width(), 
                        height, 
                        bufferDepth,
                        _hpal, 
                        fDirectDraw,
                        _fTexture);
    }

    if (_pOffscreenBuffer)
    {
        pContext->SetDispSurface(_pOffscreenBuffer);
        return TRUE;
    }

    return FALSE;
}
