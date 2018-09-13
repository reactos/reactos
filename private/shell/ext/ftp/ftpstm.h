/*****************************************************************************
 *	ftpstm.h
 *****************************************************************************/

#ifndef _FTPSTREAM_H
#define _FTPSTREAM_H

typedef HRESULT (*STMIO)(HINTERNET hinst, BOOL fAssertOnFailure, LPVOID pvBuffer, DWORD dwNumberOfBytesToWrite, LPDWORD pdwNumberOfBytesWritten);

/*****************************************************************************
 *
 *	CFtpStm
 *
 *
 *****************************************************************************/

class CFtpStm           : public IStream
{
public:
    //////////////////////////////////////////////////////
    // Public Interfaces
    //////////////////////////////////////////////////////
    
    // *** IUnknown ***
    virtual STDMETHODIMP_(ULONG) AddRef(void);
    virtual STDMETHODIMP_(ULONG) Release(void);
    virtual STDMETHODIMP QueryInterface(REFIID riid, LPVOID * ppvObj);
    
    // *** IStream ***
    virtual STDMETHODIMP Read(void *pv, ULONG cb, ULONG *pcbRead);
    virtual STDMETHODIMP Write(const void *pv, ULONG cb, ULONG *pcbWritten);
    virtual STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) { return(E_NOTIMPL); };
    virtual STDMETHODIMP SetSize(ULARGE_INTEGER libNewSize) { return(E_NOTIMPL); };
    virtual STDMETHODIMP CopyTo(IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten);
    virtual STDMETHODIMP Commit(DWORD grfCommitFlags);
    virtual STDMETHODIMP Revert(void) { return(E_NOTIMPL); };
    virtual STDMETHODIMP LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual STDMETHODIMP UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType);
    virtual STDMETHODIMP Stat(STATSTG *pstatstg, DWORD grfStatFlag);
    virtual STDMETHODIMP Clone(IStream **ppstm) { return(E_NOTIMPL); };


public:
    CFtpStm();
    ~CFtpStm(void);

    // Public Member Functions
    HRESULT ReadOrWrite(LPVOID pv, ULONG cb, ULONG * pcb, DWORD dwAccess, STMIO io, HRESULT hresFail);


    // Friend Functions
    friend HRESULT CFtpStm_Create(CFtpDir * pfd, LPCITEMIDLIST pidl, DWORD dwAccess, IStream ** ppstream, ULARGE_INTEGER uliComplete, ULARGE_INTEGER uliTotal, IProgressDialog * ppd, BOOL fClosePrgDlg);

protected:
    // Private Member Variables
    int                     m_cRef;

    HINTERNET               m_hint;             //
    DWORD                   m_dwAccessType;     //
    CFtpDir *               m_pfd;              //
    HINTERNET               m_hintSession;      //
    LPITEMIDLIST            m_pidl;             //
    IProgressDialog *       m_ppd;              //
    ULARGE_INTEGER          m_uliComplete;      //
    ULARGE_INTEGER          m_uliTotal;         //
    BOOL                    m_fClosePrgDlg;     // Do we want to close the progress dialog after we finish copying this stream?
    ULONG                   m_ulBytesSinceProgressUpdate;     // When was the last time I displayed progress?
    LPITEMIDLIST            m_pidlOriginalFtpPath;    // We need to return the current directory for this handle to this location when we are done.
};

#endif // _FTPSTREAM_H
