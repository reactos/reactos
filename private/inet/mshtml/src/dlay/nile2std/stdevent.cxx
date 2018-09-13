//--------------------------------------------------------------------
// Microsoft Nile to STD Mapping Layer
// (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  File:       stdevent.cxx
//  Author:     Ido Ben-Shachar (t-idoben)
//
//  Contents:   Event handlers for STD
//

#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

MtDefine(COSPDataCSTDEventsCellChangedHelper_rgColumns, DataBind, "COSPData::CSTDEvents::CellChangesHelper rgColumns")
MtDefine(COSPDataCSTDEventsDeleteRowsHelper_aDeletedHROWs, DataBind, "COSPData::CSTDEvents::DeleteRowsHelper aDeleteHROWs")
MtDefine(COSPDataCSTDEventsInsertRowsHelper_aInsertedHROWs, DataBind, "COSPData::CSTDEvents::InsertRowsHelper aInsertedHROWs")

DeclareTag(tagOSPRowDelta, "Databinding", "trace changes to OSP row count");

////////////////////////////////////////////////////////////////////////////////
//
// ISimpleTabularDataEvents specific interfaces
//
////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
COSPData::CSTDEvents::QueryInterface (REFIID riid, LPVOID *ppv)
{
    Assert(ppv);
    
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_OLEDBSimpleProviderListener))
    {
        *ppv = (OLEDBSimpleProviderListener *) this;
    }
    else
    {
        *ppv = NULL;
    }
    
    //  If we're going to return an interface, AddRef it first
    if (*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }
    
    return E_NOINTERFACE;
}



//+---------------------------------------------------------------------------
//  Member:     aboutToChangeCell
//
//  Synopsis:   for AboutTo phase of cellChanged
//
//  Arguments:  iRow                    row of changed cell
//              iColumn                 column of changed cell
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::aboutToChangeCell(DBROWCOUNT iRow, DB_LORDINAL iColumn)
{
    return CellChangedHelper(iRow, iColumn, TRUE);
}

//+---------------------------------------------------------------------------
//  Member:     CellChanged
//
//  Synopsis:   A cell somewhere in the rowset has been altered.  Therefore,
//              this function cleans up the rowset to reflect this change.
//              Notifications are also fired.  If multiple rows have changed
//              (iRow == -1) then FireRowsetEvent is called.  If multiple
//              columns have changed (iColumn == -1) then FireRowEvent is
//              called.  Otherwise, only one field has changed, and
//              FireFieldEvent is called.
//
//  Arguments:  iRow                    row of changed cell
//              iColumn                 column of changed cell
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::cellChanged(DBROWCOUNT iRow, DB_LORDINAL iColumn)
{
    return CellChangedHelper(iRow, iColumn, FALSE);
}

//+---------------------------------------------------------------------------
//  Member:     CellChangedHelper
//
//  Synopsis:   Does the actual work for CellChanged. fBefore tells us which
//              notification phase we're in.
//
//  Arguments:  iRow                    row of changed cell
//              iColumn                 column of changed cell
//              fBefore                 which phase are we in
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::CellChangedHelper(DBROWCOUNT iRow, DB_LORDINAL iColumn, BOOL fBefore)
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::CellChanged(%p {%u %u})",
          this, iRow, iColumn) );
    HRESULT hr = S_OK;

    COSPData *pOSPData;
    pOSPData = CONTAINING_RECORD(this, COSPData, _STDEvents);
    CImpIRowset *pRowset = pOSPData->_pRowset;
    HCHAPTER hChapter;
    DBORDINAL cColCount, iSingleCol;
    DBORDINAL *rgColumns = NULL;
    HROW hrowTemp = NULL;

    if (!pRowset)                       // just in case
        goto Cleanup;

    hChapter = pRowset->HChapterFromOSPData(pOSPData);

    // -1,-1 is a special notification meaning the STD was scrambled out
    // from under us.  Everything we know is invalid.  This should probably
    // be handled better, but for now..
    
    if (iRow == -1) // multiple rows changed
    {
        // BUGBUG: Is this a valid and appropriate reason for OnRowEvent?
        // also, we fire on two phases instead of 4..
        HRESULT hr1;
        hr1 = pRowset->FireRowEvent(0, &hrowTemp,
                               DBREASON_COLUMN_SET, 
                               fBefore ? DBEVENTPHASE_ABOUTTODO :
                               DBEVENTPHASE_DIDEVENT);
        if (fBefore && hr1)
        {
            pRowset->FireRowEvent(0, &hrowTemp,
                               DBREASON_COLUMN_SET, 
                               DBEVENTPHASE_FAILEDTODO);
            hr = hr1;
        }
    }
    else        // only one Row changed, but maybe multiple columns
    {

        hr = THR(pRowset->Index2HROW(hChapter, iRow, hrowTemp));
        if (hr)
        {
            goto Cleanup;
        }

        if (0<iColumn && iColumn<=(LONG)pOSPData->_cSTDCols)
        {
            // a single column is changing
            cColCount = 1;
            iSingleCol = iColumn;
            rgColumns = &iSingleCol;
        }
        
        else if (iColumn == -1)
        {
            // all columns in this row changed
            cColCount = pOSPData->_cSTDCols;
            
            rgColumns = new(Mt(COSPDataCSTDEventsCellChangedHelper_rgColumns)) DBORDINAL[cColCount];
            if (rgColumns == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Cleanup;
            }
            
            for (ULONG i=0; i<cColCount; ++i)
            {
                rgColumns[i] = i+1;
            }
        }

        else
        {
            // OSP gave us a bad column number
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        // BUGBUG: Currently, we force all 4 event phases to occur, even if the
        //   receiver of the notifications fails on one of the phases.  This is
        //   related to the problem of the changes to the field being visible
        //   before any of the phases actually occur.
        // BUGBUG: There is no notion of the "first time" that a field changed,
        //   thus DBREASON_ROW_FIRSTCHANGE can't be fired.
        if (fBefore)
        {
            hr = pRowset->FireFieldEvent(hrowTemp, cColCount, rgColumns,
                                       DBREASON_COLUMN_SET, DBEVENTPHASE_OKTODO);
            if (!hr)
                hr = pRowset->FireFieldEvent(hrowTemp, cColCount, rgColumns,
                                       DBREASON_COLUMN_SET, DBEVENTPHASE_ABOUTTODO);
            if (hr)
                pRowset->FireFieldEvent(hrowTemp, cColCount, rgColumns,
                                       DBREASON_COLUMN_SET, DBEVENTPHASE_FAILEDTODO);
        }
        else
        {
            pRowset->FireFieldEvent(hrowTemp, cColCount, rgColumns,
                                       DBREASON_COLUMN_SET, DBEVENTPHASE_SYNCHAFTER);
            pRowset->FireFieldEvent(hrowTemp, cColCount, rgColumns,
                                       DBREASON_COLUMN_SET, DBEVENTPHASE_DIDEVENT);
        }
    }

Cleanup:
    if (iColumn == -1)
    {
        delete [] rgColumns;
    }
    return hr;
}



//+---------------------------------------------------------------------------
//  Member:     aboutToDeleteRows
//
//  Synopsis:   About To phase of DeletedRows notification
//              
//  Arguments:  iRow                    first row deleted
//              cRows                   number of rows deleted
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::aboutToDeleteRows(DBROWCOUNT iRow, DBROWCOUNT cRows)
{
    return DeleteRowsHelper(iRow, cRows, TRUE);
}

//+---------------------------------------------------------------------------
//  Member:     DeletedRows
//
//  Synopsis:   Rows in the STD have been deleted.  Therefore,
//              this function cleans up the rowset to reflect this change.
//
//  Arguments:  iRow                    first row deleted
//              cRows                   number of rows deleted
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::deletedRows(DBROWCOUNT iRow, DBROWCOUNT cRows)
{
    return DeleteRowsHelper(iRow, cRows, FALSE);
}

//+---------------------------------------------------------------------------
//  Member:     DeletedRowsHelper
//
//  Synopsis:   Rows in the STD have been deleted.  Therefore,
//              this function cleans up the rowset to reflect this change.
//
//  Arguments:  iRow                    first row deleted
//              cRows                   number of rows deleted
//              fBefore                 which phase we're in
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::DeleteRowsHelper(DBROWCOUNT iRow, DBROWCOUNT cRows, BOOL fBefore)
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::DeletedRows(%p {%u %u})",
          this, iRow, cRows) );

    HRESULT hr = S_OK;
    HROW *aDeletedHROWs = NULL;
    DBCOUNTITEM cDeletedHROWs = 0;
    COSPData *pOSPData;
    pOSPData = CONTAINING_RECORD(this, COSPData, _STDEvents);
    CImpIRowset *pRowset = pOSPData->_pRowset;
    HCHAPTER hChapter;

    if (!pRowset)                       // just in case
        goto Cleanup;

    // ignore notifications about 0 rows
    if (cRows <= 0)
        goto Cleanup;

    hChapter = pRowset->HChapterFromOSPData(pOSPData);
    
    aDeletedHROWs = new(Mt(COSPDataCSTDEventsDeleteRowsHelper_aDeletedHROWs)) HROW[cRows];
    if (!aDeletedHROWs)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = THR(pRowset->GenerateHRowsFromHRowNumber(hChapter,
                                                  iRow, 0, cRows, &cDeletedHROWs,
                                                  &aDeletedHROWs));
    Assert(hr || ((LONG)cDeletedHROWs == cRows && "Got wrong number of rows"));

    if (hr || cDeletedHROWs == 0)
        goto Cleanup;
    
    if (fBefore)
    {
        hr = pRowset->FireRowEvent(cDeletedHROWs, aDeletedHROWs,
                              DBREASON_ROW_DELETE, DBEVENTPHASE_OKTODO);
        if (!hr)
            hr = pRowset->FireRowEvent(cDeletedHROWs, aDeletedHROWs,
                              DBREASON_ROW_DELETE, DBEVENTPHASE_ABOUTTODO);
        if (hr)
            pRowset->FireRowEvent(cDeletedHROWs, aDeletedHROWs,
                              DBREASON_ROW_DELETE, DBEVENTPHASE_FAILEDTODO);
    }
    else
    {
        // Fix up the rowmap that is used to implement bookmark stability
        pRowset->DeleteRows(iRow, (int) cDeletedHROWs, pOSPData);
        pRowset->_cRows -= (LONG)cDeletedHROWs;     // reduce rows in global rowset
        pOSPData->_cSTDRows -= (LONG)cDeletedHROWs; // reduce rows in this OSP
        
        TraceTag((tagOSPRowDelta, "Deleted %ld rows.  Rowset %p has %ld rows, chapter %p has %ld rows",
                    cDeletedHROWs, pRowset, pRowset->_cRows, pOSPData, pOSPData->_cSTDRows));

        pRowset->FireRowEvent(cDeletedHROWs, aDeletedHROWs,
                              DBREASON_ROW_DELETE, DBEVENTPHASE_SYNCHAFTER);
        pRowset->FireRowEvent(cDeletedHROWs, aDeletedHROWs,
                              DBREASON_ROW_DELETE, DBEVENTPHASE_DIDEVENT);
    }

Cleanup:
    delete [] aDeletedHROWs;
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     InsertedRowsHelper
//
//  Synopsis:   Rows in the STD have been inserted.  Therefore,
//              this function cleans up the rowset to reflect this change.
//
//  Arguments:  iRow                    first row inserted
//              cRows                   number of rows inserted
//              eReason                 so we can handle both insertedRows
//                                      and rowsAvailable events
//              fBefore                 which phases to signal
//
//  Returns:    Returns success.
//
STDMETHODIMP
COSPData::CSTDEvents::InsertRowsHelper(DBROWCOUNT iRow, DBROWCOUNT cRows,
                           INSERT_REASONS reason, BOOL fBefore)
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::InsertedRows(%p {%u %u})",
          this, iRow, cRows) );

    HRESULT hr = S_OK;
    DBCOUNTITEM cInsertedHROWs = 0;
    HROW *aInsertedHROWs = NULL;
    COSPData *pOSPData;
    pOSPData = CONTAINING_RECORD(this, COSPData, _STDEvents);
    CImpIRowset *pRowset = pOSPData->_pRowset;
    HCHAPTER hChapter;

    // True iff insertion at end of rowset
    BOOL fAtEnd = (iRow >= (LONG)pOSPData->_cSTDRows);

    if (!pRowset)                       // just in case
        goto Cleanup;

    // ignore notifications about 0 rows
    if (cRows <= 0)
        goto Cleanup;

    hChapter = pRowset->HChapterFromOSPData(pOSPData);
    
    // If this is After phase, it's time to actually do the work!
    if (!fBefore)
    {
        // RowsAvailable events may fire well beyond our current idea
        // of the row array extent.
        if (iRow > (LONG) pOSPData->_cSTDRows+1)
        {
            // Fill in rowmap array to catch up
            LONG cRowsGap = iRow - 1 - pOSPData->_cSTDRows;
            pRowset->InsertRows(pOSPData->_cSTDRows, cRowsGap, pOSPData);
            pRowset->_cRows += cRowsGap;
            pOSPData->_cSTDRows += cRowsGap;
            TraceTag((tagOSPRowDelta, "Inserted gap of %ld rows.  Rowset %p has %ld rows, chapter %p has %ld rows",
                        cRowsGap, pRowset, pRowset->_cRows, pOSPData, pOSPData->_cSTDRows));
        }
        pRowset->_cRows += (ULONG)cRows;       // increase rows in global rowset
        pOSPData->_cSTDRows += (ULONG)cRows;   // increase rows in this OSP
        pRowset->InsertRows(iRow, (int) cRows, pOSPData);
        TraceTag((tagOSPRowDelta, "Inserted %ld rows.  Rowset %p has %ld rows, chapter %p has %ld rows",
                    cRows, pRowset, pRowset->_cRows, pOSPData, pOSPData->_cSTDRows));
    }

    aInsertedHROWs = new(Mt(COSPDataCSTDEventsInsertRowsHelper_aInsertedHROWs)) HROW[cRows];
    if (!aInsertedHROWs)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = THR(pRowset->GenerateHRowsFromHRowNumber(hChapter,
                                                  iRow, 0, cRows,
                                                  &cInsertedHROWs,
                                                  &aInsertedHROWs));
    if (cInsertedHROWs == 0)
        goto Cleanup;

    // Distinguish between ROWS_ADDED at end, and Asynch Rows added in
    // in the middle.
    if (ROWS_ADDED == reason)
        reason = (fAtEnd ? ROWS_ADDED : ROWS_ASYNCHINSERTED);

    // There are four different paths through this code:

    //  1. reason == ROWS_ADDED  (fBefore irrelevant)
    //     We got a rowsAvailable notification from the OSP for rows at the
    //     end of the Rowset . --> send a POPULATION notification on the
    //     IDBAsynchNotify interface.  This notification has no phases.
    //
    //  2. reason == ROWS_ASYNCHINSERTED (fBefore irrelevant)
    //     We got a rowsAvailable notification from the OSP for rows in
    //     the middle of the rowset.  --> send a ROW_ASYNCHINSERT event
    //     on the IRowsetNotify interface, for just the DIDEVENT phase.
    //     Also send a POPULATION notification on IDBAsyncNotify, to allow
    //     listeners to keep their progress bars up to date.

    //  3. reason == ROWS_INSERTED, fBefore == TRUE
    //     We got an aboutToInsertRows notification from the OSP.  
    //     --> send ROW_INSERT events for OKTODO & ABOUTODO phases.

    // 4.  reason == ROWS_INSERTED, fBefore == FALSE
    //     We got an InsertedRows notification from the OSP.
    //     --> send ROW_INSERT events for SYNCHAFTER and DIDEVENT phases.

    if (ROWS_ADDED == reason || ROWS_ASYNCHINSERTED == reason)
    {
        DBROWCOUNT ulProgressMax;

        IGNORE_HR(pOSPData->_pSTD->getEstimatedRows(&ulProgressMax));

        pRowset->FireAsynchOnProgress(pOSPData->_cSTDRows,
                                      ulProgressMax,
                                      DBASYNCHPHASE_POPULATION);
    }
    
    if (ROWS_INSERTED==reason || ROWS_ASYNCHINSERTED==reason)
    {
        if (ROWS_INSERTED==reason)
        {
            if (fBefore)
            {
                hr = pRowset->FireRowEvent(cRows, aInsertedHROWs,
                                      DBREASON_ROW_INSERT, DBEVENTPHASE_OKTODO);
                if (!hr)
                    hr = pRowset->FireRowEvent(cRows, aInsertedHROWs,
                                      DBREASON_ROW_INSERT, DBEVENTPHASE_ABOUTTODO);
                if (hr)
                    pRowset->FireRowEvent(cRows, aInsertedHROWs,
                                      DBREASON_ROW_INSERT, DBEVENTPHASE_FAILEDTODO);
            }
            else
            {
                pRowset->FireRowEvent(cRows, aInsertedHROWs,
                                      DBREASON_ROW_INSERT, DBEVENTPHASE_SYNCHAFTER);
            }
        }

        // By spec, ROW_ASYNCINSERT fires only DIDEVENT
        if (!fBefore)
            pRowset->FireRowEvent(cRows, aInsertedHROWs,
                                  reason==ROWS_INSERTED ? DBREASON_ROW_INSERT :
                                  (DBREASONENUM)DBREASON_ROW_ASYNCHINSERT,
                                  DBEVENTPHASE_DIDEVENT);
    }

Cleanup:
    delete [] aInsertedHROWs;
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     aboutToInsertRows
//
//  Synopsis:   About To phase of InsertedRows
//              
//
//  Arguments:  iRow                    first row inserted
//              cRows                   number of rows inserted
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::aboutToInsertRows(DBROWCOUNT iRow, DBROWCOUNT cRows)
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::InsertedRows(%p {%u %u})",
          this, iRow, cRows) );

    return InsertRowsHelper(iRow, cRows, ROWS_INSERTED, TRUE);
}

//+---------------------------------------------------------------------------
//  Member:     insertedRows
//
//  Synopsis:   Rows in the STD have been inserted.  Therefore,
//              this function cleans up the rowset to reflect this change.
//
//  Arguments:  iRow                    first row inserted
//              cRows                   number of rows inserted
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::insertedRows(DBROWCOUNT iRow, DBROWCOUNT cRows)
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::InsertedRows(%p {%u %u})",
          this, iRow, cRows) );

    return InsertRowsHelper(iRow, cRows, ROWS_INSERTED, FALSE);
}

//+---------------------------------------------------------------------------
//  Member:     AsyncRowsArrived
//
//  Synopsis:   Rows in the STD have been inserted.  Therefore,
//              this function cleans up the rowset to reflect this change.
//
//  Arguments:  iRow                    first row inserted
//              cRows                   number of rows inserted
//
//  Returns:    Returns success.
//

STDMETHODIMP
COSPData::CSTDEvents::rowsAvailable(DBROWCOUNT iRow, DBROWCOUNT cRows)
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::AsyncRowsArrived(%p {%u %u})",
          this, iRow, cRows) );

    return InsertRowsHelper(iRow, cRows, ROWS_ADDED, FALSE);
}


STDMETHODIMP
COSPData::CSTDEvents::transferComplete(OSPXFER stdxfer)
{
    HRESULT hrStatus;

    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::PopulationComplete(%p)", this) );

    COSPData *pOSPData;
    pOSPData = CONTAINING_RECORD(this, COSPData, _STDEvents);
    CImpIRowset *pRowset = pOSPData->_pRowset;
    HCHAPTER hChapter;

    if (!pRowset)                       // just in case
        goto Cleanup;

    hChapter = pRowset->HChapterFromOSPData(pOSPData);
    pOSPData->_fPopulationComplete = TRUE;

    // Good time to update our impression of the final STD size..
    DBROWCOUNT cRowsTemp;
    IGNORE_HR(pOSPData->_pSTD->getRowCount(&cRowsTemp));
    pOSPData->_cSTDRows = (ULONG)cRowsTemp;

    // Fire rowset event
    IGNORE_HR(pRowset->FireAsynchOnProgress(pOSPData->_cSTDRows,
                                            pOSPData->_cSTDRows,
                                            DBASYNCHPHASE_COMPLETE));

    switch (stdxfer)
    {
        case OSPXFER_ABORT:
            hrStatus = DB_E_CANCELED;
            break;
        case OSPXFER_ERROR:
            hrStatus = E_FAIL;
            break;
        case OSPXFER_COMPLETE:
        default:                        // there shouldn't be a default, but
            hrStatus = S_OK;            // having all cases makes compiler happy.
            break;
    }

    IGNORE_HR(pRowset->FireAsynchOnStop(hrStatus));

Cleanup:
    return S_OK;
}
