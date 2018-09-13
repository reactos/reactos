/*****************************************************************************\
    FILE: progres.h

    DESCRIPTION:
        Display the Progress Dialog for the progress on the completion of some
    generic operation.  This is most often used for Deleting, Uploading, Copying,
    Moving and Downloading large numbers of files.
\*****************************************************************************/

#ifndef _PROGRESS_H
#define _PROGRESS_H


// this is how long we wait for the UI thread to create the progress hwnd before giving up
#define WAIT_PROGRESS_HWND 10*1000 // ten seconds


STDAPI CProgressDialog_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

/*****************************************************************************\
    Class: CProgressDialog

    DESCRIPTION:
        Display the Progress Dialog for the progress on the completion of some
    generic operation.  This is most often used for Deleting, Uploading, Copying,
    Moving and Downloading large numbers of files.
\*****************************************************************************/

class CProgressDialog 
                : public IProgressDialog
                , public IOleWindow
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IProgressDialog methods ***
    virtual STDMETHODIMP StartProgressDialog(HWND hwndParent, IUnknown * punkEnableModless, DWORD dwFlags, LPCVOID pvResevered);
    virtual STDMETHODIMP StopProgressDialog(void);
    virtual STDMETHODIMP SetTitle(LPCWSTR pwzTitle);
    virtual STDMETHODIMP SetAnimation(HINSTANCE hInstAnimation, UINT idAnimation);
    virtual STDMETHODIMP_(BOOL) HasUserCancelled(void);
    virtual STDMETHODIMP SetProgress(DWORD dwCompleted, DWORD dwTotal);
    virtual STDMETHODIMP SetProgress64(ULONGLONG ullCompleted, ULONGLONG ullTotal);
    virtual STDMETHODIMP SetLine(DWORD dwLineNum, LPCWSTR pwzString, BOOL fCompactPath, LPCVOID pvResevered);
    virtual STDMETHODIMP SetCancelMsg(LPCWSTR pwzCancelMsg, LPCVOID pvResevered);
    virtual STDMETHODIMP Timer(DWORD dwAction, LPCVOID pvResevered);

    // *** IOleWindow methods ***
    virtual STDMETHODIMP GetWindow(HWND * phwnd);
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return E_NOTIMPL; }
    

    // Other Public Methods
    static INT_PTR CALLBACK ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    static DWORD CALLBACK ProgressUIThreadProc(LPVOID pvThis) { return ((CProgressDialog *) pvThis)->_ProgressUIThreadProc(); };

    friend HRESULT CProgressDialog_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

protected:
    CProgressDialog();

private:
    // Private Member Variables
    ~CProgressDialog(void);
    DWORD       _cRef;

    // State Accessible thru IProgressDialog
    LPWSTR      _pwzTitle;                  // This will be used to cache the value passed to IProgressDialog::SetTitle() until the dialog is displayed
    UINT        _idAnimation;
    HINSTANCE   _hInstAnimation;
    LPWSTR      _pwzLine1;                  // NOTE:
    LPWSTR      _pwzLine2;                  // these are only used to init the dialog, otherwise, we just
    LPWSTR      _pwzLine3;                  // call through on the main thread to update the dialog directly.
    LPWSTR      _pwzCancelMsg;              // If the user cancels, Line 1 & 2 will be cleared and Line 3 will get this msg.

    // Other internal state.
    HANDLE      _hCreatedHwnd;              // we wait on this event to make sure the hwnd has been set by the UI thread
    DWORD       _dwFlags;                   // flags specifying progress dlg styles
    HWND        _hwndDlgParent;             // parent window for message boxes
    HANDLE      _hThread;                   // handle of the UI thread
    HWND        _hwndProgress;              // dialog/progress window
    DWORD       _dwFirstShowTime;           // tick count when the dialog was first shown (needed so we don't flash it up for an instant)

    BOOL        _fCompletedChanged;         // has the _dwCompleted changed since last time?
    BOOL        _fTotalChanged;             // has the _dwTotal changed since last time?
    BOOL        _fChangePosted;             // is there a change pending?
    BOOL        _fCancel;
    BOOL        _fDone;
    BOOL        _fWorking;
    BOOL        _fMinimized;
    BOOL        _fScaleBug;                 // Comctl32's PBM_SETRANGE32 msg will still cast it to an (int), so don't let the high bit be set.

    // Progress Values and Calculations
    DWORD       _dwCompleted;               // progress completed
    DWORD       _dwTotal;                   // total progress
    DWORD       _dwPrevRate;                // previous progress rate (used for computing time remaining)
    DWORD       _dwPrevTickCount;           // the tick count when we last updated the progress time
    DWORD       _dwPrevCompleted;           // the ammount we had completed when we last updated the progress time
    DWORD       _dwLastUpdatedTimeRemaining;// tick count when we last update the "Time remaining" field, we only update it every 5 seconds
    DWORD       _dwLastUpdatedTickCount;    // tick count when SetProgress was last called, used to calculate the rate
    UINT        _iNumTimesSetProgressCalled;// how many times has the user called SetProgress?

    // Private Member Functions
    DWORD _ProgressUIThreadProc(void);
    BOOL _OnInit(HWND hDlg);
    BOOL _ProgressDialogProc(HWND hDlg, UINT wMsg, WPARAM wParam, LPARAM lParam);
    void _PauseAnimation(BOOL bStop);
    void _UpdateProgressDialog(void);
    void _AsyncUpdate(void);
    HRESULT _SetProgressTime(void);
    void _SetProgressTimeEst(DWORD dwSecondsLeft);
    void _UserCancelled(void);
    HRESULT _DisplayDialog(void);
    HRESULT _CompactProgressPath(LPCWSTR pwzStrIn, BOOL fCompactPath, UINT idDlgItem, LPCVOID pvResevered, LPWSTR pwzStrOut, DWORD cchSize);
    HRESULT _SetLineHelper(LPCWSTR pwzNew, LPWSTR * ppwzDest, UINT idDlgItem, BOOL fCompactPath, LPCVOID pvResevered);
    HRESULT _SetTitleBarProgress(DWORD dwCompleted, DWORD dwTotal);
};

#endif // _PROGRESS_H
