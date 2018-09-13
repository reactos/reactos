//+-----------------------------------------------------------------------
//  Microsoft OLE/DB to STD Mapping Layer
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      locate.cxx
//  Author:    Ted Smith
//
//  Contents:  Implementation of RowsetLocate
//
//------------------------------------------------------------------------

#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

////////////////////////////////////////////////////////////////////////////////
//
// IRowsetLocate specific interfaces
//
////////////////////////////////////////////////////////////////////////////////

//+-----------------------------------------------------------------------
//
//  Member:    Bookmark2HRowNumber (private member)
//
//  Synopsis:  Figure out what a row number from a bookmark, given
//             arguments passed to various Nile methods.
//
//  Arguments: cbBookmark       number of bytes in the bookmark
//             pBookmark        pointer to bookmark
//             rulRow           where to return row number
//
//  Returns:   S_OK             all OK.
//             DB_S_BOOKMARKSKIPPED if initial bookmark referred to
//                              to a deleted row
//             DB_E_BADBOOKMARK bad bookmark: malformed, row 0,
//                              or row > 0x7fffffff
//

HRESULT
CImpIRowset::Bookmark2HRowNumber(HCHAPTER hChapter, DBBKMARK cbBookmark,
                                 const BYTE *pBookmark, DBCOUNTITEM &rulRow)
{
    HRESULT hr = S_OK;
    DBCOUNTITEM ulRow;
    
    if (!pBookmark)
    {
        goto BadBookmark;
    }

    if (cbBookmark == 1)
    {
        if (*pBookmark == DBBMK_FIRST)
        {
            ulRow = 1;
        }
        else if (*pBookmark == DBBMK_LAST)
        {
            COSPData *pOSPData = GetpOSPData(hChapter);
            if (pOSPData)
            {
                ulRow = pOSPData->_cSTDRows;
            }
            else
            {
                hr = DB_E_BADCHAPTER;
                goto Cleanup;
            }
        }
        else
        {
            goto BadBookmark;
        }
    }
    else if (cbBookmark == sizeof(ULONG))
    {
        ulRow = IndexFromHRow((ChRow)(* (HROW *) pBookmark));
        if (FhRowDeleted((ChRow)(* (HROW *) pBookmark)))
        {
            hr = DB_S_BOOKMARKSKIPPED;  // Note this is not a fatal error!
        }
        if ((LONG) ulRow < 1)
        {
            goto BadBookmark;
        }
    }
    else
    {
        goto BadBookmark;
    }

    rulRow = ulRow;
    
Cleanup:
    return hr;

BadBookmark:
    hr = DB_E_BADBOOKMARK;
    goto Cleanup;
}


//+-----------------------------------------------------------------------
//
//  Member:    GenerateHRowsFromHRowNumber (private member)
//
//  Synopsis:  Returns a set of contiguous rows within the rowset, starting
//             at the specified HRow number (which may not correspond to
//             index if a deletion is in progress).  The user can allocate
//             memory for the returned row handles by setting pahRows
//             to a block of memory.  If it is NULL, this function will
//             allocate that memory using CoTaskMemAlloc.
//
//  Arguments: ulFirstRow       the row number of the first HROW to return
//             lRowsOffset      number of rows to skip over
//             cRows            rows to fetch and direction to fetch in
//             pcRowsObtained   rows obtained
//             pahRows          handles of rows obtained
//
//  Returns:   S_OK             if all rows could be fetched.
//             DB_S_BOOKMARKSKIPPED if initial HRowNumber referred to
//                              to a deleted row
//             DB_S_ENDOFROWSET if we straddled the end of the rowset,
//                              but didn't set DB_S_BOOKMARKSKIPPED
//             E_INVALIDARG     if output pointers are null.
//             DB_E_BADBOOKMARK if bookmark was invalid.
//             DB_E_NOTREENTRANT on re-entrancy problem
//             
//

HRESULT
CImpIRowset::GenerateHRowsFromHRowNumber(HCHAPTER hChapter, DBCOUNTITEM ulFirstRow,
                                         DBROWOFFSET lRowsOffset, DBROWCOUNT cRows, 
                                         DBCOUNTITEM *pcRowsObtained, HROW **pahRows )
{

    TraceTag((tagNileRowsetProvider,
              "CImpIRowset::GenerateHRowsFromHRowNumber(%p {%u %d %d %p %p})",
              this, ulFirstRow, lRowsOffset, cRows, pcRowsObtained, pahRows ));

    const ULONG ucRows     = (cRows < 0 ? -cRows : cRows); // true count
    const int   sIncrement = (cRows < 0 ? -1     : 1);     // increment
    HRESULT     hr = S_OK;
    ULONG       cGetRows;               // number of rows to fetch
    BOOL        fEndOfRowset = FALSE;   // did we hit end of rowset?
    BOOL        fRowsAllocated = FALSE; // did we allocate space for the HROWs
    COSPData    *pOSPData = GetpOSPData(hChapter);

    Assert(pcRowsObtained);
    Assert(pahRows);

    // In case of errors:
    *pcRowsObtained = 0;

    if (pOSPData == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Cleanup;
    }
    
    if ((LONG) ulFirstRow < 0)
    {
        hr = DB_E_BADBOOKMARK;
        goto Cleanup;
    }
    

    ulFirstRow += lRowsOffset;

    // We know that index was non-negative before the above operation.
    // So we can only overflow with large positive index, lRowsOffset...
    // Check for numeric overflow, arrange for appropriate error below
    if ((LONG) ulFirstRow < 0 && lRowsOffset > 0)
    {
        ulFirstRow = pOSPData->_cSTDRows + 1;
    }

    // Now we need to figure out how many rows to fetch, so that we can
    //   allocate a memory block big enough to hold the handles.
    //   Calculate max number of rows which can be fetched in direction first.
    
    // If index is out of bounds, then we have either hit
    //  end of Rowset, or else we consider ourselves to have a bad bookmark.
    // BUGBUG: once Nile spec details are finalized, we can do better than
    //  bad bookmark in some cases.
    {
        BOOL fTooSmall = (LONG) ulFirstRow < 1;

        if (fTooSmall || (ulFirstRow > pOSPData->_cSTDRows))
        {
            if ((cRows < 0) !=  fTooSmall)
            {
                hr = DB_E_BADSTARTPOSITION;
                goto Cleanup;
            }
            else
            {
                // DB_S_ENDOFROWSET will be set below
                ulFirstRow = cRows < 0 ? 0 : pOSPData->_cSTDRows + 1;
            }
        }
    }

    // compute maximum available rows
    Assert(sIncrement < 0 || ulFirstRow > 0);
    cGetRows = (sIncrement < 0 ? ulFirstRow : pOSPData->_cSTDRows - ulFirstRow + 1);
    if ((LONG) cGetRows < 0)
    {
        cGetRows = 0;
    }
    
    if (ucRows <= cGetRows)
    {
        cGetRows = ucRows;
    }
    else
    {
        fEndOfRowset = TRUE;     // passed end of rowset.  not a failure.
    }

    // Allocate memory block if we need to:
    if (!*pahRows)
    {
        *pahRows = (HROW *)CoTaskMemAlloc(sizeof(HROW) * cGetRows);
        if (!*pahRows)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        fRowsAllocated = TRUE;
    }

    // Setup returned HROWs:
    {
        HROW *phRows;
        ULONG i;

        for (i = cGetRows, phRows = *pahRows;
             i;
             i--, ulFirstRow += sIncrement )
        {
            hr = THR(HRowNumber2HROWQuiet(hChapter, ulFirstRow, *phRows++));
            if (hr)
            {
                ReleaseRowsQuiet(cGetRows - i, *pahRows);
                if (fRowsAllocated)
                {
                    CoTaskMemFree(*pahRows);
                    *pahRows = NULL;
                }
                goto Cleanup;
            }
        }
    }

    *pcRowsObtained = cGetRows;
    if (fEndOfRowset)
    {
            hr = DB_S_ENDOFROWSET;
    }
           

#if defined(PRODUCT_97)
    FireRowEvent(cGetRows, *pahRows,
                 DBREASON_ROW_ACTIVATE, DBEVENTPHASE_DIDEVENT);
#endif

Cleanup:
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    Index2HROWQuiet (private member)
//
//  Synopsis:  Wrapper to convert an index to a HROW, without generating
//             notification of row activation.  (Up to caller).
//
//  Arguments: ulIndex      row index to convert to HROW
//             rhrow        reference to fill in with HROW
//
//  Returns:   hr           S_OK or E_OUTOFMEMORY or DB_E_NOTREENTRANT
//

HRESULT
CImpIRowset::Index2HROW(HCHAPTER hChapter, DBCOUNTITEM ulIndex, HROW &rhrow)
{
    RRETURN2(HRowNumber2HROWQuiet(hChapter, ulIndex, rhrow),
             DB_E_NOTREENTRANT, E_OUTOFMEMORY);
}

//+-----------------------------------------------------------------------
//
//  Member:    HRowNumber2HROWQuiet (private member)
//
//  Synopsis:  Wrapper to convert a HRow number to a HROW, without generating
//             notification of row activation.  (Up to caller).
//
//  Arguments: ulHRowNumber HRow number to convert to HROW
//             rhrow        reference to fill in with HROW
//
//  Returns:   hr           S_OK or E_OUTOFMEMORY or DB_E_NOTREENTRANT
//

HRESULT
CImpIRowset::HRowNumber2HROWQuiet(HCHAPTER hChapter, DBCOUNTITEM ulHRowNumber, HROW &rhrow)
{

    HRESULT hr = S_OK;
    ChRow href;

    href = HRowFromIndex(hChapter, ulHRowNumber);
    if (href.FHrefValid())
    {
        rhrow = href.ToNileHRow();
    }
    else
    {
        // We have an implicit assumption that the ONLY reason
        // an rhrow could not be created was out of memory.
        hr = E_OUTOFMEMORY;
        rhrow = 0;
    }

    RRETURN2(hr, DB_E_NOTREENTRANT, E_OUTOFMEMORY);
}

#if defined(PRODUCT_97)
//+-----------------------------------------------------------------------
//
//  Member:    Index2HROW (private member)
//
//  Synopsis:  Wrapper to convert an index to a HROW, generating
//             notification of row activation.
//
//  Arguments: ulIndex      row number to convert to HROW
//             rhrow        reference to fill in with HROW
//
//  Returns:   hr           S_OK or E_OUTOFMEMORY or DB_E_NOTREENTRANT
//

HRESULT
CImpIRowset::Index2HROW(ULONG ulIndex, HROW &rhrow)
{
    HRESULT hr = THR(Index2HROWQuiet(ulIndex, rhrow));

    if (!hr)
    {
        FireRowEvent(1, &rhrow,
                     DBREASON_ROW_ACTIVATE, DBEVENTPHASE_DIDEVENT);
    }
    return hr;
}
#endif  // defined(PRODUCT_97)

//+-----------------------------------------------------------------------
//
//  Member:    HROW2Index (private member)
//
//  Synopsis:  Given a HROW return the index
//
//  Arguments: hRow         HROW to convert
//             rulIndex     reference to fill in row number
//
//  Returns:   S_OK, DB_E_BADROWHANDLE, DB_E_DELETEDROW
//

HRESULT
CImpIRowset::HROW2Index(HROW hRow, DBCOUNTITEM &rulIndex)
{
    HRESULT hr = S_OK;
    rulIndex = IndexFromHRow((ChRow) hRow);

    if (FhRowDeleted((ChRow) hRow))
    {
        hr = DB_E_DELETEDROW;
    }

    if (rulIndex == 0)
    {
        hr = DB_E_BADROWHANDLE;
    }

    RRETURN2(hr, DB_E_BADROWHANDLE, DB_E_DELETEDROW);
}

//+-----------------------------------------------------------------------
//
//  Member:    GetRowsAt (public member)
//
//  Synopsis:  Given a bookmark and offset, returns the cRows HROWs found there.
//
//  Arguments: hChapter         chapter handle
//             cbBookmark       number of bytes in the bookmark
//             pBookmark        pointer to bookmark
//             lRowsOffset      offset from bookmark to first desired row
//             cRows            maximum number of HROWs to return
//             pcRowsObtained   number of HROWs actually returned  (OUT)
//             paRows           array of HROWs returned            (OUT)
//
//  Returns:   S_OK             if all rows could be fetched.
//             DB_S_ENDOFROWSET if we straddled the end of the rowset
//             E_INVALIDARG     if output pointers are null.
//             DB_E_BADCHAPTER  if chapter passed in
//             DB_E_BADBOOKMARK if bookmark was invalid.
//             DB_E_BADSTARTPOSITION if the offset goes off the end
//                              or beginning of the rowset
//             DB_S_BOOKMARKSKIPPED if initial bookmark referred to
//                              to a deleted row


STDMETHODIMP
CImpIRowset::GetRowsAt(HWATCHREGION, HCHAPTER hChapter,
                       DBBKMARK cbBookmark,  const BYTE *pBookmark,
                       DBROWOFFSET  lRowsOffset,     DBROWCOUNT  cRows,
                       DBCOUNTITEM *pcRowsObtained, HROW **pahRows )
{
    HRESULT thr = S_OK;                 // "temporary" hr

    HRESULT hr;
    DBCOUNTITEM ulRow;

    hr = Bookmark2HRowNumber(hChapter, cbBookmark, pBookmark, ulRow);

    // Note under the old scheme DB_S_BOOKMARKSKIPPED was returned from
    // GenerateHRowsFromRowNumber.  That scheme was fake.  We must detect
    // deleted rows now while we still have a bookmark, because once we're
    // working in terms of row numbers deleted rows are invisible.
    if (hr)
    {
        if (FAILED(hr))
            goto Cleanup;
        thr = hr;
    }

    // adjust the offset to take the skipped bookmark into account
    if (hr==DB_S_BOOKMARKSKIPPED)
    {
        if (lRowsOffset > 0)
        {
            -- lRowsOffset;
        }
        else if (lRowsOffset < 0)
        {
            ++ lRowsOffset;
            -- ulRow;
        }
    }

    hr = THR(GenerateHRowsFromHRowNumber(hChapter, ulRow, lRowsOffset, cRows,
                                         pcRowsObtained, pahRows ));

    // If we already have DB_S_BOOKMARKSKIPPED, then it takes precedence
    // over any S_ code that GenerateHRowsFromHRowNumber may return.
    if (thr && SUCCEEDED(hr))
    {
        Assert(thr == DB_S_BOOKMARKSKIPPED);
        hr = thr;
    }

Cleanup:
    return hr;
}



//+-----------------------------------------------------------------------
//
//  Member:    Compare (public member)
//
//  Synopsis:  Compare two bookmarks
//
//  Arguments: hChapter         chapter handle
//             cbBookmark1      number of bytes in the first bookmark
//             pBookmark1       pointer to first bookmark
//             cbBookmark2      number of bytes in the second bookmark
//             pBookmark2       pointer to second bookmark
//             pComparison      pointer to answer                  (OUT)
//
//  Returns:   S_OK             if we could do the comparison
//             E_INVALIDARG     if output pointer is null.
//             DB_E_BADCHAPTER  if chapter passed in
//             DB_E_BADBOOKMARK if bookmark was invalid.
//

STDMETHODIMP
CImpIRowset::Compare(HCHAPTER hChapter,
                     DBBKMARK cbBookmark1, const BYTE *pBookmark1,
                     DBBKMARK cbBookmark2, const BYTE *pBookmark2,
                     DBCOMPARE *pComparison )
{
    TraceTag((tagNileRowsetProvider,
              "CImpIRowset::Compare(%p {%u %p %u %p})",
              this, cbBookmark1, pBookmark1, cbBookmark2, pBookmark2 ));

    HRESULT hr = S_OK;

    if (!pComparison || !pBookmark1 || !pBookmark2 ||
    	cbBookmark1==0 || cbBookmark2==0)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *pComparison = DBCOMPARE_NOTCOMPARABLE; // In case something goes wrong

    DBCOUNTITEM ulRow1, ulRow2;

    // special case: one of the bookmarks is "first" or "last"
    if (cbBookmark1 == 1 && (*pBookmark1==DBBMK_FIRST || *pBookmark1==DBBMK_LAST))
    {
        if (cbBookmark2 == 1 && *pBookmark1 == *pBookmark2)
            *pComparison = DBCOMPARE_EQ;
        else
            *pComparison = DBCOMPARE_NE;
        goto Cleanup;
    }
    else if (cbBookmark2 == 1 && (*pBookmark2==DBBMK_FIRST || *pBookmark2==DBBMK_LAST))
    {
        if (cbBookmark1 == 1 && *pBookmark1 == *pBookmark2)
            *pComparison = DBCOMPARE_EQ;
        else
            *pComparison = DBCOMPARE_NE;
        goto Cleanup;
    }

    // normal case: first check that the bookmarks have the right shape
    if (cbBookmark1 != sizeof(ULONG) || cbBookmark2 != sizeof(ULONG) )
    {
        hr = DB_E_BADBOOKMARK;
        goto Cleanup;
    }

    // We use Bookmark2HRowNumber because it does First & Last bookmark special
    // cases for us, as well as DeleteInProgress handling (which might be
    // relevant? -cfranks), as well as all the bad bookmark tests.
    hr = Bookmark2HRowNumber(hChapter, cbBookmark1, pBookmark1, ulRow1);
    if (hr)
    {
        if (hr != DB_S_BOOKMARKSKIPPED) goto Cleanup;
        hr = S_OK;      // this call does not return DB_S_BOOKMARKSKIPPED
    }

    hr = Bookmark2HRowNumber(hChapter, cbBookmark2, pBookmark2, ulRow2);
    if (hr)
    {
        if (hr != DB_S_BOOKMARKSKIPPED) goto Cleanup;
        hr = S_OK;      // this call does not return DB_S_BOOKMARKSKIPPED
    }

    *pComparison = ulRow1 <  ulRow2 ? DBCOMPARE_LT :
                     ulRow1 == ulRow2 ? DBCOMPARE_EQ :
                     DBCOMPARE_GT;

Cleanup:
    return hr;
}



//+-----------------------------------------------------------------------

STDMETHODIMP
CImpIRowset::GetRowsByBookmark(HCHAPTER    hChapter,
                               DBCOUNTITEM cRows,
                               const DBBKMARK acbBookmarks[],
                               const BYTE *apBookmarks[],
                               HROW        ahRows[],
                               DBROWSTATUS aRowStatus[] )
{
#if 0                                   // started, but incomplete -cfranks
    HRESULT thr = S_OK;                 // "temporary" hr
    ULONG i, j;
    ULONG ulRow;

    HRESULT hr;
    ULONG   ulRow;
    ULONG	cErrors = 0;		// number of rows with bad status

    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::GetRowsAt(%p {%u %p %i %i})",
          this, cbBookmark, pBookmark, lRowsOffset, cRows) );

    j = 0;                              // j is output index (pahRows)
    for(i=0; i<cRows; i++)              // i is input index (acbBookmarks)
    {

        hr = Bookmark2HRowNumber(hChapter, acbBookmark[i], apBookmark[i], ulRow);

        // Note under the old scheme DB_S_BOOKMARKSKIPPED was returned from
        // GenerateHRowsFromRowNumber.  That scheme was fake.  We must detect
        // deleted rows now while we still have a bookmark, because once we're
        // working in terms of row numbers deleted rows are invisible.
        if (hr)
        {
            // S_BOOKMARKSKIPPED is not allowed in this call, so we
            // turn the skipped flag into a E_DELETEDROW
            if (hr==DB_S_BOOKMARKSKIPPED)
		        hr = DB_E_DELETEDROW;
        }
        else
        {
            hr = THR(GenerateHRowsFromHRowNumber(hChapter, ulRow, 0, 1,
                                                 pcRowsObtained, pahRows[j]));
        }

		// record the status of current requested row
		// BUGBUG: redo GenHRows to provide better status
        DBROWSTATUS drsStatus = hr ? DBROWSTATUS_E_INVALID : DBROWSTATUS_S_OK;
        if (aRowStatus)
        {
            aRowStatus[i] = drsStatus;
        }

        // Unless there was a more serious error, we want to be careful
        // to return DB_S_BOOKMARKSKIPPED, if we had one.
        if (!hr) hr = thr;
    } // for (i=0; i!=cRows; i++)

Cleanup:
    return hr;
#else

//    FireRowEvent(*pcRowsObtained, *pahRows,
//                 DBREASON_ROW_ACTIVATE, DBEVENTPHASE_DIDEVENT);

    return E_NOTIMPL; // BUGBUG: Do this
#endif
}

//+-----------------------------------------------------------------------

STDMETHODIMP
CImpIRowset::Hash(HCHAPTER    hChapter,
                  DBBKMARK    cBookmarks, const DBBKMARK acbBookmarks[],
                  const BYTE *apBookmarks[],
                  DBHASHVALUE aHashedValues[],
                  DBROWSTATUS aBookmarkStatus[] )
{
    return E_NOTIMPL; // BUGBUG: This routine changes drastically in June spec...
}
