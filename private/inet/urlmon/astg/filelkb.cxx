//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:	filelkb.cxx
//
//  Contents:	File ILockBytes implementation for async storage
//
//  Classes:	
//
//  Functions:	
//
//  History:	19-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

#include "astghead.cxx"
#pragma hdrstop

#include "filelkb.hxx"
#include <valid.h>

#define WIN32_SCODE(err) HRESULT_FROM_WIN32(err)
#define LAST_STG_SCODE Win32ErrorToScode(GetLastError())

#define boolChk(e) \
    if (!(e)) astgErr(Err, LAST_STG_SCODE) else 1
#define boolChkTo(l, e) \
    if (!(e)) astgErr(l, LAST_STG_SCODE) else 1
#define negChk(e) \
    if ((e) == 0xffffffff) astgErr(Err, LAST_STG_SCODE) else 1
#define negChkTo(l, e) \
    if ((e) == 0xffffffff) astgErr(l, LAST_STG_SCODE) else 1


class CSafeCriticalSection
{
public:
    inline CSafeCriticalSection(CRITICAL_SECTION *pcs);
    inline ~CSafeCriticalSection();
private:
    CRITICAL_SECTION *_pcs;
};

inline CSafeCriticalSection::CSafeCriticalSection(CRITICAL_SECTION *pcs)
{
    _pcs = pcs;
    EnterCriticalSection(_pcs);
}

inline CSafeCriticalSection::~CSafeCriticalSection()
{
    LeaveCriticalSection(_pcs);
#if DBG == 1
    _pcs = NULL;
#endif
}

#define TAKE_CS CSafeCriticalSection scs(&_cs);

    
//+---------------------------------------------------------------------------
//
//  Function:	Win32ErrorToScode, public
//
//  Synopsis:	Map a Win32 error into a corresponding scode, remapping
//              into Facility_Storage if appropriate.
//
//  Arguments:	[dwErr] -- Win32 error to map
//
//  Returns:	Appropriate scode
//
//  History:	22-Sep-93	PhilipLa	Created
//
//----------------------------------------------------------------------------

SCODE Win32ErrorToScode(DWORD dwErr)
{
    astgAssert((dwErr != NO_ERROR) &&
             "Win32ErrorToScode called on NO_ERROR");

    SCODE sc = STG_E_UNKNOWN;

    switch (dwErr)
    {
    case ERROR_INVALID_FUNCTION:
        sc = STG_E_INVALIDFUNCTION;
        break;
    case ERROR_FILE_NOT_FOUND:
        sc = STG_E_FILENOTFOUND;
        break;
    case ERROR_PATH_NOT_FOUND:
        sc = STG_E_PATHNOTFOUND;
        break;
    case ERROR_TOO_MANY_OPEN_FILES:
        sc = STG_E_TOOMANYOPENFILES;
        break;
    case ERROR_ACCESS_DENIED:
    case ERROR_NETWORK_ACCESS_DENIED:
        sc = STG_E_ACCESSDENIED;
        break;
    case ERROR_INVALID_HANDLE:
        sc = STG_E_INVALIDHANDLE;
        break;
    case ERROR_NOT_ENOUGH_MEMORY:
        sc = STG_E_INSUFFICIENTMEMORY;
        break;
    case ERROR_NO_MORE_FILES:
        sc = STG_E_NOMOREFILES;
        break;
    case ERROR_WRITE_PROTECT:
        sc = STG_E_DISKISWRITEPROTECTED;
        break;
    case ERROR_SEEK:
        sc = STG_E_SEEKERROR;
        break;
    case ERROR_WRITE_FAULT:
        sc = STG_E_WRITEFAULT;
        break;
    case ERROR_READ_FAULT:
        sc = STG_E_READFAULT;
        break;
    case ERROR_SHARING_VIOLATION:
        sc = STG_E_SHAREVIOLATION;
        break;
    case ERROR_LOCK_VIOLATION:
        sc = STG_E_LOCKVIOLATION;
        break;
    case ERROR_HANDLE_DISK_FULL:
    case ERROR_DISK_FULL:
        sc = STG_E_MEDIUMFULL;
        break;
    case ERROR_FILE_EXISTS:
    case ERROR_ALREADY_EXISTS:
        sc = STG_E_FILEALREADYEXISTS;
        break;
    case ERROR_INVALID_PARAMETER:
        sc = STG_E_INVALIDPARAMETER;
        break;
    case ERROR_INVALID_NAME:
    case ERROR_BAD_PATHNAME:
    case ERROR_FILENAME_EXCED_RANGE:
        sc = STG_E_INVALIDNAME;
        break;
    case ERROR_INVALID_FLAGS:
        sc = STG_E_INVALIDFLAG;
        break;
    default:
        sc = WIN32_SCODE(dwErr);
        break;
    }
        
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::CFileLockBytes, public
//
//  Synopsis:	Default constructor
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

CFileLockBytes::CFileLockBytes(void)
{
    _cReferences = 1;
    _h = INVALID_HANDLE_VALUE;
    InitializeCriticalSection(&_cs);
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::~CFileLockBytes, public
//
//  Synopsis:	Destructor
//
//  Returns:	Appropriate status code
//
//  History:	28-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

CFileLockBytes::~CFileLockBytes()
{
    if (_h != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_h);
    }
    DeleteCriticalSection(&_cs);
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::Init, public
//
//  Synopsis:	Initialization function
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

SCODE CFileLockBytes::Init(OLECHAR const *pwcsName)
{
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::Init:%p()\n", this));

#ifndef UNICODE
    TCHAR atcPath[_MAX_PATH + 1];
    UINT uCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
    
    if (!WideCharToMultiByte(
        uCodePage,
        0,
        pwcsName,
        -1,
        atcPath,
        _MAX_PATH + 1,
        NULL,
        NULL))
    {
        return STG_E_INVALIDNAME;
    }

    _h = CreateFileA(atcPath,
                    GENERIC_READ | GENERIC_WRITE, //Read-write
                    0,                            // No sharing
                    NULL,
                    CREATE_NEW,                   //Create if necessary
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                    NULL);
#else    
    _h = CreateFile(pwcsName,
                    GENERIC_READ | GENERIC_WRITE, //Read-write
                    0,                            // No sharing
                    NULL,
                    CREATE_NEW,                   //Create if necessary
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
                    NULL);
#endif    
    if (_h == INVALID_HANDLE_VALUE)
    {
        return LAST_STG_SCODE;
    }
    wcscpy(_acName, pwcsName);
    
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::Init\n"));
    return S_OK;
}


//+--------------------------------------------------------------
//
//  Member:     CFileLockBytes::QueryInterface, public
//
//  Synopsis:   Returns an object for the requested interface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    26-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP CFileLockBytes::QueryInterface(REFIID iid, void **ppvObj)
{
    TAKE_CS;
    
    SCODE sc;

    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::QueryInterface(?, %p)\n",
                ppvObj));
    
    astgChk(ValidateOutPtrBuffer(ppvObj));
    *ppvObj = NULL;

    sc = S_OK;
    if (IsEqualIID(iid, IID_IUnknown))
    {
        *ppvObj = (IUnknown *)this;
        CFileLockBytes::AddRef();
    }
    else if (IsEqualIID(iid, IID_ILockBytes))
    {
        *ppvObj = (ILockBytes *)this;
        CFileLockBytes::AddRef();
    }
    else
    {
        sc = E_NOINTERFACE;
    }

    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::QueryInterface => %p\n",
                ppvObj));

Err:
    return ResultFromScode(sc);
}


//+--------------------------------------------------------------
//
//  Member:     CFileLockBytes::AddRef, public
//
//  Synopsis:   Increments the ref count
//
//  Returns:    Appropriate status code
//
//  History:    16-Mar-92       DrewB   Created
//
//---------------------------------------------------------------

STDMETHODIMP_(ULONG) CFileLockBytes::AddRef(void)
{
    ULONG ulRet;

    astgDebugOut((DEB_TRACE, "In  CFileLockBytes::AddRef()\n"));

    InterlockedIncrement(&_cReferences);
    ulRet = _cReferences;

    astgDebugOut((DEB_TRACE, "Out CFileLockBytes::AddRef\n"));
    return ulRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::Release, public
//
//  History:	28-Dec-95	PhilipLa	Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CFileLockBytes::Release(void)
{
    LONG lRet;
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::Release:%p()\n", this));

    lRet = InterlockedDecrement(&_cReferences);
    if (lRet == 0)
    {
        delete this;
    }
    else if (lRet < 0)
    {
        lRet = 0;
    }
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::Release\n"));
    return lRet;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::ReadAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileLockBytes::ReadAt(ULARGE_INTEGER ulOffset,
                                    VOID HUGEP *pv,
                                    ULONG cb,
                                    ULONG *pcbRead)
{
    TAKE_CS;
    
    SCODE sc = S_OK;
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::ReadAt:%p()\n", this));
    ULONG ulLow = ulOffset.LowPart;
    LONG lHigh = (LONG)ulOffset.HighPart;
    negChk(SetFilePointer(_h,
                          ulLow,
                          &lHigh,
                          FILE_BEGIN));
    boolChk(ReadFile(_h, pv, cb, pcbRead, NULL));
    
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::ReadAt\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::WriteAt, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileLockBytes::WriteAt(ULARGE_INTEGER ulOffset,
                                     VOID const HUGEP *pv,
                                     ULONG cb,
                                     ULONG *pcbWritten)
{
    TAKE_CS;
    
    SCODE sc = S_OK;
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::WriteAt:%p()\n", this));

    ULONG ulLow = ulOffset.LowPart;
    LONG lHigh = (LONG)ulOffset.HighPart;
    negChk(SetFilePointer(_h,
                          ulLow,
                          &lHigh,
                          FILE_BEGIN));
    boolChk(WriteFile(_h, pv, cb, pcbWritten, NULL));
    
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::WriteAt\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::Flush, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------
//BUGBUG:  Implement something here?
STDMETHODIMP CFileLockBytes::Flush(void)
{
    TAKE_CS;
    
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::Flush:%p()\n", this));
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::Flush\n"));
    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::SetSize, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileLockBytes::SetSize(ULARGE_INTEGER cb)
{
    TAKE_CS;
    
    SCODE sc = S_OK;
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::SetSize:%p()\n", this));
    LONG lHigh = (LONG)cb.HighPart;
    ULONG ulLow = cb.LowPart;

    negChk(SetFilePointer(_h, ulLow, &lHigh, FILE_BEGIN));
    boolChk(SetEndOfFile(_h));
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::SetSize\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::LockRegion, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileLockBytes::LockRegion(ULARGE_INTEGER libOffset,
                                        ULARGE_INTEGER cb,
                                        DWORD dwLockType)
{
    TAKE_CS;
    
    SCODE sc = S_OK;
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::LockRegion:%p()\n", this));
    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
    {
        return STG_E_INVALIDFUNCTION;
    }
    
    boolChk(LockFile(_h,
                     libOffset.LowPart,
                     libOffset.HighPart,
                     cb.LowPart,
                     cb.HighPart));
            
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::LockRegion\n"));
Err:    
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFileLockBytes::UnlockRegion, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileLockBytes::UnlockRegion(ULARGE_INTEGER libOffset,
                                          ULARGE_INTEGER cb,
                                          DWORD dwLockType)
{
    TAKE_CS;
    
    SCODE sc = S_OK;
    astgDebugOut((DEB_ITRACE, "In  CFileLockBytes::UnlockRegion:%p()\n", this));
    if (dwLockType != LOCK_EXCLUSIVE && dwLockType != LOCK_ONLYONCE)
    {
        return STG_E_INVALIDFUNCTION;
    }
    
    boolChk(UnlockFile(_h,
                     libOffset.LowPart,
                     libOffset.HighPart,
                     cb.LowPart,
                     cb.HighPart));
            
    astgDebugOut((DEB_ITRACE, "Out CFileLockBytes::UnlockRegion\n"));
Err:
    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:	CFIleLockBytes::Stat, public
//
//  Synopsis:	
//
//  Arguments:	
//
//  Returns:	Appropriate status code
//
//  Modifies:	
//
//  History:	28-Dec-95	PhilipLa	Created
//
//  Notes:	
//
//----------------------------------------------------------------------------

STDMETHODIMP CFileLockBytes::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    TAKE_CS;
    
    SCODE sc;

    astgDebugOut((DEB_ITRACE, "In  CFIleLockBytes::Stat:%p()\n", this));

    negChk(pstatstg->cbSize.LowPart =
           GetFileSize(_h, &pstatstg->cbSize.HighPart));
    boolChk(GetFileTime(_h, &pstatstg->ctime, &pstatstg->atime,
                        &pstatstg->mtime));
    pstatstg->grfLocksSupported = LOCK_EXCLUSIVE | LOCK_ONLYONCE;
    
    pstatstg->type = STGTY_LOCKBYTES;
    //BUGBUG:  Set grfMode
    //pstatstg->grfMode = DFlagsToMode(_pgfst->GetDFlags());
    pstatstg->pwcsName = NULL;
    if ((grfStatFlag & STATFLAG_NONAME) == 0)
    {
        pstatstg->pwcsName = (OLECHAR *)CoTaskMemAlloc(
            (wcslen(_acName) + 1) * sizeof(OLECHAR));
        if (pstatstg->pwcsName == NULL)
            return STG_E_INSUFFICIENTMEMORY;
        wcscpy(pstatstg->pwcsName, _acName);
    }
    sc = S_OK;

    astgDebugOut((DEB_ITRACE, "Out CFIleLockBytes::Stat\n"));
    return NOERROR;

Err:
    return sc;
}
