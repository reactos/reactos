/*****************************************************************************
 *	ftpdir.h
 *****************************************************************************/

#ifndef _FTPDIR_H
#define _FTPDIR_H

#include "ftpsite.h"
#include "ftpfoldr.h"
#include "ftplist.h"
#include "ftpglob.h"
#include "ftppl.h"


typedef struct tagSETNAMEOFINFO
{
    LPCITEMIDLIST pidlOld;
    LPCITEMIDLIST pidlNew;
} SETNAMEOFINFO, * LPSETNAMEOFINFO;

int CALLBACK _CompareDirs(LPVOID pvPidl, LPVOID pvFtpDir, LPARAM lParam);

/*****************************************************************************\
    CLASS: CFtpDir

    DESCRIPTION:
        This class is the cache of a directory on some server.  m_pfs identifies
    the server.

    BUGBUG: PERF - PERF - PERF - PERF
        This directory contains the folder contents in the form of a list of
    pidls (m_pflHfpl).  We need to keep them in order based on name so that
    way looking up and changing is fast because of all the work we need to do
    with change notify.  Also, when we go to parse a display name, we look here
    first, so that needs to be fast.
\*****************************************************************************/

class CFtpDir           : public IUnknown
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
    CFtpDir();
    ~CFtpDir(void);

    // Public Member Functions
    void CollectMotd(HINTERNET hint);
    void SetCache(CFtpPidlList * pflHfpl);
    CFtpPidlList * GetHfpl(void);
    HRESULT GetHint(HWND hwnd, CStatusBar * psb, HINTERNET * phint, IUnknown * punkSite, CFtpFolder * pff);
    void ReleaseHint(HINTERNET hint);
    STDMETHODIMP WithHint(CStatusBar * psb, HWND hwnd, HINTPROC hp, LPCVOID pv, IUnknown * punkSite, CFtpFolder * pff);
    HRESULT SetNameOf(CFtpFolder * pff, HWND hwndOwner, LPCITEMIDLIST pidl, LPCTSTR pszName, DWORD dwReserved, LPITEMIDLIST *ppidlOut);
    BOOL IsRoot(void);
    BOOL IsCHMODSupported(void) {return m_pfs->IsCHMODSupported();};
    BOOL IsUTF8Supported(void) {return m_pfs->IsUTF8Supported();};
    HRESULT GetFindDataForDisplayPath(HWND hwnd, LPCWSTR pwzDisplayPath, LPFTP_FIND_DATA pwfd, CFtpFolder * pff);
    HRESULT GetFindData(HWND hwnd, LPCWIRESTR pwWireName, LPFTP_FIND_DATA pwfd, CFtpFolder * pff);
    HRESULT GetNameOf(LPCITEMIDLIST pidl, DWORD shgno, LPSTRRET pstr);
//    HRESULT DisambiguatePidl(LPCITEMIDLIST pidl);
    CFtpSite * GetFtpSite(void);
    CFtpDir * GetSubFtpDir(CFtpFolder * pff, LPCITEMIDLIST pidl, BOOL fPublic);
    HRESULT GetDisplayPath(LPTSTR pszUrlPath, DWORD cchSize);

    LPCITEMIDLIST GetPathPidlReference(void) { return m_pidlFtpDir;};
    LPCITEMIDLIST GetPidlReference(void) { return m_pidl;};
    LPCITEMIDLIST GetPidlFromWireName(LPCWIRESTR pwWireName);
    LPCITEMIDLIST GetPidlFromDisplayName(LPCWSTR pwzDisplayName);
    LPITEMIDLIST GetSubPidl(CFtpFolder * pff, LPCITEMIDLIST pidlRelative, BOOL fPublic);
    HRESULT AddItem(LPCITEMIDLIST pidl);
    HRESULT ChangeFolderName(LPCITEMIDLIST pidlFtpPath);
    HRESULT ReplacePidl(LPCITEMIDLIST pidlSrc, LPCITEMIDLIST pidlDest) { if (!m_pflHfpl) return S_OK; return m_pflHfpl->ReplacePidl(pidlSrc, pidlDest); };
    HRESULT DeletePidl(LPCITEMIDLIST pidl) { if (!m_pflHfpl) return S_OK; return m_pflHfpl->CompareAndDeletePidl(pidl); };

    static HRESULT _SetNameOfCB(HINTERNET hint, HINTPROCINFO * phpi, LPVOID pv, BOOL * pfReleaseHint);
    static HRESULT _GetFindData(HINTERNET hint0, HINTPROCINFO * phpi, LPVOID pv, BOOL * pfReleaseHint);


    // Friend Functions
    friend HRESULT CFtpDir_Create(CFtpSite * pfs, LPCITEMIDLIST pidl, CFtpDir ** ppfd);

    friend int CALLBACK _CompareDirs(LPVOID pvPidl, LPVOID pvFtpDir, LPARAM lParam);


protected:
    int                     m_cRef;

    CFtpSite *              m_pfs;          // The FTP site I belong to. (WARNING: No Ref Held)
    CFtpPidlList *          m_pflHfpl;      // The items inside this directory
    CFtpGlob *              m_pfgMotd;      // The message of the day
    LPITEMIDLIST            m_pidlFtpDir;   // Name of subdirectory w/o Virtual Root and decoded. Doesn't include Server ID
    LPITEMIDLIST            m_pidl;         // Where we live.  May include the virtual root

    BOOL _DoesItemExist(HWND hwnd, CFtpFolder * pff, LPCITEMIDLIST pidl);
    BOOL _ConfirmReplaceWithRename(HWND hwnd);
    HRESULT _SetFtpDir(CFtpSite * pfs, CFtpDir * pfd, LPCITEMIDLIST pidl);
};

#endif // _FTPDIR_H
