
#include <notiftn.h>

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationReport::QueryInterface
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
STDMETHODIMP CNotificationReport::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationReport::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown) || (riid == IID_INotificationReport) )
    {
        *ppvObj = (INotificationReport *)this;
        AddRef();
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationReport::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationReport::AddRef
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
STDMETHODIMP_(ULONG) CNotificationReport::AddRef(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationReport::AddRef\n", this));
    NotfAssert((_pCPackage));

    ULONG lRet = _pCPackage->ReportAddRef(_NotfReportObjType);

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationReport::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CNotificationReport::Release
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
STDMETHODIMP_(ULONG) CNotificationReport::Release(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationReport::Release\n", this));
    NotfAssert((_pCPackage));

    ULONG lRet = _pCPackage->ReportRelease(_NotfReportObjType);

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationReport::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationReport::DeliverUpdate
//
//  Synopsis:   delivers a notification without scheduling it
//
//  Arguments:  [pNotification] --
//              [deliverMode] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-14-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationReport::DeliverUpdate(
                            // the reply notification object and not the object handled right now
                            LPNOTIFICATION          pNotification,
                            DELIVERMODE             deliverMode,
                            DWORD                   dwReserved
                            )
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationReport::DeliverUpdate (CPackage:%p)\n", this, _pCPackage));

    HRESULT hr = E_FAIL;

    NotfAssert((_pCPackage));

    PNOTIFICATIONCOOKIE   pRunningNotfCookie = _pCPackage->GetRunningCookie();

    if (   IS_BAD_NULL_INTERFACE(pNotification)
        || !pRunningNotfCookie  //BUGBUG - this isn't an invalid arg - it's an internal error
        || (deliverMode & ~DELIVERMODE_ALL) 
       )
    {
        NotfAssert((FALSE && "Invalid parameter passed to CNotificationReport::DeliverUpdate"));
        hr = E_INVALIDARG;
    }
    else if (!pRunningNotfCookie)
    {
        // NOTE: the sender might have revoked the object
        // shall we allow to revoke the object ??
    }
    else
    {
        deliverMode &= ~(DM_NEED_COMPLETIONREPORT | DM_NEED_PROGRESSREPORT | DM_THROTTLE_MODE | DM_DELIVER_DEFAULT_PROCESS | DM_DELIVER_DEFAULT_THREAD);
        
        NotfAssert((pRunningNotfCookie));
        CPackage *pCPackage = 0;
        hr = CPackage::CreateUpdate(
                                this,
                                pNotification,
                                deliverMode,
                                dwReserved,
                                &pCPackage
                                );
        if (hr == NOERROR)
        {
            NotfAssert((pCPackage));


            if (_pCPackage->GetNotificationState() & PF_CROSSPROCESS)
            {
                pCPackage->SetNotificationState(pCPackage->GetNotificationState() | PF_CROSSPROCESS);
                pCPackage->SetDestID(_pCPackage->GetDestID());
                CDestinationPort &rCDest = _pCPackage->GetCDestPort();
                NotfAssert(( rCDest.GetPort()  != GetThreadNotificationWnd() ));
                pCPackage->SetDestPort(rCDest.GetPort(), rCDest.GetDestThreadId() );
            }



            // note: pDelAgent is not addref'd - global dude
            CDeliverAgent *pDelAgent = GetDeliverAgent();
            NotfAssert((pDelAgent));
            if (pDelAgent)
            {
                hr = pDelAgent->HandlePackage(pCPackage);
            }

            RELEASE(pCPackage);
            pCPackage = 0;
        }

        NotfAssert((!pCPackage));
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationReport::DeliverUpdate (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationReport::GetOriginalNotification
//
//  Synopsis:
//
//  Arguments:  [ppNotification] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:      get the original notification this report objet belongs too
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationReport::GetOriginalNotification(
                LPNOTIFICATION          *ppNotification
                )
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationReport::GetOriginalNotification\n", this));
    HRESULT hr = NOERROR;

    if (!ppNotification)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        // get an addref'f pointer
        LPNOTIFICATION pNotfObj = _pCPackage->GetNotification(TRUE);
        if (pNotfObj)
        {
            *ppNotification = pNotfObj;
        }
        else
        {
            *ppNotification = 0;
            hr = E_FAIL;
        }
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationReport::GetOriginalNotification (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CNotificationReport::GetNotificationStatus
//
//  Synopsis:
//
//  Arguments:  [dwStatusIn] --
//              [pdwStatusOut] --
//              [dwReserved] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CNotificationReport::GetNotificationStatus(
                // what kind of status
                DWORD                   dwStatusIn,
                // flags if update notification is pending etc.
                DWORD                  *pdwStatusOut,
                DWORD                   dwReserved
                )
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CNotificationReport::GetNotificationStatus\n", this));
    HRESULT hr = E_NOTIMPL;

    NotfDebugOut((DEB_PACKAGE, "%p OUT CNotificationReport::GetNotificationStatus (hr:%lx\n", this,hr));
    return hr;
}


