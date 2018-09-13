/*****************************************************************************
 *    ftpsite.h
 *****************************************************************************/

#ifndef _FTPSITE_H
#define _FTPSITE_H

#include "ftpfoldr.h"
#include "ftplist.h"
#include "ftpinet.h"
#include "ftpurl.h"
#include "account.h"
#include "util.h"

HRESULT SiteCache_PidlLookup(LPCITEMIDLIST pidl, BOOL fPasswordRedir, IMalloc * pm, CFtpSite ** ppfs);


int CALLBACK _CompareSites(LPVOID pvStrSite, LPVOID pvFtpSite, LPARAM lParam);
HRESULT CFtpPunkList_Purge(CFtpList ** pfl);

/*****************************************************************************
 *    CFtpSite
 *****************************************************************************/

class CFtpSite              : public IUnknown
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
    CFtpSite();
    ~CFtpSite();

    // Public Member Functions
    void CollectMotd(HINTERNET hint);
    void ReleaseHint(LPCITEMIDLIST pidlFtpPath, HINTERNET hint);
    HRESULT GetHint(HWND hwnd, LPCITEMIDLIST pidlFtpPath, CStatusBar * psb, HINTERNET * phint, IUnknown * punkSite, CFtpFolder * pff);
    BOOL QueryMotd(void);
    BOOL IsServerVMS(void) {return m_fIsServerVMS;};
    BOOL HasVirtualRoot(void);
    CFtpGlob * GetMotd(void);
    CFtpList * GetCFtpList(void);
    CWireEncoding * GetCWireEncoding(void) {return &m_cwe;};
    HRESULT GetFtpDir(LPCITEMIDLIST pidl, CFtpDir ** ppfd);
    HRESULT GetFtpDir(LPCTSTR pszUrlPath, CFtpDir ** ppfd) {return GetFtpDir(m_pszServer, pszUrlPath, ppfd);};
    HRESULT GetFtpDir(LPCTSTR pszServer, LPCTSTR pszUrlPath, CFtpDir ** ppfd);

    HRESULT GetVirtualRoot(LPITEMIDLIST * ppidl);
    HRESULT PidlInsertVirtualRoot(LPCITEMIDLIST pidlFtpPath, LPITEMIDLIST * ppidl);
    LPCITEMIDLIST GetVirtualRootReference(void) {return (LPCITEMIDLIST) m_pidlVirtualDir;};

    HRESULT GetServer(LPTSTR pszServer, DWORD cchSize) { StrCpyN(pszServer, HANDLE_NULLSTR(m_pszServer), cchSize); return S_OK; };
    HRESULT GetUser(LPTSTR pszUser, DWORD cchSize) { StrCpyN(pszUser, HANDLE_NULLSTR(m_pszUser), cchSize); return S_OK; };
    HRESULT GetPassword(LPTSTR pszPassword, DWORD cchSize) { StrCpyN(pszPassword, HANDLE_NULLSTR(m_pszPassword), cchSize); return S_OK; };
    HRESULT UpdateHiddenPassword(LPITEMIDLIST pidl);
    HRESULT SetRedirPassword(LPCTSTR pszPassword) {Str_SetPtr(&m_pszRedirPassword, pszPassword); return S_OK;};
    HRESULT FlushSubDirs(LPCITEMIDLIST pidl);

    LPITEMIDLIST GetPidl(void);

    BOOL IsCHMODSupported(void) {return m_fIsCHMODSupported;};
    BOOL IsUTF8Supported(void) {return m_fInUTF8Mode;};
    BOOL IsSiteBlockedByRatings(HWND hwndDialogOwner);
    void FlushRatingsInfo(void) {m_fRatingsChecked = m_fRatingsAllow = FALSE;};

    static void FlushHintCB(LPVOID pvFtpSite);


    // Friend Functions
    friend HRESULT CFtpSite_Create(LPCITEMIDLIST pidl, LPCTSTR pszLookupStr, IMalloc * pm, CFtpSite ** ppfs);
    friend HRESULT SiteCache_PidlLookup(LPCITEMIDLIST pidl, BOOL fPasswordRedir, IMalloc * pm, CFtpSite ** ppfs);

    friend int CALLBACK _CompareSites(LPVOID pvStrSite, LPVOID pvFtpSite, LPARAM lParam);
    friend class CFtpView;


protected:
    // Private Member Variables
    int m_cRef;

    BOOL            m_fMotd;            // There is a Motd at all
    BOOL            m_fNewMotd;         // Motd has changed
    HINTERNET       m_hint;             // Session for this site
    LPGLOBALTIMEOUTINFO m_hgti;         // Timeout for the session handle
    CFtpList *      m_FtpDirList;       // List of FtpDir's attached to me. (No Ref Held)
    CFtpGlob *      m_pfgMotd;          //
    IMalloc *       m_pm;               // Used for creating full pidls if needed.

    LPTSTR          m_pszServer;        // Server name
    LPITEMIDLIST    m_pidl;             // What ftp dir is hint in? (Not including the virtual root) (Does begin with ServerID)
    LPTSTR          m_pszUser;          // 0 or "" means "anonymous"
    LPTSTR          m_pszPassword;      // User's Password
    LPTSTR          m_pszFragment;      // URL fragment
    LPITEMIDLIST    m_pidlVirtualDir;   // Our rooted directory on the server.
    LPTSTR          m_pszRedirPassword; // What was the password if it was changed?
    LPTSTR          m_pszLookupStr;     // Str to lookup.
    INTERNET_PORT   m_ipPortNum;        // The port number
    BOOL            m_fDLTypeSpecified; // Did the user specify a Download Type to use? (ASCII vs. Binary)
    BOOL            m_fASCIIDownload;   // If specified, was it ASCII? (Else, Binary)
    CAccounts       m_cAccount;
    BOOL            m_fRatingsChecked;  // Did I check ratings yet?
    BOOL            m_fRatingsAllow;    // Does ratings allow access to this site?
    BOOL            m_fFeaturesQueried; // 
    BOOL            m_fInUTF8Mode;      // Did a success value come back from the 'UTF8' command?
    BOOL            m_fIsCHMODSupported;// Is the CHMOD UNIX command supported via the 'SITE CHMOD' FTP Command?
    BOOL            m_fIsServerVMS;     // Is this a VMS server?

    CWireEncoding   m_cwe;              // What codepage and confidence in that codepage of the MOTD and filenames?

    // Protected Member Functions
    HRESULT _RedirectAndUpdate(LPCTSTR pszServer, INTERNET_PORT ipPortNum, LPCTSTR pszUser, LPCTSTR pszPassword, LPCITEMIDLIST pidlFtpPath, LPCTSTR pszFragment, IUnknown * punkSite, CFtpFolder * pff);
    HRESULT _Redirect(LPITEMIDLIST pidl, IUnknown * punkSite, CFtpFolder * pff);
    HRESULT _SetDirectory(HINTERNET hint, HWND hwnd, LPCITEMIDLIST pidlNewDir, CStatusBar * psb, int * pnTriesLeft);

private:
    // Private Member Functions
    HRESULT _SetPidl(LPCITEMIDLIST pidlFtpPath);
    HRESULT _QueryServerFeatures(HINTERNET hint);
    HRESULT _CheckToEnableCHMOD(LPCWIRESTR pwResponse);
    HRESULT _LoginToTheServer(HWND hwnd, HINTERNET hintDll, HINTERNET * phint, LPCITEMIDLIST pidlFtpPath, CStatusBar * psb, IUnknown * punkSite, CFtpFolder * pff);
    HRESULT _SetRedirPassword(LPCTSTR pszServer, INTERNET_PORT ipPortNum, LPCTSTR pszUser, LPCTSTR pszPassword, LPCITEMIDLIST pidlFtpPath, LPCTSTR pszFragment);

    void FlushHint(void);
    void FlushHintCritial(void);

    // Private Friend Functions
    friend HRESULT SiteCache_PrivSearch(LPCTSTR pszLookup, LPCITEMIDLIST pidl, IMalloc * pm, CFtpSite ** ppfs);
};



HRESULT CFtpSite_Init(void);
HRESULT CFtpSite_Create(LPCITEMIDLIST pidl, LPCTSTR pszLookupStr, IMalloc * pm, CFtpSite ** ppfs);


#endif // _FTPSITE_H
