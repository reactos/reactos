//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       undo.cxx
//
//  Contents:   Implementation of Undo classes
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

//+---------------------------------------------------------------------------
//
//  CUndoNewControl Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoDeleteControl::CUndoDeleteControl, public
//
//  Synopsis:   CUndoDeleteControl ctor
//
//  Arguments:  [pDoc] -- Pointer to owner form
//
//----------------------------------------------------------------------------

CUndoDeleteControl::CUndoDeleteControl(CSite * pSite)
    : CUndoUnitBase(pSite, IDS_UNDODELETE)
{
    TraceTag((tagUndo, "CUndoDeleteControl ctor"));

    _pDOBag = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoDeleteControl::CUndoDeleteControl, public
//
//  Synopsis:   CUndoDeleteControl dtor
//
//----------------------------------------------------------------------------

CUndoDeleteControl::~CUndoDeleteControl()
{
    ReleaseInterface(_pDOBag);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoDeleteControl::Init, public
//
//  Synopsis:   Initializes the undo object.
//
//  Arguments:  [parySiteDelete] -- Array of sites that are going to be
//                                  deleted.
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoDeleteControl::Init(int c, CSite ** ppSite)
{
    // BUGBUG
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoDeleteControl::Do, public
//
//  Synopsis:   Performes an undo of a control deletion.
//
//  Arguments:  [pUM] -- Pointer to undo manager
//
//  Returns:    HRESULT
//
//----------------------------------------------------------------------------

HRESULT
CUndoDeleteControl::Do(IOleUndoManager * pUM)
{
    HRESULT    hr;
    IStorage * pStg      = NULL;
    DWORD      dwCookie;
    CDoc *    pDoc = Site()->_pDoc;

    if (!pUM)
    {
        hr = pDoc->BlockNewUndoUnits(&dwCookie);
        if (hr)
            goto Cleanup;
    }

//    hr = THR(GetcfPrivateFmt(_pDOBag, &pStg));
 //   if (hr)
  //      goto Cleanup;

    //
    // This puts a Redo action on the stack.
    //
    hr = S_OK;
    //BUGBUG: (alexz) reconsider this when Bag is gone away and undo works
    //hr = THR(Site()->InsertDataObject(_pDOBag, NULL));

    pDoc->OnControlInfoChanged();

    if (!pUM)
    {
        pDoc->UnblockNewUndoUnits(dwCookie);
    }

    pDoc->UndoManager()->DiscardFrom(NULL);

Cleanup:
    ReleaseInterface(pStg);
    ClearInterface(&_pDOBag);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CUndoNewControl Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoNewControl::CUndoNewControl, public
//
//  Synopsis:   CUndoNewControl ctor
//
//----------------------------------------------------------------------------

CUndoNewControl::CUndoNewControl(CSite * pSite, CSite * pSiteNew)
    : CUndoUnitBase(pSite->_pDoc, IDS_UNDONEWCTRL)
{
    TraceTag((tagUndo, "CUndoNewControl ctor"));

    _arySites.Append(pSiteNew);
}

CUndoNewControl::CUndoNewControl(CSite * pSite, int c, CSite ** ppSite)
    : CUndoUnitBase(pSite->_pDoc, IDS_UNDONEWCTRL)
{
    TraceTag((tagUndo, "CUndoNewControl ctor"));

    //  BUGBUG what about OOM? (chrisz)

    _arySites.CopyIndirect(c, ppSite, FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoNewControl::Do, public
//
//  Synopsis:   Performs an undo of adding a new control. Deletes the control
//              that was added.
//
//----------------------------------------------------------------------------

HRESULT
CUndoNewControl::Do(IOleUndoManager * pUndoMgr)
{
    TraceTag((tagUndo, "CUndoNewControl::Do"));

    HRESULT     hr = S_OK;
    DWORD       dwCookie;
    CDoc *      pDoc = Site()->_pDoc;
    CDoc::CLock Lock(pDoc);

    if (!pUndoMgr)
    {
        hr = pDoc->BlockNewUndoUnits(&dwCookie);
        if (hr)
            goto Cleanup;
    }

    //
    // Puts a redo action on the stack.
    //
    // BUGBUG (rodc) Fix when Undo object takes sites instead of forms.
    // hr = pDoc->EditDelete(_arySites.Size(), _arySites);
    Site()->ShowLastErrorInfo(hr);

    if (!pUndoMgr)
    {
        pDoc->UnblockNewUndoUnits(dwCookie);
    }

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  CUndoMove Implementation
//
//----------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Member:     CUndoMove::CUndoMove, public
//
//  Synopsis:   CUndoMove ctor
//
//----------------------------------------------------------------------------

CUndoMove::CUndoMove(CSite * pSite, RECT *prc, DWORD dwFlags)
    : CUndoUnitBase(pSite, IDS_UNDONEWCTRL)
{
    TraceTag((tagUndo, "CUndoMove ctor"));

    Assert(prc);

    _rc      = *prc;
    _dwFlags = dwFlags;
}

//+---------------------------------------------------------------------------
//
//  Member:     CUndoMove::Do, public
//
//  Synopsis:   Performs an undo of moving a control.
//
//----------------------------------------------------------------------------

HRESULT
CUndoMove::Do(IOleUndoManager * pUndoMgr)
{
    HRESULT      hr;
    DWORD        dwCookie;
    CDoc *         pDoc = Site()->_pDoc;
    CDoc::CLock Lock(pDoc);

    TraceTag((tagUndo, "CUndoMove::Do"));

    if (!pUndoMgr)
    {
        hr = pDoc->BlockNewUndoUnits(&dwCookie);
        if (hr)
            RRETURN(hr);
    }

    //
    // This move should put a corresponding move action on the redo stack
    // unless we're blocking new actions.
    //
    hr = Site()->Move(&_rc, _dwFlags);
    Site()->ShowLastErrorInfo(hr);

    if (!pUndoMgr)
    {
        pDoc->UnblockNewUndoUnits(dwCookie);
    }

    RRETURN(hr);
}
