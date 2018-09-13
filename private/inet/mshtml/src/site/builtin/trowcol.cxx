//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       table.cxx
//
//  Contents:   CTable and related classes.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTCELL_HXX_
#define X_LTCELL_HXX_
#include "ltcell.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

MtDefine(CTableRow, Elements, "CTableRow")
MtDefine(CTableRow_aryCells_pv, CTableRow, "CTableRow::_aryCells::_pv")
MtDefine(CTableCol, Elements, "CTableCol")
MtDefine(CTableSection, Elements, "CTableSection")
MtDefine(BldRowCellsCol, PerfPigs, "Build CTableRow::CELLS_COLLECTION")
MtDefine(BldSectionRowsCol, PerfPigs, "Build CTableSection::ROWS_COLLECTION")
MtDefine(CharFormatSteal, ComputeFormats, "CharFormat steal from nearby row/col/cell")
MtDefine(ParaFormatSteal, ComputeFormats, "ParaFormat steal from nearby row/col/cell")
MtDefine(FancyFormatSteal, ComputeFormats, "FancyFormat steal from nearby row/col/cell")

const CElement::CLASSDESC CTableRow::s_classdesc =
{
    {
        &CLSID_HTMLTableRow,            // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NOTIFYENDPARSE |
        0,                              // _dwFlags
        &IID_IHTMLTableRow,             // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTableRow,      // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};


HRESULT
CTableRow::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_TR));
    Assert(ppElement);

    *ppElement = new CTableRow(pDoc);
    RRETURN( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableRow::destructor, CBase
//
//  Note:       The collection cache must be deleted in the destructor, and
//              not in Passivate, because collection objects we've handed out
//              (via get_cells) merely SubAddRef the row.
//-------------------------------------------------------------------------

CTableRow::~CTableRow()
{
    delete _pCollectionCache;
}


//+------------------------------------------------------------------------
//
//  Member:     CTableRow::Passivate, CBase
//
//-------------------------------------------------------------------------

void
CTableRow::Passivate()
{
    super::Passivate();
}

//+----------------------------------------------------------------------------
//
//  Member:     CTableRow::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CTableRow::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLTableRow)
        QI_HTML_TEAROFF(this, IHTMLTableRow, NULL)
        QI_HTML_TEAROFF(this, IHTMLTableRow2, NULL)
        QI_TEAROFF(this, IHTMLTableRowMetrics, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+------------------------------------------------------------------------
//
//  Member:     CTableRow::ComputeFormats
//
//  Synopsis:   Compute Char and Para formats induced by this element and
//              every other element above it in the HTML tree.
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//  Note:       We override this here to put our defaults into the format
//              FIRST, and also to cache vertical alignment here in the object
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget )
{
    SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    HRESULT hr;
    CTableRowLayout *pRowLayout;

    Assert(pNodeTarget && SameScope(this, pNodeTarget));

    pRowLayout = Layout();

    //check row above
    //BUGBUG: (alexa) for data-binding we should take comparable row in a previous GENERATED section instead of a previous row
    if (pRowLayout->_iRow > 0)
    {
        CTable    * pTable = Table();

        // If we can't find our table, the table structure is screwed up and
        // our compute formats behave like a CElement.
        if (!pTable)
            goto DontStealFormat;

        CTableLayout * pTableLayout = pTable->Layout();
        CTableRow * pRow;
        CTreeNode * pNodeRow;

        {
            WHEN_DBG(
                BOOL fDisableTLCAssert = pTableLayout->_fDisableTLCAssert;
                pTableLayout->_fDisableTLCAssert = TRUE; )

            pRow = (pRowLayout->_iRow-1 < pTableLayout->GetRows()) ? pTableLayout->GetRow(pRowLayout->_iRow-1) : NULL;

            WHEN_DBG(
                pTableLayout->_fDisableTLCAssert = fDisableTLCAssert; )
        }

        pNodeRow = pRow ? pRow->GetFirstBranch() : NULL;

        //
        // Proxy Alert!
        //
        // Notice that to do this optimization we are comparing the parent
        // of pElementTarget to that of pRow.  pElementTarget is in the
        // tree (and may be a proxy), while pRow may not be in the tree.
        // But, if the parent of pRow is the parent of pElementTarget, then
        // pRow is in the tree.
        //

        // We do not use this optimization when ComputeFormats is called for getttng a value
        if(pCFI->_eExtraValues != ComputeFormatsType_Normal ||
               !pTableLayout->IsTableLayoutCacheCurrent()   ||
               !pRow                                        ||
               pRow->_fHasStyleExpressions                  ||
               pRow->_fHasPendingRecalcTask)
            goto DontStealFormat;

        if (pRow && pNodeRow && pNodeRow->_iPF >= 0 && pNodeRow->_iCF >= 0 && pNodeRow->_iFF >= 0 &&
            SameScope(pNodeTarget->Parent(), pNodeRow->Parent()) &&
            (_pAA == NULL && pRow->_pAA == NULL ||
             _pAA != NULL && pRow->_pAA != NULL && _pAA->Compare(pRow->_pAA)) )
        {
            THREADSTATE * pts = GetThreadState();

            // At least one of the caches is dirty.
            Assert(pNodeTarget->_iPF == -1 || pNodeTarget->_iCF == -1 || pNodeTarget->_iFF == -1);

            // The caches that were not dirty, have to match with the corresponding caches of
            // the previous row.
            Assert(pNodeTarget->_iPF == -1 || pNodeTarget->_iPF == pNodeRow->_iPF);
            Assert(pNodeTarget->_iCF == -1 || pNodeTarget->_iCF == pNodeRow->_iCF);
            Assert(pNodeTarget->_iFF == -1 || pNodeTarget->_iFF == pNodeRow->_iFF);

            //
            // Selectively copy down each of the dirty caches.
            //

            if (pNodeTarget->_iPF == -1)
            {
                pNodeTarget->_iPF = pNodeRow->_iPF;
                (pts->_pParaFormatCache)->AddRefData( pNodeRow->_iPF );
                MtAdd(Mt(ParaFormatSteal), 1, 0);
            }

            if (pNodeTarget->_iCF == -1)
            {
                pNodeTarget->_iCF = pNodeRow->_iCF;
                (pts->_pCharFormatCache)->AddRefData( pNodeRow->_iCF );
                MtAdd(Mt(CharFormatSteal), 1, 0);
            }

            if (pNodeTarget->_iFF == -1)
            {
                pNodeTarget->_iFF = pNodeRow->_iFF;
                (pts->_pFancyFormatCache)->AddRefData( pNodeRow->_iFF );
                MtAdd(Mt(FancyFormatSteal), 1, 0);
            }

            pRowLayout->CacheRowHeight(GetFancyFormatEx(pNodeTarget->_iFF));

            // Set the _fBlockNess cache bit on the node to save a little time later.
            // Need to do this here because we don't call super.
            pNodeTarget->_fBlockNess = TRUE;

            hr = S_OK;
            goto Cleanup;
        }
    }

DontStealFormat:

    hr = THR(super::ComputeFormats( pCFI, pNodeTarget ));

Cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCF - charformat to apply default properties on
//              pPF - paraformat to apply default properties on
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    CColorValue ccvBorderColor;
    CColorValue ccvColor;
    HRESULT     hr;

    hr = super::ApplyDefaultFormat(pCFI);
    if (hr)
        goto Cleanup;

    pCFI->PrepareFancyFormat();

    // Override inherited colors as necessary
    ccvBorderColor = GetAAborderColor();
    if ( ccvBorderColor.IsDefined() )
    {
        long    i;

        for ( i = BORDER_TOP; i <= BORDER_LEFT; i++ )
            pCFI->_ff()._ccvBorderColors[ i ] = ccvBorderColor;

        pCFI->_ff()._ccvBorderColorLight   =
        pCFI->_ff()._ccvBorderColorHilight =
        pCFI->_ff()._ccvBorderColorDark    =
        pCFI->_ff()._ccvBorderColorShadow  = ccvBorderColor;
        pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
    }

    ccvBorderColor = GetAAborderColorLight();
    if (ccvBorderColor.IsDefined())
    {
        pCFI->_ff()._ccvBorderColorLight   =
        pCFI->_ff()._ccvBorderColorHilight = ccvBorderColor;
        pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
    }

    ccvBorderColor = GetAAborderColorDark();
    if (ccvBorderColor.IsDefined())
    {
        pCFI->_ff()._ccvBorderColorDark    =
        pCFI->_ff()._ccvBorderColorShadow  = ccvBorderColor;
        pCFI->_ff()._fOverrideTablewideBorderDefault = TRUE;
    }


    Layout()->CacheRowHeight(&(pCFI->_ff()));

    pCFI->UnprepareForDebug();

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableRow::Save
//
//  Synopsis:   Save the element to the stream
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::Save(CStreamWriteBuff *pStmWrBuff, BOOL fEnd)
{
    HRESULT hr = S_OK;

    hr = super::Save(pStmWrBuff, fEnd);
    if (hr)
        goto Cleanup;

    if (fEnd && pStmWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        hr = pStmWrBuff->NewLine();
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableRow::OnPropertyChange
//
//  Synopsis:   Process property changes to table row element.
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr;
    CTable *pTable = NULL;
    BOOL    fPropagateChange = FALSE;
    BOOL    fSpecialProperty = FALSE;

    hr = THR(super::OnPropertyChange(dispid, dwFlags));
    if (hr)
        goto Cleanup;

    switch (dispid)
    {
    case DISPID_A_DISPLAY:
    case DISPID_A_POSITION:
        pTable = Table();
        if (pTable)
        {
            CTableLayout *pTableLayout = pTable->Layout();
            Assert (pTableLayout);
            if (_fHaveRowSpanCells || _fCrossingRowSpan)
            {
                pTableLayout->MarkTableLayoutCacheDirty();
            }
            pTableLayout->Resize();
        }
        break;

    case DISPID_A_VISIBILITY:
    case DISPID_A_BACKGROUNDIMAGE:
    case DISPID_BACKCOLOR:
        // If the backgroundcolor or image changes, let the affected table
        // cells know, so that they can update their display tree nodes.
        fPropagateChange = TRUE;
        fSpecialProperty = TRUE;
        break;

    default:
        if (dwFlags & (ELEMCHNG_REMEASUREINPARENT | ELEMCHNG_SIZECHANGED |
                       ELEMCHNG_REMEASURECONTENTS | ELEMCHNG_REMEASUREALLCONTENTS))
        {
            fPropagateChange = TRUE;
        }
        break;
    }

    if (fPropagateChange)
    {
        hr = THR(PropagatePropertyChangeToCells(dispid, dwFlags, fSpecialProperty));
        //if (hr)
        //    goto Cleanup;
    }

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableRow::PropagatePropertyChangeToCells
//
//  Synopsis:   Helper function to propagate property changes from
//              table elements to cells.  Called by CTableSection::
//              and CTableRow::OnPropertyChange().
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::PropagatePropertyChangeToCells(DISPID dispid, DWORD dwFlags, BOOL fSpecialProperty)
{
    CTableRowLayout * pRowLayout = Layout();
    CTableCell     ** ppCell;
    CTableCell      * pCell;
    CTableCellLayout * pCellLayout;
    int               cColSpan, cCols, cCells;
    HRESULT           hr = S_OK;
    CTable           *pTable = Table();
    CTableLayout     *pTableLayout = pTable? pTable->Layout() : NULL;

    if (!pRowLayout)
        goto Cleanup;
  
    if (!pTableLayout)
        goto Cleanup;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    for (cCols = pRowLayout->_aryCells.Size(), ppCell = pRowLayout->_aryCells ;  cCols > 0; cCols -= cColSpan, ppCell += cColSpan)
    {
        pCell = Cell(*ppCell);
        pCellLayout = pCell ? pCell->Layout() : NULL;

        if (pCell && pCellLayout)
        {
            cColSpan = pCell->ColSpan();
            if (fSpecialProperty)
            {
                hr = THR(pCellLayout->OnPropertyChange(dispid, dwFlags));
                if (hr)
                    goto Cleanup;
            }
            else if (dwFlags & (ELEMCHNG_REMEASUREINPARENT | ELEMCHNG_SIZECHANGED))
            {
                pCellLayout->_fSizeThis = TRUE; // cell needs to be sized
            }
            else 
            {
                Assert (dwFlags & (ELEMCHNG_REMEASURECONTENTS | ELEMCHNG_REMEASUREALLCONTENTS));
                pCell->RemeasureElement(dwFlags & ELEMCHNG_REMEASUREALLCONTENTS
                                        ? NFLAGS_FORCE
                                        : 0); // make sure we remeasure this cell
                if (pTableLayout->IsFixed())
                {
                    pTableLayout->_cCalcedRows = 0;     // hack, bug #75881, need to force fixed table to calc.
                    // pRowLayout->_fSizeThis = TRUE;   // the right solution would be to set _fSizeThis
                    // and let CalculateLayout to call CalculateRow for this layout (obvisouly we would need to
                    // enusre LAYOUT_FORCE flag via notification to trigger that logic).
                }
            }
        }
        else
        {
            cColSpan = 1;
        }
    }

    if (pRowLayout->_pDisplayNoneCells)
    {
        for (cCells = pRowLayout->_pDisplayNoneCells->Size(), ppCell = *pRowLayout->_pDisplayNoneCells ;  cCells > 0; cCells--, ppCell++)
        {
            hr = THR((*ppCell)->Layout()->OnPropertyChange(dispid, dwFlags));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  ROW CELLS collection
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
// Note: the GetNext interface is LITTLE BIT STRANGE (I am being polite to OM team: alexa):
//       it should of being called GetCurrent and Prepare for the Next
//-------------------------------------------------------------------------

CElement *
CTableRowCellsCollectionCacheItem::MoveTo ( long lIndex )
{
    _lCurrentIndex = lIndex;
    return CTableRowCellsCollectionCacheItem::GetAt(lIndex);
}


CElement *
CTableRowCellsCollectionCacheItem::GetNext ( void )
{
    CTableCell *pCell = NULL, *pCellNext;
    int i, cCols;
    
    if (_iCurrentCol < 0)
    {
        return GetAt(_lCurrentIndex++);
    }
    else if (_pRowLayout && _lCurrentIndex < _pRowLayout->_cRealCells)
    {
        // 1. Get Current
        // 2. Prepare for the next
        BOOL    fDisplayNone = _pRowLayout->_pDisplayNoneCells != NULL && _cDisplayNone < _pRowLayout->_pDisplayNoneCells->Size();

        Assert ( _iCurrentCol >= 0 && (_iCol >= 0 || _cDisplayNone >= 0) );
        cCols = _pRowLayout->_aryCells.Size();
        Assert (_iCol < cCols);

        if (_iCol >= 0)
        {
            pCell = _pRowLayout->_aryCells[_iCol];
            Assert (IsReal(pCell));
            // Prepare for the next
            i = _iCol + pCell->ColSpan();
        }
        else
        {
            Assert (_cDisplayNone >= 0);
            pCell = (*_pRowLayout->_pDisplayNoneCells)[_cDisplayNone];
            i =  pCell->Layout()->ColIndex();
            if (++_cDisplayNone >= _pRowLayout->_pDisplayNoneCells->Size())
            {
                fDisplayNone = FALSE;
            }
        }


        _lCurrentIndex ++;
        _iCol = -1;
        _iCurrentCol = -1;

        while (i < cCols)
        {
            if (fDisplayNone && (*_pRowLayout->_pDisplayNoneCells)[_cDisplayNone]->Layout()->ColIndex() == i)
            {
                _iCurrentCol = i;
                break;
            }
            pCellNext = _pRowLayout->_aryCells[i];
            if (IsReal(pCellNext))
            {
                _iCol = i;
                _iCurrentCol = i;
                break;
            }
            i++;
        }

    }

    return pCell;
}

CElement *
CTableRowCellsCollectionCacheItem::GetAt ( long lIndex )
{
    int         i;
    CTableCell  *pCell = NULL;
    int         iRealCell = 0;

    _iCol = -1;
    _iCurrentCol = -1;
    _cDisplayNone = -1;

    Assert ( lIndex >= 0);
    Assert (_pRowLayout);
    if (lIndex < _pRowLayout->_cRealCells)
    {

        BOOL            fDisplayNone = _pRowLayout->_pDisplayNoneCells != NULL;
        int             cDisplayNoneCells = 0;
        int             iDisplayNoneCol   = -1;
        CTableCell   **ppDisplayNoneCells = NULL;

        int  cCells = _pRowLayout->_aryCells.Size();
        if (fDisplayNone)
        {
            cDisplayNoneCells = _pRowLayout->_pDisplayNoneCells->Size();
            ppDisplayNoneCells = (*_pRowLayout->_pDisplayNoneCells);
            _cDisplayNone = 0;
            iDisplayNoneCol = (*ppDisplayNoneCells)->Layout()->ColIndex();
        }
        for (i = 0; i < cCells || fDisplayNone; i++)
        {
            if (fDisplayNone && i == iDisplayNoneCol)
            {
                if (iRealCell == lIndex)
                {
                    pCell = *ppDisplayNoneCells;
                    break;
                }
                else
                {
                    _cDisplayNone++;
                    cDisplayNoneCells--;
                    if (!cDisplayNoneCells)
                    {
                        fDisplayNone = FALSE;
                    }
                    else
                    {
                        ppDisplayNoneCells++;
                        iDisplayNoneCol = (*ppDisplayNoneCells)->Layout()->ColIndex();
                    }
                    iRealCell++;
                    i--;
                    continue;
                }
            }
            pCell = _pRowLayout->_aryCells[i];
            if ( IsReal(pCell) )
            {
                if (iRealCell == lIndex)
                {
                    _iCol = i;
                    break;
                }
                else
                {
                    iRealCell++;
                    i += pCell->ColSpan() - 1;    // - 1 to account for i++
                }
            }
        }

        _iCurrentCol = i;
        Assert (iRealCell < _pRowLayout->_cRealCells && pCell);
    }
    return pCell;
}

long 
CTableRowCellsCollectionCacheItem::Length ( void )
{
    if (_pRowLayout)
        return _pRowLayout->_cRealCells;
    else
        return NULL;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollectionCache
//
//  Synopsis:   Create the row's collection cache if needed.
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::EnsureCollectionCache(long lIndex)
{
    HRESULT hr = S_OK;

    CTable *pTable = Table();
    if (pTable)
    {
        CTableLayout * pTableLayout = pTable->Layout();
        Assert (pTableLayout);
        hr = pTableLayout->EnsureTableLayoutCache();
        if (hr)
            goto Cleanup;
    }
    
    if (!_pCollectionCache)
    {
        _pCollectionCache = new CCollectionCache(
                this,
                GetDocPtr(),
                ENSURE_METHOD(CTableRow, EnsureCollections, ensurecollections));
        if (!_pCollectionCache)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitReservedCacheItems(1, 1));
        if (hr)
            goto Error;

        CTableRowCellsCollectionCacheItem *pCellsCollection = new CTableRowCellsCollectionCacheItem(this);
        if ( !pCellsCollection )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitCacheItem ( CELLS_COLLECTION, pCellsCollection ));
        if (hr)
            goto Cleanup;

        //
        // Collection cache now owns this item & is responsible for freeing it
        //

    }

Cleanup:
    RRETURN(hr);

Error:
    delete _pCollectionCache;
    _pCollectionCache = NULL;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollections
//
//  Synopsis:   Refresh the cells collection, if needed.
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::EnsureCollections(long lIndex, long * plCollectionVersion)
{
    CTable       *pTable = Table();
    CTableLayout *pTableLayout = pTable? pTable->Layout() : NULL;
    HRESULT hr = S_OK;

    Assert (_pCollectionCache);

    CTableRowCellsCollectionCacheItem *pCellsCollection = (CTableRowCellsCollectionCacheItem *) _pCollectionCache->GetCacheItem (lIndex);

    Assert (pCellsCollection);
    pCellsCollection->_iCol = -1;           // Next item must be recomputed
    pCellsCollection->_iCurrentCol = -1;

    if ( pTableLayout)
    {
        hr = pTableLayout->EnsureTableLayoutCache();
        *plCollectionVersion = pTableLayout->_iCollectionVersion;   // to mark it done
    }
    RRETURN(hr);
}

const CTableSection::CLASSDESC CTableSection::s_classdesc =
{
    {
        &CLSID_HTMLTableSection,        // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOLAYOUT,           // _dwFlags
        &IID_IHTMLTableSection,         // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTableSection   // _pfnTearOff
};


//+------------------------------------------------------------------------
//
//  Member:     CTableSection::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CTableSection::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_TEAROFF(this, IHTMLTableSection2, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}


HRESULT
CTableSection::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_THEAD) || pht->Is(ETAG_TFOOT) || pht->Is(ETAG_TBODY));
    Assert(ppElement);
    *ppElement = new CTableSection(pht->GetTag(), pDoc);
    return *ppElement ? S_OK : E_OUTOFMEMORY;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableSection::Notify, CSite
//
//  Synopsis:   Handle notification
//
//----------------------------------------------------------------------------

void
CTableSection::Notify(CNotification *pNF)
{
    DWORD           dw = pNF->DataAsDWORD();

    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        {
            CTable       *pTable = Table();

            if (pTable)
            {
                CTableLayout *pTableLayout = pTable->Layout();
                Assert (pTableLayout);

                if (!(dw & ENTERTREE_PARSE) && !pTableLayout->_fEnsureTBody)
                {
                    pTableLayout->_fDontSaveHistory = TRUE;
                }
                if (dw & ENTERTREE_MOVE)   // if it is a MOVETREE notification
                {
                    // we are moved from another tree
                    if (pTableLayout->_fPastingRows)
                    {
                        Assert (pTable->IsDatabound() && pTableLayout->IsRepeating());
                        pTableLayout->AddSection(this);
                    }
                    else
                    {
                        pTableLayout->MarkTableLayoutCacheDirty();
                    }
                }
                else
                {
                    EnterTree();
                }
            }
        }
        break;
    case NTYPE_ELEMENT_EXITTREE_1:
        if (!(dw & EXITTREE_DESTROY))
        {
            CTable          *pTable = Table();

            if (!pTable)
                break;

           CTableLayout    *pTableLayout = pTable->Layout();
           Assert(pTableLayout);

           if (!(dw & EXITTREE_MOVE))      // if it is not a move tree
            {

                // don't update the caches for individual rows in case of :1) shutdown; 2) data-binding removing rows
                if (!(pTableLayout->_fRemovingRows || pTable->_fExittreePending))
                {
                    // Don't hold refs to the tree after our element leaves it
                    pTableLayout->BodyExitTree(this);
                    pTableLayout->MarkTableLayoutCacheDirty();
                }
            }
        }

        break;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CTableSection::EnterTree, CElement
//
//  Synopsis:   Add section to table.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CTableSection::EnterTree()
{
    HRESULT        hr = S_OK;
    CTable       * pTable = Table();
    CTableLayout * pTableLayout = pTable? pTable->Layout() : NULL;

    // Only maintain the table layout cache incrementally until the table
    // has finished parsing.
    if (pTableLayout)
    {
        if (!pTableLayout->IsCompleted() || pTableLayout->_fTableOM)
        {
            hr = pTableLayout->AddSection(this);
        }
        else
        {
            pTableLayout->MarkTableLayoutCacheDirty();
        }
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCF - charformat to apply default properties on
//              pPF - paraformat to apply default properties on
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CTableSection::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr;

    // clear back color, width/height, control blockalign coming from the table
    pCFI->_bCtrlBlockAlign = htmlBlockAlignNotSet;

    pCFI->PrepareFancyFormat();

    pCFI->_ff()._cuvWidth.SetNull();
    pCFI->_ff()._cuvHeight.SetNull();
    pCFI->_ff()._bPageBreaks = 0;

    pCFI->UnprepareForDebug();

    hr = super::ApplyDefaultFormat(pCFI);
    if (hr)
        goto Cleanup;

    if ( pCFI->_pff->_bPositionType != stylePositionNotSet )
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._bPositionType = stylePositionNotSet;
        pCFI->UnprepareForDebug();
    }

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableSection::OnPropertyChange
//
//  Synopsis:   Process property changes to table row element.
//
//-------------------------------------------------------------------------

HRESULT
CTableSection::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr;
    CTable       * pTable = NULL;
    CTableLayout * pTableLayout = NULL;
    BOOL           fSpecialProperty = FALSE;
    BOOL           fPropagateChange = FALSE;

    hr = THR(super::OnPropertyChange(dispid, dwFlags));
    if (hr)
        goto Cleanup;

    pTable = Table();
    pTableLayout = pTable ? pTable->Layout() : NULL;

    if (!pTableLayout)
        goto Cleanup;
    
    switch (dispid)
    {
    case DISPID_A_DISPLAY:
    // not supported on section: case DISPID_A_POSITION:
        pTableLayout->Resize();
        break;

    // If the backgroundcolor or image changes, let the affected table
    // cells know, so that they can update their display tree nodes.
    case DISPID_A_VISIBILITY:
    case DISPID_A_BACKGROUNDIMAGE:
    case DISPID_BACKCOLOR:
        fSpecialProperty = TRUE;
        fPropagateChange = TRUE;
        break;

    default:
        fPropagateChange = (dwFlags & (ELEMCHNG_REMEASUREINPARENT | ELEMCHNG_SIZECHANGED |
                                            ELEMCHNG_REMEASURECONTENTS | ELEMCHNG_REMEASUREALLCONTENTS));
        break;
    }

    if (fPropagateChange)
    {
        int            iRow;

        hr = pTableLayout->EnsureTableLayoutCache();
        if (hr)
            goto Cleanup;
        for (iRow = _iRow ; iRow < _iRow + _cRows ; iRow++)
        {
            CTableRow * pRow = pTableLayout->GetRow(iRow);
            Assert(pRow);

            hr = THR(pRow->PropagatePropertyChangeToCells(dispid, dwFlags, fSpecialProperty));
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableSection::destructor, CBase
//
//  Note:       The collection cache must be deleted in the destructor, and
//              not in Passivate, because collection objects we've handed out
//              (via get_rows) merely SubAddRef the table.
//-------------------------------------------------------------------------

CTableSection::~CTableSection()
{
    delete _pCollectionCache;
}

class CTableSectionRowsCollectionCacheItem : public CTableCollectionCacheItem
{
protected:
    CTableSection *_pSection;
public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(BldSectionRowsCol))
    CTableSectionRowsCollectionCacheItem(CTableSection *pSection) 
    {
        _pSection = pSection; 
        CTable *pTable = pSection->Table();
        if (pTable)
        {
            _pTableLayout = pTable->Layout(); 
        }
    }
    CElement *GetAt ( long lIndex );
    long Length ( void );
};


CElement *CTableSectionRowsCollectionCacheItem::GetAt ( long lIndex )
{
    Assert ( lIndex >= 0 );
    return (lIndex < _pSection->_cRows && _pTableLayout)? _pTableLayout->_aryRows[_pSection->_iRow + lIndex] : NULL;
}

long CTableSectionRowsCollectionCacheItem::Length ( void )
{
    return _pSection->_cRows;
}

//+------------------------------------------------------------------------
//
//  Member:     InvalidateCollections
//
//  Synopsis:   Invalidate the collection of the section and all
//              contained rows
//
//-------------------------------------------------------------------------

void CTableSection::InvalidateCollections(CTable *pTable)
{
    Assert(pTable);
    CTableLayout *pTableLayout = pTable->Layout();

    if (_pCollectionCache)
        _pCollectionCache->Invalidate();    // this will reset collection version number

    for (int i = _iRow; i < _iRow + _cRows; i++)
    {
        pTableLayout->_aryRows[i]->InvalidateCollections();
    }
    return;
}

//+------------------------------------------------------------------------
//
//  Member:     EnsureCollectionCache
//
//  Synopsis:   Create the table's collection cache if needed.
//
//-------------------------------------------------------------------------

HRESULT
CTableSection::EnsureCollectionCache( long lIndex )
{
    HRESULT hr = S_OK;

    CTable *pTable = Table();
    if (pTable)
    {
        CTableLayout * pTableLayout = pTable->Layout();
        Assert (pTableLayout);
        hr = pTableLayout->EnsureTableLayoutCache();
        if (hr)
            goto Cleanup;
    }

    if (!_pCollectionCache)
    {
        _pCollectionCache = new CCollectionCache(
                this,
                GetDocPtr(),
                (PFN_CVOID_ENSURE)&CTableSection::EnsureCollections);
        if (!_pCollectionCache)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitReservedCacheItems(1, 1));
        if (hr)
            goto Error;

        CTableSectionRowsCollectionCacheItem *pRowsCollection = new CTableSectionRowsCollectionCacheItem(this);
        if ( !pRowsCollection )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitCacheItem ( ROWS_COLLECTION, pRowsCollection ));
        if (hr)
            goto Cleanup;

        //
        // Collection cache now owns this item & is responsible for freeing it
        //
   }

Cleanup:
    RRETURN(hr);

Error:
    delete _pCollectionCache;
    _pCollectionCache = NULL;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     EnsureCollections
//
//  Synopsis:   Refresh the table rows collection, if needed.
//
//-------------------------------------------------------------------------

HRESULT
CTableSection::EnsureCollections(long lIndex, long * plCollectionVersion)
{
    CTable *pTable = Table();
    CTableLayout *pTableLayout = pTable? pTable->Layout() : NULL;
    HRESULT hr = S_OK;
    if ( pTableLayout)
    {
        hr = pTableLayout->EnsureTableLayoutCache();
        *plCollectionVersion = pTableLayout->_iCollectionVersion;   // to mark it done
    }
    RRETURN (hr);
}



const CTableCol::CLASSDESC CTableCol::s_classdesc =
{
    {
        &CLSID_HTMLTableCol,            // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NOLAYOUT,           // _dwFlags
        &IID_IHTMLTableCol,             // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTableCol       // _pfnTearOff
};


//+---------------------------------------------------------------------------
//
//  Member:     CTableSection::GetCellFromRowCol
//
//  Synopsis:   Returns the cell located at table grid position (iRow,iCol)
//
//  Arguments:  iRow [in]         -- visual row index
//              iCol [in]         -- visual col index
//              ppTableCell [out] -- table cell at (iRow,iCol)
//
//  Returns:    Returns S_OK with pointer to table cell.  The pointer will
//              be NULL when the cell at the specified position doesn't exist
//              or is a non-real cell part of another cell's row- or columnspan.
//
//  Note:       The rows and column indices are relative to the section
//              origin (section-top, section-left).
//
//----------------------------------------------------------------------------

HRESULT
CTableSection::GetCellFromRowCol(int iRow, int iCol, CTableCell **ppTableCell)
{
    CTableRow * pTableRow;
    CTableCell *pTableCell;
    HRESULT     hr = S_OK;
    CTableLayout * pTableLayout = Table()->Layout();

    if (!ppTableCell)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *ppTableCell = NULL;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (iRow < 0 || iRow >= _cRows || iCol < 0 || iCol >= pTableLayout->GetCols())
        goto Cleanup;

    // Obtain row from iRow.
    iRow = iRow + _iRow;
    Assert(iRow >= 0 && iRow < pTableLayout->GetRows());
    pTableRow = pTableLayout->_aryRows[iRow];
    Assert(pTableRow && !"NULL row in legal range");

    // Obtain col from iCol.
    Assert(pTableRow->Layout() && "Row without CTableRowLayout");
    pTableCell = pTableRow->Layout()->_aryCells[iCol];

    if (IsReal(pTableCell))
        *ppTableCell = pTableCell;

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableSection::GetCellsFromRowColRange
//
//  Synopsis:   Returns an array of the cells located in the inclusive range
//              spanned by (iRowTop,iColLeft)-(iRowBottom,iColRight)
//
//  Arguments:  iRowTop    [in]     -- visual top row index
//              iColLeft   [in]     -- visual left col index
//              iRowBottom [in]     -- visual row index
//              iColRight  [in]     -- visual col index
//              paryCells  [in,out] -- array of table cells in range (allocated
//                                     by caller)
//
//  Returns:    Returns S_OK with array of table cells.
//
//  Note:       The rows and column indices are relative to the section
//              origin (section-top, section-left).
//
//----------------------------------------------------------------------------

HRESULT
CTableSection::GetCellsFromRowColRange(int iRowTop, int iColLeft, int iRowBottom, int iColRight, CPtrAry<CTableCell *> *paryCells)
{
    CTableRow *    pTableRow;
    CTableCell *   pTableCell;
    HRESULT        hr;
    CTableLayout * pTableLayout = Table()->Layout();
    int            cRows, iRow, iCol;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    iRowTop = max(0, iRowTop);
    iRowBottom = min(pTableLayout->GetCols()-1, iRowBottom);
    iColLeft = max(0, iColLeft);
    iColRight = min(_cRows-1, iColRight);
    cRows = iRowBottom - iRowTop + 1;

    if (!paryCells || cRows <= 0 || iColLeft > iColRight)
    {
        hr = paryCells ? S_OK : E_POINTER;
        goto Cleanup;
    }

    // Loop from top row to bottom row.
    for (iRow = iRowTop + _iRow ; cRows ; cRows--, iRow++)
    {
        pTableRow = pTableLayout->_aryRows[iRow];
        Assert(pTableRow && !"NULL row in legal range");

        // Loop from left col to right col.
        for (iCol = iColLeft ; iCol <= iColRight ; iCol++)
        {
            Assert(pTableRow->Layout() && "Row without CTableRowLayout");
            pTableCell = pTableRow->Layout()->_aryCells[iCol];
            if (IsReal(pTableCell))
            {
                // Add cell to array.
                paryCells->Append(pTableCell);
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


HRESULT
CTableCol::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_COLGROUP) || pht->Is(ETAG_COL));
    Assert(ppElement);
    *ppElement = new CTableCol(pht->GetTag(), pDoc);
    return *ppElement ? S_OK : E_OUTOFMEMORY;
}


//+------------------------------------------------------------------------
//
//  Member:     CTableCol::EnterTree, CElement
//
//  Synopsis:   Add col/colgroup to table.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CTableCol::EnterTree()
{
    CTable       * pTable = Table();
    CTableLayout * pTableLayout = pTable? pTable->Layout() : NULL;
    HRESULT hr = S_OK;
    
    if(pTableLayout)
    {
        // Only maintain the table layout cache incrementally until the table
        // has finished parsing.
        if (pTableLayout->IsCompleted())
        {
            pTableLayout->MarkTableLayoutCacheDirty();
        }
        else
        {
            hr = ( Tag() == ETAG_COLGROUP ) ?
                THR(pTableLayout->AddColGroup(this)) :
                THR(pTableLayout->AddCol(this));
        }
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableCol::ComputeFormats
//
//  Synopsis:   Compute Char and Para formats induced by this element and
//              every other element above it in the HTML tree.
//
//  Arguments:  pCFI - Format Info needed for cascading
//
//  Returns:    HRESULT
//
//  Note:       We override this here to put our defaults into the format
//              FIRST, and also to cache vertical alignment here in the object
//
//-------------------------------------------------------------------------

HRESULT
CTableCol::ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget)
{
    SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    HRESULT hr;

    Assert(pNodeTarget && SameScope(this, pNodeTarget));

    //check column left

    if (_iCol > 0)
    {
        CTable * pTable = Table();

        if (!pTable)
            goto DontStealFormat;

        // Note:  We are not doing a pTableLayout->EnsureTableLayoutCache()
        // even though the current version might be dirty (can happen in
        // databinding).  In those cases we WANT to work with the old version
        // in case elements would disappear on us.

        CTableLayout * pTableLayout = pTable->Layout();

        WHEN_DBG( BOOL fDisableTLCAssert = pTableLayout->_fDisableTLCAssert; pTableLayout->_fDisableTLCAssert = TRUE; )
        CTableCol * pCol = pTableLayout->GetCol(_iCol - 1);
        WHEN_DBG( pTableLayout->_fDisableTLCAssert = fDisableTLCAssert; )

        CTreeNode * pNodeCol = pCol ? pCol->GetFirstBranch() : NULL;

        //
        // See comment in CTableRow::ComputeFormats
        //

        if (pCol && pNodeCol && pNodeCol->_iPF >= 0 && pNodeCol->_iCF >= 0 && pNodeCol->_iFF >= 0 &&
            !pCol->_fHasStyleExpressions &&
            SameScope(pNodeTarget->Parent(), pNodeCol->Parent()) &&
            (_pAA == NULL && pCol->_pAA == NULL ||
             _pAA != NULL && pCol->_pAA != NULL && _pAA->Compare(pCol->_pAA)) )
        {
            THREADSTATE * pts = GetThreadState();

            // At least one of the caches is dirty.
            Assert(pNodeTarget->_iPF == -1 || pNodeTarget->_iCF == -1 || pNodeTarget->_iFF == -1);

            // The caches that were not dirty, have to match with the corresponding caches of
            // the previous column.
            Assert(pNodeTarget->_iPF == -1 || pNodeTarget->_iPF == pNodeCol->_iPF);
            Assert(pNodeTarget->_iCF == -1 || pNodeTarget->_iCF == pNodeCol->_iCF);
            Assert(pNodeTarget->_iFF == -1 || pNodeTarget->_iFF == pNodeCol->_iFF);

            //
            // Selectively copy down each of the dirty caches.
            //

            if (pNodeTarget->_iPF == -1)
            {
                pNodeTarget->_iPF = pNodeCol->_iPF;
                (pts->_pParaFormatCache)->AddRefData( pNodeCol->_iPF );
                MtAdd(Mt(ParaFormatSteal), 1, 0);
            }

            if (pNodeTarget->_iCF == -1)
            {
                pNodeTarget->_iCF = pNodeCol->_iCF;
                (pts->_pCharFormatCache)->AddRefData( pNodeCol->_iCF );
                MtAdd(Mt(CharFormatSteal), 1, 0);
            }

            if (pNodeTarget->_iFF == -1)
            {
                pNodeTarget->_iFF = pNodeCol->_iFF;
                (pts->_pFancyFormatCache)->AddRefData( pNodeCol->_iFF );
                MtAdd(Mt(FancyFormatSteal), 1, 0);
            }

            // Set the _fBlockNess cache bit on the node to save a little time later.
            // Need to do this here because we don't call super.
            pNodeTarget->_fBlockNess = TRUE;

            hr = S_OK;
            goto Cleanup;
        }
    }

DontStealFormat:

    hr = THR(super::ComputeFormats(pCFI, pNodeTarget));

Cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     ApplyDefaultFormat
//
//  Synopsis:   Applies default formatting properties for that element to
//              the char and para formats passed in
//
//  Arguments:  pCF - charformat to apply default properties on
//              pPF - paraformat to apply default properties on
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CTableCol::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr;

    // clear back color, width/height and alignment coming from the table

    if (Tag() == ETAG_COLGROUP)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._cuvWidth.SetNull();
        pCFI->_ff()._cuvHeight.SetNull();
        pCFI->_ff()._bPageBreaks = 0;
        pCFI->UnprepareForDebug();

        pCFI->_bBlockAlign     = htmlBlockAlignNotSet;
        pCFI->_bCtrlBlockAlign = htmlBlockAlignNotSet;
    }

    hr = super::ApplyDefaultFormat(pCFI);
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureCols
//
//  Synopsis:   Make sure the table column count includes this column
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CTableCol::EnsureCols()
{
    CTableLayout * pTableLayout = Table()->Layout();
    HRESULT hr;
    int c;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    c = pTableLayout->GetCols();
    Assert (_iCol + Cols() <= c);

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CTableColCalc::AdjustMaxToUserWidth
//
//  Synopsis:   Adjust max width to user set width when appropriate
//
//-------------------------------------------------------------------------

void
CTableColCalc::AdjustMaxToUserWidth(CCalcInfo * pci, CTableLayout * pTableLayout)
{
    if (IsWidthSpecified() && !IsWidthSpecifiedInPercent())
    {
        _xMax = GetPixelWidth(pci, pTableLayout->ElementOwner(), 2 * pTableLayout->CellPaddingX());
    }
    if (_xMax < _xMin)
    {
        _xMax = _xMin;
    }
}

HRESULT
CTableCol::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT        hr = S_OK;
    CTable       * pTable = Table();
    CTableLayout * pTableLayout = pTable? pTable->Layout() : NULL;
    CTableCell   * pCell;
    CTableRow    * pRow;
    int            i, iLast, cR, iRow;

    hr = THR(super::OnPropertyChange(dispid, dwFlags));
    if(hr || !pTableLayout)
        goto Cleanup;

    hr = pTableLayout->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    iLast = _iCol + _cCols;

    // walk all the cells of this column and mark them dirty/clear caches
    for (cR = pTableLayout->GetRows(), iRow = pTableLayout->GetFirstRow();
        cR > 0;
        cR--, iRow = pTableLayout->GetNextRow(iRow))
    {
        pRow = pTableLayout->_aryRows[iRow];
        if ( pRow == NULL )
            continue;
        for (i = _iCol; i < iLast; i++)
        {
            pCell = Cell(pRow->Layout()->GetCell(i));
            if ( pCell == NULL )
                continue;
            hr = THR( pCell->OnPropertyChange(dispid, dwFlags) );
            if (hr)
                goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CTableCol::Notify, CSite
//
//  Synopsis:   Handle notification
//
//----------------------------------------------------------------------------

void
CTableCol::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        if (Table())
        {
            EnterTree();
        }
        break;
    }

    return;
}


//+---------------------------------------------------------------------------
//
//  Member:     CTableRow::Notify, CSite
//
//  Synopsis:   Handle notification
//
//----------------------------------------------------------------------------

void
CTableRow::Notify(CNotification *pNF)
{
    DWORD           dw = pNF->DataAsDWORD();

    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        {
            CTable          *pTable = Table();
            CTableLayout    *pTableLayout = pTable? pTable->Layout() : NULL;
            CTableRowLayout *pRowLayout = Layout();
            pRowLayout->_iRow = -1;

            if (!(dw & ENTERTREE_PARSE) && !(dw & ENTERTREE_MOVE))  // if the element cretead NOT via PARSing nor MOVing HTML
            {
                _fCompleted = TRUE; // we are not going to have NTYPE_END_PARSE notification to set the _fCompleted bit
            }
            if (pTableLayout)
            {
                if (!(dw & ENTERTREE_PARSE))
                {
                    pTableLayout->_fDontSaveHistory = TRUE;
                }
                if (dw & ENTERTREE_MOVE)   // if it is a MOVTREE notification
                {
                    // the subtree was moved
                    if (pTableLayout->_fPastingRows)
                    {
                        Assert (pTable->IsDatabound() && pTableLayout->IsRepeating());
                        pTableLayout->AddRow(this, FALSE);
                        pTableLayout->_cDirtyRows++;
                    }
                    else
                    {
                        pTableLayout->MarkTableLayoutCacheDirty();
                    }
                }
                else
                {
                    EnterTree();
                }
            }
            break;
        }

    case NTYPE_ELEMENT_EXITTREE_1:
        if (!(dw & EXITTREE_DESTROY))
        {
            CTable          *pTable = Table();
            CTableLayout    *pTableLayout = pTable? pTable->Layout() : NULL;

            if (!pTableLayout)
                break;

            if (!(dw & EXITTREE_MOVE))      // if it is not a move tree
            {

                // don't update the rows caches for individual rows in case of :1) shutdown; 2) data-binding removing rows
                if (!(pTableLayout->_fRemovingRows || pTable->_fExittreePending))
                {
                    // Don't hold refs to the tree after our element leaves it
                    ExitTree(pTableLayout);
                }
            }
        }
        break;

    case NTYPE_END_PARSE:
        {
            CTable          *pTable = Table();
            CTableLayout    *pTableLayout = pTable? pTable->Layout() : NULL;

            Assert (!_fCompleted && "NTYPE_END_PARSE notification happened more then once");

            _fCompleted = TRUE;     // loading/parsing of the row is complete.

            if (pTableLayout && pTableLayout->_fAllRowsSameShape && pTableLayout->_cCols != pTableLayout->_cTotalColSpan)
            {
                pTableLayout->_fAllRowsSameShape = FALSE;
            }
        }
        break;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     CTableRow::EnterTree, CElement
//
//  Synopsis:   Add row to table.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CTableRow::EnterTree()
{
    HRESULT hr = S_OK;

    CTable       * pTable = Table();
    CTableLayout * pTableLayout = pTable ? pTable->Layout() : NULL;

    if (pTableLayout)
    {
        // Only maintain the table layout cache incrementally until the table
        // has finished parsing.
        if (pTableLayout->IsCompleted() && !pTableLayout->_fTableOM)
        {
            pTableLayout->MarkTableLayoutCacheDirty();
        }
        else
        {
            pTableLayout->_cRowsParsed++;
            hr = pTableLayout->AddRow(this);
            if (hr)
                goto Cleanup;

            // BUGBUG: Databinding - we are going to let databound tables know if
            // there is a row inside a body section.  Sam wants to remove the
            // underlying function including this patch in the near future.  (OliverSe)
#ifndef NO_DATABINDING
            CTableSection * pSection = Section();
            if (GetFirstBranch() && pTableLayout && pSection && pSection->_fBodySection)
                pTableLayout->_fTemplateRows = TRUE;
#endif      // ndef NO_DATABINDING
        }
    }
Cleanup:
    RRETURN(hr);
}


void
CTableRow::ExitTree(CTableLayout *pTableLayout)
{
    CTableRowLayout *pRowLayout = Layout();
    
    Assert (pRowLayout);

    // 1. clear row layout cache
    pRowLayout->ClearRowLayoutCache();

    // 2. invalidate row collections
    InvalidateCollections();

    // 3. delete from the array of rows and update table caches accordingly
    pTableLayout->RowExitTree(pRowLayout->_iRow, Section());

    pRowLayout->_iRow = -1;
}


// TableRow needs to expose client* properties, but it doesn't
// inherit from IHTMLControlElement.  Since interfaces are
// immutable, to fix 60731 we add a new interface IHTMLTableRowMetrics
// to expose the client* properties.

//+----------------------------------------------------------
//
//  member  :   get_clientWidth, IHTMLTableRowMetrics
//
//  synopsis    :   returns a long value of the client window
//      width (not counting scrollbar, borders..)
//
//-----------------------------------------------------------

HRESULT
CTableRow::get_clientWidth( long * pl)
{
    HRESULT hr=S_OK;
    RECT    rect;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Layout()->GetClientRect(&rect, CLIENTRECT_VISIBLECONTENT);

    *pl = rect.right - rect.left;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientHeight, IHTMLTableRowMetrics
//
//  synopsis    :   returns a long value of the client window
//      Height of the body
//
//-----------------------------------------------------------

HRESULT
CTableRow::get_clientHeight( long * pl)
{
    HRESULT hr=S_OK;
    RECT    rect;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    Layout()->GetClientRect(&rect, CLIENTRECT_VISIBLECONTENT);

    *pl = rect.bottom - rect.top;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//+----------------------------------------------------------
//
//  member  :   get_clientTop, IHTMLTableRowMetrics
//
//  synopsis    :   TableRows can't have borders (in 4.x), so
//      just return 0.  This needs to exist so that scripts
//      can walk up the offsetParent chain from inside tables
//      and sum clientTops + offsetTops.
//
//-----------------------------------------------------------

HRESULT
CTableRow::get_clientTop( long * pl)
{
    HRESULT hr=S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}


//+----------------------------------------------------------
//
//  member  :   get_clientLeft, IHTMLTableRowMetrics
//
//  synopsis    :   TableRows can't have borders (in 4.x), so
//      just return 0.  This needs to exist so that scripts
//      can walk up the offsetParent chain from inside tables
//      and sum clientTops + offsetTops.
//
//-----------------------------------------------------------

HRESULT
CTableRow::get_clientLeft( long * pl)
{
    HRESULT hr=S_OK;

    if (!pl)
    {
        hr = E_POINTER;
        goto Cleanup;
    }

    *pl = 0;

Cleanup:
    RRETURN(SetErrorInfo( hr ));
}

//-------------------------------------------------------------------------
//
// Layout related functions
//
//-------------------------------------------------------------------------

IMPLEMENT_LAYOUT_FNS(CTableRow, CTableRowLayout)
