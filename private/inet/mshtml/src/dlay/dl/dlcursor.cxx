//+-----------------------------------------------------------------------------
//
//  Maintained by: Jerry, Terry and Ted
//
//  Microsoft DataDocs
//  Copyright (C) Microsoft Corporation, 1994-1995
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  File:       ddoc\dl\dlcursor.cxx
//
//  Contents:   Data Layer cursor object
//
//  Classes:    CDataLayerCursor
//
//  Functions:  None.
//

#include <dlaypch.hxx>

#ifndef X_DLCURSOR_HXX_
#define X_DLCURSOR_HXX_
#include "dlcursor.hxx"
#endif

DeclareTag(tagDataLayerCursor, "DataLayerCursor",
           "Nile (OLE DB) cursor helper class" );

MtDefine(CDataLayerBookmark, DataBind, "CDataLayerBookmark")
MtDefine(CDataLayerCursor, DataBind, "CDataLayerCursor")
MtDefine(CDataLayerCursor_adlColInfo, CDataLayerCursor, "CDataLayerCursor::_adlColInfo")
MtDefine(CDataLayerCursorDeleteRows_aRowStatus, DataBind, "CDataLayerCursor::DeleteRows aRowStatus")

//+-----------------------------------------------------------------------------
//
//  Default Constructor
//
CDataLayerCursor::CDataLayerCursor(CDataLayerCursorEvents *pDLCEvents) :
    _DLNotify(pDLCEvents, GetCurrentThreadId()),
    _ulRefs(1),
    _ulAllRefs(1),
    _pAccessor(NULL),
    _pColumnsInfo(NULL),
    _pRowsetChange(NULL),
    _pRowsetExactScroll(NULL),
    _pRowsetLocate(NULL),
    _pRowsetNewRowAfter(NULL),
    _pRowsetFind(NULL),
    _pRowsetIdentity(NULL),
    _pRowsetChapterMember(NULL),
    _pcpRowsetNotify(NULL),
    _pcpDBAsynchNotify(NULL),
    _wAdviseCookieRowsetNotify(0),
    _wAdviseCookieDBAsynchNotify(0),
    _cColumns(0),
    _adlColInfo(NULL),
    _cStrBlk(NULL),
    _hBookmarkAccessor(NULL),
    _pRowsetUpdate(NULL),
    _pRowsetScroll(NULL),
    _pSupportErrorInfo(NULL),
    _hNullAccessor(NULL),
    _rcCapabilities(0),
    _uChapterSize(~0ul),
    _fNewRowsSinceLastAsynchRatioFinishedCall(FALSE),
    _fFixedSizedBookmark(FALSE),
    _fDeleteAllInProgress(FALSE)
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::constructor() -> %p", this ));
}




#if DBG == 1
//+-----------------------------------------------------------------------------
//
//  Destructor
//
CDataLayerCursor::~CDataLayerCursor()
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::destructor(%p)", this));

    Assert("Passivate wasn't called before destructor" &&
           (!_pColumnsInfo      && !_pRowsetChange      &&
            !_pRowsetExactScroll &&
            !_pRowsetLocate     && !_pRowsetNewRowAfter &&
            !_pRowsetFind       && !_pRowsetIdentity    &&
            !_pcpRowsetNotify               &&  !_pcpDBAsynchNotify        && 
            !_wAdviseCookieRowsetNotify    && !_wAdviseCookieDBAsynchNotify &&
            !_rcCapabilities     && !_pAccessor ) );
}
#endif



//+-------------------------------------------------------------------------
// Member:      AddRef (public, IUnknown)
//
// Synopsis:    increase refcount
//
// Returns:     new refcount

ULONG
CDataLayerCursor::AddRef()
{
    ULONG ulRefs = ++_ulRefs;
    return ulRefs;
}


//+-------------------------------------------------------------------------
// Member:      Release (public, IUnknown)
//
// Synopsis:    decrease refcount, passivate if 0
//
// Returns:     new refcount

ULONG
CDataLayerCursor::Release()
{
    ULONG ulRefs = --_ulRefs;
    if (ulRefs == 0)
    {
        Passivate();
        SubRelease();
    }
    return ulRefs;
}


//+-------------------------------------------------------------------------
// Member:      SubAddRef
//
// Synopsis:    increase sub-refcount
//
// Returns:     new sub-refcount

ULONG
CDataLayerCursor::SubAddRef()
{
    ULONG ulAllRefs = ++_ulAllRefs;
    return ulAllRefs;
}


//+-------------------------------------------------------------------------
// Member:      SubRelease
//
// Synopsis:    decrease sub-refcount, die if 0
//
// Returns:     new refcount

ULONG
CDataLayerCursor::SubRelease()
{
    ULONG ulAllRefs = --_ulAllRefs;
    if (ulAllRefs == 0)
    {
        _ulAllRefs = ULREF_IN_DESTRUCTOR;
        delete this;
    }
    return ulAllRefs;
}



//+-----------------------------------------------------------------------------
//
//  Passivate - Cleanup and go back to the pre-Init state
//
void
CDataLayerCursor::Passivate()
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Passivate(%p)", this));

    if (_pRowsetLocate)
    {
        IS_VALID(this);


        // Stop getting notifications:
        if (_wAdviseCookieRowsetNotify)
        {
            _pcpRowsetNotify->Unadvise(_wAdviseCookieRowsetNotify);
            _wAdviseCookieRowsetNotify = 0;
        }

        // Stop getting notifications:
        if (_wAdviseCookieDBAsynchNotify)
        {
            _pcpDBAsynchNotify->Unadvise(_wAdviseCookieDBAsynchNotify);
            _wAdviseCookieDBAsynchNotify = 0;
        }

        ReleaseAccessor(_hNullAccessor);
        ReleaseAccessor(_hBookmarkAccessor);
        if (_adlColInfo)
        {
            delete [] _adlColInfo;
            _adlColInfo = NULL;
        }
        if (_cStrBlk)
        {
            CoTaskMemFree(_cStrBlk);
            _cStrBlk = NULL;
        }
        _cColumns = 0;
        ClearInterface(&_pRowsetChapterMember);
        ClearInterface(&_pRowsetIdentity);
        ClearInterface(&_pRowsetFind);
        ClearInterface(&_pRowsetNewRowAfter);
        ClearInterface(&_pRowsetExactScroll);
        ClearInterface(&_pRowsetChange);
        ClearInterface(&_pAccessor);
        ClearInterface(&_pColumnsInfo);
        ClearInterface(&_pRowsetUpdate);
        ClearInterface(&_pRowsetScroll);
        ClearInterface(&_pAsynchStatus);
        ClearInterface(&_pSupportErrorInfo);
        ClearInterface(&_pcpRowsetNotify);
        ClearInterface(&_pcpDBAsynchNotify);
        _rcCapabilities = 0;
        _fNewRowsSinceLastAsynchRatioFinishedCall = FALSE;
        _fFixedSizedBookmark = FALSE;
        _fDeleteAllInProgress = FALSE;
        ClearInterface(&_pRowsetLocate);
    }
}


//+-----------------------------------------------------------------------------
//
//  CDataLayerCursor Init method
//
HRESULT
CDataLayerCursor::Init(IUnknown *pUnkRowset, HCHAPTER hChapter)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Init(%p, %p)",
              this, pUnkRowset ));

    HRESULT hr;
    IConnectionPointContainer *pCPC = NULL;

    Assert("Passivate wasn't called before Init" &&
           (!_pRowsetChange      &&
            !_pRowsetExactScroll &&
            !_pRowsetLocate     && !_pRowsetNewRowAfter &&
            !_pRowsetFind       && !_pRowsetIdentity    &&
            !_pcpRowsetNotify               && !_wAdviseCookieRowsetNotify  &&
            !_rcCapabilities    && !_pAccessor &&
            !_pcpDBAsynchNotify           && !_wAdviseCookieDBAsynchNotify) );

    Assert("We must be passed a valid pUnkRowset" && pUnkRowset);

    _hChapter = hChapter;
    
    // we just gotta have an IRowsetLocate to get started
    hr = pUnkRowset->QueryInterface(IID_IRowsetLocate,
                                              (void **)&_pRowsetLocate);
    if (hr)
        goto Error;
    
    // look for pSupportErrorIno before anything else, so that all
    // other start up processing can feedback rich errors, if supported
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_ISupportErrorInfo,
                                              (void **)&_pSupportErrorInfo));
                                              
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IConnectionPointContainer,
                                              (void **)&pCPC));
    if (pCPC)
    {
        THR_NOTRACE(pCPC->FindConnectionPoint(IID_IRowsetNotify, &_pcpRowsetNotify));
        if (_pcpRowsetNotify)
        {
            THR_NOTRACE(_pcpRowsetNotify->Advise((IRowsetNotify *)&_DLNotify, &_wAdviseCookieRowsetNotify));
        }

        THR_NOTRACE(pCPC->FindConnectionPoint(IID_IDBAsynchNotify, &_pcpDBAsynchNotify));
        if (_pcpDBAsynchNotify)
        {
            THR_NOTRACE(_pcpDBAsynchNotify->Advise((IDBAsynchNotify *)&_DLNotify, &_wAdviseCookieDBAsynchNotify));
        }

        ClearInterface(&pCPC);
    }

    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IRowsetChange,
                                              (void **)&_pRowsetChange));
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IRowsetExactScroll,
                                              (void **)&_pRowsetExactScroll));
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IRowsetNewRowAfter,
                                              (void **)&_pRowsetNewRowAfter));
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IRowsetFind,
                                              (void **)&_pRowsetFind));
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IRowsetUpdate,
                                              (void **)&_pRowsetUpdate));
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IRowsetScroll,
                                              (void **)&_pRowsetScroll));
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IDBAsynchStatus,
                                              (void **)&_pAsynchStatus));
    THR_NOTRACE(pUnkRowset->QueryInterface(IID_IRowsetChapterMember,
                                              (void **)&_pRowsetChapterMember));

    if (_pRowsetExactScroll)
    {
        _rcCapabilities |= RCOrdinalIndex;
    }
    if (_pRowsetScroll)
    {
        _rcCapabilities |= RCScrollable;
    }
    
    if (_pAsynchStatus)
    {
        DBCOUNTITEM ulProgress, ulProgressMax;
        DBASYNCHPHASE ulStatusCode;
        
        _rcCapabilities |= RCAsynchronous;
        _pAsynchStatus->GetStatus(_hChapter, DBASYNCHOP_OPEN,
                                 &ulProgress, &ulProgressMax,
                                 &ulStatusCode, NULL);
        _fComplete = (ulStatusCode == DBASYNCHPHASE_COMPLETE);
    }
    else
    {
        _fComplete = TRUE;
    }

    hr = FetchRowsetIdentity();
    if (hr)
        goto Error;

    hr = DL_THR(RowsetLocate, QueryInterface(IID_IAccessor,
                                       (void **)&_pAccessor ) );
    if (hr)
    {
        goto Error;
    }

    CacheColumnInfo(_pRowsetLocate);

    
Cleanup:
    RRETURN(hr);

Error:
    Assert(hr);
    Passivate();
    goto Cleanup;
}

HRESULT
CDataLayerCursor::CacheColumnInfo(IUnknown *RowsetLocate)
{
    HRESULT hr;
    DBCOLUMNINFO *aColInfoTemp = NULL;

    if (!_pColumnsInfo)
    {
        hr = DL_THR(RowsetLocate, QueryInterface(IID_IColumnsInfo,
                                               (void **)&_pColumnsInfo ) );
        if (hr)
        {
            goto Cleanup;
        }
    }

    // CacheColumnInfo should only be called once per rowset.
    Assert(!_cStrBlk && !_adlColInfo);

    hr = DL_THR(ColumnsInfo, GetColumnInfo(&_cColumns, &aColInfoTemp, &_cStrBlk));
    if (hr)
    {
        goto Cleanup;
    }

    // Allocate cache:
    _adlColInfo = new(Mt(CDataLayerCursor_adlColInfo)) ColumnInfo[_cColumns];
    if (!_adlColInfo)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    {
        DBCOLUMNINFO *pInfoTempIndex;
        ColumnInfo *pInfoIndex;
        ULONG i;

        for (i = 0, pInfoTempIndex = aColInfoTemp, pInfoIndex = _adlColInfo;
             i < _cColumns;
             i++, pInfoTempIndex++, pInfoIndex++ )
        {
            pInfoIndex->pwszName    = pInfoTempIndex->pwszName;
            pInfoIndex->iNumber     = pInfoTempIndex->iOrdinal;
            pInfoIndex->dwFlags     = pInfoTempIndex->dwFlags;
            pInfoIndex->dwType      = pInfoTempIndex->wType;
            pInfoIndex->cbMaxLength = pInfoTempIndex->ulColumnSize;
        }
    }

    _uChapterSize = ~0ul; // We don't know anything about the size.

    Assert(hr == S_OK);

Cleanup:
    // Deallocate temp column info and string buffer:
    if (aColInfoTemp) CoTaskMemFree(aColInfoTemp);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  FetchRowsetIdentity (private method, called by Init())
//      See whether the provider allows bitwise comparison of HROWs.
//      If not, fetch _pRowsetIdentity interface pointer.
//
//  Params:  none
//
//  Returns: S_OK if everything was fine

HRESULT
CDataLayerCursor::FetchRowsetIdentity()
//  Only fetch _pRowsetIdentity if required by provider.
{
    Assert(_pRowsetLocate && "must have valid IRowsetLocate");

    HRESULT hr = S_OK;
    IRowsetInfo *pRowsetInfo;
    const int kRowset=0;            // index into aPropIDSets
    DBPROPIDSET aPropIDSets[kRowset + 1];
    const int kLiteralIdentity=0;   // indexes into aPropertyIDs
    const int kStrongIdentity=1;
    DBPROPID aPropertyIDs[kStrongIdentity + 1];
    ULONG cPropSets=0;
    DBPROPSET *aPropSets = NULL;
    DBPROP *pLiteralIdentity, *pStrongIdentity;
    BOOL bBitCompareOK = TRUE;  // does provider approve comparing HROWs bitwise?
    // BUGBUG defaulting to TRUE even if properties are unsupported?
    
    if (THR(_pRowsetLocate->QueryInterface(IID_IRowsetInfo,
                                        (void **)&pRowsetInfo)))
    {
        // BUGBUG?: Assume true identity
        goto Cleanup;
    }

    // prepare query to IRowsetInfo::GetProperties
    aPropIDSets[kRowset].rgPropertyIDs = aPropertyIDs;
    aPropIDSets[kRowset].cPropertyIDs = ARRAY_SIZE(aPropertyIDs);
    aPropIDSets[kRowset].guidPropertySet = DBPROPSET_ROWSET;
    aPropertyIDs[kLiteralIdentity] = DBPROP_LITERALIDENTITY;
    aPropertyIDs[kStrongIdentity] = DBPROP_STRONGIDENTITY;

    hr = THR(HandleError(IID_IRowsetInfo, pRowsetInfo->GetProperties(
                                        ARRAY_SIZE(aPropIDSets), aPropIDSets,
                                        &cPropSets, &aPropSets ) ) );
    ClearInterface(&pRowsetInfo);
    if (hr)
    {
        goto Cleanup;
    }

    // see if rowset lets us compare HROWS bitwise
    pLiteralIdentity = & aPropSets[kRowset].rgProperties[kLiteralIdentity];
    pStrongIdentity = & aPropSets[kRowset].rgProperties[kStrongIdentity];

    if (pLiteralIdentity->dwStatus == DBPROPSTATUS_OK &&
        V_BOOL(&pLiteralIdentity->vValue) == VARIANT_FALSE)
    {
        bBitCompareOK = FALSE;
    }

    if (! bBitCompareOK)
    {
        hr = DL_THR(RowsetLocate, QueryInterface(IID_IRowsetIdentity,
                                        (void **) &_pRowsetIdentity ) );
        if (hr == E_NOINTERFACE)
            hr = S_OK;          // degrade to comparing HROWs directly
        if (hr)
            goto Cleanup;
    }

Cleanup:
    // release memory returned by query
    if (cPropSets > 0)
    {
        ULONG iPropSet;
        for (iPropSet=0; iPropSet<cPropSets; ++iPropSet)
            CoTaskMemFree(aPropSets[iPropSet].rgProperties);
        CoTaskMemFree(aPropSets);
    }

    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  NewRowAfter - Inserts a new HROW in a given Chapter at position just after
//                Bookmark, the row will have no data in it and must be filled
//                in before Update is called
//
//  Params:  rdlb  Reference to Bookmark to add row just after
//           pHRow Pointer to HROW to receive new (ref-counted) row or
//                 NULL iff no record is needed just now of that row.
//
//  Returns: S_OK if everything was fine
//           E_...  error in standard OLE range on actual error
//           if no records could be returned, then *pHRow filled in with NULL
//
HRESULT
CDataLayerCursor::NewRowAfter(const CDataLayerBookmark &rdlb, HROW *pHRow)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::NewRowAfter(%p, %p)",
              this, &rdlb ));

    IS_VALID(this);

    HROW hRow = NULL;
    HRESULT hr = S_OK;

    if (!_hNullAccessor)
    {
        hr = THR(CreateAccessor(_hNullAccessor,
                                DBACCESSOR_ROWDATA,
                                NULL, 0 ));
        if (hr)
        {
            goto Cleanup;
        }
    }

    Assert(_hNullAccessor);

    if (_pRowsetNewRowAfter)
    {
        ULONG cbBookmark = 0; 
        const BYTE* pBookmark = NULL;   // to keep compiler happy

        // caller sends us DBBMK_FIRST to insert a new "first" row.
        // In this case, we give IRowsetNewRowAfter::SetNewDataAfter
        // what it wants for this situation, namely a bookmark of length 0
        if (!rdlb.IsDBBMK_FIRST())
        {
            cbBookmark = rdlb.getDataSize();
            pBookmark = rdlb.getDataPointer();
        }

        hr = DL_THR(RowsetNewRowAfter, SetNewDataAfter(
            _hChapter,
            cbBookmark, pBookmark,
            _hNullAccessor, NULL, &hRow ));
        hr = ClampITFResult(hr);
    }
    else
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    // If we are not receiving notifications from below, we should fire
    //   events ourselves.
    if (!_wAdviseCookieRowsetNotify)
    {
        IGNORE_HR(_DLNotify.OnRowChange(_pRowsetLocate, 1, &hRow,
                                DBREASON_ROW_INSERT, DBEVENTPHASE_DIDEVENT,
                                TRUE));
    }

Cleanup:
    if (pHRow)
    {
        *pHRow = hr ? NULL : hRow;
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  GetRowAt - Retrieves the specified HROW in a given Chapter
//
//  Params:  rdlb  Reference to Bookmark to start fetching at
//
//  Returns: S_OK if everything was fine
//           S_FALSE if no record could be returned for "normal" reasons
//           E_...  error in standard OLE range on actual error
//           if no records could be returned, then *pHRow filled in with NULL
//
HRESULT
CDataLayerCursor::GetRowAt(const CDataLayerBookmark &rdlb, HROW *pHRow)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::GetRowAt(%p, %p)",
              this, &rdlb ));

    IS_VALID(this);
    HRESULT hr;
    DBCOUNTITEM cRows;

    hr = GetRowsAt(rdlb, 0, 1, &cRows, pHRow);
    if (!cRows)
    {
        *pHRow = NULL;
    }

    RRETURN2(hr, S_FALSE, DB_S_BOOKMARKSKIPPED);
}


//+-----------------------------------------------------------------------------
//
//  GetRowsAt - Retrieves the next set of HROW's in a given Chapter.
//
//  Params:  rdlb           Reference to Bookmark to start fetching at
//           iOffset        Offset from rdlb to the first row to fetch
//           iRows          number of rows to fetch, signed, < 0 to go backwards
//           puFetchedRows  pointer to return the number of rows fetched
//           pHRows         caller allocated array of HROW's for result
//
//  Returns: S_OK -- everything OK
//           S_FALSE -- not all records retrieved (possibly 0), but no error
//           DB_S_BOOKMARKSKIPPED iff rdlb refers to a deleted hrow
//           DB_S_ENDOFROWSET if our fetch went past end of rowset
//           E_...  "standard" error code (no other OLE DB errors propagated)
//
//  Comments: We have to be carefull to return the new rows which have been
//            added to the cursor but not commited, iff this cursor doesn't do
//            this for us.
//
HRESULT
CDataLayerCursor::GetRowsAt(const  CDataLayerBookmark &rdlb,
                            DBROWOFFSET iOffset,
                            DBROWCOUNT  iRows,
                            DBCOUNTITEM *puFetchedRows,
                            HROW  *pHRows )
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::GetRowsAt(%p, %p, %i, %u)",
              this, &rdlb, iOffset, iRows ));

    IS_VALID(this);
    Assert(!rdlb.IsNull());
    Assert(puFetchedRows);
    Assert(pHRows);

    *puFetchedRows = 0;

    HRESULT     hr              = S_OK;
    const BYTE *pbBookmarkData  = rdlb.getDataPointer();
    size_t      uBookmarkSize   = rdlb.getDataSize();
    
    hr = DL_THR(RowsetLocate, GetRowsAt(DBWATCHREGION_NULL,
            _hChapter, uBookmarkSize, pbBookmarkData,
            iOffset, iRows, puFetchedRows, &pHRows ));

    RRETURN2(hr, DB_S_ENDOFROWSET, DB_S_BOOKMARKSKIPPED);
}


//+-----------------------------------------------------------------------------
//
//  DeleteRows - Deletes the given hRows
//
//  Params:  uRows   of rows to delete
//           pHRows  array of hRows to delete
//
HRESULT
CDataLayerCursor::DeleteRows(size_t uRows, HROW *pHRows)
{
    HRESULT hr;
    HCHAPTER hcDummy = 0;       // reserved arg for DeleteRows
    DBROWSTATUS *aRowStatus = new(Mt(CDataLayerCursorDeleteRows_aRowStatus)) DBROWSTATUS[uRows];

    if (aRowStatus == 0) {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // If we are not receiving notifications from below, we should fire
    //   events ourselves.
    if (!_wAdviseCookieRowsetNotify)
    {
        hr = THR(_DLNotify.OnRowChange(_pRowsetLocate, uRows, pHRows,
                             DBREASON_ROW_DELETE, DBEVENTPHASE_ABOUTTODO,
                             FALSE ));
        if (FAILED(hr) || hr == S_FALSE)
        {
            hr = E_FAIL;
            goto Cleanup;
        }
        Assert(!hr);    // AndrewL 5/96 Why do we allow success codes other than S_FALSE?
    }


    hr = DL_THR(RowsetChange, DeleteRows(hcDummy, uRows, pHRows, aRowStatus));

    // If we are not receiving notifications from below, we should fire
    //   events ourselves.
    if (!_wAdviseCookieRowsetNotify)
    {
        // BUGBUG: Handle partial success correctly
        Assert(hr != DB_S_ERRORSOCCURRED);
        IGNORE_HR(_DLNotify.OnRowChange(_pRowsetLocate, uRows, pHRows,
                              DBREASON_ROW_DELETE,
                              (hr == S_OK ? DBEVENTPHASE_FAILEDTODO
                                                : DBEVENTPHASE_DIDEVENT ),
                              TRUE ));
    }


Cleanup:
    delete [] aRowStatus;
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  GetData - reads data from an HROW using the specifed accessor
//
//  Params:  hRow       the row to read from
//           hAccessor  the accessor handle to use
//           pData      caller allocated databuffer (buffer MUST be big enough)
//           fOriginal  callers wants unmodified data
//
//  Returns: S_OK if everything is cool
//
HRESULT
CDataLayerCursor::GetData(HROW hRow, HACCESSOR hAccessor, void *pData,
                          BOOL fOriginal)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::GetData(%p)", this));

    IS_VALID(this);

    if (fOriginal && _pRowsetUpdate)
    {
        return DL_THR(RowsetUpdate, GetOriginalData(hRow, hAccessor,
                                                    (BYTE *)pData ) ); 
    }
    else
    {
        return DL_THR(RowsetLocate, GetData(hRow, hAccessor,
                                            (BYTE *)pData ) );
    }
}




//+-----------------------------------------------------------------------------
//
//  SetData - set data from an HROW using the specifed accessor
//
//  Params:  hRow      the row to set
//           hAccessor the accessor handle to use
//           pData     caller allocated databuffer (buffer MUST be big enough)
//
//  Returns: S_OK if everything is cool
//
HRESULT
CDataLayerCursor::SetData(HROW hRow, HACCESSOR hAccessor, void *pData)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::SetData(%p)", this));

    IS_VALID(this);

    HRESULT hr = E_ACCESSDENIED;

    if (_pRowsetChange)
    {
        hr = DL_THR(RowsetChange, SetData(hRow, hAccessor, (BYTE *)pData));
    }

    if (hr)
    {
        goto Cleanup;
    }

    // If we are not receiving notifications from below, we should fire
    //   events ourselves.
    if (!_wAdviseCookieRowsetNotify)
    {
        // BUGBUG: this should be a FieldChange, not a RowChange
        IGNORE_HR(_DLNotify.OnRowChange(_pRowsetLocate, 1, &hRow,
                                DBREASON_COLUMN_SET, DBEVENTPHASE_DIDEVENT,
                                TRUE));
    }

Cleanup:
    return hr;
}




//+-----------------------------------------------------------------------------
//
//  Update - flush data to the database
//
//  Returns: S_OK if everything is cool
//
HRESULT
CDataLayerCursor::Update()
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Update(%p)", this));

    IS_VALID(this);

    HRESULT hr = S_OK;

    if (_pRowsetUpdate)
    {
        hr = DL_THR(RowsetUpdate, Update(NULL, 0, NULL, NULL, NULL, NULL));
        hr = ClampITFResult(hr);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Update - flush one record's data to the database
//
//  Returns: S_OK if everything is cool
//
HRESULT
CDataLayerCursor::Update(HROW hrow)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Update(%p %p)", this, hrow));

    IS_VALID(this);

    HRESULT hr = S_OK;

    if (_pRowsetUpdate)
    {
        hr = DL_THR(RowsetUpdate, Update(NULL, 1, &hrow, NULL, NULL, NULL));
        hr = ClampITFResult(hr);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Undo - discard all pending changes
//
//  Returns: S_OK if everything is cool
//
HRESULT
CDataLayerCursor::Undo()
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Undo(%p)", this));

    IS_VALID(this);

    HRESULT hr = S_OK;

    if (_pRowsetUpdate)
    {
        hr = DL_THR(RowsetUpdate, Undo(NULL, 0, NULL, NULL, NULL, NULL));
        hr = ClampITFResult(hr);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Undo - discard pending changes to one row
//
//  Returns: S_OK if everything is cool
//
HRESULT
CDataLayerCursor::Undo(HROW hrow)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Undo(%p, %p)", this, hrow));

    IS_VALID(this);

    HRESULT hr = S_OK;

    if (_pRowsetUpdate)
    {
        hr = DL_THR(RowsetUpdate, Undo(NULL, 1, &hrow, NULL, NULL, NULL));
        hr = ClampITFResult(hr);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  GetRowStatus - return pending row status for given row, using the OLE DB M9
//                  definitions.
//  BUGBUG: use the enum from the M9 header file
//
//  Returns: S_OK if everything is cool
//
HRESULT
CDataLayerCursor::GetRowStatus(HROW hrow, LONG *pstatus)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Undo(%p, %p)", this, hrow));

    IS_VALID(this);

    // assume success, data not changed
    HRESULT hr = S_OK;
    *pstatus = DBPENDINGSTATUS_UNCHANGED;

    if (_pRowsetUpdate)
    {
        DBCOUNTITEM cRows;
        HROW *aRows;
        DBROWSTATUS *aStatus;
        DBPENDINGSTATUS dwRowStatus = DBPENDINGSTATUS_NEW |
                                DBPENDINGSTATUS_CHANGED | DBPENDINGSTATUS_DELETED;
        
        hr = DL_THR(RowsetUpdate, GetPendingRows(NULL, dwRowStatus,
                                                 &cRows, &aRows, &aStatus));
        if (hr)
        {
            if (hr == S_FALSE)
            {
                Assert(cRows == 0);
                Assert(aRows == NULL);
                Assert(aStatus == NULL);
                hr = S_OK;
            }
            goto Cleanup;
        }

        Assert(cRows != 0);
        Assert(aRows != NULL);
        Assert(aStatus != NULL);

        HROW *pRowLoop;
        ULONG cRowLoop;
        
        for (pRowLoop = aRows, cRowLoop = cRows;
             cRowLoop;
             --cRowLoop, pRowLoop++ )
        {
            if (IsSameRow(*pRowLoop, hrow))
            {
                switch(aStatus[cRows - cRowLoop])
                {
                case DBPENDINGSTATUS_NEW:
                case DBPENDINGSTATUS_CHANGED:
                case DBPENDINGSTATUS_DELETED:
                    *pstatus = aStatus[cRows - cRowLoop];
                    break;
                default:
                    Assert(!"Unknown Row Status");
                    break;
                }
                break;
            }
        }

        ReleaseRows(cRows, aRows);
        CoTaskMemFree(aRows);
        CoTaskMemFree(aStatus);
    }

Cleanup:
    hr = ClampITFResult(hr);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  GetPositionAndSize - Return the ordinal position within a given Chapter.
//
//  Params:  rdlb           Reference to Bookmark whose position is desired
//           puFetchedRows  pointer to ULONG to return the position in
//           puChapterSize  pointer to ULONG to return the size in
//
//  Comments:
//          NULL's are accepted and may be more efficient
//
//  Returns: S_OK -- everything OK
//           E_...  "standard" error code (no OLE DB errors propagated)
//
HRESULT
CDataLayerCursor::GetPositionAndSize(const CDataLayerBookmark &rdlb,
                                     DBCOUNTITEM *puPosition, DBCOUNTITEM *puChapterSize )
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::GetPositionAndSize(%p)", this ));

    IS_VALID(this);

    HRESULT hr = E_FAIL;
    DBCOUNTITEM   uPosition    = 0;
    DBCOUNTITEM   uChapterSize = _uChapterSize;

    if (_pRowsetExactScroll)
    {
        hr = DL_THR(RowsetExactScroll, GetExactPosition(
            _hChapter,
            rdlb.getDataSize(), rdlb.getDataPointer(),
            &uPosition, &uChapterSize ));
        hr = ClampITFResult(hr);
    }
    if (hr && _pRowsetScroll)
    {
        hr = DL_THR(RowsetScroll, GetApproximatePosition(
            _hChapter,
            rdlb.getDataSize(), rdlb.getDataPointer(),
            &uPosition, &uChapterSize ));
        hr = ClampITFResult(hr);
    }

    if (!hr)
    {
        if (puPosition)
        {
            *puPosition = uPosition;
        }
        if (puChapterSize)
        {
            _uChapterSize = uChapterSize;
            *puChapterSize = uChapterSize;
        }
    }
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  GetPositionAndSize - Return the ordinal position within a given Chapter.
//
//  Params:  rdlch          Reference to Chapter which contains...
//           hRow           hRow whose position is desired
//           puFetchedRows  pointer to ULONG to return the position in
//           puChapterSize  pointer to ULONG to return the size in
//
//  Comments:
//          NULL's are accepted and may be more efficient
//
//  Returns: S_OK -- everything OK
//           E_...  "standard" error code (no OLE DB errors propagated)
//
HRESULT
CDataLayerCursor::GetPositionAndSize(HROW hRow,
                                     DBCOUNTITEM *puPosition, DBCOUNTITEM *puChapterSize )
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::GetPositionAndSize(%p)", this ));

    IS_VALID(this);

    CDataLayerBookmark dlb;
    HRESULT hr = THR(CreateBookmark(hRow, &dlb));
    if (!hr)
    {
        hr = GetPositionAndSize(dlb, puPosition, puChapterSize);
    }
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  GetSize - Return the number of rows within a given Chapter.
//
//  Params:  rdlch          Reference to Chapter which contains...
//           puChapterSize  pointer to ULONG to return the size in
//
//  Returns: S_OK -- everything OK
//           E_...  "standard" error code (no OLE DB errors propagated)
//

HRESULT
CDataLayerCursor::GetSize(DBCOUNTITEM *puChapterSize)
{
    Assert(puChapterSize);

    return GetPositionAndSize(CDataLayerBookmark::TheFirst,
                              NULL, puChapterSize );
}



//+-----------------------------------------------------------------------------
//
//
//    AddRefRows
//
//    Params:   pHRows  caller allocated array of HROWS to AddRef
//              ulcb    number of elements in the array
//
//    Comments: NULL hRows are acceptable and will be ignored
//
void
CDataLayerCursor::AddRefRows(int ulcb, HROW *pHRows)
{
    for (; ulcb--; pHRows += 1)
    {
        if (*pHRows)
        {
            {
                DL_VERIFY_OK(RowsetLocate, AddRefRows(1, pHRows, NULL, NULL));
            }
        }
    }
}



//+-----------------------------------------------------------------------------
//
//
//    ReleaseRows
//
//    Params:   pHRows  caller allocated array of HROWS to release
//              ulcb    number of elements in the array
//
//    Comments: NULL hRows are acceptable and will be ignored
//
void
CDataLayerCursor::ReleaseRows(int ulcb, HROW *pHRows)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::ReleaseRows(%p)", this));

    IS_VALID(this);
    

    for (; ulcb--; pHRows += 1)
    {
        if (*pHRows)
        {
            {
                DL_VERIFY_OK(RowsetLocate, ReleaseRows(1, pHRows, NULL, NULL, NULL));
            }
        }
    }
}



//+-----------------------------------------------------------------------------
//
//  GetColumnNumberFromName : Gives back the number of a column given its name
//
//  Params:  pstrName     Pointer to the name of the column
//           ulColumnNum  The place to put the column index of the cursor
//                          (smallest index = 1, greatest index = NrOfColumns)
//
//  Returns: E_FAIL if the column name is invalid,
//           S_OK   if valid
//
HRESULT
CDataLayerCursor::GetColumnNumberFromName(LPCTSTR pstrName, DBORDINAL &ulColumnNum)
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::GetColumnNumberFromName(%p)", this ));

    IS_VALID(this);

    Assert("Non-null Name pointer required" && pstrName);

    HRESULT hr = S_OK;
    DBID colid = {{0}, DBKIND_NAME, {const_cast<LPTSTR>(pstrName)}};

    ulColumnNum = DB_INVALIDCOLUMN;

    Assert(_pColumnsInfo);

    hr = DL_THR(ColumnsInfo, MapColumnIDs(1, &colid, &ulColumnNum));
    if (hr)
    {
        hr = E_FAIL;
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  GetColumnNameFromNumber : Gives back the name of a column given its number
//
//  Params:  ulColumnNum  The column index
//           ppstrName    Pointer to the name of the column
//
//  Returns: E_FAIL if the column number is invalid,
//           S_OK   if valid
//

HRESULT 
CDataLayerCursor::GetColumnNameFromNumber(DBORDINAL ulColumnNum, LPCTSTR *ppstrName)
{
    HRESULT hr;

    if (0<ulColumnNum && ulColumnNum<=_cColumns)
    {
        *ppstrName = _adlColInfo[ulColumnNum].pwszName;
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  CreateBookmark
//
//  Creates and returns a bookmark coresponding to the given row
//
//  Returns:
//      S_OK iff bookmark is returned
//
HRESULT
CDataLayerCursor::CreateBookmark(HROW hRow,
                                 CDataLayerBookmark *pdlb )
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::CreateBookmark(%p, %p, %p)",
              this, hRow, pdlb ));

    IS_VALID(this);
    IS_VALID(pdlb);

    Assert(hRow);

    HRESULT hr;

    if (!_hBookmarkAccessor)
    {
        hr = InitBookmarkAccessor();
        if (hr)
        {
            goto Cleanup;
        }
    }
    if (_fFixedSizedBookmark)
    {
        ULONG uBookmark;
        hr = THR(GetData(hRow, _hBookmarkAccessor, &uBookmark));
        if (!hr)
        {
            *pdlb = CDataLayerBookmark(*this, uBookmark);
        }
    }
    else
    {
        DBVECTOR dbv;
        hr = THR(GetData(hRow, _hBookmarkAccessor, &dbv));
        if (!hr)
        {
            *pdlb = CDataLayerBookmark(*this, dbv);
        }
    }

Cleanup:
    return hr; // RRETURN(hr); BUGBUG: until we figure out better clamping (ICS)
}


//+-----------------------------------------------------------------------------
//
//  InitBookmarkAccessor
//
//  Creates an accessor handle for only 1 column : the bookmark
//
//  Returns:
//      S_OK if everything is cool
//      E_FAIL sets _hBookmarkAccessor handle to 0
//
HRESULT
CDataLayerCursor::InitBookmarkAccessor()
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::InitBookmarkAccessor(%p)", this ));

    IS_VALID(this);

    HRESULT hr = E_FAIL;

    Assert(!_hBookmarkAccessor);

    ULONG         cColumns    = _cColumns;
    ColumnInfo *pdlColumnInfo = _adlColInfo;

    for (; cColumns--; pdlColumnInfo++)
    {
        if (pdlColumnInfo->dwFlags & DBCOLUMNFLAGS_ISBOOKMARK)
        {
            DBBINDING binding;
            _fFixedSizedBookmark = (pdlColumnInfo->dwFlags &
                                    DBCOLUMNFLAGS_ISFIXEDLENGTH ) &&
                                    pdlColumnInfo->cbMaxLength == sizeof(ULONG);
            binding.eParamIO = DBPARAMIO_NOTPARAM;
            binding.iOrdinal = pdlColumnInfo->iNumber;
            binding.dwPart    = DBPART_VALUE;
            binding.bPrecision = 0;         // BUGBUG what goes here?
            binding.bScale   = 0;           // BUGBUG what goes here?
            binding.obValue  = 0;
            binding.dwMemOwner = DBMEMOWNER_CLIENTOWNED;    // BUGBUG is this right?
            binding.pBindExt = 0;
            binding.dwFlags  = 0;
            if (_fFixedSizedBookmark)
            {
                binding.cbMaxLen = sizeof(ULONG);
                binding.wType = DBTYPE_UI4;
            }
            else
            {
                binding.cbMaxLen = sizeof(DBVECTOR);
                binding.wType = DBTYPE_UI1 | DBTYPE_VECTOR;
            }
                                                     
            hr = THR(CreateAccessor(_hBookmarkAccessor,
                                    DBACCESSOR_ROWDATA,
                                    &binding, 1 ));
            break;
        }
    }

    RRETURN(hr);
}




//+-----------------------------------------------------------------------------
//
//  CreateAccessor - Creates an accessor handle, default is complete row by ref
//
//  Params:   rhAccessor  reference to an accessor handle
//            dwAccFlags  specifies the accessor type, default is read
//            rgBindings  caller allocated array of binding information
//            ulcb        number of elements in the array
//
//  Returns:  S_OK if everything is cool, E_FAIL sets the accessor handle to 0
//
HRESULT
CDataLayerCursor::CreateAccessor(HACCESSOR      &rhAccessor,
                                 DBACCESSORFLAGS dwAccFlags,
                                 const DBBINDING rgBindings[],
                                 int             ulcb )
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::CreateAccessor(%p)", this ));

    IS_VALID(this);

    Assert(_pRowsetLocate);

    rhAccessor = NULL;

    RRETURN1(DL_THR(Accessor, CreateAccessor(dwAccFlags,
                                             (ULONG)ulcb,
                                             rgBindings,
                                             0,
                                             &rhAccessor,
                                             NULL )),
              DB_E_BYREFACCESSORNOTSUPPORTED );
}




//+-----------------------------------------------------------------------------
//
//  ReleaseAccessor - releases an accessor created with CreateAccessor
//
//  Params:   rhAccessor  reference to an accessor handle (which may be NULL)
//
//  Comments: rhAccessor will always be set to NULL
//
void
CDataLayerCursor::ReleaseAccessor(HACCESSOR &rhAccessor)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::ReleaseAccessor(%p, %p)",
             this, rhAccessor ));

    IS_VALID(this);

    if (rhAccessor)
    {
        DL_VERIFY_OK(Accessor, ReleaseAccessor(rhAccessor, NULL));
        rhAccessor = NULL;
    }
}




//+-----------------------------------------------------------------------------
//
//  GetColumnCount : Gives back the number of columns in the cursor
//
//  Params:  cWidth   Place to put the number of columns.
//
//  Returns: S_OK
//
HRESULT
CDataLayerCursor::GetColumnCount(DBORDINAL &cColumns)
{
    TraceTag((tagDataLayerCursor,
              "CDataLayerCursor::GetColumnCount(%p)", this ));

    IS_VALID(this);

    DBORDINAL cCnt = _cColumns;
    ColumnInfo *pdlColumnInfo = _adlColInfo;

    cColumns = 0;
    for ( ; cCnt-- ; pdlColumnInfo++)
    {
        cColumns += !(pdlColumnInfo->dwFlags & (DBCOLUMNFLAGS_ISBOOKMARK ));
    }

    RRETURN(THR(S_OK));
}




//+-----------------------------------------------------------------------------
//
//  GetPColumnInfo : helper function to get a pointer to our internal
//                   ColumnInfo structure for a given column number,
//                   including error checking to make sure that this
//                   is an ordinary (non-bookmark, etc.) column
//
//  Params:   ulColumnNum  Column index into the cursor
//            ppColumnInfo  Receiver of pointer
//
//  Returns:  S_OK          everything is fine
//            E_INVALIDARG  column out of range, or not a regular column
//
HRESULT
CDataLayerCursor::GetPColumnInfo(DBORDINAL ulColumnNum,
                                 const ColumnInfo **ppColumnInfo)
{
    HRESULT hr = E_INVALIDARG;

    TraceTag((tagDataLayerCursor, "CDataLayerCursor::GetPColumnInfo(%p)", this));

    IS_VALID(this);

    if (ulColumnNum < _cColumns)
    {
        *ppColumnInfo = &_adlColInfo[ulColumnNum];

        if (((*ppColumnInfo)->dwFlags & DBCOLUMNFLAGS_ISBOOKMARK) == 0)
        {
            hr = S_OK;
        }
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  FilterRowsToChapter : helper function to comb through a list of HROWs
//                  keeping only the ones belonging to my chapter.  This
//                  is useful for IRowsetNotify methods.  A chaptered
//                  rowset will notify about all HROWs, but we should only
//                  react to the HROWs from my chapter.
//
//  Params:   cRows     number of HROWs in the list
//            rghRows   list of HROWs
//            pcRows    pointer to number of good HROWs (returned)
//            prghRows  pointer to list of good HROWs (returned)
//
//  Note:   The caller should compare the pointer returned through prghRows
//          with the pointer passed in as rghRows.  If they differ, the caller
//          should free the memory allocated for the returned list by executing
//          "delete *prghRows".  However, this routine will use the original
//          list whenever possible, avoiding unneeded memory allocations.
//
//  Returns:  S_OK          everything is fine
//            E_INVALIDARG  pcRows or prghRows not valid
//
HRESULT
CDataLayerCursor::FilterRowsToChapter(
        DBROWCOUNT cRows,
        const HROW rghRows[  ],
        DBROWCOUNT *pcRows,
        const HROW **prghRows
        )
{
    HRESULT hr = S_OK;
    DBROWCOUNT cRowsGood = 0;
    BOOL fNeedAllocation = FALSE;
    DBROWCOUNT i;
    const HROW *pHrow;

    if (pcRows == NULL  ||  prghRows == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    // assume most common case - reuse the input array in the output
    *prghRows = rghRows;

    // special case - chapter 0 means all rows are OK
    if (_hChapter == DB_NULL_HCHAPTER)
    {
        *pcRows = cRows;
        goto Cleanup;
    }

    // bad case - no IRowsetChapterMember means we can't even tell which
    // rows belong.  Be paranoid and say none of them.
    if (_pRowsetChapterMember == NULL)
    {
        *pcRows = 0;
        goto Cleanup;
    }

    // walk through the input array and see which HROWs belong to my chapter
    for (i=0, pHrow=rghRows;  i<cRows;  ++i, ++pHrow)
    {
        hr = DL_THR(RowsetChapterMember, IsRowInChapter(_hChapter, *pHrow));
        if (hr == S_OK)
        {
            if (cRowsGood < i)
                fNeedAllocation = TRUE;
            ++ cRowsGood;
        }
    }

    // if we need to allocate an array to return to the caller, do so now
    // and fill it in.
    if (fNeedAllocation)
    {
        HROW *pHrowReturn = new HROW[cRowsGood];

        *prghRows = pHrowReturn;

        for (i=0, pHrow=rghRows;  i<cRows;  ++i, ++pHrow)
        {
            hr = DL_THR(RowsetChapterMember, IsRowInChapter(_hChapter, *pHrow));
            if (hr == S_OK)
            {
                *pHrowReturn = *pHrow;
                ++ pHrowReturn;
            }
        }
    }

    // tell caller how many rows were good
    *pcRows = cRowsGood;
    hr = S_OK;

Cleanup:
    RRETURN(hr);
}


#if DBG == 1
//+-----------------------------------------------------------------------------
//
//  Validation method, called by macro IS_VALID(p)
//
BOOL
CDataLayerCursor::IsValidObject()
{
    ULONG i;

    Assert(this);
    Assert("We must have an IRowsetLocate" &&
           (_pRowsetLocate) );

    // _adlColInfo may not be set yet, if Init() fails for some reason
    if (_adlColInfo)
    {
        for (i = 0; i < _cColumns; i++)
        {
            Assert(_adlColInfo[i].iNumber == i);
        }
    }
    
    return TRUE;
}



char s_achCDataLayerCursor[] = "CDataLayerCursor";


//+-----------------------------------------------------------------------------
//
//  Dump function, called by macro DUMP(p,dc)
//
void
CDataLayerCursor::Dump(CDumpContext &dc)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::Dump(%p)", this));

    IS_VALID(this);
}



//+---------------------------------------------------------------------------
//
//  Member:     GetClassName
//
char *
CDataLayerCursor::GetClassName()
{
    return s_achCDataLayerCursor;
}
#endif


///////////////////////////////////////////////////////////////////////
//
//      CDataLayerNotify subobject

HRESULT
CDataLayerNotify::CheckCallbackThread()
{
    HRESULT hr = S_OK;

    if (_dwTID != GetCurrentThreadId())
    {
        Assert(!"OLEDB callback on wrong thread");
        hr = E_UNEXPECTED;
    }

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Member:     QueryInterface
//
//  Synopsis:   Normal IUnknown QI
//
//  Arguments:  riid            IID of requested interface
//              ppv             Interface object to return
//
//  Returns:    S_OK            Interface supported
//              E_NOINTERFACE   Interface not supported.
//

STDMETHODIMP
CDataLayerNotify::QueryInterface (REFIID riid, LPVOID *ppv)
{
    Assert(ppv);

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IRowsetNotify *)this;
    }
    else if (IsEqualIID(riid, IID_IRowsetNotify))
    {
        *ppv = (IRowsetNotify *)this;        
    }
    else if (IsEqualIID(riid, IID_IDBAsynchNotify))
    {
        *ppv = (IDBAsynchNotify *)this;
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


//+-------------------------------------------------------------------------
// Member:      AddRef (public, IUnknown)
//
// Synopsis:    increase refcount
//
// Returns:     new refcount

ULONG
CDataLayerNotify::AddRef()
{
    return MyDLC()->SubAddRef();
}


//+-------------------------------------------------------------------------
// Member:      Release (public, IUnknown)
//
// Synopsis:    decrease refcount, die if 0
//
// Returns:     new refcount

ULONG
CDataLayerNotify::Release()
{
    return MyDLC()->SubRelease();
}


//+---------------------------------------------------------------------------
//  Member:     OnFieldChange
//
//  Synopsis:   Called by Nile when a field changes
//              Note that the notification we receive map perfectly to the
//              events we fire.
//

STDMETHODIMP
CDataLayerNotify::OnFieldChange (IRowset *pRowset, HROW hRow,
                                 DBORDINAL cColumns, DBORDINAL aColumns[],
                                 DBREASON eReason, DBEVENTPHASE ePhase,
                                 BOOL /* fCantDeny */)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::OnFieldChange(%p, %p)",
             this, pRowset ));

    HRESULT hr = CheckCallbackThread();
    if (hr)
        goto Cleanup;

    Assert(pRowset);
    if (_pDLCEvents && (ePhase == DBEVENTPHASE_DIDEVENT))  // only fire once
    {
        DBROWCOUNT cRowsGood;
        const HROW *pHrow;

        // check that hRow belongs to my chapter
        hr = MyDLC()->FilterRowsToChapter(1, &hRow, &cRowsGood, &pHrow);
        if (hr || cRowsGood==0)
            goto Cleanup;
        Assert(pHrow == &hRow);     // Filter should never allocate an array for only one row
        
        IGNORE_HR(_pDLCEvents->FieldsChanged(hRow, cColumns, aColumns));
    }

Cleanup:
    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//  Member:     OnRowChange
//
//  Synopsis:   Called by Nile when a HROW changes
//

STDMETHODIMP
CDataLayerNotify::OnRowChange (IRowset *pRowset, DBCOUNTITEM cRows,
                               const HROW ahRows[],
                               DBREASON eReason, DBEVENTPHASE ePhase,
                               BOOL /* fCantDeny */)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::OnRowChange(%p, %p)",
             this, pRowset ));

    DBROWCOUNT cRowsGood;
    const HROW *pHrowsGood = ahRows;
    
    CDataLayerCursor *pDLC = MyDLC();
    HRESULT hr = CheckCallbackThread();
    if (hr)
        goto Cleanup;

    Assert(pRowset);

    // ignore rows that don't belong to my chapter
    hr = pDLC->FilterRowsToChapter(cRows, ahRows, &cRowsGood, &pHrowsGood);
    if (hr || cRowsGood == 0)
        goto Cleanup;

    // Note that we want to update _uChapterSize even if nobody is listening.
    switch (ePhase)      // fire any given notification exactly once
    {
    case DBEVENTPHASE_OKTODO:
        if (eReason == DBREASON_ROW_DELETE || eReason == DBREASON_ROW_UNDOINSERT)
        {
            if (_pDLCEvents && !pDLC->_fDeleteAllInProgress)
            {
                hr = THR(_pDLCEvents->DeletingRows(cRowsGood, pHrowsGood));
            }
        }
        break;

    case DBEVENTPHASE_FAILEDTODO:
        if (eReason == DBREASON_ROW_DELETE || eReason == DBREASON_ROW_UNDOINSERT)
        {
            if (_pDLCEvents && !pDLC->_fDeleteAllInProgress)
            {
                IGNORE_HR(_pDLCEvents->DeleteCancelled(cRowsGood, pHrowsGood));
            }
        }
        break;

    case DBEVENTPHASE_DIDEVENT:
        switch (eReason)
        {
        case DBREASON_ROW_DELETE:
        case DBREASON_ROW_UNDOINSERT:
            if (pDLC->_uChapterSize != ~0ul)
            {
                pDLC->_uChapterSize -= cRowsGood;
            }
            if (_pDLCEvents && !pDLC->_fDeleteAllInProgress)
            {
                IGNORE_HR(_pDLCEvents->RowsDeleted(cRowsGood, pHrowsGood));
            }
            break;

        case DBREASON_ROW_INSERT:
        case DBREASON_ROW_ASYNCHINSERT:
        case DBREASON_ROW_UNDODELETE:
            if (pDLC->_uChapterSize != ~0ul)
            {
                pDLC->_uChapterSize += cRowsGood;
            }
            if (_pDLCEvents)
            {
                IGNORE_HR(_pDLCEvents->RowsInserted(cRowsGood, pHrowsGood));
            }
            break;

        case DBREASON_COLUMN_RECALCULATED:
        case DBREASON_COLUMN_SET:
        case DBREASON_ROW_RESYNCH:
        case DBREASON_ROW_UNDOCHANGE:
            if (_pDLCEvents)
            {
                IGNORE_HR(_pDLCEvents->RowsChanged(cRowsGood, pHrowsGood));
            }
            break;
        }
    }

Cleanup:
    if (pHrowsGood != ahRows)
        delete const_cast<HROW *>(pHrowsGood);
    
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//  Member:     OnRowsetChange
//
//  Synopsis:   Called by Nile when the rowset has changed
//

STDMETHODIMP
CDataLayerNotify::OnRowsetChange (IRowset *pRowset, DBREASON eReason,
                                    DBEVENTPHASE ePhase, BOOL)
{
    TraceTag((tagDataLayerCursor, "CDataLayerCursor::OnRowsetChange(%p, %p)",
             this, pRowset ));

    HRESULT hr = CheckCallbackThread();
    if (hr)
        goto Cleanup;

//$BUGBUG (dinartem) Trident now calls ExecStop at the beginning of
//$BUGBUG UnloadContents which causes OnRowsetChange to be called with a
//$BUGBUG pRowset of NULL.
//$    Assert(pRowset);

    switch (eReason)
    {
    case DBREASON_ROWSET_CHANGED:
        if (ePhase == DBEVENTPHASE_DIDEVENT)        // only fire once
        {
            if (_pDLCEvents)
            {
                IGNORE_HR(_pDLCEvents->AllChanged());
            }
        }
        break;

    default:
        hr = DB_S_UNWANTEDREASON;
        break;
    }

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CDataLayerNotify::OnLowResource(DB_DWRESERVE dwReserved)
{
    return E_NOTIMPL;
}

STDMETHODIMP
CDataLayerNotify::OnProgress( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ DBASYNCHOP ulOperation,
            /* [in] */ DBCOUNTITEM ulProgress,
            /* [in] */ DBCOUNTITEM ulProgressMax,
            /* [in] */ DBASYNCHPHASE ulStatusCode,
            /* [in] */ LPOLESTR pwszStatusText)
{
    HRESULT hr = CheckCallbackThread();
    if (hr)
        goto Cleanup;

    if (_pDLCEvents && ulStatusCode==DBASYNCHPHASE_POPULATION)
    {
        IGNORE_HR(_pDLCEvents->RowsAdded());
        // Fire PopulationComplete in the OnStop event..
    }

Cleanup:
    RRETURN(hr);
}

STDMETHODIMP
CDataLayerNotify::OnStop( 
            /* [in] */ HCHAPTER hChapter,
            /* [in] */ ULONG ulOperation,
            /* [in] */ HRESULT hrStatus,
            /* [in] */ LPOLESTR pwszStatusText)
{
    HRESULT hr = CheckCallbackThread();
    if (hr)
        goto Cleanup;

    MyDLC()->_fComplete = TRUE;

    if (_pDLCEvents)
    {
        // Add reason arguments soon..
        IGNORE_HR(_pDLCEvents->PopulationComplete());
    }

Cleanup:
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//  Member:     HandleError
//
//  Synopsis:   Error notification wrapper for Nile function calls,
//              notifies DL client of errors.
//
//  Arguments:  riid            IID of interface which was called
//              hr              HRESULT returned (may be success)
//
//  Returns:    HRESULT         hresult passed in
//

HRESULT
CDataLayerCursor::HandleError(REFIID riid, HRESULT hr)
{
    BOOL fSupportsErrorInfo = TRUE;
    
    if (SUCCEEDED(hr))
    {
        goto Cleanup;
    }

    if (!_pSupportErrorInfo
            || _pSupportErrorInfo->InterfaceSupportsErrorInfo(riid))
    {
        // to avoid any confusion, make sure we don't have ErrorInfo lying
        //  around for some other failuer.
        ::SetErrorInfo(0, NULL);
        fSupportsErrorInfo = FALSE;
    }
    
    if (_DLNotify._pDLCEvents)
    {
        IGNORE_HR(_DLNotify._pDLCEvents->OnNileError(hr, fSupportsErrorInfo));
    }

Cleanup:
    return hr;
}



//+---------------------------------------------------------------------------
//  Member:     CDataLayerCursorEvents::OnNileError
//
//  Synopsis:   Default implementation of OnError event sink.
//
//  Arguments:  HRESULT         hresult of error (must be failure code)
//              BOOL            indicates if ErrorInfo corresponds to error
//
//  Returns:    S_OK            error message/notification handled
//              S_FALSE         error message/notification not handled
//              E_*             unexpected error blocking message/notification
//

HRESULT
CDataLayerCursorEvents::OnNileError(HRESULT, BOOL)
{
    return S_FALSE;
}

