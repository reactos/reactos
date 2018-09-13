//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       ltdata.cxx
//
//  Contents:   CTableLayout databinding methods.
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_LTROW_HXX_
#define X_LTROW_HXX_
#include "ltrow.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx" // CTreePosList in CTableLayout::Notify
#endif

#ifndef X_DETAIL_HXX_
#define X_DETAIL_HXX_
#include "detail.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

#ifndef X_SAVER_HXX_
#define X_SAVER_HXX_
#include "saver.hxx"
#endif

// DB support

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"  // DB stuff in Notify
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include <binder.hxx>
#endif

#ifndef X_ROWBIND_HXX_
#define X_ROWBIND_HXX_
#include <rowbind.hxx>
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

//+----------------------------------------------------------------------------------------------
//  Databind support methods
//-----------------------------------------------------------------------------------------------

#ifndef NO_DATABINDING
//+---------------------------------------------------------------------------
//
//  Member:     Populate
//
//  Synopsis:   Populate the table with repeated rows if the DataSrc property is
//              specified.
//
//  Retruns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::Populate()
{
    HRESULT                     hr;
    int                         cInsertNewRowAt;
    CDataLayerCursor            *pdlcCursor;

    // Don't generate or remove rows when a table is not in the tree
    if (!Table()->_fEnableDatabinding)
        return S_OK;

    hr = EnsureTableLayoutCache();
    if (hr)
        goto Cleanup;

    cInsertNewRowAt = GetHeadFootRows();

    Assert ((_fRefresh && _pDetailGenerator) || (!_fRefresh && !_pDetailGenerator));
    Assert(Table()->GetDBMembers());
    Assert(Table()->GetDBMembers()->GetBinder(ID_DBIND_DEFAULT));

    // get the DLCursor from the binder
    hr = Table()->GetDBMembers()->GetBinder(ID_DBIND_DEFAULT)->GetDLCursor(&pdlcCursor);
    if (hr)
        goto Cleanup;

    if (!_fRefresh)
    {
        _pDetailGenerator = new CDetailGenerator();    // expanded table
        if (!_pDetailGenerator)
            goto MemoryError;

        Doc()->_fBroadcastStop = TRUE;

        _pDetailGenerator->SetTemplateCount(GetRows() - cInsertNewRowAt);
    }

    hr = _pDetailGenerator->Init (pdlcCursor, Table(), cInsertNewRowAt,
                                  Table()->GetAAdataPageSize());
    if (hr)
        goto Error;

    if (!_fRefresh)
    {
        hr = _pDetailGenerator->PrepareForCloning();
        if (hr)
            goto Cleanup;

        // Remove the templates from _aryRows
        RemoveTemplate();

        if ( THR(EnsureTableLayoutCache()) )
            goto Error;

        Assert (GetRows() == cInsertNewRowAt);
    }
    hr = _pDetailGenerator->Generate ();

Cleanup:

    _fRefresh = FALSE;

    RRETURN(hr);

MemoryError:
    hr = E_OUTOFMEMORY;

Error:
    if (_pDetailGenerator)
    {
        _pDetailGenerator->Detach();
        delete _pDetailGenerator;
        _pDetailGenerator = NULL;
    }

    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     Refresh (regenerate)
//
//  Synopsis:   Populate the table with repeated rows when setting the new
//              RepeatSrc property.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------
HRESULT
CTableLayout::refresh()
{
    HRESULT hr = S_OK;

#ifndef NO_DATABINDING
    if (_pDetailGenerator)
    {
        hr = DeleteGeneratedRows();
        if (hr)
            goto Cleanup;

        hr = Populate();
        if (hr)
            goto Cleanup;
    }

Cleanup:
#endif

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:     GetTemplate
//
//  Synopsis:   create the template for repeating rows.
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::GetTemplate(BSTR * pbstr)
{
    HRESULT    hr = E_FAIL;
    int i;

    if (ElementOwner()->IsInMarkup())
    {
        IStream * pstm;

        hr = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
        if (hr)
            goto Cleanup;

        {
            CStreamWriteBuff swb(pstm, CP_UCS_2);

            swb.SetFlags(WBF_ENTITYREF|WBF_DATABIND_MODE);

            hr = EnsureTableLayoutCache();
            if (hr)
                goto Cleanup;

            // append each TBODY to the template
            for (i=0; i<_aryBodys.Size(); ++i)
            {
                CTreeSaver ts(_aryBodys[i], &swb);
                
                swb.SetElementContext(_aryBodys[i]);

                // if the body is synthesized, ts.Save won't write out its begin
                // and end tags.  But we want them in the template, so write
                // them out explicitly.  One reason we want them:  setting
                // borders=groups will write borders between each template
                // instance.
                _aryBodys[i]->WriteTag(&swb, FALSE, TRUE);
                ts.Save();
                _aryBodys[i]->WriteTag(&swb, TRUE, TRUE);
            }

            swb.Terminate();
        }

        hr = GetBStrFromStream(pstm, pbstr, TRUE);

        ReleaseInterface(pstm);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     DeleteGeneratedRows (private)
//
//  Synopsis:   Remove the generated rows, preparing for refresh
//
//  Retruns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CTableLayout::DeleteGeneratedRows()
{
    HRESULT hr = S_OK;

    if (_pDetailGenerator)
    {
        _fRefresh = TRUE;

        // delete all the generated rows

        _pDetailGenerator->ReleaseGeneratedRows();   // need to delete Xfer thunks, before deleting rows

        hr = EnsureTableLayoutCache();              // fixing bug 74288
        //if (hr)
        //    goto Cleanup;

        RemoveTemplate();
    }

    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     RemoveRowsAndTheirSections
//
//  Synopsis:   Removes all rows in the given range and their sectionsw.
//              Can't remove header/footer.
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::RemoveRowsAndTheirSections ( int iRowStart, int iRowFinish )
{
    // Don't generate or remove rows when a table is not in the tree
    if (!Table()->_fEnableDatabinding)
        return S_OK;
    {
        HRESULT         hr;
        CTableSection * pSection = NULL;
        int             iBodyStart = 0;
        int             iBodyFinish = 0;
        int             i;

        // This function should only be called to remove rows that make up one
        // or more complete template instances.  In particular, it shouldn't be
        // called for tables that aren't databound.  A general RemoveRows would
        // have to deal with row spans and other issues that don't arise in
        // databound tables.
        
        Assert(IsRepeating());
        
        //
        // Are there any rows to remove?
        //
        
        Assert(GetHeadFootRows() <= iRowStart  &&  iRowFinish < GetRows());
        
        if (iRowStart > iRowFinish)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }

        //
        // Delete the rows by deleting spans of consecutive runs.
        // The runs owned by the given rows may be non-consecutive, so be careful.
        // We delete the rows back-to-front, because each call to RemoveRuns
        // blows away and rebuilds _aryRows (via monster walk).
        //
        
        for (i=0; i < _aryBodys.Size(); i++)
        {
            pSection = _aryBodys[i];
            if (iRowStart >= pSection->_iRow && iRowStart < pSection->_iRow + pSection->_cRows)
            {
                iBodyStart = i;
            }
            if (iRowFinish >= pSection->_iRow && iRowFinish < pSection->_iRow + pSection->_cRows)
            {
                iBodyFinish = i;
                break;
            }
        }

        Assert (iBodyFinish >= iBodyStart);

        hr = RemoveBodys(iBodyStart, iBodyFinish);

    Cleanup:
        RRETURN( hr );
    }
}

//+----------------------------------------------------------------------------
//
//  Member:     RemoveBodys
//
//  Synopsis:   Removes all bodys in the given range.  Can't remove header/footer.
//
//-----------------------------------------------------------------------------

HRESULT
CTableLayout::RemoveBodys (int iBodyStart, int iBodyFinish)
{
    HRESULT         hr = S_OK;

    // Don't generate or remove rows when a table is not in the tree
    if (!Table()->_fEnableDatabinding)
        goto Cleanup;

    {
        CMarkupPointer pStart( Doc() ), pEnd( Doc() );
        
        BOOL            fRemoveAll;
        int             iBodyFinishNext;

        _fRemovingRows = TRUE;

        Verify(OpenView());

        // This function should only be called to remove bodys that make up one
        // or more complete template instances.  In particular, it shouldn't be
        // called for tables that aren't databound.  A general RemoveBodys would
        // have to deal with row spans and other issues that don't arise in
        // databound tables.
        Assert(IsRepeating());
        
        //
        // Are there any bodys to remove?
        //
        Assert(0 <= iBodyStart  &&  iBodyFinish < _aryBodys.Size());
        if (iBodyStart > iBodyFinish)
        {
            hr = E_INVALIDARG;
            goto Cleanup;
        }
        
        fRemoveAll = iBodyStart == 0 && iBodyFinish == GetBodys() - 1;

        //
        // Delete the bodys by deleting spans of consecutive runs.
        // The bodys may be non-consecutive, so be careful.
        // We delete the bodys back-to-front.
        //
        
        iBodyFinishNext = iBodyFinish;
        while (iBodyStart <= iBodyFinishNext)
        {
            int iBodyFinishDelete = iBodyFinishNext;

            // set gaps to surround the last body

            hr = THR( pStart.MoveAdjacentToElement( _aryBodys[iBodyFinishNext], ELEM_ADJ_BeforeBegin ) );

            if (hr)
                goto Cleanup;
            
            hr = THR( pEnd.MoveAdjacentToElement( _aryBodys[iBodyFinishNext], ELEM_ADJ_AfterEnd ) );

            if (hr)
                goto Cleanup;
            
            // augment the span as long as the bodys are consecutive
            while (iBodyStart <= --iBodyFinishNext)
            {
                CMarkupPointer pNextEnd ( Doc() );

                hr = THR( pNextEnd.MoveAdjacentToElement( _aryBodys[iBodyFinishNext], ELEM_ADJ_AfterEnd ) );

                if (hr)
                    goto Cleanup;
                
                if (pNextEnd.IsEqualTo( & pStart ))
                {
                    hr = THR( pStart.MoveAdjacentToElement( _aryBodys[iBodyFinishNext], ELEM_ADJ_BeforeBegin ) );

                    if (hr)
                        goto Cleanup;
                }
                else
                {
                    break;
                }
            }

            // This is one of the few places where a table is modified.
            // Mark the TLC dirty now.
            Table()->InvalidateCollections();

            if (fRemoveAll && iBodyFinishNext<iBodyStart && iBodyFinishDelete==iBodyFinish)
            {
                ReleaseRowsAndSections(FALSE, TRUE);  // fReleaseHeaderFooter = FALSE, fReleaseTCs = TRUE
            }
            else
            {
                ReleaseBodysAndTheirRows(iBodyFinishNext + 1, iBodyFinishDelete);
            }

            // remove the consecutive bodys
            hr = THR( Doc()->Remove( & pStart, & pEnd ) );

            if (hr)
                goto Cleanup;

            _fRemovingRows = FALSE;

        }

        Resize();
    }

Cleanup:
    _fRemovingRows = FALSE;     // in case of goto (error condition) we want to restore the flag
    RRETURN( hr );
}


//+----------------------------------------------------------------------------
//
//  Member:     RowIndex2RecordNumber
//
//  Synopsis:   find the database position of a given row
//
//  Arguments:  iRow        index of desired row
//
//  Returns:    index of corresponding database record
//
//-----------------------------------------------------------------------------

long
CTableLayout::RowIndex2RecordNumber(int iRow)
{
    Assert(IsRepeating());
    return _pDetailGenerator->RowIndex2RecordNumber(iRow);
}
#endif // NO_DATABINDING


