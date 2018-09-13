//+----------------------------------------------------------------------------
//
// File:        synchelp.CXX
//
// Contents:    Implementation of CQDocGlue and related classes
//
// Copyright (c) 1998 Microsoft Corporation. All rights reserved.
//
// @doc INTERNAL
//-----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X__LINE_H_
#define X__LINE_H_
#include "_line.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_IID_TREESYNC_H_
#define X_IID_TREESYNC_H_
#include "iid_treesync.h"
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_QUILGLUE_HXX_
#define X_QUILGLUE_HXX_
#include "quilglue.hxx"
#endif

#ifndef X_QDOCGLUE_HXX_
#define X_QDOCGLUE_HXX_
#include "qdocglue.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_SYNCBUF_HXX_
#define X_SYNCBUF_HXX_
#include "syncbuf.hxx"
#endif

#ifndef X_ITREESYNC_H_
#define X_ITREESYNC_H_
#include "itreesync.h"
#endif

#ifndef X_EBODY_HXX_
#define X_EBODY_HXX_
#include "ebody.hxx"
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include "_txtsave.h"
#endif


#if DBG==1
    extern BOOL g_fEnableTreeSyncDebugOutput;
    extern void DbgStr(const char * sz);
    extern void DbgStrW(const WCHAR * wz);
    extern void DbgInt(const char * sz, long l);
    extern void DbgDumpTree(char * szLabel, CElement * pElemRoot);
    extern void DumpTwoTrees(char * szLabel, CElement * pElementSyncRoot);
#else
    #define DbgStr(x) ((void)0)
    #define DbgStrW(x) ((void)0)
    #define DbgInt(x,y) ((void)0)
    #define DbgDumpTree(x,y) ((void)0)
    #define DumpTwoTrees(x,y) ((void)0)
#endif


////////////////////////////////////////////////////////////////
//    ITreeSyncServices helper functions

HRESULT
CQDocGlue::GetSyncInfoFromTreePos(CTreePos * ptp,
                             CElement ** pElementSyncRoot,
                             long *pcpRelative)
{
    HRESULT hr = S_OK;
    long cpBase;
    CElement *pThisElement = NULL;
    CElement *pFirstElement = NULL;
    CTreeNode   *pNode;
    ITreeSyncBehavior *pTreeSyncBehavior = NULL;
    CTreePos *ptpReal;
    IHTMLElement *pIThisElement = NULL;

#if DBG==1
    long dbg_cp;
    dbg_cp = ptp->GetCp();
#endif

    //  If the tree pos is at the begin/end of a sync node, then
    //  we should be using the sync point above it.
    //  $REVIEW (chrisfra) - is this legal?
    for (ptpReal = ptp; ptpReal->IsPointer(); ptpReal = ptpReal->NextTreePos())
    {
        /* NULL STMT*/;
    }
    if (ptpReal && ptpReal->IsNode())
    {
        pNode = ptpReal->GetBranch();
        if (pNode)
        {
            pFirstElement = pNode->Element();
        }
    }

    //  Search upwards for a sync root - if *pElementSyncRoot is
    //  NOT NULL, then it had better be == *pElementSyncRoot
    for (pNode = ptp->GetBranch(); pNode; pNode = pNode->Parent())
    {
        pThisElement = pNode->Element();
        ClearInterface(&pIThisElement);
        hr = THR(pThisElement->QueryInterface(IID_IHTMLElement,(void**)&pIThisElement));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
        ClearInterface(&pTreeSyncBehavior);
        if (pThisElement->Tag() == ETAG_ROOT ||
            S_OK == GetBindBehavior(pIThisElement,&pTreeSyncBehavior))
        {
            // if the element we see with xtag/behavior is the first one we
            // look at, that means ptp is just before either the start or
            // the end of the behavior.  if it's just before the start, then
            // we don't want to use it, since we should instead use the
            // enclosing region, because we aren't actually in this one
            // we see (due to the forward-scan loop at the top of this function).
            // But if it's the end node, then we are inside, so
            if (ptpReal->IsEndNode())
            {
                // end node, always inside, so don't check to see if this
                // is first element
                break;
            }
            if (pThisElement != pFirstElement)
            {
                // start node.  if it's the first one we look at, then don't
                // break, since we aren't really inside
                break;
            }
            // didn't break, so it was the first one.  keep looking
        }
    }

    if (pTreeSyncBehavior)
    {
        //  Enforce common direct sync root
        if (*pElementSyncRoot && *pElementSyncRoot != pThisElement)
        {
            goto Cleanup;
        }
        hr = THR(GetSyncBaseIndex(pThisElement, &cpBase));
        if (S_OK != hr)
        {
            goto Cleanup;
        }
        *pcpRelative = ptp->GetCp() - cpBase;
        *pElementSyncRoot = pThisElement;
    }

Cleanup:
    if (!hr && NULL == pTreeSyncBehavior)
    {
        hr = E_FAIL;
    }
    //Assert(hr == S_OK || (pThisElement ? (DbgDumpTree("GetSyncInfoFromTreePos Fault",pThisElement),0) : 0));
    ReleaseInterface(pTreeSyncBehavior);
    ReleaseInterface(pIThisElement);
    RRETURN(hr);    
}

HRESULT 
CQDocGlue::GetSyncInfoFromElement(IHTMLElement * pIElement,
                             CElement ** pElementSyncRoot,
                             long * pcpRelativeStart,
                             long * pcpRelativeEnd)
{
    HRESULT hr = S_OK;
    CElement *pElement = NULL;
    CTreePos *ptpStart;
    CTreePos *ptpEnd;

    // check argument sanity
    if (NULL==pcpRelativeStart || NULL==pcpRelativeEnd || NULL==pIElement  || !_pDoc->IsOwnerOf(pIElement))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pcpRelativeStart = 0;
    *pcpRelativeEnd = 0;

    // get the internal object corresponding to the argument
    hr = THR(pIElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    //  get corresponding treepos for start and end of element's tree and get cp from that
    pElement->GetTreeExtent(&ptpStart, &ptpEnd);
    hr = THR(GetSyncInfoFromTreePos(ptpStart, pElementSyncRoot, pcpRelativeStart));
    if (S_OK != hr)
    {
        goto Cleanup;
    }
    hr = THR(GetSyncInfoFromTreePos(ptpEnd, pElementSyncRoot, pcpRelativeEnd));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);    
}

HRESULT 
CQDocGlue::GetSyncInfoFromPointer(IMarkupPointer * pIPointer,
                             CElement ** pElementSyncRoot,
                             long * pcpRelative)
{
    HRESULT hr = S_OK;
    CMarkupPointer *pPointer = NULL;
    CTreePos *ptp;

    // check argument sanity
    if (NULL==pcpRelative || pIPointer==NULL  || !_pDoc->IsOwnerOf(pIPointer))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pcpRelative = 0;

    // get the internal object corresponding to the argument
    hr = THR( pIPointer->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointer) );
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //  get corresponding treepos and get cp from that
    ptp = pPointer->TreePos();
    if (!ptp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(GetSyncInfoFromTreePos(ptp, pElementSyncRoot, pcpRelative));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);    
}

HRESULT 
CQDocGlue::GetSyncBaseIndex(CElement * pElementSyncRoot,
                       long * pcpRelative)
{
    CTreePos *ptpStart;
    pElementSyncRoot->GetTreeExtent(&ptpStart, NULL);
    *pcpRelative = ptpStart->GetCp();
    return S_OK;
}


////////////////////////////////////////////////////////////////
//    ITreeSyncServices methods

HRESULT 
CQDocGlue::GetBindBehavior(IHTMLElement * pElemTreeSyncRoot, 
                      ITreeSyncBehavior ** ppTreeSyncRoot)
{
    HRESULT                             hr;
    CElement *                          pElementRoot;
    CPeerHolder::CPeerHolderIterator    itr;

    hr = THR(pElemTreeSyncRoot->QueryInterface( CLSID_CElement, (void **) & pElementRoot ) );
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    // hack(tomlaw): should be on the element (peerelem.cxx and called GetBindPeer)
    // hr = pElementRoot->GetBindPeer(ppTreeSyncRoot);
    // instead:
    //
    // BUGBUG (alexz)
    // (1) we should get rid of this, as we don't give out peer interfaces for various reasons -
    //     with the main one being possibility of refcount loops
    // (2) in the worst case, this should use CElement::QueryInterface, that can give out a few
    //     selected interfaces
    // (3) please review any peer / peer holder code with alexz or anandra - thanks!
    //
    {
        hr = E_FAIL;

        *ppTreeSyncRoot = NULL;

        //
        // search all peers for one that implements ITreeSyncBehavior
        //

        for (itr.Start(pElementRoot->GetPeerHolderPtr()); !itr.IsEnd(); itr.Step())
        {
            if (S_OK == itr.PH()->QueryPeerInterface(IID_ITreeSyncBehavior, (LPVOID *) ppTreeSyncRoot))
            {
                hr = S_OK;
                break;
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CQDocGlue::MoveSyncInfoToPointer(IMarkupPointer * pIPointer,
                            IHTMLElement ** pElemTreeSyncRoot,
                            long * pcpRelative)
{
    HRESULT hr;
    CElement *pElementRoot = NULL;

    *pElemTreeSyncRoot = NULL;
    *pcpRelative = 0;

    hr = THR(GetSyncInfoFromPointer(pIPointer, &pElementRoot, pcpRelative) );
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pElementRoot->QueryInterface(IID_IHTMLElement, (LPVOID *) pElemTreeSyncRoot) );
    if (S_OK != hr)
    {
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

HRESULT 
CQDocGlue::MovePointerToSyncInfo(IMarkupPointer * pIPointer,
                            IHTMLElement * pElemTreeSyncRoot,
                            long cpRelative)
{
    HRESULT hr;
    CElement *pElementRoot;
    long cpBase;

    hr = THR(pElemTreeSyncRoot->QueryInterface( CLSID_CElement, (void **) & pElementRoot ) );
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    GetSyncBaseIndex(pElementRoot, &cpBase);

    hr = MovePointerToCPos(pIPointer, cpRelative + cpBase);

Cleanup:
    RRETURN(hr);
}


HRESULT
CQDocGlue::MovePointerToCPos(IMarkupPointer *pIPointer, long cp)
{
    HRESULT hr = S_OK;
    CMarkupPointer *pPointer = NULL;
    CTreePos *ptp = NULL;
    CMarkup * pMarkup;
    CBodyElement *pBody;

    // check argument sanity
    if (pIPointer==NULL  || !_pDoc->IsOwnerOf(pIPointer))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // get the internal object corresponding to the argument
    hr = THR( pIPointer->QueryInterface(CLSID_CMarkupPointer, (void**)&pPointer) );
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //  get treeposlist from body, so that pointer doesn't need to be
    //  positioned for this to work
    hr = THR( _pDoc->GetBodyElement(&pBody) );
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    pBody->GetTreeExtent(&ptp, NULL);
    if (!ptp)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //  get the treeposlist
    pMarkup = ptp->GetMarkup();
    if (pMarkup==NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    //  move pointer to cp
    hr = THR(pPointer->MoveToCp(cp, pMarkup) );

Cleanup:
    RRETURN(hr);    
}


HRESULT
CQDocGlue::MoveSyncInfoToElement(
    IHTMLElement * pElement,
    IHTMLElement ** pElemTreeSyncRoot,
	long * pcpRelative)
{
    HRESULT         hr;
    CTreePos *      ptpStart;
    CElement *      pEl;
    CElement *      pElSyncRoot = NULL;

    if ((pElement == NULL) || (pElemTreeSyncRoot == NULL) || (pcpRelative == NULL))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pElemTreeSyncRoot = NULL;
    *pcpRelative = 0;

    hr = THR(pElement->QueryInterface(CLSID_CElement,(void**)&pEl));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    if (pEl->Doc() != _pDoc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pEl->GetTreeExtent(&ptpStart,NULL);

    hr = THR(GetSyncInfoFromTreePos(ptpStart,&pElSyncRoot,pcpRelative));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pElSyncRoot->QueryInterface(IID_IHTMLElement,(void**)pElemTreeSyncRoot));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CQDocGlue::GetElementFromSyncInfo(
    IHTMLElement ** pElement,
    IHTMLElement * pElemTreeSyncRoot,
	long cpRelative)
{
    HRESULT             hr;
    CElement *          pEl;
    CElement *          pElSyncRoot;
    IMarkupPointer *    pIPointer = NULL;
    long                cpBaseIndex;
    CMarkupPointer *    pPointer;
    CTreePos *          ptpScan;

    if ((pElement == NULL) || (pElemTreeSyncRoot == NULL))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pElement = NULL;

    hr = THR(pElemTreeSyncRoot->QueryInterface(CLSID_CElement,(void**)&pElSyncRoot));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    if (pElSyncRoot->Doc() != _pDoc)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR(GetSyncBaseIndex(pElSyncRoot,&cpBaseIndex));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(_pDoc->CreateMarkupPointer(&pIPointer));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(MovePointerToCPos(pIPointer,cpBaseIndex + cpRelative));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    hr = THR(pIPointer->QueryInterface(CLSID_CMarkupPointer,(void**)&pPointer));
    if (S_OK != hr)
    {
        goto Cleanup;
    }

    ptpScan = pPointer->TreePos();
    Assert(ptpScan != NULL);
    while (!ptpScan->IsNode())
    {
        ptpScan = ptpScan->NextTreePos();
        Assert(ptpScan != NULL);
    }
    Assert(ptpScan->IsBeginNode());

    pEl = ptpScan->Branch()->Element();

    hr = THR(pEl->QueryInterface(IID_IHTMLElement,(void**)pElement));
    if (S_OK != hr)
    {
        goto Cleanup;
    }


Cleanup:

    if (pIPointer != NULL)
    {
        pIPointer->Unposition();
        ReleaseInterface(pIPointer);
    }

    RRETURN(hr);
}
