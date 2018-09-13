//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1997                    **
//*********************************************************************

#ifndef _NOTSINK_H_

#include "notify.h"
#include "webcheck.h"

class CNotificationSink : public INotificationSink
{
private:
    NOTIFICATIONCOOKIE  _ncNotificationCookie;
    BSTR                _pwzURL;
    DWORD               _cRef;

public:
    CNotificationSink(BSTR pwzURL);
    ~CNotificationSink();

    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) ;
    virtual STDMETHODIMP_(ULONG) Release(void);

    // *** INotificationSink methods ***
    virtual STDMETHODIMP OnNotification( 
        /* [in] */ LPNOTIFICATION pn,
        /* [in] */ LPNOTIFICATIONREPORT pnr,
        /* [in] */ DWORD dwReserved);

};

HRESULT RegisterURLWithWebcheck (BSTR wz, PNOTIFICATIONCOOKIE pnc);
HRESULT RemoveURLFromWebcheck (BSTR wz, PNOTIFICATIONCOOKIE pnc);
HRESULT CNotificationSink_Unregister(PNOTIFICATIONCOOKIE pnc);
HRESULT CNotificationSink_Register(INotificationSink *pns, PNOTIFICATIONCOOKIE pnc);

#endif // _NOTSINK_H_
