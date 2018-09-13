//+------------------------------------------------------------------------
//
//  File:       cputil.cxx
//
//  Contents:   Connection point utilities.
//
//-------------------------------------------------------------------------

#include "headers.hxx"

ZERO_STRUCTS g_Zero;


//+---------------------------------------------------------------------------
//
//  Function:   ConnectSink
//
//  Synopsis:   Connects a sink to an object which fires to the sink through
//              a connection point.
//
//  Arguments:  [pUnkSource] -- The source object.
//              [iid]        -- The id of the connection point.
//              [pUnkSink]   -- The sink.
//              [pdwCookie]  -- Cookie that identifies the connection for
//                                  a later disconnect.  May be NULL.
//
//----------------------------------------------------------------------------

HRESULT
ConnectSink(
        IUnknown *  pUnkSource,
        REFIID      iid,
        IUnknown *  pUnkSink,
        DWORD *     pdwCookie)
{
    HRESULT                     hr;
    IConnectionPointContainer * pCPC;

    Assert(pUnkSource);
    Assert(pUnkSink);

    hr = THR(pUnkSource->QueryInterface(
            IID_IConnectionPointContainer, (void **) &pCPC));
    if (hr)
        RRETURN(hr);

    hr = THR(ConnectSinkWithCPC(pCPC, iid, pUnkSink, pdwCookie));
    pCPC->Release();
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   ConnectSinkWithCPC
//
//  Synopsis:   Connects a sink to an object which fires to the sink through
//              a connection point.
//
//  Arguments:  [pCPC]       -- The source object as an
//                                  IConnectionPointContainer.
//
//              [iid]        -- The id of the connection point.
//              [pUnkSink]   -- The sink.
//              [pdwCookie]  -- Cookie that identifies the connection for
//                                  a later disconnect.  May be NULL.
//
//----------------------------------------------------------------------------

HRESULT
ConnectSinkWithCPC(
        IConnectionPointContainer * pCPC,
        REFIID                      iid,
        IUnknown *                  pUnkSink,
        DWORD *                     pdwCookie)
{
    HRESULT             hr;
    IConnectionPoint *  pCP;
    DWORD               dwCookie;

    Assert(pCPC);
    Assert(pUnkSink);

    hr = ClampITFResult(THR_NOTRACE(pCPC->FindConnectionPoint(iid, &pCP)));
    if (hr)
        RRETURN1(hr, E_NOINTERFACE);

    if (!pdwCookie)
    {
        //
        // The CDK erroneously fails to handle a NULL cookie, so
        // we pass in a dummy one.
        //

        pdwCookie = &dwCookie;
    }

    hr = ClampITFResult(THR(pCP->Advise(pUnkSink, pdwCookie)));
    pCP->Release();
    RRETURN(hr);
}



//+---------------------------------------------------------------------------
//
//  Function:   DisconnectSink
//
//  Synopsis:   Disconnects a sink from a source.
//
//  Arguments:  [pUnkSource] -- The source object.
//              [iid]        -- The id of the connection point.
//              [pdwCookie]  -- Pointer to the cookie which identifies the
//                                  connection.
//
//  Modifies:   [*pdwCookie] - sets to 0 if disconnect is successful.
//
//----------------------------------------------------------------------------

HRESULT
DisconnectSink(
        IUnknown *  pUnkSource,
        REFIID      iid,
        DWORD *     pdwCookie)
{
    HRESULT                     hr;
    IConnectionPointContainer * pCPC    = NULL;
    IConnectionPoint *          pCP     = NULL;
    DWORD                       dwCookie;

    Assert(pUnkSource);
    Assert(pdwCookie);

    if (!*pdwCookie)
        return S_OK;

    hr = THR(pUnkSource->QueryInterface(
            IID_IConnectionPointContainer, (void **) &pCPC));
    if (hr)
        goto Cleanup;

    hr = ClampITFResult(THR(pCPC->FindConnectionPoint(iid, &pCP)));
    if (hr)
        goto Cleanup;

    //
    // Allow clients to use *pdwCookie as an indicator of whether
    // or not they are advised by setting it to zero before
    // calling Unadvise.  This prevents recursion in some
    // scenarios.
    //

    dwCookie = *pdwCookie;
    *pdwCookie = NULL;
    hr = ClampITFResult(THR(pCP->Unadvise(dwCookie)));
    if (hr)
    {
        *pdwCookie = dwCookie;
    }

Cleanup:
    ReleaseInterface(pCP);
    ReleaseInterface(pCPC);

    RRETURN(hr);
}



//
// Remove the #ifdev when this function is actually used.
//

#ifdef NEVER

//+---------------------------------------------------------------------------
//
//  Function:   DisconnectSinkWithoutCookie
//
//  Synopsis:   Disconnects a sink from a connection point without
//              using a cookie.
//
//  Arguments:  [pUnkSource] -- The connection point container.
//              [iid]        -- The IID of the connection point.
//
//----------------------------------------------------------------------------

HRESULT
DisconnectSinkWithoutCookie(
        IUnknown *  pUnkSource,
        REFIID      iid,
        IUnknown *  pUnkConnection)
{
    HRESULT                     hr;
    IConnectionPointContainer * pCPC    = NULL;
    IConnectionPoint *          pCP     = NULL;
    IEnumConnections *          pEC     = NULL;
    CONNECTDATA                 cd;

    Assert(pUnkSource);

    //
    // Find the connection point.
    //

    hr = THR(pUnkSource->QueryInterface(
            IID_IConnectionPointContainer, (void **) &pCPC));
    if (hr)
        goto Cleanup;

    hr = THR_NOTRACE(pCPC->FindConnectionPoint(iid, &pCP));
    if (hr)
        goto Cleanup;

    //
    // Find this connection.
    //

    hr = ClampITFResult(THR(pCP->EnumConnections(&pEC)));
    if (hr)
        goto Cleanup;

    while ((hr = THR_NOTRACE(pEC->Next(1, &cd, NULL))) == S_OK)
    {
        cd.pUnk->Release();
        if (cd.pUnk == pUnkConnection)
            break;
    }

    if (hr)
    {
        if (hr == S_FALSE)
            hr = E_FAIL;

        goto Cleanup;
    }

    // Disconnect.
    hr = ClampITFResult(THR(pCP->Unadvise(cd.dwCookie)));

Cleanup:
    ReleaseInterface(pEC);
    ReleaseInterface(pCP);
    ReleaseInterface(pCPC);

    RRETURN(hr);
}

#endif

//+---------------------------------------------------------------------------
//
//  Function:   GetDefaultDispatchTypeInfo
//
//  Synopsis:   Gets a default typeinfo of a coclass.
//
//  Arguments:  [pPCI]    -- Object containing coclass typeinfo.
//              [fSource] -- Is interface a source?
//              [ppTI]    -- Resulting typeinfo.
//
//  Returns:    HRESULT.
//
//  Modifies:   [ppTI].
//
//----------------------------------------------------------------------------

HRESULT
GetDefaultDispatchTypeInfo(IProvideClassInfo * pPCI,  BOOL fSource, ITypeInfo ** ppTI)
{
    HRESULT     hr;
    ITypeInfo * pTI = NULL;
    TYPEATTR *  pTA = NULL;
    WORD        w;
    int         implTypeFlags;
    HREFTYPE    hrt;
    ITypeInfo * pTIRef;
    TYPEATTR *  pTARef;
    int         implTypeFlagsDesired;

    Assert(pPCI);
    Assert(ppTI);

    *ppTI = NULL;
    hr = THR(pPCI->GetClassInfo(&pTI));
    if (hr)
        goto Cleanup;

    hr = THR(pTI->GetTypeAttr(&pTA));
    if (hr)
        goto Cleanup;

    if (pTA->typekind != TKIND_COCLASS)
    {
        hr = E_UNEXPECTED;
        goto Cleanup;
    }

    implTypeFlagsDesired = IMPLTYPEFLAG_FDEFAULT;
    if (fSource)
    {
        implTypeFlagsDesired |= IMPLTYPEFLAG_FSOURCE;
    }

    for (w = 0; !hr && w < pTA->cImplTypes; w++)
    {
        pTIRef = NULL;
        pTARef = NULL;

        hr = THR(pTI->GetImplTypeFlags(w, &implTypeFlags));
        if (hr)
            goto LoopCleanup;

        if (implTypeFlags != implTypeFlagsDesired)
            goto LoopCleanup;

        hr = THR(pTI->GetRefTypeOfImplType(w, &hrt));
        if (hr)
            goto LoopCleanup;

        hr = THR(pTI->GetRefTypeInfo(hrt, &pTIRef));
        if (hr)
            goto LoopCleanup;

        hr = THR(pTIRef->GetTypeAttr(&pTARef));
        if (hr)
            goto LoopCleanup;

        if (pTARef->typekind & TKIND_DISPATCH)
        {
            pTIRef->ReleaseTypeAttr(pTARef);
            *ppTI = pTIRef;
            goto Cleanup;
        }

LoopCleanup:
        if (pTARef)
            pTIRef->ReleaseTypeAttr(pTARef);
        ReleaseInterface(pTIRef);
    }

    hr = E_UNEXPECTED;

Cleanup:
    if (pTA)
        pTI->ReleaseTypeAttr(pTA);

    ReleaseInterface(pTI);
    RRETURN(hr);
}




