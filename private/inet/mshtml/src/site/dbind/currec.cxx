//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1996-1997
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  Contents:   Current Record Instance objects
//
//  History:    10/1/96     (sambent) created

#include <headers.hxx>

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#ifndef X_ROWBIND_HXX_
#define X_ROWBIND_HXX_
#include "rowbind.hxx"
#endif

#ifndef X_OLEDBERR_H_
#define X_OLEDBERR_H_
#include <oledberr.h>
#endif

/////////////////////////////////////////////////////////////////////////////
/////                 CCurrentRecordInstance methods                    /////
/////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
// Member:      constructor (public)
//
// Synopsis:    initialize to dormant state

CCurrentRecordInstance::CCurrentRecordInstance():
    _ulRefCount(1), _priCurrent(0), _hrow(0)
{
}


//+-------------------------------------------------------------------------
// Member:      Init (public)
//
// Synopsis:    Associate my record instance with a RowPosition
//
// Arguments:   pRowPos     RowPosition to attach
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::Init(CDataSourceProvider *pProvider, IRowPosition *pRowPos)
{
    Assert("must be passive to attach" && !_priCurrent);
    Assert("need provider" && pProvider);

    HRESULT hr = S_OK;
    IConnectionPointContainer *pCPC = 0;

    // remember my owner
    _pProvider = pProvider;
    
    // hold on to the RowPosition (let go in Detach)
    _pRowPos = pRowPos;

    if (_pRowPos)
    {
        _pRowPos->AddRef();
        
        // sink notifications from the RowPosition
        hr = _pRowPos->QueryInterface(IID_IConnectionPointContainer,
                                               (void **)&pCPC );
        if (hr)
            goto Cleanup;
        hr = pCPC->FindConnectionPoint(IID_IRowPositionChange, &_pCP);
        if (hr)
            goto Cleanup;
        hr = _pCP->Advise(this, &_dwAdviseCookie);
        if (hr)
        {
            ClearInterface(&_pCP);
            goto Cleanup;
        }
    }
        
Cleanup:
    ReleaseInterface(pCPC);
    return hr;
}


//+-------------------------------------------------------------------------
// Member:      InitChapter (public)
//
// Synopsis:    Hook up to a new chapter
//
// Arguments:   hChapter        chapter to attach
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::InitChapter(HCHAPTER hChapter)
{
    IRowset *pRowset = 0;
    HRESULT hr = E_FAIL;

    _hChapter = hChapter;

    Assert(_pProvider);
    hr = _pProvider->QueryDataInterface(IID_IRowset, (void**)&pRowset);
    if (hr)
        goto Cleanup;
    
    // get a DLCursor
    if (_pDLC)
    {
        _pDLC->Release();
    }
    _pDLC = new CDataLayerCursor(this);
    if (!_pDLC)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    
    // attach DLCursor to the rowset
    hr = _pDLC->Init(pRowset, _hChapter);
    if (hr)
        goto Cleanup;

    // set up the current record instance
    if (_priCurrent)
    {
        _priCurrent->Detach(TRUE);      // clear the elements first
        delete _priCurrent;
    }
    _priCurrent = new CRecordInstance(_pDLC, _hrow);
    if (!_priCurrent)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

Cleanup:
    ReleaseInterface(pRowset);
    return hr;
}


//+-------------------------------------------------------------------------
// Member:      InitPosition (public)
//
// Synopsis:    Initialize the position of my underlying RowPos
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::InitPosition(BOOL fFireRowEnter)
{
    if (!_pRowPos)
        goto Cleanup;

    IGNORE_HR(InitCurrentRow());
    
    // grab the current HROW
    IGNORE_HR(OnRowPositionChange(DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                  DBEVENTPHASE_SYNCHAFTER,
                                  TRUE));

    if (fFireRowEnter)
    {
        _pProvider->FireDelayedRowEnter();
    }
    
Cleanup:
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:      Passivate (private, called by Release())
//
// Synopsis:    return to pre-init state
//
// Arguments:   none
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::Passivate()
{
    Assert("call Detach before Passivate" && !_priCurrent);

    if (_pDLC)
    {
        _pDLC->Release();
        _pDLC = NULL;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:      Detach (public)
//
// Synopsis:    Disassociate from my RowPosition
//
// Arguments:   none
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::Detach()
{
    HRESULT hr = S_OK;

    // stop listening for events
    if (_pCP)
    {
        IGNORE_HR(_pCP->Unadvise(_dwAdviseCookie));
        ClearInterface(&_pCP);
    }

    // let go of current row
    if (_hrow != DB_NULL_HROW)
    {
        _pDLC->ReleaseRows(1, &_hrow);
        _hrow = DB_NULL_HROW;
    }

    // let go of the RowPosition
    ClearInterface(&_pRowPos);

    // let go of my current record
    if (_priCurrent)
    {
        _priCurrent->Detach();
        delete _priCurrent;
        _priCurrent = 0;
    }

    // release the cursor
    if (_pDLC)
    {
        _pDLC->Release();
        _pDLC = NULL;
    }
    
    return hr;
}


//+-------------------------------------------------------------------------
// Member:      Get Current Record Instance (public)
//
// Synopsis:    return pointer to my record instance
//
// Arguments:   ppRecInstance   where to store the pointer
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::GetCurrentRecordInstance(CRecordInstance **ppRecInstance)
{
    Assert("nowhere to store pointer" && ppRecInstance);
    *ppRecInstance = _priCurrent;
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:      Release (public, IUnknown)
//
// Synopsis:    decrease refcount, go away when it hits zero
//
// Arguments:   none
//
// Returns:     new refcount

ULONG
CCurrentRecordInstance::Release()
{
    HRESULT hr;
    ULONG ulRefCount = --_ulRefCount;
    if (ulRefCount == 0) {
        hr = Passivate();
        delete this;
    }
    return ulRefCount;
}


//+-------------------------------------------------------------------------
// Member:      Query Interface (public, IUnknown)
//
// Synopsis:    return desired interface pointer
//
// Arguments:   riid        IID of desired interface
//              ppv         where to store the pointer
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::QueryInterface(REFIID riid, void **ppv)
{
    HRESULT hr;
    IUnknown *punkReturn = 0;

    // check for bad arguments
    if (!ppv)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // look for interfaces I support
    if (    IsEqualIID(riid, IID_IUnknown)
        ||  IsEqualIID(riid, IID_ICurrentRecordInstance)
        ||  IsEqualIID(riid, IID_IRowPositionChange)
        )
    {
        punkReturn = this;          // return addref'd copy
        punkReturn->AddRef();
        *ppv = punkReturn;
        hr = S_OK;
    }
    else
        hr = E_NOINTERFACE;

Cleanup:
    return hr;        
}


//+-------------------------------------------------------------------------
// Member:      On Row Position Change (public, IRowPositionChange)
//
// Synopsis:    sink notification from RowPosition object, adjust
//              my record instance accordingly.
//
// Arguments:   eReason     reason we're being notified
//              ePhase      which notification phase
//              fCantDeny   true if I can't veto the event
//
// Returns:     HRESULT

HRESULT
CCurrentRecordInstance::OnRowPositionChange(DBREASON eReason, DBEVENTPHASE ePhase,
                            BOOL fCantDeny)
{
    HRESULT hr;
    HCHAPTER hchapter;
    DBPOSITIONFLAGS dwPositionFlags;

    hr = _pDLC->CheckCallbackThread();
    if (hr)
        goto Cleanup;
    
    switch (ePhase)
    {
    case DBEVENTPHASE_OKTODO:
        // if we're in act of deleting current record, don't bother
        //  checking if Ok to change HROW.
        if (_dlbDeleted.IsNull())
        {
            hr = _priCurrent->OkToChangeHRow();
        }
        break;
    
    case DBEVENTPHASE_ABOUTTODO:        // position is changing, release HROW
        _pDLC->ReleaseRows(1, &_hrow);
        _hrow = DB_NULL_HROW;
        break;

    case DBEVENTPHASE_FAILEDTODO:
    case DBEVENTPHASE_SYNCHAFTER:       // position changed, get new HROW
        if (_hrow == DB_NULL_HROW)
        {
            _pRowPos->GetRowPosition(&hchapter, &_hrow, &dwPositionFlags);
            
            if (hchapter == _hChapter)
            {
                _priCurrent->SetHRow(_hrow);
            }
            else
            {
                // chapter changes are handled elsewhere.  If the chapter is
                // changing, I'm about to die anyway.
                Assert(eReason == DBREASON_ROWPOSITION_CHAPTERCHANGED);
            }
            
            ReleaseChapterAndRow(hchapter, DB_NULL_HROW, _pRowPos);
        }
        break;
    }

Cleanup:
    return hr;
}


//+-------------------------------------------------------------------------
// Member:      All Changed (public, CDataLayerCursorEvents)
//
// Synopsis:    /*[synopsis]*/
//
// Arguments:   none
//
// Returns:     /*[returns]*/

HRESULT
CCurrentRecordInstance::AllChanged()
{
    return DB_S_UNWANTEDREASON;     //BUGBUG
}


//+-------------------------------------------------------------------------
// Member:      Rows Changed (public, CDataLayerCursorEvents)
//
// Synopsis:    if my HROW has changed, update my Record Instance
//
// Arguments:   cRows       count of changed rows
//              ahRows      HROW for each changed row
//
// Returns:     S_OK

HRESULT
CCurrentRecordInstance::RowsChanged(DBCOUNTITEM cRows, const HROW *ahRows)
{
    for (ULONG k=0; k<cRows; ++k)
    {
        if (_pDLC->IsSameRow(_hrow, ahRows[k]))
        {
            _priCurrent->SetHRow(ahRows[k]);
        }
    }
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:      Fields Changed (public, CDataLayerCursorEvents)
//
// Synopsis:    if my HROW is the changed one, notify my Record Instance
//
// Arguments:   hRows       the changed HROW
//              cColumns    count of changed columns
//              aColumns    index of each changed column
//
// Returns:     S_OK

HRESULT
CCurrentRecordInstance::FieldsChanged(HROW hRow, DBORDINAL cColumns, DBORDINAL aColumns[])
{
    HRESULT hr;
    hr = _priCurrent->OnFieldsChanged(hRow, cColumns, aColumns);
    return hr;
}


//+-------------------------------------------------------------------------
// Member:      Rows Inserted (public, CDataLayerCursorEvents)
//
// Synopsis:    /*[synopsis]*/
//
// Arguments:   cRows       count of inserted rows
//              ahRows      HROW of each inserted row
//
// Returns:     /*[returns]*/

HRESULT
CCurrentRecordInstance::RowsInserted(DBCOUNTITEM cRows, const HROW *ahRows)
{
    return DB_S_UNWANTEDREASON;     //BUGBUG
}


//+-------------------------------------------------------------------------
// Member:      Deleting Rows (public, CDataLayerCursorEvents)
//
// Synopsis:    notification that rows are about to be deleted.  If one of them
//              is my current row, get a bookmark for it.
//
// Arguments:   cRows       count of deleted rows
//              ahRows      HROW of each deleted row
//
// Returns:     S_OK

HRESULT
CCurrentRecordInstance::DeletingRows(DBCOUNTITEM cRows, const HROW *ahRows)
{
    ULONG i;
    
    // if my current record is being deleted, get a bookmark for it
    for (i=0; i<cRows; ++i)
    {
        if (_pDLC->IsSameRow(_hrow, ahRows[i]))
        {
            Assert(_dlbDeleted.IsNull());
            IGNORE_HR(_pDLC->CreateBookmark(ahRows[i], &_dlbDeleted));
            break;
        }
    }
    
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:      Rows Deleted (CDataLayerCursorEvents, public)
//
// Synopsis:    notification that rows have been deleted.  If we have a
//              bookmark, the current row was deleted.  In this case, move
//              the current row forward (or backward if already at the end).
//
// Arguments:   cRows       count of deleted rows
//              ahRows      HROW of each deleted row
//
// Returns:     S_OK

HRESULT
CCurrentRecordInstance::RowsDeleted(DBCOUNTITEM, const HROW *)
{
    DBCOUNTITEM cRows;
    HROW hrow;
    HRESULT hr;
    DBPOSITIONFLAGS dwPositionFlags = DBPOSITION_OK;
    
    if (!_dlbDeleted.IsNull())
    {
        // try moving forward
        hr = _pDLC->GetRowsAt(_dlbDeleted, 0, 1, &cRows, &hrow);

        // if that doesn't work, try moving backward
        if (FAILED(hr) || cRows==0)
            hr = _pDLC->GetRowsAt(_dlbDeleted, -1, -1, &cRows, &hrow);

        // if we didn't get a good row, use the null row
        if (FAILED(hr) || cRows==0)
        {
            hrow = DB_NULL_HROW;
            dwPositionFlags = DBPOSITION_EOF;
        }

        // set current position to the new row
        if (_pRowPos && S_OK == _pRowPos->ClearRowPosition())
            _pRowPos->SetRowPosition(_hChapter, hrow, dwPositionFlags);
        // clear the bookmark
        _dlbDeleted = CDataLayerBookmark::TheNull;

        // release the row
        _pDLC->ReleaseRows(1, &hrow);
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:      DeleteCancelled (public, CDataLayerCursorEvents)
//
// Synopsis:    notification that row deletion was cancelled
//
// Arguments:   cRows       count of deleted rows
//              ahRows      HROW of each deleted row
//
// Returns:     S_OK

HRESULT
CCurrentRecordInstance::DeleteCancelled(DBCOUNTITEM cRows, const HROW *ahRows)
{
    // discard the bookmark I may have taken out during pre-notification
    _dlbDeleted = CDataLayerBookmark::TheNull;
    
    return S_OK;
}

//+-------------------------------------------------------------------------
// Member:      Rows Added (public, CDataLayerCursorEvents)
//
// Synopsis:    /*[synopsis]*/
//
// Arguments:
//
// Returns:     /*[returns]*/

HRESULT
CCurrentRecordInstance::RowsAdded()
{
    return InitCurrentRow();
}


//+-------------------------------------------------------------------------
// Member:      Population Complete (public, CDataLayerCursorEvents)
//
// Synopsis:    /*[synopsis]*/
//
// Arguments:
//
// Returns:     /*[returns]*/

HRESULT
CCurrentRecordInstance::PopulationComplete()
{
    return InitCurrentRow();
}


//+-------------------------------------------------------------------------
// Member:      InitCurrentRow (private helper)
//
// Synopsis:    set the current row, if not set already

HRESULT
CCurrentRecordInstance::InitCurrentRow()
{
    HROW hrow = DB_NULL_HROW;
    HCHAPTER hchapter = DB_NULL_HCHAPTER;
    DBPOSITIONFLAGS dwPositionFlags;

    if (!_pRowPos || !_pDLC)
        goto Cleanup;

    // if current HROW is null, move to the first row
    _pRowPos->GetRowPosition(&hchapter, &hrow, &dwPositionFlags);
    if (hrow == DB_NULL_HROW)
    {
        // Get the first row.
        if (S_OK == _pDLC->GetRowAt(CDataLayerBookmark::TheFirst, &hrow))
        {
            dwPositionFlags = DBPOSITION_OK;
        }
        else
        {
            // if there isn't one, try to set the chapter anyway
            hrow = DB_NULL_HROW;
            dwPositionFlags = DBPOSITION_NOROW;
        }
        if (S_OK == _pRowPos->ClearRowPosition())
            _pRowPos->SetRowPosition(_hChapter, hrow, dwPositionFlags);
    }

    ReleaseChapterAndRow(hchapter, hrow, _pRowPos);

Cleanup:
    return S_OK;
}

