//--------------------------------------------------------------------
// IRowPosition class
// (c) 1995 Microsoft Corporation.  All Rights Reserved.
//
//  File:       position.cxx
//  Author:     Charles Frankston (cfranks)
//
//  Contents:   CRowPosition object implementation
//

#include <dlaypch.hxx>
#pragma hdrstop

#include <connect.hxx>
#include <coredisp.h>
#include "unixposition.hxx"

#ifdef ROWPOSITION_DELETE
IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CRowPosition::MyRowsetNotify,
                                   CRowPosition, _RowsetNotify);

// Gee, why doesn't the macro above give us one of these??
STDMETHODIMP
CRowPosition::MyRowsetNotify::QueryInterface (REFIID riid, LPVOID *ppv)
{
    Assert(ppv);
    
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IRowsetNotify))
    {
        *ppv = this;
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

#endif  // ROWPOSITION_DELETE

const CONNECTION_POINT_INFO CRowPosition::s_acpi[] =
{
    CPI_ENTRY(IID_IRowPositionChange, DISPID_A_ROWPOSITIONCHANGESINK)
    CPI_ENTRY_NULL
};

const CRowPosition::CLASSDESC CRowPosition::s_classdesc =
{
        NULL,                           // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        0,                              // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
};

//+---------------------------------------------------------------------------
//  Member:     GetClassDesc (public member)
//
//  Synopsis:   Return the class descriptor, we only use the descriptor to
//              describe the number of connection points our container can
//              handle and the connection points.
//
//  Arguments:  None
//
//  Returns:    CLASSDESC
//

const CBase::CLASSDESC *
CRowPosition::GetClassDesc () const
{
    return &s_classdesc;
}


STDMETHODIMP
CRowPosition::PrivateQueryInterface (REFIID riid, LPVOID * ppv)
{

    Assert(ppv);

    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *)(IPrivateUnknown *)this;
    }
#define TEST(IFace) else if (IsEqualIID(riid, IID_##IFace)) *ppv = (IFace *)this

    TEST(IRowPosition);

#undef TEST

    else if (IsEqualIID(riid, IID_IConnectionPointContainer))
    {
        *((IConnectionPointContainer **)ppv) =
                                            new CConnectionPointContainer(this, NULL);
        if (!*ppv)
        {
            RRETURN(E_OUTOFMEMORY);
        }
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


//+---------------------------------------------------------------------------
//  Member:     Passivate()
//
//  Synopsis:   Called from our base class's release (CBase::Release).
//              Clean ourselves up in preparation for destruction.
//
//  Arguments:  None
//
//  Returns:    None

void
CRowPosition::Passivate()
{
    ReleaseResources();
    super::Passivate();
}

//+---------------------------------------------------------------------------
//  Member:     ReleaseResources()      (protected helper
//
//  Synopsis:   Release my resources
//
//  Arguments:  None
//
//  Returns:    None

void
CRowPosition::ReleaseResources()
{
    if (_pCP)
    {
        _pCP->Unadvise(_wAdviseCookie);
        ClearInterface(&_pCP);
    }

    if (_pRowset)
    {
#ifdef ROWPOSITION_DELETE
        ReleaseBookmarkAccessor();
        ClearInterface(&_pRowsetLocate);
#endif

        // Special code for our cleanup
        if (_hRow != DB_NULL_HROW)
        {
            _pRowset->ReleaseRows(1, &_hRow, NULL,
                                  NULL, NULL);    
            _hRow = NULL;
        }

        if (_pChapRowset &&_hChapter != DB_NULL_HCHAPTER)
        {
            _pChapRowset->ReleaseChapter(_hChapter, NULL);
        }
        
        ClearInterface(&_pChapRowset);
        ClearInterface(&_pRowset);
    }
}

//+---------------------------------------------------------------------------
//  Member:     Init (public member)
//
//  Synopsis:   Initializes a newly create CRowPosition.
//              We initialize most things here rather than in the constructor
//              so we can do better error checking.
//
//  Arguments:  pRowSet             Pointer to the IRowset we're on top of.
//
//  Returns:    S_OK                if everything is fine,
//

HRESULT
CRowPosition::Initialize(IUnknown *pRowset)
{
    HRESULT hr;
    IConnectionPointContainer *pCPC = NULL;

#ifdef ROWPOSITION_DELETE
    Assert(_pRowsetLocate == NULL && _Bookmark.ptr==NULL && _pAccessor==NULL &&
           _hAccessorBookmark==NULL && "IRowPosition init'd more than once.");
#endif

    Assert(_pRowset==NULL && _pCP==NULL && 
           _hRow==NULL && "IRowPosition init'd more than once.");

    if (!pRowset)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    hr =  pRowset->QueryInterface(IID_IRowset, (LPVOID *)&_pRowset);
    if (FAILED(hr)) goto Error;

    IGNORE_HR(pRowset->QueryInterface(IID_IChapteredRowset, (LPVOID *)&_pChapRowset));

#ifdef ROWPOSITION_DELETE

    hr = _pRowset->QueryInterface(IID_IRowsetLocate, (void **)&_pRowsetLocate);
    if (FAILED(hr)) goto Error;

    hr = _pRowset->QueryInterface(IID_IConnectionPointContainer, (void **)&pCPC);
    if (pCPC)
    {
        hr = pCPC->FindConnectionPoint(IID_IRowsetNotify, &_pCP);
        if (_pCP)
        {
            hr = _pCP->Advise((IUnknown *)&_RowsetNotify, &_wAdviseCookie);
            if (FAILED(hr))
            {
                ClearInterface(&_pCP);  // make sure we don't DeAdvise
            }
        }
        ClearInterface(&pCPC);
    }

    if (FAILED(hr)) goto Error;

    // Initialize and remember bookmark accessor.
    hr = CreateBookmarkAccessor();
    if (FAILED(hr)) goto Error;

#endif



Cleanup:
    return hr;

Error:
    ReleaseResources();
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//  Member:     GetRowset (public member)
//
//  Synopsis:   returns an interface pointer from the rowset we're on top of.
//              Note this does a QI on that interface, remember to release!
//
//  Arguments:  iid             the IID of the interface we want.
//              *ppIUnknown     pointer to the interface requested
//
//  Returns:    S_OK                if everything is fine,
//
STDMETHODIMP
CRowPosition::GetRowset(REFIID riid, LPUNKNOWN *ppIUnknown)
{
    HRESULT hr;

    if (_pRowset)
    {
        hr = _pRowset->QueryInterface(riid, (void **)ppIUnknown);
    }
    else
    {
        Assert(FALSE && "Rowset disappeared?");
        hr = E_UNEXPECTED;
    }
    return hr;
};

STDMETHODIMP
CRowPosition::ClearRowPosition()
{
    HRESULT hr;
    int iProgress=0;                    // track # of sinks.

    if (!_pRowset || _fCleared)
        return E_UNEXPECTED;

    // First, OKTODO..
    hr = FireRowPositionChange(DBREASON_ROWPOSITION_CLEARED, DBEVENTPHASE_OKTODO, 0, &iProgress);   
    if (CANCELLED(hr))
        goto Cancel;

    // Then ABOUTTODO
    iProgress = 0;
    hr = FireRowPositionChange(DBREASON_ROWPOSITION_CLEARED,  DBEVENTPHASE_ABOUTTODO, 0, &iProgress);
    if (CANCELLED(hr))
        goto Cancel;

    // release our hold on the HROW
    if (_hRow != DB_NULL_HROW)
        _pRowset->ReleaseRows(1, &_hRow, NULL, NULL, NULL);
    
    // set internal state
    _fCleared = TRUE;   
    _hRow = DB_NULL_HROW;
    _dwPositionFlags = DBPOSITION_NOROW;

    return NOERROR;

Cancel: 
    // If Change was cancelled, we fire a FAILEDTODO event.
    FireRowPositionChange(DBREASON_ROWPOSITION_CLEARED, DBEVENTPHASE_FAILEDTODO, 1, &iProgress);
    // UNDONE:  we aren't turning off events yet per connection
    return DB_E_CANCELED;             // User cancelled
};


STDMETHODIMP
CRowPosition::SetRowPosition(HCHAPTER hChapter, HROW hRow, DBPOSITIONFLAGS dwPositionFlags)
{
    HRESULT hr;
    int iProgress=0;                    // track # of sinks.
    int iDummy=0;
    DBREASON eReason = DBREASON_ROWPOSITION_CHANGED;

    if (!_pRowset || !_fCleared)
        return E_UNEXPECTED;

    if( (hRow && dwPositionFlags != DBPOSITION_OK) || 
        (!hRow && dwPositionFlags == DBPOSITION_OK) )
        return E_INVALIDARG;

    // AddRef the new hRow
    // UNDONE:  Is it the client's duty to addref the hRow before
    // returning it back from Move?
    if (hRow!=DB_NULL_HROW) 
    {
        hr = _pRowset->AddRefRows(1, &hRow, NULL, NULL);
        if( FAILED(hr) )
            return DB_E_BADROWHANDLE;
    }

    if (_hChapter != hChapter)
    {
        if (_pChapRowset)
        {
            if (hChapter != DB_NULL_HCHAPTER)
                _pChapRowset->AddRefChapter(hChapter, NULL);
            if (_hChapter != DB_NULL_HCHAPTER)
                _pChapRowset->ReleaseChapter(_hChapter, NULL);
        }
        
        _hChapter = hChapter;   
        eReason = DBREASON_ROWPOSITION_CHAPTERCHANGED;
    }

    _hRow = hRow;               // make the change
    _dwPositionFlags = dwPositionFlags;

    // Do the SYNCHAFTER event.  This cannot be cancelled or fail!
    FireRowPositionChange(eReason, DBEVENTPHASE_SYNCHAFTER, 1, &iDummy);

    iDummy = 0;
    // Do the DIDEVENT.  This cannot be cancelled or fail!
    FireRowPositionChange(eReason, DBEVENTPHASE_DIDEVENT, 1, &iDummy);

    // reset
    _fCleared = FALSE;

    return NOERROR;
}

//+---------------------------------------------------------------------------
//  Member:     GetRowPosition (public member)
//
//  Synopsis:   Gets our current position in the Rowset (as an hRow).
//              AddRefRows the hRow.  CALLER MUST ReleaseRows!!
//
//  Arguments:  hRow            current hRow position
//
//  Returns:    S_OK            if everything is fine,
//
STDMETHODIMP
CRowPosition::GetRowPosition(HCHAPTER *phChapter, HROW *phRow,
                             DBPOSITIONFLAGS *pdwPositionFlags)
{
    if (!_pRowset)
        return E_UNEXPECTED;

    if (phRow)
    {
        if (_hRow != DB_NULL_HROW)
        {
            _pRowset->AddRefRows(1, &_hRow, NULL, NULL);
        }

        *phRow = _hRow;
    }

    if (phChapter)
    {
        if (_hChapter != DB_NULL_HCHAPTER)
        {
            if (_pChapRowset)
            {
                _pChapRowset->AddRefChapter(_hChapter, NULL);
            }
        }

        *phChapter = _hChapter;
    }

    if (pdwPositionFlags)
    {
        *pdwPositionFlags = _dwPositionFlags;
    }

    return S_OK;
};

#ifdef ROWPOSITION_DELETE
//+---------------------------------------------------------------------------
//  Member: CreateBookMarkAccessor (private)
//
//  Synopsis:   Helper function to create the Bookmark Accessor
//  
//  Arguments:  none, uses member variables.
//
//  Returns:    S_OK            if everything is fine,
//
HRESULT
CRowPosition::CreateBookmarkAccessor()
{
    HRESULT       hr=E_FAIL;            // if we can't QI IAccessor!
    DBBINDING     dbind;

    Assert(_hAccessorBookmark==NULL);   // Already init'd??

    hr = _pRowset->QueryInterface(IID_IAccessor,
                                  (void **)&_pAccessor);
    if (_pAccessor)
    {
        dbind.iOrdinal = 0;         // columns 0 must mean Bookmark
        dbind.obValue = 0;          // offset to value
        dbind.obLength = 0;         // ignored, no DBPART_LENGTH
        dbind.obStatus = 0;         // ignored no DBPART_STATUS
        dbind.pTypeInfo = NULL;     // spec sez set to null
        dbind.pObject = NULL;       // ignored unless DBTYPE_IUNKNOWN
        dbind.pBindExt = NULL;      // spec sez set to null
        dbind.dwPart = DBPART_VALUE;
        dbind.dwMemOwner = DBMEMOWNER_CLIENTOWNED;
        dbind.eParamIO = DBPARAMIO_NOTPARAM;
        dbind.cbMaxLen = 0;         // not used for DBTYPE_VECTOR
        dbind.dwFlags = 0;          // spec sez set to 0
        dbind.wType = (DBTYPE_UI1 | DBTYPE_VECTOR);
        hr = _pAccessor->CreateAccessor(DBACCESSOR_ROWDATA,
                                        1, &dbind, 0,
                                        &_hAccessorBookmark,
                                        NULL /*rgStatus*/);
        if (FAILED(hr))
        {
            ClearInterface(&_pAccessor);
        }
    } // _pAccessor

    return hr;
}

//+-----------------------------------------------------------------------
//
//  Member:    ClearBookmark
//
//  Synopsis:  Helper function to de-allocate any bookmarks we might have,
//             because we need this in a few places.
//
//  Arguments: uses member variables.
//

void
CRowPosition::ClearBookmark()
{
    if (_Bookmark.ptr)
    {
        CoTaskMemFree(_Bookmark.ptr);
        _Bookmark.ptr = NULL;
    }
}

//+-----------------------------------------------------------------------
//
//  Member:   ReleaseBookmarkAccessor
//
//  Synopsis:  Helper function to destroy the bookmark accessor.
//
//  Arguments: uses member variables.
//

void
CRowPosition::ReleaseBookmarkAccessor()
{
    ULONG ulDontCare;

    ClearBookmark();
    if (_pAccessor && _hAccessorBookmark)
    {
        _pAccessor->ReleaseAccessor(_hAccessorBookmark, &ulDontCare);
        _hAccessorBookmark = NULL;
    }
    ClearInterface(&_pAccessor);
}
#endif

//+---------------------------------------------------------------------------
//  Member:     CRowPosition::FireRowPositionChange
//
//  Synopsis:   Helper routine to Fire change notices to all of our clients.
//
//  Arguments:  eReason [in]     reason for event
//              ePhase [in]      which notification phase this is
//              fCantDeny [in]   TRUE iff client can cancel this phase
//              piProgress [in]  send this event to the first iProgress sinks
//                        [out]  on output: decremented by the number of sinks,
//                               stop firing when count reaches 0.  We always
//                               fire at least one event however.
//
//  Returns:    S_OK             client accepts change
//              S_FALSE          client wants to cancel change
//              S_UNWANTEDPHASE  client doesn't want any more phase notifications
//                               (caller doesn't have to comply)
//              S_UNWANTEDREASON client doesn't want any more of this reason
//                               (caller doesn't have to comply)
//

HRESULT
CRowPosition::FireRowPositionChange(DBREASON eReason,
                                    DBEVENTPHASE ePhase,
                                    BOOL fCantDeny,
                                    int *piProgress)
{
    AAINDEX         aaidx;
    HRESULT         hr = S_OK;
    NOTIFY_STATE    ns;

    DWORD pdwCookie = 0;                // cookie for GetNextSink
    IRowPositionChange *pRPC=NULL;      // Notification we fire.

    aaidx = AA_IDX_UNKNOWN;

    ns = EnterNotify(eReason, DBEVENTPHASE_DIDEVENT);

    for (;;)
    {
        aaidx = FindNextAAIndex(DISPID_A_ROWPOSITIONCHANGESINK, 
                                CAttrValue::AA_Internal, 
                                aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;

        ClearInterface(&pRPC);
        if (OK(GetUnknownObjectAt(aaidx, (IUnknown **)&pRPC)))
        {
            hr = pRPC->OnRowPositionChange(eReason, ePhase, fCantDeny);

            // Have we fired all we're supposed to yet?
            if (--*piProgress==0) break;

            // Stop upon the first event that fails, or was cancelled.
            // We don't stop on (or propagate upward) DB_S_UNWANTEDPHASE
            // or DB_S_UNWANTED REASON.
            if (FAILED(hr) || (hr==S_FALSE && !fCantDeny))
            {
                // flip piProgress, so if we're called again with the same
                // iProgress, we fire only as many events as we did this time.
                // (Note iProgress actually represents # sinks fired+1).
                *piProgress = -*piProgress;
                goto Cleanup;
            }

        }
    }

    // If we fired events to all sinks without error or cancel.
    *piProgress = 0;

Cleanup:
    LeaveNotify(eReason, DBEVENTPHASE_DIDEVENT, ns);

    ReleaseInterface(pRPC);
    RRETURN(hr);
}

#ifdef ROWPOSITION_DELETE
//+---------------------------------------------------------------------------
//  Member:     MyRowsetNotify::OnChapterChange
//
//  Synopsis:   Implemenation of IRowsetNotify::OnRowChange
//              This doesn't do anything.
//

// We're not interested in OnFieldChange
STDMETHODIMP
CRowPosition::MyRowsetNotify::OnFieldChange (IRowset *pRowset,
                                             HROW hRow,
                                             ULONG cColumns,
                                             ULONG aColumns[],
                                             DBREASON eReason,
                                             DBEVENTPHASE ePhase,
                                             BOOL fCantDeny)
{
    return DB_S_UNWANTEDREASON;
}

//+---------------------------------------------------------------------------
//  Member:     MyRowsetNotify::OnRowChange
//
//  Synopsis:   Implemenation of IRowsetNotify::OnRowChange
//              Looks for ROw_DELETE notifications in case they affect the hRow
//              we're currently on, in which case we pass them along.
//
//  Arguments:  hRow            new hRow position
//
//  Returns:    S_OK                if everything is fine,
//
// We are interested in OnRowChange
STDMETHODIMP
CRowPosition::MyRowsetNotify::OnRowChange (IRowset *pRowset,
                                         ULONG cRows,
                                         const HROW ahRows[],
                                         DBREASON eReason,
                                         DBEVENTPHASE ePhase,
                                         BOOL fCantDeny)
{
    HRESULT thr, hr = S_OK;
    ULONG cRowsObt, i;
    HROW * phRow;
    int iDummy=0;
    // We need a pointer to our containing class..
    CRowPosition *pCRP = CONTAINING_RECORD(this, CRowPosition, _RowsetNotify);
    
    // We only care about the DELETE notification because,
    // only the Delete notification can change the current position.
    if (eReason!=DBREASON_ROW_DELETE)
    {    
        hr = DB_S_UNWANTEDREASON;
        goto Cleanup;
    }

    if (!pCRP->_pRPCSink)               // Do we have any sinks?
    {
        goto Cleanup;                   // No, pointless to continue.
    }

    // For each hRow in the ahRows array,
    for (i=0; i!=cRows; i++)
    {
        // ignore if it doesn't match our current row.
        if (ahRows[i] != pCRP->_hRow)
            continue;
            
        // Our "current row" is the one being deleted.
        // Propogate the notifications to our clients.
        thr = pCRP->FireRowPositionChange(eReason, ePhase, fCantDeny,
                                          &iDummy);

        // Vetos are allowed only when our source says so..
        if (!fCantDeny && thr==S_FALSE )
        {
            hr = S_FALSE; // our client veto'd this change

            // Won't need bookmark anymore (if we had one).
            // (Note if our source is behaving properly, it should send
            // us a FAILEDTODO event, which would also clear the bookmark,
            // but its probably more reliable to not depend on this).
            pCRP->ClearBookmark();
        }
        else
        {
            switch (ePhase)
            {
              case (DBEVENTPHASE_ABOUTTODO):
                // Get a bookmark for the current hRow.
                pCRP->ClearBookmark(); // kill any previous

                // Try to get a bookmark.  If we fail, the
                // DIDEVENT phase is robust, so ignore HR here.
                pCRP->_pRowset->GetData(pCRP->_hRow, pCRP->_hAccessorBookmark,
                                         &pCRP->_Bookmark);
                break;

              case(DBEVENTPHASE_DIDEVENT):
                phRow = &pCRP->_hRow;
                // Make sure to decrement refcount of old _hRow..
                if (*phRow!=NULL)
                {
                    pCRP->_pRowset->ReleaseRows(1, phRow, NULL, NULL, NULL);
                    *phRow = NULL; // Null it out now.
                }

                // Make sure we really have a bookmark.
                if (!pCRP->_Bookmark.ptr) break;

                if (!pCRP->_pRowsetLocate) break;

                thr = pCRP->_pRowsetLocate->GetRowsAt(NULL, NULL,
                                                      pCRP->_Bookmark.size,
                                                      (BYTE *)pCRP->_Bookmark.ptr,
                                                      0, // lRowsOffset 
                                                      1, // cRows [in]
                                                      &cRowsObt, // [out]
                                                      &phRow);

                if (thr==DB_S_ENDOFROWSET) // off the end?
                {
                    // Hmm, try it backwards this time.
                    pCRP->_pRowsetLocate->GetRowsAt(NULL, NULL, 
                                                    pCRP->_Bookmark.size,
                                                    (BYTE *)pCRP->_Bookmark.ptr,
                                                    0, // lRowsOffset 
                                                    -1,  // cRows [in]
                                                    &cRowsObt, //[out]
                                                    &phRow);
                } // DB_S_ENDOFROWSET

                pCRP->ClearBookmark(); // We're done with the bookmark
                break;

              case(DBEVENTPHASE_FAILEDTODO):
                // Something failed, make sure bookmark is cleared.
                  pCRP->ClearBookmark();
                  break;

            } // switch ePhase

        } // else for change veto'd

        break;                      // out of for loop..

    }                                   // for (ULONG i=0; i!=cRows; i++)

Cleanup:
    return hr;
}

//+---------------------------------------------------------------------------
//  Member:     MyRowsetNotify::OnRowsetChange
//
//  Synopsis:   Implementation of IRowsetNotify::OnRowsetChange
//              Currently, the only eReason is ROWSET_RELEASE.  We can never get
//              one of these because if we're still connected, the row position
//              can't go away.
//
//  Arguments:  hRow            new hRow position
//
//  Returns:    S_OK                if everything is fine,
//
STDMETHODIMP
CRowPosition::MyRowsetNotify::OnRowsetChange (IRowset *pRowset,
                                              DBREASON eReason,
                                              DBEVENTPHASE ePhase,
                                              BOOL fCantDeny)
{
    Assert(eReason!=DBREASON_ROWSET_RELEASE && "Impossible event");
    return DB_S_UNWANTEDREASON;

#ifdef never
    CRowPosition *pCRP = CONTAINING_RECORD(this, CRowPosition, _RowsetNotify);

    // The whole rowset is going away.
    // Propagate the notification to our clients.
    IGNORE_HR(pCRP->FireRowPositionChange(eReason, ePhase, fCantDeny));
    
    // Make sure to decrement refcount of our _hRow so rowset cleans up
    // nicely.
    if (pCRP->_hRow!=NULL)
    {
        IGNORE_HR(pCRP->_pRowset->ReleaseRows(1, &pCRP->_hRow,
                                              NULL, NULL, NULL));
        pCRP->_hRow = NULL;
    }
    return S_OK;
#endif    
}

//+------------------------------------------------------------------------
//
//  Member:     CreateRowPosition
//
//  Synopsis:   Creates a new RowPosition instance.
//
//  Arguments:  pUnkOuter   Outer unknown -- must be NULL
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------


#endif

#ifdef UNIX
CBase * STDMETHODCALLTYPE
CreateRowPosition(IUnknown * pUnkOuter)
{
    CBase * pBase;
    Assert(!pUnkOuter);
    pBase = new CRowPosition();
    return(pBase);
}
#endif
