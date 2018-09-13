// worker.h : Declaration of the CWorkerThread

#ifndef __WORKER_H_
#define __WORKER_H_

#include "iface.h"          // for IWorkerEvent


// CWorkerThread

class CWorkerThread : public IARPWorker
{
public:
    CWorkerThread();
    virtual ~CWorkerThread();
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef   (void);
    virtual STDMETHODIMP_(ULONG) Release  (void);
    virtual STDMETHODIMP QueryInterface   (REFIID riid, LPVOID * ppvObj);

    // *** IARPWorker ***
    virtual STDMETHODIMP KillWT (void);
    STDMETHOD(StartWT)          (int iPriority);
    STDMETHOD(SetListenerWT)    (IWorkerEvent * pwe);

    
    BOOL    IsKilled() { return _fKillWorker; };
    BOOL    PostWorkerMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    virtual DWORD   _ThreadStartProc();

protected:
    DWORD   _dwThreadId;

private:
    void    _LockWorker(void);
    void    _UnlockWorker(void);

    LONG    _cRef;
    
    CRITICAL_SECTION _csWorker;
    DEBUG_CODE( LONG _cRefLockWorker; )
    
    LONG    _cItems;
    IWorkerEvent * _pwe;
    
    BITBOOL _fKillWorker: 1;
    HANDLE  _hthreadWorker;
    HWND    _hwndWorker;

    LRESULT _WorkerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    static DWORD CALLBACK _ThreadStartProcWrapper(LPVOID lpParam);
    static LRESULT CALLBACK _WorkerWndProcWrapper(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

};

// Window messages from worker thread that cause actions on the main thread
#define WORKERWIN_FIRE_ROW_READY          (WM_USER + 0x0101)  // wParam is row #
#define WORKERWIN_FIRE_FINISHED           (WM_USER + 0X0102)
#define WORKERWIN_FIRE_DATASETCHANGED     (WM_USER + 0X0103)
#endif //__WORKER_H_
