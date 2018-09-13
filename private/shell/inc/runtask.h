#ifndef _RUNTASK_H_
#define _RUNTASK_H_

class CRunnableTask : public IRunnableTask
{
public:
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IRunnableTask ***
    virtual STDMETHODIMP Run(void);
    virtual STDMETHODIMP Kill(BOOL bWait);
    virtual STDMETHODIMP Suspend(void);
    virtual STDMETHODIMP Resume(void);
    virtual STDMETHODIMP_(ULONG) IsRunning(void);

    // *** pure virtuals ***
    virtual STDMETHODIMP RunInitRT(void) PURE;
    virtual STDMETHODIMP KillRT(BOOL bWait)     { return S_OK; };
    virtual STDMETHODIMP SuspendRT(void)        { return S_OK; };
    virtual STDMETHODIMP ResumeRT(void)         { return InternalResumeRT(); };
    virtual STDMETHODIMP InternalResumeRT(void) { _lState = IRTIR_TASK_FINISHED; return S_OK; };
    
protected:
    CRunnableTask(DWORD dwFlags);
    virtual ~CRunnableTask();
    
    LONG            _cRef;
    LONG            _lState;
    DWORD           _dwFlags;       // RTF_*
    HANDLE          _hDone;

#ifdef DEBUG
    DWORD           _dwTaskID;
#endif
};

// CRunnableTask flags
#define RTF_DEFAULT             0x00000000
#define RTF_SUPPORTKILLSUSPEND  0x00000001

#endif  // _RUNTASK_H_

