#include "headers.hxx"

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_WCHDEFS_H_
#define X_WCHDEFS_H_
#include "wchdefs.h"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_TOMCONST_H_
#define X_TOMCONST_H_
#include "tomconst.h"
#endif

#ifndef X_BREAKER_HXX_
#define X_BREAKER_HXX_
#include "breaker.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X__TEXT_H_
#define X__TEXT_H_
#include "_text.h"
#endif

DeclareTagOther(tagMarkupPointerAlwaysEmbed, "MarkupPointer", "Force embedding of all markup pointers");

MtDefine(CMarkupPointer, Tree, "CMarkupPointer");

#if DBG == 1 || defined(DUMPTREE)
int CMarkupPointer::s_NextSerialNumber = 1;
#endif

#if DBG == 1

void
SetDebugName ( IMarkupPointer * pIPointer, LPCTSTR strDbgName )
{
    CMarkupPointer * pPointer;

    IGNORE_HR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void **) & pPointer ) );

    pPointer->SetDebugName( strDbgName );
}

#endif


inline BOOL
IsValidGravity ( POINTER_GRAVITY eGravity )
{
    return eGravity == POINTER_GRAVITY_Left || eGravity == POINTER_GRAVITY_Right;
}

inline BOOL
IsValidAdjacency ( ELEMENT_ADJACENCY eAdj )
{
    return
        eAdj == ELEM_ADJ_BeforeBegin || eAdj == ELEM_ADJ_AfterBegin ||
        eAdj == ELEM_ADJ_BeforeEnd   || eAdj == ELEM_ADJ_AfterEnd;
}

void
EnsureLogicalOrder ( CMarkupPointer * & pStart, CMarkupPointer * & pFinish )
{
    Assert( pStart && pFinish );
    Assert( pStart->IsPositioned() && pFinish->IsPositioned() );
    Assert( pStart->Markup() == pFinish->Markup() );

    if (pStart->IsRightOf( pFinish ) )
    {
        CMarkupPointer * pTemp = pStart;
        pStart = pFinish;
        pFinish = pTemp;
    }
}

void
EnsureTotalOrder ( CTreePosGap * & ptpgStart, CTreePosGap * & ptpgFinish )
{
    CTreePos * ptpStart = ptpgStart->AdjacentTreePos( TPG_RIGHT );
    CTreePos * ptpFinish = ptpgFinish->AdjacentTreePos( TPG_RIGHT );
    
    Assert( ptpStart->GetCp() <= ptpFinish->GetCp() );

    if (ptpStart == ptpFinish)
        return;

    //
    // Move the finish as far to the right as possible without going over any content,
    // looking for the start.  If we find the start, then they are not ordered properly.
    //

    while ( ptpFinish->IsPointer() || ptpFinish->IsText() && ptpFinish->Cch() == 0 )
    {
        ptpFinish = ptpFinish->NextTreePos();

        if (ptpFinish == ptpStart)
        {
            CTreePosGap * ptpgTemp = ptpgStart;
            ptpgStart = ptpgFinish;
            ptpgFinish = ptpgTemp;

            return;
        }
    }
}

#if DBG == 1

CMarkupPointer::CMarkupPointer ( CDoc * pDoc )
  : _pDoc( pDoc ), _pMarkup( NULL ),
    _pmpNext( NULL ), _pmpPrev( NULL ),
    _fRightGravity( FALSE ), _fCling( FALSE ),
    _fEmbedded( FALSE ),
    _fKeepMarkupAlive( FALSE ), _fAlwaysEmbed( FALSE ),
    _ptpRef( NULL ), _ichRef( 0 ),
    _verCp( 0 ), _cpCache( -1 ),
    _nSerialNumber( s_NextSerialNumber++ )
{
}

void
CMarkupPointer::Validate ( ) const
{
    static BOOL fValidating = FALSE;

    if (fValidating)
        return;
            
    if (!IsPositioned())
    {
        Assert( _pmpNext == NULL );
        Assert( _pmpPrev == NULL );
        Assert( ! _fEmbedded );
        Assert( _cpCache == -1 );
        Assert( _verCp == 0 );
        Assert( _ptp == NULL );
        Assert( _ichRef == 0 );

        return;
    }

    Assert( ! _fAlwaysEmbed || _fEmbedded );

    static BOOL fValidatingAll = FALSE;
        
    if (IsPositioned() && !fValidatingAll)
    {
        fValidatingAll = TRUE;
        
        for ( CMarkupPointer * pmp = Markup()->_pmpFirst ; pmp ; pmp = pmp->_pmpNext )
        {
            if (pmp != this)
                pmp->Validate();
        }
        
        fValidatingAll = FALSE;
    }

    AssertSz(
        ! _fEmbedded || _ptpEmbeddedPointer->IsPointer(),
        "Embedded pointer does not point to pointer pos" );

    if (!_fEmbedded)
    {
        AssertSz( _ptpRef && ! _ptpRef->IsPointer(), "Bad position reference" );

        Assert( ! _ptpRef->IsText() || (_ichRef >= 0 && _ichRef <= _ptpRef->Cch()) );

        //
        // The only time it is ok for the ich to be 0 when on a zero length text
        // pos is when that text pos has a non zero text id.  This is so because
        // being after a zero sized id'ed chunk of text is accomplished by pointing
        // at the text pos and setting cch to zero.  Otherwise, if you want to be
        // before the text pos, point to the previous text pos.
        //

        if (_ptpRef->IsText() && _ichRef == 0 && _ptpRef->Cch() == 0)
            AssertSz( _ptpRef->TextID() != 0, "Ambiguous unembedded pointer position" );
    }
    else
    {
        Assert( _ptpEmbeddedPointer->IsPointer() );
        
        Assert( _ptpRef->MarkupPointer() == this );
        Assert( _ptpRef->Gravity() == Gravity() );
        Assert( _ptpRef->Cling() == Cling() );
        
        Assert( ! _pmpNext && ! _pmpNext );

        //
        // Make sure this pointer is NOT in it's markups list
        //

        for ( CMarkupPointer * p = Markup()->_pmpFirst ; p ; p = p->_pmpNext )
            AssertSz( p != this, "Embedded pointer is in unembedded list" );
    }

    //
    // Make sure the pointer is not in an inclusion
    //

    {
        CTreePos * ptp;
        long       ich;

        fValidating = TRUE;

        ptp = GetNormalizedReference( ich );

        fValidating = FALSE;
        
        if (ptp->IsNode() && !ptp->IsEdgeScope() && ptp->IsEndNode())
            AssertSz( 0, "Pointer in the middle of an inclusion" );

        ptp = ptp->NextTreePos();
        
        if (ptp->IsNode() && !ptp->IsEdgeScope() && ptp->IsBeginNode())
            AssertSz( 0, "Pointer in the middle of an inclusion" );
    }

    //
    // If we are caching a cp, compute it manually and make sure it is ok
    //
    
    AssertSz( ! CpIsCached() || GetCpSlow() == _cpCache, "Cached cp is not valid" );
}

#endif

inline void
CMarkupPointer::AddMeToList ( )
{
    Assert( Markup() );
    Assert( ! _pmpNext && ! _pmpPrev );
    
    CMarkupPointer * & pmpFirst = Markup()->_pmpFirst;

    _pmpNext = pmpFirst;
    
    if (pmpFirst)
        pmpFirst->_pmpPrev = this;
    
    pmpFirst = this;
}

inline void
CMarkupPointer::RemoveMeFromList ( )
{
    Assert( Markup() );

#if DBG == 1
    
    for ( CMarkupPointer * pmp = Markup()->_pmpFirst ; pmp ; pmp = pmp->_pmpNext )
    {
        if (pmp == this)
            break;
    }

    Assert( pmp );
    
#endif

    CMarkupPointer * & pmpFirst = Markup()->_pmpFirst;
    
    if (pmpFirst == this)
        pmpFirst = _pmpNext;

    if (_pmpPrev)
        _pmpPrev->_pmpNext = _pmpNext;

    if (_pmpNext)
        _pmpNext->_pmpPrev = _pmpPrev;

    _pmpNext = _pmpPrev = NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:     GetNormalizedReference
//
//  Synopsis:   Returns a ptp/ich pair as far left as possible.  Basically,
//              skips past pointers and empty 0-textid text pos/
//
//-----------------------------------------------------------------------------

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

CTreePos *
CMarkupPointer::GetNormalizedReference ( long & ichOut ) const
{
    Assert( IsPositioned() );

    Validate();

    //
    // Unembedded pointers are already normalized!
    //

    if (!_fEmbedded)
    {
        ichOut = _ichRef;
        return _ptpRef;
    }
    
    for ( CTreePos * ptp = _ptpEmbeddedPointer ; ; )
    {
        ptp = ptp->PreviousTreePos();

        if (ptp->IsPointer())
            continue;

        if (ptp->IsText())
        {
            long cch = ptp->Cch();

            if (cch > 0)
            {
                ichOut = cch;
                return ptp;
            }

            //
            // Special case for text id, can have ich == 0 and cch = 0
            // when text has non zero ID.
            //
            
            if (ptp->TextID() != 0)
            {
                ichOut = 0;
                return ptp;
            }

            //
            // Skip past empty non-text id text pos
            //

            continue;
        }

        ichOut = 0;
        return ptp;
    }
}

HRESULT
CMarkupPointer::UnEmbed ( CTreePos * * pptpUpdate, long * pichUpdate )
{
    HRESULT    hr = S_OK;
    CTreePos * ptpSave;

    //
    // If we are already not embedded, then do nothing.
    //
    
    if (!IsPositioned() || !_fEmbedded)
        goto Cleanup;
    
    ptpSave = _ptpEmbeddedPointer;

    _ptpRef = GetNormalizedReference( _ichRef );
    
    
    AddMeToList();

    _fEmbedded = FALSE;

    //
    // Clear the CMarkupPointer pointer in the pos so that OnPositionReleased
    // isn't called, so we don't assume that the pointer is still embedded
    // and someone is taking it out of the tree (other than here).
    //
    
    ptpSave->SetMarkupPointer( NULL );
    
    hr = THR( Markup()->RemovePointerPos( ptpSave, pptpUpdate, pichUpdate ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize("", on)
#endif

///////////////////////////////////////////
//  CBase methods

const CMarkupPointer::CLASSDESC CMarkupPointer::s_classdesc =
{
    NULL,                               // _pclsid
    0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
    0,                                  // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                               // _pcpi
};

const CBase::CLASSDESC *
CMarkupPointer::GetClassDesc () const
{
    return &s_classdesc;
}

HRESULT
CMarkupPointer::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    if (ppv == NULL)
        return E_INVALIDARG;

    *ppv = NULL;

    switch (iid.Data1)
    {
        QI_INHERITS(this, IUnknown)
        QI_INHERITS(this, IMarkupPointer)

    default:
        if (iid == CLSID_CMarkupPointer)
        {
            *ppv = this;
            return S_OK;
        }
        break;
    }

    if (!*ppv)
        RRETURN( E_NOINTERFACE );

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


///////////////////////////////////////////
//  IMarkupPointer methods

STDMETHODIMP
CMarkupPointer::OwningDoc ( IHTMLDocument2 ** ppDoc )
{
    return _pDoc->_pPrimaryMarkup->QueryInterface( IID_IHTMLDocument2, (void **) ppDoc );
}


STDMETHODIMP
CMarkupPointer::Gravity ( POINTER_GRAVITY *peGravity )
{
    HRESULT hr = S_OK;

    if (!peGravity)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *peGravity = Gravity() ? POINTER_GRAVITY_Right : POINTER_GRAVITY_Left;

Cleanup:

    RRETURN( hr );
}


STDMETHODIMP
CMarkupPointer::SetGravity ( POINTER_GRAVITY eGravity )
{
    HRESULT hr = S_OK;

    Assert( IsValidGravity( eGravity ) );

    if (!IsValidGravity( eGravity ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    _fRightGravity = eGravity == POINTER_GRAVITY_Right;

    //
    // Push the gravity to the pos if we are embedded
    //

    if (_fEmbedded)
        GetEmbeddedTreePos()->SetGravity( _fRightGravity );
    
Cleanup:

    RRETURN( hr );
}

STDMETHODIMP
CMarkupPointer::Cling ( BOOL * pfCling )
{
    HRESULT hr = S_OK;

    if (!pfCling)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfCling = Cling();

Cleanup:

    RRETURN( hr );
}


STDMETHODIMP
CMarkupPointer::SetCling ( BOOL fCling )
{
    HRESULT hr = S_OK;

    _fCling = fCling;
    
    //
    // Push the cling to the pos if we are embedded
    //

    if (_fEmbedded)
        GetEmbeddedTreePos()->SetCling( _fCling );
    
    RRETURN( hr );
}

STDMETHODIMP
CMarkupPointer::MoveAdjacentToElement ( IHTMLElement *pIElement, ELEMENT_ADJACENCY eAdj )
{
    HRESULT hr;
    CElement * pElement;

    if (!IsValidAdjacency( eAdj ) || !pIElement || !_pDoc->IsOwnerOf( pIElement ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIElement->QueryInterface( CLSID_CElement, (void **) & pElement ) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pElement->IsInMarkup())
    {
        hr = CTL_E_UNPOSITIONEDELEMENT;
        goto Cleanup;
    }

    hr = THR( MoveAdjacentToElement( pElement, eAdj ) );

    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}


STDMETHODIMP
CMarkupPointer::MoveToPointer ( IMarkupPointer * pIPointer )
{
    HRESULT hr = S_OK;
    CMarkupPointer *pPointer;

    if (!pIPointer || !_pDoc->IsOwnerOf( pIPointer ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pIPointer->QueryInterface( CLSID_CMarkupPointer, (void**) & pPointer) );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!pPointer->IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }

    hr = THR( MoveToPointer( pPointer ) );

Cleanup:
    
    RRETURN( hr );
}

STDMETHODIMP
CMarkupPointer::MoveToContainer ( IMarkupContainer * pContainer, BOOL fAtStart )
{
    HRESULT     hr = S_OK;
    CMarkup *   pMarkup;

    if (!pContainer || !_pDoc->IsOwnerOf( pContainer ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( pContainer->QueryInterface( CLSID_CMarkup, (void **) & pMarkup ) );
    
    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( MoveToContainer( pMarkup, fAtStart ) );

Cleanup:
    
    RRETURN( hr );
}


STDMETHODIMP
CMarkupPointer::IsPositioned ( BOOL * pfPositioned )
{
    HRESULT hr = S_OK;

    if (!pfPositioned)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pfPositioned = IsPositioned();

Cleanup:

    RRETURN( hr );
}

STDMETHODIMP
CMarkupPointer::GetContainer ( IMarkupContainer * * ppContainer )
{
    HRESULT   hr = S_OK;
    CMarkup * pMarkup;

    if (!ppContainer)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppContainer = NULL;

    pMarkup = Markup();

    if (pMarkup)
    {
        hr = THR(
            pMarkup->QueryInterface(
                IID_IMarkupContainer, (void **) ppContainer ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

STDMETHODIMP
CMarkupPointer::Unposition ( )
{
    HRESULT hr = S_OK;
    
    if (!IsPositioned())
        goto Cleanup;

    hr = THR( UnEmbed( NULL, NULL ) );

    if (hr)
        goto Cleanup;

    //
    // Remove from the unembeded list.  Remember, it can be
    // anywhere in the list.
    //

    RemoveMeFromList();

    //
    // Now, setting the markup to NULL will finally unposition it
    //
    
    SetMarkup(NULL);
    _ptpRef = NULL;
    _ichRef = 0;
    
    //
    // We have changed the position of the pointer, invalidate the
    // cp cache
    //
    
    _verCp = 0;
    _cpCache = -1;

    Validate();
    
Cleanup:
    
    RRETURN( hr );
}

#if DBG!=1
#pragma optimize("", on)
#endif


STDMETHODIMP
CMarkupPointer::Left (
    BOOL                  fMove,
    MARKUP_CONTEXT_TYPE * pContext,
    IHTMLElement * *      ppElement,
    long *                pcch,
    OLECHAR *             pchtext )
{
    return THR( There( TRUE, fMove, pContext, ppElement, pcch, pchtext, 0 ) );
}

STDMETHODIMP
CMarkupPointer::Right (
    BOOL                  fMove,
    MARKUP_CONTEXT_TYPE * pContext,
    IHTMLElement * *      ppElement,
    long *                pcch,
    OLECHAR *             pchText )
{
    return THR( There( FALSE, fMove, pContext, ppElement, pcch, pchText, 0 ) );
}

HRESULT
CMarkupPointer::There (
    BOOL                  fLeft,
    BOOL                  fMove,
    MARKUP_CONTEXT_TYPE * pContext,
    IHTMLElement * *      ppElement,
    long *                pcch,
    OLECHAR *             pchText,
    DWORD *               pdwFlags)
{
    HRESULT     hr;
    CTreeNode * pNode;

    pNode = NULL;

    hr = THR(
        There(
            fLeft, fMove, pContext,
            ppElement ? & pNode : NULL,
            pcch, pchText, NULL, pdwFlags ) );

    if (hr)
        goto Cleanup;

    if (ppElement)
    {
        *ppElement = NULL;

        if (pNode)
        {
            hr = THR( pNode->GetElementInterface( IID_IHTMLElement, (void * *) ppElement ) );
    
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

HRESULT
CMarkupPointer::There (
    BOOL                  fLeft,
    BOOL                  fMove,
    MARKUP_CONTEXT_TYPE * pContext,
    CTreeNode * *         ppNode,
    long *                pcch,
    OLECHAR *             pchText,
    long *                plTextID,
    DWORD *               pdwFlags )
{
    HRESULT    hr = S_OK;
    long       cchIn = 0;
    CTreePos * ptp;
    long       ich;
    long       dcch;
    BOOL       fHitNode;

    Validate();
    
    //
    // We must assign something to all output params, even if they are
    // meaningless in the current context.
    //

    if (ppNode)
        *ppNode = NULL;

    if (pcch)
    {
        cchIn = *pcch;
        *pcch = 0;
    }

    if (plTextID)
        *plTextID = 0;

    if (pContext)
        *pContext = CONTEXT_TYPE_None;
    
    //
    // If we are unpositioned, then we have done all we have to do.
    //

    if (!IsPositioned())
        goto Cleanup;

    //
    // Get the current reference
    //

    if (_fEmbedded)
    {
        ptp = _ptpEmbeddedPointer;
        ich = 0;
    }
    else
    {
        ptp = _ptpRef;
        ich = _ichRef;
    }

    //
    // Compute the delta of characters moved, useful for updating cached cp.
    //

    dcch = 0;

    //
    // If the content we are adjacent to is a node, then set this var.
    //

    fHitNode = FALSE;

    //
    // The following will set ptp to either point to the edge we are to cross,
    // or set the ptp/ich pair to a significant text node.
    //

    if (fLeft)
    {
        for ( ; ; )
        {
            if (!ptp->IsPointer())
            {
                if (ptp->IsNode())
                {
                    fHitNode = TRUE;
                    break;
                }

                Assert( ptp->IsText() );

                if (ich > 0)
                    break;

                //
                // Check for an empty text pos with ID.  This is a empty
                // DOM text node.
                //

                Assert( ptp->Cch() == 0 );

                if (plTextID && ptp->TextID() != 0)
                    break;
            }

            ptp = ptp->PreviousTreePos();
            ich = ptp->IsText() ? ptp->Cch() : 0;
        }
    }
    else
    {
        for ( ; ; )
        {
            if (ptp->IsText() && ich < ptp->Cch())
                break;
            
            CTreePos * ptpNext = ptp->NextTreePos();

            if (!ptpNext->IsPointer())
            {
                if (ptpNext->IsNode())
                {
                    fHitNode = TRUE;
                    break;
                }

                Assert( ptpNext->IsText() );

                if (ptpNext->Cch())
                    break;

                if (plTextID && ptpNext->TextID() != 0)
                    break;
            }

            //
            // We should never attempt to cross over an empty dom text node.  We
            // should have broken out of hte loop by now in this case.
            //

            ptp = ptpNext;
            
            Assert( !plTextID || !ptp->IsText() || ptp->Cch() != 0 || ptp->TextID() != 0 );
            
            ich = 0;
        }
    }

    //
    // See if we stopped on a node
    //

    if (fHitNode)
    {
        long nIncl = 0;
        
        ptp = fLeft ? ptp : (dcch++, ptp->NextTreePos());

        Assert( ptp->IsNode() );
        
        //
        // Find the kernel of the inclusion
        //

        for ( nIncl = 0 ; ! ptp->IsEdgeScope() ; nIncl++ )
        {
            if (fLeft)
            {
                dcch -= 1;
                ptp = ptp->PreviousTreePos();
            }
            else
            {
                dcch += 1;
                ptp = ptp->NextTreePos();
            }
        }

        //
        // Have we butted up against the edge of the container?
        //

        if (ptp->Branch()->Tag() == ETAG_ROOT)
            goto Cleanup;

        //
        // Return the node crossed
        //

        if (ppNode)
            *ppNode = ptp->Branch();

        //
        // If we are traveling left and encounter a begin node, or we are
        // traveling right and encounter an end node, then we have left the
        // scope of an element.  Otherwise we have entered the scope of an
        // element (the else clause)
        //

        if (fLeft && ptp->IsBeginNode() || !fLeft && ptp->IsEndNode())
        {
            //
            // We better not have left the scope of a no scope element, for
            // we never must ever get into one.
            //

            Assert( ! ptp->Branch()->Element()->IsNoScope() );

            //
            // When moving out of a TXTSLAVE, behave as if we're moving off
            // the edge of the tree (dbau): report no context, don't move the
            // pointer, and report no element.
            //
            
            if (pdwFlags && !(*pdwFlags & MPTR_SHOWSLAVE) && ptp->Branch()->Tag() == ETAG_TXTSLAVE)
                goto Cleanup;

            if (pContext)
                *pContext = CONTEXT_TYPE_ExitScope;
        }
        else
        {
            //
            // If we have moved into the scope of a no scope element, then break
            // out to the other side.
            //

            if (ptp->Branch()->Element()->IsNoScope())
            {
                if (pContext)
                    *pContext = CONTEXT_TYPE_NoScope;

                Assert( nIncl == 0 );

                Assert(
                    ptp->Branch() ==
                        ptp->Branch()->Element()->GetFirstBranch() );

                if (fMove)
                {
                    //
                    // Skip over to the other side of the no scope element
                    //

                    if (fLeft)
                    {
                        dcch -= 1;
                        ptp->Branch()->Element()->GetTreeExtent( & ptp, NULL );
                    }
                    else
                    {
                        dcch += 1;
                        ptp->Branch()->Element()->GetTreeExtent( NULL, & ptp );
                    }
                }
            }
            else
            {
                if (pContext)
                    *pContext = CONTEXT_TYPE_EnterScope;
            }
        }

        //
        // Get out of the inclusion and position properly.
        //

        if (fLeft)
        {
            dcch -= nIncl;
            
            while ( nIncl-- )
                ptp = ptp->PreviousTreePos();

            Assert( ptp->IsNode() );
            
            ptp = ptp->PreviousTreePos();
            dcch--;

            ich = ptp->IsText() ? ptp->Cch() : 0;
        }
        else
        {
            dcch += nIncl;
            
            while ( nIncl-- )
                ptp = ptp->NextTreePos();
            
            ich = 0;
        }
    }
    else
    {
        if (pContext)
            *pContext = CONTEXT_TYPE_Text;
        
#if DBG == 1
        if (fLeft)
            Assert( ptp->IsText() );
        else
            Assert( ptp->IsText() && ich < ptp->Cch() || ptp->NextTreePos()->IsText() );
#endif
        
        if (plTextID)
        {
            if (fLeft)
                *plTextID = ptp->TextID();
            else
            {
                if (ptp->IsText() && ich < ptp->Cch())
                    *plTextID = ptp->TextID();
                else
                    *plTextID = ptp->NextTreePos()->TextID();
            }
        }
            
        long cchWant = (pcch && cchIn >= 0) ? cchIn : INT_MAX;
        long cchLook = cchWant;

        //
        // Here we move accross up to cchLook text or until we hit a node or
        // text with a different ID.
        //

        if (fLeft)
        {
            for ( ; ; )
            {
                if (ich > 0)
                {
                    if (plTextID && ptp->TextID() != * plTextID)
                        break;

                    long dcch2 = min( cchLook, ich );
                    cchLook -= dcch2;
                    ich -= dcch2;
                    dcch -= dcch2;

                    if (ich == 0)
                    {
                        ptp = ptp->PreviousTreePos();
                        ich = ptp->IsText() ? ptp->Cch() : 0;
                    }

                    if (cchLook == 0)
                        break;
                }
                else
                {
                    if (ptp->IsText())
                    {
                        if (plTextID)
                        {
                            long textID = ptp->TextID();

                            if (textID != 0 && textID != * plTextID)
                                break;
                        }
                    }
                    else if (!ptp->IsPointer())
                    {
                        Assert( ptp->IsNode() );
                        break;
                    }

                    ptp = ptp->PreviousTreePos();
                    ich = ptp->IsText() ? ptp->Cch() : 0;
                }
            }
        }
        else
        {
            for ( ; ; )
            {
                long cch;
                
                if (ptp->IsText() && ich < (cch = ptp->Cch()))
                {
                    if (plTextID && ptp->TextID() != * plTextID)
                        break;

                    long dcch2 = min( cchLook, cch - ich );
#if 0
                    //
                    // Special flag to stop at CR or LF in text.  Only works
                    // when going to the right.
                    //
                    
                    if (pdwFlags && (*pdwFlags & MPTR_STOPATCRLF))
                    {
                        long cp = GetCp() + dcch;
                        CTxtPtr txtPtr( Markup(), cp );
                        
                        if (txtPtr.FindCrOrLf( dcch2 ) )
                        {
                            *pdwFlags |= MPTR_FOUNDCRLF;
                            
                            Assert( long( txtPtr.GetCp() ) >= cp );
                            
                            dcch2 = txtPtr.GetCp() - cp + 1;
                            
                            cchLook -= dcch2;
                            ich += dcch2;
                            dcch += dcch2;

                            break;
                        }
                    }
#endif

                    cchLook -= dcch2;
                    ich += dcch2;
                    dcch += dcch2;

                    if (cchLook == 0)
                        break;
                }
                else
                {
                    CTreePos * ptpNext = ptp->NextTreePos();

                    if (ptpNext->IsText())
                    {
                        if (plTextID)
                        {
                            long textID = ptpNext->TextID();

                            if (textID != 0 && *plTextID != textID)
                                break;
                        }
                    }
                    else if (!ptpNext->IsPointer())
                    {
                        Assert( ptpNext->IsNode() );
                        break;
                    }

                    ptp = ptpNext;
                    ich = 0;
                }
            }

            //
            // Might be on a text node with ich == 0 which is only allowed if it is
            // an empty text id'ed node.
            //

            if (ich == 0 && ptp->IsText() && ptp->Cch())
            {
                ptp = ptp->PreviousTreePos();
                ich = ptp->IsText() ? ptp->Cch() : 0;
            }
        }

        long cchFound = cchWant - cchLook;

        //
        // Return the number of chars found
        //
        
        if (pcch)
            *pcch = cchFound;

        //
        // Return the text moved over
        //

        if (pchText && cchFound > 0 && pcch && cchIn > 0)
        {
            long cp = GetCp();

            if (fLeft)
                cp -= cchFound;

            Verify( CTxtPtr( Markup(), cp ).GetRawText( cchFound, pchText ) );
        }
    }

    //
    // The ptp/ich pair should now indicate where we should move
    //

    if (fMove)
    {
        hr = THR(
            MoveToReference(
                ptp, ich, Markup(), CpIsCached() ? _cpCache + dcch : -1 ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize("", on)
#endif

STDMETHODIMP
CMarkupPointer::MoveUnit ( MOVEUNIT_ACTION muAction )
{
    HRESULT   hr = S_OK;
    long      cp;
    long      newcp;
    CTxtPtr   tp;
    long      action;

    Validate();
            
    if (!IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }

    cp = GetCp();
    
    tp.Reinit( Markup(), cp );
    
    action = 0;
    
    switch( muAction )
    {
    case MOVEUNIT_PREVCHAR:
    case MOVEUNIT_NEXTCHAR:
        tp.MoveChar( muAction == MOVEUNIT_NEXTCHAR );
        break;
        
    case MOVEUNIT_PREVCLUSTERBEGIN:
    case MOVEUNIT_NEXTCLUSTERBEGIN:
        tp.MoveCluster( muAction == MOVEUNIT_NEXTCLUSTERBEGIN );
        break;
        
    case MOVEUNIT_PREVCLUSTEREND:
    case MOVEUNIT_NEXTCLUSTEREND:
        tp.MoveClusterEnd( muAction == MOVEUNIT_NEXTCLUSTEREND );
        break;
        
    case MOVEUNIT_PREVWORDBEGIN:
    case MOVEUNIT_NEXTWORDBEGIN:
    case MOVEUNIT_PREVWORDEND:
    case MOVEUNIT_NEXTWORDEND:
    case MOVEUNIT_PREVPROOFWORD:
    case MOVEUNIT_NEXTPROOFWORD:
        switch( muAction )
        {
            case MOVEUNIT_PREVWORDBEGIN:
                action = WB_MOVEWORDLEFT;
                break;
            case MOVEUNIT_NEXTWORDBEGIN:
                action = WB_MOVEWORDRIGHT;
                break;
            case MOVEUNIT_PREVWORDEND:
                action = WB_LEFTBREAK;
                break;
            case MOVEUNIT_NEXTWORDEND:
                action = WB_RIGHTBREAK;
                break;
            case MOVEUNIT_PREVPROOFWORD:
                action = WB_LEFT;
                break;
            case MOVEUNIT_NEXTPROOFWORD:
                action = WB_RIGHT;
                break;
        }
        tp.FindWordBreak( action );
        break;
        
    case MOVEUNIT_PREVURLBEGIN:
        if( !tp.FindUrl( FALSE, TRUE ) )
            tp.SetCp( cp );
        break;
        
    case MOVEUNIT_NEXTURLBEGIN:
        if( !tp.FindUrl( TRUE, TRUE ) )
            tp.SetCp( cp );
        break;
        
    case MOVEUNIT_PREVURLEND:
        if( !tp.FindUrl( FALSE, FALSE ) )
            tp.SetCp( cp );
        break;
        
    case MOVEUNIT_NEXTURLEND:
        if( !tp.FindUrl( TRUE, FALSE ) )
            tp.SetCp( cp );
        break;
        
    case MOVEUNIT_PREVSENTENCE:
    case MOVEUNIT_NEXTSENTENCE:
        tp.FindBOSentence( muAction == MOVEUNIT_NEXTSENTENCE );
        break;
        
    case MOVEUNIT_PREVBLOCK:
    case MOVEUNIT_NEXTBLOCK:
        tp.FindBlockBreak( muAction == MOVEUNIT_NEXTBLOCK );
        break;
        
#if DBG==1
    default:
        AssertSz( FALSE, "Invalid action" );
#endif
    }

    // If the tp moved somewhere, position us wherever it went
    newcp = tp.GetCp();

    // BUGBUG (johnbed) Due to a problem with moveunit, it is possible that
    // we will be compute a cp that is outside the document here. Fix it up
    // instead of asserting for now. This is raided bug assigned to TomFakes.
    // When it is fixed, this should be removed

    if( newcp < 1 )
        newcp = 1;
    
    if( newcp >= Markup()->Cch() )
        newcp = Markup()->Cch() -1;

    if( newcp != cp )
        hr = THR( MoveToCp( newcp, Markup() ) );
    else
        hr = S_FALSE;

Cleanup:

    RRETURN1( hr, S_FALSE );
}


STDMETHODIMP
CMarkupPointer::CurrentScope ( IHTMLElement ** ppElemCurrent )
{
    HRESULT hr = S_OK;
    CTreeNode * pNode;

    if (!ppElemCurrent)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppElemCurrent = NULL;
    
    pNode = CurrentScope();
    
    if (pNode)
    {
        hr = THR(
            pNode->GetElementInterface(
                IID_IHTMLElement, (void **) ppElemCurrent ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:

    RRETURN( hr );
}

STDMETHODIMP
CMarkupPointer::FindText (
    OLECHAR *        pchFindText, 
    DWORD            dwFlags,
    IMarkupPointer * pIEndMatch, /* =NULL */
    IMarkupPointer * pIEndSearch /* =NULL */)
{
    HRESULT hr;
    CMarkupPointer *pEndMatch  = NULL;
    CMarkupPointer *pEndSearch = NULL;

    if (!IsPositioned())
    {
        hr = CTL_E_UNPOSITIONEDPOINTER;
        goto Cleanup;
    }
    
    //
    // Convert arguments, if necessary.
    //

    // Move this pointer to the end of the match
    
    if (pIEndMatch)
    {
        Assert( _pDoc->IsOwnerOf( pIEndMatch ) );

        hr = THR( pIEndMatch->QueryInterface( CLSID_CMarkupPointer, (void **) & pEndMatch) );
        if( hr )
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    // Limit our search to this pointer.
    
    if (pIEndSearch)
    {
        hr = THR( pIEndSearch->QueryInterface( CLSID_CMarkupPointer, (void **) & pEndSearch ) );
        
        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
    }

    hr = FindText( pchFindText, dwFlags, pEndMatch, pEndSearch ) ? S_OK : S_FALSE;

Cleanup:
    
    RRETURN1( hr, S_FALSE );
}

HRESULT
CMarkupPointer::SetTextIdentity ( CMarkupPointer * pPointerFinish, long * plTextID )
{
    HRESULT          hr = S_OK;
    CMarkupPointer * pPointerStart = this;
    CTreePosGap      tpgBegin, tpgEnd;

    Assert(
        IsPositioned() && pPointerFinish->IsPositioned() &&
        Markup() == pPointerFinish->Markup() );

#if DBG == 1
    Validate();
    if (pPointerFinish)
        pPointerFinish->Validate();
#endif

    hr = THR( Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    EnsureLogicalOrder( pPointerStart, pPointerFinish );

    Verify( ! tpgBegin.MoveTo( pPointerStart->GetEmbeddedTreePos(), TPG_LEFT ) );
    Verify( ! tpgEnd.MoveTo( pPointerFinish->GetEmbeddedTreePos(), TPG_LEFT ) );

    hr = THR( Markup()->SetTextID( & tpgBegin, & tpgEnd, plTextID ) );
    
    if (hr)
        goto Cleanup;

Cleanup:
    
    RRETURN( hr );
}

HRESULT
CMarkupPointer::FindTextIdentity ( long textID, CMarkupPointer * pPointerOtherEnd )
{
    HRESULT     hr = S_OK;
    CTreePosGap tpgBegin ( TPG_RIGHT );
    CTreePosGap tpgEnd ( TPG_LEFT );

#if DBG == 1
    Validate();
    if (pPointerOtherEnd)
        pPointerOtherEnd->Validate();
#endif

    if (!IsPositioned())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    //
    // BUGBUG
    // BUGBUG This needs to be rewritten to not assume pointer pos's
    // BUGBUG
    //

    hr = THR( Markup()->EmbedPointers() );

    if (hr)
        goto Cleanup;

    Verify( ! tpgBegin.MoveTo( GetEmbeddedTreePos(), TPG_LEFT ) );

    hr = THR( Markup()->FindTextID( textID, & tpgBegin, & tpgEnd ) );
    
    if (hr == S_FALSE)
        goto Cleanup;
    
    if (hr)
        goto Cleanup;

    hr = THR( MoveToGap( & tpgBegin, Markup() ) );

    if (hr)
        goto Cleanup;

    if (pPointerOtherEnd)
    {
        hr = THR( pPointerOtherEnd->MoveToGap( & tpgEnd, Markup() ) );
        
        if (hr)
            goto Cleanup;
    }

Cleanup:
    
    RRETURN1( hr, S_FALSE );
}


HRESULT
CMarkupPointer::IsInsideURL( IMarkupPointer * pIRight, BOOL * pfResult )
{
    HRESULT          hr = S_OK;
    CTxtPtr          tpThis, tpRight;
    BOOL             fFound  = FALSE;
    long             cpStart;
    long             cpEnd;

    if (!IsPositioned() || !pfResult || !pIRight || !_pDoc->IsOwnerOf( pIRight ))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    tpThis.Reinit( Markup(), GetCp() );
    tpRight.Reinit( Markup(), GetCp() );
    
    if (tpThis.IsInsideUrl( & cpStart, & cpEnd ))
    {
        long cchOffset;
        CMarkupPointer * pRight;
        CTreePos * ptp;
        
        hr = THR( pIRight->QueryInterface( CLSID_CMarkupPointer, (void **) & pRight ) );

        if (hr)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

#if DBG==1
        SetDebugName(_T("Start Url"));
        pRight->SetDebugName(_T("End Url"));
#endif
        ptp = Markup()->TreePosAtCp( cpEnd, & cchOffset );

        hr = THR( MoveToCp( cpStart, Markup() ) );

        if (hr)
            goto Cleanup;

        if (ptp->IsNode())
        {
            CTreePosGap tpg ( ptp, TPG_LEFT );
        
            tpg.Move( TPG_LEFT, TPG_VALIDGAP | TPG_OKNOTTOMOVE );
        
            pRight->MoveToGap( & tpg, Markup() );
        }
        else
        {  
            pRight->MoveToCp( cpEnd, Markup() );
        }
        
        fFound = TRUE;
    }

Cleanup:
    
    *pfResult = fFound;
    
    RRETURN( hr );
}

long
CMarkupPointer::GetCpSlow ( ) const
{
    if (!IsPositioned())
        return -1;
    
    if (_fEmbedded)
        return _ptpEmbeddedPointer->GetCp();
    
    if (_ptpRef->IsText())
    {
        Assert( _ichRef >= 0 && _ichRef <= _ptpRef->Cch() );
        return _ptpRef->GetCp() + _ichRef;
    }
    
    Assert( _ichRef == 0 );
    
    return _ptpRef->GetCp() + _ptpRef->GetCch();
}

#define COMPARE(TYPE)                                                                      \
HRESULT                                                                                    \
CMarkupPointer::TYPE ( IMarkupPointer * pIPointerThat, BOOL * pfResult )                   \
{                                                                                          \
    HRESULT          hr;                                                                   \
    CMarkupPointer * pPointerThat;                                                         \
                                                                                           \
    if (!pIPointerThat || !pfResult)                                                       \
    {                                                                                      \
        hr = E_INVALIDARG;                                                                 \
        goto Cleanup;                                                                      \
    }                                                                                      \
                                                                                           \
    hr = pIPointerThat->QueryInterface( CLSID_CMarkupPointer, (void * *) & pPointerThat ); \
                                                                                           \
    if (hr)                                                                                \
    {                                                                                      \
        hr = E_INVALIDARG;                                                                 \
        goto Cleanup;                                                                      \
    }                                                                                      \
                                                                                           \
    if (!IsPositioned() || !pPointerThat->IsPositioned())                                  \
    {                                                                                      \
        hr = CTL_E_UNPOSITIONEDPOINTER;                                                    \
        goto Cleanup;                                                                      \
    }                                                                                      \
                                                                                           \
    if (Markup() != pPointerThat->Markup())                                                \
    {                                                                                      \
        hr = CTL_E_INCOMPATIBLEPOINTERS;                                                   \
        goto Cleanup;                                                                      \
    }                                                                                      \
                                                                                           \
    *pfResult = TYPE( pPointerThat );                                                      \
                                                                                           \
Cleanup:                                                                                   \
                                                                                           \
    RRETURN( hr );                                                                         \
}

COMPARE( IsLeftOf )
COMPARE( IsLeftOfOrEqualTo )
COMPARE( IsRightOf )
COMPARE( IsRightOfOrEqualTo )

#undef COMPARE

//
// IsEqualTo is different from the others because it is capable of
// dealing with unpositioned or incompatible markup pointers.
//

HRESULT
CMarkupPointer::IsEqualTo ( IMarkupPointer * pIPointerThat, BOOL * pfResult )
{
    HRESULT          hr;
    CMarkupPointer * pPointerThat;

    if (!pIPointerThat || !pfResult)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = pIPointerThat->QueryInterface( CLSID_CMarkupPointer, (void * *) & pPointerThat );

    if (hr)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (!IsPositioned() || !pPointerThat->IsPositioned() || Markup() != pPointerThat->Markup())
    {
        *pfResult = FALSE;
        goto Cleanup;
    }
    
    *pfResult = IsEqualTo( pPointerThat );

Cleanup:

    RRETURN( hr );
}

HRESULT
OldCompare ( IMarkupPointer * p1, IMarkupPointer * p2, int * piResult )
{
    HRESULT hr = S_OK;
    BOOL    fResult;

    Assert( piResult );

    hr = THR( p1->IsEqualTo( p2, & fResult ) );

    if (hr)
        goto Cleanup;

    if (fResult)
    {
        *piResult = 0;
        goto Cleanup;
    }

    hr = THR( p1->IsLeftOf( p2, & fResult ) );

    if (hr)
        goto Cleanup;

    *piResult = fResult ? -1 : 1;

Cleanup:

    RRETURN( hr );
}

int
OldCompare ( CMarkupPointer * p1, CMarkupPointer * p2 )
{
    if (p1->IsEqualTo( p2))
        return 0;

    return p1->IsLeftOf( p2 ) ? -1 : 1;
}

void
CMarkupPointer::SetKeepMarkupAlive ( BOOL fKeepAlive )
{
    if (!!fKeepAlive == !!_fKeepMarkupAlive)
        return;

    _fKeepMarkupAlive = !!fKeepAlive;

    if (Markup())
    {
        if (_fKeepMarkupAlive)
            Markup()->PrivateAddRef();
        else
            Markup()->PrivateRelease();
    }
}

void
CMarkupPointer::SetAlwaysEmbed ( BOOL fAlwaysEmbed )
{
    if (!!fAlwaysEmbed == !!_fAlwaysEmbed)
        return;

    _fAlwaysEmbed = !!fAlwaysEmbed;

    if (IsPositioned() && !_fEmbedded && _fAlwaysEmbed)
        IGNORE_HR( Embed( Markup(), _ptpRef, _ichRef, CpIsCached() ? _cpCache : -1 ) );
}

HRESULT
CMarkupPointer::MoveAdjacentToElement ( CElement * pElement, ELEMENT_ADJACENCY adj )
{
    HRESULT hr = S_OK;
    CTreePos * ptp = NULL;
    BOOL fBefore = adj == ELEM_ADJ_BeforeBegin || adj == ELEM_ADJ_BeforeEnd;
    BOOL fBegin  = adj == ELEM_ADJ_BeforeBegin || adj == ELEM_ADJ_AfterBegin;
    TPG_DIRECTION eDir = fBefore ? TPG_LEFT : TPG_RIGHT;
    CTreePosGap tpg;

    Assert( pElement && IsValidAdjacency( adj ) );
    Assert( pElement->IsInMarkup() );
    
    if (pElement->IsNoScope() &&
        (adj == ELEM_ADJ_AfterBegin || adj == ELEM_ADJ_BeforeEnd))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // find the TreePos where we're supposed to start looking
    
    pElement->GetTreeExtent(
        fBegin ? & ptp : NULL, fBegin ? NULL : & ptp );

    Assert( ptp );
    
    // move to the nearest legal position
    
    tpg.SetMoveDirection( eDir );
    
    hr = THR( tpg.MoveTo( ptp, eDir ) );

    if (hr)
        goto Cleanup;
    
    hr = THR( tpg.Move( TPG_VALIDGAP | TPG_OKNOTTOMOVE ) );

    if (hr)
        goto Cleanup;
    
    hr = THR( MoveToGap( & tpg, pElement->GetMarkup() ) );

Cleanup:

    RRETURN( hr );
}

HRESULT
CMarkupPointer::MoveToContainer ( CMarkup * pMarkup, BOOL fBegin, DWORD dwFlags )
{
    HRESULT     hr;
    CTreePos *  ptp = NULL;
    CTreePosGap tpg;
    CTreeNode * pNode;
    MARKUP_CONTEXT_TYPE context;

    Assert( pMarkup );
    Assert( pMarkup->Root() && pMarkup->Root()->Tag() == ETAG_ROOT );

    pMarkup->Root()->GetTreeExtent( fBegin ? & ptp : NULL, fBegin ? NULL : & ptp );

    Assert( ptp );

    hr = THR( tpg.MoveTo( ptp, fBegin ? TPG_RIGHT : TPG_LEFT ) );
    
    if (hr)
        goto Cleanup;

    hr = THR( MoveToGap( & tpg, pMarkup ) );

    if (!(dwFlags & MPTR_SHOWSLAVE) && pMarkup->Master())
    {
        //
        // move inside TEXTSLAVE if present (as if the TEXTSLAVE element were not present - dbau)
        //
        
        hr = THR( There( ! fBegin, FALSE, &context, &pNode, NULL, NULL, NULL, 0 ) );
        
        if (hr)
            goto Cleanup;

        if (context == CONTEXT_TYPE_EnterScope && pNode && pNode->Tag() == ETAG_TXTSLAVE)
        {
            hr = THR( There( ! fBegin, TRUE, NULL, NULL, NULL, NULL, NULL, 0 ) );
            
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:

    RRETURN( hr );
}


HRESULT
CMarkupPointer::MoveToPointer ( CMarkupPointer * pPointerThat )
{
    HRESULT    hr;
    CTreePos * ptp;
    long       ich;

    Validate();
            
    Assert( pPointerThat );
    Assert( pPointerThat->IsPositioned() );

    ptp = pPointerThat->GetNormalizedReference( ich );

    hr = THR(
        MoveToReference(
            ptp, ich, pPointerThat->Markup(),
            pPointerThat->CpIsCached() ? pPointerThat->_cpCache : -1 ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

void
CMarkupPointer::OnPositionReleased ( )
{
    Assert( _fEmbedded );
    Assert( ! _pmpNext && ! _pmpPrev );
    Assert( Markup() );
    Assert( _ptpEmbeddedPointer );
    Assert( _ichRef == 0 );

    _ptpEmbeddedPointer = NULL;
    _ichRef = 0;
    _fEmbedded = FALSE;
    SetMarkup(NULL);
    WHEN_DBG( _verCp = 0; )
    WHEN_DBG( _cpCache = -1; )
}

void 
CMarkupPointer::SetMarkup( CMarkup * pMarkup )
{
    if (_fKeepMarkupAlive)
    {
        CMarkup * pMarkupOld = _pMarkup;

        _pMarkup = pMarkup;

        if (_pMarkup)
            _pMarkup->PrivateAddRef();

        if (pMarkupOld)
            pMarkupOld->PrivateRelease();
    }
    else
    {
        _pMarkup = pMarkup;
    }
}

CTreeNode *
CMarkupPointer::CurrentScope ( DWORD dwFlags )
{
    CTreeNode * pNode;

    Validate();
    
    if (!IsPositioned())
        return NULL;

    pNode = Branch();

    if (pNode)
    {
        if (pNode->Tag() == ETAG_ROOT)
            return NULL;

        if (!(dwFlags & MPTR_SHOWSLAVE) && pNode->Tag() == ETAG_TXTSLAVE)
            return NULL;
    }

    return pNode;
}


BOOL
CMarkupPointer::FindText(
    TCHAR *          pchFindText, 
    DWORD            dwFlags, 
    CMarkupPointer * pEndMatch,
    CMarkupPointer * pEndSearch )
{
    long       cp;
    long       cpLimit = -1;
    CTxtPtr    tp;
    HRESULT    hr;

    Validate();
            
    Assert( IsPositioned() );
    
    cp = GetCp();
    
    tp.Reinit( Markup(), cp );

    if (pEndSearch)
    {
        Assert( pEndSearch->Markup() == Markup() );

        cpLimit = pEndSearch->GetCp();

        // Set direction based on the pointer, overriding flags
        // passed in, if necessary.
        
        if(cpLimit < cp)
            dwFlags |= FINDTEXT_BACKWARDS;
        else
            dwFlags &= ~FINDTEXT_BACKWARDS;
    }

    // ask the TP to find the text
    
    cp = tp.FindText( cpLimit, dwFlags, pchFindText, _tcslen( pchFindText ) );

    // if it succeeded, move myself accordingly

    if (cp < 0)
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    hr = THR( MoveToCp( tp._cp, Markup() ) );

    if (hr)
        goto Cleanup;

    // Set end pointer, if requested

    if (pEndMatch)
    {
        hr = THR( pEndMatch->MoveToCp( cp, Markup() ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:
    
    return hr == S_OK;
}

HRESULT 
CMarkupPointer::QueryBreaks ( DWORD * pdwBreaks )
{
    HRESULT          hr = S_OK;
    CTreePos *       ptp;
    long             ich;
    CTreePosGap      tpg;
    CLineBreakCompat breaker;

    Validate();
            
    Assert( pdwBreaks );

    *pdwBreaks = BREAK_NONE;

    if (!IsPositioned())
        goto Cleanup;

    ptp = GetNormalizedReference( ich );

    //
    // No breaks inside a text pos.  Also, we can't
    // position a gap inside a text pos.
    //

    if (ich > 0 && ich < ptp->Cch())
        goto Cleanup;
    
    hr = THR(
        tpg.MoveTo(
            ptp,
            ptp->IsText()
                ? (ich == 0 ? TPG_LEFT : TPG_RIGHT)
                : TPG_RIGHT ) );

    if (hr)
        goto Cleanup;

    hr = THR( breaker.QueryBreaks( & tpg, pdwBreaks ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

HRESULT
CMarkupPointer::MoveToOrphan ( CTreePos * ptp )
{
    HRESULT hr;
    
    Assert( ptp && ptp->IsPointer() && !ptp->MarkupPointer() );

    // if the pointer was already active, delete its old position

    hr = THR( Unposition() );

    if (hr)
        goto Cleanup;

    //
    // remember the new position
    //

    ptp->SetMarkupPointer( this );

    SetMarkup( ptp->GetMarkup() );
    _ptpEmbeddedPointer = ptp;
    _fEmbedded = TRUE;

    _cpCache = -1;
    _verCp = 0;
    
    _fRightGravity = ptp->Gravity();
    _fCling = ptp->Cling();

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

HRESULT
CMarkupPointer::MoveToReference (
    CTreePos * ptp, long ich, CMarkup * pMarkup, long cpNew )
{
    HRESULT hr;

    Assert( ptp && (ich == 0 || ich <= ptp->Cch()) );
    Assert( pMarkup && pMarkup == ptp->GetMarkup() );

    //
    // Make sure the ptp/ich are properly adjusted
    //

    while ( ptp->IsPointer() || (ich == 0 && ptp->IsText() && ptp->TextID() == 0) )
    {
        ptp = ptp->PreviousTreePos();
        ich = ptp->IsText() ? ptp->Cch() : 0;
    }

    Assert( !ptp->IsText() || ich != 0 || ptp->Cch() == 0 );

    hr = THR( UnEmbed( & ptp, & ich ) );

    if (hr)
        goto Cleanup;

    Assert( ! _fEmbedded );

    if (Markup() && Markup() != pMarkup)
    {
        RemoveMeFromList();
        SetMarkup( NULL );
    }

#if DBG == 1
    if (_fAlwaysEmbed || IsTagEnabled( tagMarkupPointerAlwaysEmbed ))
#else
    if (_fAlwaysEmbed)
#endif
    {
        if (Markup())
        {
            RemoveMeFromList();
            SetMarkup( NULL );
        }
        
        hr = THR( Embed( pMarkup, ptp, ich, cpNew ) );

        if (hr)
            goto Cleanup;
    }
    else
    {
        if (!Markup())
        {
            SetMarkup( pMarkup );
            AddMeToList();
        }

        _ptpRef = ptp;
        _ichRef = ich;

        if (cpNew == -1)
        {
            _verCp = 0;
            _cpCache = -1;
        }
        else
        {
            _cpCache = cpNew;
            _verCp = Markup()->GetMarkupContentsVersion();
            Assert( _cpCache == GetCpSlow() );
        }

        Assert( !_fEmbedded );
    }

    Validate();

Cleanup:

    RRETURN( hr );
}


HRESULT
CMarkupPointer::MoveToGap (
    CTreePosGap * ptpg, CMarkup * pMarkup, BOOL fForceEmbedding )
{
    HRESULT    hr = S_OK;
    CTreePos * ptp;
    long       ich;

    Validate();
            
    Assert( ptpg && pMarkup );
    Assert( ptpg->GetAttachedMarkup() );
    Assert( ptpg->GetAttachedMarkup() == pMarkup );

    //
    // Suck the position out of the gap, then unposition the gap
    // because an embedding may be removed, one inserted or both.
    // All uses of MoveToGap must expect the gap to be unpositioned
    // upon return.
    //

    ptp = ptpg->AdjacentTreePos( TPG_LEFT );
    ich = ptp->IsText() ? ptp->Cch() : 0;

    ptpg->UnPosition();

    //
    // Make sure the ptp/ich are properly adjusted.  Only do this
    // if not forcing an embedding.  Forcing an embedding is used
    // to place an embedded pointer exactly at the place requested.
    //

    if (!fForceEmbedding && !_fAlwaysEmbed)
    {
        while ( ptp->IsPointer() || (ich == 0 && ptp->IsText() && ptp->TextID() == 0) )
        {
            ptp = ptp->PreviousTreePos();
            ich = ptp->IsText() ? ptp->Cch() : 0;
        }
        
        Assert( ! ptp->IsText() || ich != 0 || 0 == ptp->Cch() );

        hr = THR( MoveToReference( ptp, ich, pMarkup, -1 ) );

        if (hr)
            goto Cleanup;

        goto Cleanup;
    }

    if (IsPositioned())
    {
        if (_fEmbedded)
        {
            //
            // If this pointer is already embedded at the correct
            // location, then there is nothing to do!
            //
            
            if (ptp == _ptpEmbeddedPointer)
                goto Cleanup;
            
            //
            // Remove any existing embedding, making sure to update the
            // target ptp/ich.
            //

            hr = THR( UnEmbed( & ptp, & ich ) );

            if (hr)
                goto Cleanup;
        }

        RemoveMeFromList();
        SetMarkup(NULL);
    }

    hr = THR( Embed( pMarkup, ptp, ich, -1 ) );

    if (hr)
        goto Cleanup;
    
Cleanup:

    Validate();
    
    RRETURN( hr );
}

HRESULT
CMarkupPointer::Embed ( CMarkup * pMarkup, CTreePos * ptp, long ich, long cpNew )
{
    HRESULT hr = S_OK;
    CTreePos * ptpNew;
    
    Assert( ! IsPositioned() );
    Assert( ! _fEmbedded );

    Assert( pMarkup && ptp->GetMarkup() == pMarkup );

    //
    // See if we need to split a text pos
    //

    if (ich > 0 && ich < ptp->Cch())
    {
        CMarkupPointer * pmp;

        hr = THR( pMarkup->Split( ptp, ich ) );

        if (hr)
            goto Cleanup;

        //
        // See if other unembedded pointers were pointing at this text pos
        // after where we split it.  Update those where were.
        //

        for ( pmp = pMarkup->_pmpFirst ; pmp ; pmp = pmp->_pmpNext )
        {
            if (pmp->_ptpRef == ptp && pmp->_ichRef > ich)
            {
                pmp->_ichRef -= ich;
                pmp->_ptpRef = ptp->NextTreePos();
            }
        }

#if DBG == 1
        for ( pmp = pMarkup->_pmpFirst ; pmp ; pmp = pmp->_pmpNext )
            pmp->Validate();
#endif
    }

    //
    // Make a pointer pos and put it in the right place
    //

    ptpNew = pMarkup->NewPointerPos( this, Gravity(), Cling() );

    if (!ptpNew)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR( pMarkup->Insert( ptpNew, ptp, FALSE ) );

    if (hr)
        goto Cleanup;

    SetMarkup( pMarkup );
    _ptpEmbeddedPointer = ptpNew;
    _ichRef = 0;
    _fEmbedded = TRUE;

    _cpCache = 0;
    _verCp = -1;

    Validate();

Cleanup:

    RRETURN( hr );
}

#if DBG!=1
#pragma optimize("", on)
#endif


HRESULT
CMarkupPointer::MoveToCp ( long cp, CMarkup * pMarkup )
{
    HRESULT    hr = S_OK;
    CTreePos * ptp;
    long       ich;

    Validate();
            
    ptp = pMarkup->TreePosAtCp( cp, & ich );

    //
    // TreePosAtCp gives a ptp before the given cp.  Markup pointers refer to
    // a ptp as to pointing after.
    //

    if (!ptp->IsText() || ich == 0)
    {
        ptp = ptp->PreviousTreePos();

        while ( ptp->IsPointer() )
            ptp = ptp->PreviousTreePos();

        ich = ptp->IsText() ? ptp->Cch() : 0;
    }
    
    //
    // make sure the ptp is not in an inclusion
    //

    if (ptp->IsNode() && !ptp->IsEdgeScope() && ptp->IsEndNode())
    {
        while (ptp->IsNode() && !ptp->IsEdgeScope() && ptp->IsEndNode())
            ptp = ptp->PreviousTreePos();

        if (ptp->IsText())
            ich = ptp->Cch();
    }
    else if (ptp->IsNode())
    {
        CTreePos * ptpNext = ptp->NextTreePos();
        
        if (ptpNext->IsNode() && !ptpNext->IsEdgeScope() && ptpNext->IsBeginNode())
        {
            while (ptpNext->IsNode() && !ptpNext->IsEdgeScope() && ptpNext->IsBeginNode())
            {
                ptp = ptpNext;
                ptpNext = ptp->NextTreePos();
            }
        }
    }

    //
    // Make sure we're not positioned after a pointer
    //

    if (ptp->IsPointer())
    {
        while ( ptp->IsPointer() )
            ptp = ptp->PreviousTreePos();

        ich = ptp->IsText() ? ptp->Cch() : 0;
    }
    
    //
    // Make sure were no in the middle of a noscope
    //

    if (ptp->IsNode() && ptp->IsBeginElementScope() &&
        ptp->Branch()->Element()->IsNoScope())
    {
        ptp = ptp->Branch()->GetEndPos();
    }

    //
    //
    //
    
    Assert( ptp->IsNode() || ptp->IsText() );
    Assert( ich >= 0 && ich <= ptp->GetCch() );
    Assert( pMarkup == ptp->GetMarkup() );

    hr = THR( MoveToReference( ptp, ich, pMarkup, -1 ) );

    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( hr );
}

//
// GetBookmark/MoveToBookmark stuff - these are mainly pass-thrus to 
//  functions on CTxtPtr.
//

HRESULT
CMarkupPointer::MoveToBookmark ( BSTR bstrBookmark, CMarkupPointer * pEnd )
{
    HRESULT   hr = S_OK;
    CMarkup * pMarkup;

    Validate();
            
    Assert( pEnd );
    Assert( IsPositioned() );

    pMarkup = Markup();

    {
        CTxtPtr tpLeft ( pMarkup );
        CTxtPtr tpRight ( tpLeft );
        
        hr = THR( tpLeft.MoveToBookmark( bstrBookmark, & tpRight ) );

        if (hr)
            goto Cleanup;

        hr = THR( MoveToCp( tpLeft._cp, pMarkup ) );

        if (hr)
            goto Cleanup;
    
        hr = THR( pEnd->MoveToCp( tpRight._cp, pMarkup ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:
    
    RRETURN1( hr, S_FALSE );
}


HRESULT
CMarkupPointer::GetBookmark ( BSTR * pbstrBookmark, CMarkupPointer * pEnd )
{
    HRESULT   hr      = S_OK;
    CMarkup * pMarkup = Markup();
    long      cp      = GetCp();

    Validate();
            
    Assert( pEnd );
    Assert( IsPositioned() && pEnd->IsPositioned() );
    Assert( Markup() == pEnd->Markup() );
    Assert( pbstrBookmark );

    {
        CTxtPtr tpThis ( pMarkup, cp );
        CTxtPtr tpEnd ( tpThis );
        
        tpEnd.SetCp( pEnd->GetCp() );

        hr = THR( tpThis.GetBookmark( pbstrBookmark, & tpEnd ) );

        if (hr)
            goto Cleanup;
    }

Cleanup:
    
    RRETURN( hr );
}

