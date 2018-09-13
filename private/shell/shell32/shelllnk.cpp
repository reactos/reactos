/*++

Copyright (c) 1990-1998,  Microsoft Corporation  All rights reserved.

Module Name:

    shelllnk.cpp

Abstract:

    This module implements the shell link object

Revision History:

    07/20/98    arulk    Created from C source for this object.

--*/
#include "shellprv.h"

#include <shlobjp.h>
#include "shelllnk.h"

extern "C" {

#include "vdate.h"      // For VDATEINPUTBUF
#include "ids.h"        // For String Resource identifiers
#include "pif.h"        // For manipulating PIF files
#include "trayp.h"      // For  WMTRAY_* messages 
#include "views.h"      // For FSIDM_OPENPRN
#include "fstreex.h"    // For SHGetClassFlags ..
#include "os.h"         // For Win32MoveFile ...
#include "lnktrack.h"   // For FindInFolder
#include "util.h"       // For GetMenuIndexForCanonicalVerb
#include "uemapp.h"
#include <filterr.h>
#include "folder.h"
#include "clsobj.h"     // For CFolderShortcut_CreateInstance

#ifdef WINNT
#include "tracker.h"
#endif

}

// BUGBUG:(seanf) This is sleazy - This fn is defined in shlobj.h, but only if urlmon.h
// was included first. Rather than monkey with the include order in
// shellprv.h, we'll duplicate the prototype here, where SOFTDISTINFO
// is now defined.
SHDOCAPI_(DWORD) SoftwareUpdateMessageBox( HWND hWnd,
                                           LPCWSTR szDistUnit,
                                           DWORD dwFlags,
                                           LPSOFTDISTINFO psdi );


// The following strings are used to support the shell link set path hack that
// allows us to bless links for Darwin without exposing stuff from IShellLinkDataList

#define DARWINGUID_TAG TEXT("::{9db1186e-40df-11d1-aa8c-00c04fb67863}:")
#define LOGO3GUID_TAG  TEXT("::{9db1186f-40df-11d1-aa8c-00c04fb67863}:")

// #define TEST_EXTRA_DATA
// this is to make sure the link file format is extensible
#ifdef TEST_EXTRA_DATA
BOOL bTestExtra = FALSE;
#endif // TEST_EXTRA_DATA

#define TF_DEBUGLINKCODE 0x00800000


EXTERN_C BOOL IsFolderShortcut(LPCTSTR pszName);

//-------------------------------------------------------------------------------------
CShellLink::CShellLink()
{
    _cRef = 1;
#ifdef WINNT
    if (g_bRunOnNT5)
        _ptracker = new CTracker( (IShellLink*) this );
#endif
    _ResetPersistData();
}

//-------------------------------------------------------------------------------------
CShellLink::~CShellLink()
{
}

STDMETHODIMP CShellLink::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CShellLink, IShellLinkA),
        QITABENT(CShellLink, IShellLinkW),
        QITABENT(CShellLink, IPersistFile),
        QITABENT(CShellLink, IPersistStream),
        QITABENT(CShellLink, IShellExtInit),
        QITABENTMULTI(CShellLink, IContextMenu, IContextMenu3),
        QITABENTMULTI(CShellLink, IContextMenu2, IContextMenu3),
        QITABENT(CShellLink, IContextMenu3),
        QITABENT(CShellLink, IDropTarget),
        QITABENT(CShellLink, IExtractIconA),
        QITABENT(CShellLink, IExtractIconW),
        QITABENT(CShellLink, IShellLinkDataList),
        QITABENT(CShellLink, IQueryInfo),
        QITABENT(CShellLink, IPersistPropertyBag),
        QITABENT(CShellLink, IObjectWithSite),
        QITABENT(CShellLink, IServiceProvider),
        QITABENT(CShellLink, IFilter),
        { 0 },
    };
    HRESULT hr = QISearch(this, qit, riid, ppvObj);
#ifdef WINNT
    // ISLTracker is a private test interface, and isn't implemented
    // in CShellLink
    if (FAILED(hr) && (IID_ISLTracker == riid))
    {
        *ppvObj = _ptracker;
        AddRef();
        hr = S_OK;
    }
#endif
    return hr;
}

STDMETHODIMP_(ULONG) CShellLink::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

void CShellLink::_ResetPersistData()
{
    if (_pidl)
    {
        ILFree(_pidl);
        _pidl = NULL;
    }

    if (_pli)
    {
        LocalFree((HLOCAL)_pli);
        _pli = NULL;
    }

    Str_SetPtr(&_pszName, NULL);
    Str_SetPtr(&_pszRelPath, NULL);
    Str_SetPtr(&_pszWorkingDir, NULL);
    Str_SetPtr(&_pszArgs, NULL);
    Str_SetPtr(&_pszIconLocation, NULL);

    if (_pExtraData)
    {
        SHFreeDataBlockList(_pExtraData);
        _pExtraData = NULL;
    }

#ifdef WINNT
    if (_ptracker)
        _ptracker->InitNew();
#endif

    // init data members.  all others are zero inited
    memset(&_sld, 0, SIZEOF(_sld));

    _sld.iShowCmd = SW_SHOWNORMAL;
}

//-------------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CShellLink::Release()
{
    if (InterlockedDecrement(&_cRef))
       return _cRef;

    _ResetPersistData();        // free all data

    Str_SetPtr(&_pszCurFile, NULL);
    Str_SetPtr(&_pszRelSource, NULL);

    if (_pcmTarget)
        _pcmTarget->Release();

    if (_pdtSrc)
        _pdtSrc->Release();

    if (_pxi)
        _pxi->Release();

    if (_pxiA)
        _pxiA->Release();

#ifdef WINNT
    if (_ptracker)
        delete _ptracker;
#endif

    delete this;
    return 0;
}

#if 0
void DumpPLI(PCLINKINFO pli)
{
    LPCTSTR p;

    if (!pli)
        return;

    DebugMsg(DM_TRACE, TEXT("DumpPLI:"));

    if (GetLinkInfoData(pli, LIDT_VOLUME_SERIAL_NUMBER, &p))
        DebugMsg(DM_TRACE, TEXT("\tSerial #\t%d"), *p);

    if (GetLinkInfoData(pli, LIDT_DRIVE_TYPE, &p))
        DebugMsg(DM_TRACE, TEXT("\tDrive Type\t%d"), *p);

    if (GetLinkInfoData(pli, LIDT_VOLUME_LABEL, &p))
        DebugMsg(DM_TRACE, TEXT("\tLabel\t%s"), p);

    if (GetLinkInfoData(pli, LIDT_LOCAL_BASE_PATH, &p))
        DebugMsg(DM_TRACE, TEXT("\tBase Path\t%s"), p);

    if (GetLinkInfoData(pli, LIDT_NET_RESOURCE, &p))
        DebugMsg(DM_TRACE, TEXT("\tNet Res\t%s"), p);

    if (GetLinkInfoData(pli, LIDT_COMMON_PATH_SUFFIX, &p))
        DebugMsg(DM_TRACE, TEXT("\tPath Sufix\t%s"), p);
}
#else
#define DumpPLI(p)
#endif

// Compare _sld to a WIN32_FIND_DATA

BOOL CShellLink::IsEqualFindData(const WIN32_FIND_DATA *pfd)
{
    return (pfd->dwFileAttributes == _sld.dwFileAttributes)                       &&
           (CompareFileTime(&pfd->ftCreationTime, &_sld.ftCreationTime) == 0)     &&
           (CompareFileTime(&pfd->ftLastWriteTime, &_sld.ftLastWriteTime) == 0)   &&
           (pfd->nFileSizeLow == _sld.nFileSizeLow);
}

BOOL CShellLink::SetFindData(const WIN32_FIND_DATA *pfd)
{
    if (!IsEqualFindData(pfd))
    {
        _sld.dwFileAttributes = pfd->dwFileAttributes;
        _sld.ftCreationTime = pfd->ftCreationTime;
        _sld.ftLastAccessTime = pfd->ftLastAccessTime;
        _sld.ftLastWriteTime = pfd->ftLastWriteTime;
        _sld.nFileSizeLow = pfd->nFileSizeLow;
        _bDirty = TRUE;
        return TRUE;
    }
    return FALSE;
}

PLINKINFO CopyLinkInfo(PCLINKINFO pcliSrc)
{
    ASSERT(pcliSrc);
    DWORD dwSize = *(UNALIGNED DWORD *)pcliSrc; // size of this thing
    PLINKINFO pli = (PLINKINFO)LocalAlloc(LPTR, dwSize);      // make a copy
    if (pli)
        CopyMemory(pli, pcliSrc, dwSize);
    return  pli;
}

// Creates LinkInfo for a CShellLink instance that does not yet have one
// create the LinkInfo from the pidl (assumed to be to a path) for this link
//
// returns:
//
//      success, pointer to the LINKINFO
//      NULL     this link does not have LINKINFO

PLINKINFO CShellLink::GetLinkInfo()
{
    PLINKINFO pliNew;
    TCHAR szPath[MAX_PATH];

    if ((_pidl == NULL) && (_sld.dwFlags & SLDF_HAS_RELPATH))
    {
        TCHAR szTmp[MAX_PATH];
        _ResolveRelative(szTmp);
    }

    if (!_pidl || !SHGetPathFromIDList(_pidl, szPath))
    {
        DebugMsg(DM_TRACE, TEXT("GetLinkInfo called for non file link"));
        return NULL;
    }

    if (_pli)
    {
        LocalFree((HLOCAL)_pli);
        _pli = NULL;
    }

    // this bit disables LINKINFO tracking on a per link basis, this is set
    // externally by admins to make links more "transparent"
    if (!(_sld.dwFlags & SLDF_FORCE_NO_LINKINFO))
    {
        if (CreateLinkInfo(szPath, &pliNew))
        {
            _pli = CopyLinkInfo(pliNew);
            _bDirty = TRUE;
            DestroyLinkInfo(pliNew);
        }
    }
    return _pli;
}

void PathGetRelative(LPTSTR pszPath, LPCTSTR pszFrom, DWORD dwAttrFrom, LPCTSTR pszRel)
{
    TCHAR szRoot[MAX_PATH];

    lstrcpy(szRoot, pszFrom);
    if (!(dwAttrFrom & FILE_ATTRIBUTE_DIRECTORY))
        PathRemoveFileSpec(szRoot);

    ASSERT(PathIsRelative(pszRel));

    PathCombine(pszPath, szRoot, pszRel);
}

//
// update the working dir to match changes being made to the link target
//
void CShellLink::UpdateWorkingDir(LPCITEMIDLIST pidlNew)
{
    TCHAR szOld[MAX_PATH], szNew[MAX_PATH], szPath[MAX_PATH];

    if (_pszWorkingDir == NULL ||
        _pszWorkingDir[0] == 0 ||
        StrChr(_pszWorkingDir, TEXT('%')) ||
        _pidl == NULL ||
        !SHGetPathFromIDList(_pidl, szOld) ||
        !SHGetPathFromIDList(pidlNew, szNew) ||
        (lstrcmpi(szOld, szNew) == 0))
        return;

    if (PathRelativePathTo(szPath, szOld, _sld.dwFileAttributes, _pszWorkingDir, FILE_ATTRIBUTE_DIRECTORY))
    {
        PathGetRelative(szOld, szNew, GetFileAttributes(szNew), szPath);        // get result is szOld

        if (PathIsDirectory(szOld))
        {
            DebugMsg(DM_TRACE, TEXT("working dir updated to %s"), szOld);
            Str_SetPtr(&_pszWorkingDir, szOld);
            _bDirty = TRUE;
        }
    }
}

// set the pidl either based on a new pidl or a path
// this will set the dirty flag if this info is different from the current
//
// in:
//      pidlNew         if non-null, use as new PIDL for link
//      pszPath         if non-null, create a pidl for this and set it
//      pfdNew          find data for this file (if we already have it)
//
// returns:
//      TRUE            successfully set the pidl (or it was unchanged)
//      FALSE           memory failure or pszPath does not exist

BOOL CShellLink::SetPIDLPath(LPCITEMIDLIST pidlNew, LPCTSTR pszPath, const WIN32_FIND_DATA *pfdNew)
{
    LPITEMIDLIST pidlCreated = NULL;

    if (pszPath && !pidlNew)
    {
        TCHAR szPath[MAX_PATH];
        LPITEMIDLIST pidlDesktop;
        
        // the path is the same as the current pidl, short circuit the disk hits
        if (_pidl && SHGetPathFromIDList(_pidl, szPath) && !lstrcmpi(szPath, pszPath))
            return TRUE;

        pidlDesktop = SHCloneSpecialIDList(NULL, CSIDL_DESKTOPDIRECTORY, TRUE);
        pidlCreated = ILCreateFromPath(pszPath);

        // before we just used to create the pidl from path
        // now we check if pidl is immediate child of a desktop
        // and if it is then we use ILFindLast else we use the
        // fully created pidl
        // this is done because the items on the desktop have pidls
        // relative to the desktop, not the full path
        // if we did not do this and we updated the pidl of e.g. 
        // shortcut (on desktop) to an item (on desktop), the new pidl
        // would be a full path pidl and it would be different from the
        // one of the item it points to which is a bug

        if (pidlDesktop)
        {
            if (pidlCreated && ILIsParent(pidlDesktop, pidlCreated, TRUE))
            {
                LPITEMIDLIST pidlTmp = pidlCreated;

                pidlCreated = ILClone(ILFindLastID(pidlCreated));
                ILFree(pidlTmp);
            }
            ILFree(pidlDesktop);
        }        

        if (pidlCreated == NULL)
        {
            TCHAR achPath[MAX_PATH];
            DebugMsg(DM_TRACE, TEXT("Failed to create pidl for link (trying simple PIDL)"));
            lstrcpy(achPath, pszPath);
            PathResolve(achPath, NULL, PRF_TRYPROGRAMEXTENSIONS);
            pidlCreated = SHSimpleIDListFromPath(achPath);
        }

        if (!pidlCreated)
        {
            DebugMsg(DM_TRACE, TEXT("Failed to create pidl for link"));
            return FALSE;
        }
        pidlNew = pidlCreated;
    }

    if (!pidlNew)
    {
        if (_pidl)
        {
            ILFree(_pidl);
            _pidl = NULL;
            _bDirty = TRUE;
        }
        if (_pli)
        {
            LocalFree((HLOCAL)_pli);
            _pli = NULL;
        }
        memset((void *)&_sld, 0, SIZEOF(_sld));
    }
    else
    {
        // NOTE: this can result in an asser in the fs compare items if the 2 pidls
        // are both simple.  but since this is just an optimization to avoid resetting
        // the pidl if it is the same that can be ignored.

        if (!_pidl || !ILIsEqual(_pidl, pidlNew))
        {
            /* Yes.  Save updated path IDL. */
            LPITEMIDLIST pidlClone = ILClone(pidlNew);
            if (pidlClone)
            {
                UpdateWorkingDir(pidlClone);

                if (_pidl)
                    ILFree(_pidl);

                _pidl = pidlClone;

                // we are modifing _pidl.  We should never have a EXP_SPECIAL_FOLDER_SIG section
                // when we modify the _pidl, otherwise the two will be out of ssync
                ASSERT( NULL == SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG) );

                GetLinkInfo();      // construct the LinkInfo (pli)

                DumpPLI(_pli);
            }
            else
            {
                DebugMsg(DM_TRACE, TEXT("SetPIDLPath ILClone failed"));
                return FALSE;
            }
            _bDirty = TRUE;
        }

        if (pfdNew)
            SetFindData(pfdNew); // Win9x only
    }

    if (pidlCreated)
        ILFree(pidlCreated);

    return TRUE;
}

// sees if this link might have a relative path
//
//
// out:
//      pszPath returned new path
//
// returns:
//      TRUE    we found a relative path and it exists
//      FALSE   outa luck.
//

BOOL CShellLink::_ResolveRelative(LPTSTR pszPath)
{
    LPCTSTR pszPathRel;
    TCHAR szRoot[MAX_PATH];

    // pszRelSource overrides pszCurFile

    pszPathRel = _pszRelSource ? _pszRelSource : _pszCurFile;

    if (pszPathRel == NULL || _pszRelPath == NULL)
        return FALSE;

    lstrcpy(szRoot, pszPathRel);
    PathRemoveFileSpec(szRoot);         // pszfrom is a file (not a directory)

    PathCombine(pszPath, szRoot, _pszRelPath);

    if (PathFileExistsAndAttributes(pszPath, NULL))
    {
        DebugMsg(DM_TRACE, TEXT("_ResolveRelative() returning %s"), pszPath);
        return SetPIDLPath(NULL, pszPath, NULL);
    }
    return FALSE;
}

void CShellLink::GetFindData(WIN32_FIND_DATA *pfd)
{
    TCHAR szPath[MAX_PATH];

    pfd->dwFileAttributes = _sld.dwFileAttributes;
    pfd->ftCreationTime = _sld.ftCreationTime;
    pfd->ftLastAccessTime = _sld.ftLastAccessTime;
    pfd->ftLastWriteTime = _sld.ftLastWriteTime;
    pfd->nFileSizeLow = _sld.nFileSizeLow;
    pfd->nFileSizeHigh = 0;
    SHGetPathFromIDList(_pidl, szPath);

    // no one should call this on a pidl without a path
    ASSERT(szPath[0]);

    lstrcpy(pfd->cFileName, PathFindFileName(szPath));
}

STDMETHODIMP CShellLink::GetPath(LPWSTR pszFile, int cchMaxPath, WIN32_FIND_DATAW *pfd, DWORD fFlags)
{
    TCHAR szPath[MAX_PATH];
    VDATEINPUTBUF(pszFile, TCHAR, cchMaxPath);

    DumpPLI(_pli);

    // For darwin enabled links, we do NOT want to have to go and call         
    // ParseDarwinID here because that could possible force the app to install.
    // So, instead we return the path to the icon as the path for darwin enable
    // shortcuts. This allows the icon to be correct and since the darwin icon 
    // will always be an .exe, ensuring that the context menu will be correct. 
    if (_sld.dwFlags & SLDF_HAS_DARWINID)                                 
    {                                                                          
        if (_pszIconLocation && _pszIconLocation[0])                 
        {                                                                      
            SHExpandEnvironmentStrings(_pszIconLocation, szPath, ARRAYSIZE(szPath));
            SHTCharToUnicode(szPath, pszFile, cchMaxPath);                             
            return S_OK;                                                       
        }                                                                      
        else                                                                   
        {                                                                      
            // we should never have a darwin link that does not have           
            // an icon path.                                                   
            ASSERT(FALSE);                                                     
            return S_FALSE;                                                    
        }                                                                      
    }                                                                                                                                                   

    if ((_pidl == NULL) && (_sld.dwFlags & SLDF_HAS_RELPATH))
    {
        TCHAR szTmp[MAX_PATH];
        _ResolveRelative(szTmp);
    }

    if (!_pidl || !SHGetPathFromIDListEx(_pidl, szPath, (fFlags & SLGP_SHORTPATH) ? GPFIDL_ALTNAME : 0))
        szPath[0] = 0;

    if ((_sld.dwFlags & SLDF_HAS_EXP_SZ) && (fFlags & SLGP_RAWPATH))
    {
        // Special case where we grab the Target name from
        // the extra data section of the link rather than from
        // the pidl.  We do this after we grab the name from the pidl
        // so that if we fail, then there is still some hope that a
        // name can be returned.

        LPEXP_SZ_LINK pszl = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_LINK_SIG);
        if (pszl)
        {
            SHUnicodeToTChar(pszl->swzTarget, szPath, ARRAYSIZE(szPath));
            DebugMsg( DM_TRACE, TEXT("CShellLink::GetPath() %s (from xtra data)"), szPath );
        }
    }

    if (pszFile)
    {
        SHTCharToUnicode(szPath, pszFile, cchMaxPath);
    }

    if (pfd)
    {
        memset(pfd, 0, SIZEOF(*pfd));
        if (szPath[0])
        {
            pfd->dwFileAttributes = _sld.dwFileAttributes;
            pfd->ftCreationTime = _sld.ftCreationTime;
            pfd->ftLastAccessTime = _sld.ftLastAccessTime;
            pfd->ftLastWriteTime = _sld.ftLastWriteTime;
            pfd->nFileSizeLow = _sld.nFileSizeLow;
            SHTCharToUnicode(PathFindFileName(szPath), pfd->cFileName, ARRAYSIZE(pfd->cFileName));
        }
    }

    return (_pidl != NULL && szPath[0] == 0) ? S_FALSE : S_OK;
}

STDMETHODIMP CShellLink::GetIDList(LPITEMIDLIST *ppidl)
{
    if (_pidl)
        return SHILClone(_pidl, ppidl);

    *ppidl = NULL;
    return S_FALSE;     // success but empty
}

#ifdef DEBUG

#define DumpTimes(ftCreate, ftAccessed, ftWrite) \
    DebugMsg(DM_TRACE, TEXT("create   %8x%8x"), ftCreate.dwLowDateTime,   ftCreate.dwHighDateTime);     \
    DebugMsg(DM_TRACE, TEXT("accessed %8x%8x"), ftAccessed.dwLowDateTime, ftAccessed.dwHighDateTime);   \
    DebugMsg(DM_TRACE, TEXT("write    %8x%8x"), ftWrite.dwLowDateTime,    ftWrite.dwHighDateTime);

#else

#define DumpTimes(ftCreate, ftAccessed, ftWrite)

#endif

void CheckAndFixNullCreateTime(LPCTSTR pszFile, FILETIME *pftCreationTime, const FILETIME *pftLastWriteTime)
{
    if (IsNullTime(pftCreationTime))
    {
        HANDLE hfile = CreateFile(pszFile, GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_EXISTING, 0, NULL);

        DebugMsg(DM_TRACE, TEXT("NULL create time"));
        // this file has a bogus create time, set it to the last accessed time

        if (hfile != INVALID_HANDLE_VALUE)
        {
            DebugMsg(DM_TRACE, TEXT("pfd times"));

            DebugMsg(DM_TRACE, TEXT("create   %8x%8x"), pftCreationTime->dwLowDateTime, pftCreationTime->dwHighDateTime);

            if (SetFileTime(hfile, pftLastWriteTime, NULL, NULL))
            {
                // BUGBUG: get the time back to make sure we match the precision of the file system
                *pftCreationTime = *pftLastWriteTime;     // patch this up
#ifdef DEBUG
                {
                    FILETIME ftCreate, ftAccessed, ftWrite;
                    GetFileTime((HANDLE)hfile, &ftCreate, &ftAccessed, &ftWrite);
                    AssertMsg(CompareFileTime(&ftCreate, pftCreationTime) == 0, TEXT("create times don't match"));
                    DumpTimes(ftCreate, ftAccessed, ftWrite);
                }
#endif
            }
            else
            {
                DebugMsg(DM_TRACE, TEXT("unable to set create time"));
            }
            CloseHandle(hfile);
        }
    }
}

// Compare _sld to a BY_HANDLE_FILE_INFORMATION
BOOL CShellLink::IsEqualFileInfo(const BY_HANDLE_FILE_INFORMATION *pFileInfo)
{
    return (pFileInfo->dwFileAttributes == _sld.dwFileAttributes)                       &&
           (CompareFileTime(&pFileInfo->ftCreationTime, &_sld.ftCreationTime) == 0)     &&
           (CompareFileTime(&pFileInfo->ftLastWriteTime, &_sld.ftLastWriteTime) == 0)   &&
           (pFileInfo->nFileSizeLow == _sld.nFileSizeLow);
}

//
// Query the file/directory at pszPath for it's file attributes
// and CTracker information.  Use this info to update sld
// and ptracker.
//

BOOL CShellLink::QueryAndSetFindData(LPCTSTR pszPath)
{
#ifdef WINNT
    // Open the file or directory.  We have to set FILE_FLAG_BACKUP_SEMANTICS
    // to get CreateFile to give us directory handles.

    BOOL bRes = FALSE;
    HANDLE hFile = CreateFile(pszPath, FILE_READ_ATTRIBUTES,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (INVALID_HANDLE_VALUE != hFile)
    {
        // Get the file attributes
        BY_HANDLE_FILE_INFORMATION FileInfo;

        if (GetFileInformationByHandle(hFile, &FileInfo))
        {
            bRes = TRUE;

            // If this file doesn't have a create time for some reason, set it to be the
            // current last-write time.

            CheckAndFixNullCreateTime(pszPath, &FileInfo.ftCreationTime, &FileInfo.ftLastWriteTime);

            // If the attributes are different that what we have cached, then keep
            // the new values.

            if (!IsEqualFileInfo(&FileInfo))
            {
                _sld.dwFileAttributes = FileInfo.dwFileAttributes;
                _sld.ftCreationTime = FileInfo.ftCreationTime;
                _sld.ftLastAccessTime = FileInfo.ftLastAccessTime;
                _sld.ftLastWriteTime = FileInfo.ftLastWriteTime;
                _sld.nFileSizeLow = FileInfo.nFileSizeLow;

                _bDirty = TRUE;
            }
            
            // save the Object IDs as well.
            if (_ptracker)
            {
                if (SUCCEEDED(_ptracker->InitFromHandle( hFile, pszPath)))
                {
                    if (_ptracker->IsDirty())
                        _bDirty = TRUE;
                }
                else
                {
                    //Save space in the .lnk file
                    _ptracker->InitNew();
                }
            }
        }
        else
        {
            DebugMsg( DM_ERROR, TEXT("Couldn't get file info by handle (%d)"), GetLastError() );
        }
        CloseHandle(hFile);
    }
    else
    {
        DebugMsg( DM_TRACE, TEXT("QueryAndSetFindData couldn't open \"%s\" (%08X)"), pszPath, GetLastError() );
    }
    return bRes;

#else // #ifdef WINNT
    // Win95 does not support CreateFile() on folders!

    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(pszPath, &fd);

    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;

    FindClose(hFind);
    CheckAndFixNullCreateTime(pszPath, &fd.ftCreationTime, &fd.ftLastWriteTime);
    SetFindData(&fd);
    return TRUE;
#endif // #ifdef WINNT ... #else
}

STDMETHODIMP CShellLink::SetIDList(LPCITEMIDLIST pidlnew)
{
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];

    if (pidlnew != _pidl)
        SetPIDLPath(pidlnew, NULL, NULL);

    // is this a pidl to a file?
    if (_pidl && SHGetPathFromIDList(_pidl, szPath))
    {
        // DebugMsg(DM_TRACE, "ShellLink::SetIDList(%s)", szPath);

        if (PathIsRoot(szPath))
        {
            memset(&_sld, 0, SIZEOF(_sld));
            _sld.dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        }
        else
        {
            if( !QueryAndSetFindData(szPath ) )
                hr = S_FALSE;
        }
    }

    return hr;
}

BOOL DifferentStrings(LPCTSTR psz1, LPCTSTR psz2)
{
    if (psz1 && psz2)
        return lstrcmp(psz1, psz2);
    else
        return (!psz1 && psz2) || (psz1 && !psz2);
}

// NOTE: NULL string ptr is valid argument for this function

HRESULT CShellLink::_SetField(LPTSTR *ppszField, LPCWSTR pszValueW)
{
    TCHAR szValue[INFOTIPSIZE], *pszValue;

    if (pszValueW)
    {
        SHUnicodeToTChar(pszValueW, szValue, ARRAYSIZE(szValue));
        pszValue = szValue;
    }
    else
        pszValue = NULL;

    if (DifferentStrings(*ppszField, pszValue))
        _bDirty = TRUE;

    Str_SetPtr(ppszField, pszValue);
    return S_OK;
}

HRESULT CShellLink::_SetField(LPTSTR *ppszField, LPCSTR pszValueA)
{
    TCHAR szValue[INFOTIPSIZE], *pszValue;

    if (pszValueA)
    {
        SHAnsiToTChar(pszValueA, szValue, ARRAYSIZE(szValue));
        pszValue = szValue;
    }
    else
        pszValue = NULL;

    if (DifferentStrings(*ppszField, pszValue))
        _bDirty = TRUE;

    Str_SetPtr(ppszField, pszValue);
    return S_OK;
}


HRESULT CShellLink::_GetField(LPCTSTR pszField, LPWSTR pszValue, int cchValue)
{
    if (pszField == NULL)
        *pszValue = 0;
    else
    {
        SHTCharToUnicode(pszField, pszValue, cchValue);
    }
    return S_OK;
}

HRESULT CShellLink::_GetField(LPCTSTR pszField, LPSTR pszValue, int cchValue)
{
    if (pszField == NULL)
        *pszValue = 0;
    else
    {
        SHTCharToAnsi(pszField, pszValue, cchValue);
    }
    return S_OK;
}

//  order is important
const int c_rgcsidlUserFolders[] = {
    CSIDL_MYPICTURES | TEST_SUBFOLDER,
    CSIDL_PERSONAL | TEST_SUBFOLDER,
    CSIDL_DESKTOPDIRECTORY | TEST_SUBFOLDER,
    CSIDL_COMMON_DESKTOPDIRECTORY | TEST_SUBFOLDER,
};

STDAPI_(void) SHMakeDescription(LPCITEMIDLIST pidlDesc, int ids, LPTSTR pszDesc, UINT cch)
{
    LPCITEMIDLIST pidlName = pidlDesc;
    TCHAR szPath[MAX_PATH], szFormat[64];
    DWORD gdn;

    ASSERT(pidlDesc);
    
    //
    //  we want to only show the INFOLDER name for 
    //  folders the user sees often.  so in the desktop
    //  or mydocs or mypics we just show that name.
    //  otherwise show the whole path.
    //
    //  NOTE - there can be some weirdness if you start making
    //  shortcuts to special folders off the desktop
    //  specifically if you make a shortcut to mydocs the comment
    //  ends up being %USERPROFILE%, but this is a rare enough 
    //  case that i dont think we need to worry too much.
    //
    SHGetNameAndFlags(pidlDesc, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL);
    int csidl = GetSpecialFolderID(szPath, c_rgcsidlUserFolders, ARRAYSIZE(c_rgcsidlUserFolders));
    if (-1 != csidl)
    {
        gdn = SHGDN_INFOLDER   | SHGDN_FORADDRESSBAR;
        switch (csidl)
        {
        case CSIDL_DESKTOPDIRECTORY:
        case CSIDL_COMMON_DESKTOPDIRECTORY:
            {
                ULONG cb;
                if (csidl == GetSpecialFolderParentIDAndOffset(pidlDesc, &cb))
                {
                    //  reorient based off the desktop.
                    pidlName = (LPCITEMIDLIST)(((BYTE *)pidlDesc) + cb);
                }
            }
            break;

        case CSIDL_PERSONAL:
            if (SUCCEEDED(GetMyDocumentsDisplayName(szPath, ARRAYSIZE(szPath))))
                pidlName = NULL;
            break;

        default:
            break;
        }
    }
    else
        gdn = SHGDN_FORPARSING | SHGDN_FORADDRESSBAR;

    if (pidlName)
        SHGetNameAndFlags(pidlName, gdn, szPath, ARRAYSIZE(szPath), NULL);

#if 0       //  if we ever want to handle frienly URL comments
    if (UrlIs(pszPath, URLIS_URL))
    {
        DWORD cchPath = SIZECHARS(szPath);
        if (FAILED(UrlCombine(pszPath, TEXT(""), szPath, &cchPath, 0)))
        {
            // if the URL is too big, then just use the hostname...
            cchPath = SIZECHARS(szPath);
            UrlCombine(pszPath, TEXT("/"), szPath, &cchPath, 0);
        }
    }
#endif

    if (ids != -1)
    {
        LoadString(HINST_THISDLL, ids, szFormat, ARRAYSIZE(szFormat));

        wnsprintf(pszDesc, cch, szFormat, szPath);
    }
    else
        StrCpyN(pszDesc, szPath, cch);
}

void _MakeDescription(LPCITEMIDLIST pidlTo, LPTSTR pszDesc, UINT cch)
{
    LPITEMIDLIST pidlParent = ILCloneParent(pidlTo);
    if (pidlParent)
    {
        SHMakeDescription(pidlParent, IDS_LOCATION, pszDesc, cch);
        ILFree(pidlParent);
    }
    else
        *pszDesc = 0;
}

STDMETHODIMP CShellLink::GetDescription(LPWSTR pszDesc, int cchMax)
{
    return _GetField(_pszName, pszDesc, cchMax);
}

STDMETHODIMP CShellLink::GetDescription(LPSTR pszDesc, int cchMax)
{
    return _GetField(_pszName, pszDesc, cchMax);
}

STDMETHODIMP CShellLink::SetDescription(LPCWSTR pszDesc)
{
    return _SetField(&_pszName, pszDesc);
}

STDMETHODIMP CShellLink::SetDescription(LPCSTR pszDesc)
{
    return _SetField(&_pszName, pszDesc);
}

STDMETHODIMP CShellLink::GetWorkingDirectory(LPWSTR pszDir, int cchDir)
{
    return _GetField(_pszWorkingDir, pszDir, cchDir);
}

STDMETHODIMP CShellLink::GetWorkingDirectory(LPSTR pszDir, int cchMaxPath)
{
    return _GetField(_pszWorkingDir, pszDir, cchMaxPath);
}

STDMETHODIMP CShellLink::SetWorkingDirectory(LPCWSTR pszWorkingDir)
{
    return _SetField(&_pszWorkingDir, pszWorkingDir);
}

STDMETHODIMP CShellLink::SetWorkingDirectory(LPCSTR pszDir)
{
    return _SetField(&_pszWorkingDir, pszDir);
}

STDMETHODIMP CShellLink::GetArguments(LPWSTR pszArgs, int cchArgs)
{
    return _GetField(_pszArgs, pszArgs, cchArgs);
}

STDMETHODIMP CShellLink::GetArguments(LPSTR pszArgs, int cchMaxPath)
{
    return _GetField(_pszArgs, pszArgs, cchMaxPath);
}

STDMETHODIMP CShellLink::SetArguments(LPCWSTR pszArgs)
{
    return _SetField(&_pszArgs, pszArgs);
}

STDMETHODIMP CShellLink::SetArguments(LPCSTR pszArgs)
{
    return _SetField(&_pszArgs, pszArgs);
}

STDMETHODIMP CShellLink::GetHotkey(WORD *pwHotkey)
{
    *pwHotkey = _sld.wHotkey;
    return S_OK;
}

STDMETHODIMP CShellLink::SetHotkey(WORD wHotkey)
{
    if (_sld.wHotkey != wHotkey)
    {
        _bDirty = TRUE;
        _sld.wHotkey = wHotkey;
    }
    return S_OK;
}

STDMETHODIMP CShellLink::GetShowCmd(int *piShowCmd)
{
    *piShowCmd = _sld.iShowCmd;
    return S_OK;
}

STDMETHODIMP CShellLink::SetShowCmd(int iShowCmd)
{
    if (_sld.iShowCmd != iShowCmd)
        _bDirty = TRUE;
    _sld.iShowCmd = iShowCmd;
    return S_OK;
}

// IShellLinkW::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(LPWSTR pszIconPath, int cchIconPath, int *piIcon)
{
    VDATEINPUTBUF(pszIconPath, TCHAR, cchIconPath);
    BOOL fFound = FALSE;

    if (_sld.dwFlags & SLDF_HAS_EXP_ICON_SZ)
    {
        LPEXP_SZ_LINK pszl = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_ICON_SIG);
        if (pszl)
        {
            SHExpandEnvironmentStringsW(pszl->swzTarget, pszIconPath, cchIconPath);
            fFound = (pszIconPath[0] != 0);
        }
    }

    if (!fFound)
    {
        _GetField(_pszIconLocation, pszIconPath, cchIconPath);
    }
    *piIcon = _sld.iIcon;
    return S_OK;
}

// IShellLinkA::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(LPSTR pszPath, int cchMaxPath, int *piIcon)
{
    WCHAR szPath[MAX_PATH];
    HRESULT hr = GetIconLocation(szPath, ARRAYSIZE(szPath), piIcon);
    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(szPath, pszPath, cchMaxPath);
    return hr;
}


// IShellLinkW::SetIconLocation
// NOTE: 
//      pszIconPath may be NULL

STDMETHODIMP CShellLink::SetIconLocation(LPCWSTR pszIconPath, int iIcon)
{
    TCHAR szIconPath[MAX_PATH];

    if (pszIconPath)
        SHUnicodeToTChar(pszIconPath, szIconPath, ARRAYSIZE(szIconPath));

    if (pszIconPath)
    {
        HANDLE  hToken;
        TCHAR   szIconPathEnc[MAX_PATH];

        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY | TOKEN_IMPERSONATE, TRUE, &hToken) == FALSE)
        {
            hToken = NULL;
        }
        if (PathUnExpandEnvStringsForUser(hToken, szIconPath, szIconPathEnc, ARRAYSIZE(szIconPathEnc)) != 0)
        {
            EXP_SZ_LINK expLink;

            // mark that link has expandable strings, and add them
            _sld.dwFlags |= SLDF_HAS_EXP_ICON_SZ; // should this be unique for icons?

            LPEXP_SZ_LINK lpNew = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_ICON_SIG);
            if (!lpNew) 
            {
                lpNew = &expLink;
                expLink.cbSize = 0;
                expLink.dwSignature = EXP_SZ_ICON_SIG;
            }

            // store both A and W version (for no good reason!)
            SHTCharToAnsi(szIconPathEnc, lpNew->szTarget, ARRAYSIZE(lpNew->szTarget));
            SHTCharToUnicode(szIconPathEnc, lpNew->swzTarget, ARRAYSIZE(lpNew->swzTarget));

            // See if this is a new entry that we need to add
            if (lpNew->cbSize == 0)
            {
                lpNew->cbSize = SIZEOF(*lpNew);
                _AddExtraDataSection((DATABLOCK_HEADER *)lpNew);
            }
        }
        else 
        {
            _sld.dwFlags &= ~SLDF_HAS_EXP_ICON_SZ;
            _RemoveExtraDataSection(EXP_SZ_ICON_SIG);
        }
        if (hToken != NULL)
        {
            CloseHandle(hToken);
        }
    }

    _SetField(&_pszIconLocation, pszIconPath);

    if (_sld.iIcon != iIcon)
    {
        _sld.iIcon = iIcon;
        _bDirty = TRUE;
    }

    if ((_sld.dwFlags & SLDF_HAS_DARWINID) && pszIconPath)
    {
        // NOTE: The comment below is for darwin as it shipped in win98/IE4.01,
        // and is fixed in the > NT5 versions of the shell's darwin implementation.
        //
        // for darwin enalbed links, we make the path point to the
        // icon location (which is must be of the type (ie same ext) as the real
        // destination. So, if I want a darwin link to readme.txt, the shell
        // needs the icon to be icon1.txt, which is lame!!. This ensures
        // that the context menu will be correct and allows us to return
        // from CShellLink::GetPath & CShellLink::GetIDList without faulting the 
        // application in because we lie to people and tell them that we 
        // really point to our icon, which is the same type as the real target,
        // thus making our context menu be correct.
        SetPIDLPath(NULL, szIconPath, NULL);
    }

    return S_OK;
}

// IShellLinkA::SetIconLocation
STDMETHODIMP CShellLink::SetIconLocation(LPCSTR pszPath, int iIcon)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pszPathW;

    if (pszPath)
    {
        SHAnsiToUnicode(pszPath, szPath, ARRAYSIZE(szPath));
        pszPathW = szPath;
    }
    else
        pszPathW = NULL;

    return SetIconLocation(pszPathW, iIcon);
}

// set the relative path, this is used before a link is saved so we know what
// we should use to store the link relative to as well as before the link is resolved
// so we know the new path to use with the saved relative path.
//
// in:
//      pszPathRel      path to make link target relative to, must be a path to
//                      a file, not a directory.
//
//      dwReserved      must be 0
//
// returns:
//      S_OK            relative path is set
//

STDMETHODIMP CShellLink::SetRelativePath(LPCWSTR pszPathRel, DWORD dwRes)
{
    if (dwRes != 0)
        return E_INVALIDARG;
    return _SetField(&_pszRelSource, pszPathRel);
}

STDMETHODIMP CShellLink::SetRelativePath(LPCSTR pszPathRel, DWORD dwRes)
{
    if (dwRes != 0)
        return E_INVALIDARG;
    return _SetField(&_pszRelSource, pszPathRel);
}

//
//  If SLR_UPDATE isn't set, check IPersistFile::IsDirty after
//  calling this to see if the link info has changed.
//

STDMETHODIMP CShellLink::Resolve(HWND hwnd, DWORD fFlags)
{
    return ResolveLink(hwnd, fFlags, 0);
}

//    converts version in text format (a,b,c,d) into two dwords (a,b), (c,d)
//    The printed version number is of format a.b.d (but, we don't care)
//    NOTE: Stolen from inet\urlmon\download\helpers.cxx
HRESULT GetVersionFromString(TCHAR *szBuf, DWORD *pdwFileVersionMS, DWORD *pdwFileVersionLS)
{
    const TCHAR *pch = szBuf;
    TCHAR ch;
    USHORT n = 0;
    USHORT a = 0;
    USHORT b = 0;
    USHORT c = 0;
    USHORT d = 0;

    enum HAVE { HAVE_NONE, HAVE_A, HAVE_B, HAVE_C, HAVE_D } have = HAVE_NONE;

    *pdwFileVersionMS = 0;
    *pdwFileVersionLS = 0;

    if (!pch)            // default to zero if none provided
        return S_OK;

    if (lstrcmp(pch, TEXT("-1,-1,-1,-1")) == 0) {
        *pdwFileVersionMS = 0xffffffff;
        *pdwFileVersionLS = 0xffffffff;
        return S_OK;
    }

    for (ch = *pch++;;ch = *pch++) {

        if ((ch == ',') || (ch == '\0')) {

            switch (have) {

            case HAVE_NONE:
                a = n;
                have = HAVE_A;
                break;

            case HAVE_A:
                b = n;
                have = HAVE_B;
                break;

            case HAVE_B:
                c = n;
                have = HAVE_C;
                break;

            case HAVE_C:
                d = n;
                have = HAVE_D;
                break;

            case HAVE_D:
                return E_INVALIDARG; // invalid arg
            }

            if (ch == '\0') {
                // all done convert a,b,c,d into two dwords of version

                *pdwFileVersionMS = ((a << 16)|b);
                *pdwFileVersionLS = ((c << 16)|d);

                return S_OK;
            }

            n = 0; // reset

        } else if ( (ch < '0') || (ch > '9'))
            return E_INVALIDARG;    // invalid arg
        else
            n = n*10 + (ch - '0');


    } /* end forever */

    // NEVERREACHED
}

//  Purpose:    A ResolveLink-time check to see if the link has
//              Logo3 application channel
//
//  Inputs:     [LPCTSTR] - pszLogo3ID - the id/keyname for
//                          our Logo3 software.
//
//  Outputs:    [BOOL]
//                  -   TRUE if our peek at the registry
//                      indicates we have an ad to show
//                  -   FALSE indicates no new version
//                      to advertise.
//
//  Algorithm:  Check the software update registry info for the
//              ID embedded in the link. This is a sleazy hack
//              to avoid loading shdocvw and urlmon, which are
//              the normal code path for this check.
//              NOTE: The version checking logic is stolen from
//              shell\shdocvw\sftupmb.cpp

HRESULT GetLogo3SoftwareUpdateInfo( LPCTSTR pszLogo3ID, LPSOFTDISTINFO psdi )
{    
    HRESULT     hr = S_OK;
    HKEY        hkeyDistInfo = 0;
    HKEY        hkeyAvail = 0;
    HKEY        hkeyAdvertisedVersion = 0;
    DWORD       lResult = 0;
    DWORD       dwSize = 0;
    DWORD       dwType;
    TCHAR       szBuffer[MAX_PATH];
    TCHAR       szVersionBuf[MAX_PATH];
    DWORD       dwLen = 0;
    DWORD       dwCurAdvMS = 0;
    DWORD       dwCurAdvLS = 0;

    wsprintf(szBuffer,
             TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s"),
             pszLogo3ID);

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, szBuffer, 0, KEY_READ,
                     &hkeyDistInfo) != ERROR_SUCCESS) {
        hr = E_FAIL;
        goto Exit;
    }
    
    if (RegOpenKeyEx(hkeyDistInfo, TEXT("AvailableVersion"), 0, KEY_READ,
                     &hkeyAvail) != ERROR_SUCCESS) {
        hr = E_FAIL;
        goto Exit;
    }

    dwSize = sizeof(DWORD);
    if (SHQueryValueEx(hkeyAvail, TEXT("Precache"), 0, &dwType,
                        (unsigned char *)&lResult, &dwSize) == ERROR_SUCCESS) {
        // Precached value was the code download HR
        if ( lResult == S_OK )
            psdi->dwFlags = SOFTDIST_FLAG_USAGE_PRECACHE;
    }


    dwSize = MAX_PATH;
    if (SHQueryValueEx(hkeyAvail, TEXT("AdvertisedVersion"), NULL, &dwType, 
                        (unsigned char *)szVersionBuf, &dwSize) == ERROR_SUCCESS)
    {
        GetVersionFromString(szVersionBuf, &psdi->dwAdvertisedVersionMS, &psdi->dwAdvertisedVersionLS);
       // Get the AdState, if any
        dwSize = sizeof(DWORD);
        SHQueryValueEx(hkeyAvail, TEXT("AdState"), NULL, NULL, (LPBYTE)&psdi->dwAdState, &dwSize);
    }
 


    dwSize = MAX_PATH;
    if (SHQueryValueEx(hkeyAvail, NULL, NULL, &dwType,
                        (unsigned char *)szVersionBuf, &dwSize) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    if ( FAILED(GetVersionFromString(szVersionBuf, &psdi->dwUpdateVersionMS, &psdi->dwUpdateVersionLS))){
        hr = S_FALSE;
        goto Exit;
    }

 
    dwLen = sizeof(DWORD);
    if (SHQueryValueEx(hkeyDistInfo, TEXT("VersionMajor"), 0, &dwType,
                        (LPBYTE)&psdi->dwInstalledVersionMS, &dwLen) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

    dwLen = sizeof(DWORD);
    if (SHQueryValueEx(hkeyDistInfo, TEXT("VersionMinor"), 0, &dwType,
                        (LPBYTE)&psdi->dwInstalledVersionLS, &dwLen) != ERROR_SUCCESS) {
        hr = S_FALSE;
        goto Exit;
    }

        // BUGBUG: SeanF needs to review new logic
    if (psdi->dwUpdateVersionMS > psdi->dwInstalledVersionMS ||
        (psdi->dwUpdateVersionMS == psdi->dwInstalledVersionMS &&
         psdi->dwUpdateVersionLS > psdi->dwInstalledVersionLS)) {
        hr = S_OK;
    } else {
        hr = S_FALSE;
    }

Exit:
    if (hkeyAdvertisedVersion) {
        RegCloseKey(hkeyAdvertisedVersion);
    }

    if (hkeyAvail) {
        RegCloseKey(hkeyAvail);
    }

    if (hkeyDistInfo) {
        RegCloseKey(hkeyDistInfo);
    }

    return hr;
}

//  Purpose:    A ResolveLink-time check to see if the link has
//              Logo3 application channel
//
//  Inputs:     [LPCTSTR] - pszLogo3ID - the id/keyname for
//                          our Logo3 software.
//
//  Outputs:    [BOOL]
//                  -   TRUE if our peek at the registry
//                      indicates we have an ad to show
//                  -   FALSE indicates no new version
//                      to advertise.
//
//  Algorithm:  Check the software update registry info for the
//              ID embedded in the link. This is a sleazy hack
//              to avoid loading shdocvw and urlmon, which are
//              the normal code path for this check.
//              The version checking logic is stolen from

BOOL FLogo3RegPeek( LPCTSTR pszLogo3ID )
{
    BOOL            bHaveAd = FALSE;
    HRESULT         hr;
    SOFTDISTINFO    sdi = { 0 };
    DWORD           dwAdStateNew = SOFTDIST_ADSTATE_NONE;

    hr = GetLogo3SoftwareUpdateInfo( pszLogo3ID, &sdi );
 
    // we need an HREF to work properly. The title and abstract are negotiable.
    if ( SUCCEEDED(hr) )
    {
        // see if this is an update the user already knows about.
        // If it is, then skip the dialog.
        if (  (sdi.dwUpdateVersionMS >= sdi.dwInstalledVersionMS ||
                (sdi.dwUpdateVersionMS == sdi.dwInstalledVersionMS &&
                 sdi.dwUpdateVersionLS >= sdi.dwInstalledVersionLS))    && 
              (sdi.dwUpdateVersionMS >= sdi.dwAdvertisedVersionMS ||
                (sdi.dwUpdateVersionMS == sdi.dwAdvertisedVersionMS &&
                 sdi.dwUpdateVersionLS >= sdi.dwAdvertisedVersionLS)) )
        { 
            if ( hr == S_OK ) // new version
            {
                // we have a pending update, either on the net, or downloaded
                if ( sdi.dwFlags & SOFTDIST_FLAG_USAGE_PRECACHE )
                {
                    dwAdStateNew = SOFTDIST_ADSTATE_DOWNLOADED;
                }
                else
                {
                    dwAdStateNew = SOFTDIST_ADSTATE_AVAILABLE;
                }
            }
            else if ( sdi.dwUpdateVersionMS == sdi.dwInstalledVersionMS &&
                      sdi.dwUpdateVersionLS == sdi.dwInstalledVersionLS )
            {
                // if installed version matches advertised, then we autoinstalled already
                // BUGBUG: If the user gets gets channel notification, then runs out
                // to the store and buys the new version, then installs it, we'll
                // mistake this for an auto-install.
                dwAdStateNew = SOFTDIST_ADSTATE_INSTALLED;
            }

            // only show the dialog if we've haven't been in this ad state before for
            // this update version
            if ( dwAdStateNew > sdi.dwAdState )
            {
                bHaveAd = TRUE;
            }
        } // if update is a newer version than advertised

    }

    return bHaveAd;
}


//  Purpose:    A ResolveLink-time check to see if the link has
//              Logo3 application channel
//
//  Inputs:     [HWND] hwnd
//                  -   The parent window (which could be the desktop).
//              [DWORD] fFlags
//                  -   Flags from the SLR_FLAGS enumeration.
//
//  Outputs:    [HRESULT]
//                  -   S_OK    The user wants to pursue the
//                              software update.
//                      S_FALSE No software update, or the user
//                              doesn't want it now.
//
//  Algorithm:  Check the software update registry info for the
//              ID embedded in the link. If there's a new version
//              advertised, prompt the user with shdocvw's message
//              box. If the mb says update, tell the caller we
//              don't want the link target, as we're headed to the
//              link update page.

HRESULT CShellLink::CheckLogo3Link(HWND hwnd,DWORD fFlags )
{
    HRESULT hr = S_FALSE; // default to no update.

    LPEXP_DARWIN_LINK pdl = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, EXP_LOGO3_ID_SIG);
    if (pdl)
    {
        TCHAR szLogo3ID[MAX_PATH];
        WCHAR szwLogo3ID[MAX_PATH];
        int   cchBlessData;
        TCHAR *pch;
        WCHAR *pwch;

#ifdef UNICODE
        pch = pdl->szwDarwinID;
#else
        pch = pdl->szDarwinID;
#endif

        // Ideally, we support multiple, semi-colon delmited IDs, for now
        // just grab the first one.
        for ( pwch = pdl->szwDarwinID, cchBlessData = 0;
              *pch != ';' && *pch != '\0' && cchBlessData < MAX_PATH;
              pch++, pwch++, cchBlessData++ )
        {
            szLogo3ID[cchBlessData] = *pch;
            szwLogo3ID[cchBlessData] = *pwch;
        }
        // and terminate
        szLogo3ID[cchBlessData] = '\0';
        szwLogo3ID[cchBlessData] = L'\0';
        
        // Before well haul in shdocvw, we'll sneak a peak at our Logo3 reg goo 
        if (!(fFlags & SLR_NO_UI) && FLogo3RegPeek(szLogo3ID))
        {
            // stuff stolen from shdocvw\util.cpp's CheckSoftwareUpdateUI
            BOOL fLaunchUpdate = FALSE;
            int nRes;
            SOFTDISTINFO sdi = { 0 };
            sdi.cbSize = sizeof(SOFTDISTINFO);

            nRes = SoftwareUpdateMessageBox( hwnd, szwLogo3ID, 0, &sdi );

            if (nRes != IDABORT)
            {
                if (nRes == IDYES)
                {
                    // BUGBUG: This differ's from Shdocvw in that we don't
                    // have the cool internal navigation stuff to play with.
                    // Originally, this was done with ShellExecEx. This failed
                    // because the http hook wasn't 100% reliable on Win95.
                    //ShellExecuteW(NULL, NULL, sdi.szHREF, NULL, NULL, 0);
                    hr = HlinkNavigateString(NULL, sdi.szHREF);

                } // if user wants update

                if (sdi.szTitle != NULL)
                    SHFree( sdi.szTitle );
                if (sdi.szAbstract != NULL)
                    SHFree( sdi.szAbstract );
                if (sdi.szHREF != NULL)
                    SHFree( sdi.szHREF );
    
                fLaunchUpdate = nRes == IDYES && SUCCEEDED(hr);
            } // if no message box abort ( error )

            if ( fLaunchUpdate )
                hr = S_OK;
        } // if showing UI is okay
    } // if Logo3 ID retreived.

    return hr;
}

void CShellLink::UpdateDirPIDL(LPTSTR pszPath)
{
    TCHAR  szPathSrc[MAX_PATH];
    LPTSTR psz;

    ASSERT(_sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);       

    if (pszPath)
    {
        psz = pszPath;
    }
    else
    {
        GetPath(szPathSrc, ARRAYSIZE(szPathSrc), NULL, 0);
        psz = szPathSrc;
    }
    
    if (_pidl)
    {
        ILFree(_pidl);
        _pidl = NULL;
    }

    SetPIDLPath(NULL, psz, NULL);
}

BOOL PathExistsOrMediaInserted(HWND hwnd, LPCTSTR pszPath)
{
    if (PathFileExistsAndAttributes(pszPath, NULL))
        return TRUE;

    // Give the user a chance to insert the disk.
    if (hwnd && SUCCEEDED(SHPathPrepareForWrite(hwnd, NULL, pszPath, SHPPFW_IGNOREFILENAME)) && PathFileExistsAndAttributes(pszPath, NULL))
        return TRUE;

    return FALSE;
}

//
// updates then resolves LinkInfo associated with a CShellLink instance
// if the resolve results in a new path updates the pidl to the new path
//
// in:
//      hwnd    to post resolve UI on (if dwFlags indicates UI)
//      dwFlags to be passed to ResolveLinkInfo
//
// returns:
//      FALSE   we failed the update, either UI cancel or memory failure
//      TRUE    we have a valid pli and pidl read to be used OR
//              we should search for this path using the link search code
//          2   the drive that the shortcut refers to is gone, but we should
//              give the tracker a try(in case the domain server knows where it went)

BOOL CShellLink::UpdateAndResolveLinkInfo(HWND hwnd, DWORD dwFlags)
{
    TCHAR szResolvedPath[MAX_PATH];

    if (SHRestricted(REST_LINKRESOLVEIGNORELINKINFO))
    {
        if (SHGetPathFromIDList(_pidl, szResolvedPath))
        {
            if (PathFileExistsAndAttributes(szResolvedPath, NULL))
                return TRUE;
            else if (!PathIsUNC(szResolvedPath) && IsDisconnectedNetDrive(DRIVEID(szResolvedPath)))
            {
                TCHAR szDrive[4];

                szDrive[0] = szResolvedPath[0];
                szDrive[1] = TEXT(':');
                szDrive[2] = 0;
                WNetRestoreConnection(hwnd, szDrive);
            }
        }
        return TRUE;
    }

    if (_pli)
    {
        DWORD dwOutFlags;
        BOOL bResolved = FALSE;

        ASSERT(! (dwFlags & RLI_IFL_UPDATE));

        if (ResolveLinkInfo(_pli, szResolvedPath, dwFlags, hwnd,
                                 &dwOutFlags, NULL))
        {
            ASSERT(! (dwOutFlags & RLI_OFL_UPDATED));

            bResolved = TRUE;

            // we have to hit the disk again to set the new path

            // DebugMsg(DM_TRACE, "Resolved LinkInfo -> %s %4x", szResolvedPath, dwOutFlags);

            if (PathFileExistsAndAttributes(szResolvedPath, NULL))
                return SetPIDLPath(NULL, szResolvedPath, NULL);
            else
                DebugMsg(DM_TRACE, TEXT("Link referent %s not found."), szResolvedPath);
        }
        else if (GetLastError() == ERROR_CANCELLED)
        {
            DebugMsg(DM_TRACE, TEXT("ResolveLinkInfo() failed, user canceled."));
            return FALSE;
        }
        DebugMsg(DM_TRACE, TEXT("ResolveLinkInfo() failed."));

        // Resolve failed, or resolve succeeded but the file was not found.
        // Try PIDL path.

        SHGetPathFromIDList(_pidl, szResolvedPath);

        ASSERT(szResolvedPath[0]);

        if (PathExistsOrMediaInserted(hwnd, szResolvedPath) || _ResolveRelative(szResolvedPath))
        {
            // this can happen when linkinfo can't find the original drive
            // serial # on the device. could be that the drive was
            // dblspaced or some disk utility wacked on it.

            DebugMsg(DM_TRACE, TEXT("Link referent %s found on different volume but same path.  LinkInfo will be updated."), szResolvedPath);

            /* Update LinkInfo to refer to PIDL path. */

            GetLinkInfo();

            return TRUE;
        }

        if (bResolved)
            return TRUE;        // please search for this
#ifdef WINNT
        //Give ptracker a chance before giving up
        return 2;
#else
        // BUGBUG: UI goes here
        // 1) if it is a floppy ask to insert
        // 2) if on unshared media tell them that
        // 3) net problems, tell 'em

        if (dwFlags & RLI_IFL_ALLOW_UI)
        {
            LPCTSTR pszName = _pszCurFile ? (LPCTSTR)PathFindFileName(_pszCurFile) : c_szNULL;

            ShellMessageBox(HINST_THISDLL, hwnd,
                            MAKEINTRESOURCE(IDS_LINKUNAVAILABLE),
                            MAKEINTRESOURCE(IDS_LINKERROR),
                            MB_OK | MB_ICONEXCLAMATION, pszName);
        }

        return FALSE;
#endif
    }
    else
        return TRUE;            // search for this
}


// returns:
//      S_OK    this resolution was taken care of
//      S_FALSE or FAILED(hr)   - did not happen, plese do regular stuff

HRESULT CShellLink::_SelfResolve(HWND hwnd, DWORD fFlags)
{
    HRESULT hr = S_FALSE;   // no, we did not handle it
    IShellFolder* psf;
    LPCITEMIDLIST pidlChild;
    if (_pidl && SUCCEEDED(SHBindToIDListParent(_pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild)))
    {
        IResolveShellLink* pResLink = NULL;

        // 2 ways to get the link resolve object
        // 1. ask the folder for the resolver for the item
        if (FAILED(psf->GetUIObjectOf(NULL, 1, &pidlChild, IID_IResolveShellLink, NULL, (void **)&pResLink)))
        {
            // 2. bind to the object directly and ask it (CreateViewObject)
            IShellFolder *psfItem;
            if (SUCCEEDED(psf->BindToObject(pidlChild, NULL, IID_PPV_ARG(IShellFolder, &psfItem))))
            {
                psfItem->CreateViewObject(NULL, IID_PPV_ARG(IResolveShellLink, &pResLink));
                psfItem->Release();
            }
        }

        if (pResLink)
        {
            hr = pResLink->ResolveShellLink((IUnknown*)(IShellLink*) this, hwnd, fFlags);
            pResLink->Release();
        }
        psf->Release();
    }
    return hr;
}

//
//  Purpose:    Provide the implementation for
//              IShellLink::Resolve and IShellLinkTracker::Resolve
//
//  Inputs:     [HWND] hwnd
//                  -   The parent window (which could be the desktop).
//              [DWORD] fFlags
//                  -   Flags from the SLR_FLAGS enumeration.
//              [DWORD] dwTracker
//                  -   Restrict CTracker::Resolve from the
//                      TrkMendRestrictions enumeration
//
//  Outputs:    [HRESULT]
//                  -   S_OK    resolution was successful
//                      S_FALSE user canceled
//
//  Algorithm:  Look for the link target and update the link path and IDList.
//              Check IPersistFile::IsDirty after calling this to see if the
//              link info has changed as a result.
//

HRESULT CShellLink::ResolveLink(HWND hwnd, DWORD fFlags, DWORD dwTracker)
{
    HRESULT hres = S_OK;
    TCHAR szPath[MAX_PATH];
    DWORD dwResolveFlags;
#ifdef WINNT
    DWORD fifFlags  = 0;
#endif

    // Check to see if the PIDL knows how to resolve itself (delegate to the folder)
    if (_SelfResolve(hwnd, fFlags) == S_OK)
        return S_OK;
        
    // Check to see if this link is a Logo3 link
    if (_sld.dwFlags & SLDF_HAS_LOGO3ID &&
        !SHRestricted(REST_NOLOGO3CHANNELNOTIFY))
    {
        if ( CheckLogo3Link(hwnd, fFlags) == S_OK )
            return S_FALSE;
    }

    // check to see if this is a Darwin link
    if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {
        if (!(fFlags & SLR_INVOKE_MSI))
        {
            // we only envoke darwin if they are passing the correct SLR_INVOKE_MSI
            // flag. This prevents bonehead apps from going and calling resolve and
            // faulting in a bunch of darwin apps.
            return S_OK;
        }

        LPEXP_DARWIN_LINK pdl = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, EXP_DARWIN_ID_SIG);
        if (pdl && IsDarwinEnabled())
        {
            // Special case darwin links
            TCHAR szDarwinCommand[MAX_PATH];
            TCHAR szDarwinID[MAX_PATH];

            SHUnicodeToTChar(pdl->szwDarwinID, szDarwinID, ARRAYSIZE(szDarwinID));

            DebugMsg(DM_TRACE, TEXT("CShellLink::ResolveLink() %s (Darwin ID)"), szDarwinID);

            HRESULT hres = ParseDarwinID(szDarwinID, szDarwinCommand , SIZECHARS(szDarwinCommand));
            if (FAILED(hres) || 
                HRESULT_CODE(hres) == ERROR_SUCCESS_REBOOT_REQUIRED || 
                HRESULT_CODE(hres) == ERROR_SUCCESS_REBOOT_INITIATED)
            {
                switch (HRESULT_CODE(hres))
                {
                case ERROR_INSTALL_USEREXIT:
                    break;  // User pressed cancel. They don't need UI.

                case ERROR_SUCCESS_REBOOT_INITIATED:
                case ERROR_SUCCESS_REBOOT_REQUIRED:
                    break;  // If we need to reboot, then don't launch...

                default:
                    if (!(fFlags & SLR_NO_UI))
                    {
                        TCHAR szTemp[MAX_PATH];
                        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,
                                    NULL, HRESULT_CODE(hres), 0, szTemp, ARRAYSIZE(szTemp), NULL);

                        ShellMessageBox(HINST_THISDLL,
                                    hwnd,
                                    szTemp,
                                    MAKEINTRESOURCE(IDS_LINKERROR),
                                    MB_OK | MB_ICONSTOP, NULL, NULL);
                    }
                    break;
                }
                return E_FAIL;
            }

            // We want to fire an event for the product code, not the path. Do this here since we've got the product code.
            TCHAR szProductCode[MAX_PATH];
            szProductCode[0] = TEXT('\0');

            MsiDecomposeDescriptor(szDarwinID, szProductCode, NULL, NULL, NULL);

            if (szProductCode[0] != TEXT('\0'))
            {
                UEMFireEvent(&UEMIID_SHELL, UEME_RUNPATH, UEMF_XEVENT, -1, (LPARAM)szProductCode);
            }

            SetPIDLPath(NULL, szDarwinCommand, NULL);
            return S_OK;
        }
    }

    // check to see whether this link has expandable environment strings
    if (_sld.dwFlags & SLDF_HAS_EXP_SZ)
    {
        // yep, so create a new pidl that points to expanded path
        if (_GetExpPath(szPath, SIZECHARS(szPath)))
        {
            // The best kind of pidl we can have is one created by ILCreateFromPath.
            // It's even better than any pidl we might have stored in the .lnk file.
            LPITEMIDLIST pidlTemp = ILCreateFromPath( szPath );
            if ( pidlTemp )
            {
                if (_pidl)
                    ILFree(_pidl);
                _pidl = pidlTemp;

                // we are modifing _pidl.  We should never have a EXP_SPECIAL_FOLDER_SIG section
                // when we modify the _pidl, otherwise the two will be out of ssync
                ASSERT( NULL == SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG) );
            }
            else
            {
                // The target file is no longer valid so we should dump the EXP_SZ section before
                // we continue.  Note that we don't set bDirty here, that is only set later if
                // we actually resolve this link to a new path or pidl.  The result is we'll only
                // save this modification if a new target is found and accepted by the user.
                _sld.dwFlags &= ~SLDF_HAS_EXP_SZ;

                // The second best kind of pidl we can have is the pdil stored in the link file.
                // This pidl is most likely equivalent to the simple pidl we would otherwise create
                // below and we know that either pidl will be invalid.  We simply need a pidl that
                // is of the correct class.  If no pidl is originally stored with the link then the
                // shell creates and stores the pidl the first time the link is resolved.
                if (!_pidl)
                {
                    // If we don't have our second choice, we settle for our third choice which is to
                    // create a simple pidl.  The simple pidl is good enough to allow some link resolution
                    // to occur which is better than the silent failure we get otherwise.  We should
                    // still have Link Info and Find Data to use in resolving the pidl.
                    _pidl = SHSimpleIDListFromPath(szPath);

                    // we are modifing _pidl.  We should never have a EXP_SPECIAL_FOLDER_SIG section
                    // when we modify the _pidl, otherwise the two will be out of ssync
                    ASSERT( NULL == SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG) );
                }
            }
        } 
        else 
        {
            DebugMsg(DM_TRACE, TEXT("ResolveLink called on EXP_SZ link without EXP_SZ section!"));
            _sld.dwFlags &= ~SLDF_HAS_EXP_SZ;
        }
    }

    if (_pidl == NULL)
        return E_FAIL;

    // ensure that this is a link to a file, if not validate it
    if (!SHGetPathFromIDList(_pidl, szPath))
    {
        // validate the non file system target first
        ULONG dwAttrib = SFGAO_VALIDATE;     // to check for existance
        if (FAILED(SHGetNameAndFlags(_pidl, SHGDN_NORMAL, szPath, ARRAYSIZE(szPath), &dwAttrib)))
        {
            if (!(fFlags & SLR_NO_UI))
            {
                ShellMessageBox(HINST_THISDLL, hwnd, MAKEINTRESOURCE(IDS_CANTFINDORIGINAL), NULL,
                            MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND, szPath);
            }
            return E_FAIL;
        }
        return S_OK;
    }

    // At this point, szPath contains the path+filename to the target

    dwResolveFlags = (RLI_IFL_CONNECT | RLI_IFL_TEMPORARY);

    if (!PathIsRoot(szPath))
    {
        dwResolveFlags |= RLI_IFL_LOCAL_SEARCH;

        // if we are trying to avoid link info's we can bail if we 
        // the file already exists..

        // Is the link dirty?
        if (QueryAndSetFindData(szPath))
        {
            // No

            // Is this the 'real desktop' or 'desktop folder' case AND
            // does the caller want us to update the link?
            if ((_sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (!ILIsEmpty(_pidl) && ILIsEmpty(_ILNext(_pidl))) &&
                (!(fFlags & SLR_NOUPDATE)))
            {
                // Yes, so update to disambiguate the two cases.
                UpdateDirPIDL(szPath);
            }

            // Nothing else needs to be done because nothing else is out of date.
            return S_OK;
        }
    }

    if (!(fFlags & SLR_NOLINKINFO))
    {
        if (!(fFlags & SLR_NO_UI))
            dwResolveFlags |= RLI_IFL_ALLOW_UI;

        switch (UpdateAndResolveLinkInfo(hwnd, dwResolveFlags))
        {
         //
        //  No chance of finding the file - give up now.
        //
        case FALSE:
            DebugMsg(DM_TRACE, TEXT("UpdateAndResolveLinkInfo() failed"));
            return S_FALSE; // they canceled or this failed

#ifdef WINNT
        //
        //  We have no chance of finding the file, but let the tracker try.
        //
        case 2:
            fifFlags |= FIF_NODRIVE;        // don't try ourselves
            break;
#endif

        //
        //  Maybe we can find it after all.
        //
        case TRUE:
            break;
           
        }

        // UpdateAndResolveLinkInfo() may have changed the path
        SHGetPathFromIDList(_pidl, szPath);
    }

    if (PathIsRoot(szPath))
    {
        // DebugMsg(DM_TRACE, "ShellLink::Resolve() root path %s", szPath);
        // should be golden
    }
    else
    {
        // the above code did the retry logic (UI) if needed

        if (!QueryAndSetFindData(szPath))
        {
            int id;
            BOOL fFound = FALSE;
            // this thing is a link that is broken
            id = GetLastError();

            DebugMsg(DM_TRACE, TEXT("ShellLink::Resolve() file not found %s(%d)"), szPath, id);

            // Some error codes we will try to recover from by trying to get the network
            // to restore the connection.
            if (id == ERROR_BAD_NETPATH)
            {
                TCHAR szDrive[4];
                szDrive[0] = szPath[0];
                szDrive[1] = TEXT(':');
                szDrive[2] = 0;
                if (WNetRestoreConnection(hwnd, szDrive) == WN_SUCCESS)
                {
                    fFound = QueryAndSetFindData(szPath);
                    if( fFound && (_sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
                        UpdateDirPIDL(szPath );
                }
            }

            if (!fFound)
            {
                WIN32_FIND_DATA fd;
                GetFindData(&fd);  // sld => fd

                // Find the file

                id = FindInFolder( hwnd, fFlags, szPath, &fd, _pszCurFile
#ifdef WINNT
                                   , _ptracker, dwTracker, fifFlags
#endif
                                   );
                
                switch (id) 
                {
                    case IDOK:
                        DebugMsg(DM_TRACE, TEXT("ShellLink::Resolve() resolved to %s"), fd.cFileName);

                        // fd.cFileName comes back fully qualified
#ifdef WINNT
                        QueryAndSetFindData(fd.cFileName);
                        SetPIDLPath(NULL, fd.cFileName, NULL);
#else
                        SetPIDLPath(NULL, fd.cFileName, &fd);
#endif
                        ASSERT(_bDirty);       // should be dirty now
                        break;
                    
                    default:
                        ASSERT(!_bDirty);      // should not be dirty now

                        hres = S_FALSE;
                }
            }
        }
    }

    //
    // if the link is dirty update it.
    //
    if (_bDirty && (fFlags & SLR_UPDATE))
        Save((LPCOLESTR)NULL, TRUE);

    return hres;
}   // ResolveLink


// This will just add a section to the end of the extra data -- it does
// not check to see if the section already exists, etc.
void CShellLink::_AddExtraDataSection(DATABLOCK_HEADER *peh)
{
    if (SHAddDataBlock(&_pExtraData, peh))
    {
        _bDirty = TRUE;
    }
}

// This will remove the extra data section with the given signature.
void CShellLink::_RemoveExtraDataSection(DWORD dwSig)
{
    if (SHRemoveDataBlock(&_pExtraData, dwSig))
    {
        _bDirty = TRUE;
    }
}

// currently this function is used for NT shell32 builds only
void * CShellLink::_ReadExtraDataSection(DWORD dwSig)
{
    DATABLOCK_HEADER *pdb;

    CopyDataBlock(dwSig, (void **)&pdb);
    return (void *)pdb;
}

// Darwin and Logo3 blessings share the same structure
HRESULT CShellLink::BlessLink(LPCTSTR *ppszPath, DWORD dwSignature)
{
    EXP_DARWIN_LINK expLink;
    LPEXP_DARWIN_LINK lpNew;
    TCHAR szBlessID[MAX_PATH];
    int   cchBlessData;
    TCHAR *pch;

    // Copy the blessing data and advance *ppszPath to the end of the data.
    for ( pch = szBlessID, cchBlessData = 0;
          **ppszPath != ':' && **ppszPath != '\0' && cchBlessData < MAX_PATH;
          pch++, (*ppszPath)++, cchBlessData++ )
        *pch = **ppszPath;

    // Terminate the blessing data
    *pch = 0;
    
    // Set the magic flag
    if (dwSignature == EXP_DARWIN_ID_SIG)
        _sld.dwFlags |= SLDF_HAS_DARWINID;
    else if (dwSignature == EXP_LOGO3_ID_SIG)
        _sld.dwFlags |= SLDF_HAS_LOGO3ID;
    else
    {
        TraceMsg(TF_WARNING, "BlessLink was passed a bad data block signature.");
        return E_INVALIDARG;
    }


    // locate the old block, if it's there
    lpNew = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, dwSignature);

    // if not, use our stack var
    if (!lpNew)
    {
        lpNew = &expLink;
        expLink.dbh.cbSize = 0;
        expLink.dbh.dwSignature = dwSignature;
    }

    SHTCharToAnsi(szBlessID, lpNew->szDarwinID, ARRAYSIZE(lpNew->szDarwinID));
    SHTCharToUnicode(szBlessID, lpNew->szwDarwinID, ARRAYSIZE(lpNew->szwDarwinID));

    // See if this is a new entry that we need to add
    if (lpNew->dbh.cbSize == 0)
    {
        lpNew->dbh.cbSize = SIZEOF(*lpNew);
        _AddExtraDataSection((DATABLOCK_HEADER *)lpNew);
    }

    return S_OK;
}

HRESULT CShellLink::CheckForLinkBlessing(LPCTSTR *ppszPathIn)
{
    HRESULT hr = S_FALSE; // default to no-error, no blessing

    while (SUCCEEDED(hr) && (*ppszPathIn)[0] == ':' && (*ppszPathIn)[1] == ':' )
    {
        // identify type of link blessing and perform
        if (StrCmpNI(*ppszPathIn, DARWINGUID_TAG, ARRAYSIZE(DARWINGUID_TAG) - 1) == 0)
        {
            *ppszPathIn = *ppszPathIn + ARRAYSIZE(DARWINGUID_TAG) - 1;
            hr = BlessLink(ppszPathIn, EXP_DARWIN_ID_SIG);
        }
        else if (StrCmpNI(*ppszPathIn, LOGO3GUID_TAG, ARRAYSIZE(LOGO3GUID_TAG) - 1) == 0)
        {
            HRESULT hrBless;

            *ppszPathIn = *ppszPathIn + ARRAYSIZE(LOGO3GUID_TAG) - 1;
            hrBless = BlessLink(ppszPathIn, EXP_LOGO3_ID_SIG);
            // if the blessing failed, report the error, otherwise keep the
            // default hr == S_FALSE or the result of the Darwin blessing.
            if ( FAILED(hrBless) )
                hr = hrBless;
        }
        else
            break;
    }
        
    return hr;
}

STDMETHODIMP CShellLink::SetPath(LPCWSTR pszPathW)
{
    HRESULT hr;
    TCHAR szPath[MAX_PATH];
    LPCTSTR pszPath;

    // NOTE: all the other Set* functions allow NULL pointer to be passed in, but this
    // one does not because it would AV. 
    if (!pszPathW)
    {
        return E_INVALIDARG;
    }
    else if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {
        return S_FALSE; // a darwin link already, then we dont allow the path to change
    }

    SHUnicodeToTChar(pszPathW, szPath, ARRAYSIZE(szPath));
    pszPath = szPath;

// TODO: Remove OLD_DARWIN stuff once we have transitioned Darwin to
//       the new link blessing syntax.
#define OLD_DARWIN

#ifdef OLD_DARWIN
    int iLength = lstrlen(pszPath);
    // we check to see if the path is enclosed in []'s. If so,
    // this means that we are creating a DARWIN link. We therefore
    // set SLDF_HAS_DARWINID and store the darwinID (the string
    // inside the []'s) in the extra data segment.
    if ((pszPath[0] == TEXT('[')) && (pszPath[iLength - 1] == TEXT(']')))
    {
        // we have a path that is enclosed in []'s,
        // so this must be a Darwin link.
        EXP_DARWIN_LINK expLink;
        TCHAR szDarwinID[MAX_PATH];

        // strip off the []'s
        lstrcpy(szDarwinID, &pszPath[1]);
        szDarwinID[iLength - 2] = 0;

        _sld.dwFlags |= SLDF_HAS_DARWINID;

        LPEXP_DARWIN_LINK pedl = (LPEXP_DARWIN_LINK)SHFindDataBlock(_pExtraData, EXP_DARWIN_ID_SIG);
        if (!pedl)
        {
            pedl = &expLink;
            expLink.dbh.cbSize = 0;
            expLink.dbh.dwSignature = EXP_DARWIN_ID_SIG;
        }

        SHTCharToAnsi(szDarwinID, pedl->szDarwinID, ARRAYSIZE(pedl->szDarwinID));
        SHTCharToUnicode(szDarwinID, pedl->szwDarwinID, ARRAYSIZE(pedl->szwDarwinID));

        // See if this is a new entry that we need to add
        if (pedl->dbh.cbSize == 0)
        {
            pedl->dbh.cbSize = SIZEOF(*pedl);
            _AddExtraDataSection((DATABLOCK_HEADER *)pedl);
        }

        // For darwin links, we ignore the path and pidl for now. We would
        // normally call SetPIDLPath and SetIDList but we skip these
        // steps for darwin links because all SetPIDLPath does is set the pidl
        // and all SetIDList does is set fd (the WIN32_FIND_DATA)
        // for the target, and we dont have a target since we are a darwin link.
        hr = S_OK;
    }
    else
#endif // OLD_DARWIN
    {
        // Check for ::<guid>:<data>: prefix, which signals us to bless the
        // the lnk with extra data. NOTE: we pass the &pszPath here so that this fn can 
        // advance the string pointer past the ::<guid>:<data>: sections and point to 
        // the path, if there is one.
        hr = CheckForLinkBlessing(&pszPath);
        if (S_OK != hr)
        {
            // Check to see if the target has any expandable environment strings
            // in it.  If so, set the appropriate information in the CShellLink
            // data.
            TCHAR szExpPath[MAX_PATH];
            SHExpandEnvironmentStrings(pszPath, szExpPath, ARRAYSIZE(szExpPath));

            if (lstrcmp(szExpPath, pszPath)) 
            {
                EXP_SZ_LINK expLink;

                // mark that link has expandable strings, and add them
                _sld.dwFlags |= SLDF_HAS_EXP_SZ;

                LPEXP_SZ_LINK pel = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_LINK_SIG);
                if (!pel) 
                {
                    pel = &expLink;
                    expLink.cbSize = 0;
                    expLink.dwSignature = EXP_SZ_LINK_SIG;
                }

                // store both A and W version (for no good reason!)
                SHTCharToAnsi(pszPath, pel->szTarget, ARRAYSIZE(pel->szTarget));
                SHTCharToUnicode(pszPath, pel->swzTarget, ARRAYSIZE(pel->swzTarget));

                // See if this is a new entry that we need to add
                if (pel->cbSize == 0)
                {
                    pel->cbSize = SIZEOF(*pel);
                    _AddExtraDataSection((DATABLOCK_HEADER *)pel);
                }
                SetPIDLPath(NULL, szExpPath, NULL);
            }
            else 
            {
                _sld.dwFlags &= (~SLDF_HAS_EXP_SZ);
                _RemoveExtraDataSection(EXP_SZ_LINK_SIG);

                SetPIDLPath(NULL, pszPath, NULL);
            }
            hr = SetIDList(_pidl);
        }
    }
    return hr;
}

STDMETHODIMP CShellLink::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_ShellLink;
    return S_OK;
}

STDMETHODIMP CShellLink::IsDirty()
{
    return _bDirty ? S_OK : S_FALSE;
}

HRESULT LinkInfo_LoadFromStream(IStream *pstm, PLINKINFO *ppli)
{
    DWORD dwSize;
    ULONG cbBytesRead;
    HRESULT hres;

    if (*ppli)
    {
        LocalFree((HLOCAL)*ppli);
        *ppli = NULL;
    }

    hres = pstm->Read(&dwSize, SIZEOF(dwSize), &cbBytesRead);     // size of data
    if (SUCCEEDED(hres) && (cbBytesRead == SIZEOF(dwSize)))
    {
        if (dwSize >= SIZEOF(dwSize))   // must be at least this big
        {
            /* Yes.  Read remainder of LinkInfo into local memory. */
            PLINKINFO pli = (PLINKINFO)LocalAlloc(LPTR, dwSize);
            if (pli)
            {
                *(DWORD *)pli = dwSize;         // Copy size

                dwSize -= SIZEOF(dwSize);       // Read remainder of LinkInfo

                hres = pstm->Read(((DWORD *)pli) + 1, dwSize, &cbBytesRead);
                if (SUCCEEDED(hres) && (cbBytesRead == dwSize))
                   *ppli = pli; // LinkInfo read successfully
                else
                   LocalFree((HLOCAL)pli);
            }
        }
    }
    return hres;
}

//Decodes the CSIDL_ relative target pidl
void CShellLink::_DecodeSpecialFolder()
{
#ifdef DEBUG
    if ( _pszCurFile )
    {
        TraceMsg( TF_DEBUGLINKCODE, "Enter _DecodeSpecialFolder (%s):", _pszCurFile );
    }
    else
    {
        TraceMsg( TF_DEBUGLINKCODE, "Enter _DecodeSpecialFolder (no file name):" );
    }
#endif

    LPEXP_SPECIAL_FOLDER pData = (LPEXP_SPECIAL_FOLDER)SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG);
    if (pData)
    {
        TraceMsg( TF_DEBUGLINKCODE, "  EXP_SPECIAL_FOLDER_SIG found, CSIDL=%d, cbOffset=0x%08x", pData->idSpecialFolder, pData->cbOffset );

        LPITEMIDLIST pidlFolder = SHCloneSpecialIDList(NULL, pData->idSpecialFolder, FALSE);
        if (pidlFolder)
        {
            // BUGBUG: In theory the pData->cbOffset should always match the _pidl for this shortcut so the
            // following _ILSkip should be 100% safe.  The reason this should be safe is because we delete
            // the EXP_SPECIAL_FOLDER_SIG section as soon as we use it and we don't write it back until we
            // save the link, at which time we write it using the updated _pidl.  However, somehow we are
            // seeing a lot of _pidls that don't match the pData->cbOffset so somebody someplace is directly
            // mucking with the _pidl without updating the EXP_SPECIAL_FOLDER_SIG section.
            ASSERT(IS_VALID_PIDL(_pidl));

            LPITEMIDLIST pidlTarget = _ILSkip(_pidl, pData->cbOffset);
            LPITEMIDLIST pidlSanityCheck = _pidl;

            while ( !ILIsEmpty(pidlSanityCheck) && (pidlSanityCheck < pidlTarget) )
            {
                // We go one step at a time until pidlSanityCheck == pidlTarget.  If we reach the end
                // of pidlSanityCheck, or if we go past pidlTarget, before this condition is met then
                // we have an invalid pData->cbOffset.
                pidlSanityCheck = _ILNext(pidlSanityCheck);
            }

            if ( pidlSanityCheck == pidlTarget )
            {
                LPITEMIDLIST pidlNew = ILCombine(pidlFolder, pidlTarget);
                if (pidlNew)
                {
                    UpdateWorkingDir(pidlNew);
                    ILFree(_pidl);
                    _pidl = pidlNew;

                    TraceMsg( TF_DEBUGLINKCODE, "  _DecodeSpecialFolder _pidl update successful." );
                }
            }
            else
            {
                // ToddB: If you hit this error in a debugger contact ShellHot
                TraceMsg( TF_ERROR, "  A bogus pidl offset was found in _DecodeSpecialFolder, this should not happen!" );
            }
            ILFree(pidlFolder);
        }
        // Note: We must always remove this if we have one. because the offset
        // will get trashed if some other link tracking
        // mechanism replace pidl on us.
        _RemoveExtraDataSection(EXP_SPECIAL_FOLDER_SIG);

        ASSERT( NULL==SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG) );
        TraceMsg( TF_DEBUGLINKCODE, "  EXP_SPECIAL_FOLDER_SIG removed" );
    }

    TraceMsg( TF_DEBUGLINKCODE, "Exit _DecodeSpecialFolder" );
}

STDMETHODIMP CShellLink::Load(IStream *pstm)
{
    ULONG cbBytes;
    DWORD cbSize;

    TraceMsg( TF_DEBUGLINKCODE, "Loading link from stream." );

    _ResetPersistData();        // clear out our state

    HRESULT hres = pstm->Read(&cbSize, SIZEOF(cbSize), &cbBytes);
    if (SUCCEEDED(hres))
    {
        if (cbBytes == SIZEOF(cbSize))
        {
            if (cbSize == SIZEOF(_sld))
            {
                hres = pstm->Read((LPBYTE)&_sld + SIZEOF(cbSize), SIZEOF(_sld) - SIZEOF(cbSize), &cbBytes);
                if (SUCCEEDED(hres) && cbBytes == (SIZEOF(_sld) - SIZEOF(cbSize)) && IsEqualGUID(_sld.clsid, CLSID_ShellLink))
                {
                    _sld.cbSize = SIZEOF(_sld);

                    switch (_sld.iShowCmd) 
                    {
                        case SW_SHOWNORMAL:
                        case SW_SHOWMINNOACTIVE:
                        case SW_SHOWMAXIMIZED:
                        break;

                        default:
                            DebugMsg(DM_TRACE, TEXT("Shortcut Load, mapping bogus ShowCmd: %d"), _sld.iShowCmd);
                            _sld.iShowCmd = SW_SHOWNORMAL;
                        break;
                    }

                    // save so we can generate notify on save
                    _wOldHotkey = _sld.wHotkey;   

                    // read all of the members

                    if (_sld.dwFlags & SLDF_HAS_ID_LIST)
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading pidl..." );
                        hres = ILLoadFromStream(pstm, &_pidl);

                        // success means we read the correct amount of data from the stream, not that the pidl is valid.
                        if (SUCCEEDED(hres))
                        {
                            if (_pli && (_sld.dwFlags & SLDF_FORCE_NO_LINKINFO))
                            {
                                DebugMsg( DM_TRACE, TEXT("labotimizing link"));
                                LocalFree((HLOCAL)_pli);
                                _pli = NULL;
                            }

                            // Check for a valid pidl.  File corruption can cause pidls to become bad which will cause
                            // explorer to AV unless we catch it here.  Also, people have been known to write invalid
                            // pidls into link files from time to time.
                            if ( !SHIsValidPidl(_pidl) )
                            {
                                // In theory this will only happen due to file corruption, but I've seen this too
                                // often not to suspect that we might be doing something wrong.
                                TraceMsg( TF_WARNING, "CShellLink::Load: Corrupted pidl read from link file." );

                                // turn off the flag, which we know is on to start with
                                _sld.dwFlags ^= SLDF_HAS_ID_LIST;

                                // free the memory.  I call SHFree and not ILFree to avoid getting debug assert messages.
                                // We already know this is an invalid pidl.
                                SHFree(_pidl);

                                // drop the result
                                _pidl = NULL;
                                _bDirty = TRUE;

                                // continue as though there was no SLDF_HAS_ID_LIST flag to start with
                                // REVIEW: should we only continue if certain other sections are also included
                                // in the link?  What will happen if SLDF_HAS_ID_LIST was the only data set for
                                // this link file?  We would get a null link. 
                            }
                        }
                        else
                        {
                            ASSERT(NULL==_pidl);
                        }
                    }

                    // BUGBUG: this part is not unicode ready, talk to daviddi

                    if (SUCCEEDED(hres) && (_sld.dwFlags & (SLDF_HAS_LINK_INFO)))
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading Link Info..." );
                        hres = LinkInfo_LoadFromStream(pstm, &_pli);
                        if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_FORCE_NO_LINKINFO) && _pli)
                        {
                            DebugMsg(DM_TRACE, TEXT("labotimizing link"));
                            LocalFree((HLOCAL)_pli);
                            _pli = NULL;
                        }
                    }

                    if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_NAME))
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading Name..." );
                        hres = Str_SetFromStream(pstm, &_pszName, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_RELPATH))
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading Relative Path..." );
                        hres = Str_SetFromStream(pstm, &_pszRelPath, _sld.dwFlags & SLDF_UNICODE);
                        if (!_pidl && SUCCEEDED(hres) && _pszRelPath)
                        {
                            TCHAR szTmp[ MAX_PATH +1 ];
                            if (_ResolveRelative(szTmp))
                            {
                                _pidl = ILCreateFromPath(szTmp);
                            }
                            else
                            {
                                _pidl = ILCreateFromPath(_pszRelPath);  //bug 239417:  for lnk files created on NT3.51
                            }
                        }
                    }

                    if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_WORKINGDIR))
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading Working Dir..." );
                        hres = Str_SetFromStream(pstm, &_pszWorkingDir, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_ARGS))
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading Arguments..." );
                        hres = Str_SetFromStream(pstm, &_pszArgs, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_ICONLOCATION))
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading Icon Location..." );
                        hres = Str_SetFromStream(pstm, &_pszIconLocation, _sld.dwFlags & SLDF_UNICODE);
                    }

                    if (SUCCEEDED(hres))
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: Loading Data Block..." );
                        hres = SHReadDataBlockList(pstm, &(_pExtraData));

#ifdef TEST_EXTRA_DATA
                        if (bTestExtra)
                        {
                            DATABLOCK_HEADER *pdbTest;
                
                            // verify they were added
                            pdbTest = SHFindDataBlock(_pExtraData, (DWORD)-2);
                            if (pdbTest)
                            {
                                ASSERT(SIZEOF(*pdbTest) == pdbTest->cbSize);

                                ASSERT(SHRemoveDataBlock(&_pExtraData, (DWORD)-2));
                                ASSERT(!SHFindDataBlock(_pExtraData, (DWORD)-2));
                            }
            
                            pdbTest = SHFindDataBlock(_pExtraData, (DWORD)-3);
                            if (pdbTest)
                            {
                                ASSERT(!lstrcmp((LPSTR)(pdbTest+1), TEXT("CG")));

                                ASSERT(SHRemoveDataBlock(&_pExtraData, (DWORD)-3));
                                ASSERT(!SHFindDataBlock(_pExtraData, (DWORD)-3));
                            }
            
                            pdbTest = SHFindDataBlock(_pExtraData, (DWORD)-4);
                            if (pdbTest)
                            {
                                ASSERT(!lstrcmp((LPSTR)(pdbTest+1), TEXT("123456")));

                                ASSERT(SHRemoveDataBlock(&_pExtraData, (DWORD)-4));
                                ASSERT(!SHFindDataBlock(_pExtraData, (DWORD)-4));
                            }
                        }
#endif // TEST_EXTRA_DATA
                    }

                    // reset the darwin info on load
                    if (_sld.dwFlags & SLDF_HAS_DARWINID)
                    {
                        TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: has a Darwin ID..." );
                        // NOTE: darwin ids don't coexist well with EXP_SPECIAL_FOLDER_SIG
                        ASSERT(NULL == SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG));

                        ILFree(_pidl);
                        _pidl = NULL;

                        // we should never have a darwin link that is missing
                        // the icon path
                        if (_pszIconLocation)
                        {
                            // we always put back the icon path as the pidl at
                            // load time since darwin could change the path or
                            // to the app (eg: new version of the app)
                            TCHAR szPath[MAX_PATH];
                            
                            // expand any env. strings in the icon path before
                            // creating the pidl.
                            SHExpandEnvironmentStrings(_pszIconLocation, szPath, ARRAYSIZE(szPath));

                            _pidl = ILCreateFromPath(szPath);
                        }
                        else
                        {
#ifdef DEBUG
                            static s_fBadDarwinSeen = FALSE;
                            // JohnC doesn't like this message, so we spew it only once per session
                            AssertMsg(s_fBadDarwinSeen, TEXT("Corrupted Darwin shortcut - no icon location (will be reported only once per session)"));
                            s_fBadDarwinSeen = TRUE;
#endif
                        }
                    }
                    else
                    {
                        // The Darwin stuff above creates a new pidl, which
                        // would cause this stuff to blow up. We should never
                        // get both at once, but let's be extra robust...
                        //
                        // Since we store the offset into the pidl here, and
                        // the pidl can change for various reasons, we can
                        // only do this once at load time. Do it here.
                        //
                        if (_pidl)
                            _DecodeSpecialFolder();
                    }

#ifdef WINNT
                    if (SUCCEEDED(hres) && _ptracker)
                    {
                        // load the tracker from extra data
                        EXP_TRACKER *lpData = (LPEXP_TRACKER)SHFindDataBlock(_pExtraData, EXP_TRACKER_SIG);
                        if (lpData) 
                        {
                            TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: has Tracker Data..." );
                            hres = _ptracker->Load( lpData->abTracker, lpData->cbSize - sizeof(EXP_TRACKER));
                            if (FAILED(hres))
                            {
                                // Failure of the Tracker isn't just cause to make
                                // the shortcut unusable.  So just re-init it and move on.
                                _ptracker->InitNew();
                                hres = S_OK;
                            }
                        }
                        else
                        {
                            TraceMsg( TF_DEBUGLINKCODE, "  CShellLink: no EXP_TRACKER data found..." );
                        }
                    }
#endif
                    if (SUCCEEDED(hres))
                        _bDirty = FALSE;
                }
                else
                {
                    DebugMsg(DM_TRACE, TEXT("failed to read link struct"));
                    hres = E_FAIL;      // invalid file size
                }
            }
            else
            {
                DebugMsg(DM_TRACE, TEXT("invalid length field in link:%d"), cbBytes);
                hres = E_FAIL;  // invalid file size
            }
        }
        else if (cbBytes == 0)
        {
            _sld.cbSize = 0;   // zero length file is ok
        }
        else
        {
            hres = E_FAIL;      // invalid file size
        }
    }
    return hres;
}

// set the relative path
// in:
//      pszRelSource    fully qualified path to a file (must be file, not directory)
//                      to be used to find a relative path with the link target.
//
// returns:
//      S_OK            relative path is set
//      S_FALSE         pszPathRel is not relative to the destination or the
//                      destionation is not a file (could be link to a pidl only)
// notes:
//      set the dirty bit if this is a new relative path
//

HRESULT CShellLink::_SetRelativePath(LPCTSTR pszRelSource)
{
    TCHAR szPath[MAX_PATH], szDest[MAX_PATH];

    ASSERT(!PathIsRelative(pszRelSource));

    if (_pidl == NULL || !SHGetPathFromIDList(_pidl, szDest))
    {
        DebugMsg(DM_TRACE, TEXT("SetRelative called on non path link"));
        return S_FALSE;
    }

    // assume pszRelSource is a file, not a directory
    if (PathRelativePathTo(szPath, pszRelSource, 0, szDest, _sld.dwFileAttributes))
    {
        pszRelSource = szPath;
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("paths are not relative"));
        pszRelSource = NULL;    // clear the stored relative path below
    }

    _SetField(&_pszRelPath, pszRelSource);

    return S_OK;
}

BOOL CShellLink::_EncodeSpecialFolder()
{
    BOOL bRet = FALSE;

    TraceMsg( TF_DEBUGLINKCODE, "Entering _EncodeSpecialFolder:" );

    if (_pidl)
    {
        // make sure we don't already have a EXP_SPECIAL_FOLDER_SIG data block, otherwise we would
        // end up with two of these and the first one would win on read.
        // If you hit this ASSERT in a debugger, contact ToddB with a remote.  We need to figure out
        // why we are corrupting our shortcuts.
        ASSERT( NULL==SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG) );

        EXP_SPECIAL_FOLDER exp;
        exp.idSpecialFolder = GetSpecialFolderParentIDAndOffset(_pidl, &(exp.cbOffset));
        if (exp.idSpecialFolder)
        {
            TraceMsg( TF_DEBUGLINKCODE, "  _pidl is in Special Folder with CSIDL=%d", exp.idSpecialFolder );

            exp.cbSize = SIZEOF(exp);
            exp.dwSignature = EXP_SPECIAL_FOLDER_SIG;

            _AddExtraDataSection((DATABLOCK_HEADER *)&exp);
            bRet = TRUE;
        }
    }

    TraceMsg( TF_DEBUGLINKCODE, "Exiting _EncodeSpecialFolder" );

    return bRet;
}

HRESULT LinkInfo_SaveToStream(IStream *pstm, PCLINKINFO pcli)
{
    DWORD dwSize;
    ULONG cbBytes;
    HRESULT hres;

    dwSize = *(DWORD *)pcli;    // Get LinkInfo size

    hres = pstm->Write(pcli, dwSize, &cbBytes);
    if (SUCCEEDED(hres) && (cbBytes != dwSize))
        hres = E_FAIL;
    return hres;
}


#ifdef WINNT
// NT4 EXPORTED THESE -- YIKES -- USE NEW IShellLinkDataList INSTEAD
//
EXTERN_C void Link_AddExtraDataSection(CShellLink *pObj, DATABLOCK_HEADER *lpData)
{
    pObj->_AddExtraDataSection(lpData);
}
EXTERN_C void Link_RemoveExtraDataSection(CShellLink *pObj, DWORD dwSig)
{
    pObj->_RemoveExtraDataSection(dwSig);
}
EXTERN_C void *Link_ReadExtraDataSection(CShellLink *pObj, DWORD dwSig)
{
    return pObj->_ReadExtraDataSection(dwSig);
}

//
// Replaces the tracker extra data with current tracker state
//
HRESULT CShellLink::_UpdateTrackerData()
{
    HRESULT hres = E_FAIL;
    ULONG ulSize = _ptracker->GetSize();


    if (!_ptracker->IsLoaded())
    {
        _RemoveExtraDataSection(EXP_TRACKER_SIG);
        return (S_OK);
    }

    if (!_ptracker->IsDirty())
        return (S_OK);

    // Make sure the Tracker size is a multiple of DWORDs.
    // If we hit this assert then we would have mis-aligned stuff stored in the extra data.
    //
    if (EVAL(0 == (ulSize & 3)))
    {
        EXP_TRACKER *pExpTracker = (EXP_TRACKER *)LocalAlloc(LPTR, ulSize + SIZEOF(DATABLOCK_HEADER));
        if (pExpTracker)
        {
            _RemoveExtraDataSection(EXP_TRACKER_SIG);
        
            pExpTracker->cbSize = ulSize + sizeof(DATABLOCK_HEADER);
            pExpTracker->dwSignature = EXP_TRACKER_SIG;
            _ptracker->Save( pExpTracker->abTracker, ulSize );
        
            _AddExtraDataSection((DATABLOCK_HEADER *)&pExpTracker->cbSize);
            DebugMsg(DM_TRACE, TEXT("_UpdateTrackerData: EXP_TRACKER at %08X."), &pExpTracker->cbSize);

            LocalFree(pExpTracker);
            hres = S_OK;
        }
    }
    return hres;
}

#endif  // WINNT

STDMETHODIMP CShellLink::Save(IStream *pstm, BOOL fClearDirty)
{
    ULONG cbBytes;
    HRESULT hres;
    BOOL fEncode;

    _sld.cbSize = SIZEOF(_sld);
    _sld.clsid = CLSID_ShellLink;
    //  _sld.dwFlags = 0;
    // We do the following & instead of zeroing because the SLDF_HAS_EXP_SZ and
    // SLDF_RUN_IN_SEPARATE and SLDF_RUNAS_USER and SLDF_HAS_DARWINID are passed to us and are valid,
    // the others can be reconstructed below, but these three can not, so we need to
    // preserve them!

    //(BUGBUG, this may change when we go to property stream storage for
    // the xtra data -- RICKTU).
    _sld.dwFlags &= (SLDF_HAS_EXP_SZ | SLDF_HAS_EXP_ICON_SZ | SLDF_RUN_IN_SEPARATE | SLDF_HAS_DARWINID | SLDF_HAS_LOGO3ID | SLDF_RUNAS_USER);

    if (_pszRelSource)
        _SetRelativePath(_pszRelSource);

#ifdef UNICODE
    _sld.dwFlags |= SLDF_UNICODE;
#endif

    fEncode = FALSE;
    
    if (_pidl)
    {
        _sld.dwFlags |= SLDF_HAS_ID_LIST;

        // we dont want to have special folder tracking for darwin links
        if (!(_sld.dwFlags & SLDF_HAS_DARWINID))
            fEncode = _EncodeSpecialFolder();
    }

    if (_pli)
        _sld.dwFlags |= SLDF_HAS_LINK_INFO;

    if (_pszName && _pszName[0])
        _sld.dwFlags |= SLDF_HAS_NAME;
    if (_pszRelPath && _pszRelPath[0])
        _sld.dwFlags |= SLDF_HAS_RELPATH;
    if (_pszWorkingDir && _pszWorkingDir[0])
        _sld.dwFlags |= SLDF_HAS_WORKINGDIR;
    if (_pszArgs && _pszArgs[0])
        _sld.dwFlags |= SLDF_HAS_ARGS;
    if (_pszIconLocation && _pszIconLocation[0])
        _sld.dwFlags |= SLDF_HAS_ICONLOCATION;

    hres = pstm->Write(&_sld, SIZEOF(_sld), &cbBytes);

    if (SUCCEEDED(hres) && (cbBytes == SIZEOF(_sld)))
    {
        if (_pidl)
            hres = ILSaveToStream(pstm, _pidl);

        if (SUCCEEDED(hres) && _pli)
            hres = LinkInfo_SaveToStream(pstm, _pli);

        if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_NAME))
            hres = Stream_WriteString(pstm, _pszName, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_RELPATH))
            hres = Stream_WriteString(pstm, _pszRelPath, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_WORKINGDIR))
            hres = Stream_WriteString(pstm, _pszWorkingDir, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_ARGS))
            hres = Stream_WriteString(pstm, _pszArgs, _sld.dwFlags & SLDF_UNICODE);
        if (SUCCEEDED(hres) && (_sld.dwFlags & SLDF_HAS_ICONLOCATION))
            hres = Stream_WriteString(pstm, _pszIconLocation, _sld.dwFlags & SLDF_UNICODE);

#ifdef WINNT
        if (SUCCEEDED(hres) && _ptracker && _ptracker->WasLoadedAtLeastOnce())
            hres = _UpdateTrackerData();
#endif

        if (SUCCEEDED(hres))
        {
#ifdef TEST_EXTRA_DATA
            if (bTestExtra)
            {
                BYTE buf[50];
                DATABLOCK_HEADER *pdb = (DATABLOCK_HEADER *)buf;
                LPSTR pstr = (LPSTR)(pdb+1);
                DATABLOCK_HEADER *pdbTest;
    
                // NULL block
                pdb->cbSize = SIZEOF(*pdb);
                pdb->dwSignature = (DWORD)-2;
                ASSERT(SHAddDataBlock(&(_pExtraData), pdb));
    
                // string block
                lstrcpy(pstr, TEXT("CG"));
                pdb->cbSize = SIZEOF(*pdb) + lstrlen(pstr) + 1;
                pdb->dwSignature = (DWORD)-3;
                ASSERT(SHAddDataBlock(&(_pExtraData), pdb));
    
                // string block
                lstrcpy(pstr, TEXT("123456"));
                pdb->cbSize = SIZEOF(*pdb) + lstrlen(pstr) + 1;
                pdb->dwSignature = (DWORD)-4;
                ASSERT(SHAddDataBlock(&(_pExtraData), pdb));

                // verify they were added
                pdbTest = SHFindDataBlock(_pExtraData, (DWORD)-2);
                if (EVAL(pdbTest))
                    ASSERT(SIZEOF(*pdb) == pdbTest->cbSize);

                pdbTest = SHFindDataBlock(_pExtraData, (DWORD)-3);
                if (EVAL(pdbTest))
                    ASSERT(!lstrcmp((LPSTR)(pdbTest+1), TEXT("CG")));

                pdbTest = SHFindDataBlock(_pExtraData, (DWORD)-4);
                if (EVAL(pdbTest))
                    ASSERT(!lstrcmp((LPSTR)(pdbTest+1), TEXT("123456")));
            }
#endif // TEST_EXTRA_DATA

            hres = SHWriteDataBlockList(pstm, _pExtraData);
        }

        if (SUCCEEDED(hres) && fClearDirty)
            _bDirty = FALSE;
    }
    else
    {
        DebugMsg(DM_TRACE, TEXT("Failed to write link"));
        hres = E_FAIL;
    }

    if (fEncode)
        _RemoveExtraDataSection(EXP_SPECIAL_FOLDER_SIG);

    return hres;
}

STDMETHODIMP  CShellLink::GetSizeMax(ULARGE_INTEGER *pcbSize)
{
    pcbSize->LowPart = 16 * 1024;       // 16k?  who knows...
    pcbSize->HighPart = 0;
    return S_OK;
}

BOOL PathIsPif(LPCTSTR pszPath)
{
    return lstrcmpi(PathFindExtension(pszPath), TEXT(".pif")) == 0;
}

HRESULT CShellLink::_LoadFromPIF(LPCTSTR szPath)
{
    PROPPRG ProgramProps;
    LPITEMIDLIST pidl = NULL;
    HANDLE hPif = PifMgr_OpenProperties((LPCTSTR)szPath, (LPCTSTR)NULL, 0, 0);

    if (hPif == 0)
        return E_FAIL;

    memset(&ProgramProps, 0, SIZEOF(ProgramProps));

    if (!PifMgr_GetProperties(hPif,(LPSTR)MAKEINTATOM(GROUP_PRG), &ProgramProps, SIZEOF(ProgramProps), 0))
    {
        DebugMsg(DM_TRACE, TEXT("_LoadFromPIF PifMgr_GetProperties *failed*"));
        return E_FAIL;
    }

#if 0
    DebugMsg(DM_TRACE, TEXT("    flPrg:     %04X     "), ProgramProps.flPrg);
    DebugMsg(DM_TRACE, TEXT("    flPrgInit: %04X     "), ProgramProps.flPrgInit);
    DebugMsg(DM_TRACE, TEXT("    Title:     %s       "), ProgramProps.achTitle);
    DebugMsg(DM_TRACE, TEXT("    CmdLine:   %s       "), ProgramProps.achCmdLine);
    DebugMsg(DM_TRACE, TEXT("    WorkDir:   %s       "), ProgramProps.achWorkDir);
    DebugMsg(DM_TRACE, TEXT("    HotKey:    %04X     "), ProgramProps.wHotKey);
    DebugMsg(DM_TRACE, TEXT("    Icon:      %s!%d    "), ProgramProps.achIconFile, ProgramProps.wIconIndex);
    DebugMsg(DM_TRACE, TEXT("    PIFFile:   %s       "), ProgramProps.achPIFFile);
#endif

#ifdef UNICODE
    {
        LPWSTR lpszTemp;
        UINT cchTitle;
        UINT cchWorkDir;
        UINT cchCmdLine;
        UINT cchIconFile;
        UINT cchMax;

        cchTitle    = lstrlenA(ProgramProps.achTitle)+1;
        cchWorkDir  = lstrlenA(ProgramProps.achWorkDir)+1;
        cchCmdLine  = lstrlenA(ProgramProps.achCmdLine)+1;
        cchIconFile = lstrlenA(ProgramProps.achIconFile)+1;
        cchMax = cchTitle;
        if ( cchWorkDir  > cchMax ) cchMax = cchWorkDir;
        if ( cchCmdLine  > cchMax ) cchMax = cchCmdLine;
        if ( cchIconFile > cchMax ) cchMax = cchIconFile;

        cchMax *= SIZEOF(WCHAR);                // For unicodizing

        lpszTemp = (LPWSTR)alloca(cchMax);    // For unicodizing

        MultiByteToWideChar(CP_ACP, 0, ProgramProps.achTitle, cchTitle,
                                                          lpszTemp, cchMax);
        SetDescription(lpszTemp);

        MultiByteToWideChar(CP_ACP, 0, ProgramProps.achWorkDir, cchWorkDir,
                                                          lpszTemp, cchMax);
        SetWorkingDirectory(lpszTemp);

        SetHotkey(ProgramProps.wHotKey);

        MultiByteToWideChar(CP_ACP, 0, ProgramProps.achIconFile, cchIconFile,
                                                          lpszTemp, cchMax);
        SetIconLocation(lpszTemp, ProgramProps.wIconIndex);

        MultiByteToWideChar(CP_ACP, 0, ProgramProps.achCmdLine, cchCmdLine,
                                                          lpszTemp, cchMax);
        SetArguments(PathGetArgs(lpszTemp));

        PathRemoveArgs(lpszTemp);

        // If this is a network path, we want to create a simple pidl
        // instead of a full pidl to circumvent net hits
        if (PathIsNetworkPath(lpszTemp))
            pidl = SHSimpleIDListFromPath( lpszTemp );
        else
            SetPIDLPath(NULL, lpszTemp, NULL);
    }
#else
    SetDescription(ProgramProps.achTitle);
    SetWorkingDirectory(ProgramProps.achWorkDir);
    SetArguments(PathGetArgs(ProgramProps.achCmdLine));
    SetHotkey(ProgramProps.wHotKey);
    SetIconLocation(ProgramProps.achIconFile, ProgramProps.wIconIndex);

    PathRemoveArgs(ProgramProps.achCmdLine);

    // If this is a network path, we want to create a simple pidl
    // instead of a full pidl to circumvent net hits
    if (PathIsNetworkPath(ProgramProps.achCmdLine))
        pidl = SHSimpleIDListFromPath( ProgramProps.achCmdLine );
    else
        SetPIDLPath(NULL, ProgramProps.achCmdLine, NULL);
#endif

    // if a simple pidl was created, use it here...
    if (pidl)
    {
        UpdateWorkingDir(pidl);

        if (_pidl)
            ILFree(_pidl);

        _pidl = pidl;
    }

    if (ProgramProps.flPrgInit & PRGINIT_MINIMIZED)
        SetShowCmd(SW_SHOWMINNOACTIVE);
    else if (ProgramProps.flPrgInit & PRGINIT_MAXIMIZED)
        SetShowCmd(SW_SHOWMAXIMIZED);
    else
        SetShowCmd(SW_SHOWNORMAL);

    PifMgr_CloseProperties(hPif, 0);

    _bDirty = FALSE;

    return S_OK;
}


//------------------------------------------------------------------------------------------------------
HRESULT CShellLink::_LoadFromFile(LPCTSTR pszPath)
{
    HRESULT hres;

    if (PathIsPif(pszPath))
        hres = _LoadFromPIF(pszPath);
    else
    {
        IStream *pstm;
        hres = SHCreateStreamOnFile(pszPath, STGM_READ, &pstm);
        if (SUCCEEDED(hres))
        {
            hres = Load(pstm);
            pstm->Release();
        }
    }

    if (SUCCEEDED(hres))
    {
        TCHAR szPath[MAX_PATH];

        if (_pidl && SHGetPathFromIDList(_pidl, szPath) && !lstrcmpi(szPath, pszPath))
        {
            DebugMsg(DM_TRACE, TEXT("Link points to itself, aaahhh!"));
            hres = E_FAIL;
        }
        else
        {
            Str_SetPtr(&_pszCurFile, pszPath);
        }
    }
    else if (IsFolderShortcut (pszPath))
    {
        TCHAR szPath[MAX_PATH];
        IStream *pstm;
        PathCombine(szPath, pszPath, TEXT("target.lnk"));
        hres = SHCreateStreamOnFile(szPath, STGM_READ, &pstm);
        if (SUCCEEDED(hres))
        {
            hres = Load(pstm);
            pstm->Release();
        }
    }

    ASSERT(!_bDirty);

    return hres;
}

STDMETHODIMP CShellLink::Load(LPCOLESTR pwszFile, DWORD grfMode)
{
    HRESULT hr = E_INVALIDARG;
    
    TraceMsg( TF_DEBUGLINKCODE, "Loading link from file %ls.", pwszFile );

    if (pwszFile) 
    {
        USES_CONVERSION;

        hr = _LoadFromFile(W2CT(pwszFile));

        // convert the succeeded code to NOERROR so that THOSE DUMB apps like HitNrun 
        // who do hr == 0 don't fail miserably.

        if ( SUCCEEDED( hr ))
            hr = NOERROR;    
    }
    
    return hr;
}

HRESULT CShellLink::_SaveAsLink(LPCTSTR szPath)
{
    TraceMsg( TF_DEBUGLINKCODE, "Save link to file %s.", szPath );

    IStream *pstm;
    HRESULT hres = SHCreateStreamOnFile(szPath, STGM_CREATE | STGM_WRITE | STGM_SHARE_DENY_WRITE, &pstm);
    if (SUCCEEDED(hres))
    {
        if (_pszRelSource == NULL)
            _SetRelativePath(szPath);

        hres = Save(pstm, TRUE);

        if (SUCCEEDED(hres))
        {
            hres = pstm->Commit(0);
        }

        pstm->Release();

        if (FAILED(hres))
        {
            DeleteFile(szPath);
        }
    }

    return hres;
}

BOOL RenameChangeExtension(LPTSTR pszPathSave, LPCTSTR pszExt, BOOL fMove)
{
    TCHAR szPathSrc[MAX_PATH];

    lstrcpy(szPathSrc, pszPathSave);
    PathRenameExtension(pszPathSave, pszExt);

    // this may fail because the source file does not exist, but we dont care
    if (fMove && lstrcmpi(szPathSrc, pszPathSave) != 0)
    {
        DWORD dwAttrib;

        PathYetAnotherMakeUniqueName(pszPathSave, pszPathSave, NULL, NULL);
        dwAttrib = GetFileAttributes( szPathSrc );
        if ((dwAttrib == 0xFFFFFFFF) || (dwAttrib & FILE_ATTRIBUTE_READONLY))
        {
            // Source file is read only, don't want to change the extension
            // because we won't be able to write any changes to the file...
            return FALSE;
        }
        Win32MoveFile(szPathSrc, pszPathSave, FALSE);
    }

    return TRUE;
}


// out:
//      pszDir  MAX_PATH path to get directory, maybe with env expanded
//
// returns:
//      TRUE    has a working directory, pszDir filled in.
//      FALSE   no working dir, if the env expands to larger than the buffer size (MAX_PATH)
//              this will be returned (FALSE)
//

BOOL CShellLink::_GetWorkingDir(LPTSTR pszDir)
{
    *pszDir = 0;

    return _pszWorkingDir && _pszWorkingDir[0] &&
           SHExpandEnvironmentStrings(_pszWorkingDir, pszDir, MAX_PATH);
}

HRESULT CShellLink::_SaveAsPIF(LPCTSTR pszPath, BOOL fPath)
{
    HANDLE hPif;
    PROPPRG ProgramProps;
    HRESULT hres;
    TCHAR   szDir[MAX_PATH];
    TCHAR    achPath[MAX_PATH];

    //
    // get filename and convert it to a short filename
    //
    if (fPath)
    {
        hres = GetPath(achPath, ARRAYSIZE(achPath), NULL, 0);
        PathGetShortPath(achPath);
        
        ASSERT(!PathIsPif(achPath));
        ASSERT(LOWORD(GetExeType(achPath)) == 0x5A4D);
        ASSERT(PathIsPif(pszPath));
        ASSERT(hres == S_OK);
    }
    else
    {
        lstrcpy(achPath, pszPath);
    }

    DebugMsg(DM_TRACE, TEXT("_SaveAsPIF(%s,%s)"), achPath, pszPath);

#if 0
    //
    // we should use OPENPROPS_INHIBITPIF to prevent PIFMGR from making a
    // temp .pif file in \windows\pif but it does not work now.
    //
    hPif = PifMgr_OpenProperties(achPath, pszPath, 0, OPENPROPS_INHIBITPIF);
#else
    hPif = PifMgr_OpenProperties(achPath, pszPath, 0, 0);
#endif

    if (hPif == 0)
        return E_FAIL;

    if (!PifMgr_GetProperties(hPif,(LPSTR)MAKEINTATOM(GROUP_PRG), &ProgramProps, SIZEOF(ProgramProps), 0))
    {
        DebugMsg(DM_TRACE, TEXT("_SaveToPIF: PifMgr_GetProperties *failed*"));
        hres = E_FAIL;
        goto Error1;
    }

    // Set a title based on the link name.
    if (_pszName && _pszName[0])
        SHTCharToAnsi(_pszName, ProgramProps.achTitle, SIZEOF(ProgramProps.achTitle));

    //
    // if no work dir. is given default to the dir of the app.
    //
    if (_GetWorkingDir(szDir))
    {
#ifdef UNICODE
        TCHAR szTemp[PIFDEFPATHSIZE];

        GetShortPathName(szDir, szTemp, ARRAYSIZE(szTemp));
        SHTCharToAnsi(szTemp, ProgramProps.achWorkDir, ARRAYSIZE(ProgramProps.achWorkDir));
#else
        GetShortPathName(szDir, ProgramProps.achWorkDir, ARRAYSIZE(ProgramProps.achWorkDir));
#endif
    }
    else if (fPath && !PathIsUNC(achPath))
    {
#ifdef UNICODE
        TCHAR szTemp[PIFDEFPATHSIZE];
        lstrcpyn(szTemp, achPath, ARRAYSIZE(szTemp));
        PathRemoveFileSpec(szTemp);
        SHTCharToAnsi(szTemp, ProgramProps.achWorkDir, ARRAYSIZE(ProgramProps.achWorkDir));
#else
        lstrcpyn(ProgramProps.achWorkDir, achPath, ARRAYSIZE(ProgramProps.achWorkDir));
        PathRemoveFileSpec(ProgramProps.achWorkDir);
#endif
    }

    // And for those network share points we need to quote blanks...
    PathQuoteSpaces(achPath);

    //
    // add the args to build the full command line
    //
    if (_pszArgs && _pszArgs[0])
    {
        lstrcat(achPath, c_szSpace);
        lstrcat(achPath, _pszArgs);
    }

    if (fPath)
        SHTCharToAnsi(achPath, ProgramProps.achCmdLine, ARRAYSIZE(ProgramProps.achCmdLine));

    if (_sld.iShowCmd == SW_SHOWMAXIMIZED)
        ProgramProps.flPrgInit |= PRGINIT_MAXIMIZED;
    if ((_sld.iShowCmd == SW_SHOWMINIMIZED) || (_sld.iShowCmd == SW_SHOWMINNOACTIVE))
        ProgramProps.flPrgInit |= PRGINIT_MINIMIZED;

    if (_sld.wHotkey)
        ProgramProps.wHotKey = _sld.wHotkey;

    if (_pszIconLocation && _pszIconLocation[0])
    {
        SHTCharToAnsi(_pszIconLocation, ProgramProps.achIconFile, ARRAYSIZE(ProgramProps.achIconFile));
        ProgramProps.wIconIndex = (WORD) _sld.iIcon;
    }

    if (!PifMgr_SetProperties(hPif, (LPSTR)MAKEINTATOM(GROUP_PRG), &ProgramProps, SIZEOF(ProgramProps), 0))
    {
        DebugMsg(DM_TRACE, TEXT("_SaveToPIF: PifMgr_SetProperties *failed*"));
        hres = E_FAIL;
    } 
    else 
    {
        hres = S_OK;
    }

    _bDirty = FALSE;

Error1:
    PifMgr_CloseProperties(hPif, 0);
    return hres;
}

// This will allow global hotkeys to be available immediately instead
// of having to wait for the StartMenu to pick them up.
// Similarly this will remove global hotkeys immediately if req.

const UINT c_rgHotKeyFolders[] = {
    CSIDL_PROGRAMS,
    CSIDL_COMMON_PROGRAMS,
    CSIDL_STARTMENU,
    CSIDL_COMMON_STARTMENU,
    CSIDL_DESKTOPDIRECTORY,
    CSIDL_COMMON_DESKTOPDIRECTORY,
    -1,
};

void HandleGlobalHotkey(LPCTSTR pszFile, WORD wHotkeyOld, WORD wHotkeyNew)
{
    if (PathIsEqualOrSubFolderOf(c_rgHotKeyFolders, pszFile))
    {
        // Find tray?
        HWND hwndTray = FindWindow(TEXT(WNDCLASS_TRAYNOTIFY), 0);
        if (hwndTray)
        {
            // Yep.
            if (wHotkeyOld)
                SendMessage(hwndTray, WMTRAY_SCUNREGISTERHOTKEY, wHotkeyOld, 0);
            if (wHotkeyNew)
            {
                ATOM atom = GlobalAddAtom(pszFile);
                if (atom)
                {
                    SendMessage(hwndTray, WMTRAY_SCREGISTERHOTKEY, wHotkeyNew, (LPARAM)atom);
                    GlobalDeleteAtom(atom);
                }
            }
        }
    }
}

HRESULT CShellLink::_SaveToFile(LPTSTR pszPathSave, BOOL fRemember)
{
    HRESULT hres = E_FAIL;
    BOOL bFileExisted;
    BOOL fDosApp;
    BOOL fFile;
    TCHAR szPathSrc[MAX_PATH];
    BOOL fWasSameFile = _pszCurFile && (lstrcmpi(pszPathSave, _pszCurFile) == 0);

    // when saving darwin links we dont want to resolve the path
    if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {
        fRemember = FALSE;
        hres = _SaveAsLink(pszPathSave);
        goto Update;
    }

    GetPath(szPathSrc, ARRAYSIZE(szPathSrc), NULL, 0);

    fFile = !(_sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    fDosApp = fFile && LOWORD(GetExeType(szPathSrc)) == 0x5A4D;
    bFileExisted = PathFileExistsAndAttributes(pszPathSave, NULL);

    //
    // handle a link to link case. (or link to pif)
    //
    // BUGBUG we loose all new attributes, need to look into what
    // a progman item to a .PIF file did in Win31.  who set the hotkey,
    // work dir etc.  we definitly loose the icon.
    //
    if (fFile && (PathIsPif(szPathSrc) || PathIsLnk(szPathSrc)))
    {
        if (RenameChangeExtension(pszPathSave, PathFindExtension(szPathSrc), fWasSameFile))
        {
            if (CopyFile(szPathSrc, pszPathSave, FALSE))
            {
                if (PathIsPif(pszPathSave))
                    hres = _SaveAsPIF(pszPathSave, FALSE);
                else
                    hres = S_OK;
            }
        }
        else
        {
            hres = E_FAIL;
        }
    }

    //
    //  if the linked to file is a DOS app, we need to write a .PIF file
    //
    else if (fDosApp)
    {
        if (RenameChangeExtension(pszPathSave, TEXT(".pif"), fWasSameFile))
        {
            hres = _SaveAsPIF(pszPathSave, TRUE);
        }
        else
        {
            hres = E_FAIL;
        }
    }
    //
    //  else write a link file
    //
    else
    {
        if (PathIsPif(pszPathSave))
        {
            if (!RenameChangeExtension(pszPathSave, TEXT(".lnk"), fWasSameFile))
            {
                hres = E_FAIL;
                goto Update;
            }
        }


        hres = _SaveAsLink(pszPathSave);
    }

Update:
    if (SUCCEEDED(hres))
    {
        // Knock out file close
        SHChangeNotify(bFileExisted ? SHCNE_UPDATEITEM : SHCNE_CREATE, SHCNF_PATH, pszPathSave, NULL);
        SHChangeNotify(SHCNE_FREESPACE, SHCNF_PATH, pszPathSave, NULL);

        if (_wOldHotkey != _sld.wHotkey)
            HandleGlobalHotkey(pszPathSave, _wOldHotkey, _sld.wHotkey);

        if (fRemember)
            Str_SetPtr(&_pszCurFile, pszPathSave);
    }

    return hres;
}

STDMETHODIMP CShellLink::Save(LPCOLESTR pwszFile, BOOL fRemember)
{
    TCHAR szSavePath[MAX_PATH];

    if (pwszFile == NULL)
    {
        if (_pszCurFile == NULL)
            return E_FAIL;    // fail

        lstrcpy(szSavePath, _pszCurFile);
    }
    else
    {
        SHUnicodeToTChar(pwszFile, szSavePath, ARRAYSIZE(szSavePath));
    }

    return _SaveToFile(szSavePath, fRemember);
}

STDMETHODIMP CShellLink::SaveCompleted(LPCOLESTR pwszFile)
{
    return S_OK;
}

STDMETHODIMP CShellLink::GetCurFile(LPOLESTR *ppszFile)
{
    if (_pszCurFile == NULL)
    {
        *ppszFile = NULL;
        return S_FALSE;
    }
    return SHStrDup(_pszCurFile, ppszFile);
}



//------------------------------------------------------------------------------------------------------
STDMETHODIMP CShellLink::Initialize(LPCITEMIDLIST pidlFolder, IDataObject *pdtobj, HKEY hkeyProgID)
{
    HRESULT hres;

    ASSERT(_sld.iShowCmd == SW_SHOWNORMAL);

    if (pdtobj)
    {
        STGMEDIUM medium;
        FORMATETC fmte = {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};

        hres = pdtobj->GetData(&fmte, &medium);
        if (SUCCEEDED(hres))
        {
            TCHAR szPath[MAX_PATH];
            DragQueryFile((HDROP)medium.hGlobal, 0, szPath, ARRAYSIZE(szPath));
            hres = _LoadFromFile(szPath);

            ReleaseStgMedium(&medium);
        }
    }
    else
        hres = E_FAIL;

    return hres;
}

DWORD CShellLink::_VerifyPathThreadProc(void *pv)
{
    LPTSTR  pszTarget = (LPTSTR)pv;
    BOOL    fRootExists;
    TCHAR   szRoot[MAX_PATH];

    lstrcpyn(szRoot, pszTarget, ARRAYSIZE(szRoot));
    LocalFree((HLOCAL)pszTarget);

    PathStripToRoot(szRoot);

    if (PathIsUNC(szRoot))
    {
        fRootExists = NetPathExists( szRoot, NULL );
    }
    else
    {
        fRootExists = PathFileExists(szRoot);
    }

    return fRootExists;
}

STDAPI _DarwinItemsMenuCB(IShellFolder *psf, HWND hwnd, IDataObject *pdtobj, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hres = S_OK;
    LPITEMIDLIST pidl;

    switch (uMsg) 
    {
        case DFM_MERGECONTEXTMENU:
        {
            UINT uFlags = (UINT)wParam;
            LPQCMINFO pqcm = (LPQCMINFO)lParam;

            CDefFolderMenu_MergeMenu(HINST_THISDLL,
                                     (uFlags & CMF_EXTENDEDVERBS) ? MENU_GENERIC_CONTROLPANEL_VERBS : MENU_GENERIC_OPEN_VERBS,  // if extended verbs then add "Run as..."
                                     0,
                                     pqcm);

            SetMenuDefaultItem(pqcm->hmenu, 0, MF_BYPOSITION);

            //
            //  Returning S_FALSE indicates no need to get verbs from
            // extensions.
            //

            hres = S_FALSE;
        
            break;
        }

        case DFM_GETHELPTEXT:
            LoadStringA(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPSTR)lParam, HIWORD(wParam));
            break;

        case DFM_GETHELPTEXTW:
            LoadStringW(HINST_THISDLL, LOWORD(wParam) + IDS_MH_FSIDM_FIRST, (LPWSTR)lParam, HIWORD(wParam));
            break;

        case DFM_INVOKECOMMANDEX:
            INSTRUMENT_INVOKECOMMAND(SHCNFI_FOLDER_DFM_INVOKE, hwnd, wParam);
            switch (wParam)
            {
                case FSIDM_OPENPRN:
                case FSIDM_RUNAS:
                    hres = PidlFromDataObject(pdtobj, &pidl);
                    if (SUCCEEDED(hres))
                    {
                        CMINVOKECOMMANDINFOEX iciex;
                        SHELLEXECUTEINFO sei;
                        DFMICS* pdfmics = (DFMICS *)lParam;
                        LPVOID pvFree;

                        ICI2ICIX(pdfmics->pici, &iciex, &pvFree);
                        ICIX2SEI(&iciex, &sei);
                        sei.fMask |= SEE_MASK_IDLIST;
                        sei.lpIDList = pidl;

                        if (wParam == FSIDM_RUNAS)
                        {
                            // we only set the verb in the "Run As..." case since we want
                            // the "open" verb for darwin links to really execute the default action.
                            sei.lpVerb = TEXT("runas");
                        }

                        ShellExecuteEx(&sei);
                        
                        ILFree(pidl);
                        if (pvFree)
                        {
                            LocalFree(pvFree);
                        }
                    }
                    // Never return E_NOTIMPL or defcm will try to do a default thing
                    if (hres == E_NOTIMPL)
                        hres = E_FAIL;
                    break;

                // BUGBUG raymondc - defcm needs to support GetCommandString too.
    
                default:
                    // This is common menu items, use the default code.
                    hres = S_FALSE;
                    break;
        }
        break;

    default:
        hres = E_NOTIMPL;
        break;
    }
    return hres;
}

//
// CShellLink::CreateDarwinContextMenuForPidl (non-virtual)
//
// Worker function for CShellLink::CreateDarwinContextMenu that tries
// to create the context menu for the specified pidl.

HRESULT _CreateDarwinContextMenuForPidl(HWND hwnd, LPCITEMIDLIST pidlTarget, IContextMenu **pcmOut)
{
    HRESULT hres;
    LPITEMIDLIST pidlFolder, pidlItem;

    hres = SHILClone(pidlTarget, &pidlFolder);

    if (SUCCEEDED(hres))
    {
        if (ILRemoveLastID(pidlFolder) &&
            (pidlItem = ILFindLastID(pidlTarget)))
        {
            IShellFolder *psf;
            hres = SHBindToObject(NULL, IID_IShellFolder, pidlFolder, (void **)&psf);
            if (SUCCEEDED(hres))
            {
                HKEY ahkeys[1] = { NULL }; // BUGBUG or "HKCR\MsiShortcut"

                hres = CDefFolderMenu_Create2(
                            pidlFolder,
                            hwnd,
                            1, (LPCITEMIDLIST *)&pidlItem, psf, _DarwinItemsMenuCB,
                            ARRAYSIZE(ahkeys), ahkeys, pcmOut);

                SHRegCloseKeys(ahkeys, ARRAYSIZE(ahkeys));
                psf->Release();
            }
        }
        else
        {
            // Darwin shortcut to the desktop?  I don't think so.
            hres = E_FAIL;
        }
        ILFree(pidlFolder);
    }
    return hres;
}

//
// CShellLink::CreateDarwinContextMenu (non-virtual)
//
// Creates a context menu for a Darwin shortcut.  This is special because
// the ostensible target is an .EXE file, but in reality it could be
// anything.  (It's just an .EXE file until the shortcut gets resolved.)
// Consequently, we can't create a real context menu for the item because
// we don't know what kind of context menu to create.  We just cook up
// a generic-looking one.
//
// Bonus annoyance: _pidl might be invalid, so you need to have
// a fallback plan if it's not there.  We will use c_idlDrives as our
// fallback.  That's a pidl guaranteed actually to exist.
//
// Note that this means you can't invoke a command on the fallback object,
// but that's okay because ShellLink will always resolve the object to
// a real file and create a new context menu before invoking.
//

HRESULT CShellLink::_CreateDarwinContextMenu(HWND hwnd, IContextMenu **pcmOut)
{
    HRESULT hres;

    *pcmOut = NULL;

    if (_pidl == NULL ||
        FAILED(_CreateDarwinContextMenuForPidl(hwnd, _pidl, pcmOut)))
    {
        // The link target is busted for some reason - use the fallback pidl
        hres = _CreateDarwinContextMenuForPidl(hwnd, (LPCITEMIDLIST)&c_idlDrives, pcmOut);
    }

    return hres;
}

BOOL CShellLink::_GetExpPath(LPTSTR psz, DWORD cch)
{
    LPEXP_SZ_LINK pesl = (LPEXP_SZ_LINK)SHFindDataBlock(_pExtraData, EXP_SZ_LINK_SIG);
    if (pesl) 
    {
        TCHAR sz[MAX_PATH];
        *sz = 0;
        
        // prefer the UNICODE version...
        if (*pesl->swzTarget)
            SHUnicodeToTChar(pesl->swzTarget, sz, SIZECHARS(sz));

        if (!*sz && *pesl->szTarget)
            SHAnsiToTChar(pesl->szTarget, sz, SIZECHARS(sz));

        if (*sz)
        {
            return SHExpandEnvironmentStrings(sz, psz, cch);
        }
    }

    return FALSE;
}

#define DEFAULT_TIMEOUT      7500

DWORD g_dwNetLinkTimeout = (DWORD)-1;

DWORD _GetNetLinkTimeout()
{
    DWORD dwNetLinkTimeout;

    if (g_dwNetLinkTimeout == -1)
    {
        HKEY hkey = SHGetExplorerHkey(HKEY_CURRENT_USER, TRUE);
        if (hkey)
        {
            DWORD dwSize = SIZEOF(dwNetLinkTimeout);

            // read in the registry value
            if (SHQueryValueEx(hkey, TEXT("NetLinkTimeout"), NULL, NULL,
                          &dwNetLinkTimeout, &dwSize) == ERROR_SUCCESS)
            {
                g_dwNetLinkTimeout = dwNetLinkTimeout;
                RegCloseKey(hkey);
                return g_dwNetLinkTimeout;
            }
            RegCloseKey(hkey);
        }
        g_dwNetLinkTimeout = 0;
    }

    if (g_dwNetLinkTimeout == 0)
    {
        dwNetLinkTimeout = DEFAULT_TIMEOUT;
        return dwNetLinkTimeout;
    }
    return g_dwNetLinkTimeout;
}


// since net timeouts can be very long this is a manual way to timeout
// an operation explictly rather than waiting for the net layers to do their
// long timeouts

HRESULT CShellLink::_ShortNetTimeout()
{
    TCHAR szPath[MAX_PATH];

    if (_pidl && SHGetPathFromIDList(_pidl, szPath) && PathIsNetworkPath(szPath))
    {
        LPTSTR pszTarget = StrDup(szPath);
        if (pszTarget)
        {
            DWORD dwID;
            HANDLE hThread = CreateThread(NULL, 0, _VerifyPathThreadProc, pszTarget, 0, &dwID);
            if (NULL == hThread)
            {
                LocalFree((HLOCAL)pszTarget);
            }
            else
            {
                DWORD dwRetVal;
                DWORD dwWaitResult = WaitForSingleObject(hThread, _GetNetLinkTimeout());

                if (WAIT_OBJECT_0 != dwWaitResult)
                {
                    CloseHandle(hThread);
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                }
                GetExitCodeThread(hThread, &dwRetVal);

                if (!dwRetVal)
                {
                    CloseHandle(hThread);
                    return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
                }
                CloseHandle(hThread);
            }
        }
    }
    return S_OK;
}


//
// This function returns the specified UI object from the link source.
//
// Parameters:
//  hwnd   -- optional hwnd for UI (for drop target)
//  riid   -- Specifies the interface (IID_IDropTarget, IID_IExtractIcon, IID_IContextMenu, ...)
//  ppv    -- Specifies the place to return the pointer.
//
// Notes:
//  Don't put smart-resolving code here. Such a thing should be done
//  BEFORE calling this function.
//

HRESULT CShellLink::_GetUIObject(HWND hwnd, REFIID riid, void **ppv)
{
    *ppv = NULL;     // Do this once and for all

    // We commandeer a couple of IIDs if this is a Darwin link.
    // Must do this before any pseudo-resolve goo because Darwin
    // shortcuts don't resolve the normal way.

    if (_sld.dwFlags & SLDF_HAS_DARWINID)
    {
        if (IsEqualIID(riid, IID_IContextMenu))
        {
            // Custom Darwin context menu.
            return _CreateDarwinContextMenu(hwnd, (IContextMenu **)ppv);
        }
        else if (IsEqualIID(riid, IID_IDropTarget))
        {
            // Never allow drag/drop to a Darwin shortcut because
            // even though it's an .EXE today, installing the package
            // might turn it into a pumpkin.
            return E_FAIL;
        }
    }

    if (_sld.dwFlags & SLDF_HAS_EXP_SZ)
    {
        TCHAR szPath[MAX_PATH];
        if (_GetExpPath(szPath, SIZECHARS(szPath)))
        {
            if (_pidl)
                ILFree( _pidl );

            WIN32_FIND_DATA fd = {0};
            fd.dwFileAttributes = _sld.dwFileAttributes;
            
            SHSimpleIDListFromFindData(szPath, &fd, &_pidl);

            // we are modifing _pidl.  We should never have a EXP_SPECIAL_FOLDER_SIG section
            // when we modify the _pidl, otherwise the two will be out of ssync
            ASSERT( NULL == SHFindDataBlock(_pExtraData, EXP_SPECIAL_FOLDER_SIG) );
        } 
        else 
        {
            return E_FAIL;
        }
    }

    if (_pidl)
        return SHGetUIObjectFromFullPIDL(_pidl, hwnd, riid, ppv);
    else
        return E_FAIL;
}



STDMETHODIMP CShellLink::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    HRESULT hr;

    if (_pcmTarget == NULL)
    {
        hr = _GetUIObject(NULL, IID_IContextMenu, (void **)&_pcmTarget);
        if (FAILED(hr))
            return hr;

        ASSERT(_pcmTarget);
    }

    // save these if in case we need to rebuild the cm because the resolve change the
    // target of the link

    _indexMenuSave = indexMenu;
    _idCmdFirstSave = idCmdFirst;
    _idCmdLastSave = idCmdLast;
    _uFlagsSave = uFlags;

    uFlags |= CMF_VERBSONLY;

    if (_sld.dwFlags & SLDF_RUNAS_USER)
    {
        // "runas" for exe's is an extenede verb, so we have to ask for those as well.
        uFlags |= CMF_EXTENDEDVERBS;
    }

    hr = _pcmTarget->QueryContextMenu(hmenu, indexMenu, idCmdFirst, idCmdLast, uFlags);

    // set default verb to "runas" if the "Run as different user" checkbox is checked
    if (SUCCEEDED(hr) && (_sld.dwFlags & SLDF_RUNAS_USER))
    {
        int i = GetMenuIndexForCanonicalVerb(hmenu, _pcmTarget, idCmdFirst, L"runas");

        if (i != -1)
        {
            // we found runas, so set it as the default
            SetMenuDefaultItem(hmenu, i, MF_BYPOSITION);
        }
        else
        {
            // the checkbox was enabled and checked, which means that the "runas" verb was supposed
            // to be in the context menu, but we couldnt find it.
            ASSERTMSG(FALSE, "CSL::QueryContextMenu - failed to set 'runas' as default context menu item!");
        }
    }

    return hr;
}


// BUGBUG: CANONICAL_VERB_NAME fixes some bugs where the implementaton of the
// context menu changes when the shortcut becomes dirty.  but causes some
// others where the targtets don't implement cananonical verbs... so we leave
// this turned off for win95.  we should fix this later.


HRESULT CShellLink::InvokeCommandAsync(LPCMINVOKECOMMANDINFO pici)
{
    HRESULT hres;
#ifdef CANONICAL_VERB_NAME
    TCHAR szVerb[32];
#endif

    TCHAR szWorkingDir[MAX_PATH];
    LPTSTR lpDirectory = NULL;
#ifdef UNICODE
    CHAR szWorkingDirAnsi[MAX_PATH];
#endif

    if (_pcmTarget == NULL)
        return E_FAIL;

#ifdef CANONICAL_VERB_NAME
    szVerb[0] = 0;

    // if needed, get the canonical name in case the IContextMenu changes as
    // a result of the resolve call BUT only do this for folders (to be safe)
    // as that is the only case where this happens
    // sepcifically we resolve from a D:\ -> \\SERVER\SHARE

    if (HIWORD(pici->lpVerb) == 0 && (_sld.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        _pcmTarget->GetCommandString(LOWORD(pici->lpVerb), GCS_VERB, NULL, szVerb, ARRAYSIZE(szVerb));
#endif

    ASSERT(!_bDirty);

    // we pass SLR_ENVOKE_MSI since we WANT to invoke darwin since we are
    // really going to execute the link now
    DWORD slrFlags = SLR_INVOKE_MSI;
    if (pici->fMask & CMIC_MASK_FLAG_NO_UI)
        slrFlags |= SLR_NO_UI;
        
    hres = ResolveLink(pici->hwnd, slrFlags, 0);

    if (hres == S_OK)
    {
        if (_bDirty)
        {
            // the context menu we have for this link is out of date, free it
            _pcmTarget->Release();
            _pcmTarget = NULL;

            hres = _GetUIObject(NULL, IID_IContextMenu, (void **)&_pcmTarget);
            if (SUCCEEDED(hres))
            {
                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    DebugMsg(DM_TRACE, TEXT("rebuilding cm after link resolve"));

                    hres = _pcmTarget->QueryContextMenu(hmenu, _indexMenuSave, _idCmdFirstSave, _idCmdLastSave, _uFlagsSave | CMF_VERBSONLY);

                    DestroyMenu(hmenu);
                }
            }

            // don't really care if this fails...
            Save((LPCOLESTR)NULL, TRUE);
        }
#ifdef CANONICAL_VERB_NAME
        else
        {
            szVerb[0] = 0;
            ASSERT(SUCCEEDED(hres));
        }
#endif

        if (SUCCEEDED(hres))
        {
            TCHAR szArgs[MAX_PATH];
            TCHAR szExpArgs[MAX_PATH];
            CMINVOKECOMMANDINFOEX ici;
#ifdef UNICODE
            CHAR szArgsAnsi[MAX_PATH];
#endif

            if (pici->cbSize > SIZEOF(CMINVOKECOMMANDINFOEX))
            {
                memcpy(&ici, pici, SIZEOF(CMINVOKECOMMANDINFOEX));
                // BUGBUG - We should probably alloc another use it to retain size
                ici.cbSize = SIZEOF(CMINVOKECOMMANDINFOEX);
            }
            else
            {
                memset(&ici, 0, SIZEOF(ici));
                memcpy(&ici, pici, pici->cbSize);
                ici.cbSize = SIZEOF(ici);
            }

#ifdef CANONICAL_VERB_NAME
            if (szVerb[0])
            {
                DebugMsg(DM_TRACE, TEXT("Mapping cmd %d to verb %s"), LOWORD(pici->lpVerb), szVerb);
                ici.lpVerb = szVerb;
            }
#endif
            // build the args from those passed in cated on the end of the the link args

            lstrcpyn(szArgs, _pszArgs ? _pszArgs : c_szNULL, ARRAYSIZE(szArgs));
            if (ici.lpParameters)
            {
                int nArgLen = lstrlen(szArgs);
                LPCTSTR lpParameters;
#ifdef UNICODE
                WCHAR szParameters[MAX_PATH];

                if (ici.cbSize < CMICEXSIZE_NT4
                    || (ici.fMask & CMIC_MASK_UNICODE) != CMIC_MASK_UNICODE)
                {
                    SHAnsiToUnicode(ici.lpParameters, szParameters, ARRAYSIZE(szParameters));
                    lpParameters = szParameters;
                }
                else
                {
                    lpParameters = ici.lpParametersW;
                }
#else
                lpParameters = ici.lpParameters;
#endif
                lstrcpyn(szArgs + nArgLen, c_szSpace, ARRAYSIZE(szArgs) - nArgLen - 1);
                lstrcpyn(szArgs + nArgLen + 1, lpParameters, ARRAYSIZE(szArgs) - nArgLen - 2);
            }

            // Expand environment strings in szArgs
            SHExpandEnvironmentStrings(szArgs, szExpArgs, ARRAYSIZE(szExpArgs));

#ifdef UNICODE
            SHUnicodeToAnsi(szExpArgs, szArgsAnsi, ARRAYSIZE(szArgsAnsi));
            ici.lpParameters = szArgsAnsi;
            ici.lpParametersW = szExpArgs;
            ici.fMask |= CMIC_MASK_UNICODE;
#else
            ici.lpParameters = szExpArgs;
#endif

            // if we have a working dir in the link over ride what is passed in

            if (_GetWorkingDir(szWorkingDir))
            {
                if (PathIsDirectory(szWorkingDir))
                    lpDirectory = szWorkingDir;
                if ( lpDirectory )
                {
#ifdef UNICODE
                    SHUnicodeToAnsi(lpDirectory, szWorkingDirAnsi, ARRAYSIZE(szWorkingDirAnsi));
                    ici.lpDirectory = szWorkingDirAnsi;
                    ici.lpDirectoryW = lpDirectory;
#else
                    ici.lpDirectory = lpDirectory;
#endif
                }
            }


#ifdef WINNT
            // set RUN IN SEPARATE VDM if needed
            if (_sld.dwFlags & SLDF_RUN_IN_SEPARATE)
            {
                ici.fMask |= CMIC_MASK_FLAG_SEP_VDM;
            }
#endif

            // and of course use our hotkey

            if (_sld.wHotkey)
            {
                ici.dwHotKey = _sld.wHotkey;
                ici.fMask |= CMIC_MASK_HOTKEY;
            }

            // override normal runs, but let special show cmds through

            if (ici.nShow == SW_SHOWNORMAL)
            {
                DebugMsg(DM_TRACE, TEXT("using shorcut show cmd"));
                ici.nShow = _sld.iShowCmd;
            }

#ifdef WINNT
            //
            // On NT we want to pass the title to the
            // thing that we are about to start.
            //
            // CMIC_MASK_HASLINKNAME means that the lpTitle is really
            // the full path to the shortcut.  The console subsystem
            // sees the bit and sucks all his properties directly from
            // the LNK file.
            //
            if (!(ici.fMask & CMIC_MASK_HASLINKNAME) && !(ici.fMask & CMIC_MASK_HASTITLE))
            {
                if (_pszCurFile)
                {
#ifdef UNICODE
                    ici.lpTitle = NULL;     // Title is one or the other...
                    ici.lpTitleW = _pszCurFile;
#else
                    ici.lpTitle = _pszCurFile;
#endif
                    ici.fMask |= CMIC_MASK_HASLINKNAME | CMIC_MASK_HASTITLE;
                }
            }
#endif
            ASSERT((ici.nShow > SW_HIDE) && (ici.nShow <= SW_MAX));

            hres = _pcmTarget->InvokeCommand((LPCMINVOKECOMMANDINFO)&ici);
        }
    }
    return hres;
}


// Structure which encapsulates the paramters needed for InvokeCommand (so
// that we can pass both parameters though a single LPARAM in CreateThread)

typedef struct
{
    IContextMenu3         * pcm;
    CShellLink            * pObj;
    CMINVOKECOMMANDINFOEX   ici;
} ICMPARAMS;

#define ICM_BASE_SIZE (SIZEOF(ICMPARAMS) - SIZEOF(CMINVOKECOMMANDINFOEX))


// Runs as a separate thread, does the actual work of calling the "real"
// InvokeCommand

DWORD CALLBACK CShellLink::_InvokeThreadProc(void *pv)
{
    HRESULT hr, hrInit;
    ICMPARAMS * pParams = (ICMPARAMS *) pv;
    CShellLink *pObj = pParams->pObj;

    // See ccstock.h for explanation of SHCoInitialize semantics
    hrInit = SHCoInitialize();

    hr = pObj->InvokeCommandAsync((LPCMINVOKECOMMANDINFO)&pParams->ici);

    pParams->pcm->Release();

    SHCoUninitialize(hrInit);
    
    LocalFree(pParams);
    return (DWORD) hr;
}


// CShellLink::InvokeCommand
//
// Function that spins a thread to do the real work, which has been moved into
// CShellLink::InvokeCommandASync.

HRESULT CShellLink::InvokeCommand(LPCMINVOKECOMMANDINFO piciIn)
{
    HRESULT                   hr = S_OK;
    HANDLE                    hThread;
    DWORD                     dwID;
    DWORD                     cbSize;
    DWORD                     cbBaseSize;
    ICMPARAMS               * pParams;
    CHAR                    * pPos;
    DWORD                     cchVerb,
                              cchParameters,
                              cchDirectory;
#ifdef UNICODE
    DWORD                     cchVerbW,         // Last 3 are unused ifndef Unicode
                              cchParametersW,
                              cchDirectoryW;
#endif


    LPCMINVOKECOMMANDINFOEX   pici = (LPCMINVOKECOMMANDINFOEX) piciIn;

#ifdef UNICODE
    WCHAR                 *   pPosW;
    const BOOL                fUnicode = pici->cbSize >= CMICEXSIZE_NT4 &&
                                         (pici->fMask & CMIC_MASK_UNICODE) == CMIC_MASK_UNICODE;
#endif

    if (0 == (piciIn->fMask & CMIC_MASK_ASYNCOK))
    {
        // Caller didn't indicate that Async startup was OK, so we call
        // InvokeCommandAync SYNCHRONOUSLY
        return InvokeCommandAsync(piciIn);
    }

    // Calc how much space we will need to duplicate the INVOKECOMMANDINFO
    cbBaseSize = ICM_BASE_SIZE + max(piciIn->cbSize, sizeof(CMINVOKECOMMANDINFOEX));


    //   One byte slack in case of Unicode roundup for pPosW, below
    cbSize = cbBaseSize + 1;

    if (HIWORD(pici->lpVerb))
    {
        cbSize += (cchVerb   = pici->lpVerb       ? (lstrlenA(pici->lpVerb) + 1)       : 0 ) * SIZEOF(CHAR);
    }
    cbSize += (cchParameters = pici->lpParameters ? (lstrlenA(pici->lpParameters) + 1) : 0 ) * SIZEOF(CHAR);
    cbSize += (cchDirectory  = pici->lpDirectory  ? (lstrlenA(pici->lpDirectory) + 1)  : 0 ) * SIZEOF(CHAR);

#ifdef UNICODE
    if (HIWORD(pici->lpVerbW))
    {
        cbSize += (cchVerbW  = pici->lpVerbW      ? (lstrlenW(pici->lpVerbW) + 1)       : 0 ) * SIZEOF(WCHAR);
    }
    cbSize += (cchParametersW= pici->lpParametersW? (lstrlenW(pici->lpParametersW) + 1) : 0 ) * SIZEOF(WCHAR);
    cbSize += (cchDirectoryW = pici->lpDirectoryW ? (lstrlenW(pici->lpDirectoryW) + 1)  : 0 ) * SIZEOF(WCHAR);
#endif

    pParams = (ICMPARAMS *) LocalAlloc(LPTR, cbSize);
    if (NULL == pParams)
    {
        return (hr = E_OUTOFMEMORY);
    }

    // Text data will start going in right after the structure

    pPos = (CHAR *)((LPBYTE)pParams + cbBaseSize);

    // Start with a copy of the static fields

    CopyMemory(&pParams->ici, pici, pici->cbSize);
    AddRef();
    pParams->pcm = (IContextMenu3 *)this;

    // Walk along and dupe all of the string pointer fields

    if (HIWORD(pici->lpVerb))
    {
        pPos += cchVerb   ? lstrcpyA(pPos, pici->lpVerb), pParams->ici.lpVerb = pPos, cchVerb   : 0;
    }
    pPos += cchParameters ? lstrcpyA(pPos, pici->lpParameters), pParams->ici.lpParameters = pPos, cchParameters : 0;
    pPos += cchDirectory  ? lstrcpyA(pPos, pici->lpDirectory),  pParams->ici.lpDirectory  = pPos, cchDirectory  : 0;

#ifdef UNICODE

    pPosW = (WCHAR *) ((DWORD_PTR)pPos & 0x1 ? pPos + 1 : pPos);   // Ensure Unicode alignment

    if (HIWORD(pici->lpVerbW))
    {
        pPosW += cchVerbW  ? lstrcpyW(pPosW, pici->lpVerbW), pParams->ici.lpVerbW = pPosW, cchVerbW : 0;
    }
    pPosW += cchParametersW? lstrcpyW(pPosW, pici->lpParametersW),pParams->ici.lpParametersW= pPosW, cchParametersW : 0;
    pPosW += cchDirectoryW ? lstrcpyW(pPosW, pici->lpDirectoryW), pParams->ici.lpDirectoryW = pPosW, cchDirectoryW  : 0;
#endif

    // Pass all of the info off to the worker thread that will call the actual
    // InvokeCommand API for us

    //Set the object pointer to this object
    pParams->pObj  = (CShellLink *)this;
    
    hThread = CreateThread(NULL, 0, _InvokeThreadProc, (void *)pParams, 0, &dwID);
    if (NULL == hThread)
    {
        // Couldn't start the thread, so the onus is on us to clean up

        pParams->pcm->Release();
        LocalFree(pParams);
        hr = E_OUTOFMEMORY;
    }
    else
    {
        DECLAREWAITCURSOR;
        SetWaitCursor();

        // We give the async thread a little time to complete, during which we
        // put up the busy cursor.  This is solely to let the user see that
        // some work is being done...

        #define ASYNC_CMIC_TIMEOUT 750

        if (WAIT_OBJECT_0 == WaitForSingleObject(hThread, ASYNC_CMIC_TIMEOUT))
        {
            // For consistency, we always return S_OK, but if you wanted to, you could
            // return the actual return value from InvokeCommand in those cases where
            // it completed in time like this:
            //
            // DWORD dwRetVal;
            // if (GetExitCodeThread(hThread, &dwRetVal))
            // {
            //    hr = (HRESULT) dwRetVal;
            // }
        }

        CloseHandle(hThread);
        ResetWaitCursor();
    }

    return hr;
}

HRESULT CShellLink::GetCommandString(UINT_PTR idCmd, UINT wFlags, UINT *pmf, LPSTR pszName, UINT cchMax)
{
    VDATEINPUTBUF(pszName, TCHAR, cchMax);
    
    if (_pcmTarget)
        return _pcmTarget->GetCommandString(idCmd, wFlags, pmf, pszName, cchMax);
    else
        return E_FAIL;
}

STDMETHODIMP CShellLink::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
    if (_pcmTarget)
    {
        HRESULT hres = E_FAIL;
        IContextMenu3 *pcm = NULL;
        
        if (SUCCEEDED(_pcmTarget->QueryInterface(IID_IContextMenu3, (void **)&pcm)))
            hres = pcm->HandleMenuMsg2(uMsg, wParam, lParam, lResult);
        else if (!lResult && SUCCEEDED(_pcmTarget->QueryInterface(IID_IContextMenu2, (void **)&pcm)))
            hres = pcm->HandleMenuMsg(uMsg, wParam, lParam);

        if (pcm)
            pcm->Release();

        return hres;
    }
    
    return S_OK;
}

STDMETHODIMP CShellLink::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return HandleMenuMsg2(uMsg, wParam, lParam, NULL);
}

HRESULT CShellLink::_InitDropTarget()
{
    HWND hwndOwner;

    if (_pdtSrc)
        return S_OK;

    IUnknown_GetWindow(_punkSite, &hwndOwner);
    return _GetUIObject(hwndOwner, IID_IDropTarget, (void **)&_pdtSrc);
}

STDMETHODIMP CShellLink::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres = _InitDropTarget();
    if (FAILED(hres))
        return hres;

    _grfKeyStateLast = grfKeyState;
    return _pdtSrc->DragEnter(pdtobj, grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CShellLink::DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hres = _InitDropTarget();
    if (FAILED(hres))
        return hres;

    _grfKeyStateLast = grfKeyState;
    return _pdtSrc->DragOver(grfKeyState, pt, pdwEffect);
}

STDMETHODIMP CShellLink::DragLeave()
{
    HRESULT hres = _InitDropTarget();
    if (FAILED(hres))
        return hres;
    return _pdtSrc->DragLeave();
}

STDMETHODIMP CShellLink::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    HRESULT hr = _InitDropTarget();
    if (SUCCEEDED(hr))
    {
        HWND hwndOwner;
        IUnknown_GetWindow(_punkSite, &hwndOwner);

        _pdtSrc->DragLeave();   // leave from the un-resolved drop target.

        hr = ResolveLink(hwndOwner, 0, 0);    // start over
        if (hr == S_OK)
        {
            IDropTarget *pdtgtResolved;
            if (SUCCEEDED(_GetUIObject(hwndOwner, IID_IDropTarget, (void **)&pdtgtResolved)))
            {
                hr = _ShortNetTimeout();
                if (S_OK == hr)
                {
                    IUnknown_SetSite(pdtgtResolved, (IShellLink *)this);

                    SHSimulateDrop(pdtgtResolved, pdtobj, _grfKeyStateLast, &pt, pdwEffect);

                    IUnknown_SetSite(pdtgtResolved, NULL);
                }
                pdtgtResolved->Release();
            }
        }
        else if (FAILED(hr) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
        {
            TCHAR szLinkSrc[MAX_PATH];
            if (SHGetPathFromIDList(_pidl, szLinkSrc))
            {
                ShellMessageBox(HINST_THISDLL, hwndOwner,
                            MAKEINTRESOURCE(IDS_ENUMERR_PATHNOTFOUND),
                            MAKEINTRESOURCE(IDS_LINKERROR),
                            MB_OK | MB_ICONEXCLAMATION, NULL, szLinkSrc);
            }
        }
    }

    if (hr != S_OK)
        *pdwEffect = DROPEFFECT_NONE;   // make sure nothing happens (if we failed)

    return hr;
}

STDMETHODIMP CShellLink::GetInfoTip(DWORD dwFlags, WCHAR **ppwszTip)
{
    TCHAR szTip[INFOTIPSIZE];
    TCHAR szDesc[INFOTIPSIZE];

    szTip[0] = 0;

    if ((dwFlags & QITIPF_USENAME) && _pszCurFile)
    {
        SHFILEINFO sfi;
        if (SHGetFileInfo(_pszCurFile, 0, &sfi, SIZEOF(sfi), SHGFI_DISPLAYNAME | SHGFI_USEFILEATTRIBUTES))
        {
            StrCpyN(szTip, sfi.szDisplayName, ARRAYSIZE(szTip));
        }
    }
        
    GetDescription(szDesc, ARRAYSIZE(szDesc));
    if (!szDesc[0] && !(_sld.dwFlags & SLDF_HAS_DARWINID) && !(dwFlags & QITIPF_LINKNOTARGET))
    {
        if (dwFlags & QITIPF_LINKUSETARGET)
            SHMakeDescription(_pidl, -1, szDesc, ARRAYSIZE(szDesc));
        else
            _MakeDescription(_pidl, szDesc, ARRAYSIZE(szDesc));
    }

    if (szDesc[0])
    {
        if (szTip[0])
            StrCatBuff(szTip, TEXT("\n"), ARRAYSIZE(szTip));

        StrCatBuff(szTip, szDesc, ARRAYSIZE(szTip));
    }

    if (*szTip)
    {
        return SHStrDup(szTip, ppwszTip);
    }
    else
    {
        *ppwszTip = NULL;
        return S_FALSE;
    }
}

STDMETHODIMP CShellLink::GetInfoFlags(DWORD *pdwFlags)
{
    pdwFlags = 0;
    return E_NOTIMPL;
}

HRESULT CShellLink::_GetExtractIcon(REFIID riid, void **ppvOut)
{
    HRESULT hres;
    
    if (_pszIconLocation && _pszIconLocation[0])
    {
        TCHAR szPath[MAX_PATH];
        
        if (_pszIconLocation[0] == TEXT('.'))
        {
            TCHAR szBogusFile[MAX_PATH];
            
            // We allow people to set ".txt" for an icon path. In this case 
            // we cook up a simple pidl and use it to get to the IExtractIcon for 
            // whatever extension the user has specified.
            
            hres = SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, szBogusFile);
            if (SUCCEEDED(hres))
            {
                LPITEMIDLIST pidl;
                
                PathAppend(szBogusFile, TEXT("*"));
                lstrcatn(szBogusFile, _pszIconLocation, ARRAYSIZE(szBogusFile));
                
                pidl = SHSimpleIDListFromPath(szBogusFile);
                if (pidl)
                {
                    hres = SHGetUIObjectFromFullPIDL(pidl, NULL, IID_IExtractIcon, ppvOut);
                }
                else
                {
                    hres = E_OUTOFMEMORY;
                }
                
                ILFree(pidl);
            }
        }
        else if (_sld.iIcon == 0 &&
            _pidl && SHGetPathFromIDList(_pidl, szPath) &&
            lstrcmpi(szPath, _pszIconLocation) == 0)
        {
            hres = _GetUIObject(NULL, riid, ppvOut);
        }
        else
        {
            hres = SHCreateDefExtIcon(_pszIconLocation, _sld.iIcon,
                _sld.iIcon, GIL_PERINSTANCE, riid, ppvOut);
        }
    }
    else
    {
        hres = _GetUIObject(NULL, riid, ppvOut);
    }

    return hres;
}

HRESULT CShellLink::_InitExtractIcon()
{
    if (_pxi || _pxiA)
        return S_OK;

    if (_pxiA)
        return S_OK;

    HRESULT hres = _GetExtractIcon(IID_IExtractIconW, (void **)&_pxi);
    if (FAILED(hres))
        hres = _GetExtractIcon(IID_IExtractIconA, (void **)&_pxiA);
    return hres;
}

// IExtractIconW::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(UINT uFlags, LPWSTR pszIconFile, 
                                         UINT cchMax, int *piIndex, UINT *pwFlags)
{
    HRESULT hr = _InitExtractIcon();
    if (SUCCEEDED(hr))
    {
        if (_pxi)
        {
            hr = _pxi->GetIconLocation(uFlags, pszIconFile, cchMax, piIndex, pwFlags);
        }
        else if (_pxiA)
        {
            CHAR sz[MAX_PATH];
            hr = _pxiA->GetIconLocation(uFlags, sz, ARRAYSIZE(sz), piIndex, pwFlags);
            if (SUCCEEDED(hr) && hr != S_FALSE)
                SHAnsiToUnicode(sz, pszIconFile, cchMax);
        }
        if (SUCCEEDED(hr))
            _gilFlags = *pwFlags;
    }
    return hr;
}

// IExtractIconA::GetIconLocation
STDMETHODIMP CShellLink::GetIconLocation(UINT uFlags, LPSTR pszIconFile, UINT cchMax, int *piIndex, UINT *pwFlags)
{
    WCHAR szFile[MAX_PATH];
    HRESULT hr = GetIconLocation(uFlags, szFile, ARRAYSIZE(szFile), piIndex, pwFlags);
    if (SUCCEEDED(hr))
        SHUnicodeToAnsi(szFile, pszIconFile, cchMax);
    return hr;
}

// IExtractIconW::Extract
STDMETHODIMP CShellLink::Extract(LPCWSTR pszFile, UINT nIconIndex, 
                                 HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    HRESULT hres = _InitExtractIcon();
    if (SUCCEEDED(hres))
    {
        // GIL_PERCLASS, GIL_PERINSTANCE
        if ((_gilFlags & GIL_PERINSTANCE) || !(_gilFlags & GIL_PERCLASS))
        {
            hres = _ShortNetTimeout();
            if (FAILED(hres))
                return hres;
        }

        if (_pxi)
        {
            hres = _pxi->Extract(pszFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
        }
        else if (_pxiA)
        {
            CHAR sz[MAX_PATH];
            SHUnicodeToAnsi(pszFile, sz, ARRAYSIZE(sz));
            hres = _pxiA->Extract(sz, nIconIndex, phiconLarge, phiconSmall, nIconSize);
        }
    }
    return hres;
}

// IExtractIconA::Extract
STDMETHODIMP CShellLink::Extract(LPCSTR pszFile, UINT nIconIndex, HICON *phiconLarge, HICON *phiconSmall, UINT nIconSize)
{
    WCHAR szFile[MAX_PATH];
    SHAnsiToUnicode(pszFile, szFile, ARRAYSIZE(szFile));
    return Extract(szFile, nIconIndex, phiconLarge, phiconSmall, nIconSize);
}    

STDMETHODIMP CShellLink::AddDataBlock(void *pdb)
{
    _AddExtraDataSection((DATABLOCK_HEADER *)pdb);
    return S_OK;
}

STDMETHODIMP CShellLink::CopyDataBlock(DWORD dwSig, void **ppdb)
{
    DATABLOCK_HEADER *peh = (DATABLOCK_HEADER *)SHFindDataBlock(_pExtraData, dwSig);
    if (peh)
    {
        *ppdb = LocalAlloc(LPTR, peh->cbSize);
        if (*ppdb)
        {
            CopyMemory(*ppdb, peh, peh->cbSize);
            return S_OK;
        }
        return E_OUTOFMEMORY;
    }
    *ppdb = NULL;
    return E_FAIL;
}

STDMETHODIMP CShellLink::RemoveDataBlock(DWORD dwSig)
{
    _RemoveExtraDataSection(dwSig);
    return S_OK;
}

STDMETHODIMP CShellLink::GetFlags(DWORD *pdwFlags)
{
    *pdwFlags = _sld.dwFlags;
    return S_OK;
}

STDMETHODIMP CShellLink::SetFlags(DWORD dwFlags)
{
    if (dwFlags != _sld.dwFlags)
    {
        _bDirty = TRUE;
        _sld.dwFlags = dwFlags;
        return S_OK;
    }
    return S_FALSE;     // no change made
}

STDMETHODIMP CShellLink::GetPath(LPSTR pszFile, int cchMaxPath, WIN32_FIND_DATAA *pfd, DWORD fFlags)
{
    WCHAR szPath[MAX_PATH];
    WIN32_FIND_DATAW wfd;
    VDATEINPUTBUF(pszFile, CHAR, cchMaxPath);

    //Call the unicode version
    HRESULT hr = GetPath(szPath, ARRAYSIZE(szPath), &wfd, fFlags);

    if (pszFile)
    {
        SHUnicodeToAnsi(szPath, pszFile, cchMaxPath);
    }
    if (pfd)
    {
        if (szPath[0])
        {
            pfd->dwFileAttributes = wfd.dwFileAttributes;
            pfd->ftCreationTime   = wfd.ftCreationTime;
            pfd->ftLastAccessTime = wfd.ftLastAccessTime;
            pfd->ftLastWriteTime  = wfd.ftLastWriteTime;
            pfd->nFileSizeLow     = wfd.nFileSizeLow;
            pfd->nFileSizeHigh    = wfd.nFileSizeHigh;

            SHUnicodeToAnsi(wfd.cFileName, pfd->cFileName, ARRAYSIZE(pfd->cFileName));
        }
        else
            memset(pfd, 0, SIZEOF(*pfd));
    }
    return hr;
}

STDMETHODIMP CShellLink::SetPath(LPCSTR pszPath)
{
    WCHAR szPath[MAX_PATH];
    LPWSTR pszPathW;
    
    if (pszPath)
    {
        SHAnsiToUnicode(pszPath, szPath, ARRAYSIZE(szPath));
        pszPathW = szPath;
    }
    else
        pszPathW = NULL;

    return SetPath(pszPathW);
}

// Thunk from CTracker, necessary for the private ISLTracker interface
// (used for testing)

HRESULT CShellLink::ResolveCallback(IShellLink *psl, HWND hwnd, DWORD fFlags, DWORD dwTracker)
{
    CShellLink *plink = (CShellLink *)psl;
    return plink->ResolveLink(hwnd, fFlags, dwTracker);
}

STDAPI CShellLink_CreateInstance(IUnknown *punkOuter, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;

    HRESULT hres;
    CShellLink *pshlink = new CShellLink();
    if (pshlink)
    {
        hres = pshlink->QueryInterface(riid, ppvOut);
        pshlink->Release();
    }
    else
        hres = E_OUTOFMEMORY;
    return hres;
}

STDMETHODIMP CShellLink::Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellLink::InitNew(void)
{
    _ResetPersistData();        // clear out our state
    return S_OK;
}

STDMETHODIMP CShellLink::Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog)
{
    HRESULT hres = E_FAIL;

    _ResetPersistData();        // clear out our state

    VARIANT var;
    TCHAR szPath[MAX_PATH];

    *szPath = 0;

    // TBD: Shortcut key, Run, Icon, Working Dir, Description
    
    // Target Special Folder
    var.vt = VT_BSTR;
    var.bstrVal = NULL;

    hres = pPropBag->Read(L"TargetSpecialFolder", &var, NULL);

    if (SUCCEEDED(hres))
    {
        int iCSIDL = 0;

        if (!StrToIntExW(var.bstrVal, STIF_SUPPORT_HEX, &iCSIDL))
            hres = E_FAIL;

        if (SUCCEEDED(hres))
        {
            if (!SHGetSpecialFolderPath(NULL, szPath, iCSIDL, FALSE))
                hres = E_FAIL;
        }
    }
    else
        hres = S_FALSE;

    if (SUCCEEDED(hres))
    {
        // Target
        VariantClear(&var);

        // we use a diff hres (hres2) so that we can specify only a "TargetSpecialFolder"
        // without a "Target".  But if hres2 SUCCEEDED then we go back to our hres (not hres2)
        // cause this means we tried to specify a Target and something went wrong and we want
        // to report this.
        HRESULT hres2 = pPropBag->Read(L"Target", &var, NULL);

        // Do we have a path in Target?
        if (SUCCEEDED(hres2))
        {
            // Yes 
            TCHAR szTempPath[MAX_PATH];
            
            SHUnicodeToTChar(var.bstrVal, szTempPath, ARRAYSIZE(szTempPath));

            // Do we need to append it to the Special path?
            if (0 != *szPath)
            {
                // Yes
                if (!PathAppend(szPath, szTempPath))
                    hres = E_FAIL;
            }
            else
            {
                // No, there is no special path
                // Maybe we have an Env Var to expand
                if (0 == SHExpandEnvironmentStrings(szTempPath, szPath, ARRAYSIZE(szPath)))
                    hres = E_FAIL;
            }
        }
        else
        {
            // No, we MUST have a Special path
            if (0 == *szPath)
                hres = E_FAIL;
            
        }
        if (SUCCEEDED(hres))
            hres = SetPIDLPath(NULL, szPath, NULL) ? S_OK : E_FAIL;
    }

    VariantClear(&var);

    return hres;
}

STDMETHODIMP CShellLink::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    if (guidService == SID_LinkSite)
        return QueryInterface(riid, ppv);
    return IUnknown_QueryService(_punkSite, guidService, riid, ppv);
}

const FULLPROPSPEC c_rgProps[] =
{
    { PSGUID_SUMMARYINFORMATION, {  PRSPEC_PROPID, PIDSI_COMMENTS } },
};

STDMETHODIMP CShellLink::Init(ULONG grfFlags, ULONG cAttributes,
                              const FULLPROPSPEC *rgAttributes, ULONG *pFlags)
{
    *pFlags = 0;

    if (grfFlags & IFILTER_INIT_APPLY_INDEX_ATTRIBUTES)
    {
        _iChunkIndex = 0;    // start at the beginning
    }
    else
    {
        _iChunkIndex = ARRAYSIZE(c_rgProps);    // indicate EOF
    }
    _iValueIndex = 0;
    return S_OK;
}
        
STDMETHODIMP CShellLink::GetChunk(STAT_CHUNK *pStat)
{
    HRESULT hr = S_OK;
    if (_iChunkIndex < ARRAYSIZE(c_rgProps))
    {
        pStat->idChunk          = _iChunkIndex + 1;
        pStat->idChunkSource    = _iChunkIndex + 1;
        pStat->breakType        = CHUNK_EOP;
        pStat->flags            = CHUNK_VALUE;
        pStat->locale           = GetSystemDefaultLCID();
        pStat->attribute        = c_rgProps[_iChunkIndex];
        pStat->cwcStartSource   = 0;
        pStat->cwcLenSource     = 0;

        _iValueIndex = 0;
        _iChunkIndex++;
    }
    else
        hr = FILTER_E_END_OF_CHUNKS;
    return hr;
}

STDMETHODIMP CShellLink::GetText(ULONG *pcwcBuffer, WCHAR *awcBuffer)
{
    return FILTER_E_NO_TEXT;
}
        
STDMETHODIMP CShellLink::GetValue(PROPVARIANT **ppPropValue)
{
    HRESULT hr;
    if ((_iChunkIndex <= ARRAYSIZE(c_rgProps)) && (_iValueIndex < 1))
    {
        *ppPropValue = (PROPVARIANT*)CoTaskMemAlloc(sizeof(PROPVARIANT));
        if (*ppPropValue)
        {
            (*ppPropValue)->vt = VT_BSTR;

            if (_pszName)
            {
                (*ppPropValue)->bstrVal = SysAllocStringT(_pszName);
            }
            else
            {
                // since _pszName is null, return an empty bstr
                (*ppPropValue)->bstrVal = SysAllocStringT(TEXT(""));
            }

            if ((*ppPropValue)->bstrVal)
            {
                hr = S_OK;
            }
            else
            {
                CoTaskMemFree(*ppPropValue);
                *ppPropValue = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
        else
            hr = E_OUTOFMEMORY;

        _iValueIndex++;
    }
    else
        hr = FILTER_E_NO_MORE_VALUES;
    return hr;
}
        
STDMETHODIMP CShellLink::BindRegion(FILTERREGION origPos, REFIID riid, void **ppunk)
{
    *ppunk = NULL;
    return E_NOTIMPL;
}
        
