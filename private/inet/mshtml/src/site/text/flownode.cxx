//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       flownode.cxx
//
//  Contents:   Routines for managing display tree nodes
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X__DISP_H_
#define X__DISP_H_
#include "_disp.h"
#endif

#ifndef X_LSRENDER_HXX_
#define X_LSRENDER_HXX_
#include "lsrender.hxx"
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

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DEBUGPAINT_HXX_
#define X_DEBUGPAINT_HXX_
#include "debugpaint.hxx"
#endif

extern ZERO_STRUCTS g_Zero;


//+----------------------------------------------------------------------------
//
//  Member:     AddLayoutDispNode
//
//  Synopsis:   Add, as a sibling of the passed display node,
//              the display node of passed layout
//
//  Arguments:  pTreeNode    - CTreeNode from which to obtain the layout
//              pLayout      - Layout whose display node is to be added
//              dx, dy       - Left/top offset to set on the node
//              pDispSibling - CDispNode that is the left-hand sibling
//
//  Returns:    Node to be used as the next sibling if successful, NULL otherwise
//
//-----------------------------------------------------------------------------

CDispNode *
CDisplay::AddLayoutDispNode(
    CParentInfo *   ppi,
    CLayout *       pLayout,
    long            dx,
    long            dy,
    CDispNode *     pDispSibling)
{
    CDispNode   * pDispNode;
    CFlowLayout * pFL = GetFlowLayout();

    Assert(pLayout);
    Assert(!pLayout->IsDisplayNone());

    pDispNode = pLayout->GetElementDispNode();

    Assert(!pDispNode || pDispSibling != pDispNode);

    //
    //  Insert the node if it exists
    //  (Nodes will not exist for unmeasured elements, such as hidden INPUTs or
    //   layouts which have display set to none)
    //

    if (pDispNode)
    {
        Assert(pDispNode->GetLayerType() == DISPNODELAYER_FLOW);

        //
        // If no sibling was provided, insert directly under the content containing node
        //

        if (!pDispSibling)
        {
            //
            //  Ensure the display node can contain children
            //

            if (!pFL->EnsureDispNodeIsContainer())
                goto Cleanup;

            pDispSibling = pFL->GetFirstContentDispNode();

            if (!pDispSibling)
                goto Cleanup;
        }

        pDispSibling->InsertSiblingNode(pDispNode);

        //
        //  Position the node
        //

        long xPos;
        if(!_fRTL)
            xPos = dx + pLayout->GetXProposed();
        else
        {
            // we need to set the top left corner.
            CSize size;
            pLayout->GetSize(&size);

            xPos = dx - pLayout->GetXProposed() - size.cx;
        }

        pLayout->SetPosition(CPoint(xPos, dy + pLayout->GetYProposed()), TRUE);

        pDispSibling = pDispNode;
    }
Cleanup:
    return pDispSibling;
}


//+----------------------------------------------------------------------------
//
//  Member:     GetPreviousDispNode
//
//  Synopsis:   Given a cp, find the display node just before that which
//              would contain the cp
//
//  Arguments:  cp - cp for searching
//
//  Returns:    Previous CDispNode (if found), NULL otherwise
//
//-----------------------------------------------------------------------------

CDispNode *
CDisplay::GetPreviousDispNode(
    long    cp, long iLineStart)
{
    CFlowLayout * pFlowLayout       = GetFlowLayout();
    CDispNode   * pDispNodeOwner    = pFlowLayout->GetElementDispNode();
    CDispNode   * pDispNodeSibling  = NULL;


    Assert(pDispNodeOwner);

    if(pDispNodeOwner->IsContainer())
    {
        CDispNode * pDispNode = pFlowLayout->GetFirstContentDispNode();
        void *      pvOwner;
        CElement *  pElement;

        Assert(pDispNode);

        pDispNodeSibling = pDispNode;

        //
        // Since, the first node is the flownode, we can just skip it.
        //

        pDispNode = pDispNode->GetNextSiblingNode(TRUE);

        while(pDispNode)
        {
            //
            // if the disp node corresponds to the text flow,
            // the cookie stores the line index from where the
            // current text flow node starts.
            //
            if (pDispNode->GetDispClient() == pFlowLayout)
            {
                if(iLineStart <= (LONG)(LONG_PTR)pDispNode->GetExtraCookie())
                    break;
            }
            else
            {
                pDispNode->GetDispClient()->GetOwner(pDispNode, &pvOwner);

                if (pvOwner)
                {
                    pElement = DYNCAST(CElement, (CElement *)pvOwner);
                    if(pElement->GetFirstCp() >= cp)
                        break;
                }
            }

            pDispNodeSibling = pDispNode;
            pDispNode = pDispNode->GetNextSiblingNode(TRUE);
        }
    }

    return pDispNodeSibling;
}


//+----------------------------------------------------------------------------
//
//  Member:     AdjustDispNodes
//
//  Synopsis:   Extract or translate all display nodes followwing the passed node
//
//  Arguments:  pDispNode - Left-hand sibling of starting display node
//              pled      - Current CLed (may be NULL)
//
//-----------------------------------------------------------------------------

void
CDisplay::AdjustDispNodes(
    CDispNode * pDNAdjustFrom,
    CLed *      pled)
{
    CDispNode * pDispNodeOwner = GetFlowLayout()->GetElementDispNode();

    if (pDispNodeOwner && pDispNodeOwner->IsContainer())
    {
        if (!pDNAdjustFrom)
            pDNAdjustFrom = GetFlowLayout()->GetFirstContentDispNode();

        if (pDNAdjustFrom)
            pDNAdjustFrom = pDNAdjustFrom->GetNextSiblingNode(TRUE);

        if (pDNAdjustFrom)
        {
            if (    !pled
                ||  pled->_iliMatchNew == MAXLONG)
            {
                GetFlowLayout()->ExtractDispNodes(pDNAdjustFrom);
            }
            else
            {
                //
                // Update the cookie on text disp nodes and destroy
                // any that lie in the dirty line's range
                if (_fHasMultipleTextNodes)
                {
                    CDispNode * pDispNode = pDNAdjustFrom;

                    while (pDispNode)
                    {
                        CDispNode * pDispNodeCur = pDispNode;

                        pDispNode = pDispNode->GetNextSiblingNode(TRUE);

                        if (pDispNodeCur->GetDispClient() == GetFlowLayout())
                        {
                            long iLine = (LONG)(LONG_PTR)pDispNodeCur->GetExtraCookie();

                            if (iLine < pled->_iliMatchOld)
                            {
                                Assert(!pDispNodeCur->IsOwned());

                                if (pDNAdjustFrom == pDispNodeCur)
                                {
                                    pDNAdjustFrom = pDispNode;
                                }

                                //
                                // Extract the disp node and destroy it
                                //
                                pDispNodeCur->ExtractFromTree();
                                pDispNodeCur->Destroy();
                            }
                            else
                            {
                                pDispNodeCur->SetExtraCookie(
                                                (void *)(LONG_PTR)(iLine +
                                                pled->_iliMatchNew -
                                                pled->_iliMatchOld));
                            }
                        }
                    }
                }

                if (pDNAdjustFrom)
                {
                    GetFlowLayout()->TranslateDispNodes(
                                        CSize(0, pled->_yMatchNew - pled->_yMatchOld),
                                        pDNAdjustFrom,
                                        NULL,       // dispnode to stop at
                                        TRUE,       // restrict to layer
                                        TRUE);      // extract hidden
                }
            }
        }
    }
    else
    {
        Assert(!pDNAdjustFrom);
    }
}


//+----------------------------------------------------------------------------
//
//  Member:     DestroyFlowDispNodes
//
//  Synopsis:   Destroy all display tree nodes created for the flow layer
//
//-----------------------------------------------------------------------------

void
CDisplay::DestroyFlowDispNodes()
{
}
