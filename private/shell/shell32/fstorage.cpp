/*----------------------------------------------------------------------------
/ Title;
/   fstorage.cpp
/
/ Authors;
/   David De Vorchik (daviddv)
/
/ Notes;
/   Implements IStorage for Win32 file calls.
/----------------------------------------------------------------------------*/
#include "shellprv.h"
#include "winbase.h"
#pragma hdrstop

/*-----------------------------------------------------------------------------
/ classes / static stuff etc
/----------------------------------------------------------------------------*/

class CFileStorage : public IStorage
{
    private:
        LONG _cRef;
        LPTSTR _pStoragePath;

        VOID _GetFullPath(const OLECHAR* pwcsName, LPTSTR pBufer);
        HRESULT _DoFsDelete(LPCTSTR pszFile);
        HRESULT _OpenCreateStorage(const OLECHAR *pwcsName, DWORD grfMode, IStorage **ppstg, BOOL fCreate);

    public:
        CFileStorage(LPCTSTR pPath);
        ~CFileStorage();

        // *** IUnknown methods ***
        STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
        STDMETHOD_(ULONG,AddRef)();
        STDMETHOD_(ULONG,Release)();

        // *** IStorage methods *** 
        STDMETHOD(CreateStream)(const OLECHAR* pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm);
        STDMETHOD(OpenStream)(const OLECHAR* pwcsName, VOID *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm);                
        STDMETHOD(CreateStorage)(const OLECHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg);        
        STDMETHOD(OpenStorage)(const OLECHAR *pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg);;
        STDMETHOD(CopyTo)(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest);        
        STDMETHOD(MoveElementTo)(const OLECHAR *pwcsName, IStorage *pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags);        
        STDMETHOD(Commit)(DWORD grfCommitFlags);
        STDMETHOD(Revert)();
        STDMETHOD(EnumElements)(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum);        
        STDMETHOD(DestroyElement)(const OLECHAR *pwcsName);        
        STDMETHOD(RenameElement)(const OLECHAR *pwcsOldName, const OLECHAR *pwcsNewName);        
        STDMETHOD(SetElementTimes)(const OLECHAR *pwcsName, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime);
        STDMETHOD(SetClass)(REFCLSID clsid);        
        STDMETHOD(SetStateBits)(DWORD grfStateBits, DWORD grfMask);        
        STDMETHOD(Stat)(STATSTG *pstatstg, DWORD grfStatFlag);
};

//
// Construction / destruction
//

CFileStorage::CFileStorage(LPCTSTR pPath) :
    _cRef(1)
{
    Str_SetPtr(&_pStoragePath, pPath);
}

CFileStorage::~CFileStorage()
{
    Str_SetPtr(&_pStoragePath, NULL);
}

//
// Call the shell file operation code to delete recursively the given directory,
// don't show any UI.
//

HRESULT CFileStorage::_DoFsDelete(LPCTSTR pszFile)
{
    SHFILEOPSTRUCT fos = { 0 };
    TCHAR szFile[MAX_PATH+1] = { 0 };       // zero-inits, including last character

    StrCpy(szFile, pszFile);

    fos.wFunc = FO_DELETE;
    fos.pFrom = szFile;
    fos.fFlags = FOF_NOCONFIRMATION|FOF_SILENT;

    return SHFileOperation(&fos) ? S_OK:E_FAIL;
}

//
// Do a path combine thunking accordingly
//

void CFileStorage::_GetFullPath(const OLECHAR* pwcsName, LPTSTR pBuffer)
{
#if UNICODE
    PathCombine(_pStoragePath, pwcsName, pBuffer);
#else
    TCHAR szName[MAX_PATH];
    SHUnicodeToTChar(pwcsName, szName, ARRAYSIZE(szName));
    PathCombine(_pStoragePath, szName, pBuffer);
#endif
}


/*-----------------------------------------------------------------------------
/ IUnknown
/----------------------------------------------------------------------------*/

STDMETHODIMP CFileStorage::QueryInterface(REFIID riid, void **ppvObj)
{
    if ( !ppvObj )
        return E_INVALIDARG;

    if ( IsEqualIID(riid, IID_IStorage) || IsEqualIID(riid, IID_IUnknown) )
    {
        *ppvObj = this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    _cRef++;
    return NOERROR;
}

STDMETHODIMP_(ULONG) CFileStorage::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CFileStorage::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


/*-----------------------------------------------------------------------------
/ IStorage
/----------------------------------------------------------------------------*/

STDMETHODIMP CFileStorage::CreateStream(const OLECHAR* pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStream **ppstm)
{
    TCHAR szFullPath[MAX_PATH];

    if ( !ppstm )
        return STG_E_INVALIDPOINTER;

    *ppstm = NULL;

    if ( !pwcsName )
        return STG_E_INVALIDPARAMETER;

    _GetFullPath(pwcsName, szFullPath);
    return SHCreateStreamOnFile(szFullPath, grfMode, ppstm);
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::OpenStream(const OLECHAR* pwcsName, VOID *reserved1, DWORD grfMode, DWORD reserved2, IStream **ppstm)
{
    TCHAR szFullPath[MAX_PATH];

    if ( !ppstm )
        return STG_E_INVALIDPOINTER;

    *ppstm = NULL;

    if ( !pwcsName )
        return STG_E_INVALIDPARAMETER;

    // combine the path strings (our location and the stream, and if they don't specify
    // STGM_CREATE we will fail the call.

    _GetFullPath(pwcsName, szFullPath);

    if ( !(grfMode & STGM_CREATE) )
    {
        if ( -1 == GetFileAttributes(szFullPath) )
            return STG_E_FILENOTFOUND;
    }

    return SHCreateStreamOnFile(szFullPath, grfMode, ppstm);
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::CreateStorage(const OLECHAR *pwcsName, DWORD grfMode, DWORD reserved1, DWORD reserved2, IStorage **ppstg)
{
    return _OpenCreateStorage(pwcsName, grfMode, ppstg, TRUE);
}

STDMETHODIMP CFileStorage::OpenStorage(  const OLECHAR *pwcsName, IStorage *pstgPriority, DWORD grfMode, SNB snbExclude, DWORD reserved, IStorage **ppstg)
{
    return _OpenCreateStorage(pwcsName, grfMode, ppstg, TRUE);
}

HRESULT CFileStorage::_OpenCreateStorage(const OLECHAR *pwcsName, DWORD grfMode, IStorage **ppstg, BOOL fCreate)
{
    TCHAR szFullPath[MAX_PATH];
    DWORD dwAttributes;

    if ( !ppstg )
        return STG_E_INVALIDPOINTER;

    *ppstg = NULL;

    if ( !pwcsName )
        return STG_E_INVALIDPARAMETER;

    if (grfMode &
        ~(STGM_READ             |
          STGM_WRITE            |
          STGM_SHARE_DENY_NONE  |
          STGM_SHARE_DENY_READ  |
          STGM_SHARE_DENY_WRITE |
          STGM_SHARE_EXCLUSIVE  |
          STGM_FAILIFTHERE      |
          STGM_CREATE         ))
    {
        return STG_E_INVALIDPARAMETER;
    }
    
    if ( !fCreate && (grfMode & STGM_FAILIFTHERE) )
        return STG_E_INVALIDPARAMETER;

    // if the storage doesn't exist then lets create it, then drop into the
    // open storage to do the right thing.

    _GetFullPath(pwcsName, szFullPath);
    dwAttributes = GetFileAttributes(szFullPath);

    if ( (DWORD)-1 != dwAttributes )
    {
        // an object exists, we must fail if STGM_FAILIFTHERE is set, or
        // the object that exists is not a directory.  
        //        
        // if the STGM_CREATE flag is set and the object exists we will
        // delete the existing storage.

        if ( grfMode & STGM_FAILIFTHERE )
            return STG_E_FILEALREADYEXISTS;

        if ( !(dwAttributes & FILE_ATTRIBUTE_DIRECTORY) )
            return E_FAIL;

        if ( grfMode & STGM_CREATE )
        {
            HRESULT hres = _DoFsDelete(szFullPath);
            if ( FAILED(hres) )
                return hres;
        }
    }
    else
    {
        // the object doesn't exist, and they have not set the STGM_CREATE, nor
        // is this a ::CreateStorage call.

        if ( !fCreate && !(grfMode & STGM_CREATE) )
            return STG_E_FILENOTFOUND;
    }

    // create a directory (we assume this will always succeed)

    if ( !CreateDirectory(szFullPath, NULL) )
        return E_FAIL;

    *ppstg = new CFileStorage(szFullPath);
    if ( !*ppstg )
        return E_OUTOFMEMORY;

    return S_OK;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::CopyTo(DWORD ciidExclude, const IID *rgiidExclude, SNB snbExclude, IStorage *pstgDest)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::MoveElementTo(const OLECHAR *pwcsName, IStorage *pstgDest, const OLECHAR *pwcsNewName, DWORD grfFlags)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::Commit(DWORD grfCommitFlags)
{
    return S_OK;        // changes are commited as we go, so return S_OK;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::Revert()
{
    return E_NOTIMPL;   // changes are commited as we go, so cannot implement this.
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::EnumElements(DWORD reserved1, void *reserved2, DWORD reserved3, IEnumSTATSTG **ppenum)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::DestroyElement(const OLECHAR *pwcsName)
{
    TCHAR szFullPath[MAX_PATH];

    if ( !pwcsName )
        return STG_E_INVALIDPARAMETER;

    _GetFullPath(pwcsName, szFullPath);

    if ( (DWORD)-1 == GetFileAttributes(szFullPath) )
        return STG_E_FILENOTFOUND;

    return _DoFsDelete(szFullPath);
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::RenameElement(const OLECHAR *pwcsOldName, const OLECHAR *pwcsNewName)
{
    TCHAR szOldPath[MAX_PATH];
    TCHAR szNewPath[MAX_PATH];

    if ( !pwcsOldName || !pwcsNewName )
        return STG_E_INVALIDPARAMETER;

    _GetFullPath(pwcsOldName, szOldPath);
    _GetFullPath(pwcsNewName, szNewPath);

    if ( (DWORD)-1 == GetFileAttributes(szOldPath) )
        return STG_E_FILENOTFOUND;

    if ( (DWORD)-1 != GetFileAttributes(szNewPath) )
        return STG_E_FILEALREADYEXISTS;

    if ( !MoveFile(szOldPath, szNewPath) )
        return E_FAIL;

    return S_OK;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::SetElementTimes(const OLECHAR *pwcsName, const FILETIME *pctime, const FILETIME *patime, const FILETIME *pmtime)
{
    TCHAR szFullPath[MAX_PATH];

    if ( !pwcsName )
        return STG_E_INVALIDPARAMETER;

    _GetFullPath(pwcsName, szFullPath);

    HANDLE hFile = CreateFile(szFullPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        SetFileTime(hFile, pctime, patime, pmtime);
        CloseHandle(hFile);
    }
    else
    {
        return STG_E_FILENOTFOUND;
    }

    return S_OK;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::SetClass(REFCLSID clsid)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::SetStateBits(DWORD grfStateBits, DWORD grfMask)
{
    return E_NOTIMPL;
}

//-----------------------------------------------------------------------------

STDMETHODIMP CFileStorage::Stat(STATSTG *pstatstg, DWORD grfStatFlag)
{
    return E_NOTIMPL;
}


/*-----------------------------------------------------------------------------
/ SHCreateStorateForDirectory
/ ---------------------------
/   Create a storage object for the specified path, returning a suitable
/   IStorage (or error).
/
/ In:
/   pwszPath -> directory
/   grfMode -> flags passed to IStorage::CreateStorage
/   ppstg -> receieves the storage object
/
/ Out:
/   HRESULT
/----------------------------------------------------------------------------*/

STDAPI SHCreateStorageOnDirectory(LPCTSTR pszPath, DWORD grfMode, IStorage **ppstg)
{
    HRESULT hres;
    CFileStorage* pstg;
    TCHAR szName[MAX_PATH];
    WCHAR szDirName[MAX_PATH];
    LPTSTR pDirName;

    StrCpy(szName, pszPath);
    pDirName = PathFindFileName(szName);

    if ( pDirName == szName )
        return E_FAIL;
        
    pDirName[-1] = TEXT('\0');                            // truncate the path, giving szPath with path, pName -> filename

// BUGBUG: if szPath is not a real object this code still uses it, and that is bad

    pstg = new CFileStorage(szName);
    if ( !pstg )
        return E_OUTOFMEMORY;

    SHTCharToUnicode(pDirName, szDirName, ARRAYSIZE(szDirName));
    hres = pstg->CreateStorage(szDirName, grfMode, 0, 0, ppstg);
    pstg->Release();

    return hres;
}
