//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       notfmgr.hxx
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
#ifndef _NOTFMGR_HXX_
#define _NOTFMGR_HXX_

//
// dll shared data
//
//extern CRefCount cRefPackage;
//extern CRefCount cRefCookie;

// BUBBUG: will be unlimited post beta 1
#define DESTINATIONPORT_MAX     256
#define SCHEDULELISITEM_MAX     1024
#define THROTTLEITEM_MAX        16
#define THROTTLE_RUNNING_MAX    16 
#define THROTTLE_WAITING_MAX    16

#define TL_ADD_TO_WAIT          1

typedef struct _tagDestinationPort
{
    HWND    _hWndPort;
    DWORD   _dwThreadId;
    ULONG   _CRefs;
} DestinationPort;

typedef struct _SL_ITEM
{
    __int64             date;
    NOTIFICATIONCOOKIE  notfCookie;
    DWORD               dwState;
    __int64             lastUpdateDate;
} SL_ITEM, SCHEDLISTITEMKEY, *PSCHEDLISTITEMKEY;

typedef struct _WAIT_ITEM
{
    NOTIFICATIONCOOKIE  NotificationCookie;
    DWORD               dwBasePriority;
    DWORD               dwCurrentPriority;
    HWND                hWnd;
} WAIT_ITEM, *PWAIT_ITEM;


typedef struct _THROTTLE_ITEM
{
    NOTIFICATIONTYPE    NotificationType;
    LONG                nParallel;
    LONG                nRunning;
    LONG                nWaiting;
    DWORD               dwPriority;
    //HWND                hWndNext;
    DWORD               dwPriorityMaxWaiting;

    SYSTEMTIME          dateExcludeBegin;
    SYSTEMTIME          dateExcludeEnd;
    DWORD               dwIntervalMin;
    DWORD               dwFlags;
    NOTIFICATIONCOOKIE  nRunningCookie[THROTTLE_RUNNING_MAX];
    WAIT_ITEM           nWaitItem[THROTTLE_WAITING_MAX];
} THROTTLE_ITEM, *PTHROTTLE_ITEM;

class CDestinationPort : public DestinationPort
{
public:
    ULONG AddRef()
    {
        return ++_CRefs;
    }

    ULONG Release()
    {
        return --_CRefs;
    }

    HWND GetPort()
    {
        return _hWndPort;
    }
    void SetPort(HWND hWnd)
    {
        _hWndPort = hWnd;
    }
    BOOL IsValidPort()
    {
        return IsWindow(_hWndPort) && _dwThreadId;
    }
    DWORD GetDestThreadId()
    {
        return _dwThreadId;
    }
    CDestinationPort(DestinationPort &DestPort)
    {
        _hWndPort = DestPort._hWndPort;
        _dwThreadId = DestPort._dwThreadId;
        _CRefs = 1;
    }
    void SetDestPort(HWND hWnd, DWORD dwThreadId)
    {
        _hWndPort = hWnd;
        _dwThreadId = dwThreadId;
    }

    CDestinationPort(HWND hWnd, DWORD dwThreadId = 0)
    {
        _hWndPort = hWnd;
        _CRefs = 1;
        _dwThreadId = dwThreadId ? dwThreadId : GetCurrentThreadId();
    }

    CDestinationPort()
    {
        _hWndPort = 0;
        _CRefs = 0;
        _dwThreadId = 0;
    }

    ~CDestinationPort()
    {
    }

private:
};

extern ULONG g_dwCurrentCookie;
extern ULONG g_dwCurrentPackage;
extern ULONG g_dwCurrentUserIdle;
extern ULONG g_cRefProcesses;
extern DWORD g_dwGlobalState;

#define GL_STATE_USERIDLE    0x00000001

class CGlobalNotfMgr: public CCourierAgent
{
public:
    // CCourierAgent methods
    virtual HRESULT HandlePackage(CPackage *pCPackage);

    HRESULT CGlobalNotfMgr::LookupDestination(
                            CPackage              *pCPackage,
                            CDestination         **ppCDestination,
                            DWORD                 dwFlags
                            );


    HRESULT SetLastUpdateDate(CPackage *pCPackage);

    ULONG GetPackageCount(BOOL fIncrease = TRUE)
    {
        ULONG uPkgCount = 0;
        if (fIncrease)
        {
            CLockSmMutex lck(_Smxs);
            uPkgCount = ++g_dwCurrentPackage;
        }
        else
        {
            uPkgCount = g_dwCurrentPackage;
        }

        return uPkgCount;
    }
    ULONG GetUserIdleCount(BOOL fIncrease = TRUE)
    {
        ULONG uPkgCount = 0;
        if (fIncrease)
        {
            CLockSmMutex lck(_Smxs);
            uPkgCount = ++g_dwCurrentUserIdle;
        }
        else
        {
            uPkgCount = g_dwCurrentUserIdle;
        }

        return uPkgCount;
    }

    DWORD GetGlobalState()
    {
        DWORD dwState;
        {
            CLockSmMutex lck(_Smxs);
            dwState = g_dwGlobalState;
        }

        return dwState;
    }
    
    DWORD SetGlobalStateOn(DWORD dwFlags)
    {
        DWORD dwState;
        {
            CLockSmMutex lck(_Smxs);
            dwState = g_dwGlobalState;
            g_dwGlobalState |= dwFlags;
        }

        return dwState;
    }
    
    DWORD SetGlobalStateOff(DWORD dwFlags)
    {
        DWORD dwState;
        {
            CLockSmMutex lck(_Smxs);
            dwState = g_dwGlobalState;
            g_dwGlobalState &= ~dwFlags;
        }
        return dwState;
    }
    
    ULONG GetCookieCount(BOOL fIncrease = TRUE)
    {
        ULONG uCookieCount = 0;
        if (fIncrease)
        {
            CLockSmMutex lck(_Smxs);
            g_dwCurrentCookie++;
        }
        uCookieCount = g_dwCurrentCookie;

        return uCookieCount;
    }

    ULONG GetProcessId()
    {
        return _uProcessCount;
    }

    //
    //
    HRESULT GetAllScheduleItems(SCHEDLISTITEMKEY **ppSchItems, ULONG *pCount, CFileTime &rdate);
    HRESULT AddScheduleItem(CPackage *pCPackage, BOOL fCheck = TRUE);
    HRESULT RemoveScheduleItem(SCHEDLISTITEMKEY &rSchItem,CPackage *pCPackage);
    HRESULT LookupScheduleItem(SCHEDLISTITEMKEY &rSchItem,CPackage *pCPackage);
    HRESULT LoadScheduleItemPackage(SCHEDLISTITEMKEY &rSchItem, CPackage **ppCPackage);

    HRESULT GetScheduleItemCount(ULONG *pulCount);
    HRESULT IsScheduleItemSyncd(CFileTime &rdate, ULONG uCount);

    //
    //
    HRESULT SetDefaultDestinationPort(CDestinationPort &rCDestPort, CDestinationPort *pCDestPortPrev);
    HRESULT GetDefaultDestinationPort(CDestinationPort *pCDestPort);

    HRESULT GetAllDestinationPorts(CDestinationPort **ppCDestPort, ULONG *pCount);
    HRESULT AddDestinationPort(CDestinationPort &rCDestPort, CDestination *pCDest = 0);
    HRESULT RemoveDestinationPort(CDestinationPort &rCDestPort, CDestination *pCDest = 0);
    HRESULT LookupDestinationPort(CDestinationPort &rCDestPort, CDestination *pCDest = 0);
    HRESULT SetScheduleItemState(PNOTIFICATIONCOOKIE pNotfCookie, DWORD dwStateNew, SCHEDLISTITEMKEY *pSchItem = 0);
    HRESULT SetScheduleItemState(DWORD dwIndex, DWORD dwStateNew, SCHEDLISTITEMKEY *pSchItem = 0);
    
    HRESULT AddItemToRunningList( CPackage *pCPackage, DWORD dwMode);
    HRESULT RemoveItemFromRunningList( CPackage *pCPackage, DWORD dwMode);
    HRESULT RemoveItemFromWaitingList( CPackage *pCPackage, DWORD dwMode);
    HRESULT RegisterThrottleNotificationType(ULONG cItems, PTHROTTLEITEM  pThrottleItems, ULONG *pcItemsOut, PTHROTTLEITEM  *ppThrottleItemsOut, DWORD dwMode, DWORD dwReserved);
    HRESULT WakeupAll();

    UINT SetScheduleTimer(UINT idTimer, UINT uTimeout, TIMERPROC tmprc);

    HRESULT IsConnectedToInternet();

    CGlobalNotfMgr();

    virtual ~CGlobalNotfMgr()
    {
    }

    CSmMutex  _Smxs;

private:
    ULONG     _uProcessCount;
};

extern CGlobalNotfMgr          *g_pGlobalNotfMgr        ;

inline CGlobalNotfMgr * GetGlobalNotfMgr(BOOL fAddRef = FALSE)
{
    return (CGlobalNotfMgr*) g_pGlobalNotfMgr;
}
ULONG GetUserIdleCounter();
ULONG GetGlobalCounter();
DWORD GetGlobalState();
DWORD SetGlobalStateOn(DWORD dwFlags);
DWORD SetGlobalStateOff(DWORD dwFlags);

extern ULONG            g_urgScheduleListItemtMax;
extern SCHEDLISTITEMKEY g_rgSortListItem[];
extern CFileTime        g_DateLastSync;

BOOL IsThrottleableNotificationType(REFIID riid);

HRESULT ReadFromStream(IStream *pStream, void *pv, ULONG cbRequired);
HRESULT WriteToStream(IStream *pStream, void *pv, ULONG cbRequired);

#endif // _NOTFMGR_HXX_

