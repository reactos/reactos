//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       schgroup.HXX
//
//  Contents:   class and definitions for schedule groups
//
//  Classes:
//
//  Functions:
//
//  History:    1-23-1997   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------

#ifndef _SCHEDGROUP_HXX_
#define _SCHEDGROUP_HXX_

#if 0
typedef struct _tagGROUPDATA
{
    // new  task trigger
    TASK_TRIGGER        _Tasktrigger;
    // the flags which go with the TaskTrigger
    DWORD               _grfTasktrigger;
    // new group cookie if sequential
    NOTIFICATIONCOOKIE  _GroupCookie;
    // sequential or parallel
    GROUPMODE           _grfGroupMode;
    // number of elements in group
    LONG                _Elements;

} GROUPDATA;

class CGroupPackage : public CLifePtr
{
public:

    //CGroupPackage(GROUPMODE grfGroupMode)
    CGroupPackage(DWORD grfCreateFlags)
    {
    }

    ~CGroupPackage()
    {
    }

    HRESULT SetAttributes(
                          PTASK_TRIGGER       pTaskTrigger,
                          PTASK_DATA          pTaskData,
                          PNOTIFICATIONCOOKIE pGroupCookie,
                          GROUPMODE           grfGroupMode
                         )
    {
        HRESULT hr = NOERROR;

        if (pTaskTrigger)
        {
            _Tasktrigger    = *pTaskTrigger;
        }

        _grfTasktrigger  = grfTasktrigger;

        if (pGroupCookie)
        {
            _GroupCookie     = *pGroupCookie;
        }

        _grfGroupMode    = grfGroupMode;

        return hr;
    }
    HRESULT GetAttributes(
        PTASK_TRIGGER       pTaskTrigger,
        PTASK_DATA          pTaskData,
        PNOTIFICATIONCOOKIE pGroupCookie,
        GROUPMODE          *pgrfGroupMode,
        LONG               *pElements
        )
    {
        HRESULT hr = NOERROR;

        if (pTaskTrigger)
        {
            *pTaskTrigger = _Tasktrigger;
        }

        if (pgrfTasktrigger)
        {
            *pgrfTasktrigger  = _grfTasktrigger;
        }

        if (pGroupCookie)
        {
            *pGroupCookie     = _GroupCookie;
        }
        if (pgrfGroupMode)
        {
            *pgrfGroupMode    = _grfGroupMode;
        }

        return hr;
    }

private:
    // new  task trigger
    TASK_TRIGGER        _Tasktrigger;
    // the flags which go with the TaskTrigger
    DWORD               _grfTasktrigger;
    // new group cookie if sequential
    NOTIFICATIONCOOKIE  _GroupCookie;
    // sequential or parallel
    GROUPMODE           _grfGroupMode;
    // number of elements in group
    // LONG                _Elements;

};
#endif // _foo_

typedef enum
{
     GS_Created     = 0
    ,GS_Running     = 1
    ,GS_Initialized = 2


} GROUP_STATE;

typedef enum
{
     GT_NORMAL     = 0x00000001
    ,GT_STATIC     = 0x00000002

} _GROUP_TYPE;

typedef DWORD GROUP_TYPE;

typedef struct _tagSCHEDULEGROUPITEM
{
    ULONG               cbSize;
    ULONG               cElements;     // the # of packages in the group
    NOTIFICATIONCOOKIE  GroupCookie;
    GROUPMODE           grfGroupMode;
    GROUP_STATE         grpState;
    TASK_TRIGGER        TaskTrigger;
    TASK_DATA           TaskData;
    GROUP_TYPE          GroupType;
    GROUPINFO           GroupInfo;
    

} SCHEDULEGROUPITEM, *PSCHEDULEGROUPITEM;


class CScheduleGroup: public IScheduleGroup // , public CCourierAgent, public CObject
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    // change the group attributes such as task trigger
    // name and mode - see mode for parallel and sequential
    STDMETHODIMP SetAttributes(
        // new  task trigger
        PTASK_TRIGGER       pTaskTrigger,
        // the flags which go with the TaskTrigger
        PTASK_DATA          pTaskData,
        // new group cookie if sequential
        PNOTIFICATIONCOOKIE pGroupCookie,
        // group info 
        PGROUPINFO          pGroupInfo,
        // sequential or parallel
        GROUPMODE           grfGroupMode
        );

    STDMETHODIMP GetAttributes(
        // new  task trigger
        PTASK_TRIGGER       pTaskTrigger,
        // the flags which go with the TaskTrigger
        PTASK_DATA          pTaskData,
        // new group cookie if sequential
        PNOTIFICATIONCOOKIE pGroupCookie,
        // group info 
        PGROUPINFO          pGroupInfo,
        // sequential or parallel
        GROUPMODE          *pgrfGroupMode,
        // number of elements in group
        LONG               *pElements
        );

    // add notification to this group
    STDMETHODIMP AddNotification(
                    // the notificationid and object
        LPNOTIFICATION      pNotification,
                    // destination
        REFCLSID            rNotificationDest,
                    // deliver mode - group and schedule data
        DELIVERMODE         deliverMode,
                    // info about who the sender
        LPCLSID             pClsidSender,
                    // class of sender can be NULL
        LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                    //the cookie of the new notification
        LPNOTIFICATIONREPORT *ppNotfctnReport,
                    //the cookie of the new notification
        PNOTIFICATIONCOOKIE pNotificationCookie,
                    // reserved dword
        PTASK_DATA          pTaskData
        );

    // find a notification in the group
    STDMETHODIMP FindNotification(
        // the notification cookie
        PNOTIFICATIONCOOKIE pNotificatioCookie,
        // the new object just created
        PNOTIFICATIONITEM   pNotificationItem,
        DWORD               dwReserved
        );

    // only for completness
    STDMETHODIMP RevokeNotification(
        PNOTIFICATIONCOOKIE      pnotificationCookie,
        PNOTIFICATIONITEM        pschedulNotification,
        DWORD                    dwReserved
        );

    // an enumerator over the items in this group
    STDMETHODIMP GetEnumNotification(
        // flags which items to enumerate
        DWORD                    grfFlags,
        LPENUMNOTIFICATION     *ppEnumNotification
        );


    // persiststream methods
    STDMETHODIMP GetClassID (CLSID *pClassID);
    STDMETHODIMP IsDirty(void);
    STDMETHODIMP Load(IStream *pStm);
    STDMETHODIMP Save(IStream *pStm,BOOL fClearDirty);
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER *pcbSize);

    HRESULT SaveToPersist(LPCSTR pszWhere, 
                          LPSTR pszSubKey = 0,   // default is pakage cookie
                          DWORD dwMode = 0);
    HRESULT RemovePersist(LPCSTR pszWhere, 
                          LPSTR pszSubKey = 0,  // default is pakage cookie
                          DWORD dwMode = 0);
    static HRESULT LoadFromPersist(LPCSTR pszWhere,
                                   LPSTR pszSubKey,
                                   DWORD dwMode,
                                   CScheduleGroup  **ppCScheduleGroup);

    static HRESULT LoadPersistedGroup(
                       LPCSTR                pszWhere,
                       PNOTIFICATIONCOOKIE  pNotfCookie,
                       DWORD                dwMode,
                       CScheduleGroup **ppCScheduleGroup);


    static HRESULT UnPersistScheduleGroups(
                        LPCSTR pszWhere,
                        LPSTR pszSubKeyIn,
                        DWORD dwMode,
                        CScheduleGroup **prgCSchdGrp,
                        ULONG cElements,
                        ULONG *pcElementFilled);
                       

    // private methods
    //

    HRESULT GetScheduleGroupItem(PSCHEDULEGROUPITEM pScheduleGroupItem, BOOL fGetGroupInfo)
    {
        NotfAssert((pScheduleGroupItem));
        HRESULT hr = NOERROR;
        NotfAssert((    pScheduleGroupItem
                     && (pScheduleGroupItem->cbSize >= sizeof(SCHEDULEGROUPITEM)) ));

        if (   !pScheduleGroupItem
            || (pScheduleGroupItem->cbSize < sizeof(SCHEDULEGROUPITEM))
           )
        {
            hr = E_INVALIDARG;
        }
        else
        {
            memset(pScheduleGroupItem, 0, sizeof(SCHEDULEGROUPITEM));
            pScheduleGroupItem->cbSize      = sizeof(SCHEDULEGROUPITEM);
            pScheduleGroupItem->cElements   = _CElements;
            pScheduleGroupItem->GroupCookie = _GroupCookie; 
            pScheduleGroupItem->grfGroupMode= _grfGroupMode; 
            pScheduleGroupItem->grpState    = _grpState; 
            pScheduleGroupItem->TaskTrigger = _TaskTrigger;
            pScheduleGroupItem->TaskData    = _TaskData;
            pScheduleGroupItem->GroupType   = _GroupType;
            if (fGetGroupInfo)
            {
                pScheduleGroupItem->GroupInfo   = _GroupInfo;
            }

        }

        return hr;
    }

    HRESULT SetScheduleGroupItem(PSCHEDULEGROUPITEM pScheduleGroupItem, BOOL fSetGroupInfo)
    {
        NotfAssert((pScheduleGroupItem));
        HRESULT hr = NOERROR;
        _CElements    = pScheduleGroupItem->cElements;    
        _GroupCookie  = pScheduleGroupItem->GroupCookie;
        _CGroupCookie = _GroupCookie;  
        _grfGroupMode = pScheduleGroupItem->grfGroupMode;
        _grpState     = pScheduleGroupItem->grpState;
        _TaskTrigger  = pScheduleGroupItem->TaskTrigger;
        _TaskData     = pScheduleGroupItem->TaskData;
        _GroupType    = pScheduleGroupItem->GroupType;
        if (fSetGroupInfo)
        {
            _GroupInfo    = pScheduleGroupItem->GroupInfo;
        }
        return hr;
    }

    
    GROUP_STATE GetGroupState()
    {
        return _grpState;
    }
    void SetGroupState(GROUP_STATE gs)
    {
        _grpState = gs;
    }

    BOOL IsRunning()
    {
        return _grpState == GS_Running;
    }
    BOOL IsInitialized()
    {
        return (_grpState == GS_Initialized) || (_grpState == GS_Running);
    }

    BOOL IsStaticGroup()
    {
        return (_GroupType & GT_STATIC);
    }

    static HRESULT Create(DWORD           grfGroupCreateFlags,
                      // the new group
                      CScheduleGroup    **ppCSchedGroup,
                      // the cookie of the group
                      PNOTIFICATIONCOOKIE pGroupCookie,
                      DWORD dwReserved = 0);
    
    static HRESULT CreateForEnum(
                      CPackage  *pCPackage,
                      // the new group
                      CScheduleGroup    **ppCSchedGroup,
                      DWORD dwReserved = 0);

    static HRESULT CreateDefScheduleGroup(REFNOTIFICATIONCOOKIE rGroupCookie,
                                          PTASK_TRIGGER pTaskTrigger,LPCWSTR pwzName = 0);
                      
    HRESULT ChangeAttributes(
                          PTASK_TRIGGER       pTaskTrigger,
                          PTASK_DATA          pTaskData,
                          PNOTIFICATIONCOOKIE pGroupCookie,
                          PGROUPINFO          pGroupInfo,
                          GROUPMODE           grfGroupMode
                         );


    HRESULT InitAttributes(
                          PTASK_TRIGGER       pTaskTrigger,
                          PTASK_DATA          pTaskData,
                          PNOTIFICATIONCOOKIE pGroupCookie,
                          PGROUPINFO          pGroupInfo,
                          GROUPMODE           grfGroupMode
                         );

    HRESULT RetrieveAttributes(
        PTASK_TRIGGER       pTaskTrigger,
        PTASK_DATA          pTaskData,
        PNOTIFICATIONCOOKIE pGroupCookie,
        PGROUPINFO          pGroupInfo,
        GROUPMODE          *pgrfGroupMode,
        LONG               *pElements
        );

    NOTIFICATIONCOOKIE  GetGroupCookie()
    {
        return _GroupCookie;
    }

    ~CScheduleGroup()
    {
        delete [] _GroupInfo.pwzGroupname;
    }

    CScheduleGroup(GROUP_TYPE grpType = GT_NORMAL)
    : _CRefs()
    , _CElements(0)
    , _CGroupCookie()
    ,_SchedListAgent( *g_pCSchedListAgent)
    {
        _pPackage = 0;
        _dateLastRun = 0;
        _pPackage = 0;
        _TaskTrigger.TriggerType = TASK_TIME_TRIGGER_ONCE;
        _fDirty = FALSE;
        _GroupInfo.cbSize = sizeof(GROUPINFO);
        _GroupInfo.pwzGroupname = 0;
        _GroupType = grpType;
    }

private:

    CScheduleGroup(DWORD                rfGroupCreateFlags,
                   PNOTIFICATIONCOOKIE  pGroupCookie,
                   PTASK_TRIGGER         pTaskTrigger = 0)
    : _CRefs()
    , _CElements(0)
    , _CGroupCookie()
    ,_SchedListAgent( *g_pCSchedListAgent)
    {
        _pPackage = 0;
        _dateLastRun = 0;
        _pPackage = 0;
        if (pTaskTrigger)
        {
            _TaskTrigger = *pTaskTrigger;
            _TaskTrigger.TriggerType = (_TASK_TRIGGER_TYPE)TASK_TRIGGER_FLAG_DISABLED;
            _grpState = GS_Initialized;
        }
        else
        {
            _TaskTrigger.TriggerType = TASK_TIME_TRIGGER_ONCE;
            _grpState = GS_Created;

        }
        _fDirty = FALSE;
        _GroupInfo.cbSize = sizeof(GROUPINFO);
        _GroupInfo.pwzGroupname = 0;
        _GroupType = GT_NORMAL;
    }
    
    CScheduleGroup(PNOTIFICATIONCOOKIE pGroupCookie, GROUP_STATE gs)
    : _CRefs()
    , _CElements(0)
    ,_SchedListAgent( *g_pCSchedListAgent)
    {
        NotfAssert((pGroupCookie));
        _CGroupCookie = *pGroupCookie;
        _GroupCookie =  *pGroupCookie;
        _pPackage = 0;
        _dateLastRun = 0;
        _pPackage = 0;
        _TaskTrigger.TriggerType = TASK_TIME_TRIGGER_ONCE;
        _grpState = gs;
        _fDirty = FALSE;
        _GroupInfo.cbSize = sizeof(GROUPINFO);
        _GroupInfo.pwzGroupname = 0;
        _GroupType = GT_NORMAL;
     }
     

private:
    CRefCount           _CRefs;         // the total refcount of this object
    CRefCount           _CElements;     // the # of packages in the group
    CPkgCookie          _CGroupCookie;  //
    CPackage           *_pPackage;      // pointer to the package which is gets scheduled
    CMutexSem           _mxs;           // single access to the timer
    CSchedListAgent    &_SchedListAgent;

    // state information
    GROUP_STATE         _grpState;
    GROUP_TYPE          _GroupType;

    // run information
    CFileTime           _dateLastRun;

    // group information and attributes
        // new  task trigger
    TASK_TRIGGER        _TaskTrigger;
    // the flags which go with the TaskTrigger
    TASK_DATA           _TaskData;
    // new group cookie if sequential
    NOTIFICATIONCOOKIE  _GroupCookie;
    // sequential or parallel
    GROUPMODE           _grfGroupMode;
    // group info
    GROUPINFO           _GroupInfo;
    // number of elements in group
    // LONG                _Elements;
    NOTIFICATIONCOOKIE  _GroupLeader;
    BOOL                _fDirty;

};

class CEnumSchedGroup : public IEnumScheduleGroup
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP Next(ULONG celt,LPSCHEDULEGROUP *rgelt,ULONG *pceltFetched);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumScheduleGroup **ppenum);

    static HRESULT Create(CSchedListAgent *pListAgent,ENUM_FLAGS dwMode, CEnumSchedGroup **ppCEnum);


private:
    #define cElementsMax 256
    CEnumSchedGroup(CSchedListAgent *pListAgent, ENUM_FLAGS dwMode)
    :  _CRefs()
    , _cPos(0)
    , _posCookie(COOKIE_NULL)
    {
        NotfAssert((pListAgent));
        _pListAgent = pListAgent;
        if (_pListAgent)
        {
            _pListAgent->AddRef();
        }
        _dwMode = dwMode;
        _cElements = cElementsMax;
        _cElementsFilled = 0;
    }

    void SetNewListAgent(CSchedListAgent *pListAgent)
    {
        NotfAssert((pListAgent));
        if (_pListAgent)
        {
            _pListAgent->Release();
        }
        _pListAgent = pListAgent;
        if (_pListAgent)
        {
            _pListAgent->AddRef();
        }
    }

public:
    ~CEnumSchedGroup()
    {
        if (_pListAgent)
        {
            _pListAgent->Release();
        }
        FreeSchedGroups();
    }

private:
    void FreeSchedGroups()
    {
        for (ULONG i = 0; i < _cElementsFilled; i++)
        {
            if (_prgCSchedGroups[i])
            {
                _prgCSchedGroups[i]->Release();
                _prgCSchedGroups[i] = 0;
            }
        }
        _cElementsFilled = 0;
    }
    
    CRefCount           _CRefs;        // the total refcount of this object
    ULONG               _cPos;         // the current position
    CSchedListAgent    *_pListAgent;   // the object we get the packages
    NOTIFICATIONCOOKIE  _posCookie;    // the internal position for the map
        
    ENUM_FLAGS          _dwMode;
public:    
    CScheduleGroup     *_prgCSchedGroups[cElementsMax];
    ULONG               _cElements;
    ULONG               _cElementsFilled;
};

//
//
//
BOOL IsPredefinedScheduleGroup(REFNOTIFICATIONCOOKIE rGroupCookie);

#endif // _SCHEDGROUP_HXX_

