#include <windows.h>
#include <stdio.h>
#include "notify.h"
#include "webcheck.h"

int iState = 1;


class CDialTest : public INotificationSink
{
public:
    CDialTest();
    ~CDialTest();

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID, void **);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // INotificationSink members
    STDMETHODIMP         OnNotification(
                INotification          *pNotification,
                INotificationReport    *pNotificationReport,
                DWORD                   dwReserved);

    HRESULT Dial(void);
    HRESULT OnConnected(INotificationReport *);
    HRESULT OnDisconnected(void);
    HRESULT SendNotification(NOTIFICATIONTYPE type);

    long    m_cRef;
};

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//
// IUnknown members
//
STDMETHODIMP CDialTest::QueryInterface(REFIID riid, void ** ppv)
{
    *ppv=NULL;

    // Validate requested interface
    if ((IID_IUnknown == riid) ||
        (IID_INotificationSink == riid))
    {
        *ppv=(INotificationSink*)this;
    } else {
        return E_NOINTERFACE;
    }

    ((LPUNKNOWN)*ppv)->AddRef();
    return NOERROR;
}


STDMETHODIMP_(ULONG) CDialTest::AddRef(void)
{
    return ++m_cRef;
}


STDMETHODIMP_(ULONG) CDialTest::Release(void)
{
    if( 0L != --m_cRef )
        return m_cRef;

    delete this;
    return 0L;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

// 
// INotificationSink members
//
STDMETHODIMP CDialTest::OnNotification(
                INotification          *pNotification,
                INotificationReport    *pNotReport,
                DWORD                   dwReserved)
{
    // Depending on the notification type, we call our other
    //  existing members.
    NOTIFICATIONTYPE    nt;
    HRESULT     hr;

    hr = pNotification->GetNotificationInfo(&nt, NULL,NULL,NULL,0);

    if (FAILED(hr)) {
        printf("OnNotification failed to get notification type\n");
        return E_INVALIDARG;
    }

    if (IsEqualGUID(nt, NOTIFICATIONTYPE_INET_ONLINE)) {
        printf("OnNotification: INET_ONLINE\n");
        iState = 2;
    } else if (IsEqualGUID(nt, NOTIFICATIONTYPE_INET_OFFLINE)) {
        printf("OnNotification: INET_OFFLINE\n");
    } else if (IsEqualGUID(nt, NOTIFICATIONTYPE_BEGIN_REPORT)) {
        printf("OnNotification: BEGIN_REPORT\n");
    } else if (IsEqualGUID(nt, NOTIFICATIONTYPE_END_REPORT)) {
        printf("OnNotification: END_REPORT\n");
        switch(iState) {
            case 0:
                // trying to exit...
                break;
            case 1:
                // dial failed, we're done
                iState = 0;
                break;
            case 2:
                // connected successfully - now hang up
                printf("Sending hangup notification.\n");
                SendNotification(NOTIFICATIONTYPE_DISCONNECT_FROM_INTERNET);
                iState = 3;
                break;
            case 3:
                // hung up.  We're done.
                iState = 0;
                break;
            default :
                break;
            } /* switch */
    } else printf("OnNotification unknown notification type\n");

    // Avoid bogus assert
    if (SUCCEEDED(hr)) hr = S_OK;
    
    return hr;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

//
// Other members
//
HRESULT CDialTest::SendNotification(NOTIFICATIONTYPE type)
{
    HRESULT             hr;
    INotificationMgr    *pNotificationMgr = NULL;
    INotification       *pNotification;

    // get notification manager
    hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, CLSCTX_INPROC_SERVER,
                            IID_INotificationMgr, (void**)&pNotificationMgr);

    if(SUCCEEDED(hr)) {
        // create a notification
        hr = pNotificationMgr->CreateNotification(
                type,
                (NOTIFICATIONFLAGS)0,
                NULL,
                &pNotification,
                0);
    }

    if(SUCCEEDED(hr)) {
        // deliver it
        hr = pNotificationMgr->DeliverNotification(
                pNotification,
                CLSID_DialAgent,
                (DELIVERMODE)(DM_DELIVER_IMMEDIATELY | DM_NEED_COMPLETIONREPORT),
                (INotificationSink *)this,
                NULL,
                0);
        pNotification->Release();
    }

    if(pNotificationMgr)
        pNotificationMgr->Release();

    return SUCCEEDED(hr);
}

CDialTest::CDialTest()
{
    m_cRef = 0;
}

CDialTest::~CDialTest()
{
}

HRESULT CDialTest::Dial(void)
{
    printf("Sending dial notification.\n");
    return SendNotification(NOTIFICATIONTYPE_CONNECT_TO_INTERNET);
}

int __cdecl main()
{
    BOOL fDone = FALSE;
    DWORD dwRes;
    MSG msg;
    CDialTest s;

    if(FAILED(CoInitialize(NULL)))
        return TRUE;

    s.Dial();

    while(iState && GetMessage(&msg, NULL, 0, 0)) {
        if(msg.hwnd != NULL) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    CoUninitialize();

    return 0;
}

