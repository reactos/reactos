//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1991-1993
//
// File: fsnotify.c
//
// This file contains the file system notification service.
//
// History:
//  03-05-93 AndrewCo   Created
//
//---------------------------------------------------------------------------

// REVIEW WIN32 : A lot of this will need to be re-done for Win32.

#include "shellprv.h"
#pragma  hdrstop

#include <dbt.h>
#include "printer.h"

#define TF_FSNGENERAL 0

#define FSM_WAKEUP      (WM_USER + 0x0000)
#define FSM_GOAWAY      (WM_USER + 0x0001)

#define MSEC_INTERVAL     1000   // Accumerate changes for one-second.
#define MSEC_MAXWAIT     30000   // Maximum delay.
#define MSEC_MAXINTERVAL INFINITE

//#define FSNIDEBUG
#ifdef FSNDEBUG
#define FSNIDEBUG
#endif

#ifdef DEBUG
#define DESKTOP_EVENT 0x01
#define SFP_EVENT     0x02
#define LINK_EVENT    0x04
#define ICON_EVENT    0x08
#define RLFS_EVENT    0x10
#define ALIAS_EVENT   0x20

UINT g_fPerf = (DESKTOP_EVENT | SFP_EVENT | LINK_EVENT | ICON_EVENT | RLFS_EVENT | ALIAS_EVENT);
#define PERFTEST(n) if (g_fPerf & n)
#else // !DEBUG
#define PERFTEST(n)
#endif // !DEBUG

// if a more detailed event happens withing 2 seconds of a FS UPDATEDIR,
// we might blow off the FS UPDATEDIR
#define UPDATEDIR_OVERRIDE_TIME     2000

#define FSNENTERCRITICAL     ENTERCRITICAL
#define FSNLEAVECRITICAL     LEAVECRITICAL
#define FSNASSERTCRITICAL    ASSERTCRITICAL
#define FSNASSERTNONCRITICAL ASSERTNONCRITICAL

#define FSN_EVENTSPENDING ((int)(g_fsn.dwLastEvent-g_fsnpp.dwLastFlush) >= 0)

#define WakeThread(_id) ((_id) ? PostThreadMessage(_id, FSM_WAKEUP, 0, 0) : FALSE)
#define SignalKillThread(_id) ((_id) ? PostThreadMessage(_id, FSM_GOAWAY, 0, 0) : FALSE)

//
//  Note:  The string pointers are const which is to indicate that freeing
//         them is the caller's, not the client's, responsibility.
//

#define FSSF_IN_USE      0x0001
#define FSSF_DELETE_ME   0x0002

typedef struct _FSNCI FSNotifyClientInfo, *PFSNotifyClientInfo;

typedef struct _FSNCI
{
    PFSNotifyClientInfo pfsnciNext;
    HWND                hwnd;
    ULONG               ulID;
    DWORD               dwProcID;
    int                 fSources;
    int                 iSerializationFlags;
    LONG                fEvents;
    WORD                wMsg;
    HDSA                hdsaNE;
    HDPA                hdpaPendingEvents;

} FSNotifyClientInfo, *PFSNotifyClientInfo;  // Hungarian: fsnci

typedef struct
{
    BOOL          bSuspend;
    BOOL          bRecursive;
    ITEMIDLIST    idl[0];    
} SHChangeNotifySuspendResumeStruct;


typedef struct
{
    LPCITEMIDLIST pidl;     // this is SHARED with the fs registered client structure.
    HANDLE hEvent;
    int iCount;             // how many clients are interested in this. (ref counts)
    int iSuspendCount;
    HDEVNOTIFY hPNP;        // PnP handle to warn us about drives coming and going
    BOOL bSuspendPNP;       // Did we suspend due to a PnP warning?
    BOOL bRecursiveInterrupt; // is this a recursive interrupt client?
} FSIntClient, * LPFSIntClient;

//
//  FSNotifyEvent's member pszFile points to a buffer containing the NULL
//  terminated name of the file that changed, followed by the new name of
//  that file, if the operation was RENAME, or NULL if not.
//

typedef struct
{
    // these two must be together and at the front
    LPITEMIDLIST pidl;
    LPITEMIDLIST pidlExtra;
    LONG  lEvent;
    UINT cRef;
    DWORD dwEventTime;
} FSNotifyEvent;  // Hungarian: fsnevt

typedef struct
{
    HANDLE      hThread;
    DWORD       idThread;
} AWAKETHREAD;

//
//  We don't start the event-dispatch timer until an
//  event occurs.
//
typedef struct _FSNotify
{
    UINT    cRefClientList;
    DWORD   dwLastEvent;
    HDSA    hdsaThreadAwake;
    FSNotifyClientInfo *pfsnciFirst;
    HDPA    hdpaIntEvents;
    ULONG   ulNextID;
} FSNotify;

// this data block is attached to the notify window so that we can remember
// enough information without having to change the API's
#define WM_CHANGENOTIFYMSG    WM_USER + 1

typedef struct _tag_NotifyProxyData
{
    ULONG ulShellRegCode;
    HWND hwndParent;
    UINT wMsg;
} _NotifyProxyData, * LP_NotifyProxyData;


#pragma data_seg(DATASEG_SHARED)

FSNotify g_fsn =
{
    0,
    0,
    NULL,
    NULL,
    NULL,
    1,
};

#pragma data_seg()

typedef struct _FSNotifyPerProc
{
    HANDLE  htStarting;
    DWORD   idtStarting;
    HANDLE  htRunning;
    DWORD   idtRunning;             // invalid if htRunning is NULL
    UINT    cclients;               // number of registered client
    long    iCallbackCount;
    HDSA    hdsaIntEvents;
    HDSA    hdsaIntClients;
    DWORD   dwLastFlush;
    BOOL    fFlushNow;
    HANDLE  hCallbackEvent;         // iCallbackCount == 0 ? SetEvent : ResetClear
} FSNotifyPerProc;

const TCHAR c_szCallbackName[] = TEXT("Shell_NotificationCallbacksOutstanding");
const TCHAR c_szWindowClassName[] = TEXT("Shell32HiddenNotfyWnd");
const TCHAR c_szDummyWindowName[] = TEXT("");
#define CALLBACK_TIMEOUT    30000       // 30 seconds

FSNotifyPerProc g_fsnpp =
{
    NULL,
    0,
    NULL,
    0,
    0,
    0,
    NULL,
    NULL,
    0,
    FALSE,
    NULL
};

#define MAX_EVENT_COUNT 10

//
// internal event handlers.
//
void SFP_FSEvent        (LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);
void Icon_FSEvent       (LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra);


LRESULT CALLBACK HiddenNotifyWindow_WndProc( HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam );
BOOL _RegisterNotifyProxyWndProc( void );
void _SHChangeNotifyHandleClientEvents(FSNotifyClientInfo * pfsnci);

void FSNPostInterruptEvent(LPCITEMIDLIST pidl);
int _SHChangeNotifyNukeClient(PFSNotifyClientInfo * ppfsnci, BOOL fNukeInterrupts);

void WINAPI _SHChangeNotifyHandleEvents(BOOL);
void InitAliasFolderTable(void);
void TermAliasFolderTable(void);

#ifdef DEBUG
void DebugDumpPidl(TCHAR *szOut, LPCITEMIDLIST pidl)
{
    if (IsFlagSet(g_dwDumpFlags, DF_FSNPIDL))
    {
        TCHAR szPath[MAX_PATH];
        LPTSTR lpsz;
        if (pidl)  
        {
            lpsz = szPath;
            SHGetPathFromIDList(pidl, szPath);
        } 
        else 
        {
            lpsz = TEXT("(NULL)");
        }
        TraceMsg(TF_ALWAYS, "PIDL - %s: \"%s\"", szOut, lpsz);
    }
}
#else
#define DebugDumpPidl(p, q)
#endif

void FSNDestructIntClient(LPFSIntClient lpfsic, BOOL bFreePidl)
{
    if (lpfsic->hEvent && lpfsic->hEvent != INVALID_HANDLE_VALUE)
    {
        FindCloseChangeNotification(lpfsic->hEvent);
        lpfsic->hEvent = NULL;
    }
    if (lpfsic->hPNP)
    {
        UnregisterDeviceNotification(lpfsic->hPNP);
        lpfsic->hPNP = NULL;
        lpfsic->bSuspendPNP = FALSE;
    }

    if (bFreePidl)
        ILGlobalFree((LPITEMIDLIST)lpfsic->pidl);
}

void FSNAttachDeviceNotification(LPFSIntClient lpfsic)
{
    FSNASSERTCRITICAL;
    if (lpfsic->hPNP)
        UnregisterDeviceNotification(lpfsic->hPNP);
    lpfsic->hPNP = NULL;

    if (lpfsic->hEvent && lpfsic->hEvent != INVALID_HANDLE_VALUE)
    {
        HWND hwndDesktop = GetShellWindow();
        if (hwndDesktop && IsWindowInProcess(hwndDesktop))
        {
            DEV_BROADCAST_HANDLE dbh;
            ZeroMemory(&dbh, sizeof(dbh));
            dbh.dbch_size = sizeof(dbh);
            dbh.dbch_devicetype = DBT_DEVTYP_HANDLE;
            dbh.dbch_handle = lpfsic->hEvent;
            lpfsic->hPNP = RegisterDeviceNotification(hwndDesktop, &dbh, DEVICE_NOTIFY_WINDOW_HANDLE);
        }
    }
}

#define INTERRUPT_TIMEOUT  2 * MSEC_INTERVAL

int FSNBuildEventList(LPHANDLE lphe, BOOL fAllowDelete)
{
    int i = 0;

    FSNENTERCRITICAL;

    if (g_fsnpp.hdsaIntClients)
    {
        int j;
        int cEvents;

        cEvents = DSA_GetItemCount(g_fsnpp.hdsaIntClients);
        for (j = 0 ; j < cEvents ; j++)
        {
            LPFSIntClient lpfsic = DSA_GetItemPtr(g_fsnpp.hdsaIntClients, j);

            if (lpfsic->iCount == 0)
            {
NukeIt:
                if (fAllowDelete)
                {
                    // this one is marked for deletion.
                    // we know we're not waiting for it here, so nuke now.
                    FSNDestructIntClient(lpfsic, TRUE);
                    DSA_DeleteItem(g_fsnpp.hdsaIntClients, j);
                    j--;
                    cEvents--;
                }
                continue;
            }
            else if (lpfsic->iSuspendCount > 0)
            {
                // skip suspended events
                continue;
            }
            else
            {
                // create this here so that it will be owned by our global thread
                if (!lpfsic->hEvent)
                {
                    TCHAR szPath[MAX_PATH];

                    // REVIEW: My Computer & other special folders return ":{20d04fe0-...}" for this,
                    //         which is pretty bogus and we assert in the IsLFNDrive below
                    if (!SHGetPathFromIDList(lpfsic->pidl, szPath) || !szPath[0])
                    {
                        goto Punt;
                    }

#ifdef WINNT
#define FFCN_INTERESTING_EVENTS     (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_ATTRIBUTES)
#else
#define FFCN_INTERESTING_EVENTS     (FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME )
#endif
                    lpfsic->hEvent = FindFirstChangeNotification(szPath, lpfsic->bRecursiveInterrupt, FFCN_INTERESTING_EVENTS);

                    if (lpfsic->hEvent != INVALID_HANDLE_VALUE)
                    {
                        FindNextChangeNotification(lpfsic->hEvent);
                    }
                    else
                    {
                        // FFCN will fail hitting a win95 UNC... bummer, only TF_FSNGENERAL in that case
                        TraceMsg(IsLFNDrive(szPath) ? TF_WARNING : TF_FSNGENERAL, "FindFirstChangeNotification failed on path %s, %d", szPath, GetLastError());
Punt:
                        lpfsic->iCount = 0;
                        lpfsic->hEvent = NULL;
                        lpfsic->iSuspendCount = 0;
                        goto NukeIt;
                    }

                    FSNAttachDeviceNotification(lpfsic);
                }

                ASSERT(lpfsic->hEvent);
                lphe[i] = lpfsic->hEvent;
                i++;

                // Don't allow us to overflow the maximum the system
                // supports.  Must leave one for MsgWait...
                if (i >= (MAXIMUM_WAIT_OBJECTS -1 ))
                {
                    break;
                }
            }
        }
    }

    FSNLEAVECRITICAL;

    return i;
}


void _FSN_WaitForCallbacks(void)
{
    HANDLE hCallbackEvent = OpenEvent(SYNCHRONIZE, FALSE, c_szCallbackName);
    if (!hCallbackEvent)
        return;             // No event, no callbacks...

    do {
        MSG msg;
        DWORD dwWaitResult = MsgWaitForMultipleObjects(1, &hCallbackEvent, FALSE,
                              CALLBACK_TIMEOUT, QS_SENDMESSAGE);

        TraceMsg(TF_FSNOTIFY, "FSN_WaitForCallbacks returned 0x%X", dwWaitResult);
        if (dwWaitResult == WAIT_OBJECT_0) break;   // Event completed
        if (dwWaitResult == WAIT_TIMEOUT)  break;   // Ran out of time

        if (dwWaitResult == WAIT_OBJECT_0+1) {
            //
            // Some message came in, reset message event, deliver callbacks, etc.
            //
            PeekMessage(&msg, NULL, 0, 0, PM_NOREMOVE);  // we need to do this to flush callbacks
        }
    } while (TRUE);

    TraceMsg(TF_FSNOTIFY, "FSN_WaitForCallbacks exit");
    CloseHandle(hCallbackEvent);
}


// NOTE: usually we have just one of these thread procs created for the lifetime
// of the shell, but this is written to handle multiple threads.  there are timing
// windows where one can shut down as another is starting up.  so make sure any
// code you call from here can handle multiple threads!
//
DWORD CALLBACK FSNotifyThreadProc(LPVOID lpUnused)
{
    DWORD idThread = GetCurrentThreadId();
    HANDLE ahEvents[MAXIMUM_WAIT_OBJECTS];
    int cEvents;
    int cIntEvents;
    MSG msg;
    DWORD dwWaitTime;
    DWORD dwWaitResult;
    AWAKETHREAD pat;
    // We need to initialize OLE because HIDA_FillFindData causes CoCreateIntance() to
    // be called.  In general we may be going through 3rd party Shell Extns that may want
    // to use CoCreateInstance().
    HRESULT hr = SHCoInitialize();

    // Call dummy USER API to create a message queue
    // NB Don't call PeekMessage() as that will make this the primary
    // thread and break WaitForInpuIdle()
    GetActiveWindow();

    FSNENTERCRITICAL;

    InitAliasFolderTable();

    if (g_fsnpp.htStarting && idThread == g_fsnpp.idtStarting)
    {
        g_fsnpp.htRunning = g_fsnpp.htStarting;
        g_fsnpp.idtRunning = idThread;

        g_fsnpp.htStarting = NULL;
        g_fsnpp.idtStarting = 0;
    }

    {
        pat.hThread = OpenProcess(SYNCHRONIZE,FALSE,GetCurrentProcessId());

#ifndef WINNT
        {
        // Win9x needs to convert this to global
        // I want this very tightly scoped
        HANDLE APIENTRY ConvertToGlobalHandle( HANDLE hSource );
        pat.hThread = ConvertToGlobalHandle(pat.hThread);
        }
#endif
        pat.idThread = idThread;
    }

    // BUGBUG: I am asserting this because I don't know what to do
    // if it fails
    ASSERT(pat.hThread);

    DSA_AppendItem(g_fsn.hdsaThreadAwake, &pat);

    FSNLEAVECRITICAL;

    for ( ; ; )
    {
        // check before and after waitfor to help avoid async problems.
        if (!g_fsnpp.htRunning || idThread!=g_fsnpp.idtRunning)
            break;

        cEvents = FSNBuildEventList(ahEvents, TRUE);

        // if there are events, wait a limited time only
        // cIntEvents need not be accurate, only care abou 0 vs non-0 at this moment only
        if ((g_fsnpp.hdsaIntEvents && (cIntEvents = DSA_GetItemCount(g_fsnpp.hdsaIntEvents))) ||
            FSN_EVENTSPENDING) {
            dwWaitTime = INTERRUPT_TIMEOUT;
        } else {
            cIntEvents = 0;
            dwWaitTime = INFINITE;
        }

        dwWaitResult = MsgWaitForMultipleObjects(cEvents, ahEvents, FALSE,
                dwWaitTime, QS_ALLINPUT);

        if ((int)(dwWaitResult-WAIT_OBJECT_0) <= cEvents) {
            TraceMsg(TF_FSNGENERAL, "FSNotify wait for multiple objects found %d", dwWaitResult);
        }

        if (!g_fsnpp.htRunning || idThread != g_fsnpp.idtRunning)
            break;

        if (g_fsnpp.fFlushNow) {
            // we use a process global instead of a signal so that
            // even if multiple threads signal us, we only do this once.
            // this is for flushnowait.  they want an immediate return,
            // but to flush out any changes (ie, want ui update, but
            // don't need to do anything with it.
            // need to do it here because their thread might die in which
            // case  the sendmessages don't go through
            TraceMsg(TF_FSNGENERAL, "**GotFlushNowEvent!**");
            _SHChangeNotifyHandleEvents(FALSE);
            g_fsnpp.fFlushNow = FALSE;
        }

        if ((int)(dwWaitResult-WAIT_OBJECT_0) == cEvents)
        {
            // There was some message put in our queue, so we need to dispose
            // of it
            while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                // We used to have an ASSERT(!msg.hwnd) here, but that is not always true
                // since we SHCoInitialize() above, we may acutally have OLE windows
                // on this thread (for namespace extensions), and we need to pump messages
                // to them.
                //
                if (msg.hwnd)
                {
                    TraceMsg(TF_GENERAL, "FSNotify thread proc: dispatching unknown message %#lx to hwnd %#lx", msg.message, msg.hwnd);
                    DispatchMessage(&msg);
                }
                else
                {
                    switch (msg.message)
                    {
                    case FSM_WAKEUP:
                        // Note that we do not actually do anything now; we check
                        // that some events are pending next time through the loop
                        // and wait for the WaitForMultiple to timeout
                        // Also note that since we are in a PeekMessage loop, all
                        // WAKEUP message will be removed from the queue
                        break;

                    case FSM_GOAWAY:
                        FSNENTERCRITICAL;
                        // Set the global hThread to NULL to tell this loop to end
                        if (g_fsnpp.idtRunning == idThread)
                        {
                            g_fsnpp.htRunning = NULL;
                            g_fsnpp.idtRunning = 0;
                        }
                        FSNLEAVECRITICAL;
                        break;

                    default:
                        TraceMsg(TF_ERROR, "FSNotify thread proc: eating unknown message %#lx", msg.message);
                        break;
                    }
                }
            }
        } else if (dwWaitResult == WAIT_TIMEOUT) {
            // if we had an event to
            // deal with and the wait timed out (meaning we haven't gotten
            // any new events for a while) handle it now
            if (FSN_EVENTSPENDING || cIntEvents)
                _SHChangeNotifyHandleEvents(FALSE);

            if (FSN_EVENTSPENDING)
            {
                // Apparently they did not all get flushed by the
                // HandleEvents, so wake up immediately
                WakeThread(idThread);
            }

        } else if ((int)(dwWaitResult-WAIT_OBJECT_0) < cEvents) {

            // Don't call DSA stuff if we are less than zero
            if (dwWaitResult != (DWORD)-1)
            {
                // a FindFirstChangeNotification went off.
                
                FSNENTERCRITICAL;
                
                // Note: process shutdown can nuke the hdsaIntClients list on us,
                // so double-check that it's still here.
                if (g_fsnpp.hdsaIntClients)
                {
                    LPFSIntClient lpfsic = DSA_GetItemPtr(g_fsnpp.hdsaIntClients, dwWaitResult);
                    if (lpfsic) {
                        // post and reset it.
                        FSNPostInterruptEvent(lpfsic->pidl);
                        if (0==FindNextChangeNotification(lpfsic->hEvent))
                        {
                            // Hey, the handle may be bad.  Nuke it and let FSNBuildEventList recreate
                            ASSERT(lpfsic->hEvent);
                            FSNDestructIntClient(lpfsic, FALSE); // FALSE = keep the pidl
                        }
                    }
                }
                FSNLEAVECRITICAL;
            }
        }
    }

    TraceMsg(TF_FSNGENERAL, "FSNotify is killing itself");

    _FSN_WaitForCallbacks();

    // Terminate at process cleanup, since FSNotifyThreadProc threads can come and go
    //TermAliasFolderTable();

    SHCoUninitialize(hr);
    return(0);
}


typedef struct {
    LPCITEMIDLIST pidl;
    int iCount;                 // how many of these events have we had?
    DWORD dwEventTime;          // when did we recieve this event?
} FSIntEvent, *LPFSIntEvent;


// assumes caller has checked that g_fsnpp.hdsaIntEvents exists
// and has already grabbed the semaphore
LPFSIntEvent FSNFindInterruptEvent(LPCITEMIDLIST pidl)
{
    int i;

    FSNASSERTCRITICAL;

    for ( i = DSA_GetItemCount(g_fsnpp.hdsaIntEvents) - 1  ; i >= 0  ; i-- ) 
    {
        LPFSIntEvent lpfsie = DSA_GetItemPtr(g_fsnpp.hdsaIntEvents, i);

        // check for immediate parent.  no recursive FindFirstChangeNotify allowed.
        if (ILIsEqual(lpfsie->pidl, pidl))
            return lpfsie;
    }
    return NULL;
}


BOOL _FSN_InitIntEvents(void)
{
    FSNASSERTCRITICAL;

    if (g_fsnpp.hdsaIntEvents)
        return TRUE;
    
    g_fsnpp.hdsaIntEvents = DSA_Create(SIZEOF(FSIntEvent), 4);
    if (!g_fsnpp.hdsaIntEvents)
        return FALSE;
    
    if (!g_fsn.hdpaIntEvents)
    {
        g_fsn.hdpaIntEvents = DPA_Create(4);
        if (!g_fsn.hdpaIntEvents)
            goto Error1;
    }
    
    if (DPA_AppendPtr(g_fsn.hdpaIntEvents, g_fsnpp.hdsaIntEvents) == -1)
    {
        // When we created the DPA, it should have had room for at least
        // 4 items, so we should never fail if the DPA has no items, so
        // we do not have to worry about destroying the DPA here
        ASSERT(DPA_GetPtrCount(g_fsn.hdpaIntEvents) != 0);
        goto Error1;
    }
    
    return TRUE;
    
Error1:

   DSA_Destroy(g_fsnpp.hdsaIntEvents);
   g_fsnpp.hdsaIntEvents = NULL;
   return FALSE;
}


void FSNPostInterruptEvent(LPCITEMIDLIST pidl)
{
    FSIntEvent fsie;
    LPFSIntEvent pfsie;
    int i;

#ifdef DEBUG
    {
        TCHAR szPath[MAX_PATH];
        SHGetPathFromIDList(pidl, szPath);
        TraceMsg(TF_FSNGENERAL, "FSNOTIFY: PostEvent: %s", szPath);
    }
#endif

    FSNENTERCRITICAL;

    if (_FSN_InitIntEvents())
    {
        pfsie = FSNFindInterruptEvent(pidl);

        // if we can't find the item, add it.
        if (!pfsie)
        {
            fsie.pidl = ILGlobalClone(pidl);
            if (fsie.pidl)
            {
                fsie.iCount = 0;
                i = DSA_AppendItem(g_fsnpp.hdsaIntEvents, &fsie);

                if (i != -1)
                {
                    pfsie = DSA_GetItemPtr(g_fsnpp.hdsaIntEvents, i);
                }
                else
                {
                    ILGlobalFree((LPITEMIDLIST)fsie.pidl);
                }
            }
        }

        if (pfsie)
        {
            pfsie->dwEventTime = GetTickCount();
            pfsie->iCount++;
        }
    }

    FSNLEAVECRITICAL;
}

// called when we get a GenerateEvent.  We want to remove the corresponding
// interrupt event
void FSNRemoveInterruptEvent(LPCITEMIDLIST pidl)
{
    LPFSIntEvent pfsie;
    int i, j;

    if (!g_fsn.hdpaIntEvents)
        return;

#ifdef DEBUG
    {
        TCHAR szPath[MAX_PATH];
        SHGetPathFromIDList(pidl, szPath);
        TraceMsg(TF_FSNGENERAL, "FSNOTIFY: Remove: %s", szPath);
    }
#endif

    FSNENTERCRITICAL;

    // assumes caller has checked that g_fsnpp.hdsaIntEvents exists
    // and has already grabbed the semaphore

    for ( j = DPA_GetPtrCount(g_fsn.hdpaIntEvents) - 1 ; j >= 0 ; j-- )
    {
        HDSA hdsaIntEvents = DPA_FastGetPtr(g_fsn.hdpaIntEvents, j);

        for ( i = DSA_GetItemCount(hdsaIntEvents) - 1  ; i >= 0  ; i-- )
        {
            pfsie = DSA_GetItemPtr(hdsaIntEvents, i);

#ifdef DEBUG
            {
                TCHAR szPath[MAX_PATH];
                SHGetPathFromIDList(pfsie->pidl, szPath);
                TraceMsg(TF_FSNGENERAL, "FSNOTIFY: comparing against: %s", szPath);
            }
#endif
            // check for immediate parent.  no recursive FindFirstChangeNotify allowed.
            if (ILIsParent(pfsie->pidl, pidl, TRUE) || ILIsEqual(pfsie->pidl, pidl))
            {
#ifdef DEBUG
                {
                    TCHAR szPath[MAX_PATH];
                    TraceMsg(TF_FSNGENERAL, "FSNOTIFY: RemoveEvent found: %x", pfsie);
                    SHGetPathFromIDList(pfsie->pidl, szPath);
                    TraceMsg(TF_FSNGENERAL, "FSNOTIFY: removing: %s %d", szPath, pfsie->iCount);
                }
#endif
                pfsie->iCount--;

                if (pfsie->iCount == 0)
                {
                    ILGlobalFree((LPITEMIDLIST)pfsie->pidl);
                    DSA_DeleteItem(hdsaIntEvents, i);
                }
            }
        }
    }

    FSNLEAVECRITICAL;
}


void FSNFlushInterruptEvents()
{
    LPFSIntEvent pfsie;
    int i;

    if (!g_fsnpp.hdsaIntEvents)
        return;

    FSNENTERCRITICAL;
    for (i = DSA_GetItemCount(g_fsnpp.hdsaIntEvents) - 1; i >= 0; i--)
    {
        pfsie = DSA_GetItemPtr(g_fsnpp.hdsaIntEvents, i);
        SHChangeNotifyReceiveEx(SHCNE_INTERRUPT | SHCNE_UPDATEDIR, SHCNF_IDLIST, pfsie->pidl, NULL, pfsie->dwEventTime);
        ILGlobalFree((LPITEMIDLIST)pfsie->pidl);
    }
    DSA_DeleteAllItems(g_fsnpp.hdsaIntEvents);
    FSNLEAVECRITICAL;
}

LPFSIntClient FSNFindInterruptClient(LPCITEMIDLIST pidl, LPINT lpi)
{
    int i;

    // assumes caller has checked that g_fsnpp.hdsaIntClients exists
    // and has already grabbed the semaphore
    FSNASSERTCRITICAL;

    // REVIEW: Chee, should we assert or simply return NULL?
    if (!pidl)
    {
        TraceMsg(TF_ERROR, "FSNFindInterruptClient called with NULL pidl");
        return (NULL);
    }

    for (i = DSA_GetItemCount(g_fsnpp.hdsaIntClients) - 1; i >= 0; i--)
    {
        LPFSIntClient lpfsic;

        lpfsic = DSA_GetItemPtr(g_fsnpp.hdsaIntClients, i);

        if (ILIsEqual(lpfsic->pidl, pidl))
        {
            if (lpi)
            {
                // found it
                *lpi = i;
            }
            return lpfsic;
        }
    }

    return NULL;
}


void _FSN_RemoveAwakeThread(int i)
{
    AWAKETHREAD *pAwake = DSA_GetItemPtr(g_fsn.hdsaThreadAwake, i);

    FSNASSERTCRITICAL;

    CloseHandle(pAwake->hThread);
    DSA_DeleteItem(g_fsn.hdsaThreadAwake, i);
    
    if (DSA_GetItemCount(g_fsn.hdsaThreadAwake) == 0)
    {
        DSA_Destroy(g_fsn.hdsaThreadAwake);
        g_fsn.hdsaThreadAwake = NULL;
    }
}


void _FSN_SetEvents(void)
{
    int i;
    
    if (!g_fsn.hdsaThreadAwake)
    {
        return;
    }

    FSNASSERTCRITICAL;

    for (i=DSA_GetItemCount(g_fsn.hdsaThreadAwake)-1; i>=0; --i)
    {
        AWAKETHREAD *pAwake = DSA_GetItemPtr(g_fsn.hdsaThreadAwake, i);
        if (WaitForSingleObject(pAwake->hThread, 0) == WAIT_TIMEOUT)
        {
            WakeThread(pAwake->idThread);
        }
        else
        {
            // The process associated with this thread must have
            // died unnaturally
            _FSN_RemoveAwakeThread(i);
        }
    }
}

void FSNAddInterruptClient(LPCITEMIDLIST pidl, BOOL bRecursiveInterrupt)
{
    FSIntClient fsic;
    LPFSIntClient lpfsic;
    int i;

    FSNENTERCRITICAL;

    if (!g_fsnpp.hdsaIntClients)
    {
        g_fsnpp.hdsaIntClients = DSA_Create(SIZEOF(FSIntClient), 4);

        if (!g_fsnpp.hdsaIntClients)
        {
            // failed to create the dsa, so bail
            goto Punt;
        }
    }

    lpfsic = FSNFindInterruptClient(pidl, NULL);

    // if we can't find the item, add it.
    if (!lpfsic)
    {
        ZeroMemory(&fsic, sizeof(fsic));
        fsic.pidl = ILGlobalClone(pidl);
        fsic.bRecursiveInterrupt = bRecursiveInterrupt;

        ASSERT(fsic.iCount == 0);
        // set this to null so that we'll build it on our global thread
        ASSERT(fsic.hEvent == NULL);
        ASSERT(fsic.iSuspendCount == 0);
        ASSERT(fsic.hPNP == NULL);
        ASSERT(fsic.bSuspendPNP == FALSE);

        i = DSA_AppendItem(g_fsnpp.hdsaIntClients, &fsic);

        if (i != -1)
        {
            lpfsic = DSA_GetItemPtr(g_fsnpp.hdsaIntClients, i);
        }
    }

    if (lpfsic)
    {
        lpfsic->iCount++;
        // set the event so that we'll redo the waitformultipleobjects
        WakeThread(g_fsnpp.idtRunning);
    }

Punt:
    FSNLEAVECRITICAL;
}

void FSNRemoveInterruptClient(LPCITEMIDLIST pidl)
{
    LPFSIntClient lpfsic;
    int i;

    if (!g_fsnpp.hdsaIntClients)
        return;

    // REVIEW: Chee, should we assert or simply return?
    if (!pidl)
    {
        TraceMsg(TF_ERROR, "FSNRemoveInterruptClient called with NULL pidl");
        return;
    }

    FSNENTERCRITICAL;

    lpfsic = FSNFindInterruptClient(pidl, &i);
    if (lpfsic)
    {
        lpfsic->iCount--;
        ASSERT(lpfsic->iCount >= 0);

        // rely on the next BuildEventList to remove it.
        // this keeps us from nuking an active event.
        // We only need to wake up our thread, not all
        WakeThread(g_fsnpp.idtRunning);
    }

    FSNLEAVECRITICAL;
}

void FSEventRelease(FSNotifyEvent *pfsnevt)
{
    TraceMsg(TF_FSNGENERAL, "FSEventRelease of %x called with count %d %s", pfsnevt, pfsnevt->cRef, (pfsnevt->cRef - 1) ? TEXT("") : TEXT("RELEASING!!!!!"));

    ASSERT(pfsnevt->cRef > 0);
    pfsnevt->cRef--;
    if (!pfsnevt->cRef)
        Free(pfsnevt);
}

// create an FSNotifyEvent..  but do NOT set the cRef
FSNotifyEvent* FSNAllocEvent(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    int cbPidlOrig = ILGetSize(pidl);
    int cbPidl = (cbPidlOrig + 3) & ~(0x0000003);
    int cbPidlExtra = pidlExtra ? ILGetSize(pidlExtra) : 0;
    FSNotifyEvent *pfsnevt = Alloc(cbPidl + cbPidlExtra + SIZEOF(FSNotifyEvent));
    if (pfsnevt)
    {
        pfsnevt->cRef = 1;

        if (pidl)
        {
            pfsnevt->pidl = (LPITEMIDLIST)(pfsnevt + 1);
            CopyMemory(pfsnevt->pidl, pidl, cbPidlOrig);

            if (pidlExtra)
            {
                pfsnevt->pidlExtra = (LPITEMIDLIST)(((LPBYTE)pfsnevt->pidl) + cbPidl);
                CopyMemory(pfsnevt->pidlExtra, pidlExtra, cbPidlExtra);
            }
        }

        pfsnevt->lEvent = lEvent;
        pfsnevt->dwEventTime = dwEventTime;

#ifdef FSNIDEBUG
        if (lEvent & SHCNE_UPDATEDIR) {
            ASSERT(0);
        }
#endif
    }

    return pfsnevt;
}

ULONG SHChangeNotification_Release(HANDLE hChangeNotification, DWORD dwProcId)
{
    ULONG tmp;
    LPSHChangeNotification pshcn = SHLockShared(hChangeNotification,dwProcId);
    if (!pshcn)
        return 0;

    ASSERT(pshcn->cRef > 0);
    tmp = InterlockedDecrement((LONG*)&pshcn->cRef);

    if (0 == tmp)
    {
        SHUnlockShared(pshcn);
        SHFreeShared(hChangeNotification,dwProcId);
    }
    else
    {
        SHUnlockShared(pshcn);
    }

    return tmp;
}

HANDLE SHChangeNotification_Create(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidlMain, LPCITEMIDLIST pidlExtra, DWORD dwProcId, DWORD dwEventTime)
{
    LPBYTE  lpb;
    UINT    cbPidlOrig;
    UINT    cbPidl;
    UINT    cbPidlExtra;
    DWORD   dwSize;
    HANDLE  hChangeNotification;
    LPSHChangeNotification pshcn;

    cbPidlOrig = ILGetSize(pidlMain);
    cbPidlExtra = pidlExtra ? ILGetSize(pidlExtra) : 0;

    cbPidl = (cbPidlOrig + 3) & ~(0x0000003);       // Round up to dword size
    dwSize = SIZEOF(SHChangeNotification) + cbPidl + cbPidlExtra;

    hChangeNotification = SHAllocShared(NULL,dwSize,dwProcId);
    if (!hChangeNotification)
        return (HANDLE)NULL;

    pshcn = SHLockShared(hChangeNotification,dwProcId);
    if (!pshcn)
    {
        SHFreeShared(hChangeNotification,dwProcId);
        return (HANDLE)NULL;
    }

    pshcn->dwSize   = dwSize;
    pshcn->lEvent   = lEvent;
    pshcn->uFlags   = uFlags;
    pshcn->cRef     = 1;
    pshcn->dwEventTime = dwEventTime;

    lpb = (LPBYTE)(pshcn+1);
    pshcn->uidlMain = (UINT) (lpb - (LPBYTE)pshcn);
    CopyMemory(lpb, pidlMain, cbPidlOrig);
    lpb += cbPidl;

    if (pidlExtra)
    {
        pshcn->uidlExtra = (UINT) (lpb - (LPBYTE)pshcn);
        CopyMemory(lpb, pidlExtra, cbPidlExtra);
    }
    SHUnlockShared(pshcn);

    return hChangeNotification;
}

#define SHCNL_SIG   0xbabababa

LPSHChangeNotificationLock SHChangeNotification_Lock(HANDLE hChangeNotification, DWORD dwProcId, LPITEMIDLIST **pppidl, LONG *plEvent)
{
    LPSHChangeNotificationLock pshcnl;
    LPSHChangeNotification pshcn = SHLockShared(hChangeNotification,dwProcId);
    if (!pshcn)
        return NULL;

    // BUGBUG - Bobday - We could alloc this structure on the calling functions stack for faster execution
    pshcnl = (LPSHChangeNotificationLock)LocalAlloc(LPTR, SIZEOF(SHChangeNotificationLock));
    if (!pshcnl)
    {
        SHUnlockShared(pshcn);
        return NULL;
    }

    pshcnl->pshcn       = pshcn;
#ifdef DEBUG
    pshcnl->dwSignature = SHCNL_SIG;
#endif
    pshcnl->pidlMain    = NULL;
    pshcnl->pidlExtra   = NULL;

    if (pshcn->uidlMain)
        pshcnl->pidlMain  = (LPITEMIDLIST)((LPBYTE)pshcn + pshcn->uidlMain);

    if (pshcn->uidlExtra)
        pshcnl->pidlExtra = (LPITEMIDLIST)((LPBYTE)pshcn + pshcn->uidlExtra);

    //
    // Give back some easy values (causes less code to change for now)
    //
    if (pppidl)
        *pppidl = (LPITEMIDLIST *)&(pshcnl->pidlMain);

    if (plEvent)
        *plEvent = pshcnl->pshcn->lEvent;

    return pshcnl;
}

BOOL SHChangeNotification_Unlock(LPSHChangeNotificationLock pshcnl)
{
    ASSERT(pshcnl->dwSignature == SHCNL_SIG);

    SHUnlockShared(pshcnl->pshcn);

    return (LocalFree(pshcnl) == NULL);
}

void _SHChangeNotifyEmptyEventsList(HDPA hdpaEvents)
{
    int iCount = DPA_GetPtrCount(hdpaEvents);
    while (iCount--) 
    {
        FSNotifyEvent *pfsnevt = (FSNotifyEvent *)DPA_FastGetPtr(hdpaEvents, iCount);
        FSEventRelease(pfsnevt);
    }
    DPA_DeleteAllPtrs(hdpaEvents);
}

//
// Simple function to try to make the FSNOTIFY code properly handle the SCHCNE_UPDATEDIR
// when some objects in parents or sibling folders are changed also...
//
void _StripPidlToCommonParent(LPITEMIDLIST pidl1, LPITEMIDLIST pidl2, LPCITEMIDLIST pidlWatch)
{
    if (!pidl1 || !pidl2)
        return;

    // Use the watch pidl to walk forward till we are at least at the root of the 
    // wathced directory, since both pidl1 and pidl2 have to be children or equal
    // to the watch pidl
    if (pidlWatch)
    {
        LPITEMIDLIST pidlTemp = (LPITEMIDLIST)pidlWatch;

        while (!ILIsEmpty(pidlTemp))
        {
            pidlTemp = _ILNext(pidlTemp);
            pidl1 = _ILNext(pidl1);
            pidl2 = _ILNext(pidl2);
        }
    }

    while (!ILIsEmpty(pidl1))
    {
        // If they are not equal truncate pidl1 at this point...

        // NOTE: it's kind of silly not to do a real compare, since we
        // call ILIsEqual everywhere else as well...  Why does this one
        // call need to be so much faster?
        //
        // BUGBUG (reinerf) - doing a memcmp is BAD because the pids could
        // be equal but one simple and one full, and the memcmp would fail
        // but we cant bind because we need to be fast
        if ((pidl1->mkid.cb != pidl2->mkid.cb) ||
                (memcmp(pidl1, pidl2, pidl1->mkid.cb) != 0))
        {
#ifdef DEBUG
            TCHAR szOriginalPath[MAX_PATH];
            TCHAR szTruncatedPath[MAX_PATH];

            SHGetPathFromIDList(pidl1, szOriginalPath);
#endif
            pidl1->mkid.cb = 0;    // Truncate the rest off of the pidl...

#ifdef DEBUG
            SHGetPathFromIDList(pidl1, szTruncatedPath);
            TraceMsg(TF_FSNGENERAL, "Stripped pidl from: %s -> %s", szOriginalPath, szTruncatedPath);
#endif
            break;
        }

        pidl1 = _ILNext(pidl1);
        pidl2 = _ILNext(pidl2);
    }
}


//
// This function decideds whether or not we should ignore a genuine filesystem SHCNE_UPDATEDIR event,
// because we have more specific event already in our queue that occured within UPDATEDIR_OVERRIDE_TIME of the
// UPDATEDIR event.
//
BOOL _ShouldIgnoreFSUpdateDirEvent(FSNotifyEvent *pfsnevt, HDPA hdpaEvents)                      
{ 
    int iCount = DPA_GetPtrCount(hdpaEvents);

    ASSERT((pfsnevt->lEvent & (SHCNE_INTERRUPT | SHCNE_UPDATEDIR)) == (SHCNE_INTERRUPT | SHCNE_UPDATEDIR));

    while (iCount)
    {
        FSNotifyEvent* pfsnevtOld = DPA_FastGetPtr(hdpaEvents, --iCount);

        if ((pfsnevtOld->lEvent & SHCNE_UPDATEDIR_OVERRIDE_EVENTS) && 
            (pfsnevt->dwEventTime - pfsnevtOld->dwEventTime < UPDATEDIR_OVERRIDE_TIME))
        {
            LPITEMIDLIST pidlTemp;

            // Found an old event that could possibly override the UPDATEDIR we are processing.
            // Now find out what directory that event relates to.

            if (pfsnevtOld->lEvent & (SHCNE_RENAMEITEM | SHCNE_RENAMEFOLDER))
            {
                // When doing renames, the pidlExtra has the target, so that is
                // what is important to us
                pidlTemp = ILClone(pfsnevtOld->pidlExtra);
            }
            else
            {
                pidlTemp = ILClone(pfsnevtOld->pidl);
            }

            if (pidlTemp)
            {
                DWORD dwAttribs = SFGAO_FOLDER;

                if (SUCCEEDED(SHGetNameAndFlags(pidlTemp, SHGDN_NORMAL, NULL, 0, &dwAttribs)) &&
                    !(dwAttribs & SFGAO_FOLDER))
                {
                    // The old event we found relates to an item. We therefore whack off the last ID so we can see 
                    // if the parent folder of this event is the same as the pidl of the UPDATEDIR that we are currently processing
                    ILRemoveLastID(pidlTemp);
                }

                if (ILIsEqual(pfsnevt->pidl, pidlTemp))
                {
                    // This UPDATEDIR is going to be ignored because we are assuming that it is related to the 
                    // pfsnevtOld that was already in our queue.
                    TraceMsg(TF_FSNGENERAL, "****FSNotify: Ignoring filesystem SHCNE_UPDATEDIR event on %s because", DumpPidl(pfsnevt->pidl));
                    TraceMsg(TF_FSNGENERAL, "              event %d on %s already in the queue was within 2 seconds and more specific", pfsnevtOld->lEvent, DumpPidl(pfsnevtOld->pidl));
                    ILFree(pidlTemp);
                    return TRUE;
                }

                ILFree(pidlTemp);
            }
        }
    }

    return FALSE;
}


//
// checks for null so we dont assert in ILIsEqual
//
BOOL ILIsEqualOrBothNull(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    if (!pidl1 || !pidl2)
    {
        return (pidl1 == pidl2);
    }
    
    return ILIsEqual(pidl1, pidl2);
}


//
// This searches the hdpaEvents for an event that matches pfsnevt, and
// returns the matching event if possible
//
FSNotifyEvent* FindIdenticalExistingEvent(HDPA hdpaEvents, FSNotifyEvent* pfsnevt)
{
    int i;
    int cEvents = DPA_GetPtrCount(hdpaEvents);

    // we scan the list in reverse order, should help perf a bit
    // since later events are more likely to be identical to this one
    for (i = cEvents - 1; i >= 0; i--)
    {
        FSNotifyEvent* pfsnevtOld = DPA_FastGetPtr(hdpaEvents, i);

        if (pfsnevtOld->lEvent != pfsnevt->lEvent)
        {
            continue;
        }

        if (!ILIsEqualOrBothNull(pfsnevtOld->pidl, pfsnevt->pidl) ||
            !ILIsEqualOrBothNull(pfsnevtOld->pidlExtra, pfsnevt->pidlExtra))
        {
            continue;
        }

        // same lEvent and pidls, therefore its the same event
        return pfsnevtOld;
    }

    return NULL;
}


//
// returns:     TRUE if we are going to do an UPDATEDIR
//              FALSE otherwise
//
BOOL _SHChangeNotifyAddEventToHDPA(FSNotifyClientInfo *pfsnci, FSNotifyEvent *pfsnevt, 
                                   BOOL fAllowCollapse, BOOL fRecursive, LPCITEMIDLIST pidlWatch)
{
    BOOL fUpdateClock = FALSE;
    HDPA hdpaEvents;
    BOOL fRelease = FALSE;
    BOOL bUpdateDir = FALSE;
    int iCount;

    FSNENTERCRITICAL;

    // We need to make sure HandleEvents does not attempt to flush this
    // HDSA while we are adding things to it
    hdpaEvents = pfsnci->hdpaPendingEvents;

    iCount = DPA_GetPtrCount(hdpaEvents);

    // Check to see if we can collapse this event or get rid of it altogether
    // because of more specific events already in our queue
    if ((iCount > 0) && (pfsnevt->lEvent & SHCNE_DISKEVENTS) && fAllowCollapse)
    {
        FSNotifyEvent* pfsnevtOld = FindIdenticalExistingEvent(hdpaEvents, pfsnevt);

        if (pfsnevtOld)
        {
            // we found an indentical event that matches pfsnevt already in our queue,
            // so just update the time stamp
            pfsnevtOld->dwEventTime = max(pfsnevt->dwEventTime, pfsnevtOld->dwEventTime);

            // dont need to add anything
            fUpdateClock = TRUE;
            goto ProcessEvent;
        }

        pfsnevtOld = DPA_FastGetPtr(hdpaEvents, iCount - 1);
        
        //
        // If we get too many messages in the queue at any given time,
        // we set the last message in the cue to be an UPDATEDIR that will
        // stand for all messages that we cant fit because the queue is full.
        //

        if (iCount >= MAX_EVENT_COUNT)
        {
            // if the ref is != 1 then we cannot modify this event since multiple queue's refrence it,
            // so instead we add another UPDATEDIR event
            if ((pfsnevtOld->lEvent & SHCNE_UPDATEDIR) && (pfsnevtOld->cRef == 1))
            {
                // We need to make pfsnevtOld represent this event as well. Check to 
                // see if this is a recursive noitify or not
                if (fRecursive)
                {
                    BOOL bUsePidl;
                    BOOL bUsePidlExtra;
                    DWORD dwAttribs = SFGAO_FOLDER;
                    LPITEMIDLIST pidl;
                    LPITEMIDLIST pidlExtra;

                    // we need to clone the pidls from pfsnevt since we might be whacking on them. It is ok to just clone them out of 
                    // our heap since they are not going into an actual event, but are instead going to be used to possibly modify the
                    // existing pfsnevtOld->pidl
                    pidl = ILClone(pfsnevt->pidl);
                    pidlExtra = pfsnevt->pidlExtra ? ILClone(pfsnevt->pidlExtra) : NULL;

                    if (!pidl || (pfsnevt->pidlExtra && !pidlExtra))
                    {
                        // doh, failed to clone, bail
                        TraceMsg(TF_FSNGENERAL, "_SHChangeNotifyAddEventToHDPA: failed to clone pidl or pidlExtra, cant add event to client queue!!");
                        ILFree(pidl); // ILFree won't barf on NULL
                        ILFree(pidlExtra); // ILFree won't barf on NULL
                        pfsnevt = NULL;
                        goto ProcessEvent;
                    }

                    if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_NORMAL, NULL, 0, &dwAttribs)) &&
                        !(dwAttribs & SFGAO_FOLDER))
                    {
                        // The pidl relates to an item and we are turning this into an UPDATEDIR, so remove the last id
                        ILRemoveLastID(pidl);
                    }
                    
                    dwAttribs = SFGAO_FOLDER;
                    if (pidlExtra &&
                        SUCCEEDED(SHGetNameAndFlags(pidlExtra, SHGDN_NORMAL, NULL, 0, &dwAttribs)) &&
                        !(dwAttribs & SFGAO_FOLDER))
                    {
                        // The pidlExtra relates to an item and we are turning this into an UPDATEDIR, so remove the last id
                        ILRemoveLastID(pidlExtra);
                    }

                    // BUGBUG should have ILIsChildOrSelf to avoid double-walk
                    bUsePidl = (ILIsEqual(pidlWatch, pidl) || ILIsParent(pidlWatch, pidl, FALSE));
                    bUsePidlExtra = (pidlExtra && (ILIsEqual(pidlWatch, pidlExtra) || ILIsParent(pidlWatch, pidlExtra, FALSE)));

                    ASSERT(bUsePidl || bUsePidlExtra);

                    if (bUsePidl && bUsePidlExtra)
                    {
                        // find the common parent of pidl and pidlExtra, then change the old UPDATEDIR to accomodate both of them.
                        _StripPidlToCommonParent(pidl, pidlExtra, pidlWatch);
                        _StripPidlToCommonParent((LPITEMIDLIST)pfsnevtOld->pidl, pidl, pidlWatch);
                    }
                    else if (bUsePidl)
                    {
                        _StripPidlToCommonParent((LPITEMIDLIST)pfsnevtOld->pidl, pidl, pidlWatch);
                    }
                    else if (bUsePidlExtra)
                    {
                        _StripPidlToCommonParent((LPITEMIDLIST)pfsnevtOld->pidl, pidlExtra, pidlWatch);
                    }

                    // the UPDATEDIR event we just modified had better be equal to or a child of our watch pidl!!
                    ASSERT(ILIsEqual(pidlWatch, pfsnevtOld->pidl) || ILIsParent(pidlWatch, pfsnevtOld->pidl, FALSE));

                    ILFree(pidl);
                    ILFree(pidlExtra); // ILFree won't barf on NULL
                }
                else
                {
                    // In the non-recursive case we dont have to do anything, because the only UPDATEDIR message that
                    // we could possibly have must be on the pidl that the watch was registered for.
                    ASSERT(ILIsEqual(pidlWatch, pfsnevtOld->pidl));
                }

                // We have modified the existing UPDATEDIR to fit our needs, so there is no
                // more work to be done.
                bUpdateDir = TRUE;
                fUpdateClock = TRUE;
                goto ProcessEvent;
            }
            else
            {
                if (fRecursive)
                {
                    BOOL bUsePidl;
                    BOOL bUsePidlExtra;
                    DWORD dwAttribs = SFGAO_FOLDER;
                    LPITEMIDLIST pidl;
                    LPITEMIDLIST pidlExtra;
                    int i = iCount;

                    // we need to clone the pidls from pfsnevt since we might be whacking on them. It is ok to just clone them out of 
                    // our heap since they are not going into an actual event, but are instead going to be used to create a new event
                    pidl = ILClone(pfsnevt->pidl);
                    pidlExtra = pfsnevt->pidlExtra ? ILClone(pfsnevt->pidlExtra) : NULL;

                    if (!pidl || (pfsnevt->pidlExtra && !pidlExtra))
                    {
                        // doh, failed to clone, bail
                        TraceMsg(TF_FSNGENERAL, "_SHChangeNotifyAddEventToHDPA: failed to clone pidl or pidlExtra, cant add event to client queue!!");
                        ILFree(pidl); // ILFree won't barf on NULL
                        ILFree(pidlExtra); // ILFree won't barf on NULL
 
                        pfsnevt = NULL;
                        goto ProcessEvent;
                    }

                    // Since the queue is full we need to add an UPDATEDIR that will best represent this event.
                    if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_NORMAL, NULL, 0, &dwAttribs)) &&
                        !(dwAttribs & SFGAO_FOLDER))
                    {
                        // The pidl relates to an item and we are turning this into an UPDATEDIR, so remove the last id
                        ILRemoveLastID(pidl);
                    }

                    dwAttribs = SFGAO_FOLDER;
                    if (pidlExtra &&
                        SUCCEEDED(SHGetNameAndFlags(pidlExtra, SHGDN_NORMAL, NULL, 0, &dwAttribs)) &&
                        !(dwAttribs & SFGAO_FOLDER))
                    {
                        // The pidlExtra relates to an item and we are turning this into an UPDATEDIR, so remove the last id
                        ILRemoveLastID(pidlExtra);
                    }

                    // BUGBUG should have ILIsChildOrSelf to avoid double-walk
                    bUsePidl = (ILIsEqual(pidlWatch, pidl) || ILIsParent(pidlWatch, pidl, FALSE));
                    bUsePidlExtra = (pidlExtra && (ILIsEqual(pidlWatch, pidlExtra) || ILIsParent(pidlWatch, pidlExtra, FALSE)));

                    ASSERT(bUsePidl || bUsePidlExtra);

                    if (bUsePidl && bUsePidlExtra)
                    {
                        // find the common parent of pidl and pidlExtra, so we can create the most general UPDATEDIR event
                        _StripPidlToCommonParent(pidl, pidlExtra, pidlWatch);
                    }

                    // Alloc the new updatedir event based on the event we are currently processing
                    if (bUsePidlExtra && !bUsePidl)
                    {
                        pfsnevt = FSNAllocEvent(SHCNE_UPDATEDIR, pidlExtra, NULL, pfsnevt->dwEventTime);
                    }
                    else
                    {
                        pfsnevt = FSNAllocEvent(SHCNE_UPDATEDIR, pidl, NULL, pfsnevt->dwEventTime);
                    }
                    fRelease = TRUE;

                    // the UPDATEDIR event we just created had better be equal to or a child of our watch pidl!!
                    ASSERT(ILIsEqual(pidlWatch, pfsnevt->pidl) || ILIsParent(pidlWatch, pfsnevt->pidl, FALSE));

                    ILFree(pidl);
                    ILFree(pidlExtra); // ILFree won't barf on NULL
                }
                else
                {
                    // Non-recursive case is easy, we just add an UPDATEDIR for the pidl being watched
                    pfsnevt = FSNAllocEvent(SHCNE_UPDATEDIR, pidlWatch, NULL, pfsnevt->dwEventTime);
                    fRelease = TRUE;
                }
            }
        }

        // Check to see if this is an interrupt (eg filesystem) UPDATEDIR that we can ignore since we already
        // have the more specific shell related event in our queue that cause this UPDATEDIR.
        if (((pfsnevt->lEvent & (SHCNE_INTERRUPT | SHCNE_UPDATEDIR)) == (SHCNE_INTERRUPT | SHCNE_UPDATEDIR)) &&
            _ShouldIgnoreFSUpdateDirEvent(pfsnevt, hdpaEvents))
        {
            // skip this event
            fUpdateClock = TRUE;
        }
   }

ProcessEvent:
    //
    //  If fUpdateClock is already set it means that we had a redundant
    //  message and don't want to process it.
    //
    if (!fUpdateClock && pfsnevt)
    {
        TraceMsg(TF_FSNGENERAL, "****FSNotify: DPA_InsertPtr for hwnd %x hdpa = %x, pfsnevt = %x", pfsnci->hwnd, hdpaEvents, pfsnevt);
        if (DPA_AppendPtr(hdpaEvents, pfsnevt) != -1)
        {
            pfsnevt->cRef++;
            fUpdateClock = TRUE;
        }
    }

    if (fRelease)
        FSEventRelease(pfsnevt);

    if (fUpdateClock)
    {
        // We always need to set the events in case the client that cares about
        // this event is in another process.
        g_fsn.dwLastEvent = GetCurrentTime();
        _FSN_SetEvents();
    }

    FSNLEAVECRITICAL;

    return bUpdateDir;
}

//
// returns:     TRUE if every (receiving) client does an UPDATEDIR
//              FALSE otherwise
//
BOOL _SHChangeNotifyAddEventToClientQueues(LONG lEvent,
        LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    FSNotifyClientInfo * pfsnci;
    FSNotifyEvent* pfsnevt = NULL;
    BOOL bOnlyUpdateDirs = TRUE;

    if (!g_fsn.pfsnciFirst)
        return bOnlyUpdateDirs;

    FSNENTERCRITICAL;
    // Don't let clients get deleted out of the DSA; we'll just mark them
    // as "deletion pending" and delete them when handling events
    ++g_fsn.cRefClientList;
    FSNLEAVECRITICAL;

    //
    //  Add this event to all the relevant queues.
    //

    for (pfsnci = g_fsn.pfsnciFirst; pfsnci; pfsnci=pfsnci->pfsnciNext)
    {
        int iEntry;
        BOOL fShouldAdd;
        LPCITEMIDLIST pidlRoot = NULL;
        BOOL fAllowCollapse;
        BOOL fRecursive;

        if (pfsnci->iSerializationFlags & FSSF_DELETE_ME)
        {
            // No use adding events to deleted clients
            continue;
        }

        if (lEvent & SHCNE_INTERRUPT)
        {
            if (!(pfsnci->fSources & SHCNRF_InterruptLevel))
            {
                //
                //  This event was generated by an interrupt, and the
                //  client has interrupt notification turned off, so
                //  we skip it.
                //

                continue;
            }
        }
        else if (!(pfsnci->fSources & SHCNRF_ShellLevel))
        {
            //
            //  This event was generated by the shell, and the
            //  client has shell notification turned off, so
            //  we skip it.
            //

            continue;
        }

        //
        //  If this client is not interested in the event, skip to next client.
        //
        if (!(pfsnci->fEvents & lEvent))
            continue;

        fShouldAdd = FALSE;
        for (iEntry = 0; !fShouldAdd && (iEntry < DSA_GetItemCount(pfsnci->hdsaNE)); iEntry++)
        {
            SHChangeNotifyEntry *pfsne = DSA_GetItemPtr(pfsnci->hdsaNE, iEntry);

            fRecursive = pfsne->fRecursive;
            pidlRoot = pfsne->pidl;

            // Check if this is a global notify.
            if (pfsne->pidl && !(lEvent & SHCNE_GLOBALEVENTS))
            {
                fAllowCollapse = TRUE;
                // No, we need to filter out unrelated events.
                if (fRecursive)
                {
                    //
                    //  This case treats directory entries with the recursive
                    //  bit set.
                    //

                    if (ILIsParent(pfsne->pidl, pidl, FALSE) ||
                        (pidlExtra && ILIsParent(pfsne->pidl, pidlExtra, FALSE)))
                    {
                        fShouldAdd = TRUE;
                    }
                }
                else
                {
                    //
                    //  In the non-recursive case we need to check to see if pidl or pidl extra matches us
                    //  or is our immeidate child.
                    //

                    if (ILIsEqual(pfsne->pidl, pidl) || ILIsParent(pfsne->pidl, pidl, TRUE) ||
                        (pidlExtra && ILIsParent(pfsne->pidl, pidlExtra, TRUE) || ILIsParent(pfsne->pidl, pidl, TRUE)))
                    {
                        fShouldAdd = TRUE;
                    }
                }
            } 
            else  
            {
                fAllowCollapse = FALSE;
                fShouldAdd = TRUE;
            }
        }

        // do this after the event loop so that we don't
        // add events multiple times (as in the case of a rename)
        if (fShouldAdd) 
        {
            // we try to be clever here and re-use the same event and refcount it.
            // this means that _SHChangeNotifyAddEventToHDPA is NOT allowed to change the event
            if (!pfsnevt)
            {
                // make the event struct
                pfsnevt = FSNAllocEvent(lEvent, pidl, pidlExtra, dwEventTime);
                if (!pfsnevt)
                {
                    // out of memory... bail!
                    return bOnlyUpdateDirs;
                }
            }

            if (!_SHChangeNotifyAddEventToHDPA(pfsnci, pfsnevt, fAllowCollapse, fRecursive, pidlRoot))
            {
                bOnlyUpdateDirs = FALSE;
            }
        }
    }  // End client loop

    if (pfsnevt)
        FSEventRelease(pfsnevt);

    FSNENTERCRITICAL;
    // This needs to be in a critical section so we don't accidentally
    // do this on two threads at the same time...
    --g_fsn.cRefClientList;
    FSNLEAVECRITICAL;

    return bOnlyUpdateDirs;
}


BOOL IsILShared(LPCITEMIDLIST pidl, BOOL fUpdateCache)
{
    TCHAR szTemp[MAXPATHLEN];
    SHGetPathFromIDList(pidl, szTemp);
    return IsShared(szTemp, fUpdateCache);
}

//
//  Structure that tracks an "alias folder".  An alias folder is a folder
//  in the namespace which is an alias for one -- possibly two -- file
//  system folders.  If two, then the second one is a "common" folder
//  in the sense that it requires SHID_FS_COMMONITEM.
//
//  Examples:
//
//  Desktop = CSIDL_DESKTOPDIRECTORY + CSIDL_COMMON_DESKTOPDIRECTORY;
//  MyDocs  = CSIDL_PERSONAL;
//
//  The csidl and csidlCommon >>must<< refer to filesystem folders.
//

typedef void (*ALIASCALLBACK)(LPCITEMIDLIST pidl, LONG lEvent);

typedef struct ALIASFOLDER {
    LPITEMIDLIST pidlAlias;             // The alias folder
    int     csidl;                      // CSIDL_* code for alias folder
    int     csidlCommon;                // CSIDL_* code for common alias folder
    ALIASCALLBACK InvalidateEnumCache;  // Hack for CDesktop
} ALIASFOLDER, *PALIASFOLDER;

void Desktop_InvalidateEnumCache(LPCITEMIDLIST pidl, LONG lEvent);

//
//  Static alias folder goo.
//
ALIASFOLDER s_rgaf[4] = {

  // desktop
  { 0, CSIDL_DESKTOPDIRECTORY, CSIDL_COMMON_DESKTOPDIRECTORY, Desktop_InvalidateEnumCache },

  // My Documents
  { 0, CSIDL_PERSONAL,   -1, NULL },

  // NetHood
  { 0, CSIDL_NETHOOD,    -1, NULL },

  // PrintHood
  { 0, CSIDL_PRINTHOOD,  -1, NULL },
};

#define IAF_DESKTOP     0
#define IAF_MYDOCS      1
#define IAF_NETHOOD     2
#define IAF_PRINTHOOD   3

#define ATOMICILFREE(pidl)  if (pidl) { ILFree(pidl); pidl = 0; }

void InitAliasFolderEntry(ALIASFOLDER *paf, LPITEMIDLIST pidlAlias)
{
    if (IS_INTRESOURCE(pidlAlias))
        paf->pidlAlias = SHCloneSpecialIDList(NULL, PtrToLong(pidlAlias), TRUE);
    else
        paf->pidlAlias = pidlAlias;
}

void InitAliasFolderTable(void)
{
    if (!s_rgaf[IAF_DESKTOP  ].pidlAlias )
        InitAliasFolderEntry(&s_rgaf[IAF_DESKTOP  ], (LPITEMIDLIST)MAKEINTRESOURCE(CSIDL_DESKTOP));

    if (!s_rgaf[IAF_MYDOCS   ].pidlAlias)
        InitAliasFolderEntry(&s_rgaf[IAF_MYDOCS   ], MyDocsIDList());

    if (!s_rgaf[IAF_NETHOOD  ].pidlAlias)
        InitAliasFolderEntry(&s_rgaf[IAF_NETHOOD  ], (LPITEMIDLIST)MAKEINTRESOURCE(CSIDL_NETWORK));

    if (!s_rgaf[IAF_PRINTHOOD].pidlAlias)
        InitAliasFolderEntry(&s_rgaf[IAF_PRINTHOOD], (LPITEMIDLIST)MAKEINTRESOURCE(CSIDL_PRINTERS));
}

#define ATOMICILFREE(pidl)  if (pidl) { ILFree(pidl); pidl = 0; }

void TermAliasFolderEntry(PALIASFOLDER paf)
{
    ATOMICILFREE(paf->pidlAlias);
    paf->InvalidateEnumCache = NULL;
}

void TermAliasFolderTable(void)
{
    TermAliasFolderEntry(&s_rgaf[0]);
    TermAliasFolderEntry(&s_rgaf[1]);
    TermAliasFolderEntry(&s_rgaf[2]);
    TermAliasFolderEntry(&s_rgaf[3]);
}

#if 0
//
//  Called from SetFolderPath when the user renames a special folder.
//  See if it's one of the special folders in our cache, in which case
//  we reload it.
//
void UpdateAliasFolderTable(int csidl)
{
    int i;
    for (i = 0; i < ARRAYSIZE(s_rgaf); i++)
    {
        if (s_rgaf[i].csidl == csidl || s_rgaf[i].csidlCommon == csidl)
        {
            // Re-initialize the alias folder entry with itself.
            // This will reload all the csidl goo
            FSNENTERCRITICAL;
            InitAliasFolderEntry(&s_rgaf[i], s_rgaf[i].pidlAlias);
            FSNLEAVECRITICAL;
        }

    }
}
#endif


LPCITEMIDLIST CheckAliasPidl(LPCITEMIDLIST pidlAlias, int csidl, LPCITEMIDLIST pidl, BOOL fCommon)
{
    LPCITEMIDLIST pidlRet = NULL;

    if (csidl >= 0)
    {
        LPITEMIDLIST pidlFolder;
        if (SHGetFolderLocation(NULL, csidl, NULL, 0, &pidlFolder) == S_OK)
        {
            LPCITEMIDLIST pidlChild = ILFindChild(pidlFolder, pidl);
            if (pidlChild)
            {
                DebugDumpPidl(TEXT("FSNotify::CreateAliasPidl Found - "), pidl);
                DebugDumpPidl(TEXT("for folder"), pidlFolder);
                // Match found in folder; convert it to the alias.
                pidlRet = ILCombine(pidlAlias, pidlChild);
                ASSERT(IS_VALID_PIDL(pidlRet));

                // If the child is empty, we don't want to do antying modification
                // to the resultant pidl. This can happen if we're doing a update dir
                // of the common item itself.
                if (pidlRet && fCommon && !ILIsEmpty(pidlChild))
                {
                    // Ooh wait, need to touch up the pidl so it is SHID_FS_COMMON
                    LPITEMIDLIST pidlT = (LPITEMIDLIST)((LPBYTE)pidlRet + ILGetSize(pidlAlias) - sizeof(pidl->mkid.cb));
                    ASSERT(memcmp(pidlT, pidlChild, ILGetSize(pidlChild)) == 0);
                    pidlT->mkid.abID[0] |= SHID_FS_COMMONITEM;
                }
            }
            ILFree(pidlFolder);
        }
    }
    return pidlRet;
}

//
//  If the pidl is in a folder that we are aliasing, then create and return
//  its alias version.  Otherwise, return the original.
//
LPCITEMIDLIST CreateAliasPidl(PALIASFOLDER paf, LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlRet;

    pidlRet = CheckAliasPidl(paf->pidlAlias, paf->csidl, pidl, FALSE);
    if (!pidlRet)
    {
        pidlRet = CheckAliasPidl(paf->pidlAlias, paf->csidlCommon, pidl, TRUE);
    }

    // If unable to translate, then just use the original guy
    if (pidlRet == NULL)
        pidlRet = pidl;

    return pidlRet;
}

//
//  If a ChangeNotify arrives for one of our filesystem shadows,
//  generate a matching notify in its alias folder as well.
//
void CreateAliasNotification(PALIASFOLDER paf, LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    LPCITEMIDLIST pidlAlias = pidl;
    LPCITEMIDLIST pidlExtraAlias = pidlExtra;

    if (lEvent & (SHCNE_DISKEVENTS | SHCNE_NETSHARE | SHCNE_NETUNSHARE))
    {
        pidlAlias = CreateAliasPidl(paf, pidl);
        if (pidlExtra)
            pidlExtraAlias = CreateAliasPidl(paf, pidlExtra);

        if (SHCNE_RENAMEFOLDER == lEvent)
        {
            // If we created an alias pidl from the target folder itself and the
            // event indicates that the target folder has been renamed, then we should
            // not propagate a bogus notification that the alias has been renamed:
            if ((pidl != pidlAlias) && ILIsEqual(pidlAlias, paf->pidlAlias))
            {
                ILFree((LPITEMIDLIST)pidlAlias);
                pidlAlias = pidl;
            }
            if ((pidlExtra != pidlExtraAlias) && ILIsEqual(pidlExtraAlias, paf->pidlAlias))
            {
                ILFree((LPITEMIDLIST)pidlExtraAlias);
                pidlExtraAlias = pidlExtra;
            }
        }

        // Make sure that we invalidate cache first before we may process events...
        if (paf->InvalidateEnumCache)
        {
            // whether to invalidate the enum cache
            paf->InvalidateEnumCache(pidl, lEvent);
            if (pidlExtra)
                paf->InvalidateEnumCache(pidlExtra, lEvent);
            if (pidl != pidlAlias)
                paf->InvalidateEnumCache(pidlAlias, lEvent);
            if (pidlExtra != pidlExtraAlias)
                paf->InvalidateEnumCache(pidlExtraAlias, lEvent);
        }

        if (pidl != pidlAlias || pidlExtra != pidlExtraAlias)
        {
            DebugDumpPidl(TEXT("FSNotify::CreateAliasNotification - pidl"), pidlAlias);
            DebugDumpPidl(TEXT("FSNotify::CreateAliasNotification - pidlExtra"), pidlExtraAlias);
            SHChangeNotifyReceiveEx(lEvent, SHCNF_NONOTIFYINTERNALS | SHCNF_IDLIST, pidlAlias, pidlExtraAlias, dwEventTime);
        }

        if (pidl != pidlAlias)
            ILFree((LPITEMIDLIST)pidlAlias);

        if (pidlExtra != pidlExtraAlias)
            ILFree((LPITEMIDLIST)pidlExtraAlias);
    }
}

typedef struct {
    WORD cb;
    LONG lEEvent;
    WORD null;
} ALIASREGISTER;

// in order to make an ALIASREGISTER look like a pidl
#define CBALIASREGISTER     (SIZEOF(ALIASREGISTER) - SIZEOF(LPCITEMIDLIST))

STDAPI_(void) SHChangeNotifyRegisterAlias(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    static const ALIASREGISTER ar = {CBALIASREGISTER, SHCNEE_ALIASINUSE, 0};
    LPITEMIDLIST pidlRegister = ILCombine((LPCITEMIDLIST)&ar, pidlReal);

    if (pidlRegister)
    {
        SHChangeNotify(SHCNE_EXTENDED_EVENT, SHCNF_ONLYNOTIFYINTERNALS | SHCNF_IDLIST, pidlRegister, pidlAlias);
        ILFree(pidlRegister);
    }
}

LPCITEMIDLIST IsAliasRegisterPidl(LPCITEMIDLIST pidl)
{
    ALIASREGISTER *par = (ALIASREGISTER *)pidl;

    if (par->cb == CBALIASREGISTER
    && par->lEEvent == SHCNEE_ALIASINUSE)
        return _ILNext(pidl);
    return NULL;
}

typedef struct {
    ULONG cRef;
    LPITEMIDLIST pidlReal;
    LPITEMIDLIST pidlAlias;
    DWORD dwTime;
} ANYALIAS;

#define ANYALIAS_MAX    16
ANYALIAS *g_rgpaaAliases[ANYALIAS_MAX] = {0};
LONG g_cAliases = 0;

void AnyAlias_Free(ANYALIAS *paa)
{
    ASSERT(paa);

    ILFree(paa->pidlReal);
    ILFree(paa->pidlAlias);

    LocalFree(paa);
}

void AnyAlias_Release(ANYALIAS *paa)
{
    ASSERT(paa);
    if (0 == InterlockedDecrement(&paa->cRef))
    {
        AnyAlias_Free(paa);
    }
}

ANYALIAS *AnyAlias_Get(int i)
{
    ANYALIAS *paa;
    ENTERCRITICAL;
    paa = g_rgpaaAliases[i];
    if (paa)
    {
        paa->cRef++;
    }
    LEAVECRITICAL;
    return paa;
}

void AnyAlias_Set(int i, ANYALIAS *paa)
{
    ANYALIAS *paaOld;
    ENTERCRITICAL;
    paaOld = g_rgpaaAliases[i];
    if (paaOld)
    {
        AnyAlias_Release(paaOld);
        g_cAliases--;
    }

    g_rgpaaAliases[i] = paa;
    if (paa)
    {
        paa->cRef++;
        g_cAliases++;
    }
    LEAVECRITICAL;
}

ANYALIAS *AnyAlias_Create(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    ANYALIAS *paa = (ANYALIAS*) LocalAlloc(LPTR, SIZEOF(ANYALIAS));

    if (paa)
    {
        paa->cRef = 1;
        paa->pidlReal = ILClone(pidlReal);
        paa->pidlAlias = ILClone(pidlAlias);

        if (!paa->pidlReal || !paa->pidlAlias)
        {
            AnyAlias_Free(paa);
            paa = NULL;
        }
    }
    return paa;
}

ANYALIAS *AnyAlias_Find(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias)
{
    if (g_cAliases)
    {
        ANYALIAS *paa = NULL;
        int i;
        
        for (i = 0; i < ARRAYSIZE(g_rgpaaAliases); i++)
        {
            paa = AnyAlias_Get(i);
            
            if (paa)
            {
                if (ILIsEqual(pidlReal, paa->pidlReal)
                && (ILIsEqual(pidlAlias, paa->pidlAlias)))
                {
                    //  we found it
                    break;
                }

                //  otherwise release and try again
                AnyAlias_Release(paa);
                paa = NULL;
            }
        }

        return paa;
    }
    return NULL;
}

void AnyAlias_Insert(ANYALIAS *paa)
{
    int iInsert;
    ANYALIAS *paaOldest = NULL;
    int i;

    //  find empty and oldest
    for (i = 0; i < ARRAYSIZE(g_rgpaaAliases); i++)
    {
        ANYALIAS *paaCurr = AnyAlias_Get(i);
        if (!paaCurr)
        {
            iInsert = i;
            break;
        }
        else if (!paaOldest || (paaCurr->dwTime > paaOldest->dwTime))
        {
            iInsert = i;

            //  transfer ref to oldest
            if (paaOldest)
                AnyAlias_Release(paaOldest);

            paaOldest = paaCurr;
        }
        else
        {
            AnyAlias_Release(paaCurr);
        }
    }

    AnyAlias_Set(iInsert, paa);

    if (paaOldest)
        AnyAlias_Release(paaOldest);
}

void AnyAlias_CheckRollover(void)
{
    static DWORD s_tick = 0;
    DWORD tick = GetTickCount();

    if (tick < s_tick)
    {
        // we rolled the tick count over
        int i;
        for (i = 0; i < ARRAYSIZE(g_rgpaaAliases); i++)
        {
            ANYALIAS *paaCurr = AnyAlias_Get(i);

            if (paaCurr)
            {
                paaCurr->dwTime = tick;
                AnyAlias_Release(paaCurr);
            }
        }
    }

    s_tick = tick;
}

void AnyAlias_Add(LPCITEMIDLIST pidlReal, LPCITEMIDLIST pidlAlias, DWORD dwEventTime)
{
    ANYALIAS *paa = AnyAlias_Find(pidlReal, pidlAlias);

    if(!paa)
    {
        paa = AnyAlias_Create(pidlReal, pidlAlias);

        if (paa)
        {
            AnyAlias_Insert(paa);
        }
    }
    
    if (paa)
    {
        //  we just want to update the time on the existing entry
        paa->dwTime = dwEventTime;
        AnyAlias_Release(paa);
        AnyAlias_CheckRollover();
    }
}        

LPITEMIDLIST AnyAlias_Translate(LPCITEMIDLIST pidl, ANYALIAS *paa)
{
    //
    //  see if its child of one of our watched items
    
    LPCITEMIDLIST pidlChild = pidl? ILFindChild(paa->pidlReal, pidl) : NULL;

    if (pidlChild)
    {
        return ILCombine(paa->pidlAlias, pidlChild);
    }
    return NULL;
}


void AnyAlias_TranslateEvent(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    if (g_cAliases)
    {
        int i;
        for (i = 0; i < ARRAYSIZE(g_rgpaaAliases); i++)
        {
            ANYALIAS *paa = AnyAlias_Get(i);
            if (paa)
            {
                //
                //  see if its child of one of our watched items
                
                LPITEMIDLIST pidlAlias = AnyAlias_Translate(pidl, paa);
                LPITEMIDLIST pidlAliasExtra = AnyAlias_Translate(pidlExtra, paa);

                if (pidlAlias || pidlAliasExtra)
                {
                    SHChangeNotifyReceiveEx(lEvent, SHCNF_NONOTIFYINTERNALS | SHCNF_IDLIST, 
                        pidlAlias ? pidlAlias : pidl, 
                        pidlAliasExtra ? pidlAliasExtra : pidlExtra, 
                        dwEventTime);

                    //  do some special handling here
                    //  like refresh folders or something will clean out an entry.
                    switch (lEvent)
                    {
                    case SHCNE_UPDATEDIR:
                        if (ILIsEqual(pidl, paa->pidlReal))
                        {
                            //  this is target, and it will be refreshed.
                            //  if the alias is still around, then it will
                            //  have to reenum and re-register
                            //  there-fore we will clean this out now.
                            AnyAlias_Set(i, NULL);
                        }
                        break;

                    default:
                        break;
                    }
                        
                }

                AnyAlias_Release(paa);
            }
        }
    }
}

void AnyAlias_Clear(void)
{
    if (g_cAliases)
    {
        int i;
        for (i = 0; i < ARRAYSIZE(g_rgpaaAliases); i++)
        {
            AnyAlias_Set(i, NULL);
        }
    }
}
            
void AnyAlias_Change(LONG lEvent, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    if (lEvent == SHCNE_EXTENDED_EVENT)
    {
        pidl = IsAliasRegisterPidl(pidl);
        if (pidl)
            AnyAlias_Add(pidl, pidlExtra, dwEventTime);
    }
    else
    {
        AnyAlias_TranslateEvent(lEvent, pidl, pidlExtra, dwEventTime);
    }
}

void NotifyShellInternals(LONG lEvent, LPCITEMIDLIST pidl,
                                   LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    PERFTEST(DESKTOP_EVENT) CreateAliasNotification(&s_rgaf[0], lEvent, pidl, pidlExtra, dwEventTime);
    PERFTEST(DESKTOP_EVENT) CreateAliasNotification(&s_rgaf[1], lEvent, pidl, pidlExtra, dwEventTime);
    // See comment above realted to Net hood.

    PERFTEST(DESKTOP_EVENT) CreateAliasNotification(&s_rgaf[2], lEvent, pidl, pidlExtra, dwEventTime);
    PERFTEST(DESKTOP_EVENT) CreateAliasNotification(&s_rgaf[3], lEvent, pidl, pidlExtra, dwEventTime);
    PERFTEST(RLFS_EVENT) RLFSChanged(lEvent, (LPITEMIDLIST)pidl, (LPITEMIDLIST)pidlExtra);
    PERFTEST(SFP_EVENT) SFP_FSEvent(lEvent, pidl,  pidlExtra);
    PERFTEST(ICON_EVENT) Icon_FSEvent(lEvent, pidl,  pidlExtra);
    PERFTEST(ALIAS_EVENT) AnyAlias_Change(lEvent, pidl, pidlExtra, dwEventTime);
}

#define MSEC_GUIMAXWAIT  2000   // Maximum wait from GUI thread
#define MSEC_GUIEVTWAIT    20   // dwTimeOut for WaitForMultipleObjects
#define MSEC_GUISLEEP      20   // Time to sleep when a GUI thread got a event.

//
//  Wait until the FSThread finish processing all the events. The GUI threads
// need to call this function from within GenerateEvents().
//
void _WaitFSThreadProcessEvents(void)
{
    HANDLE hEvents[MAXIMUM_WAIT_OBJECTS];
    ULONG cEvents;
    DWORD dwTick = GetCurrentTime();

    //
    // This function must not be called by the FSNotify thread.
    //
    if (g_fsnpp.idtRunning == GetCurrentThreadId())
    {
        TraceMsg(TF_ERROR, "ERROR - FSNotify threads called FS_GenerateEvents!");
        return;
    }

    //
    // Get the same list of events the FSNotify thread waits for.
    // Pass FALSE so we don't modify the global state.
    //
    cEvents = (ULONG)FSNBuildEventList(hEvents, FALSE);

    //
    // We continue until either
    //  (1) all the events are cleared by FSNotify threads, or
    //  (2) we wait it more than 2 (MSEC_GUIMAXWAIT/1000) second.
    //
    while(1)
    {
        DWORD dwWaitResult;
        ULONG iEvent;

        //
        // Wait for same sets of objects as the FSNotify thread waits for.
        //
        dwWaitResult = WaitForMultipleObjects(cEvents, hEvents, FALSE, 0);

        //
        // Check if we are signaled or not.
        //
        iEvent = dwWaitResult-WAIT_OBJECT_0;
        if (iEvent<cEvents)
        {
            //
            // Yes, we are signaled (probably by our own previous FS call).
            // Sleep (to give the FSNotify thread to process it) and wait.
            //
            Sleep(MSEC_GUISLEEP);

            if (GetCurrentTime()-dwTick > MSEC_GUIMAXWAIT)
            {
                TraceMsg(TF_WARNING, "_WaitFSTPE timeout (FSNotify thread might be dead/blocked)");
                break;
            }
        }
        else
        {
            //
            //  No, WaitForMultipleObject failed. We can assume that FSNotify
            // thread finished processing all the interrupt events.
            //
            if (dwWaitResult == (DWORD)-1)
            {
                TraceMsg(TF_WARNING, "_WaitFSTPE failed (%x)", GetLastError());
            }
            if (dwWaitResult != WAIT_TIMEOUT)
            {
                TraceMsg(TF_WARNING, "_WaitFSTPE strange result (%x)", dwWaitResult);
            }
            break;
        }
    }
}


void FreeSpacePidlToPath(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    TCHAR szPath1[MAX_PATH];
    if (SHGetPathFromIDList(pidl1, szPath1)) 
    {
        TCHAR szPath2[MAX_PATH];
        szPath2[0] = 0;
        if (pidl2) 
        {
            SHGetPathFromIDList(pidl2, szPath2);
        }
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, szPath1, szPath2[0] ? szPath2 : NULL);
    }
}

//
// NOTE: this is the OLD API, new clients should use SHChangeNotifyReceiveEx and pass the event time as well. This
// allows the shchange notify to better remove events.
//
// DO NOT USE THIS FUNCTION, INSTEAD, USE SHChangeNotifyReceiveEx!!
//
void SHChangeNotifyReceive(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra)
{
    SHChangeNotifyReceiveEx(lEvent, uFlags, pidl, pidlExtra, GetTickCount());
}

void SHChangeNotifyReceiveEx(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    if (!(uFlags & SHCNF_ONLYNOTIFYINTERNALS))
    {
        BOOL bOnlyUpdateDirs;

        /// now do the actual generating of the event
        if (lEvent & (SHCNE_NETSHARE | SHCNE_NETUNSHARE))
        {
            // Update the cache.

            IsILShared(pidl, TRUE);
        }

#ifdef DEBUG
        TraceMsg(TF_FSNOTIFY, "SHChangeNotifyReceive 0x%X, %d", lEvent, IsMainShellProcess());
        if (uFlags == 0)    // Only do this for pidls
            DebugDumpPidl(TEXT("SHChangeNotifyReceive"), pidl);

        if (pidl)
        {
            ASSERT(IsValidPIDL(pidl));
        }

        if (pidlExtra)
        {
            ASSERT(IsValidPIDL(pidlExtra));
        }
#endif


        if (lEvent)
        {
            bOnlyUpdateDirs = _SHChangeNotifyAddEventToClientQueues(lEvent, pidl, pidlExtra, dwEventTime);
        }

        // remove any shell generated events for the file system
        if ((lEvent & SHCNE_DISKEVENTS) &&
            !(lEvent & (SHCNE_INTERRUPT | SHCNE_UPDATEDIR | SHCNE_UPDATEITEM))) {
            if (!bOnlyUpdateDirs)
            {
                // No use waiting for events to come through if everybody is just
                // doing an updatedir anyway.  Note that we will still clear out
                // as many int events as we can so that we do not fill up that
                // queue
                _WaitFSThreadProcessEvents();
            }

            FSNRemoveInterruptEvent(pidl);
            if (pidlExtra) 
                FSNRemoveInterruptEvent(pidlExtra);


        }
    }

    //
    // note make sure the internal events go first.
    //
    // unless the nonotifyinteranls flag is set meaning that this was created
    // by our alias pidl creation from inside a NotifyShellInternals call
    //
    if (lEvent && (!(uFlags & SHCNF_NONOTIFYINTERNALS)))
        NotifyShellInternals(lEvent, pidl, pidlExtra, dwEventTime);

    //
    // then the registered events
    //
    if (uFlags & (SHCNF_FLUSH)) {
        if (uFlags & (SHCNF_FLUSHNOWAIT)) {
            g_fsnpp.fFlushNow = TRUE;
            WakeThread(g_fsnpp.idtRunning);
        } else
            _SHChangeNotifyHandleEvents(TRUE);
    }
}

LRESULT SHChangeNotify_OnNotify(WPARAM wParam, LPARAM lParam)
{
    HANDLE hChange = (HANDLE)wParam;
    DWORD dwProcId = (DWORD)lParam;

    LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock(hChange, dwProcId, NULL, NULL);
    if (pshcnl)
    {
        SHChangeNotifyReceiveEx(pshcnl->pshcn->lEvent,
                                pshcnl->pshcn->uFlags,
                                pshcnl->pidlMain,
                                pshcnl->pidlExtra,
                                pshcnl->pshcn->dwEventTime);
        SHChangeNotification_Unlock(pshcnl);
        SHFreeShared(hChange, dwProcId);
    }
    return TRUE;
}

// send the notify to the desktop... telling it to put it in the queue.
// if we are in the desktop's process, we can handle it directly ourselves.
// the one exception is flush.  we want the desktop to be one serializing flush so
// we send in that case as well
void SHChangeNotifyTransmit(LONG lEvent, UINT uFlags, LPCITEMIDLIST pidl, LPCITEMIDLIST pidlExtra, DWORD dwEventTime)
{
    BOOL fFlushNow = ((uFlags & (SHCNF_FLUSH | SHCNF_FLUSHNOWAIT)) == SHCNF_FLUSH);

    if (fFlushNow || !IsMainShellProcess()) 
    {
        HWND hwndDesktop = GetShellWindow();
        if (hwndDesktop)
        {
            HANDLE  hChangeNotification;
            DWORD   dwProcId;

            // apphack.  rna deadlocks on sending SHCNF_FLUSH across processes.
            // in win95, this did not happen because flush was only sync in the same process..
            // it was effectly nowait on all other processes.
            // since we use the desktop to serialize across the system, this breaks them.
#ifndef WINNT
            if (uFlags == SHCNF_FLUSH && (SHGetAppCompatFlags(ACF_FLUSHNOWAITALWAYS) & ACF_FLUSHNOWAITALWAYS))
            {
                uFlags |= SHCNF_FLUSHNOWAIT;
            }
#endif

            GetWindowThreadProcessId(hwndDesktop, &dwProcId);
            hChangeNotification = SHChangeNotification_Create(lEvent, uFlags, pidl, pidlExtra, dwProcId, dwEventTime);
            if (hChangeNotification)
            {
                // Flush but not flush no wait
                if (fFlushNow)
                {
                    TraceMsg(TF_FSNOTIFY, "SHChangeNotifyTransmit Send 0x%X, %d", lEvent, IsMainShellProcess());
                    SendMessage(hwndDesktop, CWM_FSNOTIFY,
                                (WPARAM)hChangeNotification, (LPARAM)dwProcId);
                }
                else
                {
                    TraceMsg(TF_FSNOTIFY, "SHChangeNotifyTransmit Notify 0x%X, %d", lEvent, IsMainShellProcess());
                    SendNotifyMessage(hwndDesktop, CWM_FSNOTIFY,
                                      (WPARAM)hChangeNotification, (LPARAM)dwProcId);
                }
            }
            return;
        }
    }

    TraceMsg(TF_FSNOTIFY, "SHChangeNotifyTransmit Immediate 0x%X, %d", lEvent, IsMainShellProcess());
    // if anything goes wrong, handle it directly
    SHChangeNotifyReceiveEx(lEvent, uFlags, pidl, pidlExtra, dwEventTime);
}


// NOTE: There is a copy of these functions in shdocvw util.cpp for browser only mode supprt.
// NOTE: functionality changes should also be reflected there.
STDAPI_(void) SHUpdateImageA( LPCSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex )
{
    WCHAR szWHash[MAX_PATH];

    SHAnsiToUnicode(pszHashItem, szWHash, ARRAYSIZE(szWHash));

    SHUpdateImageW(szWHash, iIndex, uFlags, iImageIndex);
}

STDAPI_(void) SHUpdateImageW( LPCWSTR pszHashItem, int iIndex, UINT uFlags, int iImageIndex )
{
    SHChangeUpdateImageIDList rgPidl;
    SHChangeDWORDAsIDList rgDWord;
    
    int cLen = MAX_PATH - (lstrlenW( pszHashItem ) + 1);
    cLen *= sizeof( WCHAR );

    if ( cLen < 0 )
        cLen = 0;

    // make sure we send a valid index
    if ( iImageIndex == -1 )
        iImageIndex = II_DOCUMENT;
        
    rgPidl.dwProcessID = GetCurrentProcessId();
    rgPidl.iIconIndex = iIndex;
    rgPidl.iCurIndex = iImageIndex;
    rgPidl.uFlags = uFlags;
    StrCpyNW( rgPidl.szName, pszHashItem, MAX_PATH );
    rgPidl.cb = (USHORT)(sizeof( rgPidl ) - cLen);
    _ILNext( (LPITEMIDLIST) &rgPidl )->mkid.cb = 0;

    rgDWord.cb = sizeof( rgDWord) - sizeof(USHORT);
    rgDWord.dwItem1 = iImageIndex;
    rgDWord.dwItem2 = 0;
    rgDWord.cbZero = 0;

    // pump it as an extended event
    SHChangeNotify( SHCNE_UPDATEIMAGE, SHCNF_IDLIST, &rgDWord, &rgPidl );
}

// REVIEW: pretty lame implementation of handling updateimage, requiring the caller
// to handle the pidl case instead of passing both pidls down here.
//
STDAPI_(int) SHHandleUpdateImage( LPCITEMIDLIST pidlExtra )
{
    SHChangeUpdateImageIDList * pUs = (SHChangeUpdateImageIDList*) pidlExtra;

    if ( !pUs )
    {
        return -1;
    }

    // if in the same process, or an old style notification
    if ( pUs->dwProcessID == GetCurrentProcessId())
    {
        return *(int UNALIGNED *)((BYTE *)&pUs->iCurIndex);
    }
    else
    {
        WCHAR szBuffer[MAX_PATH];
        int iIconIndex = *(int UNALIGNED *)((BYTE *)&pUs->iIconIndex);
        UINT uFlags = *(UINT UNALIGNED *)((BYTE *)&pUs->uFlags);

        ualstrcpyW( szBuffer, pUs->szName );
        
        // we are in a different process, look up the hash in our index to get the right one...
        return SHLookupIconIndexW( szBuffer, iIconIndex, uFlags );
    }
}

STDAPI_(void) SHChangeNotify(LONG lEvent, UINT uFlags, const void * dwItem1, const void * dwItem2)
{
    LPCITEMIDLIST pidl = NULL;
    LPCITEMIDLIST pidlExtra = NULL;
    LPITEMIDLIST pidlFree = NULL;
    LPITEMIDLIST pidlExtraFree = NULL;
    UINT uType = uFlags & SHCNF_TYPE;
    SHChangeDWORDAsIDList dwidl;
    BOOL    fPrinter = FALSE;
    BOOL    fPrintJob = FALSE;
    DWORD dwEventTime = GetTickCount();

    // first setup anything the flags request
    switch (uType)
    {
#ifdef UNICODE
    case SHCNF_PRINTJOBA:
        fPrintJob = TRUE;
        // fall through
    case SHCNF_PRINTERA:
        fPrinter = TRUE;
        // fall through
    case SHCNF_PATHA:
#else
    case SHCNF_PRINTJOBW:
        fPrintJob = TRUE;
        // fall through
    case SHCNF_PRINTERW:
        fPrinter = TRUE;
        // fall through
    case SHCNF_PATHW:
#endif
        {
            TCHAR szPath1[MAX_PATH], szPath2[MAX_PATH];
            LPCVOID pvItem1 = NULL;
            LPCVOID pvItem2 = NULL;

            if (dwItem1)
            {
#ifdef UNICODE
                SHAnsiToTChar((LPSTR)dwItem1, szPath1, ARRAYSIZE(szPath1));
#else
                SHUnicodeToTChar((LPWSTR)dwItem1, szPath1, ARRAYSIZE(szPath1));
#endif
                pvItem1 = szPath1;
            }

            if (dwItem2)
            {
                if (fPrintJob)
                    pvItem2 = dwItem2;  // SHCNF_PRINTJOB_DATA needs no conversion
                else
                {
#ifdef UNICODE
                    SHAnsiToTChar((LPSTR)dwItem2, szPath2, ARRAYSIZE(szPath2));
#else
                    SHUnicodeToTChar((LPWSTR)dwItem2, szPath2, ARRAYSIZE(szPath2));
#endif
                    pvItem2 = szPath2;
                }
            }

            SHChangeNotify(lEvent, (fPrintJob ? SHCNF_PRINTJOB : (fPrinter ? SHCNF_PRINTER : SHCNF_PATH)),
                           pvItem1, pvItem2);
            goto Cleanup;       // Let the recursive version do all the work
        }
        break;

    case SHCNF_PATH:
        if (lEvent == SHCNE_FREESPACE) 
        {
            DWORD dwItem = 0;
            int idDrive = PathGetDriveNumber((LPCTSTR)dwItem1);
            if (idDrive != -1)
                dwItem = (1 << idDrive);

            if (dwItem2) 
            {
                idDrive = PathGetDriveNumber((LPCTSTR)dwItem2);
                if (idDrive != -1)
                    dwItem |= (1 << idDrive);
            }

            dwItem1 = (LPCVOID)ULongToPtr( dwItem );
            if (dwItem1)
                goto DoDWORD;
            goto Cleanup;
        } 
        else 
        {
            if (dwItem1)
            {
                pidl = pidlFree = SHSimpleIDListFromPath((LPCTSTR)dwItem1);
                if (!pidl)
                    goto Cleanup;

                if (dwItem2) 
                {
                    pidlExtra = pidlExtraFree = SHSimpleIDListFromPath((LPCTSTR)dwItem2);
                    if (!pidlExtra)
                        goto Cleanup;
                }
            }
        }
        break;

    case SHCNF_PRINTER:
        if (dwItem1)
        {
            TraceMsg(TF_FSNGENERAL, "SHChangeNotify: SHCNF_PRINTER %s", (LPTSTR)dwItem1);

            pidl = pidlFree = Printers_GetPidl(NULL, (LPCTSTR)dwItem1);
            if (!pidl)
                goto Cleanup;

            if (dwItem2)
            {
                pidlExtra = pidlExtraFree = Printers_GetPidl(NULL, (LPCTSTR)dwItem2);
                if (!pidlExtra)
                    goto Cleanup;
            }
        }
        break;

    case SHCNF_PRINTJOB:
        if (dwItem1)
        {
#ifdef DEBUG
            switch (lEvent)
            {
            case SHCNE_CREATE:
                TraceMsg(TF_FSNGENERAL, "SHChangeNotify: SHCNE_CREATE SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            case SHCNE_DELETE:
                TraceMsg(TF_FSNGENERAL, "SHChangeNotify: SHCNE_DELETE SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            case SHCNE_UPDATEITEM:
                TraceMsg(TF_FSNGENERAL, "SHChangeNotify: SHCNE_UPDATEITEM SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            default:
                TraceMsg(TF_FSNGENERAL, "SHChangeNotify: SHCNE_? SHCNF_PRINTJOB %s", (LPTSTR)dwItem1);
                break;
            }
#endif
            pidl = pidlFree = Printjob_GetPidl((LPCTSTR)dwItem1, (LPSHCNF_PRINTJOB_DATA)dwItem2);
            if (!pidl)
                goto Cleanup;
        }
        else
        {
            // Caller goofed.
            goto Cleanup;
        }
        break;

    case SHCNF_DWORD:
DoDWORD:
        ASSERT(lEvent & SHCNE_GLOBALEVENTS);

        dwidl.cb      = SIZEOF(dwidl) - SIZEOF(dwidl.cbZero);
        dwidl.dwItem1 = PtrToUlong(dwItem1);
        dwidl.dwItem2 = PtrToUlong(dwItem2);
        dwidl.cbZero  = 0;
        pidl = (LPCITEMIDLIST)&dwidl;
        pidlExtra = NULL;
        break;

    case 0:
        if (lEvent == SHCNE_FREESPACE) {
            // convert this to paths.
            FreeSpacePidlToPath((LPCITEMIDLIST)dwItem1, (LPCITEMIDLIST)dwItem2);
            goto Cleanup;
        }
        pidl = (LPCITEMIDLIST)dwItem1;
        pidlExtra = (LPCITEMIDLIST)dwItem2;
        break;

    default:
        TraceMsg(TF_ERROR, "SHChangeNotify: Unrecognized uFlags 0x%X", uFlags);
        return;
    }

    if (lEvent && !(lEvent & SHCNE_ASSOCCHANGED) && !pidl)
    {
        // Caller goofed. SHChangeNotifyTransmit & clients assume pidl is
        // non-NULL if lEvent is non-zero (except in the SHCNE_ASSOCCHANGED case),
        // and they will crash if we try to send this bogus event. So throw out 
        // this event and rip.
        RIP(FALSE);
        goto Cleanup;
    }

    SHChangeNotifyTransmit(lEvent, uFlags, pidl, pidlExtra, dwEventTime);

Cleanup:

    if (pidlFree)
        ILFree(pidlFree);
    if (pidlExtraFree)
        ILFree(pidlExtraFree);
}

//---------------------------------------------------------------------------
//
// Clean up at process shutdown.  Be careful on Win9x platforms as g_fsn
// is a shared variable so we need to take critical sections around it.
//
// REVIEW: We assume partying on g_fsnpp is safe (i.e., other threads nuked already).
//
void SHChangeNotifyTerminate(BOOL bLastTerm)
{
    PFSNotifyClientInfo pfsnci;
    int iEvent;

#ifdef WINNT
    // On NT, everything is per-process, so bLastTerm is always TRUE.
    ASSERT(bLastTerm);
#endif
    // When bLastTerm, this is the very last process, so nobody should hold a cRefClientList any more
    //
    ASSERT(!bLastTerm || 0==g_fsn.cRefClientList);

    FSNENTERCRITICAL;

    if (bLastTerm && g_fsn.pfsnciFirst)
    {
        for (pfsnci = g_fsn.pfsnciFirst; pfsnci; )
        {
            // Delete the client, side effect: increment pfsnci
            _SHChangeNotifyNukeClient(&pfsnci, FALSE);
        }

        // bLastTerm implies no other processes exist
        ASSERT(NULL == g_fsn.pfsnciFirst);
        g_fsn.pfsnciFirst = NULL; // just to be paranoid
    }

    // free all the interrupt events
    if (g_fsnpp.hdsaIntEvents)
    {
        LPFSIntEvent pfsie;

        // Take g_fsnpp.hdsaIntEvents out of the g_fsn.hdpaIntEvents list first
        //
        for (iEvent=DPA_GetPtrCount(g_fsn.hdpaIntEvents)-1; iEvent>=0; --iEvent)
        {
            if (DPA_FastGetPtr(g_fsn.hdpaIntEvents, iEvent)
                == g_fsnpp.hdsaIntEvents)
            {
                DPA_DeletePtr(g_fsn.hdpaIntEvents, iEvent);
            }
        }

        if (DPA_GetPtrCount(g_fsn.hdpaIntEvents) == 0)
        {
            DPA_Destroy(g_fsn.hdpaIntEvents);
            g_fsn.hdpaIntEvents = NULL;
        }

        // Now it's safe to nuke everything in g_fsnpp.hdsaIntEvents
        //
        for (iEvent = DSA_GetItemCount(g_fsnpp.hdsaIntEvents) - 1; iEvent >= 0; iEvent--)
        {
            pfsie = DSA_GetItemPtr(g_fsnpp.hdsaIntEvents, iEvent);
            ILGlobalFree((LPITEMIDLIST)pfsie->pidl);
        }
        DSA_Destroy(g_fsnpp.hdsaIntEvents);
        g_fsnpp.hdsaIntEvents = NULL;
    }

    // free all the interrupt clients
    // Since this list is used in FSNotifyThreadProc, we need the critical section
    if (g_fsnpp.hdsaIntClients)
    {
        LPFSIntClient lpfsic;

        for (iEvent = DSA_GetItemCount(g_fsnpp.hdsaIntClients) - 1; iEvent >= 0; iEvent--)
        {
            lpfsic = DSA_GetItemPtr(g_fsnpp.hdsaIntClients, iEvent);
            FSNDestructIntClient(lpfsic, TRUE);
        }
        DSA_Destroy(g_fsnpp.hdsaIntClients);
        g_fsnpp.hdsaIntClients = NULL;
    }

    FSNLEAVECRITICAL;

    // These are set up by the first fsnotify thread to come along,
    // but not cleaned up until process shutdown
    TermAliasFolderTable();
    AnyAlias_Clear();
}


void _Shell32ThreadAddRef(BOOL fLeaveSuspended)
{
    FSNASSERTCRITICAL;

    // Check if this is the first client from this process.
    if (!g_fsnpp.cclients)
    {
        // This thread will only get created once for each process that
        // registers
        TraceMsg(TF_FSNGENERAL, "FSNotify creating new thread");

        if (!g_fsn.hdsaThreadAwake)
        {
            g_fsn.hdsaThreadAwake = DSA_Create(SIZEOF(AWAKETHREAD), 4);

            // BUGBUG: I'm asserting this mostly because I don't
            // know what to do if it fails
            ASSERT(g_fsn.hdsaThreadAwake);
        }

        // Suspend the thread to start with so that our globals get
        // set before they get checked
        g_fsnpp.htStarting = CreateThread(NULL, 0, FSNotifyThreadProc, NULL, CREATE_SUSPENDED, &g_fsnpp.idtStarting);
        ASSERT(g_fsnpp.htStarting && !g_fsnpp.htRunning);
        if (!fLeaveSuspended)
            ResumeThread(g_fsnpp.htStarting);
    }
    else
    {
        // No, we should have the fsnotify thread already.
        ASSERT(g_fsnpp.htRunning || g_fsnpp.htStarting);
    }

    g_fsnpp.cclients++;

    TraceMsg(TF_FSNGENERAL, "FSNOTIFY - Thread Addref %d", g_fsnpp.cclients);
}


void _Shell32ThreadRelease(UINT nClients)
{
    FSNENTERCRITICAL;

    g_fsnpp.cclients -= nClients;
    ASSERT((int)g_fsnpp.cclients >= 0);

    TraceMsg(TF_FSNGENERAL, "FSNOTIFY - Thread Release %d clients, count %d", nClients, g_fsnpp.cclients);


    // If we have no more clients, tell the FSNotify thread to kill itself
    if (!g_fsnpp.cclients)
    {
        HANDLE hThread;
        DWORD idThread;
        int i;

        ASSERT(g_fsnpp.htRunning || g_fsnpp.htStarting);
        TraceMsg(TF_FSNGENERAL, "Telling FSNotify thread to kill itself");

        hThread = g_fsnpp.htRunning;
        idThread = g_fsnpp.idtRunning;

        if (hThread)
        {
            ASSERT(g_fsn.hdsaThreadAwake);
            for (i=DSA_GetItemCount(g_fsn.hdsaThreadAwake)-1; i>=0; --i)
            {
                AWAKETHREAD *pAwake = DSA_GetItemPtr(g_fsn.hdsaThreadAwake, i);

                if (pAwake->idThread == idThread)
                {
                    _FSN_RemoveAwakeThread(i);
                    break;
                }
            }

            // We should release the critical section to allow the thread
            // to finish its processing
            FSNLEAVECRITICAL;
            FSNASSERTNONCRITICAL;

            // Check if the thread still exists
            if (WaitForSingleObject(hThread, 0) == WAIT_TIMEOUT)
            {
                SignalKillThread(idThread);

                if (WaitForSingleObject(hThread, 2000) == WAIT_TIMEOUT)
                {
                    TraceMsg(TF_FSNGENERAL, "FSNotify has not been killed");
                }
                else
                {
                    TraceMsg(TF_FSNGENERAL, "FSNotify killed itself");
                }
            }

            CloseHandle(hThread);

            FSNENTERCRITICAL;

            // Make sure nobody has started a new thread while we were not
            // looking
            if (g_fsnpp.idtRunning == idThread)
            {
                g_fsnpp.htRunning = NULL;        // Notes: no need to touch g_fsnpp.idThread
                g_fsnpp.idtRunning = 0;          // Notes: no need to touch g_fsnpp.idThread
            }
        }
        else
        {
            // We must not have initialized yet, and we know we are
            // not in the critical section that touches the DSA,
            // so kill the thread immediately
            TerminateThread(g_fsnpp.htStarting, 0);
            CloseHandle(g_fsnpp.htStarting);
            g_fsnpp.htStarting = NULL;
            g_fsnpp.idtStarting = 0;
        }
    }

    FSNLEAVECRITICAL;
}


//--------------------------------------------------------------------------
// We changed the way that the SHChangeNotifyRegister function worked, so
// to prevent people from calling the old function, we stub it out here.
// The change we made would have broken everbody because we changed the
// lparam and wparam for the notification messages which are sent to the
// registered window.
//
ULONG WINAPI NTSHChangeNotifyRegister(HWND hwnd,
                               int fSources, LONG fEvents,
                               UINT wMsg, int cEntries,
                               SHChangeNotifyEntry *pfsne)
{
    return SHChangeNotifyRegister(hwnd, fSources | SHCNRF_NewDelivery , fEvents, wMsg, cEntries, pfsne);
}
BOOL WINAPI NTSHChangeNotifyDeregister(ULONG ulID)
{
    return SHChangeNotifyDeregister(ulID);
}


//
// REVIEW: BobDay - SHChangeNotifyUpdateEntryList doesn't appear to be
// called by anybody and since we've change the notification message
// structure, anybody who calls it needs to be identified and fixed.
//
BOOL  WINAPI SHChangeNotifyUpdateEntryList(ULONG ulID, int iUpdateType,
                               int cEntries, SHChangeNotifyEntry *pfsne)
{
    ASSERT(FALSE);
    return FALSE;
}


void FreePFSNCIContents(FSNotifyClientInfo * pfsnci, BOOL fNukeInterrupts)
{
    int iEntry;

    // nuke any pending events
    if (pfsnci->hdpaPendingEvents)
    {
        _SHChangeNotifyEmptyEventsList(pfsnci->hdpaPendingEvents);
        DPA_Destroy(pfsnci->hdpaPendingEvents);
        pfsnci->hdpaPendingEvents = NULL;
    }

    if (pfsnci->hdsaNE)
    {
        for (iEntry = DSA_GetItemCount(pfsnci->hdsaNE) - 1; iEntry >= 0; iEntry--)
        {
            SHChangeNotifyEntry *pfsne = DSA_GetItemPtr(pfsnci->hdsaNE, iEntry);
            if (fNukeInterrupts && pfsne->pidl && (pfsnci->fSources & SHCNRF_InterruptLevel))
            {
                FSNRemoveInterruptClient(pfsne->pidl);
            }
            ILGlobalFree((LPITEMIDLIST)pfsne->pidl);
            // Don't need to delete items; the destory below will take care of it.
        }

        DSA_Destroy(pfsnci->hdsaNE);
    }
}


ULONG WINAPI SHChangeNotifyRegisterInternal(HWND hwnd, int fSources,
                               LONG fEvents, UINT wMsg, int cEntries,
                               SHChangeNotifyEntry *pfsne)
{
    int i;
    FSNotifyClientInfo fsnci;
    PFSNotifyClientInfo pfsnci;

    fsnci.pfsnciNext = NULL;
    fsnci.hwnd     = hwnd;
    // GetWindowThreadProcessId(hwnd, &fsnci.dwProcID);  BUGBUG BobDay - This is the old way
    fsnci.dwProcID = GetCurrentProcessId();
    fsnci.fSources = fSources;
    fsnci.fEvents  = fEvents;
    fsnci.wMsg     = (WORD) wMsg;
    fsnci.hdsaNE   = DSA_Create(SIZEOF(SHChangeNotifyEntry), 4);
    fsnci.hdpaPendingEvents   = DPA_Create(4);
    fsnci.iSerializationFlags = 0;

    if (!fsnci.hdsaNE || !fsnci.hdpaPendingEvents)
    {
        TraceMsg(TF_ERROR, "Failed to alloc notify data");
        goto ErrorExit;
    }

    for (i = 0; i < cEntries; i++)
    {
        SHChangeNotifyEntry fsne;

        // Check if this is global notification.
        if (pfsne[i].pidl == NULL)
        {
            // Yes, put NULL in pszNotificatoinPath.
            fsne.fRecursive = TRUE;
            fsne.pidl = NULL;
        }
        else
        {
            // No, copy specified path and fRecursive flag.
            fsne.fRecursive = pfsne[i].fRecursive;
            fsne.pidl = ILGlobalClone(pfsne[i].pidl);

            ASSERT(fsne.pidl);
            if (!fsne.pidl)
                goto ErrorExit;
        }

        if (DSA_InsertItem(fsnci.hdsaNE, i, &fsne) == -1)
        {
            ILGlobalFree((LPITEMIDLIST)fsne.pidl);
            goto ErrorExit;
        }

        // set up the interrupt events if desired
        if (fsne.pidl && (fSources & SHCNRF_InterruptLevel))
        {
            FSNAddInterruptClient(fsne.pidl, pfsne[i].fRecursive && (fSources & SHCNRF_RecursiveInterrupt));
        }
    }

    FSNENTERCRITICAL;
    {
        //
        // Skip ID 0, as this is our error value.
        //
        fsnci.ulID = g_fsn.ulNextID;
        if (!++g_fsn.ulNextID)
            g_fsn.ulNextID = 1;

        //
        //  Don't want to party on the client list while I'm using it in
        //  SHChangeNotifyHandleEvents() because a Realloc() could move the whole
        //  damn thing in memory.
        //
        // Must alloc from shared memory pool
        pfsnci = Alloc(SIZEOF(fsnci));
        if (pfsnci)
        {
            // Move the inforamion into our newly allocated structure.
            // and link to the head of the list.
            *pfsnci = fsnci;
            pfsnci->pfsnciNext = g_fsn.pfsnciFirst;
            g_fsn.pfsnciFirst = pfsnci;
            _Shell32ThreadAddRef(((fSources & SHCNRF_CreateSuspended) == SHCNRF_CreateSuspended));
        }

    }
    FSNLEAVECRITICAL;

    if (pfsnci)
        return fsnci.ulID;

ErrorExit:
    FreePFSNCIContents(&fsnci, (fSources & SHCNRF_InterruptLevel));

    return 0;
}

//--------------------------------------------------------------------------
//
FSNotifyClientInfo * _GetNotificationClientFromID(ULONG ulID)
{
    register FSNotifyClientInfo * pfsnci;

    //
    //  Locate the given client within the list.
    //

    FSNENTERCRITICAL;
    for (pfsnci = g_fsn.pfsnciFirst; pfsnci; pfsnci = pfsnci->pfsnciNext)
    {
        if (pfsnci->ulID == ulID)
        {
            break;
        }
    }
    FSNLEAVECRITICAL;
    return(pfsnci);
}

//--------------------------------------------------------------------------
//

// sets *ppfsnciNext to pfsnci->pfsnciNext
// returns 1 if deleted/marked for deletion, and 0 if not
//
int _SHChangeNotifyNukeClient(PFSNotifyClientInfo * ppfsnci, BOOL fNukeInterrupts)
{
    int nDeleted = 0;
    PFSNotifyClientInfo pfsnci = *ppfsnci;
    PFSNotifyClientInfo pfsnciT;
    HWND hwndTemp;

    FSNASSERTCRITICAL;

    if (g_fsn.cRefClientList)
    {
        //  Can't delete this yet, let SHChangeNotifyHandleEvents() do it.
        if (FSSF_DELETE_ME == pfsnci->iSerializationFlags)
        {
            TraceMsg(TF_FSNGENERAL, "Already marked for Delete client %lx", (ULONG)pfsnci->ulID);
        }
        else
        {
            TraceMsg(TF_FSNGENERAL, "Marking for Delete client %lx", (ULONG)pfsnci->ulID);

            pfsnci->iSerializationFlags = FSSF_DELETE_ME;

            nDeleted = 1;
        }
        pfsnciT = pfsnci->pfsnciNext;
    }
    else
    {
        BOOL fNewDelivery;

        // Stomp it.
        TraceMsg(TF_FSNGENERAL, "Deleting client %lx", (ULONG)pfsnci->ulID);

        // Unlink this item from the linked list of items...
        if (g_fsn.pfsnciFirst == pfsnci)
            g_fsn.pfsnciFirst = pfsnci->pfsnciNext;
        else
        {
            for (pfsnciT = g_fsn.pfsnciFirst; pfsnciT; pfsnciT = pfsnciT->pfsnciNext)
            {
                if (pfsnciT->pfsnciNext == pfsnci)
                {
                    // Found the one we are looking for...
                    pfsnciT->pfsnciNext = pfsnci->pfsnciNext;
                    break;
                }
            }
        }

        pfsnciT = pfsnci->pfsnciNext;

        fNewDelivery = (pfsnci->fSources & SHCNRF_NewDelivery);

        // save off the hwnd in case we need to send a message to the proxy window
        hwndTemp = pfsnci->hwnd;

        // We unlinked ourself earlier so now just free the memory.
        FreePFSNCIContents(pfsnci, fNukeInterrupts);
        Free(pfsnci);
        nDeleted = 1;

        // If we setup a proxy window for this, we should destroy the objects now...
        if (!fNewDelivery)
        {
            PostMessage(hwndTemp, WM_CLOSE, 0, 0);  // Tell other process to destroy window
        }
    }

    *ppfsnci = pfsnciT;

    return nDeleted;
}

// this deregisters anything that this window might have been registered in
void WINAPI SHChangeNotifyDeregisterWindow(HWND hwnd)
{
    int nClients = 0;
    PFSNotifyClientInfo pfsnci;

    FSNENTERCRITICAL;

    // This is always a bit tricky as if we delete an item we need to
    // start the next one off at the right place.
    for (pfsnci = g_fsn.pfsnciFirst; pfsnci; )
    {
        if (pfsnci->hwnd == hwnd)
        {
            // Delete the client, side effect: increment pfsnci
            nClients += _SHChangeNotifyNukeClient(&pfsnci, TRUE);
        }
        else
        {
            pfsnci = pfsnci->pfsnciNext;
        }
    }

    FSNLEAVECRITICAL;

    _Shell32ThreadRelease(nClients);
}

//--------------------------------------------------------------------------
//
//  Returns TRUE if we found and removed the specified Client, otherwise
//  returns FALSE.
//
BOOL WINAPI SHChangeNotifyDeregisterInternal(ULONG ulID)
{
    BOOL fRetval = TRUE;
    int nClients = 0;

    //
    //  If the client was found, free up its heap data, and then remove
    //  it from the list.
    //

    FSNENTERCRITICAL;
    {
        FSNotifyClientInfo *pfsnci = _GetNotificationClientFromID(ulID);
        if (EVAL(pfsnci)) // assert firing implies someone probably deregistered twice in a row...
        {
            // Delete the client, side effect: increment pfsnci
            nClients = _SHChangeNotifyNukeClient(&pfsnci, TRUE);
        }
        else
        {
            fRetval = FALSE;
        }
    }
    FSNLEAVECRITICAL;

    _Shell32ThreadRelease(nClients);

    return fRetval;
}

// should be named: LRESULT SHChangeNotify_OnChangeRegistration(WPARAM wParam, LPARAM lParam)
BOOL WINAPI SHChangeRegistrationReceive(HANDLE hChangeRegistration, DWORD dwProcId)
{
    LPSHChangeRegistration  pshcr;
    BOOL fResult = FALSE;

    pshcr = SHLockShared(hChangeRegistration, dwProcId);
    if (pshcr)
    {
        switch(pshcr->uCmd)
        {
            case SHCR_CMD_REGISTER:
            {
                SHChangeNotifyEntry fsne;

                fsne.pidl = NULL;
                fsne.fRecursive = pshcr->fRecursive;
                if (pshcr->uidlRegister)
                    fsne.pidl = (LPITEMIDLIST)((LPBYTE)pshcr+pshcr->uidlRegister);

                pshcr->ulID = SHChangeNotifyRegisterInternal(
                                        LongToHandle(pshcr->hwnd), pshcr->fSources,
                                        pshcr->lEvents, pshcr->uMsg,
                                        1, &fsne);
                fResult = TRUE;
                break;
            }
            case SHCR_CMD_DEREGISTER:
                fResult = SHChangeNotifyDeregisterInternal(pshcr->ulID);
                break;
            default:
                break;
        }
        SHUnlockShared(pshcr);
    }
    return fResult;
}

HANDLE SHChangeRegistration_Create( UINT uCmd, ULONG ulID,
                                    HWND hwnd, UINT uMsg,
                                    DWORD fSources, LONG lEvents,
                                    BOOL fRecursive, LPCITEMIDLIST pidl,
                                    DWORD dwProcId)
{
    LPSHChangeRegistration pshcr;
    HANDLE hChangeRegistration;
    UINT uSize = SIZEOF(SHChangeRegistration);
    UINT uidlSize = 0;

    if (pidl)
        uidlSize = ILGetSize(pidl);

    hChangeRegistration = SHAllocShared(NULL, uSize+uidlSize, dwProcId);
    if (!hChangeRegistration)
    {
        return (HANDLE)NULL;
    }

    pshcr = SHLockShared(hChangeRegistration,dwProcId);
    if (!pshcr)
    {
        SHFreeShared(hChangeRegistration,dwProcId);
        return (HANDLE)NULL;
    }

    pshcr->uCmd         = uCmd;
    pshcr->ulID         = ulID;
    pshcr->hwnd         = HandleToUlong(hwnd);
    pshcr->uMsg         = uMsg;
    pshcr->fSources     = fSources;
    pshcr->lEvents      = lEvents;
    pshcr->fRecursive   = fRecursive;
    pshcr->uidlRegister = 0;

    if (pidl)
    {
        pshcr->uidlRegister = SIZEOF(SHChangeRegistration);
        hmemcpy((LPVOID)(pshcr+1),pidl,uidlSize);
    }
    SHUnlockShared(pshcr);

    return hChangeRegistration;
}

// SHChangeNotifySuspendResume
//
// Suspends or resumes filesystem notifications on a path.  If bRecursive
// is set, disable/enables them for all child paths as well.

STDAPI_(BOOL) SHChangeNotifySuspendResume(BOOL         bSuspend, 
                                          LPITEMIDLIST pidlSuspend, 
                                          BOOL         bRecursive, 
                                          DWORD        dwReserved)
{
    HWND hwndDesktop = GetShellWindow();
    SHChangeNotifySuspendResumeStruct * psnsrs = NULL;
    HANDLE hStruct = NULL;
    UINT uSize = SIZEOF(SHChangeNotifySuspendResumeStruct);
    UINT uidlSize = 0;
    DWORD dwProcId = GetCurrentProcessId();
    BOOL bResult;

    if (dwReserved != 0 || NULL == hwndDesktop)
        return FALSE;

    // Allocate and fill in the structure we need to pass this info
    // to the desktop

    if (pidlSuspend)
        uidlSize = ILGetSize(pidlSuspend);

    hStruct = SHAllocShared(NULL, uSize+uidlSize, dwProcId);
    if (!hStruct)
    {
        return FALSE;
    }

    psnsrs = SHLockShared(hStruct, dwProcId);
    if (!psnsrs)
    {
        SHFreeShared(hStruct, dwProcId);
        return FALSE;
    }
    
    CopyMemory(&psnsrs->idl[0], pidlSuspend, uidlSize);
    psnsrs->bSuspend     = bSuspend; 
    psnsrs->bRecursive   = bRecursive; 
    SHUnlockShared(hStruct);

    // Transmit to desktop
    bResult = (BOOL)SendMessage(hwndDesktop, CWM_FSNOTIFYSUSPENDRESUME, (WPARAM)hStruct, (LPARAM)dwProcId);
    SHFreeShared(hStruct, dwProcId);

    return bResult;
}

typedef BOOL (CALLBACK* PFNSHOULDSUSPENDCLIENT)(BOOL, LPFSIntClient, LPVOID);

BOOL SHChangeNotifySuspendResumeInternal(BOOL          bSuspend,
                                         PFNSHOULDSUSPENDCLIENT ShouldSuspend,
                                         LPVOID        pvRefData,
                                         DWORD         dwReserved)
{
    int iMax, j;

    if (g_fsnpp.hdsaIntClients) 
    {
        // Flush pending events

        SHChangeNotify(0, SHCNF_FLUSH, NULL, NULL);

        FSNENTERCRITICAL;

        if (g_fsnpp.hdsaIntClients) 
        {
            iMax = DSA_GetItemCount(g_fsnpp.hdsaIntClients);
            for (j = 0; j < iMax; j++) 
            {
                LPFSIntClient lpfsic = DSA_GetItemPtr(g_fsnpp.hdsaIntClients, j);

                if (ShouldSuspend(bSuspend, lpfsic, pvRefData))
                {
                    if (bSuspend && (0 == lpfsic->iSuspendCount) && lpfsic->hEvent)
                    {
                        FindCloseChangeNotification(lpfsic->hEvent);
                        lpfsic->hEvent = NULL;
                    }

                    if (bSuspend)
                    {
                        lpfsic->iSuspendCount++;
                    }
                    else
                    {
                        lpfsic->iSuspendCount = max(0, lpfsic->iSuspendCount - 1);
                    }
                }
            }
        }

        FSNLEAVECRITICAL;

        // Have the notify thread wake up and rebuild its list

        WakeThread(g_fsnpp.idtRunning);
    }
    return TRUE;
}

BOOL CALLBACK ShouldSuspendClientPidl(BOOL bSuspend, LPFSIntClient lpfsic, LPVOID pvRefData)
{
    SHChangeNotifySuspendResumeStruct *psnsrs = (SHChangeNotifySuspendResumeStruct *)pvRefData;
    TCHAR szSuspendPath[MAX_PATH];
    TCHAR szClientPath[MAX_PATH];
    LPITEMIDLIST pidlSuspend = &psnsrs->idl[0];

    FSNASSERTCRITICAL;
    UNREFERENCED_PARAMETER(bSuspend);

    szSuspendPath[0] = TEXT('\0');
    szClientPath[0] = TEXT('\0');

    // At this point, we try to deal with paths since if we ever managed to sucessfully
    // call FindFirstChangeNotify we must have been able to get a path.
    if (SHGetPathFromIDList(pidlSuspend, szSuspendPath)       &&
        szSuspendPath[0]                                      &&
        SHGetPathFromIDList(lpfsic->pidl, szClientPath)       &&
        szClientPath[0])
    {
        if (psnsrs->bRecursive ? PathIsPrefix(szSuspendPath, szClientPath)
                               : lstrcmpi(szSuspendPath, szClientPath) == 0)
        {
            return TRUE;
        }
    }
    else
    {
        TraceMsg(TF_FSNOTIFY, "ShouldSuspendClient: failed to get a path for pidlSuspend / lpfsic->pidl");
        
        // Ok, so we couldn't get paths for both pidls. We will fall back to doing pidl comparisons, which has the unfortunate
        // shortcomming that the NULL pidl (eg desktop) is NOT equal to the filesys pidl "c:\documents and settings\user1\desktop"
        // even thought they really are in essence the same thing
        if (psnsrs->bRecursive ? ILIsParent(pidlSuspend, lpfsic->pidl, FALSE)
                               : ILIsEqual(pidlSuspend, lpfsic->pidl))
        {
            return TRUE;
        }
    }

    return FALSE;
}

// should be called: LRESULT SHChangeNotify_OnNotifySuspendResume(WPARAM wParam, LPARAM lParam)
LRESULT SHChangeNotifySuspendResumeReceive(WPARAM wParam, LPARAM lParam)
{
    BOOL bret = FALSE;
    const  DWORD dwProcId = (DWORD)  lParam;
    HANDLE hStruct        = (HANDLE) wParam;
    SHChangeNotifySuspendResumeStruct * psnsrs;

    psnsrs = SHLockShared(hStruct, dwProcId);
    if (psnsrs)
    {
        bret = SHChangeNotifySuspendResumeInternal(psnsrs->bSuspend,
                                                   ShouldSuspendClientPidl,
                                                   psnsrs,
                                                   0);
        SHUnlockShared(psnsrs);
    }
    return bret;
}

//
//  pvRefData is the hPNP to suspend or resume.  NULL is a special value
//  used during shutdown which tells us to clean up and get out.
//
BOOL CALLBACK ShouldSuspendClientNotify(BOOL bSuspend, LPFSIntClient lpfsic, LPVOID pvRefData)
{
    HDEVNOTIFY hPNP = (HDEVNOTIFY)pvRefData;
    BOOL bret = FALSE;

    FSNASSERTCRITICAL;
    ASSERT(bSuspend == TRUE || bSuspend == FALSE);

    if (hPNP ? lpfsic->hPNP == hPNP
             : lpfsic->hPNP != NULL)
    {
        ASSERT(lpfsic->hPNP);

        // If we are not currently in the desired state, then switch to
        // the new state and return TRUE to say "Do the suspend/resume".
        // Otherwise, don't issue redundant suspends/resumes or the count
        // will get all out of whack.

        if (lpfsic->bSuspendPNP != bSuspend)
        {
            lpfsic->bSuspendPNP = bSuspend;
            bret = TRUE;
        }

        if (hPNP == NULL)
        {
            // NULL means we are shutting down and should close all handles.
            UnregisterDeviceNotification(lpfsic->hPNP);
            lpfsic->hPNP = NULL;
        }
    }
    return bret;
}

void SHChangeNotify_DesktopTerm(void)
{
    SHChangeNotifySuspendResumeInternal(FALSE,
                                        ShouldSuspendClientNotify,
                                        NULL,   // Wildcard!  Get out!
                                        0);
}

LRESULT SHChangeNotify_OnDeviceChange(ULONG_PTR code, DEV_BROADCAST_HDR *pbh)
{
    DEV_BROADCAST_HANDLE *phnd = (DEV_BROADCAST_HANDLE *)pbh;
    if (pbh->dbch_devicetype == DBT_DEVTYP_HANDLE &&
        phnd->dbch_hdevnotify != NULL)
    {
        switch (code)
        {

        // When PnP is finished dorking with the drive (either successfully
        // or unsuccessfully), resume notifications on that drive.
        case DBT_DEVICEREMOVECOMPLETE:
        case DBT_DEVICEQUERYREMOVEFAILED:
            SHChangeNotifySuspendResumeInternal(FALSE,
                                                ShouldSuspendClientNotify,
                                                phnd->dbch_hdevnotify,
                                                0);
            break;

        // When PnP is starting to dork with the drive, suspend notifications
        // so it can do its thing
        case DBT_DEVICEQUERYREMOVE:
            SHChangeNotifySuspendResumeInternal(TRUE,
                                                ShouldSuspendClientNotify,
                                                phnd->dbch_hdevnotify,
                                                0);
            break;
        }
    }
    return 0;
}

// Called when the desktop window finally appears.  Walk the list of
// currently-registered changenotify's and register for associated PnP
// notifications on them now that we have a window we can deliver them to.

void SHChangeNotify_DesktopInit(void)
{
    FSNENTERCRITICAL;

    if (g_fsnpp.hdsaIntClients)
    {
        int i;
        int iMax;

        iMax = DSA_GetItemCount(g_fsnpp.hdsaIntClients);
        for (i = 0 ; i < iMax; i++)
        {
            LPFSIntClient lpfsic = DSA_GetItemPtr(g_fsnpp.hdsaIntClients, i);
            FSNAttachDeviceNotification(lpfsic);
        }
    }

    FSNLEAVECRITICAL;
}

//--------------------------------------------------------------------------
//
//  Returns a positive integer registration ID, or 0 if out of memory or if
//  invalid parameters were passed in.
//
//  If the hwnd is != NULL we do a PostMessage(hwnd, wMsg, ...) when a
//  relevant FS event takes place, otherwise if fsncb is != NULL we call it.
//
STDAPI_(ULONG) SHChangeNotifyRegister(HWND hwnd,
                               int fSources, LONG fEvents,
                               UINT wMsg, int cEntries,
                               SHChangeNotifyEntry *pfsne)
{
    int i;
    HWND hwndDesktop;
    ULONG ulID = 0;
    DWORD dwProcId;
    HWND hwndProxy = NULL;
    UINT wMsgIn = wMsg;
    HWND hwndIn = hwnd;

    // Special stupid hacked case for boot time performance.  The thread
    // is created suspended and then we unsuspend when we are done doing
    // everything else.
    if (fSources & SHCNRF_ResumeThread)
    {
        if (g_fsnpp.htStarting)
        {
            ResumeThread(g_fsnpp.htStarting);
            return 1;
        }
        else
        {
            return 0;
        }
    }

    ASSERT(pfsne);

    dwProcId = GetCurrentProcessId();
    hwndDesktop = GetShellWindow();

    if ((fSources & SHCNRF_RecursiveInterrupt) && !(fSources & SHCNRF_InterruptLevel))
    {
        // bad caller, they asked for recursive interrupt events, but not interrupt events
        ASSERTMSG(FALSE, "SHChangeNotifyRegister: caller passed SHCNRF_RecursiveInterrupt but NOT SHCNRF_InterruptLevel !!");

        // clear the flag
        fSources = fSources & (~SHCNRF_RecursiveInterrupt);
    }

    for (i = 0; i < cEntries; i++)
    {
        if (!(fSources & SHCNRF_NewDelivery))
        {
            // This is an old style notification, we need to create a hidden
            // proxy type of window to properly handle the messages...
            //
            LP_NotifyProxyData pData;
            _RegisterNotifyProxyWndProc( );

            pData = (LP_NotifyProxyData) LocalAlloc( LPTR, SIZEOF( _NotifyProxyData ) );
            if ( pData == NULL )
                return 0;

            pData->hwndParent = hwndIn;
            pData->wMsg = wMsgIn;

            hwndProxy = CreateWindow( c_szWindowClassName,
                                 c_szDummyWindowName,
                                 WS_MINIMIZE | WS_CHILD,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 hwnd,
                                 NULL,
                                 HINST_THISDLL,
                                 (CREATESTRUCT *) pData );

            if ( hwndProxy == NULL )
            {
                LocalFree((HLOCAL)pData);
                return 0;
            }

            // Now setup to use the proxy window and message through the rest of thi
            // function...
            hwnd = hwndProxy;
            wMsg = WM_CHANGENOTIFYMSG;
        }

        if (hwndDesktop)
        {
            HANDLE hChangeRegistration = SHChangeRegistration_Create(
                                        SHCR_CMD_REGISTER, ulID,
                                        hwnd, wMsg,
                                        fSources, fEvents,
                                        pfsne[i].fRecursive, pfsne[i].pidl,
                                        dwProcId);
            if (hChangeRegistration)
            {
                LPSHChangeRegistration pshcr;
                //
                // Transmit the change regsitration
                //
                SendMessage(hwndDesktop, CWM_CHANGEREGISTRATION,
                            (WPARAM)hChangeRegistration, (LPARAM)dwProcId);

                //
                // Now get back the ulID value, for further registrations and
                // for returning to the calling function...
                //
                pshcr = (LPSHChangeRegistration)SHLockShared(hChangeRegistration,dwProcId);
                if (pshcr)
                {
                    ulID = pshcr->ulID;
                    SHUnlockShared(pshcr);
                }
                else
                {
                    ASSERT(0 == ulID);       // Error condition initialized above
                }
                SHFreeShared(hChangeRegistration,dwProcId);
            }
        }
        else
        {
            ulID = SHChangeNotifyRegisterInternal(hwnd, fSources, fEvents,
                                                  wMsg, 1, &pfsne[i]);
        }
        if (ulID == 0)
        {
            if (hwndProxy)
                DestroyWindow(hwndProxy);
            break;
        }
    }
    return ulID;
}

//--------------------------------------------------------------------------
//
//  Returns TRUE if we found and removed the specified Client, otherwise
//  returns FALSE.
//
// REVIEW BUGBUG: deregistering multiple times with ulID==0 will cause us to
// nuke the notify thread!  avoid this.
//
STDAPI_(BOOL) SHChangeNotifyDeregister(ULONG ulID)
{
    BOOL fResult = FALSE;
    HWND hwndDesktop = GetShellWindow();
#ifdef DEBUG
    static DWORD dwCurrentThreadID = 0;
    static DWORD dwLastID = 0;

    if (dwCurrentThreadID == 0 || dwCurrentThreadID != GetCurrentThreadId())
    {
        dwCurrentThreadID = GetCurrentThreadId();
    }
    else if (dwLastID == 0 || dwLastID != ulID)
    {
        dwLastID = ulID;
    }
    else
    {
        // BUGBUG REVIEW: if this is true, we should fix it!
        //
        // Deregistering twice will blow away you SHChangeNotify Thread Then
        // future notifies will timeout (about 30 seconds).
        //
        // NOTE: the above should no longer true in NT5, as we protected SHChangeNotifyDeregisterInternal
        //       but it's still probably worth fixing.
        //
        TraceMsg(TF_ERROR, "FSNotify: Deregistering twice on thread %d", dwCurrentThreadID);
    }
#endif

    if (hwndDesktop)
    {
        DWORD dwProcId = GetCurrentProcessId();
        HANDLE hChangeRegistration = SHChangeRegistration_Create(
                                    SHCR_CMD_DEREGISTER, ulID,
                                    (HWND)NULL, 0, 0, 0,
                                    FALSE, NULL,
                                    dwProcId);
        if (hChangeRegistration)
        {
            //
            // Transmit the change registration
            //
            fResult = (BOOL) SendMessage(hwndDesktop, CWM_CHANGEREGISTRATION,
                                  (WPARAM)hChangeRegistration, (LPARAM)dwProcId);
            SHFreeShared(hChangeRegistration,dwProcId);
        }
    }
    else
    {
        fResult = SHChangeNotifyDeregisterInternal(ulID);
    }

#ifdef DEBUG
    dwCurrentThreadID = 0;
#endif
    return fResult;
}


//--------------------------------------------------------------------------
//  Notifies hCallbackEvent when all the notification packets for
//  all clients in this process have been handled.
//
// This function is primarily called from the FSNotifyThreadProc thread,
// but in flush cases, it can be called from the desktop thread
//
void CALLBACK _DispatchCallbackNoRef(HWND hwnd, UINT uiMsg,
                                DWORD_PTR hChangeNotification, LRESULT result)
{
    DWORD dwProcId = GetCurrentProcessId();

    SHChangeNotification_Release((HANDLE)hChangeNotification,dwProcId);
}

void CALLBACK _DispatchCallback(HWND hwnd, UINT uiMsg,
                                DWORD_PTR hChangeNotification, LRESULT result)
{
    long count;

    _DispatchCallbackNoRef(hwnd, uiMsg, hChangeNotification, result);
    
    // Note: OndrejS verified that multiple threads do go through this function
    // without the critical section being taken.
    //
    count = InterlockedDecrement(&g_fsnpp.iCallbackCount);
    TraceMsg(TF_FSNOTIFY, "FSNotify: Dispatch Callback %d, %x", g_fsnpp.iCallbackCount, hChangeNotification);

    // Waits like this happen on flush, but that really cares about flushing that thread
    // only, and this hCallbackEvent is per-process.  So that thread may be stuck
    // waiting for some dead app to respond.  Fortunately the wait is only 30 seconds,
    // but some wedged window could really make the system crawl...
    //
    if (0 == count)
        SetEvent(g_fsnpp.hCallbackEvent);    // Free up anybody waiting...
}

//--------------------------------------------------------------------------
//  Sends out all of the change notification packets for the given client.
//
// This function is primarily called from the FSNotifyThreadProc thread,
// but in flush cases, it can be called from the desktop thread
//
void _SHChangeNotifyHandleClientEvents(FSNotifyClientInfo * pfsnci)
{
    int iEvent;
    int iMax;
    DWORD dwProcId = GetCurrentProcessId();
    BOOL fDontSend = FALSE;
    DWORD_PTR dwResult = 0;
    BOOL fHungWindow = 0==SendMessageTimeout(pfsnci->hwnd, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 0, &dwResult);

    SENDASYNCPROC pfncb = fHungWindow ? _DispatchCallbackNoRef : _DispatchCallback;

    iMax = DPA_GetPtrCount(pfsnci->hdpaPendingEvents);
    for (iEvent = 0; iEvent < iMax; iEvent++)
    {
        FSNotifyEvent *pfsnevt = DPA_GetPtr(pfsnci->hdpaPendingEvents, iEvent);
        if (pfsnevt) 
        {
            if (!fDontSend)
            {
                HANDLE hChangeNotification;
                hChangeNotification = SHChangeNotification_Create(
                                            pfsnevt->lEvent & ~SHCNE_INTERRUPT, // Strip off SHCNE_INTERRUPT, don't want clients to see this flag
                                            0,
                                            pfsnevt->pidl,
                                            pfsnevt->pidlExtra,
                                            dwProcId,
                                            pfsnevt->dwEventTime);
                if (hChangeNotification)
                {
                    if (!fHungWindow)
                    {
                        InterlockedIncrement(&g_fsnpp.iCallbackCount);

                        //
                        // callback count must be non-zero, we just incremented it.
                        // Put the event into the reset/false state.
                        //
                        if (!g_fsnpp.hCallbackEvent)
                        {
                            FSNENTERCRITICAL;

                            if (!g_fsnpp.hCallbackEvent)
                            {
                                g_fsnpp.hCallbackEvent = CreateEvent(NULL, TRUE, FALSE, c_szCallbackName);
                            }

                            FSNLEAVECRITICAL;
                        }
                        else
                        {
                            ResetEvent(g_fsnpp.hCallbackEvent);
                        }
                    }

                    TraceMsg(TF_FSNOTIFY, "FSNotify: Dispatching message hwnd = %x (%x, %x) hdpa = %x with ref %d", pfsnci->hwnd, pfsnevt, hChangeNotification, pfsnci->hdpaPendingEvents, pfsnevt->cRef);
                    
                    if (!SendMessageCallback(pfsnci->hwnd, pfsnci->wMsg,
                                                    (WPARAM)hChangeNotification,
                                                    (LPARAM)dwProcId,
                                                    pfncb,
                                                    (DWORD_PTR)hChangeNotification))
                    {
                        pfncb(pfsnci->hwnd, pfsnci->wMsg, (DWORD_PTR)hChangeNotification, 0);
                        TraceMsg(TF_WARNING, "(_SHChangeNotifyHandleClientEvents) SendMessageCB timed out");
                        
                        // if the hwnd is bad, the process probably died,
                        // remove the window from future notifications.
                        if (!IsWindow(pfsnci->hwnd))
                        {
                            fDontSend = TRUE; // don't try to send any more events to this window

                            // Even if this call nukes the pfsnci we're playing
                            // with, we're still safe since we're partying on
                            // a copy on the stack.
                            SHChangeNotifyDeregisterWindow(pfsnci->hwnd);
                        }
                    }
                }
            }
            FSEventRelease(pfsnevt);
        }
    }

    DPA_Destroy(pfsnci->hdpaPendingEvents);
}


//--------------------------------------------------------------------------
//
//  Note that we allow Clients to deregister while this function is executing.
//  Each time through our main loop we recalculate the Client count to take
//  into account any shrinkange due to deregistrations.
//
//  We also allow new Clients to register while we're processing the list.
//
//  We skip "InUse" clients, because they are either in the process of
//  deregistering or of having their messages dispatched by another thread.
//
// fShouldWait==TRUE imples that we are being called from some thread
// other than the FSNotifyThreadProc
//
// This function is primarily called from the FSNotifyThreadProc thread,
// but in flush cases, it can be called from the desktop thread
//
void WINAPI _SHChangeNotifyHandleEvents(BOOL fShouldWait)
{
    static BOOL s_bAlreadyEntered = FALSE;
    DWORD dwProcID = GetCurrentProcessId();
    FSNotifyClientInfo *pfsnci;


    // If this process is already handling events, don't try to handle them again...
    //
    FSNENTERCRITICAL;
    {
        if (s_bAlreadyEntered)
        {
            // we can get here if both the desktop and the fsnotify thread are trying to
            // handle events at the same time.
            FSNLEAVECRITICAL;
            goto WaitForFlush;
        }
        else
        {
            s_bAlreadyEntered = TRUE;
        }
        ++g_fsn.cRefClientList;
    }
    FSNLEAVECRITICAL;

    g_fsnpp.dwLastFlush = GetCurrentTime();

    // flush any pending interrupt events
    FSNFlushInterruptEvents();

    // Note that when a client de-registers, it is not removed from the client
    // queue.  Instead, it is marked as DELETE_ME, and we delete it at the
    // end of handling events.  Therefore, we should not need to worry about
    // the DSA getting smaller or any clients shuffling position.
    // We also do not really need to worry about new clients being added, since
    // they would only have events in their queues that came after we set
    // the dwLastFlush, so the Notify thread should wake up and call
    // HandleEvents again for those new events.
    for (pfsnci = g_fsn.pfsnciFirst; pfsnci; pfsnci = pfsnci->pfsnciNext)
    {
        FSNotifyClientInfo fsnciT;
        int nEvents = 0;

        FSNENTERCRITICAL;
        {
            if (!(pfsnci->iSerializationFlags & FSSF_DELETE_ME))
            {
                // Don't handle events for other processes
                if (pfsnci->dwProcID == dwProcID)
                {
                    nEvents = DPA_GetPtrCount(pfsnci->hdpaPendingEvents);
                    if (nEvents)
                    {
                        // this hands off the hdpaPendingEvents by creating a
                        // temporary client, and setting the old one to have a new
                        // empty hdpaPendingEvents.
                        //
                        //  Copy the entry so we're safe from reallocs in the DSA.
                        fsnciT = *pfsnci;

                        pfsnci->hdpaPendingEvents = DPA_Create(4);
                    }
                }
            }
        }
        FSNLEAVECRITICAL;

        if (nEvents)
            _SHChangeNotifyHandleClientEvents(&fsnciT);
    }

    FSNENTERCRITICAL;
    --g_fsn.cRefClientList;

    // Well, there is the possibility that we could leave some "deletion-
    // pending" clients around, but it shouldn't be a big deal, since we will
    // just delete them at some later time
    if (g_fsn.cRefClientList == 0)
    {
        for (pfsnci = g_fsn.pfsnciFirst; pfsnci;)
        {
            if (pfsnci->iSerializationFlags & FSSF_DELETE_ME)
            {
                // Delete the client, side effect: increment pfsnci
                _SHChangeNotifyNukeClient(&pfsnci, TRUE);
            }
            else
            {
                pfsnci = pfsnci->pfsnciNext;
            }
        }
    }

    s_bAlreadyEntered = FALSE;
    FSNLEAVECRITICAL;

WaitForFlush:
    if (fShouldWait)
    {
        // now wait for all the callbacks to empty out
        _FSN_WaitForCallbacks();
    }
}


// register the hidden window class
BOOL _RegisterNotifyProxyWndProc( void )
{
    // register a hidden window class
    WNDCLASS wc;

    if ( !GetClassInfo(HINST_THISDLL, c_szWindowClassName, &wc ))
    {
        wc.style         = CS_PARENTDC;
        wc.lpfnWndProc   = (WNDPROC) HiddenNotifyWindow_WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = sizeof(LP_NotifyProxyData);
        wc.hInstance     = HINST_THISDLL;
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = c_szWindowClassName;

        return RegisterClass(&wc);
     }
     else
     {
         return TRUE;
     }
}


LRESULT CALLBACK HiddenNotifyWindow_WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
    LRESULT lRes = FALSE;
    LP_NotifyProxyData pData = ( LP_NotifyProxyData ) GetWindowLongPtr( hWnd, 0 );

    switch( iMessage )
    {
        case WM_CREATE:
            {
                // cast the create struct pointer to the object (as that is what we passed )
                LPCREATESTRUCT pCS = (LPCREATESTRUCT) lParam;

                pData = ( LP_NotifyProxyData ) pCS->lpCreateParams;
                ASSERT(pData != NULL );

                SetWindowLongPtr( hWnd, 0, (LONG_PTR) pData );
            }
            break;

        case WM_NCDESTROY:
            ASSERT(pData != NULL );

            // clear it so it won't be in use....
            SetWindowLongPtr( hWnd, 0, (LONG_PTR)NULL );

            // free the memory ...
            LocalFree( pData );
            break;

        case WM_CHANGENOTIFYMSG :
            if ( pData != NULL )
            {
                LPSHChangeNotificationLock pshcnl;
                LPITEMIDLIST *ppidl;
                LONG lEvent;

                // lock and break the info structure ....
                pshcnl = SHChangeNotification_Lock( (HANDLE)wParam,
                                                    (DWORD)lParam,
                                                    &ppidl,
                                                    &lEvent );

                // pass on to the old style client. ...
                lRes = SendMessage( pData->hwndParent, pData->wMsg, (WPARAM) ppidl, (LPARAM) lEvent );

                // new notifications ......
                SHChangeNotification_Unlock(pshcnl);
            }
            break;

        default:
            lRes = DefWindowProc( hWnd, iMessage, wParam, lParam );
            break;
    }

    return lRes;
}
