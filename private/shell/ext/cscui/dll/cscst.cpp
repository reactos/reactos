//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       cscst.cpp
//
//--------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include <shellp.h>     // STR_DESKTOPCLASS
#ifdef REPORT_DEVICE_CHANGES
#   include <dbt.h>        // Device change notifications.
#endif // REPORT_DEVICE_CHANGES
#include <sddl.h>       // For ConvertStringSidToSid
#include "cscst.h"
#include "options.h"
#include "statdlg.h"    // CStatusDlg
#include "uihooks.h"    // Self-host notifications
#include "folder.h"
#include "eventlog.h"
#include "msg.h"
#include "purge.h"
#include "security.h"


#if DBG
//
// This code is used to manage the hidden window when we 
// unhide it and display debug output to it via STDBGOUT().
//
#include <commdlg.h>
#include <stdarg.h>
const TCHAR c_szSysTrayOutput[] = TEXT("SysTrayOutput");
int STDebugLevel(void);
void STDebugOnLogEvent(HWND hwndList, LPCTSTR pszText);
void STDebugSaveListboxContent(HWND hwndParent);
DWORD STDebugOpenNetCacheKey(DWORD dwAccess, HKEY *phkey);

#endif // DBG

//
// Size of systray icons.
//
#define CSC_ICON_CX             16
#define CSC_ICON_CY             16
//
// Timer IDs are arbitrary.
//
#define ID_TIMER_FLASHICON    2953
#define ID_TIMER_REMINDER     2954
#define ID_TIMER_STATECHANGE  2955


// Prototypes
void ApplyAdminFolderPolicy(void);   // in admin.cpp

void _RefreshAllExplorerWindows(LPCTSTR pszServer);

// Globals
static HWND g_hWndNotification = NULL;

extern HWND g_hwndStatusDlg;    // in statdlg.cpp

HANDLE g_hToken = NULL;

#ifdef REPORT_DEVICE_CHANGES
HDEVNOTIFY g_hDevNotify = NULL;
#endif // REPORT_DEVICE_CHANGES

//
// RAS Autodial API.
//
typedef BOOL (WINAPI * PFNHLPNBCONNECTION)(LPCTSTR);



#if DBG
//
// Provide some text-form names for state and input values
// to support debug output.  The order of these corresponds 
// to the STS_XXXXX enumeration.
//
LPCTSTR g_pszSysTrayStates[] =      { TEXT("STS_INVALID"),
                                      TEXT("STS_ONLINE"),
                                      TEXT("STS_DIRTY"),
                                      TEXT("STS_MDIRTY"),
                                      TEXT("STS_SERVERBACK"),
                                      TEXT("STS_MSERVERBACK"),
                                      TEXT("STS_OFFLINE"),
                                      TEXT("STS_MOFFLINE"),
                                      TEXT("STS_NONET") };
//
// A simple function to translate a state value to a string.
//
LPCTSTR SysTrayStateStr(eSysTrayState s)
{
    return g_pszSysTrayStates[int(s)];
}

#endif

//
// A simple dynamic list of server names.  A name can be provided
// as either a "\\server" or "\\server\share" and only the server
// part "\\server" is stored.
//
class CServerList
{
public:
    CServerList(void)
        : m_hdpa(DPA_Create(10)) { }

    ~CServerList(void);

    bool Add(LPCTSTR pszServer);

    void Remove(LPCTSTR pszServer);

    void Clear(void);

    int Find(LPCTSTR pszServer);

    int Count(void) const;

    LPCTSTR Get(int iItem) const;

    bool Exists(LPCTSTR pszServer)
        { return -1 != Find(pszServer); }

private:
    HDPA m_hdpa;

    void GetServerFromPath(LPCTSTR pszPath, LPTSTR pszServer, int cchServer);
    //
    // Prevent copy.
    //
    CServerList(const CServerList& rhs);
    CServerList& operator = (const CServerList& rhs);
};


//
// The class that translates CSC agent input and cache status into a subsequent
// systray UI state.  Originally this was a table-driven state machine
// (hence the name).  It later proved sufficient to do a simple scan of cache
// status and determine UI state based on the statistics obtained.  The name
// has been retained for lack of something better.
//
class CStateMachine
{
public:
    CStateMachine(bool bNoNet) : m_bNoNet(bNoNet) { }

    //
    // This is THE function for converting CSC agent input (or a 
    // simple status check) into a systray icon state.
    //
    eSysTrayState TranslateInput(UINT uMsg, LPTSTR pszShare, UINT cchShare);

    void PingServers();

    bool ServerPendingReconnection(LPCTSTR pszServer)
        { return m_PendingReconList.Add(pszServer); }

    void ServerReconnected(LPCTSTR pszServer)
        { m_PendingReconList.Remove(pszServer); }

    void ServerUnavailable(LPCTSTR pszServer)
        { m_PendingReconList.Remove(pszServer); }

    void AllServersUnavailable(void)
        { m_PendingReconList.Clear(); }

    bool IsServerPendingReconnection(LPCTSTR pszServer)
        { return m_PendingReconList.Exists(pszServer); }

private:
    CServerList m_PendingReconList;
    bool        m_bNoNet;

    //
    // Some helper functions for decoding CSC share status values.
    //
    bool ShareIsOffline(DWORD dwCscStatus) const
    {
        return (0 != (FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwCscStatus));
    }

    bool ShareHasFiles(LPCTSTR pszShare, bool *pbModified = NULL, bool *pbOpen = NULL) const;

    //
    // Prevent copy.
    //
    CStateMachine(const CStateMachine& rhs);
    CStateMachine& operator = (const CStateMachine& rhs);
};



//
// The CSysTrayUI class encapsulates the manipulation of the systray icon
// so that the rest of the CSCUI code is exposed to only a narrow interface
// to the systray.  It also maintains state information to control flashing
// of the systray icon.   All flashing processing is provided by this class.
//
class CSysTrayUI
{
public:
    ~CSysTrayUI(void);
    //
    // Set the state of the systray icon.  This will only change the
    // icon if the state has changed.  Therefore this function can be
    // called without worrying about excessive redundant updates to 
    // the display.
    //
    bool SetState(eSysTrayState state, LPCTSTR pszServer = NULL);
    //
    // Retrieve the current "state" of the systray UI.  The state
    // is one of the STS_XXXXX codes.
    //
    eSysTrayState GetState(void) const
        { return m_state; }
    //
    // Retrieve the server name to be used in CSCUI elements.
    // If the server name string is empty, that means there are
    // multiple servers in the given state.
    //
    LPCTSTR GetServerName(void) const
        { return m_szServer; }
    //
    // Show the balloon text for the current systray state.
    //
    void ShowReminderBalloon(void);
    //
    // Reset the reminder timer.
    //
    void ResetReminderTimer(bool bRestart);
    //
    // Make any adjustments when a WM_WININICHANGE is received.
    //
    void OnWinIniChange(LPCTSTR pszSection);
    //
    //
    // Get a reference to THE singleton instance.
    //
    static CSysTrayUI& GetInstance(void);

private:
    //
    // A minimal autoptr class to ensure the singleton instance
    // is deleted.
    //
    class autoptr
    {
        public:
            autoptr(void)
                : m_ptr(NULL) { }
            ~autoptr(void)
                { delete m_ptr; }
            CSysTrayUI* Get(void) const
                { return m_ptr; }
            void Set(CSysTrayUI *p)
                { delete m_ptr; m_ptr = p; }

        private:
            CSysTrayUI *m_ptr;
            autoptr(const autoptr& rhs);
            autoptr& operator = (const autoptr& rhs);
    };
    //
    // Icon info maintained for each UI state.
    //
    struct IconInfo 
    {
        HICON hIcon;           // Handle to icon to display in this state.
        UINT  idIcon;          // ID of icon to display in this state.
        int   iFlashTimeout;   // 0 == No icon flash.  Time is in millisec.
    };
    //
    // Info maintained to describe the various balloon text messages.
    // Combination of state and dwTextFlags are the table keys.
    //
    struct BalloonInfo
    {
        eSysTrayState state;     // SysTray state value.
        DWORD dwTextFlags;       // BTF_XXXXX flags.
        DWORD dwInfoFlags;       // NIIF_XXXXX flag.
        UINT  idHeader;          // Res id for header part.
        UINT  idStatus;          // Res id for status part.
        UINT  idBody;            // Res id for body part.
        UINT  idDirective;       // Res id for directive part.
    };
    //
    // Info maintained to describe the various tooltip text messages.
    //
    struct TooltipInfo
    {
        eSysTrayState state;     // SysTray state value.
        UINT idTooltip;          // Tooltip text resource ID.
    };
    //
    // Info maintained for special-case supression of systray balloons.
    // There are some state transitions that shouldn't generate a balloon.
    // This structure describes each entry in an array of supression info.
    //
    struct BalloonSupression
    {
        eSysTrayState stateFrom; // Transitioning from this state.
        eSysTrayState stateTo;   // Transitioning to this state.
    };
    //
    // Enumeration for controlling what's done to the systray on update.
    //
    enum eUpdateFlags { UF_ICON      = 0x00000001,   // Update the icon.
                        UF_FLASHICON = 0x00000002,   // Flash the icon.
                        UF_BALLOON   = 0x00000004,   // Show the balloon.
                        UF_REMINDER  = 0x00000008 }; // Balloon is a reminder.
    //
    // These flags relate a cache state to balloon text message.
    // They fit into an encoded mask where the lowest 4 bits
    // contain the eSysTrayState (STS_XXXXXX) code.
    //
    //      (STS_OFFLINE | BTF_INITIAL) 
    //
    // would indicate the condition where the state is "offline" for 
    // a single server and the text to be displayed is for the initial
    // notification.
    //
    enum eBalloonTextFlags { 
                             BTF_INITIAL = 0x00000010, // Initial notification
                             BTF_REMIND  = 0x00000020  // Reminder
                           };

    static IconInfo    s_rgIconInfo[];       // The icon info
    static BalloonInfo s_rgBalloonInfo[];    // Balloon configuration info.
    static TooltipInfo s_rgTooltipInfo[];    // Tooltip configuration info.
    static BalloonSupression s_rgBalloonSupression[];
    static const int   s_iMinStateChangeInterval;
    UINT_PTR           m_idFlashingTimer;    // Flash timer id.
    UINT_PTR           m_idReminderTimer;    // Timer for showing reminder balloons.
    UINT_PTR           m_idStateChangeTimer; // Timer for queued state changes.
    UINT               m_iIconFlashTime;     // Period of icon flashes (ms).
    HICON&             m_hIconNoOverlay;     // Icon used for flashing.
    HWND               m_hwndNotify;         // Notification window.
    DWORD              m_dwFlashingExpires;  // Tick count when flash timer expires.
    DWORD              m_dwNextStateChange;  // Tick count for next queued state change.
    TCHAR              m_szServer[MAX_PATH]; // Servername for balloon messages.
    TCHAR              m_szServerQueued[MAX_PATH];
    eSysTrayState      m_state;              // Remember current state.
    eSysTrayState      m_statePrev;
    eSysTrayState      m_stateQueued;        
    bool               m_bFlashOverlay;      // Alternates 0,1 (1 == display overlay, 0 == don't)
    bool               m_bActive;            // 1 == we have an active icon in systray.

    //
    // Enforce singleton existance by making construction
    // and copy operations private.
    //
    CSysTrayUI(HWND hwndNotify);
    CSysTrayUI(const CSysTrayUI& rhs);
    CSysTrayUI& operator = (const CSysTrayUI& rhs);

    void UpdateSysTray(eUpdateFlags uFlags, LPCTSTR pszServer = NULL);

    int GetBalloonInfoIndex(eSysTrayState state, DWORD dwTextFlags);

    bool StateHasBalloonText(eSysTrayState state, DWORD dwTextFlags);

    void GetBalloonInfo(eSysTrayState state,
                        DWORD dwTextFlags,
                        LPTSTR pszTextHdr,
                        int cchTextHdr,
                        LPTSTR pszTextBody,
                        int cchTextBody,
                        DWORD *pdwInfoFlags,
                        UINT *puTimeout);
    
    bool SupressBalloon(eSysTrayState statePrev, eSysTrayState state);

    LPTSTR GetTooltipText(eSysTrayState state,
                          LPTSTR pszText,
                          int cchText);

    bool IconFlashedLongEnough(void);

    void KillIconFlashTimer(void);

    void HandleFlashTimer(void);

    void OnStateChangeTimerExpired(void);

    static VOID CALLBACK FlashTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    static VOID CALLBACK ReminderTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    static VOID CALLBACK StateChangeTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
};


#define ICONFLASH_FOREVER     (UINT(-1))
#define ICONFLASH_NONE        0
//
// These rows must stay in the same order as the STS_XXXXX enumeration members.
// For flash timeout values, 0 == no flash, -1 == never stop.
// Everything else is a timeout in milliseconds.
//
CSysTrayUI::IconInfo
CSysTrayUI::s_rgIconInfo[] = {
    { NULL, 0,                  ICONFLASH_NONE    },  /* STS_INVALID     */
    { NULL, 0,                  ICONFLASH_NONE    },  /* STS_ONLINE      */ 
    { NULL, IDI_CSCWARNING,     ICONFLASH_FOREVER },  /* STS_DIRTY       */ 
    { NULL, IDI_CSCWARNING,     ICONFLASH_FOREVER },  /* STS_MDIRTY      */ 
    { NULL, IDI_CSCINFORMATION, ICONFLASH_NONE    },  /* STS_SERVERBACK  */ 
    { NULL, IDI_CSCINFORMATION, ICONFLASH_NONE    },  /* STS_MSERVERBACK */ 
    { NULL, IDI_CSCNORMAL,      ICONFLASH_NONE    },  /* STS_OFFLINE     */ 
    { NULL, IDI_CSCNORMAL,      ICONFLASH_NONE    },  /* STS_MOFFLINE    */ 
    { NULL, IDI_CSCNORMAL,      ICONFLASH_NONE    }}; /* STS_NONET       */

//
// This table describes all information related to displaying the systray balloons.
// The first two columns are the keys to each record; those being a systray UI state
// and a mask of balloon-text flags.
// Notes:
//        1. There's no balloon for STS_NONET.  We found that the user's response is
//           duh, I know I have no net.  
//
//
CSysTrayUI::BalloonInfo
CSysTrayUI::s_rgBalloonInfo[] = {
    { STS_INVALID,    BTF_INITIAL, NIIF_NONE,    0,                 0,                   0,                        0,                   },
    { STS_INVALID,    BTF_REMIND,  NIIF_NONE,    0,                 0,                   0,                        0,                   },
    { STS_OFFLINE,    BTF_INITIAL, NIIF_INFO,    IDS_BTHDR_INITIAL, IDS_BTSTA_OFFLINE,   IDS_BTBOD_OFFLINE,        IDS_BTDIR_VIEWSTATUS },
    { STS_MOFFLINE,   BTF_INITIAL, NIIF_INFO,    IDS_BTHDR_INITIAL, IDS_BTSTA_OFFLINE,   IDS_BTBOD_OFFLINE_M,      IDS_BTDIR_VIEWSTATUS },
    { STS_OFFLINE,    BTF_REMIND,  NIIF_INFO,    IDS_BTHDR_REMIND,  IDS_BTSTA_OFFLINE,   IDS_BTBOD_STILLOFFLINE,   IDS_BTDIR_VIEWSTATUS },
    { STS_MOFFLINE,   BTF_REMIND,  NIIF_INFO,    IDS_BTHDR_REMIND,  IDS_BTSTA_OFFLINE,   IDS_BTBOD_STILLOFFLINE_M, IDS_BTDIR_VIEWSTATUS },
//    { STS_SERVERBACK, BTF_INITIAL, NIIF_INFO,    IDS_BTHDR_INITIAL, IDS_BTSTA_SERVERBACK,IDS_BTBOD_SERVERBACK,     IDS_BTDIR_RECONNECT  },
//    { STS_MSERVERBACK,BTF_INITIAL, NIIF_INFO,    IDS_BTHDR_INITIAL, IDS_BTSTA_SERVERBACK,IDS_BTBOD_SERVERBACK_M,   IDS_BTDIR_RECONNECT  },
    { STS_SERVERBACK, BTF_REMIND,  NIIF_INFO,    IDS_BTHDR_REMIND,  IDS_BTSTA_SERVERBACK,IDS_BTBOD_STILLBACK,      IDS_BTDIR_RECONNECT  },
    { STS_MSERVERBACK,BTF_REMIND,  NIIF_INFO,    IDS_BTHDR_REMIND,  IDS_BTSTA_SERVERBACK,IDS_BTBOD_STILLBACK_M,    IDS_BTDIR_RECONNECT  },
    { STS_DIRTY,      BTF_INITIAL, NIIF_WARNING, IDS_BTHDR_INITIAL, IDS_BTSTA_DIRTY,     IDS_BTBOD_DIRTY,          IDS_BTDIR_SYNC       },
    { STS_MDIRTY,     BTF_INITIAL, NIIF_WARNING, IDS_BTHDR_INITIAL, IDS_BTSTA_DIRTY,     IDS_BTBOD_DIRTY_M,        IDS_BTDIR_SYNC       },
    { STS_DIRTY,      BTF_REMIND,  NIIF_WARNING, IDS_BTHDR_REMIND,  IDS_BTSTA_DIRTY,     IDS_BTBOD_STILLDIRTY,     IDS_BTDIR_SYNC       },
    { STS_MDIRTY,     BTF_REMIND,  NIIF_WARNING, IDS_BTHDR_REMIND,  IDS_BTSTA_DIRTY,     IDS_BTBOD_STILLDIRTY_M,   IDS_BTDIR_SYNC       }
};

//
// This table lists all of the state transitions that do not generate balloons.
// Ideally, I would have a true state machine to control the UI for any given state transition.
// However, since we have quite a few states and since you can transition from any state
// to almost any other state, the state transition table would be large and confusing
// to read.  Instead, I've taken the position to assume all state transitions generate
// the balloon UI associated with the "to" state unless the transition is listed
// in this table.
//
CSysTrayUI::BalloonSupression
CSysTrayUI::s_rgBalloonSupression[] = {
    { STS_MOFFLINE, STS_OFFLINE  },
    { STS_NONET,    STS_OFFLINE  },
    { STS_NONET,    STS_MOFFLINE }
    };

//
// This table describes all information related to displaying tooltip text
// for the systray icon.
//
CSysTrayUI::TooltipInfo
CSysTrayUI::s_rgTooltipInfo[] = {
    { STS_INVALID,     0                   },
    { STS_OFFLINE,     IDS_TT_OFFLINE      },
    { STS_MOFFLINE,    IDS_TT_OFFLINE_M    },
    { STS_SERVERBACK,  IDS_TT_SERVERBACK   },
    { STS_MSERVERBACK, IDS_TT_SERVERBACK_M },
    { STS_DIRTY,       IDS_TT_DIRTY        },
    { STS_MDIRTY,      IDS_TT_DIRTY_M      },
    { STS_NONET,       IDS_TT_NONET        }
};


//
// Wrap the CEventLog class so we can control log initialization
// and also filter events based on the CSCUI event logging level.
// The idea here is to create a CscuiEventLog object whenever you
// want to write to the event log.  The ReportEvent member has
// been designed to handle log initialization as well as filtering
// message output to respect the current CSCUI event logging level
// set in the registry/policy.  It's recommended that the 
// CscuiEventLog object be created as a local variable so that 
// once the reporting is complete, the object is destroyed and
// the system event log handle is closed.
//
class CscuiEventLog
{
public:
    CscuiEventLog(void)
        : m_iEventLoggingLevel(CConfig::GetSingleton().EventLoggingLevel()) { }

    ~CscuiEventLog(void) { }

    HRESULT ReportEvent(WORD wType,
                        WORD wCategory,
                        DWORD dwEventID,
                        PSID lpUserSid = NULL,
                        LPVOID pvRawData = NULL,
                        DWORD cbRawData = 0);

    bool LoggingEnabled(void) const
        { return 0 < m_iEventLoggingLevel; }

    void Push(HRESULT hr, CEventLog::eFmt fmt)
        { m_log.Push(hr, fmt); }

    void Push(LPCTSTR psz)
        { m_log.Push(psz); }

private:
    CEventLog m_log;
    int       m_iEventLoggingLevel;
};


//-----------------------------------------------------------------------------
// CscuiEventLog member functions.
//-----------------------------------------------------------------------------
HRESULT 
CscuiEventLog::ReportEvent(
    WORD wType,
    WORD wCategory,
    DWORD dwEventID,
    PSID lpUserSid,
    LPVOID pvRawData,
    DWORD cbRawData
    )
{
    //
    // Add to this table if you add new event messages.
    //
    static const struct
    {
        DWORD dwEventID;
        int   iLevel;
    } rgEventInfo[] = {{ MSG_I_SERVER_OFFLINE,       1 },
                       { MSG_I_SERVER_AVAILABLE,     3 },
                       { MSG_I_NET_STOPPED,          2 },
                       { MSG_I_NET_STARTED,          2 },
                       { MSG_E_CACHE_CORRUPTED,      0 },
                       { MSG_I_SERVER_AUTORECONNECT, 3 }};

    int iLevel = CConfig::GetSingleton().EventLoggingLevel();

    for (int i = 0; i < ARRAYSIZE(rgEventInfo); i++)
    {
        if (dwEventID == rgEventInfo[i].dwEventID && iLevel >= rgEventInfo[i].iLevel)
        {
            if (SUCCEEDED(m_log.Initialize(TEXT("Offline Files"))))
            {
                return m_log.ReportEvent(wType, 
                                         wCategory, 
                                         dwEventID, 
                                         lpUserSid, 
                                         pvRawData, 
                                         cbRawData);
            }
        }
    }
    return S_FALSE;
}


//-----------------------------------------------------------------------------
// CServerList member functions.
//-----------------------------------------------------------------------------
CServerList::~CServerList(
    void
    )
{
    if (NULL != m_hdpa)
    {
        int cEntries = DPA_GetPtrCount(m_hdpa);
        LPTSTR pszEntry;
        for (int i = 0; i < cEntries; i++) 
        {
            pszEntry = (LPTSTR)DPA_GetPtr(m_hdpa, i);
            if (NULL != pszEntry)
                LocalFree(pszEntry);
        }
        DPA_Destroy(m_hdpa);
    }
}


void
CServerList::GetServerFromPath(
    LPCTSTR pszPath,
    LPTSTR pszServer,
    int cchServer
    )
{
    TCHAR szServer[MAX_PATH];
    lstrcpyn(szServer, pszPath, ARRAYSIZE(szServer));
    PathAddBackslash(szServer);
    PathStripToRoot(szServer);
    LPTSTR pszLastBackslash = StrRChr(szServer, szServer + lstrlen(szServer), TEXT('\\'));
    if (NULL != pszLastBackslash && pszLastBackslash > (szServer + 2))
        *pszLastBackslash = TEXT('\0');
    lstrcpyn(pszServer, szServer, cchServer);
}
    

bool
CServerList::Add(
    LPCTSTR pszServer
    )
{
    if (NULL != m_hdpa)
    {
        if (!Exists(pszServer))
        {
            int cchEntry = lstrlen(pszServer) + 1;
            LPTSTR pszEntry = (LPTSTR)LocalAlloc(LPTR, sizeof(TCHAR) * cchEntry);
            if (NULL != pszEntry)
            {
                GetServerFromPath(pszServer, pszEntry, cchEntry);
                if (-1 != DPA_AppendPtr(m_hdpa, pszEntry))
                    return true;
                //
                // Addition to DPA failed.  Delete the string buffer.
                //
                LocalFree(pszEntry);
            }
        }
    }
    return false;
}

void
CServerList::Remove(
    LPCTSTR pszServer
    )
{
    int iEntry = Find(pszServer);
    if (-1 != iEntry)
    {
        LPTSTR pszEntry = (LPTSTR)DPA_DeletePtr(m_hdpa, iEntry);
        if (NULL != pszEntry)
            LocalFree(pszEntry);
    }
}

LPCTSTR
CServerList::Get(
    int iItem
    ) const
{
    if (NULL != m_hdpa)
        return (LPCTSTR)DPA_GetPtr(m_hdpa, iItem);
    return NULL;
}


int
CServerList::Count(
    void
    ) const
{
    if (NULL != m_hdpa)
        return DPA_GetPtrCount(m_hdpa);
    return 0;
}

                
//
// Locate a server name in the "pending reconnection" list.
// pszServer can either be "\\server" or "\\server\share".
//
// Returns:  Index of entry if found.  -1 if not found.
//
int
CServerList::Find(
    LPCTSTR pszServer
    )
{
    TCHAR szServer[MAX_PATH];
    GetServerFromPath(pszServer, szServer, ARRAYSIZE(szServer));
    if (NULL != m_hdpa)
    {
        int cEntries = DPA_GetPtrCount(m_hdpa);
        LPTSTR pszEntry;
        for (int i = 0; i < cEntries; i++) 
        {
            pszEntry = (LPTSTR)DPA_GetPtr(m_hdpa, i);
            if (NULL != pszEntry)
            {
                if (0 == lstrcmpi(pszEntry, szServer))
                    return i;
            }
        }
    }
    return -1;        
}


void
CServerList::Clear( 
    void
    )
{
    if (NULL != m_hdpa)
    {
        int cEntries = DPA_GetPtrCount(m_hdpa);
        LPTSTR pszEntry;
        for (int i = 0; i < cEntries; i++) 
        {
            pszEntry = (LPTSTR)DPA_DeletePtr(m_hdpa, i);
            if (NULL != pszEntry)
            {
                LocalFree(pszEntry);
            }
        }
    }
}


//-----------------------------------------------------------------------------
// CStateMachine member functions.
//-----------------------------------------------------------------------------
//
// Translates a STWM_XXXXX message from the CSC agent into a systray UI state
// code.  The caller also provides a buffer to a server name.  If we find
// a "single server" condition in the cache (i.e. one server is dirty, one
// server is offline etc), then we write the name of this server to this
// buffer.  Otherwise, the buffer remains unchanged.  The goal here is to 
// end up with a buffer containing the name of the applicable server when
// we have one of these one-server conditions.  Ultimately, the server name
// is included in the tray balloon text message.
//
// The function returns one of the STS_XXXXX UI status codes.
//
// This function is rather long.  Much longer than I like a function to be.
// I've tried to break it up into smaller pieces but any chunks were pretty
// much arbitrary.  Without a good logical breakdown, that doesn't make much
// sense.  Even with it's length, it's not a complex function.  It merely 
// enumerates shares in the cache gathering statistics along the way.  From
// these statistics, it decides what the next UI state should be.
//
eSysTrayState
CStateMachine::TranslateInput(
    UINT uMsg,
    LPTSTR pszServer,
    UINT cchServer
    )
{
    //
    // Since this cscui code is running all the time, we don't want to keep 
    // a handle to the event log open.  Therefore, we use this CscuiEventLog
    // object to automatically close the log for us.  The ReportEvent member
    // of CscuiEventLog handles all initialization of the log and determining
    // if the event should actually be logged (depending upon the current CSCUI
    // event logging level).
    //
    CscuiEventLog log;
    bool bServerIsBack = false;
  
    if (STWM_CSCNETUP == uMsg)
    {
        m_bNoNet = false;
        if (TEXT('\0') != *pszServer)
        {
            STDBGOUT((1, TEXT("Translating STWM_CSCNETUP for server \"%s\""), pszServer));
            //
            // Server reported back by the CSC agent.
            // Add it's name to a persistent (in memory) list of
            // servers available for reconnection.
            // Also clear the "no net" flag.
            //
            bServerIsBack = true;
            ServerPendingReconnection(pszServer);
            if (log.LoggingEnabled())
            {
                log.Push(pszServer);
                log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_SERVER_AVAILABLE);
            }
        }
        else
        {
            STDBGOUT((1, TEXT("Translating STWM_CSCNETUP (no associated server)")));
            if (log.LoggingEnabled())
            {
                log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_NET_STARTED);
            }
        }
    }
    else if (STWM_CSCNETDOWN == uMsg)
    {
        //
        // This is the only place where transitions from online to
        // offline state are noted in the shell process. (CSCUISetState
        // and OnQueryNetDown execute in WinLogon's process).
        //
        if (TEXT('\0') != *pszServer)
        {
            STDBGOUT((1, TEXT("Translating STWM_CSCNETDOWN for server \"%s\""), pszServer));
            if (!m_bNoNet)
            {
                LPTSTR pszTemp;
                if (LocalAllocString(&pszTemp, pszServer))
                {
                    PostToSystray(PWM_REFRESH_SHELL, 0, (LPARAM)pszTemp);
                }                    
            }                
            //
            // Server reported down by the CSC agent.
            // Remove it's name from the persistent (in memory) list
            // of servers available for reconnection.
            //
            ServerUnavailable(pszServer);
            if (log.LoggingEnabled())
            {
                log.Push(pszServer);
                log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_SERVER_OFFLINE);
            }
        }
        else
        {
            STDBGOUT((1, TEXT("Translating STWM_CSCNETDOWN (no associated server)")));
            //
            // Entire network reported down by the CSC agent.
            // Remove all names from the persistent (in memory) list
            // of servers available for reconnection.  m_bNoNet is the only persistent
            // state we have.  Once it is set, the only thing that can reset it
            // is a STWM_CSCNETUP message from the CSC agent.
            //
            if (!m_bNoNet)
                PostToSystray(PWM_REFRESH_SHELL, 0, 0);
                
            m_bNoNet = true;
            AllServersUnavailable();
            if (log.LoggingEnabled())
            {
                log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_NET_STOPPED);
            }
        }
    }
    else if (STWM_STATUSCHECK == uMsg)
    {
        STDBGOUT((1, TEXT("Translating STWM_STATUSCHECK")));
    }
    else if (STWM_CACHE_CORRUPTED == uMsg)
    {
        //
        // Note:  No check for LoggingEnabled().  We always log corrupted cache
        //        regardless of logging level.
        //
        STDBGOUT((1, TEXT("Translating STWM_CACHE_CORRUPTED")));
        log.ReportEvent(EVENTLOG_ERROR_TYPE, 0, MSG_E_CACHE_CORRUPTED);
    }

    //
    // If CSC is disabled or the cache is empty, the default UI state
    // is "online".
    //
    eSysTrayState state = STS_ONLINE;
    if (IsCSCEnabled())
    {
        DWORD dwStatus;
        DWORD dwPinCount;
        DWORD dwHintFlags;
        WIN32_FIND_DATA fd;
        FILETIME ft;
        CCscFindHandle hFind;

        hFind = CacheFindFirst(NULL, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft);
        if (hFind.IsValid())
        {
            //
            // We need these three temporary name lists to reconcile a problem with
            // the way the CSC cache and RDR are designed.  When we enumerate the cache,
            // we enumerate individual shares in the cache.  Each share has some condition
            // (i.e. dirty, offline etc) associated with it.  The problem is that the
            // redirector handles things on a server basis.  So when a particular share
            // is offline, in reality the entire server is offline.  We've decided that
            // the UI should reflect things on a server (computer) basis so we need to 
            // avoid including the states of multiple shares from the same server in
            // our totals.  These three lists are used to store the names of servers
            // with shares in one of the three states (offline, dirty, pending recon).
            // If we enumerate a share with one of these states and find it already
            // exists in the corresponding list, we don't include this share in the
            // statistics.
            // 
            int cShares = 0;
            CServerList OfflineList;
            CServerList DirtyList;
            CServerList BackList;
            //
            // If a server is back, assume we can auto-reconnect it.
            //
            bool bAutoReconnectServer = bServerIsBack;
            TCHAR szAutoReconnectShare[MAX_PATH] = {0};
            DWORD dwPathSpeed = 0;

            do
            {
                bool bShareIsOnServer       = boolify(PathIsPrefix(pszServer, fd.cFileName));
                bool bShareHasModifiedFiles = false;
                bool bShareHasOpenFiles     = false;

                //
                // A share participates in the systray UI calculations only if the 
                // share contains files OR the share is currently "offline".  
                // Because of the CSC database design, CSC doesn't remove a share 
                // entry after all it's files have been removed from the cache. 
                // Therefore we need this extra check to avoid including empty shares in the UI.
                //
                if (ShareHasFiles(fd.cFileName, &bShareHasModifiedFiles, &bShareHasOpenFiles) ||
                    ShareIsOffline(dwStatus))
                {
                    cShares++;

                    if (bShareIsOnServer && (bShareHasModifiedFiles || bShareHasOpenFiles))
                    {
                        //
                        // Auto-reconnect isn't allowed if one or more shares on the server
                        // have open files or files modified offline.  Auto-reconnection
                        // would put the cache into a dirty state.
                        //
                        bAutoReconnectServer = false;
                    }

                    //
                    // A share can be in one of 4 states:
                    //     Online
                    //       Dirty
                    //     Offline
                    //       Pending reconnection ('back')
                    //
                    // Note that our definition of Dirty implies Online, and Pending
                    // Reconnection implies Offline.  That is, an offline share is
                    // never dirty and an online share is never pending reconnection.
                    //

                    //---------------------------------------------------------------------
                    // Is the share online?
                    //---------------------------------------------------------------------
                    if (!ShareIsOffline(dwStatus))
                    {
                        //---------------------------------------------------------------------
                        // Is the share dirty? (online + offline changes)
                        //---------------------------------------------------------------------
                        if (bShareHasModifiedFiles)
                        {
                            STDBGOUT((3, TEXT("Share \"%s\" is dirty (0x%08X)"), fd.cFileName, dwStatus));
                            DirtyList.Add(fd.cFileName);
                        }
                        else
                        {
                            STDBGOUT((3, TEXT("Share \"%s\" is online (0x%08X)"), fd.cFileName, dwStatus));
                        }
                    }
                    else    // Offline
                    {
                        //---------------------------------------------------------------------
                        // Is the server back?
                        //---------------------------------------------------------------------
                        if (IsServerPendingReconnection(fd.cFileName))
                        {
                            STDBGOUT((3, TEXT("Share \"%s\" is pending reconnection (0x%08X)"), fd.cFileName, dwStatus));
                            BackList.Add(fd.cFileName);
                        }
                        else
                        {
                            STDBGOUT((3, TEXT("Share \"%s\" is OFFLINE (0x%08X)"), fd.cFileName, dwStatus));
                            OfflineList.Add(fd.cFileName);
                        }
                    }
                }

                if (!ShareIsOffline(dwStatus))
                {
                    // It's online, so it can't be pending reconnection.
                    ServerReconnected(fd.cFileName);

                    // ...and there's no need to reconnect it.
                    if (bShareIsOnServer)
                        bAutoReconnectServer = false;
                }

                if (bAutoReconnectServer && bShareIsOnServer && TEXT('\0') == szAutoReconnectShare[0])
                {
                    //
                    // Remember the share name for possible auto-reconnection.
                    // The transition API is TransitionServerOnline but it takes a share name.
                    // Bad choice of names (IMO) but that's the way Shishir did it in the
                    // CSC APIs. It can be any share on the server.
                    //
                    // However, it's possible to have defunct shares in the
                    // database. Try to find one that's connectable.
                    //
                    if (CSCCheckShareOnlineEx(fd.cFileName, &dwPathSpeed))
                    {
                        STDBGOUT((3, TEXT("Share \"%s\" alive at %d00 bps"), fd.cFileName, dwPathSpeed));
                        lstrcpyn(szAutoReconnectShare, fd.cFileName, ARRAYSIZE(szAutoReconnectShare));
                    }
                    else
                    {
                        STDBGOUT((3, TEXT("Share \"%s\" unreachable, error = %d"), fd.cFileName, GetLastError()));
                    }
                }
            }
            while(CacheFindNext(hFind, &fd, &dwStatus, &dwPinCount, &dwHintFlags, &ft));

            if (bAutoReconnectServer)
            {
                //---------------------------------------------------------------------
                // Handle auto-reconnection.
                //---------------------------------------------------------------------
                //
                if (TEXT('\0') != szAutoReconnectShare[0])
                {
                    //
                    // Server was reported "BACK" by the CSC agent and it has no open files
                    // nor files modified offline and it's not on a slow link.  
                    // This makes it a candidate for automatic reconnection.  Try it.
                    //
                    STDBGOUT((1, TEXT("Attempting to auto-reconnect \"%s\""), szAutoReconnectShare));
                    if (TransitionShareOnline(szAutoReconnectShare, TRUE, TRUE, dwPathSpeed))
                    {
                        //
                        // The server has been reconnected.  Remove it's name from the 
                        // "pending reconnection" list.
                        //
                        ServerReconnected(pszServer);
                        //
                        // Remove this server from the temporary lists we've been keeping.
                        //
                        DirtyList.Remove(pszServer);
                        BackList.Remove(pszServer);
                        OfflineList.Remove(pszServer);

                        if (log.LoggingEnabled())
                        {
                            log.Push(pszServer);
                            log.ReportEvent(EVENTLOG_INFORMATION_TYPE, 0, MSG_I_SERVER_AUTORECONNECT);
                        }
                    }
                }
            }

            int cDirty   = DirtyList.Count();
            int cBack    = BackList.Count();
            int cOffline = OfflineList.Count();

            STDBGOUT((2, TEXT("Cache check server results: cShares = %d, cDirty = %d, cBack = %d, cOffline = %d"), 
                     cShares, cDirty, cBack, cOffline));

            //
            // This code path is a waterfall where lower-priority states are overwritten
            // by higher-priority states as they are encountered. The order of this array
            // is important.  It's ordered by increasing priority (no net is 
            // highest priority for systray UI).
            //
            CServerList *pServerList = NULL;
            struct Criteria
            {
                int           cnt;     // Number of applicable servers found.
                eSysTrayState state;   // Single-item UI state.
                eSysTrayState mstate;  // Multi-item UI state.
                CServerList *pList;    // Ptr to applicable list with server names.

            } rgCriteria[] = { 
                 { cOffline,                    STS_OFFLINE,    STS_MOFFLINE,    &OfflineList },
                 { cBack,                       STS_SERVERBACK, STS_MSERVERBACK, &BackList    },
                 { cDirty,                      STS_DIRTY,      STS_MDIRTY,      &DirtyList   },
                 { cShares && m_bNoNet ? 1 : 0, STS_NONET,      STS_NONET,       NULL         }
                 };

            for (int i = 0; i < ARRAYSIZE(rgCriteria); i++)
            {
                Criteria& c = rgCriteria[i];
                if (0 < c.cnt)
                {
                    state = c.mstate;
                    if (1 == c.cnt)
                    {
                        state = c.state;
                        pServerList = NULL;
                        if (NULL != c.pList && 1 == c.pList->Count())
                        {
                            pServerList = c.pList;
                        }
                    }
                }
            }
            if (NULL != pServerList)
            {
                //
                // We had a single-server condition so write the server name
                // to the caller's server name buffer.
                // If we didn't have a single-server condition, the buffer
                // remains unchanged.
                //
                lstrcpyn(pszServer, pServerList->Get(0), cchServer);
            }
        }
    }

    STDBGOUT((1, TEXT("Translated to SysTray UI state %s"), SysTrayStateStr(state)));
    return state;
}

//
// Ping offline servers. If any are alive, update status and
// auto-reconnect them if possible.  This is typically done
// after a sync operation has completed.
//
DWORD WINAPI
_PingServersThread(LPVOID /*pThreadData*/)
{
    DWORD dwStatus;
    WIN32_FIND_DATA fd;
    HANDLE hFind;

    hFind = CacheFindFirst(NULL, &fd, &dwStatus, NULL, NULL, NULL);
    if (INVALID_HANDLE_VALUE != hFind)
    {
        CServerList BackList;

        do
        {
            // If the tray state becomes Online or NoNet, we can quit
            eSysTrayState state = (eSysTrayState)SendToSystray(PWM_QUERY_UISTATE, 0, 0);
            if (STS_ONLINE == state || STS_NONET == state)
                break;

            // Call BackList.Exists here to avoid extra calls to
            // CSCCheckShareOnline. (Add also calls Exists)
            if ((FLAG_CSC_SHARE_STATUS_DISCONNECTED_OP & dwStatus) &&
                !BackList.Exists(fd.cFileName))
            {
                if (!CSCCheckShareOnline(fd.cFileName))
                {
                    DWORD dwErr = GetLastError();
                    if (ERROR_ACCESS_DENIED != dwErr &&
                        ERROR_LOGON_FAILURE != dwErr)
                    {
                        // The share is not reachable
                        continue;
                    }
                    // Access denied or logon failure means the server is
                    // reachable, but we don't have valid credentials.
                }

                // The share is offline but available again.
                STDBGOUT((1, TEXT("Detected server back: %s"), fd.cFileName));
                BackList.Add(fd.cFileName);

                // Get the \\server name (minus the sharename) and
                // tell ourselves that it's back.
                LPCTSTR pszServer = BackList.Get(BackList.Count() - 1);
                if (pszServer)
                {
                    CSCUISetState(STWM_CSCNETUP, 0, (LPARAM)pszServer);
                }
            }
        }
        while(CacheFindNext(hFind, &fd, &dwStatus, NULL, NULL, NULL));

        CSCFindClose(hFind);
    }

    DllRelease();
    FreeLibraryAndExitThread(g_hInstance, 0);
    return 0;
}

void
CStateMachine::PingServers()
{
    // Don't bother trying if there's no net.
    if (!m_bNoNet)
    {
        DWORD dwThreadID;

        // Give the thread a reference to the DLL
        HINSTANCE hInstThisDll = LoadLibrary(c_szDllName);
        DllAddRef();

        HANDLE hThread = CreateThread(NULL,
                                      0,
                                      _PingServersThread,
                                      NULL,
                                      0,
                                      &dwThreadID);
        if (hThread)
        {
            CloseHandle(hThread);
        }
        else
        {
            // CreateThread failed, cleanup
            DllRelease();
            FreeLibrary(hInstThisDll);
        }
    }
}

//
// Determine if a given share has files cached in the CSC cache.
// 
//
bool
CStateMachine::ShareHasFiles(
    LPCTSTR pszShare,
    bool *pbModified,
    bool *pbOpen
    ) const
{
    //
    // Exclude the following:
    //   1. Directories.
    //   2. Files marked as "locally deleted".
    //
    // NOTE:  The filtering done by this function must be the same as 
    //        in several other places throughout the CSCUI code.
    //        To locate these, search the source for the comment
    //        string CSCUI_ITEM_FILTER.
    //
    const DWORD fExclude = SSEF_LOCAL_DELETED | 
                           SSEF_DIRECTORY;
    //
    // Stop stats enumeration when we've found all of the following:
    //   1. At least one file.
    //   2. At least one modified file.
    //   3. At least one file with either USER access OR GUEST access.
    //
    const DWORD fUnity   = SSUF_TOTAL | 
                           SSUF_MODIFIED | 
                           SSUF_ACCUSER | 
                           SSUF_ACCGUEST | 
                           SSUF_ACCOR;

    CSCSHARESTATS ss;
    CSCGETSTATSINFO si = { fExclude, fUnity, true, false };
    _GetShareStatisticsForUser(pszShare, // Share name.
                               &si,
                               &ss);     // Destination buffer.

    if (NULL != pbModified)
    {
        *pbModified = (0 < ss.cModified);
    }
    if (NULL != pbOpen)
    {
        *pbOpen = ss.bOpenFiles;
    }

    return 0 < ss.cTotal;
}



//-----------------------------------------------------------------------------
// CSysTrayUI member functions.
//-----------------------------------------------------------------------------
//
// This is the minimum interval (in ms) allowed between state changes of
// the systray UI.  A value of 0 would result in immediate updates as 
// notifications are received from the CSC agent.  A value of 60000 would
// cause any state changes received less than 60 seconds after the previous
// state change to be queued.  60 seconds after the previous state change, 
// if a state change is queued it is applied to the systray UI.
// Something to consider is dynamically adjusting 
//
const int CSysTrayUI::s_iMinStateChangeInterval = 10000; // 10 seconds.


CSysTrayUI::CSysTrayUI(
    HWND hwndNotify
    ) : m_idFlashingTimer(0),
        m_idReminderTimer(0),
        m_idStateChangeTimer(0),
        m_iIconFlashTime(GetCaretBlinkTime()),
        m_hIconNoOverlay(s_rgIconInfo[int(STS_OFFLINE)].hIcon), // The offline icon is used
                                                                // as the non-overlay icon for 
                                                                // flashing.
        m_hwndNotify(hwndNotify),
        m_dwFlashingExpires(0),
        m_dwNextStateChange(0),
        m_state(STS_ONLINE),
        m_statePrev(STS_INVALID),
        m_stateQueued(STS_INVALID),
        m_bFlashOverlay(false),
        m_bActive(false)
{
    //
    // Load up the required icons.
    //
    for (int i = 0; i < ARRAYSIZE(s_rgIconInfo); i++)
    {
        IconInfo& sti = s_rgIconInfo[i];
        if (NULL == sti.hIcon && 0 != sti.idIcon)
        {
            sti.hIcon = (HICON)LoadImage(g_hInstance, 
                                         MAKEINTRESOURCE(sti.idIcon),
                                         IMAGE_ICON, 
                                         CSC_ICON_CX, 
                                         CSC_ICON_CY, 
                                         LR_LOADMAP3DCOLORS);
                        
            if (NULL == sti.hIcon)
            {
                Trace((TEXT("CSCUI ERROR %d loading Icon ID = %d"), GetLastError(), sti.idIcon));
            }
        }
    }
    m_szServer[0] = TEXT('\0');
    m_szServerQueued[0] = TEXT('\0');

    UpdateSysTray(UF_ICON);
}


CSysTrayUI::~CSysTrayUI(
    void
    )
{
    if (0 != m_idStateChangeTimer)
        KillTimer(m_hwndNotify, m_idStateChangeTimer);
}

//
// Singleton instance access.
//
CSysTrayUI& 
CSysTrayUI::GetInstance(
    void
    )
{
    static CSysTrayUI TheUI(_FindNotificationWindow());
    return TheUI;
}


//
// Change the current state of the UI to a new state.
// Returns:
//      true    = state was changed.
//      false   = state was not changed.
//
bool
CSysTrayUI::SetState(
    eSysTrayState state,
    LPCTSTR pszServer      // Optional.  Default is NULL.
    )
{
    bool bResult = false;
    //
    // Apply a state change only if the state has actually changed.
    //
    if (state != m_state)
    {
        //
        // Apply a state change only if there's not a sync in progress.
        // If there is a sync in progress, we'll receive a CSCWM_DONESYNCING
        // message when the sync is finished which will trigger a UI update.
        //
        if (!::IsSyncInProgress())
        {
            if (0 == m_idStateChangeTimer)
            {
                //
                // The state change timer is not active.  That means it's OK
                // to update the tray UI.
                //
                STDBGOUT((1, TEXT("Changing SysTray UI state %s -> %s"), 
                                    SysTrayStateStr(m_state),
                                    SysTrayStateStr(state)));

                m_statePrev = m_state;
                m_state     = state;
                UpdateSysTray(eUpdateFlags(UF_ICON | UF_BALLOON), pszServer);

                //
                // Reset the state change timer so that we will not produce a
                // visible change in the tray UI for at least another 
                // s_iMinStateChangeInterval milliseconds.
                // Also invalidate the queued state info so that if the update timer
                // expires before we queue a state change, it will be a no-op.
                //
                STDBGOUT((2, TEXT("Setting state change timer")));

                m_stateQueued = STS_INVALID;
                m_idStateChangeTimer = SetTimer(m_hwndNotify,
                                                ID_TIMER_STATECHANGE,
                                                s_iMinStateChangeInterval,
                                                StateChangeTimerProc);
                bResult  = true;
            }
            else
            {
                //
                // The state change timer is active so we can't update the tray
                // UI right now.  We'll queue up the state information so when the
                // timer expires this state will be applied.  Note that the "queue"
                // is only ONE item deep.  Each successive addition to the queue
                // overwrites the current content.
                //
                STDBGOUT((2, TEXT("Queueing state change to %s."), SysTrayStateStr(state)));
                m_stateQueued = state;
                if (NULL != pszServer)
                {
                    lstrcpyn(m_szServerQueued, pszServer, ARRAYSIZE(m_szServerQueued));
                }
                else
                {
                    m_szServerQueued[0] = TEXT('\0');
                }
            }
        }
        else
        {
            STDBGOUT((2, TEXT("Sync in progress.  SysTray state not changed.")));
        }
    }
    return bResult;
}



//
// Called each time the state change timer expires.
//
VOID CALLBACK 
CSysTrayUI::StateChangeTimerProc(
    HWND hwnd, 
    UINT uMsg, 
    UINT_PTR idEvent, 
    DWORD dwTime
    )
{
    //
    // Call a non-static function of the singleton instance so
    // we have access to private members.
    //
    CSysTrayUI::GetInstance().OnStateChangeTimerExpired();
}


void
CSysTrayUI::OnStateChangeTimerExpired(
    void
    )
{
    STDBGOUT((2, TEXT("State change timer expired. Queued state = %s"), 
             SysTrayStateStr(m_stateQueued)));

    //
    // Kill the timer and set it's ID to 0.
    // This will let SetState() know that the timer has expired and
    // it's OK to update the tray UI.
    //
    if (0 != m_idStateChangeTimer)
    {
        KillTimer(m_hwndNotify, m_idStateChangeTimer);
        m_idStateChangeTimer = 0;
    }

    if (int(m_stateQueued) != int(STS_INVALID))
    {
        //
        // Call SetState ONLY if queued info is valid; meaning
        // there was something in the queue.
        //
        SetState(m_stateQueued, m_szServerQueued);
    }
}



//
// On WM_WININICHANGED update the icon flash timer.
//
void
CSysTrayUI::OnWinIniChange(
    LPCTSTR pszSection
    )
{
    m_iIconFlashTime = GetCaretBlinkTime();
    KillIconFlashTimer();
    UpdateSysTray(UF_FLASHICON);
}


//
// Show the reminder balloon associated with the current UI state.
//
void 
CSysTrayUI::ShowReminderBalloon(
    void
    )
{
    UpdateSysTray(eUpdateFlags(UF_BALLOON | UF_REMINDER));
}


   
//
// All roads lead here.
// This function is the kitchen sink for updating the systray.
// It's kind of a long function but it centralizes all changes to 
// the systray.  It's divided into 3 basic parts:
//
//  1. Change the tray icon.           (UF_ICON)
//  2. Flash the tray icon.            (UF_FLASHICON)
//  3. Display a notification balloon. (UF_BALLOON)
//  
// Part or all of these can be performed in a single call depending
// upon the content of the uFlags argument.
//
void 
CSysTrayUI::UpdateSysTray(
    eUpdateFlags uFlags,
    LPCTSTR pszServer       // optional.  Default is NULL.
    )
{
    NOTIFYICONDATA nid = {0};

    if (!IsWindow(m_hwndNotify))
        return;
    //
    // If an icon is active, we're modifying it.
    // If none active, we're adding one.
    //        
    DWORD nimsg = NIM_MODIFY;

    nid.cbSize           = sizeof(NOTIFYICONDATA);
    nid.uID              = PWM_TRAYCALLBACK;
    nid.uFlags           = NIF_MESSAGE;
    nid.uCallbackMessage = PWM_TRAYCALLBACK;
    nid.hWnd             = m_hwndNotify;

    IconInfo& sti = s_rgIconInfo[int(m_state)];

    if (NULL != pszServer && TEXT('\0') != *pszServer)
    {
        //
        // Copy the name of the server to a member variable.
        // Skip passed the leading "\\".
        //
        while(*pszServer && TEXT('\\') == *pszServer)
            pszServer++;

        lstrcpyn(m_szServer, pszServer, ARRAYSIZE(m_szServer));
    }

    //
    // Change the icon --------------------------------------------------------
    //
    if (UF_ICON & uFlags)
    {
        nid.uFlags |= NIF_ICON;
        if (0 == sti.idIcon)
        {
            //
            // This state doesn't have an icon.  Delete from systray.
            //
            nimsg = NIM_DELETE;
        }
        else
        {
            if (!m_bActive)
                nimsg = NIM_ADD;

            nid.hIcon = sti.hIcon;
            //
            // If applicable, always flash icon when first showing it.
            //
            uFlags = eUpdateFlags(uFlags | UF_FLASHICON);
            //
            // Set the tooltip.
            //
            nid.uFlags |= NIF_TIP;
            GetTooltipText(m_state, nid.szTip, ARRAYSIZE(nid.szTip));
        }
        m_bFlashOverlay = false;
        KillIconFlashTimer();
    }

    //
    // Flash the icon ---------------------------------------------------------
    //
    if (UF_FLASHICON & uFlags)
    {
        if (0 != sti.iFlashTimeout)
        {
            nid.uFlags |= NIF_ICON; // Flashing is actually displaying a new icon.
            //
            // This icon is a flashing icon.
            //
            if (0 == m_idFlashingTimer)
            {
                //
                // No timer started yet.  Start one.
                //
                STDBGOUT((2, TEXT("Starting icon flash timer.  Time = %d ms"), m_iIconFlashTime));
                m_idFlashingTimer = SetTimer(m_hwndNotify, 
                                             ID_TIMER_FLASHICON, 
                                             m_iIconFlashTime,
                                             FlashTimerProc);
                if (0 != m_idFlashingTimer)
                {
                    //
                    // Set the tick-count when the timer expires.
                    // An expiration time of (-1) means it never expires.
                    //
                    if (ICONFLASH_FOREVER != sti.iFlashTimeout)
                        m_dwFlashingExpires = GetTickCount() + sti.iFlashTimeout;
                    else
                        m_dwFlashingExpires = ICONFLASH_FOREVER;
                }
            }
            nid.hIcon = m_bFlashOverlay ? sti.hIcon : m_hIconNoOverlay;

            m_bFlashOverlay = !m_bFlashOverlay; // Toggle flash state.
        }
    }

    //
    // Update or hide the balloon ---------------------------------------------
    //
    if (UF_BALLOON & uFlags)
    {
        //
        // If there's no balloon text mapped to the current UI state and these
        // balloon flags, any current balloon will be destroyed.  This is because
        // the tray code destroys the current balloon before displaying the new one
        // and it doesn't display a new one if it's passed a blank string.
        //
        nid.uFlags |= NIF_INFO;
        DWORD dwBalloonFlags = (UF_REMINDER & uFlags) ? BTF_REMIND : BTF_INITIAL;
        GetBalloonInfo(m_state, 
                       dwBalloonFlags, 
                       nid.szInfoTitle,
                       ARRAYSIZE(nid.szInfoTitle),
                       nid.szInfo, 
                       ARRAYSIZE(nid.szInfo), 
                       &nid.dwInfoFlags,
                       &nid.uTimeout);
        //
        // Any time we show a balloon, we reset the reminder timer.  
        // This is so that we don't get a balloon resulting from a state change
        // immediately followed by a reminder balloon because the reminder
        // timer expired.
        //
        bool bRestartReminderTimer = (BTF_REMIND == dwBalloonFlags && TEXT('\0') != nid.szInfo[0]) ||
                                     StateHasBalloonText(m_state, BTF_REMIND);

        ResetReminderTimer(bRestartReminderTimer);
    }
    //
    // Notify the systray -----------------------------------------------------
    //
    if (NIM_DELETE == nimsg)
        m_bActive = false;

    if (Shell_NotifyIcon(nimsg, &nid))
    {
        if (NIM_ADD == nimsg)
            m_bActive = true;
    }
}

//
// Get the balloon text associated with a given systray UI state and with
// a given set of BTF_XXXXX (Balloon Text Flag) flags.  The information 
// is stored in the table s_rgBalloonInfo[].  The text and balloon timeout
// are returned in caller-provided buffers.
//
// The balloon text follows this format:
//
//    <Header> <Status> \n
// 
//    <Body>
//
//    <Directive>
//
// An example would be:
//
//    Offline Files - Network Connection Lost
//
//    The network connection to '\\worf' has been lost.
//
//    Click here to view status.
//
// state is one of the STS_XXXXX flags.
// dwTextFlags is a mask of BTF_XXXXX flag bits.
//
void
CSysTrayUI::GetBalloonInfo(
    eSysTrayState state,
    DWORD dwTextFlags,
    LPTSTR pszTextHdr,
    int cchTextHdr,
    LPTSTR pszTextBody,
    int cchTextBody,
    DWORD *pdwInfoFlags,
    UINT *puTimeout
    )
{
    *pszTextHdr  = TEXT('\0');
    *pszTextBody = TEXT('\0');

    if (SupressBalloon(m_statePrev, state))
    {
        STDBGOUT((3, TEXT("Balloon supressed")));
        return;
    }

    int i = GetBalloonInfoIndex(state, dwTextFlags);
    if (-1 != i)
    {
        BalloonInfo& bi = s_rgBalloonInfo[i];

        //
        // BUGBUG:  Review these buffer sizes.   Allow for localization!
        //
        TCHAR szHeader[80];
        TCHAR szStatus[80];
        TCHAR szDirective[80];
        TCHAR szBody[MAX_PATH];
        TCHAR szFmt[MAX_PATH];
          
        if (STS_OFFLINE == state || STS_DIRTY == state || STS_SERVERBACK == state)
        {
            //
            // State has only one server associated with it so that means we'll
            // be including it in the balloon text body.  Load the format
            // string from a text resource and embed the server name in it.
            //
            LPTSTR rgpstr[] = { m_szServer };
            LoadString(g_hInstance, bi.idBody, szFmt, ARRAYSIZE(szFmt));
            FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                          FORMAT_MESSAGE_ARGUMENT_ARRAY,
                          szFmt,
                          0,0,
                          szBody,
                          ARRAYSIZE(szBody),
                          (va_list *)rgpstr);
        }
        else
        {
            //
            // State has multiple servers associated with it so that means
            // there's no name embedded in the body.  It's just a simple string
            // loaded from a text resource.
            //
            LoadString(g_hInstance, bi.idBody, szBody, ARRAYSIZE(szBody));
        }

        //
        // Create the header text.
        //
        LoadString(g_hInstance, IDS_BALLOONHDR_FORMAT, szFmt, ARRAYSIZE(szFmt));
        LoadString(g_hInstance, bi.idHeader, szHeader, ARRAYSIZE(szHeader));
        LoadString(g_hInstance, bi.idStatus, szStatus, ARRAYSIZE(szStatus));

        LPTSTR rgpstrHdr[] = { szHeader,
                               szStatus };

        FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szFmt,
                      0,0,
                      pszTextHdr,
                      cchTextHdr,
                      (va_list *)rgpstrHdr);
        //
        // Create the body text.
        //
        LoadString(g_hInstance, IDS_BALLOONBODY_FORMAT, szFmt, ARRAYSIZE(szFmt));
        LoadString(g_hInstance, bi.idDirective, szDirective, ARRAYSIZE(szDirective));
        LPTSTR rgpstrBody[] = { szBody,
                                szDirective };

        FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                      szFmt,
                      0,0,
                      pszTextBody,
                      cchTextBody,
                      (va_list *)rgpstrBody);

        if (NULL != pdwInfoFlags)
        {
            *pdwInfoFlags = bi.dwInfoFlags;
        }

        if (NULL != puTimeout)
        {
            CConfig& config = CConfig::GetSingleton();
            //
            // Balloon timeout is stored in the registry.
            //
            UINT uTimeout = (BTF_INITIAL & dwTextFlags) ? config.InitialBalloonTimeoutSeconds() :
                                                          config.ReminderBalloonTimeoutSeconds();
            *puTimeout = uTimeout * 1000;
        }
    }
}

//
// Find the index in s_rgBalloonInfo[] for a given state
// and BTF_XXXXXX flag.
// Returns -1 if no match in array.
//
int
CSysTrayUI::GetBalloonInfoIndex(
    eSysTrayState state,
    DWORD dwTextFlags
    )
{
    //
    // Scan the balloon info table until we find a record for the 
    // specified systray UI state and BTF flags.
    //
    for (int i = 0; i < ARRAYSIZE(s_rgBalloonInfo); i++)
    {
        BalloonInfo& bi = s_rgBalloonInfo[i];
        if (bi.state == state && 
            bi.dwTextFlags == dwTextFlags &&
            0 != bi.idHeader &&
            0 != bi.idStatus &&
            0 != bi.idBody &&
            0 != bi.idDirective)
        {
            return i;
        }
    }
    return -1;
}


    
//
// Determine if a balloon should not be displayed for a particular
// UI state transition.
//
bool
CSysTrayUI::SupressBalloon(
    eSysTrayState statePrev,
    eSysTrayState state
    )
{
    for (int i = 0; i < ARRAYSIZE(s_rgBalloonSupression); i++)
    {
        if (statePrev == s_rgBalloonSupression[i].stateFrom &&
            state     == s_rgBalloonSupression[i].stateTo)
        {
            return true;
        }
    }
    return false;
}



//
// Do we have balloon text for a given state and balloon style?
// state is one of the STS_XXXXX flags.
// dwTextFlags is a mask of BTF_XXXXX flag bits.
//
bool 
CSysTrayUI::StateHasBalloonText(
    eSysTrayState state,
    DWORD dwTextFlags
    )
{
    return (-1 != GetBalloonInfoIndex(state, dwTextFlags));
}



LPTSTR 
CSysTrayUI::GetTooltipText(
    eSysTrayState state,
    LPTSTR pszText,
    int cchText
    )
{
    *pszText = TEXT('\0');
    //
    // Scan the tooltip info table until we find a record for the 
    // specified systray UI state.
    //
    for (int i = 0; i < ARRAYSIZE(s_rgTooltipInfo); i++)
    {
        TooltipInfo& tti = s_rgTooltipInfo[i];
        if (tti.state == state && 0 != tti.idTooltip)
        {
            TCHAR szTemp[MAX_PATH];
            int cchHeader = LoadString(g_hInstance, IDS_TT_HEADER, szTemp, ARRAYSIZE(szTemp));
            if (STS_OFFLINE == state || STS_DIRTY == state || STS_SERVERBACK == state)
            {
                //
                // State has only one server associated with it so that means we'll
                // be including it in the tooltip text.  Embed the server name in it.
                //
                TCHAR szFmt[160];
                LPTSTR rgpstr[] = { m_szServer };
                LoadString(g_hInstance, tti.idTooltip, szFmt, ARRAYSIZE(szFmt));
                FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                              FORMAT_MESSAGE_ARGUMENT_ARRAY,
                              szFmt,
                              0,0,
                              szTemp + cchHeader,
                              ARRAYSIZE(szTemp) - cchHeader,
                              (va_list *)rgpstr);
            }
            else
            {
                //
                // State has multiple servers associated with it so that means
                // there's no name embedded in the tooltip.  It's just a simple string
                // loaded from a text resource.
                //
                LoadString(g_hInstance, 
                           tti.idTooltip, 
                           szTemp + cchHeader, 
                           ARRAYSIZE(szTemp) - cchHeader);
            }
            lstrcpyn(pszText, szTemp, cchText);
        }
    }
    return pszText;
}


//
// Stop the flashing icon by killing the timer.
//
void 
CSysTrayUI::KillIconFlashTimer(
    void
    )
{
    //
    // Force a final update so we're displaying the proper icon then
    // kill the timer.
    //
    if (0 != m_idFlashingTimer)
    {
        KillTimer(m_hwndNotify, m_idFlashingTimer);
        m_idFlashingTimer = 0;
    }
}

//
// Called by the OS each time the icon flash timer period expires.
// I use this rather than handling a WM_TIMER message so that
// timer processing is contained within the CSysTrayUI class.
//
VOID CALLBACK 
CSysTrayUI::FlashTimerProc(
    HWND hwnd,
    UINT uMsg, 
    UINT_PTR idEvent, 
    DWORD dwTime
    )
{
    CSysTrayUI::GetInstance().HandleFlashTimer();
}


void
CSysTrayUI::HandleFlashTimer(
    void
    )
{
    if (IconFlashedLongEnough())
    {
        //
        // Kill the icon flashing timer and the icon will stop flashing.
        // This doesn't actually kill the timer yet.
        //
        STDBGOUT((2, TEXT("Killing icon flash timer")));
        m_bFlashOverlay = true;
        UpdateSysTray(UF_FLASHICON);
        KillIconFlashTimer();
    }
    else
    {
        //
        // The CSysTrayUI instance maintains all information
        // needed to cycle the icon.  Just tell it to update
        // the icon and it'll do the right thing.
        //
        UpdateSysTray(UF_FLASHICON);
    }
}


//
// Determine if the flashing icon has flashed enough.
//
bool 
CSysTrayUI::IconFlashedLongEnough(
    void
    )
{
    return ICONFLASH_FOREVER != m_dwFlashingExpires && 
           GetTickCount() >= m_dwFlashingExpires;
}


//
// Stop and restart the reminder timer.
// If bRestart is false, the timer is killed and not restarted.
// If bRestart is true, the timer is killed and a new one restarted.
//
void 
CSysTrayUI::ResetReminderTimer(
    bool bRestart
    )
{
    CConfig& config = CConfig::GetSingleton();
    if (!config.NoReminders())
    {
        int cReminderInterval = (config.ReminderFreqMinutes() * 1000 * 60);
        //
        // Force a final update so we're displaying the proper icon then
        // kill the timer.
        //
        if (0 != m_idReminderTimer)
        {
            KillTimer(m_hwndNotify, m_idReminderTimer);
            m_idReminderTimer = 0;
        }
        //
        // No timer started yet.  Start one.
        //
        if (bRestart && 0 < cReminderInterval)
        {
            STDBGOUT((2, TEXT("Starting reminder timer.  Timeout = %d ms"), cReminderInterval));
            m_idReminderTimer = SetTimer(m_hwndNotify, 
                                        ID_TIMER_REMINDER, 
                                        cReminderInterval, 
                                        ReminderTimerProc);
        }
    }
}


//
// Called by the OS each time the reminder timer period expires.
// I use this rather than handling a WM_TIMER message so that
// timer processing is contained within the CSysTrayUI class.
//
VOID CALLBACK 
CSysTrayUI::ReminderTimerProc(
    HWND hwnd, 
    UINT uMsg, 
    UINT_PTR idEvent, 
    DWORD dwTime
    )
{
    STDBGOUT((2, TEXT("Showing reminder balloon")));
    CSysTrayUI::GetInstance().ShowReminderBalloon();
}


//
// Called by the systray WndProc whenever the state of the systray should be
// updated.
//
// hWnd   - HWND of the systray notification window.
//
// stwmMsg - STWM_CSCNETUP     (Net or server is available for reconnect)
//           STWM_CSCNETDOWN   (Net or server is unavailable)
//           STWM_STATUSCHECK  (Check cache state and update systray)
//
// pszServer - non-NULL means CSC agent passed a server name
//      associated with the STWM_XXXX message.
//      This means there was a single server associated with the event
//      rather than multiple servers or the entire net interface.
//
void 
UpdateStatus(
    CStateMachine *pSM,
    HWND hWnd, 
    UINT stwmMsg,
    LPTSTR pszServer
    )
{
    TraceEnter(TRACE_CSCST, "UpdateStatus");
    TraceAssert(NULL != hWnd);

    TCHAR szServerName[MAX_PATH] = { 0 };

    if (pszServer)
    {
        lstrcpyn(szServerName, pszServer, ARRAYSIZE(szServerName));
    }

    //
    // Translate the CSC agent inputs into a new systray UI state.
    //
    eSysTrayState state = pSM->TranslateInput(stwmMsg, szServerName, ARRAYSIZE(szServerName));

    //
    // Get reference to the singleton UI object and tell it to set the state.
    // Note that it remembers all current UI state and will only actually
    // update the systray if the UI state has changed.  Here we can 
    // blindly tell it to update state.  It will only do what's necessary.
    //
    CSysTrayUI::GetInstance().SetState(state, szServerName);
    TraceLeaveVoid();
}



///////////////////////////////////////////////////////////////////////////////
// _CreateMenu()
//
// Create context menu
//
HMENU _CreateMenu()
{
    HMENU hmenu = NULL;
    
    TraceEnter(TRACE_CSCST, "_CreateMenu");

    hmenu = CreatePopupMenu();
    if (NULL != hmenu)
    {
        CConfig& config = CConfig::GetSingleton();     
        TCHAR szTemp[MAX_PATH];
        //
        // Add the "Status" verb.
        //
        LoadString(g_hInstance, IDS_CSC_CM_STATUS, szTemp, ARRAYSIZE(szTemp));
        AppendMenu(hmenu, MF_STRING, PWM_STATUSDLG, szTemp);

        //
        // Add the "Synchronize" verb
        //
        LoadString(g_hInstance, IDS_CSC_CM_SYNCHRONIZE, szTemp, ARRAYSIZE(szTemp));
        AppendMenu(hmenu, MF_STRING, CSCWM_SYNCHRONIZE, szTemp);
        if (!config.NoCacheViewer())
        {
            //
            // Add the "View files" verb
            //
            LoadString(g_hInstance, IDS_CSC_CM_SHOWVIEWER, szTemp, ARRAYSIZE(szTemp));
            AppendMenu(hmenu, MF_STRING, CSCWM_VIEWFILES, szTemp);
        }
        if (!config.NoConfigCache())
        {
            //
            // Add the "Settings" verb
            //
            LoadString(g_hInstance, IDS_CSC_CM_SETTINGS, szTemp, ARRAYSIZE(szTemp));
            AppendMenu(hmenu, MF_STRING, CSCWM_SETTINGS, szTemp);
        }
        //
        // Left clicking the systray icon invokes the status dialog.
        // Therefore, the "Status" verb is our default and must be in bold text.
        //
        SetMenuDefaultItem(hmenu, PWM_STATUSDLG, MF_BYCOMMAND);
    }

    TraceLeaveValue(hmenu);
}

///////////////////////////////////////////////////////////////////////////////
// _ShowMenu()
//
UINT _ShowMenu(HWND hWnd, UINT uMenuNum, UINT uButton)
{
    UINT    iCmd = 0;
    HMENU   hmenu;

    TraceEnter(TRACE_CSCST, "_ShowMenu");

    hmenu = _CreateMenu();
    if (hmenu)
    {
        POINT   pt;

        GetCursorPos(&pt);
        SetForegroundWindow(hWnd);
        iCmd = TrackPopupMenu(hmenu,
                              uButton | TPM_RETURNCMD | TPM_NONOTIFY,
                              pt.x,
                              pt.y,
                              0,
                              hWnd,
                              NULL);
        DestroyMenu(hmenu);
    }

    TraceLeaveValue(iCmd);
}


//
// This function is used to ensure that we don't try to process 
// a WM_RBUTTONUP and WM_LBUTTONUP message at the same time.
// May be a little paranoid.
//
LRESULT
OnTrayIconSelected(
    HWND hWnd,
    UINT uMsg
    )
{
    static LONG bHandling = 0;
    LRESULT lResult = 0;

    if (0 == InterlockedCompareExchange(&bHandling, 1, 0))
    {
        UINT iCmd = 0;
        switch (uMsg)
        {
            case WM_RBUTTONUP:
                //
                // Context menu
                //
                iCmd = _ShowMenu(hWnd, 1, TPM_RIGHTBUTTON);
                break;

            case WM_LBUTTONUP:
                iCmd = PWM_STATUSDLG;
                break;

            default:
                break;

        }
        if (iCmd)
        {
            PostMessage(hWnd, iCmd, 0, 0);
            lResult = 1;
        }

        bHandling = 0;
    }
    return lResult;
}


///////////////////////////////////////////////////////////////////////////////
// _Notify() -- systray notification handler
//
LRESULT _Notify(HWND hWnd, WPARAM /*wParam*/, LPARAM lParam)
{
    LRESULT lResult = 0;
    switch (lParam)
    {
        case WM_RBUTTONUP:
        case WM_LBUTTONUP:
            lResult = OnTrayIconSelected(hWnd, (UINT)lParam);
            break;

        default:
            break;

    }
    return lResult;
}



bool IsServerBack(CStateMachine *pSM, LPCTSTR pszServer)
{
    TCHAR szServer[MAX_PATH];
    if (!PathIsUNC(pszServer))
    {
        //
        // Ensure servername uses UNC format.
        //
        wsprintf(szServer, TEXT("\\\\%s"), pszServer);
        pszServer = szServer;
    }
    return pSM->IsServerPendingReconnection(pszServer);
}


//
// Query CSC policy for the sync-at-logoff (quick vs. full)
// setting. If the policy is set, we enable SyncMgr's sync-at-logoff
// setting.  Without this the CSC policy could be set, the SyncMgr
// setting NOT set and the user wouldn't get sync-at-logoff as the 
// admin had anticipated.
//
void
ApplyCscSyncAtLogoffPolicy(
    void
    )
{
    bool bSetByPolicy = false;
    CConfig::GetSingleton().SyncAtLogoff(&bSetByPolicy);
    if (bSetByPolicy)
    {
        RegisterForSyncAtLogonAndLogoff(SYNCMGRREGISTERFLAG_PENDINGDISCONNECT,
                                        SYNCMGRREGISTERFLAG_PENDINGDISCONNECT);
    }
}



//
// WM_WININICHANGE handler.
//
LRESULT OnWinIniChange(
    HWND hwnd,
    WPARAM wParam,
    LPARAM lParam
    )
{
    STDBGOUT((1, TEXT("Rcvd WM_WININICHANGE.  wParam = %d, lParam = \"%s\""),
                     wParam, lParam ? (LPCTSTR)lParam : TEXT("<null>")));

    CSysTrayUI::GetInstance().OnWinIniChange((LPCTSTR)lParam);

    //
    // lParam == "Policy" for policy changes
    // wParam == 1 for machine policy, 0 for user policy
    //
    if (!lstrcmpi((LPTSTR)lParam, c_szPolicy))
    {
        ApplyCscSyncAtLogoffPolicy();
        ApplyAdminFolderPolicy();
    }
    return 0;
}

//
// Display the CSCUI status dialog.  Invoked when user either
// left-clicks the systray icon or selects the "Show Status" option
// from the systray context menu.
//
void
ShowCSCUIStatusDlg(
    HWND hwndParent
    )
{
    TCHAR szText[1024] = {0}; // This can be a fair amount of text.

    const struct
    {
        eSysTrayState state;  // SysTray UI state code.
        UINT idsText;         // Text for status dialog body.

    } rgMap[] = {{ STS_OFFLINE,      IDS_STATUSDLG_OFFLINE      },
                 { STS_MOFFLINE,     IDS_STATUSDLG_OFFLINE_M    },
                 { STS_SERVERBACK,   IDS_STATUSDLG_SERVERBACK   },
                 { STS_MSERVERBACK,  IDS_STATUSDLG_SERVERBACK_M },
                 { STS_DIRTY,        IDS_STATUSDLG_DIRTY        },
                 { STS_MDIRTY,       IDS_STATUSDLG_DIRTY_M      },
                 { STS_NONET,        IDS_STATUSDLG_NONET        }};

    CSysTrayUI& stui = CSysTrayUI::GetInstance();
    eSysTrayState state = stui.GetState();

    for (int i = 0; i < ARRAYSIZE(rgMap); i++)
    {
        if (state == rgMap[i].state)
        {
            LoadString(g_hInstance, rgMap[i].idsText, szText, ARRAYSIZE(szText));
            if (STS_DIRTY == state || STS_OFFLINE == state || STS_SERVERBACK == state)
            {
                LPCTSTR pszServerName = stui.GetServerName();
                if (NULL != pszServerName && TEXT('\0') != *pszServerName)
                {
                    //
                    // Current SysTray UI state has a single server associated
                    // with it.  The message will have the name embedded in
                    // it in 2 places.  Create a temp working buffer and 
                    // re-create the original string with the server name
                    // embedded.  If any of this fails, the string will just
                    // be displayed with the %1, %2 formatting characters rather
                    // than the server names.  Not a fatal problem IMO.
                    //
                    LPTSTR pszTemp = new TCHAR[lstrlen(szText) + 1];
                    if (NULL != pszTemp)
                    {
                        lstrcpy(pszTemp, szText);

                        LPCTSTR rgpstr[] = { pszServerName, pszServerName };
                        FormatMessage(FORMAT_MESSAGE_FROM_STRING |
                                      FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                      pszTemp,
                                      0,0,
                                      szText,
                                      ARRAYSIZE(szText),
                                      (va_list *)rgpstr);
                        delete[] pszTemp;
                    }
                }
            }
            break; // Break out of loop.  We have what we need.
        }
    }
    //
    // Display the dialog.
    //
    CStatusDlg::Create(hwndParent, szText, state);
}


//
// PWM_RESET_REMINDERTIMER handler.
//
void
OnResetReminderTimer(
    void
    )
{
    CSysTrayUI::GetInstance().ResetReminderTimer(true);
}


//
// Whenever we reboot, it's possible that the CSCUI cache has been
// reformatted or that the cache-size policy has been set/changed.
// When reformatted, the CSC agent uses the default size of 10%.  We 
// need to ensure that the size reflects system policy when policy
// is defined.  
//
void
InitCacheSize(
    void
    )
{
    bool bSetByPolicy = false;
    DWORD dwPctX10000 = CConfig::GetSingleton().DefaultCacheSize(&bSetByPolicy);

    if (bSetByPolicy)
    {
        ULARGE_INTEGER ulCacheSize;
        CSCSPACEUSAGEINFO sui;

        GetCscSpaceUsageInfo(&sui);
        //
        // BUGBUG:  Should we allow a 0% sized cache?  Not very useful
        //          but is it really invalid?   Maybe (pct > 10,000) is the
        //          only invalid condition.  [brianau - 8/24/98]
        //
        if (0 == dwPctX10000 || 10000 < dwPctX10000)
        {
            //
            // If value in registry is 0 or greater than 10000, it's
            // invalid.  Default to 10% of total disk space.
            //
            dwPctX10000 = 1000;  // Default to 10% (0.10 * 10,000)
        }
        ulCacheSize.QuadPart = (sui.llBytesOnVolume * dwPctX10000) / 10000i64;

        if (!CSCSetMaxSpace(ulCacheSize.HighPart, ulCacheSize.LowPart))
        {
            STDBGOUT((1, TEXT("Error %d setting cache size"), GetLastError()));
        }
    }
}


//
// Handler for CSCWM_SYNCHRONIZE.  Called when user clicks "Synchronize"
// option on systray context menu.  Also invoked when user selects the
// "Synchronize" button in a folder's web view pane.
//
HRESULT
OnSynchronize(
    void
    )
{
    //
    // This will create a status dialog hidden, invoke a synchronization of
    // servers that would be "checked" in the dialog then close the dialog
    // when the synchronization is complete.
    //
    CStatusDlg::Create(g_hWndNotification, 
                       TEXT(""), 
                       CSysTrayUI::GetInstance().GetState(), 
                       CStatusDlg::MODE_AUTOSYNC);
    return NOERROR;
}


LRESULT
OnQueryUIState(
    void
    )
{
    return CSysTrayUI::GetInstance().GetState();
}



//
// When a user profile has been removed from the local machine,
// the delete-profile code in userenv.dll will write the user's SID
// as a text string in the following reg key:
// 
// HKLM\Software\Microsoft\Windows\CurrentVersion\NetCache\PurgeAtNextLogoff
//
// Each SID is a value name under this key.
// At logoff, we enumerate all values under this key.  For each SID we
// instantiate a CCachePurger object and delete all files cached for this
// user.  Once the operation is complete, the "PurgeAtNextLogoff" key
// is deleted from the registry.
//
void
DeleteFilesCachedForObsoleteProfiles(
    void
    )
{
    HKEY hkeyNetcache;
    //
    // Open the "HKLM\...\NetCache" key.
    //
    LONG lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                c_szCSCKey,
                                0,
                                KEY_READ,
                                &hkeyNetcache);

    if (ERROR_SUCCESS == lResult)
    {
        HKEY hkey;
        //
        // Open the "PurgeAtNextLogoff" subkey.
        //
        lResult = RegOpenKeyEx(hkeyNetcache,
                               c_szPurgeAtNextLogoff,
                               0,
                               KEY_READ,
                               &hkey);

        if (ERROR_SUCCESS == lResult)
        {
            //
            // Enumerate all the SID strings.
            //
            int iValue = 0;
            TCHAR szValue[MAX_PATH];
            DWORD cchValue = ARRAYSIZE(szValue);
            while(ERROR_SUCCESS == SHEnumValue(hkey,
                                               iValue,
                                               szValue,
                                               &cchValue,
                                               NULL,
                                               NULL,
                                               NULL))
            {
                //
                // Convert each SID string to a SID and delete
                // all cached files accessed by this SID.
                // Purge files ONLY if the SID is NOT for the current
                // user.  Here's the deal...
                // While a user is NOT logged onto a system, their 
                // profile can be removed and their SID recorded in 
                // the PurgeAtNextLogoff key.  The next time they log on they
                // get new profile data.  If they're the next person to
                // logon following the removal of their profile, without 
                // this check, their new profile data would be purged during
                // the subsequent logoff.  That's bad.  Therefore, we never 
                // purge data for the user who is logging off.
                //
                PSID psid;
                if (ConvertStringSidToSid(szValue, &psid))
                {
                    if (!IsSidCurrentUser(psid))
                    {
                        CCachePurgerSel sel;
                        sel.SetFlags(PURGE_FLAG_ALL);
                        sel.SetUserSid(psid);

                        CCachePurger purger(sel, NULL, NULL);
                        purger.Delete();
                    }                        
                    FreeSid(psid);
                }
                iValue++;
                cchValue = ARRAYSIZE(szValue);
            }
            RegCloseKey(hkey);
            RegDeleteKey(hkeyNetcache, c_szPurgeAtNextLogoff);
        }
        RegCloseKey(hkeyNetcache);
    }
}



//
// This is called when the CSC hidden window is first created
// which occurs at logon.  It's just a general bucket to group the
// things that need to happen each logon.
//
void
HandleLogonTasks(
    void
    )
{   
    //
    // We must register our SyncMgr handler so that mobsync will
    // CoCreate it next logoff.
    //
    CscRegisterHandler(TRUE);
    InitCacheSize();
    ApplyCscSyncAtLogoffPolicy();
    ApplyAdminFolderPolicy();
}


//
// This is called when the CSC Agent (running in the winlogon process)
// tells us to uninitialize the CSC UI.  This happens when the user
// is logging off.
//
void
HandleLogoffTasks(
    void
    )
{
    CConfig& config = CConfig::GetSingleton();
    
    DeleteFilesCachedForObsoleteProfiles();

    if (config.PurgeAtLogoff())
    {
        //
        // If policy says to "purge all files cached by this user" 
        // delete offline-copy of all files cached by the current user.
        // Respects access bits in files so that we don't delete something
        // that is only used by some other user.  This is the same
        // behavior obtained via the "Delete Files..." button or the
        // disk cleaner.  Note that the UI callback ptr arg to the purger
        // ctor is NULL and we don't run through a "scan" phase.  This code
        // is run while the user is logging off so we don't display any
        // UI.  This deletes both auto-cached and pinned files.
        //
        CCachePurgerSel sel;
        sel.SetFlags(PURGE_FLAG_ALL);
        CCachePurger purger(sel, NULL, NULL);
        purger.Delete();
    }

    //
    // IMPORTANT:  We do any purging before registering for sync-at-logon/logoff.
    //             This is because we only register if we have something in 
    //             the cache.  Purging might remove all our cached items negating
    //             the need to register for synchronization.
    //

    bool bSetByPolicy = false;
    config.SyncAtLogoff(&bSetByPolicy);
    if (bSetByPolicy)
    {
        //
        // Sync-at-logoff (full vs. quick) policy is set for CSC.
        // Force SyncMgr's sync-at-logoff setting to ON.
        //
        RegisterForSyncAtLogonAndLogoff(SYNCMGRREGISTERFLAG_PENDINGDISCONNECT,
                                        SYNCMGRREGISTERFLAG_PENDINGDISCONNECT);
    }
    //
    // Is this the first time this user has used run CSCUI?
    //
    if (!IsSyncMgrInitialized())
    {
        CSCCACHESTATS cs;
        CSCGETSTATSINFO si = { SSEF_NONE, SSUF_TOTAL, false, false };
        if (_GetCacheStatisticsForUser(&si, &cs) && 0 < cs.cTotal)
        {
            //
            // This is the first time this user has logged off with
            // something in the cache.  Since SyncMgr doesn't turn on sync-at-logon/logoff
            // out of the box, we do it here.  This is because we want people to sync
            // if they have unknowingly cached files from an autocache share.
            // If successful the SyncMgrInitialized reg value is set to 1.
            //
            const DWORD dwFlags = SYNCMGRREGISTERFLAG_CONNECT | SYNCMGRREGISTERFLAG_PENDINGDISCONNECT;
            if (SUCCEEDED(RegisterForSyncAtLogonAndLogoff(dwFlags, dwFlags)))
            {
                SetSyncMgrInitialized();
            }
        }
    }
}


//
// Determines the status of a share for controlling the display of the
// webview in a shell folder.
//
// Returns one of the following codes (defined in cscuiext.h):
//
// CSC_SHARESTATUS_INACTIVE
// CSC_SHARESTATUS_ONLINE
// CSC_SHARESTATUS_OFFLINE
// CSC_SHARESTATUS_SERVERBACK
// CSC_SHARESTATUS_DIRTYCACHE
//
LRESULT
GetShareStatusForWebView(
    CStateMachine *pSM,
    LPCTSTR pszShare
    )
{
    LRESULT lResult = CSC_SHARESTATUS_INACTIVE;

    if (NULL != pszShare && IsCSCEnabled())
    {
        DWORD dwStatus;
        if (CSCQueryFileStatus(pszShare, &dwStatus, NULL, NULL))
        {
            if ((dwStatus & FLAG_CSC_SHARE_STATUS_CACHING_MASK) != FLAG_CSC_SHARE_STATUS_NO_CACHING)
            {
                const DWORD fExclude = SSEF_LOCAL_DELETED | 
                                       SSEF_DIRECTORY;

                CSCSHARESTATS stats;
                CSCGETSTATSINFO gsi = { fExclude, SSUF_MODIFIED, true, false };

                lResult = CSC_SHARESTATUS_ONLINE;

                if (_GetShareStatisticsForUser(pszShare, &gsi, &stats))
                {
                    if (stats.bOffline)
                    {
                        if (IsServerBack(pSM, pszShare))
                            lResult = CSC_SHARESTATUS_SERVERBACK;
                        else
                            lResult = CSC_SHARESTATUS_OFFLINE;
                    }
                    else
                    {
                        if (0 < stats.cModified)
                            lResult = CSC_SHARESTATUS_DIRTYCACHE;
                    }
                }
            }
        }
    }
    return lResult;
}


//
// This device-change code is an experiment to see what 
// WM_DEVICECHANGE activity we can receive while docking and
// undocking a portable machine.  If we decide to not use
// any of this, just delete it.  Note there are several 
// sections of code that use this conditional compilation.
// [brianau - 12/23/98]
//
#ifdef REPORT_DEVICE_CHANGES

DWORD
RegisterForDeviceNotifications(
    HWND hwndNotify
    )
{
    DWORD dwResult = ERROR_SUCCESS;

    DEV_BROADCAST_DEVICEINTERFACE dbdi;

    ZeroMemory(&dbdi, sizeof(dbdi));
    dbdi.dbcc_size       = sizeof(dbdi);
    dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    dbdi.dbcc_classguid  = GUID_DEVNODE_CHANGE;
    g_hDevNotify = RegisterDeviceNotification(hwndNotify, &dbdi, DEVICE_NOTIFY_WINDOW_HANDLE);
    if (NULL == g_hDevNotify)
        dwResult = GetLastError();

    return dwResult;
}


void
UnregisterForDeviceNotifications(
    void
    )
{
    if (NULL != g_hDevNotify)
    {
        UnregisterDeviceNotification(g_hDevNotify);
        g_hDevNotify = NULL;
    }
}


void
OnDeviceChange(
    WPARAM wParam,
    LPARAM lParam
    )
{
    PDEV_BROADCAST_DEVICEINTERFACE pdbdi = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
    TCHAR szNull[] = TEXT("<null>");
    LPCTSTR pszName = pdbdi ? pdbdi->dbcc_name : szNull;

    switch(wParam)
    {
        case DBT_DEVICEARRIVAL:
            STDBGOUT((3, TEXT("Device Arrival for : \"%s\""), pszName));
            break;
        case DBT_DEVICEREMOVEPENDING:
            STDBGOUT((3, TEXT("Device Remove pending for \"%s\""), pszName));
            break;
        case DBT_DEVICEREMOVECOMPLETE:
            STDBGOUT((3, TEXT("Device Removal complete for \"%s\""), pszName));
            break;
        case DBT_DEVICEQUERYREMOVE:
            STDBGOUT((3, TEXT("Device query remove for \"%s\""), pszName));
            break;
        case DBT_DEVICEQUERYREMOVEFAILED:
            STDBGOUT((3, TEXT("Device query remove FAILED for \"%s\""), pszName));
            break;
        case DBT_DEVICETYPESPECIFIC:
            STDBGOUT((3, TEXT("Device type specific for \"%s\""), pszName));
            break;
        case DBT_QUERYCHANGECONFIG:
            STDBGOUT((3, TEXT("Query change config for \"%s\""), pszName));
            break; 
        case DBT_CONFIGCHANGED:
            STDBGOUT((3, TEXT("Config changed for \"%s\""), pszName));
            break; 
        default:
            STDBGOUT((3, TEXT("Unknown device notification %d"), wParam));
            break;
    }
}

#endif // REPORT_DEVICE_CHANGES



LRESULT CALLBACK _HiddenWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lResult = 0;
    CStateMachine *pSysTraySM = (CStateMachine*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    TraceEnter(TRACE_CSCST, "_HiddenWndProc");

    switch(uMsg)
    {
    case WM_CREATE:
        DllAddRef();
#if DBG
        CreateWindow(TEXT("listbox"),
                     NULL,
                     WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | 
                        LBS_NOINTEGRALHEIGHT | LBS_WANTKEYBOARDINPUT,
                     0,0,0,0,
                     hWnd,
                     (HMENU)IDC_DEBUG_LIST,
                     g_hInstance,
                     NULL);
#endif
#ifdef REPORT_DEVICE_CHANGES
        RegisterForDeviceNotifications(hWnd);
#endif
        {
            BOOL bNoNet = FALSE;
            // Check whether the entire net is offline or not
            if (!CSCIsServerOffline(NULL, &bNoNet))
                bNoNet = TRUE; // RDR is dead, so net is down
                
            pSysTraySM = new CStateMachine(boolify(bNoNet));
            if (!pSysTraySM)
                TraceLeaveValue((LRESULT)-1);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pSysTraySM);

            if (bNoNet)
            {
                // Set initial status to NoNet
                PostMessage(hWnd, CSCWM_UPDATESTATUS, STWM_CSCNETDOWN, 0);
            }
            else
            {
                //
                // Calculate initial status as if the logon sync has just
                // completed.
                //
                // There's a race condition, so we can't count on getting
                // this message from the logon sync. If logon sync is still
                // proceeding, we will get another CSCWM_DONESYNCING which
                // is OK.
                //
                PostMessage(hWnd, CSCWM_DONESYNCING, 0, 0);
            }
        }
        //
        // Handle several things that happen at logon.
        //
        PostMessage(hWnd, PWM_HANDLE_LOGON_TASKS, 0, 0);
        //
        // This event is used to terminate any threads when the 
        // hidden notification window is destroyed.
        //
        if (NULL == g_heventTerminate)
            g_heventTerminate = CreateEvent(NULL, TRUE, FALSE, NULL);
        //
        // This event is used to ensure only one admin-pin operation
        // is running at a time.
        //
        if (NULL == g_hmutexAdminPin)
            g_hmutexAdminPin = CreateMutex(NULL, FALSE, NULL);
            
        break;

    case PWM_TRAYCALLBACK:
        STDBGOUT((4, TEXT("PWM_TRAYCALLBACK, wParam = 0x%08X, lParam = 0x%08X"), wParam, lParam));
        lResult = _Notify(hWnd, wParam, lParam);
        break;

#ifdef REPORT_DEVICE_CHANGES
    case WM_DEVICECHANGE:
        OnDeviceChange(wParam, lParam);
        break;
#endif // REPORT_DEVICE_CHANGES


    case WM_DESTROY:
        TraceMsg("_HiddenWndProc: hidden window destroyed");
#ifdef REPORT_DEVICE_CHANGES
        UnregisterForDeviceNotifications();
#endif
        delete pSysTraySM;
        pSysTraySM = NULL;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
        if (NULL != g_heventTerminate)
        {
            //
            // This will tell any threads that they should
            // exit asap.
            //
            SetEvent(g_heventTerminate);
        }
        DllRelease();
        break;

    case WM_COPYDATA:
        {
            // Warning: STDBGOUT here (inside WM_COPYDATA, outside the switch
            // statement) would cause an infinite loop of WM_COPYDATA messages
            // and blow the stack.
            PCOPYDATASTRUCT pcds = (PCOPYDATASTRUCT)lParam;
            if (pcds)
            {
                switch (pcds->dwData)
                {
                case STWM_CSCNETUP:
                case STWM_CSCNETDOWN:
                case STWM_CACHE_CORRUPTED:
                    {
                        LPTSTR pszServer = NULL;
                        //
                        // WM_COPYDATA is always sent, not posted, so copy the data
                        // and post a message to do the work asynchronously.
                        // This allocated string will be freed by the CSCWM_UPDATESTATUS
                        // handler UpdateStatus().  No need to free it here.
                        //
                        STDBGOUT((3, TEXT("Rcvd WM_COPYDATA, uMsg = 0x%08X, server = %s"), pcds->dwData, pcds->lpData));
                        LocalAllocString(&pszServer, (LPTSTR)pcds->lpData);
                        PostMessage(hWnd, CSCWM_UPDATESTATUS, pcds->dwData, (LPARAM)pszServer);
                    }
                    break;

                case CSCWM_GETSHARESTATUS:
                    // This one comes from outside of cscui.dll, and
                    // is always UNICODE.
                    if (pcds->lpData)
                    {
                        TCHAR szShare[MAX_PATH];
                        SHUnicodeToTChar((LPWSTR)pcds->lpData, szShare, ARRAYSIZE(szShare));
                        STDBGOUT((3, TEXT("Rcvd CSCWM_GETSHARESTATUS for \"%s\""), szShare));
                        lResult = GetShareStatusForWebView(pSysTraySM, szShare);
                    }
                    break;
                   
                case PWM_REFRESH_SHELL:
                    STDBGOUT((3, TEXT("Rcvd WM_COPYDATA, PWM_REFRESH_SHELL, server = %s"), pcds->lpData ? pcds->lpData : TEXT("<null>")));
                    if (pcds->lpData)
                    {
                        LPTSTR pszServer = NULL;
                        LocalAllocString(&pszServer, (LPTSTR)pcds->lpData);
                        PostMessage(hWnd, PWM_REFRESH_SHELL, 0, (LPARAM)pszServer);
                    }
                    break;
#if DBG
                //
                // The following messages in the "#if DBG" block are to support the
                // monitoring feature of the hidden systray window.
                //
                case PWM_STDBGOUT:
                    // Warning: no STDBGOUT here
                    STDebugOnLogEvent(GetDlgItem(hWnd, IDC_DEBUG_LIST), (LPCTSTR)pcds->lpData);
                    break;
#endif // DBG
                }
            }
        }
        break;

    case CSCWM_ISSERVERBACK:
        STDBGOUT((2, TEXT("Rcvd CSCWM_ISSERVERBACK")));
        lResult = IsServerBack(pSysTraySM, (LPCTSTR)lParam);
        break;

    case CSCWM_DONESYNCING:
        STDBGOUT((1, TEXT("Rcvd CSCWM_DONESYNCING. wParam = 0x%08X, lParam = 0x%08X"), wParam, lParam));
        pSysTraySM->PingServers();
        UpdateStatus(pSysTraySM, hWnd, STWM_STATUSCHECK, NULL);
        break;

    case CSCWM_UPDATESTATUS:
        UpdateStatus(pSysTraySM, hWnd, (UINT)wParam, (LPTSTR)lParam);
        if (lParam)
            LocalFree((LPTSTR)lParam);  // We make a copy when we get WM_COPYDATA
        break;

    case PWM_RESET_REMINDERTIMER:
        STDBGOUT((2, TEXT("Rcvd PWM_RESET_REMINDERTIMER")));
        OnResetReminderTimer();
        break;

    case PWM_HANDLE_LOGON_TASKS:
        HandleLogonTasks();
        break;

    case PWM_REFRESH_SHELL:
        STDBGOUT((3, TEXT("Rcvd PWM_REFRESH_SHELL")));
        _RefreshAllExplorerWindows((LPCTSTR)lParam);
        //
        // lParam is a server name allocated with LocalAlloc.
        //
        if (lParam)
            LocalFree((LPTSTR)lParam);
        break;

    case CSCWM_VIEWFILES:
        COfflineFilesFolder::Open();
        break;

    case PWM_STATUSDLG:
        ShowCSCUIStatusDlg(hWnd);
        break;

    case PWM_QUERY_UISTATE:
        lResult = OnQueryUIState();
        break;

    case CSCWM_SYNCHRONIZE:
        STDBGOUT((1, TEXT("Rcvd CSCWM_SYNCHRONIZE")));
        OnSynchronize();
        break;

    case CSCWM_SETTINGS:
        COfflineFilesSheet::CreateAndRun(g_hInstance, GetDesktopWindow(), &g_cRefCount);
        break;

    case WM_WININICHANGE:
        OnWinIniChange(hWnd, wParam, lParam);
        break;

#if DBG
        //
        // The following messages in the "#if DBG" block are to support the
        // monitoring feature of the hidden systray window.
        //
        case WM_GETDLGCODE:
            lResult = DLGC_WANTALLKEYS;
            break;

        case WM_VKEYTOITEM:
            wParam = LOWORD(wParam); // Extract the virtual key code.
            //
            // Fall through.
            //
        case WM_KEYDOWN:
            if (0x8000 & GetAsyncKeyState(VK_CONTROL))
            {
                if (TEXT('S') == wParam || TEXT('s') == wParam)
                {
                    //
                    // Ctrl-S saves the contents to a file.
                    //
                    STDebugSaveListboxContent(hWnd);
                }
                else if (TEXT('U') == wParam || TEXT('u') == wParam)
                {
                    //
                    // Ctrl-U forces an update to match the current cache state.
                    //
                    UpdateStatus(pSysTraySM, hWnd, STWM_STATUSCHECK, NULL);
                }
                else if (TEXT('B') == wParam || TEXT('b') == wParam)
                {
                    //
                    // Ctrl-B pings offline servers to see if they are back.
                    //
                    pSysTraySM->PingServers();
                }
            }
            else if (VK_DELETE == wParam)
            {
                //
                // [Delete] clears the contents of the listbox.
                //
                if (IDOK == MessageBox(hWnd,
                                       TEXT("Clear the list?"),
                                       c_szHiddenWindowTitle,
                                       MB_OKCANCEL))
                {
                    SendDlgItemMessage(hWnd, IDC_DEBUG_LIST, LB_RESETCONTENT, 0, 0);
                }
            }
            lResult = (WM_VKEYTOITEM == uMsg) ? -1 : 0;
            break;

        case WM_SIZE:
        {
            RECT rc;
            GetClientRect(hWnd, &rc);
            SetWindowPos(GetDlgItem(hWnd, IDC_DEBUG_LIST),
                         NULL,
                         rc.left,
                         rc.top,
                         rc.right - rc.left,
                         rc.bottom - rc.top,
                         SWP_NOZORDER);
        }
        break;
#endif // DBG

    default:
        lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
        break;
    }

    TraceLeaveValue(lResult);
}


HWND _CreateHiddenWnd(void)
{
    WNDCLASS wc;
    HWND hwnd;

    TraceEnter(TRACE_CSCST, "_CreateHiddenWnd");

    GetClassInfo(NULL, WC_DIALOG, &wc);
    wc.style         |= CS_NOCLOSE;
    wc.lpfnWndProc   = _HiddenWndProc;
    wc.hInstance     = g_hInstance;
    wc.lpszClassName = c_szHiddenWindowClassName;
    RegisterClass(&wc);

    hwnd = CreateWindow(c_szHiddenWindowClassName,
                        c_szHiddenWindowTitle,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        CW_USEDEFAULT, CW_USEDEFAULT,
                        NULL,
                        NULL,
                        g_hInstance,
                        NULL);
    if (hwnd)
    {
#if DBG
        //
        // In debug builds, if registry is set up to display 
        // systray debug output, create the CSCUI "hidden" window
        // as visible.
        //
        if (0 < STDebugLevel())
        {
            ShowWindow(hwnd, SW_NORMAL);
            UpdateWindow(hwnd);
        }
#endif // DBG
    }
    else
    {
        Trace((TEXT("CSCSysTrayThreadProc: CreateWindow failed GLE: %Xh"), GetLastError()));
    }

    TraceLeaveValue(hwnd);
}


BOOL
PostToSystray(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    SetLastError(ERROR_SUCCESS);

    if (IsWindow(g_hWndNotification))
        return PostMessage(g_hWndNotification, uMsg, wParam, lParam);

    // Search for the window and try again
    _FindNotificationWindow();

    if (IsWindow(g_hWndNotification))
        return PostMessage(g_hWndNotification, uMsg, wParam, lParam);

    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
}

LRESULT
SendToSystray(
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    SetLastError(ERROR_SUCCESS);

    if (IsWindow(g_hWndNotification))
        return SendMessage(g_hWndNotification, uMsg, wParam, lParam);

    // Search for the window and try again
    _FindNotificationWindow();

    if (IsWindow(g_hWndNotification))
        return SendMessage(g_hWndNotification, uMsg, wParam, lParam);

    SetLastError(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
}


HWND _FindNotificationWindow()
{
    g_hWndNotification = FindWindow(c_szHiddenWindowClassName, NULL);
    return g_hWndNotification;
}


STDAPI_(HWND) CSCUIInitialize(HANDLE hToken, DWORD dwFlags)
{
    TraceEnter(TRACE_CSCST, "CSCUIInitialize");

    _FindNotificationWindow();

    //
    // When running within winlogon.exe, we get logon/logoff messages from
    // cscdll.  On logoff, we need to close any open registry keys so that
    // user profile information can be saved.  The global CSettings object is
    // designed to re-open the keys and re-establish any required change
    // notifications the next time settings/policy information is requested.
    //
    if (dwFlags & CI_INITIALIZE)
    {
        if (hToken)
        {
            SECURITY_ATTRIBUTES sa;
            SECURITY_DESCRIPTOR sd;

            InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
            SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);

            sa.lpSecurityDescriptor = &sd;
            sa.bInheritHandle       = FALSE;
            sa.nLength              = sizeof(sa);

            DuplicateTokenEx(hToken,
                             TOKEN_ALL_ACCESS,
                             &sa,
                             SecurityImpersonation,
                             TokenPrimary,
                             &g_hToken);

            Trace((TEXT("CSCUIInitialize: Using new token handle:%Xh"), g_hToken));
        }

        if (dwFlags & CI_CREATEWINDOW)
        {
            BOOL bCSCEnabled = IsCSCEnabled();
            //
            // The CI_CREATEWINDOW bit is set by systray/explorer
            //
            if (!bCSCEnabled || CConfig::GetSingleton().NoCacheViewer())
            {
                //
                // If CSC is currently disabled, or system policy prevents the
                // user from viewing the cache contents, remove the Offline Files
                // folder shortcut from the user's desktop.
                //
                DeleteOfflineFilesFolderLink();
            }

            if (g_hWndNotification)
            {
                Trace((TEXT("CSCUIInitialize: returning existing hWnd:%Xh"), g_hWndNotification));
            }
            else if (!bCSCEnabled)
            {
                ExitGracefully(g_hWndNotification, NULL, "CSCUIInitialize: CSC not enabled");
            }
            else
            {
                g_hWndNotification = _CreateHiddenWnd();
                Trace((TEXT("CSCUIInitialize: Created new hWnd:%Xh"), g_hWndNotification));
            } 
        }    
    }    
    else if (dwFlags & CI_TERMINATE)
    {
        if (dwFlags & CI_DESTROYWINDOW)
        {
            //
            // The CI_DESTROYWINDOW bit is set by systray.exe
            //
            if (g_hWndNotification)
            {
                TraceMsg("CSCUIInitialize: Destroying hidden window");
                DestroyWindow(g_hWndNotification);
                g_hWndNotification = NULL;
            }
            UnregisterClass(c_szHiddenWindowClassName, g_hInstance);
        }    
        else
        {
            //
            // This call is a result of a logoff notification from the
            // CSC agent running within winlogon.exe.  
            //
            if (g_hToken)
                ImpersonateLoggedOnUser(g_hToken);            
                
            HandleLogoffTasks();
            
            if (g_hToken)
                RevertToSelf();
        }
        if (g_hToken)
        {
            TraceMsg("CSCUIInitialize: Freeing token handle");
            CloseHandle(g_hToken);
            g_hToken = NULL;
        }
    }

exit_gracefully:

    TraceLeaveValue(g_hWndNotification);
}


LRESULT 
AttemptRasConnect(
    LPCTSTR pszServer
    )
{
    LRESULT lRes = LRESULT_CSCFAIL;
    HMODULE hMod = LoadLibrary(TEXT("rasadhlp.dll"));

    if (hMod)
    {
        PFNHLPNBCONNECTION pfn;
        pfn = (PFNHLPNBCONNECTION)GetProcAddress(hMod, (LPCSTR)"AcsHlpNbConnection");

        STDBGOUT((1, TEXT("Attempting RAS connection to \"%s\""), pszServer ? pszServer : TEXT("<null>")));
       
        if (pfn)
        {
            if ((*pfn)(pszServer))
            {
                STDBGOUT((1, TEXT("RAS connection successful. Action is LRESULT_CSCRETRY.")));
                lRes = LRESULT_CSCRETRY;
            }    
            else
            {
                STDBGOUT((2, TEXT("AttemptRasConnect: AcsHlpNbConnection() failed.")));
            }
        }
        else
        {
            STDBGOUT((2, TEXT("AttemptRasConnect: Error %d getting addr of AcsHlpNbConnection()"), GetLastError()));
        }
        FreeLibrary(hMod);    
    }
    else 
    {
        STDBGOUT((2, TEXT("AttemptRasConnect: Error %d loading rasadhlp.dll.  Action is LRESULT_CSCFAIL"), GetLastError()));
    }
    if (LRESULT_CSCFAIL == lRes)
    {
        STDBGOUT((1, TEXT("RAS connection failed.")));
    }

    return lRes;
}


//////////////////////////////////////////////////////////////////////////////
// _OnQueryNetDown
//
// Handler for STWM_CSCQUERYNETDOWN
//
// Returns:
//
//    LRESULT_CSCFAIL         - Fail the connection NT4-style.
//    LRESULT_CSCWORKOFFLINE  - Transition this server to "offline" mode.
//    LRESULT_CSCRETRY        - We have a RAS connection.  Retry.
//
LRESULT OnQueryNetDown(
    DWORD dwAutoDialFlags,
    LPCTSTR pszServer
    )
{
    LRESULT lResult = LRESULT_CSCFAIL;

    if (CSCUI_NO_AUTODIAL != dwAutoDialFlags)
    {
        //
        // The server is not in the CSC database and CSCDLL wants us
        // to offer the USER a RAS connection.
        //
        lResult = AttemptRasConnect(pszServer);
    }
    //
    // lResult will be LRESULT_CSCFAIL under two conditions:
    //
    // 1. dwAutoDialFlags is CSCUI_NO_AUTODIAL so lResult has it's initial value.
    // 2. AttemptRasConnect() failed and returned LRESULT_CSCFAIL.
    //    In this case we now want to determine if we really want to
    //    fail the request or if we should transition offline.
    //
    // Also, only execute this if the server is in the cache.  If not,
    // we don't want to go offline on the server; we just want to fail
    // it.
    //
    if ((LRESULT_CSCFAIL == lResult) &&
        (CSCUI_AUTODIAL_FOR_UNCACHED_SHARE != dwAutoDialFlags))
    {
        //
        // This code is called from within the winlogon process.  Because
        // it's winlogon, there's some funky stuff going on with user tokens
        // and registry keys.  In order to read the user preference for
        // "offline action" we need to temporarily impersonate the currently
        // logged on user.  
        //
        if (g_hToken)
            ImpersonateLoggedOnUser(g_hToken);
            
        int iAction = CConfig::GetSingleton().GoOfflineAction(pszServer);
        
        if (g_hToken)
            RevertToSelf();

        switch(iAction)
        {
            case CConfig::eGoOfflineSilent:
                STDBGOUT((1, TEXT("Action is LRESULT_CSCWORKOFFLINE")));
                lResult = LRESULT_CSCWORKOFFLINE;
                break;

            case CConfig::eGoOfflineFail:
                STDBGOUT((1, TEXT("Action is LRESULT_CSCFAIL")));
                lResult = LRESULT_CSCFAIL;
                break;

            default:
                STDBGOUT((1, TEXT("Invalid action (%d), defaulting to LRESULT_CSCWORKOFFLINE"), iAction));
                //
                // An invalid action code defaults to "work offline".
                //
                lResult = LRESULT_CSCWORKOFFLINE;
                break;
        }
    }
    return lResult;
}

//
// Structure allocated in SendCopyDataToSystrayAsync and
// passed to _SendCopyDataProc through the QueueUserWorkItem
// system API.  We were previously passing only a COPYDATASTRUCT
// to the thread proc but we then found the SendToSysTray call 
// was occuring on the wrong desktop when executed within 
// winlogon.exe.  Now the caller of QueueUserWorkItem includes 
// it's desktop handle in the SENDCOPYDATAPROCPARAMS structure.  
// The thread proc then sets it's thread's desktop to this value 
// before calling SendToSystray.  Afterward, the thread's 
// desktop is restored.
//
typedef struct
{
    HDESK           hDesktop; // SendToSystray needs proper desktop.
    PCOPYDATASTRUCT pcds;
    
} SENDCOPYDATAPROCPARAMS, *PSENDCOPYDATAPROCPARAMS;


DWORD WINAPI _SendCopyDataProc(LPVOID pvData)
{
    PSENDCOPYDATAPROCPARAMS pscdpp = (PSENDCOPYDATAPROCPARAMS)pvData;
    if (pscdpp)
    {
        if (pscdpp->pcds)
        {
            //
            // Set thread's desktop to that of the code that
            // queued the worker thread.
            //
            HDESK hDesktop = GetThreadDesktop(GetCurrentThreadId());
            if (NULL != pscdpp->hDesktop)
                SetThreadDesktop(pscdpp->hDesktop);
                
            SendToSystray(WM_COPYDATA, 0, (LPARAM)pscdpp->pcds);
            //
            // Restore thread's desktop.
            //
            if (NULL != hDesktop)
                SetThreadDesktop(hDesktop);

            LocalFree(pscdpp->pcds);
        }
        LocalFree(pscdpp);
    }
    return 0;
}

BOOL SendCopyDataToSystrayAsync(DWORD dwData, DWORD cbData, PVOID pData)
{
    BOOL bResult = FALSE;
    PSENDCOPYDATAPROCPARAMS pscdpp = (PSENDCOPYDATAPROCPARAMS)LocalAlloc(LPTR, sizeof(SENDCOPYDATAPROCPARAMS));
    if (pscdpp)
    {
        pscdpp->hDesktop = GetThreadDesktop(GetCurrentThreadId());
        pscdpp->pcds = (PCOPYDATASTRUCT)LocalAlloc(LPTR, sizeof(COPYDATASTRUCT) + cbData);
        if (pscdpp->pcds)
        {
            pscdpp->pcds->dwData = dwData;
            pscdpp->pcds->cbData = cbData;
            pscdpp->pcds->lpData = ByteOffset(pscdpp->pcds, sizeof(COPYDATASTRUCT));
            CopyMemory(pscdpp->pcds->lpData, pData, cbData);
            bResult = QueueUserWorkItem(_SendCopyDataProc, pscdpp, 0);
            if (!bResult)
            {
                LocalFree(pscdpp->pcds);
                LocalFree(pscdpp);
            }
        }
        else
        {
            LocalFree(pscdpp->pcds);
        }
    }
    return bResult;
}

void _UpdateStatusFromWinLogon(UINT uMsg, LPTSTR pszServer)
{
    //
    // If we have a server name, use WM_COPYDATA to get the data
    // into explorer's process. Do this from a worker thread so
    // we don't block the CSC Agent in winlogon.
    //
    if (pszServer)
    {
        SendCopyDataToSystrayAsync(uMsg, StringByteSize(pszServer), pszServer);
    }
    else
    {
        PostToSystray(CSCWM_UPDATESTATUS, uMsg, 0);
    }
}


//
// This function is typically called from the CSC Agent (cscdll) in winlogon.
// The Agent asks us whether to transition offline or not, and also notifies
// us of status changes (net-up, net-down, etc.). Status changes are passed
// on to the hidden systray window.
//
// Special care must be taken to not call SendMessage back to the UI thread,
// since it is possible, although unlikely, that the UI thread is hitting
// the net and blocked waiting for a response from this function (deadlock).
//
// The debug-only STDBGOUT is exempted from the SendMessage ban. If you hit
// a deadlock due to STDBGOUT, reboot and turn off SysTrayOutput.
//
STDAPI_(LRESULT) CSCUISetState(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = 0;
    LPTSTR pszServer = (LPTSTR)lParam;

    if (pszServer && (IsBadStringPtr(pszServer, 48) || !*pszServer))
        pszServer = NULL;

    switch(uMsg)
    {
    case STWM_CSCQUERYNETDOWN:
        STDBGOUT((1, TEXT("Rcvd STWM_CSCQUERYNETDOWN, wParam = 0x%08X, lParam = 0x%08X"), wParam, lParam));
        lRes = OnQueryNetDown((DWORD)wParam, pszServer);
        //
        // HACK!  This is a hack to handle the way the redirector and the CSC
        //        agent work in the "net down" case.  The CSC agent tells us 
        //        about "no net" in a CSCQUERYNETDOWN rather than a CSCNETDOWN 
        //        like I would prefer it.  The problem is that the redirector 
        //        doesn't actually transition the servers to offline until 
        //        a server is touched.  Therefore, when lParam == 0 
        //        we need to first handle the "query" case to determine what to tell
        //        the CSC agent (fail, work offline, retry etc).  Then, if the
        //        result is not "retry", we need to continue processing the message 
        //        as if it were STWM_CSCNETDOWN. [brianau]
        //
        if (LRESULT_CSCRETRY == lRes || NULL != pszServer)
            return lRes;
        uMsg = STWM_CSCNETDOWN;
        //
        // Fall through...
        //
        case STWM_CSCNETDOWN:
            STDBGOUT((1, TEXT("Rcvd STWM_CSCNETDOWN, wParam = 0x%08X, lParam = 0x%08X"), wParam, lParam));
            CSCUI_NOTIFYHOOK((CSCH_NotifyOffline, TEXT("NetDown: %1"), pszServer ? pszServer : TEXT("<no server>")));
            break;

        case STWM_CSCNETUP:
            STDBGOUT((1, TEXT("Rcvd STWM_CSCNETUP, wParam = 0x%08X, lParam = 0x%08X"), wParam, lParam));
            CSCUI_NOTIFYHOOK((CSCH_NotifyAvailable, TEXT("NetBack: %1"), pszServer ? pszServer : TEXT("<no server>")));
            break;

        case STWM_CACHE_CORRUPTED:
            STDBGOUT((1, TEXT("Rcvd STWM_CACHE_CORRUPTED, wParam = 0x%08X, lParam = 0x%08X"), wParam, lParam));
            break;
    }

    _UpdateStatusFromWinLogon(uMsg, pszServer);

    return lRes;
}    


const TCHAR c_szExploreClass[]  = TEXT("ExploreWClass");
const TCHAR c_szIExploreClass[] = TEXT("IEFrame");
const TCHAR c_szCabinetClass[]  = TEXT("CabinetWClass");
const TCHAR c_szDesktopClass[]  = TEXT(STR_DESKTOPCLASS);


BOOL IsExplorerWindow(HWND hwnd)
{
    TCHAR szClass[32];

    GetClassName(hwnd, szClass, ARRAYSIZE(szClass));
    if ( (lstrcmp(szClass, c_szCabinetClass) == 0) 
       ||(lstrcmp(szClass, c_szIExploreClass) == 0)
       ||(lstrcmp(szClass, c_szExploreClass) == 0))
       return TRUE;

    return FALSE;   
}


BOOL IsPathOnServer(LPCTSTR pszPath, LPCTSTR pszServer)
{
    BOOL bResult = FALSE;
    LPTSTR pszRemotePath;
    if (S_OK == GetRemotePath(pszPath, &pszRemotePath))
    {
        if (NULL == pszServer)
        {
            bResult = TRUE;
        }                            
        else
        {
            PathStripToRoot(pszRemotePath);
            PathRemoveFileSpec(pszRemotePath);
            bResult = (0 == lstrcmpi(pszServer, pszRemotePath));
        }
        LocalFreeString(&pszRemotePath);
    }
    return bResult;
}


//
// IsWindowBrowsingServer determines if a given window is browsing a particular
// server. The function assumes that the window is an explorer window.
// If pszServer == NULL, return TRUE if the window is browsing a remote path
// even if the window is not browsing this particular server.
//
BOOL IsWindowBrowsingServer(
    HWND hwnd,
    LPCTSTR pszServer
    )
{
    BOOL bResult = FALSE;
    DWORD_PTR dwResult;
    DWORD dwPID = GetCurrentProcessId();
    const UINT uFlags = SMTO_NORMAL | SMTO_ABORTIFHUNG;
    if (SendMessageTimeout(hwnd,
                           CWM_CLONEPIDL,
                           (WPARAM)dwPID,
                           0L,
                           uFlags,
                           5000,
                           &dwResult))
    {
        HANDLE hmem = (HANDLE)dwResult;
        if (NULL != hmem)
        {
            LPITEMIDLIST pidl = (LPITEMIDLIST)SHLockShared(hmem, dwPID);
            if (NULL != pidl)
            {
                TCHAR szPath[MAX_PATH];
                if (SHGetPathFromIDList(pidl, szPath))
                {
                    bResult = IsPathOnServer(szPath, pszServer);
                }
                SHUnlockShared(pidl);
            }
            SHFreeShared(hmem, dwPID);
        }
    }
    return bResult;
}

void RefreshExplorerWindow(HWND hwnd, LPCTSTR pszServer)
{
    PostMessage(hwnd, WM_COMMAND, FCIDM_REFRESH, 0L);
}


BOOL CALLBACK _RefreshEnum(HWND hwnd, LPARAM lParam)
{
    LPCTSTR pszServer = (LPCTSTR)lParam;
    if (IsExplorerWindow(hwnd) && IsWindowBrowsingServer(hwnd, pszServer))
    {
        STDBGOUT((2, TEXT("Refreshing explorer wnd 0x%08X for \"%s\""), hwnd, pszServer));
        RefreshExplorerWindow(hwnd, pszServer);
    }        
    return(TRUE);
}

//
// _RefreshAllExplorerWindows is called by the CSC tray wnd proc
// in response to a PWM_REFRESH_SHELL message.  The pszServer argument
// is the name of the server (i.e. "\\scratch") that has transitioned
// either online or offline.  The function refreshes windows that are 
// currently browsing the server.
//
void _RefreshAllExplorerWindows(LPCTSTR pszServer)
{
    //
    // Without initializing COM, we hit a "com not initialized" assertion
    // in shdcocvw when calling SHGetPathFromIDList in IsWindowBrowsingServer.
    // 
    if (SUCCEEDED(CoInitialize(NULL)))
    {
        //
        // We have to special case the desktop window for 2 reasons:
        // 1. It isn't enumerated by EnumWindows
        // 2. It doesn't handle CWM_CLONEPIDL, so can't use IsWindowBrowsingServer
        //
        BOOL bRefreshDesktop = FALSE;
        const int csidl[] = { CSIDL_DESKTOPDIRECTORY, CSIDL_COMMON_DESKTOPDIRECTORY };
        for (int i = 0; !bRefreshDesktop && i < ARRAYSIZE(csidl); i++)
        {
            TCHAR szPath[MAX_PATH];
            if (SHGetSpecialFolderPath(NULL, szPath, csidl[i] | CSIDL_FLAG_DONT_VERIFY, FALSE))
            {
                bRefreshDesktop = IsPathOnServer(szPath, pszServer);
            }
        }
        if (bRefreshDesktop)
        {
            HWND hwndDesktop = FindWindowEx(NULL, NULL, c_szDesktopClass, NULL);
            if (hwndDesktop)
            {
                InvalidateTheDesktop();
                STDBGOUT((2, TEXT("Refreshing desktop wnd 0x%08X for \"%s\""), hwndDesktop, pszServer));
                RefreshExplorerWindow(hwndDesktop, pszServer);
            }            
        }

        EnumWindows(_RefreshEnum, (LPARAM)pszServer);
        CoUninitialize();
    }        
}


STDAPI_(BOOL) CSCUIMsgProcess(LPMSG pMsg)
{
    return IsDialogMessage(g_hwndStatusDlg, pMsg);
}


//-----------------------------------------------------------------------------
// SysTray debug monitoring code.
//
//
// This function can run in either winlogon, systray or mobsync processes.
// That's why we use WM_COPYDATA to communicate the text information.
//
#if DBG
void STDebugOut(
    int iLevel,
    LPCTSTR pszFmt,
    ...
    )
{
    if (STDebugLevel() >= iLevel)
    {
        TCHAR szText[1024];
        SYSTEMTIME t;
        GetLocalTime(&t);

        wsprintf(szText, TEXT("[pid %d] %02d:%02d:%02d.%03d  "),
                 GetCurrentProcessId(),
                 t.wHour,
                 t.wMinute,
                 t.wSecond,
                 t.wMilliseconds);

        va_list args;
        va_start(args, pszFmt);
        wvsprintf(szText + lstrlen(szText), pszFmt, args);
        va_end(args);

        COPYDATASTRUCT cds;
        cds.dwData = PWM_STDBGOUT;
        cds.cbData = StringByteSize(szText);
        cds.lpData = szText;
        SendToSystray(WM_COPYDATA, 0, (LPARAM)&cds);
    }
}


int STDebugLevel(void)
{
    static DWORD dwMonitor = (DWORD)-1;

    if ((DWORD)-1 == dwMonitor)
    {
        dwMonitor = 0;
        HKEY hkey;
        DWORD dwType;
        DWORD cbData = sizeof(DWORD);
        DWORD dwStatus = STDebugOpenNetCacheKey(KEY_QUERY_VALUE, &hkey);
        if (ERROR_SUCCESS == dwStatus)
        {
            RegQueryValueEx(hkey,
                            c_szSysTrayOutput,
                            NULL,
                            &dwType,
                            (LPBYTE)&dwMonitor,
                            &cbData);
            RegCloseKey(hkey);
        }
    }
    return int(dwMonitor);
}

//
// Called in response to PWM_STDBGOUT.  This occurs in the systray process only.
//
void STDebugOnLogEvent(
    HWND hwndList,
    LPCTSTR pszText
    )
{
    if (pszText && *pszText)
    {
        int iTop = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)pszText);
        SendMessage(hwndList, LB_SETTOPINDEX, iTop - 5, 0);
    }
}


typedef BOOL (WINAPI * PFNGETSAVEFILENAME)(LPOPENFILENAME);

#ifdef UNICODE 
#   define GETSAVEFILENAME "GetSaveFileNameW"
#else
#   define GETSAVEFILENAME "GetSaveFileNameA"
#endif

//
// This function will always run on the window's thread and in the systray process.
//
void STDebugSaveListboxContent(
    HWND hwndParent
    )
{
    static bool bSaving = false;  // Re-entrancy guard.
    if (bSaving)
        return;

    HMODULE hModComdlg = LoadLibrary(TEXT("comdlg32"));
    if (NULL == hModComdlg)
        return;

    PFNGETSAVEFILENAME pfnSaveFileName = (PFNGETSAVEFILENAME)GetProcAddress(hModComdlg, GETSAVEFILENAME);
    if (NULL != pfnSaveFileName)
    {
        bSaving = true;
        TCHAR szFile[MAX_PATH] = TEXT("C:\\CSCUISystrayLog.txt");
        OPENFILENAME ofn = {0};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner   = hwndParent;
        ofn.hInstance   = g_hInstance;
        ofn.lpstrFile   = szFile;
        ofn.nMaxFile    = ARRAYSIZE(szFile);
        ofn.lpstrDefExt = TEXT("txt");
        if ((*pfnSaveFileName)(&ofn))
        {
            HANDLE hFile = CreateFile(szFile,
                                      GENERIC_WRITE,
                                      FILE_SHARE_READ,
                                      NULL,
                                      CREATE_ALWAYS,
                                      FILE_ATTRIBUTE_NORMAL,
                                      NULL);

            if (INVALID_HANDLE_VALUE != hFile)
            {
                int n = (int)SendDlgItemMessage(hwndParent, IDC_DEBUG_LIST, LB_GETCOUNT, 0, 0);
                TCHAR szText[MAX_PATH];
                for (int i = 0; i < n; i++)
                {
                    //
                    // WARNING:  This could potentially overwrite the szText[] buffer.
                    //           However, since the text should be of a readable length
                    //           in a listbox I doubt if it will exceed MAX_PATH.
                    //
                    SendDlgItemMessage(hwndParent, IDC_DEBUG_LIST, LB_GETTEXT, i, (LPARAM)szText);
                    lstrcat(szText, TEXT("\r\n"));
                    DWORD dwWritten = 0;
                    WriteFile(hFile, szText, lstrlen(szText) * sizeof(TCHAR), &dwWritten, NULL);
                }

                CloseHandle(hFile);
            }
            else
            {
                TCHAR szMsg[MAX_PATH];
                wsprintf(szMsg, TEXT("Error %d creating file \"%s\""), GetLastError(), szFile);
                MessageBox(hwndParent, szMsg, c_szHiddenWindowTitle, MB_ICONERROR | MB_OK);
            }
        }
    }
    bSaving = false;
    FreeLibrary(hModComdlg);
}



DWORD STDebugOpenNetCacheKey(
    DWORD dwAccess,
    HKEY *phkey
    )
{
    DWORD dwDisposition;
    return RegCreateKeyEx(HKEY_LOCAL_MACHINE, 
                          REGSTR_KEY_OFFLINEFILES,
                          0,
                          NULL,
                          0,
                          dwAccess,
                          NULL,
                          phkey,
                          &dwDisposition);
}

#endif // DBG
