//+---------------------------------------------------------------------
//
//  File:       brptr.cxx
//
//  Contents:   CTreeNode class implementation
//
//  Classes:    CTreeNode
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include "csite.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_MISCPROT_H_
#define X_MISCPROT_H_
#include "miscprot.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

MtDefine(CTreeNode, Tree, "CTreeNode")
MtDefine(CTreeNodeCLock, Locals, "CTreeNode::CLock")

#if DBG == 1 || defined(DUMPTREE)
int CTreeNode::s_NextSerialNumber = 0;
int s_nSNTrace = -1;
#endif


CTreeNode::CTreeNode ( CTreeNode * pParent, CElement * pElement /* = NULL */ )
#if DBG == 1 || defined(DUMPTREE)
            : _nSerialNumber( s_NextSerialNumber++ )
#endif
{
    Assert( _pNodeParent == NULL);
    Assert( _pElement == NULL);

    _iFF = _iCF = _iPF = -1;

    SetElement( pElement );
    SetParent( pParent );

    Assert( pElement && pElement->Doc() && pElement->Doc()->AreLookasidesClear( this, LOOKASIDE_NODE_NUMBER ) );
}

#if DBG == 1
CTreeNode::~CTreeNode()
{
    Assert( _fInDestructor );
    Assert( IsDead() );
    Assert( _iCF == -1 && _iPF == -1 && _iFF == -1 );
    Assert( !_pNodeParent );
    Assert( _iCF == -1 && _iPF == -1 && _iFF == -1 );
    Assert( !HasPrimaryTearoff() );
    Assert( !HasCurrentStyle() );
}
#endif

CTreeNode *
CTreeNode::NextBranch()
{
    Assert( IsInMarkup() );

    CTreePos * ptpCurr = GetEndPos();

    if( ptpCurr->IsEdgeScope() )
        return NULL;
    else
    {
        CElement *  pElement = Element();
        CTreeNode * pNodeCurr;

        do
        {
            ptpCurr = ptpCurr->NextTreePos();

            Assert( ptpCurr->IsNode() );

            pNodeCurr = ptpCurr->Branch();
        }
        while( pNodeCurr->Element() != pElement );

        return pNodeCurr;
    }
}

CTreeNode *
CTreeNode::PreviousBranch()
{
    Assert( IsInMarkup() );

    CTreePos * ptpCurr = GetBeginPos();

    if( ptpCurr->IsEdgeScope() )
        return NULL;
    else
    {
        CElement *  pElement = Element();
        CTreeNode * pNodeCurr;

        do
        {
            ptpCurr = ptpCurr->PreviousTreePos();

            Assert( ptpCurr->IsNode() );

            pNodeCurr = ptpCurr->Branch();
        }
        while( pNodeCurr->Element() != pElement );

        return pNodeCurr;
    }
}

CTreeNode *
CTreeNode::Ancestor (ELEMENT_TAG etag)
{
    CTreeNode * context = this;

    while (context && context->Tag() != etag)
        context = context->Parent();

    return context;
}

CTreeNode *
CTreeNode::Ancestor (ELEMENT_TAG *arytag)
{
    CTreeNode * context = this;
    ELEMENT_TAG etag;
    ELEMENT_TAG *petag;

    while (context)
    {
        etag = context->Tag();

        for (petag = arytag; *petag; petag++)
        {
            if (etag == *petag)
                return context;
        }

        context = context->Parent();
    }

    return context; // NULL context
}

CTreeNode *
CTreeNode::GetContainerBranch()
{
    CTreeNode *pNode = this;

    for ( ; pNode ; pNode = pNode->Parent() )
        if (pNode->IsContainer())
            break;

    return pNode;
}

//+---------------------------------------------------------------------------
//
// CTreeNode::RenderParent()
// CTreeNode::ZParent()
// CTreeNode::ClipParent()
//
// Parent accessor methods used for positioning support.
//
// The following chart defines the different parents that are used for
// positioning.  Each parent determines different parameters for any
// relatively positioned or absolutely positioned element.
//
//     PARENT        RELATIVE            ABSOLUTE               PARENT TYPE       USED IN
//     ------        --------            --------               -----------       -------
//
//  "ElementParent"  Coordinates       Not meaningful             Element         GetRenderPosition/PositionObjects (implicit)
//
//  "ParentSite"           Percent/Auto/CalcSize (both)             Site          Measuring (implicit)
//
//  "RenderParent"                Painting (both)                   Site          GetSiteDrawList/HitTestPoint
//
//  "ZParent"        Z-Order            Z-Order/Coordinates    Element or Site    GetElementsInZOrder/GetRenderPosition
//
//  "Clip Parent"  Clip Rect/Auto/SetPos   Clip Rect/SetPos     Absolute Site     SetPosition/PositionObjects
//
//
//  If the ZParent is a site, then it is the same as the RenderParent.
//
//----------------------------------------------------------------------------

CTreeNode *
CTreeNode::ZParentBranch()
{
    CTreeNode * pNode;

    if (Element()->IsPositionStatic() && !GetCharFormat()->_fRelative)
    {
        return GetUpdatedParentLayoutNode();
    }

    pNode = this;
    if (pNode)
    {
        pNode = pNode->Parent();
    }
    for (;
         pNode;
         pNode = pNode->Parent())
    {
        if(pNode->Element()->IsZParent())
        {
            break;
        }
    }

    //
    // If pNode is NULL then 'this' is the BODY or this node is not parented
    //   into the main document tree.  Return the body branch for the first
    //   case and NULL for the second.
    //

    return pNode
             ? pNode
             : ((Tag() == ETAG_BODY)
                  ? this
                  : NULL);
}

CTreeNode *
CTreeNode::RenderParentBranch()
{
    CTreeNode * pNode;

    if (Element()->IsPositionStatic())
    {
        return GetUpdatedParentLayoutNode();
    }


    pNode = this;
    if (pNode)
    {
        pNode = pNode->Parent();
    }
    for (;
         pNode;
         pNode = pNode->Parent())
    {
        if (pNode->NeedsLayout() && pNode->Element()->IsZParent())
        {
            break;
        }
    }

    return pNode
             ? pNode
             : ((Tag() == ETAG_BODY)
                  ? this
                  : NULL);
}

CTreeNode *
CTreeNode::ClipParentBranch()
{
    CTreeNode * pNode;

    if (Element()->IsPositionStatic())
    {
        return GetUpdatedParentLayoutNode();
    }

    pNode = this;
    if (pNode)
    {
        pNode = pNode->Parent();
    }
    for (;
         pNode;
         pNode = pNode->Parent())
    {
        if (pNode->NeedsLayout() && pNode->Element()->IsClipParent())
        {
            break;
        }
    }

    Assert(pNode);

    return pNode;
}

CTreeNode *
CTreeNode::ScrollingParentBranch()
{
    CTreeNode * pNode = NULL;

    if (Parent())
    {
        pNode = GetUpdatedParentLayoutNode();

        for (;
             pNode;
             pNode = pNode->GetUpdatedParentLayoutNode())
        {
            if (pNode->Element()->IsScrollingParent())
                break;
        }
    }

    return pNode;
}


//+----------------------------------------------------------------------------
//
// Member:      GetCascadedbackgroundColor
//
// Synopsis:    Return the background color of the element
//
//-----------------------------------------------------------------------------

CColorValue
CTreeNode::GetCascadedbackgroundColor()
{
    return (CColorValue) CTreeNode::GetFancyFormat()->_ccvBackColor;
}


// The following function is used by CTxtRange::GetExtendedSelectionInfo

CTreeNode *
CTreeNode::SearchBranchForPureBlockElement ( CFlowLayout * pFlowLayout )
{
    return pFlowLayout->GetContentMarkup()->SearchBranchForBlockElement( this, pFlowLayout );
}

//+----------------------------------------------------------------------------
//
//  Member:     CTreeNode::SearchBranchToFlowLayoutForTag
//
//  Synopsis:   Looks up the parent chain for the first element which
//              matches the tag.  No stopper element here, but stops
//              at the first text site it encounters.
//
//-----------------------------------------------------------------------------

CTreeNode *
CTreeNode::SearchBranchToFlowLayoutForTag ( ELEMENT_TAG etag )
{
    CTreeNode * pNode = this;

    do
    {
        if (pNode->Tag() == etag)
            return pNode;

        pNode = pNode->Parent();
    }
    while (pNode && !pNode->HasFlowLayout());

    return NULL;
}

//+-----------------------------------------------------
//
//  Member  : GetFontHeightInTwips
//
//  Sysnopsis : helper function that returns the base font
//          height in twips. by default this will return 1
//
//          for now only EMs are wired up to the point that
//          it makes sense to pass through fontsize.
//--------------------------------------------------------

long
CTreeNode::GetFontHeightInTwips(CUnitValue *pCuv)
{
    long  lFontHeight = 1;

    if (pCuv->GetUnitType()==CUnitValue::UNIT_EM ||
        pCuv->GetUnitType()==CUnitValue::UNIT_EX)
    {
        const CCharFormat * pCF = GetCharFormat();
        if (pCF)
        {
            lFontHeight = pCF->GetHeightInTwips(Doc());
        }
    }

    return lFontHeight;
}

//+----------------------------------------------------------------------------
//
//  Member:     CTreeNode::SearchBranchToRootForScope
//
//  Synopsis:   Looks up the parent chain for the first element which
//              has the same scope as the given element.  Will not stop
//              until there is no parent.
//
//-----------------------------------------------------------------------------

CTreeNode *
CTreeNode::SearchBranchToRootForScope( CElement * pElementFindMe )
{
    CTreeNode * pNode = this;

    do
    {
        if (pNode->Element() == pElementFindMe)
            return pNode;
    }
    while ( (pNode = pNode->Parent()) != NULL );

    return NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     CTreeNode::SearchBranchToRootForNode
//
//  Synopsis:   Looks up the parent chain for the given element. Will not stop
//              until there is no parent.
//
//-----------------------------------------------------------------------------

BOOL
CTreeNode::SearchBranchToRootForNode( CTreeNode * pNodeFindMe )
{
    CTreeNode * pNode = this;

    do
    {
        if (pNode == pNodeFindMe)
            return TRUE;
    }
    while ( (pNode = pNode->Parent()) != NULL );

    return FALSE;
}
//+----------------------------------------------------------------------------
//
//  Member:     CElement::SearchBranchToRootForTag
//
//  Synopsis:   Looks up the parent chain for the first element which
//              matches the tag.  No stopper element here, goes all the
//              way up to the <HTML> tag.
//
//-----------------------------------------------------------------------------

CTreeNode *
CTreeNode::SearchBranchToRootForTag ( ELEMENT_TAG etag )
{
    CTreeNode * pNode = this;

    do
    {
        if (pNode->Tag() == etag)
            return pNode;
    }
    while ( (pNode = pNode->Parent()) != NULL );

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTreeNode::GetFirstCommonAncestor, public
//
//  Synopsis:   Returns the first branch whose element is common to
//              both of the branches specified.
//
//  Arguments:  [pNode] -- branch to find common parent of with [this]
//              [pEltStop]  -- Stop walking tree if you hit this element.
//                             If NULL then search to root.
//
//  Returns:    Branch with common element from first starting point.
//
//----------------------------------------------------------------------------

CTreeNode *
CTreeNode::GetFirstCommonAncestor(CTreeNode * pNodeTwo, CElement* pEltStop)
{
    CTreeNode * pNode;
    CElement * pElement;

    if( pNodeTwo->Element() == Element() )
        return this;

    for ( pNode = this; pNode; pNode = pNode->Parent() )
    {
        pElement = pNode->Element();

        pElement->_fFirstCommonAncestor = 0;

        if (pElement == pEltStop)
            break;
    }

    for ( pNode = pNodeTwo; pNode; pNode = pNode->Parent() )
    {
        pElement = pNode->Element();

        pElement->_fFirstCommonAncestor = 1;

        if (pElement == pEltStop)
            break;
    }

    for ( pNode = this; pNode; pNode = pNode->Parent() )
    {
        pElement = pNode->Element();

        if (pElement->_fFirstCommonAncestor)
            return pNode;

        Assert( pElement != pEltStop );
    }

    return NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CTreeNode::GetFirstCommonBlockOrLayoutAncestor, public
//
//  Synopsis:   Returns the first branch whose element is common to
//              both of the branches specified and is a block or layout element
//
//  Arguments:  [pNode] -- branch to find common parent of with [this]
//              [pEltStop]  -- Stop walking tree if you hit this element.
//                             If NULL then search to root.
//
//  Returns:    Branch with common element from first starting point.
//
//----------------------------------------------------------------------------

CTreeNode *
CTreeNode::GetFirstCommonBlockOrLayoutAncestor(CTreeNode * pNodeTwo, CElement* pEltStop)
{
    CTreeNode * pNode;
    CElement * pElement;

    if( pNodeTwo->Element() == Element() )
        return this;

    for ( pNode = this; pNode; pNode = pNode->Parent() )
    {
        pElement = pNode->Element();

        pElement->_fFirstCommonAncestor = 0;

        if (pElement == pEltStop)
            break;
    }

    for ( pNode = pNodeTwo; pNode; pNode = pNode->Parent() )
    {
        pElement = pNode->Element();

        pElement->_fFirstCommonAncestor = 1;

        if (pElement == pEltStop)
            break;
    }

    for ( pNode = this; pNode; pNode = pNode->Parent() )
    {
        pElement = pNode->Element();

        if ((pElement->HasLayout() || pElement->IsBlockElement())
            && pElement->_fFirstCommonAncestor)
        {
            return pNode;
        }

        Assert( pElement != pEltStop );
    }

    return NULL;
}
//+---------------------------------------------------------------------------
//
//  Member:     CTreeNode::GetFirstCommonAncestorNode, public
//
//  Synopsis:   Returns the first node that is common to
//              both of the branches specified.
//
//  Arguments:  [pNode] -- branch to find common parent of with [this]
//              [pEltStop]  -- Stop walking tree if you hit this element.
//                             If NULL then search to root.
//
//  Returns:    Branch with common node from first starting point.
//
//----------------------------------------------------------------------------

CTreeNode *
CTreeNode::GetFirstCommonAncestorNode(CTreeNode * pNodeTwo, CElement* pEltStop)
{
    CTreeNode * pNode;

    if( this == pNodeTwo )
        return this;

    for ( pNode = this; pNode; pNode = pNode->Parent() )
    {
        pNode->_fFirstCommonAncestorNode = 0;

        if (pNode->Element() == pEltStop)
            break;
    }

    for ( pNode = pNodeTwo; pNode; pNode = pNode->Parent() )
    {
        pNode->_fFirstCommonAncestorNode = 1;

        if (pNode->Element() == pEltStop)
            break;
    }

    for ( pNode = this; pNode; pNode = pNode->Parent() )
    {
        if (pNode->_fFirstCommonAncestorNode)
            return pNode;

        Assert( pNode->Element() != pEltStop );
    }

    return NULL;
}


//+------------------------------------------------------------------------
//
//  Member:     CTreeNode::Depth
//
//  Synopsis:   Finds the depth of the node in the html tree
//
//  Returns:    int
//
//-------------------------------------------------------------------------

int
CTreeNode::Depth() const
{
    CTreeNode * pNode = const_cast<CTreeNode *>(this);
    int nDepth = 0;

    while ( pNode)
        nDepth++, pNode = pNode->Parent();

    Assert( nDepth > 0);

    return nDepth;
}


BEGIN_TEAROFF_TABLE_NAMED(CTreeNode, s_apfnNodeVTable)
    TEAROFF_METHOD(CTreeNode, GetInterface, getinterface, (REFIID riid, void **ppv))
    TEAROFF_METHOD_(CTreeNode, PutRef, putref, ULONG, ())
    TEAROFF_METHOD_(CTreeNode, RemoveRef, removeref, ULONG, ())
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
    TEAROFF_METHOD_NULL
END_TEAROFF_TABLE()

//+---------------------------------------------------------------------------
//
//  Member:     QueryInterface
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CTreeNode::GetInterface(REFIID iid, void **ppv)
{
    void *  pv = NULL;
    void *  pvWhack;
    void *  apfnWhack;
    HRESULT hr = E_NOINTERFACE;
    BOOL    fReuseTearoff = FALSE;
    const IID * piidDisp;

    *ppv = NULL;

    AssertSz( _pElement, "_pElement is NULL in CTreeNode::GetInterface -- VERY BAD!!!" );

    // handle IUnknown when tearoff is already created
    if (iid == IID_IUnknown && HasPrimaryTearoff())
    {
        IUnknown * pTearoff;

        pTearoff = GetPrimaryTearoff();

        Assert( pTearoff );

        pTearoff->AddRef();

        *ppv = pTearoff;

        return S_OK;
    }

    if (iid == CLSID_CTreeNode)
    {
        *ppv = this;
        return S_OK;
    }
    else if (iid == CLSID_CElement  ||
             iid == CLSID_CTextSite )
    {
        return _pElement->QueryInterface( iid, ppv );
    }

    // Create a tearoff to return

    // Get the interface from the element
    hr = THR_NOTRACE(_pElement->PrivateQueryInterface(iid, &pv));
    if(hr)
    {
        pv = NULL;
        goto Cleanup;
    }

    //
    // Whack in our node information, or the primary tearoff
    //
    if( HasPrimaryTearoff() )
    {
        pvWhack = (void*)GetPrimaryTearoff();
        apfnWhack = *(void**)pvWhack;
    }
    else
    {
        pvWhack = this;
        apfnWhack = (void*)s_apfnNodeVTable;
    }

#if DBG==1
    if( !HasPrimaryTearoff() )
        _fSettingTearoff = TRUE;
#endif

    Assert( pvWhack );

    // InstallTearOffObject puts a pointer to an object
    // into pvObject2 of the tearoff.  This means that we
    // are assuming that when an element is QI'd, the interfaces
    // listed below will be returned as tearoffs.  Also, every
    // interface that uses the eax trick for passing context
    // through must be in this list.  Otherwise we will
    // create another tearoff pointing to the tearoff.
    //

    piidDisp = _pElement->BaseDesc()->_piidDispinterface;
    piidDisp = piidDisp ? piidDisp : &IID_NULL;

    if(iid == *piidDisp                 ||
       iid == IID_IHTMLElement          ||
       iid == IID_IDispatch             ||
       iid == IID_IDispatchEx           ||
       iid == IID_IHTMLControlElement)
    {
        hr = THR(InstallTearOffObject(pv, pvWhack,
                    apfnWhack, QI_MASK));
        if(hr)
            goto Cleanup;

        *ppv = pv;

        fReuseTearoff = TRUE;
    }
    else
    {
        hr = THR(
            CreateTearOffThunk(
                pv, * (void **) pv, NULL, ppv, pvWhack,
                apfnWhack, QI_MASK, NULL ));
        ((IUnknown *)pv)->Release();
        if(hr)
            goto Cleanup;
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    if( ! HasPrimaryTearoff() )
    {
        // This tearoff that we just created is now the primary tearoff
        SetPrimaryTearoff( (IUnknown *)*ppv );
        WHEN_DBG( _fSettingTearoff = FALSE );
    }

    if(!fReuseTearoff)
        ((IUnknown *)*ppv)->AddRef();

    hr = S_OK;

Cleanup:
    if(hr && pv)
        ((IUnknown *)pv)->Release();

    RRETURN( hr );
}


ULONG
CTreeNode::PutRef(void)
{
#if DBG==1
    if (s_nSNTrace == _nSerialNumber)
    {
        TraceTag((0, "treenode %d PutRef", _nSerialNumber));
        TraceCallers(0, 0, 12);
    }
#endif

    Assert( ! _fInDestructor );
    Assert( _fSettingTearoff );
    Assert( ! HasPrimaryTearoff() );

    // We do this so that we know that the node will
    // die before the element.  If it was the other way
    // around, we wouldn't be able to get to the doc to
    // del our primary tearoff lookaside pointer
    Element()->AddRef();

    return 1;
}

ULONG
CTreeNode::RemoveRef(void)
{
    CElement * pElement = Element();

#if DBG==1
    if (s_nSNTrace == _nSerialNumber)
    {
        TraceTag((0, "treenode %d RemoveRef", _nSerialNumber));
        TraceCallers(0, 0, 12);
    }
#endif

    Assert( !_fInDestructor );
    Assert( HasPrimaryTearoff() );

    DelPrimaryTearoff();

    if (!_fInMarkup)
    {
        WHEN_DBG( _fInDestructor = TRUE );
        delete this;
    }

    // Release the ref that we put on the element above.
    pElement->Release();

    return 1;
}

ULONG
CTreeNode::NodeAddRef(void)
{
    IUnknown *pTearoff = NULL;

#if DBG==1
    if (s_nSNTrace == _nSerialNumber)
    {
        TraceTag((0, "treenode %d NodeAddRef", _nSerialNumber));
        TraceCallers(0, 0, 12);
    }
#endif

    Assert( ! _fInDestructor );

    if( ! HasPrimaryTearoff() )
    {
        // Use GetInterface to create the primary interface
        Verify( ! GetInterface( IID_IUnknown, (void**)&pTearoff ) );

        Assert( pTearoff );

        return 1;
    }
    else
    {
        pTearoff = GetPrimaryTearoff();

        Assert( pTearoff );

        return pTearoff->AddRef();
    }
}

ULONG
CTreeNode::NodeRelease(void)
{
    IUnknown *pTearoff;

#if DBG==1
    if (s_nSNTrace == _nSerialNumber)
    {
        TraceTag((0, "treenode %d ReleaseRef", _nSerialNumber));
        TraceCallers(0, 0, 12);
    }
#endif

    Assert( !_fInDestructor );
    Assert( HasPrimaryTearoff() );

    pTearoff = GetPrimaryTearoff();

    Assert( pTearoff );

    return pTearoff->Release();

}

//
// Ref counting helpers
//

void
CTreeNode::ReplacePtr      ( CTreeNode * * pbrlhs, CTreeNode * brrhs )
{
    if (pbrlhs)
    {
        if (brrhs)
        {
            brrhs->NodeAddRef();
        }
        if (*pbrlhs)
        {
            (*pbrlhs)->NodeRelease();
        }
        *pbrlhs = brrhs;
    }
}

void
CTreeNode::SetPtr          ( CTreeNode * * pbrlhs, CTreeNode * brrhs )
{
    if (pbrlhs)
    {
        if (brrhs)
        {
            brrhs->NodeAddRef();
        }
        *pbrlhs = brrhs;
    }
}

void
CTreeNode::ClearPtr        ( CTreeNode * * pbrlhs )
{
    if (pbrlhs && * pbrlhs)
    {
        CTreeNode * pNode = *pbrlhs;
        *pbrlhs = NULL;
        pNode->NodeRelease();
    }
}

void
CTreeNode::ReleasePtr        ( CTreeNode * pNode )
{
    if (pNode)
    {
        pNode->NodeRelease();
    }
}

void
CTreeNode::StealPtrSet     ( CTreeNode * * pbrlhs, CTreeNode * brrhs )
{
    SetPtr( pbrlhs, brrhs );

    if (pbrlhs && *pbrlhs)
        (*pbrlhs)->NodeRelease();
}

void
CTreeNode::StealPtrReplace ( CTreeNode * * pbrlhs, CTreeNode * brrhs )
{
    ReplacePtr( pbrlhs, brrhs );

    if (pbrlhs && *pbrlhs)
        (*pbrlhs)->NodeRelease();
}

//+---------------------------------------------------------------------------
//
//  Lookaside storage
//
//----------------------------------------------------------------------------

void *
CTreeNode::GetLookasidePtr(int iPtr)
{
#if DBG == 1
    Assert( Doc() );
    if(HasLookasidePtr(iPtr))
    {
        void * pLookasidePtr =  Doc()->GetLookasidePtr((DWORD *)this + iPtr);

        Assert(pLookasidePtr == _apLookAside[iPtr]);

        return pLookasidePtr;
    }
    else
        return NULL;
#else
    return(HasLookasidePtr(iPtr) ? Doc()->GetLookasidePtr((DWORD *)this + iPtr) : NULL);
#endif
}

HRESULT
CTreeNode::SetLookasidePtr(int iPtr, void * pvVal)
{
    Assert( Doc() );
    Assert (!HasLookasidePtr(iPtr) && "Can't set lookaside ptr when the previous ptr is not cleared");

    HRESULT hr = THR(Doc()->SetLookasidePtr((DWORD *)this + iPtr, pvVal));

    if (hr == S_OK)
    {
        _fHasLookasidePtr |= 1 << iPtr;

#if DBG == 1
        _apLookAside[iPtr] = pvVal;
#endif
    }

    RRETURN(hr);
}

void *
CTreeNode::DelLookasidePtr(int iPtr)
{
    Assert( Doc() );
    if (HasLookasidePtr(iPtr))
    {
        void * pvVal = Doc()->DelLookasidePtr((DWORD *)this + iPtr);
        _fHasLookasidePtr &= ~(1 << iPtr);
#if DBG == 1
        _apLookAside[iPtr] = NULL;
#endif
        return(pvVal);
    }

    return(NULL);
}


//+----------------------------------------------------------------------------
//
// Member:      ConvertFmToCSSBorderStyle
//
// Synopsis:    Converts the border style from the internal type to the type
//                  used to set it.
//-----------------------------------------------------------------------------

styleBorderStyle
ConvertFmToCSSBorderStyle(BYTE bFmBorderStyle)
{
    switch ( bFmBorderStyle )
    {
    case fmBorderStyleDotted:
        return styleBorderStyleDotted;
    case fmBorderStyleDashed:
        return styleBorderStyleDashed;
    case fmBorderStyleDouble:
        return styleBorderStyleDouble;
    case fmBorderStyleSingle:
        return styleBorderStyleSolid;
    case fmBorderStyleEtched:
        return styleBorderStyleGroove;
    case fmBorderStyleBump:
        return styleBorderStyleRidge;
    case fmBorderStyleSunken:
        return styleBorderStyleInset;
    case fmBorderStyleRaised:
        return styleBorderStyleOutset;
    case fmBorderStyleNone:
        return styleBorderStyleNone;
    case 0xff:
        return styleBorderStyleNotSet;
    }

    Assert( FALSE && "Unknown Border Style!" );
    return styleBorderStyleNotSet;
}


//+----------------------------------------------------------------------------
//
// Member:      GetCascadedborderTopStyle
//
// Synopsis:    Return the top border style value for the node
//
//-----------------------------------------------------------------------------

styleBorderStyle
CTreeNode::GetCascadedborderTopStyle()
{
    const CFancyFormat *pFF = GetFancyFormat();
    Assert(pFF);
    return ConvertFmToCSSBorderStyle(pFF->_bBorderStyles[BORDER_TOP]);
}


//+----------------------------------------------------------------------------
//
// Member:      GetCascadedborderRightStyle
//
// Synopsis:    Return the right border style value for the node
//
//-----------------------------------------------------------------------------

styleBorderStyle
CTreeNode::GetCascadedborderRightStyle()
{
    const CFancyFormat *pFF = GetFancyFormat();
    Assert(pFF);
    return ConvertFmToCSSBorderStyle(pFF->_bBorderStyles[BORDER_RIGHT]);
}


//+----------------------------------------------------------------------------
//
// Member:      GetCascadedborderBottomStyle
//
// Synopsis:    Return the bottom border style value for the node
//
//-----------------------------------------------------------------------------

styleBorderStyle
CTreeNode::GetCascadedborderBottomStyle()
{
    const CFancyFormat *pFF = GetFancyFormat();
    Assert(pFF);
    return ConvertFmToCSSBorderStyle(pFF->_bBorderStyles[BORDER_BOTTOM]);
}


//+----------------------------------------------------------------------------
//
// Member:      GetCascadedborderLeftStyle
//
// Synopsis:    Return the left border style value for the node
//
//-----------------------------------------------------------------------------

styleBorderStyle
CTreeNode::GetCascadedborderLeftStyle()
{
    const CFancyFormat *pFF = GetFancyFormat();
    Assert(pFF);
    return ConvertFmToCSSBorderStyle(pFF->_bBorderStyles[BORDER_LEFT]);
}



//+----------------------------------------------------------------------------
//
// Member:      GetCascadedclearLeft
//
// Synopsis:
//
//-----------------------------------------------------------------------------

BOOL
CTreeNode::GetCascadedclearLeft()
{
    return !!GetFancyFormat()->_fClearLeft;
}


//+----------------------------------------------------------------------------
//
// Member:      GetCascadedclearRight
//
// Synopsis:
//
//-----------------------------------------------------------------------------

BOOL
CTreeNode::GetCascadedclearRight()
{
    return !!GetFancyFormat()->_fClearRight;
}



//+------------------------------------------------------------------------
//
//  Member:     CTreeNode::IsInlinedElement
//
//  Synopsis:   Determines if the element is rendered inflow or not, If the
//              element is not absolutely positioned and is left or right
//              aligned with hr and legend an exception (they can only be
//              aligned but nothing should wrap around them).
//              For non-sites we return TRUE.
//
//  Returns:    BOOL indicating whether or not the site is inlined
//
//-------------------------------------------------------------------------

BOOL
CTreeNode::IsInlinedElement ()
{
    if (NeedsLayout())
    {
        const CFancyFormat * pFF = GetFancyFormat();

        return      pFF->_bPositionType != stylePositionabsolute
                &&  !pFF->_fAlignedLayout;
    }

    return TRUE;
}

void
CTreeNode::GetRelTopLeft(
    CElement    * pElementFL,
    CParentInfo * ppi,
    long * pxRelLeft,
    long * pyRelTop)
{
    CTreeNode * pNode = this;
    CDoc      * pDoc = pElementFL->Doc();
    long        lFontHeight;
    const CCharFormat  * pCF;
    const CFancyFormat * pFF;

    Assert(pyRelTop && pxRelLeft);

    *pyRelTop = 0;
    *pxRelLeft = 0;

    while(pNode && pNode->Element() != pElementFL)
    {
        pCF = pNode->GetCharFormat();

        if(!pCF->_fRelative)
            break;

        lFontHeight = pCF->GetHeightInTwips(pDoc);
        pFF = pNode->GetFancyFormat();

        if(pFF->_fRelative)
        {
            *pyRelTop  += pFF->_cuvTop.YGetPixelValue(ppi, ppi->_sizeParent.cy, lFontHeight);
            *pxRelLeft += pFF->_cuvLeft.XGetPixelValue(ppi, ppi->_sizeParent.cx, lFontHeight);
        }

        pNode = pNode->Parent();
    }
}

//+----------------------------------------------------------------------------
//
// Member:      GetCurrentRelativeNode
//
// Synopsis:    Get the node that is relative, which causes the current
//              chunk to be relative.
//
//-----------------------------------------------------------------------------
CTreeNode *
CTreeNode::GetCurrentRelativeNode(CElement * pElementFL)
{
    const CFancyFormat * pFF;
    CTreeNode * pNodeStart = this;

    while(pNodeStart && DifferentScope(pElementFL, pNodeStart))
    {
        pFF = pNodeStart->GetFancyFormat();

        // BUGBUG (jbeda): I'm pretty sure this is wrong for some
        // overlapped cases

        // relatively positioned layout elements are to be ignored
        if(!pNodeStart->Element()->NeedsLayout() && pFF->_fRelative)
            return pNodeStart;

        pNodeStart = pNodeStart->Parent();
    }
    return NULL;
}

//+----------------------------------------------------------------------------
//
// Member:      EnsureFormats
//
// Synopsis:    Compute the formats if dirty
//
//-----------------------------------------------------------------------------
void
CTreeNode::EnsureFormats()
{
    if (_iCF < 0)
    {
        GetCharFormatHelper();
    }
    if (_iPF < 0)
    {
        GetParaFormatHelper();
    }
    if (_iFF < 0)
    {
        GetFancyFormatHelper();
    }

    // BUGBUG (MohanB) Might unnecessarily cause of multiple walks of the
    // slave tree if the master element is overlapping and CMarkup::EnsureFormats
    // is called on the master's markup. Rare case, not important for IE5.

    if (Element()->HasSlaveMarkupPtr())
    {
        Element()->GetSlaveMarkupPtr()->EnsureFormats();
    }
}
