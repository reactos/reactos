/*****************************************************************************\
    FILE: view.h

    DESCRIPTION:
        This is our ShellView which implements FTP specific behavior.  We get
    the default DefView implementation and then use IShellFolderViewCB to 
    override behavior specific to us.
\*****************************************************************************/

#ifndef _FTPVIEW_H
#define _FTPVIEW_H

#include "isfvcb.h"
#include "statusbr.h"
#include "msieftp.h"
#include "dspsprt.h"


CFtpView * GetCFtpViewFromDefViewSite(IUnknown * punkSite);
CStatusBar * GetCStatusBarFromDefViewSite(IUnknown * punkSite);
HRESULT FtpView_SetRedirectPidl(IUnknown * punkSite, LPCITEMIDLIST pidl);


class CFtpView
                : public CBaseFolderViewCB
                , public IFtpWebView
                , public CImpIDispatch
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void) {return CBaseFolderViewCB::AddRef();};
    virtual STDMETHODIMP_(ULONG) Release(void) {return CBaseFolderViewCB::Release();};

    // *** IDispatch methods ***
    virtual STDMETHODIMP GetTypeInfoCount(UINT * pctinfo);
    virtual STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo * * pptinfo);
    virtual STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR * * rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid);
    virtual STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);
    
    // *** IFtpWebView methods ***
    virtual STDMETHODIMP get_Server(BSTR * pbstr);
    virtual STDMETHODIMP get_Directory(BSTR * pbstr);
    virtual STDMETHODIMP get_UserName(BSTR * pbstr);
    virtual STDMETHODIMP get_PasswordLength(long * plLength);
    virtual STDMETHODIMP get_EmailAddress(BSTR * pbstr);
    virtual STDMETHODIMP put_EmailAddress(BSTR bstr);
    virtual STDMETHODIMP get_CurrentLoginAnonymous(VARIANT_BOOL * pfAnonymousLogin);
    virtual STDMETHODIMP get_MessageOfTheDay(BSTR * pbstr);
    virtual STDMETHODIMP LoginAnonymously(void);
    virtual STDMETHODIMP LoginWithPassword(BSTR bUserName, BSTR bPassword);
    virtual STDMETHODIMP LoginWithoutPassword(BSTR bUserName);
    virtual STDMETHODIMP InvokeHelp(void) {return _OnInvokeFtpHelp(m_hwndOwner);};

    // *** CFtpViewPriv methods ***
    BOOL IsForegroundThread(void);
    CStatusBar * GetStatusBar(void) { return m_psb; };
    HRESULT SetRedirectPidl(LPCITEMIDLIST pidlRedirect);

public:
    // Public Member Functions
    static HRESULT DummyHintCallback(HWND hwnd, CFtpFolder * pff, HINTERNET hint, LPVOID pv1, LPVOID pv2);

    // Friend Functions
    friend HRESULT CFtpView_Create(CFtpFolder * pff, HWND hwndOwner, REFIID riid, LPVOID * ppv);

protected:
    // Private Member Variables
    HWND                    m_hwndOwner;            // The owner window
    HWND                    m_hwndStatusBar;        // The Status Bar window
    CFtpFolder *            m_pff;                  // The owner Folder
    LPGLOBALTIMEOUTINFO     m_hgtiWelcome;          // The timeout for the welcome message
    CStatusBar *            m_psb;                  // The timeout for the welcome message
    HINSTANCE               m_hinstInetCpl;         // HANDLE to Internet Control panel for View.Options.
    RECT                    m_rcPrev;               // Previous size so we know when to ignore resizes.
    UINT                    m_idMergedMenus;        // Where did I start merging menus?
    UINT                    m_nMenuItemsAdded;      // How many menu items did I had?
    LPITEMIDLIST            m_pidlRedirect;         // We want to redirect to this pidl. See the comments in _OnBackGroundEnumDone().
    UINT                    m_nThreadID;            // What is the main thread?

    // Private Member Functions
    CFtpView(CFtpFolder * pff, HWND hwndOwner);
    ~CFtpView();

    void _InitStatusBar(void);
    void _ShowMotd(void);
    HRESULT _OnInvokeFtpHelp(HWND hwnd);
    HRESULT _LoginWithPassword(LPCTSTR pszUserName, LPCTSTR pszPassword);

    virtual HRESULT _OnWindowCreated(void);
    virtual HRESULT _OnDefItemCount(LPINT pi);
    virtual HRESULT _OnGetHelpText(LPARAM lParam, WPARAM wParam);
    virtual HRESULT _OnGetZone(DWORD * pdwZone, WPARAM wParam);
    virtual HRESULT _OnGetPane(DWORD dwPaneID, DWORD * pdwPane);
    virtual HRESULT _OnRefresh(BOOL fReload);
    virtual HRESULT _OnDidDragDrop(DROPEFFECT de, IDataObject * pdto);
    virtual HRESULT _OnGetDetailsOf(UINT ici, PDETAILSINFO pdi);
    virtual HRESULT _OnInvokeCommand(UINT idc);
    virtual HRESULT _OnMergeMenu(LPQCMINFO pqcm);
    virtual HRESULT _OnUnMergeMenu(HMENU hMenu);
    virtual HRESULT _OnColumnClick(UINT ici);
    virtual HRESULT _OnGetNotify(LPITEMIDLIST * ppidl, LONG * lEvents);
    virtual HRESULT _OnSize(LONG x, LONG y);
    virtual HRESULT _OnUpdateStatusBar(void);
    virtual HRESULT _OnThisIDList(LPITEMIDLIST * ppidl);
    virtual HRESULT _OnAddPropertyPages(SFVM_PROPPAGE_DATA * pData);
    virtual HRESULT _OnInitMenuPopup(HMENU hmenu, UINT idCmdFirst, UINT nIndex);
    virtual HRESULT _OnGetHelpTopic(SFVM_HELPTOPIC_DATA * phtd);
    virtual HRESULT _OnBackGroundEnumDone(void);

    HRESULT _OnInvokeLoginAs(HWND hwndOwner);
    HRESULT _OnInvokeNewFolder(HWND hwndOwner);

    void _ShowMotdPsf(HWND hwndOwner);

private:
    static INT_PTR CALLBACK _MOTDDialogProc(HWND hDlg, UINT wm, WPARAM wParam, LPARAM lParam);
};

#endif // _FTPVIEW_H
