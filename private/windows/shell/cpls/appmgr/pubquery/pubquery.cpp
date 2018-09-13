/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    pubform.cpp

Abstract:

    This module contains the implementation for the PublishedApplicationQueryForm.

Author:

    Dave Hastings (daveh) creation-date 11-Nov-1997

Revision History:

--*/
#include "pubquery.h"


//
// creator/destructor
//
CPublishedApplicationQuery::CPublishedApplicationQuery()
{
    InterlockedIncrement(&g_RefCount);
    m_QueryForm = NULL;
}

CPublishedApplicationQuery::~CPublishedApplicationQuery()
{
    InterlockedDecrement(&g_RefCount);
}
//
// IUnknown
//
STDMETHODIMP CPublishedApplicationQuery::QueryInterface(
    REFIID riid,
    PVOID *ppvInterface
    )
/*++

Routine Description:

    This is the query interface function for the query.
    We don't use the standard query interface, because
    this lets us construct the interfaces on the fly

Arguments:

    riid - Supplies the iid of the interface desired.
    ppvInterface - Returns the interface pointer.

Return Value:

--*/
{
    HRESULT hr;

    *ppvInterface = NULL;
    hr = S_OK;

    if (IsEqualIID(riid, IID_IUnknown)) {
        *ppvInterface = (LPUNKNOWN)this;
    } else if (IsEqualIID(riid, IID_IQueryForm)) {

        //
        // if we don't already have an IQueryForm, create one
        //
        if (m_QueryForm == NULL) {
            m_QueryForm = new CPublishedApplicationQueryForm((LPUNKNOWN)this);

            if (m_QueryForm == NULL) {
                ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create CPublishedAppQueryForm");
            }
        } 
        *ppvInterface = (IQueryForm *)m_QueryForm;

    } else  if (IsEqualIID(riid, IID_IQueryHandler)) {
        
        //
        // if we don't already have an IQueryHandler, create one
        //
        if (m_QueryHandler == NULL) {
            
            m_QueryHandler = new CPublishedApplicationQueryHandler((LPUNKNOWN)this);

            if (m_QueryHandler == NULL) {
                ExitGracefully(hr, E_OUTOFMEMORY, "Unable to create CPublishedAppQueryHandler");
            }
        } 
        *ppvInterface = (IQueryHandler *)m_QueryHandler;

    } else {
        hr = E_NOINTERFACE;
    }

    if (SUCCEEDED(hr)) {
        ((LPUNKNOWN)*ppvInterface)->AddRef();
    }

exit_gracefully:

    return hr;

}

STDMETHODIMP_(ULONG) CPublishedApplicationQuery::AddRef(
    VOID
    )
/*++

Routine Description:

    This is the AddRef entrypoint for the query form.

Arguments:

    None.

Return Value:

    New reference count

--*/

{
    return InterlockedIncrement(&m_RefCount);
}

STDMETHODIMP_(ULONG) CPublishedApplicationQuery::Release(
    VOID
    )
/*++

Routine Description:

    This is the Release function for the query form.

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
