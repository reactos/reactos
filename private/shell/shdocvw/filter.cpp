#include "priv.h"
#include "filter.h"

#define MEM_SLOP    1024


BOOL BindToIDList(LPCITEMIDLIST pidl, REFIID riid, void ** ppObj)
{
    IShellFolder* psfDesktop;

    if (FAILED(CoCreateInstance(CLSID_ShellDesktop, NULL, CLSCTX_INPROC_SERVER,
        IID_IShellFolder, (void **)&psfDesktop)))
    {
        return(FALSE);
    }

    if (ILIsEmpty(pidl))
    {
        // We are looking at the Desktop
        if (FAILED(psfDesktop->QueryInterface(riid, ppObj)))
        {
            // Just in case
            *ppObj = NULL;
        }
    }
    else if (FAILED(psfDesktop->BindToObject(pidl, NULL, riid, ppObj)))
    {
        // Just in case
        *ppObj = NULL;
    }

    psfDesktop->Release();

    return(*ppObj != NULL);
}


LPSTR StrStrN(LPCSTR pSrc, ULONG uSrcSize, LPCSTR pSearch)
{
    UINT uSearchSize = lstrlenA(pSearch);
    if (uSrcSize < uSearchSize)
    {
        return(NULL);
    }

    LPCSTR pEnd = pSrc + uSrcSize - uSearchSize;

    for ( ; pSrc<=pEnd; ++pSrc)
    {
        if (memcmp(pSrc, pSearch, uSearchSize) == 0)
        {
            return((LPSTR)pSrc);
        }
    }

    return(NULL);
}

#if 0
BOOL CFile::Read( 
    /* [out] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead)
{
    if (!Open())
    {
        return(FALSE);
    }

    UINT uRead = _lread(m_fh, pv, cb);
    if (pcbRead)
    {
        *pcbRead = uRead;
    }

    return(uRead != HFILE_ERROR);
}


BOOL CFile::Write( 
    /* [size_is][in] */ const void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbWritten)
{
    if (!Open())
    {
        return(FALSE);
    }

    UINT uWrite = _lwrite(m_fh, (LPCSTR)pv, cb);
    if (pcbWritten)
    {
        *pcbWritten = uWrite;
    }

    return(uWrite != HFILE_ERROR);
}


BOOL CFile::Seek( 
    /* [in] */ LONG lMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULONG *plPosition)
{
    if (!Open())
    {
        return(FALSE);
    }

    UINT uPos = _llseek(m_fh, lMove, dwOrigin);
    if (plPosition)
    {
        *plPosition = uPos;
    }

    return(uPos != HFILE_ERROR);
}


BOOL CFile::Load( 
    /* [in] */ LPCOLESTR pszFileName,
    /* [in] */ DWORD dwMode,
    BOOL bExist)
{
    Close();

    if (pszFileName)
    {
        m_dwMode = dwMode;
        OleStrToStrN(m_szFile, ARRAYSIZE(m_szFile), pszFileName, -1);

        if (bExist)
        {
            if (GetFileAttributes(m_szFile) == (DWORD)-1)
            {
                m_szFile[0] = '\0';
                return(FALSE);
            }
        }
    }
    else
    {
        // Just meant to clena up
    }

    return(TRUE);
}
#endif

BOOL CIDList::Save(LPCITEMIDLIST pidl, BOOL bCopy)
{
    Free();

    if (!pidl)
    {
        return(TRUE);
    }

    m_pidl = bCopy ? ILClone(pidl) : (LPITEMIDLIST)pidl;

    return(m_pidl != NULL);
}


BOOL CIDList::GetFullPath(LPTSTR szPath, LPCITEMIDLIST pidlRel)
{
    LPITEMIDLIST pidl;

    if (pidlRel)
    {
        pidl = ILCombine(m_pidl, pidlRel);
    }
    else
    {
        pidl = m_pidl;
    }
    if (!pidl)
    {
        return(FALSE);
    }

    BOOL bRet = SHGetPathFromIDList(pidl, szPath);

    if (pidlRel)
    {
        ILFree(pidl);
    }

    return(bRet);
}


BOOL CIDList::InitParentFolder()
{
    if (m_psfParent)
    {
        return(TRUE);
    }

    if (!m_pidl)
    {
        // I think this is an error
        return(FALSE);
    }

    if (0 == m_pidl->mkid.cb)
    {
        // The Desktop has no parent
        return(FALSE);
    }

    // m_pidl is a copy, so I can play with it
    LPITEMIDLIST pidlLast = ILFindLastID(m_pidl);
    USHORT cbSave = pidlLast->mkid.cb;
    pidlLast->mkid.cb = 0;
    if (!BindToIDList(m_pidl, IID_IShellFolder, (void **)&m_psfParent))
    {
        // Just in case
        m_psfParent = NULL;
    }
    pidlLast->mkid.cb = cbSave;

    return(m_psfParent != NULL);
}


BOOL CIDList::InitFolder()
{
    if (m_psfHere)
    {
        return(TRUE);
    }

    if (!m_pidl)
    {
        // I think this is an error
        return(FALSE);
    }

    if (!BindToIDList(m_pidl, IID_IShellFolder, (void **)&m_psfHere))
    {
        // Just in case
        m_psfHere = NULL;
    }

    return(m_psfHere != NULL);
}


void CIDList::CleanUp()
{
    if (m_psfParent)
    {
        m_psfParent->Release();
    }
    m_psfParent = NULL;

    if (m_psfHere)
    {
        m_psfHere->Release();
    }
    m_psfHere = NULL;
}


BOOL CIDList::GetDisplayName(STRRET* psr, LPCITEMIDLIST pidl)
{
    IShellFolder* psfQuery;
    LPCITEMIDLIST pidlQuery;

    if (pidl)
    {
        if (!InitFolder())
        {
            return(FALSE);
        }

        psfQuery = m_psfHere;
        pidlQuery = pidl;
    }
    else
    {
        if (InitParentFolder())
        {
            psfQuery = m_psfParent;
            pidlQuery = ILFindLastID(m_pidl);
        }
        else
        {
            // Might be the Desktop; try the current folder
            if (!InitFolder())
            {
                return(FALSE);
            }

            psfQuery = m_psfHere;
            pidlQuery = NULL;
        }
    }

    if (FAILED(psfQuery->GetDisplayNameOf(pidlQuery,
        SHGDN_INFOLDER | SHGDN_NORMAL, psr)))
    {
        return FALSE;
    }

    // BUGBUG: UNICODE: Probably need some special code here; always want to
    // return STRRET_CSTRA
    if (FAILED(StrRetToBufA(psr, pidlQuery, psr->cStr, SIZEOF(psr->cStr))))
    {
        return FALSE;
    }
    psr->uType = STRRET_CSTR;

    return TRUE;
}


STDMETHODIMP CStreamHLocal::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IStream))
    {
        *ppvObj = SAFECAST(this, IStream *);
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return NOERROR;
}

ULONG CStreamHLocal::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CStreamHLocal::Release()
{
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

// IStream
STDMETHODIMP CStreamHLocal::Read( 
    /* [out] */ void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbRead)
{
    if (pcbRead)
    {
        *pcbRead = 0;
    }

    if (!m_pData)
    {
        return(S_FALSE);
    }

    if (m_cbPos >= m_cbAll)
    {
        return(S_FALSE);
    }

    ULONG cbRest = m_cbAll - m_cbPos;
    if (cb > cbRest)
    {
        cb = cbRest;
    }

    CopyMemory(pv, (LPSTR)m_pData + m_cbPos, cb);
    m_cbPos += cb;

    if (pcbRead)
    {
        *pcbRead = cb;
    }

    return(S_OK);
}


STDMETHODIMP CStreamHLocal::Write( 
    /* [size_is][in] */ const void *pv,
    /* [in] */ ULONG cb,
    /* [out] */ ULONG *pcbWritten)
{
    if (pcbWritten)
    {
        *pcbWritten = 0;
    }

    if (!m_pData)
    {
        m_pData = (LPBYTE)LocalAlloc(LPTR, m_cbPos+cb+MEM_SLOP);
        if (!m_pData)
        {
            return(E_OUTOFMEMORY);
        }

        m_cbAlloced = LocalSize((HLOCAL)m_pData);
        m_cbAll = 0;
    }

    if (m_cbAlloced < m_cbPos+cb)
    {
        HLOCAL pTemp = LocalReAlloc((HLOCAL)m_pData, m_cbPos+cb+MEM_SLOP,
            LMEM_MOVEABLE|LMEM_ZEROINIT);
        if (!pTemp)
        {
            return(E_OUTOFMEMORY);
        }

        m_cbAlloced = LocalSize(pTemp);
        m_pData = (LPBYTE)pTemp;
    }

    CopyMemory(m_pData + m_cbPos, pv, cb);
    m_cbPos += cb;

    if (m_cbPos > m_cbAll)
    {
        m_cbAll = m_cbPos;
    }

    if (pcbWritten)
    {
        *pcbWritten = cb;
    }

    return(S_OK);
}


STDMETHODIMP CStreamHLocal::Seek( 
    /* [in] */ LARGE_INTEGER dlibMove,
    /* [in] */ DWORD dwOrigin,
    /* [out] */ ULARGE_INTEGER *plibNewPosition)
{
    LONG lOffset = (LONG)dlibMove.LowPart;

    if (!IsSmallLarge(dlibMove))
    {
        return(E_INVALIDARG);
    }

    switch (dwOrigin)
    {
    case STREAM_SEEK_SET:
        break;

    case STREAM_SEEK_CUR:
        lOffset = (LONG)m_cbPos + lOffset;
        break;

    case STREAM_SEEK_END:
        lOffset = (LONG)m_cbAll + lOffset;
        break;

    default:
        return(E_INVALIDARG);
    }

    // Check the new offset is in range
    if (lOffset < 0)
    {
        return(E_INVALIDARG);
    }

    // Store the new offset and return it
    m_cbPos = (ULONG)lOffset;

    if (plibNewPosition)
    {
        plibNewPosition->HighPart = 0;
        plibNewPosition->LowPart = m_cbPos;
    }

    return(S_OK);
}


STDMETHODIMP CStreamHLocal::SetSize( 
    /* [in] */ ULARGE_INTEGER libNewSize)
{
    if (!IsSmallULarge(libNewSize))
    {
        return(STG_E_INVALIDFUNCTION);
    }

    if (!m_pData)
    {
        m_pData = (LPBYTE)LocalAlloc(LPTR, libNewSize.LowPart);
        if (!m_pData)
        {
            return(STG_E_MEDIUMFULL);
        }
    }
    else
    {
        HLOCAL pTemp = LocalReAlloc((HLOCAL)m_pData, libNewSize.LowPart,
            LMEM_MOVEABLE|LMEM_ZEROINIT);
        if (!pTemp)
        {
            return(STG_E_MEDIUMFULL);
        }

        m_pData = (LPBYTE)pTemp;
    }

    m_cbAlloced = LocalSize((HLOCAL)m_pData);
    m_cbAll = libNewSize.LowPart;

    return(S_OK);
}


STDMETHODIMP CStreamHLocal::Stat( 
    /* [out] */ STATSTG *pstatstg,
    /* [in] */ DWORD grfStatFlag)
{
    if (!pstatstg)
        return E_INVALIDARG;

    ZeroMemory(pstatstg, SIZEOF(*pstatstg));

    pstatstg->cbSize.LowPart = m_cbAll;

    // that's all the info we have!

    return S_OK;
}

