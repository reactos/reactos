//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       agent.CXX
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    11-21-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <agent.h>

//+---------------------------------------------------------------------------
//
//  Method:     COInetAgent::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetAgent::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_SESSION, "%p _IN COInetAgent::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_IOInetSession) )
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetAgent::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetAgent::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetAgent::AddRef(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetAgent::AddRef\n", this));

    LONG lRet = ++_CRefs;

    TransDebugOut((DEB_SESSION, "%p OUT COInetAgent::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetAgent::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetAgent::Release(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetAgent::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        // this is global
        //delete this;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetAgent::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

STDMETHODIMP COInetAgent::StartListen(
    const LPCWSTR  szProtocol,
    IOInetItemFilter   *pWChkItemFilter,
    DWORD          grfFilterMode,
    CLSID        *pclsidProtocol,
    DWORD          dwReserved
    )
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAgent::StartListen\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetAgent::StartListen (hr:%lx)\n",this, hr));
    return hr;
}

// will release the sink passed in at StartListen
STDMETHODIMP COInetAgent::StopListen(CLSID *pclsidProtocol)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAgent::StopListen\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetAgent::StopListen (hr:%lx)\n",this, hr));
    return hr;
}

// add item to the spooler
STDMETHODIMP COInetAgent::ScheduleItem(
    IOInetItem          *pWChkItem,
    IOInetDestination   *pWChkDest,
    SCHEDULEDATA       *pschdata,
    DWORD               dwMode,
    DWORD     *pdwCookie
    )
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAgent::ScheduleItem\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetAgent::ScheduleItem (hr:%lx)\n",this, hr));
    return hr;
}


STDMETHODIMP COInetAgent::RevokeItem(DWORD dwCookie)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAgent::RevokeItem\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetAgent::RevokeItem (hr:%lx)\n",this, hr));
    return hr;
}


#if 0
//+---------------------------------------------------------------------------
//
//  Method:     COInetItem::QueryInterface
//
//  Synopsis:
//
//  Arguments:  [riid] --
//              [ppvObj] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetItem::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_SESSION, "%p _IN COInetItem::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_IOInetSession) )
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetItem::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetItem::AddRef
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetItem::AddRef(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetItem::AddRef\n", this));

    LONG lRet = ++_CRefs;

    TransDebugOut((DEB_SESSION, "%p OUT COInetItem::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetItem::Release
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    11-22-96   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) COInetItem::Release(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetItem::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        //delete this;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetItem::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

STDMETHODIMP COInetItem::GetURL(LPOLESTR *ppwzUrl)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetItem::GetURL\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetItem::GetURL (hr:%lx)\n",this, hr));
    return hr;
}


STDMETHODIMP COInetItem::GetInfo(
        DWORD     dwOptions,
        LPOLESTR *ppwzItemMime,
        LPCLSID  *pclsidItem,
        LPOLESTR *ppwzProtocol,
        LPCLSID  *pclsidProtocol,
        DWORD    *pdwOut
        )
{
    TransDebugOut((DEB_PROT, "%p _IN COInetItem::GetInfo\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetItem::GetInfo (hr:%lx)\n",this, hr));
    return hr;
}


STDMETHODIMP COInetItem::GetItemData(DWORD *grfITEMF,ITEMDATA * pitemdata)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetItem::GetItemData\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetItem::GetItemData (hr:%lx)\n",this, hr));
    return hr;
}

#endif // 0

