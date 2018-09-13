//+-----------------------------------------------------------------------
//  Microsoft OLE/DB to STD Mapping Layer
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      scroll.cxx
//  Author:    Ido Ben-Shachar (t-idoben@microsoft.com)
//
//  Contents:  Implementation of RowsetExactScroll
//             Implementation of RowsetScroll
//
//------------------------------------------------------------------------

#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

////////////////////////////////////////////////////////////////////////////////
//
// IRowsetScroll specific interfaces
//
////////////////////////////////////////////////////////////////////////////////


//+-----------------------------------------------------------------------
//
//  Member:    GetApproximatePosition (public member)
//
//  Synopsis:  Given a bookmark, returns the corresponding row number.
//
//  Arguments: hChapter         chapter handle
//             cbBookmark       number of bytes in the bookmark
//             pBookmark        pointer to bookmark
//             pulPosition      row number                       (OUT)
//             pcRows           total number of rows             (OUT)
//
//  Returns:   Success          if inputs are valid.
//             E_INVALIDARG     if output pointers are null.
//             DB_E_BADCHAPTER  if chapter passed in
//             DB_E_BADBOOKMARK if bookmark is invalid.
//             DB_E_NOTREENTRANT if illegal reentrancy (nested delete)
//

STDMETHODIMP
CImpIRowset::GetApproximatePosition (HCHAPTER hChapter,
                                     DBBKMARK cbBookmark,   const BYTE *pBookmark,
                                     DBCOUNTITEM *pulPosition, DBCOUNTITEM *pcRows )
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::GetApproximatePosition(%p {%u %p %p %p})",
          this, cbBookmark, pBookmark, pulPosition, pcRows) );

    HRESULT hr = S_OK;
    DBCOUNTITEM ulPosition;
    COSPData *pOSPData = GetpOSPData(hChapter);

    if (cbBookmark && pBookmark==NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pOSPData == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Cleanup;
    }
    
    if (pcRows)
        *pcRows = pOSPData->_cSTDRows; // get rows in table

    if (cbBookmark == 0)
        goto Cleanup;                   // no position desired, just size

    hr = THR(Bookmark2HRowNumber(hChapter, cbBookmark, pBookmark, ulPosition));
    if (hr)
    {
        if (hr != DB_S_BOOKMARKSKIPPED) goto Cleanup;
        hr = S_OK;      // this call does not return DB_S_BOOKMARKSKIPPED
    }
    
    if (ulPosition > pOSPData->_cSTDRows && cbBookmark != 1) // Predefined bmks OK
    {
        hr = DB_E_BADBOOKMARK;
    }

    if (pOSPData->_cSTDRows == 0) // if rowset is empty, always return 0 position
        ulPosition = 0;
	
    if (pulPosition)
    	*pulPosition = ulPosition;

Cleanup:
    return hr;
}


//+-----------------------------------------------------------------------
//
//  Member:    GetRowsAtRatio (public member)
//
//  Synopsis:  Returns a set of contiguous rows within the rowset, starting
//             at a certain ratio into the rowset.  The user can allocate
//             memory for the returned row handles by setting pahRows
//             to a block of memory.  If it is NULL, this function will
//             allocate that memory.
//
//  Arguments: hChapter         chapter handle
//             ulNumberator     numerator of ratio
//             ulDenominator    denominator of ratio
//             cRows            rows to fetch and direction to fetch in
//             pcRowsObtained   rows obtained
//             pahRows          handles of rows obtained
//
//  Returns:   S_OK             if all rows could be fetched.
//             DB_S_ENDOFROWSET if we straddled the end of the rowset
//             E_INVALIDARG     if output pointers are null.
//             DB_E_BADCHAPTER  if chapter passed in
//             DB_E_BADBOOKMARK if bookmark was invalid.
//

STDMETHODIMP
CImpIRowset::GetRowsAtRatio (HWATCHREGION,
                             HCHAPTER hChapter,
                             DBCOUNTITEM ulNumerator,
                             DBCOUNTITEM ulDenominator,
                             DBROWCOUNT cRows,
                             DBCOUNTITEM *pcRowsObtained,
                             HROW **pahRows )
{
    TraceTag((tagNileRowsetProvider,
             "CImpIRowset::GetRowsAtRatio(%p {%u %u %d %p %p})",
             this, ulNumerator, ulDenominator, cRows,
             pcRowsObtained, pahRows) );

    HRESULT hr;
    LONG    iRowFirst;                   // row to start fetching at
    COSPData *pOSPData = GetpOSPData(hChapter);

    if (pOSPData == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Cleanup;
    }

    // Find row to start fetching at.  Note that this function returns -1
    //   on overflow or division by 0, so the error checking, in checking
    //   bounds, catches this.
    // Note the +1 because ulNumerator is zero-based, and we're 1-based.
    iRowFirst = MulDiv(pOSPData->_cSTDRows,
                       ulNumerator, ulDenominator) + 1;

    if (pOSPData->_cSTDRows < (ULONG)iRowFirst)
    {
        hr = DB_E_BADRATIO;
        goto Cleanup;
    }

    // GenerateHRowsFromHRowNumber fails NULL args
    hr = THR(GenerateHRowsFromHRowNumber(hChapter, iRowFirst, 0, cRows,
                                         pcRowsObtained, pahRows ));
    Assert(FAILED(hr) || (pcRowsObtained != NULL && pahRows != NULL));
    if (hr)
    {
        // We can't return DB_S_BOOKMARKSKIPPED; figure out if that was
        //  masking a DB_S_ENDOFROWSET.
        if (hr == DB_S_BOOKMARKSKIPPED)
        {
            hr = (*pcRowsObtained == (ULONG) abs(cRows)
                    ? S_OK : DB_S_ENDOFROWSET);
        }
        else if (FAILED(hr))
        {
            // We can't return DB_E_BADBOOMARK, either
            if (hr == DB_E_BADBOOKMARK)
            {
                hr = DB_E_BADRATIO;
            }
            goto Cleanup;
        }
    }                                
            

    FireRowEvent(*pcRowsObtained, *pahRows,
                 DBREASON_ROW_ACTIVATE, DBEVENTPHASE_DIDEVENT);

Cleanup:
    return hr;
}


////////////////////////////////////////////////////////////////////////////////
//
// IRowsetExactScroll specific interfaces
//
////////////////////////////////////////////////////////////////////////////////


//+-----------------------------------------------------------------------
//
//  Member:    GetExactPosition (public member)
//
//  Synopsis:  Given a bookmark, returns the corresponding row number.
//
//  Arguments: hChapter         hChapter
//             cbBookmark       number of bytes in the bookmark
//             pBookmark        pointer to bookmark
//             pulPosition      row number                       (OUT)
//             pcRows           total number of rows             (OUT)
//
//  Returns:   Success          if inputs are valid.
//             E_INVALIDARG     if output pointers are null.
//             DB_E_BADCHAPTER  if chapter passed in
//             DB_E_BADBOOKMARK if bookmark is invalid.
//

STDMETHODIMP
CImpIRowset::GetExactPosition (HCHAPTER hChapter,
                               DBBKMARK cbBookmark,
                               const BYTE *pBookmark,
                               DBCOUNTITEM *pulPosition,
                               DBCOUNTITEM *pcRows )
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::GetExactPosition(%p {%u %p %p %p})",
          this, cbBookmark, pBookmark, pulPosition, pcRows) );

    return GetApproximatePosition(hChapter, cbBookmark, pBookmark,
                                  pulPosition, pcRows );
}
