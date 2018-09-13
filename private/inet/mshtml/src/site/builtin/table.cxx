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

#include <headers.hxx>

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_TABLE_HXX_
#define X_TABLE_HXX_
#include "table.hxx"
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include <download.hxx>
#endif

#ifndef X_OTHRGUID_H_
#define X_OTHRGUID_H_
#include <othrguid.h>
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>
#endif

#ifndef X_DETAIL_HXX_
#define X_DETAIL_HXX_
#include <detail.hxx>
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include <treepos.hxx>
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include <ltable.hxx>
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include <ltrow.hxx>
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#define _cxx_
#include "table.hdl"

MtDefine(CTable, Elements, "CTable")
MtDefine(BldRowsCol, PerfPigs, "Build CTable::TABLE_ROWS_COLLECTION")
MtDefine(BldBodysCol, PerfPigs, "Build CTable::TABLE_BODYS_COLLECTION")
MtDefine(BldCellsCol, PerfPigs, "Build CTable::TABLE_CELLS_COLLECTOIN")

ExternTag(tagTableRecalc);
extern void WriteHelp(HANDLE hFile, TCHAR *format, ...);
extern void WriteString(HANDLE hFile, TCHAR *pszStr);


ELEMENT_TAG s_atagTSection[] = {ETAG_TBODY, ETAG_THEAD, ETAG_TFOOT, ETAG_NULL};

#ifndef NO_PROPERTY_PAGE
const CLSID * CTable::s_apclsidPages[] =
{
    // Browse-time pages
    NULL,
    // Edit-time pages
#if DBG==1    
    &CLSID_CCDGenericPropertyPage,
    &CLSID_CInlineStylePropertyPage,
#endif // DBG==1    
    NULL
};
#endif // NO_PROPERTY_PAGE


const CElement::CLASSDESC CTable::s_classdesc =
{
    {
        &CLSID_HTMLTable,               // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NEVERSCROLL    |
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOPCTRESIZE,        // _dwFlags
        &IID_IHTMLTable,                // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLTable,         // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};


HRESULT
CTable::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_TABLE));
    Assert(ppElement);

    *ppElement = new CTable(pDoc);
    RRETURN( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}


CTable::CTable(CDoc *pDoc)
    : super(ETAG_TABLE, pDoc)
{
    _fOwnsRuns    = TRUE;
}


//+------------------------------------------------------------------------
//
//  Member:     CTable::destructor, CBase
//
//  Note:       The collection cache must be deleted in the destructor, and
//              not in Passivate, because collection objects we've handed out
//              (via get_rows) merely SubAddRef the table.
//-------------------------------------------------------------------------

CTable::~CTable()
{
    delete _pCollectionCache;

}


//+------------------------------------------------------------------------
//
//  Member:     CTable::Notify
//
//  Synopsis:   Catch the leave-tree notification
//
//-------------------------------------------------------------------------

void
CTable::Notify(CNotification *pNF)
{
    CTableLayout *  pTableLayout;
    HRESULT         hr;
    DWORD           dw;

    Assert(HasLayout());
    pTableLayout = DYNCAST(CTableLayout, GetCurLayout());

    if (pNF->IsType(NTYPE_ELEMENT_EXITTREE_1))
    {
        _fEnableDatabinding = FALSE;
    }

    super::Notify(pNF);

    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_ENTERTREE:
        dw = pNF->DataAsDWORD();
        if (!(dw & ENTERTREE_PARSE))
        {
            pTableLayout->_fDontSaveHistory = TRUE;
        }
        if (!(dw & ENTERTREE_PARSE) && !(dw & ENTERTREE_MOVE))  // if the element cretead NOT via PARSing nor MOVing HTML
        {
            pTableLayout->_fCompleted = TRUE; // we are not going to have NTYPE_END_PARSE notification to set the _fCompleted bit
        }
        _fEnableDatabinding = TRUE;

        // Load the history stream
        IGNORE_HR(Doc()->GetLoadHistoryStream(GetSourceIndex(), 
                                             HistoryCode(), 
                                             &_pStreamHistory));

        _iDocDotWriteVersionEnterTree = Doc()->_iDocDotWriteVersion;
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        pTableLayout->ClearTopTableLayoutCache(); // Don't hold refs to the tree after our element leaves it

        // Whenever we clear the TLC, we have to mark it dirty in case the element
        // reenters the tree at some point.
        pTableLayout->MarkTableLayoutCacheDirty();

        // 47172: Make sure we don't think we're hanging on to the caption dispnode.
        pTableLayout->_fHasCaptionDispNode = FALSE;
        if (pTableLayout->_pTableBorderRenderer)
            pTableLayout->_pTableBorderRenderer->_pDispNode = NULL;

        ClearInterface(&_pStreamHistory);
        break;

        break;

    case NTYPE_END_PARSE:
        Assert (!pTableLayout->_fCompleted && "NTYPE_END_PARSE notification happened more then once");

        if (_iDocDotWriteVersionEnterTree != Doc()->_iDocDotWriteVersion)
        {
            // there were document.write in between <TABLE> and </TABLE> parsing
            pTableLayout->_fDontSaveHistory = TRUE;
            ClearInterface(&_pStreamHistory);     // don't use history
            if (pTableLayout->_fUsingHistory)     // if we are already using History
            {
                if(pTableLayout->_fCompleted)
                    pTableLayout->Resize();       // then, ensure full resize.
            }
        }
        if (!pTableLayout->_fCompleted)           // (even with the above Assert), make sure we don't do this more then ONCE
        {
            hr = pTableLayout->EnsureCells();
            if (hr)
                return;

            hr = pTableLayout->ensureTBody();
            if (hr)
                return;

            pTableLayout->_fCompleted = TRUE;     // loading/parsing of the table is complete.

    #ifndef NO_DATABINDING
            DBMEMBERS * pdbm = GetDBMembers();
            if (pdbm)
            {
                pdbm->MarkReadyToBind();
            }
    #endif // ndef NO_DATABINDING

            ResizeElement();
        }
        break;

    case NTYPE_SAVE_HISTORY_1:
        if (   !pTableLayout->_fDontSaveHistory  
            && pTableLayout->_fCompleted
            && pTableLayout->_fCalcedOnce
            && !pTableLayout->_fTLCDirty
            && pTableLayout->_aryRows.Size() >= 1
            && pTableLayout->GetCols() >= 1
            && !pTableLayout->IsFixed() 
            && !IsDatabound())
        {
            pNF->SetSecondChanceRequested();
        }
        break;

    case NTYPE_SAVE_HISTORY_2:
        // Do not save history for small tables or for tables that are already using "fixed" style
        //  or for tables that haven't been finished being parsed (bug 57537)
        if (   !pTableLayout->_fDontSaveHistory  
            && pTableLayout->_fCompleted
            && pTableLayout->_fCalcedOnce
            && !pTableLayout->_fTLCDirty
            && pTableLayout->_aryRows.Size() >= 1
            && pTableLayout->GetCols() >= 1
            && !pTableLayout->IsFixed() 
            && !IsDatabound())
        {
            CHistorySaveCtx *   phsc;

            pNF->Data((void **)&phsc);
            hr = pTableLayout->SaveHistoryValue(phsc);
        }

        break;
    }

    return;
}


//+----------------------------------------------------------------------------
//
//  Member:     CTable::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CTable::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLTable)
        QI_HTML_TEAROFF(this, IHTMLElement2, NULL)
        QI_HTML_TEAROFF(this, IHTMLTable, NULL)
        QI_TEAROFF(this, IHTMLTable2, NULL)
        QI_TEAROFF_DISPEX(this, NULL)
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
//  Member:     CTable::Init2(), CElement
//
//  Synopsis:   called after calls to: Init(), InitAttrBag()
//
//-------------------------------------------------------------------------

HRESULT
CTable::Init2(CInit2Context * pContext)
{
    HRESULT hr;

    hr = THR(super::Init2(pContext));
    if (!OK(hr))
        goto Cleanup;

    _readyStateTable = FormsIsEmptyString(GetAAdataSrc()) ? READYSTATE_COMPLETE : READYSTATE_LOADING;

Cleanup:
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
CTable::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    const CCharFormat * pCF;
    CColorValue ccvBorderColor;
    CTreeNode * pNodeBody;
    BOOL        fVisibilityHidden;
    BOOL        fDisplayNone;
    BOOL        fHasBgImage;
    BOOL        fHasBgColor;
    BOOL        fDisabled;
    BOOL        fRelative;
    BOOL        fNoBreak;
    BOOL        fHaveBorderColor = FALSE;
    LONG        fRTL;
    LONG        lCursorIndex;
    COLORREF    cr;
    int         i;
    HRESULT     hr = S_OK;
    CTableLayout *pTableLayout = Layout();
    CDoc *      pDoc = Doc();
    BOOL        fCharGridSizeOverride;
    BOOL        fLineGridSizeOverride;
    BYTE        uLayoutGridType;
    BYTE        uLayoutGridTypeInner;
    BYTE        uLayoutGridMode;
    BYTE        uLayoutGridModeInner;
    BOOL        fHasGridValues;

    pCFI->PrepareCharFormat();
    pCFI->PrepareFancyFormat();

    // cache the values before init default
    fVisibilityHidden = pCFI->_cf()._fVisibilityHidden;
    fDisplayNone      = pCFI->_cf()._fDisplayNone;
    fRTL              = pCFI->_cf()._fRTL;
    fHasBgColor       = pCFI->_cf()._fHasBgColor;
    fHasBgImage       = pCFI->_cf()._fHasBgImage;
    fDisabled         = pCFI->_cf()._fDisabled;
    fRelative         = pCFI->_cf()._fRelative;
    lCursorIndex      = pCFI->_cf()._bCursorIdx;
    fNoBreak          = pCFI->_cf()._fNoBreak;
    pCFI->_bCtrlBlockAlign  = pCFI->_bBlockAlign;
    pCFI->_bBlockAlign      = htmlBlockAlignNotSet;
    fCharGridSizeOverride   = pCFI->_cf()._fCharGridSizeOverride;
    fLineGridSizeOverride   = pCFI->_cf()._fLineGridSizeOverride;
    uLayoutGridType         = pCFI->_cf()._uLayoutGridType;
    uLayoutGridTypeInner    = pCFI->_cf()._uLayoutGridTypeInner;
    uLayoutGridMode         = pCFI->_cf()._uLayoutGridMode;
    uLayoutGridModeInner    = pCFI->_cf()._uLayoutGridModeInner;
    fHasGridValues          = pCFI->_cf()._fHasGridValues;

    if (!pDoc->PaintBackground())
    {
        cr = 0;
        ccvBorderColor.SetValue(cr, FALSE);
    }
    else
    {
        ccvBorderColor = GetAAborderColor();
        if (ccvBorderColor.IsDefined())
        {
            fHaveBorderColor = TRUE;

            cr = ccvBorderColor.GetColorRef();
        }
        else
        {
            CLayout * pParentLayout = GetUpdatedParentLayout();
            cr = GetSysColorQuick(COLOR_BTNHIGHLIGHT);
            if (pParentLayout && (0x00FFFFFF & pParentLayout->ElementOwner()->GetInheritedBackgroundColor()) == (0x00FFFFFF & cr))
            {
                cr = GetSysColorQuick(COLOR_BTNFACE);
            }

            ccvBorderColor.SetValue(cr, FALSE);
        }

        pCFI->_ff()._ccvBorderColorLight   =
        pCFI->_ff()._ccvBorderColorHilight = GetAAborderColorLight();
        pCFI->_ff()._ccvBorderColorDark    =
        pCFI->_ff()._ccvBorderColorShadow  = GetAAborderColorDark();
    }

    for ( i = BORDER_TOP; i <= BORDER_LEFT; i++ )
    {
        pCFI->_ff()._ccvBorderColors[ i ] = ccvBorderColor;
    }

    if (!pCFI->_ff()._ccvBorderColorLight.IsDefined())
        pCFI->_ff()._ccvBorderColorLight.SetValue(cr, FALSE);
    if (!pCFI->_ff()._ccvBorderColorHilight.IsDefined())
        pCFI->_ff()._ccvBorderColorHilight.SetValue(cr, FALSE);

    if (pDoc->PaintBackground() && !fHaveBorderColor)
    {
        cr = GetSysColorQuick(COLOR_BTNSHADOW);
    }

    if (!pCFI->_ff()._ccvBorderColorDark.IsDefined())
        pCFI->_ff()._ccvBorderColorDark.SetValue(cr, FALSE);
    if (!pCFI->_ff()._ccvBorderColorShadow.IsDefined())
        pCFI->_ff()._ccvBorderColorShadow.SetValue(cr, FALSE);

    // Tables don't allow any character formatting information to leak through
    // except for font names & inline style display, and layout grid
    pCFI->_cf().InitDefault(
                pDoc->_pOptionSettings,
                pDoc->_pCodepageSettings,
                pCFI->_cf()._fExplicitFace);


    // restore of init default
    pCFI->_cf()._fVisibilityHidden = fVisibilityHidden;
    pCFI->_cf()._fDisplayNone      = fDisplayNone;
    pCFI->_cf()._fRTL              = fRTL;
    pCFI->_cf()._fHasBgColor       = fHasBgColor;
    pCFI->_cf()._fHasBgImage       = fHasBgImage;
    pCFI->_cf()._fDisabled         = fDisabled;
    pCFI->_cf()._fRelative         = fRelative;
    pCFI->_cf()._bCursorIdx        = lCursorIndex;
    pCFI->_cf()._fNoBreak          = fNoBreak;
    pCFI->_cf()._fCharGridSizeOverride  = fCharGridSizeOverride;
    pCFI->_cf()._fLineGridSizeOverride  = fLineGridSizeOverride;
    pCFI->_cf()._uLayoutGridType        = uLayoutGridType;
    pCFI->_cf()._uLayoutGridTypeInner   = uLayoutGridTypeInner;
    pCFI->_cf()._uLayoutGridMode        = uLayoutGridMode;
    pCFI->_cf()._uLayoutGridModeInner   = uLayoutGridModeInner;
    pCFI->_cf()._fHasGridValues         = fHasGridValues;

    // Now get the text color default value from the body tag.
    // We do it this way because color attribute values don't leak into
    // tables.
    pNodeBody = GetFirstBranch()->SearchBranchToRootForTag( ETAG_BODY );
    if (pNodeBody)
    {
        pCF = pNodeBody->GetCharFormat();
        pCFI->_cf()._ccvTextColor = pCF->_ccvTextColor;
    }

    // This is where to put the code to inherit character attributes created by
    // styles.
    /* */

    pCFI->PrepareParaFormat();

    // set up for potential EMs, ENs, and ES Conversions
    pCFI->_pf()._lFontHeightTwips = pCFI->_cf().GetHeightInTwips( pDoc );
    if (pCFI->_pf()._lFontHeightTwips <=0)
        pCFI->_pf()._lFontHeightTwips = 1;

    // Tables don't inherit vertical alignment from containing tables.
    pCFI->_pf()._bTableVAlignment = htmlCellVAlignNotSet;

    // iniitalize the default paragraph property for PRE, if we are inside PRE it shouldn't effect the cells
    pCFI->_pf()._fPre      = FALSE;
    pCFI->_pf()._fPreInner = FALSE;
    pCFI->_pf()._cuvTextIndent = 0;

    pCFI->UnprepareForDebug();

    hr = super::ApplyDefaultFormat( pCFI );

    pCFI->PrepareFancyFormat();

    pTableLayout->_fFixed = pCFI->_ff()._bTableLayout;
    pTableLayout->_fCollapse = pCFI->_ff()._bBorderCollapse;

    // Clear the border default and the cache on table.
    pCFI->_ff()._fOverrideTablewideBorderDefault = FALSE;
    pTableLayout->_fBorderInfoCellDefaultCached = FALSE;

    pCFI->UnprepareForDebug();

    RRETURN( hr );
}



//+------------------------------------------------------------------------
//
//  Collection cache items implementation for TABLE collection
//
//-------------------------------------------------------------------------

class CTableRowsCollectionCacheItem : public CTableCollectionCacheItem
{
    typedef CTableCollectionCacheItem super;

protected:
#if NEED_A_SOURCE_ORDER_ITERATOR
    unsigned    _fAryRowsInSourceOrder: 1;
#else
    int         _iRow;  // index to _aryRows array for _lCurrentIndex
#endif

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(BldRowsCol))
    CTableRowsCollectionCacheItem(CTable *pTable) {_pTableLayout = pTable->Layout(); }
    CElement *GetNext (void);
    CElement *GetAt ( long lIndex );
    long Length ( void );
};

class CTableBodysCollectionCacheItem : public CTableCollectionCacheItem
{
    typedef CTableCollectionCacheItem super;

public:
    DECLARE_MEMCLEAR_NEW_DELETE(Mt(BldBodysCol))
    CTableBodysCollectionCacheItem(CTable *pTable) {_pTableLayout = pTable->Layout(); }
    CElement *GetAt ( long lIndex );
    long Length ( void );
};

//+------------------------------------------------------------------------
//
//  Generic TABLE collection (abstract class)
//
//-------------------------------------------------------------------------
CElement *
CTableCollectionCacheItem::GetNext ( void )
{
    return GetAt ( _lCurrentIndex++ );
}

CElement *
CTableCollectionCacheItem::MoveTo ( long lIndex )
{
    _lCurrentIndex = lIndex;
    return GetAt(lIndex);
}

//+------------------------------------------------------------------------
//
//  TABLE ROWS collection
//
//-------------------------------------------------------------------------

CElement *
CTableRowsCollectionCacheItem::GetNext ( void )
{
    if (_lCurrentIndex++ < _pTableLayout->_aryRows.Size())
    {
        CTableRow * pRow = _pTableLayout->_aryRows[_iRow];
        _iRow = _pTableLayout->GetNextRow(_iRow);
        return pRow;
    }
    return NULL;
}

CElement *
CTableRowsCollectionCacheItem::GetAt ( long lIndex )
{
    Assert ( lIndex >= 0 );
    if (lIndex < _pTableLayout->_aryRows.Size())
    {
#if NEED_A_SOURCE_ORDER_ITERATOR
        return _fAryRowsInSourceOrder
            ? _pTableLayout->_aryRows[lIndex] 
            : _pTableLayout->GetRowInSourceOrder(lIndex)
#else
        _iRow = _pTableLayout->VisualRow2Index(lIndex);
        _lCurrentIndex = lIndex;
        return _pTableLayout->_aryRows[_iRow];
#endif
    }
    return NULL;
}

long 
CTableRowsCollectionCacheItem::Length ( void )
{
#if NEED_A_SOURCE_ORDER_ITERATOR
    _fAryRowsInSourceOrder = _pTableLayout->IsAryRowsInSourceOrder();
#endif
    return _pTableLayout->_aryRows.Size();
}

//+------------------------------------------------------------------------
//
//  TABLE BODYS collection
//
//-------------------------------------------------------------------------

CElement *
CTableBodysCollectionCacheItem::GetAt ( long lIndex )
{
    Assert ( lIndex >= 0 );
    return (lIndex < _pTableLayout->_aryBodys.Size())?_pTableLayout->_aryBodys[lIndex] : NULL;
}

long 
CTableBodysCollectionCacheItem::Length ( void )
{
    return _pTableLayout->_aryBodys.Size();
}


//+------------------------------------------------------------------------
//
//  TABLE CELLS collection
//
//-------------------------------------------------------------------------

CElement *
CTableCellsCollectionCacheItem::MoveTo ( long lIndex )
{
    int iRowIndex;

    _lCurrentCellIndex = lIndex;
    iRowIndex = SetCurrentRowAndGetRowIndex(lIndex);
    if (iRowIndex >= 0)
    {
        return super::MoveTo(iRowIndex);
    }
    return NULL;
}

CElement *
CTableCellsCollectionCacheItem::GetNext ( void )
{
    _lCurrentCellIndex++;
    if (_pRowLayout && _lCurrentIndex < _pRowLayout->_cRealCells)
    {
        return super::GetNext();
    }
    else
    {
        int iRow = _pRowLayout? _pTableLayout->GetNextRowSafe(_pRowLayout->_iRow) : _pTableLayout->GetFirstRow();  // get next row in visual order
        while (iRow < _pTableLayout->_aryRows.Size())
        {
            CTableRowLayout *pRowLayout = _pTableLayout->_aryRows[iRow]->Layout();
            if (pRowLayout)
            {
                _pRowLayout = pRowLayout;
                if (CTableRowCellsCollectionCacheItem::MoveTo(0))   // if there are cells in this row 
                    return CTableRowCellsCollectionCacheItem::GetNext();    //then return the 0 and prepare for the next
                else
                    iRow = _pTableLayout->GetNextRowSafe(_pRowLayout->_iRow);
            }
        }
    }
    return NULL;
}


CElement *
CTableCellsCollectionCacheItem::GetAt ( long lIndex )
{
    int iRowIndex = SetCurrentRowAndGetRowIndex(lIndex);
    if (iRowIndex >= 0)
    {
        return super::GetAt(iRowIndex);
    }
    return NULL;
}

int  
CTableCellsCollectionCacheItem::SetCurrentRowAndGetRowIndex(int lIndex)
{
    int cRows = _pTableLayout->_aryRows.Size();
    if (cRows)
    {
        int cR, iRow;
        int cRealCells = 0;
        for (cR = cRows, iRow = _pTableLayout->GetFirstRow();
            cR > 0;
            cR--, iRow = _pTableLayout->GetNextRow(iRow))   // get next row in visual order
        {
            _pRowLayout = _pTableLayout->_aryRows[iRow]->Layout();
            if (lIndex < cRealCells + _pRowLayout->_cRealCells)
            {
                break;
            }
            cRealCells += _pRowLayout->_cRealCells;
        }

        return lIndex - cRealCells;   // relative to the row index
    }
    return -1;
}

long 
CTableCellsCollectionCacheItem::Length ( void )
{
    int i, cRows;
    int cRealCells = 0;
    cRows = _pTableLayout->_aryRows.Size();
    for (i = 0; i < cRows; i++)
    {
        cRealCells += _pTableLayout->_aryRows[i]->Layout()->_cRealCells;
    }
    return cRealCells;
}

//+------------------------------------------------------------------------
//
//  Member:     EnsureCollectionCache
//
//  Synopsis:   Create the table's collection cache if needed.
//
//-------------------------------------------------------------------------

HRESULT
CTable::EnsureCollectionCache()
{
    HRESULT hr = S_OK;

    hr = Layout()->EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    if (!_pCollectionCache)
    {
        _pCollectionCache = new CCollectionCache(
                this,          // double cast needed for Win16.
                GetDocPtr(),
                ENSURE_METHOD(CTable, EnsureCollections, ensurecollections));
        if (!_pCollectionCache)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitReservedCacheItems(NUMBER_OF_TABLE_COLLECTION, NUMBER_OF_TABLE_COLLECTION));
        if (hr)
            goto Error;

        CTableRowsCollectionCacheItem *pRowsCollection = new CTableRowsCollectionCacheItem(this);
        if ( !pRowsCollection )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitCacheItem ( TABLE_ROWS_COLLECTION, pRowsCollection ));
        if (hr)
            goto Cleanup;

        CTableBodysCollectionCacheItem *pBodysCollection = new CTableBodysCollectionCacheItem(this);
        if ( !pBodysCollection )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitCacheItem ( TABLE_BODYS_COLLECTION, pBodysCollection ));
        if (hr)
            goto Cleanup;

        CTableCellsCollectionCacheItem *pCellsCollection = new CTableCellsCollectionCacheItem(this);
        if ( !pCellsCollection )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        hr = THR(_pCollectionCache->InitCacheItem ( TABLE_CELLS_COLLECTION, pCellsCollection ));
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
CTable::EnsureCollections(long lIndex, long * plCollectionVersion)
{
    Assert ( lIndex < NUMBER_OF_TABLE_COLLECTION  && plCollectionVersion); // parameter validation

    HRESULT hr = S_OK;

    CTableLayout *pTableLayout = Layout();
    if ( pTableLayout)
    {
        hr = pTableLayout->EnsureTableLayoutCache();
        *plCollectionVersion = pTableLayout->_iCollectionVersion;   // to mark it done
    }

    RRETURN (hr);
}



//+---------------------------------------------------------------------------
//
//  Member: CTable::GetInfo
//
//  Params: [gi]: The GETINFO enumeration.
//
//  Descr:  Returns the information requested in the enum
//
//----------------------------------------------------------------------------
DWORD
CTable::GetInfo(GETINFO gi)
{
    switch (gi)
    {
    case GETINFO_ISCOMPLETED:
        return (Layout()->_fCompleted ? TRUE : FALSE);
    }

    return super::GetInfo(gi);
}


#if DBG == 1
TCHAR g_achTabs[] = _T("\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t");
#define PRINTLN(f) WriteHelp(pF, _T("<0s>")_T(##f)_T("\n"), &g_achTabs[ARRAY_SIZE(g_achTabs) - iTabs]


void
CTableCol::Print(HANDLE pF, int iTabs)
{
    TCHAR           achBuf[30];
    const TCHAR *   psz;

    PRINTLN("\n*** COL ***\n") );
    psz = GetAAid();
    if (psz)
        PRINTLN("ID: <1s>"), psz);
    psz = GetAAname();
    if (psz)
        PRINTLN("NAME: <1s>"), psz);
    PRINTLN("_iCol: <1d>"), _iCol);
    PRINTLN("_cCols: <1d>"), _cCols);
    PRINTLN("CELLS = <1s>"), achBuf);

    GetFirstBranch()->GetCascadedwidth().FormatBuffer(achBuf, ARRAY_SIZE(achBuf), &s_propdescCTableColwidth.a);
    PRINTLN("WIDTH = <1s>"), achBuf);
}

void
CTableColCalc::Print(HANDLE pF, int iTabs)
{
    TCHAR achBuf[30];

    PRINTLN("\n*** COLCALC ***\n") );
    PRINTLN("_xMin: <1d>"), _xMin); // Moved to ColCalc
    PRINTLN("_xMax: <1d>"), _xMax);
    PRINTLN("_xWidth: <1d>"), _xWidth);
    _uvWidth.FormatBuffer(achBuf, ARRAY_SIZE(achBuf), &s_propdescCTableColwidth.a);
}


#endif // DBG == 1


//+------------------------------------------------------------------------
//
//  Member :    InvalidateCollections()
//
//  Synopsis :  invalidate the table's and documents collection caches
//
//-------------------------------------------------------------------------


void CTable::InvalidateCollections()
{
    CTableLayout * pTableLayout = Layout();

    if (pTableLayout)
    {
        pTableLayout->_iCollectionVersion++;  // this will tell the cache collection manager that our collection is updated

        if (_pCollectionCache)
            _pCollectionCache->Invalidate();  // this will reset collection version number

        for (int i = 0; i < pTableLayout->_aryBodys.Size(); i++)
        {
            pTableLayout->_aryBodys[i]->InvalidateCollections(this);
        }

        if (pTableLayout->_pHead)
        {
            pTableLayout->_pHead->InvalidateCollections(this);
        }
        if (pTableLayout->_pFoot)
        {
            pTableLayout->_pFoot->InvalidateCollections(this);
        }
    }
}


HRESULT
CTable::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT        hr;
    CTableCaption *pCaption;
    CTableCaption **ppCaption;
    int            cC;
    CTableLayout  *pTableLayout = Layout();

    switch(dispid)
    {
    case DISPID_CTable_cellSpacing: 
        // note, the cellSpacing is a part of the padding for the captions,
        // so we need make sure we remeasure them

        for (cC = pTableLayout->_aryCaptions.Size(), ppCaption = pTableLayout->_aryCaptions;
             cC > 0;
             cC--, ppCaption++)
        {

            pCaption = (*ppCaption);
            if (pCaption->Tag() == ETAG_CAPTION)
            {
                pCaption->RemeasureElement(0);
            }
        }
        pTableLayout->ResetMinMax();

        // Make sure super is asking for a layout.
        Assert(dwFlags | ELEMCHNG_SIZECHANGED);
        break;
         
    case DISPID_CTable_cellPadding: // "
        dwFlags |= ELEMCHNG_REMEASUREALLCONTENTS;

    case DISPID_A_BORDERWIDTH:      // CalculateBorderAndSpacing
    case DISPID_A_BORDERTOPWIDTH:   // "
    case DISPID_A_BORDERRIGHTWIDTH: // "
    case DISPID_A_BORDERBOTTOMWIDTH:// "
    case DISPID_A_BORDERLEFTWIDTH:  // "
    case DISPID_CTable_border:      // "

        // Whenever a table (border) property change has the ability
        // to toggle table cell borders on and off, we need to
        // remeasure all contents, so that the dispnodes can be updated.
        // If the table is collapsing borders and the borders on the
        // table are toggled on or off, this affects cell layouts
        // because their borders along the table edges be deactivated
        // or activated.  (45145, 50744)
        if (pTableLayout->CollapseBorders() || DISPID_CTable_border == dispid)
            dwFlags |= ELEMCHNG_REMEASUREALLCONTENTS;

        // fall through
    case DISPID_CTable_width:       // because Calc'MinMax sets MM to xWidth.
    case DISPID_A_BORDERCOLLAPSE:   // because every cell minmax will change

        pTableLayout->ResetMinMax();

        // Make sure super is asking for a layout.
        Assert(dwFlags | ELEMCHNG_SIZECHANGED);
        break;
    }

    hr = THR(super::OnPropertyChange(dispid, dwFlags));

    RRETURN(hr);
}

CTableRow *CTableCell::Row() const
{
    return DYNCAST(CTableRow, GetParentAncestorSafe(ETAG_TR));
}

CTableSection *CTableCell::Section() const
{
    return DYNCAST(CTableSection, GetParentAncestorSafe(s_atagTSection));
}

CTable *CTableCell::Table() const
{
    return DYNCAST(CTable, GetParentAncestorSafe(ETAG_TABLE));
}

CTable *CTableRow::Table() const
{
    return DYNCAST(CTable, GetParentAncestorSafe(ETAG_TABLE));
}

CTableSection *CTableRow::Section() const
{
    return DYNCAST(CTableSection, GetParentAncestorSafe(s_atagTSection));
}

CTable *CTableSection::Table() const
{
    return DYNCAST(CTable, GetParentAncestorSafe(ETAG_TABLE));
}

CTable *CTableCol::Table() const
{
    return DYNCAST(CTable, GetParentAncestorSafe(ETAG_TABLE));
}

CTableCalcInfo::CTableCalcInfo(CCalcInfo * pci, CTable *pTable, CTableLayout *pTableLayout)
{
    Init(pci);
    _dwFlags = 0;
    _pme = NULL;
    _pTable = pTable;
    _pTableLayout = pTableLayout;
    _fTableCalcInfo = TRUE;
    if (pci->_fTableCalcInfo)
        _cNestedCalcs = ((CTableCalcInfo *)pci)->_cNestedCalcs + 1;
    else
        _cNestedCalcs = 0;

}

CTableCalcInfo::CTableCalcInfo(CTable *pTable, CTableLayout *pTableLayout)
        :CCalcInfo (pTableLayout)
{
    _dwFlags = 0;
    _pme = NULL;
    _pTable = pTable;
    _pTableLayout = pTableLayout;
    _fTableCalcInfo = TRUE;
    Assert (pTableLayout->_cNestedLevel != - 1);
    _cNestedCalcs = pTableLayout->_cNestedLevel;
}

// implementation of CTable::CreateLayout()
IMPLEMENT_LAYOUT_FNS(CTable, CTableLayout)
