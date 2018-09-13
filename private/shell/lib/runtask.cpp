#include "proj.h"

#include "runtask.h"

#define SUPERCLASS  

// #define TF_RUNTASK  TF_GENERAL
#define TF_RUNTASK  0
// #define TF_RUNTASKV TF_CUSTOM1     // verbose version
#define TF_RUNTASKV 0


// constructor
CRunnableTask::CRunnableTask(DWORD dwFlags)
{
    _lState = IRTIR_TASK_NOT_RUNNING;
    _dwFlags = dwFlags;

    ASSERT(NULL == _hDone);
    
    if (_dwFlags & RTF_SUPPORTKILLSUSPEND)
    {
        // we signal this on suspend or kill
        // Explicitly call the ANSI version so we don't need to worry
        // about whether we're being built UNICODE and have to switch
        // to a wrapper function...
        _hDone = CreateEventA(NULL, TRUE, FALSE, NULL);
    }

#ifdef DEBUG
    _dwTaskID = GetTickCount();

    TraceMsg(TF_RUNTASK, "CRunnableTask (%#lx): creating task", _dwTaskID);
#endif

    _cRef = 1;
}


// destructor
CRunnableTask::~CRunnableTask()
{
    DEBUG_CODE( TraceMsg(TF_RUNTASK, "CRunnableTask (%#lx): deleting task", _dwTaskID); )

    if (_hDone)
        CloseHandle(_hDone);
}


STDMETHODIMP CRunnableTask::QueryInterface( REFIID riid, LPVOID * ppvObj )
{
    if ( ppvObj == NULL )
    {
        return E_INVALIDARG;
    }
    if ( riid == IID_IRunnableTask )
    {
        *ppvObj = SAFECAST( this, IRunnableTask *);
        AddRef();
    }
    else
        return E_NOINTERFACE;


    return NOERROR;
}


STDMETHODIMP_(ULONG) CRunnableTask::AddRef()
{
    InterlockedIncrement(&_cRef);
    return _cRef;
}


STDMETHODIMP_ (ULONG) CRunnableTask::Release()
{
    if (0 == _cRef)
    {
        AssertMsg(0, TEXT("CRunnableTask::Release called too many times!"));
        return 0;
    }
    
    if ( InterlockedDecrement(&_cRef) == 0)
    {
        delete this;
        return 0;
    }
    return _cRef;
}


/*----------------------------------------------------------
Purpose: IRunnableTask::Run method

         This does a lot of the state-related work, and then
         calls the derived-class's RunRT() method.
         
*/
STDMETHODIMP CRunnableTask::Run(void)
{
    HRESULT hr = E_FAIL;

    // Are we already running?
    if (_lState == IRTIR_TASK_RUNNING)
    {
        // Yes; nothing to do 
        hr = S_FALSE;
    }
    else if ( _lState == IRTIR_TASK_PENDING )
    {
        hr = E_FAIL;
    }
    else if ( _lState == IRTIR_TASK_NOT_RUNNING )
    {
        // Say we're running
        LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_RUNNING);
        if ( lRes == IRTIR_TASK_PENDING )
        {
            _lState = IRTIR_TASK_FINISHED;
            return NOERROR;
        }

        if (_lState == IRTIR_TASK_RUNNING)
        {
            // Prepare to run 
            DEBUG_CODE( TraceMsg(TF_RUNTASKV, "CRunnableTask (%#lx): initialize to run", _dwTaskID); )
            
            hr = RunInitRT();
            
            ASSERT(E_PENDING != hr);
        }

        if (SUCCEEDED(hr) && _lState == IRTIR_TASK_RUNNING)
        {
            // Continue to do the work
            hr = InternalResumeRT();
        }

        if (FAILED(hr) && E_PENDING != hr)
        {
            DEBUG_CODE( TraceMsg(TF_WARNING, "CRunnableTask (%#lx): task failed to run: %#lx", _dwTaskID, hr); )
        }            

        // Are we finished?
        if (_lState != IRTIR_TASK_SUSPENDED || hr != E_PENDING)
        {
            // Yes
            _lState = IRTIR_TASK_FINISHED;
        }
    }
    
    return hr;
}


/*----------------------------------------------------------
Purpose: IRunnableTask::Kill method

*/
STDMETHODIMP CRunnableTask::Kill(BOOL fWait)
{
    if ( !(_dwFlags & RTF_SUPPORTKILLSUSPEND) )
        return E_NOTIMPL;
        
    if (_lState != IRTIR_TASK_RUNNING)
        return S_FALSE;

    DEBUG_CODE( TraceMsg(TF_RUNTASKV, "CRunnableTask (%#lx): killing task", _dwTaskID); )

    LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_PENDING);
    if (lRes == IRTIR_TASK_FINISHED)
    {
        DEBUG_CODE( TraceMsg(TF_RUNTASKV, "CRunnableTask (%#lx): task already finished", _dwTaskID); )

        _lState = lRes;
    }
    else if (_hDone)
    {
        // signal the event it is likely to be waiting on
        SetEvent(_hDone);
    }

    return KillRT(fWait);
}


/*----------------------------------------------------------
Purpose: IRunnableTask::Suspend method

*/
STDMETHODIMP CRunnableTask::Suspend( void )
{
    if ( !(_dwFlags & RTF_SUPPORTKILLSUSPEND) )
        return E_NOTIMPL;
        
    if (_lState != IRTIR_TASK_RUNNING)
        return E_FAIL;
    
    DEBUG_CODE( TraceMsg(TF_RUNTASKV, "CRunnableTask (%#lx): suspending task", _dwTaskID); )
    
    LONG lRes = InterlockedExchange(&_lState, IRTIR_TASK_SUSPENDED);

    if (IRTIR_TASK_FINISHED == lRes)
    {
        // we finished before we could suspend
        DEBUG_CODE( TraceMsg(TF_RUNTASKV, "CRunnableTask (%#lx): task already finished", _dwTaskID); )
        
        _lState = lRes;
        return NOERROR;
    }

    if (_hDone)
        SetEvent(_hDone);

    return SuspendRT();
}


/*----------------------------------------------------------
Purpose: IRunnableTask::Resume method

*/
STDMETHODIMP CRunnableTask::Resume(void)
{
    if (_lState != IRTIR_TASK_SUSPENDED)
        return E_FAIL;

    DEBUG_CODE( TraceMsg(TF_RUNTASKV, "CRunnableTask (%#lx): resuming task", _dwTaskID); )

    _lState = IRTIR_TASK_RUNNING;

    return ResumeRT();
}


/*----------------------------------------------------------
Purpose: IRunnableTask::IsRunning method

*/
STDMETHODIMP_( ULONG ) CRunnableTask:: IsRunning ( void )
{
    return _lState;
}
