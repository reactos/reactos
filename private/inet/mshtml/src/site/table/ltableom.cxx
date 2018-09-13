//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltableom.cxx
//
//  Contents:   CTableLayout object model methods.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx" // CTreePosList
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif


#ifndef X_MARKUPUNDO_HXX_
#define X_MARKUPUNDO_HXX_
#include "markupundo.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  Member: CTableLayout::createTHead
//
//  Synopsis:   Table Layout OM method helper
//
//  Arguments:  ppHead - return value
//
//----------------------------------------------------------------------------

HRESULT 
CTableLayout::createTHead(IDispatch** ppHead)
{
    HRESULT               hr;
    CElement  *           pAdjacentElement = NULL;
    CElement::Where       where;
    CElement  *           pElement = NULL;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (!_pHead)
    {
        hr = Doc()->CreateElement(ETAG_THEAD, &pElement);
        if (hr)
            goto Cleanup;

        Assert (pElement);

        hr = ensureTBody();
        if (hr)
            goto Cleanup;

        pAdjacentElement = _pFoot? _pFoot : _aryBodys[0];    // insert right before 1st body, or before the footer
        where = CElement::BeforeBegin;

        hr = insertElement(pAdjacentElement, pElement, where, TRUE);

        if (hr)
            goto Cleanup;
        
        Assert (pElement == _pHead);
    }
    
    if (ppHead)
    {
        hr = _pHead->QueryInterface(IID_IHTMLTableSection, (void **)ppHead);
    }

Cleanup:

    CElement::ReleasePtr(pElement);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CTableLayout::deleteTHead
//
//  Synopsis:   Table Layout OM method helper
//
//----------------------------------------------------------------------------

HRESULT 
CTableLayout::deleteTHead()
{
    HRESULT hr = EnsureTableLayoutCache();

    if (hr)
        goto Cleanup;

    if (_pHead)
    {
        hr = deleteElement(_pHead);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CTableLayout::createTFoot
//
//  Synopsis:   Table Layout OM method helper
//
//  Arguments:  ppHead - return value
//
//----------------------------------------------------------------------------

HRESULT 
CTableLayout::createTFoot(IDispatch** ppFoot)
{
    HRESULT               hr;
    CElement  *           pAdjacentElement = NULL;
    CElement::Where       where;
    CElement  *           pElement = NULL;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (!_pFoot)
    {
        hr = Doc()->CreateElement(ETAG_TFOOT, &pElement);
        if (hr)
            goto Cleanup;

        Assert (pElement);
        hr = ensureTBody();
        if (hr)
            goto Cleanup;

        pAdjacentElement = _aryBodys[0];    // insert right before the first body
        where = CElement::BeforeBegin;

        hr = insertElement(pAdjacentElement, pElement, where, TRUE);

        if (hr)
            goto Cleanup;
        
        Assert (pElement == _pFoot);
    }
    
    if (ppFoot)
    {
        hr = _pFoot->QueryInterface(IID_IHTMLTableSection, (void **)ppFoot);
    }

Cleanup:

    CElement::ReleasePtr(pElement);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableLayout::ensureTBody
//
//  Synopsis:   Table Layout OM method helper
//
//
//----------------------------------------------------------------------------

HRESULT 
CTableLayout::ensureTBody()
{
    HRESULT               hr = S_OK;
    CElement  *           pAdjacentElement = NULL;
    CElement::Where       where;
    CElement  *           pElement = NULL;

    if (!_aryBodys.Size())
    {
        hr = Doc()->CreateElement(ETAG_TBODY, &pElement);
        if (!hr)
        {
            Assert (pElement);
            pAdjacentElement = Table();    // insert right before the end of table
            where = CElement::BeforeEnd;
            _fEnsureTBody = TRUE;
            hr = insertElement(pAdjacentElement, pElement, where, TRUE);
            _fEnsureTBody = FALSE;
        }
    }
    
    CElement::ReleasePtr(pElement);
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member: CTableLayout::deleteTFoot
//
//  Synopsis:   Table Layout OM method helper
//
//----------------------------------------------------------------------------

HRESULT 
CTableLayout::deleteTFoot()
{
    HRESULT hr = EnsureTableLayoutCache();

    if (hr)
        goto Cleanup;

    if (_pFoot)
    {
        hr = deleteElement(_pFoot);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CTableLayout::createCaption
//
//  Synopsis:   Table Layout OM method helper
//
//----------------------------------------------------------------------------

HRESULT 
CTableLayout::createCaption(IHTMLTableCaption** ppCaption)
{
    HRESULT         hr;
    CElement      * pNewElement = NULL;
    CTableCaption * pCaption;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    pCaption = GetFirstCaption();
    if (!pCaption)
    {
        hr = Doc()->CreateElement(ETAG_CAPTION, &pNewElement);
        if (hr)
            goto Cleanup;

        Assert (pNewElement);

        // If there are no captions then insert right after the beginning of the table.
        hr = insertElement(ElementOwner(), pNewElement, CElement::AfterBegin, TRUE);

        if (hr)
            goto Cleanup;

        Assert (pNewElement == _aryCaptions[0]);

        pCaption = DYNCAST(CTableCaption, pNewElement);
    }
    
    if (ppCaption)
    {
        hr = pCaption->QueryInterface(IID_IHTMLTableCaption, (void **)ppCaption);
    }

Cleanup:
    CElement::ReleasePtr(pNewElement);

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member: CTableLayout::deleteCaption
//
//  Synopsis:   Table Layout OM method helper
//
//----------------------------------------------------------------------------

HRESULT 
CTableLayout::deleteCaption()
{
    CTableCaption * pCaption;
    HRESULT         hr;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    pCaption = GetFirstCaption();

    if (pCaption)
    {
        hr = deleteElement(pCaption);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     VisualRow2Index (helper function)
//
//  Synopsis:   converts visual row index to an index into an _aryRows.
//
//----------------------------------------------------------------------------

int
CTableLayout::VisualRow2Index(int iRow)
{
    int cRowsFoot = _pFoot? _pFoot->_cRows : 0;

    Assert(IsTableLayoutCacheCurrent());

    if (cRowsFoot)
    {
        int cRowsHead = _pHead? _pHead->_cRows : 0;
        if (iRow >= cRowsHead)
        {
            int cRowsTotal= GetRows();
            if (iRow >= cRowsTotal - cRowsFoot)
            {
                // it is a footer row
                iRow += _pFoot->_iRow - (cRowsTotal - cRowsFoot);
            }
            else
            {
                // it is a body row
                iRow += cRowsFoot;
            }
        }
        // else it is a header row
    }// else there is nothing to do

    return iRow;
}


HRESULT
CTableLayout::insertElement(CElement *pAdjacentElement, 
                            CElement *pInsertElement, 
                            CElement::Where where, 
                            BOOL fIncrementalUpdatePossible)
{
    HRESULT     hr;
    CDoc* pDoc = Doc();
    CParentUndo pu( pDoc  );

    Assert (pAdjacentElement);
    Assert (pInsertElement);

    if( IsEditable() )
    {
        pu.Start( IDS_UNDONEWCTRL );
        CSelectionUndo Undo( pDoc->_pElemCurrent, pDoc->GetCurrentMarkup() );  
    }
    
    _fTableOM = fIncrementalUpdatePossible;
    hr = pAdjacentElement->InsertAdjacent(where, pInsertElement);
    _fTableOM = FALSE;
    if (!hr)
    {
        Fixup(fIncrementalUpdatePossible);

        // VID wants us to save out end tags to elements inserted via the TOM (IE5, 23789).
        pInsertElement->_fExplicitEndTag = TRUE;
    }
    
    {
        CDeferredSelectionUndo DeferredUndo( pDoc->GetCurrentMarkup() );
    } 
    pu.Finish( hr );

    RRETURN (hr);
}


HRESULT
CTableLayout::deleteElement(CElement *pDeleteElement, 
                            BOOL fIncrementalUpdatePossible)
{
    HRESULT     hr;
    BOOL        fInBrowseMode = !IsEditable();
    CDoc* pDoc = Doc();
    CParentUndo pu( pDoc );
    
    Assert (pDeleteElement);

    if( !fInBrowseMode )
    {
        pu.Start( IDS_UNDODELETE );   
        CSelectionUndo Undo( pDoc->_pElemCurrent, pDoc->GetCurrentMarkup() );                        
    }

    _fTableOM = fIncrementalUpdatePossible;
    hr = pDeleteElement->RemoveOuter();
    _fTableOM = FALSE;
    if (!hr)
    {
        Fixup(fIncrementalUpdatePossible);
    }


    {
        CDeferredSelectionUndo DeferredUndo( pDoc->GetCurrentMarkup() );
    } 
    
    pu.Finish( hr );

    RRETURN (hr);
}


HRESULT
CTableLayout::moveElement( IMarkupServices * pMarkupServices,
                           IMarkupPointer  * pmpBegin,
                           IMarkupPointer  * pmpEnd,
                           IMarkupPointer  * pmpTarget )
{
    HRESULT     hr;
    CDoc* pDoc = Doc();
    CParentUndo pu( pDoc );

    Assert(pMarkupServices && pmpBegin && pmpEnd && pmpTarget);

    if( IsEditable() )
    {
        pu.Start( IDS_UNDONEWCTRL );
        CSelectionUndo Undo( pDoc->_pElemCurrent, pDoc->GetCurrentMarkup() );  
    }
    hr = THR(pMarkupServices->Move(pmpBegin, pmpEnd, pmpTarget));
    if (!hr)
    {
        Fixup();
    }
    
    {
        CDeferredSelectionUndo DeferredUndo( pDoc->GetCurrentMarkup() );
    } 

    pu.Finish( hr );
    
    RRETURN (hr);
}



