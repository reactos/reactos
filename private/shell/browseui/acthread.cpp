// Copyright 1998 Microsoft

#include "priv.h"
#include "autocomp.h"
#include "dbgmem.h"

#define AC_GIVEUP_COUNT           1000
#define AC_TIMEOUT          (60 * 1000)

//
// Thread messages
//
enum
{
    ACM_FIRST = WM_USER,
    ACM_STARTSEARCH,
    ACM_STOPSEARCH,
    ACM_SETFOCUS,
    ACM_KILLFOCUS,
    ACM_QUIT,
    ACM_LAST,
};

//
// Special prefixes that we optionally filter out
//
struct MyBstr
{
    int cch;
    LPCWSTR psz;
};

MyBstr g_rgSpecialPrefix[] =
{
    {4,  L"www."},
    {11, L"http://www."},   // This must be before "http://"
    {7,  L"http://"},
    {8,  L"https://"},
};


//+-------------------------------------------------------------------------
// CACString functions - Hold autocomplete strings
//--------------------------------------------------------------------------
ULONG CACString::AddRef()
{
    InterlockedIncrement((LPLONG)&m_dwRefCount);
    return m_dwRefCount;
}

ULONG CACString::Release()
{
    ASSERT(m_dwRefCount > 0);

    if (InterlockedDecrement((LPLONG)&m_dwRefCount))
    {
        return m_dwRefCount;
    }

    delete this;
    return 0;
}

CACString* CreateACString(LPCWSTR pszStr, int iIgnore)
{
    ASSERT(pszStr);

    int cChars = lstrlen(pszStr);
    CACString* pStr = (CACString*)LocalAlloc(LPTR, cChars * SIZEOF(WCHAR) + sizeof(CACString));
    if (pStr)
    {
        StrCpy(pStr->m_sz, pszStr);
        pStr->m_dwRefCount  = 1;
        pStr->m_cChars      = cChars;
        pStr->m_iIgnore     = iIgnore;
    }
    return pStr;
}

//+-------------------------------------------------------------------------
// IUnknown methods
//--------------------------------------------------------------------------
HRESULT CACThread::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = SAFECAST(this, IUnknown*);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG CACThread::AddRef(void)
{
    InterlockedIncrement((LPLONG)&m_dwRefCount);
    return m_dwRefCount;
}

ULONG CACThread::Release(void)
{
    ASSERT(m_dwRefCount > 0);

    if (InterlockedDecrement((LPLONG)&m_dwRefCount))
    {
        return m_dwRefCount;
    }

    delete this;
    return 0;
}

//+-------------------------------------------------------------------------
// Constructor
//--------------------------------------------------------------------------
CACThread::CACThread(CAutoComplete& rAutoComp)
:
    m_pAutoComp(&rAutoComp),
    m_dwRefCount(1)
{
    ASSERT(!m_fWorkItemQueued);
    ASSERT(!m_idThread);
    ASSERT(!m_hCreateEvent);
    ASSERT(!m_fDisabled);
    ASSERT(!m_pszSearch);
    ASSERT(!m_hdpa_list);
    ASSERT(!m_pes);
    ASSERT(!m_pacl);

    DllAddRef();
}

//+-------------------------------------------------------------------------
// Destructor
//--------------------------------------------------------------------------
CACThread::~CACThread()
{
    SyncShutDownBGThread();  // In case somehow

    // These should have been freed.
    ASSERT(!m_idThread);
    ASSERT(!m_hdpa_list);

    SAFERELEASE(m_pes);
    SAFERELEASE(m_pacl);

    DllRelease();
}

//+-------------------------------------------------------------------------
// Initialize the background thread
//--------------------------------------------------------------------------
BOOL CACThread::Init
(
    IEnumString* pes,   // source of the autocomplete strings
    IACList* pacl       // optional interface to call Expand
)
{
    // BUGBUG: We need to marshal these interfaces to this thread!
    ASSERT(pes);
    m_pes = pes;
    m_pes->AddRef();

    if (pacl)
    {
        m_pacl = pacl;
        m_pacl->AddRef();
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
// Called when the edit box recieves focus. We use this event to create
// a background thread or to keep the backgroung thread from shutting down
//--------------------------------------------------------------------------
void CACThread::GotFocus()
{
    TraceMsg(AC_GENERAL, "CACThread::GotFocus()");

    // Should not be NULL if the foreground thread is calling us!
    ASSERT(m_pAutoComp);

    //
    // Check to see if autocomplete is supposed to be enabled.
    //
    if (m_pAutoComp && m_pAutoComp->IsEnabled())
    {
        m_fDisabled = FALSE;

        if (m_fWorkItemQueued)
        {
            // If the thread hasn't started yet, wait for a thread creation event
            if (0 == m_idThread && m_hCreateEvent)
            {
                WaitForSingleObject(m_hCreateEvent, 1000);
            }

            if (m_idThread)
            {
                //
                // Tell the thread to cancel its timeout and stay alive.
                //
                // BUGBUG: We have a race condition here.  The thread can be
                // in the process of shutting down!
                PostThreadMessage(m_idThread, ACM_SETFOCUS, 0, 0);
            }
        }
        else
        {
            //
            // The background thread signals an event when it starts up.
            // We wait on this event before trying a synchronous shutdown
            // because any posted messages would be lost.
            //
            if (NULL == m_hCreateEvent)
            {
                m_hCreateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            }
            else
            {
                ResetEvent(m_hCreateEvent);
            }

            //
            // Make sure we have a background search thread.
            //
            // If we start it any later, we run the risk of not
            // having its message queue available by the time
            // we post a message to it.
            //
            // AddRef ourselves now, to prevent us getting freed
            // before the thread proc starts running.
            //
            AddRef();

            // Call to Shlwapi thread pool
            if (SHQueueUserWorkItem(_ThreadProc,
                                     this,
                                     0,
                                     (DWORD_PTR)NULL,
                                     NULL,
                                     "browseui.dll",
                                     TPS_LONGEXECTIME | TPS_DEMANDTHREAD
                                     ))
            {
                InterlockedExchange(&m_fWorkItemQueued, TRUE);
            }
            else
            {
                // Couldn't get thread
                Release();
            }
        }
    }
    else
    {
        m_fDisabled = TRUE;
        _SendAsyncShutDownMsg();
    }
}

//+-------------------------------------------------------------------------
// Called when the edit box loses focus.
//--------------------------------------------------------------------------
void CACThread::LostFocus()
{
    TraceMsg(AC_GENERAL, "CACThread::_LostFocus()");

    //
    // If there is a thread around, tell it to stop searching.
    //
    if (m_idThread)
    {
        StopSearch();
        PostThreadMessage(m_idThread, ACM_KILLFOCUS, 0, 0);
    }
}

//+-------------------------------------------------------------------------
// Sends the search request to the background thread.
//--------------------------------------------------------------------------
BOOL CACThread::StartSearch
(
    LPCWSTR pszSearch,  // String to search
    DWORD dwOptions     // ACO_* flags
)
{
    BOOL fRet = FALSE;

    // If the thread hasn't started yet, wait for a thread creation event
    if (0 == m_idThread && m_fWorkItemQueued && m_hCreateEvent)
    {
        WaitForSingleObject(m_hCreateEvent, 1000);
    }

    if (m_idThread)
    {
        LPWSTR pszSrch = TrcStrDup(pszSearch);
        if (pszSrch)
        {
            //
            // This is being sent to another thread, remove it from this thread's
            // memlist.
            //
            DbgRemoveFromMemList(pszSrch);

            //
            // If the background thread is already searching, abort that search
            //
            StopSearch();

            //
            // Send request off to the background search thread.
            //
            if (PostThreadMessage(m_idThread, ACM_STARTSEARCH, dwOptions, (LPARAM)pszSrch))
            {
                fRet = TRUE;
            }
            else
            {
                TraceMsg(AC_GENERAL, "CACThread::_StartSearch could not send message to thread!");
                TrcLocalFree(pszSrch);
            }
        }
    }
    return fRet;
}

//+-------------------------------------------------------------------------
// Tells the background thread to stop and pending search
//--------------------------------------------------------------------------
void CACThread::StopSearch()
{
    TraceMsg(AC_GENERAL, "CACThread::_StopSearch()");

    //
    // Tell the thread to stop.
    //
    if (m_idThread)
    {
        PostThreadMessage(m_idThread, ACM_STOPSEARCH, 0, 0);
    }
}

//+-------------------------------------------------------------------------
// Posts a quit message to the background thread
//--------------------------------------------------------------------------
void CACThread::_SendAsyncShutDownMsg()
{
    if (0 == m_idThread && m_fWorkItemQueued && m_hCreateEvent)
    {
        //
        // Make sure that the thread has started up before posting a quit
        // message or the quit message will be lost!
        //
        WaitForSingleObject(m_hCreateEvent, 3000);
    }

    if (m_idThread)
    {
        // Stop the search because it can hold up the thread for quite a
        // while by waiting for disk data.
        StopSearch();

        // Tell the thread to go away, we won't be needing it anymore.
        PostThreadMessage(m_idThread, ACM_QUIT, 0, 0);
    }
}

//+-------------------------------------------------------------------------
// Synchroniously shutdown the background thread
//
// Note: this is no longer synchronous because we now orphan this object
// when the associated autocomplet shuts down.
//
//--------------------------------------------------------------------------
void CACThread::SyncShutDownBGThread()
{
    _SendAsyncShutDownMsg();

    // Block shutdown if background thread is about to use this variable
    ENTERCRITICAL;
    m_pAutoComp = NULL;
    LEAVECRITICAL;

    if (m_hCreateEvent)
    {
        CloseHandle(m_hCreateEvent);
        m_hCreateEvent = NULL;
    }
}

//+-------------------------------------------------------------------------
// Frees all thread data
//--------------------------------------------------------------------------
void CACThread::_FreeThreadData()
{
    if (m_hdpa_list)
    {
        CAutoComplete::_FreeDPAPtrs(m_hdpa_list);
        m_hdpa_list = NULL;
    }

    if (m_pszSearch)
    {
        TrcLocalFree(m_pszSearch);
        m_pszSearch = NULL;
    }

    InterlockedExchange(&m_idThread, 0);
    InterlockedExchange(&m_fWorkItemQueued, 0);
}

//+-------------------------------------------------------------------------
// Background thread procedure - calls the main thread loop
//--------------------------------------------------------------------------
DWORD WINAPI CACThread::_ThreadProc(LPVOID lpv)
{
    CACThread * pThis = (CACThread *)lpv;
    HRESULT hr = CoInitialize(0);

    AssertMsg(SUCCEEDED(hr), TEXT("CACThread::_ThreadProc() we weren't able to init COM.  Why?  This will make us useless.  Can this happen when out of memory?"));
    if (SUCCEEDED(hr))
    {
        hr = pThis->_ThreadLoop();
    }

    // We can now release the thread's reference on this object
    pThis->Release();

    if (SUCCEEDED(hr))
    {
        // It's necessary to uninitialize OLE after we release pThis
        // because it releases COM objects (m_pacl and m_pacl).
        // COM will unload DLLs when COM is totally uninitialized, even
        // if someone still holds onto a COM object.  This means referensing
        // that COM object will fault because it's DLL is unloaded.
        // NT #330058 - BryanSt
        CoUninitialize();
    }

    return 0;
}


//+-------------------------------------------------------------------------
// Message pump for the background thread
//--------------------------------------------------------------------------
HRESULT CACThread::_ThreadLoop()
{
    MSG Msg;
    DWORD dwTimeout = INFINITE;
    BOOL fStayAlive = TRUE;

    TraceMsg(AC_WARNING, "AutoComplete service thread started.");

    DebugMemLeak(DML_TYPE_THREAD | DML_BEGIN);

    //
    // We need to call a window's api for a message queue to be created
    // so we call peekmessage.  Then we get the thread id and thread handle
    // and we signal an event to tell the forground thread that we are listening.
    //
    while (PeekMessage(&Msg, NULL, ACM_FIRST, ACM_LAST, PM_REMOVE))
    {
        // purge any messages we care about from previous owners of this thread.
    }

    // The forground thread needs this is so that it can post us messages
    InterlockedExchange(&m_idThread, GetCurrentThreadId());

    SetEvent(m_hCreateEvent);
    HANDLE hThread = GetCurrentThread();
    int nOldPriority = GetThreadPriority(hThread);
    SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);

    while (fStayAlive)
    {
        while (fStayAlive && PeekMessage(&Msg, NULL, 0, (UINT)-1, PM_NOREMOVE))
        {
            GetMessage(&Msg, NULL, 0, 0);
            ASSERT(Msg.hwnd == NULL);

            TraceMsg(AC_GENERAL, "AutoCompleteThread: Message %x recieved.", Msg.message);

            switch (Msg.message)
            {
            case ACM_STARTSEARCH:
                TraceMsg(AC_GENERAL, "AutoCompleteThread: Search started.");
                dwTimeout = INFINITE;
                _Search((LPWSTR)Msg.lParam, (DWORD)Msg.wParam);
                TraceMsg(AC_GENERAL, "AutoCompleteThread: Search completed.");
                break;

            case ACM_STOPSEARCH:
                while (PeekMessage(&Msg, Msg.hwnd, ACM_STOPSEARCH, ACM_STOPSEARCH, PM_REMOVE))
                    ;
                TraceMsg(AC_GENERAL, "AutoCompleteThread: Search stopped.");
                break;

            case ACM_SETFOCUS:
                TraceMsg(AC_GENERAL, "AutoCompleteThread: Got Focus.");
                dwTimeout = INFINITE;
                break;

            case ACM_KILLFOCUS:
                TraceMsg(AC_GENERAL, "AutoCompleteThread: Lost Focus.");
                dwTimeout = AC_TIMEOUT;
                break;

            case ACM_QUIT:
                TraceMsg(AC_GENERAL, "AutoCompleteThread: ACM_QUIT received.");
                fStayAlive = FALSE;
                break;
            }
        }

        if (fStayAlive)
        {
            TraceMsg(AC_GENERAL, "AutoCompleteThread: Sleeping for%s.", dwTimeout == INFINITE ? "ever" : " one minute");
            DWORD dwWait = MsgWaitForMultipleObjects(0, NULL, FALSE, dwTimeout, QS_ALLINPUT);
#ifdef DEBUG
            switch (dwWait)
            {
            case 0xFFFFFFFF:
                ASSERT(dwWait != 0xFFFFFFFF);
                break;

            case WAIT_TIMEOUT:
                TraceMsg(AC_GENERAL, "AutoCompleteThread: Timeout expired.");
                break;
            }
#endif
            fStayAlive = (dwWait == WAIT_OBJECT_0);
        }
    }

    TraceMsg(AC_GENERAL, "AutoCompleteThread: Thread dying.");

    _FreeThreadData();
    SetThreadPriority(hThread, nOldPriority);
    DebugMemLeak(DML_TYPE_THREAD | DML_END);

    // Purge any remaining messages before returning this thread to the pool.
    while (PeekMessage(&Msg, NULL, ACM_FIRST, ACM_LAST, PM_REMOVE))
    {
    }

    TraceMsg(AC_WARNING, "AutoCompleteThread: Thread dead.");
    return S_OK;
}

//+-------------------------------------------------------------------------
// Returns true if the search string matches one or more characters of a
// prefix that we filter out matches to
//--------------------------------------------------------------------------
BOOL CACThread::MatchesSpecialPrefix(LPCWSTR pszSearch)
{
    BOOL fRet = FALSE;
    int cchSearch = lstrlen(pszSearch);
    for (int i = 0; i < ARRAYSIZE(g_rgSpecialPrefix); ++i)
    {
        MyBstr& rPrefix = g_rgSpecialPrefix[i];

        // See if the search string matches one or more characters of the prefix
        if (cchSearch <= rPrefix.cch &&
            StrCmpNI(rPrefix.psz, pszSearch, cchSearch) == 0)
        {
            fRet = TRUE;
            break;
        }
    }
    return fRet;
}

//+-------------------------------------------------------------------------
// Returns the length of the prefix it the string starts with a special
// prefix that we filter out matches to.  Otherwise returns zero.
//--------------------------------------------------------------------------
int CACThread::GetSpecialPrefixLen(LPCWSTR psz)
{
    int nRet = 0;
    int cch = lstrlen(psz);
    for (int i = 0; i < ARRAYSIZE(g_rgSpecialPrefix); ++i)
    {
        MyBstr& rPrefix = g_rgSpecialPrefix[i];

        if (cch >= rPrefix.cch &&
            StrCmpNI(rPrefix.psz, psz, rPrefix.cch) == 0)
        {
            nRet = rPrefix.cch;
            break;
        }
    }
    return nRet;
}

//+-------------------------------------------------------------------------
// Searches for items that match pszSearch.
//--------------------------------------------------------------------------
void CACThread::_Search
(
    LPWSTR pszSearch,   // String to search for (we must free this)
    DWORD dwOptions     // ACO_* flags
)
{
    TraceMsg(AC_GENERAL, "CACThread(BGThread)::_Search(pszSearch=0x%x)", pszSearch);

    // Save the search string in our thread data so it is still freed if this thread is killed
    m_pszSearch = pszSearch;
    DbgAddToMemList(pszSearch);

    // If we were passed a wildcard string, then everything matches
    BOOL fWildCard = ((pszSearch[0] == CH_WILDCARD) && (pszSearch[1] == L'\0'));

    // To avoid huge number of useless matches, avoid matches
    // to common prefixes
    BOOL fFilter = (dwOptions & ACO_FILTERPREFIXES) && MatchesSpecialPrefix(pszSearch);
    BOOL fAppendOnly = IsFlagSet(dwOptions, ACO_AUTOAPPEND) && IsFlagClear(dwOptions, ACO_AUTOSUGGEST);

    if (m_pes)    // paranoia
    {
        // If this fails, the m_pes->Next() will likely do something
        // bad, so we will avoid it altogether.
        if (SUCCEEDED(m_pes->Reset()))
        {
            BOOL fStopped = FALSE;
            BOOL fLimitReached = FALSE;
            LPOLESTR pszUrl;
            ULONG cFetched;

            _DoExpand(pszSearch);
            int cchSearch = lstrlen(pszSearch);

            while (!fStopped && !fLimitReached && (m_pes->Next(1, &pszUrl, &cFetched) == S_OK))
            {
                //
                // First check for a simple match
                //
                if (fWildCard ||
                    (StrCmpNI(pszUrl, pszSearch, cchSearch) == 0) &&

                    // Filter out matches to common prefixes
                    (!fFilter || GetSpecialPrefixLen(pszUrl) == 0))
                {
                    if (!_AddToList(pszUrl, 0))
                    {
                        // Out of memory or we reached our limit
                        fLimitReached = TRUE;
                    }
                }

                // If the dropdown is enabled, check for matches after common prefixes.
                if (!fAppendOnly)
                {
                    //
                    // Also check for a match if we skip the protocol. We
                    // assume that pszUrl has been cononicalized (protocol
                    // in lower case).
                    //
                    LPCWSTR psz = pszUrl;
                    if (StrCmpN(pszUrl, L"http://", 7) == 0)
                    {
                        psz += 7;
                    }
                    if (StrCmpN(pszUrl, L"https://", 8) == 0 ||
                        StrCmpN(pszUrl, L"file:///", 8) == 0)
                    {
                        psz += 8;
                    }

                    if (psz != pszUrl &&
                        StrCmpNI(psz, pszSearch, cchSearch) == 0 &&

                        // Filter out "www." prefixes
                        (!fFilter || GetSpecialPrefixLen(psz) == 0))
                    {
                        if (!_AddToList(pszUrl, (int)(psz - pszUrl)))
                        {
                            // Out of memory or we reached our limit
                            fLimitReached = TRUE;
                        }
                    }

                    //
                    // Finally check for a match if we skip "www." after
                    // the optional protocol
                    //
                    if (StrCmpN(psz, L"www.", 4) == 0 &&
                        StrCmpNI(psz + 4, pszSearch, cchSearch) == 0)
                    {
                        if (!_AddToList(pszUrl, (int)(psz + 4 - pszUrl)))
                        {
                            // Out of memory or we reached our limit
                            fLimitReached = TRUE;
                        }
                    }
                }

                CoTaskMemFree(pszUrl);

                // Check to see if the search was canceled
                MSG msg;
                fStopped = PeekMessage(&msg, NULL, ACM_STOPSEARCH, ACM_STOPSEARCH, PM_NOREMOVE);
#ifdef DEBUG
//fStopped = FALSE;
                if (fStopped)
                    TraceMsg(AC_GENERAL, "AutoCompleteThread: Search TERMINATED");
#endif
            }

            if (fStopped)
            {
                // Search aborted so free the results
                if (m_hdpa_list)
                {
                    // clear the list
                    CAutoComplete::_FreeDPAPtrs(m_hdpa_list);
                    m_hdpa_list = NULL;
                }
            }
            else
            {
                //
                // Sort the results and remove duplicates
                //
                if (m_hdpa_list)
                {
                    DPA_Sort(m_hdpa_list, _DpaCompare, 0);

                    //
                    // Perge duplicates.
                    //
                    for (int i = DPA_GetPtrCount(m_hdpa_list) - 1; i > 0; --i)
                    {
                        CACString& rStr1 = *(CACString*)DPA_GetPtr(m_hdpa_list, i-1);
                        CACString& rStr2 = *(CACString*)DPA_GetPtr(m_hdpa_list, i);

                        // Since URLs are case sensitive, we can't ignore case.
                        if (rStr1.StrCmpI(rStr2) == 0)
                        {
                            // We have a match, so keep the longest string.
                            if (rStr1.GetLength() > rStr2.GetLength())
                            {
                                DPA_DeletePtr(m_hdpa_list, i);
                                rStr2.Release();
                            }
                            else
                            {
                                DPA_DeletePtr(m_hdpa_list, i-1);
                                rStr1.Release();
                            }
                        }
                        else
                        {
                            //
                            // Special case: If this is a web site and the entries
                            // are identical except one has an extra slash on the end
                            // from a redirect, remove the redirected one.
                            //
                            int cch1 = rStr1.GetLengthToCompare();
                            int cch2 = rStr2.GetLengthToCompare();
                            int cchDiff = cch1 - cch2;

                            if (
                                // Length must differ by one
                                (cchDiff == 1 || cchDiff == -1) &&

                                // One string must have a terminating slash
                                ((cch1 > 0 && rStr1[rStr1.GetLength() - 1] == L'/') ||
                                 (cch2 > 0 && rStr2[rStr2.GetLength() - 1] == L'/')) &&

                                // Must be a web site
                                ((StrCmpN(rStr1, L"http://", 7) == 0 || StrCmpN(rStr1, L"https://", 8) == 0) ||
                                 (StrCmpN(rStr2, L"http://", 7) == 0 || StrCmpN(rStr2, L"https://", 8) == 0)) &&

                                // Must be identical up to the slash (ignoring prefix)
                                StrCmpNI(rStr1.GetStrToCompare(), rStr2.GetStrToCompare(), (cchDiff > 0) ? cch2 : cch1) == 0)
                            {
                                // Remove the longer string with the extra slash
                                if (cchDiff > 0)
                                {
                                    DPA_DeletePtr(m_hdpa_list, i-1);
                                    rStr1.Release();
                                }
                                else
                                {
                                    DPA_DeletePtr(m_hdpa_list, i);
                                    rStr2.Release();
                                }
                            }
                        }
                    }
                }

                // Pass the results to the foreground thread
                ENTERCRITICAL;
                if (m_pAutoComp)
                {
                    HWND hwndEdit = m_pAutoComp->m_hwndEdit;
                    UINT uMsgSearchComplete = m_pAutoComp->m_uMsgSearchComplete;
                    LEAVECRITICAL;

                    // Unix loses keys if we post the message, so we send the message
                    // outside our critical section
                    SendMessage(hwndEdit, uMsgSearchComplete, fLimitReached, (LPARAM)m_hdpa_list);
                }
                else
                {
                    LEAVECRITICAL;

                    // We've been orphaned, so free the list and bail
                    CAutoComplete::_FreeDPAPtrs(m_hdpa_list);
                }

                // The foreground thread owns the list now
                m_hdpa_list = NULL;
            }
        }
        else
        {
            ASSERT(0);    // m_pes->Reset Failed!!
        }
    }

    // We must free the search string
    m_pszSearch = NULL;

    // Note if the thread is killed here, we leak the string
    // but at least we will not try to free it twice (which is worse)
    // because we nulled m_pszSearch first.
    TrcLocalFree(pszSearch);
}

//+-------------------------------------------------------------------------
// Used to sort items alphabetically
//--------------------------------------------------------------------------
int CALLBACK CACThread::_DpaCompare(LPVOID p1, LPVOID p2, LPARAM lParam)
{
    CACString* ps1 = (CACString*)p1;
    CACString* ps2 = (CACString*)p2;

    return ps1->StrCmpI(*ps2);
}

//+-------------------------------------------------------------------------
// Adds a string to our HDPA.  Returns TRUE is successful.
//--------------------------------------------------------------------------
BOOL CACThread::_AddToList
(
    LPTSTR pszUrl,  // string to add
    int cchMatch    // offset into string where the match occurred
)
{
    TraceMsg(AC_GENERAL, "CACThread(BGThread)::_AddToList(pszUrl = %s)",
        (pszUrl ? pszUrl : TEXT("(null)")));

    BOOL fRet = TRUE;

    //
    // Create a new list if necessary.
    //
    if (!m_hdpa_list)
    {
        m_hdpa_list = DPA_Create(AC_LIST_GROWTH_CONST);
    }

    if (m_hdpa_list && DPA_GetPtrCount(m_hdpa_list) < AC_GIVEUP_COUNT)
    {
        CACString* pStr = CreateACString(pszUrl, cchMatch);
        if (pStr)
        {
            if (DPA_AppendPtr(m_hdpa_list, pStr) == -1)
            {
                pStr->Release();
                fRet = FALSE;
            }
        }
    }
    else
    {
        fRet = FALSE;
    }

    return fRet;
}

//+-------------------------------------------------------------------------
// This function will attempt to use the autocomplete list to bind to a
// location in the Shell Name Space. If that succeeds, the AutoComplete List
// will then contain entries which are the display names in that ISF.
//--------------------------------------------------------------------------
void CACThread::_DoExpand(LPCWSTR pszSearch)
{
    LPCWSTR psz;

#ifdef UNIX
    // IEUNIX : NFS mounted file systems may hang during this operation on UNIX.
    // so we don't do path expansion(s).
    return;
#endif
    if (!m_pacl)
    {
        //
        // Doesn't support IAutoComplete, doesn't have Expand method.
        //
        return;
    }

    if (*pszSearch == 0)
    {
        //
        // No string means no expansion necessary.
        //
        return;
    }

    //
    // psz points to last character.
    //
    psz = pszSearch + lstrlen(pszSearch);
    psz = CharPrev(pszSearch, psz);

    //
    // Search backwards for an expand break character.
    //
    while (psz != pszSearch && *psz != TEXT('/') && *psz != TEXT('\\'))
    {
        psz = CharPrev(pszSearch, psz);
    }

    if (*psz == TEXT('/') || *psz == TEXT('\\'))
    {
        SHSTR ss;

        psz++;
        if (SUCCEEDED(ss.SetStr(pszSearch)))
        {
            //
            // Trim ss so that it contains everything up to the last
            // expand break character.
            //
            ss[psz - pszSearch] = TEXT('\0');

            //
            // Call expand on the string.
            //
            m_pacl->Expand(ss);
        }
    }
}
