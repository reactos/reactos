//+---------------------------------------------------------------------------
//  Microsoft Trident
//  Copyright (C) Microsoft Corporation, 1996-1996
//  All rights Reserved.
//  Information contained herein is Proprietary and Confidential.
//
//  Contents:   Data Source Binder objects
//
//  Classes:    CDataSourceBinder
//
//  History:    10/1/96     (sambent) created

#include <headers.hxx>

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include <formkrnl.hxx>     // for safetylevel in safety.hxx (via olesite.hxx)
#endif

#ifndef X_OLESITE_HXX_
#define X_OLESITE_HXX_
#include <olesite.hxx>
#endif

#ifndef X_LTABLE_HXX_
#define X_LTABLE_HXX_
#include "ltable.hxx"
#endif

#ifndef X_SIMPDATA_H_
#define X_SIMPDATA_H_
#include <simpdata.h>
#endif

#ifndef X_MSTDWRAP_H_
#define X_MSTDWRAP_H_
#include "mstdwrap.h"
#endif

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

#ifndef X_BINDER_HXX_
#define X_BINDER_HXX_
#include "binder.hxx"
#endif

#ifndef X_DBTASK_HXX_
#define X_DBTASK_HXX_
#include "dbtask.hxx"
#endif

#ifndef X_ROWBIND_HXX_
#define X_ROWBIND_HXX_
#include "rowbind.hxx"
#endif

#ifndef X_DMEMBMGR_HXX_
#define X_DMEMBMGR_HXX_
#include "dmembmgr.hxx"       // for CDataMemberMgr
#endif

MtDefine(DataBind, Mem, "Data Binding")
MtDefine(CDataSourceBinder, DataBind, "CDataSourceBinder")
MtDefine(CCurrentRecordInstance, DataBind, "CCurrentRecordInstance")
MtDefine(CDataSourceProvider, DataBind, "CDataSourceProvider")
MtDefine(CDataSourceProvider_aryAdvisees_pv, CDataSourceProvider, "CDataSourceProvider::_aryAdvisees::_pv")

/////////////////////////////////////////////////////////////////////////////
/////                    CDataSourceBinder methods                      /////
/////////////////////////////////////////////////////////////////////////////


//+---------------------------------------------------------------------------
//
//  Member:     Passivate
//
//  Synopsis:   revert to pre-init state
//

HRESULT
CDataSourceBinder::Passivate()
{
    HRESULT hr;

    // During binding, we may trigger events that cause the binder to go away.
    // If so, simply set a flag and do the real shutdown later (see TryToBind).
    // An example of this situation (which only Neetu would ever dream of):
    // I'm the first element trying to bind to provider foo, doing so triggers
    // foo.ondatasetcomplete, the handler changes dataFld on my element.
    if (_fBinding)
    {
        _fAbort = TRUE;
        hr = S_OK;
        goto Cleanup;
    }

    if (_fOnTaskList)
    {
        CDataBindTask *pdbt = GetDataBindTask();
        if (pdbt)
        {
            pdbt->RemoveDeferredBinding(this);
        }
    }

    hr = UnBind();

    ClearInterface(&_pConsumer);

    delete this;

Cleanup:
    return hr;
}


#if DBG==1
//+---------------------------------------------------------------------------
//
//  Member:     Destructor
//
//  Synopsis:   go away
//

CDataSourceBinder::~CDataSourceBinder()
{
    Assert("Passivate must be called first" &&
        !_pConsumer && !_pProvider
        );
}
#endif


//+---------------------------------------------------------------------------
//
//  Member:     GetDataBindTask
//
//  Synopsis:   return the task that owns me

CDataBindTask *
CDataSourceBinder::GetDataBindTask()
{
    return _pDoc->GetDataBindTask();
}


//+---------------------------------------------------------------------------
//
//  Member:     SetReady
//
//  Synopsis:   set the "readiness" of the binder.  If it becomes ready, notify
//              the task that it's worth trying to bind.

void
CDataSourceBinder::SetReady(BOOL fReady)
{
    _fNotReady = !fReady;
    if (fReady && _fOnTaskList)
    {
        GetDataBindTask()->SetWaiting();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     Register
//
//  Synopsis:   register myself with the databinding task
//

HRESULT
CDataSourceBinder::Register(const DBSPEC *pdbs)
{
    Assert("consumer required" && _pelConsumer);
    Assert("consumer already set" && !_pConsumer);
    Assert("provider already set" && !_pProvider && !_cstrProviderName);

    BSTR bstrElement=NULL, bstrDataset=NULL;
    HRESULT hr = S_OK;
    CDataBindTask *pDBTask;

    _pDoc = _pelConsumer->Doc();
    Assert("consumer must belong to a CDoc" && _pDoc);

    // get provider info
    FormsSplitFirstComponent(pdbs->_pStrDataSrc, &bstrElement, &bstrDataset);
    hr = _cstrProviderName.Set(bstrElement);
    if (!hr)
        hr = _cstrDataMember.Set(bstrDataset);
    if (hr)
        goto Cleanup;

    // defer the actual binding
    pDBTask = GetDataBindTask();
    if (!pDBTask)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    hr = pDBTask->AddDeferredBinding(this);
    if (hr)
        goto Cleanup;

Cleanup:
    FormsFreeString(bstrElement);
    FormsFreeString(bstrDataset);
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     TryToBind (called by CDataBindTask)
//
//  Synopsis:   try to bind to my provider
//

HRESULT
CDataSourceBinder::TryToBind()
{
    HRESULT hr = E_FAIL;
    CDataMemberMgr *pDataMemberManager;
    DBIND_KIND dbk;
    DBSPEC dbs;
    DWORD dwFilter;

    // The element's gotta want to bind.
    dbk = CDBindMethods::DBindKind(_pelConsumer, _idConsumer);
    if (dbk == DBIND_NONE)
        goto Cleanup;       // hr is E_FAIL

    if (S_OK != CDBindMethods::GetDBSpec(_pelConsumer, _idConsumer, &dbs,
                                   DBIND_SETFILTER|DBIND_CURRENTFILTER) )
        goto Cleanup;

    // The element's spec and its inherent binding preference must agree.
    dwFilter = (dbk == DBIND_SINGLEVALUE) ? DBIND_CURRENTFILTER : DBIND_SETFILTER;
    if (!dbs.FFilter(dwFilter, _pelConsumer->Tag()==ETAG_TABLE))
        goto Cleanup;

    // set the re-entrancy flag (see Passivate)
    _fBinding = TRUE;

    // set up consumer
    if (!_pConsumer)
    {
        hr = CConsumer::Create(this, &_pConsumer);
        if (hr)
            goto Cleanup;
    }

    Assert(_pConsumer);
    Assert(!_pProvider);

    // get the HTML element designated as the DataSrc
    if (!_pelProvider)
    {
        hr = THR_NOTRACE(FindProviderByName(&_pelProvider));
        if (hr)
            goto Cleanup;
    }

    // ask it for a CDataSourceProvider
    _pelProvider->EnsureDataMemberManager();
    pDataMemberManager = _pelProvider->GetDataMemberManager();
    if (pDataMemberManager)
    {
        hr = pDataMemberManager->GetDataSourceProvider(_cstrDataMember, &_pProvider);
    }
    else
    {
        hr = E_FAIL;
    }

    // if something went wrong, hook up the null provider
    if (hr)
    {
        hr = CDataSourceProvider::Create(NULL, _pDoc, _cstrDataMember, &_pProvider);  // null provider
        if (hr)
            goto Cleanup;
    }

    // getting the provider may cause me to die (see Passivate).
    // if so, carry out my death now.
    if (_fAbort)
        goto Cleanup;

    // do the actual binding
    Assert(_pProvider);
    _pProvider->AdviseDataProviderEvents(this);
    hr = Bind();

    // if it didn't work, don't hang on to the provider
    if (hr)
    {
        if (_pProvider)
        {
            _pProvider->UnadviseDataProviderEvents(this);
            _pProvider->Release();
            _pProvider = NULL;
        }
        _pelProvider = NULL;
    }

Cleanup:
    _fBinding = FALSE;

    if (_fAbort)
    {
        Passivate();        // calls "delete this", no work after here
        hr = S_OK;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     Bind (public - called SetProvider)
//
//  Synopsis:   hook up the consumer to the provider
//

HRESULT
CDataSourceBinder::Bind()
{
    HRESULT hr;

    Assert(_pConsumer);
    _pelConsumer->AddRef();         // keep consumer alive while binding

    hr = _pConsumer->Bind();
    if (hr == E_NOINTERFACE)        // this means we've bound to the null provider
        hr = S_OK;

    if (!hr && !_fAbort)
    {
        _pConsumer->FireOnDataReady(TRUE);
    }

    Verify(_pelConsumer->Release());

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     DetachBinding (public)
//
//  Synopsis:   detach the binding
//

HRESULT
CDataSourceBinder::DetachBinding()
{
    if (!_pDoc->TestLock(FORMLOCK_UNLOADING))
    {
        _dbop = BINDEROP_UNBIND;
        GetDataBindTask()->AddDeferredBinding(this);
    }

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:     UnBind (public)
//
//  Synopsis:   unhook the consumer from the provider
//

HRESULT
CDataSourceBinder::UnBind()
{
    HRESULT hr=S_OK;

    if (_pConsumer)
    {
        _pConsumer->FireOnDataReady(FALSE);
        hr = _pConsumer->UnBind();
        if (hr == E_NOINTERFACE)            // bound to null provider
            hr = S_OK;
    }

    if (_pProvider)
    {
        _pProvider->UnadviseDataProviderEvents(this);
        _pProvider->Release();
        _pProvider = NULL;
    }

    _pelProvider = NULL;

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     ReBind (public)
//
//  Synopsis:   rehook the consumer to the (possibly new) provider
//

HRESULT
CDataSourceBinder::ReBind()
{
    HRESULT hr;

    hr = UnBind();
    if (!hr)
    {
        hr = TryToBind();
    }

    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     EnsureDataEvents (public)
//
//  Synopsis:   make sure data events can fire for my "consumer"
//

HRESULT
CDataSourceBinder::EnsureDataEvents()
{
    HRESULT hr;
    CDataMemberMgr *pDataMemberManager;

    _pelConsumer->EnsureDataMemberManager();
    pDataMemberManager = _pelConsumer->GetDataMemberManager();

    if (pDataMemberManager)
    {
        hr = pDataMemberManager->IsReady();
        if (!hr)
        {
            // It's enough to simply ask for the provider and release it.
            // This creates it, and it will live on in the OleSite's cache.
            CDataSourceProvider *pProvider = NULL;
            IGNORE_HR(pDataMemberManager->GetDataSourceProvider(NULL, &pProvider));
            if (pProvider)
            {
                pProvider->Release();
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }
    
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     FindProviderByName (private - called by SetProvider)
//
//  Synopsis:   find an element corresponding to _cstrProviderName
//

HRESULT
CDataSourceBinder::FindProviderByName(CElement **ppelProvider)
{
    HRESULT     hr;

    Assert(GetProviderName() != 0);
    Assert(ppelProvider);
    Assert(_pDoc);

    // DataSrc is supposed to be #<IDRef>
    if (*((LPTSTR)GetProviderName()) != _T('#'))
    {
        hr = S_FALSE;
        goto Cleanup;
    }

    Assert(_pelConsumer->GetMarkup());

    // ask the Consumer for the named element, don't use the '#' in the name match.
    hr = THR_NOTRACE(_pelConsumer->GetMarkup()->GetElementByNameOrID((LPTSTR)GetProviderName() + 1,
                                                ppelProvider));

    // If the lookup failed then stop everything.  It returns S_FALSE if
    // more than one element with that name exists;  we'll use the first one.
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto Cleanup;
    }
    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     SubstituteProvider (private, called by CConsumer) 
//
//  Synopsis:   substitute the new provider for the current one.  This should
//              only be called in the early stages of binding.  It's used by
//              hierarchically-bound tables, whose dataSrc gives a top-level
//              provider;  the real provider must be computed during binding
//              by examining the repeating context.
//

void
CDataSourceBinder::SubstituteProvider(CDataSourceProvider *pProviderNew)
{
    CDataSourceProvider *pProviderOld = _pProvider;

    _pProvider = pProviderNew;
    
    if (pProviderNew)
    {
        pProviderNew->AddRef();
        pProviderNew->AdviseDataProviderEvents(this);
    }
    
    if (pProviderOld)
    {
        pProviderOld->UnadviseDataProviderEvents(this);
        pProviderOld->Release();
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     ReplaceProvider (public, called by CDataSourceProvider) 
//
//  Synopsis:   cope with provider changing its interfaces
//

HRESULT
CDataSourceBinder::ReplaceProvider(CDataSourceProvider *pdspNewProvider)
{
    HRESULT hr = S_OK;

    if (!_pDoc->TestLock(FORMLOCK_UNLOADING))
    {
        _dbop = BINDEROP_REBIND;
        GetDataBindTask()->AddDeferredBinding(this);
    }

    return hr;
}
