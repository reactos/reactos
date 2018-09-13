//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltpos.cxx
//
//  Contents:   CTableLayout positioning methods.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_MSHTMDID_H_
#define X_MSHTMDID_H_
#include <mshtmdid.h>
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif

#ifndef X_DISPRGN_HXX_
#define X_DISPRGN_HXX_
#include "disprgn.hxx"
#endif

#ifndef X_DISPITEMPLUS_HXX_
#define X_DISPITEMPLUS_HXX_
#include "dispitemplus.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

#ifndef X_FILTER_HXX_
#define X_FILTER_HXX_
#include "filter.hxx"
#endif


ExternTag(tagTableRecalc);
ExternTag(tagLayout);


//+----------------------------------------------------------------------------
//
//  Member:     GetPositionInFlow
//
//  Synopsis:   Return the position of a layout derived from its position within
//              the document text flow
//
//  Arguments:  pLayout - Layout to position
//              ppt     - Returned top/left (in parent content relative coordinates)
//
//-----------------------------------------------------------------------------

void
CTableLayout::GetPositionInFlow(CElement *pElement, CPoint  *ppt)
{
    CLayout *pLayout = pElement->GetUpdatedLayout();
    Assert(pLayout && "We are in deep trouble if the element passed in doesn't have a layout");

    Assert(DYNCAST(CTableCell, pElement));
    Assert(ppt);

    //
    // Locate the layout within the document
    //

    ppt->x = pLayout->GetXProposed();
    ppt->y = pLayout->GetYProposed();
}


//+----------------------------------------------------------------------------
//
//  Member:     AddLayoutDispNode
//
//  Synopsis:   Add the display node of passed layout
//
//  Arguments:  pLayout        - Layout whose display node is to be added
//              pDispContainer - CDispNode which should contain the display node
//              pDispSibling   - Left-hand sibling display node
//              ppt            - Pointer to POINT with position or NULL
//              fBefore        - FALSE to add as next sibling, TRUE to add as previous sibling
//
//              NOTE: Either pDispContainer or pDispSibling must be supplied, the
//                    other may be NULL
//
//  Returns:    S_OK if added, S_FALSE if ignored, E_FAIL otherwise
//-----------------------------------------------------------------------------
HRESULT
CTableLayout::AddLayoutDispNode(
    CLayout *           pLayout,
    CDispContainer *    pDispContainer,
    CDispNode *         pDispNodeSibling,
    const POINT *       ppt,
    BOOL                fBefore)
 {
    CDispNode * pDispNode;
    HRESULT     hr = S_OK;
    CPoint      ptTopRight = g_Zero.pt;

    Assert(pLayout);
    Assert(pLayout != this);
    Assert(pDispContainer || pDispNodeSibling);

    if (!_pDispNode)
        goto Error;

    pDispNode = pLayout->GetElementDispNode();

    if (!pDispNode)
        goto Error;

    // If the node is already right to left, we need to make sure
    // that the left is set to 0. We will be flipping it down below.
    // We may or may not already have RTL coordinate system on this
    // node. This will prevent RTL getting flipped back to LTR. We need to get the
    // current top right to reset the position after it has been flipped.
    if(pDispNode->IsRightToLeft())
    {
        if(pDispNode->GetParentNode() && pDispNode->GetParentNode()->IsRightToLeft())
            pDispNode->GetPositionTopRight(&ptTopRight);

        // set up for flipping below
        CPoint pt(0, ptTopRight.y);
        pDispNode->SetPosition(pt);
    }

    Assert(pDispNode != pDispNodeSibling);
    Assert(pDispNode->IsOwned());
    Assert(pDispNode->GetLayerType() == DISPNODELAYER_FLOW);

    if (!pLayout->IsDisplayNone())
    {
        if (ppt)
        {
            pLayout->SetPosition(*ppt, TRUE);
        }

        if (pDispNodeSibling)
        {
            // get parent node and find out if it is RTL
            BOOL fRightToLeft = pDispNodeSibling->GetParentNode() != NULL &&
                                pDispNodeSibling->GetParentNode()->IsRightToLeft();

            // cells are sized before they are inserted. This means that the cell
            // is LTR. We need to change this here.
            if(fRightToLeft)
            {
                pDispNode->FlipBounds();
                pDispNode->SetPositionTopRight(ptTopRight);
            }
            pDispNodeSibling->InsertSiblingNode(pDispNode, fBefore);

        }
        else
        {
            // get parent node and find out if it is RTL
            BOOL fRightToLeft = pDispContainer->IsRightToLeft();

            // cells are sized before they are inserted. This means that the cell
            // is LTR. We need to change this here.
            if(fRightToLeft)
            {
                pDispNode->FlipBounds();
                pDispNode->SetPositionTopRight(ptTopRight);
            }
            pDispContainer->InsertFirstChildInFlow(pDispNode);
        }

    }
    else
    {
        pDispNode->ExtractFromTree();
        hr = S_FALSE;
    }

    return hr;

Error:
    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureTableDispNode
//
//  Synopsis:   Manage the lifetime of the table layout display node
//
//  Arugments:  pdci   - Current CDocInfo
//              fForce - Forcibly update the display node(s)
//
//  Returns:    S_OK    if successful
//              S_FALSE if nodes were created/destroyed
//              E_FAIL  otherwise
//
//-----------------------------------------------------------------------------
HRESULT
CTableLayout::EnsureTableDispNode(
    CDocInfo *  pdci,
    BOOL        fForce)
{
    CElement *          pElement = ElementOwner();
    CDispContainer *    pDispNode;
    CDispNode *         pDispNodeElement;
    CDispContainer *    pDispNodeTableOuter;
// BUGBUG: Save this structure throughout table measuring? (brendand)
    CDispNodeInfo       dni;
    HRESULT             hr = S_OK;

    Assert(pdci);

    //
    //  Get display node attributes
    //

    GetDispNodeInfo(&dni, pdci, TRUE);

    //
    //  Locate the display node that anchors cells
    //  (If a separate CAPTIONs display node exists, the display node for cells
    //   will be the only unowned node in the flow layer)
    //

    pDispNodeElement    = GetElementDispNode();
    pDispNodeTableOuter = GetTableOuterDispNode();
    
    //
    //  If a display node is needed to hold CAPTIONs and TCs, ensure one exists
    //

    if (    _aryCaptions.Size()
        &&  (   !_fHasCaptionDispNode
            ||  fForce
            ||  dni.HasUserClip() != pDispNodeElement->HasUserClip()
            ||  dni.IsRTL()       != pDispNodeElement->IsRightToLeft()))
    {
        pDispNode = CDispRoot::CreateDispContainer(
                                    this,
                                    FALSE,
                                    dni.HasUserClip(),
                                    FALSE,
                                    DISPNODEBORDER_NONE,
                                    dni.IsRTL());

        if (!pDispNode)
            goto Error;

        pDispNode->SetOwned();
        pDispNode->SetFiltered(pElement->HasFilterPtr());
        pDispNode->SetAffectsScrollBounds(!ElementOwner()->IsRelative());

        EnsureDispNodeLayer(dni, pDispNode);

        //
        // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
        // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
        //        
        EnsureTableFatHitTest( pDispNode );
        
        if (pDispNodeElement)
        {
            if (_fHasCaptionDispNode)
            {
                pDispNode->ReplaceNode(pDispNodeElement);
            }
            else
            {
                pDispNodeElement->InsertParent(pDispNode);
                pDispNodeElement->SetOwned(FALSE);
                pDispNodeElement->SetFiltered(FALSE);
                pDispNodeElement->SetAffectsScrollBounds(TRUE);
                pDispNodeElement->SetLayerType(DISPNODELAYER_FLOW);
            }
        }

        if (_pDispNode == pDispNodeElement)
        {
            _pDispNode = pDispNode;
        }
        pDispNodeElement     = pDispNode;
        _fHasCaptionDispNode = TRUE;

        hr = S_FALSE;
    }

    //
    //  Otherwise, if a CAPTION/TC node exists and is not needed, remove it
    //  (The display node which anchors the table cells is the only unowned
    //   node within the flow layer)
    //

    else if (   !_aryCaptions.Size()
            &&  _fHasCaptionDispNode)
    {
        pDispNodeTableOuter->ReplaceParent();

        pDispNodeElement =
        _pDispNode       = pDispNodeTableOuter;
        _pDispNode->SetOwned();
        _pDispNode->SetFiltered(pElement->HasFilterPtr());
        _pDispNode->SetAffectsScrollBounds(!ElementOwner()->IsRelative());

        _fHasCaptionDispNode = FALSE;

        //
        // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
        // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
        //
        EnsureTableFatHitTest( _pDispNode );

        hr = S_FALSE;
    }

    //
    //  If no display node for the cells exist or if an interesting property has changed, create a display node
    //

    if (    !pDispNodeTableOuter
        ||  fForce
        ||  dni.GetBorderType() != pDispNodeTableOuter->GetBorderType()
        ||  (   !_fHasCaptionDispNode
            &&  dni.HasUserClip() != pDispNodeTableOuter->HasUserClip())
        ||  dni.IsRTL()         != pDispNodeTableOuter->IsRightToLeft() 

        //
        // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
        // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
        //
        ||  GetFatHitTest()     != pDispNodeTableOuter->IsFatHitTest() )
    {
        BOOL    fHasUserClip;

        fHasUserClip = (    dni.HasUserClip()
                        &&  !_fHasCaptionDispNode);

        pDispNode = CDispRoot::CreateDispContainer(
                                    this,
                                    FALSE,
                                    fHasUserClip,
                                    FALSE,
                                    dni.GetBorderType(),
                                    dni.IsRTL());

        if (!pDispNode)
            goto Error;

        pDispNode->SetOwned(!_fHasCaptionDispNode);

        if (_fHasCaptionDispNode)
        {
            pDispNode->SetLayerType(DISPNODELAYER_FLOW);
        }
        else
        {
            EnsureDispNodeLayer(dni, pDispNode);
            pDispNode->SetFiltered(pElement->HasFilterPtr());
            pDispNode->SetAffectsScrollBounds(!ElementOwner()->IsRelative());
        }

        EnsureDispNodeBackground(dni, pDispNode);
        EnsureDispNodeVisibility(dni.GetVisibility(), ElementOwner(), pDispNode);

        //
        // BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
        // Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
        //
        EnsureTableFatHitTest( pDispNode );

        if (pDispNodeTableOuter)
        {
            pDispNode->ReplaceNode(pDispNodeTableOuter);
        }
        else if (_fHasCaptionDispNode)
        {
            Assert(pDispNodeElement);
            DYNCAST(CDispContainer, pDispNodeElement)->InsertChildInFlow(pDispNode);
        }

        if (    !_fHasCaptionDispNode
            &&  _pDispNode == pDispNodeElement)
        {
            _pDispNode = pDispNode;
        }

        hr = S_FALSE;
    }

    return hr;

Error:
    if (pDispNode)
    {
        pDispNode->Destroy();
    }

    if (pDispNodeElement)
    {
        pDispNodeElement->Destroy();
    }

    _pDispNode = NULL;
    _fHasCaptionDispNode = FALSE;
    return E_FAIL;
}


//+----------------------------------------------------------------------------
//
//  Member:     EnsureTableBorderDispNode
//
//  Synopsis:   Manage the lifetime of the table border display node during measuring
//
//  Arugments:  pdci   - Current CDocInfo
//              fForce - Forcibly update the display node(s)
//
//  Returns:    S_OK    if successful
//              S_FALSE if nodes were created/destroyed
//              E_FAIL  otherwise
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::EnsureTableBorderDispNode()
{
    CDispContainer * pDispNodeTableGrid;
    HRESULT hr = S_OK;

    Assert(_pDispNode);

    CElement*  pElement  = ElementOwner();
    CTreeNode* pTreeNode = pElement->GetFirstBranch();
    styleDir         dir = pTreeNode->GetCascadedBlockDirection();
    BOOL            fRTL = (dir == styleDirRightToLeft);

    //
    // Locate the display node that anchors all cells.  This will be the
    // parent of the border display node if one is needed.
    //

    pDispNodeTableGrid = GetTableOuterDispNode();

    Assert(pDispNodeTableGrid);

    if ((_fCollapse || _fRuleOrFrameAffectBorders) && GetRows())
    {
        CDispItemPlus * pDispItemNew = NULL;
        CDispItemPlus * pDispItemCurrent = NULL;
        CRect   rcClientRect;
        SIZE    size;


        if (!_pTableBorderRenderer)
        {
            _pTableBorderRenderer = new CTableBorderRenderer(this);
            if (!_pTableBorderRenderer)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
        }
        else
        {
            if(_pTableBorderRenderer->_pDispNode)
                pDispItemCurrent = DYNCAST(CDispItemPlus, _pTableBorderRenderer->_pDispNode);
        }

        // if we don't have a disp node, or are changing directions, create a new node
        if (!_pTableBorderRenderer->_pDispNode ||
            fRTL != _pTableBorderRenderer->_pDispNode->IsRightToLeft())
        {
            // Create display leaf node.
            pDispItemNew =
                CDispRoot::CreateDispItemPlus(_pTableBorderRenderer,
                                              FALSE,                // no extra cookie
                                              FALSE,                // no user clip
                                              FALSE,                // no inset
                                              DISPNODEBORDER_NONE,  // this border dispnode has no border of its own
                                              fRTL);               // not right to left

            if (!pDispItemNew)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }

            pDispItemNew->SetOwned(FALSE);
            // The border display node is in the flow layer (highest z-order among flow display nodes).
            pDispItemNew->SetLayerType(DISPNODELAYER_FLOW);

            if(pDispItemCurrent)
            {
                // we changed directions. Delete the old node.
                Assert(fRTL == pDispItemNew->IsRightToLeft());

                pDispItemCurrent->SetOwned(TRUE);

                pDispItemCurrent->Destroy();
            }

            pDispNodeTableGrid->InsertChildInFlow(pDispItemNew);

            _pTableBorderRenderer->_pDispNode = pDispItemNew;
        }
        else
        {
            pDispItemNew = DYNCAST(CDispItemPlus, _pTableBorderRenderer->_pDispNode);
            // Make sure border display node is last in list (highest z order among display node FLOW layers).

            // If we have a "next" (right) sibling, reinsert the border display node in the last position
            if (pDispItemNew->GetNextSiblingNode(TRUE))
            {
                pDispItemNew->ExtractFromTree();
                pDispNodeTableGrid->InsertChildInFlow(pDispItemNew);

                Assert(!pDispItemNew->GetNextSiblingNode(TRUE));
            }
        }

        Assert(pDispItemNew);

        // Make sure the border display node has the right size.
        // Size should always be adjusted for table borders since we
        // just finished a layout.
        pDispNodeTableGrid->GetClientRect(&rcClientRect, CLIENTRECT_CONTENT);
        rcClientRect.GetSize(&size);
        pDispItemNew->SetSize(size, FALSE);
    }
    else if (_pTableBorderRenderer)
    {
        CDispNode * pDispNode = _pTableBorderRenderer->_pDispNode;

        if (pDispNode)
        {
            pDispNode->Destroy();
        }

        _pTableBorderRenderer->Release();
        _pTableBorderRenderer = NULL;
    }

Cleanup:

    RRETURN(hr);
}


void
CTableLayout::EnsureContentVisibility(
    CDispNode * pDispNode,
    BOOL        fVisible)
{
    // take care of visibility of collapsed border display node
    if (    pDispNode == GetElementDispNode()
        &&  (   _fCollapse
            ||  _fRuleOrFrameAffectBorders)
        &&  _pTableBorderRenderer
        &&  _pTableBorderRenderer->_pDispNode)
    {
        _pTableBorderRenderer->_pDispNode->SetVisible(fVisible);
    }
}

//+====================================================================================
//
// Method: EnsureTableFatHitTest
//
// Synopsis: Ensure the FatHit Test bit on the DispNode is set appropriately. 
//           See bugbug below.
//
//------------------------------------------------------------------------------------

//
// BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
//

HRESULT             
CTableLayout::EnsureTableFatHitTest(CDispNode* pDispNode)
{
    pDispNode->SetFatHitTest( GetFatHitTest() );

    RRETURN( S_OK );
}

//+====================================================================================
//
// Method: GetFatHitTest
//
// Synopsis: Get whether this table layout requires "fat" hit testing.
//
//------------------------------------------------------------------------------------

//
// BUGBUG. FATHIT. marka - Fix for Bug 65015 - enabling "Fat" hit testing on tables.
// Edit team is to provide a better UI-level way of dealing with this problem for post IE5.
//

BOOL
CTableLayout::GetFatHitTest()
{
    return ( Doc()->_fDesignMode &&
             CellSpacingX() == 0 && 
             CellSpacingY() == 0 &&
             BorderX() <= 1 && 
             BorderY() <= 1 ) ;
}

//+----------------------------------------------------------------------------
//
//  Member:     GetTableInnerDispNode
//
//  Synopsis:   Find and return the CDispNode which contains TBODY cells
//
//-----------------------------------------------------------------------------
CDispContainer *
CTableLayout::GetTableInnerDispNode()
{
// BUGBUG: Not done yet...assume all THEAD/TFOOT/TBODY cells live under the same display node (brendand)
    return GetTableOuterDispNode();
}


//+----------------------------------------------------------------------------
//
//  Member:     GetTableOuterDispNode
//
//  Synopsis:   Find and return the CDispNode which contains THEAD/TFOOT cells
//
//-----------------------------------------------------------------------------
CDispContainer *
CTableLayout::GetTableOuterDispNode()
{
    CDispContainer *    pDispNodeTableOuter;

    if (_fHasCaptionDispNode)
    {
        CDispNode * pDispNode;

        Assert(GetElementDispNode());

        for (pDispNode = DYNCAST(CDispContainer, GetElementDispNode())->GetFirstChildNodeInLayer(DISPNODELAYER_FLOW);
             pDispNode && pDispNode->IsOwned();
             pDispNode = pDispNode->GetNextSiblingNode(TRUE));
        Assert(pDispNode);

        pDispNodeTableOuter = DYNCAST(CDispContainer, pDispNode);
    }
    else
    {
        pDispNodeTableOuter = DYNCAST(CDispContainer, GetElementDispNode());
    }

    return pDispNodeTableOuter;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetTableSize
//
//  Synopsis:   Return the current width/height of the table
//
//  Arguments:  psize - Pointer to CSize
//
//-----------------------------------------------------------------------------
void
CTableLayout::GetTableSize(
    CSize * psize)
{
    CDispNode * pDispNode;

    Assert(psize);

    pDispNode = GetTableOuterDispNode();

    if (pDispNode)
    {
        pDispNode->GetSize(psize);
    }
    else
    {
        *psize = g_Zero.size;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     SizeDispNode
//
//  Synopsis:   Adjust the size of the table layout display node
//
//  Arugments:  pci       - Current CCalcInfo
//              size      - Size including CAPTIONs
//              sizeTable - Size excluding CAPTIONs
//
//-----------------------------------------------------------------------------
void
CTableLayout::SizeTableDispNode(
    CCalcInfo *     pci,
    const SIZE &    size,
    const SIZE &    sizeTable)

{
    CElement *          pElement = ElementOwner();
    CDispContainer *    pDispNodeTableOuter;
    CDispNode *         pDispNodeElement;
    BOOL                fInvalidateAll;
    DISPNODEBORDER      dnb;
    CSize               sizeOriginal;
    CDoc *              pDoc;

    Assert(pci);

    if (!_pDispNode)
        goto Cleanup;

    //
    //  Locate the display node that anchors all cells
    //  (If a separate CAPTIONs display node exists, the display node for cells
    //   will be the only unowned node in the flow layer)
    //

    pDispNodeElement    = GetElementDispNode();
    pDispNodeTableOuter = GetTableOuterDispNode();

    //
    // Invalidate the entire table area if its size has changed.
    //

    pDispNodeElement->GetSize(&sizeOriginal);

    fInvalidateAll = sizeOriginal != sizeTable;

    //
    //  Set the border size (if any)
    //  NOTE: These are set before the size because a change in border widths
    //        forces a full invalidation of the display node. If a full
    //        invalidation is necessary, less code is executed when the
    //        display node's size is set.
    //

    dnb            = pDispNodeTableOuter->GetBorderType();
    pDoc           = Doc();

    if (dnb != DISPNODEBORDER_NONE)
    {
        CRect       rcBorderWidths;
        CRect       rc;
        CBorderInfo bi;

        pDispNodeTableOuter->GetBorderWidths(&rcBorderWidths);

        pElement->GetBorderInfo(pci, &bi, FALSE);

        rc.left   = pci->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT]);
        rc.top    = pci->DocPixelsFromWindowY(bi.aiWidths[BORDER_TOP]);
        rc.right  = pci->DocPixelsFromWindowX(bi.aiWidths[BORDER_RIGHT]);
        rc.bottom = pci->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM]);

        if (rc != rcBorderWidths)
        {
            if (dnb == DISPNODEBORDER_SIMPLE)
            {
                pDispNodeTableOuter->SetBorderWidths(rc.top);
            }
            else
            {
                pDispNodeTableOuter->SetBorderWidths(rc);
            }

            fInvalidateAll = TRUE;
        }
    }

    //
    //  Determine if a full invalidation is necessary
    //  (A full invalidation is necessary only when there is a fixed
    //   background located at a percentage of the width/height)
    //

    if (    !fInvalidateAll
        &&  pDispNodeTableOuter->HasBackground()
        &&  pDispNodeTableOuter->IsScroller()
        &&  pDispNodeTableOuter->HasFixedBackground())
    {
        const CFancyFormat *    pFF = GetFirstBranch()->GetFancyFormat();

        fInvalidateAll =    pFF->_lImgCtxCookie
                    &&  (   pFF->_cuvBgPosX.GetUnitType() == CUnitValue::UNIT_PERCENT
                        ||  pFF->_cuvBgPosY.GetUnitType() == CUnitValue::UNIT_PERCENT);
    }

    //
    //  Size the table node
    //

    pDispNodeTableOuter->SetSize(sizeTable, fInvalidateAll);

    //
    //  Finally, if CAPTIONs exist, size that node as well
    //

    if (_fHasCaptionDispNode)
    {
        pDispNodeElement->SetSize(size, fInvalidateAll);
    }

    //
    //  If the display node has an explicit user clip, size it
    //

    if (pDispNodeElement->HasUserClip())
    {
        SizeDispNodeUserClip(pci, size, pDispNodeElement);
    }

    //
    //  Fire related events
    //

    if (    (CSize &)size != sizeOriginal
        &&  !IsDisplayNone()
        &&  pDoc->_state >= OS_INPLACE
        &&  pDoc->_fFiredOnLoad)
    {
        pDoc->GetView()->AddEventTask(pElement, DISPID_EVMETH_ONRESIZE);
    }

    if (pElement->ShouldFireEvents())
    {
        if (size.cx != sizeOriginal.cx)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETWIDTH);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTWIDTH);
        }

        if (size.cy != sizeOriginal.cy)
        {
            pElement->FireOnChanged(DISPID_IHTMLELEMENT_OFFSETHEIGHT);
            pElement->FireOnChanged(DISPID_IHTMLELEMENT2_CLIENTHEIGHT);
        }
    }

Cleanup:
    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     DestroyFlowDispNodes
//
//  Synopsis:   Destroy any created flow nodes
//
//-----------------------------------------------------------------------------
void
CTableLayout::DestroyFlowDispNodes()
{
}
