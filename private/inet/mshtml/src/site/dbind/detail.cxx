//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       detail.cxx
//
//  Contents:   HTML Tables Repeating Extensions
//
//  History:
//
//  Jul-96      AlexA   Creation
//  8/14/96     SamBent Support record generator as a lightweight task
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_CSITE_HXX_
#define X_CSITE_HXX_
#include <csite.hxx>
#endif

#ifndef X__TXTSAVE_H_
#define X__TXTSAVE_H_
#include <_txtsave.h>
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_OLEDBERR_H_
#define X_OLEDBERR_H_
#include <oledberr.h>       // for DB_S_UNWANTEDREASON
#endif

#ifndef X_DETAIL_HXX_
#define X_DETAIL_HXX_
#include "detail.hxx"
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_DRECGEN_HXX_
#define X_DRECGEN_HXX_
#include "drecgen.hxx"
#endif

#ifndef X_ROWBIND_HXX_
#define X_ROWBIND_HXX_
#include "rowbind.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#if defined(PRODUCT_PROF)
//#  define PROFILE_REPEATED_TABLES
#endif

#ifdef PROFILE_REPEATED_TABLES
#include <icapexp.h>
#endif

DeclareTag(tagDetail,"src\\site\\builtin\\detail.cxx","Detail");
DeclareTag(tagTableProgress, "Databinding", "Trace table progress");
PerfDbgTag(tagTableExpand,  "Databinding", "Trace table expansion");
PerfDbgTag(tagRecRequest,   "Databinding", "Trace record requests");

MtDefine(CDetailGenerator, DataBind, "CDetailGenerator")
MtDefine(CDetailGenerator_aryRecInstance_pv, CDetailGenerator, "CDetailGenerator::_aryRecInstance::_pv");

const LARGE_INTEGER LINULL = {0, 0};    // for IStream->Seek()
const RECORDINDEX  RECORDINDEX_INVALID  = MAXLONG;


//+------------------------------------------------------------------------
//
//  class       CDetailGenerator
//
//  Member:     constructor
//
//-------------------------------------------------------------------------

CDetailGenerator::CDetailGenerator() :
      _aryRecInstance(Mt(CDetailGenerator_aryRecInstance_pv))
{
    TraceTag((tagDetail, "CDetailGenerator::constructor() -> %p", this));

    _reqCurrent._riBot = -1;            // prepare for NewRecord on an empty table
}




//+------------------------------------------------------------------------
//
//  class       CDetailGenerator
//
//  Member:     destructor
//
//-------------------------------------------------------------------------

CDetailGenerator::~CDetailGenerator ()
{
    TraceTag((tagDetail, "CDetailGenerator::destructor() -> %p", this));

    Assert (!_pRecordGenerator);
    Assert (!_bstrHtmlText);
}


//+------------------------------------------------------------------------
//
//  Member:     Detach
//
//  Synopsis:   Release all my resources.
//
//-------------------------------------------------------------------------

void
CDetailGenerator::Detach()
{
    TraceTag((tagDetail, "CDetailGenerator::Detach() -> %p", this));

    CancelRequest();
    
    ReleaseGeneratedRows();

    // stop my hookup task
    if (_pDGTask)
    {
        _pDGTask->Release();
        _pDGTask = NULL;
    }

    ReleaseProgress();
    
    // Detach from the cursor
    if (_pDLC)
    {
        _pDLC->SetDLCEvents(0);
        _pDLC->Release();
        _pDLC = NULL;
    }

    // Release the template (both text and elements)
    FormsFreeString(_bstrHtmlText);
    _bstrHtmlText = NULL;

    // Release dataSrc and dataFld strings
    FormsFreeString(_bstrDataSrc);
    FormsFreeString(_bstrDataFld);
    _bstrDataSrc = _bstrDataFld = NULL;
}


//+------------------------------------------------------------------------
//
//  Member:     ReleaseGeneratedRows
//
//  Synopsis:   Release resources for generated rows
//
//-------------------------------------------------------------------------

void
CDetailGenerator::ReleaseGeneratedRows()
{
    CancelRequest();

    // Release the RecordInstances
    for (int i = _aryRecInstance.Size() - 1; i >= 0; i--)
    {
        _aryRecInstance[i]->Detach();
        delete _aryRecInstance[i];
    }
    _aryRecInstance.DeleteAll();

    // Release the record generator
    if (_pRecordGenerator)
    {
        _pRecordGenerator->Detach();
        delete _pRecordGenerator;
        _pRecordGenerator = NULL;
    }
}


//+------------------------------------------------------------------------
//
//  Member:     ReleaseRecords
//
//  Synopsis:   Release resources for generated rows
//
//-------------------------------------------------------------------------

void
CDetailGenerator::ReleaseRecords(RECORDINDEX riFirst, RECORDINDEX riLast)
{
    if (riFirst <= riLast)
    {
        // release the rows
        if (_pTable->_fEnableDatabinding)
        {
            CTableLayout   *pTableLayout = _pTable->Layout();
            int             iRowStart = RecordIndex2RowIndex(riFirst);
            int             iRowFinish = RecordIndex2RowIndex(riLast + 1) - 1;
            pTableLayout->RemoveRowsAndTheirSections(iRowStart, iRowFinish);
        }
        // release the record instances
        for (int i=riLast; i>=riFirst; --i)
        {
            _aryRecInstance[i]->Detach();
            delete _aryRecInstance[i];
            _aryRecInstance.Delete(i);
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     Init
//
//  Synopsis:   Initialization
//
//  Arguments:  [pdlc]       -- pointer to the data layer cursor
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::Init (CDataLayerCursor         *pDLC,
                        CTable                   *pTable,
                        unsigned int              cInsertNewRowAt,
                        int                       cPageSize)
{
    TraceTag((tagDetail, "CDetailGenerator::Init() -> %p", this));

    HRESULT hr;

    Assert (pTable);
    _pTable = pTable;
    _iFirstGeneratedRow = cInsertNewRowAt;
    _cPageSize = cPageSize;

    // cache my table's provider
    _pProvider = _pTable->GetProvider();

    // get the full dataSrc and dataFld
    _pTable->GetDBMembers()->GetBinder(ID_DBIND_DEFAULT)->
                GetDataSrcAndFld(&_bstrDataSrc, &_bstrDataFld);
    
    // attach to the cursor
    AssertSz(pDLC, "need a valid cursor");
    _pDLC = pDLC;
    _pDLC->AddRef();
    _pDLC->SetDLCEvents(this);

    // set up my hookup task
    _pDGTask = new CDGTask(this);
    if (!_pDGTask)
        goto MemoryError;

    // Create a record generator for assync CElements population.
    _pRecordGenerator = new CRecordGenerator ();
    if (!_pRecordGenerator)
        goto MemoryError;
    hr = _pRecordGenerator->Init(_pDLC, this);
    if (hr)
        goto Error;

    // this silly-looking code sets my state so that I can survive
    // a call to nextPage before I even start generating records
    _reqCurrent.Activate(CDataLayerBookmark::TheFirst, 0, 0, 0);
    _reqCurrent.Deactivate();

Cleanup:
    RRETURN (hr);

MemoryError:
    hr = E_OUTOFMEMORY;
Error:
    Detach();
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     IsFieldKnown
//
//  Synopsis:   Determine whether the first component of the given
//              field describes a column in my datasource
//
//  Arguments:  strField    field path (dot-separated components)
//
//  Returns:    TRUE/FALSE
//
//-------------------------------------------------------------------------

BOOL
CDetailGenerator::IsFieldKnown(LPCTSTR strField)
{
    BSTR bstrHead, bstrTail;
    BOOL bResult;
    DBORDINAL ulOrdinal;

    if (!FormsIsEmptyString(strField))
    {
        FormsSplitAtDelimiter(strField, &bstrHead, &bstrTail);
        bResult = (S_OK == _pDLC->GetColumnNumberFromName(bstrHead, ulOrdinal));

        FormsFreeString(bstrHead);
        FormsFreeString(bstrTail);
    }
    else
    {
        bResult = FALSE;
    }

    return bResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     AddDataboundElement
//
//  Synopsis:   An element has entered the tree that wants to bind to the
//              record that generated one of my rows.  Post a request
//              to hook it up to the data.
//
//  Arguments:  pElement        the newly arrived element
//              id              id of binding
//              pRowContaining  the row governing pElement
//              strField        the element's dataFld (or a portion thereof)
//                              describing which column to bind
//
//----------------------------------------------------------------------------

HRESULT
CDetailGenerator::AddDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining,
                                LPCTSTR strField)
{
    HRESULT hr = S_OK;

    // if we're restoring the original template, ignore the request
    if (_fRestoringTemplate)
        goto Cleanup;
    
    // if pElement is a table, create a binder and do it the regular way
    if (pElement->Tag() == ETAG_TABLE)
    {
        hr = pElement->CreateDatabindRequest(id);
    }

    // otherwise add it to the DGTask's list - we'll hook it up later
    else
    {
        hr = _pDGTask->AddDataboundElement(pElement, id, pRowContaining, strField);
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     HookUpDataboundElement
//
//  Synopsis:   Hook up an element to the data.
//
//  Arguments:  pElement        the newly arrived element
//              id              id of binding
//              pRowContaining  the row governing pElement
//              strField        the element's dataFld (or a portion thereof)
//                              describing which column to bind
//
//----------------------------------------------------------------------------

HRESULT
CDetailGenerator::HookUpDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining,
                                LPCTSTR strField)
{
    HRESULT hr = E_FAIL;
    CRecordInstance *pInstance;

    // check that the element, row, and table still have the right
    // relationship and are still in the tree
    if (pElement->IsInPrimaryMarkup() && pRowContaining->IsInPrimaryMarkup())
    {
        CTreeNode *pNodeElt = pElement->GetFirstBranch();
        AssertSz(pNodeElt, "IsInPrimaryMarkup lied!");
        CTreeNode *pNodeRow = pNodeElt->SearchBranchToRootForScope(pRowContaining);
        
        if (pNodeRow)
        {
            CTreeNode *pNodeTable = pNodeRow->Ancestor(ETAG_TABLE);
            
            if (pNodeTable && pNodeTable->Element() == _pTable)
            {
                hr = S_OK;
            }
        }
    }
    if (hr)
        goto Cleanup;

    hr = _pTable->GetInstanceForRow(pRowContaining, &pInstance);
    if (hr)
        goto Cleanup;
    
    hr = CXfer::CreateBinding(pElement, id, strField, _pProvider, pInstance);
    if (SUCCEEDED(hr))
        hr = S_OK;      // S_FALSE just means object/applet wasn't ready yet

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     OnMetaDataAvail
//
//  Synopsis:   Call back from the record generator
//              (the meta data has arived/ Cursor is ready).
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::OnMetaDataAvailable()
{
    TraceTag((tagDetail, "CDetailGenerator::OnMetaDataAvail() -> %p", this));

    HRESULT             hr = S_OK;
    RRETURN(hr);
}




//+------------------------------------------------------------------------
//
//  Member:     OnRecordsAvailable
//
//  Synopsis:   Call back from the record generator
//              (new records are available).
//
//  Argumnets:  [pTask]     -- task pointer
//              [cRecords]  -- number of records available
//
//  Returns:    HRESULT (S_FALSE is permited, means that we have read
//              everything that was available);
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::OnRecordsAvailable(ULONG cRecords)
{
    TraceTag((tagDetail, "CDetailGenerator::OnRecordsAvailable() -> %p %ul",
                        this, cRecords));
    PerfDbgLog1(tagRecRequest, this, "got %lu records", cRecords);
    PerfDbgLog1(tagTableExpand, GetHost(), "+OnRecordsAvailable %lu", cRecords);

    HRESULT             hr;
    ULONG               cFetched;
    ULONG               cDetails = 0;
    HROW                ahRows[cBufferCapacity];
    LONG                cNewRows;
    ULONG               cRecordsSupplied = cRecords;
    RECORDINDEX         riFirst;

    Assert(cRecords <= cBufferCapacity);

    // determine how many templates we need to create
    if (_reqCurrent.IsForward())
    {
        riFirst = FirstAvailableIndex();
        cNewRows = riFirst + cRecords - RecordCount();
    }
    else
    {
        riFirst = (GetCurrentIndex()==RECORDINDEX_INVALID) ? -1 : GetCurrentIndex();
        cNewRows = cRecords - (riFirst + 1);
    }

    // ignore records that exceed the page size
    // (this can happen if Inserts occur during the request)
    if (_cPageSize > 0 && RecordCount()+cNewRows > _cPageSize)
    {
        LONG cNewRowsAmended = (RecordCount() < _cPageSize) ? _cPageSize - RecordCount() : 0;
        cRecords -= (cNewRows - cNewRowsAmended);
        cNewRows = cNewRowsAmended;
    }

    // get the records from the generator
    hr = _pRecordGenerator->FetchRecords(cRecords, ahRows, &cFetched);
    Assert(cFetched == cRecords);

    // paste a bunch of rows into the table, and update current position
    if (_reqCurrent.IsForward())
    {
        if (cNewRows > 0)
        {
            hr = Clone(cNewRows);
            if (hr)
                goto Cleanup;
            SetCurrentIndex( riFirst );
        }
    }
    else
    {
        if (cNewRows > 0)
        {
            hr = Clone(cNewRows, 0);
            if (hr)
                goto Cleanup;
            SetCurrentIndex( riFirst + cNewRows );
        }
    }

    // build detail objects from the records
    for (cDetails=0; cDetails<cRecords; ++cDetails)
    {
        RECORDINDEX riCurrent = GetCurrentIndex();

        hr = CreateDetail(ahRows[cDetails], &riCurrent);
        if (hr)
            goto Cleanup;

        SetCurrentIndex(AdvanceRecordIndex(riCurrent));

        _reqCurrent._fGotRecords = TRUE;
    }

Cleanup:
    // advance the progress item
    if (_dwProgressCookie)
    {
        IProgSink *pProgSink = _pTable->Doc()->GetProgSink();

        _reqCurrent._cRecordsRetrieved += cRecordsSupplied;
        pProgSink->SetProgress(_dwProgressCookie, PROGSINK_SET_POS, 0, NULL, 0,
                                      _reqCurrent._cRecordsRetrieved, 0);
    }
    
    // release the records
    _pRecordGenerator->ReleaseRecords(cRecordsSupplied);

    PerfDbgLog1(tagTableExpand, GetHost(), "-OnRecordsAvailable %lu", cRecords);
    RRETURN(hr);
}


HRESULT
CDetailGenerator::OnRequestDone(BOOL fEndOfData)
{
    HRESULT hr = S_OK;

    if (_cPageSize > 0 && fEndOfData)
    {
        // if we paged off the end of the data, delete any extra templates
        if (_reqCurrent.GotRecords())
        {
            RECORDINDEX riCurrent = GetCurrentIndex();

            if (riCurrent != RECORDINDEX_INVALID)
            {
                if (_reqCurrent.IsForward())
                {
                    // we fell of the bottom.  Delete rows at the end
                    ReleaseRecords(riCurrent, RecordCount() - 1);
                }
                else
                {
                    // we fell off the top.  Delete rows at the beginning
                    ReleaseRecords(0, riCurrent);

                    // make sure we're displaying dataPageSize rows
                    if (RecordCount() < _cPageSize)
                    {
                        SetBookmarks();
                        MakeRequest(_reqCurrent._dlbBot, 1,
                                    _cPageSize - RecordCount(), RecordCount());
                        goto Cleanup;
                    }
                }
            }
        }

        // if several next/prev page requests combined to overshoot the end of
        // the data, back up and try again
        else    // !_reqCurrent.GotRecords()
        {
            if (_reqCurrent.IsForward() && _reqCurrent._lOffset > _cPageSize)
            {
                MakeRequest(_reqCurrent._dlbStart,
                            _reqCurrent._lOffset - _cPageSize,
                            _cPageSize, 0);
                goto Cleanup;
            }
            if (!_reqCurrent.IsForward() && _reqCurrent._lOffset < -_cPageSize)
            {
                MakeRequest(_reqCurrent._dlbStart,
                            _reqCurrent._lOffset + _cPageSize,
                            -_cPageSize, RecordCount()-1);
                goto Cleanup;
            }
        }
    }

    // also hook up scripts to the records we got
    // bugbug (mwagner) doing this here adds more N**2 ish behavior to databinding
    GetHost()->Doc()->CommitScripts();

    // remember bookmarks for top and bottom rows
    EndRequest();
    // WARNING: EndRequest() changes table.readyState, which may call scripts
    // that change the world - e.g. removing table.dataSrc.  So "this" may
    // not even exist after this point.

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     GetHrow
//
//  Synopsis:   return the HROW associated with the given index.
//              return DB_NULL_HROW if index out of bounds

HROW
CDetailGenerator::GetHrow(RECORDINDEX ri)
{
    return (ri<0 || ri>=RecordCount())  ? DB_NULL_HROW
                                        : _aryRecInstance[ri]->GetHRow();
}


//+------------------------------------------------------------------------
//
//  Member:     RecordIndex2RecordNumber
//
//  Synopsis:   Compute the record number (offset from beginning of dataset)
//              for the record with the given index
//
//  Arguments:  ri      index of desired record
//
//  Returns:    index of corresponding record in database
//              -1      if record not found or error

long
CDetailGenerator::RecordIndex2RecordNumber(RECORDINDEX ri)
{
    HRESULT hr = E_FAIL;
    DBCOUNTITEM ulPosition = (DBCOUNTITEM)-1;
    DBCOUNTITEM ulSize;
    HROW hrow;

    // get the corresponding hrow
    hrow = GetHrow(ri);
    if (hrow == DB_NULL_HROW)
        goto Cleanup;

    // find its position in the dataset
    hr = _pDLC->GetPositionAndSize(hrow, &ulPosition, &ulSize);
    if (hr)
        goto Cleanup;

Cleanup:
    return hr ? -1 : (LONG)ulPosition;
}


//+------------------------------------------------------------------------
//
//  Member:     AssignHrowToRecord
//
//  Synopsis:   Send the given HROW to the record at the given index
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::AssignHrowToRecord(HROW hrow, RECORDINDEX ri)
{
    HRESULT hr;
    CRecordInstance *pRecInstance;

    Assert(0<=ri && ri<RecordCount());

    pRecInstance = _aryRecInstance[ri];
    hr = pRecInstance->SetHRow(hrow);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     NewRecord
//
//  Synopsis:   Called from CTabularDataAgent upon arrival of a new record
//              in the Rowset.  For now, we always assume this new record
//              arrived at the end of the rowset.
//
//  Arguments:  none
//
//  Returns:    S_OK
//
//-------------------------------------------------------------------------
HRESULT
CDetailGenerator::NewRecord()
{
    HRESULT hr = S_OK;

    // If a request is active, it'll pick up the new record in due course.
    if (_reqCurrent.IsActive())
        goto Cleanup;

    // We only care if we're either displaying the whole table, or if the
    // last page we displayed on the table was not complete
    if (_cPageSize==0 || _reqCurrent._riBot+1 < _cPageSize)
    {
        // request more records
        RECORDINDEX riStart = _reqCurrent._riBot + 1;
        CDataLayerBookmark dlbStart = (riStart<=0)? CDataLayerBookmark::TheFirst
                                                    : _reqCurrent._dlbBot;
        LONG lOffset = (riStart<=0)? 0 : 1;
        LONG cRecords = _cPageSize==0 ? MAXLONG : _cPageSize - riStart;

        hr = MakeRequest(dlbStart, lOffset, cRecords, riStart);
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CreateDetail
//
//  Synopsis:   Create an instance of an repeated elements from their templates,
//              1. Clone the template (or reuse the instance)
//              2. RefreshData for the Elements instance
//              3. add the detail (Elements) to the host.
//
//  Arguments:  [hrow]      -- HROW to generate detail for.
//              pRECORDINDEX  -- [in] address of RECORDINDEX to re-use
//                    [out] if not null, pointer to the RECORDINDEX used
//
//  Returns:    S_OK, if the detail was careted
//              S_FALSE if reached end of Element set
//              otherwise returns an error
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::CreateDetail (HROW hrow, RECORDINDEX  *pri)
{
    TraceTag((tagDetail, "CDetailGenerator::CreateDetail() -> %p (%ul)", this, hrow));

    HRESULT             hr;
    RECORDINDEX         ri = RECORDINDEX_INVALID;

    if (pri && *pri!=RECORDINDEX_INVALID)
    {
        ri = *pri;
    }
    else
    {
        // We create the record with HROW 0, then change the HROW afterward.  This
        //  prevents a bug in which we manipulate the element tree while we walk it
        hr = CreateRecordInstance(&ri);

        if (FAILED(hr))
        {
            Assert(ri == RECORDINDEX_INVALID);
            goto Error;
        }

        // we got a new record index
        Assert (ri != RECORDINDEX_INVALID);
        Assert (ri < RecordCount());
    }

    {
        // actually transfer the values.  Turn off table recalc during transfer.
        CElement::CLock Lock(GetHost(), CElement::ELEMENTLOCK_BLOCKCALC);
        PerfDbgLog1(tagTableExpand, GetHost(), "+ChangeHRow %x", hrow);
        hr = AssignHrowToRecord(hrow, ri);
        PerfDbgLog1(tagTableExpand, GetHost(), "-ChangeHRow %x", hrow);
    }
    if (hr)
        goto Error;

Cleanup:
    if (pri)
    {
        *pri = ri;
    }
    RRETURN (hr);

Error:
    DeleteDetail(ri);
    ri = RECORDINDEX_INVALID;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     CreateRecordInstance
//
//  Synopsis:   Create a record instance and return index where it's stored
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------
HRESULT
CDetailGenerator::CreateRecordInstance (RECORDINDEX *pri)
{
    HRESULT             hr = S_OK;
    CRecordInstance    *pRecInstance;
    RECORDINDEX         ri = RECORDINDEX_INVALID;

    pRecInstance = new CRecordInstance(_pDLC, DB_NULL_HROW);
    if (!pRecInstance)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // add the new instance to my array, either at the indicated position or
    // at the end
    if (pri && *pri != RECORDINDEX_INVALID)
    {
        ri = *pri;
        hr = _aryRecInstance.Insert(ri, pRecInstance);
    }
    else
    {
        ri = RecordCount();
        hr = _aryRecInstance.Append(pRecInstance);
    }
    if (hr)
        goto Error;

Cleanup:
    if (pri)
        *pri = hr ? RECORDINDEX_INVALID : ri;
    RRETURN(hr);

Error:
    pRecInstance->Detach();
    delete pRecInstance;
    goto Cleanup;
}


//+------------------------------------------------------------------------
//
//  Member:     PrepareForCloning
//
//  Synopsis:   prepare the repeated Elements of the table to Clone.
//              (Save the Element templates into a stream).
//              create a parser
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::PrepareForCloning()
{
    TraceTag((tagDetail, "CDetailGenerator::PrepareForCloning() -> %p", this));

    HRESULT hr = S_OK;

#ifndef NO_DATABINDING
    if (!_bstrHtmlText)     // else it is a refresh case
    {
        //
        // Save the table without header and footer to a null-terminated
        // stream.  Save only inside the runs of the <TABLE>.
        //
        hr = _pTable->Layout()->GetTemplate(&_bstrHtmlText);
    }
#endif

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     Clone
//
//  Synopsis:   Clone the detail (template)..
//              Generate all the Elements from the template Elements.
//
//  Arguments:  nTemplates      number of templates to clone
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::Clone(UINT nTemplates, RECORDINDEX riClone)
{
    TraceTag((tagDetail, "CDetailGenerator::Clone() -> %p", this));

    HRESULT hr;
    UINT i;

    Verify(_pTable->Layout()->OpenView());

    // add the new rows
    hr = AddTemplates(nTemplates, riClone);

    // update my array of instances
    for (i=0; i<nTemplates; ++i)
    {
        RECORDINDEX ri = riClone;
        hr = CreateRecordInstance(&ri);
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     AddTemplates
//
//  Synopsis:   add template rows to the table.  Don't do anything more -
//              RestoreTemplate relies on this.
//
//  Arguments:  nTemplates      number of templates to clone
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::AddTemplates(UINT nTemplates, RECORDINDEX ri)
{
    HRESULT         hr = S_OK;
    if (_pTable->_fEnableDatabinding)
    {
        BSTR    bstrTemplates = NULL;
        OLECHAR *pStr;
        UINT i;
        int iRow;
        CTableSection *pSectionInsertBefore = NULL;
        CTableLayout * pTableLayout = _pTable->Layout();
        static const OLECHAR * oszPreamble = _T("<TABLE>");
        static const OLECHAR * oszPostamble = _T("</TABLE>");
        const UINT cTemplateLength = FormsStringLen(_bstrHtmlText);
        const UINT cLength = wcslen(oszPostamble) +
                            nTemplates * cTemplateLength +
                            wcslen(oszPostamble) + 1;

        // allocate the string and prepare to copy
        hr = FormsAllocStringLen(NULL, cLength, &bstrTemplates);
        if (hr)
            goto Cleanup;
        pStr = bstrTemplates;

        // copy the preamble
        wcscpy(pStr, oszPreamble);
        pStr += wcslen(oszPreamble);

        // copy the template body the requested number of times
        for (i=0; i<nTemplates; ++i)
        {
            if (cTemplateLength > 0)
                wcscpy(pStr, _bstrHtmlText);
            pStr += cTemplateLength;
        }

        // copy the postamble
        wcscpy(pStr, oszPostamble);

        // locate where to paste in the cloned rows

        hr = pTableLayout->EnsureTableLayoutCache();
        if (hr)
            goto Cleanup;

        iRow = ri==RECORDINDEX_INVALID ? -1 : RecordIndex2RowIndex(ri);
        if (0 <= iRow && iRow < pTableLayout->GetRows())
        {
            pSectionInsertBefore = pTableLayout->_aryRows[iRow]->Section();
        }

        // paste the result into the main table
        PerfDbgLog1(tagTableExpand, GetHost(), "+PasteRows %u", nTemplates);
        hr = PasteRows(bstrTemplates, pSectionInsertBefore);
        PerfDbgLog1(tagTableExpand, GetHost(), "-PasteRows %u", nTemplates);
        if (hr)
            goto Cleanup;

    Cleanup:
        // deallocate the string
        FormsFreeString(bstrTemplates);
    }

    RRETURN (hr);
}


//+------------------------------------------------------------------------
//
//  Member:     PasteRows
//
//  Synopsis:   paste generated rows into the tree
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::PasteRows(BSTR bstrTemplates, CTableSection *pSectionInsertBefore)
{
    HRESULT hr;
    BOOL fSaveDataBindingEnabled;
    IHTMLElement *pElementTarget = NULL;
    IMarkupContainer *pMC = NULL;
    IMarkupPointer *pmpStart = NULL, *pmpFinish = NULL, *pmpTarget = NULL;
    CDoc *pDoc = _pTable->Doc();
    CTableLayout *pTableLayout = _pTable->Layout();
    IHTMLElement *pElementCurrent = NULL;
    MARKUP_CONTEXT_TYPE context = CONTEXT_TYPE_None;
    BSTR bstrTagName = NULL;
    Assert(pDoc);

    // memo to world:  we're pasting rows, don't interfere
    fSaveDataBindingEnabled = pDoc->SetDataBindingEnabled(FALSE);
    pTableLayout->PastingRows(TRUE, pSectionInsertBefore);
    
    // locate the target
    hr = pDoc->CreateMarkupPointer(&pmpTarget);
    if (pSectionInsertBefore == NULL)
    {
        if (!hr)
            hr = _pTable->QueryInterface(IID_IHTMLElement, (void**)&pElementTarget);
        if (!hr)
            hr = pmpTarget->MoveAdjacentToElement(pElementTarget, ELEM_ADJ_BeforeEnd);
    }
    else
    {
        if (!hr)
            hr = pSectionInsertBefore->QueryInterface(IID_IHTMLElement, (void**)&pElementTarget);
        if (!hr)
            hr = pmpTarget->MoveAdjacentToElement(pElementTarget, ELEM_ADJ_BeforeBegin);
    }
    
    // create the source, and narrow the range to include just the new TBODY
    hr = THR( pDoc->CreateMarkupPointer( & pmpStart ) );
    if (hr)
        goto Cleanup;
    hr = THR( pDoc->CreateMarkupPointer( & pmpFinish ) );
    if (hr)
        goto Cleanup;
    if (!hr)
        pDoc->ParseString(bstrTemplates, PARSE_ABSOLUTIFYIE40URLS,
                            &pMC, pmpStart, pmpFinish);
    while (!hr && !(context==CONTEXT_TYPE_EnterScope && 0==FormsStringCmp(bstrTagName, _T("TABLE"))))
    {
        hr = pmpStart->Right(TRUE, &context, &pElementCurrent, NULL, NULL);
        FormsFreeString(bstrTagName);
        bstrTagName = NULL;
        if (!hr)
        {
            hr = pElementCurrent->get_tagName(&bstrTagName);
            ReleaseInterface(pElementCurrent);
        }
    }
    context = CONTEXT_TYPE_None;
    while (!hr && !(context==CONTEXT_TYPE_EnterScope && 0==FormsStringCmp(bstrTagName, _T("TABLE"))))
    {
        hr = pmpFinish->Left(TRUE, &context, &pElementCurrent, NULL, NULL);
        FormsFreeString(bstrTagName);
        bstrTagName = NULL;
        if (!hr)
        {
            hr = pElementCurrent->get_tagName(&bstrTagName);
            ReleaseInterface(pElementCurrent);
        }
    }

    // move the new TBODY into the main tree
    if (!hr)
        hr = pDoc->Move(pmpStart, pmpFinish, pmpTarget);

    // memo to world:  we're done now
    pTableLayout->PastingRows(FALSE);
    pDoc->SetDataBindingEnabled(fSaveDataBindingEnabled);

Cleanup:
    // Databinding should not save undo information
    pDoc->FlushUndoData();
    
    // cleanup
    ReleaseInterface(pMC);
    ReleaseInterface(pmpStart);
    ReleaseInterface(pmpFinish);
    ReleaseInterface(pmpTarget);
    ReleaseInterface(pElementTarget);
    FormsFreeString(bstrTagName);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     Generate
//
//  Synopsis:   populate the host (table/repeater) with the Elements
//              Generate all the Elements from the template Elements.
//              (is called when populating 1st time).
//
//  Returns:    HRESULT, (S_FALSE means the end of Element set was reached).
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::Generate()
{
    TraceTag((tagDetail, "CDetailGenerator::Generate() -> %p", this));

    Assert(_pRecordGenerator);
    Assert(_aryRecInstance.Size() == 0);            // No record instances yet.

    HRESULT hr;

    hr = _pRecordGenerator->RequestMetaData();
    if (hr)
        goto Cleanup;

    if (_cPageSize<0)                   // Clip negatives.
        _cPageSize = 0;

    // Request to generate all the Elements
    hr = MakeRequest(CDataLayerBookmark::TheFirst, 0,
                        _cPageSize ? _cPageSize : MAXLONG, 0);

Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     RestoreTemplate
//
//  Synopsis:   Put the template back into the table.  Called when dataSrc
//              changes, and we need to go back to the original state.
//
//-------------------------------------------------------------------------

HRESULT
CDetailGenerator::RestoreTemplate()
{
    HRESULT hr;
    BOOL fRestoringTemplateSave = _fRestoringTemplate;

    // this works because AddTemplates just pastes the template into the
    // table.
    _fRestoringTemplate = TRUE;
    hr = AddTemplates(1);
    _fRestoringTemplate = fRestoringTemplateSave;

    RRETURN(hr);
}


HRESULT
CDetailGenerator::nextPage()
{
    TraceTag((tagDetail, "CDetailGenerator::next() -> %p", this));

    Assert (_pRecordGenerator);

    HRESULT hr = S_OK;

    if (_cPageSize>0)
    {
        CDataLayerBookmark dlb = _reqCurrent._dlbStart;
        LONG lOffset = _reqCurrent._lOffset + _cPageSize;
        LONG cSize = _cPageSize;
        RECORDINDEX riStart = 0;

        if (!_reqCurrent.IsActive())
        {
            dlb = _reqCurrent._dlbBot;
            lOffset = 1;
        }

        else if (_reqCurrent.IsForward())
        {
            // defaults are already correct for extending a forward move
        }

        else
        {
            cSize = -_cPageSize;
            riStart = RecordCount() > 0 ? RecordCount() - 1 : RECORDINDEX_INVALID;

            // ignore, if reversing direction past starting point
            if (lOffset > 0)
                goto Cleanup;
        }

        hr = MakeRequest(dlb, lOffset, cSize, riStart);
    }

Cleanup:
    return hr;
}

HRESULT
CDetailGenerator::previousPage()
{
    TraceTag((tagDetail, "CDetailGenerator::next() -> %p", this));

    Assert (_pRecordGenerator);

    HRESULT hr = S_OK;

    if (_cPageSize>0)
    {
        CDataLayerBookmark dlb = _reqCurrent._dlbStart;
        LONG lOffset = _reqCurrent._lOffset - _cPageSize;
        LONG cSize = -_cPageSize;
        RECORDINDEX riStart = RecordCount() > 0 ? RecordCount() - 1 : RECORDINDEX_INVALID;

        if (!_reqCurrent.IsActive())
        {
            dlb = _reqCurrent._dlbTop;
            lOffset = -1;
        }

        else if (!_reqCurrent.IsForward())
        {
            // defaults are already correct for extending a reverse move
        }

        else
        {
            cSize = _cPageSize;
            riStart = 0;

            // ignore, if reversing direction past starting point
            if (lOffset < 0)
                goto Cleanup;
        }

        hr = MakeRequest(dlb, lOffset, cSize, riStart);
    }

Cleanup:
    return hr;
}


HRESULT
CDetailGenerator::firstPage()
{
    TraceTag((tagDetail, "CDetailGenerator::firstPage() -> %p", this));

    Assert (_pRecordGenerator);

    HRESULT hr = S_OK;

    if (_cPageSize>0)
    {
        hr = MakeRequest(CDataLayerBookmark::TheFirst, 0, _cPageSize, 0);
    }

    return hr;
}


HRESULT
CDetailGenerator::lastPage()
{
    TraceTag((tagDetail, "CDetailGenerator::lastPage() -> %p", this));

    Assert (_pRecordGenerator);

    HRESULT hr = S_OK;

    if (_cPageSize>0)
    {
        hr = MakeRequest(CDataLayerBookmark::TheLast, 0, -_cPageSize,
                RecordCount() - 1);
    }

    return hr;
}


HRESULT
CDetailGenerator::SetPageSize(long cPageSize)
{
    HRESULT hr = S_OK;

    // if the new size is the same as the old, do nothing
    if (cPageSize == _cPageSize)
        goto Cleanup;

    // set the new page size
    _cPageSize = cPageSize;

    // if the new size is 0, regenerate the full table
    if (_cPageSize == 0)
    {
        hr = MakeRequest(CDataLayerBookmark::TheFirst, 0, MAXLONG, 0);
        goto Cleanup;
    }

    // if the table is shrinking, delete rows from the end
    if (_cPageSize < RecordCount())
    {
        ReleaseRecords(_cPageSize, RecordCount() - 1);
        SetBookmarks();
    }

    // if there's an active request, we use the top bookmark so that
    // we'll repopulate the entire table and guarantee that there are no
    // gaps or overlap (as there might be if we used the bottom bookmark)
    if (_reqCurrent.IsActive())
    {
        SetBookmarks();
        hr = MakeRequest(_reqCurrent._dlbTop, 0, _cPageSize, 0);
    }
    
    // if the (inactive) table is growing, add records at the end
    else if (_cPageSize > RecordCount())
    {
        hr = MakeRequest(_reqCurrent._dlbBot, 1,
                        _cPageSize - RecordCount(), RecordCount());
    }
    
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     DeleteDetail
//
//  Synopsis:   Delete the detail section
//
//  Arguments:  [RECORDINDEX] -- cookie of a new detail
//
//  Note:       the detail might be partialy populated, in this case it is the
//              last one constructed (cloned).
//
//-------------------------------------------------------------------------

void
CDetailGenerator::DeleteDetail (RECORDINDEX  riDelete)
{
    int iRowDelete;

    if (riDelete == RECORDINDEX_INVALID)
        goto Done;

    // detach and delete the record
    _aryRecInstance[riDelete]->Detach();
    delete _aryRecInstance[riDelete];
    _aryRecInstance.Delete(riDelete);

    // delete the corresponding rows from the table
    if (_pTable->_fEnableDatabinding)
    {
        iRowDelete = RecordIndex2RowIndex(riDelete);
        _pTable->Layout()->RemoveRowsAndTheirSections(iRowDelete, iRowDelete + GetTemplateSize() - 1);
    }

Done:
    return;
}



//+------------------------------------------------------------------------
//
//  Member:     AdvanceRecordIndex
//
//  Synopsis:   get the next RecordIndex, using the current iteration direction
//
//  Arguments:  [riPrev]         -- previous RecordIndex
//
//-------------------------------------------------------------------------

RECORDINDEX
CDetailGenerator::AdvanceRecordIndex(RECORDINDEX riPrev)
{
    if (riPrev != RECORDINDEX_INVALID)
    {
        if (_reqCurrent.IsForward())
        {
            if (++riPrev >= RecordCount())
                riPrev = RECORDINDEX_INVALID;
        }
        else
        {
            if (--riPrev < 0)
                riPrev = RECORDINDEX_INVALID;
        }
    }
    return riPrev;
}



//+------------------------------------------------------------------------
//
//  Member:     RetreatRecordIndex
//
//  Synopsis:   get the previous RecordIndex, using the current iteration direction
//
//  Arguments:  [riPrev]         -- previous RecordIndex
//
//-------------------------------------------------------------------------

RECORDINDEX
CDetailGenerator::RetreatRecordIndex(RECORDINDEX riPrev)
{
    if (_reqCurrent.IsForward())
        riPrev = ((riPrev == RECORDINDEX_INVALID) ? RecordCount() : riPrev) - 1;
    else
        riPrev = ((riPrev == RECORDINDEX_INVALID) ? -1 : riPrev) + 1;
    return riPrev;
}



//+==========================================================================
//
// Request to record generator
//

//+------------------------------------------------------------------------
//
//  Member:     Activate (CRecRequest)
//
//  Synopsis:   initialize a new request
//

void
CDetailGenerator::CRecRequest::Activate(const CDataLayerBookmark& dlbStart,
                                LONG lOffset, LONG cRecords, RECORDINDEX riStart)
{
    _dlbStart   = dlbStart;
    _lOffset    = lOffset;
    _riCurrent  = riStart;
    _cRecords   = cRecords;
    _fActive    = TRUE;
    _fGotRecords = FALSE;
}


//+------------------------------------------------------------------------
//
//  Member:     MakeRequest
//
//  Synopsis:   make a request to the record generator
//
//  Arguments:  dlbStart        bookmark for starting location
//              lOffset         offset from bookmark to first record desired
//              cRecords        number of records desired (ULMAX = all records)

HRESULT
CDetailGenerator::MakeRequest(const CDataLayerBookmark& dlbStart, LONG lOffset,
                                LONG cRecords, RECORDINDEX riStart)
{
    const ULONG CHUNK_SIZE = 10;
    HRESULT hr;
    RECORDINDEX riRequest = riStart;
    IProgSink *pProgSink = NULL;
    DBCOUNTITEM ulMax = cRecords >= 0 ? cRecords : -cRecords;
    DWORD dwProgressCookieNew;

    // cancel the active request (if any)
    hr = CancelRequest(FALSE);
    if (hr)
        goto Cleanup;

    PerfDbgLog3(tagRecRequest, GetHost(), "+RecReqest off=%ld count=%ld index=%ld",
                lOffset, cRecords, riStart);
#ifdef PROFILE_REPEATED_TABLES
    StartCAP();         // for perf of repeated tables - ends in EndRequest
#endif

    // start the new request
    if (riRequest >= RecordCount())
        riRequest = RECORDINDEX_INVALID;
    _reqCurrent.Activate(dlbStart, lOffset, cRecords, riRequest);

    hr = _pRecordGenerator->RequestRecordsAtBookmark(dlbStart, lOffset, cRecords);
    if (hr)
    {
        CancelRequest();
        goto Cleanup;
    }

    // put the table into "chunk" mode.
    _pTable->Layout()->SetDirtyRowThreshold(CHUNK_SIZE);
    _pTable->SetReadyStateTable(READYSTATE_INTERACTIVE);

    // if it's an open-ended task, ask the cursor how long
    if (cRecords == MAXLONG)
    {
        Assert(_pDLC);
        IGNORE_HR(_pDLC->GetSize(&ulMax));
    }

    // start up a progress item to track table expansion
    _reqCurrent._cRecordsRetrieved = 0;
    pProgSink = _pTable->Doc()->GetProgSink();

    hr = THR(pProgSink->AddProgress(PROGSINK_CLASS_DATABIND, &dwProgressCookieNew));
    if (hr)
        goto Cleanup;
    TraceTag((tagTableProgress, "AddProgress %x for table %d (%x)",
                dwProgressCookieNew, _pTable->_nSerialNumber, _pTable));

    ReleaseProgress();
    _dwProgressCookie = dwProgressCookieNew;
    
    hr = THR(pProgSink->SetProgress(_dwProgressCookie,
            PROGSINK_SET_STATE | PROGSINK_SET_IDS | PROGSINK_SET_POS | PROGSINK_SET_MAX,
            PROGSINK_STATE_LOADING, NULL, IDS_LOADINGTABLE, 0, ulMax));
    if (hr)
        goto Cleanup;
    
Cleanup:
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CancelRequest
//
//  Synopsis:   cancel an active request to the record generator
//

HRESULT
CDetailGenerator::CancelRequest(BOOL fLastRequest)
{
    HRESULT hr = S_OK;

    if (_reqCurrent.IsActive())
    {
        PerfDbgLog(tagRecRequest, GetHost(), "request cancelled");
        IGNORE_HR(EndRequest(fLastRequest));
        IGNORE_HR(_pRecordGenerator->CancelRequest());
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     EndRequest
//
//  Synopsis:   post processing, when request terminates
//

HRESULT
CDetailGenerator::EndRequest(BOOL fLastRequest)
{
    HRESULT hr = S_OK;
    Assert(_reqCurrent.IsActive());

    // if request produced new records, remember bookmarks for top and bottom rows
    if (_reqCurrent.GotRecords())
    {
        SetBookmarks();
    }

    _reqCurrent.Deactivate();

    // change the table's readystate
    if (fLastRequest && _pDLC->IsComplete())
    {
        SetReadyState();
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     SetBookmarks
//
//  Synopsis:   remember a bookmark for the first and last displayed records.
//              This association is used when moving to the next/previous page
//              of a paged table.

void
CDetailGenerator::SetBookmarks()
{
    CDataLayerBookmark dlb;

    if (RecordCount() > 0)
    {
        if (S_OK == _pDLC->CreateBookmark(GetHrow(0), &dlb))
        {
            _reqCurrent._dlbTop = dlb;
            _reqCurrent._riTop = 0;
        }

        if (S_OK == _pDLC->CreateBookmark(GetHrow(RecordCount()-1), &dlb))
        {
            _reqCurrent._dlbBot = dlb;
            _reqCurrent._riBot = RecordCount() - 1;
        }
    }
}


//+-------------------------------------------------------------------------
// Member:              FindHrow
//
// Synopsis:    Locate the position in the table where the given hrow belongs
//
// Arguments:   hrow        the desired hrow
//              pri         pointer to return value:  index where hrow should be
//                          inserted, or RECORDINDEX_INVALID if hrow doesn't
//                          belong in the active area
//
// Returns:             S_OK        hrow found, *pri set to its index
//              S_FALSE     hrow not found, *pri set to where it belongs
//              E_FAIL      error or hrow out of bounds, *pri set to INVALID

HRESULT
CDetailGenerator::FindHrow(HROW hrow, RECORDINDEX *pri)
{
    CDataLayerBookmark dlbSearch;
    HRESULT hr;

    hr = _pDLC->CreateBookmark(hrow, &dlbSearch);
    if (hr)
        goto Cleanup;

    hr = FindBookmark(dlbSearch, pri);

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
// Member:              FindBookmark
//
// Synopsis:    Locate the position in the table where the given bookmark belongs
//
// Arguments:   dlb         the desired hrow
//              pri         pointer to return value:  index where hrow should be
//                          inserted, or RECORDINDEX_INVALID if dlb doesn't
//                          belong in the active area
//
// Returns:             S_OK        dlb found, *pri set to its index
//              S_FALSE     dlb not found, *pri set to where it belongs
//              E_FAIL      error or dlb out of bounds, *pri set to INVALID

HRESULT
CDetailGenerator::FindBookmark(const CDataLayerBookmark& dlbSearch, RECORDINDEX *pri)
{
    RECORDINDEX riFirst = FirstActiveIndex();
    RECORDINDEX riLast = LastActiveIndex();
    RECORDINDEX riLow=riFirst, riHigh=riLast+1, riMid;
    CDataLayerBookmark dlbMid;
    HRESULT hr;

    // binary search
    while (riLow < riHigh)
    {
        riMid = (riLow + riHigh)/2;
        hr = _pDLC->CreateBookmark(GetHrow(riMid), &dlbMid);
        if (hr)
            goto Cleanup;

        if (dlbMid < dlbSearch)
            riLow = riMid + 1;
        else
            riHigh = riMid;
    }

    // now see if the hrow already exists
    if (riLow > riLast)
        hr = S_FALSE;
    else
    {
        hr = _pDLC->CreateBookmark(GetHrow(riLow), &dlbMid);
        if (hr)
            goto Cleanup;
        if (dlbMid != dlbSearch)
            hr = S_FALSE;
    }

Cleanup:
    // return INVALID if any errors occurred, or if the answer is out of range
    // (except in the special case of a nonpaged, fully-expanded table)
    if (FAILED(hr) ||
        (_reqCurrent.IsActive() && riLow > LastActiveIndex()) ||
        (!_reqCurrent.IsActive() && _cPageSize > 0 && riLow >= _cPageSize) )
        riLow = RECORDINDEX_INVALID;
    *pri = riLow;

    RRETURN1(hr, S_FALSE);
}


//+-------------------------------------------------------------------------
// Member:              AllChanged (public, CDataLayerCursorEvents)
//
// Synopsis:    /*[synopsis]*/
//
// Arguments:   /*[args]*/
//
// Returns:             /*[returns]*/

HRESULT
CDetailGenerator::AllChanged()
{
    CElement * pElement = GetHost();

    // Make sure it's a table site.
    if (pElement->Tag()==ETAG_TABLE)
    {
        DYNCAST(CTable, pElement)->refresh();     // refresh it.
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              RowsChanged (public, CDataLayerCursorEvents)
//
// Synopsis:    /*[synopsis]*/
//
// Arguments:   /*[args]*/
//
// Returns:             /*[returns]*/

HRESULT
CDetailGenerator::RowsChanged(DBCOUNTITEM cRows, const HROW *ahRows)
{
    RECORDINDEX ri;

    for (ULONG k=0; k<cRows; ++k)
        if (S_OK == FindHrow(ahRows[k], &ri))
            AssignHrowToRecord(ahRows[k], ri);
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:              FieldsChanged (public, CDataLayerCursorEvents)
//
// Synopsis:    forward notification to all my instances
//
// Arguments:   /*[args]*/
//
// Returns:             /*[returns]*/

HRESULT
CDetailGenerator::FieldsChanged(HROW hRow, DBORDINAL cColumns, DBORDINAL aColumns[])
{
    RECORDINDEX ri;

    if (S_OK == FindHrow(hRow, &ri))
        IGNORE_HR(_aryRecInstance[ri]->OnFieldsChanged(hRow, cColumns, aColumns));
    return S_OK;
}


//+-------------------------------------------------------------------------
// Member:      RowsInserted (public, CDataLayerCursorEvents)
//
// Synopsis:    add new rows to the table
//
// Arguments:   cRows       number of new rows
//              ahRows      HROWs for the new rows
//
// Returns:     HRESULT

HRESULT
CDetailGenerator::RowsInserted(DBCOUNTITEM cRows, const HROW *ahRows)
{
    HRESULT hr = S_OK;
    DBCOUNTITEM cRowsInserted = 0;

    for (ULONG iRow=0; iRow<cRows; ++iRow)
    {
        // locate the new row in the up-to-date area of the table.
        RECORDINDEX riInsert;
        Verify(S_OK != FindHrow(ahRows[iRow], &riInsert));

        // ignore insert if not in bounds.  The new row is either outside
        // the current page of the table, or the active request will find it
        // soon anyway.
        if (riInsert == 0 && _cPageSize > 0)
            riInsert = RECORDINDEX_INVALID;
        if (riInsert == RECORDINDEX_INVALID)
            continue;

        // insert a new row at the desired place
        hr = Clone(1, riInsert);
        if (hr)
            goto Cleanup;
        hr = CreateDetail(ahRows[iRow], &riInsert);
        if (hr)
            goto Cleanup;

        // if we've exceeded the page size, trim
        if (_cPageSize > 0 && RecordCount() > _cPageSize)
        {
            DeleteDetail(RecordCount() - 1);
        }

        // tell the active request what happened
        if (_reqCurrent.IsActive())
        {
            RECORDINDEX riCurrent = GetCurrentIndex();
            if (riCurrent != RECORDINDEX_INVALID &&
                (_reqCurrent.IsForward() && riCurrent > riInsert))
            {
                SetCurrentIndex(AdvanceRecordIndex(riCurrent));
            }
        }

        ++ cRowsInserted;
    }

Cleanup:
    // if rows arrived, we told the table not to recalc while we pasted stuff in.
    // Tell it to recalc now.
    if (cRowsInserted > 0)
    {
        _pTable->Layout()->Resize();

        // adjust bookmarks, if necessary
        SetBookmarks();
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:              Deleting Rows (public, CDataLayerCursorEvents)
//
// Synopsis:    /*[synopsis]*/
//
// Arguments:   /*[args]*/
//
// Returns:             /*[returns]*/

HRESULT
CDetailGenerator::DeletingRows(DBCOUNTITEM cRows, const HROW *ahRows)
{
    return S_OK;
}



HRESULT
CDetailGenerator::RowsDeleted(DBCOUNTITEM cRows, const HROW *ahRows)
{
    HRESULT hr = S_OK;
    int iRow;
    int cRecordsDeleted = 0;
    RECORDINDEX riDelete = 0;   // force first delete to do a full search

    // delete in reverse order - this is slightly gentler for shifting rows
    for (iRow = cRows-1;  iRow >=0;  --iRow)
    {
        HROW hrowDelete = ahRows[iRow];

        // check for special case:  deleting a bunch of consecutive rows
        if (riDelete > 0 && GetHrow(riDelete-1) == hrowDelete)
        {
            riDelete = riDelete - 1;
        }
        // otherwise do a linear search
        else
        {
            for (riDelete = RecordCount()-1;  riDelete >= 0;  --riDelete)
            {
                if (GetHrow(riDelete) == hrowDelete)
                    break;
            }
        }

        // if we found a record to delete, do so
        if (riDelete >= 0)
        {
            DeleteDetail(riDelete);
            ++ cRecordsDeleted;

            // tell the active request what happened
            if (_reqCurrent.IsActive())
            {
                RECORDINDEX riCurrent = GetCurrentIndex();
                if (riCurrent != RECORDINDEX_INVALID &&
                    (_reqCurrent.IsForward() && riCurrent > riDelete))
                {
                    SetCurrentIndex(RetreatRecordIndex(riCurrent));
                }
            }
        }
    }

    // if we did any work, adjust for it
    if (cRecordsDeleted > 0)
    {
        SetBookmarks();

        // for paged tables, fill in the end from database
        if (_cPageSize > 0)
        {
            MakeRequest(_reqCurrent._dlbBot, +1,
                            _cPageSize - RecordCount(), RecordCount());
        }
    }

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:              DeleteCancelled (public, CDataLayerCursorEvents)
//
// Synopsis:    notification that a delete-row operation is cancelled.
//
// Arguments:   /*[args]*/
//
// Returns:             /*[returns]*/

HRESULT
CDetailGenerator::DeleteCancelled(DBCOUNTITEM cRows, const HROW *ahRows)
{
    return S_OK;
}


HRESULT
CDetailGenerator::RowsAdded()
{
    NewRecord();
    return S_OK;
}


HRESULT
CDetailGenerator::PopulationComplete()
{
    SetReadyState();

    return S_OK;
}


void
CDetailGenerator::SetReadyState()
{
    if (!_reqCurrent.IsActive() && _pDGTask->IsIdle())
    {
        // take the table out of "chunk" mode and do a full recalc/redisplay
        CTableLayout *pTableLayout = _pTable->Layout();
        pTableLayout->SetDirtyRowThreshold(0);
        if (_pTable->GetFirstBranch())
        {
            pTableLayout->Resize();
        }

#ifdef PROFILE_REPEATED_TABLES
        StopCAP();      // for perf of repeated tables - starts in MakeRequest
#endif
        PerfDbgLog(tagRecRequest, GetHost(), "-RecRequest");
        
        // we can't call ReleaseProgress from here, because of 
        // the possible reentrancy from table.onreadystate change.
        // Instead, imitate it with local variables.
        IProgSink *pProgSink = _pTable->Doc()->GetProgSink();
        DWORD dwProgCookie = _dwProgressCookie;
        _dwProgressCookie = 0;
        
        _pTable->SetReadyStateTable(READYSTATE_COMPLETE);
        // because table.onreadystatechange may alter the page,
        // assume (this) is no longer valid
        
        // ReleaseProgress();
        if (dwProgCookie)
        {
            TraceTag((tagTableProgress, "DelProgress (ready state) %x for table %d (%x)",
                        dwProgCookie, _pTable->_nSerialNumber, _pTable));
            pProgSink->DelProgress(dwProgCookie);
        }
    }
}


void
CDetailGenerator::ReleaseProgress()
{
    // release the progress item that's tracking table expansion
    if (_dwProgressCookie)
    {
        IProgSink *pProgSink = _pTable->Doc()->GetProgSink();

        TraceTag((tagTableProgress, "Release Progress %x for table %d (%x)",
                    _dwProgressCookie, _pTable->_nSerialNumber, _pTable));
        pProgSink->DelProgress(_dwProgressCookie);
        _dwProgressCookie = 0;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//      CDGTask  -  this task is responsible for hooking up databound
//                  elements that have been added to a repeated table
//

MtDefine(CDGTask, DataBind, "CDGTask");
MtDefine(CDGTask_aryBindRequest_pv, CDGTask, "CDGTask::_aryBindRequest::_pv");

CDGTask::CDGTask(CDetailGenerator *pDG) :
    CTask(TRUE),
    _pDG(pDG),
    _aryBindRequest(Mt(CDGTask_aryBindRequest_pv))
{
}

//+---------------------------------------------------------------------------
//
//  Member:     AddDataboundElement
//
//  Synopsis:   An element has entered the tree that wants to bind to the
//              record that generated one of my rows.  Post a request
//              to hook it up to the data.
//
//  Arguments:  pElement        the newly arrived element
//              id              id of binding
//              pRowContaining  the row governing pElement
//              strField        the element's dataFld (or a portion thereof)
//                              describing which column to bind
//
//----------------------------------------------------------------------------

HRESULT
CDGTask::AddDataboundElement(CElement *pElement, LONG id,
                                CTableRow *pRowContaining,
                                LPCTSTR strField)
{
    HRESULT hr = S_OK;

    CBindRequestRecord *pBindRequest = _aryBindRequest.Append();
    if (pBindRequest)
    {
        pBindRequest->pElement = pElement;
        pBindRequest->id = id;
        pBindRequest->pRowContaining = pRowContaining;
        pBindRequest->strField = strField;
        pElement->AddRef();         // don't let element die
        pRowContaining->AddRef();   // or row, either
        SetBlocked(FALSE);          // get scheduled to run
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     OnRun (CTask)
//
//  Synopsis:   Hook up all the elements currently on my list.
//
//----------------------------------------------------------------------------

void
CDGTask::OnRun(DWORD dwTimeout)
{
    UINT i;
    UINT cSize = _aryBindRequest.Size();
    
    // hook up the instances to the cloned rows
    for (i = 0;  i < cSize;  ++i)
    {
        // copy the current request.  We can't use a pointer to it, because
        // new requests might arrive and reallocate the array.
        CBindRequestRecord request = _aryBindRequest[i];
        
        IGNORE_HR(_pDG->HookUpDataboundElement(request.pElement,
                                    request.id,
                                    request.pRowContaining,
                                    request.strField));

        request.pElement->Release();  // addref'd when added to array
        request.pRowContaining->Release();  // addref'd when added to array
    }

    // now delete the requests I've serviced.  More requests may have arrived
    // during the loop above, so be careful.  This is why we cache the array
    // size before looping.
    _aryBindRequest.DeleteMultiple(0, cSize-1);

    // if there are new elements on my list, schedule myself to run again
    SetBlocked( _aryBindRequest.Size() == 0 );

    // set table's readystate
    _pDG->SetReadyState();
}


//+---------------------------------------------------------------------------
//
//  Member:     OnTerminate (CTask)
//
//  Synopsis:   Release elements currently on my list.
//
//----------------------------------------------------------------------------

void
CDGTask::OnTerminate()
{
    UINT i;
    CBindRequestRecord *pBindRequest;
    UINT cSize = _aryBindRequest.Size();
    
    // release any unserviced requests
    for (pBindRequest=_aryBindRequest, i=cSize;
         i > 0;
         ++pBindRequest, --i)
    {
        pBindRequest->pElement->Release();  // addref'd when added to array
        pBindRequest->pRowContaining->Release();  // addref'd when added to array
    }

    // delete all requests.
    _aryBindRequest.DeleteAll();
}

