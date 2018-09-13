/*****************************************************************************
 *    ftpicon.h
 *****************************************************************************/

#ifndef _FTPICON_H
#define _FTPICON_H


INT GetFtpIcon(UINT uFlags, BOOL fIsRoot);

/*****************************************************************************
    CFtpIcon

    The stuff that tells the shell which icon to use.
    Just plain annoying.  No real work is happening.
    Fortunately, the shell does most of the real work.

    Again, note that the szName is a plain char and not a TCHAR,
    because UNIX filenames are always ASCII.

    Extract() returning S_FALSE means "Could you do it for me?  Thanks."
 *****************************************************************************/

class CFtpIcon          : public IExtractIconW
                        , public IExtractIconA
                        , public IQueryInfo
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IExtractIconA ***
    virtual STDMETHODIMP GetIconLocation(UINT uFlags, LPSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags);
    virtual STDMETHODIMP Extract(LPCSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize) {return S_FALSE;};
    
    // *** IExtractIconW ***
    virtual STDMETHODIMP GetIconLocation(UINT uFlags, LPWSTR szIconFile, UINT cchMax, int * piIndex, UINT * pwFlags);
    virtual STDMETHODIMP Extract(LPCWSTR pszFile, UINT nIconIndex, HICON * phiconLarge, HICON * phiconSmall, UINT nIconSize) {return S_FALSE;};

    // *** IQueryInfo ***
    virtual STDMETHODIMP GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip);
    virtual STDMETHODIMP GetInfoFlags(DWORD *pdwFlags);


public:
    CFtpIcon();
    ~CFtpIcon(void);
    // Friend Functions
    friend HRESULT CFtpIcon_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, REFIID riid, LPVOID * ppv);
    friend HRESULT CFtpIcon_Create(CFtpFolder * pff, CFtpPidlList * pflHfpl, CFtpIcon ** ppfm);

protected:
    // Private Member Variables
    int                     m_cRef;

    CFtpPidlList *          m_pflHfpl;      // FtpDir in which our pidls live
    int                     m_nRoot;        // Gross HACKHACK (see CFtpIcon_Create)
    SINGLE_THREADED_MEMBER_VARIABLE;

    // Private Member Functions
    int ParseIconLocation(LPSTR pszIconFile);
    void GetDefaultIcon(LPSTR szIconFile, UINT cchMax, HKEY hk);
    HRESULT GetIconLocHkey(LPSTR szIconFile, UINT cchMax, LPINT pi, HKEY hk);
};

#endif // _FTPICON_H
