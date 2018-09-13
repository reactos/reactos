//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       connect.cxx
//
//  Contents:   Connection points.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_OLECTL_H_
#define X_OLECTL_H_
#include <olectl.h>
#endif


MtDefine(CConnectionPointContainer, ObjectModel, "CConnectionPointContainer")
MtDefine(CCPCEnumCPAry, Locals, "CConnectionPointContainer::EnumConnectionPoints CPtrAry<IConnectionPoint *>")
MtDefine(CCPCEnumCPAry_pv, CCPCEnumCPAry, "CConnectionPointContainer::EnumConnectionPoints CPtrAry<IConnectionPoint *>::_pv")
MtDefine(CConnectionPtEnumConnectionsAry_pv, Locals, "CConnectionPt::EnumConnections CPtrAry<LPUNKNOWN>::_pv")

DECLARE_CPtrAry(CCPCEnumCPAry, IConnectionPoint *, Mt(CCPCEnumCPAry), Mt(CCPCEnumCPAry_pv))

EXTERN_C const IID DIID_HTMLElementEvents;
EXTERN_C const IID DIID_HTMLControlElementEvents;
EXTERN_C const IID IID_IControlEvents;

#ifdef _WIN64
DWORD   g_dwCookieForWin64 = 0;
#endif

//+---------------------------------------------------------------------------
//
//  Method:     CBase::DoAdvise
//
//  Synopsis:   Implements IConnectionPoint::Advise.
//
//  Arguments:  [ppUnkSinkOut]    -- Pointer to outgoing sink ptr.
//              [iid]             -- Interface of the advisee.
//              [fEvent]          -- Whether it's an event iface advise
//              [pUnkSinkAdvise]  -- Advisee's sink.
//              [pdwCookie]       -- Cookie to identify connection.
//
//----------------------------------------------------------------------------

HRESULT
CBase::DoAdvise(
    REFIID          iid,
    DISPID          dispidBase,
    IUnknown *      pUnkSinkAdvise,
    IUnknown **     ppUnkSinkOut,
    DWORD *         pdwCookie)
{
    HRESULT                     hr;
    IUnknown                   *pUnk = NULL;
    CAttrValue::AAExtraBits     wAAExtra = CAttrValue::AA_Extra_Empty;
    DWORD                       dwCookie;

    Assert(iid != IID_NULL);
    
    if (!pUnkSinkAdvise)
        RRETURN(E_INVALIDARG);

    hr = THR_NOTRACE(pUnkSinkAdvise->QueryInterface(iid, (void **) &pUnk));
    if (hr)
    {
        // If someone is trying to listen to IE4 bad guid for IHTMLControlElementEvents the IID
        // was changed to be unique.  Java VM needs to be advised on this interface not IDispatch
        // (it can't handle it although it should).
        if (iid == DIID_HTMLControlElementEvents)
        {
            hr = THR_NOTRACE(pUnkSinkAdvise->QueryInterface(IID_IControlEvents, (void **) &pUnk));
        }

        if (hr)
        {
            if (dispidBase == DISPID_A_EVENTSINK)
		    {
		        hr = THR(pUnkSinkAdvise->QueryInterface(IID_IDispatch, (void **) &pUnk));
		    }

            if (hr)
                RRETURN1(CONNECT_E_CANNOTCONNECT, CONNECT_E_CANNOTCONNECT);
        }
    }


    if (iid == IID_ITridentEventSink)
    {
        // Quick VBS event hookup.
        wAAExtra = CAttrValue::AA_Extra_TridentEvent;
    }
    // Any event IIDs between 0x305f60e and max UUID of 0x30c38c70 would support
    // new style event binding of the eventObj being a parameter when the event is
    // fired.
    else if (iid.Data1 >= 0x3050f60f && iid.Data1 <= 0x30c38c70)
    {
        // is the rest of GUID in the Trident range?  If not then we're old style
        // event hookup (no eventObj argument
        if (!memcmp(&DIID_HTMLElementEvents + sizeof(DWORD),
                    &(iid.Data2),
                    sizeof(REFIID) - sizeof(DWORD)))
        {
            // Old style Trident event hookup w/o EventObj as a parameter.
            wAAExtra = CAttrValue::AA_Extra_OldEventStyle;
        }
    }
    else
    {
        // Old style Trident event hookup w/o EventObj as a parameter.
        wAAExtra = CAttrValue::AA_Extra_OldEventStyle;
    }

    hr = THR(AddUnknownObjectMultiple(dispidBase, pUnk, CAttrValue::AA_Internal, wAAExtra));
    if (hr)
        goto Cleanup;

    if (ppUnkSinkOut)
    {
        *ppUnkSinkOut = pUnk;
        pUnk->AddRef();
    }

#ifdef _WIN64
    AAINDEX aaidx;
    aaidx = AA_IDX_UNKNOWN;
    Verify(FindAdviseIndex(dispidBase, 0, pUnk, &aaidx) == S_OK);    // We just added it, so this better not fail
    dwCookie = (DWORD)InterlockedIncrement((LONG *)&g_dwCookieForWin64);
    if (aaidx != AA_IDX_UNKNOWN)
    {
        Verify(SetCookieAt(aaidx, dwCookie) == S_OK);
    }
#else
    dwCookie = (DWORD)pUnk;
#endif

    if (pdwCookie)
    {
        *pdwCookie = dwCookie;
    }

Cleanup:
    ReleaseInterface(pUnk);
    RRETURN1(hr, CONNECT_E_ADVISELIMIT);
}


//+---------------------------------------------------------------------------
//
//  Method:     CBase::DoUnadvise
//
//  Synopsis:   Implements IConnectionPoint::Unadvise.
//
//  Arguments:  [dwCookie]        -- Cookie handed out upon Advise.
//
//----------------------------------------------------------------------------

HRESULT
CBase::DoUnadvise(DWORD dwCookie, DISPID dispidBase)
{
    HRESULT hr;
    AAINDEX aaidx;
    
    if (!dwCookie)
        return S_OK;

#ifdef _WIN64
    hr = THR(FindAdviseIndex(dispidBase, dwCookie, NULL, &aaidx));
#else
    hr = THR(FindAdviseIndex(dispidBase, 0, (IUnknown *)dwCookie, &aaidx));
#endif
    if (hr)
        goto Cleanup;

    DeleteAt(aaidx);

Cleanup:
    RRETURN1(hr, CONNECT_E_NOCONNECTION);
}

HRESULT
CBase::FindAdviseIndex(DISPID dispidBase, DWORD dwCookie, IUnknown * pUnkCookie, AAINDEX * paaidx)
{
    HRESULT     hr       = S_OK;
    IUnknown *  pUnkSink = NULL;
    AAINDEX     aaidx    = AA_IDX_UNKNOWN;

    if (!*GetAttrArray())
    {
        hr = CONNECT_E_NOCONNECTION;
        goto Cleanup;
    }

    //
    // Loop through the attr array indices looking for the dwCookie and/or
    // pUnkCookie as the value.
    //

    for (;;)
    {
        aaidx = FindNextAAIndex(
            dispidBase,
            CAttrValue::AA_Internal,
            aaidx);
        if (aaidx == AA_IDX_UNKNOWN)
            break;
            
        if (pUnkCookie)
        {
            ClearInterface(&pUnkSink);
            hr = THR(GetUnknownObjectAt(aaidx, &pUnkSink));
            if (hr || !pUnkSink)
            {
                hr = CONNECT_E_NOCONNECTION;
                goto Cleanup;
            }

            if (pUnkSink == pUnkCookie)
                break;
        }

#ifdef _WIN64
        if (dwCookie)
        {
            DWORD dwCookieSink;
            hr = THR(GetCookieAt(aaidx, &dwCookieSink));
            if (dwCookie == dwCookieSink)
                break;
        }
#endif
    }

    if (aaidx == AA_IDX_UNKNOWN)
    {
        hr = CONNECT_E_NOCONNECTION;
        goto Cleanup;
    }

    *paaidx = aaidx;

Cleanup:
    ReleaseInterface(pUnkSink);
    RRETURN1(hr, CONNECT_E_NOCONNECTION);
}

//+------------------------------------------------------------------------
//
//  CConnectionPointContainer implementation
//
//-------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::CConnectionPointContainer
//
//  Synopsis:   ctor.
//
//  Arguments:  [pBase]     -- CBase object using this container.
//
//----------------------------------------------------------------------------

CConnectionPointContainer::CConnectionPointContainer(CBase * pBase, IUnknown *pUnkOuter)
{
    CONNECTION_POINT_INFO * pcpi;
    long            i;
    
    MemSetName((this, "CPC pBase=%08x", pBase));

    Assert(pBase);
    Assert(pBase->BaseDesc()->_pcpi);

    for (i = 0; i < CONNECTION_POINTS_MAX; i++)
    {
        _aCP[i]._index = -1;
    }
    
    //
    // Note: we set the refcount to 0 because the caller of this function will
    // AddRef it as part of its implementation of QueryInterface.
    //

    _ulRef      = 0;
    _pBase      = pBase;
    _pUnkOuter  = pUnkOuter;

    // _pUnkOuter is specified explicitly when creating a new
    // COM identity on top of the CBase object. In this case,
    // we only need to SubAddRef the CBase object.

    if (_pUnkOuter)
    {
        _pBase->SubAddRef();
        _pUnkOuter->AddRef();
    }
    else
    {
        _pBase->PrivateAddRef();
        _pBase->PunkOuter()->AddRef();
    }
    
    for (i = 0, pcpi = GetCPI(); pcpi && pcpi->piid; pcpi++, i++)
    {
        _aCP[i]._index = i;
    }

    Assert(i < CONNECTION_POINTS_MAX - 1);
}

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::~CConnectionPointContainer
//
//  Synopsis:   dtor.
//
//----------------------------------------------------------------------------

CConnectionPointContainer::~CConnectionPointContainer()
{
    // _pUnkOuter is specified explicitly when creating a new
    // COM identity on top of the CBase object. In this case,
    // we only need to SubRelease the CBase object.

    if (_pUnkOuter)
    {
        _pBase->SubRelease();
        _pUnkOuter->Release();
    }
    else
    {
        _pBase->PrivateRelease();
        _pBase->PunkOuter()->Release();
    }
}

//+------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::QueryInterface, IUnknown
//
//  Synopsis:   As per IUnknown.
//
//-------------------------------------------------------------------------

STDMETHODIMP
CConnectionPointContainer::QueryInterface(REFIID iid, void **ppvObj)
{
    IUnknown *pUnk = _pUnkOuter ? _pUnkOuter : _pBase->PunkOuter();
    RRETURN_NOTRACE(THR_NOTRACE(pUnk->QueryInterface(iid, ppvObj)));
}


//+------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::AddRef, IUnknown
//
//  Synopsis:   As per IUnknown.
//
//-------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CConnectionPointContainer::AddRef()
{
    return _ulRef++;
}



//+------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::Release, IUnknown
//
//  Synopsis:   As per IUnknown.
//
//-------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CConnectionPointContainer::Release()
{
    Assert(_ulRef > 0);

    if (--_ulRef == 0)
    {
        _ulRef = ULREF_IN_DESTRUCTOR;
        delete this;
        return 0;
    }

    return _ulRef;
}



//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::EnumConnectionPoints
//
//  Synopsis:   Enumerates the container's connection points.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPointContainer::EnumConnectionPoints(IEnumConnectionPoints ** ppEnum)
{
    HRESULT                         hr;
    CCPCEnumCPAry *                 paCP;
    CConnectionPt *                 pCP;

    if (!ppEnum)
        RRETURN(E_POINTER);

    //
    // Copy pointers to connection points to new array.
    //

    *ppEnum = NULL;
    paCP = new CCPCEnumCPAry;
    if (!paCP)
        RRETURN(E_OUTOFMEMORY);

    for (pCP = _aCP; pCP->_index != -1; pCP++)
    {
        hr = THR(paCP->Append(pCP));
        if (hr)
            goto Error;

        pCP->AddRef();
    }

    //
    // Create enumerator which references array.
    //

    hr = THR(paCP->EnumElements(
            IID_IEnumConnectionPoints,
            (LPVOID*)ppEnum,
            TRUE,
            FALSE,
            TRUE));
    if (hr)
        goto Error;

Cleanup:
    RRETURN(hr);

Error:
    paCP->ReleaseAll();
    delete paCP;
    goto Cleanup;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::FindConnectionPoint
//
//  Synopsis:   Finds a connection point with a particular IID.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPointContainer::FindConnectionPoint(
        REFIID              iid,
        IConnectionPoint ** ppCP)
{
    CConnectionPt * pCP;

    if (!ppCP)
        RRETURN(E_POINTER);

    *ppCP = NULL;

    for (pCP = _aCP; pCP->_index != -1; pCP++)
    {
        if (*(pCP->MyCPI()->piid) == iid)
        {
            *ppCP = pCP;
            pCP->AddRef();
            return S_OK;
        }
    }

    // If someone is trying to listen to IE4 bad guid for IHTMLControlElementEvents the IID
    // was changed to be unique.  IE4 shipped with IControlEvents for the guid of
    // IControlEvents so we'll map that interface to the correct IE5 IHTMLControlElementEvents
    // IID.
    if (iid == IID_IControlEvents)
    {
        for (pCP = _aCP; pCP->_index != -1; pCP++)
        {
            if (*(pCP->MyCPI()->piid) == DIID_HTMLControlElementEvents)
            {
                *ppCP = pCP;
                pCP->AddRef();
                return S_OK;
            }
        }
    }

    RRETURN(E_NOINTERFACE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPointContainer::GetCPI
//
//  Synopsis:   Get a hold of the array of connection pt information.
//
//----------------------------------------------------------------------------

CONNECTION_POINT_INFO *
CConnectionPointContainer::GetCPI()
{
    return (CONNECTION_POINT_INFO *)(_pBase->BaseDesc()->_pcpi);
}


//+------------------------------------------------------------------------
//
//  CConnectionPt implementation
//
//-------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::QueryInterface
//
//  Synopsis:   Per IUnknown.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPt::QueryInterface(REFIID iid, LPVOID * ppvObj)
{
    if (!ppvObj)
        RRETURN(E_INVALIDARG);

    if (iid == IID_IUnknown || iid == IID_IConnectionPoint)
    {
        *ppvObj = (IConnectionPoint *) this;
    }
    else
    {
        *ppvObj = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *) *ppvObj)->AddRef();
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::AddRef
//
//  Synopsis:   Per IUnknown.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CConnectionPt::AddRef()
{
    MyCPC()->_ulRef++;
    return MyCPC()->_pBase->SubAddRef();
}



//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::Release
//
//  Synopsis:   Per IUnknown.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CConnectionPt::Release()
{
    CBase * pBase;

    Assert(MyCPC()->_ulRef > 0);

    pBase = MyCPC()->_pBase;
    if (--MyCPC()->_ulRef == 0)
    {
        MyCPC()->_ulRef = ULREF_IN_DESTRUCTOR;
        delete MyCPC();
    }

    return pBase->SubRelease();
}



//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::GetConnectionInterface
//
//  Synopsis:   Returns the IID of this connection point.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPt::GetConnectionInterface(IID * pIID)
{
    if (!pIID)
        RRETURN(E_POINTER);

    Assert(_index != -1);
    *pIID = *MyCPI()->piid;
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::GetConnectionPointContainer
//
//  Synopsis:   Returns the container of this connection point.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPt::GetConnectionPointContainer(IConnectionPointContainer ** ppCPC)
{
    Assert(_index != -1);
    
    if (!ppCPC)
        RRETURN(E_POINTER);

    if (MyCPC()->_pBase->PunkOuter() != (IUnknown *) MyCPC()->_pBase)
    {
        RRETURN(MyCPC()->_pBase->PunkOuter()->QueryInterface(
                IID_IConnectionPointContainer, (void **) ppCPC));
    }

    *ppCPC = MyCPC();
    (*ppCPC)->AddRef();

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::Advise
//
//  Synopsis:   Adds a connection.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPt::Advise(LPUNKNOWN pUnkSink, DWORD * pdwCookie)
{
    Assert(_index != -1);

    const CONNECTION_POINT_INFO * pcpi = MyCPI();
    
    RRETURN(MyCPC()->_pBase->DoAdvise(
        *pcpi->piid,
        pcpi->dispid,
        pUnkSink,
        NULL,
        pdwCookie));
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::Unadvise
//
//  Synopsis:   Removes a connection.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPt::Unadvise(DWORD dwCookie)
{
    Assert(_index != -1);
    
    RRETURN(MyCPC()->_pBase->DoUnadvise(
        dwCookie,
        MyCPI()->dispid));
}


//+---------------------------------------------------------------------------
//
//  Member:     CConnectionPt::EnumConnections
//
//  Synopsis:   Enumerates the connections.
//
//----------------------------------------------------------------------------

STDMETHODIMP
CConnectionPt::EnumConnections(LPENUMCONNECTIONS * ppEnum)
{
    Assert(_index != -1);
    
    HRESULT                 hr;
    DISPID                  dispidBase;
    CEnumConnections *      pEnum   = NULL;
    CDataAry<CONNECTDATA>   aryCD(Mt(CConnectionPtEnumConnectionsAry_pv));
    CONNECTDATA             cdNew;
    AAINDEX                 aaidx = AA_IDX_UNKNOWN;
    
    cdNew.pUnk = NULL;

    if (!ppEnum)
        RRETURN(E_POINTER);

    if (*(MyCPC()->_pBase->GetAttrArray()))
    {
        dispidBase = MyCPI()->dispid;

        //
        // Iterate thru attr array values with this dispid, appending
        // the IUnknown into a local array.
        //

        for (;;)
        {
            aaidx = MyCPC()->_pBase->FindNextAAIndex(
                dispidBase,
                CAttrValue::AA_Internal,
                aaidx);
            if (aaidx == AA_IDX_UNKNOWN)
                break;
                
            ClearInterface(&cdNew.pUnk);
            hr = THR(MyCPC()->_pBase->GetUnknownObjectAt(
                    aaidx, 
                    &cdNew.pUnk));
            if (hr)
                goto Cleanup;

#ifdef _WIN64
            Verify(MyCPC()->_pBase->GetCookieAt(aaidx, &cdNew.dwCookie) == S_OK);
#else
            cdNew.dwCookie = (DWORD)cdNew.pUnk;
#endif

            hr = THR(aryCD.AppendIndirect(&cdNew));
            if (hr)
                goto Cleanup;
        }
    }

    hr = THR(CEnumConnections::Create(aryCD.Size(), (CONNECTDATA *)(LPVOID)aryCD, &pEnum));

    //  Note that since our enumeration base class doesn't officially
    //    support the IEnumConnections interface, we have to do
    //    an explicit cast via void *

    *ppEnum = (IEnumConnections *) (void *) pEnum;

Cleanup:
    ReleaseInterface(cdNew.pUnk);
    RRETURN(hr);
}

