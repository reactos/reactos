/*****************************************************************************
 *
 *    ftpstm.cpp - IStream interface
 *
 *****************************************************************************/

#include "priv.h"
#include "ftpstm.h"
#include "ftpurl.h"

#define     UPDATE_PROGRESS_EVERY       (10*1024)       // Update progress every 10k

/*****************************************************************************
 *    CFtpStm::ReadOrWrite
 *****************************************************************************/
HRESULT CFtpStm::ReadOrWrite(LPVOID pv, ULONG cb, ULONG * pcb, DWORD dwAccess, STMIO io, HRESULT hresFail)
{
    HRESULT hr = STG_E_ACCESSDENIED;

    if (EVAL(m_dwAccessType & dwAccess))
    {
        ULONG cbOut;
        if (!pcb)
            pcb = &cbOut;

        hr = io(m_hint, TRUE, pv, cb, pcb);
        if (SUCCEEDED(hr) && m_ppd)
        {
            m_uliComplete.QuadPart += cb;
            m_ulBytesSinceProgressUpdate += cb;
            if (m_ulBytesSinceProgressUpdate > UPDATE_PROGRESS_EVERY)
            {
                m_ulBytesSinceProgressUpdate = 0;
                EVAL(SUCCEEDED(m_ppd->SetProgress64(m_uliComplete.QuadPart, m_uliTotal.QuadPart)));
            }

            if (TRUE == m_ppd->HasUserCancelled())
                hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
    }

    return hr;
}


//===========================
// *** IStream Interface ***
//===========================

/*****************************************************************************
 *    IStream::Read
 *****************************************************************************/
HRESULT CFtpStm::Read(LPVOID pv, ULONG cb, PULONG pcb)
{
    return ReadOrWrite(pv, cb, pcb, GENERIC_READ, InternetReadFileWrap, S_FALSE);
}


/*****************************************************************************
 *    IStream::Write
 *****************************************************************************/
HRESULT CFtpStm::Write(LPCVOID pv, ULONG cb, PULONG pcb)
{
    return ReadOrWrite((LPVOID)pv, cb, pcb, GENERIC_WRITE, (STMIO) InternetWriteFileWrap, STG_E_WRITEFAULT);
}


/*****************************************************************************
 *    IStream::CopyTo
 *
 *    _UNOBVIOUS_:  Implementing CopyTo is mandatory for drag/drop to work.
 *****************************************************************************/
#define SIZE_STREAM_COPY_BUFFER     (1024*16)        // 16k is the perfect size for 

HRESULT CFtpStm::CopyTo(IStream * pstmDest, ULARGE_INTEGER cbToCopy, ULARGE_INTEGER *pcbRead, ULARGE_INTEGER *pcbWritten)
{
    HRESULT hr = E_FAIL;
    IStream * pstmSrc;

    if (EVAL(SUCCEEDED(hr = QueryInterface(IID_IStream, (LPVOID *) &pstmSrc))))
    {
        ULARGE_INTEGER uliTotalIn;
        ULARGE_INTEGER uliTotalOut;
        uliTotalIn.QuadPart = uliTotalOut.QuadPart = 0;
        BYTE buffer[SIZE_STREAM_COPY_BUFFER];

        for (;;)
        {
            // Very unusual loop control
            ULONG cbIn = 0;        // In case pstmSrc forgets to

            //    No matter how you write this, the compiler emits horrid code.
            ULONG cb = (ULONG)min(SIZE_STREAM_COPY_BUFFER, cbToCopy.LowPart);
            hr = pstmSrc->Read(buffer, cb, &cbIn);
            uliTotalIn.QuadPart += cbIn;
            if (SUCCEEDED(hr) && cbIn)
            {
                ULARGE_INTEGER uliOut;    // In case pstmDest forgets to
                uliOut.QuadPart = 0;

                hr = pstmDest->Write(buffer, cbIn, &(uliOut.LowPart));
                uliTotalOut.QuadPart += uliOut.QuadPart;
                if (EVAL(SUCCEEDED(hr) && uliOut.QuadPart))
                {
                    // Onward
                }
                else
                {
                    break;        // Error or medium full
                }
            }
            else
            {
                break;            // Error or EOF reached
            }
        }

        if (pcbRead)
            pcbRead->QuadPart = uliTotalIn.QuadPart;

        if (pcbWritten)
            pcbWritten->QuadPart = uliTotalOut.QuadPart;

        pstmSrc->Release();
    }

    return hr;
}


/*****************************************************************************
 *    IStream::Commit
 *
 *    NOTE: WinINet doesn't really implement this, so I just do my best
 *****************************************************************************/
HRESULT CFtpStm::Commit(DWORD grfCommitFlags)
{
    return S_OK;
}


/*****************************************************************************
 *    IStream::LockRegion
 *
 *    You can't lock an ftp stream.
 *****************************************************************************/
HRESULT CFtpStm::LockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}


/*****************************************************************************
 *    IStream::UnlockRegion
 *
 *    You can't unlock an ftp stream because you can't lock one...
 *****************************************************************************/
HRESULT CFtpStm::UnlockRegion(ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType)
{
    return STG_E_INVALIDFUNCTION;
}


/*****************************************************************************
 *    IStream::Stat
 *
 *    We fill in what we can.
 *
 *    As the pwcsName, we put the URL that the stream represents, and
 *    install ourselves as the clsid.
 *****************************************************************************/
HRESULT CFtpStm::Stat(STATSTG *pstat, DWORD grfStatFlag)
{
    HRESULT hr;

    ZeroMemory(pstat, sizeof(*pstat));
    pstat->type = STGTY_STREAM;

    pstat->mtime = FtpPidl_GetFileTime(ILFindLastID(m_pidl));
    pstat->cbSize.QuadPart = FtpItemID_GetFileSize(m_pidl);

    pstat->grfMode |= STGM_SHARE_EXCLUSIVE | STGM_DIRECT;
    if (m_dwAccessType & GENERIC_READ)
        pstat->grfMode |= STGM_READ;

    if (m_dwAccessType & GENERIC_WRITE)
        pstat->grfMode |= STGM_WRITE;

    if (grfStatFlag & STATFLAG_NONAME)
        hr = S_OK;
    else
    {
        DWORD cchSize = (lstrlenW(FtpPidl_GetLastFileDisplayName(m_pidl)) + 1);

        pstat->pwcsName = (LPWSTR) SHAlloc(cchSize * sizeof(WCHAR));
        if (pstat->pwcsName)
        {
            StrCpyNW(pstat->pwcsName, FtpPidl_GetLastFileDisplayName(m_pidl), cchSize);
            hr = S_OK;
        }
        else
            hr = STG_E_INSUFFICIENTMEMORY;    // N.B., not E_OUTOFMEMORY
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION:   CFtpStm_Create

    DESCRIPTION:
        The caller will display errors, so don't do that here.
\*****************************************************************************/
HRESULT CFtpStm_Create(CFtpDir * pfd, LPCITEMIDLIST pidl, DWORD dwAccess, IStream ** ppstream, ULARGE_INTEGER uliComplete, ULARGE_INTEGER uliTotal, IProgressDialog * ppd, BOOL fClosePrgDlg)
{
    CFtpStm * pfstm = new CFtpStm();
    HRESULT hr = E_OUTOFMEMORY;
    DWORD dwError = ERROR_SUCCESS;

    *ppstream = NULL;
    if (pfstm)
    {
        Pidl_Set(&(pfstm->m_pidl), pidl);
        ASSERT(pfstm->m_pidl);
        pfstm->m_dwAccessType = dwAccess;
        IUnknown_Set(&pfstm->m_pfd, pfd);
        IUnknown_Set((IUnknown **)&pfstm->m_ppd, (IUnknown *)ppd);
        pfstm->m_uliComplete = uliComplete;
        pfstm->m_uliTotal = uliTotal;
        pfstm->m_fClosePrgDlg = fClosePrgDlg;

        //      GetHint() is going to want to spew status into the Status Bar
        //   But how do we get the hwnd?  This is an architectural question that
        //   we need to solve for all Shell Extensions.  The answer is to not use
        //   the progress bar in the status bar but a Progress Dialog.  But it's
        //   the responsibility of the caller to do that.
        HWND hwnd = NULL;

        hr = pfd->GetHint(hwnd, NULL, &pfstm->m_hintSession, NULL, NULL);
        if (EVAL(SUCCEEDED(hr)))
        {
            LPITEMIDLIST pidlVirtualRoot;

            hr = pfd->GetFtpSite()->GetVirtualRoot(&pidlVirtualRoot);
            if (EVAL(SUCCEEDED(hr)))
            {
                LPITEMIDLIST pidlOriginalFtpPath;
                CWireEncoding * pwe = pfd->GetFtpSite()->GetCWireEncoding();

                hr = FtpGetCurrentDirectoryPidlWrap(pfstm->m_hintSession, TRUE, pwe, &pidlOriginalFtpPath);
                if (SUCCEEDED(hr))
                {
                    LPITEMIDLIST pidlWithVirtualRoot;

                    hr = FtpPidl_InsertVirtualRoot(pidlVirtualRoot, pidl, &pidlWithVirtualRoot);
                    if (SUCCEEDED(hr))
                    {
                        hr = FtpSetCurrentDirectoryPidlWrap(pfstm->m_hintSession, TRUE, pidlWithVirtualRoot, TRUE, TRUE);
                        if (SUCCEEDED(hr))
                        {
                            DWORD dwDownloadType = FtpPidl_GetDownloadType(pidl);

                            // PERF: I bet we would be faster if we delayed the open until
                            //       the first ::Read(), ::Write(), or ::CopyToStream() call.
                            Pidl_Set(&pfstm->m_pidlOriginalFtpPath, pidlOriginalFtpPath);
                            hr = FtpOpenFileWrap(pfstm->m_hintSession, TRUE, FtpPidl_GetLastItemWireName(pidl), pfstm->m_dwAccessType, dwDownloadType, 0, &pfstm->m_hint);
                        }

                        ILFree(pidlWithVirtualRoot);
                    }

                    ILFree(pidlOriginalFtpPath);
                }

                ILFree(pidlVirtualRoot);
            }
        }

        if (SUCCEEDED(hr))
            hr = pfstm->QueryInterface(IID_IStream, (LPVOID *) ppstream);

        pfstm->Release();
    }

    return hr;
}




/****************************************************\
    Constructor
\****************************************************/
CFtpStm::CFtpStm() : m_cRef(1)
{
    DllAddRef();

    // This needs to be allocated in Zero Inited Memory.
    // Assert that all Member Variables are inited to Zero.
    ASSERT(!m_hint);
    ASSERT(!m_dwAccessType);
    ASSERT(!m_pfd);
    ASSERT(!m_hintSession);
    ASSERT(!m_pidl);
    ASSERT(!m_ppd);

    LEAK_ADDREF(LEAK_CFtpStm);
}


/****************************************************\
    Destructor
\****************************************************/
CFtpStm::~CFtpStm()
{
    if (m_hint)
    {
        InternetCloseHandle(m_hint);
    }

    // This COM object works like this:
    // 1. The constructor opens a handle to the server and
    //    Changes directory into the dir we are going to work in.
    // 2. The original dir is saved (m_pidlOriginalFtpPath) in order to be restored later
    //    because we cache the internet handle for perf and to keep our place on the server.
    // 3. The caller of this COM object can then copy data.
    // 4. We then Change directory to the original dir here before we close the internet handle.s
    if (m_pidlOriginalFtpPath && EVAL(m_hintSession))
    {
        EVAL(SUCCEEDED(FtpSetCurrentDirectoryPidlWrap(m_hintSession, TRUE, m_pidlOriginalFtpPath, TRUE, TRUE)));
        Pidl_Set(&m_pidlOriginalFtpPath, NULL);
    }

    if (m_hintSession)
        m_pfd->ReleaseHint(m_hintSession);

    ATOMICRELEASE(m_pfd);

    if (m_ppd && m_fClosePrgDlg)
        EVAL(SUCCEEDED(m_ppd->StopProgressDialog()));
    ATOMICRELEASE(m_ppd);

    ILFree(m_pidl);
    ILFree(m_pidlOriginalFtpPath);

    DllRelease();
    LEAK_DELREF(LEAK_CFtpStm);
}


//===========================
// *** IUnknown Interface ***
//===========================

ULONG CFtpStm::AddRef()
{
    m_cRef++;
    return m_cRef;
}

ULONG CFtpStm::Release()
{
    ASSERT(m_cRef > 0);
    m_cRef--;

    if (m_cRef > 0)
        return m_cRef;

    delete this;
    return 0;
}

HRESULT CFtpStm::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IStream))
    {
        *ppvObj = SAFECAST(this, IStream*);
    }
    else
    {
        TraceMsg(TF_FTPQI, "CFtpStm::QueryInterface() failed.");
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}
