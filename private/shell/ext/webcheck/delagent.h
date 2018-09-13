//
// Delivery Agents base class

#ifndef _DELAGENT_H
#define _DELAGENT_H

#include "offline.h"

#define INET_S_AGENT_BASIC_SUCCESS           _HRESULT_TYPEDEF_(0x000C0FFEL)

class CDeliveryAgent :  public ISubscriptionAgentControl,
                        public IShellPropSheetExt,
#ifdef UNICODE
                        public IExtractIconA,
#endif
                        public IExtractIcon,
                        public ISubscriptionAgentShellExt
{
private:
// Data for our OLE support
    ULONG           m_cRef;

#ifdef AGENT_AUTODIAL
    enum DIALER_STATUS { DIALER_OFFLINE, DIALER_CONNECTING, DIALER_ONLINE };
    DIALER_STATUS   m_iDialerStatus;
#endif

    enum {
        FLAG_BUSY          =0x00010000,   // addrefed ourselves; between begin & end reports
        FLAG_PAUSED        =0x00020000,   // We are paused
        FLAG_OPSTARTED     =0x00040000,   // We've entered StartOperation
    };

    // Derived agents can use high 8 bits of this field
    DWORD       m_dwAgentFlags;

    void        SendUpdateBegin();
    void        SendUpdateEnd();
    HRESULT     ProcessEndItem(ISubscriptionItem *pEndItem);


protected:
    // Upper 16 bits allowable here
    enum    {
        FLAG_HOSTED        =0x00100000,     // hosted by another delivery agent
        FLAG_CHANGESONLY   =0x00200000,     // We're in "Changes Only" mode
        FLAG_WAITING_FOR_INCREASED_CACHE = 0x00400000, // Special paused state
    };

    POOEBuf         m_pBuf;
    HPROPSHEETPAGE  m_hPage[MAX_WC_AGENT_PAGES];

    ISubscriptionAgentEvents *m_pAgentEvents;
    ISubscriptionItem        *m_pSubscriptionItem;
    
    SUBSCRIPTIONCOOKIE      m_SubscriptionCookie;

    long        m_lSizeDownloadedKB;    // Size downloaded in KB

    SCODE       m_scEndStatus;

    void        SendUpdateNone();   // Call from StartOperation if we won't be doing anything

    void        SendUpdateProgress(LPCWSTR pwszURL, long lProgress, long lMax, long lCurSizeKB=-1);

    BOOL        IsAgentFlagSet(int iFlag) { return (m_dwAgentFlags & iFlag); }
    void        ClearAgentFlag(int iFlag) { m_dwAgentFlags &= ~iFlag; }
    void        SetAgentFlag(int iFlag) { m_dwAgentFlags |= iFlag; }
    
    HRESULT     CheckResponseCode(DWORD dwHttpResponseCode);    // Also sets EndStatus. E_ABORT on error.

//  DIALER_STATUS GetDialerStatus() { return m_iDialerStatus; }
    void          SetEndStatus(SCODE sc) { m_scEndStatus = sc; }

    virtual ~CDeliveryAgent();

public:
    CDeliveryAgent();

    BOOL        GetBusy() { return IsAgentFlagSet(FLAG_BUSY); }
    BOOL        IsPaused() { return IsAgentFlagSet(FLAG_PAUSED); }

    SCODE       GetEndStatus() { return m_scEndStatus; }

    // IUnknown members
    STDMETHODIMP         QueryInterface(REFIID riid, void **punk);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ISubscriptionAgentControl members
    STDMETHODIMP    StartUpdate(IUnknown *pItem, IUnknown *punkAdvise);
    STDMETHODIMP    PauseUpdate(DWORD dwFlags);
    STDMETHODIMP    ResumeUpdate(DWORD dwFlags);
    STDMETHODIMP    AbortUpdate(DWORD dwFlags);
    STDMETHODIMP    SubscriptionControl(IUnknown *pItem, DWORD dwControl);   // Called on delete

    // IShellPropSheetExt members
    STDMETHODIMP    AddPages(LPFNADDPROPSHEETPAGE, LPARAM);
    STDMETHODIMP    ReplacePage(UINT, LPFNADDPROPSHEETPAGE, LPARAM);

    // ISubscriptionAgentShellExt
    STDMETHODIMP    Initialize(SUBSCRIPTIONCOOKIE *pSubscriptionCookie, 
                               LPCWSTR pwszURL, LPCWSTR pwszName, 
                               SUBSCRIPTIONTYPE subsType);
    STDMETHODIMP    RemovePages(HWND hdlg);
    STDMETHODIMP    SaveSubscription();
    STDMETHODIMP    URLChange(LPCWSTR pwszNewURL);

#ifdef UNICODE
    //  IExtractIconA
    STDMETHODIMP    GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags);
    STDMETHODIMP    Extract(LPCSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);
#endif

    //  IExtractIconT
    STDMETHODIMP    GetIconLocation(UINT uFlags, LPTSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags);
    STDMETHODIMP    Extract(LPCTSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize);

private:
    // Functions we provide common implementations for
    HRESULT DoStartDownload();

#ifdef AGENT_AUTODIAL
    HRESULT NotifyAutoDialer();

    HRESULT OnInetOnline();
    HRESULT OnInetOffline();
#endif

protected:
    // Virtual functions for our derived classes to override as necessary
    // We provide implementations which should be called after processing
    virtual HRESULT     AgentPause(DWORD dwFlags);
    virtual HRESULT     AgentResume(DWORD dwFlags);
    virtual HRESULT     AgentAbort(DWORD dwFlags);

    virtual HRESULT     ModifyUpdateEnd(ISubscriptionItem *pEndItem, UINT *puiRes);

    virtual HRESULT     StartOperation();       // connects to internet
    virtual HRESULT     StartDownload() = 0;    // we just got connected
    virtual void        CleanUp();
};

#endif // _DELAGENT_H
