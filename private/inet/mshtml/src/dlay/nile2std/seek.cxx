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

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

////////////////////////////////////////////////////////////////////////////////
//
// IRowsetFind specific interface
//
////////////////////////////////////////////////////////////////////////////////




//+-----------------------------------------------------------------------
//
//  Member:    FindNextRows (public member)
//
//  Synopsis:  Given a bookmark and comparison spec, returns cRows HROWs
//             matching.
//
//  Arguments: hChapter         chapter handle
//             hAccessor        accessor for value to be matched
//             pValue           pointer to the value to be matched
//             CompareOp        comparison operation to be used
//             cbBookmark       length of bookmark in bytes
//             pBookmark        pointer to bookmark vector
//             fSkipCurrent     skip current row if TRUE
//             cRows            number of consecutive rows requested to fetch
//             pcRowsObtained   number of consecutive rows actually fetched (OUT)
//             paRows           array of HROWs returned (OUT)
//
//  Returns:   S_OK             if all rows could be fetched.
//             DB_EROWLIMITEDEXCEEDED if cRows is > # rows in rowset
//             DB_E_BADINFO     hAccessor specified binding for more than one
//                              column, or was otherwise gubbish
//             DB_S_ENDOFROWSET if we straddled the end of the rowset
//             E_INVALIDARG     if output pointers are null.
//             DB_E_BADCHAPTER  if chapter passed in
//             DB_E_BADBOOKMARK if bookmark was invalid.
//

STDMETHODIMP
CImpIRowset::FindNextRow(HCHAPTER hChapter, HACCESSOR hAccessor, void * pValue,
                          DBCOMPAREOP CompareOp, DBBKMARK cbBookmark,
                          const BYTE *pBookmark, DBROWOFFSET  lRowsOffset,
                          DBROWCOUNT  cRows, DBCOUNTITEM *pcRowsObtained, HROW  **pahRows)
{
    TraceTag((tagNileRowsetProvider,
          "CImpIRowset::GetRowsByValues(%p {%u %p %i %i})",
          this, cbBookmark, pBookmark, cRows) );

    HRESULT hr;
    DBCOUNTITEM ulHRow, ulIndex;  // holds HRowNumber, index
    DBROWCOUNT  lIndex2;
    LONG    iter=1;             // -1 for backwards, 1 for forwards
    OSPFIND stdFind=(OSPFIND)0; // stdFind flags for ISimpleTabularData Find function
    OSPCOMP stdComp;
    VARIANT var;
    BOOL    fRowsAllocated=FALSE; // TRUE iff we allocated memory for pahRows
    DBBINDING       *pBinding;
    ULONG           cBindings;
    AccessorFormat  *pAccessor;
    COSPData        *pOSPData = GetpOSPData(hChapter);

    Assert(hAccessor);
    pAccessor = (AccessorFormat *)hAccessor;

    cBindings = pAccessor->cBindings;
    pBinding  = pAccessor->aBindings;

    // Binding must be for one column, and must have a value part..
    // (anything else we should add here??)
    if (cBindings != 1)
    {
        hr = DB_E_BADBINDINFO;
        goto Cleanup;
    }

    if (pcRowsObtained == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (pOSPData == NULL)
    {
        hr = DB_E_BADCHAPTER;
        goto Cleanup;
    }
    
    *pcRowsObtained = 0;

    // NULL bookmark argument means use internal cursor
    if (!pBookmark)
    {
        pBookmark = (BYTE *)&pOSPData->_iFindRowsCursor;
        cbBookmark = sizeof(ULONG);
        if (pOSPData->_iFindRowsCursor==DBBMK_INITIAL)
        {
            pOSPData->_iFindRowsCursor = cRows > 0 ? DBBMK_FIRST : DBBMK_LAST;
        }
    }

    hr = THR(Bookmark2HRowNumber(hChapter, cbBookmark, pBookmark, ulHRow));
    if (hr)
    {
        if (hr != DB_S_BOOKMARKSKIPPED) goto Cleanup;
        hr = S_OK;      // this call does not return DB_S_BOOKMARKSKIPPED
    }

    // Convert OLE-DB compareOps to STD compType.
    switch (CompareOp)
    {
        case DBCOMPAREOPS_LT:
            stdComp = OSPCOMP_LT; 
            break;
        case DBCOMPAREOPS_LE:
            stdComp = OSPCOMP_LE;
            break;
        case DBCOMPAREOPS_EQ:
            stdComp = OSPCOMP_EQ;
            break;
        case DBCOMPAREOPS_GE:
            stdComp = OSPCOMP_GE;
            break;
        case DBCOMPAREOPS_GT:
            stdComp = OSPCOMP_GT;
            break;

//        case DBCOMPAREOPS_PARTIALEQ:    // special case
//            hr = E_FAIL;                // not yet implemented
//            goto Cleanup;
//            stdComp = OSPCOMP_EQ;       // BUGBUG - partial not done yet.
//            break;

        case DBCOMPAREOPS_NE:
            stdComp = OSPCOMP_NE;
            break;
        default:
            hr = E_INVALIDARG;
            goto Cleanup;
    }

    ulHRow += lRowsOffset;

    // range check rowIndex, esp after above compuation.
    if ((LONG) ulHRow < 1 || ulHRow > pOSPData->_cSTDRows)
    {
        hr = DB_S_ENDOFROWSET;
        goto Cleanup;
    }

    if (cRows < 0)
    {
        stdFind = OSPFIND_UP;
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

    ulIndex = ulHRow;

    // BUGBUG:: We depend on OLEDB types and Variant types to be the same here.
    // This is true at least through DBYTE_UI8 == VT_UI8
    if (pBinding->wType > DBTYPE_UI8)
    {
        hr = DB_E_UNSUPPORTEDCONVERSION;
        goto Cleanup;
    }

    // special case NULL binding
    if (pBinding->dwPart&DBPART_STATUS &&
        DBSTATUS_S_ISNULL==*(DBSTATUS *)((BYTE *)pValue+pBinding->obStatus))
    {
        var.vt = VT_NULL;
    }
    else if (pBinding->dwPart&DBPART_VALUE)
    {
        CVarToVARIANTARG((void *)((BYTE *)pValue+pBinding->obValue),
                         pBinding->wType, (VARIANTARG *)&var);
    }
    else
    {
        // We were passed neither a NULL binding nor a value.
        hr = DB_E_BADBINDINFO;
        goto Cleanup;
    }

 
    // See if we can find a matching row.
    hr = GetpOSP(hChapter)->find(ulIndex,  pBinding->iOrdinal, var, stdFind, stdComp,
                                 &lIndex2);
    // -1 is returned for not found
    if (lIndex2==-1)
    {
        hr = DB_S_ENDOFROWSET;          // by spec
        goto Cleanup;
    }

    if (FAILED(hr))
    {
        // Do some conversion of OLE error codes to OLE-DB error codes if we
        // know how, otherwise clamp to E_UNEXPECTED
        if (DISP_E_TYPEMISMATCH==hr)
            hr = DB_E_UNSUPPORTEDCONVERSION;
        else
            hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // Get the next cRows rows.. cRows might even be zero
    while (cRows--)
    {
        // Convert our hRowIndex to a real hRow and put it in the result array
        Index2HROW(hChapter, (ULONG)lIndex2, *pahRows[*pcRowsObtained]);

        (*pcRowsObtained)++;
        lIndex2 += iter;

        // About to step off the beginning?
        if (iter<0 && lIndex2==0)
        {
            hr = DB_S_ENDOFROWSET;
            break;
        }

    }

    // Spec says we set internal cursor to bookmark of last row found,
    // not the last row returned!
    if (cRows)
    {
        pOSPData->_iFindRowsCursor = *pahRows[0];
    }

Cleanup:
    if (fRowsAllocated && !*pcRowsObtained)
    {
        CoTaskMemFree((void *)pahRows);
        *pahRows = NULL;
    }

    // We don't need to set a whole clamp list here, since standard OLE-DB error
    // codes are handled by RRETURN now.
    RRETURN(hr);
}

