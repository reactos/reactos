//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       update.cpp
//
//  Contents:   update subscriptions agent
//
//  Classes:
//
//  Functions:
//
//  History:    01-14-1997  rayen (Raymond Endres)  Created
//
//----------------------------------------------------------------------------

//xnotfmgr - darremi owns this

#include "private.h"
#include "offline.h"
#include "offl_cpp.h"

#undef TF_THISMODULE
#define TF_THISMODULE   TF_UPDATEAGENT

const IDMSG_NOTHING         = 100 + IDCANCEL;
const IDMSG_INITFAILED      = 101 + IDCANCEL;
const IDMSG_SESSIONEND      = 102 + IDCANCEL;
const IDMSG_UPDATEBEGIN     = 103 + IDCANCEL;
const IDMSG_UPDATEPROGRESS  = 104 + IDCANCEL;
const IDMSG_ADJUSTPROBAR    = 105 + IDCANCEL;

const TID_UPDATE = 7405;        //  TimerID, BUGBUGBUG
const TID_STATISTICS = 1243;    //  TimerID for statistics update

#define SHRESTARTDIALOG_ORDINAL    59       // restart only exported by ordinal
typedef BOOL (WINAPI *SHRESTARTDIALOG)( HWND, LPTSTR, DWORD );

CUpdateAgent    * g_pUpdate = NULL;
BOOL    CUpdateDialog::m_bDetail = FALSE;

CDialHelper::CDialHelper() : m_cRef(1),
                             m_iDialerStatus(DIALER_OFFLINE)
{
    ASSERT(0 == m_cConnection);
    ASSERT(NULL == m_pController);
}

STDMETHODIMP_(ULONG) CDialHelper::AddRef(void)
{
//    DBG("CDialHelper::AddRef");
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CDialHelper::Release(void)
{
//    DBG("CDialHelper::Release");
    if( 0L != --m_cRef )
        return m_cRef;

    DBG("CDialHelper::Release, ref count down to 0");
    ASSERT(!m_cConnection);
    ASSERT(m_iDialerStatus == DIALER_OFFLINE);
    if (m_iDialerStatus != DIALER_OFFLINE) {
        DBG("CDialHelper::Release, send disconnect message(abnormal)");
        NotifyAutoDialer(NOTIFICATIONTYPE_TASKS_COMPLETED);
    }
    SAFERELEASE(m_pNotMgr);

    PostThreadMessage(m_ThreadID, UM_DECREASE, 0, 0);
    delete this;
    return 0L;
}

STDMETHODIMP CDialHelper::QueryInterface(REFIID riid, void ** ppv)
{
//    DBG("CDialHelper::QueryInterface");
    *ppv = NULL;

    if ((IID_IUnknown == riid) ||
        (IID_INotificationSink == riid))
    {
        *ppv = (INotificationSink *)this;
    }

    if( NULL != *ppv ) 
    {
//        DBG("CDialHelper::QueryInterface/AddRef");
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP CDialHelper::OnNotification(
    LPNOTIFICATION         pNotification,
    LPNOTIFICATIONREPORT   pNotificationReport,
    DWORD                  dwReserved)
{
//    DBG("CDialHelper::OnNotification called");
    ASSERT(pNotification);
    
    // Extract Notification Type
    HRESULT          hr;
    NOTIFICATIONTYPE notfType;
    hr = pNotification->GetNotificationInfo(&notfType, NULL, NULL, NULL, 0);
    ASSERT(SUCCEEDED(hr));

    if (NOTIFICATIONTYPE_PROGRESS_REPORT == notfType) {
        // get hresult from notification
        HRESULT hrConnect;
        ReadSCODE(pNotification, NULL, c_szPropStatusCode, &hrConnect);
        if(SUCCEEDED(hrConnect))
            return OnInetOnline(pNotification);
        else
            return OnInetOffline(pNotification);
    } else {
        DBG("CDialHelper::OnNotification - Unknown notification");
        return S_OK;
    }
}

STDMETHODIMP CDialHelper::Init(CUpdateController * pController)
{
    ASSERT(pController);
    m_pController = pController;
    ASSERT(pController->m_pNotMgr);
    m_pNotMgr = pController->m_pNotMgr;
    m_ThreadID = pController->m_ThreadID;
    m_pNotMgr->AddRef();

    return S_OK;
}

STDMETHODIMP CDialHelper::DialOut(void)
{
    HRESULT hr = S_OK;

    if (m_iDialerStatus == DIALER_OFFLINE)  {
        DBG("CDialHelper::DialOut - Dialing Out");
        hr = NotifyAutoDialer(NOTIFICATIONTYPE_AGENT_START);
        if (SUCCEEDED(hr))  {
            m_iDialerStatus = DIALER_CONNECTING;
            m_cConnection ++;
        }
    }


    return hr;
}

STDMETHODIMP CDialHelper::HangUp(void)
{
    m_cConnection --;

    if (!m_cConnection) {
        if (m_iDialerStatus != DIALER_OFFLINE)  {
            DBG("CDialHelper::HangUp - Hanging up");
            m_iDialerStatus = DIALER_OFFLINE;
            NotifyAutoDialer(NOTIFICATIONTYPE_TASKS_COMPLETED);
        }
    }

    return S_OK;
}

STDMETHODIMP CDialHelper::CleanUp()
{
    m_pController = NULL;
    return S_OK;
}

STDMETHODIMP CDialHelper::OnInetOnline(
            INotification          *pNotification)
{
    HRESULT hr=S_OK;

    if (m_iDialerStatus == DIALER_CONNECTING)
    {
        DBG("Dial Helper: CONNECTION SUCCESSFUL, BEGINNING DOWNLOAD");

        m_iDialerStatus = DIALER_ONLINE;
        if (m_pController)
            m_pController->StartService();
        else
            HangUp();
    }

    return hr;
}

STDMETHODIMP CDialHelper::OnInetOffline(
            INotification          *pNotification)
{
    DBG("Dial Helper: received InetOffline, aborting");
    SCODE   eCode = S_OK;
    TCHAR szCaption[128];
    TCHAR szString[1024];
    CONNECT_ERROR   error;

    if (m_iDialerStatus == DIALER_CONNECTING)   {
        error = E_ATTEMPT_FAILED;
    } else  {
        error = E_CONNECTION_LOST;
    }

    if (SUCCEEDED(ReadSCODE(pNotification, NULL, c_szPropStatusCode, & eCode)))   {
        UINT uID;
        if (eCode == E_INVALIDARG)  {
             uID = IDS_STRING_E_CONFIG; 
        } else if (E_ABORT == eCode) {
            uID = IDS_STRING_E_SECURITYCHECK;
        } else {
            uID = IDS_STRING_E_FAILURE;
        }
        MLLoadString(uID, szString , ARRAYSIZE(szString));
        MLLoadString(IDS_CAPTION_ERROR_CONNECTING, szCaption, ARRAYSIZE(szCaption));
        
        MessageBox(NULL, szString, szCaption, MB_ICONWARNING | MB_SYSTEMMODAL);
    }

    if (m_pController)
        m_pController->StopService(error);
    else
        HangUp();

    m_iDialerStatus = DIALER_OFFLINE;

    return S_OK;
}

STDMETHODIMP CDialHelper::NotifyAutoDialer(NOTIFICATIONTYPE pType)
{
    HRESULT hr;
    INotification * pNot = NULL;
    ASSERT(m_pNotMgr);

    hr = m_pNotMgr->CreateNotification(
                pType,
                (NOTIFICATIONFLAGS) 0,
                NULL,
                &pNot,
                0);

    if (pNot)
    {
        INotificationSink *pSink = NULL;

        if (pType == NOTIFICATIONTYPE_AGENT_START)
        {
            DBG("CDialHelper::NotifyAutoDialer AGENT_START");
            pSink = (INotificationSink *)this;

            // HACK HACK [darrenmi] Until DZhang yanks out the umbrella code
            // we need something here - tell conn agent to let this connection
            // slide
            WriteAnsiSTR(pNot, NULL, c_szPropURL, TEXT("<override>"));

            // have not mgr deliver for us
            hr = m_pNotMgr->DeliverNotification(
                        pNot, 
                        CLSID_ConnectionAgent, 
                        DM_NEED_COMPLETIONREPORT | DM_DELIVER_DEFAULT_PROCESS,
                        pSink, &m_pConnAgentReport, 0);

        } else {
            DBG("CDialHelper::NotifyAutoDialer TASKS_COMPLETED");
            if(m_pConnAgentReport) {
                // deliver using the sink we've already got
                hr = m_pConnAgentReport->DeliverUpdate(pNot, 0, 0);
                TraceMsg(TF_THISMODULE, "CDialHelper::NotifyAutoDialer releasing report pointer");
                SAFERELEASE(m_pConnAgentReport);
            }
        }

        SAFERELEASE(pNot);
    }

    return hr;
}

DWORD WINAPI DialogThreadProc(LPVOID pData)
{
    ASSERT(pData);
    CUpdateDialog * pDialog = (CUpdateDialog *)pData;
    MSG     msg;

    pDialog->m_ThreadID = GetCurrentThreadId();

#ifdef DEBUG
    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }
    if(g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_BEGIN, TEXT(__FILE__), __LINE__);
#endif 

    while (GetMessage(&msg, NULL, 0, 0))    {
        switch (msg.message)    {
        case UM_READY:  
            {
                pDialog->Init(NULL, (CUpdateController *)msg.lParam);
                pDialog->Show(TRUE);
            }
            break;
        default:
            IsDialogMessage(pDialog->m_hDlg, &msg);
            break;
        }
    }
#ifdef DEBUG
    if(g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_END, TEXT(__FILE__), __LINE__);
    
#endif 
    DBG("DialogThreadProc returning");
    return 0;
}

// application subscription channels can force a reboot
void DoReboot()
{
    HRESULT hrReboot = S_OK;
    HINSTANCE    hShell32Lib;

    DBG("UpdateThreadProc returning - attempting reboot");
    SHRESTARTDIALOG          pfSHRESTARTDIALOG = NULL;

    if ((hShell32Lib = LoadLibrary("shell32.dll")) != NULL) {

        if (!(pfSHRESTARTDIALOG = (SHRESTARTDIALOG)
            GetProcAddress( hShell32Lib, MAKEINTRESOURCE(SHRESTARTDIALOG_ORDINAL)))) {

            hrReboot = HRESULT_FROM_WIN32(GetLastError());

        } else {
            //BUGBUG: What hwnd to use?
            pfSHRESTARTDIALOG(NULL, NULL, EWX_REBOOT);
        }

    } else  {
        hrReboot = HRESULT_FROM_WIN32(GetLastError());
    }

    if (hShell32Lib) {
        FreeLibrary(hShell32Lib);
    }
}

DWORD WINAPI UpdateThreadProc(LPVOID pData)
{
    ASSERT(pData);
    CUpdateController * pController = (CUpdateController *) pData;
    INotification   * pNotification = NULL;
    MSG     msg;
    int     l_cObj;
    BOOL    bNeedReboot = FALSE;

    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        DBG("UpdateThreadProc exiting, failed to CoInitialize.");
        return hr;
    }

#ifdef DEBUG
    if(!g_fInitTable)
    {
        if(GetLeakDetectionFunctionTable(&LeakDetFunctionTable))
            g_fInitTable = TRUE;
    }
    if(g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_BEGIN, TEXT(__FILE__), __LINE__);
#endif 
    while (GetMessage(&msg, NULL, 0, 0))    {
        switch (msg.message)    {
        case UM_ONREQUEST:
            if (!pController->m_fInit)
                break;
            pNotification = (INotification *)msg.lParam;
            //  BUGBUG. There is a chance we fail in OnRequest (Failed to
            //  send out Notification to dialer agent).
            hr = pController->OnRequest(pNotification);
            if (FAILED(hr))  {
                ASSERT(0);
                if ((!pController->m_count) && pController->m_pDialog)
                {
                    PostMessage(pController->m_pDialog->m_hDlg,
                                        WM_COMMAND, IDMSG_SESSIONEND, 0);
                }
            }
            SAFERELEASE(pNotification);
            break;
        case UM_BACKGROUND:
            if (!pController->m_fInit)
                break;
            break;
        case UM_ONABORT:
            if (!pController->m_fInit)
                break;
#ifdef  DEBUG
            hr =
#endif
            pController->Abort();
#ifdef  DEBUG
            ASSERT(SUCCEEDED(hr));
#endif
            break;
        case UM_ONSKIP:
            if (!pController->m_fInit)
                break;
#ifdef  DEBUG
            hr =
#endif
            pController->Skip();
#ifdef  DEBUG
            ASSERT(SUCCEEDED(hr));
#endif
            break;
        case UM_ONADDSINGLE:
            if (!pController->m_fInit)
                break;
            MemFree((HLOCAL)msg.lParam);
            break;
        case UM_ONSKIPSINGLE:
            if (!pController->m_fInit)
                break;
#ifdef  DEBUG
            hr =
#endif
            pController->SkipSingle((CLSID *)msg.lParam);
            MemFree((HLOCAL)msg.lParam);
#ifdef  DEBUG
            ASSERT(SUCCEEDED(hr));
#endif
            break;
        case UM_CLEANUP:
            pController->CleanUp();
            pController->Release();
            break;
        case UM_READY:
            pController->AddRef();
            if (FAILED(pController->Init((CUpdateDialog *)msg.lParam))) {
                DBG("UpdateThreadProc - failed to init controller");
                CUpdateDialog * pDlg = (CUpdateDialog *)msg.lParam;
                PostMessage(pDlg->m_hDlg, WM_COMMAND, IDMSG_INITFAILED, 0);
            } else  {
                l_cObj = 2;
            }
            break;
        case UM_DECREASE:
            l_cObj --;
            if (!l_cObj)
                goto QUIT;
            break;
        case UM_NEEDREBOOT:
            bNeedReboot = TRUE;
            break;

        default:
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            break;
        }
    }

QUIT:   ;
#ifdef DEBUG
    if(g_fInitTable)
        LeakDetFunctionTable.pfnDebugMemLeak(DML_TYPE_THREAD | DML_END, TEXT(__FILE__), __LINE__);
#endif 
    CoUninitialize();

    //BUGBUG: This may need to be moved to a more appropriate location
    if (bNeedReboot)
        DoReboot();

    DBG("UpdateThreadProc returning");
    return 0;
}

STDMETHODIMP CUpdateController::ResyncData()
{
    return S_OK;
}

STDMETHODIMP CUpdateController::StartPending(void)
{
    HRESULT hr;

    DBG("CUpdateController::StartPending - entered");
    ASSERT(GetCurrentThreadId() == m_ThreadID);

    ASSERT(m_pDialer);
    ASSERT(m_pDialer->m_iDialerStatus == DIALER_ONLINE);
    
    for ( UINT ui = 0; ui < m_cReportCount; ui ++)  {
        if (m_aReport[ui].status == ITEM_STAT_PENDING)  {
            hr = DispatchRequest(&(m_aReport[ui]));
            ASSERT(SUCCEEDED(hr));
        }
    }

    return S_OK;
}

STDMETHODIMP CUpdateController::StartService(void)
{
    HRESULT hr = S_OK;

    DBG("CUpdateController: Start Service");
    ASSERT(GetCurrentThreadId() == m_ThreadID);
    ASSERT(!m_cFinished);

    hr = StartPending();

    if (!m_count)   {
        m_fSessionEnded = TRUE;
        m_pDialer->HangUp();
        if (m_pDialog)
            PostMessage(m_pDialog->m_hDlg, WM_COMMAND, IDMSG_SESSIONEND, 0);
    } else if (m_pDialog)
        PostMessage(m_pDialog->m_hDlg, WM_COMMAND, IDMSG_UPDATEBEGIN, 0);
        
    return S_OK;
}

STDMETHODIMP CUpdateController::StopService(CONNECT_ERROR err)
{
    DBG("Update Controller: Stop Service, aborting");
    ASSERT(GetCurrentThreadId() == m_ThreadID);
    HRESULT hr;

    if (!m_count && (err == E_ATTEMPT_FAILED))
    {
        if (m_pDialer)
            m_pDialer->HangUp();
    }

    for ( UINT ui = 0; ui < m_cReportCount; ui ++)  {
        switch (m_aReport[ui].status)   {
        case ITEM_STAT_UPDATING:
        case ITEM_STAT_QUEUED:
            hr = CancelRequest(&(m_aReport[ui]));
            if (FAILED(hr))
                break;
            else
                ;   //  Fall through.
        case ITEM_STAT_PENDING:
            m_aReport[ui].status = ITEM_STAT_ABORTED;
            if (m_pDialog)
                m_pDialog->RefreshStatus(&(m_aReport[ui].startCookie), NULL,  
                                            m_aReport[ui].status);
            break;
        default:
            break;
        }
    }

    if (!m_count)   {
        m_fSessionEnded = TRUE;
        PostMessage(m_pDialog->m_hDlg, WM_COMMAND, IDMSG_SESSIONEND, 0);
    }

    return S_OK;
}

STDMETHODIMP CUpdateController::IncreaseCount()
{
    ASSERT(m_count >= 0);
    InterlockedIncrement(&m_count);

    return S_OK;
}

STDMETHODIMP CUpdateController::DecreaseCount(CLSID * pCookie)
{
    InterlockedDecrement(&m_count);
    ASSERT(m_count >= 0);

    m_cFinished ++;
    // check for growing subscriptions (this could be better)
    if (m_cFinished > m_cTotal)
        m_cTotal = m_cFinished;

    if (m_count == 0)   {
        m_pDialer->HangUp();
    }

    if (m_pDialog)  {
        PostMessage(m_pDialog->m_hDlg, WM_COMMAND, IDMSG_UPDATEPROGRESS, 0);
        if (!m_count)   {
            m_fSessionEnded = TRUE;
            PostMessage(m_pDialog->m_hDlg, WM_COMMAND, IDMSG_SESSIONEND, 0);
        }
    }
    
    return S_OK;
}

STDMETHODIMP CUpdateController::GetItemList(UINT * pNewItem)
{
    DBG("CUpdateController::GetItemList - entered");

    NOTIFICATIONITEM    item;
    item.cbSize = sizeof(NOTIFICATIONITEM);
    ASSERT(m_pNotMgr);

    IEnumNotification * pEnumNot = NULL;
    HRESULT hr;
    ULONG   cItems = 0;
    UINT    count = 0;

    hr = m_pNotMgr->GetEnumNotification(0, &pEnumNot);
    RETURN_ON_FAILURE(hr);

    ASSERT(pEnumNot);

    hr = pEnumNot->Next(1, &item, &cItems);
    while (SUCCEEDED(hr) && cItems)
    {
        ASSERT(item.pNotification);

        if ((NOTIFICATIONTYPE_AGENT_START == item.NotificationType) &&
            (item.pNotification) &&
            (TASK_FLAG_HIDDEN & ~item.TaskData.dwTaskFlags))
        {
            SCODE   scodeLast;
            STATUS  statusLast;

            hr = ReadSCODE(item.pNotification, NULL, c_szPropStatusCode, &scodeLast);
            if (FAILED(scodeLast))  {
                statusLast = ITEM_STAT_FAILED;
            } else  {
                statusLast = ITEM_STAT_SUCCEEDED;
            }

            hr = AddEntry(&item, statusLast);
            if (SUCCEEDED(hr))  {
                count ++;
#ifdef  DEBUG
            } else  {
                DBGIID("CUpdateController::GetItemList - Failed to add entry", item.NotificationCookie);
#endif
            }
        }
        SAFERELEASE(item.pNotification);
        item.cbSize = sizeof(NOTIFICATIONITEM);
        hr = pEnumNot->Next(1, &item, &cItems);
    }

    if (pNewItem)
        *pNewItem = count;

    SAFERELEASE(pEnumNot);
    return hr;
}

STDMETHODIMP CUpdateController::Abort(void)
{
    DBG("CUpdateController::Abort - entered");

    ASSERT(GetCurrentThreadId() == m_ThreadID);
    ASSERT(m_pDialer);

    HRESULT hr = StopService(ITEM_STAT_ABORTED);

    return hr;
}

STDMETHODIMP CUpdateController::SkipSingle(CLSID * pCookie)
{
    ASSERT(pCookie);
    if (!pCookie)
        return E_INVALIDARG;

    HRESULT hr = S_OK;
    PReportMap pEntry = FindReportEntry(pCookie);
    if (pEntry) {
        switch (pEntry->status) {
        case ITEM_STAT_UPDATING:
        case ITEM_STAT_QUEUED:
            hr = CancelRequest(pEntry);
            if (FAILED(hr))
                break;
            else
                ;   //  Fall through.
        case ITEM_STAT_PENDING:
            pEntry->status = ITEM_STAT_SKIPPED;
            if (m_pDialog)
            {
                m_pDialog->RefreshStatus(pCookie, NULL, pEntry->status);
                //select first updating item in list, which should be skippable
                m_pDialog->SelectFirstUpdatingSubscription();
            }
            break;
        default:
            break;
        }
    }

    return hr;
}

STDMETHODIMP CUpdateController::Skip(void)
{
    DBG("CUpdateController::Skip - entered");
    HRESULT hr = S_OK;
    
    ASSERT(GetCurrentThreadId() == m_ThreadID);
    ASSERT(m_pDialog);
    UINT selCount = 0;

    hr = m_pDialog->GetSelectionCount(&selCount);
    if (FAILED(hr))
        return hr;

    if (!selCount)
        return S_OK;

    CLSID * pSelCookies = (CLSID *)MemAlloc(LPTR, sizeof(CLSID)*selCount);
    if (!pSelCookies)
        return E_OUTOFMEMORY;

    hr = m_pDialog->GetSelectedCookies(pSelCookies, &selCount);
    if (FAILED(hr)) {
        MemFree(pSelCookies); pSelCookies = NULL;
        return hr;
    }
    
    for (UINT ui = 0; ui < selCount; ui ++) {
        SkipSingle(pSelCookies + ui);
    }

    MemFree(pSelCookies); pSelCookies = NULL;
    return S_OK;
}

const GUIDSTR_LEN  = GUIDSTR_MAX - 1;

STDMETHODIMP CUpdateController::AddSingle(CLSID * pCookie)
{
    ASSERT(pCookie);
    if (!pCookie)
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    PReportMap  pEntry = FindReportEntry(pCookie);

    if (!pEntry)    {
        NOTIFICATIONITEM    item;
        item.cbSize = sizeof(NOTIFICATIONITEM);
        ASSERT(m_pNotMgr);

        hr = m_pNotMgr->FindNotification(pCookie, &item, 0);
        if (FAILED(hr))
            return hr;

        ASSERT(item.pNotification);
        hr = E_FAIL;
        if ((NOTIFICATIONTYPE_AGENT_START == item.NotificationType) &&
            (item.pNotification) &&
            (TASK_FLAG_HIDDEN & ~item.TaskData.dwTaskFlags))
        {
            SCODE   scodeLast;
            STATUS  statusLast;

            hr = ReadSCODE(item.pNotification, NULL, c_szPropStatusCode, &scodeLast);
            if (FAILED(scodeLast))  {
                statusLast = ITEM_STAT_FAILED;
            } else  {
                statusLast = ITEM_STAT_SUCCEEDED;
            }

            hr = AddEntry(&item, statusLast);
#ifdef  DEBUG
            if (FAILED(hr))  {
                DBGIID("CUpdateController::AddSingle - Failed to add new entry", item.NotificationCookie);
            }
#endif
        }
        SAFERELEASE(item.pNotification);
        item.cbSize = sizeof(NOTIFICATIONITEM);     // BUGBUG: Why is this line here?
        if (FAILED(hr))
            return hr;

        pEntry = FindReportEntry(pCookie);
        ASSERT(pEntry);
    }

    if (pEntry) {
        switch (pEntry->status) {
        case ITEM_STAT_QUEUED:
        case ITEM_STAT_UPDATING:
            hr = S_FALSE;
            break;
#ifdef  DEBUG
        case ITEM_STAT_IDLE:
            ASSERT(0);
            break;
#endif
        default:
            pEntry->status = ITEM_STAT_PENDING;
            if (m_pDialog)
                m_pDialog->RefreshStatus(pCookie,pEntry->name,pEntry->status);
            hr = S_OK;
            break;
        }
    }
    return hr;
}

STDMETHODIMP CUpdateController::Restart(UINT count)
{
    if (!count)
        return S_OK;

    HRESULT hr = S_OK;
    m_cTotal = m_cTotal + count;
    if (m_pDialog)
        PostMessage(m_pDialog->m_hDlg, WM_COMMAND, IDMSG_ADJUSTPROBAR, 0);

    ASSERT(m_pDialer);
    if (m_pDialer->IsOffline())   {
        hr = m_pDialer->DialOut();
    } else if (m_pDialer->IsConnecting()) {
        ;   //  Nothing to do;
    } else  {
        hr = StartPending();
    }

    return hr;
}

STDMETHODIMP CUpdateController::OnRequest(INotification * pNotification)
{
    BOOL    bUpdateAll = TRUE;
    HRESULT hr;
    UINT    count = 0;

    DBG("CUpdateController::OnRequest - entered");
    ASSERT(GetCurrentThreadId() == m_ThreadID);

    //  There is a chance that we are still receiving request even through
    //  we have already ended the session.
    if (m_fSessionEnded)
        return S_FALSE;     //  We don't accept any more requests.
    
    if (pNotification)  {
        VARIANT var;

        VariantInit(&var);

        //  BUGBUG.
        //      Right now (02/21/97) urlmon can't handle the SAFEARRAY.
        //      We assembly this array of GUIDs as a BSTR in SendUpdateRequest
        //      and disassembly it here.
        
        hr = pNotification->Read(c_szPropGuidsArr, &var);
        if (var.vt == VT_BSTR)    {
            UINT    bstrLen = 0;
            BSTR    bstr = var.bstrVal;
            int     guidCount, i;
            CLSID   cookie;

            ASSERT(bstr);
            DBG("CUpdateController::OnRequest - found cookie list");
            
            bstrLen = lstrlenW(bstr);
            guidCount = bstrLen / GUIDSTR_LEN;

            SYSTEMTIME  stNow;
            DATE        dtNow;
            NOTIFICATIONITEM item;

            GetSystemTime(&stNow);
            SystemTimeToVariantTime(&stNow, &dtNow);
            
            item.cbSize = sizeof(NOTIFICATIONITEM);
            for (i = 0; i < guidCount; i ++) {
                BSTR    bstrCookie = NULL;

                bstrCookie = SysAllocStringLen(bstr+i*GUIDSTR_LEN, GUIDSTR_LEN);
                hr = CLSIDFromString(bstrCookie, &cookie);
#ifdef  DEBUG
                DBGIID(TEXT("On request to update "), cookie);
#endif
                SysFreeString(bstrCookie);
                if (FAILED(hr))
                    continue;
    
                if (S_OK == AddSingle(&cookie))
                    count ++;
            }

            hr = Restart(count);
            VariantClear(&var);
            return hr;
        } else  {
            VariantClear(&var);
        }
    }

    DBG("CUpdateController::OnRequest - Update all");

    for ( UINT ui = 0; ui < m_cReportCount; ui ++)  {
        switch (m_aReport[ui].status)   {
#ifdef  DEBUG
        case ITEM_STAT_IDLE:
            ASSERT(0);
            break;
#endif
        case ITEM_STAT_UPDATING:
        case ITEM_STAT_QUEUED:
            break;
        default:
            m_aReport[ui].status = ITEM_STAT_PENDING;
            if (m_pDialog)
                m_pDialog->RefreshStatus(&(m_aReport[ui].startCookie),
                                            m_aReport[ui].name,
                                            m_aReport[ui].status);
            count ++;
            break;
        }
    }

    hr = Restart(count);
    return hr;
}

CUpdateController::CUpdateController() : m_fInit(FALSE), m_fSessionEnded(FALSE)
{
    m_pNotMgr = NULL;
    m_pDialog = NULL;
    m_pDialer = NULL;
    m_cRef = 1;
}

CUpdateController::~CUpdateController()
{
    ASSERT(0L == m_cRef);
}

STDMETHODIMP_(ULONG) CUpdateController::AddRef(void)
{
//    DBG("CUpdateController::AddRef");
    return ++m_cRef;
}

STDMETHODIMP_(ULONG) CUpdateController::Release(void)
{
//    DBG("CUpdateController::Release");
    if( 0L != --m_cRef )
        return m_cRef;

    DBG("Destroying Controller object");
    m_fInit = FALSE;
    PostThreadMessage(m_ThreadID, UM_DECREASE, 0, 0);
    SAFERELEASE(m_pNotMgr);

    if (m_pDialer)  {
        m_pDialer->CleanUp();
        SAFERELEASE(m_pDialer);
    }

    m_pDialog = NULL;
    for (UINT ui = 0; ui < m_cReportCount; ui ++)   {
        SAFELOCALFREE(m_aReport[ui].name);
        SAFELOCALFREE(m_aReport[ui].url);
    }
    SAFELOCALFREE(m_aReport);
    delete this;
    return 0L;
}

STDMETHODIMP CUpdateController::QueryInterface(REFIID riid, void ** ppv)
{
//    DBG("CUpdateController::QueryInterface");
    *ppv = NULL;

    if ((IID_IUnknown == riid) ||
        (IID_INotificationSink == riid))
    {
        *ppv = (INotificationSink *)this;
    }

    if( NULL != *ppv ) 
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP CUpdateController::Init(CUpdateDialog * pDialog)
{
    DBG("CUpdateController::Init");
    ASSERT( !m_pNotMgr );

    ASSERT(pDialog);
    ASSERT(!m_fInit);
    m_pDialog = pDialog;
    m_cReportCount = m_cReportCapacity = 0;

    m_ThreadID = GetCurrentThreadId();
    ASSERT (!m_aReport);

    m_aReport = (PReportMap)MemAlloc(LPTR, sizeof(ReportMapEntry) * CUC_ENTRY_INCRE);
    if (!m_aReport) {
        DBG("CUpdateController::Init - Out of mem");
        return E_OUTOFMEMORY;
    }

    m_cReportCapacity = CUC_ENTRY_INCRE;

    HRESULT hr = CoCreateInstance(CLSID_StdNotificationMgr, NULL, 
        CLSCTX_INPROC_SERVER,  IID_INotificationMgr,(void **)&m_pNotMgr);

    if (SUCCEEDED(hr))  {
        m_pDialer = new CDialHelper;
        if (!m_pDialer) {
            DBG("CUpdateController::Init - Failed to dialer");
            hr = E_OUTOFMEMORY;
        } else    {
            m_pDialer->AddRef();
            hr = m_pDialer->Init(this);
        }
    }

    if (SUCCEEDED(hr))  {
        m_cFinished = m_cTotal = 0;
        m_count = 0;
        m_fInit = TRUE;
        GetItemList(NULL);
        if ((!m_cReportCount) && (m_pDialog))   {   //  Didn't find nothing.
            PostMessage(m_pDialog->m_hDlg, WM_COMMAND, IDMSG_NOTHING, 0);
        }
    } else  {
        SAFERELEASE(m_pNotMgr);
        SAFERELEASE(m_pDialer);
    }

    return hr;
}

//  REVIEW (BUGBUG): Copied from NotificationMgr code (notifctn.hxx)
#define WZ_COOKIE  L"Notification_COOKIE"

//
// INotificationSink member(s)
//
STDMETHODIMP CUpdateController::OnNotification(
    LPNOTIFICATION         pNotification,
    LPNOTIFICATIONREPORT   pNotificationReport,
    DWORD                  dwReserved)
{
    DBG("CUpdateController::OnNotification called");
    ASSERT(pNotification);
    
    HRESULT          hr;
    NOTIFICATIONTYPE notfType;
    ASSERT(GetCurrentThreadId() == m_ThreadID);

    //  Extract Notification Type
    hr = pNotification->GetNotificationInfo(&notfType, NULL, NULL, NULL, 0);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr)) {
        return E_INVALIDARG;
    }

    if (notfType == NOTIFICATIONTYPE_END_REPORT)
    {
        //
        //  Except those we fail to DeliverNotification in the first place for
        //  each request we will get an End Report when we finished the current
        //  update or aborted/skipped the current update. 
        //
        DBG("CUpdateController::OnNotification - END REPORT");
        CLSID   cookie;
        PReportMap  pEntry = NULL;

        if (SUCCEEDED(ReadGUID(pNotification, NULL, c_szStartCookie, &cookie)))
        {
            pEntry = FindReportEntry(&cookie);
        }

        if (!pEntry)    {
            DBGIID("CUpdateController::OnNotification(END_REPORT) - invalid cookie", cookie);
            return E_FAIL;
        }

        //update count of total kbytes downloaded with size of this site from end report
        DWORD dwCrawlKBytes;
        if (SUCCEEDED (ReadDWORD (pNotification, NULL, c_szPropCrawlActualSize, &dwCrawlKBytes)))
        {
            if (m_pDialog) {
                DWORD dwKBytesPrevious = m_pDialog->SetSiteDownloadSize (&cookie, dwCrawlKBytes);

                m_pDialog->m_cDlKBytes += dwCrawlKBytes - dwKBytesPrevious;
                SendMessage (m_pDialog->m_hDlg, WM_TIMER, TID_STATISTICS, 0);   //force update
            }
        }

        switch (pEntry->status) {
        case ITEM_STAT_UPDATING :
        {
            SCODE   eCode = S_OK;

            hr = ReadSCODE(pNotification, NULL, c_szPropStatusCode, & eCode);
            ASSERT(SUCCEEDED(hr));
            if (FAILED(eCode))  {
                pEntry->status = ITEM_STAT_FAILED;
            } else  {
                pEntry->status = ITEM_STAT_SUCCEEDED;
            }

            if (m_pDialog)  {
                m_pDialog->RefreshStatus(&cookie, NULL, pEntry->status);
                PostMessage(m_pDialog->m_hDlg, WM_COMMAND,
                                IDMSG_UPDATEPROGRESS, 100 - pEntry->progress);
            }
            pEntry->progress = 0;
            break;
        }
        case ITEM_STAT_SKIPPED:
        case ITEM_STAT_ABORTED:
            ASSERT(!(pEntry->progress));
            break;
        default:
            ASSERT(0);
            break;
        }
        return S_OK;
    }    
    else if (notfType == NOTIFICATIONTYPE_TASKS_COMPLETED 
            || notfType == NOTIFICATIONTYPE_TASKS_ERROR)
    {
        DBG("CUpdateController::OnNotification - TASKS_ENDED");
        CLSID   cookie;
        PReportMap  pEntry = NULL;

        if (SUCCEEDED(ReadGUID(pNotification, NULL, WZ_COOKIE, &cookie)))
        {
            pEntry = FindReportEntry(&cookie);
        }

        if (!pEntry)    {
            DBGIID("\t(TASKS_ENDED) - invalid cookie ", cookie);
            return E_FAIL;
        } else  {
            DBGIID("\t(TASKS_ENDED) - cookie ", cookie);
        }

        DecreaseCount(&cookie);
        switch (pEntry->status) {
        case ITEM_STAT_UPDATING :
        {
            if (notfType == NOTIFICATIONTYPE_TASKS_ERROR)   {
                pEntry->status = ITEM_STAT_FAILED;
            } else  {
                pEntry->status = ITEM_STAT_SUCCEEDED;
            }

            if (m_pDialog)  {
                m_pDialog->RefreshStatus(&cookie, NULL, pEntry->status);
                PostMessage(m_pDialog->m_hDlg, WM_COMMAND,
                                IDMSG_UPDATEPROGRESS, 100 - pEntry->progress);
            }
            pEntry->progress = 0;
            break;
        }
        case ITEM_STAT_SKIPPED:
        case ITEM_STAT_ABORTED:
            ASSERT(!(pEntry->progress));
            break;
        case ITEM_STAT_QUEUED:
            pEntry->status = ITEM_STAT_ABORTED;
            if (m_pDialog)  {
                m_pDialog->RefreshStatus(&cookie, NULL, pEntry->status);
                PostMessage(m_pDialog->m_hDlg, WM_COMMAND,
                                IDMSG_UPDATEPROGRESS, 100 - pEntry->progress);
            }
            pEntry->progress = 0;
            break;
        default:
            ASSERT(!(pEntry->progress));
            break;
        }
        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_PROGRESS_REPORT)
    {
        DBG("CUpdateController::OnNotification - progress report");
        CLSID   cookie;
        PReportMap  pEntry = NULL;

        if (SUCCEEDED(ReadGUID(pNotification, NULL, c_szStartCookie, &cookie)))
        {
            pEntry = FindReportEntry(&cookie);
        }

        if (!pEntry)    {
            DBGIID("CUpdateController::OnNotification(PROGRESS_REPORT) - invalid cookie", cookie);
            return E_FAIL;
        }

        //start a document dl -- update count and status indicators
        if (m_pDialog)  {
            BSTR bCurrentUrl;
            TCHAR szCurrentUrl[MAX_URL + 1];

            if (SUCCEEDED(ReadBSTR(pNotification, NULL, c_szPropCurrentURL, &bCurrentUrl)))
            {
                //does not appear to be a real BSTR (with length byte) -- just an OLESTR
                MyOleStrToStrN (szCurrentUrl, MAX_URL, bCurrentUrl);
                SAFEFREEBSTR(bCurrentUrl);
                m_pDialog->RefreshStatus(&cookie, NULL, pEntry->status, szCurrentUrl);

                //update size of download
                DWORD dwKBytesCurrent;
                if (SUCCEEDED (ReadDWORD (pNotification, NULL, c_szPropCurrentSize, &dwKBytesCurrent))
                    && (dwKBytesCurrent != -1))
                {
                    DWORD dwKBytesPrevious = m_pDialog->SetSiteDownloadSize (&cookie, dwKBytesCurrent);

                    m_pDialog->m_cDlKBytes += dwKBytesCurrent - dwKBytesPrevious;
                }

                ++m_pDialog->m_cDlDocs;     //increase number of docs downloaded
                SendMessage (m_pDialog->m_hDlg, WM_TIMER, TID_STATISTICS, 0);   //force update
            }
        }

        DWORD dwProgress;
        DWORD dwProgressMax;
        if (SUCCEEDED(ReadDWORD(pNotification, NULL, c_szPropProgressMax, &dwProgressMax)) && SUCCEEDED(ReadDWORD(pNotification, NULL, c_szPropProgress, &dwProgress)))
        {
            //  (INT)dwProgressMax could be -1!
            if ((((INT)dwProgress) >= 0) && (((INT)dwProgressMax) >= 0))   {
                ASSERT(dwProgress <= dwProgressMax);

                //  The progress report is sent at the beginning of current
                //  download. We should substrat Progress by 1.
                UINT cProgress, cProgressMax, newPercentage;
                cProgress = (dwProgress)?dwProgress - 1:0;
                cProgressMax = dwProgressMax;

                newPercentage = MulDiv(cProgress, 100, cProgressMax);
                if ((newPercentage > pEntry->progress) && m_pDialog)   {
                    PostMessage(m_pDialog->m_hDlg, WM_COMMAND,
                                    IDMSG_UPDATEPROGRESS,
                                    (LPARAM)(newPercentage - pEntry->progress));
                }
                pEntry->progress = newPercentage;
            }
        }
        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_BEGIN_REPORT)
    {
        DBG("CUpdateController::OnNotification - begin report");
        CLSID   cookie;
        PReportMap  pEntry = NULL;

        if (SUCCEEDED(ReadGUID(pNotification, NULL, c_szStartCookie, &cookie)))
        {
            pEntry = FindReportEntry(&cookie);
        }

        if (!pEntry)    {
            DBGIID("CUpdateController::OnNotification(BEGIN_REPORT) - invalid cookie", cookie);
            return E_FAIL;
        }

        if (pEntry->status == ITEM_STAT_UPDATING)
            return S_OK;

        //  Note there is a case that we send out the 'Abort' notification to
        //  the agent, and the agent sends 'begin report' at almost the same
        //  time. In that case we can get begin-report when we think we already
        //  cancelled the update.
        if (pEntry->status != ITEM_STAT_QUEUED) {
            ASSERT((pEntry->status == ITEM_STAT_SKIPPED) ||
                   (pEntry->status == ITEM_STAT_ABORTED));
            return S_OK;    //  We bail out.
        }
        pEntry->status = ITEM_STAT_UPDATING;
        if (m_pDialog)
            m_pDialog->RefreshStatus(&cookie, NULL, ITEM_STAT_UPDATING);

        return S_OK;
    }
    else if (notfType == NOTIFICATIONTYPE_TASKS_STARTED)
    {
        DBG("CUpdateController::OnNotification - TASKS_STARTED");
        CLSID   cookie;
        PReportMap  pEntry = NULL;

        if (SUCCEEDED(ReadGUID(pNotification, NULL, WZ_COOKIE, &cookie)))
        {
            pEntry = FindReportEntry(&cookie);
        }

        if (!pEntry)    {
            DBGIID("\t(TASKS_STARTED) - invalid cookie ", cookie);
            return E_FAIL;
        } else  {
            DBGIID("\t(TASKS_STARTED) - cookie ", cookie);
        }
        ASSERT(pEntry->status == ITEM_STAT_QUEUED);

        if (pEntry->status != ITEM_STAT_QUEUED) {
            ASSERT((pEntry->status == ITEM_STAT_SKIPPED) ||
                   (pEntry->status == ITEM_STAT_ABORTED));
            return S_OK;    //  We bail out.
        }
        pEntry->status = ITEM_STAT_UPDATING;
        if (m_pDialog)
            m_pDialog->RefreshStatus(&cookie, NULL, ITEM_STAT_UPDATING);

        return S_OK;
    }
    else
    {
        DBG("CUpdateController::OnNotification - unknown notification");
        return S_OK;
    }
}

STDMETHODIMP CUpdateController::DispatchRequest(PReportMap pEntry)
{
    DBG("CUpdateController::Dispatch - entered");
    ASSERT(pEntry);
    ASSERT(m_pNotMgr);
    ASSERT(m_pDialog);
    ASSERT(ITEM_STAT_PENDING == pEntry->status);
    
    HRESULT hr;
    NOTIFICATIONITEM item;
    item.cbSize = sizeof(item);

    hr = m_pNotMgr->FindNotification(&(pEntry->startCookie), &item, 0);
    ASSERT(SUCCEEDED(hr));
    if (SUCCEEDED(hr))  {
        hr = m_pNotMgr->DeliverNotification(item.pNotification,
                                            item.clsidDest,
                                            DM_DELIVER_DEFAULT_PROCESS |
                                            DM_NEED_COMPLETIONREPORT |
                                            DM_NEED_PROGRESSREPORT |
                                            DM_THROTTLE_MODE,

                                            (INotificationSink *)this,
                                            NULL,
                                            0);
        SAFERELEASE(item.pNotification);
        if (FAILED(hr)) {
            DBG("CUpdateController::Dispatch - failed to DeliverNotification");
            m_pDialog->RefreshStatus(&(pEntry->startCookie), NULL, ITEM_STAT_FAILED);
        } else  {
            DBG("Increase Count");
            pEntry->status = ITEM_STAT_QUEUED;
            pEntry->progress = 0;
            IncreaseCount();
        }
    }
    return hr;
}

//  In CancelRequest() we only attempt to send out the notification of
//  abort. The count on agent side will be decreased when we get end report.
//  So matched number of request and end report is crucial.

STDMETHODIMP CUpdateController::CancelRequest(PReportMap pEntry)
{
    ASSERT(pEntry);

    if ((ITEM_STAT_UPDATING != pEntry->status) 
        && (ITEM_STAT_QUEUED != pEntry->status))
            return S_OK;

    ASSERT(m_pNotMgr);

    HRESULT hr = S_OK;
    INotification *pNot = NULL;
    hr = m_pNotMgr->CreateNotification(NOTIFICATIONTYPE_TASKS_ABORT,
                                       (NOTIFICATIONFLAGS)0,
                                       NULL,
                                       &pNot,
                                       0);
    ASSERT(SUCCEEDED(hr));
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr))
        {
            hr = m_pNotMgr->DeliverReport(pNot,  &(pEntry->startCookie), 0);
            if (SUCCEEDED(hr))  {
                if (m_pDialog)  {
                    PostMessage(m_pDialog->m_hDlg, WM_COMMAND,
                                IDMSG_UPDATEPROGRESS, 100 - pEntry->progress);
                }
                pEntry->progress = 0;
                //  This is the default status afterwards. 
                pEntry->status = ITEM_STAT_ABORTED;
            } else  {
                TraceMsg(TF_THISMODULE, TEXT("CancelRequest:Error:%x"), hr);
            }
        } else  {
            DBG("CUpdateController::Stop failed");
        }
        SAFERELEASE(pNot);
    }

    return hr;
}

PReportMap CUpdateController::FindReportEntry(CLSID * pCookie)
{
    ASSERT(pCookie);

    for (UINT ui = 0; ui < m_cReportCount; ui ++)   {
        if (m_aReport[ui].startCookie == * pCookie) {
            return &(m_aReport[ui]);
        }
    }

    return NULL;
}

STDMETHODIMP CUpdateController::GetLocationOf(CLSID * pCookie, LPTSTR pszText, UINT cchTextMax)
{
    ASSERT(pCookie && pszText);

    PReportMap pEntry = FindReportEntry(pCookie);

    if (pEntry)    {
        lstrcpyn(pszText, pEntry->url, cchTextMax);
    } else  {
        lstrcpyn(pszText, c_szStrEmpty, cchTextMax);
    }

    return S_OK;
}

SUBSCRIPTIONTYPE CUpdateController::GetSubscriptionType(CLSID * pCookie)
{
    ASSERT(pCookie);

    SUBSCRIPTIONTYPE    subType = SUBSTYPE_EXTERNAL;
    PReportMap pEntry = FindReportEntry(pCookie);

    if (pEntry) {
        subType = pEntry->subType;
    }

    return subType;
}

BOOL CUpdateController::IsSkippable(CLSID * pCookie)
{
    ASSERT(pCookie);

    PReportMap pEntry = FindReportEntry(pCookie);

    if (pEntry)
        if (pEntry->status == ITEM_STAT_PENDING ||
            pEntry->status == ITEM_STAT_QUEUED  ||
            pEntry->status == ITEM_STAT_UPDATING)
        {
            return TRUE;
        }
    return FALSE;
}

STDMETHODIMP CUpdateController::AddEntry(NOTIFICATIONITEM *pItem, STATUS status)
{
    ASSERT(pItem);
    ASSERT(m_cReportCount <= m_cReportCapacity);

    if (m_cReportCount == m_cReportCapacity)    {
        UINT    newSize = m_cReportCapacity + CUC_ENTRY_INCRE;
        ASSERT(newSize <= CUC_MAX_ENTRY);
        HLOCAL  newBuf = MemReAlloc(m_aReport, newSize * sizeof(ReportMapEntry), LHND);
        if (!newBuf)    {
            DBG("CUpdateController::AddEntry - Failed to realloc");
            return E_OUTOFMEMORY;
        }

        m_aReport = (PReportMap)(newBuf);
        m_cReportCapacity = newSize;
    }

    m_aReport[m_cReportCount].startCookie = pItem->NotificationCookie;
    m_aReport[m_cReportCount].progress = 0;
    m_aReport[m_cReportCount].status = status;

    OOEBuf  ooeBuf;
    DWORD   dwSize = 0;

    ZeroMemory((void *)&ooeBuf, sizeof(ooeBuf));
    HRESULT hr = LoadOOEntryInfo(&ooeBuf, pItem, &dwSize); 
    if (S_OK != hr)
    {
        if (FAILED(hr))
            return hr;
        else
            return E_FAIL;
    }

    LPTSTR nameStr = NULL, urlStr = NULL;

    nameStr = (LPTSTR)MemAlloc(LPTR, (lstrlen(ooeBuf.m_Name) + 1) * sizeof(TCHAR));
    urlStr = (LPTSTR)MemAlloc(LPTR, (lstrlen(ooeBuf.m_URL) + 1) * sizeof(TCHAR));
    if (!(nameStr && urlStr))   {
        SAFELOCALFREE(nameStr);
        SAFELOCALFREE(urlStr);

        return E_OUTOFMEMORY;
    }

    lstrcpy(nameStr, ooeBuf.m_Name);
    lstrcpy(urlStr, ooeBuf.m_URL);

    m_aReport[m_cReportCount].name = nameStr;
    m_aReport[m_cReportCount].url = urlStr;
    m_aReport[m_cReportCount].subType = GetItemCategory(&ooeBuf);

    m_cReportCount ++;
    return S_OK;
}

STDMETHODIMP CUpdateController::CleanUp()
{
    m_pDialog = NULL;
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
//
//  CUpdateAgent
//
//      The only reason we need this class is that so we can create the
//      dialog in the different thread.

CUpdateAgent::CUpdateAgent()
{
    DBG("Creating CUpdateAgent object");
    ASSERT(!(m_pDialog || m_pController));
}

CUpdateAgent::~CUpdateAgent()
{
    DBG("Destroying CUpdateAgent object");

    if (m_pController)  {
        PostThreadMessage(m_ThreadID, UM_CLEANUP, 0,0);
        m_pController = NULL;
    }

    if (m_pDialog)  {
//        delete m_pDialog;
//              m_pDialog will be destroyed by m_pController in CleanUp.
        m_pDialog = NULL;
    }

    g_pUpdate = NULL;
}

STDMETHODIMP CUpdateAgent::Init(void)
{
    DBG("CUpdateAgent::Init");
    HRESULT hr = S_OK;

    ASSERT(!(m_pDialog || m_pController));

    if (SUCCEEDED(hr))  {
        m_pDialog = new CUpdateDialog;
        if (!m_pDialog) {
            DBG("CUpdateAgent::Init - Failed to create dialog");
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))  {
        m_pController = new CUpdateController;
        if (!m_pController) {
            DBG("CUpdateAgent::Init - Failed to create download controller");
            hr = E_OUTOFMEMORY;
        }
    }

    if (SUCCEEDED(hr))  {
        HANDLE  hThread;

        hThread = CreateThread(NULL, 0, DialogThreadProc, 
                                (LPVOID)m_pDialog, 0, &m_DialogThreadID);
        if (!hThread)   {
            DBG("CUpdateAgent::Init - Failed to create dialog thread");
            hr = E_FAIL;
        } else  {
            int i = 0;
            while ( i < 3)  {
                if (PostThreadMessage(m_DialogThreadID, UM_READY, 0, (LPARAM)m_pController))
                    break;
                i ++;
                Sleep(1000);
            }
            //  BUGBUG BUGBUG, Is there a safer way to do this?
            if (i >= 3) {
                ASSERT(0);
                hr = E_FAIL;
            }
            CloseHandle(hThread);
        }
    }

    if (SUCCEEDED(hr))  {
        HANDLE  hThread;

        hThread = CreateThread(NULL, 0, UpdateThreadProc, 
                                (LPVOID)m_pController, 0, &m_ThreadID);
        if (!hThread)   {
            DBG("CUpdateAgent::Init - Failed to create thread");
            hr = E_FAIL;
        } else  {
            int i = 0;
            while ( i < 3)  {
                if (PostThreadMessage(m_ThreadID, UM_READY, 0, (LPARAM)m_pDialog))
                    break;
                i ++;
                Sleep(1000);
            }
            //  BUGBUG BUGBUG, Is there a safer way to do this?
            if (i >= 3) {
                ASSERT(0);
                hr = E_FAIL;
            }
            SetThreadPriority(hThread, THREAD_PRIORITY_IDLE);
            CloseHandle(hThread);
        }
        if (FAILED(hr)) {
            PostThreadMessage(m_DialogThreadID, WM_QUIT, 0, 0);
        }
    }

    if (FAILED(hr)) {
        if (m_pController)  {
            delete m_pController;
            m_pController = NULL;
        }
        if (m_pDialog)  {
            delete m_pDialog;
            m_pDialog = NULL;
        }
    }
            
    return hr;
}

BOOL ListView_OnNotify(HWND hDlg, NM_LISTVIEW* plvn, CUpdateController * pController)
{
    ASSERT(plvn && pController);
    CUpdateDialog * pDialog = pController->m_pDialog;

    if (!pDialog)
        return FALSE;   //  If m_pDialog is NULL, we haven't call Init
                        //  for pController on second thread yet.

    HRESULT hr;

    switch (plvn->hdr.code)   {
        case LVN_ITEMCHANGED:
        {
            if (!(plvn->uChanged & LVIF_STATE))
                break;

            UINT uOldState = plvn->uOldState & LVIS_SELECTED;
            UINT uNewState = plvn->uNewState & LVIS_SELECTED;

            UINT count = 0;
            hr = pDialog->GetSelectionCount(&count);
            if (FAILED(hr))
                break;

            HWND hButton = GetDlgItem(hDlg, IDCMD_SKIP);
            BOOL fEnable = FALSE;
            if (count)  {

                CLSID cookie;
                int iItem = plvn->iItem;
                
                hr = pDialog->IItem2Cookie(iItem, &cookie);
                if (SUCCEEDED(hr)) {
                    fEnable = pController->IsSkippable(&cookie);
                }
            }

            Button_Enable(hButton, fEnable);
            return TRUE;
        }
        default:
            break;
    }

    return FALSE;
}


void ResizeDialog(HWND hDlg, BOOL bShowDetail)
{
    ASSERT(hDlg);
    RECT    rcDlg, rcChild;
    HWND    hSplitter, hLV;
    TCHAR   szButton[32];

    //calculate margin (upper-left position of IDC_SIZENODETAILS)
    GetWindowRect(GetDlgItem (hDlg, IDC_SIZENODETAILS), &rcChild);
    MapWindowPoints(NULL, hDlg, (LPPOINT)&rcChild, 2);
    int iMargin = rcChild.left;

    GetWindowRect(hDlg, &rcDlg);

    if (bShowDetail)  {
        hLV = GetDlgItem(hDlg, IDL_SUBSCRIPTION);
        ASSERT(hLV);
        GetWindowRect(hLV, &rcChild);
        rcDlg.bottom = rcChild.bottom + iMargin + GetSystemMetrics (SM_CXSIZEFRAME);
        rcDlg.right = rcChild.right + iMargin + GetSystemMetrics (SM_CYSIZEFRAME);

        LONG dwStyle = GetWindowLong (hDlg, GWL_STYLE);
        dwStyle = dwStyle | WS_MAXIMIZEBOX | WS_THICKFRAME;
        SetWindowLong (hDlg, GWL_STYLE, dwStyle);

        rcDlg.left -= (GetSystemMetrics (SM_CXSIZEFRAME) - GetSystemMetrics (SM_CXFIXEDFRAME));
        rcDlg.top -= (GetSystemMetrics (SM_CYSIZEFRAME) - GetSystemMetrics (SM_CYFIXEDFRAME));
        MoveWindow(hDlg, rcDlg.left, rcDlg.top,
                    rcDlg.right - rcDlg.left,
                    rcDlg.bottom - rcDlg.top,
                    TRUE);
        MLLoadString(IDS_NODETAILS, szButton, ARRAYSIZE(szButton));
    } else  {
        hSplitter = GetDlgItem(hDlg, IDC_SIZENODETAILS);
        ASSERT(hSplitter);
        GetWindowRect(hSplitter, &rcChild);

        LONG dwStyle = GetWindowLong (hDlg, GWL_STYLE);
        dwStyle = dwStyle & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME;
        SetWindowLong (hDlg, GWL_STYLE, dwStyle);

        MoveWindow(hDlg, rcDlg.left + (GetSystemMetrics (SM_CXSIZEFRAME) - GetSystemMetrics (SM_CXFIXEDFRAME)),
            rcDlg.top + (GetSystemMetrics (SM_CYSIZEFRAME) - GetSystemMetrics (SM_CYFIXEDFRAME)),
                rcChild.right - rcDlg.left,
                rcChild.bottom - rcDlg.top,
                TRUE);
        MLLoadString(IDS_DETAILS, szButton, ARRAYSIZE(szButton));
    }
    SetDlgItemText(hDlg, IDCMD_DETAILS, szButton);
}


void UpdateStatistics (HWND hDlg, int nFiles, int nKBytes, int nSeconds)
{
    TCHAR szStats[128], szFormat[64];

    MLLoadString (IDS_STATISTICS, szFormat, ARRAYSIZE(szFormat));
    wnsprintf (szStats, ARRAYSIZE(szStats), szFormat, 
               nFiles, nKBytes, nSeconds/60, nSeconds%60);
    SetDlgItemText (hDlg, IDC_STATISTICS, szStats);
}


void DrawResizeWidget (HWND hDlg)   //copied from athena's CGroupListDlg::OnPaint
{
    PAINTSTRUCT ps;
    RECT rc;

    GetClientRect(hDlg, &rc);
    rc.left = rc.right - GetSystemMetrics(SM_CXSMICON);
    rc.top = rc.bottom - GetSystemMetrics(SM_CYSMICON);
    BeginPaint(hDlg, &ps);

    if (CUpdateDialog::m_bDetail && !IsZoomed(hDlg))
        DrawFrameControl(ps.hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);

    CUpdateDialog * pDialog = (CUpdateDialog *)GetWindowLong(hDlg, DWL_USER);
    if (pDialog != NULL)
    {
        pDialog->m_cxWidget = rc.left;
        pDialog->m_cyWidget = rc.top;
    }

    EndPaint(hDlg, &ps);
}


void EraseResizeWidget (HWND hDlg)
{
    CUpdateDialog * pDialog = (CUpdateDialog *)GetWindowLong(hDlg, DWL_USER);
    RECT rWidget;

    if (pDialog != NULL)
    {
        //invalidate resize widget
        rWidget.left = pDialog->m_cxWidget;
        rWidget.top = pDialog->m_cyWidget;
        rWidget.right = pDialog->m_cxWidget + GetSystemMetrics(SM_CXSMICON);
        rWidget.bottom = pDialog->m_cyWidget + GetSystemMetrics(SM_CYSMICON);
        InvalidateRect(hDlg, &rWidget, FALSE);

//        pDialog->m_cxWidget = rWidget.left;
//        pDialog->m_cxWidget = rWidget.top;
    }
}


extern BOOL GetSubscriptionFolderPath(LPTSTR);
//----------------------------------------------------------------------------
// UpdateDlgProc function
//----------------------------------------------------------------------------
BOOL CALLBACK UpdateDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static DWORD dwStartTicks;
    CUpdateDialog * pDialog = (CUpdateDialog *)GetWindowLong(hDlg, DWL_USER);
    CUpdateController * pController = (pDialog)?pDialog->m_pController:NULL;
    
    switch (uMsg)
    {
        case WM_INITDIALOG :
        {
            DBG("DLGBOX: Creating dialog box.");
            ASSERT(lParam);
            ASSERT(GetWindowLong(hDlg, DWL_USER) == 0);
            SetWindowLong(hDlg, DWL_USER, lParam);
            SetForegroundWindow(hDlg);
            TCHAR szString[1024];
            MLLoadString(IDS_CONNECTING, szString, ARRAYSIZE(szString));
            SetDlgItemText(hDlg, IDC_AGENTSTATUS, szString);
            ResizeDialog(hDlg, CUpdateDialog::m_bDetail);

            SetTimer (hDlg, TID_STATISTICS, 1000, NULL);
            dwStartTicks = GetTickCount();

            // Check current keyboard layout is IME or not
            if (((DWORD)GetKeyboardLayout(0) & 0xF0000000L) == 0xE0000000L)
            {
                HWND hwndIME = ImmGetDefaultIMEWnd(hDlg);
                if (hwndIME)
                    SendMessage(hwndIME, WM_IME_NOTIFY, IMN_CLOSESTATUSWINDOW, 0);
            }

            return FALSE;   // keep the focus on the dialog
        }
        case WM_ACTIVATE:
            if (LOWORD(wParam) != WA_INACTIVE)
            {
                // Check current keyboard layout is IME or not
                if (((DWORD)GetKeyboardLayout(0) & 0xF0000000L) == 0xE0000000L)
                {
                    HWND hwndIME = ImmGetDefaultIMEWnd(hDlg);
                    if (hwndIME)
                        SendMessage(hwndIME, WM_IME_NOTIFY, IMN_CLOSESTATUSWINDOW, 0);
                }
            }
            break;
        case WM_NOTIFY :
            switch (LOWORD (wParam))   {
            case IDL_SUBSCRIPTION:
                if (!pController)
                    break;
                return ListView_OnNotify(hDlg, (NM_LISTVIEW *)lParam, pController);
            default:
                return FALSE;
            }
            break;
        case WM_TIMER:
            switch (wParam)
            {
            case TID_UPDATE:
                KillTimer(hDlg, wParam);
                pDialog->CleanUp();
                SetWindowLong(hDlg, DWL_USER, 0);
                delete pDialog;
                return TRUE;
                
            case TID_STATISTICS:
                UpdateStatistics (hDlg, pDialog->m_cDlDocs, pDialog->m_cDlKBytes,
                    (GetTickCount() - dwStartTicks)/1000);
                return TRUE;
            }
            break;
        case WM_GETMINMAXINFO :
            if (CUpdateDialog::m_bDetail)
            {
                LPMINMAXINFO lpmmi = (LPMINMAXINFO)lParam;
                DWORD style = GetWindowLong (hDlg, GWL_STYLE);
                RECT smallest;
                GetWindowRect (GetDlgItem (hDlg, IDC_MINBORDER), &smallest);
                AdjustWindowRect (&smallest, style, FALSE);
                lpmmi->ptMinTrackSize.x = smallest.right - smallest.left;
                lpmmi->ptMinTrackSize.y = smallest.bottom - smallest.top;
                return 0;
            }
            break;
        case WM_PAINT:
            DrawResizeWidget (hDlg);
            break;
        case WM_SIZE:
            if (wParam == SIZE_MAXIMIZED || wParam == SIZE_RESTORED)
            {
                const int CHILDREN[] = { IDCMD_HIDE, IDCMD_ABORT, IDCMD_DETAILS, IDCMD_SKIP };
                int width = LOWORD(lParam), height = HIWORD(lParam);
                RECT rChild;
                int child;

                //calculate margin (upper-left position of IDC_SIZENODETAILS)
                GetWindowRect(GetDlgItem (hDlg, IDC_SIZENODETAILS), &rChild);
                // Use MapWindowPints for mirroring
                MapWindowPoints(NULL, hDlg, (LPPOINT)&rChild, 2);
                int iMargin = rChild.left;

                for (child = 0; child < sizeof(CHILDREN)/sizeof(CHILDREN[0]); child++)
                {
                    GetWindowRect (GetDlgItem (hDlg, CHILDREN[child]), &rChild);
                    MapWindowPoints(NULL, hDlg, (LPPOINT)&rChild, 2);
                    SetWindowPos (GetDlgItem (hDlg, CHILDREN[child]), 0,
                        width - iMargin - (rChild.right - rChild.left), rChild.top,
                        0, 0, SWP_NOSIZE | SWP_NOZORDER);
                }

                if (CUpdateDialog::m_bDetail)   //only apply to bottom half
                {
                    EraseResizeWidget (hDlg);

                    GetWindowRect (GetDlgItem (hDlg, IDD_SPLITTER), &rChild);
                    MapWindowPoints(NULL, hDlg, (LPPOINT)&rChild, 2);                    SetWindowPos (GetDlgItem (hDlg, IDD_SPLITTER), 0,
                        0, 0, width - 2 * rChild.left, rChild.bottom - rChild.top,
                        SWP_NOMOVE | SWP_NOZORDER | SWP_NOCOPYBITS);   //for some reason there's a weird redraw bug on this control -- NOCOPYBITS fixes it
                    
                    GetWindowRect (GetDlgItem (hDlg, IDL_SUBSCRIPTION), &rChild);
                    MapWindowPoints(NULL, hDlg, (LPPOINT)&rChild, 2);                    SetWindowPos (GetDlgItem (hDlg, IDL_SUBSCRIPTION), 0,
                        0, 0, width - rChild.left - iMargin, height - rChild.top - iMargin,
                        SWP_NOMOVE | SWP_NOZORDER);
                }
                return 0;
            }
            break;

        case WM_CLOSE:
            KillTimer (hDlg, TID_STATISTICS);
            break;

        case WM_COMMAND :
        {
            if (!pController)   {
                if (IDCANCEL == LOWORD (wParam))    {
                    KillTimer(hDlg, TID_UPDATE);
                    pDialog->CleanUp();
                    SetWindowLong(hDlg, DWL_USER, 0);
                    delete pDialog;
                    return TRUE;
                } else  {
                    break;
                }
            }

            HWND    hProgress = GetDlgItem(hDlg, IDD_PROBAR);
            HWND    hAnimate = GetDlgItem(hDlg, IDD_ANIMATE);
            TCHAR   szString[1024];

            switch (LOWORD (wParam))
            {
                case IDMSG_SESSIONEND:      //  Stop dialog, session concludes.
                    DBG("UpdateDlgProc - all updates are complete");
                    pController->m_cTotal = pController->m_cFinished = 0;
                    pDialog->m_pController = pController = NULL;
                    delete g_pUpdate;
                    MessageBeep(0xFFFFFFFF);
                    Animate_Close(hAnimate);
                    ShowWindow(hAnimate, SW_HIDE);
                    MLLoadString(IDS_SESSIONEND, szString, ARRAYSIZE(szString));
                    SetDlgItemText(hDlg, IDC_AGENTSTATUS, szString);
                    ShowWindow (GetDlgItem (hDlg, IDC_AGENTSTATUS), SW_SHOW);
                    
                    Button_Enable(GetDlgItem(hDlg, IDCMD_ABORT), FALSE);
                    Button_Enable(GetDlgItem(hDlg, IDCMD_DETAILS), FALSE);
                    SetTimer(hDlg, TID_UPDATE, 3000, NULL);
                    KillTimer (hDlg, TID_STATISTICS);
                    return TRUE;

                case IDMSG_UPDATEBEGIN:
                {
                    DBG("UpdateDlgProc - Start updating");
                    SetForegroundWindow(hDlg);
                    ShowWindow (hAnimate, SW_SHOW);
                    ShowWindow (GetDlgItem (hDlg, IDC_AGENTSTATUS), SW_HIDE);
                    Animate_Open(hAnimate, IDA_DOWNLOAD);
                    Animate_Play(hAnimate, 0, -1, -1);
                    return TRUE;
                }

                case IDMSG_ADJUSTPROBAR:
                {
                    ASSERT(pController->m_cTotal);
                    ASSERT(pController->m_cFinished <= pController->m_cTotal);
                    SendMessage(hProgress, PBM_SETRANGE32,
                            0, pController->m_cTotal * 100);
                    SendMessage(hProgress, PBM_SETPOS,
                            pController->m_cFinished * 100, 0);
                    return TRUE;
                }

                case IDMSG_NOTHING:   //  Nothing to show yet.
                {
                    DBG("UpdateDlgProc - No item found");
                    MLLoadString(IDS_STRING_NOTHING_TO_UPDATE, 
                                    szString , ARRAYSIZE(szString));
                    SetDlgItemText(hDlg, IDC_AGENTSTATUS, szString);
                    pController->m_cTotal = pController->m_cFinished = 0;
                    pDialog->m_pController = pController = NULL;
                    delete g_pUpdate;
                    MessageBeep(0xFFFFFFFF);
                    Button_Enable(GetDlgItem(hDlg, IDCMD_ABORT), FALSE);
                    Button_Enable(GetDlgItem(hDlg, IDCMD_DETAILS), FALSE);
                    SetTimer(hDlg, TID_UPDATE, 3000, NULL);
                    KillTimer (hDlg, TID_STATISTICS);
                    return TRUE;
                }

                case IDMSG_UPDATEPROGRESS:
                    SendMessage(hProgress, PBM_DELTAPOS, lParam, 0);
                    return TRUE;

                case IDMSG_INITFAILED:
                    DBG("UpdateDlgProc - Controller Init Failed.");
                    SetWindowLong(hDlg, DWL_USER, 0);
                    pDialog->m_pController = pController = NULL;
                    delete g_pUpdate;
                    MessageBeep(0xFFFFFFFF);
                    Button_Enable(GetDlgItem(hDlg, IDCMD_ABORT), FALSE);
                    Button_Enable(GetDlgItem(hDlg, IDCMD_DETAILS), FALSE);
                    pDialog->CleanUp();
                    SetWindowLong(hDlg, DWL_USER, 0);
                    delete pDialog;
                    return TRUE;

                case IDCANCEL:
                {
                    int mbRet = SGMessageBox(hDlg, IDS_DOWNLOAD_ABORT_WARNING,
                                                MB_YESNO | MB_ICONQUESTION |
                                                MB_DEFBUTTON2 | MB_APPLMODAL);
                    if (mbRet == IDNO)  {
                        return TRUE;
                    } else  {
                        if (!pDialog->m_pController)
                            return TRUE;
                        ;   //  Fall through.
                    }
                }

                //  Following messages are from the UI and need to be
                //  forward to update thread.

                case IDCMD_ABORT:   //  Abort all updates
                    DBG("UpdateDlgProc - closing, all downloads aborted");
                    ASSERT(pController->m_ThreadID);
                    PostThreadMessage(pController->m_ThreadID, UM_ONABORT, 0,0);
                    MLLoadString(IDS_ABORTING, szString, ARRAYSIZE(szString));
                    SetDlgItemText(hDlg, IDC_AGENTSTATUS, szString);
                    Animate_Close(hAnimate);
                    ShowWindow(hAnimate, SW_HIDE);
                    ShowWindow (GetDlgItem (hDlg, IDC_AGENTSTATUS), SW_SHOW);
                    return TRUE;

                case IDCMD_SKIP:
                    PostThreadMessage(pController->m_ThreadID, UM_ONSKIP, 0,0);
                    return TRUE;

                case IDCMD_DETAILS:
                {
                    CUpdateDialog::m_bDetail = !CUpdateDialog::m_bDetail;
                    ResizeDialog(hDlg, CUpdateDialog::m_bDetail);
                    
                    return TRUE;
                }

                case IDCMD_HIDE:
                    ShowWindow(hDlg, SW_SHOWMINIMIZED);
                    return TRUE;

                default:
                    break;
            }
            break;
        }
    }
    
    return FALSE;
}

HRESULT GetActiveUpdateAgent(CUpdateAgent ** ppUpdate)
{
    ASSERT (ppUpdate);
    //  We are assuming the Update agent is free threaded.
    *ppUpdate = NULL;

    if (g_pUpdate == NULL)  {
        DBG("GetActiveUpdateAgent - Creating new agent");
        g_pUpdate = new CUpdateAgent();
        if (!g_pUpdate) {
            DBG("GetActiveUpdateAgent - Failed to create new agent");
            return E_OUTOFMEMORY;
        }

        HRESULT hr = g_pUpdate->Init();
        if (FAILED(hr)) {
            DBG("GetActiveUpdateAgent - Failed to init new agent");
            return hr;
        }
    }
    *ppUpdate = g_pUpdate;
    return NOERROR;
}

DWORD WINAPI BackgroundUpdate(void)
{
    DBG("BackgroundUpdate entered");

    HRESULT hr;
    CUpdateAgent * pAgent = NULL;

    hr = GetActiveUpdateAgent(&pAgent);
    if (SUCCEEDED(hr))  {
        ASSERT(pAgent);
        ASSERT(pAgent->m_ThreadID);
        //  BUGBUG Even when we succeed here, there are chances that we won't
        //  get updated because CONTROLLER CAN FAIL TO INITIALIZE AND WE DON'T
        //  KNOW IT!
        if (!PostThreadMessage(pAgent->m_ThreadID, UM_BACKGROUND,0,0))
        {
            hr = E_FAIL;
            DBG("Failed to post ONREQUEST message.");
        }
        pAgent = NULL;
    }
    DBG("BackgroundUpdate ended");
    return (DWORD)hr;
}

DWORD WINAPI UpdateRequest(UINT idCmd, INotification *pNot)
{
//    DBG("UpdateRequest entered");

    if (idCmd != UM_ONREQUEST)
        return E_FAIL;

    HRESULT hr;
    CUpdateAgent * pAgent = NULL;

    hr = GetActiveUpdateAgent(&pAgent);
    if (SUCCEEDED(hr))  {
        ASSERT(pAgent);
        ASSERT(pAgent->m_ThreadID);
        if (pNot)
            pNot->AddRef();
        
        //  BUGBUG Even when we succeed here, there are chances that we won't
        //  get updated because CONTROLLER CAN FAIL TO INITIALIZE AND WE DON'T
        //  KNOW IT!
        if (!PostThreadMessage(pAgent->m_ThreadID, UM_ONREQUEST,0,(LPARAM)pNot))
        {
            hr = E_FAIL;
            SAFERELEASE(pNot);
            DBG("Failed to post ONREQUEST message.");
        }
        pAgent = NULL;
    }
//    DBG("UpdateRequest ended");
    return (DWORD)hr;
}

HRESULT UpdateNotifyReboot(void)
{
    DBG("UpdateNotifyReboot entered");

    HRESULT hr;
    CUpdateAgent * pAgent = NULL;

    hr = GetActiveUpdateAgent(&pAgent);
    if (SUCCEEDED(hr))  {
        ASSERT(pAgent);
        ASSERT(pAgent->m_ThreadID);

        hr = PostThreadMessage(pAgent->m_ThreadID, UM_NEEDREBOOT, 0, 0);
    }

    DBG("UpdateNotifyReboot ended");
    return hr;
}
