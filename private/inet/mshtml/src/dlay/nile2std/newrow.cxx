//+-----------------------------------------------------------------------
//  Microsoft OLE/DB to STD Mapping Layer
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      newrow.cxx
//  Author:    Ted Smith
//
//  Contents:  Implementation of RowsetNewRowAfter
//
//------------------------------------------------------------------------

#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

////////////////////////////////////////////////////////////////////////////////
//
// IRowsetNewRowAfter specific interface
//
////////////////////////////////////////////////////////////////////////////////


//+-----------------------------------------------------------------------
//
//  Member:    SetNewDataAfter (public member)
//
//  Synopsis:  Create and initialize a new row in the STD.
//
//  Arguments: hChapter            chapter handle [IN]
//             cbBookmarkPrevious  number of bytes in the bookmark [IN]
//             pBookmarkPrevious   pointer to bookmark [IN]
//             hAccessor           Accessor to use [IN]
//             pData               Pointer to buffer of data to set [IN]
//             phRow               Row Handle [OUT]
//
//  Returns:   S_OK                    if data changed successfully
//             E_FAIL                  if Catch all (NULL pData, etc.)
//             E_OUTOFMEMORY           if output error array couldn't be allocated
//             DB_E_BADACCESSORHANDLE  if invalid accessor
//             DB_E_BADCHAPTER         if chapter passed in
//             DB_E_BADBOOKMARK        if bookmark was invalid.
//             DB_E_NOTREENTRANT       if illegal reentrancy
//

STDMETHODIMP
CImpIRowset::SetNewDataAfter(HCHAPTER    hChapter,
                             DBBKMARK    cbBookmarkPrevious,
                             const BYTE *pBookmarkPrevious,
                             HACCESSOR   hAccessor,
                             BYTE       *pData,
                             HROW       *phRow )
{
    TraceTag((tagNileRowsetProvider,
              "CImpIRowset::SetNewDataAfter(%p {%u %p %p %p})",
              this, cbBookmarkPrevious, pBookmarkPrevious, hAccessor, pData ));

    HRESULT hr;
    HRESULT hrBookmarkSkipped = S_OK;
    DBCOUNTITEM   ulIndex;
    DBROWCOUNT    cRowsInserted;
    HROW    hRow = NULL;
    HROW    *phRowSafe = (phRow ? phRow : &hRow);
    COSPData *pOSPData = GetpOSPData(hChapter);

    if (FAnyPhaseInProgress(DBREASON_ROW_DELETE)
            || FAnyPhaseInProgress(DBREASON_ROW_INSERT))
    {
        hr = DB_E_NOTREENTRANT;
        goto Cleanup;
    }

    if (pOSPData == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Cleanup;
    }
    
    if (cbBookmarkPrevious == 0)
    {
        ulIndex = 1;
    }
    else if (pOSPData->_cSTDRows==0 && cbBookmarkPrevious==1 &&
             (*pBookmarkPrevious==DBBMK_FIRST || *pBookmarkPrevious==DBBMK_LAST))
    {
        ulIndex = 1;
    }
    else
    {
        hr = THR(Bookmark2HRowNumber(hChapter, cbBookmarkPrevious, pBookmarkPrevious,
                                     ulIndex));
        if (hr)
        {
            if (hr != DB_S_BOOKMARKSKIPPED) goto Cleanup;
            hrBookmarkSkipped = hr;
        }

        if (ulIndex > pOSPData->_cSTDRows)
        {
            hr = DB_S_BOOKMARKSKIPPED;
            ulIndex = pOSPData->_cSTDRows;
        }
        ulIndex += 1;                   // +1 to insert AFTER current row
    }
    Assert(ulIndex > 0 && ulIndex <= pOSPData->_cSTDRows + 1);

    hr = THR(GetpOSP(hChapter)->insertRows((LONG)ulIndex, 1, &cRowsInserted));
    if (hr)
    {
        if (hr != E_OUTOFMEMORY)
        {
            hr = E_FAIL;
        }
        goto Cleanup;
    }
	
    hr = THR(Index2HROW(hChapter, ulIndex, hRow));
    if (hr)
    {
        goto Error;
    }

	// BUGBUG: set default values into the columns of the new row (by writing
	// no code, we're accepting whatever the STD happens to leave in memory
	// when it allocates the new row)
	
	// BUGBUG: generate a notification that a new row is being added

    Assert(hRow != NULL);        
    // BUGBUG ole-db will presumably change pData from (const BYTE*) to (void*)
    // someday, and this cast can go away
    hr = THR(SetData(hRow, hAccessor, (void*)pData));
    if (FAILED(hr))
    {
        goto Error;
    }
        
Cleanup:
    if ((phRow == NULL || hr) && hRow != NULL)
    {
        ReleaseRows(1, &hRow, NULL, NULL, NULL);
        hRow = NULL;
    }
    *phRowSafe = hRow;

    if (hr == S_OK)
    	hr = hrBookmarkSkipped;
    return hr;
    
Error:
    GetpOSP(hChapter)->deleteRows((LONG) ulIndex, 1, &cRowsInserted);
    goto Cleanup;
}
