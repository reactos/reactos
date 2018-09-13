//**********************************************************************
// File name: cdropsrc.cxx
//
// Implementation file for CDropSource
// Implements the IDropSource interface required for an application to
// act as a Source in a drag and drop operation
// History :
//       Dec 23, 1997   [v-nirnay]    wrote it.
//
//
// Copyright (c) 1997-1999 Microsoft Corporation.
//**********************************************************************

#include <windows.h>
#include <ole2.h>

#include "olecomon.h"
#include "cdropsrc.h"

//**********************************************************************
// CDropSource::CDropSource
//
// Purpose:
//      Constructor
//
// Parameters:
//      None
// Return Value:
//      None
//**********************************************************************
CDropSource::CDropSource()
{
    TRACE(TEXT("Creating New Drop SOurce\n"));
    // Initialise reference count to 0
    m_cRef = 1;
}

//**********************************************************************
// CDropSource::QueryInterface
//
// Purpose:
//      Return a pointer to a requested interface
//
// Parameters:
//      REFIID riid         -   ID of interface to be returned
//      PPVOID ppv          -   Location to return the interface
//
// Return Value:
//      NOERROR             -   Interface supported
//      E_NOINTERFACE       -   Interface NOT supported
//**********************************************************************
STDMETHODIMP CDropSource::QueryInterface(REFIID riid,
                                         PPVOID ppv)
{
    // Initialise interface pointer to NULL
    *ppv = NULL;

    // If the interface asked for is what we have set ppv
    if (riid == IID_IUnknown || riid == IID_IDropSource) {
        *ppv = this;
    }

    // Increment reference count if the interface asked for is correct
    // and return no error
    if (*ppv != NULL) {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    // Else return no such interface
    return ResultFromScode(E_NOINTERFACE);
}

//**********************************************************************
// CDropSource::AddRef
//
// Purpose:
//      Increments the reference count for an interface on an object
//
// Parameters:
//      None
//
// Return Value:
//      int                 -   Value of the new reference count
//**********************************************************************
ULONG STDMETHODCALLTYPE CDropSource::AddRef()
{
    // Increment reference count
    return InterlockedIncrement((LONG*)&m_cRef);
}


//**********************************************************************
// CDropSource::Release
//
// Purpose:
//      Decrements the reference count for the interface on an object
//
// Parameters:
//      None
//
// Return Value:
//      int                 -   Value of the new reference count
//**********************************************************************
ULONG STDMETHODCALLTYPE CDropSource::Release()
{
    ULONG   ulTempCount;

    // Decrement reference count
    InterlockedDecrement((LONG*)&m_cRef);
    ulTempCount = m_cRef;

    // If there are no references then free current copy
    if (m_cRef == 0L) {
        delete this;
    }

    return ulTempCount;
}

//**********************************************************************
// CDropSource::QueryContinueDrag
//
// Purpose:
//      Determines whether a drag-and-drop operation should be continued,
//      canceled, or completed
//
// Parameters:
//      BOOL fEsc           -   Status of escape key since previous call
//      DWORD grfKeyState   -   Current state of keyboard modifier keys
//
// Return Value:
//      DRAGDROP_S_CANCEL   -   Drag operation is to be cancelled
//      DRAGDROP_S_DROP     -   Drop operation is to be completed
//      S_OK                -   Drag operation is to be continued
//**********************************************************************
STDMETHODIMP CDropSource::QueryContinueDrag(BOOL fEsc,
                                            DWORD grfKeyState)
{
    // If escape key is pressed stop drag and drop
    if (fEsc) {
        return ResultFromScode(DRAGDROP_S_CANCEL);
    }

    // If LButton is up then complete the drag operation
    if (!(grfKeyState & MK_LBUTTON)) {
        return ResultFromScode(DRAGDROP_S_DROP);
    }

    return ResultFromScode(S_OK);
}

//**********************************************************************
// CDropSource::GiveFeedback
//
// Purpose:
//      Enables a source application to give visual feedback to the end
//      user during a drag-and-drop
//
// Parameters:
//      DWORD dwEffect      -   Effect of a drop operation
//
// Return Value:
//      DRAGDROP_S_USEDEFAULTCURSORS    -   Use default cursors
//**********************************************************************
STDMETHODIMP CDropSource::GiveFeedback(DWORD dwEffect)
{
    return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);
}

