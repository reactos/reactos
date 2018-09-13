//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       msgflter.cxx
//
//  Contents:   class for intellignet modal loop uses in sychronous binding
//
//  Classes:
//
//  Functions:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <trans.h>

//+---------------------------------------------------------------------------
//
//  Method:     CModalLoop::CModalLoop
//
//  Synopsis:
//
//  Arguments:  [phr] --
//
//  Returns:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CModalLoop::CModalLoop(HRESULT *phr)
{
    UrlMkAssert((phr));

        // BUGBUG 3384: message filter is bogus.  Disable for now.
    // *phr = CoRegisterMessageFilter(this, &_pMsgFlter);
        _pMsgFlter = NULL;
        *phr = 0x80000000;
}

//+---------------------------------------------------------------------------
//
//  Method:     CModalLoop::~CModalLoop
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CModalLoop::~CModalLoop()
{
    IMessageFilter *pthis;

    if (_pMsgFlter)
    {
        HRESULT hr = CoRegisterMessageFilter(_pMsgFlter, &pthis);
        // we should get back our messagefilter we installed
        // in ctor
        TransAssert(( (hr == NOERROR) && (this == pthis) ));
        _pMsgFlter->Release();
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CModalLoop::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppv] --
//
//  Returns:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CModalLoop::QueryInterface( REFIID riid, void **ppv )
{
    HRESULT     hr = NOERROR;
    UrlMkDebugOut((DEB_BINDING, "%p _IN CModalLoop::QueryInterface (%lx, %p)\n", this, riid, ppv));

    if (   IsEqualIID(riid, IID_IUnknown)
        || IsEqualIID(riid, IID_IMessageFilter) )
    {
        *ppv = (void FAR *)(IMessageFilter *)this;
        AddRef();
    }
    else
    {
        *ppv = NULL;
        hr = E_NOINTERFACE;
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CModalLoop::QueryInterface (%lx)[%p]\n", this, hr, *ppv));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CModalLoop::AddRef
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CModalLoop::AddRef( void )
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CModalLoop::AddRef\n", this));

    LONG lRet = ++_CRefs;

    UrlMkDebugOut((DEB_BINDING, "%p OUT CModalLoop::AddRef (%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CModalLoop::Release
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    11-11-95   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CModalLoop::Release( void )
{
    UrlMkDebugOut((DEB_BINDING, "%p _IN CModalLoop::Release\n", this));

    LONG lRet = --_CRefs;
    if (_CRefs == 0)
    {
        delete this;
    }
    UrlMkDebugOut((DEB_BINDING, "%p OUT CModalLoop::Release (%ld)\n", this, lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   HandleInComingCall
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//
//  Returns:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(DWORD) CModalLoop::HandleInComingCall(DWORD dwCallType,HTASK htaskCaller,DWORD dwTickCount,LPINTERFACEINFO lpInterfaceInfo)
{
    DWORD dwRet = 0;
    UrlMkDebugOut((DEB_BINDING, "%p _IN CModalLoop::HandleInComingCall \n", this));

    if (_pMsgFlter)
    {
        dwRet = _pMsgFlter->HandleInComingCall(dwCallType, htaskCaller, dwTickCount, lpInterfaceInfo);
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CModalLoop::HandleInComingCall (dwRet:%lx)\n", this, dwRet));
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   RetryRejectedCall
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//
//  Returns:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(DWORD) CModalLoop::RetryRejectedCall(HTASK htaskCallee,DWORD dwTickCount,DWORD dwRejectType)
{
    DWORD dwRet = 0;
    UrlMkDebugOut((DEB_BINDING, "%p _IN CModalLoop::RetryRejectedCall \n", this));

    if (_pMsgFlter)
    {
        dwRet = _pMsgFlter->RetryRejectedCall( htaskCallee, dwTickCount, dwRejectType);
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CModalLoop::RetryRejectedCall (dwRet:%lx)\n", this, dwRet));
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   MessagePending
//
//  Synopsis:
//
//  Arguments:  [DWORD] --
//
//  Returns:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(DWORD) CModalLoop::MessagePending(HTASK htaskCallee,DWORD dwTickCount,DWORD dwPendingType)
{
    DWORD dwRet = 0;
    UrlMkDebugOut((DEB_BINDING, "%p _IN CModalLoop::MessagePending \n", this));

    if (_pMsgFlter)
    {
        dwRet = _pMsgFlter->MessagePending( htaskCallee, dwTickCount, dwPendingType);
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CModalLoop::MessagePending (dwRet:%lx)\n", this, dwRet));
    return dwRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CModalLoop::HandlePendingMessage
//
//  Synopsis:
//
//  Arguments:  [dwPendingType] --
//              [dwPendingRecursion] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    8-21-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CModalLoop::HandlePendingMessage(DWORD dwPendingType, DWORD dwPendingRecursion, DWORD dwReserved)
{
    HRESULT hr = NOERROR;
    DWORD dwRet = 0;
    HTASK htaskCallee = 0;
    DWORD dwTickCount = 0;

    UrlMkDebugOut((DEB_BINDING, "%p _IN CModalLoop::HandlePendingMessage \n", this));


    if (_pMsgFlter)
    {
        dwRet = _pMsgFlter->MessagePending( htaskCallee, dwTickCount, dwPendingType);
    }

    UrlMkDebugOut((DEB_BINDING, "%p OUT CModalLoop::HandlePendingMessage (dwRet:%lx)\n", this, dwRet));
    return dwRet;
}

