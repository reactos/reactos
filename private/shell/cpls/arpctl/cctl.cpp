//  CCtl.CPP
//
//      Implementation of the control
//
//  Created 17-Apr-98 [JonT]

#include "priv.h"

#define CPP_FUNCTIONS
#include <crtfree.h>        // declare new, delete, etc.

// Define some things for debug.h
#define SZ_DEBUGINI         "arpctl.ini"
#define SZ_DEBUGSECTION     "arpctl"
#define SZ_MODULE           "ARPCTL"
#define DECLARE_DEBUG
#include <debug.h>

//--------------------------------------------------------------------
// Methods

//  CARPCtl::InitData
//      Script tells the control to get data from the app management object
//      and which order to sort in.

STDMETHODIMP
CARPCtl::InitData(
    DWORD dwSortOrder
    )
{
    IEnumInstalledApps* pEnumIns;
    IInstalledApp* pAppIns;
    APPINFODATA ai;
    CAppData* pcad;

    // Load the app manager if we haven't already
    if (m_pam == NULL)
    {
        if (FAILED(CoCreateInstance(CLSID_ShellAppManager, NULL,
            CLSCTX_INPROC, IID_IShellAppManager, (void**)&m_pam)))
        {
            TraceMsg(TF_ERROR, "Couldn't instantiate ShellAppManager object");
            return S_FALSE;
        }
    }

    // Make sure the worker thread isn't already running. Stop it if it is.
    _KillWorkerThread();

    // If we already have a list, nuke it
    _FreeAppData();

    // Now create the new list
    m_hdpaList = DPA_Create(8);

    // Now that we have the object, start enumerating the items
    if (FAILED(m_pam->EnumInstalledApps(&pEnumIns)))
    {
        TraceMsg(TF_ERROR, "Couldn't get installed app enumerator");
        return E_FAIL;
    }

    // Loop through all the apps on the machine, building our table
    while (SUCCEEDED(pEnumIns->Next(&pAppIns)))
    {
        // Get the 'fast' app info from the app manager object
        ai.cbSize = sizeof ai;
        ai.dwMask = AIM_DISPLAYNAME | AIM_VERSION | AIM_PUBLISHER | AIM_PRODUCTID | AIM_REGISTEREDOWNER
                       | AIM_REGISTEREDCOMPANY | AIM_SUPPORTURL | AIM_SUPPORTTELEPHONE | AIM_HELPLINK
                       | AIM_INSTALLLOCATION | AIM_INSTALLDATE;
        if (SUCCEEDED(pAppIns->GetAppInfo(&ai)) &&
            lstrlen(ai.pszDisplayName) > 0)
        {
            // Now save all this information away
            pcad = new CAppData(pAppIns, &ai);
            DPA_AppendPtr(m_hdpaList, pcad);
            m_dwcItems++;
            TraceMsg(TF_CTL, "ARP: Added item to list");
        }
        // NOTE: we do NOT release the pointer (pAppIns) here,
        // its lifetime is passed to the CAppData object
    }
    pEnumIns->Release();
    pEnumIns = NULL;

    // Tell the script we're done getting the synchronous data
    Fire_OnSyncDataReady();

    // Create and kick off the worker thread
    return _CreateWorkerThread();
}


//  CARPCtl::MoveFirst
//      Script tells control to move to the first item in the list.
//      Returns false if there are no items.

STDMETHODIMP
CARPCtl::MoveFirst(
    BOOL* pbool
    )
{
    // Set our current index to the start
    m_dwCurrentIndex = 0;

    // Return TRUE iff we are pointing to a valid item
    *pbool = m_dwCurrentIndex >= m_dwcItems ? FALSE : TRUE;

    return S_OK;
}


//  CARPCtl::MoveNext
//      Script tells control to move to the next item in the curren tlist.
//      Returns false if off the end of the list.

STDMETHODIMP
CARPCtl::MoveNext(
    BOOL* pbool
    )
{
    m_dwCurrentIndex++;

    // Return TRUE iff we are pointing to a valid item
    *pbool = m_dwCurrentIndex >= m_dwcItems ? FALSE : TRUE;

    return S_OK;
}


//  CARPCtl::MoveTo
//      Tells the control to move to a specific item

STDMETHODIMP
CARPCtl::MoveTo(
    DWORD dwRecNum,
    BOOL* pbool
    )
{
    // If they want to go past the end, fail it and don't move the pointer
    if (dwRecNum >= m_dwcItems)
    {
        *pbool = FALSE;
        return S_OK;
    }

    // Update the pointer and return success
    m_dwCurrentIndex = dwRecNum;
    *pbool = TRUE;
    return S_OK;
}

//--------------------------------------------------------------------
//  Properties

// SIMPLE_PROPERTY_GET
//
//  Defines a simple property get method so that we don't type the same
//  code over and over. It only works for strings copied from the APPINFODATA
//  structure.
//
//  This keeps the code cleaned up. but doesn't help
//  with the code bloat, so a better approach would be great.

#define SIMPLE_PROPERTY_GET(PropName)                                       \
STDMETHODIMP                                                                \
CARPCtl::get_##PropName##(BSTR* pVal)                                       \
{                                                                           \
    USES_CONVERSION;                                                        \
                                                                            \
    if (m_dwCurrentIndex >= m_dwcItems)                                     \
        return E_FAIL;                                                      \
                                                                            \
    *pVal = W2BSTR(((CAppData*)DPA_GetPtr(m_hdpaList, m_dwCurrentIndex))->  \
        GetData()->psz##PropName);                                          \
	return S_OK;                                                            \
}

// TODO: Since this is big code bloat, make sure we really need all these...

SIMPLE_PROPERTY_GET(DisplayName)
SIMPLE_PROPERTY_GET(Version)
SIMPLE_PROPERTY_GET(Publisher)
SIMPLE_PROPERTY_GET(ProductID)
SIMPLE_PROPERTY_GET(RegisteredOwner)
SIMPLE_PROPERTY_GET(Language)
SIMPLE_PROPERTY_GET(SupportUrl)
SIMPLE_PROPERTY_GET(SupportTelephone)
SIMPLE_PROPERTY_GET(HelpLink)
SIMPLE_PROPERTY_GET(InstallLocation)
SIMPLE_PROPERTY_GET(InstallSource)
SIMPLE_PROPERTY_GET(InstallDate)
SIMPLE_PROPERTY_GET(RequiredByPolicy)
SIMPLE_PROPERTY_GET(Contact)

//  Size
//      The calculated size of the application. Returns "Unknown" if not available

STDMETHODIMP
CARPCtl::get_Size(BSTR* pVal)
{
    USES_CONVERSION;
    TCHAR szSize[256];
    ULONG ulSize;
    LPTSTR WINAPI ShortSizeFormat(DWORD dw, LPTSTR szBuf);

    if (m_dwCurrentIndex >= m_dwcItems)
        return E_FAIL;

    // Get the size and truncate to a ULONG. If the app is bigger than 4G,
    // well, too bad.
    ulSize = (ULONG)((CAppData*)DPA_GetPtr(m_hdpaList, m_dwCurrentIndex))->
        GetSlowData()->ullSize;

    // If the size is zero, return unknown, otherwise,
    // Use the shell32 function to make a nicely formatted size string
    if (ulSize == 0)
        LoadString(g_hInst, IDS_UNKNOWN, szSize, ARRAYSIZE(szSize));
    else
        ShortSizeFormat(ulSize, szSize);

    // Return as a BSTR
    *pVal = W2BSTR(szSize);
	return S_OK;
}


//  TimesUsed
//      Returns the number of times used for this item

STDMETHODIMP
CARPCtl::get_TimesUsed(BSTR* pVal)
{
    USES_CONVERSION;
    int ncUsed;
    WCHAR szUsed[256];

    if (m_dwCurrentIndex >= m_dwcItems)
        return E_FAIL;

    ncUsed = ((CAppData*)DPA_GetPtr(m_hdpaList, m_dwCurrentIndex))->
        GetSlowData()->iTimesUsed;

    // Convert to a BSTR
    wsprintf(szUsed, TEXT("%d"), ncUsed);
    *pVal = W2BSTR(szUsed);
	return S_OK;
}


//  LastUsed
//      Returns last date the app was used

STDMETHODIMP
CARPCtl::get_LastUsed(BSTR* pVal)
{
    USES_CONVERSION;
    FILETIME ft;
    WCHAR szDate[256];

    if (m_dwCurrentIndex >= m_dwcItems)
        return E_FAIL;

    ft = ((CAppData*)DPA_GetPtr(m_hdpaList, m_dwCurrentIndex))->
        GetSlowData()->ftLastUsed;

    // Convert to a BSTR
    FileTimeToDateTimeString(&ft, szDate, sizeof szDate);
    *pVal = W2BSTR(szDate);
	return S_OK;
}


//  ItemCount
//      Number of items in current list

STDMETHODIMP
CARPCtl::get_ItemCount(long * pVal)
{
    *pVal = m_dwcItems;

	return S_OK;
}


//--------------------------------------------------------------------
// Object lifetime stuff

//  CARPCtl constructor

CARPCtl::CARPCtl()
{
    m_hdpaList = NULL;
    m_dwCurrentIndex = 0;
    m_dwcItems = 0;
    m_pam = NULL;
    m_fKillWorker = FALSE;
    m_hthreadWorker = NULL;
    m_hwndWorker = NULL;
}


//  CARPCtl::FinalRelease
//      Make sure memory is cleaned up

void
CARPCtl::FinalRelease()
{
    // Kill the worker thread if it's still around
    _KillWorkerThread();

    // Free our contained object
    if (m_pam)
    {
        m_pam->Release();
        m_pam = NULL;
    }

    // Clean up the application list
    _FreeAppData();
}

//--------------------------------------------------------------------
// Private methods


//  CARPCtl::_FreeAppData
//      Frees all memory associated with the application

void
CARPCtl::_FreeAppData()
{
    CAppData* pcad;

    if (!m_hdpaList)
        return;

    // Nuke everything on the list
    while ((pcad = (CAppData*)DPA_DeletePtr(m_hdpaList, 0)) != NULL)
        delete pcad;

    DPA_Destroy(m_hdpaList);
    m_hdpaList = NULL;
    m_dwcItems = 0;
}


//  CARPCtl::_CreateWorkerThread
//      Creates and starts a worker thread to do slow enumeration

HRESULT
CARPCtl::_CreateWorkerThread()
{
    DWORD thid;     // Not used but we have to pass something in

    // Create a hidden top-level window to post messages to from the
    // worker thread
    m_hwndWorker = SHCreateWorkerWindow(_WorkerWndProcWrapper,
        NULL, 0, 0, NULL, this);

    // Kick off the worker thread to do the slow enumeration.
    m_hthreadWorker = CreateThread(NULL, 0,
        (LPTHREAD_START_ROUTINE)_ThreadStartProcWrapper,
        (LPVOID)this, 0, &thid);

    return m_hthreadWorker != NULL ? S_OK : E_FAIL;
}


//  CARPCtl::_WorkerWndProcWrapper
//      Static wndproc wrapper. Calls the real non-static WndProc.

LRESULT
CALLBACK
CARPCtl::_WorkerWndProcWrapper(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    return ((CARPCtl*)GetWindowLong(hwnd, 0))->
        _WorkerWndProc(hwnd, uMsg, wParam, lParam);
}


//  CARPCtl::_WorkerWndProc
//      Used to fire events back to Trident since they can't be fired from
//      the worker thread.

LRESULT
CARPCtl::_WorkerWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WORKERWIN_EXIT_WINDOW:     // Sent by worker thread just before exit
        DestroyWindow(hwnd);
        m_hwndWorker = NULL;
        TraceMsg(TF_CTL, "Destroyed worker window");
        break;

    case WORKERWIN_FIRE_ROW_READY:  // Posted by worker thread when async data is ready
        Fire_OnAsyncDataReady(wParam);
#ifdef DEBUG
        char szText[256];
        wsprintfA(szText, "Fired event for row #%d", wParam);
        TraceMsg(TF_CTL, szText);
#endif
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


//  CARPCtl::_ThreadStartProcWrapper
//      Start and exit of worker thread to do slow app information
//      Friend function of CARPCtl.

DWORD
CALLBACK
CARPCtl::_ThreadStartProcWrapper(
    LPVOID lpParam          // this pointer of object since wrapper is static
    )
{
    return ((CARPCtl*)lpParam)->_ThreadStartProc();
}


// CARPCtl::_ThreadStartProc
//      Contains the code run on the worker thread where we get the slow
//      information about applications

DWORD
CARPCtl::_ThreadStartProc()
{
    DWORD i;
#ifdef DEBUG
    char szText[256];
#endif

    TraceMsg(TF_CTL, "Starting worker thread");

    // Loop through all enumerated items, getting the 'slow' information
    for (i = 0 ; i < m_dwcItems ; i++)
    {
        // If we've been asked to bail, do so
        if (m_fKillWorker)
            break;

        // Call to get the slow info for the item. Fire an event on success
        if (SUCCEEDED(((CAppData*)DPA_GetPtr(m_hdpaList, i))->ReadSlowData()))
        {
            PostMessage(m_hwndWorker, WORKERWIN_FIRE_ROW_READY, i, 0);

#ifdef DEBUG
            wsprintfA(szText, "Got slow info for item %d (%ls)\r\n",
                i, ((CAppData*)DPA_GetPtr(m_hdpaList, i))->GetData()->pszDisplayName);
            TraceMsg(TF_CTL, szText);
#endif
        }
        else
        {
#ifdef DEBUG
            wsprintfA(szText, "Failed getting slow info for %d (%ls)",
                i, ((CAppData*)DPA_GetPtr(m_hdpaList, i))->GetData()->pszDisplayName);
            TraceMsg(TF_CTL, szText);
#endif
        }
    }

    // Cause the window to be destroyed only if we weren't being killed.
    // In the kill case, we're making a GUI thread hang waiting on another thread.
    // The ONLY way this is legal is if the worker thread is known not to block,
    // which we don't. So, the window is killed in-thread by the killing thread.
    if (!m_fKillWorker)
        PostMessage(m_hwndWorker, WORKERWIN_EXIT_WINDOW, 0, 0);

    // Signal that we don't have a worker thread anymore. Prevents race
    // conditions.
    m_fKillWorker = FALSE;

    TraceMsg(TF_CTL, "Exiting worker thread");

    return 0;
}


//  CARPCtl::_KillWorkerThread
//      Kills the worker thread if one is around

void
CARPCtl::_KillWorkerThread()
{
    MSG msg;

    // If we have no worker thread, nothing to do
    if (!m_hthreadWorker)
         return;

    TraceMsg(TF_CTL, "Killing worker thread...");

    // Tell the worker thread to stop when it can
    m_fKillWorker = TRUE;

    // Now wait for the worker to stop
    if (WaitForSingleObject(m_hthreadWorker, 10000) == WAIT_TIMEOUT)
        TraceMsg(TF_ERROR, "Worker thread termination wait timed out!");
    else
        TraceMsg(TF_CTL, "Worker thread wait exited cleanly");

    // Now that the thread is stopped, release our hold so all its memory can go away
    CloseHandle(m_hthreadWorker);
    m_hthreadWorker = NULL;

    // Make sure that all messages to our worker HWND get processed
    if (m_hwndWorker)
    {
        while (PeekMessage(&msg, m_hwndWorker, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);
    }

    // Now tell the window to exit
    SendMessage(m_hwndWorker, WORKERWIN_EXIT_WINDOW, 0, 0);
}


//  FileTimeToDateTimeString


STDAPI_(void)
FileTimeToDateTimeString(LPFILETIME pft, LPTSTR pszBuf, UINT cchBuf)
{
    SYSTEMTIME st;
    int cch;

    FileTimeToLocalFileTime(pft, pft);
    FileTimeToSystemTime(pft, &st);

    cch = GetDateFormat(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &st, NULL, pszBuf, cchBuf);
    cchBuf -= cch;
    pszBuf += cch - 1;

    *pszBuf++ = TEXT(' ');
    *pszBuf = 0;          // (in case GetTimeFormat doesn't add anything)
    cchBuf--;

    GetTimeFormat(LOCALE_USER_DEFAULT, TIME_NOSECONDS, &st, NULL, pszBuf, cchBuf);
}


