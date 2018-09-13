//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 
//
// File: treewkcb.cpp
//
// This file contains the implementation of CBaseTreeWalkerCB, the base class for
// the shell Tree Walker Callback objects who implement IShellTreeWalkerCallBack.
// And ...
// CFolderSizeTreeWalkerCB, which walks the File System tree on a folder and computes the size
// of the folder
// CLinkResolveTreeWalkerCB, which search through the File System to resolve a lost link
// History:
//         12-9-97  by dli implemented CFolderSizeTreeWalkerCB
//         12-17-97 by dli implemendted CBaseTreeWalkerCB, added CLinkResolveTreeWalkerCB
//------------------------------------------------------------------------
#include "shellprv.h"
#include "treewkcb.h"
#include "resolve.h"


#define TF_TREEWKCB  0

CBaseTreeWalkerCB::CBaseTreeWalkerCB(): _cRef(1)
{
    ASSERT(_pstw == NULL);
    _hrInit = SHCoInitialize();
}

CBaseTreeWalkerCB::~CBaseTreeWalkerCB()
{
    ATOMICRELEASE(_pstw);
    SHCoUninitialize(_hrInit);
}

HRESULT CBaseTreeWalkerCB::Initialize()
{
    return CoCreateInstance(CLSID_CShellTreeWalker, NULL, CLSCTX_INPROC_SERVER, IID_IShellTreeWalker, (LPVOID *)&_pstw);
}   

HRESULT CBaseTreeWalkerCB::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    ASSERT(ppvObj != NULL);

    static const QITAB qit[] = {
        QITABENT(CBaseTreeWalkerCB, IShellTreeWalkerCallBack),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}


ULONG CBaseTreeWalkerCB::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CBaseTreeWalkerCB::Release()
{
    _cRef--;
    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CBaseTreeWalkerCB::FoundFile(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    return E_NOTIMPL;
}

HRESULT CBaseTreeWalkerCB::EnterFolder(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    return E_NOTIMPL;
}

HRESULT CBaseTreeWalkerCB::LeaveFolder(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws)
{
    return E_NOTIMPL;
}

HRESULT CBaseTreeWalkerCB::HandleError(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, HRESULT ErrorCode)
{
    return E_NOTIMPL;
}



//
// Folder size computation tree walker callback class
//
class CFolderSizeTreeWalkerCB : public CBaseTreeWalkerCB
{
    friend HRESULT _stdcall FolderSize(LPCTSTR pszDir, FOLDERCONTENTSINFO * pfci);
public:
    CFolderSizeTreeWalkerCB(FOLDERCONTENTSINFO * pfci);

    // *** IShellTreeWalkerCallBack overridden methods ***
    virtual STDMETHODIMP FoundFile(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    virtual STDMETHODIMP EnterFolder(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    virtual STDMETHODIMP Initialize();

protected:
    FOLDERCONTENTSINFO * _pfci;
    TREEWALKERSTATS _twsInitial;
}; 

CFolderSizeTreeWalkerCB::CFolderSizeTreeWalkerCB(FOLDERCONTENTSINFO * pfci): _pfci(pfci)
{
}

//
// CFolderSizeTreeWalkerCB::Initialize
//
HRESULT CFolderSizeTreeWalkerCB::Initialize()
{
    HRESULT hr;

    // we call the base class which will create the _pstw, and
    // if that succeeds then we initialize it to have the starting
    // size values of _pcfi. 
    hr = CBaseTreeWalkerCB::Initialize();

    if (FAILED(hr))
    {
        return hr;
    }

    ASSERT(_pfci);

    // set the starting values for the twsInitial so we can have cumulative results
    _twsInitial.nFiles = _pfci->cFiles;
    _twsInitial.nFolders = _pfci->cFolders;
    _twsInitial.ulTotalSize = _pfci->cbSize;
    _twsInitial.ulActualSize = _pfci->cbActualSize;

    return S_OK;
}


//
// CFolderSizeTreeWalkerCB::FoundFile 
//
HRESULT CFolderSizeTreeWalkerCB::FoundFile(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    if (!_pfci->bContinue)
    {
        // we have been signaled to stop, so return failure
        return E_FAIL;
    }

    _pfci->cbSize = _twsInitial.ulTotalSize + ptws->ulTotalSize;
    _pfci->cbActualSize = _twsInitial.ulActualSize + ptws->ulActualSize;
    _pfci->cFiles = _twsInitial.nFiles + ptws->nFiles;
    
    return S_OK;
}

//
// CFolderSizeTreeWalkerCB::EnterFolder 
//
HRESULT CFolderSizeTreeWalkerCB::EnterFolder(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    if (!_pfci->bContinue)
    {
        // we have been signaled to stop, so return failure
        return E_FAIL;
    }

    _pfci->cFolders = _twsInitial.nFolders + ptws->nFolders;
    
    return S_OK;
}

//
//  Main function for folder size computation, friend of the CFolderSizeTreeWalkerCB class
//
STDAPI FolderSize(LPCTSTR pszDir, FOLDERCONTENTSINFO * pfci)
{
    HRESULT hres = E_FAIL;
    CFolderSizeTreeWalkerCB * pfstwcb = new CFolderSizeTreeWalkerCB(pfci);
    if (pfstwcb)
    {
        if (SUCCEEDED(pfstwcb->Initialize()))
        {
            LPCWSTR pwszDir;
            ASSERT(IS_VALID_CODE_PTR(pfstwcb->_pstw, IShellTreeWalker));
#ifdef UNICODE
            pwszDir = pszDir;
#else
            WCHAR wszDir[MAX_PATH];
            SHTCharToUnicode(pszDir, wszDir, ARRAYSIZE(wszDir));
            pwszDir = wszDir;
#endif
            hres = pfstwcb->_pstw->WalkTree(WT_NOTIFYFOLDERENTER, pwszDir, NULL, 0, SAFECAST(pfstwcb, IShellTreeWalkerCallBack *));
        }
        pfstwcb->Release();
    }
    
    return hres;
}

//
//  Link Resolution tree walker callback class
//
class CLinkResolveTreeWalkerCB : public CBaseTreeWalkerCB
{
    friend VOID DoDownLevelSearch(RESOLVE_SEARCH_DATA *prs, TCHAR szFolderPath[], int iStopScore);
public:
    CLinkResolveTreeWalkerCB(RESOLVE_SEARCH_DATA *prs, int iStopScore);

    // *** IShellTreeWalkerCallBack overridden methods ***
    virtual STDMETHODIMP FoundFile(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);
    virtual STDMETHODIMP EnterFolder(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd);

    BOOL SearchInFolder(LPCTSTR pszFolder, int cLevels);

protected:
    RESOLVE_SEARCH_DATA * _prs;
    int    _iStopScore;
    DWORD  _dwSearchFlags;
    int    _iFolderBonus;
    
    WCHAR  _wszSearchSpec[64];
    LPCWSTR _pwszSearchSpec;
    
    int     _ScoreFindData(const WIN32_FIND_DATA *pfd);
    HRESULT _ProcessFoundFile(LPCTSTR pszPath, WIN32_FIND_DATAW * pwfdw);
}; 

CLinkResolveTreeWalkerCB::CLinkResolveTreeWalkerCB(RESOLVE_SEARCH_DATA *prs, int iStopScore): _prs(prs), _iStopScore(iStopScore)
{
    ASSERT(_pwszSearchSpec == NULL);

    // Note: We only search files with the same extension, this saves us a lot
    // of useless work and from the humiliation of coming up with a ridiculous answer
    _dwSearchFlags = WT_NOTIFYFOLDERENTER | WT_EXCLUDEWALKROOT;
    if (_prs->dwMatch & FILE_ATTRIBUTE_DIRECTORY)
        _dwSearchFlags |= WT_FOLDERONLY;
    else
    {
        // Note that this does the right thing if the file has no extension
        LPTSTR pszExt = PathFindExtension(prs->pfd->cFileName);
        _wszSearchSpec[0] = L'*';
        SHTCharToUnicode(pszExt, &_wszSearchSpec[1], ARRAYSIZE(_wszSearchSpec) - 1);
        _pwszSearchSpec = _wszSearchSpec;
    }

}


//
// Compare two FileTime structures.  First, see if they're really equal
// (using CompareFileTime).  If not, see if one has 10ms granularity,
// and if the other rounds down to the same value.  This is done
// to handle the case where a file is moved from NTFS to FAT;
// FAT file tiems are 10ms granularity, while NTFS is 100ns.  When
// an NTFS file is moved to FAT, its time is rounded down.
//

#define NTFS_UNITS_PER_FAT_UNIT  100000

BOOL IsEqualFileTimesWithTruncation( const FILETIME *pft1, const FILETIME *pft2 )
{
    ULARGE_INTEGER uli1, uli2;
    ULARGE_INTEGER *puliFAT, *puliNTFS;
    FILETIME ftFAT, ftNTFS;

    if( 0 == CompareFileTime( pft1, pft2 ))
        return( TRUE );

    uli1.LowPart  = pft1->dwLowDateTime;
    uli1.HighPart = pft1->dwHighDateTime;

    uli2.LowPart  = pft2->dwLowDateTime;
    uli2.HighPart = pft2->dwHighDateTime;

    // Is one of the times 10ms granular?

    if( 0 == (uli1.QuadPart % NTFS_UNITS_PER_FAT_UNIT) )
    {
        puliFAT = &uli1;
        puliNTFS = &uli2;
    }
    else if( 0 == (uli2.QuadPart % NTFS_UNITS_PER_FAT_UNIT) )
    {
        puliFAT = &uli2;
        puliNTFS = &uli1;
    }
    else
    {
        // Neither time appears to be FAT, so they're
        // really different.
        return( FALSE );
    }

    // If uliNTFS is already 10ms granular, then again the two times
    // are really different.

    if( 0 == (puliNTFS->QuadPart % NTFS_UNITS_PER_FAT_UNIT) )
        return( FALSE );

    // Now see if the FAT time is the same as the NTFS time
    // when the latter is rounded down to the nearest 10ms.

    puliNTFS->QuadPart = (puliNTFS->QuadPart / NTFS_UNITS_PER_FAT_UNIT) * NTFS_UNITS_PER_FAT_UNIT;

    ftNTFS.dwLowDateTime = puliNTFS->LowPart;
    ftNTFS.dwHighDateTime = puliNTFS->HighPart;
    ftFAT.dwLowDateTime = puliFAT->LowPart;
    ftFAT.dwHighDateTime = puliFAT->HighPart;

    return( 0 == CompareFileTime( &ftFAT, &ftNTFS ));

}


//
// compute a weighted score for a given find
//
int CLinkResolveTreeWalkerCB::_ScoreFindData(const WIN32_FIND_DATA *pfd)
{
    int iScore = 0;
    BOOL bSameName, bSameCreateDate, bSameWriteTime, bSameExt, bHasCreateDate;

    bSameName = lstrcmpi(_prs->pfd->cFileName, pfd->cFileName) == 0;

    bSameExt = lstrcmpi(PathFindExtension(_prs->pfd->cFileName), PathFindExtension(pfd->cFileName)) == 0;

    bHasCreateDate = !IsNullTime(&pfd->ftCreationTime);

    bSameCreateDate = bHasCreateDate &&
                      IsEqualFileTimesWithTruncation(&pfd->ftCreationTime, &_prs->pfd->ftCreationTime);

    bSameWriteTime  = !IsNullTime(&pfd->ftLastWriteTime) &&
                      IsEqualFileTimesWithTruncation(&pfd->ftLastWriteTime, &_prs->pfd->ftLastWriteTime);

    if (bSameName || bSameCreateDate)
    {
        if (bSameName)
            iScore += bHasCreateDate ? 16 : 32;

        if (bSameCreateDate)
        {
            iScore += 32;

            if (bSameExt)
                iScore += 8;
        }

        if (bSameWriteTime)
            iScore += 8;

        if (pfd->nFileSizeLow == _prs->pfd->nFileSizeLow)
            iScore += 4;

        // if it is in the same folder as the original give it a slight bonus
        iScore += _iFolderBonus;
    }
    else
    {
        // doesn't have create date, apply different rules

        if (bSameExt)
            iScore += 8;

        if (bSameWriteTime)
            iScore += 8;

        if (pfd->nFileSizeLow == _prs->pfd->nFileSizeLow)
            iScore += 4;
    }

    return iScore;
}

//
//  Helper function for both EnterFolder and FoundFile
//
HRESULT CLinkResolveTreeWalkerCB::_ProcessFoundFile(LPCTSTR pszPath, WIN32_FIND_DATAW * pwfdw)
{
    HRESULT hres = S_OK;
#ifdef UNICODE
    WIN32_FIND_DATA * pwfd = pwfdw;
#else
    WIN32_FIND_DATA wfd;
    WIN32_FIND_DATA * pwfd = &wfd;

    hmemcpy(&wfd, pwfdw, SIZEOF(WIN32_FIND_DATA));
    SHUnicodeToTChar(pwfdw->cFileName, wfd.cFileName, ARRAYSIZE(wfd.cFileName));
    SHUnicodeToTChar(pwfdw->cAlternateFileName, wfd.cAlternateFileName, ARRAYSIZE(wfd.cAlternateFileName));
    pwfd = &wfd;
#endif

    if (!PathIsLnk(pwfd->cFileName))
    {
        // both are files or folders, see how it scores
        int iScore = _ScoreFindData(pwfd);

        if (iScore > _prs->iScore)
        {
            _prs->fdFound = *pwfd;

            TraceMsg(TF_TREEWKCB, "Better match found %s, %d", pszPath, iScore);

            // store the score and fully qualified path
            _prs->iScore = iScore;
            lstrcpyn(_prs->fdFound.cFileName, pszPath, ARRAYSIZE(_prs->fdFound.cFileName));
        }
    }

    if ((_prs->iScore >= _iStopScore) || (GetTickCount() >= _prs->dwTimeLimit))
    {
        _prs->bContinue = FALSE;
        hres = E_FAIL;
    }
    
    return hres;
}

//
// IShellTreeWalkerCallBack::FoundFile
//
HRESULT CLinkResolveTreeWalkerCB::FoundFile(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    ASSERT(IS_VALID_STRING_PTRW(pwszPath, -1));
    ASSERT(pwfd);

    //
    //  Respond quickly to the Cancel button.
    //
    if (!_prs->bContinue)
    {
        return E_FAIL;
    }

    // We should've excluded files if we're looking for a folder
    ASSERT(!(_prs->dwMatch & FILE_ATTRIBUTE_DIRECTORY));

    LPCTSTR ptszPath;
#ifdef UNICODE
    ptszPath = pwszPath;
#else
    TCHAR szPath[MAX_PATH];
    SHUnicodeToTChar(pwszPath, szPath, ARRAYSIZE(szPath));
    ptszPath = szPath;
#endif

    return _ProcessFoundFile(ptszPath, pwfd);
}

//
// IShellTreeWalkerCallBack::EnterFolder
//
HRESULT CLinkResolveTreeWalkerCB::EnterFolder(LPCWSTR pwszPath, LPTREEWALKERSTATS ptws, WIN32_FIND_DATAW * pwfd)
{
    HRESULT hres = S_OK;
    ASSERT(IS_VALID_STRING_PTRW(pwszPath, -1));
    ASSERT(pwfd);

    //
    //  Respond quickly to the Cancel button.
    //
    if (!_prs->bContinue)
    {
        return E_FAIL;
    }

    LPCTSTR ptszPath;
#ifdef UNICODE
    ptszPath = pwszPath;
#else
    TCHAR szPath[MAX_PATH];
    SHUnicodeToTChar(pwszPath, szPath, ARRAYSIZE(szPath));
    ptszPath = szPath;
#endif

    // Once we enter a directory, we lose the "we are still in the starting
    // folder" bonus.
    _iFolderBonus = 0;

    // If we're about to enter a directory we've already looked in,
    // or if this is a dead directory, then skip it.
    if (PathIsPrefix(ptszPath, _prs->pszSearchOrigin) || IsFileInBitBucket(ptszPath))
        return S_FALSE;

    // If our target was a folder, treat this folder as a file found
    if (_prs->dwMatch & FILE_ATTRIBUTE_DIRECTORY)
    {
        hres = _ProcessFoundFile(ptszPath, pwfd);
    }
    return hres;
}

//
// Wrapper around WalkTree
//
BOOL CLinkResolveTreeWalkerCB::SearchInFolder(LPCTSTR pszFolder, int cLevels)
{
    LPCWSTR pwszFolder;
#ifdef UNICODE
    pwszFolder = pszFolder;
#else
    WCHAR wszFolder[MAX_PATH];
    SHTCharToUnicode(pszFolder, wszFolder, ARRAYSIZE(wszFolder));
    pwszFolder = wszFolder;
#endif

    int iMaxDepth = 0;

    // cLevels == -1 means inifinite depth
    if (cLevels != -1)
    {
        _dwSearchFlags |= WT_MAXDEPTH;
        iMaxDepth = cLevels;
    }
    else
        _dwSearchFlags &= ~WT_MAXDEPTH;

    // Our folder bonus code lies on the fact that files in the
    // starting folder come before anything else.
    ASSERT(!(_dwSearchFlags & WT_FOLDERFIRST));

#ifdef DEBUG
    // Test the QueryInterface code path by QI'ing ourselves for IUnknown and
    // making sure we get ourselves back.  We have to use IUnknown because
    // anything else could get usurped into a tear-off by QIStub.
    IUnknown *punkSelf;
    EVAL(SUCCEEDED(this->QueryInterface(IID_IShellTreeWalkerCallBack, (LPVOID *)&punkSelf)));
    ASSERT(punkSelf == SAFECAST(this, IUnknown *));
    punkSelf->Release();
#endif

    ASSERT(IS_VALID_CODE_PTR(_pstw, IShellTreeWalker));
    _pstw->WalkTree(_dwSearchFlags, pwszFolder, _pwszSearchSpec, iMaxDepth, SAFECAST(this, IShellTreeWalkerCallBack *));
    _iFolderBonus = 0; // You only get one chance at the folder bonus
    return _prs->bContinue;
}

//
// Main search function for link resolution, the result will be in prs->fdFound.cFileName
//
VOID DoDownLevelSearch(RESOLVE_SEARCH_DATA *prs, LPTSTR pszFolderPath, int iStopScore)
{
    //
    // Skip this search if it's restricted
    //

    if (SHRestricted(REST_NORESOLVESEARCH)
        ||
        SLR_NOSEARCH & prs->uFlags )
    {
        return;
    }

    CLinkResolveTreeWalkerCB * plrtwcb = new CLinkResolveTreeWalkerCB(prs, iStopScore);
    if (plrtwcb)
    {
        if (SUCCEEDED(plrtwcb->Initialize()))
        {
            int cUp = LNKTRACK_HINTED_UPLEVELS;
            LPITEMIDLIST pidl;
            BOOL bSearchOrigin = TRUE;
            TCHAR szRealSearchOrigin[MAX_PATH];

            //
            // search up from old location
            //

            // In the olden days pszSearchOriginFirst was verified to be a valid directory
            // (ie it returned TRUE to PathIsDirectory) and DoDownLevelSearch was never called
            // if this was not true.  Alas, this is no more.  Why not search the desktop and
            // fixed drives anyway?  In the interest of saving some time the check that used
            // to be in FindInFolder which caused an early out is now here instead.  The rub
            // is that we only skip the downlevel search of the original volume instead of
            // skipping the entire link resolution phase.
            lstrcpy(szRealSearchOrigin, prs->pszSearchOriginFirst);
            while (!PathIsDirectory(szRealSearchOrigin))
            {
                if (PathIsRoot(szRealSearchOrigin) || !PathRemoveFileSpec(szRealSearchOrigin))
                {
                    DebugMsg(DM_TRACE, TEXT("root path does not exists %s"), szRealSearchOrigin);
                    bSearchOrigin = FALSE;
                    break;
                }
            }

            if ( bSearchOrigin )
            {
                lstrcpy(pszFolderPath, szRealSearchOrigin);
                prs->pszSearchOrigin = szRealSearchOrigin;

                // Files found in the starting folder get a slight bonus.
                // _iFolderBonus is set to zero by
                // CLinkResolveTreeWalkerCB::EnterFolder when we leave
                // the starting folder and enter a new one.
                plrtwcb->_iFolderBonus = 2;

                while (cUp-- != 0 && plrtwcb->SearchInFolder(pszFolderPath, LNKTRACK_HINTED_DOWNLEVELS))
                {
                    if (PathIsRoot(pszFolderPath) || !PathRemoveFileSpec(pszFolderPath))
                        break;
                }
            }

            if (prs->bContinue)
            {
                //
                // search down from desktop
                //
                HRESULT hr = SHGetSpecialFolderLocation(NULL, CSIDL_DESKTOP, &pidl);
                if (hr == NOERROR)
                {
                    if (SHGetPathFromIDList(pidl, pszFolderPath))
                    {
                        prs->pszSearchOrigin = pszFolderPath;
                        plrtwcb->SearchInFolder(pszFolderPath, LNKTRACK_DESKTOP_DOWNLEVELS);

                    }
                    ILFree(pidl);
                }
            }

            if (prs->bContinue)
            {
                //
                // search down from root of fixed drives
                //
                TCHAR atch[4];

                // BUGBUG:  Is it valid to assume that no fixed drive can have a letter
                // less than c:?  I say no, I remember a PC98 machine that had b: as
                // it's hard drive letter.  Research the effects of making this start
                // at a: (or using GetLogicalDriveStrings?)
                lstrcpy(atch, TEXT("A:\\"));
                prs->pszSearchOrigin = atch;

                for (; prs->bContinue && atch[0] <= TEXT('Z'); atch[0]++)
                {
                    if (GetDriveType(atch) == DRIVE_FIXED)
                    {
                        lstrcpy(pszFolderPath, atch);
                        plrtwcb->SearchInFolder(pszFolderPath, LNKTRACK_ROOT_DOWNLEVELS);
                    }
                }
            }

            if (prs->bContinue && bSearchOrigin)
            {
                //
                // resume search of last volume (should do an exclude list)
                //

                lstrcpy(pszFolderPath, szRealSearchOrigin);
                prs->pszSearchOrigin = szRealSearchOrigin;

                while (plrtwcb->SearchInFolder(pszFolderPath, -1))
                {
                    if (PathIsRoot(pszFolderPath) || !PathRemoveFileSpec(pszFolderPath))
                        break;
                }
            }
        }
        plrtwcb->Release();
    }
}
