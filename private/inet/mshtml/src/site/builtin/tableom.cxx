//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       tableom.cxx
//
//  Contents:   CTable object model and interface implementations
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_CLRNGPRS_HXX_
#define X_CLRNGPRS_HXX_
#include <clrngprs.hxx>
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

#ifndef X_UNDO_HXX_
#define X_UNDO_HXX_
#include "undo.hxx"
#endif

#if defined(_M_ALPHA)
#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif
#endif

//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::createTHead
//
//  Synopsis:   Table OM method implementation
//
//  Arguments:  ppHead - return value
//
//----------------------------------------------------------------------------

HRESULT
CTable::createTHead(IDispatch** ppHead)
{
    HRESULT     hr = S_OK;

    if (ppHead)
    {
        *ppHead = NULL;
    }

    if (IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = Layout()->createTHead(ppHead);

Cleanup:

    RRETURN(SetErrorInfo( hr ));
}



//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::deleteTHead
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::deleteTHead()
{
    HRESULT     hr = S_OK;

    if (IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = Layout()->deleteTHead();

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::createTFoot
//
//  Synopsis:   Table OM method implementation
//
//  Arguments:  ppFoot - return value
//
//----------------------------------------------------------------------------

HRESULT
CTable::createTFoot(IDispatch** ppFoot)
{
    HRESULT     hr = S_OK;

    if (ppFoot)
    {
        *ppFoot = NULL;
    }

    if (IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = Layout()->createTFoot(ppFoot);

Cleanup:

    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::deleteTFoot
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::deleteTFoot()
{
    HRESULT     hr = S_OK;

    if (IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = Layout()->deleteTFoot();

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable :: createCaption
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::createCaption(IHTMLTableCaption** ppCaption)
{
    HRESULT         hr = S_OK;

    if (ppCaption)
    {
        *ppCaption = NULL;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = Layout()->createCaption(ppCaption);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable :: deleteCaption
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::deleteCaption()
{
    HRESULT         hr;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = Layout()->deleteCaption();

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::get_tHead
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::get_tHead(IHTMLTableSection** ppHead)
{
    CTableLayout * pTableLayout;
    HRESULT hr;

    if (!ppHead)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppHead = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = Layout();
    hr = pTableLayout->EnsureTableLayoutCache();

    if (hr)
        goto Cleanup;

    if (pTableLayout->_pHead)
    {
        hr = pTableLayout->_pHead->QueryInterface(IID_IHTMLTableSection, (void **)ppHead);
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::get_tFoot
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::get_tFoot(IHTMLTableSection ** ppFoot)
{
    CTableLayout * pTableLayout = Layout();
    HRESULT hr = S_OK;

    if (!ppFoot)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppFoot = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (pTableLayout->_pFoot)
    {
        hr = pTableLayout->_pFoot->QueryInterface(IID_IHTMLTableSection, (void **)ppFoot);
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::get_rows
//
//  Synopsis:   Table OM method implementation. Return the rows collection
//
//----------------------------------------------------------------------------

HRESULT
CTable::get_rows(IHTMLElementCollection ** ppRows)
{
    HRESULT hr;

    if (!ppRows)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppRows = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    // Create a collection cache if we don't already have one.
    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->EnsureAry(TABLE_ROWS_COLLECTION));
    if ( hr )
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(TABLE_ROWS_COLLECTION, (IDispatch **)ppRows));

Cleanup:
    RRETURN(SetErrorInfo(hr));
}

//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::get_tBodies
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::get_tBodies(IHTMLElementCollection ** ppSections)
{
    HRESULT hr;

    if (!ppSections)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    *ppSections = NULL;

    // Create a collection cache if we don't already have one.
    hr = THR(EnsureCollectionCache());
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->EnsureAry(TABLE_BODYS_COLLECTION));
    if ( hr )
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(TABLE_BODYS_COLLECTION, (IDispatch **)ppSections));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable::get_caption
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::get_caption(IHTMLTableCaption **ppCaption)
{
    HRESULT     hr = S_OK;
    CTableCaption * pCaption;
    CTableLayout *  pTableLayout;

    if (!ppCaption)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppCaption = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = Layout();
    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    pCaption = pTableLayout->GetFirstCaption();

    if (pCaption)
    {
        hr = THR(pCaption->QueryInterface(IID_IHTMLTableCaption ,(void **)ppCaption));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+---------------------------------------------------------------------------
//
//  Member: IHTMLTableSection::get_rows
//
//  Synopsis:   Table OM method implementation. Return the rows collection
//
//----------------------------------------------------------------------------

HRESULT
CTableSection::get_rows(IHTMLElementCollection ** ppRows)
{
    HRESULT hr;

    if (!ppRows)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppRows = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    // Create a collection cache if we don't already have one.
    hr = THR(EnsureCollectionCache(ROWS_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(ROWS_COLLECTION, (IDispatch **)ppRows));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable2::cells
//
//  Synopsis:   Table OM 
//
//----------------------------------------------------------------------------

HRESULT
CTable::get_cells(IHTMLElementCollection ** ppCells)
{
    HRESULT hr;

    if (!ppCells)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppCells = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    // do the work to get the Cells Collection
    // Create a collection cache if we don't already have one.
    hr = THR(EnsureCollectionCache());  // TABLE_CELL_COLLECTION
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(TABLE_CELLS_COLLECTION, (IDispatch**)ppCells));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}



//+---------------------------------------------------------------------------
//
//  Member: IHTMLTable2::cells
//
//  Synopsis:   Table OM method implementation. Return the cells collection
//
//----------------------------------------------------------------------------

HRESULT
CTable::cells(BSTR szRange, IHTMLElementCollection ** ppCells)
{
    HRESULT hr;
    CStr                    strNormRange;

    if (!ppCells)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppCells = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    if(_tcslen(szRange) == 0)
    {

        // Return the whole collection
        hr = THR(_pCollectionCache->GetDisp(TABLE_CELLS_COLLECTION, (IDispatch**)ppCells));
    }
    else
    {
        CCellRangeParser        cellRangeParser(szRange);
        RECT                    rectRange;

        if(cellRangeParser.Failed())
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        strNormRange.Set(cellRangeParser.GetNormalizedRangeName());

        // do the work to get the Cells Collection
        // Create a collection cache if we don't already have one.
        hr = THR(EnsureCollectionCache());
        if (hr)
            goto Cleanup;

        cellRangeParser.GetRangeRect(&rectRange);

        hr = THR(_pCollectionCache->EnsureAry(TABLE_CELLS_COLLECTION));
        if (hr)
            goto Cleanup;

        hr = THR(_pCollectionCache->GetDisp(TABLE_CELLS_COLLECTION,  strNormRange,
                        CacheType_CellRange, (IDispatch**)ppCells, FALSE, &rectRange));
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+---------------------------------------------------------------------------
//
//  Member:     IHTMLTable::insertRow
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::insertRow(long iRow, IDispatch **ppRow)
{
    HRESULT         hr;
    CTableSection * pSection;
    CTableLayout *  pTableLayout = Layout();
    int             cRows;

    if (ppRow)
    {
        *ppRow = NULL;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    cRows = pTableLayout->GetRows();

    if (iRow < - 1 || iRow > cRows)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (iRow == -1 || iRow == cRows)
    {
        hr = pTableLayout->ensureTBody();
        if (hr)
            goto Cleanup;

        pSection = pTableLayout->_aryBodys[pTableLayout->_aryBodys.Size() - 1];

        if (cRows && pTableLayout->_pFoot)
        {
            pSection = pTableLayout->_pFoot;
        }
        iRow = pSection->_cRows;    // insert after the last row
    }
    else
    {
        iRow = pTableLayout->VisualRow2Index(iRow);
        pSection = pTableLayout->GetRow(iRow)->Section();
        if (!pSection)
        {
            hr = S_OK;
            goto Cleanup;
        }
        iRow -= pSection->_iRow;    // relative to the section row index
    }

    Assert(pSection);
    hr = pSection->insertRow(iRow, ppRow);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member:     IHTMLTable::deleteRow
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::deleteRow(long iRow)
{
    CTableRow *     pRowDelete;
    CTableLayout *  pTableLayout = Layout();
    int             cRows;
    HRESULT         hr;
    BOOL            fIncrementalUpdatePossible;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    cRows = pTableLayout->GetRows();

    if (iRow == -1)
    {
        iRow = pTableLayout->GetRows() - 1;
    }

    if (iRow < 0 || iRow >= cRows)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    iRow = pTableLayout->VisualRow2Index(iRow);

    pRowDelete = pTableLayout->_aryRows[iRow];
    Assert (pRowDelete);

    fIncrementalUpdatePossible = !pRowDelete->_fCrossingRowSpan       && 
                                  pTableLayout->_fAllRowsSameShape    && 
                                  pTableLayout->_aryRows.Size() != 1  && 
                                 !pRowDelete->_fParentOfTC;

    hr = pTableLayout->deleteElement(pRowDelete, fIncrementalUpdatePossible);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member:     IHTMLTable2::moveRow, moveRowHelper
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTable::moveRow(long iRow, long iRowTarget, IDispatch **ppRow)
{
    RRETURN(moveRowHelper(iRow, iRowTarget, ppRow));
}

HRESULT
CTable::moveRowHelper(long iRow, long iRowTarget, IDispatch **ppRow, BOOL fConvertRowIndices)
{
    CTableLayout    * pTableLayout;
    CTableRow       * pRowMoving = NULL;
    CTableRow       * pRowNeighbor;
    IHTMLElement    * pElemMoving = NULL;
    IHTMLElement    * pElemNeighbor = NULL;
    IMarkupServices * pMarkupServices = NULL;
    CDoc            * pDoc;
    IMarkupPointer  * pmpRowMovingBegin = NULL;
    IMarkupPointer  * pmpRowMovingEnd = NULL;
    IMarkupPointer  * pmpRowTarget = NULL;
    int               cRows;
    BOOL              fMoveToEnd = FALSE;
    HRESULT           hr = S_OK;

    if (IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (ppRow)
    {
        *ppRow = NULL;
    }

    if (iRow == iRowTarget)
        goto Cleanup;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = Layout();
    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    cRows = pTableLayout->GetRows();

    if (iRow == -1)
        iRow = cRows-1;
    if (iRowTarget == -1)
        iRowTarget = cRows-1;

    if (   iRow < 0 || iRow >= cRows
        || iRowTarget < 0 || iRowTarget >= cRows)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (iRow == iRowTarget)
        goto Cleanup;

    // If row is moved down and since it is to become row number iRowTarget,
    // we need to add the number of rows disappearing to the bottom index.
    if (iRow < iRowTarget)
        iRowTarget += 1; // c

    if (iRowTarget >= cRows)
    {
        fMoveToEnd = TRUE;
        iRowTarget = cRows-1;
    }

    // Convert visual row index to internal row index.
    if (fConvertRowIndices)
    {
        iRow = pTableLayout->VisualRow2Index(iRow);
        iRowTarget = pTableLayout->VisualRow2Index(iRowTarget);
    }

    pRowMoving = pTableLayout->GetRow(iRow);
    pRowNeighbor = pTableLayout->GetRow(iRowTarget);
    pDoc = Doc();

    Assert(pRowMoving && pRowNeighbor && pDoc);

    hr = THR(pDoc->QueryInterface(IID_IMarkupServices, (LPVOID *) &pMarkupServices));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpRowMovingBegin));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpRowMovingEnd));
    if (hr)
        goto Cleanup;

    hr = THR(pMarkupServices->CreateMarkupPointer(&pmpRowTarget));
    if (hr)
        goto Cleanup;

    hr = THR(pRowMoving->QueryInterface(IID_IHTMLElement, (LPVOID *) &pElemMoving));
    if (hr)
        goto Cleanup;

    hr = THR(pRowNeighbor->QueryInterface(IID_IHTMLElement, (LPVOID *) &pElemNeighbor));
    if (hr)
        goto Cleanup;

    hr = THR(pmpRowMovingBegin->MoveAdjacentToElement(pElemMoving, ELEM_ADJ_BeforeBegin));
    if (hr)
        goto Cleanup;

    hr = THR(pmpRowMovingEnd->MoveAdjacentToElement(pElemMoving, ELEM_ADJ_AfterEnd));
    if (hr)
        goto Cleanup;

    hr = THR(pmpRowTarget->MoveAdjacentToElement(pElemNeighbor, fMoveToEnd?ELEM_ADJ_AfterEnd:ELEM_ADJ_BeforeBegin));
    if (hr)
        goto Cleanup;

    hr = THR(pTableLayout->moveElement(pMarkupServices, pmpRowMovingBegin, pmpRowMovingEnd, pmpRowTarget));

Cleanup:

    if (!hr && ppRow && pRowMoving)
    {
        hr = THR(pRowMoving->QueryInterface(IID_IDispatch, (LPVOID *) ppRow));
    }

    ReleaseInterface(pElemMoving);
    ReleaseInterface(pElemNeighbor);
    ReleaseInterface(pMarkupServices);

    ReleaseInterface(pmpRowMovingBegin);
    ReleaseInterface(pmpRowMovingEnd);
    ReleaseInterface(pmpRowTarget);

    RRETURN(hr);
}


//*********************************************************************
// CTable::GetDispID, IDispatch
//
//*********************************************************************

STDMETHODIMP
CTable::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    HRESULT         hr;
    LONG            lDisp;

    // Now let CBase to search for the dispid
    hr = THR_NOTRACE(super::GetDispID(bstrName, grfdex, pid));

    if(hr == DISP_E_UNKNOWNNAME)
    {
        // This may be a tbl["a3:c6"] syntax, check of presence of the : or ..
        if(_tcschr(bstrName, _T(':')) || _tcsstr(bstrName, _T("..")))
        {
            // Create an atom to pass the string to the Invoke
            CAtomTable * pTable = GetAtomTable();
            hr = THR_NOTRACE(pTable->AddNameToAtomTable(bstrName, &lDisp));
            if(hr)
                goto Cleanup;
            lDisp += DISPID_COLLECTION_MIN;
            *pid = lDisp;
        }
    }

Cleanup:
    RRETURN1(hr, DISP_E_UNKNOWNNAME);
}


#ifdef USE_STACK_SPEW
#pragma check_stack(off)
#endif

HRESULT
CTable::ContextThunk_InvokeEx (DISPID dispid, LCID lcid, WORD wFlags, DISPPARAMS *    pdispparams,
    VARIANT * pvarResult, EXCEPINFO *pexcepinfo, IServiceProvider *pSrvProvider)
{
    IUnknown * pUnkContext;
    HRESULT    hr;

    // Magic macro which pulls context out of nowhere (actually eax)
    CONTEXTTHUNK_SETCONTEXT

    if(dispid >= DISPID_COLLECTION_MIN && 
       dispid <= DISPID_COLLECTION_MAX)
    {
        CAtomTable * pTable = GetAtomTable();
        IHTMLElementCollection * pCol;
        const TCHAR            * pProrName;
        BSTR                     bstrName;
        
        hr = THR(pTable->GetNameFromAtom (dispid - DISPID_COLLECTION_MIN, &pProrName));
        if(hr)
            goto Cleanup;
        hr = THR(FormsAllocString(pProrName, &bstrName));
        if(hr)
            goto Cleanup;
        hr = THR(cells(bstrName, &pCol));
        FormsFreeString(bstrName);
        if(hr)
            goto Cleanup;
        V_VT(pvarResult) = VT_DISPATCH;
        V_DISPATCH(pvarResult) = pCol;
    }
    else
    {          
        hr = THR_NOTRACE(super::ContextInvokeEx (dispid, lcid, wFlags, pdispparams, pvarResult,
                       pexcepinfo, pSrvProvider, pUnkContext ? pUnkContext : (IUnknown*)this));
    }
        
Cleanup:
    RRETURN(hr);
}

#ifdef USE_STACK_SPEW
#pragma check_stack(on)
#endif


//+---------------------------------------------------------------------------
//
//  Member:     IHTMLTableSection::insertRow
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableSection::insertRow(long iRow, IDispatch** ppRow)
{
    CElement      * pNewElement = NULL;
    CElement      * pAdjacentElement = NULL;
    HRESULT         hr;
    Where           where;
    BOOL            fIncrementalUpdatePossible;
    CTable        * pTable;
    CTableLayout  * pTableLayout;

    if (ppRow)
        *ppRow = NULL;

    pTable = Table();
    if (!pTable)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = pTable->Layout();
    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (iRow < -1 || iRow > _cRows)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (pTable->IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = THR( Doc()->CreateElement( ETAG_TR, & pNewElement ) );
    
    if (hr)
        goto Cleanup;

    Assert (pNewElement);

    // OM: iRow == -1                    means add the row at the end of the rows collection
    // OM: iRow == sizeof the collection means the same as iRow == -1
    // OM: iRow == good                  means insert as a row (iRow)

    where = BeforeBegin;                // insert the row before the "iRow"
    if (iRow == -1 || iRow == _cRows)
    {
        if (_cRows)
        {
            where = AfterEnd;           // insert after the last row
            iRow = _cRows - 1;
        }
        else
        {
            where = AfterBegin;         // insert After <TBODY> tag
            iRow = 0;
            pAdjacentElement = this;
        }
    }

    if (!pAdjacentElement)
    {
        Assert (iRow >= 0 && iRow < _cRows);
        pAdjacentElement = pTableLayout->_aryRows[_iRow + iRow];
        Assert (pAdjacentElement);
    }
    
    fIncrementalUpdatePossible = !_cRows                             ||     // in case of empty section, OR
                                  (where == AfterBegin && iRow == 0) ||     // in case of inserting first row to the section, OR
                                                                            // in case next row doesn't cross rowspans
                                  !pTableLayout->_aryRows[_iRow + iRow]->_fCrossingRowSpan;

    if (fIncrementalUpdatePossible)
        pTableLayout->_iInsertRowAt = _iRow + iRow + (where == AfterEnd? 1 : 0);
    hr = pTableLayout->insertElement(pAdjacentElement, pNewElement, where, fIncrementalUpdatePossible);
    pTableLayout->_iInsertRowAt = -1;
    if (hr)
        goto Cleanup;

    Assert(pNewElement->Tag() == ETAG_TR);
    DYNCAST(CTableRow, pNewElement)->_fCompleted = TRUE;

    if (ppRow)
    {
        hr = pNewElement->QueryInterface(IID_IHTMLTableRow, (void **)ppRow);
    }

Cleanup:
    CElement::ReleasePtr(pNewElement);

    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTableSection::deleteRow
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableSection::deleteRow(long iRow)
{
    CTableRow     * pRowDelete;
    CTableLayout  * pTableLayout;
    CTable        * pTable;
    HRESULT         hr;
    BOOL            fIncrementalUpdatePossible;

    pTable = Table();
    if (!pTable)
    {
        hr = S_OK;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = pTable->Layout();
    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (iRow == -1)
    {
        iRow = _cRows - 1;
    }

    if (iRow < 0 || iRow >= _cRows)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    if (pTable->IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pRowDelete = pTableLayout->_aryRows[iRow + _iRow];
    Assert (pRowDelete);

    fIncrementalUpdatePossible = !pRowDelete->_fCrossingRowSpan       && 
                                  pTableLayout->_fAllRowsSameShape    && 
                                  pTableLayout->_aryRows.Size() != 1  &&
                                 !pRowDelete->_fParentOfTC;

    hr = pTableLayout->deleteElement(pRowDelete, fIncrementalUpdatePossible);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member:     IHTMLTableSection2::moveRow
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableSection::moveRow(long iRow, long iRowTarget, IDispatch **ppRow)
{
    CTable * pTable = Table();
    HRESULT  hr;

    if (!pTable)
    {
        hr = E_PENDING;
        goto Cleanup;
    }

    if (   iRow < -1 || iRow >= _cRows
        || iRowTarget < -1 || iRow >= _cRows )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    if (iRow == -1)
        iRow = _cRows-1;
    if (iRowTarget == -1)
        iRowTarget = _cRows-1;

    hr = THR(pTable->moveRowHelper(iRow + _iRow, iRowTarget + _iRow, ppRow, FALSE));

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     IHTMLTableRow::get_rowIndex
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableRow::get_rowIndex(long *plIndex)
{
    CTableLayout    * pTableLayout;
    CTable          * pTable;
    CTableRowLayout * pRowLayout;
    long              cRowsHead;
    long              cRowsFoot;
    HRESULT           hr;

    if (!plIndex)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pTable = Table();
    if (!pTable)
    {
        *plIndex = -1;
        hr = S_OK;
        goto Cleanup;
    }

    *plIndex = 0;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = pTable->Layout();
    pRowLayout = Layout();

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    cRowsHead = (pTableLayout->_pHead ? pTableLayout->_pHead->_cRows : 0);
    cRowsFoot = (pTableLayout->_pFoot ? pTableLayout->_pFoot->_cRows : 0);

    //
    // THEAD rows are first in the table
    //

    if (pRowLayout->_iRow < cRowsHead)
    {
        *plIndex = pRowLayout->_iRow;
    }

    //
    // TBODY rows are after the THEAD, but before the TFOOT
    //

    else if (pRowLayout->_iRow >= (cRowsHead + cRowsFoot))
    {
        *plIndex = pRowLayout->_iRow - cRowsFoot;
    }

    //
    // TFOOT rows are after the THEAD and TBODY
    //

    else
    {
        Assert(pRowLayout->_iRow >= cRowsHead && pRowLayout->_iRow < (cRowsHead + cRowsFoot));
        *plIndex = pTableLayout->GetRows() - cRowsFoot + pRowLayout->_iRow - cRowsHead;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+---------------------------------------------------------------------------
//
//  Member:     IHTMLTableRow::get_sectionRowIndex
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableRow::get_sectionRowIndex(long *plIndex)
{
    HRESULT hr = S_OK;
    CTableSection *pSection;
    CTable        *pTable;
    CTableLayout  *pTableLayout;

    if (!plIndex)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTable = Table();
    if (!pTable)
        goto Cleanup;

    pTableLayout = pTable->Layout();

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    pSection = Section();
    if (pSection)
    {
        *plIndex = Layout()->_iRow - pSection->_iRow;
    }
    else
    {
        *plIndex = -1;
        hr = S_OK;
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTableRow::get_cells
//
//  Synopsis:   Table OM method implementation. Return the cells collection
//
//----------------------------------------------------------------------------

HRESULT
CTableRow::get_cells(IHTMLElementCollection ** ppCells)
{
    HRESULT hr;

    if (!ppCells)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppCells = NULL;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    // do the work to get the Cells Collection
    // Create a collection cache if we don't already have one.
    hr = THR(EnsureCollectionCache(CELLS_COLLECTION));
    if (hr)
        goto Cleanup;

    hr = THR(_pCollectionCache->GetDisp(CELLS_COLLECTION, (IDispatch**)ppCells));

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTableRow::insertCell
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableRow::insertCell(long iCell, IDispatch** ppCell)
{
    CElement    *pNewElement = NULL;
    CTableCell  *pCell = NULL;
    CElement    *pAdjacentElement = NULL;
    HRESULT     hr;
    Where       where;
    int         i, iRealCell = 0;
    CTable      *pTable;
    CTableLayout *pTableLayout;
    BOOL        fIncrementalUpdatePossible, fLastCell = FALSE;
    CTableRowLayout *pRowLayout = Layout();
    Assert (pRowLayout);

    int         cAryCellsSize = pRowLayout->_aryCells.Size();
    int         cCells = pRowLayout->_cRealCells;

    if (ppCell)
    {
        *ppCell = NULL;
    }

    // OM: iCell == -1                    means add the cell at the end of the cells collection
    // OM: iCell == sizeof the collection means the same as iCell == -1
    // OM: iCell == good                  means insert as a cell (iCell)

    if (iCell < -1 || iCell > cCells)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    pTable = Table();
    if (!pTable)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (pTable->IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = pTable->Layout();

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    hr = Doc()->CreateElement(ETAG_TD, &pNewElement);
    if (hr)
        goto Cleanup;

    Assert (pNewElement);

    where = BeforeBegin;                // insert the row before the "iCell"
    if (iCell == -1 || iCell == cCells)
    {
        if (cCells)
        {
            where = AfterEnd;           // insert after the last cell
            iCell = cCells - 1;
        }
        else
        {
            where = AfterBegin;         // insert After <TR> tag
            pAdjacentElement = this;
        }
        fLastCell = TRUE;
    }

    if (!pAdjacentElement)
    {
        Assert (iCell >= 0 && iCell < cCells);
        for (i = 0; i < cAryCellsSize; i++)
        {
            pCell = pRowLayout->_aryCells[i];
            if (IsReal(pCell) && iCell == iRealCell)
            {
                fLastCell = FALSE;
                break;
            }
            if (IsReal(pCell))
            {
                iRealCell++;
                Assert (iRealCell < cCells);
            }
        }

        Assert (IsReal(pCell));
        pAdjacentElement = pCell;
        if (!pCell)
        {
            // insert the cell before the </TR> tag
            pAdjacentElement = this;
            where = BeforeEnd;
            fLastCell = FALSE;
        }
    }

    fIncrementalUpdatePossible =  !_fCrossingRowSpan && fLastCell && (cCells + 1 <= cAryCellsSize || pTableLayout->_aryRows.Size() == 1);

    hr = pTableLayout->insertElement(pAdjacentElement, pNewElement, where, fIncrementalUpdatePossible);

    if (hr)
    {
        goto Cleanup;
    }

    if (ppCell)
    {
        hr = pNewElement->QueryInterface(IID_IHTMLTableCell, (void **)ppCell);
    }

Cleanup:
    CElement::ReleasePtr(pNewElement);

    RRETURN(SetErrorInfo( hr ));
}



//+---------------------------------------------------------------------------
//
//  Member: IHTMLTableRow::deleteCell
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableRow::deleteCell(long iCell)
{
    HRESULT         hr = S_OK;
    CTableCell **   ppCell;
    CTableCell *    pCellDelete;
    long            cCells, iReal;
    CTableRowLayout * pRowLayout;
    CTable          * pTable;
    CTableLayout    * pTableLayout;

    pTable = Table();
    if (!pTable)
    {
        hr = S_OK;
        goto Cleanup;
    }

    if (pTable->IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTableLayout = pTable->Layout();
    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    pRowLayout = Layout();
    if (iCell < -1 || iCell >= pRowLayout->_aryCells.Size())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    iReal       = -1;
    pCellDelete = NULL;
    for (ppCell = pRowLayout->_aryCells, cCells = pRowLayout->_aryCells.Size();
        cCells > 0;
        ppCell++, cCells--)
    {
        if (IsReal(*ppCell))
        {
            iReal++;
            pCellDelete = *ppCell;

            if (iReal == iCell)
                break;
        }
    }

    if (    iReal < 0
        ||  (   iCell >= 0
            &&  iCell != iReal))
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    Assert(pCellDelete);
    Assert(IsReal(pCellDelete));

    hr = pTable->Layout()->deleteElement(pCellDelete);

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+---------------------------------------------------------------------------
//
//  Member: IHTMLTableCell::get_index
//
//  Synopsis:   Table OM method implementation
//
//----------------------------------------------------------------------------

HRESULT
CTableCell::get_cellIndex(long *plIndex)
{
    HRESULT     hr = S_OK;
    CTableRow * pRow;
    CTableRowLayout *pRowLayout;
    int         i;
    int         iRealCell = 0;
    CTableCell *pCell;
    CTable       * pTable;
    CTableLayout * pTableLayout;

    if (!plIndex)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    pRow = Row();
    if (!pRow)
    {
        iRealCell = -1;
        hr = S_OK;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTable = Table();
    if (pTable)
    {
        pTableLayout = pTable->Layout();
        hr = pTableLayout->EnsureTableLayoutCache();
        if (hr)
            goto Cleanup;
    }

    pRowLayout = pRow->Layout();
    for (i = 0; i < pRowLayout->_aryCells.Size(); i++)
    {
        pCell = pRowLayout->_aryCells[i];
        if (pCell == this)
        {
            break;
        }
        if (IsReal(pCell))
        {
            iRealCell++;
        }
    }

    Assert (iRealCell < pRowLayout->_cRealCells);
  

Cleanup:
    if (plIndex)
        *plIndex = iRealCell;

    RRETURN(SetErrorInfo( hr ));
}


//+------------------------------------------------------------------------
//
//  Member:     put_colSpan, IHTMLTable
//
//  Synopsis:   change the colSpan
//
//  Arguments:  cColSpan       new col span
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
STDMETHODIMP
CTableCell::put_colSpan(long cColSpan)
{
    CTable * pTable = Table();
    HRESULT hr = S_OK;
    long    cOldColSpan = ColSpan();

    if (cColSpan < 1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pTable && pTable->IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (cColSpan > MAX_COL_SPAN && cOldColSpan == MAX_COL_SPAN)
    {
        goto Cleanup;   // nothing to do
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;


    {   // Just do it, and be ready do undo it...
        CParentUndo pu(Doc());
        
        if( IsEditable() )
            pu.Start( IDS_UNDOPROPCHANGE );

        // this is a fancy way of calling the base implementation
        hr = s_propdescCTableCellcolSpan.b.SetNumberProperty(cColSpan, this, CVOID_CAST(GetAttrArray()));
        if (!hr && pTable)
        {
            pTable->Layout()->Fixup(FALSE);
        }

        pu.Finish( hr );
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+------------------------------------------------------------------------
//
//  Member:     get_colSpan, IHTMLTable
//
//  Synopsis:   return the colSpan
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
STDMETHODIMP
CTableCell::get_colSpan(long * p)
{
    if (p)
    {
        *p = ColSpan();
    }
    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     put_rowSpan, IHTMLTable
//
//  Synopsis:   change the rowSpan
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
STDMETHODIMP
CTableCell::put_rowSpan(long cRowSpan)
{
    CTable * pTable = Table();
    HRESULT hr;

   if (cRowSpan < 1)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    if (pTable && pTable->IsDataboundInBrowseMode())
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    {   // Just do it, and be ready do undo it...
        CParentUndo pu(Doc());
        
        if( IsEditable() )
            pu.Start( IDS_UNDOPROPCHANGE );

        // this is a fancy way of calling the base implementation
        hr = s_propdescCTableCellrowSpan.b.SetNumberProperty(cRowSpan, this, CVOID_CAST(GetAttrArray()));
        if (!hr && pTable)
        {
            pTable->Layout()->Fixup(FALSE);
        }

        pu.Finish( hr );
    }

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+------------------------------------------------------------------------
//
//  Member:     get_rowSpan, IHTMLTable
//
//  Synopsis:   return the rowSpan
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
STDMETHODIMP
CTableCell::get_rowSpan(long * p)
{
    HRESULT        hr = S_OK;
    int            iRowSpan = 1;
    CTable       * pTable;
    CTableLayout * pTableLayout;

    hr = EnsureInMarkup();
    if (hr)
        goto Cleanup;

    pTable = Table();
    if (pTable)
    {
        pTableLayout = pTable->Layout();
        hr = pTableLayout->EnsureTableLayoutCache();
        if (!hr)
            iRowSpan = RowSpan();
    }
    
    if (p)
    {
        *p = iRowSpan;
    }

Cleanup:
    RRETURN (hr);
}
