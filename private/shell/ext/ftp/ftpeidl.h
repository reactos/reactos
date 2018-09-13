/*****************************************************************************
 *	ftpeidl.h
 *****************************************************************************/

#ifndef _FTPEIDL_H
#define _FTPEIDL_H

#include "cowsite.h"


/*****************************************************************************
 *
 *	CFtpEidl
 *
 *	The stuff that tracks the state of an enumeration.
 *
 *****************************************************************************/

class CFtpEidl          : public IEnumIDList
                        , public CObjectWithSite
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IEnumIDList ***
    virtual STDMETHODIMP Next(ULONG celt, LPITEMIDLIST *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt);
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumIDList **ppenum);

public:
    CFtpEidl();
    ~CFtpEidl(void);

    // Friend Functions
    friend HRESULT CFtpEidl_Create(CFtpDir * pfd, CFtpFolder * pff, HWND hwndOwner, DWORD shcontf, IEnumIDList ** ppenum);

protected:
    // Private Member Variables

    int                     m_cRef;
    BITBOOL                 m_fInited : 1;      // Have we been inited?
    BITBOOL                 m_fDead : 1;        // Is this enumerator dead because we had to do a redirect (because password was needed or was invalid).
    BITBOOL                 m_fErrorDisplayed : 1; // Only show one error per enum.  This is how we keep track.
    DWORD                   m_nIndex;           // Current Item in the list
    DWORD                   m_shcontf;          // Filter flags
    CFtpPidlList *          m_pflHfpl;          // Where the cache lives
    CFtpDir *               m_pfd;              // My FtpDir
    CFtpFolder *            m_pff;              // My FtpDir
    IMalloc *               m_pm;               // Today's itemid allocator
    HWND                    m_hwndOwner;        // My UI window
    HRESULT                 m_hrOleInited;      // We need to Init Ole because of Win95's Shell32 doesn't do that on the background enum thread.
// BUGBUG -- someday  int m_cHiddenFiles;


    // Public Member Functions
    BOOL _fFilter(DWORD shcontf, DWORD dwFAFLFlags);
    HRESULT _Init(void);
    LPITEMIDLIST _NextOne(DWORD * pdwIndex);
    HRESULT _PopulateItem(HINTERNET hint0, HINTPROCINFO * phpi);
    HRESULT _AddFindDataToPidlList(LPCITEMIDLIST pidl);
    HRESULT _HandleSoftLinks(HINTERNET hint, LPITEMIDLIST pidl, LPWIRESTR pwCurrentDir, DWORD cchSize);
    BOOL _IsNavigationItem(LPWIN32_FIND_DATA pwfd);
    static HRESULT _PopulateItemCB(HINTERNET hint0, HINTPROCINFO * phpi, LPVOID pvCFtpEidl, BOOL * pfReleaseHint) {return ((CFtpEidl *)pvCFtpEidl)->_PopulateItem(hint0, phpi);};

    
    // Friend Functions
    friend HRESULT CFtpEidl_Create(CFtpDir * pfd, CFtpFolder * pff, HWND hwndOwner, DWORD shcontf, CFtpEidl ** ppfe);
    friend HRESULT CFtpEidl_Create(CFtpDir * pfd, CFtpFolder * pff, HWND hwndOwner, DWORD shcontf, DWORD dwIndex, IEnumIDList ** ppenum);
};

#endif // _FTPEIDL_H
