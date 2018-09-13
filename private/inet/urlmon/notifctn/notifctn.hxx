//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       notifctn.hxx
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
#ifndef _NOTIFCTN_HXX_
#define _NOTIFCTN_HXX_

#include <stdio.h>
#include <msterr.h>
#include "smmutex.hxx"
#define _with_internal_notificationprocessmgr_
#include "notftn.h"
#include "notfstr.h"
#define SCHED_S_EVENT_TRIGGER            ((HRESULT)0x00041308L)
#define SCHED_S_TASK_NO_VALID_TRIGGERS   ((HRESULT)0x00041307L)
#define SCHED_S_TASK_NO_MORE_RUNS        ((HRESULT)0x00041304L)
#include "xcookie.h"
#ifndef unix
#include "..\utils\coll.hxx"
#include "..\utils\afxtempl.h"
#else
#include "../utils/coll.hxx"
#include "../utils/afxtempl.h"
#endif /* unix */
#include <delaydll.h>
#include "filetime.h"

#define HKEY_READ_WRITE_ACCESS (KEY_READ | KEY_WRITE)

#   undef DEB_ASYNCAPIS   
#   undef DEB_URLMON      
#   undef DEB_ISTREAM     
//#   undef DEB_DLL         
#   undef DEB_FORMAT      
#   undef DEB_CODEDL      
#   undef DEB_BINDING       
#   undef DEB_TRANS         
#   undef DEB_TRANSPACKET   
#   undef DEB_DATA          
#   undef DEB_TRANSMGR      
#   undef DEB_SESSION       
#   undef DEB_PROT       
#   undef DEB_PROTHTTP   
#   undef DEB_PROTFTP    
#   undef DEB_PROTGOPHER 
#   undef DEB_PROTSIMP   

#   define DEB_PROPMAP          DEB_NOTF_1
#   define DEB_MGRGLOBAL        DEB_NOTF_2
#   define DEB_DELIVER          DEB_NOTF_3
#   define DEB_NOTFENUM         DEB_NOTF_4
#   define DEB_DEST             DEB_NOTF_5
#   define DEB_PACKAGE          DEB_NOTF_6
#   define DEB_SCHEDLST         DEB_NOTF_7
#   define DEB_CMAPX            DEB_NOTF_8

// none chatty flags
#   define DEB_MGR              DEB_NOTF_9
#   define DEB_SCHEDGRP         DEB_NOTF_10
#   define DEB_ENUM             DEB_NOTF_11
#   define DEB_REPORT           DEB_NOTF_12
#   define DEB_NOTFOBJ          DEB_NOTF_13
#   define DEB_NOTFX14          DEB_NOTF_14 
#   define DEB_REFCOUNT         DEB_NOTF_15

//  TNOTF flags
#   define DEB_TPKGDETAIL       DEB_TNOTF_1
#   define DEB_TLISTS           DEB_TNOTF_2
#   define DEB_TMEMORY          DEB_TNOTF_3
#   define DEB_TPERSIST         DEB_TNOTF_4
#   define DEB_TFLOW            DEB_TNOTF_5
#   define DEB_TSCHEDULE        DEB_TNOTF_6
#   define DEB_TUNUSED7         DEB_TNOTF_7
#   define DEB_TUNUSED8         DEB_TNOTF_8

#   define DEB_TUNUSED9         DEB_TNOTF_9
#   define DEB_TUNUSED10        DEB_TNOTF_10
#   define DEB_TUNUSED11        DEB_TNOTF_11
#   define DEB_TUNUSED12        DEB_TNOTF_12
#   define DEB_TUNUSED13        DEB_TNOTF_13
#   define DEB_TUNUSED14        DEB_TNOTF_14 
#   define DEB_TUNUSED15        DEB_TNOTF_15
#   define DEB_TALL             (DEB_TNOTF_1  | DEB_TNOTF_2  | DEB_TNOTF_3  | DEB_TNOTF_4  | \
                                 DEB_TNOTF_5  | DEB_TNOTF_6  | DEB_TNOTF_7  | DEB_TNOTF_8  | \
                                 DEB_TNOTF_9  | DEB_TNOTF_10 | DEB_TNOTF_11 | DEB_TNOTF_12 )

// the global registry key - the root
extern const CHAR c_pszRegKeyRoot[];
// the current registry key string
//extern const CHAR c_pszRegKey[];
extern const CHAR *c_pszRegKey;
// the destination key
extern const CHAR c_pszRegKeyDestination[];
extern const CHAR c_pszRegKeyPackage[];
extern const CHAR c_pszRegKeyRunningDest[];
extern const CHAR c_pszRegKeyRunningItem[];
extern const CHAR c_pszRegKeyScheduleGroup[];
extern const CHAR c_pszRegKeyPackageDelete[];
extern const CHAR c_pszRegKeyRunningDestDelete[];

#define SZREGSETTING_MAX    1024
#define SZREGVALUE_MAX      256

#define MAX_NOTF_FORMAT_STRING_LENGTH 512

typedef REFGUID REFNOTIFICATIONCOOKIE;

#ifdef unix
#undef offsetof
#endif /* unix */
#define offsetof(s,m) (size_t)&(((s *)0)->m)

#define NOTFMGR_E_NOTIFICATION_NOT_FOUND          _HRESULT_TYPEDEF_(0x800C0101L)      

#define NOTFSINKMODE_ALL                            \
        (                                           \
           NM_PERMANENT                             \
         | NM_ACCEPT_DIRECTED_NOTIFICATION          \
        )

#define DELIVERMODE_ALL                             \
        (                                           \
           DM_DELIVER_PREFERED                      \
         | DM_DELIVER_DELAYED                       \
         | DM_DELIVER_LAST_DELAYED                  \
         | DM_SYNCHRONOUS                           \
         | DM_ONLY_IF_RUNNING                       \
         | DM_THROTTLE_MODE                         \
         | DM_NEED_COMPLETIONREPORT                 \
         | DM_NEED_PROGRESSREPORT                   \
         | DM_ONLY_IF_NOT_PENDING                   \
         | DM_DELIVER_DEFAULT_THREAD                \
         | DM_DELIVER_DEFAULT_PROCESS               \
        )
        
#define ENUM_FLAGS_ALL                              \
        (                                           \
           EF_NOT_NOTIFICATION                      \
         | EF_NOT_SCHEDULEGROUPITEM                 \
         | EF_NOTIFICATION_INPROGRESS               \
        )


//+----------------------------------------------------------------------------
//
//      Macro:
//              GETPPARENT
//
//      Synopsis:
//              Given a pointer to something contained by a struct (or
//              class,) the type name of the containing struct (or class),
//              and the name of the member being pointed to, return a pointer
//              to the container.
//
//      Arguments:
//              [pmemb] -- pointer to member of struct (or class.)
//              [struc] -- type name of containing struct (or class.)
//              [membname] - name of member within the struct (or class.)
//
//      Returns:
//              pointer to containing struct (or class)
//
//      Notes:
//              Assumes all pointers are FAR.
//
//      History:
//
//-----------------------------------------------------------------------------
#define GETPPARENT(pmemb, struc, membname) (\
                (struc FAR *)(((char FAR *)(pmemb))-offsetof(struc, membname)))


#define LPDESTID    LPCLSID
#define DESTID      CLSID
#define REFDESTID   REFCLSID
#define COOKIE_NULL CLSID_NULL



typedef DWORD THREADID;
class CThreadId
{
public:
    CThreadId()
    {
        _ThreadId = GetCurrentThreadId();
    }
    CThreadId(THREADID ThreadId)
    {
        _ThreadId = ThreadId;
    }
    THREADID GetThreadId()
    {
        return _ThreadId;
    }
    void SetThreadId(THREADID ThreadId)
    {
        _ThreadId = ThreadId;
    }

    operator THREADID()
    {
        return (THREADID) _ThreadId;
    }
    BOOL IsApartmentThread()
    {
       NotfAssert((_ThreadId != 0));
       return (_ThreadId == GetCurrentThreadId());
    }

private:
    THREADID _ThreadId;
};

class CThreadPacket : public CLifePtr
{
public:
    CThreadPacket() : CLifePtr()
    {}
    virtual ~CThreadPacket()
    {}

    virtual LRESULT OnPacket(UINT msg, DWORD dwParam) = 0;
    virtual HRESULT Deliver(HWND hwnd, DWORD dwMode) = 0;
};


class CKey
{
public:
    CKey()
    {
        _szStr[0] = 0;
        _pszStr = 0;
    }
    CKey(LPCWSTR pwzStr)
    {
        W2A(pwzStr, _szStr, SZMIMESIZE_MAX);
        _pszStr = _szStr;

    }

    operator LPCSTR ()
    {
        return _pszStr;
    }


    ~CKey()
    {
    }

private:
    LPCSTR  _pszStr;
    char    _szStr[SZMIMESIZE_MAX];
};

class CNodeObject : public CObject
{
    public:
        CNodeObject(CObject *pCVal) : _cElements(1)
        {
            _Map.SetAt(pCVal, pCVal);
        }

        BOOL Add(CObject *pCVal)
        {
            _cElements++;
            _Map.SetAt(pCVal, pCVal);
            NotfAssert((_cElements == _Map.GetCount() ));
            return TRUE;
        }

        BOOL Remove(CObject *pCVal)
        {
            CObject *pCValVal = 0;

            if (_Map.Lookup(pCVal, (void *&)pCValVal) )
            {
                _Map.RemoveKey(pCVal);
                _cElements--;
            }
            else
            {
                NotfAssert((FALSE));
            }

            NotfAssert((_cElements == _Map.GetCount() ));
            return (_cElements) ? TRUE : FALSE;
        }


        BOOL FindFirst(CObject *& prUnk)
        {
            BOOL fRet = FALSE;
            prUnk = 0;

            _pos = _Map.GetStartPosition();

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (VOID *&)dwKey, (void *&) prUnk);
                if (prUnk)
                {
                    fRet = TRUE;
                }
            }

            return fRet;
        }

        BOOL FindNext(CObject *& prUnk)
        {
            BOOL fRet = FALSE;
            prUnk = 0;

            if (_pos)
            {
                DWORD dwKey;
                _Map.GetNextAssoc(_pos, (VOID *&)dwKey, (void *&) prUnk);
                if (prUnk)
                {
                    fRet = TRUE;
                }
            }

            return fRet;
        }



        ~CNodeObject()
        {
            NotfAssert((_cElements == _Map.GetCount() ));
            NotfAssert((_cElements == 0));
        }

    private:
        CRefCount       _cElements;
        CMap<LPVOID , LPVOID, LPVOID , LPVOID> _Map;
        POSITION        _pos;
};


template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CMapCKeyToCValue
{
public:
    STDMETHODIMP AddVal     (ARG_KEY key, ARG_VALUE newValue)
    {
        NotfDebugOut((DEB_CMAPX, "%p _IN CMapCKeyToCValue::\n", this));
        HRESULT hr = NOERROR;

        CLock lck(_mxs);

        CNodeObject *pNode;

        if (_Map.Lookup(key, (CObject *&)pNode) )
        {
            pNode->Add(newValue);
        }
        else
        {
           pNode = new CNodeObject(newValue);
           if (pNode)
           {
               _Map.SetAt(key, pNode);
               _cElements++;
           }
           else
           {
               hr = E_OUTOFMEMORY;
           }
        }

        NotfAssert((_cElements == _Map.GetCount() ));

        NotfDebugOut((DEB_CMAPX, "%p OUT CMapCKeyToCValue:: (hr:%lx)\n",this, hr));
        return hr;
    }

    STDMETHODIMP RemoveVal  (ARG_KEY key, ARG_VALUE newValue)
    {
        NotfDebugOut((DEB_CMAPX, "%p _IN CMapCKeyToCValue::\n", this));
        HRESULT hr = E_FAIL;

        CLock lck(_mxs);

        CNodeObject *pNode;

        if (_Map.Lookup(key, (CObject *&)pNode) )
        {
            if (pNode->Remove(newValue) == FALSE)
            {
                _Map.RemoveKey(key);
                _cElements--;
            }

            hr = S_OK;
        }

        NotfAssert((_cElements == _Map.GetCount() ));

        NotfDebugOut((DEB_CMAPX, "%p OUT CMapCKeyToCValue:: (hr:%lx)\n",this, hr));
        return hr;
    }


    STDMETHODIMP FindFirst  (KEY& rKey, VALUE& rValue)
    {
        NotfDebugOut((DEB_CMAPX, "%p _IN CMapCKeyToCValue::\n", this));
        HRESULT hr = E_FAIL;
        CLock lck(_mxs);

        if (_cElements)
        {
            CNodeObject *pNode;

            if (   (_Map.Lookup(rKey, (CObject *&)pNode) )
                && (pNode->FindFirst(rValue)) )
            {
                hr = NOERROR;
            }
        }

        NotfDebugOut((DEB_CMAPX, "%p OUT CMapCKeyToCValue:: (hr:%lx)\n",this, hr));
        return hr;
    }

    STDMETHODIMP FindNext   (KEY& rKey, VALUE& rValue)
    {
        NotfDebugOut((DEB_CMAPX, "%p _IN CMapCKeyToCValue::\n", this));
        HRESULT hr = E_FAIL;
        CLock lck(_mxs);

        if (_cElements)
        {
            CNodeObject *pNode;

            if (   (_Map.Lookup(rKey, (CObject *&)pNode) )
                && (pNode->FindNext(rValue)) )
            {
                hr = NOERROR;
            }
        }


        NotfDebugOut((DEB_CMAPX, "%p OUT CMapCKeyToCValue:: (hr:%lx)\n",this, hr));
        return hr;
    }


    // Lookup and add if not there
    VALUE& operator[](ARG_KEY key)
    {
        return _Map[key];
    }

public:
    CMapCKeyToCValue() : _cElements(0), _CRefs(1)
    {
    }

    ~CMapCKeyToCValue()
    {
        NotfAssert((_cElements == 0));
    }

private:
    CRefCount       _CRefs;         // the total refcount of this object
    CRefCount       _cElements;     // # of elements
    CMutexSem       _mxs;           // single access to the list
    POSITION       _pos;
    CMap<KEY , ARG_KEY, VALUE, ARG_VALUE> _Map;
};

class CUnknown : public IUnknown
{
public:
    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

private:
    CRefCount           _CRefs;          // the total refcount of this object
};


class CMapCStrToCVal
{
public:
    STDMETHODIMP AddVal   (LPCWSTR pwzKey, CObject *pCVal);
    STDMETHODIMP RemoveVal(LPCWSTR pwzKey, CObject *pCVal);
    STDMETHODIMP FindFirst(LPCWSTR pwzKey, CObject *&ppCVal);
    STDMETHODIMP FindNext (LPCWSTR pwzKey, CObject *&ppCVal);

    STDMETHODIMP GetCount(ULONG *pCount)
    {
        HRESULT hr = NOERROR;
        if (pCount)
        {
            *pCount = _cElements;
        }
        else
        {
            hr = E_INVALIDARG;
        }

        return hr;
    }


public:
    CMapCStrToCVal() : _cElements(0), _CRefs(1)
    {
    }

    ~CMapCStrToCVal()
    {
        NotfAssert((_cElements == 0));
    }

private:
    CRefCount       _CRefs;         // the total refcount of this object
    CRefCount       _cElements;     // # of elements in map
    CMutexSem       _mxs;           // single access
    CMapStringToOb  _Map;
};

class CMapCookieToCVal
{
public:
    STDMETHODIMP AddVal   (CPkgCookie *rKey, CObject *pCVal);
    STDMETHODIMP RemoveVal(CPkgCookie *rKey, CObject *pCVal);
    STDMETHODIMP FindFirst(CPkgCookie *rKey, CObject *&ppCVal);
    STDMETHODIMP FindNext (CPkgCookie *rKey, CObject *&ppCVal);

public:
    CMapCookieToCVal() : _cElements(0), _CRefs(1)
    {
    }

    ~CMapCookieToCVal()
    {
        //NotfAssert((_cElements == 0));
        // elements get released by dtor of list
    }

private:
    CRefCount       _CRefs;         // the total refcount of this object
    CRefCount       _cElements;     // # of elements
    CMutexSem       _mxs;           // single access to the list
    CMap<CLSID , REFCLSID,  CObject*, CObject* > _Map;
};

class CScheduleGroup;
#ifndef unix
#include "Dest.hxx"
#include "Courier.hxx"
#include "NotfMgr.hxx"
#include "Package.hxx"
#include "DelAgent.hxx"
#include "SchAgent.hxx"
#else
/* just the same files with all locase names */
#include "dest.hxx"
#include "courier.hxx"
#include "notfmgr.hxx"
#include "package.hxx"
#include "delagent.hxx"
#include "schagent.hxx"
#endif /* unix */
#include "schgroup.hxx"
#include "notftobj.hxx"
#include "regstm.hxx"

class CNotificationMgr : public INotificationMgr, public INotificationProcessMgr0, public INotificationSink
{
public:

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    STDMETHODIMP RegisterNotificationSink(
        LPNOTIFICATIONSINK  pNotfctnSink,          // can be null - see mode
        LPCLSID             pNotificationDest,
        NOTFSINKMODE        NotfctnSinkMode,
        ULONG               cNotifications,
        PNOTIFICATIONTYPE   pNotificationIDs,
        PNOTIFICATIONCOOKIE pRegisterCookie,
        DWORD               dwReserved
        );

    STDMETHODIMP UnregisterNotificationSink(
        PNOTIFICATIONCOOKIE pRegisterCookie
        );

    STDMETHODIMP CreateNotification(
        // the type of notification
        REFNOTIFICATIONTYPE rNotificationType,
        // creation flags
        NOTIFICATIONFLAGS   NotificationFlags,
        // the pUnkOuter for aggregation
        LPUNKNOWN           pUnkOuter,
        // the new object just created
        LPNOTIFICATION     *ppNotification,
        DWORD               dwReserved
        );

    // find a scheduled  notification
    STDMETHODIMP FindNotification(
        // the notification cookie
        PNOTIFICATIONCOOKIE pNotificatioCookie,
        // the new object just created
        PNOTIFICATIONITEM   pNotificationItem,
        DWORD               dwReserved
        );

    // deliver a notification
    STDMETHODIMP DeliverNotification(
                    // the notificationid and object
        LPNOTIFICATION      pNotification,
                    // destination
        REFCLSID            rNotificationDest,
                    // deliver mode - group and schedule data
        DELIVERMODE         deliverMode,
                    // info about who the sender
        LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                    // the notification update interface
        LPNOTIFICATIONREPORT *ppNotfctnReport,
        //DWORD               dwReserved
        PTASK_DATA          pTaskData
        );

    // add notification to scheduler
    STDMETHODIMP ScheduleNotification(
                    // the notificationid and object
        LPNOTIFICATION      pNotification,
                    // destination
        REFCLSID            rNotificationDest,
                    // deliver mode - group and schedule data
        PTASK_TRIGGER       pTasktrigger,
                    // flags which go with the TaskTrigger
        PTASK_DATA          pTaskData,
                    // the mode how it should be delivered
        DELIVERMODE         deliverMode,
                    // info about who the sender
        LPCLSID             pClsidSender,           // class of sender can be NULL
        LPNOTIFICATIONSINK  pReportNotfctnSink,     // can be null - see mode
                    // the notification update interface
        LPNOTIFICATIONREPORT *ppNotfctnReport,
                    //the cookie of the new notification
        PNOTIFICATIONCOOKIE pNotificationCookie,
                    // reserved dword
        DWORD               dwReserved
        );

    STDMETHODIMP UpdateNotification(
        //the cookie of the new notification
        PNOTIFICATIONCOOKIE pNotificationCookie,
        // deliver mode - group and schedule data
        PTASK_TRIGGER       pTasktrigger,
        // flags which go with the TaskTrigger
        PTASK_DATA          pTaskData,
        // the mode how it should be delivered
        DELIVERMODE         deliverMode,
        // info about who the sender
        // reserved dword
        DWORD               dwReserved
        );


    STDMETHODIMP RevokeNotification(
        PNOTIFICATIONCOOKIE     pNotificationCookie,
        PNOTIFICATIONITEM       pNotificationItem,
        DWORD                   dwMode
        );

    STDMETHODIMP GetEnumNotification(
        DWORD                   grfFlags,
        LPENUMNOTIFICATION     *ppEnumNotification
        );

    // creates a new  group
    STDMETHODIMP CreateScheduleGroup(
        // the group creation flags
        DWORD               grfGroupCreateFlags,
        // the new group
        LPSCHEDULEGROUP    *ppSchGroup,
        // the cookie of the group
        PNOTIFICATIONCOOKIE pGroupCookie,
        DWORD               dwReserved
        );

    // finds an existing group
    STDMETHODIMP FindScheduleGroup(
        PNOTIFICATIONCOOKIE pGroupCookie,
        LPSCHEDULEGROUP    *ppSchGroup,
        // the cookie of the group
        DWORD               dwReserved
        );

    // revokes an entire group from the scheduler
    STDMETHODIMP RevokeScheduleGroup(
        // cookie of group to be revoked
        PNOTIFICATIONCOOKIE pGroupCookie,
        LPSCHEDULEGROUP    *ppSchGroup,
        DWORD               dwReserved
        );

    // an enumerator over the groups in the scheduler
    STDMETHODIMP GetEnumScheduleGroup(
        DWORD                   grfFlags,
        LPENUMSCHEDULEGROUP    *ppEnumScheduleGroup
        );

    //
    // INotificationProcessMgr0 - not exposed interface
    STDMETHODIMP SetMode(
        // the clsid of the process
        REFCLSID                rClsID,
        // initialization mode
        NOTIFICATIONMGRMODE     NotificationMgrMode,
        // out previous default notification mgr
        LPCLSID                *pClsIDPre,
        // a reserved again
        DWORD                   dwReserved
        );

    // notification report  method
    STDMETHODIMP DeliverReport(
        // the notification object itself
        LPNOTIFICATION          pNotification,
        // the cookie of the object the notification is targeted too
        PNOTIFICATIONCOOKIE     pRunningNotfCookie,
        DWORD                   dwReserved
        );
        

    // notificationSink //used internally
    STDMETHODIMP OnNotification(
        // the notification object itself
        LPNOTIFICATION          pNotification,
        // the report sink if - can be NULL
        LPNOTIFICATIONREPORT    pNotfctnReport,
        DWORD                   dwReserved
        );


    STDMETHODIMP RegisterThrottleNotificationType(
        ULONG               cItems,
        PTHROTTLEITEM       pThrottleItems,
        ULONG              *pcItemsOut,
        PTHROTTLEITEM      *ppThrottleItemsOut,
        DWORD               dwMode,
        DWORD               dwReserved
        );
        
        
//
// internal methods
//
private:

    HRESULT ReadRegSetting(LPCSTR pszRoot, LPCSTR pszKey, LPSTR pszValue, DWORD dwSize, CLSID *pclsid = 0);
    HRESULT WriteRegSetting(LPCSTR pszRoot, LPCSTR pszKey, LPSTR pszValue, DWORD dwSize, CLSID *pclsid =0);
    HRESULT DeletRegSetting(LPCSTR pszRoot, LPCSTR pszKey,LPSTR pszValue = 0);

public:
    HRESULT UnPersistScheduleItem(
                                  LPCSTR pszWhere,
                                  LPSTR pszSubKeyIn,
                                  DWORD  dwMode);
    HRESULT UnPersistDestinationItem(
                                     LPCSTR pszWhere,
                                     LPSTR pszSubKeyIn,
                                     DWORD dwMode);
    HRESULT CreateDefaultScheduleGroups(DWORD dwMode);

    HRESULT UnPersistRunningItems(
                                    LPCSTR pszWhere,
                                    LPSTR pszSubKeyIn,
                                    DWORD dwMode,
                                    CPackage *prgCSchdGrp[],
                                    ULONG cElements,
                                    ULONG *pcElementFilled
                                    );

    CNotificationMgr(CDeliverAgentStart &rDelAgent, CScheduleAgent &rSchAgent)
        : _CRefs(1)
        , _rScheduleAgent(rSchAgent)
        , _rDeliverAgent(rDelAgent)
        , _ClsId(CLSID_StdNotificationMgr)
        , _cookieNow()
        ,_cookiePrev()
        //,_cookiePrev(COOKIE_NULL)
    {
        _cookiePrev = (NOTIFICATIONCOOKIE) COOKIE_NULL;
    }

    ~CNotificationMgr()
    {
    }

private:
    CRefCount           _CRefs;         // the total refcount of this object
    CScheduleAgent     &_rScheduleAgent;
    CDeliverAgentStart &_rDeliverAgent;
    CLSID               _ClsId;         // the class id of the process
    CPkgCookie          _cookiePrev;    // the previous cookie of this process
    CPkgCookie          _cookieNow;     // the new cookie of this process

    DWORD               _dwMode;        //
    DWORD               _dwState;       //
};
//
// the NotificationMgr is a process variable
//
// extern CNotificationMgr *g_pCNotificationMgr;
// Helper API called by class factory and exported helper API's
HRESULT CreateNotificationMgr(DWORD dwId, REFCLSID rclsid, IUnknown *pUnkOuter, REFIID riid, IUnknown **ppUnk);

//
// helper functions
CDeliverAgentStart  * GetDeliverAgentStart(BOOL fAddRef = FALSE);
CScheduleAgent      * GetScheduleAgent(BOOL fAddRef = FALSE);
//CSchedListAgent     * GetScheduleListAgent(BOOL fAddRef = FALSE);

inline CDeliverAgentAsync *GetDeliverAgentAsync(BOOL fAddRef = FALSE)
{
    return  (CDeliverAgentAsync *) g_pDeliverAgentAsync;
}

inline CDeliverAgentThread *GetDeliverAgentThread(BOOL fAddRef = FALSE)
{
    return  (CDeliverAgentThread *) g_pDeliverAgentThread;
}

inline CDeliverAgentProcess *GetDeliverAgentProcess(BOOL fAddRef = FALSE)
{
    return  (CDeliverAgentProcess *) g_pDeliverAgentProcess;
}

inline CDeliverAgent * GetDeliverAgent(BOOL fAddRef = FALSE)
{
    return (CDeliverAgent*) GetDeliverAgentStart(fAddRef);
}

inline CThrottleListAgent *GetThrottleListAgent(BOOL fAddRef = FALSE)
{
    return  (CThrottleListAgent *) g_pCIdleListAgent;
}
inline CSchedListAgent *GetScheduleListAgent(BOOL fAddRef = FALSE)
{
    return g_pCSchedListAgent;
}

extern  CNotificationMgr            *g_pCNotificationMgr;

inline CNotificationMgr *GetNotificationMgr(BOOL fAddRef = FALSE)
{
    return g_pCNotificationMgr;
}

PTASK_TRIGGER GetDefTaskTrigger(BOOL fRand = FALSE);
HRESULT DeletRegSetting(LPCSTR pszRoot, LPCSTR pszKey);
HRESULT WriteDWORD(LPNOTIFICATION pNotObj, LPWSTR pwzStr,  DWORD dwWord);
HRESULT WriteGUID(LPNOTIFICATION pNotObj, LPCWSTR szName, GUID *pGuid);
HRESULT WriteOLESTR(LPNOTIFICATION pNotObj, LPCWSTR szName, LPCWSTR szVal);

#define WZ_HRESULT L"Notification_HRESULT"
#define WZ_COOKIE  L"Notification_COOKIE"

extern HINSTANCE g_hInst;
LPWSTR GetResourceString(UINT iResId);
BOOL AreWeTheDefaultProcess();
BOOL IsThereADefaultProcess();
void PostSyncDefProcNotifications();

//#if DBG == 1
#define IS_BAD_INTERFACE(punk) \
    ( \
     punk && \
     (IsBadWritePtr((void *)(punk), sizeof(void *)) || \
      IsBadCodePtr(FARPROC(*((DWORD **)punk))) \
     ) \
    )

#define IS_BAD_NULL_INTERFACE(punk) \
    ((!punk) || IS_BAD_INTERFACE(punk))

#define IS_BAD_READPTR(ptr, cb) \
    (ptr && IsBadReadPtr(ptr, cb))

#define IS_BAD_NULL_READPTR(ptr, cb) \
    ((!ptr) || IS_BAD_READPTR(ptr, cb))
     
#define IS_BAD_WRITEPTR(ptr, cb) \
    (ptr && IsBadWritePtr(ptr, cb))

#define IS_BAD_NULL_WRITEPTR(ptr, cb) \
    ((!ptr) || IS_BAD_WRITEPTR(ptr, cb))

/*
#else // DBG = 1

#define IS_BAD_INTERFACE(punk) 

#define IS_BAD_NULL_INTERFACE(punk) \
    (!punk)

#define IS_BAD_READPTR(ptr, cb)

#define IS_BAD_NULL_READPTR(ptr, cb) \
    (!ptr)

#define IS_BAD_WRITEPTR(ptr, cb)

#define IS_BAD_NULL_WRITEPTR(ptr, cb) \
    (!ptr)

#endif // DBG = 1
*/

#if DBG == 1

void TrackAlloc(void *ptr, int line);
void UntrackAlloc(void *ptr, int line);
void DumpAllocTrack();

//#define TRACK_ALLOC(ptr)    TrackAlloc(ptr, __LINE__)
//#define UNTRACK_ALLOC(ptr)  UntrackAlloc(ptr, __LINE__)
//#define DUMP_ALLOC_TRACK()  DumpAllocTrack()
#define TRACK_ALLOC(ptr)
#define UNTRACK_ALLOC(ptr)
#define DUMP_ALLOC_TRACK()

#else

#define TRACK_ALLOC(ptr)
#define UNTRACK_ALLOC(ptr)
#define DUMP_ALLOC_TRACK()

#endif


#endif // _NOTIFCTN_HXX_
 

