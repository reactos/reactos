/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    clsfact.cpp

Abstract:

    This module contains the class factory implmentation for the published app
    query form.

Author:

    Dave Hastings (daveh) creation-date 08-Nov-1997

Revision History:

--*/

#include "pubquery.h"

//
// Creator/Destructor
//
CPublishedApplicationQueryFormClassFactory::CPublishedApplicationQueryFormClassFactory()
{
    InterlockedIncrement(&g_RefCount);
}

CPublishedApplicationQueryFormClassFactory::~CPublishedApplicationQueryFormClassFactory()
{
    InterlockedDecrement(&g_RefCount);
}

//
// IUnknown
//
STDMETHODIMP CPublishedApplicationQueryFormClassFactory::QueryInterface(
    REFIID riid,
    PVOID *ppvInterface
    )
/*++

Routine Description:

    This is the query interface function for the class factory.

Arguments:

    riid - Supplies the iid of the interface desired.
    ppvInterface - Returns the interface pointer.

Return Value:

--*/
{
    INTERFACES iface[] =
    {
        &IID_IClassFactory, (LPCLASSFACTORY)this,
    };

    return HandleQueryInterface(riid, ppvInterface, iface, ARRAYSIZE(iface));
}

STDMETHODIMP_(ULONG) CPublishedApplicationQueryFormClassFactory::AddRef(
    VOID
    )
/*++

Routine Description:

    This is the AddRef entrypoint for the class factory.

Arguments:

    None.

Return Value:

    New reference count

--*/

{
    return InterlockedIncrement(&m_RefCount);
}

STDMETHODIMP_(ULONG) CPublishedApplicationQueryFormClassFactory::Release(
    VOID
    )
/*++

Routine Description:

    This is the Release function for the class factory.

Arguments:

    None.

Return Value:

    the new ref count

--*/
{
    LONG NewRefCount;

    NewRefCount = InterlockedDecrement(&m_RefCount);

    if (NewRefCount == 0) {
        delete this;
    }

    return NewRefCount;
}

//
// IClassFactory
//
STDMETHODIMP CPublishedApplicationQueryFormClassFactory::CreateInstance(
    IUnknown *pUnkOuter,
    REFIID riid,
    PVOID * ppvInterface
    )
/*++

Routine Description:

    This is the CreateInstance function for the class factory.
    This class does not support aggregation.

Arguments:

    pUnkOuter -- Supplies a pointer to the controlling IUnknown.
        Since this object doesn't support aggregation, this must be
        NULL
    riid -- Supplies the IID of the desired interface.
    ppvInterface -- Returns the requested interface.

Return Value:


--*/
{
    HRESULT hr;
    CPublishedApplicationQuery *PublishedApplicationQuery;

    if (ppvInterface == NULL) {
        ExitGracefully(hr, E_INVALIDARG, "ppvInterface is NULL");
    }

    if (pUnkOuter != NULL) {
        ExitGracefully(hr, CLASS_E_NOAGGREGATION, "Aggregation is not supported");
    }

    PublishedApplicationQuery = new CPublishedApplicationQuery;

    if (PublishedApplicationQuery == NULL) {
        ExitGracefully(hr, E_OUTOFMEMORY, "Failed to create CPublishedApplicationQueryForm");
    }

    hr = PublishedApplicationQuery->QueryInterface(riid, ppvInterface);

    if (FAILED(hr)) {
        delete PublishedApplicationQuery;
    }

exit_gracefully:
    return hr;
}

STDMETHODIMP CPublishedApplicationQueryFormClassFactory::LockServer(
    BOOL fLock
    )
/*++

Routine Description:

    This is the LockServer function for the class factory.  It doesn't do 
    anything.  

Arguments:

    fLock - Indicates whether to lock or unlock the server.

Return Value:

    S_OK
--*/
{
    return S_OK;
}