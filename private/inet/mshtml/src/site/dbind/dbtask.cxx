//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1996-1997
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  Contents:   Data Bind Task objects
//
//  Classes:    
//
//  History:    10/1/96     (sambent) created


#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>
#endif

#ifndef X_PRGSNK_H_
#define X_PRGSNK_H_
#include <prgsnk.h>
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"
#endif

#ifndef X_SIMPDC_HXX_
#define X_SIMPDC_HXX_
#include "simpdc.hxx"
#endif

PerfDbgTag(tagDBTask,"Databinding","Databinding task");
MtDefine(CDataBindTask, DataBind, "CDataBindTask");
MtDefine(CDataBindTask_aryCRI_pv, CDataBindTask, "CDataBindTask::_aryCRI::_pv");


/////////////////////////////////////////////////////////////////////////////
/////                       CDataBindTask methods                       /////
/////////////////////////////////////////////////////////////////////////////

/*  Each CDoc owns a CDataBindTask to manage its databinding chores.
*/


//+-------------------------------------------------------------------------
// Member:      SetEnabled (public)
//
// Synopsis:    change the enabled state of the task
//
// Arguments:   fEnabled    new state
//
// Returns:     old state

BOOL
CDataBindTask::SetEnabled(BOOL fEnabled)
{
    BOOL fOldEnabled = _fEnabled;
    PerfDbgLog1(tagDBTask, this, "fEnabled is now %d", fEnabled);
    _fEnabled = fEnabled;
    DecideToRun();
    return fOldEnabled;
}


//+-------------------------------------------------------------------------
// Member:      Add Deferred Binding (public)
//
// Synopsis:    add a binder to the list of deferred bindings
//
// Arguments:   pdsb        the new binding

HRESULT
CDataBindTask::AddDeferredBinding(CDataSourceBinder *pdsb, BOOL fSetWait)
{
    HRESULT hr = S_OK;
    Assert(pdsb);

    if (pdsb->_fOnTaskList)
    {
        RemoveDeferredBinding(pdsb);
    }
    Assert(!pdsb->_fOnTaskList);

    PerfDbgLog2(tagDBTask, this, "Add binding for %ls %x",
                pdsb->GetElementConsumer()->TagName(), pdsb->GetElementConsumer());
    
    // add new binder to my waiting list
    pdsb->_pdsbNext = _pdsbWaiting;
    _pdsbWaiting = pdsb;
    pdsb->_fOnTaskList = TRUE;

    if (fSetWait)
    {
        SetWaiting();
    }
    
    DecideToRun();
    
    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      RemoveBindingFromList (private)
//
// Synopsis:    remove a binder from the list of deferred bindings
//
// Arguments:   pdsb                binder to remove
//              ppdsbListHead       pointer to head of list
//              pdwProgCookie       progress cookie for this list

void
CDataBindTask::RemoveBindingFromList(CDataSourceBinder *pdsb,
                                        CDataSourceBinder **ppdsbListHead,
                                        DWORD *pdwProgCookie)
{
    CDataSourceBinder **ppdsbPrev = ppdsbListHead;
    
    // remove binder the list
    while (*ppdsbPrev)
    {
        if (*ppdsbPrev == pdsb)
            *ppdsbPrev = pdsb->_pdsbNext;       // delete binder from list
        else
            ppdsbPrev = &((*ppdsbPrev)->_pdsbNext);
    }

    // if list is now empty, deletes its progress item
    if (*ppdsbListHead == NULL && *pdwProgCookie)
    {
        IProgSink *pProgSink = _pDoc->GetProgSink();
        Assert(pProgSink);
        
        pProgSink->DelProgress(*pdwProgCookie);
        *pdwProgCookie = 0;
    }
}


//+-------------------------------------------------------------------------
// Member:      Remove Deferred Binding (public)
//
// Synopsis:    remove a binder from the list of deferred bindings
//
// Arguments:   pdsb        the new binding

HRESULT
CDataBindTask::RemoveDeferredBinding(CDataSourceBinder *pdsb)
{
    HRESULT hr = S_OK;
    Assert(pdsb);

    PerfDbgLog2(tagDBTask, this, "Remove binding for %ls %x",
                pdsb->GetElementConsumer()->TagName(), pdsb->GetElementConsumer());

    // remove binder from my in-process list
    RemoveBindingFromList(pdsb, &_pdsbInProcess, &_dwProgCookieActive);

    // remove binder from my waiting list
    RemoveBindingFromList(pdsb, &_pdsbWaiting, &_dwProgCookieWait);
    
    pdsb->_fOnTaskList = FALSE;

    RRETURN(hr);
}


//+-------------------------------------------------------------------------
// Member:      InitCurrentRecord (public)
//
// Synopsis:    add to the list of CRI's that need init
//
// Arguments:   pCRI        CRI that needs init

HRESULT
CDataBindTask::InitCurrentRecord(CCurrentRecordInstance *pCRI)
{
    HRESULT hr = _aryCRI.Append(pCRI);
    if (!hr)
    {
        pCRI->AddRef();
        DecideToRun();
    }
    return hr;
}


//+-------------------------------------------------------------------------
// Member:      DecideToRun (public)
//
// Synopsis:    unblock if there's work to do, and permission to do it
//
// Arguments:   none

void
CDataBindTask::DecideToRun()
{
    BOOL fBlocked = !_fEnabled;

    // if there's work waiting, set up a progress sink in the Doc.
    // This delays onload until databinding is done.
    if (_dwProgCookieWait == 0 && _fWorkWaiting && _pdsbWaiting)
    {
        IProgSink *pProgSink = _pDoc->GetProgSink();
        Assert(pProgSink);

        IGNORE_HR(pProgSink->AddProgress(PROGSINK_CLASS_DATABIND, &_dwProgCookieWait));
    }

    if (_fEnabled)
    {
        // transfer waiting tasks to the active list, if it's empty
        if (_fWorkWaiting && !_pdsbInProcess)
        {
            int cBindingsMoved = 0;

            // The binders were added to the front of the waiting list, so
            // they appear in reverse order.  We reverse the list here, so that
            // we process them in the same order they appeared on the page.
            // This is pshychologically more pleasant, and also improves performance
            // (we change the page top-to-bottom, so recalc is faster).
            // This also processes nested tables in the right order (outside-in).
            while (_pdsbWaiting)
            {
                CDataSourceBinder *pdsbTemp = _pdsbWaiting->_pdsbNext;
                _pdsbWaiting->_pdsbNext = _pdsbInProcess;
                _pdsbInProcess = _pdsbWaiting;
                _pdsbWaiting = pdsbTemp;
                ++ cBindingsMoved;
            }

            PerfDbgLog1(tagDBTask, this, "Moving %d bindings to active list", cBindingsMoved);

            // the previous active list may not have shut down its progress sink
            // yet.  If not, do it now.
            if (_dwProgCookieActive)
            {
                IProgSink *pProgSink = _pDoc->GetProgSink();
                Assert(pProgSink);

                pProgSink->DelProgress(_dwProgCookieActive);
                _dwProgCookieActive = 0;
            }
            
            _dwProgCookieActive = _dwProgCookieWait;
            _dwProgCookieWait = 0;

            // tell my progress sink to start supplying feedback
            if (_pdsbInProcess && _dwProgCookieActive)
            {
                IProgSink *pProgSink = _pdsbInProcess->_pDoc->GetProgSink();
                Assert(pProgSink);
                
                IGNORE_HR(pProgSink->SetProgress(_dwProgCookieActive,
                    PROGSINK_SET_STATE | PROGSINK_SET_IDS |
                    PROGSINK_SET_POS | PROGSINK_SET_MAX,
                    PROGSINK_STATE_LOADING, NULL, IDS_DATABINDING, 0, cBindingsMoved));
            }
            _cInProcess = 0;        // OnRun will count up again
        }

        // if there's nothing to do, init the CRIs
        if (!_fBinding && _pdsbInProcess == NULL)
        {
            int i;
            CCurrentRecordInstance **ppCRI;
            CPtrAry<CCurrentRecordInstance*> aryCRI(Mt(CDataBindTask_aryCRI_pv));

            // make a local copy of the array, in case there's reentrancy
            for (i=_aryCRI.Size(), ppCRI=_aryCRI; i>0;  --i, ++ppCRI)
            {
                if (S_OK == aryCRI.Append(*ppCRI))
                {
                    (*ppCRI)->AddRef();
                }
            }
            _aryCRI.ReleaseAll();

            // init all the CRI's 
            for (i=aryCRI.Size(), ppCRI=aryCRI; i>0;  --i, ++ppCRI)
            {
                (*ppCRI)->InitPosition(TRUE);
            }
            aryCRI.ReleaseAll();
        }

        // init-ing the CRI's may have caused new bindings to join the list
        fBlocked = (_pdsbInProcess == NULL);
    }

    // if the waiting list is empty, turn off the waiting flag
    if (_pdsbWaiting == NULL)
        _fWorkWaiting = FALSE;

    SetBlocked(fBlocked);
}


//+-------------------------------------------------------------------------
// Member:      On Run (public, CTask)
//
// Synopsis:    process deferred bindings
//
// Arguments:   dwTimeout       time by which I should finish

void
CDataBindTask::OnRun(DWORD dwTimeout)
{
    HRESULT hr=S_OK;

    _fBinding = TRUE;
    
    // process as many bindings as we can do in the allotted time
    while (_pdsbInProcess)
    {
        CDataSourceBinder *pdsbCurrent = _pdsbInProcess;
#if DBG==1 || defined(PERFTAGS)
        int dbop = pdsbCurrent->_dbop;
        CElement *pElemConsumer = pdsbCurrent->GetElementConsumer();
#endif
        
        _pdsbInProcess = _pdsbInProcess->_pdsbNext; // update the list
        pdsbCurrent->_fOnTaskList = FALSE;
        ++ _cInProcess;                             // track progress

        PerfDbgLog3(tagDBTask, this, "+Op %d for %ls %x",
                        dbop, pElemConsumer->TagName(), pElemConsumer);

        // do the operation the current binder wants
        switch (pdsbCurrent->_dbop)
        {
        case BINDEROP_BIND:
            hr = pdsbCurrent->_fNotReady ? S_FALSE : pdsbCurrent->TryToBind();
            break;

        case BINDEROP_UNBIND:
            hr = pdsbCurrent->UnBind();
            break;

        case BINDEROP_REBIND:
            hr = pdsbCurrent->_fNotReady ? S_FALSE : pdsbCurrent->ReBind();
            break;

        case BINDEROP_ENSURE_DATA_EVENTS:
            hr = pdsbCurrent->EnsureDataEvents();
            // once the events are ensured, we don't need the binder
            if (!hr)
            {
                delete pdsbCurrent;
            }
            break;
        }

        PerfDbgLog4(tagDBTask, this, "-Op %d for %ls %x yielded hr %x",
                        dbop, pElemConsumer->TagName(), pElemConsumer,
                        hr);
        
        // If something went wrong, throw the binder back on the waiting list;
        // we'll try again later, perhaps it'll work then.
        if (hr)
        {
            AddDeferredBinding(pdsbCurrent, FALSE);
        }

        if (GetTickCount() >= dwTimeout)            // don't exceed allotted time
            break;
    }

    // report progress
    if (_dwProgCookieActive)
    {
        IProgSink *pProgSink = _pDoc->GetProgSink();
        Assert(pProgSink);

        if (_pdsbInProcess)
        {
            pProgSink->SetProgress(_dwProgCookieActive,
                                    PROGSINK_SET_POS, 0, NULL, 0, _cInProcess, 0);
        }
        else
        {
            pProgSink->DelProgress(_dwProgCookieActive);
            _dwProgCookieActive = 0;
        }
    }

    _fBinding = FALSE;
    
    DecideToRun();              // see if there's more work to do
}


//+-------------------------------------------------------------------------
// Member:      On Terminate (public, CTask)
//
// Synopsis:    shut down the task (possibly before all bindings have happened)
//
// Arguments:   none

void
CDataBindTask::OnTerminate()
{
    if (_pDoc == NULL)
        goto Cleanup;

    Stop();

    _pDoc->SubRelease();
    _pDoc = NULL;

Cleanup:
    return;
}


//+-------------------------------------------------------------------------
// Member:      Stop (public)
//
// Synopsis:    shut down the task (possibly before all bindings have happened)
//
// Arguments:   none

void
CDataBindTask::Stop()
{
    CDataSourceBinder *pdsb, *pdsbNext;
    IProgSink *pProgSink;

    _fWorkWaiting = FALSE;               // don't process the waiting list
    
    // discard waiting list
    for (pdsb=_pdsbWaiting; pdsb; pdsb=pdsbNext)
    {
        pdsbNext = pdsb->_pdsbNext;
        pdsb->_fOnTaskList = FALSE;
        if (pdsb->_dbop == BINDEROP_ENSURE_DATA_EVENTS)
        {
            delete pdsb;
        }
    }
    _pdsbWaiting = 0;
    
    // discard in-process list
    for (pdsb=_pdsbInProcess; pdsb; pdsb=pdsbNext)
    {
        pdsbNext = pdsb->_pdsbNext;
        pdsb->_fOnTaskList = FALSE;
        if (pdsb->_dbop == BINDEROP_ENSURE_DATA_EVENTS)
        {
            delete pdsb;
        }
    }
    _pdsbInProcess = 0;

    // discard CRI list
    _aryCRI.ReleaseAll();

    // turn off progress items
    pProgSink = _pDoc->GetProgSink();
    if (pProgSink && _dwProgCookieWait)
    {
        pProgSink->DelProgress(_dwProgCookieWait);
        _dwProgCookieWait = 0;
    }
    if (pProgSink && _dwProgCookieActive)
    {
        pProgSink->DelProgress(_dwProgCookieActive);
        _dwProgCookieActive = 0;
    }

    ClearInterface(&_pSDC);
}


//+-------------------------------------------------------------------------
// Member:      GetSimpleDataConverter (public)
//
// Synopsis:    return the doc-global SimpleDataConverter, creating it
//              if necessary.  This is used for elements bound with
//              dataFormatAs = localized-text.
//
// Arguments:   none

ISimpleDataConverter *
CDataBindTask::GetSimpleDataConverter()
{
    if (!_pSDC)
    {
        _pSDC = new CSimpleDataConverter;
    }
    return _pSDC;       // not refcounted!!
}
