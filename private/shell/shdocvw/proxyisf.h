#ifndef _PROXYISF_H_
#define _PROXYISF_H_

#include "iface.h"

// Host Proxy IShellFolder Object.  It wraps a single object that supports
// IShellFolder and forwards all methods to that object.  It takes 
// first dibs at responding to GetUIObjectOf, so it can provide more
// objects via this method.
//
// Win95 Shell32 does not support aggregation.  So this cannot fully
// aggregate the wrapped object.  Because of this, we implement the 
// same interfaces that CFSFolder implements so we at least support
// that.
//


class CHostProxyISF : public IProxyShellFolder,
                      public IShellIcon,
                      public IPersistFolder,
                      public IShellService
{
    
public:
    // *** IUnknown methods ***
    virtual STDMETHODIMP QueryInterface(REFIID,void **);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    
    // *** IShellFolder methods ***
    virtual STDMETHODIMP ParseDisplayName(HWND hwndOwner,
                                LPBC pbcReserved, LPOLESTR lpszDisplayName,
                                ULONG * pchEaten, LPITEMIDLIST * ppidl, ULONG *pdwAttributes);

    virtual STDMETHODIMP EnumObjects( THIS_ HWND hwndOwner, DWORD grfFlags, LPENUMIDLIST * ppenumIDList);

    virtual STDMETHODIMP BindToObject(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, LPVOID * ppvOut);
    virtual STDMETHODIMP BindToStorage(LPCITEMIDLIST pidl, LPBC pbcReserved,
                                REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP CompareIDs(LPARAM lParam, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2);
    virtual STDMETHODIMP CreateViewObject (HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
    virtual STDMETHODIMP GetAttributesOf(UINT cidl, LPCITEMIDLIST * apidl,
                                ULONG * rgfInOut);
    virtual STDMETHODIMP GetUIObjectOf(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
    virtual STDMETHODIMP GetDisplayNameOf(LPCITEMIDLIST pidl, DWORD uFlags, LPSTRRET lpName);
    virtual STDMETHODIMP SetNameOf(HWND hwndOwner, LPCITEMIDLIST pidl,
                                LPCOLESTR lpszName, DWORD uFlags,
                                LPITEMIDLIST * ppidlOut);

    // *** IShellIcon methods ***
    virtual STDMETHODIMP GetIconOf(LPCITEMIDLIST pidl, UINT flags, LPINT lpIconIndex);

    // *** IPersist methods ***
    virtual STDMETHODIMP GetClassID(LPCLSID lpClassID);

    // *** IPersistFolder methods ***
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);

    // *** IShellService methods ***
    virtual STDMETHODIMP SetOwner(IUnknown * punkOwner);

    // *** IProxyShellFolder methods ***
    virtual STDMETHODIMP InitHostProxy(IShellFolder * psf, LPCITEMIDLIST pidl, DWORD dwFlags);

    // *** Other methods to be implemented by derived class ***

    virtual STDMETHODIMP CloneProxyPSF(REFIID riid, LPVOID * ppvObj) PURE;
    virtual STDMETHODIMP GetUIObjectOfPSF(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                      REFIID riid, UINT * prgfInOut, LPVOID * ppvOut) PURE;
    virtual STDMETHODIMP CreateViewObjectPSF(HWND hwndOwner, REFIID riid, LPVOID * ppvOut) PURE;

    virtual STDMETHODIMP SetOwnerPSF(IUnknown * punkOwner) PURE;

protected:
    CHostProxyISF();
    virtual ~CHostProxyISF();

    int             _cRef;
    DWORD           _dwFlags;           // SPF_*
    IShellFolder *  _psf;
    LPITEMIDLIST    _pidl;
    IUnknown *      _punkOwner;

    // Cached pointers to interfaces supported by CFSFolder
    IShellIcon *    _psi;
    IPersist *      _pp;
    IPersistFolder * _ppf;
    IShellFolder *  _psfRealCFSFolder;
};


// 
//  Base file-system proxy class
//

class CFSProxyISF : public CHostProxyISF
{
    
public:
    // *** CHostProxyISF methods ***
    virtual STDMETHODIMP CloneProxyPSF(REFIID riid, LPVOID * ppvObj);
    virtual STDMETHODIMP GetUIObjectOfPSF(HWND hwndOwner, UINT cidl, LPCITEMIDLIST * apidl,
                                      REFIID riid, UINT * prgfInOut, LPVOID * ppvOut);
    virtual STDMETHODIMP CreateViewObjectPSF(HWND hwndOwner, REFIID riid, LPVOID * ppvOut);
    virtual STDMETHODIMP SetOwnerPSF(IUnknown * punkOwner);

    CFSProxyISF();
    
protected:
    virtual ~CFSProxyISF();

    virtual IShellService * v_NewProxy(void) PURE;
    
};


#endif  // _PROXYISF_H__

