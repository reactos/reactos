//  worker.cpp
//
//      Implementation of the worker thread object
//

#include "priv.h"

// Do not build this file if on Win9X or NT4
#ifndef DOWNLEVEL_PLATFORM

#include "resource.h"
#include "worker.h"


//--------------------------------------------------------------------
//
//
//  CWorkerThread class
//
//
//--------------------------------------------------------------------


//  CWorkerThread constructor

CWorkerThread::CWorkerThread() : _cRef(1)
{
    ASSERT(NULL == _hthreadWorker);
    ASSERT(NULL == _hwndWorker);
    ASSERT(FALSE == _fKillWorker);
    ASSERT(NULL == _pwe);
    ASSERT(0 == _cRefLockWorker);

    InitializeCriticalSection(&_csWorker);
    DllAddRef();
}


//  CWorkerThread destructor

CWorkerThread::~CWorkerThread()
{
    ASSERT(0 == _cRefLockWorker);

    SetListenerWT(NULL);
    DeleteCriticalSection(&_csWorker);

    DllRelease();
}


BOOL CWorkerThread::PostWorkerMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL bRet = FALSE;

    _LockWorker();
    // Only if we are not being killed, and the worker window is valid we
    // post the message, we don't want to post to NULL window (desktop)
    if (!_fKillWorker && _hwndWorker)
        bRet = PostMessage(_hwndWorker, uMsg, wParam, lParam);
    _UnlockWorker();
    return bRet;
}

/*--------------------------------------------------------------------
Purpose: IUnknown::QueryInterface
*/
STDMETHODIMP CWorkerThread::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CWorkerThread, IARPWorker),
        { 0 },
    };

    return QISearch(this, (LPCQITAB)qit, riid, ppvObj);
}


STDMETHODIMP_(ULONG) CWorkerThread::AddRef()
{
    LONG cRef = InterlockedIncrement(&_cRef);
    TraceAddRef(CWorkerThread, cRef);
    return cRef;
}


STDMETHODIMP_(ULONG) CWorkerThread::Release()
{
    LONG cRef = InterlockedDecrement(&_cRef);
    TraceRelease(CWorkerThread, cRef);
    if (cRef)
        return cRef;

    delete this;
    return 0;
}

/*-------------------------------------------------------------------------
Purpose: Sets the event listener so the worker thread can fire
         events incrementally.
*/
HRESULT CWorkerThread::SetListenerWT(IWorkerEvent * pwe)
{
    _LockWorker();
    {
        // We have to protect _pwe because it can be accessed by this
        // thread and the main thread.

        // Don't AddRef the event listener or we'll have a circular
        // reference.
        _pwe = pwe;
    }
    _UnlockWorker();
    
    return S_OK;
}


/*-------------------------------------------------------------------------
Purpose: Start the thread
*/
HRESULT CWorkerThread::StartWT(int iPriority)
{
    DWORD thid;     // Not used but we have to pass something in

    // Create a hidden top-level window to post messages to from the
    // worker thread
    _hwndWorker = SHCreateWorkerWindow(_WorkerWndProcWrapper, NULL, 0, 0, NULL, this);

    if (_hwndWorker)
    {
        AddRef(); // AddRef myself, the background thread is responsible of releasing
        // this ref count

        // Kick off the worker thread to do the slow enumeration.
        _hthreadWorker = CreateThread(NULL, 0,
                                      (LPTHREAD_START_ROUTINE)_ThreadStartProcWrapper,
                                      (LPVOID)this, CREATE_SUSPENDED, &thid);

        if (_hthreadWorker)
        {
            // Demote the priority so it doesn't interfere with the
            // initial HTML databinding.
            SetThreadPriority(_hthreadWorker, iPriority);
            ResumeThread(_hthreadWorker);
        }
        else
        {
            // Release my refcount in case of failure
            Release();

            // If we can't create the background thread, don't bother with the window
            DestroyWindow(_hwndWorker);
            _hwndWorker = NULL;
        }
    }
    return (_hthreadWorker != NULL) ? S_OK : E_FAIL;
}


/*-------------------------------------------------------------------------
Purpose: Kills the worker thread if one is around
*/

HRESULT CWorkerThread::KillWT(void)
{
    MSG msg;

    // I should never call KillWT to kill myself
    ASSERT(_hthreadWorker != GetCurrentThread());
    
    TraceMsg(TF_TASKS, "[%x] Killing worker thread...", _dwThreadId);

    // Tell the worker thread to stop when it can
    // Do this inside the critical section because we don't want random messages
    // get posted to the worker window after this. 
    _LockWorker();
    _fKillWorker = TRUE;
    _UnlockWorker();
    
    // If we have no worker thread, nothing to do
    if (_hthreadWorker)
    {
        // Now wait for the worker to stop
        if (WaitForSingleObject(_hthreadWorker, 10000) == WAIT_TIMEOUT)
            TraceMsg(TF_ERROR, "[%x] Worker thread termination wait timed out!", _dwThreadId);
        else
            TraceMsg(TF_TASKS, "[%x] Worker thread wait exited cleanly", _dwThreadId);

        // Now that the thread is stopped, release our hold so all its memory can go away
        CloseHandle(_hthreadWorker);
        _hthreadWorker = NULL;
    }
    
    // Make sure that all messages to our worker HWND get processed
    if (_hwndWorker)
    {
        while (PeekMessage(&msg, _hwndWorker, 0, 0, PM_REMOVE))
            DispatchMessage(&msg);

        DestroyWindow(_hwndWorker);
        _hwndWorker = NULL;
    }
    SetListenerWT(NULL);

    return S_OK;
}



//--------------------------------------------------------------------
// Private methods



void CWorkerThread::_LockWorker(void)
{
    EnterCriticalSection(&_csWorker);
    DEBUG_CODE( _cRefLockWorker++; )
}

void CWorkerThread::_UnlockWorker(void)
{
    DEBUG_CODE( _cRefLockWorker--; )
    LeaveCriticalSection(&_csWorker);
}


/*-------------------------------------------------------------------------
Purpose: Static wndproc wrapper. Calls the real non-static WndProc.
*/
LRESULT
CALLBACK
CWorkerThread::_WorkerWndProcWrapper(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    if (uMsg == WM_DESTROY)
        SetWindowLong(hwnd, 0, 0);
    else
    {
        CWorkerThread * pWorker = (CWorkerThread*)GetWindowLong(hwnd, 0);
        if (pWorker)
            return pWorker->_WorkerWndProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}


/*-------------------------------------------------------------------------
Purpose: Used to fire events back to Trident since they can't be fired from
         the worker thread.
*/
LRESULT
CWorkerThread::_WorkerWndProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (uMsg)
    {
    case WORKERWIN_FIRE_ROW_READY:  
        // Posted by worker thread when async data is ready
        _LockWorker();
        {
            if (_pwe)
            {
                TraceMsg(TF_TASKS, "[%x] Firing event for row #%d", _dwThreadId, wParam);
                _pwe->FireOnDataReady((LONG)wParam);
            }
        }
        _UnlockWorker();
        return 0;
        
    case WORKERWIN_FIRE_FINISHED:   
        // Posted by worker thread when thread is finished enumerating
        // async data.
        _LockWorker();
        {
            if (_pwe)
            {
                TraceMsg(TF_TASKS, "[%x] Firing finished", _dwThreadId);
                _pwe->FireOnFinished();
            }
        }
        _UnlockWorker();
        return 0;

    case WORKERWIN_FIRE_DATASETCHANGED:
        // Posted by worker thread when matrix array finished enumerating
        _LockWorker();
        {
            if (_pwe)
            {
                TraceMsg(TF_TASKS, "[%x] Firing DatasetChanged", _dwThreadId);
                _pwe->FireOnDatasetChanged();
            }
        }
        _UnlockWorker();
        return 0;

    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


/*-------------------------------------------------------------------------
Purpose: Start and exit of worker thread to do slow app information
*/
DWORD
CALLBACK
CWorkerThread::_ThreadStartProcWrapper(
    LPVOID lpParam          // this pointer of object since wrapper is static
    )
{
    CWorkerThread * pwt = (CWorkerThread *)lpParam;

    pwt->_dwThreadId = GetCurrentThreadId();
    
    return pwt->_ThreadStartProc();
}


/*-------------------------------------------------------------------------
Purpose: Contains the code run on the worker thread where we get the slow
         information about applications
*/
DWORD
CWorkerThread::_ThreadStartProc()
{
    // Don't bother killing the worker window here, let the main thread take care
    // of the life time of the worker window.
    
    // Signal that we don't have a worker thread anymore. Prevents race
    // conditions.
    _fKillWorker = FALSE;

    TraceMsg(TF_TASKS, "[%x] Exiting worker thread", _dwThreadId);

    // Release the ref count to "this" object at end of thread, be it CMtxArray or CDataSrc  because we
    // AddRef()ed before this thread started. 
    Release();
    return 0;
}

#endif //DOWNLEVEL_PLATFORM
