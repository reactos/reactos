/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#include "core.hxx"
#pragma hdrstop

#include "utils.hxx"
#include "mimedownload.hxx"
#include "bufferedstream.hxx"

#if MIMEASYNC

#include "viewerfactory.hxx"

#define MDQCHUNK 10
#define MTLSCHUNK 10

extern HINSTANCE g_hInstance;


// *************************************************
//                   QUEUE CLASS 

MimeDwnQueue::MimeDwnQueue(CRITICAL_SECTION *cs)
{
    m_prMTA = NULL;
    m_pCS = cs;
    m_nElems = m_nSize = 0;
}

MimeDwnQueue::~MimeDwnQueue()
{
    Clear();
    SafeDelete(m_prMTA);
}

// add the action to end the queue
// will return NULL if it could not add it (out of memory etc).
// otherwise will return the input argument
MimeThreadAction *MimeDwnQueue::Add(MimeThreadAction *pMTA)
{
    MimeThreadAction **prNew;
    MimeThreadAction *pRet = NULL;
    UINT newSize;

    EnterCriticalSection(m_pCS);

    // grow the array
    if (m_nElems == m_nSize)
    {
        newSize = m_nSize + MDQCHUNK;
        prNew = new_ne MimeThreadAction *[newSize];  
        if (!prNew)
            goto CleanUp;
        m_nSize = newSize;
        if (m_prMTA)
        {
            ::memmove(prNew, m_prMTA, m_nElems * sizeof(MimeThreadAction *));
            SafeDelete(m_prMTA);
            m_prMTA = prNew;
        }
        else
            m_prMTA = prNew;
    }

    // stuff new elem
    Assert(m_prMTA);
    *(m_prMTA + m_nElems) = pMTA;
    m_nElems++;

    pRet = pMTA;
CleanUp:
    LeaveCriticalSection(m_pCS);
    return pRet;
}


// remove the action at the front of the queue
// if none, will return NULL
// remove will never shrink down the allocated array (arbitrary decision)
// This is because often the download thread will normally just push it back on after processing a bit of data.
MimeThreadAction *MimeDwnQueue::Remove(void)
{
    MimeThreadAction *pRet = NULL;

    EnterCriticalSection(m_pCS);
    
    if (m_nElems)
    {
        Assert(m_prMTA);
        pRet = *m_prMTA;
        m_nElems--;
        ::memmove(m_prMTA, m_prMTA+1, m_nElems * sizeof(MimeThreadAction *));
    }
    LeaveCriticalSection(m_pCS);
    return pRet;
}

void MimeDwnQueue::RemoveAdd(void)
{
    EnterCriticalSection(m_pCS);
    
    MimeThreadAction* pMta = Remove();
    if (pMta)
    {
        Add(pMta);
    }
    LeaveCriticalSection(m_pCS);
    return;
}


MimeThreadAction *MimeDwnQueue::Peek(void)
{
    MimeThreadAction *pRet = NULL;

    EnterCriticalSection(m_pCS);
    
    if (m_nElems)
    {
        Assert(m_prMTA);
        pRet = *m_prMTA;
    }
    LeaveCriticalSection(m_pCS);
    return pRet;
}

// Clear removes all elements from the queue, aborts them and deletes them
void MimeDwnQueue::Clear(void)
{
    MimeThreadAction **pStep;
    UINT i;
    EnterCriticalSection(m_pCS);
    
    if (m_prMTA)
    {
        for (i=0, pStep = m_prMTA; i < m_nElems; i++, pStep++)
        {
            (*pStep)->Abort();
            delete (*pStep);
        }
        m_nElems = 0;
    }    

    LeaveCriticalSection(m_pCS);
}

void MimeDwnQueue::StopAsync(MDHANDLE handle)
{
    MimeThreadAction **pStep;
    UINT i;
    EnterCriticalSection(m_pCS);
    
    if (m_prMTA)
    {
        for (i=0, pStep = m_prMTA; i < m_nElems; i++, pStep++)
        {
            if ((*pStep)->GetDownloadHandle() == handle)
            {
                (*pStep)->StopAsync();
                break;
            }
        }
    }
    LeaveCriticalSection(m_pCS);
}

// *************************************************
//               PARSER ACTION CLASS
MimeDwnThreadAction::MimeDwnThreadAction(MDHANDLE mdh) 
: _mdh(mdh) 
{ 
    g_pMimeDwnWndMgr->AddRefGUIWnd(mdh);
    _fStopped = FALSE;
}

MimeDwnThreadAction::~MimeDwnThreadAction() 
{ 
    g_pMimeDwnWndMgr->ReleaseGUIWnd(_mdh);
}

MimeDwnParserAction::MimeDwnParserAction(MDHANDLE mdh, IXMLParser *pParser, ViewerFactory *pFactory, HANDLE evtLoadComplete)
    : MimeDwnThreadAction(mdh)
{
    _fFirstRun = TRUE;
    _evtLoadComplete = evtLoadComplete;
    _pParser = pParser;
    _pFactory = pFactory;
    Assert(_pParser && _pFactory);
    _pParser->AddRef();
    _pFactory->AddRef();
}

MimeDwnParserAction::~MimeDwnParserAction()
{
    Abort();
}

MTARESULT MimeDwnParserAction::Run(void)
{
    HRESULT hr = S_OK;
    MTARESULT mta;
    if (_fStopped)
    {
        mta = MTA_FAIL;
        hr = XML_E_STOPPED;
        goto CleanUp;
    }

    Assert(_pParser && _pFactory);
    if (_fFirstRun)
    {
        ResetEvent(_evtLoadComplete);
        _fFirstRun = false;
    }

    hr = _pParser->GetParserState();
    if (XMLPARSER_SUSPENDED != hr)
    {
        hr = _pParser->Run(4096L);
    }
    else
        hr = E_PENDING;
    
    // If E_PENDING is returned, either the XML download has blocked waiting, 
    // or we returned it from the factory NOTIFY_DATAAVAIL after processing this chunk
    // so add it back onto the end of the queue
    mta = (hr == E_PENDING) ? MTA_CONTINUE :
                    (SUCCEEDED(hr))   ? MTA_SUCCESS : MTA_FAIL;

    // We have to insure that the error is reported, if we generated it from the XML side
    // such as is the case with parse errors.  

    // When ViewerFactory::Error is called, we must report the error before delegating
    // because otherwise it will be too late.  However, parser and other doc errors are
    // often set in the default factories, and we don't have an error to report yet.
    // In the sync case we would evetually get a factory ENDDOCUMENT which would then
    // report the error.  In the Async case, however, this does not come because part
    // of the parse error handling is to abort the parser.  This sets _fStopped to TRUE
    // in the Run method and rightfully ENDDOCUMENT does not get called.
    // So instead we wait for the RUN to eventually return and report the error here.

    // Note that if we do not report the error, we get a hang.  This is because Trident
    // insists on getting an LastDataNotification and ONSTOPBinding or else it spins.
    // Rightfully so, because from it's point of view we're just writing [error] text to 
    // stream and it needs to know we are finished.
                     
    // Also note it is OK to report the error even if for user cancellations.  We won't
    // write spurious text at the end of the [aborted] downloaded because trident simply
    // cuts off stream reading.  Actually in that case the ENDDOCUMENT does comes in
    // and we would have seen error text on cancellations already.
CleanUp:
    if (mta == MTA_FAIL)
    {
        if (!_pFactory->isErrReported())
             _pFactory->reportError(hr);
    }

    return mta;
}


void MimeDwnParserAction::Close(void)
{
    // normal termination is the same as abnormal termination
    Abort();
}

void MimeDwnParserAction::Abort(void)
{
    HWND hWnd;

    if (_pFactory)
    {
        Assert(_pParser);
        _pFactory->closeXSL();
        Assert(g_pMimeDwnWndMgr);
        hWnd = g_pMimeDwnWndMgr->GetGUIWnd(GetDownloadHandle());
        Assert(hWnd != NULL);
        ::SendMessage(hWnd, WM_MIMECBEND, 0, (LPARAM)this);
        SetEvent(_evtLoadComplete);
    }
}

// clean up all resources associated with this action (on main thread)
void MimeDwnParserAction::End(void)
{
    if (_pFactory)
    {
        ViewerFactory* temp = _pFactory; // make this recurrsion safe.
        _pFactory = NULL;
        temp->releaseDocs();
        temp->Release();
    }
    if (_pParser) {
        release(&_pParser);  // this release method is recurrsion safe.
    }
}

// *************************************************
//                   WORKER THREAD
static DWORD g_MimeDwnThreadID = NULL;

void MimeDownloadMsgLoop()
{
    MSG msg;
    // process messages
    while(::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
	    ::DispatchMessage(&msg) ;
    }
}

DWORD WINAPI MimeDownloadThread(LPVOID lpvParam)
{
    DWORD dw;
    HRESULT hr;

    g_MimeDwnThreadID = GetCurrentThreadId();
    hr = CoInitialize(NULL);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_BELOW_NORMAL);
    
Wait: 
    // wait for terminate or process event
    dw = ::MsgWaitForMultipleObjects(2, g_MimeDwnEvents, FALSE, INFINITE, QS_ALLINPUT);

    switch (dw)
    {
    case WAIT_OBJECT_0:
    {
        MimeThreadAction *pMTA;
        MTARESULT res;
        // BUGBUG we are possibly in a busy wait loop here.  
        // A given download that is waiting on data will continually be taken off the queue
        // and put back on, continually returning E_PENDING (This typically doesn't happen 
        // on high-speed connections).  Better to go to sleep and then post an event
        // to wake it up.
        //
        // keep processing as long as something is in the queue
        // remove is atomic
        while ((pMTA = g_pMimeDwnQueue->Peek()) != NULL)
        {
            EnsureTls _EnsureTls;
            if (_EnsureTls.getTlsData())
            {
                Model model(_EnsureTls.getTlsData(),Rental);

                res = pMTA->Run();
                switch (res)
                {
                case MTA_SUCCESS:
                    pMTA->Close();
                    delete g_pMimeDwnQueue->Remove();
                    break;
                case MTA_FAIL:
                    pMTA->Abort();
                    delete g_pMimeDwnQueue->Remove();
                    break;
                case MTA_CONTINUE:
                    // We must remove/add so that we get a round robin effect
                    // which gives us better load balancing when multiple downloads
                    // are happening.  This has to be an atomic operation so that
                    // all downloads are always in the queue while they are active
                    // so that they can be found when they need to be aborted.
                    g_pMimeDwnQueue->RemoveAdd();
                    break;
                }
            }
            // give the messages a chance to churn
            MimeDownloadMsgLoop();
        }
        // nothing left in the queue, so reset the signal and sleep again
        ResetEvent(g_MimeDwnEvents[MDEVT_PROCESS]);
        goto Wait;
    }
    case WAIT_OBJECT_0 + 1:
    {
        EnsureTls _EnsureTls;
        if (_EnsureTls.getTlsData())
        {
            Model model(_EnsureTls.getTlsData(),Rental);

            // terminate event
            // Having this event gives us the flexibility to terminate the thread when we want.
            // clear the queue
            g_pMimeDwnQueue->Clear();
        }
        // again let the messages clear
        MimeDownloadMsgLoop();
        // reset the termination event
        ResetEvent(g_MimeDwnEvents[MDEVT_TERMINATE]);
        break;      // exit the thread proc
    }
    case WAIT_OBJECT_0 + 2:
        // process messages
        MimeDownloadMsgLoop();
        goto Wait;
    case WAIT_FAILED:
        // BUGBUG what to do?
        break;
    case WAIT_ABANDONED:
        // ONLY for mutexes, ignore?
        break;
    }

    CoUninitialize();
    g_MimeDwnThreadID = NULL;

    return 0;
}


// MIMECBEND handler for free download resources
LRESULT MimeCleanupDownload(HWND hwnd, MimeDwnParserAction *pPA)
{
    MSG msg;
    // Since the Cleanup is triggered via ::SendMessage while the
    // OnDataAvail is triggered via ::PostMessage, we cannot just blindly
    // terminate. We have to let OnDataAvail messages go through first, because
    // they write whatever data has been generated and commit the stream from
    // the trident point of view.
    while (::PeekMessage(&msg, hwnd, WM_MIMECBDATA, WM_MIMECBDATA, PM_REMOVE))
        ::DispatchMessage(&msg);

    // free the resources of the download
    pPA->End();
    return 0L;
}

// *************************************************
//                   UI THREAD WNDPROC
    
LRESULT CALLBACK MimeWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_MIMECBDATA:
        // validity check probably no longer needed, but just in case
        if ((DWORD_PTR)g_pMimeDwnWndMgr && g_pMimeDwnWndMgr->IsGUIWndRegistered(hwnd)) 
            ((MIMEBufferedStream *)lParam)->NotifyDataTrident((WORD)wParam); // WIN64, (WORD) is OK, because this really is a word
        return 0L;
    case WM_MIMECBEND:
        return MimeCleanupDownload(hwnd, (MimeDwnParserAction *)lParam);
    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
}


// *************************************************
//                DOWNLOAD SUBSYSTEM INIT/TERM
// *************************************************

// global variables which perform the threading and sychronization for the
// download thread

// One download thread per process
// As per win32 these variables will be per process

// Hence we can use critical sections rather than mutexes for access control

// The array of Windows for each download initiated from a GUI thread
MimeDwnWindowMgr *g_pMimeDwnWndMgr = NULL;

// The queue used by the download thread 
MimeDwnQueue *g_pMimeDwnQueue = NULL;

// Critical Section for gui thread manager
CRITICAL_SECTION g_MimeDwnCSWndMgr;

// Critical Section for queue handling
static CRITICAL_SECTION g_MimeDwnCSQueue;

// Critical Section for the thread spinner
static BOOL g_CSSpinInit = FALSE;
static CRITICAL_SECTION g_MimeDwnCSSpin;

// We have two synchronization events
// One to process more data, the other to terminate 
HANDLE g_MimeDwnEvents[MDEVT_LIM] = { NULL, NULL };

// The class registered for the window below
static ATOM g_mimeWndClass = NULL;

static ATOM g_MimeClassAtom = NULL;
static char g_MimeWndClassName[] = "XMLMimeWnd";

// Here is the handle to the thread
static HANDLE g_MimeDwnThread = NULL;

#ifdef _DEBUG
static g_cWindows = 0; // count of live windows !!
#endif

// Initialize the download subsystem
// This must be called from the GUI thread
//
// This will be called typically from the viewerfactory once it gets past the 
// prolog and start to download the heart of the document. It is at that
// point where it tries to download the rest on the other thread.
//
// No harm calling this each time, if already initialized this just
// returns S_OK

HRESULT InitializeMimeDwn(void)
{
    // create the thread mgr
    if (!g_pMimeDwnWndMgr)
    {
        InitializeCriticalSection(&g_MimeDwnCSWndMgr);

        g_pMimeDwnWndMgr = new_ne MimeDwnWindowMgr(&g_MimeDwnCSWndMgr);
        if (!g_pMimeDwnWndMgr)
            return E_OUTOFMEMORY;
    }

    // The hidden window
    // Create the per-thread "global" window
    // first register the class
    if (!g_MimeClassAtom)
    {
        WNDCLASSA wc;

        ::memset(&wc, 0, sizeof(WNDCLASS));
        wc.lpfnWndProc = (WNDPROC)MimeWndProc;
        wc.lpszClassName = g_MimeWndClassName;
        wc.hInstance = g_hInstance;
        
        g_MimeClassAtom = RegisterClassA(&wc);
        if (!g_MimeClassAtom)
            return E_FAIL;
    }
    
    // create the queue
    if (!g_pMimeDwnQueue)
    {
        InitializeCriticalSection(&g_MimeDwnCSQueue); 

        g_pMimeDwnQueue = new_ne MimeDwnQueue(&g_MimeDwnCSQueue);
        if (!g_pMimeDwnQueue)
            return E_OUTOFMEMORY;
    }

    // the synchronization events
    for (int i = 0; i < MDEVT_LIM; i++)
    {
        // BUGBUG TODO security attributes
        // manual-reset event.  The thread will reset the signal 
        // (after it is done with the loop, it calls reset and puts itself back to sleep)
        // (initial state is nonsignaled: wait for some download to wake it up)
        if (!g_MimeDwnEvents[i])
        {
            g_MimeDwnEvents[i] = CreateEventA(NULL, TRUE, FALSE, NULL);
            if (g_MimeDwnEvents[i] == NULL)
                return E_FAIL;
        }
    }

    return (StartMimeDownloadThread());
}


// Close down the subsystem above if at all active
// Terminate the thread by sending an event to it
// Wait for it to return, then close all handles and DeleteCriticalSection
// do in reverse order to allocate

void TerminateMimeDwn(void)
{
    // kill the download thread if there
    KillMimeDownloadThread();

    // Remove the critical section if it's initialized
    if (g_CSSpinInit)
        DeleteCriticalSection(&g_MimeDwnCSSpin);
    
    // clean up the events
    // the synchronization events
    for (int i = MDEVT_LIM - 1; i >= 0; i--)
    {
        // BUGBUG TODO security attributes
        // manual-reset event.  The thread will reset the signal 
        // (after it is done with the loop, it calls reset and puts itself back to sleep)
        // (initial state is nonsignaled: wait for some download to wake it up)
        if (g_MimeDwnEvents[i])
        {
            CloseHandle(g_MimeDwnEvents[i]);
            g_MimeDwnEvents[i] = NULL;
        }
    }

    // wipe out the queue
    if (g_pMimeDwnQueue)
    {
        g_pMimeDwnQueue->Clear();
        SafeDelete(g_pMimeDwnQueue);

        // finally the critical sections
        DeleteCriticalSection(&g_MimeDwnCSQueue);
    }

    // deregister the class
    if (g_MimeClassAtom)
    {
        UnregisterClassA(g_MimeWndClassName, g_hInstance);
        g_MimeClassAtom = NULL;
    }

    // the ThreadMgr

    if (g_pMimeDwnWndMgr)
    {
        EnterCriticalSection(&g_MimeDwnCSWndMgr);
        SafeDelete(g_pMimeDwnWndMgr);
        LeaveCriticalSection(&g_MimeDwnCSWndMgr);

        // finally the critical sections
        DeleteCriticalSection(&g_MimeDwnCSWndMgr);
    }
}

// Start the download thread
HRESULT StartMimeDownloadThread(void)
{
    HRESULT hr = S_OK;
    if (!g_CSSpinInit)
    {
        InitializeCriticalSection(&g_MimeDwnCSSpin);
        g_CSSpinInit = TRUE;
    }
    
    EnterCriticalSection(&g_MimeDwnCSSpin);
    if (!g_MimeDwnThread)
    {

        // BUGBUG TODO security attributes? 
        // immediate execution
        // 
        DWORD tid;
        g_MimeDwnThread = CreateThread(NULL, 500 * 1024L, MimeDownloadThread, NULL, 0, &tid);
        if (!g_MimeDwnThread)
            hr = E_FAIL;
    }
    LeaveCriticalSection(&g_MimeDwnCSSpin);
    return hr;
}



// Kill the download thread
void KillMimeDownloadThread(void)
{
    if (!g_CSSpinInit)
    {
        InitializeCriticalSection(&g_MimeDwnCSSpin);
        g_CSSpinInit = TRUE;
    }

    EnterCriticalSection(&g_MimeDwnCSSpin);
    // to terminate the thread, fire an event to it
    // wait until all elements in the queue are processed
    if (g_MimeDwnThread)
    {
        Assert(g_MimeDwnEvents[MDEVT_TERMINATE]);

        // to terminate the thread, fire an event to it
        // this ensures that all the unwinding of any downloads happens first,
        // which is necessary to cleanup and free all resources
        SetEvent(g_MimeDwnEvents[MDEVT_TERMINATE]);

        // wait for the thread to return

        // BUGBUG: all of the downloads should be sent abnormal terminations
        // unwinded normally with error conditions, all resources freed and
        // the queue emptied. The spun thread should then wait for more events
        // and finally pick up the signal above, and clean up.
        // But just to be paranoid in case this doesn't happen (who knows what
        // trident might do), we don't risk a total hang of IE.
        MsgWaitForDownloadObjects(1, &g_MimeDwnThread, FALSE, 10000);

        // close it
        CloseHandle(g_MimeDwnThread);
        g_MimeDwnThread = NULL;
    }
    LeaveCriticalSection(&g_MimeDwnCSSpin);
}

// return TRUE if the currently executing thread is the mime thread
BOOL IsMimeDownloadThread(void)
{
    return (GetCurrentThreadId() == g_MimeDwnThreadID);
}


// helper function for the GUI thread which will process messages
// while waiting for a signal from the worker thread
DWORD MsgWaitForDownloadObjects(DWORD count, LPHANDLE lpObjHandle, BOOL fWaitAll, DWORD timeout)
{
    DWORD dw = -1;
	DWORD dwMax;

    dwMax = WAIT_OBJECT_0 + count;
    while (true) {
        dw = ::MsgWaitForMultipleObjects(count, lpObjHandle, fWaitAll, timeout, QS_POSTMESSAGE | QS_SENDMESSAGE);
        if (dw == dwMax)
        {
            MSG msg;
            while(::PeekMessage(&msg, NULL, /* 0 */ WM_MIMECBFIRST, /* 0 */ WM_MIMECBLAST, PM_REMOVE))
                ::DispatchMessage(&msg);
        }
        else
			break;
    }
	return dw;
}


// *************************************************
//                   GUI PER DOWNLOAD MANAGER

// For each GUI thread we create a hidden window which
// is used to "marshall" the notification from the 
// worker thread to the gui thread that data is available
//
// We need a table because there is an option in IE to 
// launch a new browser as part of the same process
// one to one corresponce between IE browser windows and a GUI thread 
// (multiple frames in a browser window execute on the same thread)
MimeDwnWindowMgr::MimeDwnWindowMgr(CRITICAL_SECTION *cs)
{
    _prMimeWnd = NULL;
    m_pCS = cs;
    m_nSize = 0;
}

MimeDwnWindowMgr::~MimeDwnWindowMgr()
{
    // don't delete the windows. Let the process or gui thread delete the window
    // as part of its [ab]normal exiting
    SafeDelete(_prMimeWnd);
}

// Create a GUI Window for the download
// return a handle to identify the context
// return NULL handle if failure
MDHANDLE MimeDwnWindowMgr::AddGUIWnd(void)
{
    HWND h;
    UINT newSize, i, slot;
    WindInfo *prNew, *pNew, *pSearch;
    BOOL fCS = FALSE;
    MDHANDLE mdh = NULL;

    EnterCriticalSection(m_pCS);
    fCS = TRUE;

    // look for unused entry
    if (_prMimeWnd)
    {
        for (i = 0, pSearch = _prMimeWnd; i < m_nSize; i++, pSearch++)
        {
            if (pSearch->_hWnd == NULL)
                break;
        }
        slot = i;        
    }
    else
        slot = 0;

    // Grow the array we didn't find an unused slot
    if (slot == m_nSize)
    {
        newSize = m_nSize + MTLSCHUNK;
        prNew = new_ne WindInfo[newSize];
        if (!prNew)
            goto CleanUp;

        memset(prNew, 0, newSize * sizeof(WindInfo));
        m_nSize = newSize;
        if (_prMimeWnd)
        {
            ::memmove(prNew, _prMimeWnd, slot * sizeof(WindInfo));
            SafeDelete(_prMimeWnd);
            _prMimeWnd = prNew;
        }
        else
            _prMimeWnd = prNew;
    }
    fCS = FALSE;
    LeaveCriticalSection(m_pCS);

    // try and create the window
    h = CreateWindowA(g_MimeWndClassName, NULL, WS_POPUP, 0, 0, 0, 0, NULL, NULL, g_hInstance, NULL);
    if (!h)
        goto CleanUp;
#ifdef _DEBUG
    g_cWindows++;
#endif
    // stuff in the window
    EnterCriticalSection(m_pCS);
    fCS = TRUE;
    Assert(_prMimeWnd);

    _prMimeWnd[slot]._hWnd = h;
    _prMimeWnd[slot]._ulRefs = 1;

    // return one-based handle so NULL can be used
    mdh = slot + 1;

CleanUp:
    if (fCS)
        LeaveCriticalSection(m_pCS);
    return mdh;
}

HRESULT MimeDwnWindowMgr::AddRefGUIWnd(MDHANDLE handle)
{
    HRESULT hr = E_FAIL;
    EnterCriticalSection(m_pCS);
    if (GetGUIWnd(handle) != NULL)
    {
        WindInfo* ph = &_prMimeWnd[handle - 1];
        ph->_ulRefs++;
        hr = S_OK;
    }
    LeaveCriticalSection(m_pCS);
    return hr;

}

// Get the GUI Window corresponding to the specified GUI thread
// return NULL if none
HWND MimeDwnWindowMgr::GetGUIWnd(MDHANDLE handle)
{
    HWND hRet = NULL;
    EnterCriticalSection(m_pCS);
    if ((_prMimeWnd) && (handle != 0) && (handle-1 < m_nSize))   // one-based handle
        hRet = _prMimeWnd[handle - 1]._hWnd;      // if it's null, so be it
    LeaveCriticalSection(m_pCS);
    return hRet;
}

// Destroy the GUI Window for this download
HRESULT MimeDwnWindowMgr::ReleaseGUIWnd(MDHANDLE handle)
{
    HRESULT hr;
    UINT i;
    WindInfo *ph;
    BOOL fKillThread = FALSE;

    EnterCriticalSection(m_pCS);
    if (GetGUIWnd(handle) != NULL)
    {
        ph = &_prMimeWnd[handle - 1];
        Assert(ph->_hWnd != NULL);
        ph->_ulRefs--;
        if (ph->_ulRefs == 0)
        {
            ::DestroyWindow(ph->_hWnd);
#ifdef _DEBUG
            g_cWindows--;
#endif
            ph->_hWnd = NULL;      // clear the handle
        }
        hr = S_OK;
    }
    else
        hr = E_FAIL;
    LeaveCriticalSection(m_pCS);
    return hr;
}


// Check for hwnd validity
BOOL MimeDwnWindowMgr::IsGUIWndRegistered(HWND h)
{
    BOOL f = FALSE;
    UINT i;
    WindInfo *ph;

    EnterCriticalSection(m_pCS);
    if (_prMimeWnd)
    {
        for (i = 0, ph = _prMimeWnd; i < m_nSize; i++, ph++)
        {
            if (ph->_hWnd == h)
            {
                f = TRUE;
                break;
            }
        }
    }
    LeaveCriticalSection(m_pCS);    
    return f;
}

#endif
