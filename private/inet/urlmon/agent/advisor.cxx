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

COInetAdvisor *g_pCOInetAdvisor = 0;

//+---------------------------------------------------------------------------
//
//  Function:   GetOInetAdvisor
//
//  Synopsis:
//
//  Arguments:  [dwMode] --
//              [ppOInetAdvisor] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    11-07-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT GetOInetAdvisor(DWORD dwMode, IOInetAdvisor **ppOInetAdvisor, DWORD dwReserved)
{

    HRESULT hr = NOERROR;
    TransDebugOut((DEB_SESSION, "API _IN GetOInetAdvisor\n"));

    TransAssert(( (ppOInetAdvisor != NULL) && (dwMode == 0) && (dwReserved == 0) && "Invalid argument"));

    if (ppOInetAdvisor && !dwMode && !dwReserved)
    {
        /*
        if (g_pCOInetAdvisor == 0)
        {
            g_pCOInetAdvisor = new COInetAdvisor();
        }

        if (g_pCOInetAdvisor)
        {
            g_pCOInetAdvisor->AddRef();
            *ppOInetAdvisor = g_pCOInetAdvisor;
        }
        else
        {
            hr =  E_OUTOFMEMORY;
        }
        */
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TransDebugOut((DEB_SESSION, "API OUT GetOInetAdvisor (hr:%lx) \n", hr));
    return hr;
}

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
    if ((riid == IID_IUnknown) || (riid == IID_IOInetAdvisor) )
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

//+---------------------------------------------------------------------------
//
//  Method:     COInetItem::GetURL
//
//  Synopsis:
//
//  Arguments:  [ppwzUrl] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetItem::GetURL(LPOLESTR *ppwzUrl)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetItem::GetURL\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetItem::GetURL (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetItem::GetInfo
//
//  Synopsis:
//
//  Arguments:  [dwOptions] --
//              [ppwzItemMime] --
//              [pclsidItem] --
//              [ppwzProtocol] --
//              [pclsidProtocol] --
//              [pdwOut] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
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


//+---------------------------------------------------------------------------
//
//  Method:     COInetItem::GetItemData
//
//  Synopsis:
//
//  Arguments:  [ITEMDATA] --
//              [pitemdata] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetItem::GetItemData(DWORD *grfITEMF,ITEMDATA * pitemdata)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetItem::GetItemData\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetItem::GetItemData (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     COInetItemSink::QueryInterface
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
STDMETHODIMP COInetItemSink::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_SESSION, "%p _IN COInetItemSink::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_IOInetAdvisor) )
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetItemSink::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetItemSink::AddRef
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
STDMETHODIMP_(ULONG) COInetItemSink::AddRef(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetItemSink::AddRef\n", this));

    LONG lRet = ++_CRefs;

    TransDebugOut((DEB_SESSION, "%p OUT COInetItemSink::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetItemSink::Release
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
STDMETHODIMP_(ULONG) COInetItemSink::Release(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetItemSink::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        //delete this;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetItemSink::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

STDMETHODIMP COInetItemSink::OnItem(
    ITEMTYPE   itemtype,
    IOInetItem *pWChkItem,
    DWORD      dwReserved
    )
{
    TransDebugOut((DEB_PROT, "%p _IN COInetItemSink::OnItem\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetItemSink::OnItem (hr:%lx)\n",this, hr));
    return hr;
}

////////////////////////////////////////////////////////
//+---------------------------------------------------------------------------
//
//  Method:     COInetAdvisor::QueryInterface
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
STDMETHODIMP COInetAdvisor::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    TransDebugOut((DEB_SESSION, "%p _IN COInetAdvisor::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_IOInetAdvisor) )
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetAdvisor::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetAdvisor::AddRef
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
STDMETHODIMP_(ULONG) COInetAdvisor::AddRef(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetAdvisor::AddRef\n", this));

    LONG lRet = ++_CRefs;

    TransDebugOut((DEB_SESSION, "%p OUT COInetAdvisor::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   COInetAdvisor::Release
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
STDMETHODIMP_(ULONG) COInetAdvisor::Release(void)
{
    TransDebugOut((DEB_SESSION, "%p _IN COInetAdvisor::Release\n", this));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        // this is global
        //delete this;
    }

    TransDebugOut((DEB_SESSION, "%p OUT COInetAdvisor::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetAdvisor::StartListen
//
//  Synopsis:
//
//  Arguments:  [szProtocol] --
//              [pWChkItemSink] --
//              [grfFilterMode] --
//              [pclsidProtocol] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetAdvisor::StartListen(
    const LPCWSTR  szProtocol,
    IOInetItemFilter   *pWChkItemFilter,
    DWORD          grfFilterMode,
    CLSID        *pclsidProtocol,
    DWORD          dwReserved
    )
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAdvisor::StartListen\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetAdvisor::StartListen (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetAdvisor::StopListen
//
//  Synopsis:   will release the sink passed in at StartListen
//
//  Arguments:  [pclsidProtocol] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetAdvisor::StopListen(CLSID *pclsidProtocol)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAdvisor::StopListen\n", this));
    HRESULT hr = E_FAIL;

    TransDebugOut((DEB_PROT, "%p OUT COInetAdvisor::StopListen (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetAdvisor::Advise
//
//  Synopsis:
//
//  Arguments:  [pWChkItemSink] --
//              [grfMode] --
//              [cMimes] --
//              [ppwzItemMimes] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetAdvisor::Advise(IOInetItemSink *pWChkItemSink, DWORD grfMode, ULONG cMimes, const LPCWSTR *ppwzItemMimes, DWORD dwReserved)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAdvisor::Advise\n", this));
    HRESULT hr = NOERROR;

    for (ULONG i = 0; i < cMimes; i++)
    {
        LPCWSTR  pwzStr = *(ppwzItemMimes+i);
        _AdvSinks.AddVal(pWChkItemSink, pwzStr);
    }

    TransDebugOut((DEB_PROT, "%p OUT COInetAdvisor::Advise (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetAdvisor::Unadvise
//
//  Synopsis:
//
//  Arguments:  [pWChkItemSink] --
//              [cMimes] --
//              [ppwzItemMimes] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetAdvisor::Unadvise(IOInetItemSink *pWChkItemSink, ULONG cMimes, const LPCWSTR *ppwzItemMimes)
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAdvisor::Unadvise\n", this));
    HRESULT hr = NOERROR;

    for (ULONG i = 0; i < cMimes; i++)
    {
        LPCWSTR  pwzStr = *(ppwzItemMimes+i);
        _AdvSinks.RemoveVal(pWChkItemSink, pwzStr);
    }

    TransDebugOut((DEB_PROT, "%p OUT COInetAdvisor::Unadvise (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     COInetAdvisor::SendAdvise
//
//  Synopsis:
//
//  Arguments:  [itemtype] --
//              [pWChkItem] --
//              [grfMode] --
//              [pwzItemMimes] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP COInetAdvisor::SendAdvise(
    ITEMTYPE        itemtype,
    IOInetItem     * pWChkItem,
    DWORD           grfMode,
    LPCWSTR         pwzItemMimes,
    DWORD           dwReserved
    )
{
    TransDebugOut((DEB_PROT, "%p _IN COInetAdvisor::SendAdvise\n", this));
    HRESULT hr;
    HRESULT hr1 = E_FAIL;

    IOInetItemSink   *pWChkItemSink = 0;

    hr = _AdvSinks.FindFirst(pwzItemMimes, (IUnknown **)&pWChkItemSink);

    if (hr == NOERROR)
    {

        do
        {
            hr1 = pWChkItemSink->OnItem(itemtype, pWChkItem, 0);
            pWChkItemSink->Release();
            pWChkItemSink = 0;

            hr = _AdvSinks.FindNext(pwzItemMimes, (IUnknown **)&pWChkItemSink);

        } while (hr == NOERROR);

    }

    TransDebugOut((DEB_PROT, "%p OUT COInetAdvisor::SendAdvise (hr:%lx)\n",this, hr1));
    return hr1;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapStrToXVal::AddVal
//
//  Synopsis:
//
//  Arguments:  [LPCWSTR] --
//              [ULONG] --
//              [cNames] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapStrToXVal::AddVal(IUnknown *pUnk, LPCWSTR pwzName)
{
    TransDebugOut((DEB_PROT, "%p _IN CMapStrToXVal::AddVal\n", this));
    HRESULT hr = NOERROR;

    TransAssert((pUnk && pwzName));
    CLock lck(_mxs);

    CXUnknown cunk = pUnk;

    if (pUnk && pwzName)
    {
        CProtNode *pNode;
        CKey ckey = pwzName;

        if (_Map.Lookup(ckey, (CObject *&)pNode) )
        {
            pNode->Add(pUnk);
        }
        else
        {
           pNode = new CProtNode(pUnk);
           if (pNode)
           {
               _Map.SetAt(ckey, pNode);
               _cElements++;
           }
           else
           {
               hr = E_OUTOFMEMORY;
           }
        }

    }
    else
    {
        hr = E_INVALIDARG;
    }

    TransDebugOut((DEB_PROT, "%p OUT CMapStrToXVal::AddVal (hr:%lx)\n",this, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CMapStrToXVal::RemoveVal
//
//  Synopsis:
//
//  Arguments:  [pUnk] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapStrToXVal::RemoveVal(IUnknown *pUnk,LPCWSTR pwzName)
{
    TransDebugOut((DEB_PROT, "%p _IN CMapStrToXVal::RemoveVal\n", this));
    HRESULT hr = NOERROR;

    CLock lck(_mxs);

    if (pUnk && pwzName)
    {
        CProtNode *pNode;
        CKey ckey = pwzName;

        if (_Map.Lookup(ckey, (CObject *&)pNode) )
        {
            pNode->Remove(pUnk);
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    TransDebugOut((DEB_PROT, "%p OUT CMapStrToXVal::RemoveVal (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapStrToXVal::FindFirst
//
//  Synopsis:
//
//  Arguments:  [pwzName] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapStrToXVal::FindFirst(LPCWSTR pwzName, IUnknown **ppUnk)
{
    TransDebugOut((DEB_PROT, "%p _IN CMapStrToXVal::FindFirst\n", this));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    TransAssert(( pwzName && ppUnk));

    if (_cElements)
    {
        CProtNode *pNode;
        CKey ckey = pwzName;

        if (   (_Map.Lookup(ckey, (CObject *&)pNode) )
            && (pNode->FindFirst(*ppUnk)) )
        {
            hr = NOERROR;
        }
    }

    TransDebugOut((DEB_PROT, "%p OUT CMapStrToXVal::FindFirst (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CMapStrToXVal::FindNext
//
//  Synopsis:
//
//  Arguments:  [pwzName] --
//              [ppUnk] --
//
//  Returns:
//
//  History:    10-29-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CMapStrToXVal::FindNext(LPCWSTR pwzName, IUnknown **ppUnk)
{
    TransDebugOut((DEB_PROT, "%p _IN CMapStrToXVal::FindNext\n", this));
    HRESULT hr = E_FAIL;

    CLock lck(_mxs);
    TransAssert(( pwzName && ppUnk));

    if (_cElements)
    {
        CProtNode *pNode;
        CKey ckey = pwzName;

        if (   (_Map.Lookup(ckey, (CObject *&)pNode) )
            && (pNode->FindNext(*ppUnk)) )
        {
            hr = NOERROR;
        }
    }

    TransDebugOut((DEB_PROT, "%p OUT CMapStrToXVal::FindNext (hr:%lx)\n",this, hr));
    return hr;
}



