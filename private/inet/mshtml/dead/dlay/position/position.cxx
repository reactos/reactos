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
#include "position.hxx"

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


CONNECTION_POINT_INFO CRowPosition::s_acpi[] =
{
    CPI_ENTRY(CRowPosition, IRowPositionChange, _pRPCSink, CPIF_ALLOWMULTIPLE)
};


CRowPosition::CLASSDESC CRowPosition::s_classdesc =
{
        NULL,                               // _pclsid
        0,                                  // _idrBase
        0,                                  // _apClsidPages
        ARRAY_SIZE(CRowPosition::s_acpi),   // _ccp
        CRowPosition::s_acpi,               // _pcpi
};

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
                                            new CConnectionPointContainer(this);
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

CBase::CLASSDESC *
CRowPosition::GetClassDesc () const
{
    return &s_classdesc;
}

//+---------------------------------------------------------------------------
//  Member:     Constructor (public member)
//
//  Synopsis:   Instantiate an CRowPosition
//
//  Arguments:  None
//
//  Returns:    None
CRowPosition::CRowPosition ()
{
    // Should we kill this constructor altogether?
    // Most real work is done by CBase::Init

    // 0-initialized by operator new; we'll put some assertions here
    //   double-check, although we could check all members. 
    Assert(_pRowset == NULL);
    Assert(_pRPCSink == NULL);
}

//+---------------------------------------------------------------------------
//  Member:     Create (STATIC public member)
//
//  Synopsis:   Generate a CRowPosition object, either by finding one in the
//              IRowset we've been passed, or by generating one of our own.
//
//  Arguments:  punkOuter        someday, when we're aggregatable
//              *pRowset         pointer to the rowset that hosts us
//              *ppRowPosition   pointer to the newly create RowPosition object
//  
//  Returns:    None
//
HRESULT
CRowPosition::Create(IUnknown *punkOuter,
                     IRowset *pRowset,
                     IRowPosition **ppRowPosition)
{
    HRESULT hr;
    IRowPositionSource *pRowPositionSource;
    CRowPosition *pCRowPosition=NULL;

    *ppRowPosition = NULL;     // for callers that don't check HRESULT

    Assert(pRowset && "Bad call to CRowPosition::Create");

    if (punkOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
        goto Cleanup;
    }

    hr = pRowset->QueryInterface(IID_IRowPositionSource, (void **)&pRowPositionSource);
    if (SUCCEEDED(hr))
    {
        // The Rowset provider that we're on top of does implement IRowPosition!
        // Return it's IRowPosition rather than ours.
        hr = pRowPositionSource->CreateRowPosition(punkOuter, ppRowPosition);
        pRowPositionSource->Release();
    }
    else if (hr==E_NOINTERFACE)
    {
        // Note that this path is not an error case, it merely means we're
        // simulating the IRowPosition interface for now..
        pCRowPosition = new CRowPosition(/* punkOuter */);
        if (!pCRowPosition)
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
        hr = pCRowPosition->Init(pRowset);
        // Check HR anyway, but there really isn't a failure mode now..
        if (FAILED(hr))
            goto Error;

        // Convert our object pointer to an interface pointer to return.
        *ppRowPosition = (IRowPosition *)pCRowPosition;
    }

Cleanup:
    return hr;

Error:
    delete pCRowPosition;
    goto Cleanup;
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
        ReleaseBookmarkAccessor();

        // Special code for our cleanup
        if (_hRow)
        {
            _pRowset->ReleaseRows(1, &_hRow, NULL,
                                  NULL, NULL);    
            _hRow = NULL;
        }
        ClearInterface(&_pRowsetLocate);
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
CRowPosition::Init(IRowset *pRowset)
{
    HRESULT hr;
    IConnectionPointContainer *pCPC = NULL;

    Assert(_pRowset==NULL && _pCP==NULL && _pRowsetLocate == NULL &&
           _hRow==NULL && _Bookmark.ptr==NULL && _pAccessor==NULL &&
           _hAccessorBookmark==NULL && "IRowPosition init'd more than once.");

    if (!pRowset)
    {
        hr = E_INVALIDARG;
        goto Error;
    }

    pRowset->AddRef();
    _pRowset = pRowset;

    hr = pRowset->QueryInterface(IID_IRowsetLocate, (void **)&_pRowsetLocate);
    if (FAILED(hr)) goto Error;

    hr = pRowset->QueryInterface(IID_IConnectionPointContainer, (void **)&pCPC);
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

//+---------------------------------------------------------------------------
//  Member:     SetRowPosition (public member)
//
//  Synopsis:   Changes the hRow that represents our current rowset position
//              Issues OKTODO (canceallable) and DIDEVENT notifications.
//              Does an AddRefRows on the new position and a release on the old!
//
//  Arguments:  hRow            new hRow position
//
//  Returns:    S_OK                if everything is fine,
//
STDMETHODIMP
CRowPosition::SetRowPosition(HROW hRow)
{
    HRESULT hr = S_OK;                  // in case _hRow == hRow
    int iProgress=0;                    // track # of sinks.
    int iDummy=0;

    if (_hRow != hRow)                  // optimize no-op case
    {
        //
        // We must fire 4 events on success, and FAILED_TODO on failure..
        //

        // First, OKTODO..
        hr = FireRowPositionChange(DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                   DBEVENTPHASE_OKTODO, 0, &iProgress);
        if (hr!=S_OK)
            goto Cancel;

        // Then ABOUTTODO
        hr = FireRowPositionChange(DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                                   DBEVENTPHASE_ABOUTTODO, 0, &iDummy);
        if (hr!=S_OK)
            goto Cancel;

        // Now we do our part of the work.
        
        // AddRef the new hRow
        if (hRow!=NULL) _pRowset->AddRefRows(1, &hRow, NULL, NULL);
        
        // Release the previous _hRow
        if (_hRow!=NULL) _pRowset->ReleaseRows(1, &_hRow, NULL, NULL, NULL);

        _hRow = hRow;               // make the change

        // Do the SYNCHAFTER event.  This cannot be cancelled or fail!
        FireRowPositionChange(DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                              DBEVENTPHASE_SYNCHAFTER, 1, &iDummy);

        iDummy = 0;
        // Do the DIDEVENT.  This cannot be cancelled or fail!
        FireRowPositionChange(DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                              DBEVENTPHASE_DIDEVENT, 1, &iDummy);
    }
    return hr;

Cancel:
    // If Change was cancelled, we fire a FAILEDTODO event.
    hr = FireRowPositionChange(DBREASON_ROWSET_FETCHPOSITIONCHANGE,
                               DBEVENTPHASE_FAILEDTODO, 1, &iProgress);
    return ERROR_CANCELLED;             // User cancelled
};

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
CRowPosition::GetRowPosition(HROW *phRow)
{
    if (_hRow)
        _pRowset->AddRefRows(1, &_hRow, NULL, NULL);
    *phRow = _hRow;
    return S_OK;
};

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
    IRowPositionChange *pRPC;           // Notification we fire.
    DWORD pdwCookie = 0;                // cookie for GetNextSink
    HRESULT hr = S_OK;                  // Init for no sink case.

    if (_pRPCSink)
    {
        while ((pRPC= (IRowPositionChange*)GetNextSink(_pRPCSink,
                                                       &pdwCookie)) != NULL)
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
        // If we fired events to all sinks without error or cancel.
        *piProgress = 0;
    }

Cleanup:
    return hr;
}

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
        if (ahRows[i] != (ePhase==DBEVENTPHASE_OKTODO ? pCRP->_hRow : _hRow))
            continue;
            
        switch (ePhase)
        {
        case DBEVENTPHASE_OKTODO:
            pCRP->ClearBookmark(); // kill any previous bookmarks
            _hRow = pCRP->_hRow;    // remember active hrow
        
            // Propagate the notifications to our clients.
            thr = pCRP->FireRowPositionChange(eReason, ePhase, fCantDeny,
                                             &iDummy);

            // Vetos are allowed only when our source says so..
            if (!fCantDeny && thr==S_FALSE )
            {
                hr = S_FALSE; // our client veto'd this change
            }
            break;
        
        case DBEVENTPHASE_ABOUTTODO:
            // Propagate the notifications to our clients.
            thr = pCRP->FireRowPositionChange(eReason, ePhase, fCantDeny,
                                             &iDummy);

            // Vetos are allowed only when our source says so..
            if (!fCantDeny && thr==S_FALSE )
            {
                hr = S_FALSE; // our client veto'd this change
            }
            else
            {
                // Try to get a bookmark.  If we fail, the
                // DIDEVENT phase is robust, so ignore HR here.
                IGNORE_HR(pCRP->_pRowset->
                          GetData(pCRP->_hRow,
                                  pCRP->_hAccessorBookmark,
                                  &pCRP->_Bookmark));
          
            }
            break;

        case DBEVENTPHASE_SYNCHAFTER:
            phRow = &pCRP->_hRow;
            // Make sure to decrement refcount of old _hRow..
            if (*phRow!=NULL)
            {
                pCRP->_pRowset->ReleaseRows(1, phRow,
                                            NULL, NULL, NULL);
                *phRow = NULL; // Null it out now.
            }

            // If we have a bookmark, set the new position
            if (pCRP->_Bookmark.ptr)
            {
                thr = pCRP->_pRowsetLocate->
                          GetRowsAt(NULL, NULL,
                                    pCRP->_Bookmark.size,
                                    (BYTE *)pCRP->_Bookmark.ptr,
                                    0, // lRowsOffset 
                                    1, // cRows [in]
                                    &cRowsObt, // [out]
                                    &phRow);
                
                if (thr==DB_S_ENDOFROWSET) // off the end?
                {
                    // Hmm, try it backwards this time.
                    IGNORE_HR(pCRP->_pRowsetLocate->
                              GetRowsAt(NULL, NULL,
                                       pCRP->_Bookmark.size,
                                       (BYTE *)pCRP->_Bookmark.ptr,
                                       0, // lRowsOffset 
                                       -1,  // cRows [in]
                                       &cRowsObt, //[out]
                                       &phRow));
                } // DB_S_ENDOFROWSET

                pCRP->ClearBookmark(); // We're done with the bookmark
            }
        
            // Propagate the notifications to our clients.
            thr = pCRP->FireRowPositionChange(eReason, ePhase, fCantDeny,
                                             &iDummy);
            break;

        case DBEVENTPHASE_DIDEVENT:
            // Propagate the notifications to our clients.
            thr = pCRP->FireRowPositionChange(eReason, ePhase, fCantDeny,
                                             &iDummy);
            break;

        case(DBEVENTPHASE_FAILEDTODO):
            // Something failed, make sure bookmark is cleared.
            pCRP->ClearBookmark();
        
            // Propagate the notifications to our clients.
            thr = pCRP->FireRowPositionChange(eReason, ePhase, fCantDeny,
                                             &iDummy);
            break;

        } // switch ePhase

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

