#ifndef _NOTFSINK_HXX_
#define _NOTFSINK_HXX_

#include <notftn.h>

class CNotfSink : public INotificationSink , public IDebugRegister
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP OnNotification(
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
        );

    STDMETHODIMP GetFacilities (LPCWSTR *ppwzNames, DWORD *pcNames, DWORD dwReserved);
    STDMETHODIMP Register ( LPCWSTR pwzName, IDebugOut *pDbgOut, DWORD dwFlags, DWORD dwReserved);

    // internal methods
    BOOL IsApartmentThread()
    {
       EProtAssert((_ThreadId != 0));
       return (_ThreadId == GetCurrentThreadId());
    }


public:
    CNotfSink(LPCWSTR pwzName ) : _CRefs(1)
    {
        _pwzName= pwzName;
        _ThreadId = GetCurrentThreadId();
    }

    ~CNotfSink()
    {
    }


private:
    CRefCount           _CRefs;          // the total refcount of this object
    DWORD               _ThreadId;
    LPCWSTR             _pwzName;
    //PNOTIFICATION            _pNotification;
};

#endif //_NOTFSINK_HXX_

