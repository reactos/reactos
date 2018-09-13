//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       package.hxx
//
//  Contents:   the package class
//
//  Classes:
//
//  Functions:
//
//  History:    2-20-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _PACKAGE_HXX_
#define _PACKAGE_HXX_

#if DBG == 1
void SpewTime(CFileTime& ft, char *psz);
#endif

#ifdef unix
extern "C"
#endif /* unix */
HRESULT GetRunTimes(TASK_TRIGGER & jt,
                    TASK_DATA    * ptd,
                    LPSYSTEMTIME   pstBracketBegin,
                    LPSYSTEMTIME   pstBracketEnd,
                    WORD *         pCount,
                    FILETIME *     pRunList);

class CPackage;
class CNotificationReport;

#define RUNDATE_MAX 16

/*
typedef enum
{
     RC_NOCOOKIE          = 0
    ,RC_GOTCOOKIE         = 1
    ,RC_INVALID           = 2

} RC_STATE;
*/
#define NOTF_BASE_PRIORITY 8
typedef enum
{
     RS_NO_REPLY        = 0   // the package is not set up for reply and report
    ,RS_REPLY           = 1
    ,RS_INVALID         = 2

} NOTREPLY_STATE;

// the package type indicates the type of the package -
typedef enum
{
     PT_NOT_INITIALIZED     = 0   //
    ,PT_NORMAL              = 1
    ,PT_WITHREPLY
    ,PT_REPORT_TO_SENDER
    ,PT_REPORT_TO_DEST
    ,PT_INVALID
    ,PT_GROUPLEADER
    ,PT_GROUPMEMBER

} PACKAGE_TYPE;

// package flags inidicate what the package is doing and is
typedef enum
{
     PF_READY            = 0x00000001
    ,PF_RUNNING          = 0x00000002
    ,PF_WAITING          = 0x00000004
    ,PF_REVOKED          = 0x00000008
    ,PF_SUSPENDED        = 0x00000010
    ,PF_ABORTED          = 0x00000020

    // the pacakge was delivered cross process
    ,PF_CROSSPROCESS     = 0x00010000
    ,PF_SCHEDULED        = 0x00020000
    ,PF_DELIVERED        = 0x00040000
    ,PF_DISPATCHED       = 0x00080000
    
    // idle flags
    ,PF_WAITING_USER_IDLE= 0x00100000
    
} _PACKAGE_FLAGS;
typedef DWORD PACKAGE_FLAGS;

typedef enum
{
      RO_INVALID    = 0
     ,RO_SENDER     = 1
     ,RO_DEST       = 2
} NOTFREPORTOBJ_TYPE;


typedef enum _tagPACKAGE_CONTENT_ENUM
{
     PC_EMPTY            = 0x00000000
    ,PC_CLSIDSENDER      = 0x00000001
    ,PC_CLSIDDEST        = 0x00000002
    ,PC_GROUPCOOKIE      = 0x00000004
    ,PC_RUNCOOKIE        = 0x00000008
    ,PC_TASKTRIGGER      = 0x00000010
    ,PC_TASKDATA         = 0x00000020
    ,PC_BASECOOKIE       = 0x00000040

    ,PC_CLSID            = 0x00000100
    ,PC_SINK             = 0x00000200
    ,PC_THREADID         = 0x00000400
} PACKAGE_CONTENT_ENUM;

typedef DWORD PACKAGE_CONTENT;
const CPACKAGE_MAGIC = 1742;

typedef enum _tagSORTORDER
{
    SO_NEXTRUNDATE
   ,SO_IDLEDATE
} SORTORDER;

typedef enum _tagGLB_NOTF_FLAGS
{
    GLB_NO_TRIGGER = 0x00000001
    
} _GLB_NOTF_FLAGS;

typedef DWORD GLB_NOTF_FLAGS;

//
// private find flags - these flags are used internally and
// can be combined with the ENUM_FALGS
//
typedef enum _tagFIND_FLAGS
{

    //
    // no notification object is returned
     EF_PERSISTED         = 0x00010000
    ,EF_NOPERSISTCHECK    = 0x00020000
    ,EF_WAITING           = 0x00040000
} _FIND_FLAGS;

typedef DWORD FIND_FLAGS;


typedef enum _tagPERIST_FLAGS
{

    //
    // no notification object is returned
     PF_NOTIFICATIONOBJECT_ONLY      = 0x00010000
} _PERSIST_FLAGS;

typedef DWORD PERSIST_FLAGS;


//
// the CNotificationReport represents the either the destination
// or the sender who want to sent report on an notification
// in progress
//
class CNotificationReport : public INotificationReport, public CThreadId
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    // Report methods
    STDMETHODIMP DeliverUpdate(
                    // the Report notification object itself
                    LPNOTIFICATION          pNotification,
                    // not completion report allowed here
                    DELIVERMODE             deliverMode,
                    // the cookie of the object the notification is targeted too
                    DWORD                   dwReserved
                    );
    // get the original notification this report objet belongs too
    STDMETHODIMP GetOriginalNotification(
                    LPNOTIFICATION          *ppNotification
                    );

    STDMETHODIMP GetNotificationStatus(
                    // what kind of status
                    DWORD                   dwStatusIn,
                    // flags if update notification is pending etc.
                    DWORD                  *pdwStatusOut,
                    DWORD                   dwReserved
                    );


    ~CNotificationReport()
    {
        // a lot of work needs to be done here
        // revoke the register sink or clsid
        if (_pNotfctnSink)
        {
            _pNotfctnSink->Release();
            NotfDebugOut((DEB_REPORT, "%p OUT CNotificationReport::~CNotificationReport Release NotfSink(%p)\n",this, _pNotfctnSink));
            _pNotfctnSink = 0;
        }

    }

    CNotificationReport(CPackage *pCPackage, NOTFREPORTOBJ_TYPE type)
                      :   CThreadId()
    {
        // NOTE: do not set the thread id yet
        // we don't know it a creation time
        // the creating thread might be not the correct one

        _pCPackage = pCPackage;
        _NotfReportObjType = type;
        _PackageContent = PC_EMPTY;
        _pNotfctnSink = 0;
    }
    CNotificationReport(NOTFREPORTOBJ_TYPE type)
    {
        _pCPackage = NULL;
        _NotfReportObjType = type;
        _PackageContent = PC_EMPTY;
        _pNotfctnSink = 0;
    }


    //  BUGBUG: This is bad - this can cause us to have a invalid pointer to the _pCPackage!
    
    CNotificationReport(CNotificationReport &pFrom)
    {
        *this = pFrom;
        if (_pNotfctnSink)
        {
            NotfDebugOut((DEB_REPORT, "%p OUT CNotificationReport::CNotificationReport AddRef NotfSink(%p)\n",this, _pNotfctnSink));
            _pNotfctnSink->AddRef();
        }
    }


    void SetCPackage(CPackage *pCPackage)
    {
        // don't addref the pointer
        _pCPackage = pCPackage;
    }

    NOTFREPORTOBJ_TYPE GetReportObjType()
    {
        return _NotfReportObjType;
    }

    LPCLSID GetClsId()
    {
        return (_PackageContent & PC_CLSIDSENDER) ? &_clsid : NULL;
    }

    void SetClsId(LPCLSID pClsId)
    {

        if (pClsId)
        {
            _clsid = *pClsId;
            _PackageContent |= PC_CLSID;
        }
        else
        {
            _PackageContent &= ~PC_CLSID;
        }
    }

    LPNOTIFICATIONSINK GetNotificationSink(BOOL fAddRef = FALSE)
    {
        if (fAddRef && _pNotfctnSink)
        {
            NotfDebugOut((DEB_REPORT, "%p OUT CNotificationReport::GetNotificationSink AddRef NotfSink(%p)\n",this, _pNotfctnSink));
            _pNotfctnSink->AddRef();
        }

        return _pNotfctnSink;
    }

    void SetNotificationSink(LPNOTIFICATIONSINK pSink)
    {
        if (_pNotfctnSink)
        {
            NotfDebugOut((DEB_REPORT, "%p OUT CNotificationReport::SetNotificationSink Release NotfSink(%p)\n",this, _pNotfctnSink));
            _pNotfctnSink->Release();
            _PackageContent &= ~PC_SINK;
        }
        _pNotfctnSink = pSink;
        if (_pNotfctnSink)
        {
            NotfDebugOut((DEB_REPORT, "%p OUT CNotificationReport::SetNotificationSink AddRef NotfSink(%p)\n",this, _pNotfctnSink));
            _pNotfctnSink->AddRef();
            _PackageContent |= PC_SINK;
        }
    }

    CPackage *GetCPackage()
    {
        NotfAssert((_pCPackage));
        return _pCPackage;
    }

private:
    // pointer to the outer object - the package
    // the pointer is not addref'd
    CPackage               *_pCPackage;
    NOTFREPORTOBJ_TYPE      _NotfReportObjType;
    // class id of object this report object represents
    CLSID                   _clsid;
    // pointer to the sink which this
    // object represents - the pointer is addref'd
    //
    LPNOTIFICATIONSINK      _pNotfctnSink;
    PACKAGE_CONTENT         _PackageContent;
}; // CNotificationReport

typedef struct _tagNOTIFICATIONITEMEXTRA
{
    DELIVERMODE             deliverMode;
    CFileTime               dateNextRun;
    CFileTime               datePrevRun;
    NOTIFICATIONCOOKIE      RunningCookie;
    NOTIFICATIONCOOKIE      BaseCookie;
    PACKAGE_TYPE            PackageType;
    PACKAGE_FLAGS           PackageFlags;
    PACKAGE_CONTENT         PackageContent;
    DWORD                   dwThreadIdDestPort;
    HWND                    hWndDestPort;

} NOTIFICATIONITEMEXTRA, *PNOTIFICATIONITEMEXTRA;


class CPackage : public CThreadId, public CThreadPacket, public IUnknown
{
public:
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // Reply Methods

    // CThreadPacket methods
    LRESULT OnPacket(UINT msg, DWORD dwParam);
    HRESULT Deliver(HWND hwnd, DWORD dwMode);

    HRESULT SyncTo(CPackage *pUpdatedPkg);


    STDMETHODIMP_(ULONG) ReportAddRef(NOTFREPORTOBJ_TYPE NotfReportObjType);
    STDMETHODIMP_(ULONG) ReportRelease(NOTFREPORTOBJ_TYPE NotfReportObjType);

    // needed for persistence
    HRESULT GetNotificationItemExtra(PNOTIFICATIONITEMEXTRA pNotfItemExtra)
    {
        NotfAssert((pNotfItemExtra));

        pNotfItemExtra->deliverMode     = _deliverMode;
        pNotfItemExtra->dateNextRun     = _dateNextRun;
        pNotfItemExtra->datePrevRun     = _datePrevRun;
        pNotfItemExtra->RunningCookie   = _RunningCookie;
        pNotfItemExtra->BaseCookie      = _BaseCookie;
        pNotfItemExtra->PackageType     = _PackageType;
        pNotfItemExtra->PackageFlags    = _PackageFlags;
        pNotfItemExtra->PackageContent  = _PackageContent;
        
        pNotfItemExtra->dwThreadIdDestPort  = _CDestPort.GetDestThreadId()   ;
        pNotfItemExtra->hWndDestPort        = _CDestPort.GetPort()   ;

        return NOERROR;
    }

    HRESULT SetNotificationItemExtra(PNOTIFICATIONITEMEXTRA pNotfItemExtra)
    {
        NotfAssert((pNotfItemExtra));

        _deliverMode    =    pNotfItemExtra->deliverMode     ;
        _dateNextRun    =    pNotfItemExtra->dateNextRun     ;
        _datePrevRun    =    pNotfItemExtra->datePrevRun     ;
        _RunningCookie  =    pNotfItemExtra->RunningCookie   ;
        _BaseCookie     =    pNotfItemExtra->BaseCookie      ;
        _PackageType    =    pNotfItemExtra->PackageType     ;
        _PackageFlags   =    pNotfItemExtra->PackageFlags    ;
        _PackageContent =    pNotfItemExtra->PackageContent  ;
        _CDestPort.SetDestPort(pNotfItemExtra->hWndDestPort, pNotfItemExtra->dwThreadIdDestPort);
        
        return NOERROR;
    }

    //  Return
    //      S_OK:       Running
    //      S_FALSE:    Not Running. Throttle list cleaned up.
    //      ERRORCODE:  Error
    HRESULT VerifyRunning();

    HRESULT GetNotificationItem(PNOTIFICATIONITEM pNotificationItem, ENUM_FLAGS dwMode = (ENUM_FLAGS)0)
    {
        NotfAssert((pNotificationItem));
        HRESULT hr = NOERROR;
        NotfAssert((    pNotificationItem
                     && (pNotificationItem->cbSize >= sizeof(NOTIFICATIONITEM)) ));

        if (   !pNotificationItem
            || (pNotificationItem->cbSize < sizeof(NOTIFICATIONITEM))
           )
        {
            NotfAssert((_pNotification));
            hr = E_INVALIDARG;
        }
        else
        {
            memset(pNotificationItem, 0, sizeof(NOTIFICATIONITEM));
            pNotificationItem->cbSize              = sizeof(NOTIFICATIONITEM);
            pNotificationItem->NotificationType    = _NotificationType;
            if ( !(dwMode & EF_NOT_NOTIFICATION) )
            {
                NotfAssert((_pNotification));
                pNotificationItem->pNotification   = _pNotification;
                _pNotification->AddRef();
            }
            pNotificationItem->NotificationFlags   = _NotificationFlags;
            pNotificationItem->NotificationCookie  = _NotificationCookie;
            pNotificationItem->TaskTrigger         = _schdata;
            pNotificationItem->DeliverMode         = _deliverMode;
            pNotificationItem->TaskData            = _TaskData;
            pNotificationItem->groupCookie         = _GroupCookie;
            pNotificationItem->clsidSender         = _clsidSender;
            pNotificationItem->clsidDest           = _clsidDest;
            pNotificationItem->dateLastRun         = _datePrevRun;
            pNotificationItem->dateNextRun         = _dateNextRun;
        }

        return hr;
    }

    HRESULT SetNotificationItem(PNOTIFICATIONITEM pNotificationItem)
    {
        NotfAssert((pNotificationItem));
        HRESULT hr = NOERROR;

        _NotificationType = pNotificationItem->NotificationType;
        if (pNotificationItem->pNotification)
        {
            _pNotification = pNotificationItem->pNotification;
            _pNotification->AddRef();
        }
        else
        {
            _pNotification = 0;
        }

        _NotificationFlags   = pNotificationItem->NotificationFlags   ;
        _NotificationCookie  = pNotificationItem->NotificationCookie  ;
        _schdata             = pNotificationItem->TaskTrigger         ;
        _TaskData            = pNotificationItem->TaskData            ;
        _deliverMode         = pNotificationItem->DeliverMode         ;
        _GroupCookie         = pNotificationItem->groupCookie         ;
        _clsidSender         = pNotificationItem->clsidSender         ;
        _clsidDest           = pNotificationItem->clsidDest           ;
        _datePrevRun         = pNotificationItem->dateLastRun         ;
        _dateNextRun         = pNotificationItem->dateNextRun         ;

        return hr;
    }

    NOTIFICATIONTYPE GetNotificationID()
    {
        return _NotificationType;
    }
    
    PNOTIFICATIONTYPE GetNotificationType()
    {
        return &_NotificationType;
    }

    NOTIFICATIONCOOKIE & GetNotificationCookie()
    {
        return _NotificationCookie;
    }

    void GetNotificationCookie(PNOTIFICATIONCOOKIE pNotCookie)
    {
        NotfAssert((pNotCookie));
        *pNotCookie = _NotificationCookie;
    }

    NOTIFICATIONFLAGS GetNotificationFlags()
    {
        return _NotificationFlags;
    }

    REFDESTID GetDestID()
    {
        NotfAssert(( _PackageContent & PC_CLSIDDEST ));
        return _clsidDest;
    }
    
    void SetDestID(REFDESTID rclsid)
    {
        NotfAssert(( !(_PackageContent & PC_CLSIDDEST) ));
        _PackageContent |= PC_CLSIDDEST;
        _clsidDest = rclsid;
    }

    REFDESTID GetClassIdSender()
    {
        NotfAssert(( _PackageContent & PC_CLSIDSENDER ));
        return _clsidSender;
    }


    PACKAGE_CONTENT GetPackageContent()
    {
        return _PackageContent;
    }
    
    DELIVERMODE GetDeliverMode()
    {
        return _deliverMode;
    }

    void SetDeliverMode(DELIVERMODE dm)
    {
        _deliverMode = dm;
    }

    PACKAGE_TYPE GetPackageType()
    {
        return _PackageType;
    }
    BOOL IsReportRequired()
    {
        return (_deliverMode & (DM_NEED_COMPLETIONREPORT | DM_NEED_PROGRESSREPORT | DM_THROTTLE_MODE));
    }

    BOOL IsSynchronous()
    {
        return (BOOL)(_deliverMode & (DM_SYNCHRONOUS | DM_DELIVER_PREFERED));
    }
    
    BOOL IsForDefaultProcess()
    {
        return (BOOL) (_deliverMode & DM_DELIVER_DEFAULT_PROCESS);
    }
    
    BOOL IsBroadcast()
    {
        return (BOOL) (   (_clsidDest == CLSID_THREAD_BROADCAST)
                       || (_clsidDest == CLSID_PROCESS_BROADCAST)
                       || (_clsidDest == CLSID_GLOBAL_BROADCAST));
    }

    BOOL IsKnowReportNotification()
    {
        return (BOOL) (   (_NotificationType == NOTIFICATIONTYPE_TASKS_SUSPEND)
                       || (_NotificationType == NOTIFICATIONTYPE_TASKS_RESUME) 
                       || (_NotificationType == NOTIFICATIONTYPE_TASKS_ABORT) 
                       || (_NotificationType == NOTIFICATIONTYPE_TASKS_COMPLETED) 
                       || (_NotificationType == NOTIFICATIONTYPE_TASKS_PROGRESS) 
                      );
    }
    /*
    BOOL SetStateOnReportNotification(BOOL fDelivered = TRUE)
    {
        return TRUE;
    }
    */
    /*
    PACKAGE_FLAGS GetPackageFlags()
    {
        return _PackageFlags;
    }
    */
    
    PACKAGE_FLAGS GetNotificationState()
    {
        return _PackageFlags;
    }

    PACKAGE_FLAGS SetNotificationState(PACKAGE_FLAGS newState)
    {
        PACKAGE_FLAGS oldState = _PackageFlags;
        _PackageFlags = newState;
        return _PackageFlags;
    }

    BOOL IsUserIdle()
    {
        return (BOOL) (   (_NotificationType == NOTIFICATIONTYPE_USER_IDLE_BEGIN)
                       || (_NotificationType == NOTIFICATIONTYPE_USER_IDLE_END) 
                      );
    }

    BOOL IsDeliverIfConnectToInternet()
    {
        return (_TaskData.dwTaskFlags & TASK_FLAG_RUN_IF_CONNECTED_TO_INTERNET);
    }

    BOOL IsDeliverOnUserIdle()
    {
        return (_TaskData.dwTaskFlags & TASK_FLAG_START_ONLY_IF_IDLE);
    }

    BOOL IsGroupMember()
    {
        return (_PackageType == PT_GROUPMEMBER);
    }
    BOOL IsReport()
    {
        return (_PackageType == PT_REPORT_TO_SENDER) || (_PackageType == PT_REPORT_TO_DEST);
    }

    PTASK_TRIGGER  GetTaskTrigger()
    {
        return (_PackageContent & PC_TASKTRIGGER) ? &_schdata : NULL;
    }

    void SetTaskTrigger(PTASK_TRIGGER pTaskTrigger, PTASK_DATA pTaskData = 0)
    {
        NotfAssert((pTaskTrigger));
        _PackageContent |= PC_TASKTRIGGER;
        _schdata = *pTaskTrigger;
        if (pTaskData)
        {
            _PackageContent |= PC_TASKDATA;
            _TaskData = *pTaskData;
        }
    }

    PTASK_DATA  GetTaskData()
    {
        return (_PackageContent & PC_TASKDATA) ? &_TaskData : NULL;
    }
    
    DWORD   GetPriority()
    {
        return (_TaskData.dwPriority) ? _TaskData.dwPriority : NOTF_BASE_PRIORITY;
    }

    DWORD  GetTaskTriggerFlags()
    {
        return _TaskData.dwTaskFlags;
    }

    void SetNextRunDate(CFileTime& dateNextRun)
    {
        _dateNextRun = dateNextRun;
    }

    CFileTime GetNextRunDate()
    {
        return _dateNextRun;
    }

    HRESULT CalcNextRunDate()
    {
        HRESULT hr = S_OK;
        SYSTEMTIME stNow;
        
        GetLocalTime(&stNow);

        if (!_nextRunInterval)
        {
            PTASK_TRIGGER pTaskTrigger = GetTaskTrigger();

            if (pTaskTrigger)
            {
                if (pTaskTrigger->rgFlags & TASK_TRIGGER_FLAG_DISABLED)
                {
                    _dateNextRun = MAX_FILETIME;
                }
                else
                {
                    WORD wCount = 1;

                    hr = GetRunTimes(*pTaskTrigger, GetTaskData(), &stNow,  NULL, &wCount, &_dateNextRun);
                    if (!wCount)
                    {
                        SystemTimeToFileTime(&stNow, &_dateNextRun);
                        hr = E_FAIL;
                    }
                }
                SPEW_TIME((_dateNextRun, "CalcNextRunDate"));
            }
            else
            {
                hr = E_FAIL;
            }
        }
        else
        {
            _dateNextRun = _datePrevRun + _nextRunInterval;
            _nextRunInterval = 0;
            SPEW_TIME((_dateNextRun, "CalcNextRunDate using _nextRunInterval"));
        }
        
        return hr;
    }

    void UpdatePrevRunDate()
    {
        SYSTEMTIME      st;

        GetLocalTime(&st);
        SystemTimeToFileTime(&st, &_datePrevRun);

        SPEW_TIME((_datePrevRun, "UpdatePrevRunDate"));
    }

    CFileTime GetSortDate(SORTORDER soHow)
    {
        CFileTime RetDate;
                
        switch (soHow)
        {
        case SO_NEXTRUNDATE:
            RetDate = _dateNextRun;
        break;
        case SO_IDLEDATE:
            RetDate = _dateIdleRun;
        break;
        default:
            NotfAssert((FALSE));
        } // end switch
        
        return RetDate;
    }
    
    void SetSortDate(SORTORDER soHow, CFileTime newDate)
    {
        CFileTime RetDate;
        
        switch (soHow)
        {
        case SO_NEXTRUNDATE:
            _dateNextRun = newDate;
        break;
        case SO_IDLEDATE:
            _dateIdleRun = newDate;
        break;
        default:
            NotfAssert((FALSE));
        } // end switch
        
    }

    void SetNextRunInterval(__int64 nextRunInterval)
    {
        _nextRunInterval = nextRunInterval;
    }
    
    void SetPrevRunDate(CFileTime date)
    {
        _datePrevRun = date;
    }

    CFileTime GetPrevRunDate()
    {
        return _datePrevRun;
    }

    BOOL SetRunningCookie(PNOTIFICATIONCOOKIE  pRunningCookie)
    {
        if (pRunningCookie && (CLSID_NULL != * pRunningCookie)) {
            _PackageContent |= PC_RUNCOOKIE;
            _RunningCookie = *pRunningCookie;
        } else  {
            _RunningCookie = CLSID_NULL;
            _PackageContent &= ~PC_RUNCOOKIE;
        }

        return TRUE;
    }

    PNOTIFICATIONCOOKIE GetRunningCookie()
    {
        return (_PackageContent & PC_RUNCOOKIE) ? &_RunningCookie : NULL;
    }


    BOOL SetGroupCookie(PNOTIFICATIONCOOKIE  pGroupCookie)
    {
        BOOL  fRet = FALSE;
        NotfAssert((pGroupCookie));

        if (!(_PackageContent & PC_GROUPCOOKIE))
        {
            _GroupCookie = *pGroupCookie;
            _PackageContent |= PC_GROUPCOOKIE;
            fRet = TRUE;
        }

        return fRet;
    }

    PNOTIFICATIONCOOKIE GetGroupCookie()
    {
        return (_PackageContent & PC_GROUPCOOKIE) ? &_GroupCookie : NULL;
    }

    void ClearGroupCookie()
    {
        _GroupCookie = COOKIE_NULL;
        _PackageContent &= ~PC_GROUPCOOKIE;
    }
    
    BOOL SetBaseCookie(PNOTIFICATIONCOOKIE  pBaseCookie)
    {
        BOOL  fRet = FALSE;
        NotfAssert((pBaseCookie));

        if (!(_PackageContent & PC_BASECOOKIE))
        {
            _BaseCookie = *pBaseCookie;
            _PackageContent |= PC_BASECOOKIE;
            fRet = TRUE;
        }

        return fRet;
    }

    PNOTIFICATIONCOOKIE GetBaseCookie()
    {
        return (_PackageContent & PC_BASECOOKIE) ? &_BaseCookie : NULL;
    }


    // return the nofication report which is handed to the
    // NotificationSink
    // It is set up when the CPackage gets created
    LPNOTIFICATIONREPORT GetNotificationReport(BOOL fAddRef = FALSE)
    {
        if (fAddRef && _pNotfctnReport)
        {
            _pNotfctnReport->AddRef();
        }

        return _pNotfctnReport;
    }

    CNotificationReport *GetNotificationReportSender(BOOL fAddRef = FALSE)
    {
        if (fAddRef)
        {
            _CNotfctnReportSender.AddRef();
        }

        return &_CNotfctnReportSender;
    }
    CNotificationReport *GetNotificationReportDest(BOOL fAddRef = FALSE)
    {
        if (fAddRef)
        {
            _CNotfctnReportDest.AddRef();
        }

        return &_CNotfctnReportDest;
    }

    LPNOTIFICATIONSINK GetNotificationSinkDest(BOOL fAddRef = FALSE)
    {
        return _CNotfctnReportDest.GetNotificationSink(fAddRef);
    }

    LPNOTIFICATIONSINK GetNotificationSinkSender(BOOL fAddRef = FALSE)
    {
        return _CNotfctnReportSender.GetNotificationSink(fAddRef);
    }
    void SetNotificationSinkDest(LPNOTIFICATIONSINK pSink)
    {
        _CNotfctnReportDest.SetNotificationSink(pSink);
    }
    void SetNotificationSinkSender(LPNOTIFICATIONSINK pSink)
    {
        _CNotfctnReportSender.SetNotificationSink(pSink);
    }
    
    void SetNotificationReportDest()
    {
        _pNotfctnReport = &_CNotfctnReportDest;
    }

    LPNOTIFICATION GetNotification(BOOL fAddRef = FALSE)
    {
        if (fAddRef && _pNotification)
        {
            _pNotification->AddRef();
        }

        return _pNotification;
    }
    void SetNotification(LPNOTIFICATION pNotification)
    {
        if (_pNotification)
        {
            _pNotification->Release();
        }
        _pNotification = pNotification;
        if (_pNotification)
        {
            _pNotification->AddRef();
        }
    }
    void SetDestPort(HWND hWnd, DWORD dwThreadId)
    {
        _CDestPort.SetDestPort(hWnd, dwThreadId);
    }
    CDestinationPort &GetCDestPort()
    {
        return _CDestPort;
    }

    //
    // used to break refcount cycle
    ULONG GetRefCount()
    {
        return _CRefs;
    }

    // persiststream methods
    STDMETHODIMP GetClassID (CLSID *pClassID);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm,BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);


    static HRESULT CreateDeliver(
                         LPCLSID             pClsidSender,      // class of sender
                         LPNOTIFICATIONSINK  pNotfctnSink,      // can be null - see mode
                         LPNOTIFICATION      pNotification,
                         DELIVERMODE         deliverMode,
                         PNOTIFICATIONCOOKIE pGroupCookie,
                         PTASK_TRIGGER       pTaskTrigger,
                         PTASK_DATA          pTaskData,
                         REFDESTID           rNotificationDest,
                         CPackage          **ppCPackage,
                         PACKAGE_FLAGS       packageflags
                         );

    static HRESULT CreateUpdate(
                                CNotificationReport    *pNotfctnRprt,
                                // the notification object itself
                                LPNOTIFICATION          pNotification,
                                // the cookie of the object the notification is targeted too
                                DELIVERMODE             deliverMode,
                                DWORD                   dwReserved,
                                CPackage              **ppCPackage
                                );

    static HRESULT CreateReport(
                               // the notification object itself
                               LPNOTIFICATION          pNotification,
                               // the cookie of the object the notification is targeted too
                               PNOTIFICATIONCOOKIE     pRunningNotfCookie,
                               DWORD                   dwReserved,
                               CPackage              **ppCPackage
                               );

    ULONG SetIndex(ULONG index)
    {
        ULONG i = _index;
        _index = index;
        return i;
    }
    ULONG GetIndex()
    {
        return _index;
    }

    void SetCrossProcessId(DWORD dwParam)
    {
        _dwParam = dwParam;
    }
    
    LPCWSTR NotfCookieStr()
    {

        if (!_pwzNotfCookie)
        {
            StringFromCLSID(_NotificationCookie, &_pwzNotfCookie);
        }
        return (LPCWSTR)_pwzNotfCookie;
    }

    LPCWSTR NotfTypeStr()
    {

        if (!_pwzNotfType)
        {
            StringFromCLSID(_NotificationType, &_pwzNotfType);
        }
        return (LPCWSTR)_pwzNotfType;
    }


    ~CPackage();

    // dispatch helper
    HRESULT PreDispatch();
    HRESULT PostDispatch();
    HRESULT PreDeliver();
    VOID SetHResultDest(HRESULT hr)
    {
        _hrDest = hr;
    }
    
    // persist helper
    HRESULT SaveToPersist(LPCSTR pszWhere,
                         LPSTR pszSubKey = 0,   // default is pakage cookie
                         DWORD dwMode = 0);
    HRESULT RemovePersist(LPCSTR pszWhere,
                          LPSTR pszSubKey = 0,  // default is pakage cookie
                          DWORD dwMode = 0);
    static HRESULT LoadPersistedPackage(LPCSTR pszWhere,
                                   PNOTIFICATIONCOOKIE     pNotfCookie,
                                   DWORD dwMode,
                                   CPackage  **ppCPackage);
    static HRESULT LoadFromPersist(LPCSTR pszWhere,
                                   LPSTR pszSubKey,
                                   DWORD dwMode,
                                   CPackage  **ppCPackage);
    static HRESULT IsPersisted(LPCSTR pszWhere,PNOTIFICATIONCOOKIE pNotfCookie);

    HRESULT IsPersisted(LPCSTR pszWhere,
                        LPSTR pszSubKey = 0);


    ULONG GetRefs() { return _CRefs; };
#if DBG == 1
    void Dump(DWORD dwFlags, const char *pszPrefix, HRESULT hr = 0xffffffff);
#endif

private:
    // used to create a normal deliver package
    CPackage(
             LPCLSID             pClsidSender,      // class of sender
             LPNOTIFICATIONSINK  pNotfctnSink,      // can be null - see mode
             LPNOTIFICATION      pNotification,
             DELIVERMODE         deliverMode,
             PNOTIFICATIONCOOKIE pGroupCookie,
             PTASK_TRIGGER       pschdata,
             PTASK_DATA          pTaskData,
             LPCLSID             pClsidDest
             );

    // used to create report packages
    CPackage(
             CNotificationReport *pCNotfRprtTo,
             LPNOTIFICATION       pNotification,
             DELIVERMODE          deliverMode
             );

    // create an empty package - LoadFromPerist will be called
    CPackage(DWORD dwWhere);
    
    #if 0
    // need to finish package validation for debug
    BOOL Validate()
    {
        BOOL fRet = FALSE;
        do
        {
            if (!GetNotification())
            {
                NotfAssert((FALSE));
                break;
            }

            if ( ((CNotfctnObj *)_pNotification)->GetCPackage() != this )
            {
                NotfAssert((FALSE));
                break;
            }
            
            break;
        } while (TRUE);
        
    }
    #endif // 0

private:
    CRefCount               _CRefs;        // the total refcount of this object

    DELIVERMODE             _deliverMode;
    // only for enumeration here
    CLSID                   _clsidSender;
    // the package
    PNOTIFICATIONITEM       _pNotificationItem;
    // deliver and schedule data
    TASK_TRIGGER            _schdata;
    TASK_DATA               _TaskData;
    // destination
    CLSID                   _clsidDest;
    // return info - cookie of package
    NOTIFICATIONCOOKIE      _NotificationCookie;
    DWORD                   _dwReserved;
    LPNOTIFICATION          _pNotification;
    NOTIFICATIONTYPE        _NotificationType;
    NOTIFICATIONFLAGS       _NotificationFlags;

    // running info
    CFileTime               _dateNextRun;
    CFileTime               _datePrevRun;
    // idle info
    CFileTime               _dateIdleRun;
    __int64                 _nextRunInterval;

    // info about the running cookie etc.
    NOTIFICATIONCOOKIE      _RunningCookie;
    NOTIFICATIONCOOKIE      _GroupCookie;
    NOTIFICATIONCOOKIE      _BaseCookie;

    // the notification item handed out for enumeration etc.
    // more work here!
    NOTIFICATIONITEM        _NotificationItem;

    // package information
    PACKAGE_TYPE            _PackageType;
    PACKAGE_FLAGS           _PackageFlags;
    PACKAGE_CONTENT         _PackageContent;
    ULONG                   _index;

    DWORD                   _dwParam;       // used for cross process packages
    HRESULT                 _hrDest;        // hresult from desitantion notification sink

//public:
    //
    // notification report object - for progress and completion report
    //
    CDestinationPort         _CDestPort;
    // the NotificationReport the destination get
    //LPNOTIFICATIONREPORT     _pNotfctnReportDest;
    CNotificationReport      _CNotfctnReportDest;

    // the NotificatnReport the sender gets when creating the first
    //LPNOTIFICATIONREPORT     _pNotfctnReportSender;
    CNotificationReport      _CNotfctnReportSender;

    // the notificationreport passed on in OnNotification
    LPNOTIFICATIONREPORT     _pNotfctnReport;

    CRefCount               _CRefsReportDest;        
    CRefCount               _CRefsReportSender;        

    LPWSTR                  _pwzNotfCookie;
    LPWSTR                  _pwzNotfType;
    
};  // end CPackage

typedef struct _tagSaveSTATPROPMAP
{
    DWORD           cbSize;
    DWORD           cbStrLen;
    DWORD           dwFlags;
    DWORD           cbVarSizeExtra;
} SaveSTATPROPMAP;


#define BREAK_ONERROR(hrBreak) if (hrBreak != NOERROR) { break; }

#if DBG == 1

inline ULONG DebugAddRef(IUnknown *pUnk, char *pszFile, long line)
{
    ULONG refs = pUnk->AddRef();
    TNotfDebugOut((DEB_TMEMORY, "CPackage %p AddRef [%d] in %s at %d\n", pUnk, refs, pszFile, line));
    return refs;
}

inline ULONG DebugRelease(IUnknown *pUnk, char *pszFile, long line)
{
    ULONG refs = pUnk->Release();
    TNotfDebugOut((DEB_TMEMORY, "CPackage %p Release [%d] in %s at %d\n", pUnk, refs, pszFile, line));
    return refs;
}

#define ADDREF(p)   DebugAddRef(p, __FILE__, __LINE__)
#define RELEASE(p)  DebugRelease(p, __FILE__, __LINE__)

#else  //DBG

#define ADDREF(p)   p->AddRef()
#define RELEASE(p)  p->Release()
    
#endif //DBG
#endif // _PACKAGE_HXX_

 

