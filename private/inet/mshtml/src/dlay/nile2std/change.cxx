//--------------------------------------------------------------------
// Microsoft Nile to STD Mapping Layer
// (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  Author:     Terry L. Lucas (TerryLu)
//
//  Contents:   CImpIRowsetChange object implementation
//


#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

////////////////////////////////////////////////////////////////////////////////
//
// IRowsetChange specific interfaces
//
////////////////////////////////////////////////////////////////////////////////




//+-----------------------------------------------------------------------
//
//  Member:    DeleteRows (public member)
//
//  Synopsis:  Given an array of of HROWs, this will remove the rows from
//             the table.  Note that the rows, in the case of the STD, are
//             immediately removed.  If there are any errors, error
//             information will be returned to the user, provided the user
//              has allocated an array for the information.
//
//  Arguments: hReserved        future use.  Ignored
//             cRows            number of rows to delete
//             ahRows           an array of HROWs to remove
//              aRowStatus      array for errors, one per HROW (or NULL)
//
//  Returns:   Success           if inputs are valid.
//             E_INVALIDARG      if cRows>0 but ahRows is NULL
//             E_FAIL            if STD reports an error
//             DB_E_NOTREENTRANT illegal reentrancy
//             DB_S_ERRORSOCCURRED  something failed, but at least one row
//                                  was deleted
//

STDMETHODIMP
CImpIRowset::DeleteRows (HCHAPTER hChapter, DBCOUNTITEM cRows, const HROW ahRows[],
                         DBROWSTATUS aRowStatus[])
{
    TraceTag((tagNileRowsetProvider,
              "CImpIRowset::DeleteRows(%p {%p, %u, %p, %p})",
              this, hChapter, cRows, ahRows, aRowStatus));

    HRESULT hr = S_OK;
    BOOL    fRowsDeleted = FALSE;           // were rows actually deleted
    BOOL    fRowsNotDeleted = FALSE;        // did any rows fail to delete
    const HROW *phRow = ahRows;                 // index for stepping HROWs

    if (cRows && !ahRows)                   // no input array
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (FAnyPhaseInProgress(DBREASON_ROW_DELETE)
            || FAnyPhaseInProgress(DBREASON_ROW_INSERT))
    {
        hr = DB_E_NOTREENTRANT;
        goto Cleanup;
    }

    if (GetpOSPData(hChapter) == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Cleanup;
    }
    
    // mark all the rows OK.  later events may change this
    LogErrors(aRowStatus, 0, cRows, DBROWSTATUS_S_OK);

    // we attempt to coalesce runs of consecutive ascending or descending rows,
    //  because our overhead in handling notifications will be much more
    //  manageable.
    while(cRows)
    {
        const HROW *phRowStartRun = phRow;
        DBCOUNTITEM ulRow;
        HRESULT hrT;
        HCHAPTER hChapter;              // chapter for this hRow

        hr = HROW2Index(*phRow, ulRow);
        if (hr)
        {
            // Advance loop variable, and log one error
            DBROWSTATUS dbs = hr==DB_E_DELETEDROW   ? DBROWSTATUS_E_DELETED
                                                    : DBROWSTATUS_E_INVALID;
            (++phRow, --cRows);

            LogErrors(aRowStatus, phRowStartRun-ahRows, 1, dbs);
            fRowsNotDeleted = TRUE;
            hr = S_OK;
        }
        else
        {
            DBCOUNTITEM ulRow2;
            DBCOUNTITEM cRowsRun;
            INT iDirection = 1; // algorithm works even if uninitialized, but
                                //  this keeps the compiler happy/
            DBROWCOUNT cRowsDeleted;

            hChapter = ChapterFromHRow((ChRow) *phRow);

            // Advance loop variables, continue advance if run length > 1
            phRow++;
            --cRows;
            if (cRows && !HROW2Index(*phRow, ulRow2))
            {
                iDirection = ulRow2 - ulRow;

                if (iDirection == 1 || iDirection == -1)
                {
                    // we've got a run!  Advance loop variables to past end.
                    for (;;)
                    {
                        DBCOUNTITEM ulRowTemp;

                        ulRow2 += iDirection;
                        if ((phRow++, --cRows) == 0)
                            break;
                        // Break this run if we ran off the end, changed chapter
                        // (not technically legal), or are simply not contiguous.
                        if (HROW2Index(*phRow, ulRowTemp) ||
                            ChapterFromHRow((ChRow) *phRow) != hChapter ||
                            ulRowTemp != ulRow2)
                        {
                            break;
                        }
                    }
                }
            }

            cRowsRun = (phRow - phRowStartRun);

            // Note that if cRowsRun == 1, then any value of iDirection
            //  produces identical results.  So it's OK that we may hit here
            //  with a garbage iDirection in certain circumstances.

            Assert(iDirection == -1 || iDirection == 1 || cRowsRun == 1);
            if (iDirection < 0)
                ulRow -= cRowsRun - 1;

            hrT = GetpOSP(hChapter)->deleteRows((LONG)ulRow, (LONG)cRowsRun, &cRowsDeleted);
            if (cRowsDeleted > 0)
                fRowsDeleted = TRUE;

            // Note that Indexes insides HROWs (CImpRows) should just
            //  have been fixed up!

            if (hrT)
            {
                fRowsNotDeleted = TRUE;
                Assert(cRowsDeleted != (DBROWCOUNT) cRowsRun);
                if (iDirection > 0)
                {
                    phRowStartRun += cRowsDeleted;
                }
                LogErrors(aRowStatus, phRowStartRun - ahRows,
                                cRowsRun - cRowsDeleted, DBROWSTATUS_E_CANCELED);
            }
            else
            {
                Assert(cRowsDeleted == (DBROWCOUNT) cRowsRun);
            }
        }
    }
Cleanup:
    if (hr == S_OK)     // no global errors - see whether row deletions worked
        hr = fRowsNotDeleted    ? fRowsDeleted  ? DB_S_ERRORSOCCURRED
                                                : DB_E_ERRORSOCCURRED
                                : S_OK;
    return hr;
}


//+-----------------------------------------------------------------------
//
//  Member:    InsertRow (public member)
//
//  Synopsis:  Create and initialize a new row in the STD.
//
//  Arguments: hChapter    chapter handle [IN]
//             hAccessor   Accessor to use [IN]
//             pData       Pointer to buffer of data to set [IN]
//             phRow       Row Handle [OUT]
//
//  Returns:   S_OK                    if data changed successfully
//             E_FAIL                  if Catch all (NULL pData, etc.)
//             E_INVALIDARG            if pcErrors!=NULL and paErrors==NULL
//             E_OUTOFMEMORY           if output error array couldn't be allocated
//             DB_E_BADACCESSORHANDLE  if invalid accessor
//             DB_E_BADCHAPTER         if chapter passed in
//

STDMETHODIMP
CImpIRowset::InsertRow(HCHAPTER   hChapter,
                        HACCESSOR  hAccessor, void *pData,
                        HROW      *phRow )
{
    TraceTag((tagNileRowsetProvider,
              "CImpIRowset::InsertRow(%p {%p %p})",
              this, hAccessor, pData ));
    return SetNewDataAfter(hChapter, 1, &g_bBmkLast,
                           hAccessor, (BYTE*)pData, phRow );
}


//+-----------------------------------------------------------------------
//
//  Member:     SetData (public member)
//
//  Synopsis:   Sets new data values into fields of a row.
//
//  Arguments:  hRow        Row Handle [IN]
//              hAccessor   Accessor to use [IN]
//              pData       Pointer to buffer of data to set [IN/OUT]
//
//  Returns:    S_OK                    Data changed successfully
//              DB_S_ERRORSOCCURRED     Parital success
//              DB_E_ERRORSOCCURRED     All columns failed
//              DB_E_BADACCESSORHANDLE  Invalid accessor
//              DB_E_BADROWHANDLE       Bad HROW (0)
//              E_FAIL                  Catch all (including out-of-mem)
//              DB_E_NOTREENTRANT       Illegal reentrancy
//
//  Sets status field (in *pData) for each column that has one

STDMETHODIMP
CImpIRowset::SetData (HROW hRow, HACCESSOR hAccessor, void* pData)
{
    TraceTag((tagNileRowsetProvider,
             "IRowsetChange::SetData(%p {%p, %p, %p})",
             this, hRow, hAccessor, pData ));

    HRESULT         hr;
    HRESULT         hrLastFailure;
    AccessorFormat  *pAccessor;
    ULONG           cBindings;
    DBBINDING       *pBinding;
    ULONG           ibind;
    ULONG           ulErrorCount = 0;
    DBCOUNTITEM     uStdRow;
    HCHAPTER        hChapter;

    // check that accessor is valid
    pAccessor = (AccessorFormat *)hAccessor;
    if (pAccessor == NULL)
    {
        hr = DB_E_BADACCESSORHANDLE;
        goto Cleanup;
    }
    if (!(pAccessor->dwAccFlags & DBACCESSOR_ROWDATA))
    {
        hr = DB_E_BADACCESSORTYPE;
        goto Cleanup;
    }

    cBindings = pAccessor->cBindings;
    pBinding  = pAccessor->aBindings;

    // check that hrow is valid
    hr = HROW2Index(hRow, uStdRow);
    if (hr)
    {
        goto Cleanup;
    }

    hChapter = ChapterFromHRow((ChRow) hRow);

    // Ensure a place to put data, unless the accessor is the null accessor then
    // a NULL pData is okay.
    if (pData == NULL && cBindings > 0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // Apply accessor to data.
    for (ibind = 0; ibind < cBindings; ++ibind)
    {
        XferInfo        xInfo;
        DBBINDING &     currBinding = pBinding[ibind];
        DBPART          dwPart = currBinding.dwPart;
        ULONG           iord = currBinding.iOrdinal;
        CSTDColumnInfo &stdColInfo = _astdcolinfo[ColToDBColIndex(iord)];
        ULONG           uDummyLength;
        DWORD           wDummyStatus;

        xInfo.dwDBType = stdColInfo.dwType;
        xInfo.dwAccType = currBinding.wType;

        xInfo.pData = dwPart & DBPART_VALUE ?
            ((BYTE *)pData + currBinding.obValue) : NULL;
        xInfo.pulXferLength = dwPart & DBPART_LENGTH ?
            (ULONG *)((BYTE *)pData + currBinding.obLength) : &uDummyLength;
        xInfo.pdwStatus = dwPart & DBPART_STATUS ?
            (DWORD *)((BYTE *)pData + currBinding.obStatus) : &wDummyStatus;
        xInfo.ulDataMaxLength = stdColInfo.cbMaxLength;

        // check that the accessor is allowed to write to this column
        if (!(stdColInfo.dwFlags & DBCOLUMNFLAGS_WRITE) )
        {
            *xInfo.pdwStatus = DBROWSTATUS_E_PERMISSIONDENIED;
            ++ ulErrorCount;
            continue;
        }

        hr = DataCoerce(DataToProvider, hChapter, uStdRow, iord, xInfo);

        if (hr)
        {
            Assert(FAILED(hr));
            hrLastFailure = hr;
            ++ ulErrorCount;
        }
    }

    hr = ulErrorCount == 0 ?            S_OK :
         ulErrorCount == cBindings ?    DB_E_ERRORSOCCURRED :
                                        DB_S_ERRORSOCCURRED;

Cleanup:
    Assert(hr != E_OUTOFMEMORY);    // would conflict with Nile spec
    return hr;
}



//+-----------------------------------------------------------------------
//
//  Member:    LogErrors (private member)
//
//  Synopsis:  Helper function to fill in an array of DBROWSTATUS objects
//
//  Arguments: aRowStatus       array of DBROWSTATUSs, one per HROW.  If this
//                              is 0, this routine does nothing.
//             iFirstRow        index of first row to log error
//             cErrorRows       number of rows to log errors for
//             dbrsStatus       What status code to return for each HROW
//
//  Returns:   nothing
//

void
CImpIRowset::LogErrors(DBROWSTATUS aRowStatus[], DBCOUNTITEM iFirstRow, DBCOUNTITEM cErrorRows,
                       DBROWSTATUS dbrs)
{
    if (aRowStatus) {
        for ( ; cErrorRows>0; --cErrorRows, ++iFirstRow)
        {
            aRowStatus[iFirstRow] = dbrs;
        }
    }
}

