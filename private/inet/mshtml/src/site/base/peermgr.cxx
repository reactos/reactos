//+---------------------------------------------------------------------
//
//  File:       peer.cxx
//
//  Contents:   peer holder
//
//  Classes:    CPeerHolder
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_PEER_HXX_
#define X_PEER_HXX_
#include "peer.hxx"
#endif

//////////////////////////////////////////////////////////////////////////////
//
// misc
//
//////////////////////////////////////////////////////////////////////////////

MtDefine(CPeerMgr, Elements, "CPeerMgr")

//////////////////////////////////////////////////////////////////////////////
//
// CPeerMgr methods
//
//////////////////////////////////////////////////////////////////////////////

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr constructor
//
//----------------------------------------------------------------------------

CPeerMgr::CPeerMgr(CElement * pElement)
{
    Assert (!_cPeerDownloads);
    _readyState = READYSTATE_COMPLETE;
    _pElement = pElement;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr destructor
//
//----------------------------------------------------------------------------

CPeerMgr::~CPeerMgr()
{
    DelDownloadProgress();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::EnsurePeerMgr, static helper
//
//----------------------------------------------------------------------------

HRESULT
CPeerMgr::EnsurePeerMgr(CElement * pElement, CPeerMgr ** ppPeerMgr)
{
    HRESULT     hr = S_OK;

    (*ppPeerMgr) = pElement->GetPeerMgr();

    if (*ppPeerMgr)     // if we already have peer mgr
        goto Cleanup;   // done

    // create it

    (*ppPeerMgr) = new CPeerMgr(pElement);
    if (!(*ppPeerMgr))
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    pElement->SetPeerMgr(*ppPeerMgr);

Cleanup:
    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::EnsureDeletePeerMgr, static helper
//
//  Synopsis:   if no one needs the PeerMgr, this method deletes it and removes ptr
//              to it from the element
//
//----------------------------------------------------------------------------

void
CPeerMgr::EnsureDeletePeerMgr(CElement * pElement, BOOL fForce)
{
    CPeerMgr *  pPeerMgr = pElement->GetPeerMgr();

    if (!pPeerMgr)
        return;

    if (!fForce && !pPeerMgr->CanDelete())
        return;

    delete pElement->DelPeerMgr();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::UpdateReadyState, static helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::UpdateReadyState(CElement * pElement, READYSTATE readyStateNew)
{
    HRESULT         hr;
    CPeerMgr *      pPeerMgr;
    CPeerHolder *   pPeerHolder = pElement->GetPeerHolder();
    READYSTATE      readyState = pPeerHolder ? pPeerHolder->GetReadyStateMulti() : READYSTATE_COMPLETE;

    if (READYSTATE_UNINITIALIZED != readyStateNew)
    {
        readyState = min (readyState, readyStateNew);
    }

    if (readyState < READYSTATE_COMPLETE)
    {
        hr = THR(CPeerMgr::EnsurePeerMgr(pElement, &pPeerMgr));
        if (hr)
            goto Cleanup;

        pPeerMgr->UpdateReadyState(readyState);
    }
    else
    {
        Assert (readyState == READYSTATE_COMPLETE);

        pPeerMgr = pElement->GetPeerMgr();

        if (pPeerMgr)
        {
            pPeerMgr->UpdateReadyState();

            CPeerMgr::EnsureDeletePeerMgr(pElement);
        }
    }

Cleanup:
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::UpdateReadyState
//
//----------------------------------------------------------------------------

void
CPeerMgr::UpdateReadyState(READYSTATE readyStateNew)
{
    CPeerHolder *   pPeerHolder = _pElement->GetPeerHolder();
    READYSTATE      readyStatePrev;

    readyStatePrev = _readyState;

    if (READYSTATE_UNINITIALIZED == readyStateNew)
        readyStateNew = pPeerHolder ? pPeerHolder->GetReadyStateMulti() : READYSTATE_COMPLETE;

    _readyState = min (_cPeerDownloads ? READYSTATE_LOADING : READYSTATE_COMPLETE, readyStateNew);

    if (readyStatePrev == READYSTATE_COMPLETE && _readyState <  READYSTATE_COMPLETE)
    {
        AddDownloadProgress();
    }

    if (readyStatePrev != _readyState)
    {
        _pElement->OnReadyStateChange();
    }

    if (readyStatePrev <  READYSTATE_COMPLETE && _readyState == READYSTATE_COMPLETE)
    {
        DelDownloadProgress();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::AddDownloadProgress, helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::AddDownloadProgress()
{
    CMarkup *   pMarkup = _pElement->GetMarkup();

    if (pMarkup)
    {
        IProgSink * pProgSink = pMarkup->GetProgSink();

        Assert (0 == _dwDownloadProgressCookie);

        if (pProgSink)
        {
            pProgSink->AddProgress (PROGSINK_CLASS_CONTROL, &_dwDownloadProgressCookie);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::DelDownloadProgress, helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::DelDownloadProgress()
{
    DWORD dwDownloadProgressCookie;

    if (0 != _dwDownloadProgressCookie)
    {
        // element should be in markup if _dwDownloadProgressCookie is set;
        // if element leaves markup before progress is released, OnExitTree should release the progress
        Assert (_pElement->GetMarkup());

        // progsink on markup should be available because we did AddProgress on it
        Assert (_pElement->GetMarkup()->GetProgSink());

        dwDownloadProgressCookie = _dwDownloadProgressCookie;
        _dwDownloadProgressCookie = 0;

        _pElement->GetMarkup()->GetProgSink()->DelProgress (dwDownloadProgressCookie);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CPeerMgr::OnExitTree, helper
//
//----------------------------------------------------------------------------

void
CPeerMgr::OnExitTree()
{
    DelDownloadProgress();
}

