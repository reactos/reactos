//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       package.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-21-1996   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#include <notiftn.h>

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::CPackage
//
//  Synopsis:
//
//  Arguments:  [pClsidSender] --
//              [pNotfctnSink] --
//              [pNotification] --
//              [deliverMode] --
//              [pGroupCookie] --
//              [pschdata] --
//              [pTaskData] --
//              [pClsidDest] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CPackage::CPackage(
             LPCLSID             pClsidSender,      // class of sender
             LPNOTIFICATIONSINK  pNotfctnSink,      // can be null - see mode
             LPNOTIFICATION      pNotification,
             DELIVERMODE         deliverMode,
             PNOTIFICATIONCOOKIE pGroupCookie,
             PTASK_TRIGGER       pschdata,
             PTASK_DATA          pTaskData,
             LPCLSID             pClsidDest
             )
            :   CThreadId(NULL)
            ,_CNotfctnReportDest(NULL, RO_DEST)
            ,_CNotfctnReportSender(NULL,RO_SENDER)
            ,_CRefsReportDest(0)
            ,_CRefsReportSender(0)
{
    _CNotfctnReportDest.SetCPackage(this);
    _CNotfctnReportSender.SetCPackage(this);

    _PackageType   = PT_NORMAL;
    _PackageFlags  = PF_READY;

    _nextRunInterval = 0;

    // reset the destination thread id - the current one is not correct
    _CNotfctnReportDest.SetThreadId(NULL);

    _PackageContent = PC_EMPTY;

    _pNotification = pNotification;
    NotfAssert((_pNotification));

    _pNotification->AddRef();

    if (pschdata)
    {
        CPkgCookie ccookieNot1;
        _NotificationCookie = ccookieNot1;

        _pNotification->GetNotificationInfo(
                                         &_NotificationType
                                        ,0 
                                        ,0
                                        ,0
                                        ,0
                                        );

        ((CNotfctnObj *) _pNotification)->SetNotificationCookie(_NotificationCookie);
    }
    else
    {
            _pNotification->GetNotificationInfo(
                                         &_NotificationType
                                        ,&_NotificationCookie
                                        ,0
                                        ,0
                                        ,0
                                        );

    }

    _CNotfctnReportSender.SetNotificationSink(pNotfctnSink);
    _CNotfctnReportSender.SetClsId(pClsidSender);
    if (pClsidSender)
    {
        _clsidSender = *pClsidSender;
        _PackageContent |= PC_CLSIDSENDER;
    }
    _CNotfctnReportDest.SetClsId(pClsidDest);

    _deliverMode = deliverMode;

    if (pTaskData)
    {
        _TaskData = *pTaskData;
        _PackageContent |= PC_TASKDATA;
    }
    else
    {
        _TaskData.dwPriority = THREAD_PRIORITY_NORMAL;
    }

    if (pschdata)
    {
        _schdata = *pschdata;
        _PackageContent |= PC_TASKTRIGGER;
    }

    if (pClsidDest)
    {
        _clsidDest   = *pClsidDest;
        _PackageContent |= PC_CLSIDDEST;
    }

    _dwReserved    = 0;
    _dateNextRun   = 0;
    _datePrevRun   = 0;

    // Report stuff
    _pNotfctnReport = 0;

    // group stuff
    if (pGroupCookie)
    {
        _GroupCookie = *pGroupCookie;
        _PackageContent |= PC_GROUPCOOKIE;
    }
    else
    {
        _GroupCookie = CLSID_NULL;
    }
    _BaseCookie = _NotificationCookie;
    _PackageContent |= PC_BASECOOKIE;

    CPkgCookie ccookieNot;
    _RunningCookie = ccookieNot;
    _PackageContent |= PC_RUNCOOKIE;
    _index = 0xffffffff;
    _hrDest = NOERROR;
}

// used to create report packages
//+---------------------------------------------------------------------------
//
//  Method:     CPackage::CPackage
//
//  Synopsis:
//
//  Arguments:  [pCNotfRprtTo] --
//              [pNotification] --
//              [deliverMode] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CPackage::CPackage(
             CNotificationReport *pCNotfRprtTo,
             LPNOTIFICATION       pNotification,
             DELIVERMODE          deliverMode
             )
            :   CThreadId(NULL)
            ,_CNotfctnReportDest(*pCNotfRprtTo)
            ,_CNotfctnReportSender(NULL,RO_SENDER)
            ,_CRefsReportDest(0)
            ,_CRefsReportSender(0)

{
    _CNotfctnReportDest.SetCPackage(this);
    _CNotfctnReportSender.SetCPackage(this);
    _PackageContent = PC_EMPTY;

    _pNotification = pNotification;
    NotfAssert((_pNotification));
    _TaskData.dwPriority = THREAD_PRIORITY_NORMAL;

    _pNotification->AddRef();
    _nextRunInterval = 0;


    _pNotification->GetNotificationInfo(
                                         &_NotificationType
                                        ,&_NotificationCookie
                                        ,0
                                        ,0
                                        ,0
                                        );

    _deliverMode = deliverMode;

    if ( pCNotfRprtTo->GetClsId() )
    {
        _clsidDest   = * pCNotfRprtTo->GetClsId() ;
        _PackageContent |= PC_CLSIDDEST;
    }
    // get the run cookie
    {
        CPackage *pCPkg = pCNotfRprtTo->GetCPackage();
        if (pCPkg->GetRunningCookie())
        {
            _RunningCookie = *pCPkg->GetRunningCookie();
            _PackageContent |= PC_RUNCOOKIE;
        }
        if (pCPkg->GetBaseCookie())
        {
            _BaseCookie = *pCPkg->GetBaseCookie();
            _PackageContent |= PC_BASECOOKIE;
        }

        THREADID threadId = pCNotfRprtTo->GetThreadId();
        NotfAssert((threadId));
        SetThreadId(threadId);
    }

    //_NotificationCookie = 0;
    _dwReserved    = 0;
    _dateNextRun   = 0;
    _datePrevRun   = 0;

    // Report stuff
    _pNotfctnReport = 0;

    //_TaskData = 0;
    _PackageType   = (pCNotfRprtTo->GetReportObjType() == RO_SENDER) ? PT_REPORT_TO_SENDER : PT_REPORT_TO_DEST;

    _PackageFlags  = PF_READY;
    _index = 0xffffffff;
    _pwzNotfCookie = 0;
    _pwzNotfType = 0;
    _hrDest = NOERROR;
}

// create an empty package - LoadFromPerist will be called
//+---------------------------------------------------------------------------
//
//  Method:     CPackage::CPackage
//
//  Synopsis:
//
//  Arguments:  [dwWhere] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CPackage::CPackage(
             DWORD dwWhere
             )
            :   CThreadId(NULL)
            ,_CNotfctnReportDest(NULL,RO_DEST)
            ,_CNotfctnReportSender(NULL,RO_SENDER)
            ,_CRefsReportDest(0)
            ,_CRefsReportSender(0)

{
    NotfAssert((dwWhere == CPACKAGE_MAGIC));
    _CNotfctnReportDest.SetCPackage(this);
    _CNotfctnReportSender.SetCPackage(this);
    _PackageContent = PC_EMPTY;
    _pNotification = 0;
    _NotificationType = CLSID_NULL;
    _NotificationCookie = CLSID_NULL;
    _deliverMode = (DELIVERMODE)0;
    _clsidDest = CLSID_NULL;
    _RunningCookie = CLSID_NULL;
    _TaskData.dwPriority = THREAD_PRIORITY_NORMAL;
    _dwReserved    = 0;
    _dateNextRun   = 0;
    _datePrevRun   = 0;
    _nextRunInterval = 0;
    _pNotfctnReport = 0;
    _PackageType   = PT_NOT_INITIALIZED;
    _PackageFlags  = (PACKAGE_FLAGS)0;
    _index = 0xffffffff;
    _pwzNotfCookie = 0;
    _pwzNotfType = 0;
    _hrDest = NOERROR;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::~CPackage
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
CPackage::~CPackage()
{
    UNTRACK_ALLOC(this);
    
    // if loaded cross process remove the key
    if (_dwParam)
    {
        NotfAssert(( GetNotificationState() & PF_CROSSPROCESS ));
        CHAR szPackageSubKey[SZREGVALUE_MAX] = {0};
        wsprintf(szPackageSubKey,"%lx",_dwParam);
        RemovePersist(c_pszRegKeyPackage,szPackageSubKey);
    }

    if (_pNotification)
    {
        _pNotification->Release();
    }
    _CNotfctnReportDest.~CNotificationReport();
    _CNotfctnReportSender.~CNotificationReport();

    delete [] _pwzNotfCookie;
    delete [] _pwzNotfType;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPackage::OnPacket
//
//  Synopsis:   called when a package got delivererd to this theread via
//              Deliver
//              uses the correct deliver agent which will handle the package
//
//  Arguments:  [msg] --
//              [wParam] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
LRESULT CPackage::OnPacket(UINT msg, DWORD dwParam)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::OnPacket\n", this));
    NotfDebugOut((DEB_MGR, "%p _IN OnPacket Notification:[%ws]\n", this, NotfCookieStr()));
    
    HRESULT hr = E_FAIL;
    NotfAssert((this));

    CDeliverAgent *pDelAgent = GetDeliverAgentThread();

    NotfAssert((pDelAgent));

    // not all singel-destinated packages have a threadid
    //NotfAssert((   (!GetThreadId() && IsBroadcast())
    //             || (GetThreadId() && !IsBroadcast()) ));

    switch (msg)
    {
    default:
        NotfAssert((FALSE && "Received package with invalid delivery mode"));
    break;
    case WM_THREADPACKET_POST :
    case WM_THREADPACKET_SEND :
    {
        NotfAssert((GetNotification()));

        if (pDelAgent)
        {
            hr = pDelAgent->HandlePackage(this);
        }
    }
    break;
    case WM_PROCESSPACKET_SEND :
    case WM_PROCESSPACKET_POST :
    {
        CPackage *pCPackageRunning = 0;

        do
        {
            CDeliverAgent *pDelAgent = 0;

            if (msg == WM_PROCESSPACKET_SEND)
            {
                pDelAgent = GetDeliverAgentAsync();
            }
            else
            {
                pDelAgent = GetDeliverAgentThread();
            }

            if (!pDelAgent)
            {
                break;
            }

            //CDestinationPort &rCDest = GetCDestPort();
            NotfAssert(( GetCDestPort().GetPort()  != 0 ));
            NotfAssert(( GetCDestPort().GetPort()  != GetThreadNotificationWnd() ));

            SetThreadId(GetCurrentThreadId());
            // handle reports here
            if (IsReportRequired())
            {
                CPackage    * pkgRunning = NULL;
                PNOTIFICATIONCOOKIE pBaseCookie = GetBaseCookie();
                NotfAssert((pBaseCookie));
                if (!pBaseCookie)
                {
                    break;
                }
                CSchedListAgent *pCSchAgent = GetScheduleListAgent();
                if (!pCSchAgent)
                {
                    hr = E_FAIL;
                    break;
                }
                hr = pCSchAgent->FindPackage(
                                             pBaseCookie
                                             ,&pkgRunning
                                             ,LM_LOCALCOPY
                                             );
                if (FAILED(hr)) {
                    //  Add it to our local list.
                    //  This happens when we create a notification and delivered
                    //  it XProcess.
                    CListAgent *pListAgent1 = pCSchAgent->GetListAgent(0);
                    // set the date and add to list
                    SetNextRunDate(CFileTime(MAX_FILETIME));
                    NotfAssert( GetNotificationState() & PF_RUNNING);
                    hr = pListAgent1->HandlePackage(this);
                } else  {
                    if (pkgRunning) {
                        pkgRunning->Release();
                        pkgRunning = NULL;
                    }
                }

                _CNotfctnReportDest.SetCPackage(this);
                _pNotfctnReport = &_CNotfctnReportDest;

            }

            // deliver the package to our deliver agent
            hr = pDelAgent->HandlePackage(this);

            break;
        }
        while (TRUE);

        if (pCPackageRunning)
        {
            RELEASE(pCPackageRunning);
        }

    }
    break;

    } // end switch


    NotfDebugOut((DEB_MGR, "%p Out OnPacket Notification:[%ws], hr:%lx\n", this, NotfCookieStr(), hr));
    
    // matching release of addref in deliver - might be last release
    RELEASE(this);

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::OnPacket (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::Deliver
//
//  Synopsis:   delivers a notification by posting or sending a window msg
//
//
//  Arguments:  [hwnd] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-06-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::Deliver(HWND hwnd, DWORD dwMode)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::Deliver\n", this));
    NotfDebugOut((DEB_MGR, "%p _IN Deliver Notification:[%ws]\n", this, NotfCookieStr()));
    
    HRESULT hr = NOERROR;
    NotfAssert((this));
    NotfAssert(( GetNotification() ));

    //
    // there is no threadID if the notification gets broadcasted
    //
    // add this time it is not know where the package will go
    // there might be not threadId
    //NotfAssert((   (!GetThreadId() && IsBroadcast())
    //             || (GetThreadId() && !IsBroadcast()) ));
    if ( IsWindow(hwnd)  )
    {
        DWORD dwProcessId = 0;
        DWORD dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
    
        if (GetCurrentProcessId() == dwProcessId)
        {
            dwMode = 0;
        }
    
        //if (dwMode && && (hwnd != GetThreadNotificationWnd()))
        if (dwMode)
        {
            NotfAssert(( GetCDestPort().GetPort()  != 0 ));
            NotfAssert(( GetCDestPort().GetPort()  == GetThreadNotificationWnd() ));
            
            hr = PreDeliver();

            NotfDebugOut((DEB_MGR, "%p _In CGlobalNotfMgr:%lx === SendNotifyMessage (hwnd:%lx) WM_PROCESSPACKET_SEND\n", this,dwMode, hwnd));
            BOOL fSent = SendNotifyMessage(hwnd, WM_PROCESSPACKET_SEND, 0, (LPARAM)dwMode);
            NotfDebugOut((DEB_MGR, "%p Out CGlobalNotfMgr:%p === SendNotifyMessage (hwnd:%lx, fSend:%lx) WM_PROCESSPACKET_SEND\n", this,this, hwnd, fSent));

            if (fSent == FALSE)
            {
                hr = NOTF_E_NOTIFICATION_NOT_DELIVERED;
            }
            else
            {
            }

            NotfAssert((fSent));
        }
        else
        {
            // addref - release is called in OnPacket
            ADDREF(this);

            if (IsSynchronous())
            {
                NotfAssert((GetCurrentProcessId() == dwProcessId));

                NotfDebugOut((DEB_MGR, "%p CPackage:%p === SendNotifyMessage (hwnd:%lx) WM_THREADPACKET_SEND\n", this,this, hwnd));
                BOOL fSent = SendNotifyMessage(hwnd, WM_THREADPACKET_POST, 0, (LPARAM)this);
                NotfDebugOut((DEB_MGR, "%p Out CGlobalNotfMgr:%p === SendNotifyMessage (hwnd:%lx, fSend:%lx) WM_THREADPACKET_POST\n", this,this, hwnd, fSent));
                if (fSent == FALSE)
                {
                    hr = NOTF_E_NOTIFICATION_NOT_DELIVERED;
                    RELEASE(this);
                }

                NotfAssert((fSent));
            }
            else
            {
                NotfDebugOut((DEB_MGR, "%p CPackage:%p === PostMessage (hwnd:%lx) WM_THREADPACKET_POST\n", this,this, hwnd));
                BOOL fPosted = PostMessage(hwnd, WM_THREADPACKET_POST, 0, (LPARAM)this);
                if (fPosted == FALSE)
                {
                    hr = NOTF_E_NOTIFICATION_NOT_DELIVERED;
                    RELEASE(this);
                }

                NotfAssert((fPosted));
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }

    NotfDebugOut((DEB_MGR, "%p Out Deliver Notification:[%ws], hr:%lx\n", this, NotfCookieStr(), hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::Deliver (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::GetClassID
//
//  Synopsis:
//
//  Arguments:  [pClassID] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CPackage::GetClassID (CLSID *pClassID)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::\n", this));
    HRESULT hr = NOERROR;

    *pClassID = CLSID_StdNotificationMgr;

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage:: (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::IsDirty
//
//  Synopsis:
//
//  Arguments:  [void] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CPackage::IsDirty(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::\n", this));
    HRESULT hr = NOERROR;

    IPersistStream *pPrstStm = 0;
    NotfAssert((_pNotification));
    hr = _pNotification->QueryInterface(IID_IPersistStream,(void **)&pPrstStm);
    if (hr == NOERROR)
    {
        hr = pPrstStm->IsDirty();
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage:: (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::Load
//
//  Synopsis:
//
//  Arguments:  [pStm] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CPackage::Load(IStream *pStm)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::Load\n", this));
    TNotfDebugOut((DEB_TPERSIST, "%p _IN CPackage::Load\n", this));

    HRESULT hr = NOERROR;
    NotfAssert((pStm));

    NotfAssert((!_pNotification));
    NOTIFICATIONITEM       notfItem;
    NOTIFICATIONITEMEXTRA  notfItemExtra;
    CLSID clsid = CLSID_NULL;

    do
    {
        notfItem.cbSize = sizeof(NOTIFICATIONITEM);
        // read the notification item
        hr = ReadFromStream(pStm, &notfItem, sizeof(NOTIFICATIONITEM));

        BREAK_ONERROR(hr);
        NotfAssert((notfItem.pNotification == NULL));

        hr = SetNotificationItem(&notfItem);
        BREAK_ONERROR(hr);

        // read the  some additional info
        hr = ReadFromStream(pStm, &notfItemExtra, sizeof(NOTIFICATIONITEMEXTRA));
        BREAK_ONERROR(hr);

        hr = SetNotificationItemExtra(&notfItemExtra);
        BREAK_ONERROR(hr);

        // read the classid
        hr = ReadFromStream(pStm, &clsid, sizeof(CLSID));
        BREAK_ONERROR(hr);

        if (clsid == CLSID_StdNotificationMgr)
        {
            CNotfctnObj *pCNotfObj = 0;
            // of our own guy
            hr = CNotfctnObj::Create(&notfItem.NotificationType, &pCNotfObj);

            if (hr == NOERROR)
            {
                NotfAssert((pCNotfObj));
                IPersistStream *pPrstStm = 0;

                hr = pCNotfObj->QueryInterface(IID_IPersistStream,(void **)&pPrstStm);

                // ask the item to persist itself
                if (hr == NOERROR)
                {
                    hr = pPrstStm->Load(pStm);
                    pPrstStm->Release();
                }

                // the object is already addref'd
                _pNotification = pCNotfObj;
                pCNotfObj->SetCPackage(this);
            }
        }
        else if (clsid != CLSID_NULL)
        {
            INotification *pNotf = 0;

            hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_INotification, (void**)&pNotf);
            if (hr == NOERROR)
            {
                IPersistStream *pPrstStm = 0;

                hr = _pNotification->QueryInterface(IID_IPersistStream,(void **)&pPrstStm);

                // ask the item to persist itself
                if (hr == NOERROR)
                {
                    hr = pPrstStm->Load(pStm);
                    pPrstStm->Release();
                }

                pNotf->Release();
            }
        }

        break;
    } while (TRUE);

    if (hr == NOERROR)
    {
        NotfAssert((_pNotification));
    }

    TNotfDebugOut((DEB_TPERSIST, "%p OUT CPackage::Load (hr:%lx)\n",this, hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::Load (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::Save
//
//  Synopsis:
//
//  Arguments:  [BOOL] --
//              [fClearDirty] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CPackage::Save(IStream *pStm,BOOL fClearDirty)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::Save\n", this));
    TNotfDebugOut((DEB_TPERSIST, "%p IN CPackage::Save\n", this));

    NotfAssert((pStm));
    HRESULT hr = NOERROR;

    IPersistStream *pPrstStm = 0;
    NotfAssert((_pNotification));
    NOTIFICATIONITEM       notfItem;
    NOTIFICATIONITEMEXTRA  notfItemExtra;

    do
    {
        memset(&notfItem, 0, sizeof(NOTIFICATIONITEM));
        notfItem.cbSize = sizeof(NOTIFICATIONITEM);
        // get and write the notification item
        hr = GetNotificationItem(&notfItem, EF_NOT_NOTIFICATION);
        NotfAssert((notfItem.pNotification == NULL));
        BREAK_ONERROR(hr);
        hr = WriteToStream(pStm, &notfItem, sizeof(NOTIFICATIONITEM));
        BREAK_ONERROR(hr);

        // get and write some additional info
        hr = GetNotificationItemExtra(&notfItemExtra);
        BREAK_ONERROR(hr);
        hr =  WriteToStream(pStm, &notfItemExtra, sizeof(NOTIFICATIONITEMEXTRA));
        BREAK_ONERROR(hr);

        CLSID clsid = CLSID_NULL;

        // ask the item to persist itself
        hr = _pNotification->QueryInterface(IID_IPersistStream,(void **)&pPrstStm);
        if (hr == NOERROR)
        {
            hr = pPrstStm->GetClassID(&clsid);
        }

        {
            IUnknown *pUnk = 0;
            HRESULT hr1 = _pNotification->QueryInterface(IID_INotificationRunning,(void **)&pUnk);
            if (hr1 == NOERROR)
            {
                NotfAssert((pUnk));
                ((CNotfctnObj *)_pNotification)->SetCPackage(this);
                pUnk->Release();
            }
        }

        // write the class of the item
        hr =  WriteToStream(pStm, &clsid, sizeof(CLSID));
        BREAK_ONERROR(hr);

        // save the object if saveable
        if (pPrstStm)
        {
            hr = pPrstStm->Save(pStm, fClearDirty);
            pPrstStm->Release();
        }

        break;
    } while (TRUE);

    TNotfDebugOut((DEB_TPERSIST, "%p OUT CPackage::Save (hr:%lx)\n",this, hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::Save (hr:%lx)\n",this, hr));
    return hr;
}

HRESULT CPackage::SyncTo(CPackage *pUpdatedPkg)
{
    HRESULT hr = E_INVALIDARG;

    if (pUpdatedPkg)
    {
        CNotfctnObj *pThisNotfObj = (CNotfctnObj *)_pNotification;
        CNotfctnObj *pUpdatedNotfObj = (CNotfctnObj *)pUpdatedPkg->_pNotification;

        if (pThisNotfObj && pUpdatedNotfObj)
        {
            hr = pThisNotfObj->SyncTo(pUpdatedNotfObj);
        }

        if (pUpdatedPkg->GetTaskTrigger())
        {
            _schdata = pUpdatedPkg->_schdata;
            _PackageContent |= PC_TASKTRIGGER;
        }
        if (pUpdatedPkg->GetTaskData())
        {
            _TaskData = pUpdatedPkg->_TaskData;
            _PackageContent |= PC_TASKDATA;
        }
    }

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::GetSizeMax
//
//  Synopsis:
//
//  Arguments:  [pcbSize] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP CPackage::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::GetSizeMax\n", this));
    HRESULT hr = NOERROR;

    IPersistStream *pPrstStm = 0;
    NotfAssert((_pNotification));
    hr = _pNotification->QueryInterface(IID_IPersistStream,(void **)&pPrstStm);
    if (hr == NOERROR)
    {
        hr = pPrstStm->GetSizeMax(pcbSize);
    }

    pcbSize->LowPart += sizeof(NOTIFICATIONITEM);
    pcbSize->HighPart = 0;

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::GetSizeMax (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::CreateDeliver
//
//  Synopsis:
//
//  Arguments:  [pClsidSender] --
//              [pNotfctnSink] --
//              [pNotification] --
//              [deliverMode] --
//              [pGroupCookie] --
//              [pTaskTrigger] --
//              [rNotificationDest] --
//              [ppCPackage] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::CreateDeliver(
                      LPCLSID             pClsidSender,
                      LPNOTIFICATIONSINK  pNotfctnSink,
                      LPNOTIFICATION      pNotification,
                      DELIVERMODE         deliverMode,
                      PNOTIFICATIONCOOKIE pGroupCookie,
                      PTASK_TRIGGER       pTaskTrigger,
                      PTASK_DATA          pTaskData,
                      REFDESTID           rNotificationDest,
                      CPackage          **ppCPackage,
                      PACKAGE_FLAGS       packageflags
                      )
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::CreateDeliver\n", NULL));
    HRESULT hr = E_INVALIDARG;

    NotfAssert((ppCPackage));
    CPackage *pCPackage = 0;
    CNotificationMgr  *pCNotMgr = 0;    //needed to register and unregister
    CLSID clsid = rNotificationDest;

    do
    {
        if (!ppCPackage)
        {
            break;
        }

        if (!pNotification)
        {
            break;
        }

        if (deliverMode & (DM_NEED_COMPLETIONREPORT | DM_NEED_PROGRESSREPORT | DM_THROTTLE_MODE))
        {
            // need sink or class id for completion reprot

            if (!pClsidSender && !pNotfctnSink)
            {
                // can not send back a completion report notification
                // if not classid and not sink
                break;
            }
            if (!pClsidSender)
            {
                // can not be persisted and reastablished
            }
            if (pNotfctnSink)
            {
                CDeliverAgentStart  *pDelAgentStart =  GetDeliverAgentStart();
                pDelAgentStart->VerifyDestinationPort();
            }
        }

        //
        // check if this our notification and not in use
        //
        {
            IUnknown *pUnk = 0;
            HRESULT hr1 = pNotification->QueryInterface(IID_INotificationRunning,(void **)&pUnk);
            if (hr1 == NOERROR)
            {
                NotfAssert((pUnk));
                hr = NOERROR;
                pUnk->Release();
            }
        }
        BREAK_ONERROR(hr);

        pCPackage = new CPackage(
                               pClsidSender,
                               pNotfctnSink,
                               pNotification,
                               deliverMode,
                               pGroupCookie,
                               pTaskTrigger,
                               pTaskData,
                               &clsid
                              );
        if (!pCPackage)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        TRACK_ALLOC(pCPackage);

        if (pCPackage->IsReportRequired())
        {
            //BUGBUG: some work left here!!

            // now create a notificationreply object
            pCPackage->_PackageType = PT_WITHREPLY;
            // Note: this pointer is now not addref'f
            // GetNotificationReply does not addref the pointer either unless specified.
            // So, the object should go away is normal
            // BUT if the receiver holds on to the pointer everthing should stay alive.

            // set up the notification report of the destination
            pCPackage->_pNotfctnReport = &pCPackage->_CNotfctnReportDest;
        }
        /*
        pCPackage->_PackageFlags |=  packageflags;
        {
            CNotfctnObj *pCNotf = (CNotfctnObj *)pNotification;
            //
            // this package needs to keep the notification object alive
            //
            if (pCNotf->GetCPackage() == 0)
            {
                pCNotf->SetCPackage(pCPackage);
            }
        }
        */

        DWORD dwNotifionStateNotfObj = 0;
        {
            CNotfctnObj *pCNotf = (CNotfctnObj *)pNotification;
            //
            // this package needs to keep the notification object alive
            //
            CPackage *pCPkgNotf = pCNotf->GetCPackage();

            if (pCPkgNotf == 0)
            {
                pCNotf->SetCPackage(pCPackage);
            }
            else
            {   
                CLSID   cookieType; 

                if ((packageflags & PF_DELIVERED) &&
                    SUCCEEDED(pCNotf->GetNotificationType(&cookieType)) &&
                    (cookieType == NOTIFICATIONTYPE_AGENT_START) &&
                    (*pCPkgNotf->GetNotificationType() == cookieType))
                {
#if 0    
                    hr = pCPkgNotf->VerifyRunning();
                    if (S_OK == hr)
                    {
                        pCPackage->Release();
                        hr = S_FALSE;
                        break;
                }
#endif
                    pCPkgNotf->VerifyRunning();
                }
                dwNotifionStateNotfObj = pCPkgNotf->GetNotificationState();
            }
            
        }
        pCPackage->_PackageFlags |=  packageflags;

        if (dwNotifionStateNotfObj & PF_WAITING)
        {
            pCPackage->_PackageFlags |=  PF_WAITING;
        }

        *ppCPackage = pCPackage;

        PPKG_DUMP(pCPackage, (DEB_TFLOW | DEB_TMEMORY, "Created in CPackage::CreateDeliver", NOERROR));

        hr  = NOERROR;
        break;
    } while (TRUE);

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::CreateDeliver (hr:%lx\n", pCPackage,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::CreateUpdate
//
//  Synopsis:
//
//  Arguments:  [pNotfctnRprt] --
//              [pNotification] --
//              [deliverMode] --
//              [dwReserved] --
//              [ppCPackage] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::CreateUpdate(
                     CNotificationReport    *pNotfctnRprt,
                     // the notification object itself
                     LPNOTIFICATION          pNotification,
                     // the cookie of the object the notification is targeted too
                     DELIVERMODE             deliverMode,
                     DWORD                   dwReserved,
                     CPackage              **ppCPackage
                     )

{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::CreateUpdate\n", NULL));
    HRESULT hr = E_INVALIDARG;
    NotfAssert((ppCPackage && pNotfctnRprt));
    CPackage *pCPackage = 0;
    CPackage *pCPackageNew = 0;

    do
    {
        if (!ppCPackage)
        {
            break;
        }

        if (!pNotification)
        {
            break;
        }

        //
        // check if this our notification and not in use
        //
        {
            IUnknown *pUnk = 0;
            HRESULT hr1 = pNotification->QueryInterface(IID_INotificationRunning,(void **)&pUnk);
            if (hr1 == NOERROR)
            {
                NotfAssert((pUnk));
                hr = NOERROR;
                pUnk->Release();
            }
        }
        BREAK_ONERROR(hr);


        CNotificationReport *pRprtNew = 0;
        pCPackage = pNotfctnRprt->GetCPackage();
        DWORD dwNotificationState = pCPackage->GetNotificationState();

        NotfAssert((pCPackage));
        if (!pCPackage)
        {
            hr = E_FAIL;
            break;
        }

        switch (pNotfctnRprt->GetReportObjType())
        {
        default:
            break;
        case RO_SENDER:
            pRprtNew = pCPackage->GetNotificationReportDest();
            break;
        case RO_DEST:
            pRprtNew = pCPackage->GetNotificationReportSender();
            break;
        }

        NotfAssert((pRprtNew));

        if (!pRprtNew)
        {
            hr = E_FAIL;
            break;
        }

        NotfAssert(( pRprtNew->GetClsId() ||
                    pRprtNew->GetNotificationSink() ||
                    pRprtNew->GetThreadId() ||
                    (dwNotificationState & PF_CROSSPROCESS)));

        if (!pRprtNew->GetClsId() && 
            !pRprtNew->GetNotificationSink() && 
            !pRprtNew->GetThreadId() && 
            !(dwNotificationState & PF_CROSSPROCESS))
        {
            hr = E_FAIL;
            break;
        }

        pCPackageNew = new CPackage(
                                  pRprtNew               // CNotificationReport
                                 ,pNotification          // pNotification,
                                 ,(DELIVERMODE)0         // deliverMode,
                                 );

        if (!pCPackageNew)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        TRACK_ALLOC(pCPackageNew);

        // transfer the state of the ole
        if (dwNotificationState & PF_WAITING)
        {
            pCPackageNew->SetNotificationState(pCPackageNew->GetNotificationState() | PF_WAITING);
        }
        
        {
            CNotfctnObj *pCNotf = (CNotfctnObj *)pNotification;

            //
            // assosiate this package with the notification object
            // the package and the notification object will go away together
            if ( !(dwNotificationState & PF_CROSSPROCESS))
            {
                if (pCNotf->GetCPackage() == 0)
                {
                    pCNotf->SetCPackage(pCPackage);
                }
            }
        }

        PPKG_DUMP(pCPackageNew, (DEB_TFLOW | DEB_TMEMORY, "CPackage created in CPackage::CreateUpdate", NOERROR));

        *ppCPackage = pCPackageNew;
        hr  = NOERROR;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::CreateUpdate (hr:%lx\n", pCPackageNew,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::CreateReport
//
//  Synopsis:   creates a report packag
//
//  Arguments:  [pNotification] --
//              [pRunningNotfCookie] --
//              [dwReserved] --
//              [ppCPackage] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::CreateReport(
                    // the notification object itself
                    LPNOTIFICATION          pNotification,
                    // the cookie of the object the notification is targeted too
                    PNOTIFICATIONCOOKIE     pRunningNotfCookie,
                    DWORD                   dwReserved,
                    CPackage              **ppCPackage
                    )

{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::CreateReport\n", NULL));
    HRESULT hr = E_INVALIDARG;
    NotfAssert((ppCPackage));
    CPackage *pCPackage = 0;
    CNotificationMgr  *pCNotMgr = 0;    //needed to register and unregister

    do
    {
        if (!ppCPackage)
        {
            break;
        }

        if (!pNotification)
        {
            break;
        }

        if (!pCPackage)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        *ppCPackage = pCPackage;
        hr  = NOERROR;

        break;
    } while (TRUE);

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::CreateReport (hr:%lx\n", pCPackage,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::RemovePersist
//
//  Synopsis:   remove a persistance package form the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::RemovePersist(LPCSTR pszWhere,
                                LPSTR pszSubKeyIn,
                                DWORD dwMode)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::RemovePersist\n", NULL));
    PPKG_DUMP(this, (DEB_TPERSIST /*| DEB_TFLOW*/, "CPackage::RemovePersist"));

    HRESULT hr = NOERROR;
    NotfAssert((pszWhere));
    // remove the peristed package from the stream

    LPSTR       pszRegKey = 0;
    LPSTR       pszSubKey = 0;

    do
    {
        if (!pszWhere)
        {
            hr = E_INVALIDARG;
            break;
        }

        if (pszSubKeyIn == 0)
        {
            pszSubKey = StringAFromCLSID( &(GetNotificationCookie()) );
        }
        else
        {
            pszSubKey = pszSubKeyIn;
        }

        if (!pszSubKey)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        TNotfDebugOut((DEB_PACKAGE, "%p Removing in CPackage::RemovePersist\n  pszWhere=%s pszSubKey=%s\n", 
            this, pszWhere, pszSubKey));

        {
            long    lRes;
            DWORD   dwDisposition, dwType, dwSize;
            HKEY    hKey;
            char szKeyToDelete[1024];
            strcpy(szKeyToDelete, pszWhere);
            strcat(szKeyToDelete, pszSubKey);
            lRes = RegDeleteKey(HKEY_CURRENT_USER,szKeyToDelete);
        }

        if (pszRegKey)
        {
            delete [] pszRegKey;
        }
        if (pszSubKey && !pszSubKeyIn)
        {
            delete [] pszSubKey;
        }

        break;
    } while ( TRUE );

    TNotfDebugOut((DEB_TPERSIST /*| DEB_TFLOW*/, "%p OUT CPackage::RemovePersist (hr:%lx)\n",this, hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::RemovePersist (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::SaveToPersist
//
//  Synopsis:   saves a peristance package to the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::SaveToPersist(LPCSTR pszWhere,
                                LPSTR pszSubKeyIn,
                                DWORD dwMode)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::SaveToPersist\n", NULL));
    PPKG_DUMP(this, (DEB_TPERSIST, "IN CPackage::SaveToPersist"));

    HRESULT hr = E_INVALIDARG;
    NotfAssert((pszWhere));
    // save the package

    CRegStream *pRegStm = 0;
    LPSTR       pszRegKey = 0;
    LPSTR       pszSubKey = 0;

    do
    {
        // BUBUG: need to save the clsid of the current process
        //CLSID clsid = CLSID_StdNotificationMgr;
        //pszRegKey = StringAFromCLSID( &clsid );
        if (pszSubKeyIn == 0)
        {
            pszSubKey = StringAFromCLSID( &(GetNotificationCookie()) );
        }
        else
        {
            pszSubKey = pszSubKeyIn;
        }

        if (pszSubKey)
        {
            CHAR szTemp[512];
            strcpy(szTemp, pszWhere);
            strcat(szTemp, pszSubKey);
            pRegStm = new CRegStream(HKEY_CURRENT_USER, szTemp,pszSubKey, TRUE);
        }

        if (pszRegKey)
        {
            delete [] pszRegKey;
        }

        if (!pRegStm)
        {
           hr = E_OUTOFMEMORY;
           break;
        }

        IStream *pStm = 0;
        hr = pRegStm->GetStream(&pStm);
        if (hr != NOERROR)
        {

            delete pRegStm;
            break;
        }

        if (   (dwMode & PF_NOTIFICATIONOBJECT_ONLY)
            && (IsPersisted(c_pszRegKey) == S_OK) )
        
        {
            CPackage *pCPkgLoad = 0;
            hr = CPackage::LoadFromPersist(c_pszRegKey,pszSubKey, PF_NOTIFICATIONOBJECT_ONLY, &pCPkgLoad);
            if (hr == NOERROR)
            {
                TNotfDebugOut((DEB_TPERSIST, "0x%p After LoadFromPersist CPackage::SaveToPersist\n", pCPkgLoad));

                ADDREF(this);
                LPNOTIFICATION pNotfOld = pCPkgLoad->GetNotification(TRUE);
                LPNOTIFICATION pNotfNew = GetNotification(TRUE);

                NotfAssert((pNotfNew));
                pNotfNew->AddRef();

                pCPkgLoad->SetNotification(pNotfNew);
                
                hr = pCPkgLoad->Save(pStm, TRUE);    
                pCPkgLoad->SetNotification(NULL);

                ((CNotfctnObj *)pNotfNew)->SetCPackage(this);

                pNotfNew->Release();
                pNotfNew->Release();

                pNotfOld->Release();
                RELEASE(pCPkgLoad);
                RELEASE(this);
            }
        }
        else
        {
            // save the item and the notification
            hr = Save(pStm, TRUE);
        }
        BREAK_ONERROR(hr);

        if (pStm)
        {
            pStm->Release();
        }

        if (pRegStm)
        {
            pRegStm->SetDirty();
            delete pRegStm;
        }

        break;
    } while ( TRUE );

    if (pszSubKey && !pszSubKeyIn)
    {
        delete [] pszSubKey;
    }


    TNotfDebugOut((DEB_TPERSIST, "%p OUT CPackage::SaveToPersist (hr:%lx)\n",this, hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::SaveToPersist (hr:%lx)\n",this, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::LoadFromPersist
//
//  Synopsis:   loads a persistence package form the registry
//
//  Arguments:  [pszWhere] --
//              [dwMode] --
//
//  Returns:
//
//  History:    1-15-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::LoadFromPersist(LPCSTR pszWhere,
                                  LPSTR pszSubKey,
                                  DWORD dwMode,
                                  CPackage **ppCPackage)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::LoadFromPersist\n", NULL));
    
    HRESULT hr = E_INVALIDARG;
    NotfAssert((pszWhere));
    // save the package

    CRegStream *pRegStm = 0;
    CPackage *pCPkg = 0;

    do
    {
        if (   !pszWhere
            || !pszSubKey
            || !ppCPackage )

        {
            break;
        }

        pCPkg = new CPackage(CPACKAGE_MAGIC);

        if (!pCPkg)
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        TRACK_ALLOC(pCPkg);

        CHAR szTemp[512];
        strcpy(szTemp, pszWhere);
        strcat(szTemp, pszSubKey);
        
        pRegStm = new CRegStream(HKEY_CURRENT_USER, szTemp,pszSubKey, FALSE);

        if (!pRegStm)
        {
           hr = E_OUTOFMEMORY;
           break;
        }

        IStream *pStm = 0;
        hr = pRegStm->GetStream(&pStm);
        if (hr != NOERROR)
        {
            delete pRegStm;

            TNotfDebugOut((DEB_TMEMORY, "CPackage %p deleted because of GetStream failure in CPackage::LoadFromPersist\n", pCPkg));
            delete pCPkg;
            pCPkg = NULL;
            break;
        }

        // save the item and the notification
        hr = pCPkg->Load(pStm);
        BREAK_ONERROR(hr);

        if (pStm)
        {
            pStm->Release();
        }

        if (pRegStm)
        {
            delete pRegStm;
        }

        PPKG_DUMP(pCPkg, (DEB_TPERSIST | DEB_TMEMORY, "Created in CPackage::LoadFromPersist"));

        *ppCPackage = pCPkg;

        break;
    } while ( TRUE );

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::LoadFromPersist (hr:%lx)\n",pCPkg, hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::IsPersisted
//
//  Synopsis:
//
//  Arguments:  [LPSTR] --
//              [pszSubKey] --
//
//  Returns:
//
//  History:    2-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::IsPersisted(LPCSTR pszWhere,LPSTR pszSubKey)

{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::IsPersisted\n", this));
    HRESULT hr = E_FAIL;
    LPSTR szPackageSubKey = 0;

    if (!pszSubKey)
    {
        szPackageSubKey = pszSubKey =
            StringAFromCLSID( &(GetNotificationCookie()) );
    }

    if (pszSubKey)
    {
        CHAR szTemp[512];
        strcpy(szTemp, pszWhere);
        strcat(szTemp, pszSubKey);

        hr = RegIsPersistedKey(HKEY_CURRENT_USER, szTemp, pszSubKey);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    if (szPackageSubKey)
    {
        delete [] szPackageSubKey;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::IsPersisted (hr:%lx)\n",this, hr));
    return hr;
}

HRESULT CPackage::IsPersisted(LPCSTR pszWhere, PNOTIFICATIONCOOKIE pNotfCookie)

{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::IsPersisted\n", NULL));
    HRESULT hr = E_FAIL;
    LPSTR szPackageSubKey = 0;

    if (pNotfCookie)
    {
        szPackageSubKey = StringAFromCLSID( pNotfCookie );
    }

    if (szPackageSubKey)
    {
        CHAR szTemp[512];
        strcpy(szTemp, pszWhere);
        strcat(szTemp, szPackageSubKey);
        hr = RegIsPersistedKey(HKEY_CURRENT_USER, szTemp, szPackageSubKey);
    }
    else
    {
        hr = E_FAIL;
    }
    delete [] szPackageSubKey;

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::IsPersisted (hr:%lx)\n",NULL, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPackage::LoadPersistedPackage
//
//  Synopsis:
//
//  Arguments:  [pszWhere] --
//              [pNotfCookie] --
//              [dwMode] --
//              [ppCPackage] --
//
//  Returns:
//
//  History:    2-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::LoadPersistedPackage(
                       LPCSTR               pszWhere,
                       PNOTIFICATIONCOOKIE  pNotfCookie,
                       DWORD                dwMode,
                       CPackage  **ppCPackage)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::LoadPersistedPackage\n", NULL));
    HRESULT hr = E_FAIL;
    LPSTR szPackageSubKey = 0;

    if (pNotfCookie)
    {
        szPackageSubKey = StringAFromCLSID( pNotfCookie);
    }
    if (szPackageSubKey)
    {
        CHAR szTemp[512];
        strcpy(szTemp, pszWhere);
        strcat(szTemp, szPackageSubKey);
    
        hr = RegIsPersistedKey(HKEY_CURRENT_USER, szTemp, szPackageSubKey);
        if (hr == NOERROR)
        {
            hr = LoadFromPersist(pszWhere,szPackageSubKey,
                                 dwMode,ppCPackage);
        }
        delete [] szPackageSubKey;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::LoadPersistedPackage (hr:%lx)\n", NULL, hr));
    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   RegIsPersistedKey
//
//  Synopsis:
//
//  Arguments:  [hBaseKey] --
//              [pszRegKey] --
//              [pszSubKey] --
//
//  Returns:
//
//  History:    2-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT RegIsPersistedKey(HKEY hBaseKey, LPCSTR pszRegKey, LPSTR pszSubKey)
{
    long    lRes;
    DWORD   dwDisposition, dwType, dwSize;
    HKEY    hKey = 0;
    BOOL    fRet = FALSE;
    
    lRes = RegOpenKey(
                         hBaseKey,
                         pszRegKey,
                         &hKey
                         );
                         

    if (lRes == ERROR_SUCCESS)
    {
        lRes = RegQueryValueEx(hKey, pszSubKey, NULL, &dwType, NULL, &dwSize);
        fRet = (lRes == ERROR_SUCCESS);
        RegCloseKey(hKey);
    }

    return (fRet) ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Function:   RegIsPersistedValue
//
//  Synopsis:
//
//  Arguments:  [hBaseKey] --
//              [pszRegKey] --
//              [pszSubKey] --
//
//  Returns:
//
//  History:    2-28-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT RegIsPersistedValue(HKEY hBaseKey, LPCSTR pszRegKey, LPSTR pszSubKey)
{
    long    lRes;
    DWORD   dwDisposition, dwType, dwSize;
    HKEY    hKey = 0;
    BOOL    fRet = FALSE;
    
    lRes = RegCreateKeyEx(
                         hBaseKey,
                         pszRegKey,
                         0,
                         NULL,
                         0,
                         HKEY_READ_WRITE_ACCESS,
                         NULL,
                         &hKey,
                         &dwDisposition
                         );

    if (lRes == ERROR_SUCCESS)
    {
        lRes = RegQueryValueEx(hKey, pszSubKey, NULL, &dwType, NULL, &dwSize);
        fRet = (lRes == ERROR_SUCCESS);
        RegCloseKey(hKey);
    }

    return (fRet) ? S_OK : E_FAIL;
}


//+---------------------------------------------------------------------------
//
//  Method:     CPackage::QueryInterface
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
STDMETHODIMP CPackage::QueryInterface(REFIID riid, void **ppvObj)
{
    VDATEPTROUT(ppvObj, void *);
    VDATETHIS(this);
    HRESULT hr = NOERROR;

    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::QueryInterface\n", this));

    *ppvObj = NULL;
    if ((riid == IID_IUnknown))
    {
        *ppvObj = (IUnknown *)this;
        ADDREF(this);
    }
    else
    {
        hr = E_NOINTERFACE;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::QueryInterface (hr:%lx\n", this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   CPackage::AddRef
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
STDMETHODIMP_(ULONG) CPackage::AddRef(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::AddRef\n", this));

    LONG lRet = ++_CRefs;

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::AddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   CPackage::Release
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
STDMETHODIMP_(ULONG) CPackage::Release(void)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::Release\n", this));
    --_CRefs;
    LONG lRet = _CRefs;

    if (lRet == 1)
    {
        // need to break refcount cycle
        //
        if (_pNotification)
        {
            CNotfctnObj *pNotf = (CNotfctnObj *)_pNotification;
            if (   (pNotf->GetCPackage() == this)
                //BUGBUG this needs to go!
                && (pNotf->GetRefCount() == 1)
               )
            {
                _pNotification =  0;
                pNotf->Release();
            }
        }
    }
    else if (lRet == 0)
    {
        if (_pNotification)
        {
            CNotfctnObj *pNotf = (CNotfctnObj *)_pNotification;

            if (pNotf->GetCPackage() == this)
            {
                //NotfAssert(( pNotf->GetRefCount() == 1 ));
            }
        }

        delete this;
    }

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::Release (cRefs:%ld)\n",this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   STDMETHODIMP_
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPackage::ReportAddRef(NOTFREPORTOBJ_TYPE NotfReportObjType)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::ReportAddRef\n", this));

    ULONG lRet = ADDREF(this);

    switch (NotfReportObjType)
    {
    case RO_SENDER:
        _CRefsReportSender++;
        break;
    case RO_DEST:
        _CRefsReportDest++;
        break;
    default:
        NotfAssert((FALSE));
    }   // end switch

    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::ReportAddRef (cRefs:%ld)\n", this,lRet));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Function:   STDMETHODIMP_
//
//  Synopsis:
//
//  Arguments:  [ULONG] --
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CPackage::ReportRelease(NOTFREPORTOBJ_TYPE NotfReportObjType)
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::ReportRelease\n", this));
    ULONG lRetReprot = 0;
    switch (NotfReportObjType)
    {
    case RO_SENDER:
        NotfAssert((_CRefsReportSender));
        _CRefsReportSender--;
        lRetReprot = _CRefsReportSender;
        if (_CRefsReportSender == 0)
        {
            // BUGBUG: more work needs to be done here
            // send completion report to dest
            
            if (_CRefsReportDest == 0)
            {
                _CNotfctnReportSender.SetNotificationSink(NULL);
                _CNotfctnReportDest.SetNotificationSink(NULL);
            }
        }
        break;
    case RO_DEST:
        NotfAssert((_CRefsReportDest));
        _CRefsReportDest--;
        lRetReprot = _CRefsReportDest;
        if (_CRefsReportDest == 0)
        {
            PostDispatch();
            // BUGBUG: more work needs to be done here
            // send completion report to sender

            if (_CRefsReportSender == 0)
            {
                _CNotfctnReportDest.SetNotificationSink(NULL);
                _CNotfctnReportSender.SetNotificationSink(NULL);
            }
        }

        break;
    default:
        NotfAssert((FALSE));
    }   // end switch


    ULONG lRet = RELEASE(this);
    
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::ReportRelease (cRefs:%ld)\n",this,lRet, lRetReprot));
    return lRet;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::PreDispatch
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::PreDispatch()
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::PreDispatch\n", this));
    TNotfDebugOut((DEB_TPKGDETAIL | DEB_TFLOW, "%p IN CPackage::PreDispatch\n", this));

    HRESULT hr = NOERROR;


    if (GetNotificationReport())
    {
        //
        // inform the sender that the notification gets dispatched.
        //
        if ( GetDeliverMode() & (DM_NEED_PROGRESSREPORT) )
        {
            LPNOTIFICATION pNotification = 0;
            NOTIFICATIONTYPE notfType = NOTIFICATIONTYPE_TASKS_STARTED;

            hr = GetNotificationMgr()->CreateNotification(
                                                    notfType
                                                    ,0
                                                    ,0
                                                    ,&pNotification
                                                    ,0);
            if (hr == NOERROR)
            {
                WriteGUID(pNotification, WZ_COOKIE, &GetNotificationCookie());
                GetNotificationReport()->DeliverUpdate(pNotification, 0 , 0);
                pNotification->Release();
                pNotification = 0;
            }
        }        

        if (GetNotificationState() & PF_DELIVERED)
        {

            if (IsPersisted(c_pszRegKey) != NOERROR)
            {
                // NOTIFICATION WAS DELIVERED with a notification report
                //
                CSchedListAgent *pListAgent = GetScheduleListAgent();
                NotfAssert((pListAgent));
                CListAgent *pListAgent1 = pListAgent->GetListAgent(0);
                // set the date and add to list
                SetNextRunDate(CFileTime(MAX_FILETIME));
                SetNotificationState( (GetNotificationState() | PF_RUNNING) & ~(PF_DELIVERED | PF_WAITING));
                hr = pListAgent1->HandlePackage(this);
            }
            else
            {
                SetNotificationState( (GetNotificationState() | PF_RUNNING) & ~PF_DELIVERED);

                CPackage *pCPkgPersisted = 0;
                HRESULT hr1 = CPackage::LoadPersistedPackage(c_pszRegKey, &GetNotificationCookie(), 0, &pCPkgPersisted);
                if (hr1 == NOERROR)
                {
                    NotfAssert((pCPkgPersisted));
                    pCPkgPersisted->SetRunningCookie(GetRunningCookie());
                    pCPkgPersisted->SetDestPort(GetThreadNotificationWnd(),
                                                GetCurrentThreadId());
                    pCPkgPersisted->SetNotificationState( (GetNotificationState() | PF_RUNNING) & ~PF_DELIVERED);
                    hr1 = pCPkgPersisted->SaveToPersist(c_pszRegKey);
                    RELEASE(pCPkgPersisted);
                }
            }
            SetNotificationState( GetNotificationState() | PF_DISPATCHED);
        }
        else
        {
            TransAssert(( GetNotificationState() & PF_SCHEDULED ));

            // NORMAL SCHEDULED NOTIFICATION
            CSchedListAgent *pListAgent = GetScheduleListAgent();
            NotfAssert((pListAgent));
            CListAgent *pListAgent1 = pListAgent->GetListAgent(0);

            CDestinationPort &rCDest = GetCDestPort();
            SetDestPort(GetThreadNotificationWnd(), GetCurrentThreadId());
            SetNotificationState( (GetNotificationState() |  PF_RUNNING) & ~PF_DELIVERED);
            hr = SaveToPersist(c_pszRegKey);
            SetDestPort(rCDest.GetPort(), rCDest.GetDestThreadId());
            SetNotificationState( GetNotificationState() | PF_DISPATCHED);
        }

    }
    else if (!(GetNotificationState() & PF_WAITING))
    {
        CScheduleAgent *pSchedLst = GetScheduleAgent();
        HRESULT hr1 = NOERROR; 
        hr1 = pSchedLst->RevokePackage(&GetNotificationCookie(), 0, 0);
    }

    TNotfDebugOut((DEB_TPKGDETAIL | DEB_TFLOW, "%p OUT CPackage::PreDispatch (hr:%lx)\n",this,hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::PreDispatch (hr:%lx)\n",this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::PostDispatch
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::PostDispatch()
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::PostDispatch\n", this));
    TNotfDebugOut((DEB_TPKGDETAIL | DEB_TFLOW, "%p IN CPackage::PostDispatch\n", this));
    
    HRESULT hr = NOERROR;

    // if loaded cross process remove the key
    // 
    if (_dwParam)
    {
        NotfAssert(( GetNotificationState() & PF_CROSSPROCESS ));
        CHAR szPackageSubKey[SZREGVALUE_MAX] = {0};
        wsprintf(szPackageSubKey,"%lx",_dwParam);

        RemovePersist(c_pszRegKeyPackage,szPackageSubKey);
    }

    HRESULT hr1 = CalcNextRunDate();
    //NotfAssert((hr1 == S_OK));
    
    UpdatePrevRunDate();
    SetNotificationState(GetNotificationState() & ~PF_SUSPENDED);
    //
    // deliver the completion report notification to the sender
    //
    if (    GetNotificationReport()
        &&  (GetDeliverMode() & (DM_NEED_COMPLETIONREPORT | DM_NEED_PROGRESSREPORT)) ) 
    {
        LPNOTIFICATION pNotification = 0;
        NOTIFICATIONTYPE notfType = (_hrDest == NOERROR) ? NOTIFICATIONTYPE_TASKS_COMPLETED 
                                                         : NOTIFICATIONTYPE_TASKS_ERROR ;
        
        hr = GetNotificationMgr()->CreateNotification(
                                                notfType
                                                ,0
                                                ,0
                                                ,&pNotification
                                                ,0);
        if (hr == NOERROR)
        {
            WriteGUID(pNotification, WZ_COOKIE, &GetNotificationCookie());
            if (_hrDest != NOERROR)
            {
                WriteDWORD(pNotification, WZ_HRESULT,  _hrDest);
                _hrDest = NOERROR;
            }

            GetNotificationReport()->DeliverUpdate(pNotification, 0 , 0);
            pNotification->Release();
            pNotification = 0;
        }
    } 
    
    BOOL    fResched = FALSE;
    //
    // normal cleanup stuff
    //
    if (   !(GetNotificationState() & PF_WAITING)
        &&  (GetNotificationState() & PF_DISPATCHED) )
    {
        CScheduleAgent *pSchedAgent = GetScheduleAgent();
        HRESULT hr1 = NOERROR; 
        hr1 = pSchedAgent->RevokePackage(&GetNotificationCookie(), 0, 0);
    }
    else
    {
        // update state
        SetNotificationState(GetNotificationState() & ~PF_RUNNING);

        if (!(GetNotificationState() & PF_WAITING))
        {
            CScheduleAgent *pSchedLst = GetScheduleAgent();
            HRESULT hr1 = NOERROR; 
            hr1 = pSchedLst->RevokePackage(&GetNotificationCookie(), 0, 0);
        }
        else if (IsPersisted(c_pszRegKey) == S_OK)
        {
            SetNotificationState( GetNotificationState() & ~PF_RUNNING);

            CPackage *pCPkgPersisted = 0;
            HRESULT hr1 = CPackage::LoadPersistedPackage(c_pszRegKey, &GetNotificationCookie(), 0, &pCPkgPersisted);
            if (hr1 == NOERROR)
            {
                NotfAssert((pCPkgPersisted));
                pCPkgPersisted->SetNextRunDate(GetNextRunDate());
                pCPkgPersisted->SetPrevRunDate(GetPrevRunDate());
                pCPkgPersisted->SetNotificationState( GetNotificationState() & ~PF_RUNNING);
                pCPkgPersisted->SetRunningCookie(NULL);
                pCPkgPersisted->SetDestPort(NULL, 0);
                hr1 = pCPkgPersisted->SaveToPersist(c_pszRegKey);
                RELEASE(pCPkgPersisted);
                fResched = TRUE;
            }
        }
    }

    if (GetDeliverMode() & DM_THROTTLE_MODE)
    {
        if ( GetNotificationState() & PF_CROSSPROCESS)
        {
            TransAssert(( GetGlobalNotfMgr() ));
            GetGlobalNotfMgr()->RemoveItemFromRunningList( this, 0);
        }
        else
        {
            CPackage *pExistingPkg;
            TransAssert(( GetThrottleListAgent() ));
            if (SUCCEEDED(GetThrottleListAgent()->FindPackage(&GetNotificationCookie(),
                                                              &pExistingPkg, 0)))
            {
                NotfAssert(pExistingPkg);
                if (pExistingPkg == this)
                {
                    GetThrottleListAgent()->RevokePackage( &GetNotificationCookie(), 0, 0);
                }
                pExistingPkg->CalcNextRunDate();
                RELEASE(pExistingPkg);
                fResched = TRUE;
            }
        }
    }

    if (GetNotificationState() & PF_REVOKED)
    {
        GetScheduleAgent()->RevokePackage(&GetNotificationCookie(), 0, 0);
        RemovePersist(c_pszRegKeyPackage);
        fResched = FALSE;
    } else  {
        CLSID   cookieNULL = CLSID_NULL;
        GetScheduleAgent()->RevokePackage(&cookieNULL, 0, 0);   //  Force to Synchronize.
    }

    CSchedListAgent *pCSchLst = GetScheduleListAgent();
    NotfAssert((pCSchLst));
    
    if (pCSchLst)
    {
        if (fResched && (IsPersisted(c_pszRegKey) == S_OK))
        {
            if (GetTaskTrigger() == NULL)
            {
                CLSID   cookie = GetNotificationCookie();
                CPackage *pPkgFound = NULL;
                hr = pCSchLst->FindPackage(&cookie, &pPkgFound, LM_LOCALCOPY);
                if ((NOERROR == hr) && pPkgFound)
                {
                    if (pPkgFound->GetTaskTrigger() != NULL)
                    {
                        pPkgFound->CalcNextRunDate();
                        pPkgFound->SetPrevRunDate(GetPrevRunDate());
                        hr = pCSchLst->HandlePackage(pPkgFound);
                    }
                    RELEASE(pPkgFound);
                }
            }
            else
            {
                hr = pCSchLst->HandlePackage(this);
            }
        }
        hr = pCSchLst->SetWakeup();
    }
    else
    {
        hr = E_FAIL;
    }

    TNotfDebugOut((DEB_TPKGDETAIL | DEB_TFLOW, "%p OUT CPackage::PostDispatch (hr:%lx)\n",this,hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::PostDispatch (hr:%lx)\n",this,hr));
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::PreDeliver
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    1-24-1997   JohannP (Johann Posch)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
HRESULT CPackage::PreDeliver()
{
    NotfDebugOut((DEB_PACKAGE, "%p _IN CPackage::PreDeliver\n", this));
    TNotfDebugOut((DEB_TPKGDETAIL, "IN CPackage::PreDeliver\n", this));
    HRESULT hr = NOERROR;

    if (IsReportRequired() && (IsPersisted(c_pszRegKey) != NOERROR) )
    {
        // NOTIFICATION WAS DELIVERED with a notification report
        //
        CSchedListAgent *pListAgent = GetScheduleListAgent();
        NotfAssert((pListAgent));
        CListAgent *pListAgent1 = pListAgent->GetListAgent(0);
        // set the date and add to list
        SetNextRunDate(CFileTime(MAX_FILETIME));
        SetNotificationState( GetNotificationState() | PF_RUNNING);
        hr = pListAgent1->HandlePackage(this);
    }
    
    TNotfDebugOut((DEB_TPKGDETAIL, "%p OUT CPackage::PreDeliver (hr:%lx)\n",this,hr));
    NotfDebugOut((DEB_PACKAGE, "%p OUT CPackage::PreDeliver (hr:%lx)\n",this,hr));
    return hr;
}


HRESULT
CPackage::VerifyRunning(void)
{
    if (GetNotificationState() & PF_RUNNING)    {
        if (_CDestPort.GetPort() != NULL)   {
            if (IsWindow(_CDestPort.GetPort())) {
                //  Verify the running status.
                return S_OK;
            }
        }
        SetNotificationState(GetNotificationState() & ~PF_RUNNING);

        CGlobalNotfMgr *pGlbNotfMgr = GetGlobalNotfMgr();
        NotfAssert(pGlbNotfMgr);
        pGlbNotfMgr->RemoveItemFromRunningList(this, 0);
        //  Review:
        //      dwMode of RemoveItemFromRunningList is never used.
    }
    return S_FALSE;
}

#if DBG == 1

void SpewTime(CFileTime& ft, char *psz)
{
    SYSTEMTIME st;

    FileTimeToSystemTime(&ft, &st);

    TNotfDebugOut((DEB_TALL, "%s %02d:%02d:%02d %02d/%02d/%04d\n",
                  psz, st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));
}

#include <webcheck.h>

char *StringFromPackage(PACKAGE_TYPE pt)
{
    switch (pt)
    {
        case PT_NOT_INITIALIZED:    return "Not Init";
        case PT_NORMAL:             return "Normal  ";
        case PT_WITHREPLY:          return "W/Reply ";
        case PT_REPORT_TO_SENDER:   return "Rpt2Send";
        case PT_REPORT_TO_DEST:     return "Rpt2Dest";
        case PT_INVALID:            return "Invalid ";
        case PT_GROUPLEADER:        return "GrpLead ";
        case PT_GROUPMEMBER:        return "GrpMembr";
        default:                    return "Unknown ";
    }
}

WCHAR *StringFromCookie(REFGUID rCookie, WCHAR *pwszDest, DWORD cbDest)
{
    WCHAR *pwsz = L"* Dead Code *";

/*
    if (rCookie == GUID_NULL)
        pwsz = L"* Empty *        ";
    else if (rCookie == CLSID_WebCheck)
        pwsz = L"WebCheck         ";
    else if (rCookie == CLSID_WebCrawlerAgent)
        pwsz = L"WebCrawlerAgent  ";
    else if (rCookie == CLSID_ChannelAgent)
        pwsz = L"ChannelAgent     ";
    else if (rCookie == CLSID_MailAgent)
        pwsz = L"MailAgent        ";
    else if (rCookie == CLSID_OfflineTrayAgent)
        pwsz = L"OfflineTrayAgent ";
    else if (rCookie == CLSID_ConnectionAgent)
        pwsz = L"ConnectionAgent  ";
    else if (rCookie == CLSID_SubscriptionMgr)
        pwsz = L"SubscriptionMgr  ";
    else if (rCookie == CLSID_PostAgent)
        pwsz = L"PostAgent        ";
    else if (rCookie == CLSID_CDLAgent)
        pwsz = L"CDLAgent         ";
    else if (rCookie == NOTIFICATIONTYPE_NULL             )
        pwsz = L"NULL             ";
    else if (rCookie == NOTIFICATIONTYPE_ANOUNCMENT       )
        pwsz = L"ANOUNCMENT       ";
    else if (rCookie == NOTIFICATIONTYPE_TASK             )
        pwsz = L"TASK             ";
    else if (rCookie == NOTIFICATIONTYPE_ALERT            )
        pwsz = L"ALERT            ";
    else if (rCookie == NOTIFICATIONTYPE_INET_IDLE        )
        pwsz = L"INET_IDLE        ";
    else if (rCookie == NOTIFICATIONTYPE_INET_OFFLINE     )
        pwsz = L"INET_OFFLINE     ";
    else if (rCookie == NOTIFICATIONTYPE_INET_ONLINE      )
        pwsz = L"INET_ONLINE      ";
    else if (rCookie == NOTIFICATIONTYPE_TASKS_SUSPEND    )
        pwsz = L"TASKS_SUSPEND    ";
    else if (rCookie == NOTIFICATIONTYPE_TASKS_RESUME     )
        pwsz = L"TASKS_RESUME     ";
    else if (rCookie == NOTIFICATIONTYPE_TASKS_ABORT      )
        pwsz = L"TASKS_ABORT      ";
    else if (rCookie == NOTIFICATIONTYPE_TASKS_COMPLETED  )
        pwsz = L"TASKS_COMPLETED  ";
    else if (rCookie == NOTIFICATIONTYPE_TASKS_PROGRESS   )
        pwsz = L"TASKS_PROGRESS   ";
    else if (rCookie == NOTIFICATIONTYPE_AGENT_INIT       )
        pwsz = L"AGENT_INIT       ";
    else if (rCookie == NOTIFICATIONTYPE_AGENT_START      )
        pwsz = L"AGENT_START      ";
    else if (rCookie == NOTIFICATIONTYPE_BEGIN_REPORT     )
        pwsz = L"BEGIN_REPORT     ";
    else if (rCookie == NOTIFICATIONTYPE_END_REPORT       )
        pwsz = L"END_REPORT       ";
    else if (rCookie == NOTIFICATIONTYPE_CONNECT_TO_INTERNET)
        pwsz = L"CONNECT_TO_INTERNET";
    else if (rCookie == NOTIFICATIONTYPE_DISCONNECT_FROM_INTERNET)
        pwsz = L"DISCONNECT_FROM_INTERNET";
    else if (rCookie == NOTIFICATIONTYPE_CONFIG_CHANGED   )
        pwsz = L"CONFIG_CHANGED   ";
    else if (rCookie == NOTIFICATIONTYPE_PROGRESS_REPORT  )
        pwsz = L"PROGRESS_REPORT  ";
    else if (rCookie == NOTIFICATIONTYPE_USER_IDLE_BEGIN  )
        pwsz = L"USER_IDLE_BEGIN  ";
    else if (rCookie == NOTIFICATIONTYPE_USER_IDLE_END    )
        pwsz = L"USER_IDLE_END    ";
    else if (rCookie == NOTIFICATIONTYPE_TASKS_STARTED    )
        pwsz = L"TASKS_STARTED    ";
    else if (rCookie == NOTIFICATIONTYPE_TASKS_ERROR      )
        pwsz = L"TASKS_ERROR      ";
    else if (rCookie == NOTIFICATIONTYPE_d                )
        pwsz = L"d                ";
    else if (rCookie == NOTIFICATIONTYPE_e                )
        pwsz = L"e                ";
    else if (rCookie == NOTIFICATIONTYPE_f                )
        pwsz = L"f                ";
    else if (rCookie == NOTIFICATIONTYPE_11               )
        pwsz = L"11               ";
    else if (rCookie == NOTIFICATIONTYPE_12               )
        pwsz = L"12               ";
    else if (rCookie == NOTIFICATIONTYPE_13               )
        pwsz = L"13               ";
    else if (rCookie == NOTIFICATIONTYPE_14               )
        pwsz = L"14               ";
    else if (rCookie == NOTIFICATIONTYPE_ITEM_START       )
        pwsz = L"ITEM_START       ";
    else if (rCookie == NOTIFICATIONTYPE_ITEM_RESTART     )
        pwsz = L"ITEM_RESTART     ";
    else if (rCookie == NOTIFICATIONTYPE_ITEM_DONE        )
        pwsz = L"ITEM_DONE        ";
    else if (rCookie == NOTIFICATIONTYPE_GROUP_START      )
        pwsz = L"GROUP_START      ";
    else if (rCookie == NOTIFICATIONTYPE_GROUP_RESTART    )
        pwsz = L"GROUP_RESTART    ";
    else if (rCookie == NOTIFICATIONTYPE_GROUP_DONE       )
        pwsz = L"GROUP_DONE       ";
    else if (rCookie == NOTIFICATIONTYPE_START_0          )
        pwsz = L"START_0          ";
    else if (rCookie == NOTIFICATIONTYPE_START_1          )
        pwsz = L"START_1          ";
    else if (rCookie == NOTIFICATIONTYPE_START_2          )
        pwsz = L"START_2          ";
    else if (rCookie == NOTIFICATIONTYPE_START_3          )
        pwsz = L"START_3          ";
    else if (rCookie == NOTIFICATIONTYPE_START_4          )
        pwsz = L"START_4          ";
    else if (rCookie == NOTIFICATIONTYPE_START_5          )
        pwsz = L"START_5          ";
    else if (rCookie == NOTIFICATIONTYPE_START_6          )
        pwsz = L"START_6          ";
    else if (rCookie == NOTIFICATIONTYPE_START_7          )
        pwsz = L"START_7          ";
    else if (rCookie == NOTIFICATIONTYPE_START_8          )
        pwsz = L"START_8          ";
    else if (rCookie == NOTIFICATIONTYPE_START_9          )
        pwsz = L"START_9          ";
    else if (rCookie == NOTIFICATIONTYPE_START_A          )
        pwsz = L"START_A          ";
    else if (rCookie == NOTIFICATIONTYPE_START_B          )
        pwsz = L"START_B          ";
    else if (rCookie == NOTIFICATIONTYPE_START_C          )
        pwsz = L"START_C          ";
    else if (rCookie == NOTIFICATIONTYPE_START_D          )
        pwsz = L"START_D          ";
    else if (rCookie == NOTIFICATIONTYPE_START_E          )
        pwsz = L"START_E          ";
    else if (rCookie == NOTIFICATIONTYPE_START_F          )
        pwsz = L"START_F          ";
    else
        StringFromGUID2(rCookie, pwszDest, cbDest);
*/

    if (pwsz)
        StrCpyW(pwszDest, pwsz);

    return pwszDest;
}

//+---------------------------------------------------------------------------
//
//  Method:     CPackage::Dump (aka Spew)
//
//  Synopsis:
//
//  Arguments:  pszPrefix - text to prepend to message
//
//  Returns:
//
//  History:    7-24-1997   tnoonan (Tim Noonan)   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
void CPackage::Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr)
{
    if (!(TNotfInfoLevel & dwFlags))
        return;     //  no point in wasting time here

    LPNOTIFICATION pNotification = GetNotification();
    VARIANT varName = {VT_EMPTY}, 
            varURL = {VT_EMPTY};
    varName.bstrVal = NULL;
    WCHAR wszNotfType[40] = L"",
          wszNotfCookie[40] = L"",
          wszSender[40] = L"",
          wszDest[40] = L"";

    if (pNotification)
    {
        pNotification->Read(L"Name", &varName);
    }

   static const char fmt[] = \
        /*  Object Address              */ "%p " \
        /*  Passed in pszPrefix         */ "%s " \
        /*  Passed in HRESULT           */ "hr=%08x\n  " \
        
        /*  Property Map Name           */ "NAME=%40ws " \
        /*  INotification *             */ "Pnotf=%p\n  " \
        
        /*  Package Type                */ "PkgType=%s " \
        /*  Task Trigger Flags          */ "TrigFlags=%08x " \
        /*  Deliver Mode                */ "DelMode=%08x " \
        /*  HWND                        */ "HWND=%08x " \
        /*  Thread ID                   */ "TID=%08x\n  " \
        
        /*  Notification State          */ "State=%08x " \
        /*  Notification Flags          */ "Flags=%08x " \
        /*  Notification Cookie (guid)  */ "Cookie=%ws\n  " \

        /*  Notification Type (guid)    */ "NotfType=%ws " \
        /*  Sender (guid)               */ "Sender=%ws " \
        /*  Destination (guid)          */ "Dest=%ws\n";

    TNotfDebugOut((dwFlags, fmt, 
                  this, 
                  (pszPrefix ? pszPrefix : ""),
                  hr,
                  (varName.bstrVal ? varName.bstrVal : L"* No Name *"),
                  GetNotification(),
                  StringFromPackage(GetPackageType()),
                  GetTaskTriggerFlags(),
                  _deliverMode,
                  GetCDestPort().GetPort(),
                  GetCDestPort().GetDestThreadId(),
                  GetNotificationState(),
                  GetNotificationFlags(),
                  StringFromCookie(GetNotificationCookie(), wszNotfCookie, ARRAYSIZE(wszNotfCookie)),
                  StringFromCookie(GetNotificationID(), wszNotfType, ARRAYSIZE(wszNotfType)),
                  StringFromCookie(GetClassIdSender(), wszSender, ARRAYSIZE(wszSender)),
                  StringFromCookie(GetDestID(), wszDest, ARRAYSIZE(wszDest))
                 )
                );
         
    VariantClear(&varName);
    VariantClear(&varURL);
}

CMap<void *, void *, int, int> *g_pAllocMap = NULL;

void TrackAlloc(void *ptr, int line)
{
    CGlobalNotfMgr *pGlobalNofMgr = GetGlobalNotfMgr();
    if (!pGlobalNofMgr)
        return;

    {
        CLockSmMutex lck(pGlobalNofMgr->_Smxs);

        if (!g_pAllocMap)
            g_pAllocMap = new CMap<void *, void *, int, int>;

        int x;
        if (g_pAllocMap->Lookup(ptr, x))
        {
            TNotfAssert(FALSE);
        }
        g_pAllocMap->SetAt(ptr, line);
    }
}

void UntrackAlloc(void *ptr, int line)
{   
    CGlobalNotfMgr *pGlobalNofMgr = GetGlobalNotfMgr();
    if (!pGlobalNofMgr)
        return;

    {
        CLockSmMutex lck(pGlobalNofMgr->_Smxs);
        TNotfAssert(g_pAllocMap);

        int x;
        if (!g_pAllocMap->Lookup(ptr, x))
        {
            TNotfAssert(FALSE);
        }
        g_pAllocMap->RemoveKey(ptr);
    }
}

void DumpAllocTrack()
{
    CGlobalNotfMgr *pGlobalNofMgr = GetGlobalNotfMgr();
    if (!pGlobalNofMgr)     //  Probably a call from regsvr32 or display.cpl
        return;

    {
        CLockSmMutex lck(pGlobalNofMgr->_Smxs);

        if (g_pAllocMap)
        {
            POSITION pos = g_pAllocMap->GetStartPosition();
            if (pos)
            {
                TNotfDebugOut((DEB_TALL, "*** Leaked %d packages!\n", g_pAllocMap->GetCount()));
            }

            while (pos)
            {
                void *ptr;
                CPackage *pPackage;
                int line;
                g_pAllocMap->GetNextAssoc(pos, ptr, line);
                pPackage = (CPackage *)ptr;
                TNotfDebugOut((DEB_TALL, "%p leaked at line %d refs=%d\n", ptr, line, pPackage->GetRefs()));
                PPKG_DUMP(pPackage, (DEB_TALL, "DumpAllocTrack"));
                LPNOTIFICATION pNotification = NULL;
                pPackage->GetNotification()->QueryInterface(IID_INotification, (void **)&pNotification);
                if (pNotification)
                {
                    TNotfDebugOut((DEB_TALL, "INotification has %d refs\n", pNotification->Release()));
                }
            }
        }
    }
}

#endif
