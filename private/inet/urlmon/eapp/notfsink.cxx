#include <eapp.h>

extern "C" const GUID IID_IDebugRegister;
extern "C" const GUID IID_IDebugOut;

#if DBG==1
HRESULT DumpIID(REFIID riid)
{

    HRESULT hr;
    LPOLESTR pszStr = NULL;
    hr = StringFromCLSID(riid, &pszStr);
    EProtDebugOut((DEB_NOTFSINK, "API >>> DumpIID (riid:%ws) \n", pszStr));

    if (pszStr)
    {
        delete pszStr;
    }
    return hr;
}
#else
#define DumpIID(x)
#endif


HRESULT CreateNotificationTest(DWORD dwId, REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk)
{
    EProtDebugOut((DEB_NOTFSINK, "API _IN CreateKnownProtocolInstance\n"));
    HRESULT hr = NOERROR;

    EProtAssert(( (dwId == 0) && ppUnk));

    do
    {
        if (!ppUnk)
        {
            // Note: aggregation only works if asked for IUnknown
            EProtAssert((FALSE && "Dude, need out parameter"));
            hr = E_INVALIDARG;
            break;
        }

        if (pUnkOuter)
        {
            hr = CLASS_E_NOAGGREGATION;
            break;
        }

        CNotfSink *pNotfSink = new CNotfSink(L"Notificaion Sink");

        if (pNotfSink)
        {
            hr = pNotfSink->QueryInterface(riid, (void **)ppUnk);
            // remove extra refcount
            pNotfSink->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }

        break;
    } while (TRUE);



    EProtDebugOut((DEB_NOTFSINK, "API OUT CreateKnownProtocolInstance(hr:%lx)\n", hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CNotfSink::QueryInterface
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
STDMETHODIMP CNotfSink::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    EProtDebugOut((DEB_NOTFSINK,  "%p _IN CNotfSink::QueryInterface\n", this));
    EProtAssert(( IsApartmentThread() ));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_INotificationSink) )
    {
        *ppvObj = this;
        AddRef();
    }
    else if ((riid == IID_IDebugRegister) )
    {
        *ppvObj = (IDebugRegister *) this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    EProtDebugOut((DEB_NOTFSINK,  "%p OUT CNotfSink::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotfSink::AddRef
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
STDMETHODIMP_(ULONG) CNotfSink::AddRef(void)
{
    EProtDebugOut((DEB_NOTFSINK,  "%p _IN CNotfSink::AddRef\n", this));
    EProtAssert(( IsApartmentThread() ));

    LONG lRet = ++_CRefs;

    EProtDebugOut((DEB_NOTFSINK,  "%p OUT CNotfSink::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotfSink::Release
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
STDMETHODIMP_(ULONG) CNotfSink::Release(void)
{
    EProtDebugOut((DEB_NOTFSINK,  "%p _IN CNotfSink::Release\n", this));
    EProtAssert(( IsApartmentThread() ));

    LONG lRet = --_CRefs;

    if (_CRefs == 0)
    {
        delete this;
    }

    EProtDebugOut((DEB_NOTFSINK,  "%p OUT CNotfSink::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Method:     CNotfSink::OnNotification
//
//  Synopsis:
//
//  Arguments:  [itemtype] --
//              [pWChkItem] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    12-02-1996   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotfSink::OnNotification(
                // the notification object itself
                LPNOTIFICATION          pNotification,
                // the cookie of the object the notification is targeted too
                //PNOTIFICATIONCOOKIE     pRunningNotfCookie,
                // flags how it was delivered and how it should
                // be processed
                //NOTIFICATIONFLAGS       grfNotification,
                // the report sink if - can be NULL
                LPNOTIFICATIONREPORT    pNotfctnReport,
                DWORD                   dwReserved
                )
{
    EProtDebugOut((DEB_NOTFSINK, "%p _IN CNotfSink::OnNotification\n", this));
    EProtAssert(( IsApartmentThread() ));
    EProtAssert((pNotification));
    HRESULT hr = NOERROR;
    LPOLESTR pszStr = NULL;
    NOTIFICATIONTYPE notfType;
    NOTIFICATIONCOOKIE notfCookie;

    PNOTIFICATIONCOOKIE     pRunningNotfCookie = 0;
    // flags how it was delivered and how it should
    // be processed
    NOTIFICATIONFLAGS       grfNotification = (NOTIFICATIONFLAGS)0;

    //hr = pNotification->GetNotificationType(&notfType);
    hr = pNotification->GetNotificationInfo(&notfType, &notfCookie, 0, 0, 0);

    if (hr == NOERROR)
    {
        hr = StringFromCLSID(notfType, &pszStr);
    }

    EProtDebugOut((DEB_NOTFSINK, "%p CNotfSink:%ws received pNotification:%p\n         with id:%ws\n", this, _pwzName, pNotification,pszStr));
    //EProtDebugOut((DEB_NOTFSINK, "%p CNotfSink:%ws received pNotification:%p \n", this, _pwzName, pNotification));
    MessageBeep(1000);

    if (pszStr)
    {
        delete pszStr;
    }

    /*
    if (pNotification->pCustomData != 0)
    {
        IDebugOut *pDbgOut = 0;
        char szOutBuffer[256];

        pDbgOut = (IDebugOut *) pNotification->pCustomData;

        sprintf(szOutBuffer, "%p CNotfSink:%ws received pNotification:%p with id:%ws\n", this, _pwzName, pNotification,  pNotification->pNotificationid);

        pDbgOut->SendEntry(GetCurrentThreadId(), 0, szOutBuffer, 0);
    }
    */

    // don't return an error!
    hr = NOERROR;

    EProtDebugOut((DEB_NOTFSINK,  "%p OUT CNotfSink::OnNotification (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CNotfSink::GetFacilities (LPCWSTR *ppwzNames, DWORD *pcNames, DWORD dwReserved)
{
    EProtDebugOut((DEB_NOTFSINK, "%p _IN CNotfSink::GetFacilities\n", this));
    EProtAssert(( IsApartmentThread() ));
    EProtAssert(( ppwzNames && pcNames ));

    HRESULT hr = NOERROR;

    ppwzNames = v_gDbgFacilitieNames;
    LONG x = sizeof(*v_gDbgFacilitieNames);
    LONG y = sizeof(LPCWSTR);

    *pcNames = (sizeof(*v_gDbgFacilitieNames)/sizeof(LPCWSTR));

    EProtDebugOut((DEB_NOTFSINK,  "%p OUT CNotfSink::GetFacilities (hr:%lx)\n",this, hr));
    return hr;
}

STDMETHODIMP CNotfSink::Register ( LPCWSTR pwzName, IDebugOut *pDbgOut, DWORD dwFlags, DWORD dwReserved)
{
    EProtDebugOut((DEB_NOTFSINK, "%p _IN CNotfSink::Register\n", this));
    EProtAssert(( IsApartmentThread() ));
    HRESULT hr = NOERROR;

    hr = RegisterDebugOut(pwzName, dwFlags, pDbgOut, dwReserved);

    EProtDebugOut((DEB_NOTFSINK,  "%p OUT CNotfSink::Register (hr:%lx)\n",this, hr));
    return hr;
}


