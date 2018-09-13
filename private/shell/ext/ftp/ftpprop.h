/*****************************************************************************
 *	ftpprop.h
 *****************************************************************************/

#ifndef _FTPPROP_H
#define _FTPPROP_H

#include "ftpdhlp.h"
#include "ftppl.h"

HRESULT CFtpProp_DoProp(CFtpPidlList * pflHfpl, CFtpFolder * pff, HWND hwnd);


// BUGBUG/TODO: Add Change UNIX Permissions feature
// This is done by sending the command "SITE CHMOD <permissions> <filename>"
// We can probably test permissions by using CHMOD or something to test it.

/*****************************************************************************
 *
 *	CFtpProp
 *
 *	NOTE! that we use the IShellFolder on the wrong thread!
 *
 *	Take careful note that all the CFtpFolder methods we call
 *	are thread-safe.
 *
 *****************************************************************************/

class CFtpProp          : public IUnknown
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
public:
    CFtpProp();
    ~CFtpProp(void);

    // Friend Functions
    friend HRESULT CFtpProp_Create(CFtpPidlList * pflHfpl, CFtpFolder * pff, HWND hwnd, CFtpProp ** ppfp);
    static HRESULT _CommitCHMOD_CB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pv, BOOL * pfReleaseHint);
    static DWORD _PropertySheetThreadProc(LPVOID pvCFtpProp) {return ((CFtpProp *) pvCFtpProp)->_PropertySheetThread(); };

protected:
    // Public Member Variables
    int                     m_cRef;

    CFtpFolder *            m_pff;          // Folder that owns the pidls
    CFtpPidlList *          m_pflHfpl;      // 
    HWND                    m_hwnd;         // browser window for UI and Legacy ChangeNotify
    CFtpDialogTemplate      m_ftpDialogTemplate;
    BOOL                    m_fChangeModeSupported;
    DWORD                   m_dwNewPermissions;

    
    // Public Member Functions
    BOOL OnInitDialog(HWND hdlg);
    BOOL OnClose(HWND hDlg);
    DWORD _PropertySheetThread(void);
    void _HideCHMOD_UI(HWND hDlg);
    HRESULT _SetCHMOD_UI(HWND hDlg);
    DWORD _GetCHMOD_UI(HWND hDlg);
    DWORD _GetUnixPermissions(void);
    HRESULT _CommitCHMOD(HINTERNET hint, HINTPROCINFO * phpi, BOOL * pfReleaseHint);
    INT_PTR _SetWhiteBGCtlColor(HWND hDlg, HDC hdc, HWND hwndCtl);

    static INT_PTR DlgProc(HWND hdlg, UINT wm, WPARAM wp, LPARAM lp);
};

DWORD DoProp_OnThread(LPVOID pv);


#endif // _FTPPROP_H
