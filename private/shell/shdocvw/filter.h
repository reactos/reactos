#ifndef _FILTER_H_
#define _FILTER_H_

HRESULT CopyTo(IStream* pstmIn,
    /* [unique][in] */ IStream *pstm,
    /* [in] */ ULARGE_INTEGER cb,
    /* [out] */ ULARGE_INTEGER *pcbRead,
    /* [out] */ ULARGE_INTEGER *pcbWritten);

#if 0
class CFile
{
public:
    CFile() : m_fh(HFILE_ERROR) {m_szFile[0] = 0;}
    ~CFile() {Close();}

    BOOL Read( 
        /* [out] */ void *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG *pcbRead);

    BOOL Write( 
        /* [size_is][in] */ const void *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG *pcbWritten);

    BOOL Seek( 
        /* [in] */ LONG lMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULONG *plPosition);

    BOOL Load( 
        /* [in] */ LPCOLESTR pszFileName,
        /* [in] */ DWORD dwMode,
        BOOL bExist=FALSE);

    BOOL GetFileName(LPTSTR szFile)
    { lstrcpy(szFile, m_szFile); return(m_szFile[0]!='\0'); }

private:
    BOOL Open()
    {
        if (m_fh != HFILE_ERROR) 
            return TRUE;
        if (m_szFile[0] == '\0') 
            return FALSE;
#ifdef UNICODE
        m_fh = CreateFile(m_szFile, m_dwMode, NULL, );
        return m_fh != INVALID_HANDLE_VALUE;
#else
        m_fh = _lopen(m_szFile, m_dwMode);
        return m_fh != HFILE_ERROR;
#endif
    }
    void Close()
    {
        if (m_fh != HFILE_ERROR) 
            _lclose(m_fh);
        m_fh = HFILE_ERROR;
    }

    TCHAR m_szFile[MAX_PATH];
    HFILE m_fh;
    DWORD m_dwMode;
} ;
#endif

class CIDList
{
public:
    CIDList() : m_pidl(NULL), m_psfParent(NULL), m_psfHere(NULL) {}
    ~CIDList() {Free();}

    operator !() {return(m_pidl==NULL);}
    operator LPCITEMIDLIST() {return(m_pidl);}

    BOOL Save(LPCITEMIDLIST pidl, BOOL bCopy=TRUE);

    BOOL GetDisplayName(STRRET* psr, LPCITEMIDLIST pidl=NULL);
    BOOL GetFullPath(LPTSTR szPath, LPCITEMIDLIST pidlRel=NULL);

    IEnumIDList* EnumObjects()
    {
        if (!InitFolder()) return(NULL);
        IEnumIDList* pEnum;
        return(SUCCEEDED(m_psfHere->EnumObjects(NULL,
            SHCONTF_FOLDERS|SHCONTF_NONFOLDERS, &pEnum)) ? pEnum : NULL);
    }
    BOOL GetAttributes(DWORD* pdwAttr, LPCITEMIDLIST pidl)
    {
        if (!InitFolder()) return(FALSE);
        return(SUCCEEDED(m_psfHere->GetAttributesOf(1, &pidl, pdwAttr)));
    }

    void CleanUp();

    LPITEMIDLIST ParseDisplayName(LPCOLESTR szName)
    {
        if (!InitFolder()) return(NULL);
        LPITEMIDLIST pidl;
        if(FAILED(m_psfHere->ParseDisplayName(NULL, NULL, (LPOLESTR)szName, NULL,
            &pidl, NULL)))
        { return(NULL); }
        return(pidl);
    }

private:
    BOOL InitParentFolder();
    BOOL InitFolder();

    void Free() {CleanUp(); if (m_pidl) ILFree(m_pidl); m_pidl=NULL;}

    LPITEMIDLIST m_pidl;

    IShellFolder* m_psfParent;
    IShellFolder* m_psfHere;
} ;


class CStreamHLocal : public IStream
{
public:
    CStreamHLocal() : _cRef(1), m_pData(NULL), m_cbPos(0) { DllAddRef(); }
    virtual ~CStreamHLocal() { Clear(); DllRelease(); }

    // IUnknown
    virtual STDMETHODIMP QueryInterface(REFIID riid,void **ppvObj);
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);

    // IStream
    virtual STDMETHODIMP Read( 
        /* [out] */ void *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG *pcbRead);
    
    virtual STDMETHODIMP Write( 
        /* [size_is][in] */ const void *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG *pcbWritten);
    
    virtual STDMETHODIMP Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER *plibNewPosition);
    
    virtual STDMETHODIMP SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize);
    
    virtual STDMETHODIMP CopyTo( 
        /* [unique][in] */ IStream *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER *pcbRead,
        /* [out] */ ULARGE_INTEGER *pcbWritten)
    { return (::CopyTo(this, pstm, cb, pcbRead, pcbWritten)); }
    
    virtual STDMETHODIMP Commit( 
        /* [in] */ DWORD grfCommitFlags)
    { return(E_NOTIMPL); }
    
    virtual STDMETHODIMP Revert(void)
    { return(E_NOTIMPL); }
    
    virtual STDMETHODIMP LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType)
    { return(E_NOTIMPL); }
    
    virtual STDMETHODIMP UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType)
    { return(E_NOTIMPL); }
    
    virtual STDMETHODIMP Stat( 
        /* [out] */ STATSTG *pstatstg,
        /* [in] */ DWORD grfStatFlag);
    
    virtual STDMETHODIMP Clone( 
        /* [out] */ IStream **ppstm)
    { return(E_NOTIMPL); }

public:
    LPBYTE GetPtr() { return(m_pData); }
    UINT GetSize() { return(m_cbAll); }

    void Reset() { m_cbAll=m_cbPos=0; }
    void Clear() { if (m_pData) LocalFree((HLOCAL)m_pData); m_pData=NULL; Reset(); }

private:
    ULONG _cRef;

    UINT m_cbPos;
    UINT m_cbAlloced;

    UINT m_cbAll;
    LPBYTE m_pData;
};

inline LPSTR SkipSpace(LPCSTR pIn)
{
    while (*pIn == ' ')
    {
        ++pIn;
    }

    return((LPSTR)pIn);
}


inline BOOL IsSmallLarge(LARGE_INTEGER cb)
{
    LONG lOffset = (LONG)cb.LowPart;

    // Make sure we are only using 32 bits
    if (cb.HighPart==0 && lOffset>=0)
    {
        return(TRUE);
    }
    else if (cb.HighPart==-1 && lOffset<0)
    {
        return(TRUE);
    }

    return(FALSE);
}


inline BOOL IsSmallULarge(ULARGE_INTEGER cb)
{
    // Make sure we are only using 32 bits
    if (cb.HighPart == 0)
    {
        return(TRUE);
    }

    return(FALSE);
}


LPSTR StrStrN(LPCSTR pSrc, ULONG uSrcSize, LPCSTR pSearch);

#endif // _FILTER_H_

