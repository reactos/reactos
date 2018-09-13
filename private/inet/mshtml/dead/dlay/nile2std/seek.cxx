//+-----------------------------------------------------------------------
//  Microsoft OLE/DB to STD Mapping Layer
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      seek.cxx
//  Author:    Ted Smith  (tedsmith@microsoft.com)
//
//  Contents:  Implementation of RowsetFind
//
//------------------------------------------------------------------------

#include <dlaypch.hxx>
#pragma hdrstop

#include "rowset.hxx"

#ifdef NEVER
// We're removing GetRowsByValues because it looks like the OLE-DB spec is going with
// a different IRowsetFind methods (FindNextRows).  However, we're keeping the code
// here because it's proably going to be a good starting place for the eventual
// implementation of FindNextRows.  -cfranks 8 May 1997

////////////////////////////////////////////////////////////////////////////////
//
// IRowsetFind specific interface
//
////////////////////////////////////////////////////////////////////////////////




//+-----------------------------------------------------------------------
//
//  Member:    GetRowsByValues (public member)
//
//  Synopsis:  Given a bookmark and comparison spec, returns cRows HROWs
//             matching.
//
//  Arguments: hChapter         chapter handle
//             cbBookmark       number of bytes in the bookmark
//             pBookmark        pointer to bookmark
//             lRowsOffset      offset from bookmark to first desired row
//             cValues          # of columes to search on
//             aColumns         which column(s)
//             aValueTypes      types of the coresponding entries in aValues
//             aValues          values to search for
//             aCompareOps      how to search
//             cRows            maximum number of HROWs to return
//             pcRowsObtained   number of HROWs actually returned  (OUT)
//             paRows           array of HROWs returned            (OUT)
//
//  Notes:     BUGBUG:  Only 0 or 1 are accepted for cValues.  I.e. no multi-
//                        column support.
//             BUGBUG:  When IRowset::GetNextRow is implemented this routine
//                        will have to move the cursor also.
//             BUGBUG:  Only DBTYPE_WSTR is accepted.
//             BUGBUG:  Only DBCOMPAREOPS_EQ, _NE, _PARTIALEQ and
//                        _NOTPARTIALEQ are accepted.  (This isn't in Nile yet!)
//             BUGBUG:  Only 1 or -1 is accepted for cRows
//
//  Returns:   S_OK             if all rows could be fetched.
//             DB_S_ENDOFROWSET if we straddled the end of the rowset
//             E_INVALIDARG     if output pointers are null.
//             DB_E_BADCHAPTER  if chapter passed in
//             DB_E_BADBOOKMARK if bookmark was invalid.
//             E_FAIL           one of the above bugbugs is still there
//

STDMETHODIMP
CImpIRowset::GetRowsByValues(HCHAPTER hChapter,
                             ULONG  cbBookmark,    const BYTE *pBookmark,
                             LONG   lRowsOffset,
                             ULONG  cValues,       const ULONG aColumns[],
                             const DBTYPE aValueTypes[], const BYTE *aValues[],
                             const DBCOMPAREOPS aCompareOps[],
                             LONG  cRows, ULONG *pcRowsObtained,
                             HROW **pahRows )
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::GetRowsByValues(%p {%u %p %i %i})",
          this, cbBookmark, pBookmark, lRowsOffset, cRows) );

    HRESULT hr, hrLocal;
    ULONG   ulHRow, ulIndex;  // holds HRowNumber, index
    LONG    lIndex2;
    LONG    iter=1;             // -1 for backwards, 1 for forwards
    STDFIND stdFind=(STDFIND)0; // stdFind flags for ISimpleTabularData Find function
    STDCOMP stdComp;
    VARIANT var;
    BOOL    fRowsAllocated=FALSE; // TRUE iff we allocated memory for pahRows

    if (pcRowsObtained == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
    
    *pcRowsObtained = 0;
    
    hr = WhineAboutChapters(hChapter);
    if (hr)
    {
        goto Cleanup;
    }

    // BUGBUG:: We don't implement the whole spec.  We can only handle
    // one column.
    if (cValues > 1)
    {
        hr = E_NOTIMPL;
        goto Cleanup;
    }

    hr = THR(Bookmark2HRowNumber(cbBookmark, pBookmark, ulHRow));
    if (hr)
    {
        if (hr != DB_S_BOOKMARKSKIPPED) goto Cleanup;
        hr = S_OK;      // this call does not return DB_S_BOOKMARKSKIPPED
    }


    // Convert OLE-DB compareOps to STD compType.
    // BUGBUG:: What is INCLUDENULLS about?  -cfranks 17 Dec 96
    switch (aCompareOps[0]&~DBCOMPAREOPS_INCLUDENULLS)
    {
        case DBCOMPAREOPS_LT:
            stdComp = STDCOMP_LT; 
            break;
        case DBCOMPAREOPS_LE:
            stdComp = STDCOMP_LE;
            break;
        case DBCOMPAREOPS_EQ:
            stdComp = STDCOMP_EQ;
            break;
        case DBCOMPAREOPS_GE:
            stdComp = STDCOMP_GE;
            break;
        case DBCOMPAREOPS_GT:
            stdComp = STDCOMP_GT;
            break;

        case DBCOMPAREOPS_PARTIALEQ:    // special case
            stdComp = STDCOMP_EQ;       // BUGBUG - partial not done yet.
            break;

        case DBCOMPAREOPS_NE:
            stdComp = STDCOMP_NE;
            break;
        default:
            hr = E_INVALIDARG;
            goto Cleanup;
    }

    // In case of errors:
    *pcRowsObtained = 0;

    if ((LONG) ulHRow < 0)
    {
        hr = DB_E_BADBOOKMARK;
        goto Cleanup;
    }

    ulHRow += lRowsOffset;

    // We know that index was non-negative before the above operation.
    // So we can only overflow with large positive index, lRowsOffset...
    // Check for numeric overflow, arrange for appropriate error below
    if ((LONG) ulHRow < 0 && lRowsOffset > 0)
    {
        ulHRow = _cRowsVirtual + 1;
    }

    // If index is out of bounds, then we have either hit
    //  end of Rowset, or else we consider ourselves to have a bad bookmark.
    // BUGBUG: once Nile spec details are finalized, we can do better than
    //  bad bookmark in some cases.
    {
        BOOL fTooSmall = (LONG) ulHRow < 1;

        if (fTooSmall || (ulHRow > _cRowsVirtual))
        {
            if ((cRows < 0) !=  fTooSmall)
            {
                hr = DB_E_BADBOOKMARK;
                goto Cleanup;
            }
            else
            {
                // DB_S_ENDOFROWSET will be set below
                ulHRow = cRows < 0 ? 0 : _cRowsVirtual + 1;
            }
        }
    }

    if (cRows < 0)
    {
        stdFind = STDFIND_UP;
        cRows = -cRows;     // now its an iteration count, not a direction
        iter = -1;
    }

    // If we were asked to, allocate a block of memory large enough to hold all
    // the rows requested (even though our match might not turn up this many).
    if (!*pahRows)
    {
        *pahRows = (HROW *)CoTaskMemAlloc(sizeof(HROW) * cRows);
        if (!*pahRows)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        fRowsAllocated = TRUE;
    }

    // Convert to hRow to an hROWIndex so we're working in terms the real STD
    // (where any deletions are actually gone), rather than the virtual STD
    // (which might try to mask a delete in progress).
    // We also do our lRowsOffset calculation here.
    hrLocal = _Dip.HRowNumberToIndex(ulHRow+lRowsOffset, FALSE, ulIndex,
                                     !!(stdFind&STDFIND_UP));

    // GetRowsByValues is a filter.  The STD Find operation searches for the
    // next match, so to do GetRowsByValues, we need to iterate.
    for (;*pcRowsObtained < (ULONG)cRows; (*pcRowsObtained)++)
    {
        // Are we about to go off the end?
        if (ulIndex >= _cSTDRows)
        {
            hr = DB_S_ENDOFROWSET;
            break;
        }

        // BUGBUG:: We are passing an OLE-DB dbtype (rgValueTypes) here directly
        // to a routine  that is expecting a VARIANT vt parameter.  This happens
        // to work, but there might be a cleaner way to do this.
        CVarToVARIANTARG(aValues, aValueTypes[0], (VARIANTARG *)&var);
 
        // Start finding hRows.
        // BUGBUG:: OLE-DB spec says aColumns is an array of columns to match
        // against.  We just match against one!
        hr = _pSTD->Find(ulIndex, aColumns[0], var, stdFind, stdComp,
                         &lIndex2);
        // -1 is returned for not found
        if (lIndex2==-1)
        {
            break;      // no more to find, we're done
        }

        if (FAILED(hr))
        {
            // We rely on the RRETURN below to clamp the error codes
            goto Cleanup;
        }

        // Convert our hRowIndex to a real hRow and put it in the result array
        Index2HROWQuiet((ULONG)lIndex2, *pahRows[*pcRowsObtained]);

        // About to step off the beginning?
        if (iter<0 && ulIndex==0)
        {
            hr = DB_S_ENDOFROWSET;
            break;
        }
        ulIndex += iter;
    }

Cleanup:
    RRETURN2(hr, DB_S_ENDOFROWSET, DB_E_BADBOOKMARK);
}
#endif
