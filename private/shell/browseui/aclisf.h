/* Copyright 1996 Microsoft */

#ifndef _ACLISF_H_
#define _ACLISF_H_

#include "shellurl.h"

class CACLIShellFolder
                : public IEnumString
                , public IACList2
                , public ICurrentWorkingDirectory
                , public IShellService
                , public IPersistFolder
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);

    // *** IEnumString ***
    virtual STDMETHODIMP Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched);
    virtual STDMETHODIMP Skip(ULONG celt) {return E_NOTIMPL;}
    virtual STDMETHODIMP Reset(void);
    virtual STDMETHODIMP Clone(IEnumString **ppenum) {return E_NOTIMPL;}

    // *** IACList ***
    virtual STDMETHODIMP Expand(LPCOLESTR pszExpand);

    // *** IACList2 ***
    virtual STDMETHODIMP SetOptions(DWORD dwFlag);
    virtual STDMETHODIMP GetOptions(DWORD* pdwFlag);

    // *** ICurrentWorkingDirectory ***
    virtual STDMETHODIMP GetDirectory(LPWSTR pwzPath, DWORD cchSize) {return E_NOTIMPL;};
    virtual STDMETHODIMP SetDirectory(LPCWSTR pwzPath);

    // *** IPersistFolder ***
    virtual STDMETHODIMP Initialize(LPCITEMIDLIST pidl);        // Save as SetDirectory() but for pidls
    virtual STDMETHODIMP GetClassID(CLSID *pclsid);

    // *** IShellService ***
    virtual STDMETHODIMP SetOwner(IUnknown* punkOwner);


private:
    // Constructor / Destructor (protected so we can't create on stack)
    CACLIShellFolder();
    ~CACLIShellFolder(void);

    HRESULT _SetLocation(LPCITEMIDLIST pidl);
    HRESULT _TryNextPath(void);
    HRESULT _Init(void);
    BOOL _SkipForPerf(LPCWSTR pwzExpand);
    HRESULT _PassesFilter(LPCITEMIDLIST pidl);
    HRESULT _GetNextWrapper(LPITEMIDLIST * ppidl, ULONG *pceltFetched);

    // Instance creator
    friend HRESULT CACLIShellFolder_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    // Private variables
    DWORD           _cRef;          // COM reference count
    IEnumIDList*    _peidl;         // PIDL enumerator
    IShellFolder*   _psf;           // Shell folder
    IBrowserService* _pbs;          // Browser Service to find Current Location in Shell Name Space.
    LPITEMIDLIST    _pidl;          // PIDL of current directory.
    LPITEMIDLIST    _pidlCWD;       // PIDL of current working directory. 
    LPITEMIDLIST    _pidlInFolder;  // Sometimes the user string matches SHGDN_INFOLDER, but not (SHGDN_INFOLDER | SHGDN_FORPARSING).  My Computer for example.
    BOOL            _fExpand;       // Are we expanding?
    LPTSTR          _szExpandStr;   // String we are expanding
    CShellUrl *     _pshuLocation; 
    int             _nPathIndex; 
    DWORD           _dwOptions;     // ACLO_* flags
    BOOL            _fShowHidden;   // Enumerate hidden files?
//    BOOL            _fShowSysFiles; // Enumerate system files?
};

#endif // _ACLISF_H_