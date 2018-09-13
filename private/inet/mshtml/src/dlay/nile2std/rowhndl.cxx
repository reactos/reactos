//+-----------------------------------------------------------------------
//  Microsoft OLE/DB to STD Mapping Layer
//  Copyright (C) Microsoft Corporation, 1995
//
//  File:      rowhndl.cxx
//  Author:    Charles Frankston
//
//  Contents:  Implementation of new scheme to manage row handles & bookmarks
//  Date:      started May 28, 1996
////
//------------------------------------------------------------------------

// keep our code to 80 columns:
//       1         2         3         4         5         6         7         
// 4567890123456789012345678901234567890123456789012345678901234567890123456789

#include <dlaypch.hxx>

#ifndef X_ROWSET_HXX_
#define X_ROWSET_HXX_
#include "rowset.hxx"
#endif

MtDefine(CRowArray, DataBind, "CRowArray")

//+------------------------------------------------------------------------
//
//  Class:      CRowMap
//
//  Purpose:    Class used to track translations from hRows to STD row
//              indexes, and visa versa.
//
//-------------------------------------------------------------------------

//+------------------------------------------------------------------------
//
//  Member: CRowMap::HandleFromRow
//
//  Synopsis:   Returns the handle corresponding to a row.  These handles
//              are meant to be used at hRows or Bookmarks.  Handles persist
//              for the life of the CRowMap class they came from.  They
//              always continue to point to the same row (even in the face
//              of row inserts and deletes) unless that row is deleted, in
//              which case they will point to the first non-deleted row
//              following the original row.
//
//  Arguments:  row   row # to get a handle to.
//
//-------------------------------------------------------------------------
ChRow
CImpIRowset::HRowFromIndex(HCHAPTER hChapter, DBCOUNTITEM row)
{
    ChRow href;                 // Handle reference
    CRowArray<ChRow> *pMapIndex2hRow = &GetpOSPData(hChapter)->_mapIndex2hRow;

   // Get the hRow for this row if there already is one.
    href = pMapIndex2hRow->GetElem ((int) row);

    if (!href.FHrefValid())                    // Didn't already have one.
    {
        // Get the next free handle, set it to point to this row.
        if (S_OK != _maphRow2Index.SetElem(_NextH2R,
                                           CIndex((int)row, (COSPData *)(LONG_PTR)hChapter)))
        {
            // Must have been a mem alloc failure.  Don't assign to href.
            goto Error;
        }
        // Record the hRow we just claimed,
        href = href.SetRef(_NextH2R++);;

        // and set the entry for this row to point back to href entry
        pMapIndex2hRow->SetElem((int) row, href);
    }

Error:
    return (href);
}
    
//+------------------------------------------------------------------------
//
//  Member: CRowMap::RowFromHRow
//
//  Synopsis:   Returns the Row # that a particular handle addresses.
//              If the row has been deleted, the handle will still be valid
//              but would now point to the first non-deleted row following.
//
//  Arguments:  handle
//
//-------------------------------------------------------------------------
/* Inline'd in .hxx file! */

//+------------------------------------------------------------------------
//
//  Member: CRowMap::FhRowDeleted
//
//  Synopsis:   Returns TRUE if the row referred to by handle has been deleted.
//
//  Arguments:  handle
//
//-------------------------------------------------------------------------
/* Inline'd in .hxx file! */

// For iterating through ChRow's.
ChRow
CImpIRowset::FirsthRef(HCHAPTER hChapter)
{
    ChRow hRef;                         // contructed at 0

    return NexthRef(hChapter, hRef);
}

ChRow
CImpIRowset::NexthRef(HCHAPTER hChapter, ChRow hRef)
{
    // Advance to the next CIndex that matches the chapter
    // we're working on.
    for (hRef=hRef.NextHRef();  ValidhRef(hRef);  hRef=hRef.NextHRef())
    {
        if (_maphRow2Index.GetElem(hRef.DeRef()).GetpChap() ==
           (COSPData *)(LONG_PTR)hChapter)
       {
           break;
       }
    }
    return hRef;
}

BOOL
CImpIRowset::ValidhRef(ChRow hRef)
{
    return ((LONG)hRef.DeRef() < _NextH2R);
}

//+------------------------------------------------------------------------
//
//  Member: CRowMap::InsertRows
//
//  Synopsis:   Inserts new rows.  The row table (mpIndex2hRow) is shifted
//              up to make room, and all the handles in the handle table
//              (_maphRow2Index) are adjusted to account for the shift.
//
//  Arguments:  row     index of row to start insertion at (i.e. 0 would
//                      insert before the first element).
//              c       count of # of rows to insert
//              pChap   Pointer to chapter that we're dealing with now
//              mpIndex2hRow The handle table for this chapter
//
//-------------------------------------------------------------------------
HRESULT
CImpIRowset::InsertRows(ULONG row, int c, COSPData *pOSPData)
{
    HRESULT hr;
    ChRow hRef;                         // Constructor will create a 0 hRef
    CIndex v;
    HCHAPTER hChapter = HChapterFromOSPData(pOSPData);
    
    Assert(row >= 0 && c >= 0);

    // Create space for new rows in Row2hRow table.
    // New entries are initialized to invalid hRef, showing that there
    // aren't handles checked out yet for the new rows.
    hr = pOSPData->_mapIndex2hRow.InsertMultiple(row, c, hRef);
    if (hr)
        goto Error;         // out of mem for insert
    
    // March through hRow array fixing up values
    for (hRef = FirsthRef(hChapter); ValidhRef(hRef);
         hRef = NexthRef(hChapter, hRef))
    {
        v = _maphRow2Index.GetElemNoCheck(hRef.DeRef());

        if (v.Row() >= (int)row)
        {
            _maphRow2Index.SetElemNoCheck(hRef.DeRef(),
                                         v.SetRow(v.Row()+c, v.FDeleted()));
        }
    }
    
Error:
    return hr;
}

//+------------------------------------------------------------------------
//
//  Member: CRowMap::DeleteRows
//
//  Synopsis:   Deletes rows.  The row table (mpIndex2hRow) is shifted
//              down, and all the handles in the handle table
//              (_mapIndex2hRow) are adjusted to account for the shift.
//
//  Arguments:  row     index of row to start deletion at (i.e. 0 would
//                      start deletion at the first element).
//              c       count of # of rows to delete
//              pChap   Pointer to chapter that we're dealing with now
//              mpIndex2hRow The handle table for this chapter
//
//-------------------------------------------------------------------------
void
CImpIRowset::DeleteRows(ULONG row, int c, COSPData *pOSPData)
{
    ChRow hRef;
    CIndex v;
    HCHAPTER hChapter = HChapterFromOSPData(pOSPData);

    Assert(row >= 0 && c >= 0);
    // adjust Row2hRow mapping table
    pOSPData->_mapIndex2hRow.DeleteMultiple(row, c);

    // March through hRow array fixing up values
    for (hRef = FirsthRef(hChapter); ValidhRef(hRef);
         hRef = NexthRef(hChapter, hRef))
    {
        v = _maphRow2Index.GetElemNoCheck(hRef.DeRef());
        if (v.Row() >= (int)row)        // In the range affected by deletion?
        {
            if (v.Row() < (int)(row+c))  // In the range of now deleted rows?
            {
                // If this row has been deleted, set it to point to
                // first undeleted element, & mark it negative to show
                // that its been deleted.
                _maphRow2Index.SetElemNoCheck(hRef.DeRef(), v.SetRowDel(row));
            }
            else
            {
                // Adjust all values after deleted range.  Be sure to retain
                // any existing deleted flag.
                _maphRow2Index.SetElemNoCheck(hRef.DeRef(),
                                             v.SetRow(v.Row()-c, v.FDeleted()));
            }
        }

    }
}

#ifdef NEVER
//+------------------------------------------------------------------------
//
//  Member: CRowMap::InvalidateMap
//
//  Synopsis:   Invalidates the entire HRow to CRow mapping table.  Used
//              when the rowset has disappeared (or been sorted) out from
//              under us.  This will ensure that any and all outstanding
//              hRows, Bookmarks, etc. are invalid, can't be stored through
//              can't be used to fetch new data, etc.
//
//-------------------------------------------------------------------------
void
CImpIRowset::InvalidateMap()
{
    ChRow hRef;
    CIndex v;

    // March through hRow array deleting all values.
    // March through hRow array fixing up values
    for (hRef = FirsthRef(hChapter); ValidhRef(hRef);
         hRef = NexthRef(hRef, hChapter))
    {
        v = _maphRow2Index.GetElemNoCheck(hRef.DeRef());
        _maphRow2Index.SetElemNoCheck(hRef.DeRef(), v.SetRowDel(1));
    }
}
#endif
