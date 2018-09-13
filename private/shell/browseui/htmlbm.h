
#ifndef _HTMLBM_H_
#define _HTMLBM_H_

#include "logo.h"

class CThumbnail :
    public IThumbnail,
    public CLogoBase
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IThumbnail ***
    virtual STDMETHODIMP Init(HWND hwnd, UINT uMsg);
    virtual STDMETHODIMP GetBitmap(LPCWSTR wszFile, DWORD dwItem, LONG lWidth, LONG lHeight);

private:
    // Constructor / Destructor (protected so we can't create on stack)
    CThumbnail(void);
    ~CThumbnail(void);

    // Instance creator
    friend HRESULT CThumbnail_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    // Private variables
protected:
    DWORD m_cRef;      // COM reference count
    HWND _hwnd;
    UINT _uMsg;
    LPSHELLIMAGESTORE _pImageStore;

    virtual IShellFolder * GetSF() {ASSERT(0);return NULL;};
    virtual HWND GetHWND() {ASSERT(0); return _hwnd;};

    virtual HRESULT UpdateLogoCallback( DWORD dwItem, int iIcon, HBITMAP hImage, LPCWSTR pszCache, BOOL fCache ) ;
    virtual REFTASKOWNERID GetTOID();

    BOOL IsItInCache( LPCWSTR pszItemPath, LPCWSTR pszGLocation, const FILETIME * pftDateStamp );
    void ReleaseImageStore( void );
};

#endif  // _HTMLBM_H
